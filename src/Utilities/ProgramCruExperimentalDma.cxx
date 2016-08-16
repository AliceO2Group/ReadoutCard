///
/// \file ProgramCruExperimentalDma.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Port of https://gitlab.cern.ch/alice-cru/pciedma_eval
///


// Notes:
// Sometimes the card is not recognized properly as PCIe (check lspci), and the program will hang. Cause unknown.

#include "Utilities/Program.h"
#include <iostream>
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
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <ctype.h>
#include <time.h>
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

/// Use busy wait instead of condition variable (cv is very slow)
#define USE_BUSY_INTERRUPT_WAIT

namespace b = boost;
namespace bfs = boost::filesystem;
namespace Register = AliceO2::Rorc::CruRegisterIndex;
using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

namespace {

struct descriptor_entry
{
    uint32_t src_low;
    uint32_t src_high;
    uint32_t dst_low;
    uint32_t dst_high;
    uint32_t ctrl;
    uint32_t reservd1;
    uint32_t reservd2;
    uint32_t reservd3;
};

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

const boost::format PROGRESS_FORMAT("  %-5s %-8s %-8s %-8s %-8s %-8.1f");

constexpr int ROLLS_DEFAULT = 1500;

constexpr double MAX_TEMPERATURE = 45.0;

const std::string RESET_SWITCH = "reset";
const std::string TO_FILE_SWITCH = "tofile";
const std::string ROLLS_SWITCH = "rolls";
const std::string INTERRUPT_SWITCH = "interrupts";

/// Test class for PCI interrupts
class PdaIsr
{
  public:

    PdaIsr(PciDevice* pciDevice)
        : mPciDevice(pciDevice)
    {
      cout << "\nREGISTERING ISR\n";
      PciDevice_registerISR(pciDevice, PdaIsr::serviceRoutine, this);
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

    static uint64_t serviceRoutine(uint32_t sequenceNumber, const void* _this)
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

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("CRU EXPERIMENTAL DMA", "!!! USE WITH CAUTION !!!", "./rorc-cru-experimental-dma");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      options.add_options()(RESET_SWITCH.c_str(), "Reset card");
      options.add_options()(TO_FILE_SWITCH.c_str(), "Read out to file");
      options.add_options()(ROLLS_SWITCH.c_str(),
          boost::program_options::value<int>(&maxRolls)->default_value(ROLLS_DEFAULT),
          "Amount of iterations over the FIFO. Give 0 for infinite.");
      options.add_options()(INTERRUPT_SWITCH.c_str(), "Use PCIe interrupts");
    }

    virtual void mainFunction(const boost::program_options::variables_map& map)
    {
      using namespace AliceO2::Rorc;

      infiniteRolls = maxRolls == 0;
      enableResetCard = bool(map.count(RESET_SWITCH));
      enableFileOutput = bool(map.count(TO_FILE_SWITCH));
      enableInterrupts = bool(map.count(INTERRUPT_SWITCH));

      cout << "Initializing" << endl;
      init();

      cout << "Starting temperature monitor" << endl;
      temperatureMonitor.start(&bar[Register::TEMPERATURE]);

      cout << "Starting DMA test" << endl;
      runDma();
      temperatureMonitor.stop();
    }

  private:

    /// Max amount of errors that are recorded into the error stream
    static constexpr int64_t MAX_RECORDED_ERRORS = 1000;

    /// Array for data emulator pattern
    using Pattern = std::array<int, 1024>;

    /// Array the size of a page
    using PageBuffer = std::array<uint32_t, DMA_PAGE_SIZE_32>;

    /// Array of pages
    using PageBufferArray = std::array<PageBuffer, NUM_PAGES>;

    int maxRolls;
    int64_t errorCount;
    bool infiniteRolls; // true means no limit on rolling
    bool enableFileOutput;
    bool enableResetCard;
    bool enableInterrupts;

    TemperatureMonitor temperatureMonitor;

    b::scoped_ptr<RorcDevice> rorcDevice;
    b::scoped_ptr<Pda::PdaBar> pdaBar;
    b::scoped_ptr<PdaIsr> pdaIsr;
    b::scoped_ptr<MemoryMappedFile> mappedFilePages;
    b::scoped_ptr<Pda::PdaDmaBuffer> bufferPages;

    volatile uint32_t* bar;

    CruFifoTable* fifoUser;

    uint32_t* dataUser; ///< DMA buffer (userspace address)

    void init()
    {
      system("modprobe -r uio_pci_dma");
      system("modprobe uio_pci_dma");

      bfs::remove(DMA_BUFFER_PAGES_PATH);

      int serial = 12345;
      int channel = 0;
      Util::resetSmartPtr(rorcDevice, serial);
      Util::resetSmartPtr(pdaBar, rorcDevice->getPciDevice(), channel);
      bar = pdaBar->getUserspaceAddressU32();
      if (enableInterrupts) {
        Util::resetSmartPtr(pdaIsr, rorcDevice->getPciDevice());
      }
      Util::resetSmartPtr(mappedFilePages, DMA_BUFFER_PAGES_PATH.c_str(), DMA_BUFFER_PAGES_SIZE);
      Util::resetSmartPtr(bufferPages, rorcDevice->getPciDevice(), mappedFilePages->getAddress(),
          mappedFilePages->getSize(), BUFFER_INDEX_PAGES);

      auto entry = bufferPages->getScatterGatherList().at(0);
      if (entry.size < (2l * 1024l * 1024l)) {
        BOOST_THROW_EXCEPTION(CruException()
            << errinfo_rorc_error_message("Unsupported configuration: DMA scatter-gather entry size less than 2 MiB")
            << errinfo_rorc_scatter_gather_entry_size(entry.size)
            << errinfo_rorc_possible_causes(
                {"DMA buffer was not allocated in hugepage shared memory (hugetlbfs may not be properly mounted)"}));
      }

      if (enableResetCard) {
        cout << "Resetting..." << std::flush;
        bar[Register::RESET_CONTROL] = 0x1;
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
      fifoUser = reinterpret_cast<CruFifoTable*>(entry.addressUser);
      dataUser = getDataAddress(fifoUser);
      CruFifoTable* fifoDevice = reinterpret_cast<CruFifoTable*>(entry.addressBus);
      uint32_t* dataDevice = getDataAddress(fifoDevice);

      // Initializing the descriptor table
      fifoUser->resetStatusEntries();

      for (int i = 0; i < NUM_PAGES; i++) {
        auto sourceAddress = reinterpret_cast<void*>((i % NUM_OF_BUFFERS) * DMA_PAGE_SIZE);
        auto destinationAddress = dataDevice + i * DMA_PAGE_SIZE_32;
        fifoUser->descriptorEntries[i].setEntry(i, DMA_PAGE_SIZE_32, sourceAddress, destinationAddress);
      }

      // Setting the buffer to the default value

//      cout << "Data address user       : " << (void*) data_user << '\n';
      for (int i = 0; i < DMA_PAGE_SIZE_32 * FIFO_ENTRIES; i++) {
        dataUser[i] = BUFFER_DEFAULT_VALUE;
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

      // Status base address, low and high part, respectively
      bar[Register::STATUS_BASE_BUS_HIGH] = Util::getUpper32Bits(uint64_t(fifoDevice));
      bar[Register::STATUS_BASE_BUS_LOW] = Util::getLower32Bits(uint64_t(fifoDevice));

      // Destination (card's memory) addresses, low and high part, respectively
      bar[Register::STATUS_BASE_CARD_HIGH] = 0x0;
      bar[Register::STATUS_BASE_CARD_LOW] = 0x8000;

      // Set descriptor table size (must be size - 1)
      bar[Register::DESCRIPTOR_TABLE_SIZE] = NUM_PAGES - 1;

      // Send command to the DMA engine to write to every status entry, not just the final one
      bar[Register::DONE_CONTROL] = 0x1;

      // Send command to the DMA engine to write NUM_PAGES
      bar[Register::DMA_POINTER] = NUM_PAGES - 1;

      // make buffer ready signal
      bar[Register::BUFFER_READY] = 0x1;

      // Programming the user module to trigger the data emulator
      bar[Register::DATA_EMULATOR_CONTROL] = 0x1;

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      cout << "  Firmware version  " << Utilities::Common::make32hexString(bar[Register::FIRMWARE_COMPILE_INFO]) << '\n';
      cout << "  Serial number     " << Utilities::Common::make32hexString(bar[Register::SERIAL_NUMBER]) << '\n';
    }

    void runDma()
    {
      /// Pattern for error checking
      auto pattern = makePattern();

      int i = 0;
      int errorCheckPage = 0; ///< Used for error checking...
      int statusCount = 0;
      int64_t pageCounter = 0;
      int64_t rollingCounter = 0;

      // Allocating memory where the data will be read out to
      PageBufferArray readoutPages;

      if (isVerbose()) {
        auto format = PROGRESS_FORMAT;
        cout << '\n' << format % "i" % "Counter" % "Rolling" % "Status" % "Errors" % "°C";
        cout << '\n' << format % '-' % '-' % '-' % '-' % '-' % '-';
      }

      auto timeStart = std::chrono::high_resolution_clock::now();
      auto displayInterval = std::chrono::milliseconds(10);
      auto lastDisplayUpdate = timeStart - displayInterval;

      // Update the status display every 100 milliseconds
      auto isDisplayInterval = [&displayInterval, & lastDisplayUpdate](){
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds, int64_t>(now - lastDisplayUpdate) > displayInterval) {
          lastDisplayUpdate = now;
          return true;
        }
        return false;
      };

      auto updateStatusDisplay = [&](){
        // Stats
        auto format = b::format(PROGRESS_FORMAT) % i % pageCounter % rollingCounter % statusCount % errorCount;
        if (temperatureMonitor.isValid()) {
          format % temperatureMonitor.getTemperature();
        } else {
          format % "n/a";
        }
        cout << '\r' << format;

        // FIFO status
//        cout << "  ";
//        for (int i = 0; i < fifoUser->statusEntries.size(); ++i) {
//          if ((i % 8) == 0) {
//            cout << "|";
//          }
//          cout << (ffoUser->statusEntries[i].isPageArrived() ? 'X' : '.');
//        }
//        cout << "|";
      };

      std::ofstream fileStream;
      std::ostringstream errorStream;
      if (enableFileOutput) {
        fileStream.open("readout_data.txt");
      }

      while (true) {
        // Interrupts are skipped on first iteration, because it would always time out otherwise
        if (enableInterrupts && pageCounter != 0
            && (pdaIsr->waitOnInterrupt(std::chrono::milliseconds(100)) == std::cv_status::timeout)) {
          cout << "Interrupt wait TIMED OUT at counter:" << pageCounter << " rolling:" << rollingCounter << "\n";
        }

        if (fifoUser->statusEntries[i].isPageArrived()) {
          fifoUser->statusEntries[i].reset();

          uint32_t* pushedPage = &dataUser[i*DMA_PAGE_SIZE_32];
          auto& readoutPage = readoutPages[i];

          // Copy page
          copyPage(readoutPage.data(), pushedPage);

          // Setting the buffer to the default value after the readout
          resetPage(pushedPage);

          // Read out to file
          if (enableFileOutput) {
            fileOutput(fileStream, i , readoutPage);
          }

          // Data error checking
          checkErrors(errorStream, i, errorCheckPage, rollingCounter, readoutPage, pattern);
          errorCheckPage++;

          // points to the first descriptor
          if (i == 0) {
            bar[Register::DMA_POINTER] = NUM_PAGES - 1;
          }

          // Push the same descriptor again when its gets executed
          // TODO Not sure why this was done, it seems to have a bad effect on performance (cuts it in half)
          //bar[Register::DMA_POINTER] = i;

          pageCounter++;

          // After every 32Kbytes reset the current_page for online checking
          if ((i % FIFO_ENTRIES) == FIFO_ENTRIES - 1) {
            errorCheckPage = 0;
            rollingCounter++;
          }

          if (i == NUM_PAGES - 1) {
            // read status count after 1MByte dma transfer
            statusCount = bar[Register::READ_STATUS_COUNT];
            i = 0;
          } else {
            i++;
          }
        }

        if (!infiniteRolls && rollingCounter >= maxRolls) {
          cout << "\n\nMaximum amount of rolls reached\n";
          break;
        }
        else if (temperatureMonitor.isMaxExceeded()) {
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

      auto timeEnd = std::chrono::high_resolution_clock::now();

      // Error output
      {
        auto errorStr = errorStream.str();

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

        std::ofstream stream("readout_errors.txt");
        stream << errorStr;
      }

      // Calculating throughput
      double runTime = std::chrono::duration<double>(timeEnd - timeStart).count();
      double bytes = double(rollingCounter) * FIFO_ENTRIES * DMA_PAGE_SIZE;
      double GB = bytes / (1000 * 1000 * 1000);
      double GBs = GB / runTime;
      double Gbs = GBs * 8.0;
      double GiB = bytes / (1024 * 1024 * 1024);
      double GiBs = GiB / runTime;
      double Gibs = GiBs * 8.0;

      auto format = b::format("  %-10s  %-10s\n");
      cout << '\n';
      cout << format % "Seconds" % runTime;
      cout << format % "Bytes" % bytes;
      if (bytes > 0.00001) {
        cout << format % "GB" % GB;
        cout << format % "GB/s" % GBs;
        cout << format % "Gb/s" % Gbs;
        cout << format % "GiB" % GiB;
        cout << format % "GiB/s" % GiBs;
        cout << format % "Gibit/s" % Gibs;
        cout << format % "Errors" % errorCount;
      }
      cout << '\n';
    }

    Pattern makePattern()
    {
      Pattern pattern;
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
        const PageBuffer& page, const Pattern& pattern)
    {
      for (int j = 0; j < (DMA_PAGE_SIZE_32 / 8); j++) {
        size_t pattern_index = errorCheckPage * (DMA_PAGE_SIZE_32 / 8) + j;
        if (page[j * 8] != pattern[pattern_index]) {
          errorCount++;
          if (isVerbose() && errorCount < MAX_RECORDED_ERRORS) {
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
};

} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCruExperimentalDma().execute(argc, argv);
}
