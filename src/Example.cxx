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

// The DMA channel to use
const int channel = 0;

// The total amount of pages to push. We're doing more than 128 so we can see that the FIFO wraparound works.
const int pagesToPush = 130;

// Push pages concurrently. It seems to work.
const bool concurrentPushing = true;

// The amount of pages to push at a time if concurrentPushing == true
const int nConcurrent = 5;

// Although it generally isn't needed, reinserting the driver module may be necessary if the driver has been abused.
// While we're in the early prototype stage, it's convenient to just always do it.
const bool reinsertDriverModule = true;

/// Prints the first 10 integers of a page
void printPage(Rorc::Page& page, int index)
{
    cout << std::setw(4) << index << " -> ";

    for (int j = 0; j < 10; ++j) {
      cout << std::setw(j == 0 ? 4 : 0) << page.getAddressU32()[j] << " ";
    }

    cout << endl;
}

/// Prints the first 10 integers of each page
void printPages(Rorc::PageVector& pages)
{
  cout << "### PAGES\n";
  int i = 0;

  for (auto& page : pages) {
    printPage(page, i);
    i++;
  }
}

int main(int argc, char** argv)
{
  if (reinsertDriverModule) {
    system("modprobe -r uio_pci_dma");
    system("modprobe uio_pci_dma");
  }

  // Get the shared pointer to the card
  auto card = Rorc::CardFactory().getCardFromSerialNumber(0);

  // Initialize the parameters for configuring the DMA channel
  Rorc::ChannelParameters params;
  params.dma.bufferSizeMiB = 512;
  params.dma.pageSize = 2 * 1024 * 1024;
  params.generator.useDataGenerator = true;
  params.generator.dataSize = 2 * 1024;
  params.generator.pattern = Rorc::GeneratorPattern::INCREMENTAL;
  params.initialResetLevel = Rorc::ResetLevel::RORC_ONLY;

  // Open the DMA channel
  cout << "\n### Opening DMA channel\n" << endl;
  card->openChannel(channel, params);

  // Start the DMA
  cout << "\n### Starting DMA\n" << endl;
  card->startDma(channel);

  // Hopefully, this is enough to insure the freeFifo transfers have completed
  // TODO A more robust wait
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

  if (!concurrentPushing) {
    cout << "### Pushing pages one-by-one" << endl;

    for (auto i : boost::irange(0, pagesToPush)) {
      // Get page handle (contains FIFO index)
      Rorc::PageHandle handle = card->pushNextPage(channel);

      // Wait for page to arrive
      while (!card->isPageArrived(channel, handle) && !timeExceeded()) { ; }

      // Get page (contains userspace address)
      Rorc::Page page = card->getPage(channel, handle);

      printPage(page, handle.index);

      // Mark page as read so it can be written to again
      card->markPageAsRead(channel, handle);
    }
  } else {
    cout << "### Pushing pages " << nConcurrent << " at a time" << endl;

    for (auto i : boost::irange(0, pagesToPush, nConcurrent)) {
      auto range = boost::irange(0, nConcurrent);

      if (timeExceeded()) { break; }

      std::vector<Rorc::PageHandle> handles(nConcurrent);
      std::vector<Rorc::Page> pages(nConcurrent);

      cout << "# Pushing " << nConcurrent << " pages" << endl;

      // Push pages
      for (auto j : range) {
        handles[j] = card->pushNextPage(channel);
      }

      // Wait until the arrive and get their addresses
      for (auto j : range) {
        while (!card->isPageArrived(channel, handles[j]) && !timeExceeded()) { ; }
        pages[j] = card->getPage(channel, handles[j]);
      }

      // Print pages and mark as read to indicate we're done with them
      for (auto j : range) {
        printPage(pages[j], handles[j].index);
        card->markPageAsRead(channel, handles[j]);
      }
    }
  }

  cout << "\n### Stopping DMA\n" << endl;
  card->stopDma(channel);

  // Accessing all pages
  auto pages = card->getMappedPages(channel);
  printPages(pages);

  cout << "\n### Closing DMA channel\n" << endl;
  card->closeChannel(channel);

  return 0;
}
