/// \file ProgramCruExperimentalDma.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Based on https://gitlab.cern.ch/alice-cru/pciedma_eval

#include "Utilities/Program.h"
#include <iostream>
#include <iomanip>
#include <condition_variable>
#include <thread>
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

constexpr int FIFO_ENTRIES = 4;

/// DMA page length in bytes
constexpr int DMA_PAGE_SIZE = 8 * 1024;

/// DMA page length in 32-bit words
constexpr int DMA_PAGE_SIZE_32 = DMA_PAGE_SIZE / 4;

constexpr int NUM_OF_BUFFERS = 32;

constexpr int NUM_PAGES = FIFO_ENTRIES * NUM_OF_BUFFERS;

/// Offset in bytes from start of status table to data buffer
constexpr size_t DATA_OFFSET = 0x200;

constexpr size_t DMA_BUFFER_PAGES_SIZE = 2l * 1024l * 1024l; // One 2MiB hugepage. Should be enough...

constexpr uint32_t BUFFER_DEFAULT_VALUE = 0xCcccCccc;

/// PDA DMA buffer index for the pages buffer
constexpr int BUFFER_INDEX_PAGES = 0;

const bfs::path DMA_BUFFER_PAGES_PATH = "/mnt/hugetlbfs/rorc-cru-experimental-dma-pages";

/// Fields: Time(hour:minute:second), i, Counter, Rolling, Errors, Status, °C
const std::string PROGRESS_FORMAT_HEADER("  %-8s   %-12s  %-12s  %-12s  %-10s  %-10.1f");

/// Fields: Time(hour:minute:second), i, Counter, Rolling, Errors, Status, °C
const std::string PROGRESS_FORMAT("  %02s:%02s:%02s   %-12s  %-12s  %-12s  %-10s  %-10.1f");

constexpr int ROLLS_DEFAULT = 1500;

constexpr double MAX_TEMPERATURE = 45.0;

const std::string RESET_SWITCH = "reset";
const std::string TO_FILE_SWITCH = "tofile";
const std::string ROLLS_SWITCH = "rolls";
const std::string INTERRUPT_SWITCH = "interrupts";

const std::string READOUT_ERRORS_PATH = "readout_errors.txt";
const std::string READOUT_DATA_PATH = "readout_data.txt";
const std::string READOUT_LOG_FORMAT = "readout_log_%d.txt";

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
      return UtilsDescription("CRU EXPERIMENTAL DMA", "!!! USE WITH CAUTION !!!", "./rorc-cru-experimental-dma");
    }

    virtual void addOptions(boost::program_options::options_description& options) override
    {
      options.add_options()(RESET_SWITCH.c_str(), "Reset card during initialization");
      options.add_options()(TO_FILE_SWITCH.c_str(), "Read out to file");
      options.add_options()(ROLLS_SWITCH.c_str(),
          boost::program_options::value<int>(&mMaxRolls)->default_value(ROLLS_DEFAULT),
          "Amount of iterations over the FIFO. Give 0 for infinite.");
      options.add_options()(INTERRUPT_SWITCH.c_str(), "Use PCIe interrupts");
    }

    virtual void mainFunction(const boost::program_options::variables_map& map) override
    {
      using namespace AliceO2::Rorc;

      mInfiniteRolls = mMaxRolls == 0;
      mEnableResetCard = bool(map.count(RESET_SWITCH));
      mEnableFileOutput = bool(map.count(TO_FILE_SWITCH));
      mEnableInterrupts = bool(map.count(INTERRUPT_SWITCH));


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

    using TimePoint = std::chrono::high_resolution_clock::time_point;

    /// Array for data emulator pattern
    using DataPattern = std::array<int, 1024>;

    /// Array the size of a page
    using PageBuffer = std::array<uint32_t, DMA_PAGE_SIZE_32>;

    /// Array of pages
    using PageBufferArray = std::array<PageBuffer, NUM_PAGES>;


    void initDma()
    {
      system("modprobe -r uio_pci_dma");
      system("modprobe uio_pci_dma");

      bfs::remove(DMA_BUFFER_PAGES_PATH);

      mDataPattern = makePattern();

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

      auto entry = mBufferPages->getScatterGatherList().at(0);
      if (entry.size < (2l * 1024l * 1024l)) {
        BOOST_THROW_EXCEPTION(CruException()
            << errinfo_rorc_error_message("Unsupported configuration: DMA scatter-gather entry size less than 2 MiB")
            << errinfo_rorc_scatter_gather_entry_size(entry.size)
            << errinfo_rorc_possible_causes(
                {"DMA buffer was not allocated in hugepage shared memory (hugetlbfs may not be properly mounted)"}));
      }

      if (mEnableResetCard) {
        cout << "Resetting..." << std::flush;
        mBar[Register::RESET_CONTROL] = 0x1;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cout << "done!" << endl;
      }

      auto getDataAddress = [](void* fifoAddress) {
        // Data starts directly after the FIFO
        return reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(fifoAddress) + sizeof(CruFifoTable));
      };

      // Addresses in the RAM (DMA destination)
      // the start address of the descriptor table is always at the same place:
      // adding a 0x200 offset (= sizeof(CruFifoTable)) to the status start address, no
      // matter how many entries there are
      mFifoUser = reinterpret_cast<CruFifoTable*>(entry.addressUser);
      mDataUser = getDataAddress(mFifoUser);
      CruFifoTable* fifoDevice = reinterpret_cast<CruFifoTable*>(entry.addressBus);
      uint32_t* dataDevice = getDataAddress(fifoDevice);

      // Initializing the descriptor table
      mFifoUser->resetStatusEntries();

      for (int i = 0; i < NUM_PAGES; i++) {
        auto sourceAddress = reinterpret_cast<void*>((i % NUM_OF_BUFFERS) * DMA_PAGE_SIZE);
        auto destinationAddress = dataDevice + i * DMA_PAGE_SIZE_32;
        mFifoUser->setDescriptor(i, DMA_PAGE_SIZE_32, sourceAddress, destinationAddress);
      }

      // Setting the buffer to the default value

//      cout << "Data address user       : " << (void*) data_user << '\n';
      for (int i = 0; i < DMA_PAGE_SIZE_32 * FIFO_ENTRIES; i++) {
        mDataUser[i] = BUFFER_DEFAULT_VALUE;
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
      mBar[Register::STATUS_BASE_BUS_HIGH] = Util::getUpper32Bits(uint64_t(fifoDevice));
      mBar[Register::STATUS_BASE_BUS_LOW] = Util::getLower32Bits(uint64_t(fifoDevice));

      // Destination (card's memory) addresses, low and high part, respectively
      mBar[Register::STATUS_BASE_CARD_HIGH] = 0x0;
      mBar[Register::STATUS_BASE_CARD_LOW] = 0x8000;

      // Set descriptor table size (must be size - 1)
      mBar[Register::DESCRIPTOR_TABLE_SIZE] = NUM_PAGES - 1;

      // Send command to the DMA engine to write to every status entry, not just the final one
      mBar[Register::DONE_CONTROL] = 0x1;

      // Send command to the DMA engine to write NUM_PAGES
      mBar[Register::DMA_POINTER] = NUM_PAGES - 1;

      // make buffer ready signal
      mBar[Register::BUFFER_READY] = 0x1;

      // Programming the user module to trigger the data emulator
      mBar[Register::DATA_EMULATOR_CONTROL] = 0x1;

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      cout << "  Firmware version  " << Utilities::Common::make32hexString(mBar[Register::FIRMWARE_COMPILE_INFO]) << '\n';
      cout << "  Serial number     " << Utilities::Common::make32hexString(mBar[Register::SERIAL_NUMBER]) << '\n';
      mLogStream << "# Firmware version  " << Utilities::Common::make32hexString(mBar[Register::FIRMWARE_COMPILE_INFO]) << '\n';
      mLogStream << "# Serial number     " << Utilities::Common::make32hexString(mBar[Register::SERIAL_NUMBER]) << '\n';
    }

    void updateStatusDisplay()
    {
      static bool updateNewline = false;

      using namespace std::chrono;
      auto diff = high_resolution_clock::now() - mRunStart;
      auto second = duration_cast<seconds>(diff).count() % 60;
      auto minute = duration_cast<minutes>(diff).count() % 60;
      auto hour = duration_cast<hours>(diff).count();

      auto format = b::format(PROGRESS_FORMAT) % hour % minute % second % mPageCounter % mRollingCounter
          % mErrorCount % mStatusCount;
      if (mTemperatureMonitor.isValid()) {
        format % mTemperatureMonitor.getTemperature();
      } else {
        format % "n/a";
      }

      cout << '\r' << format;

      // This takes care of adding a "line" to the stdout and log table every so many seconds
      {
        int interval = 60;
        auto second = duration_cast<seconds>(diff).count() % interval;
        if (updateNewline && second == 0) {
          cout << '\n';
          mLogStream << '\n' << format;
          updateNewline = false;
        }
        if (second >= 1) {
          updateNewline = true;
        }
      }
    }

    void printHeader()
    {
      auto line1 = b::format(PROGRESS_FORMAT_HEADER) % "Time" % "Counter" % "Rolling" % "Errors" % "Status" % "°C";
      auto line2 = b::format(PROGRESS_FORMAT) % "00" % "00" % "00" % '-' % '-' % '-' % '-' % '-';

      cout << '\n' << line1;
      cout << '\n' << line2;
      mLogStream << '\n' << line1;
      mLogStream << '\n' << line2;
    }

    bool isDisplayInterval()
    {
      auto displayInterval = std::chrono::milliseconds(10);
      auto now = std::chrono::high_resolution_clock::now();
      if (std::chrono::duration_cast<std::chrono::milliseconds, int64_t>(now - mLastDisplayUpdate) > displayInterval) {
        mLastDisplayUpdate = now;
        return true;
      }
      return false;
    }

    void readoutTask(int i, int errorCheckPage, int rollingCounter)
    {
      uint32_t* pushedPage = &mDataUser[i*DMA_PAGE_SIZE_32];
      auto& readoutPage = mReadoutPages[i];

      // Copy page
      copyPage(readoutPage.data(), pushedPage);

      // Setting the buffer to the default value after the readout
      resetPage(pushedPage);

      // Read out to file
      if (mEnableFileOutput) {
        fileOutput(mReadoutStream, i, readoutPage);
      }

      // Data error checking
      checkErrors(mErrorStream, i, errorCheckPage, rollingCounter, readoutPage, mDataPattern);
    }

    void runDma()
    {
      if (isVerbose()) {
        printHeader();
      }

      mRunStart = std::chrono::high_resolution_clock::now();

#ifdef USE_ASYNC_READOUT
      std::vector<std::future<void>> handles(NUM_PAGES);
#endif

      auto readoutTask = [&](int i, int errorCheckPage, int rollingCounter){
        uint32_t* pushedPage = &mDataUser[i*DMA_PAGE_SIZE_32];
        auto& readoutPage = mReadoutPages[i];

        // Copy page
        copyPage(readoutPage.data(), pushedPage);

        // Setting the buffer to the default value after the readout
        resetPage(pushedPage);

        // Read out to file
        if (mEnableFileOutput) {
          fileOutput(mReadoutStream, i, readoutPage);
        }

        // Data error checking
        checkErrors(mErrorStream, i, errorCheckPage, rollingCounter, readoutPage, mDataPattern);
      };

      int i = 0;
      while (true) {
        // Interrupts are skipped on first iteration, because it would always time out otherwise
        if (mEnableInterrupts && mPageCounter != 0
            && (mPdaIsr->waitOnInterrupt(std::chrono::milliseconds(100)) == std::cv_status::timeout)) {
          cout << "Interrupt wait TIMED OUT at counter:" << mPageCounter << " rolling:" << mRollingCounter << "\n";
        }

        if (mFifoUser->statusEntries[i].isPageArrived()) {
#ifdef USE_ASYNC_READOUT
          // Complete last cycle's task if it's still ongoing
          if (handles[i].valid() && handles[i].wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout) {
            BOOST_THROW_EXCEPTION(CruException() << errinfo_rorc_error_message("Future timed out"));
          }
          handles[i] = std::async(std::launch::async, readoutTask, i, mErrorCheckPage, mRollingCounter);
#else
          readoutTask(i, mErrorCheckPage, mRollingCounter);
#endif
          mErrorCheckPage++;

          // Reset page status
          mFifoUser->statusEntries[i].reset();

          // points to the first descriptor
          if (i == 0) {
            mBar[Register::DMA_POINTER] = NUM_PAGES - 1;
          }

          // Push the same descriptor again when its gets executed
          // TODO Not sure why this was done, it seems to have a bad effect on performance (cuts it in half)
          //bar[Register::DMA_POINTER] = i;

          mPageCounter++;

          // After every 32Kbytes reset the current_page for online checking
          if ((i % FIFO_ENTRIES) == FIFO_ENTRIES - 1) {
            mErrorCheckPage = 0;
            mRollingCounter++;
          }

          if (i == NUM_PAGES - 1) {
            // read status count after 1MByte dma transfer
            mStatusCount = mBar[Register::READ_STATUS_COUNT];
            i = 0;
          } else {
            i++;
          }
        }

        if (!mInfiniteRolls && mRollingCounter >= mMaxRolls) {
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

#ifdef USE_ASYNC_READOUT
      for (auto& handle : handles) {
        if (handle.valid()) {
          handle.wait();
        }
      }
#endif

      mRunEnd = std::chrono::high_resolution_clock::now();

      outputErrors();
      outputStats();
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
      double bytes = double(mRollingCounter) * FIFO_ENTRIES * DMA_PAGE_SIZE;
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

    DataPattern makePattern()
    {
      DataPattern pattern;
      pattern[0] = 0;
      pattern[1] = 0;
      for (int i = 2; i < 1024; i++) {
        pattern[i] = i - 1;
      }
      return pattern;
    }

    void copyPage (uint32_t* target, const uint32_t* source)
    {
      std::copy(source, source + DMA_PAGE_SIZE_32, target);
    }

    void fileOutput(std::ostream& stream, const int i, const PageBuffer& page)
    {
      stream << i << '\n';
      for (int j = 0; j < (DMA_PAGE_SIZE_32 / 8); j += 8) {
        for (int k = 7; k >= 0; k--) {
          stream << page[j + k];
        }
        stream << '\n';
      }
      stream << '\n';
    }

    void checkErrors(std::ostream& errorStream, const int pageIndex, const int errorCheckPage, const int rolling,
        const PageBuffer& page, const DataPattern& pattern)
    {
      for (int j = 0; j < (DMA_PAGE_SIZE_32 / 8); j++) {
        size_t pattern_index = errorCheckPage * (DMA_PAGE_SIZE_32 / 8) + j;
        if (page[j * 8] != pattern[pattern_index]) {
          mErrorCount++;
          if (isVerbose() && mErrorCount < MAX_RECORDED_ERRORS) {
            errorStream << "data error at rolling " << rolling << ", page " << pageIndex << ", data " << j * 8 << ", "
                << page[j * 8] << " - " << pattern[pattern_index] << '\n';
          }
        }
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

    /// DMA buffer (userspace address)
    uint32_t* mDataUser;

    /// Page counter used for error checking
    int mErrorCheckPage;

    /// Value of some kind of CRU status register. Seems to always be 0.
    uint32_t mStatusCount;

    /// Amount of pages pushed
    int64_t mPageCounter;

    /// Amount of "rolls"
    int64_t mRollingCounter;

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

    /// Readout buffer
    PageBufferArray mReadoutPages;

    /// Pattern of the data, used for error checking
    DataPattern mDataPattern;
};

} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCruExperimentalDma().execute(argc, argv);
}
