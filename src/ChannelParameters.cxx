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
  bufferSizeMiB = 512;
  pageSize = 2 * 1024 * 1024;
}

size_t DmaParameters::getBufferSizeBytes() const
{
  return bufferSizeMiB * 1024 * 1024;
}

FifoParameters::FifoParameters()
{
  dataOffset = 0;
  entries = 128;
  softwareOffset = 0;
}

size_t FifoParameters::getFullOffset() const
{
  return softwareOffset + entries * 8 + dataOffset;
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
