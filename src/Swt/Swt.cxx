// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Swt.cxx
/// \brief Definition of SWT operations
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Swt/Swt.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace AliceO2 {
namespace roc {

namespace Registers {
constexpr uint32_t SWT_BASE_INDEX = 0x0f00000 / 4;

constexpr uint32_t SWT_WR_WORD_L = 0x40 / 4;
constexpr uint32_t SWT_WR_WORD_M = 0x44 / 4;
constexpr uint32_t SWT_WR_WORD_H = 0x48 / 4;
constexpr uint32_t SWT_WR_CMD = 0x4c / 4;

constexpr uint32_t SWT_RD_WORD_L = 0x50 / 4;
constexpr uint32_t SWT_RD_WORD_M = 0x54 / 4;
constexpr uint32_t SWT_RD_WORD_H = 0x58 / 4;
constexpr uint32_t SWT_RD_CMD = 0x4c / 4;

constexpr uint32_t SWT_RD_WORD_MON = 0x5c / 4;

constexpr uint32_t SWT_SET_CHANNEL = 0x60 / 4; 
constexpr uint32_t SWT_RESET_CORE = 0x64 / 4; 

} //namespace Registers

Swt::Swt(RegisterReadWriteInterface &bar2, int gbtChannel) : mBar2(bar2)
{
  reset();
  setChannel(gbtChannel);
}

void Swt::setChannel(int gbtChannel)
{
  barWrite(Registers::SWT_SET_CHANNEL, gbtChannel);
}

void Swt::reset()
{
  barWrite(Registers::SWT_RESET_CORE, 0x1);
  barWrite(Registers::SWT_RESET_CORE, 0x0); //void cmd to sync clocks
}

uint32_t Swt::write(SwtWord& swtWord)
{
  // prep the swt word
  barWrite(Registers::SWT_WR_WORD_L, swtWord.getLow());
  barWrite(Registers::SWT_WR_WORD_M, swtWord.getMed());
  barWrite(Registers::SWT_WR_WORD_H, swtWord.getHigh());

  // perform write
  barWrite(Registers::SWT_WR_CMD, 0x1);
  barWrite(Registers::SWT_WR_CMD, 0x0); //void cmd to sync clocks

  return barRead(Registers::SWT_RD_WORD_MON);
}

uint32_t Swt::read(SwtWord& swtWord)
{
  barWrite(Registers::SWT_RD_CMD, 0x2);
  barWrite(Registers::SWT_RD_CMD, 0x0); // void cmd to sync clocks

  swtWord.setLow(barRead(Registers::SWT_RD_WORD_L));
  swtWord.setMed(barRead(Registers::SWT_RD_WORD_M));
  swtWord.setHigh(barRead(Registers::SWT_RD_WORD_H));

  //return 0x0;
  return barRead(Registers::SWT_RD_WORD_MON);
}

void Swt::barWrite(uint32_t offset, uint32_t data)
{
  mBar2.writeRegister(Registers::SWT_BASE_INDEX + offset, data);
}

uint32_t Swt::barRead(uint32_t offset)
{
  uint32_t read = mBar2.readRegister(Registers::SWT_BASE_INDEX + offset);
  return read;
}

} //namespace roc
} //namespace AliceO2
