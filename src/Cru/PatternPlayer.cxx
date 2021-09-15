
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
/// \file PatternPlayer.cxx
/// \brief Implementation of the PatternPlayer class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Constants.h"
#include "Ttc.h"
#include "ReadoutCard/PatternPlayer.h"

using namespace boost::multiprecision;

namespace o2
{
namespace roc
{

PatternPlayer::PatternPlayer(std::shared_ptr<BarInterface> bar) : mBar(bar)
{
}

void PatternPlayer::play(PatternPlayer::Info info)
{
  Ttc ttc = Ttc(mBar);
  ttc.selectDownstreamData(DownstreamData::Pattern);

  configure(true);

  setIdlePattern(info.idlePattern);
  setSyncPattern(info.syncPattern);
  setResetPattern(info.resetPattern);

  configureSync(info.syncLength, info.syncDelay);
  configureReset(info.resetLength);

  selectPatternTrigger(info.syncTriggerSelect, info.resetTriggerSelect);
  configure(false);

  enableSyncAtStart(info.syncAtStart);

  if (info.triggerReset) {
    triggerReset();
  }

  if (info.triggerSync) {
    triggerSync();
  }
}

/// Configure has to be called to enable/disable pattern player configuration
void PatternPlayer::configure(bool startConfig)
{
  if (startConfig) {
    mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 0, 1, 0x1);
  } else {
    mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 0, 1, 0x0);
  }
}

void PatternPlayer::setIdlePattern(uint128_t pattern)
{
  mBar->writeRegister(Cru::Registers::PATPLAYER_IDLE_PATTERN_0.index, uint32_t(pattern & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_IDLE_PATTERN_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_IDLE_PATTERN_2.index, uint32_t((pattern >> 64) & 0xffff));
}

void PatternPlayer::setSyncPattern(uint128_t pattern)
{
  mBar->writeRegister(Cru::Registers::PATPLAYER_SYNC_PATTERN_0.index, uint32_t(pattern & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_SYNC_PATTERN_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_SYNC_PATTERN_2.index, uint32_t((pattern >> 64) & 0xffff));
}

void PatternPlayer::configureSync(uint32_t length, uint32_t delay)
{
  mBar->writeRegister(Cru::Registers::PATPLAYER_SYNC_CNT.index, length + delay);
  mBar->writeRegister(Cru::Registers::PATPLAYER_DELAY_CNT.index, delay);
}

void PatternPlayer::setResetPattern(uint128_t pattern)
{
  mBar->writeRegister(Cru::Registers::PATPLAYER_RESET_PATTERN_0.index, uint32_t(pattern & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_RESET_PATTERN_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_RESET_PATTERN_2.index, uint32_t((pattern >> 64) & 0xffff));
}

void PatternPlayer::configureReset(uint32_t length)
{
  mBar->writeRegister(Cru::Registers::PATPLAYER_RESET_CNT.index, length);
}

void PatternPlayer::selectPatternTrigger(uint32_t syncTrigger, uint32_t resetTrigger)
{
  mBar->writeRegister(Cru::Registers::PATPLAYER_TRIGGER_SEL.index, (resetTrigger << 16) | syncTrigger);
}

/*** No need to enable configuration for these last 3 ***/
void PatternPlayer::enableSyncAtStart(bool enable)
{
  if (enable) {
    mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 12, 1, 0x1);
  } else {
    mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 12, 1, 0x0);
  }
}

void PatternPlayer::triggerSync()
{
  mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 8, 1, 0x1);
  mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 8, 1, 0x0);
}

void PatternPlayer::triggerReset()
{
  mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 4, 1, 0x1);
  mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 4, 1, 0x0);
}

} // namespace roc
} // namespace o2
