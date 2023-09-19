
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

  uint128_t pattern;

  pattern = info.pat0;
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT0_0.index, uint32_t(pattern & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT0_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT0_2.index, uint32_t((pattern >> 64) & 0xffff));

  pattern = info.pat1;
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT1_0.index, uint32_t(pattern & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT1_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT1_2.index, uint32_t((pattern >> 64) & 0xffff));

  pattern = info.pat2;
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT2_0.index, uint32_t(pattern & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT2_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT2_2.index, uint32_t((pattern >> 64) & 0xffff));

  pattern = info.pat3;
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT3_0.index, uint32_t(pattern & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT3_1.index, uint32_t((pattern >> 32) & 0xffffffff));
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT3_2.index, uint32_t((pattern >> 64) & 0xffff));

  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT1_LENGTH.index, info.pat1Length + info.pat1Delay);
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT1_DELAY_CNT.index, info.pat1Delay);
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT2_LENGTH.index, info.pat2Length);
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT3_LENGTH.index, info.pat3Length);

  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT1_TRIGGER_SEL.index, info.pat1TriggerSelect);
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT2_TRIGGER_SEL.index, info.pat2TriggerSelect);
  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT3_TRIGGER_SEL.index, info.pat3TriggerSelect);

  mBar->writeRegister(Cru::Registers::PATPLAYER_PAT2_TRIGGER_TF.index, info.pat2TriggerTF);

  configure(false);

  exePat1AtStart(info.exePat1AtStart);

  if (info.exePat2Now) {
    exePat2();
  }

  if (info.exePat1Now) {
    exePat1();
  }
}

PatternPlayer::Info PatternPlayer::read()
{
  PatternPlayer::Info info;
  uint128_t pattern;

  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT0_2.index);
  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT0_1.index) | pattern << 32;
  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT0_0.index) | pattern << 32;
  info.pat0 = pattern;

  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT1_2.index);
  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT1_1.index) | pattern << 32;
  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT1_0.index) | pattern << 32;
  info.pat1 = pattern;

  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT2_2.index);
  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT2_1.index) | pattern << 32;
  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT2_0.index) | pattern << 32;
  info.pat2 = pattern;

  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT3_2.index);
  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT3_1.index) | pattern << 32;
  pattern = mBar->readRegister(Cru::Registers::PATPLAYER_PAT3_0.index) | pattern << 32;
  info.pat3 = pattern;

  info.pat1Length = mBar->readRegister(Cru::Registers::PATPLAYER_PAT1_LENGTH.index) -
         mBar->readRegister(Cru::Registers::PATPLAYER_PAT1_DELAY_CNT.index);
  info.pat1Delay = mBar->readRegister(Cru::Registers::PATPLAYER_PAT1_DELAY_CNT.index);
  info.pat2Length = mBar->readRegister(Cru::Registers::PATPLAYER_PAT2_LENGTH.index);
  info.pat3Length = mBar->readRegister(Cru::Registers::PATPLAYER_PAT3_LENGTH.index);

  info.pat1TriggerSelect = mBar->readRegister(Cru::Registers::PATPLAYER_PAT1_TRIGGER_SEL.index);
  info.pat2TriggerSelect = mBar->readRegister(Cru::Registers::PATPLAYER_PAT2_TRIGGER_SEL.index);
  info.pat3TriggerSelect = mBar->readRegister(Cru::Registers::PATPLAYER_PAT3_TRIGGER_SEL.index);

  info.pat2TriggerTF = mBar->readRegister(Cru::Registers::PATPLAYER_PAT2_TRIGGER_TF.index);

  return info;
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

/*** No need to enable configuration for these last 3 ***/
void PatternPlayer::exePat1AtStart(bool enable)
{
  if (enable) {
    mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 12, 1, 0x1);
  } else {
    mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 12, 1, 0x0);
  }
}

void PatternPlayer::exePat1()
{
  mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 8, 1, 0x1);
  mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 8, 1, 0x0);
}

void PatternPlayer::exePat2()
{
  mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 4, 1, 0x1);
  mBar->modifyRegister(Cru::Registers::PATPLAYER_CFG.index, 4, 1, 0x0);
}

} // namespace roc
} // namespace o2
