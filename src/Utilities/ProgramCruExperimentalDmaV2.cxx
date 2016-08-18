/// \file ProgramCruExperimentalDmaV2.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Based on https://gitlab.cern.ch/alice-cru/pciedma_eval

#include "Utilities/Program.h"
#include <iostream>
#include <iomanip>
#include <condition_variable>
#include <thread>
#include <queue>
#include <future>
#include <chrono>
#include <pda.h>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <ctype.h>
#include <boost/scoped_ptr.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
#include "RorcException.h"
#include "RorcDevice.h"
#include "MemoryMappedFile.h"
#include "Cru/CruRegisterIndex.h"
#include "Cru/CruFifoTable.h"
#include "Cru/CruRegisterIndex.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDmaBuffer.h"

/// Use busy wait instead of condition variable (c.v. impl incomplete, is very slow)
#define USE_BUSY_INTERRUPT_WAIT

/// Use asynchronous readout. Not the greatest implementation: slower, lots of readout errors...
//#define USE_ASYNC_READOUT

namespace b = boost;
namespace bfs = boost::filesystem;
namespace Register = AliceO2::Rorc::CruRegisterIndex;
using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

namespace {

constexpr std::chrono::milliseconds DISPLAY_INTERVAL(10);

constexpr int FIFO_ENTRIES = 4;

/// DMA page length in bytes
constexpr int DMA_PAGE_SIZE = 8 * 1024;

/// DMA page length in 32-bit words
constexpr int DMA_PAGE_SIZE_32 = DMA_PAGE_SIZE / 4;

constexpr int NUM_OF_BUFFERS = 32;

constexpr int NUM_PAGES = FIFO_ENTRIES * NUM_OF_BUFFERS;

/// Offset in bytes from start of status table to data buffer
constexpr size_t DATA_OFFSET = 0x200;

/// Two 2MiB hugepage. Should be enough...
constexpr size_t DMA_BUFFER_PAGES_SIZE = 4l * 1024l * 1024l;

constexpr uint32_t BUFFER_DEFAULT_VALUE = 0xCcccCccc;

/// PDA DMA buffer index for the pages buffer
constexpr int BUFFER_INDEX_PAGES = 0;

const bfs::path DMA_BUFFER_PAGES_PATH = "/mnt/hugetlbfs/rorc-cru-experimental-dma-pages-v2";

/// Fields: Time(hour:minute:second), i, Counter, Errors, Status, °C
const std::string PROGRESS_FORMAT_HEADER("  %-8s   %-12s  %-12s  %-10s  %-10.1f");

/// Fields: Time(hour:minute:second), i, Counter, Errors, Status, °C
const std::string PROGRESS_FORMAT("  %02s:%02s:%02s   %-12s  %-12s  %-10s  %-10.1f");

/// Default number of pages
constexpr int PAGES_DEFAULT = 1500;

constexpr double MAX_TEMPERATURE = 45.0;

/// Minimum random pause interval in milliseconds
constexpr int NEXT_PAUSE_MIN = 10;
/// Maximum random pause interval in milliseconds
constexpr int NEXT_PAUSE_MAX = 2000;
/// Minimum random pause in milliseconds
constexpr int PAUSE_LENGTH_MIN = 1;
/// Maximum random pause in milliseconds
constexpr int PAUSE_LENGTH_MAX = 500;

const std::string RESET_SWITCH = "reset";
const std::string TO_FILE_SWITCH = "tofile";
const std::string PAGES_SWITCH = "pages";
const std::string INTERRUPT_SWITCH = "interrupts";
const std::string DISPLAY_FIFO_SWITCH = "showfifo";
const std::string RANDOM_PAUSES_SWITCH = "rand-pauses";

const std::string READOUT_ERRORS_PATH = "readout_errors.txt";
const std::string READOUT_DATA_PATH = "readout_data.txt";
const std::string READOUT_LOG_FORMAT = "readout_log_%d.txt";

template <typename Index>
class LoopingIndex
{
  public:
    LoopingIndex(Index max, Index start = 0) : max(max), i(start)
    {
    }

    Index get() const
    {
      return i;
    }

    void increment()
    {
      i++;
      if (i >= max) {
        i = 0;
      }
    }

  private:
    Index max;
    Index i;
};

/// Test class for PCI interrupts
class PdaIsr
{
  public:

    PdaIsr(PciDevice* pciDevice)
        : mPciDevice(pciDevice)
    {
      cout << "\nREGISTERING ISR\n";
      if (PciDevice_registerISR(pciDevice, PdaIsr::serviceRoutine, nullptr) != PDA_SUCCESS) {
        BOOST_THROW_EXCEPTION(CruException() << errinfo_rorc_error_message("Failed to register ISR"));
      }
    }

    ~PdaIsr()
    {
      cout << "\nDE-REGISTERING ISR\n";
      PciDevice_killISR(mPciDevice);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cv_status waitOnInterrupt(const std::chrono::milliseconds& timeout)
    {
#ifdef USE_BUSY_INTERRUPT_WAIT
      auto start = std::chrono::high_resolution_clock::now();

      int i = 0;
      while (((std::chrono::high_resolution_clock::now() - start) < timeout) && i < 10000000) {
        i++;
//        bool expectedValue = true;
//        bool newValue = false;
//        if (mInterruptFlag.compare_exchange_strong(expectedValue, newValue) == true) {
        if (mInterruptFlag.load() == true) {
          return std::cv_status::no_timeout;
        }
      }
      return std::cv_status::timeout;
#else
      // Wait until the interrupt flag is true, and atomically swap its value to false
      std::unique_lock<decltype(mMutex)> lock(mMutex);
      return mConditionVariable.wait_for(lock, timeout);
#endif
    }

  private:

#ifdef USE_BUSY_INTERRUPT_WAIT
    static std::atomic<bool> mInterruptFlag;
#else
    static std::mutex mMutex;
    static std::condition_variable mConditionVariable;
#endif
    PciDevice* mPciDevice;

    static uint64_t serviceRoutine(uint32_t, const void*)
    {
#ifdef USE_BUSY_INTERRUPT_WAIT
      mInterruptFlag = true;
#else
      //cout << "GOT INTERRUPT: " << sequenceNumber << endl;
      std::unique_lock<decltype(mMutex)> lock(mMutex);
      mConditionVariable.notify_all();
#endif
      return 0;
    }
};
#ifdef USE_BUSY_INTERRUPT_WAIT
std::atomic<bool> PdaIsr::mInterruptFlag;
#else
std::mutex PdaIsr::mMutex;
std::condition_variable PdaIsr::mConditionVariable;
#endif

/// Manages a temperature monitor thread
class TemperatureMonitor {
  public:

    /// Start monitoring
    /// \param temperatureRegister The register to read the temperature data from.
    ///   The object using this must guarantee it will be accessible until stop() is called or this object is destroyed.
    void start(const volatile uint32_t* temperatureRegister)
    {
      mTemperatureRegister = temperatureRegister;
      mThread = std::thread(std::function<void(TemperatureMonitor*)>(TemperatureMonitor::function), this);
    }

    void stop()
    {
      mStopFlag = true;
      try {
        if (mThread.joinable()) {
          mThread.join();
        }
      } catch (const std::exception& e) {
        cout << "Failed to join thread: " << boost::diagnostic_information(e) << endl;
      }
    }

    ~TemperatureMonitor()
    {
      stop();
    }

    bool isValid() const
    {
      return mValidFlag.load();
    }

    bool isMaxExceeded() const
    {
      return mMaxExceeded.load();
    }

    double getTemperature() const
    {
      return mTemperature.load();
    }

  private:

    /// Thread object
    std::thread mThread;

    /// Flag to indicate max temperature was exceeded
    std::atomic<bool> mMaxExceeded;

    /// Variable to communicate temperature value
    std::atomic<double> mTemperature;

    /// Flag for valid temperature value
    std::atomic<bool> mValidFlag;

    /// Flag to stop the thread
    std::atomic<bool> mStopFlag;

    /// Register to read temperature value from
    const volatile uint32_t* mTemperatureRegister;

    // Thread function to monitor temperature
    static void function(TemperatureMonitor* _this)
    {
      while (!_this->mStopFlag && !Program::isSigInt()) {
        uint32_t value = *_this->mTemperatureRegister;
        if (value == 0) {
          _this->mValidFlag = false;
        } else {
          _this->mValidFlag = true;
          // Conversion formula from: https://documentation.altera.com/#/00045071-AA$AA00044865
          double C = double(value);
          double temperature = ((693.0 * C) / 1024.0) - 265.0;
          _this->mTemperature = temperature;

          if (temperature > MAX_TEMPERATURE) {
            _this->mMaxExceeded = true;
            cout << "\n!!! MAXIMUM TEMPERATURE WAS EXCEEDED: " << temperature << endl;
            break;
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    }
};

class ProgramCruExperimentalDma: public Program
{
  public:

    virtual UtilsDescription getDescription() override
    {
      return { "CRU EXPERIMENTAL DMA", "!!! USE WITH CAUTION !!!", "./rorc-cru-experimental-dma" };
    }

    virtual void addOptions(boost::program_options::options_description& options) override
    {
      options.add_options()
          (RESET_SWITCH.c_str(), "Reset card during initialization")
          (TO_FILE_SWITCH.c_str(), "Read out to file")
          (PAGES_SWITCH.c_str(), boost::program_options::value<int>(&mMaxRolls)->default_value(PAGES_DEFAULT),
              "Amount of pages to transfer. Give 0 for infinite.")
          (INTERRUPT_SWITCH.c_str(), "Use PCIe interrupts")
          (DISPLAY_FIFO_SWITCH.c_str(), "Display FIFO status (wide terminal recommended)")
          (RANDOM_PAUSES_SWITCH.c_str(), "Randomly pause readout (to test robustness)");
    }

    virtual void run(const boost::program_options::variables_map& map) override
    {
      using namespace AliceO2::Rorc;

      mInfiniteRolls = mMaxRolls == 0;
      mEnableResetCard = bool(map.count(RESET_SWITCH));
      mEnableFileOutput = bool(map.count(TO_FILE_SWITCH));
      mEnableInterrupts = bool(map.count(INTERRUPT_SWITCH));
      mEnableFifoDisplay = bool(map.count(DISPLAY_FIFO_SWITCH));
      mEnableRandomPauses = bool(map.count(RANDOM_PAUSES_SWITCH));


      if (mEnableFileOutput) {
        mReadoutStream.open(READOUT_DATA_PATH.c_str());
      }

      auto time = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
      auto filename = b::str(b::format(READOUT_LOG_FORMAT) % time);
      mLogStream.open(filename, std::ios_base::out);
      mLogStream << "# Time " << time << "\n";

      cout << "Initializing" << endl;
      initDma();

      cout << "Starting temperature monitor" << endl;
      mTemperatureMonitor.start(&mBar[Register::TEMPERATURE]);

      cout << "Starting DMA test" << endl;
      runDma();
      mTemperatureMonitor.stop();
    }

  private:
    /// A simple struct that holds the userspace and bus address of a page
    struct PageAddress
    {
        void* user;
        void* bus;
    };

    using TimePoint = std::chrono::high_resolution_clock::time_point;

    /// Array the size of a page
    using PageBuffer = std::array<uint32_t, DMA_PAGE_SIZE_32>;

    /// Array of pages
    using PageBufferArray = std::array<PageBuffer, NUM_PAGES>;

    void initDma()
    {
      system("modprobe -r uio_pci_dma");
      system("modprobe uio_pci_dma");

      bfs::remove(DMA_BUFFER_PAGES_PATH);

      int serial = 12345;
      int channel = 0;
      Util::resetSmartPtr(mRorcDevice, serial);
      Util::resetSmartPtr(mPdaBar, mRorcDevice->getPciDevice(), channel);
      mBar = mPdaBar->getUserspaceAddressU32();
      if (mEnableInterrupts) {
        Util::resetSmartPtr(mPdaIsr, mRorcDevice->getPciDevice());
      }
      Util::resetSmartPtr(mMappedFilePages, DMA_BUFFER_PAGES_PATH.c_str(), DMA_BUFFER_PAGES_SIZE);
      Util::resetSmartPtr(mBufferPages, mRorcDevice->getPciDevice(), mMappedFilePages->getAddress(),
          mMappedFilePages->getSize(), BUFFER_INDEX_PAGES);

      if (mEnableResetCard) {
        cout << "Resetting..." << std::flush;
        mBar[Register::RESET_CONTROL] = 0x1;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cout << "done!" << endl;
      }

      // Initialize the fifo & page addresses
      {
        mPageAddresses.clear();

        /// Amount of space reserved for the FIFO, we use multiples of the page size for uniformity
        size_t fifoSpace = ((sizeof(CruFifoTable) / DMA_PAGE_SIZE) + 1) * DMA_PAGE_SIZE;

        auto list = mBufferPages->getScatterGatherList();

        if (list.size() < 1) {
          BOOST_THROW_EXCEPTION(CruException()
              << errinfo_rorc_error_message("Scatter-gather list empty"));
        }

        if (list.at(0).size < fifoSpace) {
          BOOST_THROW_EXCEPTION(CruException()
              << errinfo_rorc_error_message("First SGL entry size insufficient for FIFO"));
        }

        mFifoUser = reinterpret_cast<CruFifoTable*>(list.at(0).addressUser);
        mFifoDevice = reinterpret_cast<CruFifoTable*>(list.at(0).addressBus);

        for (int i = 0; i < list.size(); ++i) {
          auto& entry = list[i];
          if (entry.size < (2l * 1024l * 1024l)) {
            BOOST_THROW_EXCEPTION(CruException()
                << errinfo_rorc_error_message("Unsupported configuration: DMA scatter-gather entry size less than 2 MiB")
                << errinfo_rorc_scatter_gather_entry_size(entry.size)
                << errinfo_rorc_possible_causes(
                    {"DMA buffer was not allocated in hugepage shared memory (hugetlbfs may not be properly mounted)"}));
          }

          bool first = i == 0;

          // How many pages fit in this SGL entry
          int64_t pagesInSglEntry = first
              ? (entry.size - fifoSpace) / DMA_PAGE_SIZE // First entry also contains the FIFO
              : entry.size / DMA_PAGE_SIZE;

          for (int64_t j = 0; j < pagesInSglEntry; ++j) {
            int64_t offset = first
              ? fifoSpace + j * DMA_PAGE_SIZE
              : j * DMA_PAGE_SIZE;
            PageAddress pa;
            pa.bus = (void*) (((char*) entry.addressBus) + offset);
            pa.user = (void*) (((char*) entry.addressUser) + offset);

            mPageAddresses.push_back(pa);
          }
        }

        if (mPageAddresses.size() <= NUM_PAGES) {
          BOOST_THROW_EXCEPTION(CrorcException()
              << errinfo_rorc_error_message("Insufficient amount of pages fit in DMA buffer"));
        }
      }

      // Initializing the descriptor table
      mFifoUser->resetStatusEntries();
      // As a safety measure, we put "valid" addresses in the descriptor table, even though we're not pushing pages yet
      // This helps prevent the card from writing to invalid addresses and crashing absolutely everything
      for (int i = 0; i < mFifoUser->descriptorEntries.size(); i++) {
        setDescriptor(i, i);
      }

      // Reset buffer to default value
      for (auto& page : mPageAddresses) {
        resetPage(reinterpret_cast<uint32_t*>(page.user));
      }

      // Note: the sleeps are needed until firmware implements proper "handshakes"

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Init temperature sensor
      mBar[Register::TEMPERATURE] = 0x1;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      mBar[Register::TEMPERATURE] = 0x0;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      mBar[Register::TEMPERATURE] = 0x2;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      // Status base address, low and high part, respectively
      mBar[Register::STATUS_BASE_BUS_HIGH] = Util::getUpper32Bits(uint64_t(mFifoDevice));
      mBar[Register::STATUS_BASE_BUS_LOW] = Util::getLower32Bits(uint64_t(mFifoDevice));

      // Destination (card's memory) addresses, low and high part, respectively
      mBar[Register::STATUS_BASE_CARD_HIGH] = 0x0;
      mBar[Register::STATUS_BASE_CARD_LOW] = 0x8000;

      // Set descriptor table size (must be size - 1)
      mBar[Register::DESCRIPTOR_TABLE_SIZE] = NUM_PAGES - 1;

      // Send command to the DMA engine to write to every status entry, not just the final one
      mBar[Register::DONE_CONTROL] = 0x1;

      cout << "DMA_POINTER = " << mBar[Register::DMA_POINTER] << '\n';
      // Send command to the DMA engine to point to the last descriptor so we can start with 0
      mBar[Register::DMA_POINTER] = NUM_PAGES - 1;

      // Give buffer ready signal
      mBar[Register::BUFFER_READY] = 0x1;

      // Programming the user module to trigger the data emulator
      mBar[Register::DATA_EMULATOR_CONTROL] = 0x1;

      // Wait for initial pages
      while(!isSigInt() && !mFifoUser->statusEntries[NUM_PAGES - 1].isPageArrived()) {;}
      mFifoUser->resetStatusEntries();

      cout << "  Firmware version  " << Utilities::Common::make32hexString(mBar[Register::FIRMWARE_COMPILE_INFO]) << '\n';
      cout << "  Serial number     " << Utilities::Common::make32hexString(mBar[Register::SERIAL_NUMBER]) << '\n';
      cout << "  Buffer size       " << mPageAddresses.size() << " pages, " << " "
          << mPageAddresses.size() * DMA_BUFFER_PAGES_SIZE << " bytes\n";

      mLogStream << "# Firmware version  " << Utilities::Common::make32hexString(mBar[Register::FIRMWARE_COMPILE_INFO]) << '\n';
      mLogStream << "# Serial number     " << Utilities::Common::make32hexString(mBar[Register::SERIAL_NUMBER]) << '\n';
      mLogStream << "# Buffer size       " << mPageAddresses.size() << " pages, " << " "
          << mPageAddresses.size() * DMA_BUFFER_PAGES_SIZE << " bytes\n";
    }

    void setDescriptor(int pageIndex, int descriptorIndex)
    {
      auto& pageAddress = mPageAddresses.at(pageIndex);
      auto sourceAddress = reinterpret_cast<void*>((descriptorIndex % NUM_OF_BUFFERS) * DMA_PAGE_SIZE);
      mFifoUser->setDescriptor(descriptorIndex, DMA_PAGE_SIZE_32, sourceAddress, pageAddress.bus);
    }

    void updateStatusDisplay()
    {
      using namespace std::chrono;
      auto diff = high_resolution_clock::now() - mRunStart;
      auto second = duration_cast<seconds>(diff).count() % 60;
      auto minute = duration_cast<minutes>(diff).count() % 60;
      auto hour = duration_cast<hours>(diff).count();

      auto format = b::format(PROGRESS_FORMAT) % hour % minute % second % mPageCounter % mErrorCount % mStatusCount;
      if (mTemperatureMonitor.isValid()) {
        format % mTemperatureMonitor.getTemperature();
      } else {
        format % "n/a";
      }

      cout << '\r' << format;

      if (mEnableFifoDisplay) {
        for (int i = 0; i < 128; ++i) {
          if ((i % 8) == 0) {
            cout << '|';
          }
          cout << (mFifoUser->statusEntries.at(i).isPageArrived() ? 'X' : ' ');
        }
        cout << '|';
      }

      // This takes care of adding a "line" to the stdout and log table every so many seconds
      {
        int interval = 60;
        auto second = duration_cast<seconds>(diff).count() % interval;
        if (mDisplayUpdateNewline && second == 0) {
          cout << '\n';
          mLogStream << '\n' << format;
          mDisplayUpdateNewline = false;
        }
        if (second >= 1) {
          mDisplayUpdateNewline = true;
        }
      }
    }

    void printHeader()
    {
      auto line1 = b::format(PROGRESS_FORMAT_HEADER) % "Time" % "Counter" % "Errors" % "Status" % "°C";
      auto line2 = b::format(PROGRESS_FORMAT) % "00" % "00" % "00" % '-' % '-' % '-' % '-';

      cout << '\n' << line1;
      cout << '\n' << line2;
      mLogStream << '\n' << line1;
      mLogStream << '\n' << line2;
    }

    bool isDisplayInterval()
    {
      auto now = std::chrono::high_resolution_clock::now();
      if (std::chrono::duration_cast<std::chrono::milliseconds, int64_t>(now - mLastDisplayUpdate) > DISPLAY_INTERVAL) {
        mLastDisplayUpdate = now;
        return true;
      }
      return false;
    }

    struct Handle
    {
        /// Index for CRU DMA descriptor table
        int descriptorIndex;

        /// Index for mPageAddresses
        int pageIndex;
    };

    bool isPageArrived(const Handle& handle)
    {
      return mFifoUser->statusEntries[handle.descriptorIndex].isPageArrived();
    }

    uint32_t* getPageAddress(const Handle& handle)
    {
      return reinterpret_cast<uint32_t*>(mPageAddresses.at(handle.pageIndex).user);
    }

    void runDma()
    {
      if (isVerbose()) {
        printHeader();
      }

      mRunStart = std::chrono::high_resolution_clock::now();

      std::queue<Handle> queue;
      const int MAX_QUEUE_SIZE = NUM_PAGES;

      int descriptorCounter = 0;
      int pageIndexCounter = 0;

      int i = 0;
      while (true) {
        while (queue.size() < MAX_QUEUE_SIZE) {
          // Push page
          Handle handle;
          handle.descriptorIndex = descriptorCounter;
          handle.pageIndex = pageIndexCounter;
          descriptorCounter = (descriptorCounter + 1) % NUM_PAGES;
          pageIndexCounter = (pageIndexCounter + 1) % mPageAddresses.size();

          setDescriptor(handle.pageIndex, handle.descriptorIndex);
          mBar[Register::DMA_POINTER] = handle.descriptorIndex;

          queue.push(handle);
        }

        if (isPageArrived(queue.front())) {
          auto handle = queue.front();
          mPageCounter++;

          uint32_t* pushedPage = getPageAddress(handle);

          // Read out to file
          if (mEnableFileOutput) {
            fileOutput(handle);
          }

          // Data error checking
          checkErrors(handle);

          // Setting the buffer to the default value after the readout
          resetPage(pushedPage);

          // Reset status entry
          mFifoUser->statusEntries[handle.descriptorIndex].reset();

          // Pop the handle
          queue.pop();
        }

        if (mEnableRandomPauses) {
          auto now = std::chrono::high_resolution_clock::now();
          if (now >= mRandomPauses.nextPause) {
            cout << b::format("! pause %-4d ms ... ") % mRandomPauses.nextPauseLength.count() << std::flush;
            std::this_thread::sleep_for(mRandomPauses.nextPauseLength);
            cout << " resume\n";

            auto now = std::chrono::high_resolution_clock::now();
            mRandomPauses.nextPause = now + std::chrono::milliseconds(getRandRange(NEXT_PAUSE_MIN, NEXT_PAUSE_MAX));
            mRandomPauses.nextPauseLength = std::chrono::milliseconds(getRandRange(PAUSE_LENGTH_MIN, PAUSE_LENGTH_MAX));
          }
        }

        if (!mInfiniteRolls && mPageCounter >= mMaxRolls) {
          cout << "\n\nMaximum amount of rolls reached\n";
          break;
        }
        else if (mTemperatureMonitor.isMaxExceeded()) {
          cout << "\n\n!!! ABORTING: MAX TEMPERATURE EXCEEDED\n";
          break;
        }
        else if (isSigInt()) {
          cout << "\n\nInterrupted\n";
          break;
        }
        else if (isVerbose() && isDisplayInterval()) {
          // TODO NOTE: It seems this update sometimes messes with the PCI interrupt, resulting in timeouts.
          // Does it mask the interrupt??
          updateStatusDisplay();
        }
      }

      mRunEnd = std::chrono::high_resolution_clock::now();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      outputErrors();
      outputStats();
    }

    int getRandRange(int min, int max)
    {
      return (std::rand() % max - min) + min;
    }

    void outputErrors()
    {
      auto errorStr = mErrorStream.str();

      if (isVerbose()) {
        size_t maxChars = 2000;
        if (!errorStr.empty()) {
          cout << "Errors:\n";
          cout << errorStr.substr(0, maxChars);
          if (errorStr.length() > maxChars) {
            cout << "\n... more follow (" << (errorStr.length() - maxChars) << " characters)\n";
          }
        }
      }

      std::ofstream stream(READOUT_ERRORS_PATH);
      stream << errorStr;
    }

    void outputStats()
    {
      // Calculating throughput
      double runTime = std::chrono::duration<double>(mRunEnd - mRunStart).count();
      double bytes = double(mPageCounter) * DMA_PAGE_SIZE;
      double GB = bytes / (1000 * 1000 * 1000);
      double GBs = GB / runTime;
      double Gbs = GBs * 8.0;
      double GiB = bytes / (1024 * 1024 * 1024);
      double GiBs = GiB / runTime;
      double Gibs = GiBs * 8.0;

      auto format = b::format("  %-10s  %-10s\n");
      std::ostringstream stream;
      stream << '\n';
      stream << format % "Seconds" % runTime;
      stream << format % "Bytes" % bytes;
      if (bytes > 0.00001) {
        stream << format % "GB" % GB;
        stream << format % "GB/s" % GBs;
        stream << format % "Gb/s" % Gbs;
        stream << format % "GiB" % GiB;
        stream << format % "GiB/s" % GiBs;
        stream << format % "Gibit/s" % Gibs;
        stream << format % "Errors" % mErrorCount;
      }
      stream << '\n';

      auto str = stream.str();
      cout << str;
      mLogStream << '\n' << str;
    }

    void copyPage (uint32_t* target, const uint32_t* source)
    {
      std::copy(source, source + DMA_PAGE_SIZE_32, target);
    }

    void fileOutput(const Handle& handle)
    {
      uint32_t* page = getPageAddress(handle);
      mReadoutStream << handle.pageIndex << '\n';
      int perLine = 8;
      for (int i = 0; i < DMA_PAGE_SIZE_32; i+= perLine) {
        for (int j = 0; j < perLine; ++j) {
          mReadoutStream << page[i+j] << ' ';
        }
        mReadoutStream << '\n';
      }
      mReadoutStream << '\n';
    }

    void checkErrors(const Handle& handle)
    {
      uint32_t* page = getPageAddress(handle);

      auto reportError = [&](int i, int expectedValue, int actualValue) {
        mErrorCount++;
        if (isVerbose() && mErrorCount < MAX_RECORDED_ERRORS) {
          mErrorStream << "Error @ page:" << handle.pageIndex << " i:" << i << " exp:" << expectedValue
              << " val:" << actualValue << '\n';
        }
      };

      // The data emulator's counter resets every 4 pages
      const int section = handle.descriptorIndex % 4;
      // The data emulator writes to every 8th 32-bit word
      constexpr int stride = 8;

      // For the first section, the first number is 0, and the count starts on the second number
      const bool first = (section == 0);
      const int start = first ? stride : 0;

      if (first && (page[0] != 0)) {
        reportError(0, 0, page[0]);
      }

      int expectedValue = first ? 0 : section * 256 - 1;
      for (int i = start; i < DMA_PAGE_SIZE_32; i += stride)
      {
        if (page[i] != expectedValue) {
          reportError(i, expectedValue, page[i]);
        }
        expectedValue++;
      }
    }

    void resetPage(uint32_t* page)
    {
      for (size_t i = 0; i < DMA_PAGE_SIZE_32; i++) {
        page[i] = BUFFER_DEFAULT_VALUE;
      }
    }

    /// Max amount of errors that are recorded into the error stream
    static constexpr int64_t MAX_RECORDED_ERRORS = 1000;

    // Program options
    int mMaxRolls;
    bool mInfiniteRolls; // true means no limit on rolling
    bool mEnableFileOutput;
    bool mEnableResetCard;
    bool mEnableInterrupts;
    bool mEnableFifoDisplay;
    bool mEnableRandomPauses;

    /// Start of run time
    TimePoint mRunStart;

    /// End of run time
    TimePoint mRunEnd;

    /// Temperature monitor thing
    TemperatureMonitor mTemperatureMonitor;

    // PDA, buffer, etc stuff
    b::scoped_ptr<RorcDevice> mRorcDevice;
    b::scoped_ptr<Pda::PdaBar> mPdaBar;
    b::scoped_ptr<PdaIsr> mPdaIsr;
    b::scoped_ptr<MemoryMappedFile> mMappedFilePages;
    b::scoped_ptr<Pda::PdaDmaBuffer> mBufferPages;

    /// Convenience pointer to the BAR, which is actually owned by mPdaBar
    volatile uint32_t* mBar;

    /// Aliased userspace FIFO
    CruFifoTable* mFifoUser;
    CruFifoTable* mFifoDevice;

    /// Value of some kind of CRU status register. Seems to always be 0.
    uint32_t mStatusCount;

    /// Amount of pages pushed
    int64_t mPageCounter;

    /// Amount of data errors detected
    int64_t mErrorCount;

    /// Stream for file readout, only opened if enabled by the --tofile program option
    std::ofstream mReadoutStream;

    /// Stream for log output
    std::ofstream mLogStream;

    /// Stream for error output
    std::ostringstream mErrorStream;

    /// Time of the last display update
    TimePoint mLastDisplayUpdate;

    /// Indicates the display must add a newline to the table
    bool mDisplayUpdateNewline;

    /// Addresses of pages
    std::vector<PageAddress> mPageAddresses;

    struct RandomPauses {
        TimePoint nextPause;
        std::chrono::milliseconds nextPauseLength;
    } mRandomPauses;
};

} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCruExperimentalDma().execute(argc, argv);
}
