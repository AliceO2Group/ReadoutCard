// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file PatternPlayer.cxx
/// \brief Implementation of the PatternPlayer class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Constants.h"
#include "PatternPlayer.h"

using namespace boost::multiprecision;

namespace AliceO2
{
namespace roc
{

PatternPlayer::PatternPlayer(std::shared_ptr<Pda::PdaBar> pdaBar) : mPdaBar(pdaBar)
{
}

/// Configure has to be called to enable/disable pattern player configuration
void PatternPlayer::configure(bool startConfig)
{
  if (startConfig) {
    mPdaBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 0, 1, 0x1);
  } else {
    mPdaBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 0, 1, 0x0);
  }
}

void PatternPlayer::setIdlePattern(uint128_t pattern)
{
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_IDLE_PATTERN_0.index, uint32_t(pattern & 0xffffffff));
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_IDLE_PATTERN_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_IDLE_PATTERN_2.index, uint32_t((pattern >> 64) & 0xffff));
}

void PatternPlayer::setSyncPattern(uint128_t pattern)
{
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_SYNC_PATTERN_0.index, uint32_t(pattern & 0xffffffff));
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_SYNC_PATTERN_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_SYNC_PATTERN_2.index, uint32_t((pattern >> 64) & 0xffff));
}

void PatternPlayer::configureSync(uint32_t length, uint32_t delay)
{
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_SYNC_CNT.index, length + delay);
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_DELAY_CNT.index, delay);
}

void PatternPlayer::setResetPattern(uint128_t pattern)
{
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_RESET_PATTERN_0.index, uint32_t(pattern & 0xffffffff));
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_RESET_PATTERN_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_RESET_PATTERN_2.index, uint32_t((pattern >> 64) & 0xffff));
}

void PatternPlayer::configureReset(uint32_t length)
{
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_RESET_CNT.index, length);
}

void PatternPlayer::selectPatternTrigger(uint32_t syncTrigger, uint32_t resetTrigger)
{
  mPdaBar->writeRegister(Cru::Registers::PATPLAYER_TRIGGER_SEL.index, (resetTrigger << 16) | syncTrigger);
}

/*** No need to enable configuration for these last 3 ***/
void PatternPlayer::enableSyncAtStart(bool enable)
{
  if (enable) {
    mPdaBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 12, 1, 0x1);
  } else {
    mPdaBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 12, 1, 0x0);
  }
}

void PatternPlayer::triggerSync()
{
  mPdaBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 8, 1, 0x1);
  mPdaBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 8, 1, 0x0);
}

void PatternPlayer::triggerReset()
{
  mPdaBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 4, 1, 0x1);
  mPdaBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 4, 1, 0x0);
}

} // namespace roc
} // namespace AliceO2
