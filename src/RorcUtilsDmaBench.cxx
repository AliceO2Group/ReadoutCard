///
/// \file RorcUtilsDmaBench.cxx
/// \author Pascal Boeschoten
///
/// Utility that tests RORC DMA performance
///

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/ChannelFactory.h"
#include "RORC/ChannelParameters.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsDescription.h"

using namespace AliceO2::Rorc::Util;
using std::cout;
using std::endl;

static const UtilsDescription DESCRIPTION(
    "DMA Benchmark",
    "Test RORC DMA performance",
    "./rorc-dma-bench"
    );

int main(int argc, char** argv)
{
  auto optionsDescription = Options::createOptionsDescription();
  Options::addOptionChannel(optionsDescription);
  Options::addOptionSerialNumber(optionsDescription);

  try {
    auto variablesMap = Options::getVariablesMap(argc, argv, optionsDescription);
    int serialNumber = Options::getOptionSerialNumber(variablesMap);
    int channelNumber = Options::getOptionChannel(variablesMap);

    // Initialize channel parameters
    AliceO2::Rorc::ChannelParameters params;
    params.dma.bufferSize = 4 * 1024 * 1024;
    params.dma.pageSize = 4 * 1024;
    params.dma.useSharedMemory = true;
    params.generator.useDataGenerator = true;
    params.generator.dataSize = 4 * 1024;
    params.initialResetLevel = AliceO2::Rorc::ResetLevel::RORC_ONLY;

    // Get master lock on channel
    auto channel = AliceO2::Rorc::ChannelFactory().getMaster(serialNumber, channelNumber, params);
    channel->stopDma();
    channel->startDma();

    // Vector to keep track of event numbers
    const size_t maxPagesToPush = 500l * 1000l;
    std::vector<uint32_t> eventNumbers;
    eventNumbers.reserve(maxPagesToPush);

    cout << "### Starting benchmark" << endl;

    // Values & function to keep track of time
    const auto startTime = std::chrono::high_resolution_clock::now();
    const auto maxTime = std::chrono::seconds(3);
    auto isMaxTimeExceeded = [&]() { return (std::chrono::high_resolution_clock::now() - startTime) > maxTime; };

    // Transfer some data
    for (size_t i = 0; i < maxPagesToPush; ++i) {
      // Push page
      auto handle = channel->pushNextPage();

      // Wait for page
      while (!channel->isPageArrived(handle) && !isMaxTimeExceeded()) { ; }

      // Abort if time is exceeded
      if (isMaxTimeExceeded()) { break; }

      // Get page
      auto page = channel->getPage(handle);
      uint32_t eventNumber = page.getAddressU32()[0];
      eventNumbers.push_back(eventNumber);

      // Mark page as read so it can be written to again
      channel->markPageAsRead(handle);
    }

    const auto endTime= std::chrono::high_resolution_clock::now();
    channel->stopDma();

    cout << "### Benchmark complete" << endl;

    // Check if 'event numbers' are correct
    int nonConsecutives = 0;
    for (size_t i = 0; i < (eventNumbers.size() - 1); ++i) {
      if (eventNumbers[i] != (eventNumbers[i + 1] - 1)) {
        nonConsecutives++;
      }
    }
    if (nonConsecutives > 0) {
      cout << "WARNING: non-consecutive event numbers found (amount: " << nonConsecutives << ")" << endl;
    }

    // Calculate performance
    auto pagesPushed = eventNumbers.size();
    auto bytesPushed = pagesPushed * params.dma.pageSize;
    auto seconds = std::chrono::duration<double>(endTime - startTime).count();
    auto bytesPerSecond = bytesPushed / seconds;

    cout << "### Statistics\n";
    cout << "------------------------------------\n";
    cout << "Pages pushed   " << pagesPushed << endl;
    cout << "Bytes pushed   " << bytesPushed << endl;
    cout << "Seconds        " << seconds << endl;
    cout << "Bytes/second   " << bytesPerSecond << endl;
    cout << "------------------------------------\n";

  } catch (std::exception& e) {
    Options::printErrorAndHelp(boost::current_exception_diagnostic_information(), DESCRIPTION, optionsDescription);
  }
  return 0;
}
