///
/// \file RorcUtilsReadRegister.cxx
/// \author Pascal Boeschoten
///
/// Utility that reads a register from a RORC
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
    "Read Register",
    "Read a single register",
    "./rorc-reg-read -a0x8"
    );

int main(int argc, char** argv)
{
  auto optionsDescription = Options::createOptionsDescription();
  Options::addOptionRegisterAddress(optionsDescription);
  Options::addOptionChannel(optionsDescription);
  Options::addOptionSerialNumber(optionsDescription);

  try {
    auto variablesMap = Options::getVariablesMap(argc, argv, optionsDescription);
    int serialNumber = Options::getOptionSerialNumber(variablesMap);
    int address = Options::getOptionRegisterAddress(variablesMap);
    int channelNumber = Options::getOptionChannel(variablesMap);
    auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

    // Registers are indexed by 32 bits (4 bytes)
    uint32_t value = channel->readRegister(address / 4);
    std::cout << Common::makeRegisterString(address, value);

  } catch (std::exception& e) {
    RORC_UTILS_HANDLE_EXCEPTION(e, DESCRIPTION, optionsDescription);
  }

  return 0;
}
