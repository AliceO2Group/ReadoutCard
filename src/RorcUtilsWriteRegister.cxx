///
/// \file RorcUtilsWriteRegister.cxx
/// \author Pascal Boeschoten
///
/// Utility that writes to a register on a RORC
///

#include <stdexcept>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/ChannelFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsDescription.h"

using namespace AliceO2::Rorc::Util;

static const UtilsDescription DESCRIPTION(
    "Write Register",
    "Write a value to a single register",
    "./rorc-reg-write -a0x8 -v0"
    );

int main(int argc, char** argv)
{
  auto optionsDescription = Options::createOptionsDescription();
  Options::addOptionRegisterAddress(optionsDescription);
  Options::addOptionChannel(optionsDescription);
  Options::addOptionSerialNumber(optionsDescription);
  uint32_t registerValue = 0;
  optionsDescription.add_options()("value,v", boost::program_options::value<uint32_t>()->required(), "Register value");

  try {
    auto variablesMap = Options::getVariablesMap(argc, argv, optionsDescription);

    int serialNumber = Options::getOptionSerialNumber(variablesMap);
    int address = Options::getOptionRegisterAddress(variablesMap);
    int channelNumber = Options::getOptionChannel(variablesMap);
    auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

    // Registers are indexed by 32 bits (4 bytes)
    channel->writeRegister(address / 4, registerValue);

  } catch (std::exception& e) {
    Options::printErrorAndHelp(boost::current_exception_diagnostic_information(), DESCRIPTION, optionsDescription);
  }

  return 0;
}
