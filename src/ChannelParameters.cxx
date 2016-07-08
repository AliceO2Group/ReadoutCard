///
/// \file ChannelParameters.cxx
/// \author Pascal Boeschoten
///

#include "RORC/ChannelParameters.h"

namespace AliceO2 {
namespace Rorc {

bool ResetLevel::includesExternal(const ResetLevel::type& mode)
{
  return mode == ResetLevel::RORC_DIU || mode == ResetLevel::RORC_DIU_SIU;
}

bool LoopbackMode::isExternal(const LoopbackMode::type& mode)
{
  return mode == LoopbackMode::EXTERNAL_SIU || mode == LoopbackMode::EXTERNAL_DIU;
}

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
  loopbackMode = Rorc::LoopbackMode::INTERNAL_RORC;
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

} // namespace Rorc
} // namespace AliceO2


std::string AliceO2::Rorc::ResetLevel::toString(const ResetLevel::type& level)
{
  static const std::map<ResetLevel::type, std::string> map {
    { ResetLevel::NOTHING, "NOTHING" },
    { ResetLevel::RORC_ONLY, "RORC_ONLY" },
    { ResetLevel::RORC_DIU, "RORC_DIU" },
    { ResetLevel::RORC_DIU_SIU, "RORC_DIU_SIU" },
  };

  if (map.count(level) != 0) {
    return map.at(level);
  }
  return std::string("UNKNOWN");
}

std::string AliceO2::Rorc::LoopbackMode::toString(const LoopbackMode::type& mode)
{
  static const std::map<LoopbackMode::type, std::string> map {
    { LoopbackMode::NONE, "NONE" },
    { LoopbackMode::INTERNAL_RORC, "INTERNAL_RORC" },
    { LoopbackMode::EXTERNAL_DIU, "EXTERNAL_DIU" },
    { LoopbackMode::EXTERNAL_SIU, "EXTERNAL_SIU" },
  };

  if (map.count(mode) != 0) {
    return map.at(mode);
  }
  return std::string("UNKNOWN");
}
