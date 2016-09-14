/// \file PdaBar.cxx
/// \brief Implementation of the PdaBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "PdaBar.h"

#include <string>
#include <boost/lexical_cast.hpp>

#include "RORC/Exception.h"

namespace AliceO2 {
namespace Rorc {
namespace Pda {

PdaBar::PdaBar() : mPdaBar(nullptr), mBarLength(-1), mUserspaceAddress(nullptr)
{
}

PdaBar::PdaBar(PciDevice* pciDevice, int channel)
{
  // Getting the BAR struct
  if(PciDevice_getBar(pciDevice, &mPdaBar, channel) != PDA_SUCCESS ){
    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message("Failed to get BAR")
        << errinfo_rorc_channel_number(channel));
  }

  // Mapping the BAR starting  address
  if(Bar_getMap(mPdaBar, (void**) &mUserspaceAddress, &mBarLength) != PDA_SUCCESS ){
    BOOST_THROW_EXCEPTION(Exception()
        << errinfo_rorc_error_message("Failed to map BAR")
        << errinfo_rorc_channel_number(channel));
  }
}

} // namespace Pda
} // namespace Rorc
} // namespace AliceO2
