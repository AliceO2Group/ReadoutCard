// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Sca.h
/// \brief Implementation of SCA operations
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Sca.h"
#include <chrono>
#include <vector>
#include "ReadoutCard/Register.h"
#include "Utilities/Util.h"

// TODO Sort out magic numbers

namespace AliceO2 {
namespace roc {

namespace Registers {
constexpr int CRU_BASE_INDEX = 0x4224000 / 4;
constexpr int CRU_LINK_OFFSET = 0x20000 / 4;
constexpr int CRU_MAX_LINKS = 7;
constexpr int CRORC_BASE_INDEX = 0x30 / 4;
constexpr int CRORC_LINK_OFFSET = 0x50 / 4;
constexpr int CRORC_MAX_LINKS = 2;
constexpr int WRITE_DATA = 0x20 / 4;
constexpr int WRITE_COMMAND = 0x24 / 4;
constexpr int CONTROL = 0x28 / 4;
constexpr int READ_DATA = 0x30 / 4;
constexpr int READ_COMMAND = 0x34 / 4;
constexpr int READ_BUSY = 0x38 / 4;
constexpr int READ_TIME = 0x3c / 4;
} // namespace Registers

constexpr auto BUSY_TIMEOUT = std::chrono::milliseconds(10);
constexpr auto CHANNEL_BUSY_TIMEOUT = std::chrono::milliseconds(10);

Sca::Sca(RegisterReadWriteInterface &bar2, CardType::type cardType, int link) : mBar2(bar2)
{
  auto setOffset = [&](auto base, auto offset, auto maxLinks) {
    if (link >= maxLinks ) {
      BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message("Maximum link number exceeded"));
    }
    mOffset = base + link * offset;
  };

  if (cardType == CardType::Cru) {
    setOffset(Registers::CRU_BASE_INDEX, Registers::CRU_LINK_OFFSET, Registers::CRU_MAX_LINKS);
  } else if (cardType == CardType::Crorc) {
    setOffset(Registers::CRORC_BASE_INDEX, Registers::CRORC_LINK_OFFSET, Registers::CRORC_MAX_LINKS);
  } else if (cardType == CardType::Dummy){
    setOffset(0, 0x100, 1);
  } else {
    throw std::runtime_error("Unknown card type, could not calculate SCA offset");
  }
}

void Sca::initialize()
{
  init();
  gpioEnable();
}

void Sca::init()
{
  barWrite(Registers::CONTROL, 0x1);
  waitOnBusyClear();
  barWrite(Registers::CONTROL, 0x2);
  waitOnBusyClear();
  barWrite(Registers::CONTROL, 0x1);
  waitOnBusyClear();
  barWrite(Registers::CONTROL, 0x0);
}

void Sca::write(uint32_t command, uint32_t data)
{
  barWrite(Registers::WRITE_DATA, data);
  barWrite(Registers::WRITE_COMMAND, command);
  auto transactionId = (command >> 16) & 0xff;
  if (transactionId == 0x0 || transactionId == 0xff) {
    BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message("Invalid transaction ID"));
  }
  executeCommand();
}

auto Sca::read() -> ReadResult
{
  auto data = barRead(Registers::READ_DATA);
  auto command = barRead(Registers::READ_COMMAND);
//  printf("Sca::read   DATA=0x%x   CH=0x%x   TR=0x%x   CMD=0x%x\n", data, command >> 24, (command >> 16) & 0xff, command & 0xff);

  auto endTime = std::chrono::steady_clock::now() + CHANNEL_BUSY_TIMEOUT;
  while (std::chrono::steady_clock::now() < endTime){
    if (!isChannelBusy(barRead(Registers::READ_COMMAND))) {
      checkError(command);
      return { command, data };
    }
  }
  BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message("Exceeded timeout on channel busy wait"));
}

bool Sca::isChannelBusy(uint32_t command)
{
  return (command & 0xff) == 0x40;
}

void Sca::checkError(uint32_t command)
{
  uint32_t errorCode = command & 0xff;

  auto toString = [&](int flag){
    switch (flag) {
      case 1:
        return "invalid channel request";
      case 2:
        return "invalid command request";
      case 3:
        return "invalid transaction number";
      case 4:
        return "invalid length";
      case 5:
        return "channel not enabled";
      case 6:
        return "channel busy";
      case 7:
        return "channel busy";
      case 0:
      default:
        return "generic error flag";
    }
  };

  // Check which error bits are enabled
  std::vector<int> flags;
  for (int flag = 0; flag < 7; ++flag) {
    if (Utilities::getBit(errorCode, flag) == 1) {
      flags.push_back(flag);
    }
  }

  // Turn into an error message
  if (!flags.empty()) {
    std::stringstream stream;
    stream << "error code 0x" << errorCode << ": ";
    for (size_t i = 0; i < flags.size(); ++i) {
      stream << toString(flags[i]);
      if (i < flags.size()) {
        stream << ", ";
      }
    }
    BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message(stream.str()));
  }
}

void Sca::gpioEnable()
{
  // Enable GPIO
  // WR CONTROL REG B
  write(0x00010002, 0xff000000);
  read();
  // RD CONTROL REG B
  write(0x00020003, 0xff000000);
  read();

  // WR GPIO DIR
  write(0x02030020, 0xffffffff);
  // RD GPIO DIR
  write(0x02040021, 0x0);
  read();
}

auto Sca::gpioWrite(uint32_t data) -> ReadResult
{
//  printf("Sca::gpioWrite DATA=0x%x\n", data);
  initialize();
  // WR REGISTER OUT DATA
  write(0x02040010, data);
  // RD DATA
  write(0x02050011, 0x0);
  read();
  // RD REGISTER DATAIN
  write(0x02060001, 0x0);
  return read();
}

auto Sca::gpioRead() -> ReadResult
{
//  printf("Sca::gpioRead\n", data);
  // RD DATA
  write(0x02050011, 0x0);
  return read();
}


void Sca::barWrite(int index, uint32_t data)
{
  mBar2.writeRegister(index + mOffset, data);
}

uint32_t Sca::barRead(int index)
{
  return mBar2.readRegister(index + mOffset);
}

void Sca::executeCommand()
{
  barWrite(Registers::CONTROL, 0x4);
  barWrite(Registers::CONTROL, 0x0);
  waitOnBusyClear();
}

void Sca::waitOnBusyClear()
{
  auto endTime = std::chrono::steady_clock::now() + BUSY_TIMEOUT;
  while (std::chrono::steady_clock::now() < endTime){
    if ((((barRead(Registers::READ_BUSY)) >> 31) & 0x1) == 0) {
      return;
    }
  }
  BOOST_THROW_EXCEPTION(ScaException() << ErrorInfo::Message("Exceeded timeout on busy wait"));
}


} // namespace roc
} // namespace AliceO2
