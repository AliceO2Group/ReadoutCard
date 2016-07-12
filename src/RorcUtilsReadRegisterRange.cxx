///
/// \file RorcUtilsReadRegisterRange.cxx
/// \author Pascal Boeschoten
///
/// Utility that reads a range of registers from a RORC
///

#include <iostream>
#include <stdexcept>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/ChannelFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsDescription.h"

using namespace AliceO2::Rorc::Util;

static const UtilsDescription DESCRIPTION(
    "Read Register Range",
    "Read a range of registers",
    "./rorc-reg-read-range -a0x8 -r10"
    );

int main(int argc, char** argv)
{
  auto optionsDescription = Options::createOptionsDescription();
  Options::addOptionRegisterAddress(optionsDescription);
  Options::addOptionChannel(optionsDescription);
  Options::addOptionSerialNumber(optionsDescription);
  Options::addOptionRegisterRange(optionsDescription);

  try {
    auto variablesMap = Options::getVariablesMap(argc, argv, optionsDescription);

    int serialNumber = Options::getOptionSerialNumber(variablesMap);
    int baseAddress = Options::getOptionRegisterAddress(variablesMap);
    int channelNumber = Options::getOptionChannel(variablesMap);
    int range = Options::getOptionRegisterRange(variablesMap);

    auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

    // Registers are indexed by 32 bits (4 bytes)
    int baseIndex = baseAddress / 4;

    for (int i = 0; i < range; ++i) {
      uint32_t value = channel->readRegister(baseIndex + i);
      std::cout << Common::makeRegisterString((baseIndex + i) * 4, value);
    }

  } catch (std::exception& e) {
    Options::printErrorAndHelp(boost::current_exception_diagnostic_information(), DESCRIPTION, optionsDescription);
  }

  return 0;
}
