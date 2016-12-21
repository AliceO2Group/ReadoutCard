/// \file PdaBar.cxx
/// \brief Implementation of the PdaBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "PdaBar.h"

#include <limits>
#include <string>
#include <boost/lexical_cast.hpp>

#include "RORC/Exception.h"

namespace AliceO2 {
namespace Rorc {
namespace Pda {

PdaBar::PdaBar() : mPdaBar(nullptr), mBarLength(-1), mBarNumber(-1), mUserspaceAddress(nullptr)
{
}

PdaBar::PdaBar(PdaDevice::PdaPciDevice pciDevice, int barNumberInt) : mBarNumber(barNumberInt)
{
  uint8_t barNumber = mBarNumber;

  if (mBarNumber > std::numeric_limits<decltype(barNumber)>::max()) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("BAR number out of range (max 256)")
        << ErrorInfo::ChannelNumber(barNumber));
  }

  // Getting the BAR struct
  if(PciDevice_getBar(pciDevice.get(), &mPdaBar, barNumber) != PDA_SUCCESS) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Failed to get BAR")
        << ErrorInfo::ChannelNumber(barNumber));
  }

  // Mapping the BAR starting  address
  if(Bar_getMap(mPdaBar, const_cast<void**>(&mUserspaceAddress), &mBarLength) != PDA_SUCCESS) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Failed to map BAR")
        << ErrorInfo::ChannelNumber(barNumber));
  }
}

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2
