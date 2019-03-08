// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Numa.cxx
/// \brief Implementation of functions for inspecting the process's memory mappings
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Numa.h"
#include <fstream>
#include <sstream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "ExceptionInternal.h"
#include "Common/System.h"

namespace AliceO2 {
namespace roc {
namespace Utilities {
namespace b = boost;
namespace {

std::string getPciSysfsDirectory(const PciAddress& pciAddress)
{
  return (b::format("/sys/bus/pci/devices/0000:%s") % pciAddress.toString()).str();
}

std::string slurp(std::string filePath)
{
  std::ifstream ifstream;
  ifstream.open(filePath, std::ifstream::in);
  std::stringstream stringstream;
  stringstream << ifstream.rdbuf();
  ifstream.close();
  return stringstream.str();
}

} // Anonymous namespace

int getNumaNode(const PciAddress& pciAddress)
{
  auto string = slurp((b::format("%s/numa_node") % getPciSysfsDirectory(pciAddress)).str());
  string.pop_back(); // Pop the newline character, it messes up conversion
  int result = 0;
  if (!b::conversion::try_lexical_convert<int>(string, result)) {
    BOOST_THROW_EXCEPTION(
        Exception() << ErrorInfo::Message("Failed to get numa node") << ErrorInfo::PciAddress(pciAddress));
  }
  return result;
}

} // namespace Util
} // namespace roc
} // namespace AliceO2
