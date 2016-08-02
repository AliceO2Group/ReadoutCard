///
/// \file RorcUtilsDmaBench.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that tests RORC DMA performance
///

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <queue>
#include <thread>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/ChannelFactory.h"
#include "RORC/ChannelParameters.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsProgram.h"
#include "Util.h"

using namespace AliceO2::Rorc::Util;
using std::cout;
using std::endl;

class ProgramDmaBench: public RorcUtilsProgram
{
  public:
    virtual ~ProgramDmaBench()
    {
    }

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("DMA Benchmark", "Test RORC DMA performance",
          "./rorc-dma-bench --serial=12345 --channel=0");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionChannel(options);
      Options::addOptionSerialNumber(options);
      Options::addOptionsChannelParameters(options);
    }

    virtual void mainFunction(const boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int channelNumber = Options::getOptionChannel(map);
      auto params = Options::getOptionsChannelParameters(map);
      params.generator.dataSize = params.dma.pageSize;
      params.initialResetLevel = AliceO2::Rorc::ResetLevel::RORC;

      // Get master lock on channel
      auto channel = AliceO2::Rorc::ChannelFactory().getMaster(serialNumber, channelNumber, params);
      channel->stopDma();
      channel->startDma();
      std::this_thread::sleep_for(std::chrono::microseconds(500)); // XXX See README.md

      // Vector to keep track of event numbers
      const size_t maxPagesToPush = 500l * 1000l;
      struct EventNumber {
          int actual;
          int expected;
      };
      std::vector<EventNumber> eventNumbers;
      eventNumbers.reserve(maxPagesToPush);

      cout << "### Starting benchmark" << endl;

      // Values & function to keep track of time
      const auto startTime = std::chrono::high_resolution_clock::now();
      const auto maxTime = std::chrono::seconds(3);
      auto isMaxTimeExceeded = [&]() { return (std::chrono::high_resolution_clock::now() - startTime) > maxTime; };

      if (false) {
        // XXX Test for zero dead time page pushing

        auto handleQueue = std::queue<AliceO2::Rorc::ChannelMasterInterface::PageHandle>();
        auto maxQueued = 32ul;

        for (size_t i = 0; i < maxPagesToPush; ++i) {
          // Push pages
          while (handleQueue.size() < maxQueued) {
            // Note that currently, we may overshoot the maxPagesToPush by a little bit
            handleQueue.push(channel->pushNextPage());
          }

          auto handle = handleQueue.front();
          handleQueue.pop();

          // Wait for page
          while (!channel->isPageArrived(handle) && !isMaxTimeExceeded() &&!isSigInt()) { ; }
          //std::this_thread::sleep_for(std::chrono::microseconds(1)); // XXX See README.md

          // Abort if time is exceeded
          if (isMaxTimeExceeded()) {
            cout << "Reached max time!" << endl;
            break;
          }

          if (isSigInt()) { break; }

          // Get page
          auto page = channel->getPage(handle);
          int eventNumber = page.getAddressU32()[0];
          eventNumbers.push_back(EventNumber{int(eventNumber), int(i) + 1 + 128});

          // Mark page as read so it can be written to again
          channel->markPageAsRead(handle);
        }
      } else {
        // Normal, sequential page pushing

        // Transfer some data
        for (size_t i = 0; i < maxPagesToPush; ++i) {
          // Push page
          auto handle = channel->pushNextPage();

          // Wait for page
          while (!channel->isPageArrived(handle) && !isMaxTimeExceeded() && !isSigInt()) { ; }
          //std::this_thread::sleep_for(std::chrono::microseconds(1)); // XXX See README.md

          // Abort if time is exceeded
          if (isMaxTimeExceeded()) {
            cout << "Reached max time!" << endl;
            break;
          }

          if (isSigInt()) { break; }

          // Get page
          auto page = channel->getPage(handle);
          uint32_t eventNumber = page.getAddressU32()[0];
          eventNumbers.push_back(EventNumber{int(eventNumber), int(i) + 1 + 128});

          // Mark page as read so it can be written to again
          channel->markPageAsRead(handle);
        }
      }

      const auto endTime= std::chrono::high_resolution_clock::now();
      channel->stopDma();

      cout << "### Benchmark complete" << endl;
      cout << "Pushed " << eventNumbers.size() << " pages" << endl;

      // Check if 'event numbers' are correct
      int nonConsecutives = 0;
      int unexpecteds = 0;
      for (size_t i = 0; (i + 1) < eventNumbers.size(); ++i) {
        if (eventNumbers[i].actual != (eventNumbers[i + 1].actual - 1)) {
          nonConsecutives++;
          if (isVerbose()) {
            cout << "NC: " << i << "   " << eventNumbers[i].actual << " != (" << eventNumbers[i + 1].actual << " - 1)\n";
          }
        }

        if (eventNumbers[i].actual != eventNumbers[i].expected) {
          unexpecteds++;
          if (isVerbose()) {
            cout << "UE: " << i << "   " << eventNumbers[i].actual << " != " << eventNumbers[i].expected << "\n";
          }
        }
      }
      if (nonConsecutives > 0) {
        cout << "WARNING: non-consecutive event numbers found (amount: " << nonConsecutives << ")" << endl;
      }
      if (nonConsecutives > 0) {
        cout << "WARNING: unexpected event numbers found (amount: " << unexpecteds << ")" << endl;
      }

      // Calculate performance
      auto pagesPushed = eventNumbers.size();
      auto bytesPushed = pagesPushed * params.dma.pageSize;
      auto seconds = std::chrono::duration<double>(endTime - startTime).count();
      auto bytesPerSecond = bytesPushed / seconds;

      cout << "### Statistics\n";
      cout << "====================================\n";
      cout << "Pages pushed   " << pagesPushed << endl;
      cout << "Bytes pushed   " << bytesPushed << endl;
      cout << "Seconds        " << seconds << endl;
      cout << "Bytes/second   " << bytesPerSecond << endl;
      cout << "GB/second      " << (bytesPerSecond / (1000000000)) << endl;
      cout << "====================================\n";
    }
};

int main(int argc, char** argv)
{
  return ProgramDmaBench().execute(argc, argv);
}
