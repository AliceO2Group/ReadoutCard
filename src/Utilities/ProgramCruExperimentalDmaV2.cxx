/// \file ProgramCruExperimentalDmaV2.cxx
/// \brief Based on https://gitlab.cern.ch/alice-cru/pciedma_eval
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

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
#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>
#include "RORC/Exception.h"
#include "RorcDevice.h"
#include "MemoryMappedFile.h"
#include "Cru/CruRegisterIndex.h"
#include "Cru/CruFifoTable.h"
#include "Cru/CruRegisterIndex.h"
#include "Cru/Temperature.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDmaBuffer.h"

/// Use busy wait instead of condition variable (c.v. impl incomplete, is very slow)
#define USE_BUSY_INTERRUPT_WAIT

namespace b = boost;
namespace bfs = boost::filesystem;
namespace Register = AliceO2::Rorc::CruRegisterIndex;
using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
//using namespace std::literals::chrono_literals;
using std::cout;
using std::endl;

namespace {

constexpr std::chrono::milliseconds DISPLAY_INTERVAL(10);

/// Amount of pages to push at a time
constexpr int PAGES_PER_PUSH = 4;

/// DMA addresses must be 32-byte aligned
constexpr uint64_t DMA_ALIGNMENT = 32;

/// DMA page length in bytes
constexpr int DMA_PAGE_SIZE = 8 * 1024;

/// DMA page length in 32-bit words
constexpr int DMA_PAGE_SIZE_32 = DMA_PAGE_SIZE / 4;

constexpr int NUM_OF_BUFFERS = 32;
constexpr int FIFO_ENTRIES = 4;
constexpr int NUM_PAGES = FIFO_ENTRIES * NUM_OF_BUFFERS;

/// Two 2MiB hugepage. Should be enough...
constexpr size_t DMA_BUFFER_PAGES_SIZE = 4l * 1024l * 1024l;

constexpr uint32_t BUFFER_DEFAULT_VALUE = 0xCcccCccc;

/// PDA DMA buffer index for the pages buffer
constexpr int BUFFER_INDEX_PAGES = 0;

const bfs::path DMA_BUFFER_PAGES_PATH = "/mnt/hugetlbfs/rorc-cru-experimental-dma-pages-v2";

/// Fields: Time(hour:minute:second), i, Counter, Errors, Fill, °C
const std::string PROGRESS_FORMAT_HEADER("  %-8s   %-12s  %-12s  %-10s  %-10.1f");

/// Fields: Time(hour:minute:second), i, Counter, Errors, Fill, °C
const std::string PROGRESS_FORMAT("  %02s:%02s:%02s   %-12s  %-12s  %-10s  %-10.1f");

/// Timeout of SIGINT handling
constexpr std::chrono::milliseconds HANDLING_SIGINT_TIMEOUT {10};

/// Default number of pages
constexpr int PAGES_DEFAULT = 1500;

/// Minimum random pause interval in milliseconds
constexpr int NEXT_PAUSE_MIN = 10;
/// Maximum random pause interval in milliseconds
constexpr int NEXT_PAUSE_MAX = 2000;
/// Minimum random pause in milliseconds
constexpr int PAUSE_LENGTH_MIN = 1;
/// Maximum random pause in milliseconds
constexpr int PAUSE_LENGTH_MAX = 500;

const std::string RESET_SWITCH = "reset";
const std::string TO_FILE_SWITCH = "to-file";
const std::string PAGES_SWITCH = "pages";
const std::string DISPLAY_FIFO_SWITCH = "show-fifo";
const std::string RANDOM_PAUSE_SOFTWARE_SWITCH = "rand-pause-sw";
const std::string RANDOM_PAUSE_FIRMWARE_SWITCH = "rand-pause-fw";
const std::string NO_CHECK_SWITCH = "no-errorcheck";
const std::string REMOVE_SHARED_MEMORY_SWITCH = "rm-sharedmem";
const std::string RELOAD_KERNEL_MODULE_SWITCH = "reload-kmod";
const std::string RESYNC_DATA_GENERATOR_COUNTER = "resync-counter";

const std::string READOUT_ERRORS_PATH = "readout_errors.txt";
const std::string READOUT_DATA_PATH = "readout_data.txt";
const std::string READOUT_LOG_FORMAT = "readout_log_%d.txt";

template <typename Predicate, typename Duration>
bool waitOnPredicateWithTimeout(Duration duration, Predicate predicate)
{
  auto start = std::chrono::high_resolution_clock::now();
  while (predicate() == false) {
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - start) > duration) {
      return false;
    }
  }
  return true;
}

/// Test class for PCI interrupts

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

        b::optional<double> temperature = Cru::Temperature::convertRegisterValue(value);
        if (!temperature) {
          _this->mValidFlag = false;
        } else {
          _this->mValidFlag = true;
          _this->mTemperature = *temperature;
          if (*temperature > Cru::Temperature::MAX_TEMPERATURE) {
            _this->mMaxExceeded = true;
            cout << "\n!!! MAXIMUM TEMPERATURE WAS EXCEEDED: " << *temperature << endl;
            break;
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    }
};

/// Some wrapper for keeping track of user memory space and bus memory space ...
template <typename T>
struct AddressSpaces
{
    AddressSpaces() : user(nullptr), bus(nullptr)
    {
    }

    AddressSpaces(void* user, void* bus) : user(reinterpret_cast<T*>(user)), bus(reinterpret_cast<T*>(bus))
    {
    }

    T* user;
    T* bus;
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
          (RESET_SWITCH.c_str(),
              "Reset card during initialization")
          (TO_FILE_SWITCH.c_str(),
              "Read out to file")
          (PAGES_SWITCH.c_str(),
              boost::program_options::value<int64_t>(&mOptions.maxPages)->default_value(PAGES_DEFAULT),
              "Amount of pages to transfer. Give <= 0 for infinite.")
          (DISPLAY_FIFO_SWITCH.c_str(),
              "Display FIFO status (wide terminal recommended)")
          (RANDOM_PAUSE_SOFTWARE_SWITCH.c_str(),
              "Randomly pause readout using software method")
          (RANDOM_PAUSE_FIRMWARE_SWITCH.c_str(),
              "Randomly pause readout using firmware method")
          (NO_CHECK_SWITCH.c_str(),
              "Skip error checking")
          (REMOVE_SHARED_MEMORY_SWITCH.c_str(),
              "Remove shared memory after DMA transfer")
          (RELOAD_KERNEL_MODULE_SWITCH.c_str(),
              "Reload kernel module before DMA initialization")
          (RESYNC_DATA_GENERATOR_COUNTER.c_str(),
              "Automatically resynchronize data generator counter in case of errors")
              ;
    }

    virtual void run(const boost::program_options::variables_map& map) override
    {
      using namespace AliceO2::Rorc;

      mInfinitePages = (mOptions.maxPages <= 0);
      mOptions.resetCard = bool(map.count(RESET_SWITCH));
      mOptions.fileOutput = bool(map.count(TO_FILE_SWITCH));
      mOptions.fifoDisplay = bool(map.count(DISPLAY_FIFO_SWITCH));
      mOptions.randomPauseSoft = bool(map.count(RANDOM_PAUSE_SOFTWARE_SWITCH));
      mOptions.randomPauseFirm = bool(map.count(RANDOM_PAUSE_FIRMWARE_SWITCH));
      mOptions.errorCheck = !bool(map.count(NO_CHECK_SWITCH));
      mOptions.removeSharedMemory = bool(map.count(REMOVE_SHARED_MEMORY_SWITCH));
      mOptions.reloadKernelModule = bool(map.count(RELOAD_KERNEL_MODULE_SWITCH));
      mOptions.resyncCounter = bool(map.count(RESYNC_DATA_GENERATOR_COUNTER));

//      mDataGeneratorOffsetNeedsResync = mOptions.resyncCounter;

      if (mOptions.fileOutput) {
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
      mTemperatureMonitor.start(&(mPdaBar->getUserspaceAddressU32()[Register::TEMPERATURE]));

      cout << "Starting DMA test" << endl;
      runDma();
      mTemperatureMonitor.stop();

      if (mOptions.removeSharedMemory) {
        cout << "Removing shared memory file\n";
        removeDmaBufferFile();
      }
    }

  private:
    /// A simple struct that holds the userspace and bus address of a page
    struct PageAddress
    {
        void* user;
        void* bus;
    };

    struct Handle
    {
        int descriptorIndex; ///< Index for CRU DMA descriptor table
        int pageIndex; ///< Index for mPageAddresses
    };

    /// Underlying buffer for the ReadoutQueue
    using QueueBuffer = boost::circular_buffer<Handle>;

    /// Queue for readout page handles
    using ReadoutQueue = std::queue<Handle, QueueBuffer>;

    using TimePoint = std::chrono::high_resolution_clock::time_point;

    /// Array the size of a page
    using PageBuffer = std::array<uint32_t, DMA_PAGE_SIZE_32>;

    /// Array of pages
    using PageBufferArray = std::array<PageBuffer, NUM_PAGES>;

    void removeDmaBufferFile()
    {
      bfs::remove(DMA_BUFFER_PAGES_PATH);
    }

    void printDeviceInfo(PciDevice* device)
    {
      uint16_t domainId;
      PciDevice_getDomainID(device, &domainId);

      uint8_t busId;
      PciDevice_getBusID(device, &busId);

      uint8_t functionId;
      PciDevice_getFunctionID(device, &functionId);

      const PciBarTypes* pciBarTypesPtr;
      PciDevice_getBarTypes(device, &pciBarTypesPtr);

      auto barType = *pciBarTypesPtr;
      auto barTypeString =
          barType == PCIBARTYPES_NOT_MAPPED ? "NOT_MAPPED" :
          barType == PCIBARTYPES_IO ? "IO" :
          barType == PCIBARTYPES_BAR32 ? "BAR32" :
          barType == PCIBARTYPES_BAR64 ? "BAR64" :
          "n/a";

      cout << "Device info";
      cout << "\n  Domain ID      " << domainId;
      cout << "\n  Bus ID         " << uint32_t(busId);
      cout << "\n  Function ID    " << uint32_t(functionId);
      cout << "\n  BAR type       " << barTypeString << " (" << *pciBarTypesPtr << ")\n";
    }

    bool checkAlignment(void* address, int64_t alignment) const
    {
      return (uint64_t(address) % alignment) == 0;
    }

    /// Partition the memory of the scatter gather list into pages
    /// \param list Scatter gather list
    /// \param fifoSpace Amount of space reserved for the FIFO, must use multiples of the page size for uniformity
    /// \return A tuple containing: address of the page for the FIFO, vector with addresses of the rest of the pages
    std::tuple<AddressSpaces<CruFifoTable>, std::vector<PageAddress>> partitionScatterGatherList(
        const Pda::PdaDmaBuffer::ScatterGatherVector& list,
        size_t fifoSpace) const
    {
      std::tuple<AddressSpaces<CruFifoTable>, std::vector<PageAddress>> tuple;
      auto& fifoAddress = std::get<0>(tuple);
      auto& pageAddresses = std::get<1>(tuple);

      if (list.size() < 1) {
        BOOST_THROW_EXCEPTION(CruException()
            << errinfo_rorc_error_message("Scatter-gather list empty"));
      }

      if (list.at(0).size < fifoSpace) {
        BOOST_THROW_EXCEPTION(CruException()
            << errinfo_rorc_error_message("First SGL entry size insufficient for FIFO"));
      }

      fifoAddress = AddressSpaces<CruFifoTable>(list.at(0).addressUser, list.at(0).addressBus);

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

          pageAddresses.push_back(pa);
        }
      }

      return tuple;
    }

    void initDma()
    {
      if (mOptions.reloadKernelModule) {
        system("modprobe -r uio_pci_dma");
        system("modprobe uio_pci_dma");
      }

      //removeDmaBufferFile();

      int serial = 12345;
      int channel = 0;
      Util::resetSmartPtr(mRorcDevice, serial);
      if (isVerbose()) {
        printDeviceInfo(mRorcDevice->getPciDevice());
      }
      Util::resetSmartPtr(mPdaBar, mRorcDevice->getPciDevice(), channel);
      auto bar = mPdaBar->getUserspaceAddressU32();
      Util::resetSmartPtr(mMappedFilePages, DMA_BUFFER_PAGES_PATH.c_str(), DMA_BUFFER_PAGES_SIZE);
      Util::resetSmartPtr(mBufferPages, mRorcDevice->getPciDevice(), mMappedFilePages->getAddress(),
          mMappedFilePages->getSize(), BUFFER_INDEX_PAGES);

      if (mOptions.resetCard) {
        cout << "Resetting..." << std::flush;
        bar[Register::RESET_CONTROL] = 0x2;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bar[Register::RESET_CONTROL] = 0x1;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cout << "done!" << endl;
      }

      // Initialize the fifo & page addresses
      {
        /// Amount of space reserved for the FIFO, we use multiples of the page size for uniformity
        size_t fifoSpace = ((sizeof(CruFifoTable) / DMA_PAGE_SIZE) + 1) * DMA_PAGE_SIZE;

        std::tie(mFifoAddress, mPageAddresses) = partitionScatterGatherList(mBufferPages->getScatterGatherList(), fifoSpace);

        if (mPageAddresses.size() <= NUM_PAGES) {
          BOOST_THROW_EXCEPTION(CrorcException()
              << errinfo_rorc_error_message("Insufficient amount of pages fit in DMA buffer"));
        }
      }

      // Initializing the descriptor table
      mFifoAddress.user->resetStatusEntries();
      // As a safety measure, we put "valid" addresses in the descriptor table, even though we're not pushing pages yet
      // This helps prevent the card from writing to invalid addresses and crashing absolutely everything
      for (int i = 0; i < mFifoAddress.user->descriptorEntries.size(); i++) {
        setDescriptor(i, i);
      }

      // Reset buffer to default value
      for (auto& page : mPageAddresses) {
        resetPage(reinterpret_cast<uint32_t*>(page.user));
      }

      // Note: the sleeps are needed until firmware implements proper "handshakes"

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Init temperature sensor
      bar[Register::TEMPERATURE] = 0x1;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      bar[Register::TEMPERATURE] = 0x0;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      bar[Register::TEMPERATURE] = 0x2;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      // Status base address in the bus address space
      if (Util::getUpper32Bits(uint64_t(mFifoAddress.bus)) != 0) {
        cout << "Warning: using 64-bit region for status bus address (" << reinterpret_cast<void*>(mFifoAddress.bus)
            << "), may be unsupported by PCI/BIOS configuration.\n";
      } else {
        cout << "Info: using 32-bit region for status bus address (" << reinterpret_cast<void*>(mFifoAddress.bus) << ")\n";
      }
      cout << "Info: status user address (" << reinterpret_cast<void*>(mFifoAddress.user) << ")\n";

      if (!checkAlignment(mFifoAddress.bus, DMA_ALIGNMENT)) {
        BOOST_THROW_EXCEPTION(CruException() << errinfo_rorc_error_message("mFifoDevice not 32 byte aligned"));
      }
      bar[Register::STATUS_BASE_BUS_HIGH] = Util::getUpper32Bits(uint64_t(mFifoAddress.bus));
      bar[Register::STATUS_BASE_BUS_LOW] = Util::getLower32Bits(uint64_t(mFifoAddress.bus));

      // Status table address in the card's address space
      bar[Register::STATUS_BASE_CARD_HIGH] = 0x0;
      bar[Register::STATUS_BASE_CARD_LOW] = 0x8000;

      // Set descriptor table size (must be size - 1)
      bar[Register::DESCRIPTOR_TABLE_SIZE] = NUM_PAGES - 1;

      // Send command to the DMA engine to write to every status entry, not just the final one
      bar[Register::DONE_CONTROL] = 0x1;

      mFifoAddress.user->resetStatusEntries();

      // Give buffer ready signal
      bar[Register::DATA_EMULATOR_CONTROL] = 0x3;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      cout << "  Firmware version  " << Utilities::Common::make32hexString(bar[Register::FIRMWARE_COMPILE_INFO]) << '\n';
      cout << "  Serial number     " << Utilities::Common::make32hexString(bar[Register::SERIAL_NUMBER]) << '\n';
      cout << "  Buffer size       " << mPageAddresses.size() << " pages, " << " "
          << mPageAddresses.size() * DMA_BUFFER_PAGES_SIZE << " bytes\n";

      mLogStream << "# Firmware version  " << Utilities::Common::make32hexString(bar[Register::FIRMWARE_COMPILE_INFO]) << '\n';
      mLogStream << "# Serial number     " << Utilities::Common::make32hexString(bar[Register::SERIAL_NUMBER]) << '\n';
      mLogStream << "# Buffer size       " << mPageAddresses.size() << " pages, " << " "
          << mPageAddresses.size() * DMA_BUFFER_PAGES_SIZE << " bytes\n";
    }

    void setDescriptor(int pageIndex, int descriptorIndex)
    {
      auto& pageAddress = mPageAddresses.at(pageIndex);
      auto sourceAddress = reinterpret_cast<void*>((descriptorIndex % NUM_OF_BUFFERS) * DMA_PAGE_SIZE);
      mFifoAddress.user->setDescriptor(descriptorIndex, DMA_PAGE_SIZE_32, sourceAddress, pageAddress.bus);
    }

    void updateStatusDisplay()
    {
      using namespace std::chrono;
      auto diff = high_resolution_clock::now() - mRunTime.start;
      auto second = duration_cast<seconds>(diff).count() % 60;
      auto minute = duration_cast<minutes>(diff).count() % 60;
      auto hour = duration_cast<hours>(diff).count();

      auto format = b::format(PROGRESS_FORMAT) % hour % minute % second % mReadoutCounter;
      mOptions.errorCheck ? format % mErrorCount : format % "n/a";
      format % mLastFillSize;
      mTemperatureMonitor.isValid() ? format % mTemperatureMonitor.getTemperature() : format % "n/a";
      cout << '\r' << format;

      if (mOptions.fifoDisplay) {
        for (int i = 0; i < 128; ++i) {
          if ((i % 8) == 0) {
            cout << '|';
          }
          cout << (mFifoAddress.user->statusEntries.at(i).isPageArrived() ? 'X' : ' ');
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

    void printStatusHeader()
    {
      auto line1 = b::format(PROGRESS_FORMAT_HEADER) % "Time" % "Pages" % "Errors" % "Fill" % "°C";
      auto line2 = b::format(PROGRESS_FORMAT) % "00" % "00" % "00" % '-' % '-' % '-' % '-';
      cout << '\n' << line1;
      cout << '\n' << line2;
      mLogStream << '\n' << line1;
      mLogStream << '\n' << line2;
    }

    bool isStatusDisplayInterval()
    {
      auto now = std::chrono::high_resolution_clock::now();
      if (std::chrono::duration_cast<std::chrono::milliseconds, int64_t>(now - mLastDisplayUpdate) > DISPLAY_INTERVAL) {
        mLastDisplayUpdate = now;
        return true;
      }
      return false;
    }

    bool isPageArrived(const Handle& handle)
    {
      return mFifoAddress.user->statusEntries[handle.descriptorIndex].isPageArrived();
    }

    uint32_t* getPageAddress(const Handle& handle)
    {
      return reinterpret_cast<uint32_t*>(mPageAddresses[handle.pageIndex].user);
    }

    void lowPriorityTasks()
    {
      // Handle a max temperature abort
      if (mTemperatureMonitor.isMaxExceeded()) {
        cout << "\n\n!!! ABORTING: MAX TEMPERATURE EXCEEDED\n";
        mDmaLoopBreak = true;
        return;
      }

      // Handle a SIGINT abort
      if (isSigInt()) {
        // We want to finish the readout cleanly if possible, so we stop pushing and try to wait a bit until the
        // queue is empty
        if (!mHandlingSigint) {
          mHandlingSigintStart = std::chrono::high_resolution_clock::now();
          mHandlingSigint = true;
          mPushEnabled = false;
        }

        if (mQueue.size() == 0) {
          // Finished readout cleanly
          cout << "\n\nInterrupted\n";
          mDmaLoopBreak = true;
          return;
        }

        if ((std::chrono::high_resolution_clock::now() - mHandlingSigintStart) > HANDLING_SIGINT_TIMEOUT) {
          // Timed out
          cout << "\n\nInterrupted (did not finish readout queue)\n";
          mDmaLoopBreak = true;
          return;
        }
      }

      // Status display updates
      if (isVerbose() && isStatusDisplayInterval()) {
        updateStatusDisplay();
      }

      // Random pauses
      if (mOptions.randomPauseSoft) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now >= mRandomPausesSoft.next) {
          cout << b::format("sw pause %-4d ms\n") % mRandomPausesSoft.length.count() << std::flush;
          std::this_thread::sleep_for(mRandomPausesSoft.length);

          // Schedule next pause
          auto now = std::chrono::high_resolution_clock::now();
          mRandomPausesSoft.next = now + std::chrono::milliseconds(getRandRange(NEXT_PAUSE_MIN, NEXT_PAUSE_MAX));
          mRandomPausesSoft.length = std::chrono::milliseconds(getRandRange(PAUSE_LENGTH_MIN, PAUSE_LENGTH_MAX));
        }
      }

      // Random pauses
      if (mOptions.randomPauseFirm) {
        auto now = std::chrono::high_resolution_clock::now();
        if (!mRandomPausesFirm.isPaused && now >= mRandomPausesFirm.next) {
          cout << b::format("fw pause %-4d ms\n") % mRandomPausesFirm.length.count() << std::flush;
          mPdaBar->getUserspaceAddressU32()[CruRegisterIndex::DATA_EMULATOR_CONTROL] = 0x1;
          mRandomPausesFirm.isPaused = true;
        }

        if (mRandomPausesFirm.isPaused && now >= mRandomPausesFirm.next + mRandomPausesFirm.length) {
          mPdaBar->getUserspaceAddressU32()[CruRegisterIndex::DATA_EMULATOR_CONTROL] = 0x3;
          mRandomPausesFirm.isPaused = false;

          // Schedule next pause
          auto now = std::chrono::high_resolution_clock::now();
          mRandomPausesFirm.next = now + std::chrono::milliseconds(getRandRange(NEXT_PAUSE_MIN, NEXT_PAUSE_MAX));
          mRandomPausesFirm.length = std::chrono::milliseconds(getRandRange(PAUSE_LENGTH_MIN, PAUSE_LENGTH_MAX));
        }
      }
    }

    bool shouldPushQueue()
    {
      return (mQueue.size() < NUM_PAGES) && (mInfinitePages || (mPushCounter < mOptions.maxPages)) && mPushEnabled;
    }

    void fillReadoutQueue()
    {
      mLastFillSize = 0;
      // Note: This should be more optimized later, since pages are always pushed in blocks of 4.
      while (shouldPushQueue()) {
        // Push page
        setDescriptor(mPageIndexCounter, mDescriptorCounter);

        // Add the page to the readout queue
        mQueue.push(Handle{mDescriptorCounter, mPageIndexCounter});

        // Increment counters
        mDescriptorCounter = (mDescriptorCounter + 1) % NUM_PAGES;
        mPageIndexCounter = (mPageIndexCounter + 1) % mPageAddresses.size();
        mPushCounter++;
        mLastFillSize++;
      }
    }

    bool readoutQueueHasPageAvailable()
    {
      return !mQueue.empty() && isPageArrived(mQueue.front());
    }

    void readoutPage(const Handle& handle)
    {
      // Read out to file
      if (mOptions.fileOutput) {
        printToFile(handle, mReadoutCounter);
      }

      // Data error checking
      if (mOptions.errorCheck) {
        if (mDataGeneratorCounter == -1) {
          // First page initializes the counter
          mDataGeneratorCounter = getPageAddress(handle)[0];
        }
        if (hasErrors(handle, mReadoutCounter, mDataGeneratorCounter) && mOptions.resyncCounter) {
          // Resync the counter
          mDataGeneratorCounter = getPageAddress(handle)[0];
        }
      }

      // Setting the buffer to the default value after the readout
      resetPage(getPageAddress(handle));

      // Reset status entry
      mFifoAddress.user->statusEntries[handle.descriptorIndex].reset();

      mDataGeneratorCounter += 256;
      mReadoutCounter++;
    }

    void runDma()
    {
      if (isVerbose()) {
        printStatusHeader();
      }

      mRunTime.start = std::chrono::high_resolution_clock::now();

      while (true) {
        // Check if we need to stop in the case of a page limit
        if (!mInfinitePages && mReadoutCounter >= mOptions.maxPages) {
          cout << "\n\nMaximum amount of pages reached\n";
          break;
        }

        // The loop break may be set because of interrupts, max temperature, etc.
        if (mDmaLoopBreak) {
          break;
        }

        // This stuff doesn't need to be run every cycle, so we reduce the overhead.
        if (mLowPriorityCounter >= LOW_PRIORITY_INTERVAL) {
          lowPriorityTasks();
          mLowPriorityCounter = 0;
        }
        mLowPriorityCounter++;

        // Keep the readout queue filled
        fillReadoutQueue();

        // Read out a page if available
        if (readoutQueueHasPageAvailable()) {
          readoutPage(mQueue.front());
          if (mReadoutCounter % PAGES_PER_PUSH == 0) {
            // Indicate to the firmware we've read out 4 pages
            mPdaBar->getUserspaceAddressU32()[CruRegisterIndex::SOFTWARE_BUFFER_READY] = 0x1;
          }
          mQueue.pop();
        }
      }

      mRunTime.end = std::chrono::high_resolution_clock::now();

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
      double runTime = std::chrono::duration<double>(mRunTime.end - mRunTime.start).count();
      double bytes = double(mReadoutCounter) * DMA_PAGE_SIZE;
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
      stream << format % "Pages" % mReadoutCounter;
      if (bytes > 0.00001) {
        stream << format % "Bytes" % bytes;
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

    void printToFile(const Handle& handle, int64_t pageNumber)
    {
      uint32_t* page = getPageAddress(handle);
      mReadoutStream << "Event #" << pageNumber << " Buffer #" << handle.pageIndex << '\n';
      int perLine = 8;
      for (int i = 0; i < DMA_PAGE_SIZE_32; i+= perLine) {
        for (int j = 0; j < perLine; ++j) {
          mReadoutStream << page[i+j] << ' ';
        }
        mReadoutStream << '\n';
      }
      mReadoutStream << '\n';
    }

    bool hasErrors(const Handle& handle, int64_t eventNumber, uint32_t counter)
    {
      /// The data emulator writes to every 8th 32-bit word
      constexpr uint32_t PATTERN_STRIDE = 8;
      /// The data emulator adds an offset of 2, apparently
      constexpr uint32_t PATTERN_OFFSET = 2;
      /// Pages are pushed in 4 consecutive patterns, i.e. the data emulator's counter resets every 4 pages
      constexpr uint32_t PATTERN_SECTIONS = 4;

      uint32_t* page = getPageAddress(handle);

      int64_t errorCountBefore = mErrorCount;

      auto reportError = [&](int i, uint32_t expectedValue, uint32_t actualValue) {
        mErrorCount++;
        if (isVerbose() && mErrorCount < MAX_RECORDED_ERRORS) {
          mErrorStream << "Error @ event:" << eventNumber << " page:" << handle.pageIndex << " i:" << i << " exp:"
              << expectedValue << " val:" << actualValue << '\n';
        }
      };

      for (uint32_t i = 0; i < DMA_PAGE_SIZE_32; i += PATTERN_STRIDE)
      {
        uint32_t expectedValue = counter + i/8;
        if (page[i] != expectedValue) {
          reportError(i, expectedValue, page[i]);
          return true;
        }
      }

      return mErrorCount > errorCountBefore;
    }

    void resetPage(uint32_t* page)
    {
      for (size_t i = 0; i < DMA_PAGE_SIZE_32; i++) {
        page[i] = BUFFER_DEFAULT_VALUE;
      }
    }

    /// Max amount of errors that are recorded into the error stream
    static constexpr int64_t MAX_RECORDED_ERRORS = 1000;

    /// Program options
    struct Options {
        int64_t maxPages; ///< Limit of pages to push
        bool fileOutput;
        bool resetCard;
        bool fifoDisplay;
        bool randomPauseSoft;
        bool randomPauseFirm;
        bool errorCheck;
        bool removeSharedMemory;
        bool reloadKernelModule;
        bool resyncCounter;
    } mOptions;

    /// A value of true means no limit on page pushing
    bool mInfinitePages;

    struct RunTime
    {
        TimePoint start; ///< Start of run time
        TimePoint end; ///< End of run time
    } mRunTime;

    /// Temperature monitor thread thing
    TemperatureMonitor mTemperatureMonitor;

    // PDA, buffer, etc stuff
    b::scoped_ptr<RorcDevice> mRorcDevice;
    b::scoped_ptr<Pda::PdaBar> mPdaBar;
    b::scoped_ptr<MemoryMappedFile> mMappedFilePages;
    b::scoped_ptr<Pda::PdaDmaBuffer> mBufferPages;

    /// Aliased userspace FIFO
    AddressSpaces<CruFifoTable> mFifoAddress;

    /// Amount of pages pushed
    int64_t mPushCounter = 0;

    /// Amount of pages read out
    int64_t mReadoutCounter = 0;

    /// Data generator counter
    int64_t mDataGeneratorCounter = -1;

    /// Indicates the offset should be reinitialized at the next page
//    bool mDataGeneratorOffsetNeedsResync = true;

    /// Indicates current descriptor
    int mDescriptorCounter = 0;

    /// Indicates current page in mPageAddresses
    int mPageIndexCounter = 0;

    /// Amount of data errors detected
    int64_t mErrorCount = 0;

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

    struct RandomPausesSoft
    {
        TimePoint next; ///< Next pause at this time
        std::chrono::milliseconds length; ///< Next pause has this length
    } mRandomPausesSoft;

    struct RandomPausesFirm
    {
        bool isPaused = false;
        TimePoint next; ///< Next pause at this time
        std::chrono::milliseconds length; ///< Next pause has this length
    } mRandomPausesFirm;

    /// Set when the DMA loop must be stopped (e.g. SIGINT, max temperature reached)
    bool mDmaLoopBreak = false;

    /// Indicates a SIGINT is being handled
    bool mHandlingSigint = false;

    /// Start time of SIGINT handling
    TimePoint mHandlingSigintStart;

    /// Enables / disables the pushing loop
    bool mPushEnabled = true;

    /// Counter for low priority checks in the loop
    int mLowPriorityCounter = 0;

    /// Low priority counter interval
    static constexpr int LOW_PRIORITY_INTERVAL = 10000;

    /// Queue for page handles
    ReadoutQueue mQueue = ReadoutQueue(QueueBuffer(NUM_PAGES));

    /// Amount of pages pushed in last queue fill. If this is ever greater than 1 during normal operation, it means
    /// we're not keeping the queue filled up all the time, i.e. not keeping up with the card.
    int mLastFillSize = 0;
};

} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCruExperimentalDma().execute(argc, argv);
}
