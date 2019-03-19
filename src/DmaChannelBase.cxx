// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DmaChannelBase.cxx
/// \brief Implementation of the DmaChannelBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/filesystem.hpp>
#include "DmaChannelBase.h"
#include <iostream>
//#include "ChannelPaths.h"
#include "Common/System.h"
#include "Utilities/SmartPointer.h"
#include "Visitor.h"

namespace AliceO2 {
namespace roc {

namespace b = boost;
namespace bfs = boost::filesystem;

void DmaChannelBase::checkChannelNumber(const AllowedChannels& allowedChannels)
{
  if (!allowedChannels.count(mChannelNumber)) {
    std::ostringstream stream;
    stream << "Channel number not supported, must be one of: ";
    for (int c : allowedChannels) {
      stream << c << ' ';
    }
    BOOST_THROW_EXCEPTION(InvalidParameterException() << ErrorInfo::Message(stream.str())
        << ErrorInfo::ChannelNumber(mChannelNumber));
  }
}

void DmaChannelBase::checkParameters(Parameters& parameters)
{
  // Generator enabled is not allowed in conjunction with none loopback mode: default to Internal
  auto enabled = parameters.getGeneratorEnabled();
  auto loopback = parameters.getGeneratorLoopback();
  if (enabled && loopback && (*enabled && (*loopback == LoopbackMode::None))) {
    log("Generator enabled, defaulting to LoopbackMode = Internal", InfoLogger::InfoLogger::Warning);
    parameters.setGeneratorLoopback(LoopbackMode::Internal);
  }

  // Generator disabled. Loopback mode should be set to None
  if (enabled && loopback && (!*enabled && (*loopback != LoopbackMode::None))){
    log("Generator disabled, defaulting to LoopbackMode = None", InfoLogger::InfoLogger::Warning);
    parameters.setGeneratorLoopback(LoopbackMode::None);
  }
}

void DmaChannelBase::freeUnusedChannelBuffer()
{
  namespace bfs = boost::filesystem;
  InfoLogger::InfoLogger logger;
 
  try {
    Pda::PdaLock lock{}; // We're messing around with PDA buffers so we need this even though we hold the DMA lock
  } catch (const LockException& exception) {
    log("Failed to acquire PDA lock", InfoLogger::InfoLogger::Debug);
    throw;
  }

  try {
    std::string pciPath = "/sys/bus/pci/drivers/uio_pci_dma/";
    if (boost::filesystem::exists(pciPath)) {
      for (auto &entry : boost::make_iterator_range(bfs::directory_iterator(pciPath), {})) {
        auto filename = entry.path().filename().string();
        if (filename.size() == 12) {
          // The PCI directory names are 12 characters long
          auto pciAddress = filename.substr(5); // Remove leading '0000:'

          if (PciAddress::fromString(pciAddress) && (pciAddress == mCardDescriptor.pciAddress.toString())) {
            // This is a valid PCI address and it's *ours*
            std::string dmaPath("/sys/bus/pci/drivers/uio_pci_dma/" + filename + "/dma");
            for (auto &entry : boost::make_iterator_range(bfs::directory_iterator(dmaPath), {})) {
              auto bufferId = entry.path().filename().string();
              if (bfs::is_directory(entry)) {
                if ((mCardDescriptor.cardType == CardType::Crorc) && (stoi(bufferId) != getChannelNumber())) { // don't free another channel's buffer
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
  } catch (const boost::filesystem::filesystem_error &e) {
    logger << "Failed to free buffers: " << e.what() << InfoLogger::InfoLogger::endm;
    throw;
  }
}

DmaChannelBase::DmaChannelBase(CardDescriptor cardDescriptor, Parameters& parameters,
    const AllowedChannels& allowedChannels)
    : mCardDescriptor(cardDescriptor), mChannelNumber(parameters.getChannelNumberRequired())
{
#ifndef NDEBUG
  log("Backend compiled without NDEBUG; performance may be severely degraded", InfoLogger::InfoLogger::Info);
#endif

  // Check the channel number is allowed
  checkChannelNumber(allowedChannels);

  // Do some basic Parameters validity checks
  checkParameters(parameters);

  //try to acquire lock
  log("Acquiring DMA channel lock", InfoLogger::InfoLogger::Debug);
  try{
    if (mCardDescriptor.cardType == CardType::Crorc) {
      Utilities::resetSmartPtr(mInterprocessLock, "Alice_O2_RoC_DMA_" + cardDescriptor.pciAddress.toString() + "_chan" + 
          std::to_string(mChannelNumber) + "_lock");
    } else {
      Utilities::resetSmartPtr(mInterprocessLock, "Alice_O2_RoC_DMA_" + cardDescriptor.pciAddress.toString() + "_lock");
    }
  }
  catch (const LockException& exception) {
    log("Failed to acquire DMA channel lock", InfoLogger::InfoLogger::Debug);
    throw;
  }

  log("Acquired DMA channel lock", InfoLogger::InfoLogger::Debug);

  freeUnusedChannelBuffer();
}

DmaChannelBase::~DmaChannelBase()
{
  freeUnusedChannelBuffer();
  log("Releasing DMA channel lock", InfoLogger::InfoLogger::Debug);
}

void DmaChannelBase::log(const std::string& message, boost::optional<InfoLogger::InfoLogger::Severity> severity)
{
  mLogger << severity.get_value_or(mLogLevel);
  mLogger << "[pci=" << getCardDescriptor().pciAddress.toString();
  if (auto serial = getSerialNumber()) {
    mLogger << " serial=" << serial.get();
  }
  mLogger << " channel=" << getChannelNumber() << "] ";
  mLogger << message;
  mLogger << InfoLogger::InfoLogger::endm;
}


} // namespace roc
} // namespace AliceO2
