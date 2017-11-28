/// \file Driver.h
/// \brief Implementation of global functions for the ReadoutCard module
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/Driver.h"
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <Common/System.h>
#include <InfoLogger/InfoLogger.hxx>
#include "ReadoutCard/ParameterTypes/PciAddress.h"

namespace AliceO2 {
namespace roc {
namespace driver {
namespace {
}

void initialize()
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
  InfoLogger::InfoLogger logger;

  try {
    std::string pciPath = "/sys/bus/pci/drivers/uio_pci_dma/";
    if (boost::filesystem::exists(pciPath)) {
      for (auto &entry : boost::make_iterator_range(bfs::directory_iterator(pciPath), {})) {
        auto filename = entry.path().filename().string();
        if (filename.size() == 12) {
          // The PCI directory names are 12 characters long
          auto pciAddress = filename.substr(5); // Remove leading '0000:'

          if (PciAddress::fromString(pciAddress)) {
            // This is a valid PCI address
            std::string dmaPath("/sys/bus/pci/drivers/uio_pci_dma/" + filename + "/dma");
            for (auto &entry : boost::make_iterator_range(bfs::directory_iterator(dmaPath), {})) {
              auto bufferId = entry.path().filename().string();
              if (bfs::is_directory(entry)) {
                std::string mapPath = dmaPath + "/" + bufferId + "/map";
                std::string freePath = dmaPath + "/free";
                auto fuserResult = AliceO2::Common::System::executeCommand("fuser " + mapPath);
                if (fuserResult.empty()) {
                  // No process is using it, we can free the buffer!
                  logger << "Freeing PDA buffer '" + mapPath + "'" << InfoLogger::InfoLogger::endm;
                  auto fuserResult = AliceO2::Common::System::executeCommand("echo " + bufferId + " > " + freePath);
                }
              }
            }
          }
        }
      }
    }
  } catch (const boost::filesystem::filesystem_error &e) {
    logger << "Failed to free buffers: " << e.what() << InfoLogger::InfoLogger::endm;
    throw;
  }
}

} // namespace driver
} // namespace roc
} // namespace AliceO2
