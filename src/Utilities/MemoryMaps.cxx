/// \file MemoryMaps.cxx
/// \brief Implementation of functions for inspecting the process's memory mappings
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <map>
#include <boost/throw_exception.hpp>
#include <boost/algorithm/string/split.hpp>

namespace AliceO2 {
namespace roc {
namespace Utilities {

std::vector<MemoryMap> getMemoryMaps()
{
  std::vector<MemoryMap> memoryMaps;

  auto procMaps = executeCommand("cat /proc/self/maps");
  auto procNumaMaps = executeCommand("cat /proc/self/numa_maps");

  auto splitLines = [](std::string string) {
    std::vector<std::string> lines;
    boost::split(lines, string, boost::is_any_of("\n"));

    std::vector<std::vector<std::string>> splitLines(lines.size());
    for (int i = 0; i < lines.size(); ++i) {
      boost::split(splitLines.at(i), lines, boost::is_any_of(" "));
    }

    return splitLines;
  };

  auto mapsTable = splitLines(procMaps);
  auto numaMapsTable = splitLines(procNumaMaps);

  for (int i = 0; i < mapsTable.size(); ++i) {
    std::vector<std::string> memoryRange;
    const auto& memoryRangeString = mapsTable.at(i).at(0);
    boost::split(memoryRange, memoryRangeString, boost::is_any_of(" "));
  }

  return memoryMaps;
}

} // namespace Util
} // namespace roc
} // namespace AliceO2
