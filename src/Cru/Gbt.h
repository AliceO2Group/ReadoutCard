
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
/// \file Gbt.h
/// \brief Definition of the Gbt class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_GBT_H_
#define O2_READOUTCARD_CRU_GBT_H_

#include "Pda/PdaBar.h"
#include "Common.h"
#include "Constants.h"
#include "I2c.h"

#include <map>

namespace o2
{
namespace roc
{

class Gbt
{

  using Link = Cru::Link;
  using LinkStatus = Cru::LinkStatus;
  using LoopbackStats = Cru::LoopbackStats;

 public:
  //Gbt(std::shared_ptr<Pda::PdaBar> pdaBar, std::vector<Link> &mLinkList, int wrapperCount);
  Gbt(std::shared_ptr<Pda::PdaBar> pdaBar, std::map<int, Link>& mLinkMap, int wrapperCount, int endpoint);
  void setMux(int link, uint32_t mux);
  void setInternalDataGenerator(Link link, uint32_t value);
  void setTxMode(Link link, uint32_t mode);
  void setRxMode(Link link, uint32_t mode);
  void setLoopback(Link link, uint32_t enabled);
  void calibrateGbt(std::map<int, Link> linkMap);
  void getGbtModes();
  void getGbtMuxes();
  void getLoopbacks();
  LinkStatus getStickyBit(Link link);
  uint32_t getRxClockFrequency(Link link);
  uint32_t getTxClockFrequency(Link link);
  void resetFifo();
  std::map<int, LoopbackStats> getLoopbackStats(bool reset, GbtPatternMode::type patternMode = GbtPatternMode::type::Counter,
                                                GbtCounterType::type counterType = GbtCounterType::type::ThirtyBit,
                                                GbtStatsMode::type statsMode = GbtStatsMode::type::All,
                                                uint32_t lowMask = 0xffffffff,
                                                uint32_t medMask = 0xffffffff,
                                                uint32_t highMask = 0xffffffff);
  std::map<int, LoopbackStats> getStats(GbtStatsMode::type statsMode);
  void setPatternMode(GbtPatternMode::type patternMode);
  void setTxCounterType(GbtCounterType::type counterType);
  void setRxPatternMask(uint32_t lowMask, uint32_t midMask, uint32_t highMask);

 private:
  uint32_t getSourceSelectAddress(Link link);
  uint32_t getStatusAddress(Link link);
  uint32_t getClearErrorAddress(Link link);
  uint32_t getRxControlAddress(Link link);
  uint32_t getTxControlAddress(Link link);
  uint32_t getAtxPllRegisterAddress(int wrapper, uint32_t reg);
  uint32_t getRxErrorCount(Link link);
  uint32_t getFecErrorCount(Link link);
  void resetErrorCounters();

  void atxcal(uint32_t baseAddress = 0x0);
  void cdrref(std::map<int, Link> linkMap, uint32_t refClock);
  void txcal(std::map<int, Link> linkMap);
  void rxcal(std::map<int, Link> linkMap);

  void resetStickyBit(Link link);

  std::shared_ptr<Pda::PdaBar> mPdaBar;
  std::map<int, Link>& mLinkMap;
  int mWrapperCount;
  int mEndpoint;
};
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_GBT_H_
