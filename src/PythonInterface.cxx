
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
/// \file PythonInterface.cxx
/// \brief Python wrapper interface for simple channel actions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include <string>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/python.hpp>
#include "Common/GuardFunction.h"
#include "ExceptionInternal.h"
#include "ReadoutCard/ChannelFactory.h"

namespace
{
using namespace o2::roc;
/// This is a Python wrapper class for a BAR channel. It only provides register read and write access.

/// Documentation for the init function (constructor)
auto sInitDocString =
  R"(Initializes a BarChannel object

Args:
    card id: String containing PCI address (e.g. 42:0.0) or serial number (e.g. 12345)
    channel number: Number of the BAR channel to open)";

/// Documentation for the register read function
auto sRegisterReadDocString =
  R"(Read the 32-bit value at given 32-bit aligned address

Args:
    index: 32-bit aligned address of the register
Returns:
    The 32-bit value of the register)";

/// Documentation for the register write function
auto sRegisterWriteDocString =
  R"(Write a 32-bit value at given 32-bit aligned address

Args:
    index: 32-bit aligned address of the register
    value: 32-bit value to write to the register)";

/// Documentation for the register modify function
auto sRegisterModifyDocString =
  R"(Modify the width# of bits value at given position of the 32-bit aligned address

Args:
    index: 32-bit aligned address of the register
    position: position to modify (0-31)
    width: number of bits to modify
    value: width bits value to write at position (masked to width if more))";

class BarChannel
{
 public:
  BarChannel(std::string cardIdString, int channelNumber)
  {
    auto cardId = Parameters::cardIdFromString(cardIdString);
    mBarChannel = ChannelFactory().getBar(Parameters::makeParameters(cardId, channelNumber));
  }

  uint32_t read(uint32_t address)
  {
    return mBarChannel->readRegister(address / 4);
  }

  void write(uint32_t address, uint32_t value)
  {
    return mBarChannel->writeRegister(address / 4, value);
  }

  void modify(uint32_t address, int position, int width, uint32_t value)
  {
    return mBarChannel->modifyRegister(address / 4, position, width, value);
  }

 private:
  std::shared_ptr<o2::roc::BarInterface> mBarChannel;
};
} // Anonymous namespace

// Note that the name given here to BOOST_PYTHON_MODULE must be the actual name of the shared object file this file is
// compiled into
BOOST_PYTHON_MODULE(libO2ReadoutCard)
{
  using namespace boost::python;

  class_<BarChannel>("BarChannel", init<std::string, int>(sInitDocString))
    .def("register_read", &BarChannel::read, sRegisterReadDocString)
    .def("register_write", &BarChannel::write, sRegisterWriteDocString)
    .def("register_modify", &BarChannel::modify, sRegisterModifyDocString);
}

// Keep libReadoutCard for backward compatility for now
BOOST_PYTHON_MODULE(libReadoutCard)
{
  using namespace boost::python;

  class_<BarChannel>("BarChannel", init<std::string, int>(sInitDocString))
    .def("register_read", &BarChannel::read, sRegisterReadDocString)
    .def("register_write", &BarChannel::write, sRegisterWriteDocString)
    .def("register_modify", &BarChannel::modify, sRegisterModifyDocString);
}
