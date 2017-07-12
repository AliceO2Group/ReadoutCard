/// \file Sca.h
/// \brief Implementation of ALICE Lowlevel Frontend (ALF) SCA operations
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Sca.h"
#include "ExceptionInternal.h"
#include "Utilities/Util.h"

// TODO Sort out magic numbers

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Alf {

namespace Registers {
constexpr int WRITE_DATA = 0x1e0 / 4;
constexpr int WRITE_COMMAND = 0x1e4 / 4;
constexpr int CONTROL = 0x1e8 / 4;
constexpr int BUSY = 0x1ec / 4;
constexpr int READ_DATA = 0x1f0 / 4;
constexpr int READ_COMMAND = 0x1f4 / 4;
constexpr int TIME = 0x1fc / 4;

} // namespace Registers

namespace Offset {
constexpr int CRORC = 0;
constexpr int CRU = 0;
constexpr int OTHER = 0;
}

constexpr int MAX_BUSY_ITERATIONS = 10000;

Sca::Sca(RegisterReadWriteInterface &bar2, CardType::type cardType) : bar2(bar2),
  offset((cardType == CardType::Crorc) ? Offset::CRORC : (cardType == CardType::Cru) ? Offset::CRU : Offset::OTHER)
{
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
  executeCommand();
  //auto time = barRead(Registers::TIME);
}

auto Sca::read() -> ReadResult
{
  auto data = barRead(Registers::READ_DATA);
  auto command = barRead(Registers::READ_COMMAND);
  checkError(command);
  return { data, command };
}

void Sca::checkError(uint32_t command)
{
  auto toString = [&](int flag){
    switch (flag) {
      case 1:
        return "Invalid channel request";
      case 2:
        return "Invalid command request";
      case 3:
        return "Invalid transaction number";
      case 4:
        return "Invalid length";
      case 5:
        return "Channel not enabled";
      case 6:
        return "Channel busy";
      case 7:
        return "Channel busy";
      case 0:
      default:
        return "Generic error flag";
    }
  };

  for (int flag = 0; flag < 7; ++flag) {
    if (Utilities::getBit(command & 0xff, flag) == 1) {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(toString(flag)));
    }
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
  init();
  // WR REGISTER OUT DATA
  write(0x02040010, data);
  // RD DATA
  write(0x02050011, 0x0);
  read();
  // RD REGISTER DATAIN
  write(0x02060001, 0x0);
  return read();
}

void Sca::barWrite(int index, uint32_t data)
{
  bar2.writeRegister(index + offset, data);
}

uint32_t Sca::barRead(int index)
{
  return bar2.readRegister(index + offset);
}

void Sca::executeCommand()
{
  barWrite(Registers::CONTROL, 0x4);
  barWrite(Registers::CONTROL, 0x0);
  waitOnBusyClear();
}

void Sca::waitOnBusyClear()
{
  for (int i = 0; i < MAX_BUSY_ITERATIONS; ++i) {
    if (barRead(Registers::BUSY) == 0) {
      return;
    }
  }
  BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Exceeded timeout on busy wait"));
}


} // namespace Alf
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
