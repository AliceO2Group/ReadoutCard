// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Util.h
/// \brief Definition of PDA utilities.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_PDA_UTIL_H_
#define ALICEO2_SRC_READOUTCARD_PDA_UTIL_H_

#include <boost/filesystem.hpp>
#include <vector>
#include <InfoLogger/InfoLogger.hxx>
#include "Common/System.h"
#include "Pda/PdaLock.h"
#include "ReadoutCard/CardDescriptor.h"

namespace bfs = boost::filesystem;

namespace AliceO2
{
namespace roc
{
namespace Pda
{

// PDA keeps a handle to buffers that are registered to it.
// This exists as a file "/sys/bus/pci/drivers/uio_pci_dma/[PCI address]/dma/[some number]/map"
// This can be problematic when a readout process crashes without giving the driver the chance to deregister DMA buffer,
// because then even if the readout's handle to the buffer is manually deleted, PDA's handle stays.
// And if there's not enough memory to create a new buffer, we are stuck.
//
// But there's a way out.
// If you echo that [some number] into "/sys/bus/pci/drivers/uio_pci_dma/[PCI address]/dma/free" it will be freed.
void freePdaDmaBuffersWrapped(const CardDescriptor cardDescriptor, const int channelNumber, bool force = false)
{
  namespace bfs = boost::filesystem;
  InfoLogger::InfoLogger logger;

  try {
    Pda::PdaLock lock{}; // We're messing around with PDA buffers so we need this even though we hold the DMA lock
  } catch (const LockException& exception) {
    logger << InfoLogger::InfoLogger::Debug << "Failed to acquire PDA lock" << InfoLogger::InfoLogger::endm;
    throw;
  }

  try {
    std::string pciPath = "/sys/bus/pci/drivers/uio_pci_dma/";
    if (boost::filesystem::exists(pciPath)) {
      for (auto& entry : boost::make_iterator_range(bfs::directory_iterator(pciPath), {})) {
        auto filename = entry.path().filename().string();
        if (filename.size() == 12) {
          // The PCI directory names are 12 characters long
          auto pciAddress = filename.substr(5); // Remove leading '0000:'

          if (force || (PciAddress::fromString(pciAddress) && (pciAddress == cardDescriptor.pciAddress.toString()))) {
            // This is a valid PCI address and it's *ours*
            std::string dmaPath("/sys/bus/pci/drivers/uio_pci_dma/" + filename + "/dma");
            for (auto& entry : boost::make_iterator_range(bfs::directory_iterator(dmaPath), {})) {
              auto bufferId = entry.path().filename().string();
              if (bfs::is_directory(entry)) {
                int bufferChannel = -1;
                if (bufferId.size() == 10) {
                  bufferChannel = stoi(bufferId.substr(6)) / 1000;
                } else if (bufferId.size() == 1) {
                  bufferChannel = stoi(bufferId);
                } else { // Some other dir entry - ignore
                  continue;
                }

                if (!force &&
                    cardDescriptor.cardType == CardType::Crorc &&
                    bufferChannel != channelNumber) {
                  continue;
                }

                std::string mapPath = dmaPath + "/" + bufferId + "/map";
                std::string freePath = dmaPath + "/free";
                logger << "Freeing PDA buffer '" + mapPath + "'" << InfoLogger::InfoLogger::endm;
                AliceO2::Common::System::executeCommand("echo " + bufferId + " > " + freePath);
              }
            }
          }
        }
      }
    }
  } catch (const boost::filesystem::filesystem_error& e) {
    logger << "Failed to free buffers: " << e.what() << InfoLogger::InfoLogger::endm;
    throw;
  }
}

void freePdaDmaBuffers()
{
  const CardDescriptor cardDescriptor = { CardType::Cru, -1, PciId{ "-1", "-1" }, PciAddress{ 0, 0, 0 }, -1 }; // a dummy card descriptor - call is anyway forced
  freePdaDmaBuffersWrapped(cardDescriptor, -1, true);
}

void freePdaDmaBuffers(const CardDescriptor cardDescriptor, const int channelNumber)
{
  freePdaDmaBuffersWrapped(cardDescriptor, channelNumber);
}

} // namespace Pda
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_PDA_UTIL_H_
