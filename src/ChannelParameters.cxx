///
/// \file ChannelParameters.cxx
/// \author Pascal Boeschoten
///

#include "RORC/ChannelParameters.h"
#include <vector>
#include <map>

namespace AliceO2 {
namespace Rorc {

DmaParameters::DmaParameters()
{
  pageSize = 2 * 1024 * 1024;
  bufferSize = 512 * 1024 * 1024;
  useSharedMemory = false;
}

GeneratorParameters::GeneratorParameters()
{
  initialValue = 1;
  initialWord = 0;
  loopbackMode = Rorc::LoopbackMode::RORC;
  pattern = GeneratorPattern::INCREMENTAL;
  seed = 0;
  useDataGenerator = false;
  maximumEvents = 0;
  dataSize = 1 * 1024;
}
ChannelParameters::ChannelParameters()
{
  ddlHeader = 0;
  useFeeAddress = false;
  noRDYRX = true;
  initialResetLevel = Rorc::ResetLevel::NOTHING;
}

/// Flips a map around. Note that it will lead to data loss if multiple values of the original map are equal.
template <typename Map, typename ReverseMap = std::map<typename Map::mapped_type, typename Map::key_type>>
ReverseMap reverseMap(const Map& map)
{
  ReverseMap reverse;
  for (auto it = map.begin(); it != map.end(); ++it) {
    reverse.emplace(it->second, it->first);
  }
  return reverse;
}

/// Convenience function for implementing the enum to/from string functions
template <typename Map>
typename Map::mapped_type getValue(const Map& map, const typename Map::key_type& key)
{
  if (map.count(key) != 0) {
    return map.at(key);
  }
  BOOST_THROW_EXCEPTION(std::runtime_error("Invalid conversion"));
}

// ResetLevel functions

static const std::map<ResetLevel::type, std::string> resetLevelMap = {
  { ResetLevel::NOTHING, "NOTHING" },
  { ResetLevel::RORC, "RORC" },
  { ResetLevel::RORC_DIU, "RORC_DIU" },
  { ResetLevel::RORC_DIU_SIU, "RORC_DIU_SIU" },
};

static const auto resetLevelMapReverse = reverseMap(resetLevelMap);

bool ResetLevel::includesExternal(const ResetLevel::type& mode)
{
  return mode == ResetLevel::RORC_DIU || mode == ResetLevel::RORC_DIU_SIU;
}

std::string ResetLevel::toString(const ResetLevel::type& level)
{
  return getValue(resetLevelMap, level);
}

ResetLevel::type ResetLevel::fromString(const std::string& string)
{
  return getValue(resetLevelMapReverse, string);
}

// LoopbackMode functions

static const std::map<LoopbackMode::type, std::string> loopbackModeMap {
  { LoopbackMode::NONE, "NONE" },
  { LoopbackMode::RORC, "RORC" },
  { LoopbackMode::DIU, "DIU" },
  { LoopbackMode::SIU, "SIU" },
};

static const auto loopbackModeMapReverse = reverseMap(loopbackModeMap);

bool LoopbackMode::isExternal(const LoopbackMode::type& mode)
{
  return mode == LoopbackMode::SIU || mode == LoopbackMode::DIU;
}

std::string LoopbackMode::toString(const LoopbackMode::type& mode)
{
  return getValue(loopbackModeMap, mode);
}

LoopbackMode::type LoopbackMode::fromString(const std::string& string)
{
  return getValue(loopbackModeMapReverse, string);
}

} // namespace Rorc
} // namespace AliceO2

