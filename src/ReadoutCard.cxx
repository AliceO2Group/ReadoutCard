/// \file ReadoutCard.h
/// \brief Implementation of global functions for the ReadoutCard module
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ReadoutCard.h"
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <Common/System.h>

namespace AliceO2 {
namespace roc {
namespace {
}

void initializeDriver()
{
  freeUnusedChannelBuffers();
}

// PDA keeps a handle to buffers that are registered to it.
// This exists as a file "/sys/bus/pci/drivers/uio_pci_dma/[PCI address]/dma/[some number]/map"
// If you echo that [some number] into "/sys/bus/pci/drivers/uio_pci_dma/[PCI address]/dma/free" it will be freed.
// Not sure what happens if you do this while it's in use by a readout process...
void freeUnusedChannelBuffers()
{
  namespace bfs = boost::filesystem;

  bfs::path pciPath("/sys/bus/pci/drivers/uio_pci_dma/");
  for (auto& entry : boost::make_iterator_range(bfs::directory_iterator(pciPath), {})) {
    auto filename = entry.path().filename().string();
    std::cout << "ENTRY = " << entry << " - " << filename << std::endl;

    if (auto address = PciAddress::fromString(filename)) {
      std::string dmaPath("/sys/bus/pci/drivers/uio_pci_dma/" + filename + "/dma/");
      for (auto& entry : boost::make_iterator_range(bfs::directory_iterator(dmaPath), {})) {
        auto filename = entry.path().filename().string();
        if (bfs::is_directory(entry)) {
          std::cout << "    DMA dir = " << entry << " - " << filename << std::endl;

          // TODO fuser & echo filename into /dma/[filename]/free
          std::string mapPath = dmaPath + "/map";
          std::string freePath = dmaPath + "/free";
          auto fuserResult = AliceO2::Common::System::executeCommand("fuser " + mapPath);
          fuserResult.pop_back(); // Get rid of trailing newline

          if (fuserResult.empty()) {
            // No process is using it, we can free the buffer!
            auto fuserResult = AliceO2::Common::System::executeCommand("echo " + filename + " > " + freePath);
          }
        }
      }
    }

//    auto mapPath = entry + "/dma/map";

//    auto lsResult = AliceO2::Common::System::executeCommand("ls /sys/bus/pci/drivers/uio_pci_dma/*/dma/*/map");
//    auto fuserResult = AliceO2::Common::System::executeCommand(
//      "fuser /sys/bus/pci/drivers/uio_pci_dma/0000\:42\:00.0/dma/*/map");

//    if (isInUse(mapPath)) {
//      bfs::remove(mapPath);
//    }
  }
}

} // namespace roc
} // namespace AliceO2
