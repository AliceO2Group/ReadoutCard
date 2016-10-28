/// \file Example.cxx
/// \brief Example of pushing pages with the RORC C++ interface
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <boost/range/irange.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/RORC.h"

using std::cout;
using std::endl;
using namespace AliceO2;

namespace {

// The serial number of the card to use
// For dummy implementation, use Rorc::ChannelMasterFactory::DUMMY_SERIAL_NUMBER
const int serialNumber = 33333;

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
const bool stopDma = false;

const size_t pageSize = 4*1024;
const size_t bufferSize = 4*1024*1024;

/// Prints the first 10 integers of a page
void printPage(Rorc::ChannelMasterInterface::Page& page, int index, std::ostream& ios)
{
    ios << std::setw(4) << index << " (0x" << std::hex << (uint64_t) page.getAddress() << std::dec << ") -> ";
    for (int j = 0; j < 10; ++j) {
      ios << std::setw(j == 0 ? 4 : 0) << page.getAddressU32()[j] << " ";
    }
    ios << '\n';
}

} // Anonymous namespace

Rorc::Parameters makeParams()
{
  using namespace Rorc;
  Parameters params;
  params.put<Parameters::SerialNumber>(serialNumber);
  params.put<Parameters::ChannelNumber>(channelNumber);
  params.put<Parameters::DmaPageSize>(pageSize);
  params.put<Parameters::DmaBufferSize>(bufferSize);
  params.put<Parameters::GeneratorEnabled>(true);
  return params;
}

int main(int, char**)
{
  if (reinsertDriverModule) {
    system("modprobe -r uio_pci_dma");
    system("modprobe uio_pci_dma");
  }

  try {
    // Get the channel master object
    cout << "\n### Acquiring channel master object" << endl;
    std::shared_ptr<Rorc::ChannelMasterInterface> channel = Rorc::ChannelFactory().getMaster(makeParams());

    // Start the DMA
    cout << "\n### Starting DMA" << endl;
    channel->startDma();

    // Hopefully, this is enough to insure the freeFifo transfers have completed
    // TODO A more robust wait, built into the framework
    std::this_thread::sleep_for(std::chrono::microseconds(500));

    // Keep track of time, so we don't wait forever for pages to arrive if things break
    const auto start = std::chrono::high_resolution_clock::now();
    const auto maxTime = std::chrono::milliseconds(10000);
    auto timeExceeded = [&]() {
      if ((std::chrono::high_resolution_clock::now() - start) > maxTime) {
        cout << "!!! Max time of " << maxTime.count() << " ms exceeded" << endl;
        throw std::runtime_error("Max time exceeded");
      } else {
        return false;
      }
    };

    cout << "### Pushing pages" << endl;

    // The CRORC data generator puts an incremental number on the first 32 bits of the page.
    // It starts with 128, because that's how many pages are pushed during initialization.
    // They are a sort of "event number".
    // We keep track of them so we can later check if they are as expected.
    std::vector<uint32_t> eventNumbers;
    eventNumbers.reserve(pagesToPush);
    std::stringstream stringStream;

    for (int i = 0; i < pagesToPush; ++i) {
      // Fill FIFO
      channel->fillFifo();

      // Wait for page to arrive
      // Uses a busy wait, because the wait time is (or should be) extremely short
      while (!timeExceeded()) {
        if (std::shared_ptr<Rorc::ChannelMasterInterface::Page> page = Rorc::ChannelMasterInterface::popPage(channel)) {
          // Get page (contains userspace address)
          uint32_t eventNumber = page->getAddressU32()[0];
          eventNumbers.push_back(eventNumber);

          printPage(*page.get(), page->getId(), stringStream);

          // Page is automatically freed when shared pointer is destroyed
        }
      }
      std::this_thread::sleep_for(std::chrono::microseconds(1)); // See README.md


    }

    if (stopDma) {
      cout << "\n### Stopping DMA" << endl;
      channel->stopDma();
    } else {
      cout << "\n### NOT stopping DMA" << endl;
    }

    cout << stringStream.rdbuf() << endl;

    // Check if 'event numbers' are correct
    for (size_t i = 0; i < (eventNumbers.size() - 1); ++i) {
      if (eventNumbers[i] != (eventNumbers[i + 1] - 1)) {
        cout << "WARNING: Page " << std::setw(4) << i << " number not consecutive with next page ("
            << std::setw(4) << eventNumbers[i] << " & "
            << std::setw(4) << eventNumbers[i + 1] << ")\n";
      }
    }

    cout << "\n### Releasing channel master object" << endl;
  }
  catch (const std::exception& e) {
    cout << boost::diagnostic_information(e) << endl;
  }
  return 0;
}
