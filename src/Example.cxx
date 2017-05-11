/// \file Example.cxx
/// \brief Example of pushing pages with the RORC C++ interface
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include <chrono>
#include <thread>
#include <boost/exception/diagnostic_information.hpp>
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/MemoryMappedFile.h"

using std::cout;
using std::endl;
using namespace AliceO2;

int main(int, char**)
{
  try {
    // Get the channel master object
    cout << "\n### Acquiring channel master object" << endl;

    // Create a 10MiB file in 2MiB hugepage filesystem
    constexpr size_t superpageSize = 2*1024*1024;
    constexpr size_t superpageCount = 5;
    constexpr size_t bufferSize = superpageCount * superpageSize;
    roc::MemoryMappedFile file {"/dev/hugepages/rorc_example.bin", bufferSize};

    // Create parameters object for channel
    auto parameters = roc::Parameters()
        .setCardId(-1) // Dummy card
        .setChannelNumber(0) // DMA channel 0
        .setBufferParameters(roc::buffer_parameters::Memory{file.getAddress(), bufferSize}); // Register our buffer

    // Get the DMA channel
    std::shared_ptr<roc::ChannelMasterInterface> channel = roc::ChannelFactory().getMaster(parameters);

    // Start the DMA
    cout << "\n### Starting DMA" << endl;
    channel->startDma();

    // Keep track of time, so we don't wait forever for pages to arrive if things break
    const auto start = std::chrono::steady_clock::now();
    auto timeExceeded = [&]() { return ((std::chrono::steady_clock::now() - start) > std::chrono::seconds(5)); };

    cout << "### Pushing pages" << endl;

    // Queue up some superpages
    for (size_t i = 0; i < superpageCount; ++i) {
      auto offset = i * superpageSize;
      auto size = superpageSize;
      channel->pushSuperpage(roc::Superpage(offset, size));
      cout << "Pushed superpage " << i << '\n';
    }

    while(true) {
      if (timeExceeded()) {
        cout << "Time was exceeded!\n";
        break;
      }
      if (channel->getReadyQueueSize() == 0) {
        cout << "Done!\n";
        break;
      }

      // Does internal driver business, filling up superpages
      channel->fillSuperpages();

      // Get superpage at front of queue
      auto superpage = channel->getSuperpage();
      if (superpage.isReady()) {
        channel->popSuperpage();
        cout << "Superpage " << (superpage.offset / superpageSize) << " arrived\n";
      }

      // Give the CPU some resting time
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  catch (const std::exception& e) {
    // Most exceptions thrown from the library inherit from boost::exception and will contain information besides
    // the "what()" message that can help diagnose the problem. Here we print this information for the user.
    cout << boost::diagnostic_information(e) << endl;
  }

  return 0;
}
