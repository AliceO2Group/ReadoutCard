
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
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
#include <boost/algorithm/string/predicate.hpp>
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
#include "DataFormat.h"
#include "ExceptionInternal.h"
#include "folly/ProducerConsumerQueue.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/MemoryMappedFile.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ReadoutCard.h"
#include "time.h"
#include "Utilities/Hugetlbfs.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/Util.h"

using namespace o2::roc::CommandLineUtilities;
using namespace o2::roc;
using AliceO2::Common::SuffixOption;
using std::cout;
using std::endl;
using namespace std::literals;
namespace b = boost;
namespace po = boost::program_options;

namespace
{
/// Initial value for link counters
constexpr auto DATA_COUNTER_INITIAL_VALUE = std::numeric_limits<uint32_t>::max();
/// Initial value for link packet counters
constexpr auto PACKET_COUNTER_INITIAL_VALUE = std::numeric_limits<uint32_t>::max();
/// Initial value for link event counters
constexpr auto EVENT_COUNTER_INITIAL_VALUE = std::numeric_limits<uint32_t>::max();
/// Maximum supported links
constexpr auto MAX_LINKS = 16;
/// Interval for low priority thread (display updates, etc)
constexpr auto LOW_PRIORITY_INTERVAL = 10ms;
/// Fields: Time(hour:minute:second), Pages pushed, Pages read, Errors, °C
const std::string PROGRESS_FORMAT_HEADER("  %-8s   %-12s  %-12s %-18s  %-12s  %-5.1f");
/// Fields: Time(hour:minute:second), Pages pushed, Pages read, Errors, °C
const std::string PROGRESS_FORMAT("  %02s:%02s:%02s   %-12s  %-12s  %-18s  %-12s  %-5.1f");
/// Path for error log
auto READOUT_ERRORS_PATH = "readout_errors.txt";
/// Max amount of errors that are recorded into the error stream
constexpr int64_t MAX_RECORDED_ERRORS = 10000;
/// We use steady clock because otherwise system clock changes could affect the running of the program
using TimePoint = std::chrono::steady_clock::time_point;
/// Struct used for benchmark time limit
struct TimeLimit {
  uint64_t seconds = 0;
  uint64_t minutes = 0;
  uint64_t hours = 0;
};
/// Struct used for the readoutQueue
struct SuperpageInfo {
  size_t bufferOffset;
  size_t effectiveSize;
};
} // Anonymous namespace

/// This class handles command-line DMA benchmarking.
/// It has grown far beyond its original design, accumulating more and more options and extensions.
/// Ideally, this would be split up into multiple classes.
/// For example, the core DMA tasks should be extracted into a separate, generic DMA class and this benchmark class
/// would provide all the command-line options and handling around that.
/// The error checking should be functions defined by the data format, but this is "in progress" for now.
class ProgramDmaBench : public Program
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
      "o2-roc-bench-dma --verbose --id=42:0.0 --bytes=10G"
    };
  }

  virtual void addOptions(po::options_description& options)
  {
    options.add_options()("bar-hammer",
                          po::bool_switch(&mOptions.barHammer),
                          "Stress the BAR with repeated writes and measure performance");
    options.add_options()("bytes",
                          SuffixOption<uint64_t>::make(&mOptions.maxBytes)->default_value("0"),
                          "Limit of bytes to transfer. Give 0 for infinite.");
    options.add_options()("buffer-full-check",
                          po::bool_switch(&mOptions.bufferFullCheck),
                          "Test how quickly the readout buffer gets full, if it's not emptied");
    options.add_options()("buffer-size",
                          SuffixOption<size_t>::make(&mBufferSize)->default_value("1Gi"),
                          "Buffer size in bytes. Rounded down to 2 MiB multiple. Minimum of 2 MiB. Use 2 MiB hugepage by default; |"
                          "if buffer size is a multiple of 1 GiB, will try to use GiB hugepages");
    options.add_options()("data-source",
                          po::value<std::string>(&mOptions.dataSourceString)->default_value("INTERNAL"),
                          "Data source [FEE, INTERNAL, DIU, SIU, DDG]");
    options.add_options()("dma-channel",
                          po::value<int>(&mOptions.dmaChannel)->default_value(0),
                          "DMA channel selection (note: C-RORC has channels 0 to 5, CRU only 0)");
    options.add_options()("error-check-frequency",
                          po::value<uint64_t>(&mOptions.errorCheckFrequency)->default_value(1),
                          "Frequency of dma pages to check for errors");
    options.add_options()("fast-check",
                          po::bool_switch(&mOptions.fastCheckEnabled),
                          "Enable fast error checking");
    Options::addOptionCardId(options);
    options.add_options()("max-rdh-packetcount",
                          po::value<size_t>(&mOptions.maxRdhPacketCounter)->default_value(255),
                          "Maximum packet counter expected in the RDH");
    options.add_options()("no-errorcheck",
                          po::bool_switch(&mOptions.noErrorCheck),
                          "Skip error checking");
    options.add_options()("no-display",
                          po::bool_switch(&mOptions.noDisplay),
                          "Disable command-line display");
    options.add_options()("no-resync",
                          po::bool_switch(&mOptions.noResyncCounter),
                          "Disable counter resync");
    options.add_options()("no-rm-pages-file",
                          po::bool_switch(&mOptions.noRemovePagesFile),
                          "Don't remove the file used for pages after benchmark completes");
    options.add_options()("no-temperature",
                          po::bool_switch(&mOptions.noTemperature),
                          "No temperature readout");
    options.add_options()("page-size",
                          SuffixOption<size_t>::make(&mOptions.dmaPageSize)->default_value("8Ki"),
                          "Card DMA page size");
    options.add_options()("pause-push",
                          po::value<uint64_t>(&mOptions.pausePush)->default_value(1),
                          "Push thread pause time in microseconds if no work can be done");
    options.add_options()("pause-read",
                          po::value<uint64_t>(&mOptions.pauseRead)->default_value(10),
                          "Readout thread pause time in microseconds if no work can be done");
    options.add_options()("print-sp-change",
                          po::bool_switch(&mOptions.printSuperpageChange),
                          "Print superpage change market when printing to file");
    options.add_options()("random-pause",
                          po::bool_switch(&mOptions.randomPause),
                          "Randomly pause readout");
    options.add_options()("stbrd",
                          po::bool_switch(&mOptions.stbrd),
                          "Set the STBRD trigger command for the CRORC");
    options.add_options()("superpage-size",
                          SuffixOption<size_t>::make(&mSuperpageSize)->default_value("1Mi"),
                          "Superpage size in bytes. Note that it can't be larger than the buffer. If the IOMMU is not enabled, the "
                          "hugepage size must be a multiple of the superpage size");
    options.add_options()("time",
                          po::value<std::string>(&mOptions.timeLimitString),
                          "Time limit for benchmark. Any combination of [n]h, [n]m, & [n]s. For example: '5h30m', '10s', '1s2h3m'.");
    options.add_options()("timeframe-length",
                          po::value<uint32_t>(&mOptions.timeFrameLength)->default_value(256),
                          "Time Frame length");
    options.add_options()("to-file-ascii",
                          po::value<std::string>(&mOptions.fileOutputPathAscii),
                          "Read out to given file in ASCII format");
    options.add_options()("to-file-bin",
                          po::value<std::string>(&mOptions.fileOutputPathBin),
                          "Read out to given file in binary format (only contains raw data from pages)");
    options.add_options()("bypass-fw-check",
                          po::bool_switch(&mOptions.bypassFirmwareCheck),
                          "Flag to bypass the firmware checker");
    options.add_options()("no-tf-check",
                          po::bool_switch(&mOptions.noTimeFrameCheck),
                          "Skip error checking");
  }

  virtual void run(const po::variables_map& map)
  {
    for (auto& i : mDataGeneratorCounters) {
      i = DATA_COUNTER_INITIAL_VALUE;
    }

    for (auto& i : mPacketCounters) {
      i = PACKET_COUNTER_INITIAL_VALUE;
    }

    for (auto& i : mEventCounters) {
      i = EVENT_COUNTER_INITIAL_VALUE;
    }

    std::cout << "DMA channel: " << mOptions.dmaChannel << std::endl;

    auto cardId = Options::getOptionCardId(map);
    auto params = Parameters::makeParameters(cardId, mOptions.dmaChannel);
    params.setDmaPageSize(mOptions.dmaPageSize);
    params.setDataSource(DataSource::fromString(mOptions.dataSourceString));
    params.setFirmwareCheckEnabled(!mOptions.bypassFirmwareCheck);

    mDataSource = params.getDataSourceRequired();

    params.setStbrdEnabled(mOptions.stbrd); //Set STBRD for the CRORC

    // Handle file output options
    mOptions.fileOutputAscii = !mOptions.fileOutputPathAscii.empty();
    mOptions.fileOutputBin = !mOptions.fileOutputPathBin.empty();

    if (mOptions.fileOutputAscii && mOptions.fileOutputBin) {
      BOOST_THROW_EXCEPTION(ParameterException() << ErrorInfo::Message("File output can't be both ASCII and binary"));
    } else {
      if (mOptions.fileOutputAscii) {
        mReadoutStream.open(mOptions.fileOutputPathAscii);
      }
      if (mOptions.fileOutputBin) {
        mReadoutStream.open(mOptions.fileOutputPathBin, std::ios::binary);
      }
    }

    // Log IOMMU status
    std::cout << "IOMMU " << (AliceO2::Common::Iommu::isEnabled() ? "enabled" : "not enabled") << std::endl;

    // Create channel buffer

    if (mBufferSize < mSuperpageSize) {
      BOOST_THROW_EXCEPTION(ParameterException() << ErrorInfo::Message("Buffer size smaller than superpage size"));
    }

    std::string bufferName = (b::format("o2-roc-bench-dma_id=%s_chan=%s_pages") % map["id"].as<std::string>() % mOptions.dmaChannel).str();

    Utilities::HugepageType hugepageType;
    mMemoryMappedFile = Utilities::tryMapFile(mBufferSize, bufferName, !mOptions.noRemovePagesFile, &hugepageType);

    mBufferBaseAddress = reinterpret_cast<uintptr_t>(mMemoryMappedFile->getAddress());
    std::cout << "Using buffer file path: " << mMemoryMappedFile->getFileName() << std::endl;

    // Set up channel parameters
    mPageSize = params.getDmaPageSize().get();
    params.setCardId(cardId);
    params.setChannelNumber(mOptions.dmaChannel);
    params.setBufferParameters(buffer_parameters::Memory{ mMemoryMappedFile->getAddress(),
                                                          mMemoryMappedFile->getSize() });

    mInfinitePages = (mOptions.maxBytes <= 0);
    mSuperpageLimit = mOptions.maxBytes / mSuperpageSize;

    if (!Utilities::isMultiple(mSuperpageSize, mPageSize)) {
      throw ParameterException() << ErrorInfo::Message("Superpage size not a multiple of page size");
    }

    mSuperpagesInBuffer = mBufferSize / mSuperpageSize;
    std::cout << "Buffer size: " << mBufferSize << std::endl;
    std::cout << "Superpage size: " << mSuperpageSize << std::endl;
    std::cout << "Superpages in buffer: " << mSuperpagesInBuffer << std::endl;
    std::cout << "Superpage limit: " << mSuperpageLimit << std::endl;
    std::cout << "DMA page size: " << mPageSize << std::endl;
    if (mOptions.bufferFullCheck) {
      std::cout << "Buffer-Full Check enabled" << std::endl;
      mBufferFullCheck = true;
    }

    if (!mOptions.noErrorCheck) {
      if (mOptions.errorCheckFrequency < 0x1 || mOptions.errorCheckFrequency > 0xff) {
        throw ParameterException() << ErrorInfo::Message("Frequency of dma pages to fast check has to be in the range [1,255]");
      }
      mErrorCheckFrequency = mOptions.errorCheckFrequency;
      std::cout << "Error check frequency: " << mErrorCheckFrequency << " dma page(s)" << std::endl;
      if (mOptions.fastCheckEnabled) {
        mFastCheckEnabled = mOptions.fastCheckEnabled;
        std::cout << "Fast check enabled" << std::endl;
      }
      mMaxRdhPacketCounter = mOptions.maxRdhPacketCounter;
      std::cout << "Maximum RDH packet counter " << mMaxRdhPacketCounter << std::endl;
      mTimeFrameLength = mOptions.timeFrameLength;
      std::cout << "TimeFrame length " << mTimeFrameLength << std::endl;
      if (mOptions.noTimeFrameCheck) {
        mTimeFrameCheckEnabled = false;
        std::cout << "TimeFrame check disabled" << std::endl;
      }
    }

    // Get DMA channel object
    try {
      mChannel = ChannelFactory().getDmaChannel(params);
    } catch (const LockException& e) {
      std::cerr << "Another process is holding the channel lock (no automatic cleanup possible)" << std::endl;
      throw;
    }

    mCardType = mChannel->getCardType();
    std::cout << "Card type: " << CardType::toString(mChannel->getCardType()) << std::endl;
    std::cout << "Card PCI address: " << mChannel->getPciAddress().toString() << std::endl;
    std::cout << "Card NUMA node: " << mChannel->getNumaNode() << std::endl;
    std::cout << "Card firmware info: " << mChannel->getFirmwareInfo().value_or("unknown") << std::endl;

    std::cout << "Starting benchmark" << std::endl;
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
      mTimeLimitOptional = std::chrono::steady_clock::now() + std::chrono::hours(limit.hours) + std::chrono::minutes(limit.minutes) + std::chrono::seconds(limit.seconds);
      std::cout << (b::format("Time limit: %1%h %2%m %3%s") % limit.hours % limit.minutes % limit.seconds).str()
                << std::endl;
    }

    if (mBufferFullCheck) {
      mBufferFullTimeStart = std::chrono::high_resolution_clock::now();
    }
    dmaLoop();
    mRunTime.end = std::chrono::steady_clock::now();

    if (mBarHammer) {
      mBarHammer->join();
    }

    std::cout << "\n\n";
    mChannel->stopDma();
    int numPopped = freeExcessPages(10ms);
    std::cout << "Popped " << numPopped << " remaining superpages" << std::endl;

    outputErrors();
    outputStats();
    std::cout << "Benchmark complete" << std::endl;
  }

 private:
  void dmaLoop()
  {
    if (mSuperpagesInBuffer < 1) {
      throw std::runtime_error("Buffer too small");
    }

    // Lock-free queues. Usable size is (size-1), so we add 1
    /// Queue for passing filled superpages from the push thread to the readout thread
    folly::ProducerConsumerQueue<SuperpageInfo> readoutQueue{ static_cast<uint32_t>(mSuperpagesInBuffer) + 1 };
    /// Queue for free superpages. This starts out as full, then the readout thread consumes them. When superpages
    /// arrive, they are passed via the readoutQueue to the readout thread. When the readout thread is done with it,
    /// it is put back in the freeQueue.
    folly::ProducerConsumerQueue<size_t> freeQueue{ static_cast<uint32_t>(mSuperpagesInBuffer) + 1 };
    for (size_t i = 0; i < mSuperpagesInBuffer; ++i) {
      size_t offset = i * mSuperpageSize;
      if (!freeQueue.write(offset)) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Something went horribly wrong"));
      }
    }

    std::atomic<bool> mDmaLoopBreak{ false };
    auto isStopDma = [&] { return mDmaLoopBreak.load(std::memory_order_relaxed); };

    // Thread for low priority tasks
    auto lowPriorityFuture = std::async(std::launch::async, [&] {
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
          auto limit = mTimeLimitOptional;
          if (limit && std::chrono::steady_clock::now() >= limit) {
            mDmaLoopBreak = true;
            return;
          }

          if (mSuperpagesPushed.load(std::memory_order_relaxed) != 0) {

            // Start our run timer when DMA starts
            if (!mRunTimeStarted) {
              mRunTime.start = std::chrono::steady_clock::now();
              mRunTimeStarted = true;
            }

            // Status display updates
            // Wait until the DMA has really started before printing our table to avoid messy output
            if (!mOptions.noDisplay) {
              updateStatusDisplay();
            }
          }

          // Intermittently check that the FIFOs are healthy
          // Will potentially log a warning as a side-effect
          mChannel->areSuperpageFifosHealthy();

          next += LOW_PRIORITY_INTERVAL;
          std::this_thread::sleep_until(next);
        }
      } catch (std::exception& e) {
        mDmaLoopBreak = true;
        throw;
      }
    });

    // Thread for pushing & checking arrivals
    auto pushFuture = std::async(std::launch::async, [&] {
      try {
        RandomPauses pauses;

        while (!isStopDma()) {
          // Check if we need to stop in the case of a superpage limit
          if (!mInfinitePages && mSuperpagesPushed.load(std::memory_order_relaxed) >= mSuperpageLimit) {
            break;
          }
          if (mOptions.randomPause) {
            pauses.pauseIfNeeded();
          }

          // Keep the driver's queue filled
          mChannel->fillSuperpages();

          bool shouldRest = true;

          while (mChannel->getTransferQueueAvailable() != 0) {
            Superpage superpage;
            size_t offsetRead;

            if (freeQueue.read(offsetRead)) {
              superpage.setSize(mSuperpageSize);
              superpage.setOffset(offsetRead);
              mChannel->pushSuperpage(superpage);
            } else {
              // freeQueue is backed up and we should rest
              shouldRest = true;
              break;
            }
          }

          // Check for filled superpages
          while (mChannel->getReadyQueueSize() != 0) {
            auto superpage = mChannel->getSuperpage();
            fetchAddSuperpagesPushed();

            if (mBufferFullCheck && (mSuperpagesPushed.load(std::memory_order_relaxed) == mSuperpageLimit)) {
              mBufferFullTimeFinish = std::chrono::high_resolution_clock::now();
              mDmaLoopBreak = true;
            }

            // Move full superpage to readout queue
            if (superpage.isReady() && readoutQueue.write(SuperpageInfo{ superpage.getOffset(), superpage.getReceived() })) {
              mChannel->popSuperpage();
            } else {
              // readyQueue(=readout) is backed up, so rest a while
              shouldRest = true;
              break;
            }
          }

          if (shouldRest) {
            std::this_thread::sleep_for(std::chrono::microseconds(mOptions.pausePush));
          }
        }
      } catch (std::exception& e) {
        mDmaLoopBreak = true;
        throw;
      }
    });

    // Readout thread (main thread)
    try {
      RandomPauses pauses;

      while (!isStopDma()) {
        if (!mInfinitePages && mSuperpagesReadOut.load(std::memory_order_relaxed) >= mSuperpageLimit) {
          mDmaLoopBreak = true;
          break;
        }

        if (mOptions.randomPause) {
          pauses.pauseIfNeeded();
        }

        SuperpageInfo superpageInfo;
        if (readoutQueue.read(superpageInfo) && !mBufferFullCheck) {

          // Read out pages
          size_t readoutBytes = 0;
          auto superpageAddress = mBufferBaseAddress + superpageInfo.bufferOffset;

          auto superpageCount = fetchAddSuperpagesReadOut();

          bool atStartOfSuperpage = true;
          while ((readoutBytes < superpageInfo.effectiveSize) && !isStopDma()) {
            auto pageAddress = superpageAddress + readoutBytes;
            auto readoutCount = fetchAddDmaPagesReadOut();
            size_t pageSize = readoutPage(pageAddress, readoutCount, superpageCount, atStartOfSuperpage);

            atStartOfSuperpage = false; //Update the boolean value as soon as we move...

            mByteCount.fetch_add(pageSize, std::memory_order_relaxed);
            readoutBytes += pageSize;
          }

          if (readoutBytes > mSuperpageSize) {
            mDmaLoopBreak = true; // Dump superpage somewhere
            mReadoutStream.open("RDH_CUMULATIVE_SP_SIZE_FAILURE.bin");
            mReadoutStream.write(reinterpret_cast<const char*>(superpageAddress), mSuperpageSize);
            BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("RDH reports cumulative dma page sizes that exceed the superpage size"));
          }

          // Page has been read out
          // Add superpage back to free queue
          if (!freeQueue.write(superpageInfo.bufferOffset)) {
            mDmaLoopBreak = true;
            BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Something went horribly wrong"));
          }
        } else {
          // No superpages available to read out, so have a nap
          std::this_thread::sleep_for(std::chrono::microseconds(mOptions.pauseRead));
        }
      }
    } catch (Exception& e) {
      mDmaLoopBreak = true;
      throw;
    }

    pushFuture.get();
    lowPriorityFuture.get();
  }

  /// Free the pages that remain after stopping DMA (these may not be filled)
  int freeExcessPages(std::chrono::milliseconds timeout)
  {
    auto start = std::chrono::steady_clock::now();
    int popped = 0;
    while ((std::chrono::steady_clock::now() - start) < timeout) {
      auto size = mChannel->getReadyQueueSize();
      for (int i = 0; i < size; ++i) {
        auto superpage = mChannel->popSuperpage();
        auto superpageCount = fetchAddSuperpagesReadOut();
        if ((mDataSource == DataSource::Fee) || (mDataSource == DataSource::Ddg)) {
          auto superpageAddress = mBufferBaseAddress + superpage.getOffset();
          size_t readoutBytes = 0;
          bool atStartOfSuperpage = true;
          while ((readoutBytes < superpage.getReceived())) { // At least one more dma page fits in the superpage
            auto pageAddress = superpageAddress + readoutBytes;
            auto readoutCount = fetchAddDmaPagesReadOut();
            size_t pageSize = readoutPage(pageAddress, readoutCount, superpageCount, atStartOfSuperpage);
            atStartOfSuperpage = false; // Update the boolean value as soon as we move...
            readoutBytes += pageSize;
          }

          if (readoutBytes > mSuperpageSize) {
            BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("RDH reports cumulative dma page sizes that exceed the superpage size"));
          }
        }
        //std::cout << "[popped superpage " << i << " ], size= " << superpage.getSize() << " received= " << superpage.getReceived() << " isFilled=" << superpage.isFilled() << " isReady=" << superpage.isReady() << std::endl;
      }
      popped += size;
    }
    return popped;
  }

  uint32_t getDataGeneratorCounterFromPage(uintptr_t pageAddress, size_t headerSize)
  {
    auto payload = reinterpret_cast<const volatile uint32_t*>(pageAddress + headerSize);
    return payload[0];
  };

  size_t readoutPage(uintptr_t pageAddress, int64_t readoutCount, int64_t superpageCount, bool atStartOfSuperpage)
  {
    size_t pageSize;
    if (mCardType == CardType::Cru && mDataSource == DataSource::Internal) {
      pageSize = mPageSize;
    } else {
      pageSize = DataFormat::getOffset(reinterpret_cast<const char*>(pageAddress));
    }

    // Read out to file
    printToFile(pageAddress, pageSize, readoutCount, superpageCount, atStartOfSuperpage, pageSize == 0);

    // Data error checking
    if (!mOptions.noErrorCheck) {

      // Get link ID if needed
      uint32_t linkId = 0; // Use 0 for non-CRU cards
      if (mCardType == CardType::Cru && mDataSource != DataSource::Internal) {
        linkId = DataFormat::getLinkId(reinterpret_cast<const char*>(pageAddress));
        if (linkId >= mDataGeneratorCounters.size()) {
          mReadoutStream.open("LINK_ID_OUT_OF_RANGE.bin");
          mReadoutStream.write(reinterpret_cast<const char*>(pageAddress), mSuperpageSize);
          BOOST_THROW_EXCEPTION(Exception()
                                << ErrorInfo::Message("Link ID from superpage out of range")
                                << ErrorInfo::Index(linkId));
        }
      }

      // Check for errors
      bool hasError = true;
      mEventCounters[linkId] = (mEventCounters[linkId] + 1) % EVENT_COUNTER_INITIAL_VALUE;
      if (mEventCounters[linkId] % mErrorCheckFrequency == 0) {
        switch (mCardType) {
          case CardType::Crorc:
            hasError = checkErrorsCrorc(pageAddress, pageSize, readoutCount, linkId, atStartOfSuperpage);
            break;
          case CardType::Cru:
            hasError = checkErrorsCru(pageAddress, pageSize, readoutCount, linkId, atStartOfSuperpage);
            break;
          default:
            BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Error checking unsupported for this card type"));
        }
      } else {
        hasError = false;
      }

      if (hasError && !mOptions.noResyncCounter) {
        // There was an error, so we resync the counter on the next page
        mDataGeneratorCounters[linkId] = DATA_COUNTER_INITIAL_VALUE;
        mPacketCounters[linkId] = PACKET_COUNTER_INITIAL_VALUE;
      }
    }

    return pageSize;
  }

  bool checkErrorsCru(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, int linkId, bool atStartOfSuperpage)
  {
    if (mDataSource == DataSource::Ddg || mDataSource == DataSource::Fee) {
      return checkErrorsCruDdg(pageAddress, pageSize, eventNumber, linkId, atStartOfSuperpage);
    } else if (mDataSource == DataSource::Internal) {
      return checkErrorsCruInternal(pageAddress, pageSize, eventNumber, linkId); //TODO: Update internal err checking with bool for sp start
    } else {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("CRU error check: Data Source " + DataSource::toString(mDataSource) + " not supported"));
    }
  }

  bool checkErrorsCruInternal(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, int linkId)
  {
    // pcie internal pattern
    // 32 bits counter + 32 bits (counter + 1) + ...

    // Get initial data counter value from page
    if (mDataGeneratorCounters[linkId] == DATA_COUNTER_INITIAL_VALUE) {
      auto dataCounter = getDataGeneratorCounterFromPage(pageAddress, 0x0); // no header!
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("resync dataCounter for e:%d l:%d cnt:%x\n") % eventNumber % linkId % dataCounter;
      }
      mDataGeneratorCounters[linkId] = dataCounter - 1; // -- so that the for loop offset incrementer logic is consistent
    }

    const uint32_t dataCounter = mDataGeneratorCounters[linkId];

    bool foundError = false;
    auto checkValue = [&](uint32_t i, uint32_t expectedValue, uint32_t actualValue) {
      if (expectedValue != actualValue) {
        foundError = true;
        addError(eventNumber, linkId, i, dataCounter, expectedValue, actualValue, pageSize);
      }
    };

    const auto payload = reinterpret_cast<const volatile uint32_t*>(pageAddress);

    // Check iterating through 256-bit words
    uint32_t offset = (dataCounter % (0x100000000)); // mod 0xffffffff + 1

    for (uint32_t i = 0; (i * 4) < pageSize; i++) { // iterate every 32bit word in the page
      uint32_t word32 = i;                          // 32-bit word pointer
      if (i % 8 == 0) {
        offset = (offset + 1) % (0x100000000); //Increment dataCounter for every 8th wordth - mod 0xffffffff + 1
      }

      checkValue(word32, offset, payload[word32]);
    }

    mDataGeneratorCounters[linkId] = offset;
    return foundError;
  }

  bool checkErrorsCruDdg(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, int linkId, bool atStartOfSuperpage)
  {
    // Get memsize from the header
    const auto memBytes = DataFormat::getMemsize(reinterpret_cast<const char*>(pageAddress)); // Memory size [RDH, Payload]

    if (memBytes < 0x40 || memBytes > pageSize) {
      // Report RDH error
      mErrorCount++;
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("[RDHERR]\tevent:%1% l:%2% payloadBytes:%3% size:%4% words out of range\n") % eventNumber % linkId % memBytes % pageSize;
      }
      return true;
    }

    // check link's packet counter here
    const auto packetCounter = DataFormat::getPacketCounter(reinterpret_cast<const char*>(pageAddress));

    if (mPacketCounters[linkId] == PACKET_COUNTER_INITIAL_VALUE) {
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("resync packet counter for e:%d l:%d packet_cnt:%x mpacket_cnt:%x le:%d \n") % eventNumber % linkId % packetCounter %
                          mPacketCounters[linkId] % mEventCounters[linkId];
      }
      mPacketCounters[linkId] = packetCounter;
    } else if (((mPacketCounters[linkId] + mErrorCheckFrequency) % (mMaxRdhPacketCounter + 1)) != packetCounter) { //packetCounter is 8bits long
      // log packet counter error
      mErrorCount++;
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("[RDHERR]\tevent:%1% l:%2% payloadBytes:%3% size:%4% packet_cnt:%5% mpacket_cnt:%6% levent:%7% unexpected packet counter\n") % eventNumber % linkId % memBytes % pageSize % packetCounter % mPacketCounters[linkId] % mEventCounters[linkId];
      }
      return true;
    } else {
      mPacketCounters[linkId] = packetCounter; // same as = (mPacketCounters + mErrorCheckFrequency) % mMaxRdhPacketCounter
    }

    if (mTimeFrameCheckEnabled && !checkTimeFrameAlignment(pageAddress, atStartOfSuperpage)) {
      // log TF not at the beginning of the superpage error
      mErrorCount++;
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("[RDHERR]\tevent:%1% l:%2% payloadBytes:%3% size:%4% packet_cnt:%5% orbit:%6$#x nextTForbit:%7$#x atSPStart:%8% TF unaligned w/ start of superpage\n") % eventNumber % linkId % memBytes % pageSize % packetCounter % mOrbit % mNextTFOrbit % atStartOfSuperpage;
      }
    }

    // Skip data check if fast check enabled or FEE data source selected
    if (mFastCheckEnabled || mDataSource == DataSource::Fee) {
      return false;
    }

    // Get counter value only if page is valid...
    const auto dataCounter = getDataGeneratorCounterFromPage(pageAddress, DataFormat::getHeaderSize());
    if (mDataGeneratorCounters[linkId] == DATA_COUNTER_INITIAL_VALUE) {
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("resync counter for e:%d l:%d cnt:%x\n") % eventNumber % linkId % dataCounter;
      }
      mDataGeneratorCounters[linkId] = dataCounter;
    }
    //const uint32_t dataCounter = mDataGeneratorCounters[linkId];

    //skip the header -> address + 0x40
    const auto payload = reinterpret_cast<const volatile uint32_t*>(pageAddress + DataFormat::getHeaderSize());
    const auto payloadBytes = memBytes - DataFormat::getHeaderSize();

    bool foundError = false;
    auto checkValue = [&](uint32_t i, uint32_t expectedValue, uint32_t actualValue) {
      if (expectedValue != actualValue) {
        foundError = true;
        addError(eventNumber, linkId, i, dataCounter, expectedValue, actualValue, payloadBytes);
      }
    };

    // Check iterating through 256-bit words
    uint32_t offset = (dataCounter % (0x100000000)); // mod 0xffffffff + 1
    uint32_t i = 0;

    // ddg pattern
    // Every 256-bit word is built as follows:
    // 32 bits counter       + 32 bits counter       + 16 lsb counter       + 32 bit 0
    // 32 bits (counter + 1) + 32 bits (counter + 1) + 16 lsb (counter + 1) + 32 bit 0

    while ((i * 4) < payloadBytes) {
      uint32_t word32 = i; // 32-bit word pointer

      checkValue(word32 + 0, offset, payload[word32 + 0]);          //32-bit counter
      checkValue(word32 + 1, offset, payload[word32 + 1]);          //32-bit counter
      checkValue(word32 + 2, offset & 0xffff, payload[word32 + 2]); //16-lsb truncated counter
      checkValue(word32 + 3, 0x0, payload[word32 + 3]);             //32-bit 0-padding word

      offset = (offset + 1) % (0x100000000); //Increase counter by 1 - mod 0xffffffff + 1
      i += 4;                                // 4 = expected sizeof(uint32_t);
    }
    mDataGeneratorCounters[linkId] = offset;
    return foundError;
  }

  bool checkTimeFrameAlignment(uintptr_t pageAddress, bool atStartOfSuperpage)
  {
    static bool overflowGuard = false;
    static uint32_t prevOrbit = 0x0;

    // check that the TimeFrame starts at the beginning of the superpage
    const auto triggerType = DataFormat::getTriggerType(reinterpret_cast<const char*>(pageAddress));
    //const auto orbit = DataFormat::getOrbit(reinterpret_cast<const char*>(pageAddress));
    mOrbit = DataFormat::getOrbit(reinterpret_cast<const char*>(pageAddress));
    //const auto pagesCounter = DataFormat::getPagesCounter(reinterpret_cast<const char*>(pageAddress));

    //std::cout << atStartOfSuperpage << " 0x" << std::hex << orbit << " 0x" << std::hex << mNextTFOrbit << std::endl;
    if (prevOrbit > mOrbit) { // orbit overflown, remove guard
      overflowGuard = false;
    }
    prevOrbit = mOrbit;

    if (Utilities::getBit(triggerType, 9) == 0x1 || Utilities::getBit(triggerType, 7) == 0x1) { // If SOX, use current orbit as the first one
      mNextTFOrbit = mOrbit + mTimeFrameLength;
    } else if (!overflowGuard && mOrbit >= mNextTFOrbit) {
      // next orbit should be previous orbit + time frame length
      if (!atStartOfSuperpage) {
        return false;
      }
      while (mNextTFOrbit <= mOrbit) { // If we end up on an orbit that is more than a TF length away
        // Update next TF orbit expected
        overflowGuard = (mNextTFOrbit + mTimeFrameLength) & 0x100000000; // next TF orbit overflown, need to wait for orbit to overflow as well
        mNextTFOrbit = mNextTFOrbit + mTimeFrameLength;
      }
    }

    return true;
  }

  void addError(int64_t eventNumber, int linkId, int index, uint32_t generatorCounter, uint32_t expectedValue,
                uint32_t actualValue, uint32_t payloadBytes)
  {
    mErrorCount++;
    if (mErrorCount < MAX_RECORDED_ERRORS) {
      mErrorStream << b::format("[ERROR]\tevent:%d link:%d cnt:%x payloadBytes:%d i:%d exp:%x val:%x\n") % eventNumber % linkId % generatorCounter % payloadBytes % index % expectedValue % actualValue;
    }
  }

  bool checkErrorsCrorc(uintptr_t pageAddress, size_t pageSize, int64_t eventNumber, int linkId, bool atStartOfSuperpage)
  {
    const auto memBytes = DataFormat::getMemsize(reinterpret_cast<const char*>(pageAddress));
    if (memBytes > pageSize) {
      mErrorCount++;
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("[RDHERR]\tevent:%1% l:%2% payloadBytes:%3% size:%4% words out of ranger\n") % eventNumber % linkId % memBytes % pageSize;
      }
      return true;
    }

    uint32_t packetCounter = DataFormat::getPacketCounter(reinterpret_cast<const char*>(pageAddress));

    if (mPacketCounters[linkId] == PACKET_COUNTER_INITIAL_VALUE) {
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("resync packet counter for e%d l:%d packet_cnt:%x mpacket_cnt:%x, le:%d \n") % eventNumber % linkId % packetCounter %
                          mPacketCounters[linkId] % mEventCounters[linkId];
      }
      mPacketCounters[linkId] = packetCounter;
    } else if (((mPacketCounters[linkId] + mErrorCheckFrequency) % (mMaxRdhPacketCounter + 1)) != packetCounter) {
      mErrorCount++;
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("[RDHERR]\tevent:%1% l:%2% packet_cnt:%3% mpacket_cnt:%4% unexpected packet counter\n") % eventNumber % linkId % packetCounter % mPacketCounters[linkId];
      }
      return true;
    } else {
      mPacketCounters[linkId] = packetCounter;
    }

    if (mTimeFrameCheckEnabled && !checkTimeFrameAlignment(pageAddress, atStartOfSuperpage)) {
      // log TF not at the beginning of the superpage error
      mErrorCount++;
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("[RDHERR]\tevent:%1% l:%2% payloadBytes:%3% size:%4% packet_cnt:%5% orbit:%6$#x nextTForbit:%7$#x atSPStart:%8% TF unaligned w/ start of superpage\n") % eventNumber % linkId % memBytes % pageSize % packetCounter % mOrbit % mNextTFOrbit % atStartOfSuperpage;
      }
    }

    if (mFastCheckEnabled) {
      return false;
    }

    // Get counter value only if page is valid...
    const auto dataCounter = getDataGeneratorCounterFromPage(pageAddress, DataFormat::getHeaderSize());
    if (mDataGeneratorCounters[linkId] == DATA_COUNTER_INITIAL_VALUE) {
      if (mErrorCount < MAX_RECORDED_ERRORS) {
        mErrorStream << b::format("resync counter for e:%d l:%d cnt:%x\n") % eventNumber % linkId % dataCounter;
      }
      mDataGeneratorCounters[linkId] = dataCounter;
    }

    // Skip the RDH
    auto page = reinterpret_cast<const volatile uint32_t*>(pageAddress + DataFormat::getHeaderSize());

    uint32_t offset = dataCounter;

    bool foundError = false;
    auto checkValue = [&](uint32_t i, uint32_t expectedValue, uint32_t actualValue) {
      if (expectedValue != actualValue) {
        foundError = true;
        addError(eventNumber, linkId, i, offset, expectedValue, actualValue, pageSize);
      }
    };

    size_t pageSize32 = (memBytes - DataFormat::getHeaderSize()) / sizeof(uint32_t); // Addressable size of dma page (after RDH)

    for (size_t i = 0; i < pageSize32; i++) { // iterate through dmaPage
      checkValue(i, offset, page[i]);
      offset = (offset + 1) % (0x100000000);
    }

    mDataGeneratorCounters[linkId] = offset;
    return foundError;
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
    format % mSuperpagesPushed.load(std::memory_order_relaxed);
    format % mSuperpagesReadOut.load(std::memory_order_relaxed);

    double runTime = std::chrono::duration<double>(steady_clock::now() - mRunTime.start).count();
    double bytes = double(mByteCount.load(std::memory_order_relaxed));
    double Gb = bytes * 8 / (1000 * 1000 * 1000);
    double Gbps = Gb / runTime;
    format % Gbps;

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
    auto line1 = b::format(PROGRESS_FORMAT_HEADER) % "Time" % "Pushed" % "Read" % "Throughput (Gbps)" % "Errors" % "°C";
    auto line2 = b::format(PROGRESS_FORMAT) % "00" % "00" % "00" % '-' % '-' % '-' % '-' % '-';
    cout << '\n'
         << line1;
    cout << '\n'
         << line2;
  }

  void outputStats()
  {
    // Calculating throughput
    double runTime = std::chrono::duration<double>(mRunTime.end - mRunTime.start).count();
    double bytes = double(mByteCount.load());
    double GB = bytes / (1000 * 1000 * 1000);
    double GBs = GB / runTime;
    double GiB = bytes / (1024 * 1024 * 1024);
    double GiBs = GiB / runTime;
    double Gbs = GBs * 8;

    auto put = [&](auto label, auto value) { cout << b::format("  %-24s  %-10s\n") % label % value; };
    cout << '\n';
    put("Seconds", runTime);
    put("Superpages", mSuperpagesReadOut.load());
    put("Superpage Latency(s)", runTime / mSuperpagesReadOut.load());
    put("DMA Pages", mDmaPagesReadOut.load());
    put("DMA Page Latency(s)", runTime / mDmaPagesReadOut.load());
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
    if (mBufferFullCheck) {
      put("Total time needed to fill the buffer (ns) ", std::chrono::duration_cast<std::chrono::nanoseconds>(mBufferFullTimeFinish - mBufferFullTimeStart).count());
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
      std::cout << "Outputting " << std::min(mErrorCount, MAX_RECORDED_ERRORS) << " errors to '"
                << READOUT_ERRORS_PATH << "'" << std::endl;
      std::ofstream stream(READOUT_ERRORS_PATH);
      stream << errorStr;
    }
  }

  /// Prints the page to a file in ASCII or binary format if such output is enabled
  void printToFile(uintptr_t pageAddress, size_t pageSize, int64_t pageNumber, int64_t superpageNumber, bool atStartOfSuperpage, bool isEmpty = false)
  {
    auto page = reinterpret_cast<const volatile uint32_t*>(pageAddress);
    auto pageSize32 = pageSize / sizeof(uint32_t);

    if (mOptions.fileOutputAscii) {
      if (atStartOfSuperpage && mOptions.printSuperpageChange) {
        mReadoutStream << "Superpage #" << std::hex << "0x" << superpageNumber << '\n';
      }
      if (isEmpty && mOptions.printSuperpageChange) {
        mReadoutStream << "!!EMPTY DMA PAGE!!\n";
      }
      mReadoutStream << "Event #" << std::hex << "0x" << pageNumber << '\n';
      uint32_t perLine = 8;

      for (uint32_t i = 0; i < pageSize32; i += perLine) {
        for (uint32_t j = 0; j < perLine; ++j) {
          mReadoutStream << std::hex << "0x" << std::setw(8) << page[i + j] << ' ';
          mReadoutStream << '\t';
        }
        mReadoutStream << '\n';
      }
      mReadoutStream << '\n';
    } else if (mOptions.fileOutputBin) {
      if (atStartOfSuperpage && mOptions.printSuperpageChange) {
        uint32_t newSP = 0x0badf00d;
        for (int i = 0; i < 4; i++) { // Marker is 128bits long
          mReadoutStream.write(reinterpret_cast<const char*>(&newSP), sizeof(newSP));
        }
      }
      if (isEmpty && mOptions.printSuperpageChange) {
        uint32_t emptySP = 0xdeadbeef;
        for (int i = 0; i < 4; i++) { // Marker is 128bits long
          mReadoutStream.write(reinterpret_cast<const char*>(&emptySP), sizeof(emptySP));
        }
      }

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

      auto unit = tokens[i + 1];
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

  /// Atomically fetch and increment the Superpage and DMA page read out and pushed counts.
  /// We do this because they are accessed by multiple threads.
  /// Although there is currently only one writer at a time and a regular increment probably would be OK.
  uint64_t fetchAddDmaPagesReadOut()
  {
    return mDmaPagesReadOut.fetch_add(1, std::memory_order_relaxed);
  }

  uint64_t fetchAddSuperpagesReadOut()
  {
    return mSuperpagesReadOut.fetch_add(1, std::memory_order_relaxed);
  }

  uint64_t fetchAddSuperpagesPushed()
  {
    return mSuperpagesPushed.fetch_add(1, std::memory_order_relaxed);
  }

  struct RandomPauses {
    static constexpr int NEXT_PAUSE_MIN = 10;    ///< Minimum random pause interval in milliseconds
    static constexpr int NEXT_PAUSE_MAX = 2000;  ///< Maximum random pause interval in milliseconds
    static constexpr int PAUSE_LENGTH_MIN = 1;   ///< Minimum random pause in milliseconds
    static constexpr int PAUSE_LENGTH_MAX = 500; ///< Maximum random pause in milliseconds
    TimePoint next;                              ///< Next pause at this time
    std::chrono::milliseconds length;            ///< Next pause has this length

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
    uint64_t errorCheckFrequency;
    bool fastCheckEnabled;
    bool fileOutputAscii = false;
    bool fileOutputBin = false;
    bool resetChannel = false;
    bool randomPause = false;
    bool noErrorCheck = false;
    bool noTemperature = false;
    bool noDisplay = false;
    bool noResyncCounter = false;
    bool barHammer = false;
    bool noRemovePagesFile = false;
    std::string fileOutputPathBin;
    std::string fileOutputPathAscii;
    bool bufferFullCheck = false;
    size_t dmaPageSize;
    std::string dataSourceString;
    std::string timeLimitString;
    uint64_t pausePush;
    uint64_t pauseRead;
    size_t maxRdhPacketCounter;
    bool stbrd = false;
    bool bypassFirmwareCheck = false;
    uint32_t timeFrameLength = 256;
    bool printSuperpageChange = false;
    bool noTimeFrameCheck = false;
  } mOptions;

  /// The DMA channel
  std::shared_ptr<DmaChannelInterface> mChannel;

  /// The type of the card we're using
  CardType::type mCardType;

  /// Page counters per link. Indexed by link ID.
  std::array<std::atomic<uint32_t>, MAX_LINKS> mDataGeneratorCounters;

  // Packet counter per link
  std::array<std::atomic<uint32_t>, MAX_LINKS> mPacketCounters;

  // Event counter per link
  std::array<std::atomic<uint32_t>, MAX_LINKS> mEventCounters;

  // Superpage counters
  /// Amount of Superpages pushed
  std::atomic<uint64_t> mSuperpagesPushed{ 0 };

  // Amount of Superpages read out
  std::atomic<uint64_t> mSuperpagesReadOut{ 0 };

  // DMA page counters for better granularity
  /// Amount of DMA pages pushed
  //std::atomic<uint64_t> mDmaPagesPushed{ 0 };

  // Amount of DMA pages read out
  std::atomic<uint64_t> mDmaPagesReadOut{ 0 };

  // Amount of bytes read out (as reported in the RDH)
  std::atomic<uint64_t> mByteCount{ 0 };

  /// Total amount of errors encountered
  int64_t mErrorCount = 0;

  /// Keep on pushing until we're explicitly stopped
  bool mInfinitePages = false;

  /// Size of superpages
  size_t mSuperpageSize = 0;

  /// Maximum amount of superpages to exchange
  size_t mSuperpageLimit = 0;

  /// Maximum amount of superpages in buffer
  size_t mSuperpagesInBuffer = 0;

  /// Maximum size of pages
  size_t mPageSize;

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

  /// Flag to enable fast error checking
  bool mFastCheckEnabled = false;

  /// The frequency of dma pages to error check
  uint64_t mErrorCheckFrequency = 1;

  struct RunTime {
    TimePoint start; ///< Start of run time
    TimePoint end;   ///< End of run time
  } mRunTime;

  std::chrono::high_resolution_clock::time_point mBufferFullTimeStart;
  std::chrono::high_resolution_clock::time_point mBufferFullTimeFinish;

  /// Flag that marks that runtime has started
  bool mRunTimeStarted = false;

  /// The maximum value of the RDH Packet Counter
  size_t mMaxRdhPacketCounter;

  /// Flag to test how quickly the readout buffer gets full in case of error
  bool mBufferFullCheck;

  /// Data Source
  DataSource::type mDataSource;

  /// Current orbit
  uint32_t mOrbit = 0x0;

  /// The orbit number that coincides with the next TimeFrame
  uint32_t mNextTFOrbit = 0x0;

  /// The orbit number that coincides with the next TimeFrame
  uint32_t mTimeFrameLength = 0x0;

  /// Flag for TF check
  bool mTimeFrameCheckEnabled = true;
};

int main(int argc, char** argv)
{
  // true here enables InfoLogger output by default
  // see the Program constructor
  return ProgramDmaBench().execute(argc, argv);
}
