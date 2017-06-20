/// \file PythonInterface.cxx
/// \brief Python wrapper interface for simple channel actions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include <string>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/python.hpp>
#include "Common/GuardFunction.h"
#include "ExceptionInternal.h"
#include "ReadoutCard/ChannelFactory.h"

namespace {
using namespace AliceO2::roc;
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

  private:
    std::shared_ptr<AliceO2::roc::BarInterface> mBarChannel;
};
} // Anonymous namespace

// Note that the name given here to BOOST_PYTHON_MODULE must be the actual name of the shared object file this file is
// compiled into
BOOST_PYTHON_MODULE(libReadoutCard)
{
  using namespace boost::python;

  class_<BarChannel>("BarChannel", init<std::string, int>(sInitDocString))
      .def("register_read", &BarChannel::read, sRegisterReadDocString)
      .def("register_write", &BarChannel::write, sRegisterWriteDocString);
}

