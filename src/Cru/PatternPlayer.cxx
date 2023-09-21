
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

uint128_t PatternPlayer::getValueFromString(const std::string &s, unsigned int nBits, const std::string &name) {
  if (nBits > 128) {
    nBits = 128;
  }
  uint128_t v = 0;
  uint128_t vmax = (((uint128_t)1) << nBits) - 1;
  std::string error = "Parsing parameter " + name + " : ";
  auto addDigit = [&] (uint128_t digit, uint128_t base) {
    bool overflow = 0;
    if (digit >= base) {
      overflow =1;
    } else if (v > (vmax / base)) {
      overflow = 1;
    }
    v = v * ((uint128_t)base);
    if (v > vmax - ((uint128_t)digit)) {
      overflow = 1;
    }
    v = v + ((uint128_t)digit);
    if (v > vmax) {
      overflow = 1;
    }
    if (overflow) {
      BOOST_THROW_EXCEPTION(InvalidOptionValueException()
                      << ErrorInfo::Message(error + "Value exceeds " + std::to_string(nBits) + " bits"));
    }
    return;
  };
  if (!strncmp(s.c_str(), "0x", 2)) {
    // hexadecimal string
    for (unsigned int i=2; i<s.length(); i++) {
      uint8_t c = s[i];
      uint128_t x = 0;
      if ( ( c >= '0') && ( c <= '9')) {
        x = c - '0';
      } else if ( ( c >= 'A') && ( c <= 'F')) {
        x = 10 + c - 'A';
      } else if ( ( c >= 'a') && ( c <= 'f')) {
        x = 10 + c - 'a';
      } else {
        BOOST_THROW_EXCEPTION(InvalidOptionValueException()
           << ErrorInfo::Message(error + "Value has wrong hexadecimal syntax"));
      }
      addDigit(x, (uint128_t)16);
    }
  } else {
    // decimal string
    for (unsigned int i=0; i<s.length(); i++) {
      uint8_t c = s[i];
      uint128_t x = 0;
      if ( ( c >= '0') && ( c <= '9')) {
        x = c - '0';
      } else {
        BOOST_THROW_EXCEPTION(InvalidOptionValueException()
           << ErrorInfo::Message(error + "Value has wrong decimal syntax"));
      }
      addDigit(x, (uint128_t)10);
    }
  }
  return v;
}

PatternPlayer::Info PatternPlayer::getInfoFromString(const std::vector<std::string> &parameters) {
  roc::PatternPlayer::Info ppInfo;

  int infoField = 0;
  for (const auto& parameter : parameters) {
    if (parameter.find('#') == std::string::npos) {
      infoField++;
    }
  }

  if (infoField != 15) { // Test that we have enough non-comment parameters
    BOOST_THROW_EXCEPTION(InvalidOptionValueException() << ErrorInfo::Message("Wrong number of non-comment parameters for the Pattern Player: " + std::to_string(infoField) + "/15"));
  }

  infoField = 0;
  for (const auto& parameter : parameters) {
    if (parameter.find('#') == std::string::npos) {
      switch (infoField) {
        bool boolValue;
        case 0:
          ppInfo.pat0 = PatternPlayer::getValueFromString(parameter, 80, "pat0");
          break;
        case 1:
          ppInfo.pat1 = PatternPlayer::getValueFromString(parameter, 80, "pat1");
          break;
        case 2:
          ppInfo.pat2 = PatternPlayer::getValueFromString(parameter, 80, "pat2");
          break;
        case 3:
          ppInfo.pat3 = PatternPlayer::getValueFromString(parameter, 80, "pat3");
          break;
        case 4:
          ppInfo.pat1Length = (uint32_t)PatternPlayer::getValueFromString(parameter, 32, "pat1Length");
          break;
        case 5:
          ppInfo.pat1Delay = (uint32_t)PatternPlayer::getValueFromString(parameter, 32, "pat1Delay");
          break;
        case 6:
          ppInfo.pat2Length = (uint32_t)PatternPlayer::getValueFromString(parameter, 32, "pat2Length");
          break;
        case 7:
          ppInfo.pat3Length = (uint32_t)PatternPlayer::getValueFromString(parameter, 32, "pat3Length");
          break;
        case 8:
          ppInfo.pat1TriggerSelect = (uint32_t)PatternPlayer::getValueFromString(parameter, 32, "pat1TriggerSelect");
          break;
        case 9:
          ppInfo.pat2TriggerSelect = (uint32_t)PatternPlayer::getValueFromString(parameter, 32, "pat2TriggerSelect");
          break;
        case 10:
          ppInfo.pat3TriggerSelect = (uint32_t)PatternPlayer::getValueFromString(parameter, 32, "pat3TriggerSelect");
          break;
        case 11:
          ppInfo.pat2TriggerTF = (uint32_t)PatternPlayer::getValueFromString(parameter, 32, "pat2TriggerTF");
          break;
        case 12:
          std::istringstream(parameter) >> std::boolalpha >> boolValue;
          ppInfo.exePat1AtStart = boolValue;
          break;
        case 13:
          std::istringstream(parameter) >> std::boolalpha >> boolValue;
          ppInfo.exePat1Now = boolValue;
          break;
        case 14:
          std::istringstream(parameter) >> std::boolalpha >> boolValue;
          ppInfo.exePat2Now = boolValue;
          break;
      }
      infoField++;
    }
  }
  return ppInfo;
}


} // namespace roc
} // namespace o2
