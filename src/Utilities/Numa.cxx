/// \file MemoryMaps.cxx
/// \brief Implementation of functions for inspecting the process's memory mappings
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "Numa.h"
#include <boost/format.hpp>

namespace AliceO2 {
namespace Rorc {
namespace Utilities {
namespace b = boost;
namespace {

std::string getPciSysfsDirectory(const PciAddress& pciAddress)
{
  b::str(b::format("/sys/bus/pci/devices/0000:%s") % pciAddress.toString());
}


} // Anonymous namespace

int getNumaNode(const PciAddress& pciAddress)
{
  auto string = executeCommand(b::str(b::format("cat %s/numa_node") % getPciSysfsDirectory(pciAddress)));
  int result = 0;
  if (!b::try_lexical_convert<int>(string, result)) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Failed to get numa node")
        << ErrorInfo::PciAddress(pciAddress));
  }
  return result;
}

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
