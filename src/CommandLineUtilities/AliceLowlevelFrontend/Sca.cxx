/// \file Sca.h
/// \brief Implementation of ALICE Lowlevel Frontend (ALF) SCA operations
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Sca.h"

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Alf {

namespace Registers
{
constexpr int DATA    = 0x1e0 / 4;
constexpr int COMMAND = 0x1e4 / 4;
constexpr int CONTROL = 0x1e8 / 4;
constexpr int READ_DATA    = 0x1f0 / 4;
constexpr int READ_COMMAND = 0x1f4 / 4;
constexpr int READ_TIME    = 0x1ec / 4;
} // namespace Registers

namespace Offset
{
constexpr int CRORC = 0;
constexpr int CRU = 0;
constexpr int OTHER = 0;
}

Sca::Sca(RegisterReadWriteInterface &bar2, CardType::type cardType) : bar2(bar2),
  offset((cardType == CardType::Crorc) ? Offset::CRORC : (cardType == CardType::Cru) ? Offset::CRU : Offset::OTHER)
{
}

void Sca::init()
{
  barWrite(Registers::CONTROL, 0x1);
  barWrite(Registers::CONTROL, 0x2);
  barWrite(Registers::CONTROL, 0x1);
  barWrite(Registers::CONTROL, 0x0);
}

void Sca::write(uint32_t command, uint32_t data)
{
  barWrite(Registers::DATA, data);
  barWrite(Registers::COMMAND, command);
  barWrite(Registers::CONTROL, 0x4);
  barWrite(Registers::CONTROL, 0x0);
}

auto Sca::read() -> ReadResult
{
  return {
    barRead(Registers::READ_DATA),
    barRead(Registers::READ_COMMAND),
    barRead(Registers::READ_TIME)
  };
}

auto Sca::gpioWrite(uint32_t data) -> ReadResult
{
  init();
  write(0x00013302, 0xFf000000);
  write(0x02023320, 0xFfffFfff);
  write(0x02031010, data);
  write(0x02030011, 0x0);
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

} // namespace Alf
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
