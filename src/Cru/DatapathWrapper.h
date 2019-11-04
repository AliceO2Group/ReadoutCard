// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Cru/DatapathWrapper.h
/// \brief Definition of Datpath Wrapper
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_DATAPATHWRAPPER_H_
#define ALICEO2_READOUTCARD_CRU_DATAPATHWRAPPER_H_

#include "Common.h"
#include "Pda/PdaBar.h"

namespace AliceO2
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
  bool getLinkEnabled(Link link);
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

 private:
  uint32_t getDatapathWrapperBaseAddress(int wrapper);

  std::shared_ptr<Pda::PdaBar> mPdaBar;
};
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_DATAPATHWRAPPER_H_
