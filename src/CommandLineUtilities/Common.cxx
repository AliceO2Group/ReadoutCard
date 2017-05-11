/// \file Common.cxx
/// \brief Implementation of common functions for the ReadoutCard utilities.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CommandLineUtilities/Common.h"
#include <iostream>
#include <iomanip>
#include <bitset>

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {
namespace Common {

using std::cout;
using std::endl;
namespace po = boost::program_options;

std::string make32bitString(uint32_t bits)
{
  std::ostringstream oss;
  for (int i = 0; i < 8; ++i) {
    oss << std::bitset<4>(bits >> ((7 - i) * 4));
    if (i < 7) {
      oss << ".";
    }
  }
  return oss.str();
}

std::string make32hexString(uint32_t bits)
{
  std::ostringstream oss;
  oss << std::hex << std::setw(4) << std::setfill('0') << uint16_t(bits >> 16)
    << "." << std::hex << std::setw(4) << std::setfill('0') << uint16_t(bits);
  return oss.str();
}

std::string makeRegisterAddressString(int address)
{
  std::ostringstream oss;
  oss << std::hex << std::setw(3) << std::setfill('0') << address;
  return oss.str();
}

std::string makeRegisterString(int address, uint32_t value)
{
  std::ostringstream oss;
  oss << "  0x" << makeRegisterAddressString(address)
    << "  =>  0x" << make32hexString(value)
    << "  =  0b" << make32bitString(value)
    << "  =  " << value
    << endl;
  return oss.str();
}

} // namespace Common
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
