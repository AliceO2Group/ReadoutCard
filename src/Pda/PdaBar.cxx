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

PdaBar::PdaBar(PciDevice* pciDevice, int barNumberInt) : mBarNumber(barNumberInt)
{
  uint8_t barNumber = mBarNumber;

  if (mBarNumber > std::numeric_limits<decltype(barNumber)>::max()) {
    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message("BAR number out of range (max 256)")
        << errinfo_rorc_channel_number(barNumber));
  }

  // Getting the BAR struct
  if(PciDevice_getBar(pciDevice, &mPdaBar, barNumber) != PDA_SUCCESS) {
    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message("Failed to get BAR")
        << errinfo_rorc_channel_number(barNumber));
  }

  // Mapping the BAR starting  address
  if(Bar_getMap(mPdaBar, const_cast<void**>(&mUserspaceAddress), &mBarLength) != PDA_SUCCESS) {
    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message("Failed to map BAR")
        << errinfo_rorc_channel_number(barNumber));
  }
}

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2
