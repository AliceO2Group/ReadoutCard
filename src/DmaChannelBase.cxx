
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
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
#include "Pda/Util.h"
#include "ReadoutCard/FirmwareChecker.h"
#include "Utilities/SmartPointer.h"
#include "Visitor.h"

namespace o2
{
namespace roc
{

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

void DmaChannelBase::checkParameters(Parameters& /*parameters*/) //TODO: Possible use case?
{
}

DmaChannelBase::DmaChannelBase(CardDescriptor cardDescriptor, Parameters& parameters,
                               const AllowedChannels& allowedChannels)
  : mCardDescriptor(cardDescriptor), mChannelNumber(parameters.getChannelNumberRequired())
{
  mLoggerPrefix = "[" + mCardDescriptor.serialId.toString() + " | ch" + std::to_string(mChannelNumber) + "] ";
  Logger::setFacility("ReadoutCard/DMA");
#ifndef NDEBUG
  log("Backend compiled without NDEBUG; performance may be severely degraded", LogWarningDevel_(4200));
#endif

  // Check the channel number is allowed
  checkChannelNumber(allowedChannels);

  // Check that the firmware is compatible with the software
  auto parameters2 = parameters;
  parameters2.setChannelNumber(2);
  if (parameters.getFirmwareCheckEnabled().get_value_or(true)) {
    FirmwareChecker().checkFirmwareCompatibility(parameters2);
  }

  // Do some basic Parameters validity checks
  //checkParameters(parameters);

  //try to acquire lock
  log("Acquiring DMA channel lock", LogInfoDevel_(4201));
  try {
    if (mCardDescriptor.cardType == CardType::Crorc) {
      Utilities::resetSmartPtr(mInterprocessLock, "Alice_O2_RoC_DMA_" + cardDescriptor.pciAddress.toString() + "_chan" +
                                                    std::to_string(mChannelNumber) + "_lock");
    } else {
      Utilities::resetSmartPtr(mInterprocessLock, "Alice_O2_RoC_DMA_" + cardDescriptor.pciAddress.toString() + "_lock");
    }
  } catch (const LockException& exception) {
    log("Failed to acquire DMA channel lock", LogErrorDevel_(4202));
    throw;
  }

  log("Acquired DMA channel lock", LogInfoDevel_(4203));
  Pda::freePdaDmaBuffers(mCardDescriptor, getChannelNumber());
}

DmaChannelBase::~DmaChannelBase()
{
  Pda::freePdaDmaBuffers(mCardDescriptor, getChannelNumber());
  log("Releasing DMA channel lock", LogInfoDevel_(4204));
}

void DmaChannelBase::log(const std::string& logMessage, ILMessageOption ilgMsgOption)
{
  Logger::get() << mLoggerPrefix << logMessage << ilgMsgOption << endm;
}

} // namespace roc
} // namespace o2
