
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
/// \file Cru/DatapathWrapper.h
/// \brief Definition of Datpath Wrapper
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_DATAPATHWRAPPER_H_
#define O2_READOUTCARD_CRU_DATAPATHWRAPPER_H_

#include "Common.h"
#include "Pda/PdaBar.h"

namespace o2
{
namespace roc
{

using Link = Cru::Link;

class DatapathWrapper
{

 public:
  DatapathWrapper(std::shared_ptr<Pda::PdaBar> pdaBar);

  /// Set links with a bitmask
  void setLinksEnabled(uint32_t dwrapper, uint32_t mask);
  void setLinkEnabled(Link link);
  void setLinkDisabled(Link link);
  bool isLinkEnabled(Link link);
  void setDatapathMode(Link link, uint32_t mode);
  DatapathMode::type getDatapathMode(Link link);
  void setPacketArbitration(int wrapperCount, int arbitrationMode = 0);
  void setFlowControl(int wrapper, int allowReject = 0);
  uint32_t getFlowControl(int wrapper);
  void resetDataGeneratorPulse();
  void useDataGeneratorSource(bool enable);
  void enableDataGenerator(bool enable);
  void setDynamicOffset(int wrapper, bool enable);
  bool getDynamicOffsetEnabled(int wrapper);
  uint32_t getDroppedPackets(int wrapper);
  uint32_t getTotalPacketsPerSecond(int wrapper);
  uint32_t getAcceptedPackets(Link link);
  uint32_t getRejectedPackets(Link link);
  uint32_t getForcedPackets(Link link);
  void setTriggerWindowSize(int wrapper, uint32_t size = 1000);
  uint32_t getTriggerWindowSize(int wrapper);
  void toggleUserAndCommonLogic(bool enable, int wrapper);
  bool getUserAndCommonLogicEnabled(int wrapper);
  void setSystemId(Link link, uint32_t systemId);
  uint32_t getSystemId(Link link);
  void setFeeId(Link link, uint32_t feeId);
  uint32_t getFeeId(Link link);
  void setDropBadRdhEnabled(bool enable, int wrapper);
  bool getDropBadRdhEnabled(int wrapper);

  // generic function to retrieve given register for given link
  // (to avoid defining one getter function per register...)
  uint32_t getLinkRegister(const Link link, const Register reg);

 private:
  uint32_t getDatapathWrapperBaseAddress(int wrapper);

  std::shared_ptr<Pda::PdaBar> mPdaBar;
};
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_DATAPATHWRAPPER_H_
