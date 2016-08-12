///
/// \file ChannelParameters.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#include "RORC/ChannelParameters.h"
#include <vector>
#include <map>
#include <tuple>
#include "Util.h"

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
  loopbackMode = Rorc::LoopbackMode::Rorc;
  pattern = GeneratorPattern::Incremental;
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
  initialResetLevel = Rorc::ResetLevel::Nothing;
}

// ResetLevel functions

static const std::map<ResetLevel::type, std::string> resetLevelMap = {
  { ResetLevel::Nothing, "NOTHING" },
  { ResetLevel::Rorc, "RORC" },
  { ResetLevel::RorcDiu, "RORC_DIU" },
  { ResetLevel::RorcDiuSiu, "RORC_DIU_SIU" },
};

static const auto resetLevelMapReverse = Util::reverseMap(resetLevelMap);

bool ResetLevel::includesExternal(const ResetLevel::type& mode)
{
  return mode == ResetLevel::RorcDiu || mode == ResetLevel::RorcDiuSiu;
}

std::string ResetLevel::toString(const ResetLevel::type& level)
{
  return Util::getValueFromMap(resetLevelMap, level);
}

ResetLevel::type ResetLevel::fromString(const std::string& string)
{
  return Util::getValueFromMap(resetLevelMapReverse, string);
}

// LoopbackMode functions

static const std::map<LoopbackMode::type, std::string> loopbackModeMap {
  { LoopbackMode::None, "NONE" },
  { LoopbackMode::Rorc, "RORC" },
  { LoopbackMode::Diu, "DIU" },
  { LoopbackMode::Siu, "SIU" },
};

static const auto loopbackModeMapReverse = Util::reverseMap(loopbackModeMap);

bool LoopbackMode::isExternal(const LoopbackMode::type& mode)
{
  return mode == LoopbackMode::Siu || mode == LoopbackMode::Diu;
}

std::string LoopbackMode::toString(const LoopbackMode::type& mode)
{
  return Util::getValueFromMap(loopbackModeMap, mode);
}

LoopbackMode::type LoopbackMode::fromString(const std::string& string)
{
  return Util::getValueFromMap(loopbackModeMapReverse, string);
}

} // namespace Rorc
} // namespace AliceO2

