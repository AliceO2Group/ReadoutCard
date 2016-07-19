///
/// \file ChannelParameters.cxx
/// \author Pascal Boeschoten
///

#include "RORC/ChannelParameters.h"
#include <vector>
#include <map>
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

// ResetLevel functions

static const std::map<ResetLevel::type, std::string> resetLevelMap = {
  { ResetLevel::NOTHING, "NOTHING" },
  { ResetLevel::RORC, "RORC" },
  { ResetLevel::RORC_DIU, "RORC_DIU" },
  { ResetLevel::RORC_DIU_SIU, "RORC_DIU_SIU" },
};

static const auto resetLevelMapReverse = Util::reverseMap(resetLevelMap);

bool ResetLevel::includesExternal(const ResetLevel::type& mode)
{
  return mode == ResetLevel::RORC_DIU || mode == ResetLevel::RORC_DIU_SIU;
}

std::string ResetLevel::toString(const ResetLevel::type& level)
{
  return Util::getValue(resetLevelMap, level);
}

ResetLevel::type ResetLevel::fromString(const std::string& string)
{
  return Util::getValue(resetLevelMapReverse, string);
}

// LoopbackMode functions

static const std::map<LoopbackMode::type, std::string> loopbackModeMap {
  { LoopbackMode::NONE, "NONE" },
  { LoopbackMode::RORC, "RORC" },
  { LoopbackMode::DIU, "DIU" },
  { LoopbackMode::SIU, "SIU" },
};

static const auto loopbackModeMapReverse = Util::reverseMap(loopbackModeMap);

bool LoopbackMode::isExternal(const LoopbackMode::type& mode)
{
  return mode == LoopbackMode::SIU || mode == LoopbackMode::DIU;
}

std::string LoopbackMode::toString(const LoopbackMode::type& mode)
{
  return Util::getValue(loopbackModeMap, mode);
}

LoopbackMode::type LoopbackMode::fromString(const std::string& string)
{
  return Util::getValue(loopbackModeMapReverse, string);
}

} // namespace Rorc
} // namespace AliceO2

