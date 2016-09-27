///
/// \file ProgramDmaBench.cxx
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
#include <boost/lexical_cast.hpp>
#include "RORC/Parameters.h"
#include "Utilities/Common.h"
#include "Utilities/Options.h"
#include "Utilities/Program.h"
#include "RORC/ChannelFactory.h"
#include "Util.h"

using namespace AliceO2::Rorc::Utilities;
using namespace ::AliceO2::Rorc;
using std::cout;
using std::endl;

class ProgramDmaBench: public Program
{
  public:

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

    virtual void run(const boost::program_options::variables_map& map)
    {
      int serialNumber = Options::getOptionSerialNumber(map);
      int channelNumber = Options::getOptionChannel(map);
      const auto maxTime = std::chrono::seconds(10);

      auto params = Options::getOptionsParameterMap(map);
      size_t pageSize = boost::lexical_cast<size_t>(params.at(Parameters::Keys::dmaPageSize()));
      params[Parameters::Keys::generatorDataSize()] = std::to_string(pageSize);

      // Get master lock on channel
      auto channel = AliceO2::Rorc::ChannelFactory().getMaster(serialNumber, channelNumber, params);
      channel->stopDma(); // Not necessary, but for the benchmark we want to start from a clean slate.
      channel->startDma();
      std::this_thread::sleep_for(std::chrono::microseconds(500)); // XXX See README.md

      // Vector to keep track of event numbers
      const size_t maxPagesToPush = 50l * 1000l * 1000l;
      struct EventNumber {
          int actual;
          int expected;
      };
      std::vector<EventNumber> eventNumbers;
      eventNumbers.reserve(maxPagesToPush);

      cout << "### Starting benchmark" << endl;
      cout << "Max time = " << maxTime.count() << " seconds\n";

      // Values & function to keep track of time
      const auto startTime = std::chrono::high_resolution_clock::now();
      auto isMaxTimeExceeded = [&]() { return (std::chrono::high_resolution_clock::now() - startTime) > maxTime; };

      if (false) {
        // XXX Test for zero dead time page pushing

        auto handleQueue = std::queue<PageHandle>();
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
          //std::this_thread::sleep_for(std::chrono::nanoseconds(1)); // XXX See README.md

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
          //std::this_thread::sleep_for(std::chrono::nanoseconds(1)); // XXX See README.md

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
        if (isSigInt()) { break; }

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
      auto bytesPushed = double(pagesPushed) * double(pageSize);
      auto seconds = std::chrono::duration<double>(endTime - startTime).count();
      auto bytesPerSecond = bytesPushed / seconds;

      cout << "### Statistics\n";
      cout << "====================================\n";
      cout << "Pages pushed   " << pagesPushed << '\n';
      cout << "Bytes pushed   " << bytesPushed << '\n';
      cout << "Seconds        " << seconds << '\n';
      cout << "Bytes/s        " << bytesPerSecond << '\n';
      cout << "GB/s           " << (bytesPerSecond / (1000*1000*1000)) << '\n';
      cout << "GiB/s          " << (bytesPerSecond / (1024*1024*1024)) << '\n';
      cout << "====================================\n";
    }
};

int main(int argc, char** argv)
{
  return ProgramDmaBench().execute(argc, argv);
}
