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
    if (filename.size() == 12) {
      // The PCI directory names are 12 characters long
      std::cout << "ENTRY = " << entry << " - " << filename << std::endl;
      auto pciAddress = filename.substr(5); // Remove leading '0000:'

      if (PciAddress::fromString(pciAddress)) {
        // This is a valid PCI address
        std::string dmaPath("/sys/bus/pci/drivers/uio_pci_dma/" + filename + "/dma/");
        for (auto &entry : boost::make_iterator_range(bfs::directory_iterator(dmaPath), {})) {
          auto bufferId = entry.path().filename().string();
          if (bfs::is_directory(entry)) {
            std::cout << "    DMA dir = " << entry << " - " << bufferId << std::endl;

            // TODO fuser & echo filename into /dma/[filename]/free
            std::string mapPath = dmaPath + bufferId + "/map";
            std::string freePath = dmaPath + bufferId + "/free";
            auto fuserResult = AliceO2::Common::System::executeCommand("fuser " + mapPath);
            fuserResult.pop_back(); // Get rid of trailing newline

            if (fuserResult.empty()) {
              // No process is using it, we can free the buffer!
              std::cout << "Freeing buffer '" + freePath + "'" << std::endl;
              auto fuserResult = AliceO2::Common::System::executeCommand("echo " + bufferId + " > " + freePath);
            }
          }
        }
      }
    }
  }
}

} // namespace roc
} // namespace AliceO2
