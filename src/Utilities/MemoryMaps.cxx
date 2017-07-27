/// \file MemoryMaps.cxx
/// \brief Implementation of functions for inspecting the process's memory mappings
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "MemoryMaps.h"

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <iostream>
#include <boost/throw_exception.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "Common/System.h"

namespace AliceO2 {
namespace roc {
namespace Utilities {

namespace {
std::string slurp(std::string file)
{
  std::ifstream ifstream(file);
  std::stringstream buffer;
  buffer << ifstream.rdbuf();
  return buffer.str();
}

/// Splits string into rows and "columns"
std::vector<std::vector<std::string>> tablify(std::string string) {
  std::vector<std::string> lines;
  boost::split(lines, string, boost::is_any_of("\n"), boost::token_compress_on);
  std::vector<std::vector<std::string>> splitLines(lines.size());
  for (size_t i = 0; i < lines.size(); ++i) {
    boost::split(splitLines.at(i), lines.at(i), boost::is_any_of(" "), boost::token_compress_on);
  }
  return splitLines;
}

struct Mapping
{
    uintptr_t addressStart;
    uintptr_t addressEnd;
    std::string permissions;
    uintptr_t offset;
    std::string dev;
    size_t inode;
    std::string path;
};

struct NumaMapping
{
    std::string path;
    size_t pageSizeKiB;
};

std::vector<Mapping> getMaps()
{
  std::vector<Mapping> maps;
  auto procMaps = slurp("/proc/self/maps");
  auto table = tablify(procMaps);

  for (const auto& row : table) {
    if (row.size() != 6) {
      continue;
    }

    Mapping mapping;
    std::vector<std::string> addressSplit;
    boost::split(addressSplit, row.at(0), boost::is_any_of("-"), boost::token_compress_on);
    mapping.addressStart = std::strtoul(addressSplit.at(0).c_str(), nullptr, 16);
    mapping.addressEnd = std::strtoul(addressSplit.at(1).c_str(), nullptr, 16);
    mapping.permissions = boost::trim_copy(row.at(1));
    mapping.offset = std::strtoul(row.at(2).c_str(), nullptr, 16);
    mapping.dev = boost::trim_copy(row.at(3));
    mapping.inode = boost::lexical_cast<size_t>(row.at(4));
    mapping.path = boost::trim_copy(row.at(5));
    maps.push_back(mapping);
  }

  return maps;
}

std::map<uintptr_t, NumaMapping> getNumaMaps()
{
  std::map<uintptr_t, NumaMapping> maps;
  auto numaMaps = slurp("/proc/self/numa_maps");
  auto table = tablify(numaMaps);

  for (const auto& row : table) {
    auto address = std::strtoul(row.at(0).c_str(), nullptr, 16);

    NumaMapping mapping;

    for (const auto& item : row) {
      auto setKeyValue = [&](std::string key, auto setFunction) {
        if (item.find(key) == 0) {
          setFunction(item.substr(key.size() + 1)); // Add 1 for the =
        }
      };

      setKeyValue("file", [&](auto value){mapping.path = value;});
      setKeyValue("kernelpagesize_kB", [&](auto value){mapping.pageSizeKiB = boost::lexical_cast<size_t>(value);});

      if (item.find("huge") == 0) {
        mapping.pageSizeKiB = 2*1024;
      }
    }

    maps[address] = mapping;
  }

  return maps;
}


} // Anonymous namespace

std::vector<MemoryMap> getMemoryMaps()
{
  std::vector<MemoryMap> memoryMaps;

  const auto maps = getMaps();
  const auto numaMaps = getNumaMaps();

  for (const auto& map : maps) {
    MemoryMap memMap;
    memMap.addressStart = map.addressStart;
    memMap.addressEnd = map.addressEnd;
    memMap.path = map.path;
    if (numaMaps.count(map.addressStart)) {
      memMap.pageSizeKiB = numaMaps.at(map.addressStart).pageSizeKiB;
    }
    memoryMaps.push_back(memMap);
  }

  return memoryMaps;
}

} // namespace Util
} // namespace roc
} // namespace AliceO2
