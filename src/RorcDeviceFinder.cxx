///
/// \file RorcDeviceFinder.cxx
/// \author Pascal Boeschoten
///

#include "RorcDeviceFinder.h"

#include <map>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
#include <functional>
#include <iostream>
#include "PdaBar.h"
#include "PdaDevice.h"
#include "RorcException.h"

namespace AliceO2 {
namespace Rorc {

namespace b = boost;
namespace bfs = boost::filesystem;

/// The PCI device ID for CRORC cards
static const std::string CRORC_DEVICE_ID = "0033";

/// The PCI device ID for CRU cards (TODO)
static const std::string CRU_DEVICE_ID = "????";

/// The PCI vendor ID of CERN
static const std::string CERN_VENDOR_ID = "10dc";

int crorcGetSerialNumber(const std::string& vendorId, const std::string deviceId);
int cruGetSerialNumber(const std::string& vendorId, const std::string deviceId);

const std::map<std::string, RorcDeviceFinder::CardType::type> typeMapping = {
    {CRORC_DEVICE_ID, RorcDeviceFinder::CardType::CRORC},
    {CRU_DEVICE_ID, RorcDeviceFinder::CardType::CRU}
};

const std::map<std::string, std::function<int(const std::string&, const std::string&)>> serialFunctionMapping = {
    {CRORC_DEVICE_ID, crorcGetSerialNumber},
    {CRU_DEVICE_ID, cruGetSerialNumber}
};

// Dumps the contents of a file into a string.
// Probably not the most efficient way, but the files this is used with are tiny so it doesn't matter.
std::string fileToString(const bfs::path& path) {
  bfs::ifstream fs(path);
  std::stringstream buffer;
  buffer << fs.rdbuf();
  return buffer.str();
}

RorcDeviceFinder::RorcDeviceFinder(int serialNumber)
    : pciDeviceId("unknown"), pciVendorId("unknown"), cardType(CardType::UNKNOWN), rorcSerialNumber(-1)
{
  // These files are mapped to PCI registers
  // For more info on these: https://en.wikipedia.org/wiki/PCI_configuration_space
  const bfs::path dirPath("/sys/bus/pci/devices/");

  if (!bfs::exists(dirPath)) {
    BOOST_THROW_EXCEPTION(RorcException()
            << errinfo_rorc_generic_message("Failed to open directory")
            << errinfo_rorc_directory(dirPath.string()));
  }

  for (auto& entry : boost::make_iterator_range(bfs::directory_iterator(dirPath), bfs::directory_iterator())) {
    const bfs::path dir = entry.path();

    // Get vendor & device ID, trimming off the leading '0x' and grabbing the next 4 characters
    const std::string vendorId = fileToString(dir/"vendor").substr(2, 4);
    const std::string deviceId = fileToString(dir/"device").substr(2, 4);

    if (vendorId != CERN_VENDOR_ID) {
      // Not a CERN card, not interesting
      continue;
    }

    if (typeMapping.count(deviceId) != 0) {
      // Found a CRORC or CRU. Check its serial number
      int tempSerial = serialFunctionMapping.at(deviceId)(vendorId, deviceId);

      std::cout << "Found serial: " << tempSerial << std::endl;

      //if (tempSerial == serialNumber) {
        // We found the right card
        rorcSerialNumber = serialNumber;
        cardType = typeMapping.at(deviceId);
        pciDeviceId = deviceId;
        pciVendorId = vendorId;
        return;
      //}
    }
  }

  BOOST_THROW_EXCEPTION(RorcException()
          << errinfo_rorc_generic_message("Failed to find RORC")
          << errinfo_rorc_possible_causes("Incorrect serial number")
          << errinfo_rorc_serial_number(serialNumber));
}

RorcDeviceFinder::~RorcDeviceFinder()
{
}

// The RORC headers have a lot of macros that cause problems with the rest of this file, so we include it down here.
#include "c/rorc/rorc.h"

// TODO Clean up, C++ificate
int crorcGetSerialNumber(const std::string& vendorId, const std::string deviceId)
{
  // Getting the serial number of the card from the FLASH. See rorcSerial() in rorc_lib.c.
  char data[RORC_SN_LENGTH+1];
  memset(data, 'x', RORC_SN_LENGTH+1);
  unsigned int flashAddr;
  unsigned int status = 0;

  PdaDevice pdaDevice(vendorId, deviceId);
  int channel = 0; // Must use channel 0 to access flash
  PdaBar pdaBar(pdaDevice.getPciDevice(), channel);
  uint32_t* barAddress = const_cast<uint32_t*>(pdaBar.getUserspaceAddressU32());

  // Reading the FLASH.
  flashAddr = FLASH_SN_ADDRESS;
  status = initFlash(barAddress, flashAddr, 10);

  // Setting the address to the serial number's (SN) position. (Actually it's the position
  // one before the SN's, because we need even position and the SN is at odd position.
  flashAddr += (RORC_SN_POSITION-1)/2;
  data[RORC_SN_LENGTH] = '\0';
  // Retrieving information character by caracter from the HW ID string.
  for (int i = 0; i < RORC_SN_LENGTH; i+=2, flashAddr++){
    readFlashWord(barAddress, flashAddr, (unsigned char*) &data[i], 10);
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

int cruGetSerialNumber(const std::string& vendorId, const std::string deviceId)
{
  BOOST_THROW_EXCEPTION(DeviceFinderException() << errinfo_rorc_generic_message("CRU unsupported"));
}


} // namespace Rorc
} // namespace AliceO2
