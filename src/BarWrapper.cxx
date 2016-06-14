///
/// \file BarWrapper.cxx
/// \author Pascal Boeschoten
///

#include "BarWrapper.h"
#include <string>
#include <boost/lexical_cast.hpp>

#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {

BarWrapper::BarWrapper() : pdaBar(nullptr), barLength(-1), userspaceAddress(nullptr)
{
}

BarWrapper::BarWrapper(PciDevice* pciDevice, int channel)
{
  // Getting the BAR struct
  if(PciDevice_getBar(pciDevice, &pdaBar, channel) != PDA_SUCCESS ){
    ALICEO2_RORC_THROW_EXCEPTION("Failed to get BAR");
  }

  // Mapping the BAR starting  address
  if(Bar_getMap(pdaBar, (void**) &userspaceAddress, &barLength) != PDA_SUCCESS ){
    ALICEO2_RORC_THROW_EXCEPTION("Failed to map BAR");
  }
}

} // namespace Rorc
} // namespace AliceO2
