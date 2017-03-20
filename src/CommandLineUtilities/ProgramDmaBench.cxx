/// \file ProgramDmaBench.cxx
///
/// \brief Utility that tests RORC DMA performance
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)


#include <chrono>
#include <iomanip>
#include <iostream>
#include <future>
#include <fstream>
#include <random>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <boost/algorithm/string/trim.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/format.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include "Cru/CruRegisterIndex.h"
#include "ExceptionLogging.h"
#include "InfoLogger/InfoLogger.hxx"
#include "folly/ProducerConsumerQueue.h"
#include "RORC/ChannelFactory.h"
#include "RORC/MemoryMappedFile.h"
#include "RORC/Parameters.h"
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/System.h"
#include "Utilities/Thread.h"
#include "Utilities/Util.h"

using namespace AliceO2::Rorc::CommandLineUtilities;
using namespace AliceO2::Rorc;
using namespace AliceO2::InfoLogger;
using std::cout;
using std::endl;
using namespace std::literals;
namespace b = boost;
namespace po = boost::program_options;
namespace bfs = boost::filesystem;

using MasterSharedPtr = ChannelMasterInterface::MasterSharedPtr;

namespace {
constexpr auto endm = InfoLogger::StreamOps::endm;

/// Determines how often the status display refreshes
constexpr auto DISPLAY_INTERVAL = 10ms;

/// Max amount of errors that are recorded into the error stream
constexpr int64_t MAX_RECORDED_ERRORS = 1000;

/// Minimum random pause interval in milliseconds
constexpr int NEXT_PAUSE_MIN = 10;
/// Maximum random pause interval in milliseconds
constexpr int NEXT_PAUSE_MAX = 2000;
/// Minimum random pause in milliseconds
constexpr int PAUSE_LENGTH_MIN = 1;
/// Maximum random pause in milliseconds
constexpr int PAUSE_LENGTH_MAX = 500;
// Low priority counter interval
constexpr int LOW_PRIORITY_INTERVAL = 10000;

constexpr uint32_t BUFFER_DEFAULT_VALUE = 0xCcccCccc;

/// The data emulator writes to every 8th 32-bit word
constexpr uint32_t PATTERN_STRIDE = 8;

/// Fields: Time(hour:minute:second), Pages pushed, Pages read, Errors, °C
const std::string PROGRESS_FORMAT_HEADER("  %-8s   %-12s  %-12s  %-12s  %-5.1f");

/// Fields: Time(hour:minute:second), Pages pushed, Pages read, Errors, °C
const std::string PROGRESS_FORMAT("  %02s:%02s:%02s   %-12s  %-12s  %-12s  %-5.1f");

auto READOUT_ERRORS_PATH = "readout_errors.txt";

constexpr size_t SUPERPAGE_SIZE = 2*1024*1024;
}

class BarHammer : public Utilities::Thread
{
  public:
    void start(const std::shared_ptr<ChannelMasterInterface>& channelIn)
    {
      mChannel = channelIn;
      Thread::start([&](std::atomic<bool>* stopFlag) {
        auto channel = mChannel;

        if (channel->getCardType() != CardType::Cru) {
          cout << "BarHammer only supported for CRU\n";
          return;
        }

        if (!channel) {
          return;
        }

        int64_t hammerCount = 0;
        uint32_t writeCounter = 0;
        while (!stopFlag->load() && !Program::isSigInt()) {
          for (int i = 0; i < getMultiplier(); ++i) {
            channel->writeRegister(CruRegisterIndex::DEBUG_READ_WRITE, writeCounter);
            writeCounter++;
          }
          hammerCount++;
        }
        mHammerCount = hammerCount;
      });
    }

    double getCount()
    {
      return double(mHammerCount.load()) * double(getMultiplier());
    }

  private:
    std::shared_ptr<ChannelMasterInterface> mChannel;
    std::atomic<int64_t> mHammerCount;

    int64_t getMultiplier()
    {
      return 10000;
    }
};

class ProgramDmaBench: public Program
{
  public:

    virtual Description getDescription()
    {
      return {"DMA Benchmark", "Test RORC DMA performance", "./rorc-dma-bench --id=12345 --channel=0"};
    }

    virtual void addOptions(po::options_description& options)
    {
      Options::addOptionChannel(options);
      Options::addOptionCardId(options);
      options.add_options()
          ("pages",
              po::value<int64_t>(&mOptions.maxPages)->default_value(1500),
              "Amount of pages to transfer. Give <= 0 for infinite.")
          ("buffer-size",
              po::value<std::string>(&mOptions.bufferSizeString)->default_value("10MB"),
              "Buffer size in GB or MB (MB rounded down to 2 MB multiple, min 10 MB). If MB is given, 2 MB hugepages "
              "will be used; If GB is given, 1 GB hugepages will be used.")
          ("reset",
              po::bool_switch(&mOptions.resetChannel),
              "Reset channel during initialization")
          ("to-file-ascii",
              po::bool_switch(&mOptions.fileOutputAscii),
              "Read out to file in ASCII format")
          ("to-file-bin",
              po::bool_switch(&mOptions.fileOutputBin),
              "Read out to file in binary format (only contains raw data from pages)")
//          ("rand-pause-sw",
//              po::bool_switch(&mOptions.randomPauseSoft),
//              "Randomly pause readout using software method")
          ("no-errorcheck",
              po::bool_switch(&mOptions.noErrorCheck),
              "Skip error checking")
          ("pattern",
              po::value<std::string>(&mOptions.generatorPatternString),
              "Error check with given pattern [INCREMENTAL, ALTERNATING, CONSTANT, RANDOM]")
          ("no-resync",
              po::bool_switch(&mOptions.noResyncCounter),
              "Disable counter resync")
          ("page-reset",
              po::bool_switch(&mOptions.pageReset),
              "Reset page to default values after readout (slow)")
          ("bar-hammer",
              po::bool_switch(&mOptions.barHammer),
              "Stress the BAR with repeated writes and measure performance")
          ("rm-pages-file",
              po::bool_switch(&mOptions.removePagesFile)->default_value(true),
              "Remove the file used for pages after benchmark completes")
              ;
      Options::addOptionsChannelParameters(options);
    }

    virtual void run(const po::variables_map& map)
    {
      if (mOptions.fileOutputAscii && mOptions.fileOutputBin) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("File output can't be both ASCII and binary"));
      }
      if (mOptions.fileOutputAscii) {
        mReadoutStream.open("readout_data.txt");
      }
      if (mOptions.fileOutputBin) {
        mReadoutStream.open("readout_data.bin", std::ios::binary);
      }

      if (!mOptions.generatorPatternString.empty()) {
        mOptions.generatorPattern = GeneratorPattern::fromString(mOptions.generatorPatternString);
      }

      // Parse buffer size
      {
        const auto& input = mOptions.bufferSizeString;
        if (input.size() < 3) {
          BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Invalid buffer size given"));
        }

        try {
          auto unit = input.substr(input.size() - 2); // Get last two characters
          auto value = boost::lexical_cast<size_t>(input.substr(0, input.size() - 2));

          if (unit == "MB") {
            mOptions.hugePageSize = HugePageSize::SIZE_2MB;
            value = std::max(size_t(10), value);
            mBufferSize = (value - (value % 2)) * 1024 * 1024; // Round down for 2M hugepages
          } else if (unit == "GB") {
            mOptions.hugePageSize = HugePageSize::SIZE_1GB;
            mBufferSize = value * 1024 * 1024 * 1024;
          } else {
            BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Invalid buffer size unit given"));
          }
        } catch (const boost::bad_lexical_cast& e) {
          BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Invalid buffer size argument"));
        }
      }

      mInfinitePages = (mOptions.maxPages <= 0);

      auto cardId = Options::getOptionCardId(map);
      int channelNumber = Options::getOptionChannel(map);
      auto params = Options::getOptionsParameterMap(map);


      // Create buffer
      mBufferFilePath = boost::str(
          boost::format("/var/lib/hugetlbfs/global/pagesize-%s/rorc-dma-bench_id=%s_chan=%s_pages")
              % (mOptions.hugePageSize == HugePageSize::SIZE_2MB ? "2MB" : "1GB") % map["id"].as<std::string>()
              % channelNumber);
      cout << "Using buffer file path: " << mBufferFilePath << '\n';
      Utilities::resetSmartPtr(mMemoryMappedFile, mBufferFilePath, mBufferSize, mOptions.removePagesFile);
      mBufferBaseAddress = reinterpret_cast<uintptr_t>(mMemoryMappedFile->getAddress());

      // Set up channel parameters
      mPageSize = params.getDmaPageSize().get();
      params.setCardId(cardId);
      params.setChannelNumber(channelNumber);
      params.setGeneratorDataSize(mPageSize);
      params.setGeneratorPattern(mOptions.generatorPattern);
      params.setBufferParameters(BufferParameters::Memory { mMemoryMappedFile->getAddress(),
          mMemoryMappedFile->getSize() });

      // Get master lock on channel
      mChannel = ChannelFactory().getMaster(params);
      mCardType = mChannel->getCardType();

      if (mOptions.resetChannel) {
        cout << "Resetting channel...";
        mChannel->resetChannel(ResetLevel::Rorc);
        cout << " done!\n";
      }

      cout << "### Starting benchmark" << endl;

      mChannel->startDma();

      if (mOptions.barHammer) {
        if (mChannel->getCardType() != CardType::Cru) {
          BOOST_THROW_EXCEPTION(ParameterException()
              << ErrorInfo::Message("BarHammer option currently only supported for CRU\n"));
        }
        Utilities::resetSmartPtr(mBarHammer);
        mBarHammer->start(mChannel);
      }

      mRunTime.start = std::chrono::high_resolution_clock::now();

      if (mOptions.randomReadout) {
        dmaLoopReadoutRandom();
      } else {
        dmaLoop();
      }
      mRunTime.end = std::chrono::high_resolution_clock::now();

      if (mBarHammer) {
        mBarHammer->join();
      }

      freeExcessPages(10ms);
      mChannel->stopDma();

      outputErrors();
      outputStats();

      cout << "### Benchmark complete" << endl;
    }

  private:

    void dmaLoop()
    {
      auto indexToOffset = [&](int i){ return i * SUPERPAGE_SIZE; };
      const int maxSuperpages = mBufferSize / SUPERPAGE_SIZE;
      const int pagesPerSuperpage = SUPERPAGE_SIZE / mPageSize;

      folly::ProducerConsumerQueue<size_t> freeQueue {maxSuperpages + 1};
      for (int i = 0; i < maxSuperpages; ++i) {
        if (!freeQueue.write(indexToOffset(i))) {
          BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Something went horribly wrong"));
        }
      }

      folly::ProducerConsumerQueue<size_t> readoutQueue {maxSuperpages + 1};

      auto isStopDma = [&]{ return mDmaLoopBreak.load(std::memory_order_relaxed); };

      // Readout thread
      auto readoutFuture = std::async(std::launch::async, [&]{
        while (!isStopDma()) {
          if (!mInfinitePages && mReadoutCount.load(std::memory_order_relaxed) >= mOptions.maxPages) {
            mDmaLoopBreak = true;
            break;
          }

          size_t offset;
          if (readoutQueue.read(offset)) {
            // Read out pages
            int pages = SUPERPAGE_SIZE / mPageSize;
            for (int i = 0; i < pages; ++i) {
              auto readoutCount = mReadoutCount.fetch_add(1, std::memory_order_relaxed);
              readoutPage(mBufferBaseAddress + offset + i * mPageSize, mPageSize, readoutCount);
            }

            // Page has been read out
            // Add superpage back to free queue
            if (!freeQueue.write(offset)) {
              BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Something went horribly wrong"));
            }
          }
        }
      });

      auto pushFuture = std::async(std::launch::async, [&]{
        while (!isStopDma()) {
          // Check if we need to stop in the case of a page limit
          if (!mInfinitePages && mPushCount.load(std::memory_order_relaxed) >= mOptions.maxPages) {
            break;
          }

          // Keep the driver's queue filled
          mChannel->fillSuperpages();

          // Give free superpages to the driver
          while (mChannel->getSuperpageQueueAvailable() != 0) {
            Superpage superpage;
            if (freeQueue.read(superpage.offset)) {
              superpage.size = SUPERPAGE_SIZE;
              mChannel->pushSuperpage(superpage);
            } else {
              break;
            }
          }

          // Check for filled superpages
          if (mChannel->getSuperpageQueueCount() > 0) {
            auto superpage = mChannel->getSuperpage();
            if (superpage.isFilled()) {
              if (readoutQueue.write(superpage.getOffset())) {
                // Move full superpage to readout queue
                mPushCount.fetch_add(pagesPerSuperpage, std::memory_order_relaxed);
                mChannel->popSuperpage();
              }
            }
          }
        }
      });

      // Main thread for low priority tasks
      while (!isStopDma()) {
        // Note: these low priority tasks are not run on every cycle, to reduce overhead
        lowPriorityTasks();
      }

      pushFuture.get();
      readoutFuture.get();
    }

    void dmaLoopReadoutRandom()
    {
//      constexpr int READOUT_THRESHOLD = 200; ///< Amount of pages to "cache" before reading out randomly
//      std::default_random_engine generator;
//      std::vector<PageSharedPtr> pages;
//
//      while (!mDmaLoopBreak) {
//        // Check if we need to stop in the case of a page limit
//        if (!mInfinitePages && mReadoutCount >= mOptions.maxPages) {
//          mDmaLoopBreak = true;
//          cout << "\n\nMaximum amount of pages reached\n";
//          break;
//        }
//
//        // Note: these low priority tasks are not run on every cycle, to reduce overhead
//        lowPriorityTasks();
//
//        // Keep the readout queue filled
//        mPushCount += mChannel->fillFifo();
//
//        // Read out a page if available
//        if (auto sharedPage = ChannelMasterInterface::popPage(mChannel)) {
//          pages.push_back(sharedPage);
//        }
//
//        if (pages.size() > READOUT_THRESHOLD) {
//          std::uniform_int_distribution<size_t> distribution(0, pages.size() - 1);
//          size_t index = distribution(generator);
//
//          readoutPage(pages[index]);
//
//          pages.erase(pages.begin() + index);
//          mReadoutCount++;
//        }
//      }
    }

    /// Free the pages that were pushed in excess
    void freeExcessPages(std::chrono::milliseconds timeout)
    {
      auto start = std::chrono::high_resolution_clock::now();
      int popped = 0;
      while ((std::chrono::high_resolution_clock::now() - start) < timeout) {
        if (mChannel->getSuperpageQueueCount() > 0) {
          auto superpage = mChannel->getSuperpage();
          if (superpage.isFilled()) {
            mChannel->popSuperpage();
            popped += superpage.getReceived() / mPageSize;
          }
        }
      }
      cout << "Popped " << popped << " excess pages\n";
    }

    uint32_t getEventNumber(uintptr_t pageAddress)
    {
      auto page = reinterpret_cast<void*>(pageAddress);
      uint32_t eventNumber = 0;
      memcpy(&eventNumber, page, sizeof(eventNumber));
      return eventNumber;
    }

    void readoutPage(uintptr_t pageAddress, size_t pageSize, int64_t readoutCount)
    {
      // Read out to file
      if (mOptions.fileOutputAscii || mOptions.fileOutputBin) {
        printToFile(pageAddress, pageSize, readoutCount);
      }

      // Data error checking
      if (!mOptions.noErrorCheck) {
        if (mDataGeneratorCounter == -1) {
          // First page initializes the counter
          mDataGeneratorCounter = getEventNumber(pageAddress);
        }

        bool hasError = checkErrors(pageAddress, pageSize, readoutCount, mDataGeneratorCounter);
        if (hasError && !mOptions.noResyncCounter) {
          // Resync the counter

          switch (mCardType) {
            case CardType::Crorc:
              mDataGeneratorCounter = getEventNumber(pageAddress);
              break;
            case CardType::Cru:
              mDataGeneratorCounter = getEventNumber(pageAddress) / 256;
              break;
            default: throw std::runtime_error("Error checking unsupported for this card type");
          }

          mDataGeneratorCounter = getEventNumber(pageAddress);
        }
      }

      if (mOptions.pageReset) {
        // Set the buffer to the default value after the readout
        resetPage(pageAddress, pageSize);
      }

      mDataGeneratorCounter++;
    }

    bool checkErrorsCru(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, uint32_t counter)
    {
      auto check = [&](auto patternFunction) {
        auto page = reinterpret_cast<const volatile uint32_t*>(pageAddress);
        auto pageSize32 = pageSize / sizeof(int32_t);
        for (uint32_t i = 0; i < pageSize32; i += PATTERN_STRIDE)
        {
          uint32_t expectedValue = patternFunction(i);
          uint32_t actualValue = page[i];
          if (actualValue != expectedValue) {
            // Report error
            mErrorCount++;
            if (isVerbose() && mErrorCount < MAX_RECORDED_ERRORS) {
              mErrorStream << boost::format("event:%d i:%d cnt:%d exp:0x%x val:0x%x\n")
                  % eventNumber % i % counter % expectedValue % actualValue;
            }
            return true;
          }
        }
        return false;
      };

      switch (mOptions.generatorPattern) {
        case GeneratorPattern::Incremental: return check([&](uint32_t i) { return counter * 256 + i / 8; });
        case GeneratorPattern::Alternating: return check([&](uint32_t)   { return 0xa5a5a5a5; });
        case GeneratorPattern::Constant:    return check([&](uint32_t)   { return 0x12345678; });
        default: ;
      }

      BOOST_THROW_EXCEPTION(Exception()
          << ErrorInfo::Message("Unsupported pattern for CRU error checking")
          << ErrorInfo::GeneratorPattern(mOptions.generatorPattern));
    }

    bool checkErrorsCrorc(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, uint32_t counter)
    {
      auto check = [&](auto patternFunction) {
        auto page = reinterpret_cast<const volatile uint32_t*>(pageAddress);
        auto pageSize32 = pageSize / sizeof(int32_t);

        for (uint32_t i = 0; i < pageSize32; ++i) {
          uint32_t expectedValue = (i == 0) ? counter : patternFunction(i);
          uint32_t actualValue = page[i];
          if (actualValue != expectedValue) {
            // Report error
            mErrorCount++;
            if (isVerbose() && mErrorCount < MAX_RECORDED_ERRORS) {
              mErrorStream << boost::format("event:%d i:%d cnt:%d exp:0x%x val:0x%x\n")
                  % eventNumber % i % counter % expectedValue % actualValue;
            }
            return true;
          }
        }
        return false;
      };

      switch (mOptions.generatorPattern) {
        case GeneratorPattern::Incremental: return check([&](uint32_t i) { return i - 1; });
        case GeneratorPattern::Alternating: return check([&](uint32_t)   { return 0xa5a5a5a5; });
        case GeneratorPattern::Constant:    return check([&](uint32_t)   { return 0x12345678; });
        default: ;
      }

      BOOST_THROW_EXCEPTION(Exception()
          << ErrorInfo::Message("Unsupported pattern for C-RORC error checking")
          << ErrorInfo::GeneratorPattern(mOptions.generatorPattern));
    }

    /// Checks and reports errors
    bool checkErrors(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, uint32_t counter)
    {
      switch (mCardType) {
        case CardType::Crorc: return checkErrorsCrorc(pageAddress, pageSize, eventNumber, counter);
        case CardType::Cru: return checkErrorsCru(pageAddress, pageSize, eventNumber, counter);
        default: throw std::runtime_error("Error checking unsupported for this card type");
      }
    }

    void resetPage(uintptr_t pageAddress, size_t pageSize)
    {
      auto page = reinterpret_cast<volatile uint32_t*>(pageAddress);
      auto pageSize32 = pageSize / sizeof(uint32_t);
      for (size_t i = 0; i < pageSize32; i++) {
        page[i] = BUFFER_DEFAULT_VALUE;
      }
    }

    void lowPriorityTasks()
    {
      // This stuff doesn't need to be run every cycle, so we reduce the overhead.
      if (mLowPriorityCount < LOW_PRIORITY_INTERVAL) {
        mLowPriorityCount++;
        return;
      }
      mLowPriorityCount = 0;

      // Handle a SIGINT abort
      if (isSigInt()) {
        // We want to finish the readout cleanly if possible, so we stop pushing and try to wait a bit until the
        // queue is empty
        cout << "\n\nInterrupted\n";
        mDmaLoopBreak = true;
        return;
      }

      // Status display updates
      if (isVerbose() && isStatusDisplayInterval()) {
        updateStatusDisplay();
      }

      // Random pauses in software: a thread sleep
      if (mOptions.randomPauseSoft) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now >= mRandomPausesSoft.next) {
          cout << b::format("sw pause %-4d ms\n") % mRandomPausesSoft.length.count() << std::flush;
          std::this_thread::sleep_for(mRandomPausesSoft.length);

          // Schedule next pause
          auto now = std::chrono::high_resolution_clock::now();
          mRandomPausesSoft.next = now + std::chrono::milliseconds(Utilities::getRandRange(NEXT_PAUSE_MIN, NEXT_PAUSE_MAX));
          mRandomPausesSoft.length = std::chrono::milliseconds(Utilities::getRandRange(PAUSE_LENGTH_MIN, PAUSE_LENGTH_MAX));
        }
      }
    }

    void updateStatusDisplay()
     {
      if (!mHeaderPrinted) {
        printStatusHeader();
        mHeaderPrinted = true;
      }

       using namespace std::chrono;
       auto diff = high_resolution_clock::now() - mRunTime.start;
       auto second = duration_cast<seconds>(diff).count() % 60;
       auto minute = duration_cast<minutes>(diff).count() % 60;
       auto hour = duration_cast<hours>(diff).count();

       auto format = b::format(PROGRESS_FORMAT);
       format % hour % minute % second; // Time
       format % mPushCount.load(std::memory_order_relaxed);
       format % mReadoutCount.load(std::memory_order_relaxed);

       mOptions.noErrorCheck ? format % "n/a" : format % mErrorCount; // Errors

       if (auto temperature = mChannel->getTemperature()) {
         format % *temperature;
       } else {
         format % "n/a";
       }

       cout << '\r' << format;

       // This takes care of adding a "line" to the stdout every so many seconds
       {
         constexpr int interval = 60;
         auto second = duration_cast<seconds>(diff).count() % interval;
         if (mDisplayUpdateNewline && second == 0) {
           cout << '\n';
           mDisplayUpdateNewline = false;
         }
         if (second >= 1) {
           mDisplayUpdateNewline = true;
         }
       }
     }

     void printStatusHeader()
     {
       auto line1 = b::format(PROGRESS_FORMAT_HEADER) % "Time" % "Pushed" % "Read" % "Errors" % "°C";
       auto line2 = b::format(PROGRESS_FORMAT) % "00" % "00" % "00" % '-' % '-' % '-' % '-';
       cout << '\n' << line1;
       cout << '\n' << line2;
     }

     bool isStatusDisplayInterval()
     {
       auto now = std::chrono::high_resolution_clock::now();
       if (std::chrono::duration_cast<std::chrono::milliseconds, int64_t>(now - mLastDisplayUpdate)
           > DISPLAY_INTERVAL) {
         mLastDisplayUpdate = now;
         return true;
       }
       return false;
     }

     void outputStats()
     {
       // Calculating throughput
       double runTime = std::chrono::duration<double>(mRunTime.end - mRunTime.start).count();
       double bytes = double(mReadoutCount.load()) * mPageSize;
       double GB = bytes / (1000 * 1000 * 1000);
       double GBs = GB / runTime;
       double Gbs = GBs * 8;

       auto put = [&](auto label, auto value) { cout << b::format("  %-10s  %-10s\n") % label % value; };
       cout << '\n';
       put("Seconds", runTime);
       put("Pages", mReadoutCount.load());
       if (bytes > 0.00001) {
         put("Bytes", bytes);
         put("GB", GB);
         put("GB/s", GBs);
         put("Gb/s", Gbs);
         if (mOptions.noErrorCheck) {
           put("Errors", "n/a");
         } else {
           put("Errors", mErrorCount);
         }
       }

       if (mOptions.barHammer) {
         size_t writeSize = sizeof(uint32_t);
         double hammerCount = mBarHammer->getCount();
         double bytes = hammerCount * writeSize;
         double MB = bytes / (1000 * 1000);
         double MBs = MB / runTime;
         put("BAR writes", hammerCount);
         put("BAR write size (bytes)", writeSize);
         put("BAR MB", MB);
         put("BAR MB/s", MBs);
       }

       cout << '\n';
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

    void printToFile(uintptr_t pageAddress, size_t pageSize, int64_t pageNumber)
    {
      auto page = reinterpret_cast<const volatile uint32_t*>(pageAddress);
      auto pageSize32 = pageSize / sizeof(int32_t);

      if (mOptions.fileOutputAscii) {
        mReadoutStream << "Event #" << pageNumber << '\n';
        int perLine = 8;

        for (int i = 0; i < pageSize32; i += perLine) {
          for (int j = 0; j < perLine; ++j) {
            mReadoutStream << page[i + j] << ' ';
          }
          mReadoutStream << '\n';
        }
        mReadoutStream << '\n';
      } else if (mOptions.fileOutputBin) {
        // TODO Is there a more elegant way to write from volatile memory?
        mReadoutStream.write(reinterpret_cast<const char*>(pageAddress), pageSize);
      }
    }

    enum class HugePageSize
    {
       SIZE_2MB, SIZE_1GB
    };

    /// Program options
    struct OptionsStruct {
        int64_t maxPages = 0; ///< Limit of pages to push
        bool fileOutputAscii = false;
        bool fileOutputBin = false;
        bool resetChannel = false;
        bool randomPauseSoft = false;
        bool noErrorCheck = false;
        bool pageReset = false;
        bool noResyncCounter = false;
        bool randomReadout = false;
        bool barHammer = false;
        bool removePagesFile = false;
        bool delayReadout = false;
        std::string generatorPatternString;
        std::string bufferSizeString;
        HugePageSize hugePageSize;
        GeneratorPattern::type generatorPattern = GeneratorPattern::Incremental;
    } mOptions;

//    BufferParameters::File mBufferParameters;

    std::atomic<bool> mDmaLoopBreak {false};
    bool mInfinitePages = false;
    std::atomic<int64_t> mPushCount { 0 };
    std::atomic<int64_t> mReadoutCount { 0 };
    int64_t mErrorCount = 0;
    int64_t mDataGeneratorCounter = -1;
    int mLowPriorityCount = 0;

    std::unique_ptr<MemoryMappedFile> mMemoryMappedFile;

    /// Stream for file readout, only opened if enabled by the --file program options
    std::ofstream mReadoutStream;

    /// Stream for error output
    std::ostringstream mErrorStream;

    using TimePoint = std::chrono::high_resolution_clock::time_point;

    struct RunTime
    {
        TimePoint start; ///< Start of run time
        TimePoint end; ///< End of run time
    } mRunTime;

    /// Was the header printed?
    bool mHeaderPrinted = false;

    /// Time of the last display update
    TimePoint mLastDisplayUpdate;

    /// Indicates the display must add a newline to the table
    bool mDisplayUpdateNewline;

    /// Enables / disables the pushing loop
    bool mPushEnabled = true;

    struct RandomPausesSoft
    {
        TimePoint next; ///< Next pause at this time
        std::chrono::milliseconds length; ///< Next pause has this length
    } mRandomPausesSoft;

    size_t mPageSize;

    std::unique_ptr<BarHammer> mBarHammer;

    std::string mBufferFilePath;

    size_t mBufferSize;

    uintptr_t mBufferBaseAddress;

    CardType::type mCardType;

    std::shared_ptr<ChannelMasterInterface> mChannel;
};

int main(int argc, char** argv)
{
  return ProgramDmaBench().execute(argc, argv);
}
