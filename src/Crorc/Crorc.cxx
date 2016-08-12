/// \file Crorc.cxx
/// \brief Implementation of low level C-RORC functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Crorc.h"

#include <pda.h>
#include <string>
#include <memory>
#include "c/rorc/rorc.h"

namespace AliceO2 {
namespace Rorc {
namespace Crorc {

// TODO Clean up, C++ificate
int getSerial(volatile void* barAddress)
{
  // Getting the serial number of the card from the FLASH. See rorcSerial() in rorc_lib.c.
  char data[RORC_SN_LENGTH+1];
  memset(data, 'x', RORC_SN_LENGTH+1);
  unsigned int flashAddr;

  // Reading the FLASH.
  flashAddr = FLASH_SN_ADDRESS;
  initFlash(barAddress, flashAddr, 10); // TODO check returned status code

  // Setting the address to the serial number's (SN) position. (Actually it's the position
  // one before the SN's, because we need even position and the SN is at odd position.
  flashAddr += (RORC_SN_POSITION-1)/2;
  data[RORC_SN_LENGTH] = '\0';
  // Retrieving information character by caracter from the HW ID string.
  for (int i = 0; i < RORC_SN_LENGTH; i+=2, flashAddr++){
    readFlashWord(barAddress, flashAddr, &data[i], 10);
    if ((data[i] == '\0') || (data[i+1] == '\0')) {
      break;
    }
  }

  // We started reading the serial number one position before, so we don't need
  // the first character.
  memmove(data, data+1, strlen(data));
  int serial = 0;
  serial = atoi(data);

  return serial;
}

} // namespace Crorc
} // namespace Rorc
} // namespace AliceO2
