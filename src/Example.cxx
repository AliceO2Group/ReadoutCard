///
/// \file Example.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// Example of pushing pages with the RORC C++ interface
///

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <boost/range/irange.hpp>
#include <RORC/RORC.h>

using std::cout;
using std::endl;
using namespace AliceO2;

// The serial number of the card to use
const int serialNumber = 0;

// The DMA channel to use
const int channelNumber = 0;

// The total amount of pages to push.
// Do more than 128 to test the FIFO wraparound.
const int pagesToPush = 128 + 3;

// Although it generally isn't needed, reinserting the driver module may be necessary if the driver has been abused.
// While we're in the early prototype stage, it's convenient to just always do it.
const bool reinsertDriverModule = true;

// Stop DMA after pushing all the pages.
// Explicit stopping should not be necessary, and will allow other processes to resume without restarting the DMA
const bool stopDma = true;

/// Prints the first 10 integers of a page
void printPage(Rorc::Page& page, int index)
{
    cout << std::setw(4) << index << " (0x" << std::hex << (uint64_t) page.getAddress() << std::dec << ") -> ";
    for (int j = 0; j < 10; ++j) {
      cout << std::setw(j == 0 ? 4 : 0) << page.getAddressU32()[j] << " ";
    }
    cout << endl;
}

int main(int argc, char** argv)
{
  if (reinsertDriverModule) {
    system("modprobe -r uio_pci_dma");
    system("modprobe uio_pci_dma");
  }

  // Initialize the parameters for configuring the DMA channel
  Rorc::ChannelParameters params;
  params.dma.bufferSize = 2 * 1024 * 1024;
  params.dma.pageSize = 4 * 1024;
  params.dma.useSharedMemory = true;
  params.generator.useDataGenerator = true;
  params.generator.dataSize = 2 * 1024;
  params.generator.pattern = Rorc::GeneratorPattern::INCREMENTAL;
  params.generator.seed = 0;
  params.initialResetLevel = Rorc::ResetLevel::RORC_ONLY;

  // Get the channel master object
  cout << "\n### Acquiring channel master object" << endl;
  auto channel = Rorc::ChannelMasterFactory().getChannel(serialNumber, channelNumber, params);

  // Start the DMA
  cout << "\n### Starting DMA" << endl;
  channel->startDma();

  // Hopefully, this is enough to insure the freeFifo transfers have completed
  // TODO A more robust wait, built into the framework
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Keep track of time, so we don't wait forever for pages to arrive if things break
  const auto start = std::chrono::high_resolution_clock::now();
  const auto maxTime = std::chrono::milliseconds(1000);
  auto timeExceeded = [&]() {
    if ((std::chrono::high_resolution_clock::now() - start) > maxTime) {
      cout << "!!! Max time of " << maxTime.count() << " ms exceeded" << endl;
      return true;
    } else {
      return false;
    }
  };

  cout << "### Pushing pages" << endl;

  for (auto i : boost::irange(0, pagesToPush)) {
    // Get page handle (contains FIFO index)
    auto handle = channel->pushNextPage();

    // Wait for page to arrive
    while (!channel->isPageArrived(handle) && !timeExceeded()) { ; }

    // Get page (contains userspace address)
    auto page = channel->getPage(handle);

    printPage(page, handle.index);

    // Mark page as read so it can be written to again
    channel->markPageAsRead(handle);
  }

  if (stopDma) {
    cout << "\n### Stopping DMA" << endl;
    channel->stopDma();
  } else {
    cout << "\n### NOT stopping DMA" << endl;
  }

  cout << "\n### Releasing channel master object" << endl;
  return 0;
}
