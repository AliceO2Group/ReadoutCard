///
/// \file RorcDeviceFinder.cxx
/// \author Pascal Boeschoten
///

#include "RorcDeviceFinder.h"

#include <map>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/filesystem/fstream.hpp>
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

std::map<std::string, RorcDeviceFinder::CardType::type> typeMapping = {
    {CRORC_DEVICE_ID, RorcDeviceFinder::CardType::CRORC},
    {CRU_DEVICE_ID, RorcDeviceFinder::CardType::CRU}
};

RorcDeviceFinder::RorcDeviceFinder(int serialNumber)
    : cardType(CardType::UNKNOWN), pciDeviceId("unknown"), pciVendorId("unknown")
{
  // Dumps the contents of a file into a string.
  // Probably not the most efficient way, but the files this is used with are tiny so it doesn't matter.
  auto fileToString = [](const bfs::path& path) {
    bfs::ifstream fs(path);
    std::stringstream buffer;
    buffer << fs.rdbuf();
    return buffer.str();
  };

  // These files are mapped to PCI registers
  // For more info on these: https://en.wikipedia.org/wiki/PCI_configuration_space
  const bfs::path dirPath("/sys/bus/pci/devices/");

  if (!bfs::exists(dirPath)) {
    BOOST_THROW_EXCEPTION(AliceO2RorcException()
            << errinfo_aliceO2_rorc_generic_message("Failed to open directory")
            << errinfo_aliceO2_rorc_directory(dirPath.string()));
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
      cardType = typeMapping[deviceId];
      pciDeviceId = deviceId;
      pciVendorId = vendorId;
      return;
    }
  }

  BOOST_THROW_EXCEPTION(AliceO2RorcException()
          << errinfo_aliceO2_rorc_generic_message("Failed to find RORC")
          << errinfo_aliceO2_rorc_possible_causes("Incorrect serial number")
          << errinfo_aliceO2_rorc_serial_number(serialNumber));
}

RorcDeviceFinder::~RorcDeviceFinder()
{
}

} // namespace Rorc
} // namespace AliceO2
