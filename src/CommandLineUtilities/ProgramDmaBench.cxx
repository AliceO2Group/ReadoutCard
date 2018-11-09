/// \file ProgramDmaBench.cxx
///
/// \brief Utility that tests ReadoutCard DMA performance
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)


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
#include <boost/circular_buffer.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/tokenizer.hpp>
#include "BarHammer.h"
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Common/Iommu.h"
#include "Common/SuffixOption.h"
#include "Cru/DataFormat.h"
#include "ExceptionInternal.h"
#include "InfoLogger/InfoLogger.hxx"
#include "folly/ProducerConsumerQueue.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/MemoryMappedFile.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ReadoutCard.h"
#include "time.h"
#include "Utilities/Hugetlbfs.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/Util.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
using AliceO2::Common::SuffixOption;
using std::cout;
using std::endl;
using namespace std::literals;
namespace b = boost;
namespace po = boost::program_options;

namespace {
/// Initial value for link counters
constexpr auto LINK_COUNTER_INITIAL_VALUE = std::numeric_limits<uint32_t>::max();
/// Maximum supported links
constexpr auto MAX_LINKS = 32;
/// Interval for low priority thread (display updates, etc)
constexpr auto LOW_PRIORITY_INTERVAL = 10ms;
/// Buffer value to reset to
constexpr uint32_t BUFFER_DEFAULT_VALUE = 0xCcccCccc;
/// Fields: Time(hour:minute:second), Pages pushed, Pages read, Errors, °C
const std::string PROGRESS_FORMAT_HEADER("  %-8s   %-12s  %-12s  %-12s  %-5.1f");
/// Fields: Time(hour:minute:second), Pages pushed, Pages read, Errors, °C
const std::string PROGRESS_FORMAT("  %02s:%02s:%02s   %-12s  %-12s  %-12s  %-5.1f");
/// Path for error log
auto READOUT_ERRORS_PATH = "readout_errors.txt";
/// Max amount of errors that are recorded into the error stream
constexpr int64_t MAX_RECORDED_ERRORS = 10000;
/// End InfoLogger message alias
constexpr auto endm = InfoLogger::endm;
/// We use steady clock because otherwise system clock changes could affect the running of the program
using TimePoint = std::chrono::steady_clock::time_point;
/// Struct used for benchmark time limit
struct TimeLimit {
    uint64_t seconds = 0;
    uint64_t minutes = 0;
    uint64_t hours = 0;
};
} // Anonymous namespace


/// This class handles command-line DMA benchmarking.
/// It has grown far beyond its original design, accumulating more and more options and extensions.
/// Ideally, this would be split up into multiple classes.
/// For example, the core DMA tasks should be extracted into a separate, generic DMA class and this benchmark class
/// would provide all the command-line options and handling around that.
/// The error checking should be functions defined by the data format, but this is "in progress" for now.
class ProgramDmaBench: public Program
{
  public:

    virtual Description getDescription()
    {
      return {
        "DMA Benchmark",
        "Test ReadoutCard DMA performance\n"
          "Various options are available to change aspects of the DMA process, error checking and recording of data.\n"
          "This program requires the user to preallocate a sufficient amount of hugepages for its DMA buffer. See the "
          "README.md for more information.\n"
          "The options specifying a size take power-of-10 and power-of-2 unit prefixes. For example '--bytes=1T' "
          "(1 terabyte) or '--buffer-size=1Gi' (1 gibibyte)",
        "roc-bench-dma --verbose --id=42:0.0 --bytes=10G"};
    }

    virtual void addOptions(po::options_description& options)
    {
      options.add_options()
          ("bar-hammer",
              po::bool_switch(&mOptions.barHammer),
              "Stress the BAR with repeated writes and measure performance")
          ("bytes",
              SuffixOption<uint64_t>::make(&mOptions.maxBytes)->default_value("0"),
              "Limit of bytes to transfer. Give 0 for infinite.")
          ("buffer-size",
              SuffixOption<size_t>::make(&mBufferSize)->default_value("1Gi"),
              "Buffer size in bytes. Rounded down to 2 MiB multiple. Minimum of 2 MiB. Use 2 MiB hugepage by default; |"
              "if buffer size is a multiple of 1 GiB, will try to use GiB hugepages")
          ("dma-channel",
              po::value<int>(&mOptions.dmaChannel)->default_value(0),
              "DMA channel selection (note: C-RORC has channels 0 to 5, CRU only 0)")
          ("generator",
              po::value<bool>(&mOptions.generatorEnabled)->default_value(true),
              "Enable data generator")
          ("generator-size",
              SuffixOption<size_t>::make(&mOptions.dataGeneratorSize)->default_value("0"),
              "Data generator data size. 0 will use internal driver default.");
      Options::addOptionCardId(options);
      options.add_options()
          ("links",
              po::value<std::string>(&mOptions.links)->default_value("0"),
              "Links to open. A comma separated list of integers or ranges, e.g. '0,2,5-10'")
          ("loopback",
              po::value<std::string>(&mOptions.loopbackModeString)->default_value("INTERNAL"),
              "Generator loopback mode [NONE, INTERNAL, DIU, SIU]")
          ("no-errorcheck",
              po::bool_switch(&mOptions.noErrorCheck),
              "Skip error checking")
          ("no-display",
              po::bool_switch(&mOptions.noDisplay),
              "Disable command-line display")
          ("no-resync",
              po::bool_switch(&mOptions.noResyncCounter),
              "Disable counter resync")
          ("no-rm-pages-file",
              po::bool_switch(&mOptions.noRemovePagesFile),
              "Don't remove the file used for pages after benchmark completes")
          ("no-temperature",
              po::bool_switch(&mOptions.noTemperature),
              "No temperature readout")
          ("page-reset",
              po::bool_switch(&mOptions.pageReset),
              "Reset page to default values after readout (slow)")
          ("page-size",
              SuffixOption<size_t>::make(&mOptions.dmaPageSize)->default_value("8Ki"),
              "Card DMA page size")
          ("pattern",
              po::value<std::string>(&mOptions.generatorPatternString)->default_value("INCREMENTAL"),
              "Error check with given pattern [INCREMENTAL, ALTERNATING, CONSTANT, RANDOM]")
          ("pause-push",
              po::value<uint64_t>(&mOptions.pausePush)->default_value(1),
              "Push thread pause time in microseconds if no work can be done")
          ("pause-read",
              po::value<uint64_t>(&mOptions.pauseRead)->default_value(10),
              "Readout thread pause time in microseconds if no work can be done")
          ("random-pause",
              po::bool_switch(&mOptions.randomPause),
              "Randomly pause readout")
          ("readout-mode",
              po::value<std::string>(&mOptions.readoutModeString),
              "Set readout mode [CONTINUOUS]")
          ("superpage-size",
              SuffixOption<size_t>::make(&mSuperpageSize)->default_value("1Mi"),
              "Superpage size in bytes. Note that it can't be larger than the buffer. If the IOMMU is not enabled, the "
              "hugepage size must be a multiple of the superpage size")
          ("time",
              po::value<std::string>(&mOptions.timeLimitString),
              "Time limit for benchmark. Any combination of [n]h, [n]m, & [n]s. For example: '5h30m', '10s', '1s2h3m'.")
          ("to-file-ascii",
              po::value<std::string>(&mOptions.fileOutputPathAscii),
              "Read out to given file in ASCII format")
          ("to-file-bin",
              po::value<std::string>(&mOptions.fileOutputPathBin),
              "Read out to given file in binary format (only contains raw data from pages)");
    }

    virtual void run(const po::variables_map& map)
    {
      for (auto& i : mLinkCounters) {
        i = LINK_COUNTER_INITIAL_VALUE;
      }

      getLogger() << "DMA channel: " << mOptions.dmaChannel << endm;

      auto cardId = Options::getOptionCardId(map);
      auto params = Parameters::makeParameters(cardId, mOptions.dmaChannel);
      params.setDmaPageSize(mOptions.dmaPageSize);
      params.setGeneratorEnabled(mOptions.generatorEnabled);
      params.setGeneratorLoopback(LoopbackMode::fromString(mOptions.loopbackModeString));

      // Handle file output options
      {
        mOptions.fileOutputAscii = !mOptions.fileOutputPathAscii.empty();
        mOptions.fileOutputBin = !mOptions.fileOutputPathBin.empty();

        if (mOptions.fileOutputAscii && mOptions.fileOutputBin) {
          throw ParameterException() << ErrorInfo::Message("File output can't be both ASCII and binary");
        } else {
          if (mOptions.fileOutputAscii) {
            mReadoutStream.open(mOptions.fileOutputPathAscii);
          }
          if (mOptions.fileOutputBin) {
            mReadoutStream.open(mOptions.fileOutputPathBin, std::ios::binary);
          }
        }
      }

      // Handle generator pattern option
      if (!mOptions.generatorPatternString.empty()) {
        mOptions.generatorPattern = GeneratorPattern::fromString(mOptions.generatorPatternString);
      }

      // Handle readout mode option
      if (!mOptions.readoutModeString.empty()) {
        mOptions.readoutMode = ReadoutMode::fromString(mOptions.readoutModeString);
      }

      // Log IOMMU status
      getLogger() << "IOMMU " << (AliceO2::Common::Iommu::isEnabled() ? "enabled" : "not enabled") << endm;

      // Create channel buffer
      {
        if (mBufferSize < mSuperpageSize) {
          throw ParameterException() << ErrorInfo::Message("Buffer size smaller than superpage size");
        }

        // Add time to the buffer's filename. This way we guard our buffer from being overwritten by another process
        // as this is before the DMA Channel initialization and no lock protection is in place.
        std::string bufferName = (b::format("roc-bench-dma_id=%s_chan=%s_%s_pages") % map["id"].as<std::string>()
            % mOptions.dmaChannel
            % time(0)).str();

        Utilities::HugepageType hugepageType;
        mMemoryMappedFile = Utilities::tryMapFile(mBufferSize, bufferName, !mOptions.noRemovePagesFile, &hugepageType);

        mBufferBaseAddress = reinterpret_cast<uintptr_t>(mMemoryMappedFile->getAddress());
        getLogger() << "Using buffer file path: " << mMemoryMappedFile->getFileName() << endm;
      }

      // Set up channel parameters
      mPageSize = params.getDmaPageSize().get();
      params.setCardId(cardId);
      params.setChannelNumber(mOptions.dmaChannel);
      params.setGeneratorDataSize(mPageSize);
      params.setGeneratorPattern(mOptions.generatorPattern);
      params.setBufferParameters(buffer_parameters::Memory { mMemoryMappedFile->getAddress(),
          mMemoryMappedFile->getSize() });
      // Note that we can force unlock because we know for sure this process is not holding the lock. If we did not know
      // this, it would be very dangerous to force the lock.
      params.setForcedUnlockEnabled(true);
      params.setLinkMask(Parameters::linkMaskFromString(mOptions.links));

      mInfinitePages = (mOptions.maxBytes <= 0);
      mMaxPages = mOptions.maxBytes / mPageSize;

      if (mOptions.readoutMode) {
        params.setReadoutMode(*mOptions.readoutMode);
      }

      if (!Utilities::isMultiple(mSuperpageSize, mPageSize)) {
        throw ParameterException() << ErrorInfo::Message("Superpage size not a multiple of page size");
      }

      mMaxSuperpages = mBufferSize / mSuperpageSize;
      mPagesPerSuperpage = mSuperpageSize / mPageSize;
      getLogger() << "Buffer size: " << mBufferSize << endm;
      getLogger() << "Superpage size: " << mSuperpageSize << endm;
      getLogger() << "Superpages in buffer: " << mMaxSuperpages << endm;
      getLogger() << "Page size: " << mPageSize << endm;
      getLogger() << "Page limit: " << mMaxPages << endm;
      getLogger() << "Pages per superpage: " << mPagesPerSuperpage << endm;

      if (mOptions.dataGeneratorSize != 0) {
        params.setGeneratorDataSize(mOptions.dataGeneratorSize);
        getLogger() << "Generator data size: " << mOptions.dataGeneratorSize << endm;
      } else {
        getLogger() << "Generator data size: <internal default>"  << endm;
      }

      // Get DMA channel object
      try {
        mChannel = ChannelFactory().getDmaChannel(params);
      }
      catch (const FileLockException& e) {
        getLogger() << InfoLogger::Error
          << "Another process is holding the channel lock (no automatic cleanup possible)" << endm;
        throw;
      }

      mCardType = mChannel->getCardType();
      getLogger() << "Card type: " << CardType::toString(mChannel->getCardType()) << endm;
      getLogger() << "Card PCI address: " << mChannel->getPciAddress().toString() << endm;
      getLogger() << "Card NUMA node: " << mChannel->getNumaNode() << endm;
      getLogger() << "Card firmware info: " << mChannel->getFirmwareInfo().value_or("unknown") << endm;

      getLogger() << "Starting benchmark" << endm;
      mChannel->startDma();

      if (mOptions.barHammer) {
        if (mChannel->getCardType() != CardType::Cru) {
          BOOST_THROW_EXCEPTION(ParameterException()
              << ErrorInfo::Message("BarHammer option currently only supported for CRU"));
        }
        Utilities::resetSmartPtr(mBarHammer);
        mBarHammer->start(ChannelFactory().getBar(Parameters::makeParameters(cardId, 0)));
      }

      if (!mOptions.timeLimitString.empty()) {
        auto limit = convertTimeString(mOptions.timeLimitString);
        mTimeLimitOptional = std::chrono::steady_clock::now() + std::chrono::hours(limit.hours)
          + std::chrono::minutes(limit.minutes) + std::chrono::seconds(limit.seconds);
        getLogger() << (b::format("Time limit: %1%h %2%m %3%s") % limit.hours % limit.minutes % limit.seconds).str()
          << endm;
      }

      mRunTime.start = std::chrono::steady_clock::now();
      dmaLoop();
      mRunTime.end = std::chrono::steady_clock::now();

      if (mBarHammer) {
        mBarHammer->join();
      }

      std::cout << "\n\n";
      mChannel->stopDma();
      int popped = freeExcessPages(10ms);
      getLogger() << "Popped " << popped << " remaining superpages" << endm;

      outputErrors();
      outputStats();
      getLogger() << "Benchmark complete" << endm;
    }

  private:

    void dmaLoop()
    {
      if (mMaxSuperpages < 1) {
        throw std::runtime_error("Buffer too small");
      }

      // Lock-free queues. Usable size is (size-1), so we add 1
      /// Queue for passing filled superpages from the push thread to the readout thread
      folly::ProducerConsumerQueue<size_t> readoutQueue {static_cast<uint32_t>(mMaxSuperpages) + 1};
      /// Queue for free superpages. This starts out as full, then the readout thread consumes them. When superpages
      /// arrive, they are passed via the readoutQueue to the readout thread. When the readout thread is done with it,
      /// it is put back in the freeQueue.
      folly::ProducerConsumerQueue<size_t> freeQueue {static_cast<uint32_t>(mMaxSuperpages) + 1};
      for (size_t i = 0; i < mMaxSuperpages; ++i) {
        size_t offset = i * mSuperpageSize;
        if (!freeQueue.write(offset)) {
          BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Something went horribly wrong"));
        }
      }

      std::atomic<bool> mDmaLoopBreak {false};
      auto isStopDma = [&]{ return mDmaLoopBreak.load(std::memory_order_relaxed); };

      // Thread for low priority tasks
      auto lowPriorityFuture = std::async(std::launch::async, [&]{
        try {
          auto next = std::chrono::steady_clock::now();
          while (!isStopDma()) {
            // Handle a SIGINT abort
            if (isSigInt()) {
              // We want to finish the readout cleanly if possible, so we stop pushing and try to wait a bit until the
              // queue is empty
              mDmaLoopBreak = true;
              return;
            }

            // If there's a time limit, check it
            if (auto limit = mTimeLimitOptional) {
              if (std::chrono::steady_clock::now() >= limit) {
                mDmaLoopBreak = true;
                return;
              }
            }

            // Status display updates
            // Wait until the DMA has really started before printing our table to avoid messy output
            if (!mOptions.noDisplay && mPushCount.load(std::memory_order_relaxed) != 0) {
              updateStatusDisplay();
            }
            next += LOW_PRIORITY_INTERVAL;
            std::this_thread::sleep_until(next);
          }
        }
        catch (std::exception& e) {
          mDmaLoopBreak = true;
          throw;
        }
      });

      // Thread for pushing & checking arrivals
      auto pushFuture = std::async(std::launch::async, [&]{
        try {
          RandomPauses pauses;
          int currentSuperpagePagesCounted = 0;

          while (!isStopDma()) {
            // Check if we need to stop in the case of a page limit
            if (!mInfinitePages && mPushCount.load(std::memory_order_relaxed) >= mMaxPages
                && (currentSuperpagePagesCounted == 0)) {
              break;
            }
            if (mOptions.randomPause) {
              pauses.pauseIfNeeded();
            }

            // Keep the driver's queue filled
            mChannel->fillSuperpages();

            auto shouldRest = false;

            // Give free superpages to the driver
            if (mChannel->getTransferQueueAvailable() != 0) {
              while (mChannel->getTransferQueueAvailable() != 0) {
                Superpage superpage;
                if (freeQueue.read(superpage.offset)) {
                  superpage.size = mSuperpageSize;
                  mChannel->pushSuperpage(superpage);
                } else {
                  // No free pages available, so take a little break
                  shouldRest = true;
                  break;
                }
              }
            } else {
              // No transfer queue slots available on the card
              shouldRest = true;
            }

            // Check for filled superpages
            while (mChannel->getReadyQueueSize() != 0) {
              auto superpage = mChannel->getSuperpage();
              // We do partial updates of the mPushCount because we can have very large superpages, which would otherwise
              // cause hiccups in the display
              int pages = superpage.received / mPageSize;
              int pagesToCount = pages - currentSuperpagePagesCounted;
              mPushCount.fetch_add(pagesToCount, std::memory_order_relaxed);
              currentSuperpagePagesCounted += pagesToCount;

              if (superpage.isReady() && readoutQueue.write(superpage.getOffset())) {
                // Move full superpage to readout queue
                currentSuperpagePagesCounted = 0;
                mChannel->popSuperpage();
              } else {
                // Readout is backed up, so rest a while
                shouldRest = true;
                break;
              }
            }

            if (shouldRest) {
              std::this_thread::sleep_for(std::chrono::microseconds(mOptions.pausePush));
            }
          }
        }
        catch (std::exception& e) {
          mDmaLoopBreak = true;
          throw;
        }
      });

      // Readout thread (main thread)
      {
        RandomPauses pauses;

        while (!isStopDma()) {
          if (!mInfinitePages && mReadoutCount.load(std::memory_order_relaxed) >= mMaxPages) {
            mDmaLoopBreak = true;
            break;
          }
          if (mOptions.randomPause) {
            pauses.pauseIfNeeded();
          }

          size_t offset;
          if (readoutQueue.read(offset)) {
            // Read out pages
            int pages = mSuperpageSize / mPageSize;
            for (int i = 0; i < pages; ++i) {
              auto readoutCount = fetchAddReadoutCount();
              readoutPage(mBufferBaseAddress + offset + i * mPageSize, mPageSize, readoutCount);
            }

            // Page has been read out
            // Add superpage back to free queue
            if (!freeQueue.write(offset)) {
              BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Something went horribly wrong"));
            }
          } else {
            // No superpages available to read out, so have a nap
            std::this_thread::sleep_for(std::chrono::microseconds(mOptions.pauseRead));
          }
        }
      }

      pushFuture.get();
      lowPriorityFuture.get();
    }

    /// Atomically fetch and increment the readout count. We do this because it is accessed by multiple threads.
    /// Although there is currently only one writer at a time and a regular increment probably would be OK.
    uint64_t fetchAddReadoutCount()
    {
      return mReadoutCount.fetch_add(1, std::memory_order_relaxed);
    }

    /// Free the pages that remain after stopping DMA (these may not be filled)
    int freeExcessPages(std::chrono::milliseconds timeout)
    {
      auto start = std::chrono::steady_clock::now();
      int popped = 0;
      while ((std::chrono::steady_clock::now() - start) < timeout) {
        auto size = mChannel->getReadyQueueSize();
        for (int i = 0; i < size; ++i)
          mChannel->popSuperpage();
        popped += size;
      }
      return popped;
    }

    uint32_t get32bitFromPage(uintptr_t pageAddress, size_t index)
    {
      auto page = reinterpret_cast<void*>(pageAddress + index*4);
      uint32_t value = 0;
      memcpy(&value, page, sizeof(value));
      return value;
    }

    uint32_t getDataGeneratorCounterFromPage(uintptr_t pageAddress, size_t headerSize)
    {
      switch (mCardType) {
        case CardType::Crorc:
          return get32bitFromPage(pageAddress, 0);
        case CardType::Cru: {
          // Grab the first payload word as the counter's beginning
          auto payload = reinterpret_cast<const volatile uint32_t *>(pageAddress + headerSize);
          return payload[0];
        }
        default: throw std::runtime_error("Error checking unsupported for this card type");
      }
    };

    void readoutPage(uintptr_t pageAddress, size_t pageSize, int64_t readoutCount)
    {
      // Read out to file
      printToFile(pageAddress, pageSize, readoutCount);

      // Data error checking
      if (!mOptions.noErrorCheck) {

        // Get link ID if needed
        uint32_t linkId = 0; // Use 0 for non-CRU cards
        if (mCardType == CardType::Cru) {
          linkId = Cru::DataFormat::getLinkId(reinterpret_cast<const char*>(pageAddress));
          if (linkId >= mLinkCounters.size()) {
            BOOST_THROW_EXCEPTION(Exception()
              << ErrorInfo::Message("Link ID from superpage out of range")
              << ErrorInfo::Index(linkId));
          }
        }

        // Check for errors
        bool hasError = true;
        switch (mCardType) {
          case CardType::Crorc:
            hasError = checkErrorsCrorc(pageAddress, pageSize, readoutCount, linkId);
            break;
          case CardType::Cru:
            hasError = checkErrorsCru(pageAddress, pageSize, readoutCount, linkId, mOptions.loopbackModeString);
            break;
          default:
            throw std::runtime_error("Error checking unsupported for this card type");
        }

        if (hasError && !mOptions.noResyncCounter) {
          // There was an error, so we resync the counter on the next page
          mLinkCounters[linkId] = LINK_COUNTER_INITIAL_VALUE;
        }
      }

      if (mOptions.pageReset) {
        // Set the buffer to the default value after the readout
        resetPage(pageAddress, pageSize);
      }
    }

    bool checkErrorsCru(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, int linkId, std::string loopbackMode)
    {
      if (loopbackMode == "NONE")
        return checkErrorsCruDdg(pageAddress, pageSize, eventNumber, linkId);
      else if (loopbackMode == "INTERNAL")
        return checkErrorsCruInternal(pageAddress, pageSize, eventNumber, linkId);
      else
        throw std::runtime_error("Loopback Mode not supported");
    }
 
    bool checkErrorsCruInternal(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, int linkId)
    {
      // pcie internal pattern
      // Every 256-bit word is built as follows:
      // 32 bits counter + 32 bits counter + 32-bit counter + 32 bit counter
      // 32 bits counter + 32 bits counter + 32-bit counter + 32 bit counter
      
      // Get counter value only if page is valid...
      if (mLinkCounters[linkId] == LINK_COUNTER_INITIAL_VALUE) {
        auto counter = getDataGeneratorCounterFromPage(pageAddress, 0x0); // no header!
        mErrorStream << b::format("resync counter for e:%d l:%d cnt:%x\n") % eventNumber % linkId % counter;
        mLinkCounters[linkId] = counter;
      }
      
      const uint32_t counter = mLinkCounters[linkId];

      bool foundError = false;
      auto checkValue = [&](uint32_t i, uint32_t expectedValue, uint32_t actualValue) {
        if (expectedValue != actualValue) {
          foundError = true;
          addError(eventNumber, linkId, i, counter, expectedValue, actualValue, pageSize);
        }
      };

      const auto payload = reinterpret_cast<const volatile uint32_t*> (pageAddress);

      // Check iterating through 256-bit words
      uint32_t offset = (counter % (0x100000000)); // mod 0xffffffff + 1
      uint32_t i = 0;

      while ((i*4) < pageSize) { //this is indexing, it has nothing to do with iterating step
        uint32_t word32 = i; // 32-bit word pointer

        checkValue(word32 + 0, offset, payload[word32 + 0]); //32-bit counter
        checkValue(word32 + 1, offset, payload[word32 + 1]); //32-bit counter
        checkValue(word32 + 2, offset, payload[word32 + 2]); //32-bit counter 
        checkValue(word32 + 3, offset, payload[word32 + 3]); //32-bit counter
        checkValue(word32 + 4, offset, payload[word32 + 4]); //32-bit counter
        checkValue(word32 + 5, offset, payload[word32 + 5]); //32-bit counter
        checkValue(word32 + 6, offset, payload[word32 + 6]); //32-bit counter 
        checkValue(word32 + 7, offset, payload[word32 + 7]); //32-bit counter 

        offset = (offset+1) % (0x100000000); //Increase counter by 1 - mod 0xffffffff + 1
        i += 8; // 8 = expected 2*sizeof(uint32_t);
      }
      
      mLinkCounters[linkId] = offset;
      return foundError;
    }

    bool checkErrorsCruDdg(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, int linkId)
    {
      // Get memsize from the header
      const auto memBytes = Cru::DataFormat::getEventSize(reinterpret_cast<const char*>(pageAddress)); // Memory size [RDH, Payload]

      if (memBytes < 40 || memBytes > pageSize) {
        // Report RDH error
        mErrorCount++;
        if (mErrorCount < MAX_RECORDED_ERRORS) {
          mErrorStream << b::format("[RDHERR]\tevent:%1% l:%2% payloadBytes:%3% size:%4% words out of range\n") % eventNumber
            % linkId % memBytes % pageSize;
        }
        return false;
      }

      // Get counter value only if page is valid...
      if (mLinkCounters[linkId] == LINK_COUNTER_INITIAL_VALUE) {
        auto counter = getDataGeneratorCounterFromPage(pageAddress, Cru::DataFormat::getHeaderSize());
        mErrorStream << b::format("resync counter for e:%d l:%d cnt:%x\n") % eventNumber % linkId % counter;
        mLinkCounters[linkId] = counter;
      }
      const uint32_t counter = mLinkCounters[linkId];

      //skip the header -> address + 0x40
      const auto payload = reinterpret_cast<const volatile uint32_t*>(pageAddress + Cru::DataFormat::getHeaderSize()); 
      const auto payloadBytes = memBytes - Cru::DataFormat::getHeaderSize();
      
      bool foundError = false;
      auto checkValue = [&](uint32_t i, uint32_t expectedValue, uint32_t actualValue) {
        if (expectedValue != actualValue) {
          foundError = true;
          addError(eventNumber, linkId, i, counter, expectedValue, actualValue, payloadBytes);
        }
      };

      // Check iterating through 256-bit words
      uint32_t offset = (counter % (0x100000000)); // mod 0xffffffff + 1
      uint32_t i = 0;

      // ddg pattern
      // Every 256-bit word is built as follows:
      // 32 bits counter       + 32 bits counter       + 16 lsb counter       + 32 bit 0
      // 32 bits (counter + 1) + 32 bits (counter + 1) + 16 lsb (counter + 1) + 32 bit 0

      while ((i*4) < payloadBytes) {
        uint32_t word32 = i; // 32-bit word pointer

        checkValue(word32 + 0, offset         , payload[word32 + 0]); //32-bit counter
        checkValue(word32 + 1, offset         , payload[word32 + 1]); //32-bit counter
        checkValue(word32 + 2, offset & 0xffff, payload[word32 + 2]); //16-lsb truncated counter 
        checkValue(word32 + 3, 0x0            , payload[word32 + 3]); //32-bit 0-padding word

        offset = (offset+1) % (0x100000000); //Increase counter by 1 - mod 0xffffffff + 1
        i += 4; // 4 = expected sizeof(uint32_t);
      }
      mLinkCounters[linkId] = offset;
      return foundError;
    }

    void addError(int64_t eventNumber, int linkId, int index, uint32_t generatorCounter, uint32_t expectedValue,
        uint32_t actualValue, uint32_t payloadBytes)
    {
      mErrorCount++;
       if (mErrorCount < MAX_RECORDED_ERRORS) {
         mErrorStream << b::format("[ERROR]\tevent:%d link:%d cnt:%x payloadBytes:%d i:%d exp:%x val:%x\n")
             % eventNumber % linkId % generatorCounter % payloadBytes % index % expectedValue % actualValue;
       }
    }

    bool checkErrorsCrorc(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, int linkId)
    {
      uint64_t counter = mLinkCounters[linkId];
      mLinkCounters[linkId]++;

      auto check = [&](auto patternFunction) {
        auto page = reinterpret_cast<const volatile uint32_t*>(pageAddress);
        auto pageSize32 = pageSize / sizeof(int32_t);

        if (page[0] != counter) {
          addError(eventNumber, linkId, 0, counter, counter, page[0], 0);
        }

        // We skip the SDH
        for (uint32_t i = 8; i < pageSize32; ++i) {
          uint32_t expectedValue = patternFunction(i);
          uint32_t actualValue = page[i];
          if (actualValue != expectedValue) {
            addError(eventNumber, linkId, i, counter, expectedValue, actualValue, 0);
            return true;
          }
        }
        return false;
      };

      switch (mOptions.generatorPattern) {
        case GeneratorPattern::Incremental:
          return check([&](uint32_t i) {return i - 1;});
        case GeneratorPattern::Alternating:
          return check([&](uint32_t) {return 0xa5a5a5a5;});
        case GeneratorPattern::Constant:
          return check([&](uint32_t) {return 0x12345678;});
        default: ;
      }

      BOOST_THROW_EXCEPTION(Exception()
          << ErrorInfo::Message("Unsupported pattern for C-RORC error checking")
          << ErrorInfo::GeneratorPattern(mOptions.generatorPattern));
    }

    void resetPage(uintptr_t pageAddress, size_t pageSize)
    {
      auto page = reinterpret_cast<volatile uint32_t*>(pageAddress);
      auto pageSize32 = pageSize / sizeof(uint32_t);
      for (size_t i = 0; i < pageSize32; i++) {
        page[i] = BUFFER_DEFAULT_VALUE;
      }
    }

    void updateStatusDisplay()
    {
      if (!mHeaderPrinted) {
        printStatusHeader();
        mHeaderPrinted = true;
      }

       using namespace std::chrono;
       auto diff = steady_clock::now() - mRunTime.start;
       auto second = duration_cast<seconds>(diff).count() % 60;
       auto minute = duration_cast<minutes>(diff).count() % 60;
       auto hour = duration_cast<hours>(diff).count();

       auto format = b::format(PROGRESS_FORMAT);
       format % hour % minute % second; // Time
       format % mPushCount.load(std::memory_order_relaxed);
       format % mReadoutCount.load(std::memory_order_relaxed);

       mOptions.noErrorCheck ? format % "n/a" : format % mErrorCount; // Errors

       if (mOptions.noTemperature) {
         format % "n/a";
       } else {
         if (auto temperature = mChannel->getTemperature()) {
           format % *temperature;
         } else {
           format % "n/a";
         }
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

     void outputStats()
     {
       // Calculating throughput
       double runTime = std::chrono::duration<double>(mRunTime.end - mRunTime.start).count();
       double bytes = double(mReadoutCount.load()) * mPageSize;
       double GB = bytes / (1000 * 1000 * 1000);
       double GBs = GB / runTime;
       double GiB = bytes / (1024 * 1024 * 1024);
       double GiBs = GiB / runTime;
       double Gbs = GBs * 8;

       auto put = [&](auto label, auto value) { cout << b::format("  %-10s  %-10s\n") % label % value; };
       cout << '\n';
       put("Seconds", runTime);
       put("Superpages", mReadoutCount.load());
       if (bytes > 0.00001) {
         put("Bytes", bytes);
         put("GB", GB);
         put("GB/s", GBs);
         put("Gb/s", Gbs);
         put("GiB/s", GiBs);
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

      if (!errorStr.empty()) {
        getLogger() << "Outputting " << std::min(mErrorCount, MAX_RECORDED_ERRORS) << " errors to '"
          << READOUT_ERRORS_PATH << "'" << endm;
        std::ofstream stream(READOUT_ERRORS_PATH);
        stream << errorStr;
      }
    }

    /// Prints the page to a file in ASCII or binary format if such output is enabled
    void printToFile(uintptr_t pageAddress, size_t pageSize, int64_t pageNumber)
    {
      auto page = reinterpret_cast<const volatile uint32_t*>(pageAddress);
      auto pageSize32 = pageSize / sizeof(uint32_t);

      if (mOptions.fileOutputAscii) {
        mReadoutStream << "Event #" << pageNumber << '\n';
        uint32_t perLine = 8;

        for (uint32_t i = 0; i < pageSize32; i += perLine) {
          for (uint32_t j = 0; j < perLine; ++j) {
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

    TimeLimit convertTimeString(std::string input)
    {
      TimeLimit limit;

      boost::char_separator<char> separators("", "hms"); // Keep the hms separators
      boost::tokenizer<boost::char_separator<char>> tokenizer(input, separators);
      std::vector<std::string> tokens(tokenizer.begin(), tokenizer.end());

      if (((tokens.size() % 2) != 0) || (tokens.size() > 6)) {
        BOOST_THROW_EXCEPTION(ParameterException() << ErrorInfo::Message("Malformed time limit string"));
      }

      for (size_t i = 0; i < tokens.size(); i += 2) {
        uint64_t number;
        if (!boost::conversion::try_lexical_convert<uint64_t>(tokens[i], number)) {
          BOOST_THROW_EXCEPTION(ParameterException()
              << ErrorInfo::Message("Malformed time limit string; failed to parse number"));
        }

        auto unit = tokens[i+1];
        if (unit == "h") {
          limit.hours = number;
        } else if (unit == "m") {
          limit.minutes = number;
        } else if (unit == "s") {
          limit.seconds = number;
        } else {
          BOOST_THROW_EXCEPTION(ParameterException()
            << ErrorInfo::Message("Malformed time limit string; unrecognized time unit"));
        }
      }

      return limit;
    }

    struct RandomPauses
    {
        static constexpr int NEXT_PAUSE_MIN = 10; ///< Minimum random pause interval in milliseconds
        static constexpr int NEXT_PAUSE_MAX = 2000; ///< Maximum random pause interval in milliseconds
        static constexpr int PAUSE_LENGTH_MIN = 1; ///< Minimum random pause in milliseconds
        static constexpr int PAUSE_LENGTH_MAX = 500; ///< Maximum random pause in milliseconds
        TimePoint next; ///< Next pause at this time
        std::chrono::milliseconds length; ///< Next pause has this length

        void pauseIfNeeded()
        {
          auto now = std::chrono::steady_clock::now();
          if (now >= next) {
            std::this_thread::sleep_for(length);
            // Schedule next pause
            auto now = std::chrono::steady_clock::now();
            next = now + std::chrono::milliseconds(Utilities::getRandRange(NEXT_PAUSE_MIN, NEXT_PAUSE_MAX));
            length = std::chrono::milliseconds(Utilities::getRandRange(PAUSE_LENGTH_MIN, PAUSE_LENGTH_MAX));
          }
        }
    };

    /// Program options
    struct OptionsStruct {
        uint64_t maxBytes = 0; ///< Limit of bytes to push
        int dmaChannel = 0;
        bool fileOutputAscii = false;
        bool fileOutputBin = false;
        bool resetChannel = false;
        bool randomPause = false;
        bool noErrorCheck = false;
        bool noTemperature = false;
        bool noDisplay = false;
        bool pageReset = false;
        bool noResyncCounter = false;
        bool barHammer = false;
        bool noRemovePagesFile = false;
        std::string generatorPatternString;
        std::string readoutModeString;
        std::string fileOutputPathBin;
        std::string fileOutputPathAscii;
        GeneratorPattern::type generatorPattern = GeneratorPattern::Incremental;
        b::optional<ReadoutMode::type> readoutMode;
        std::string links;
        bool generatorEnabled = false;
        size_t dataGeneratorSize;
        size_t dmaPageSize;
        std::string loopbackModeString;
        std::string timeLimitString;
        uint64_t pausePush;
        uint64_t pauseRead;
    } mOptions;

    /// The DMA channel
    std::shared_ptr<DmaChannelInterface> mChannel;

    /// The type of the card we're using
    CardType::type mCardType;

    /// Page counters per link. Indexed by link ID.
    std::array<std::atomic<uint32_t>, MAX_LINKS> mLinkCounters;

    /// Amount of superpages pushed
    std::atomic<uint64_t> mPushCount { 0 };

    //Kostas TODO: I'm under the impression this counter
    //             actually refers to dma pages...
    /// Amount of superpages read out 
    std::atomic<uint64_t> mReadoutCount { 0 };

    /// Total amount of errors encountered
    int64_t mErrorCount = 0;

    /// Keep on pushing until we're explicitly stopped
    bool mInfinitePages = false;

    /// Size of superpages
    size_t mSuperpageSize = 0;

    /// Maximum amount of superpages
    size_t mMaxSuperpages = 0;

    /// Amount of DMA pages per superpage
    size_t mPagesPerSuperpage = 0;

    /// Maximum size of pages
    size_t mPageSize;

    /// Maximum amount of pages to transfer
    size_t mMaxPages;

    /// The size of the channel DMA buffer
    size_t mBufferSize;

    /// The base address of the channel DMA buffer
    uintptr_t mBufferBaseAddress;

    /// The memory mapped file that contains the channel DMA buffer
    std::unique_ptr<MemoryMappedFile> mMemoryMappedFile;

    /// Object for BAR throughput testing
    std::unique_ptr<BarHammer> mBarHammer;

    /// Stream for file readout, only opened if enabled by the --file program options
    std::ofstream mReadoutStream;

    /// Stream for error output
    std::ostringstream mErrorStream;

    /// Was the header printed?
    bool mHeaderPrinted = false;

    /// Indicates the display must add a newline to the table
    bool mDisplayUpdateNewline;

    /// Optional time limit
    boost::optional<TimePoint> mTimeLimitOptional;

    struct RunTime
    {
        TimePoint start; ///< Start of run time
        TimePoint end; ///< End of run time
    } mRunTime;
};

int main(int argc, char** argv)
{
  return ProgramDmaBench().execute(argc, argv);
}
