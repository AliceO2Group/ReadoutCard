///
/// \file BarWrapper.cxx
/// \author Pascal Boeschoten
///

#include "PdaBar.h"

#include <string>
#include <boost/lexical_cast.hpp>

#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {

PdaBar::PdaBar() : pdaBar(nullptr), barLength(-1), userspaceAddress(nullptr)
{
}

PdaBar::PdaBar(PciDevice* pciDevice, int channel)
{
  // Getting the BAR struct
  if(PciDevice_getBar(pciDevice, &pdaBar, channel) != PDA_SUCCESS ){
    BOOST_THROW_EXCEPTION(AliceO2RorcException()
        << errinfo_aliceO2_rorc_generic_message("Failed to get BAR")
        << errinfo_aliceO2_rorc_channel_number(channel));
  }

  // Mapping the BAR starting  address
  if(Bar_getMap(pdaBar, (void**) &userspaceAddress, &barLength) != PDA_SUCCESS ){
    BOOST_THROW_EXCEPTION(AliceO2RorcException()
        << errinfo_aliceO2_rorc_generic_message("Failed to map BAR")
        << errinfo_aliceO2_rorc_channel_number(channel));
  }
}

} // namespace Rorc
} // namespace AliceO2
