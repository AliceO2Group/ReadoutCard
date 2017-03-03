/// \file ChannelParameters.cxx
/// \brief Implementation of the ChannelParameters class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelParameters.h"

namespace AliceO2 {
namespace Rorc {

DmaParameters::DmaParameters()
{
  pageSize = 8 * 1024;
//  bufferSize = 8 * 1024 * 1024;
  useSharedMemory = false;
}

GeneratorParameters::GeneratorParameters()
{
  initialValue = 1;
  initialWord = 0;
  loopbackMode = Rorc::LoopbackMode::Rorc;
  pattern = GeneratorPattern::Incremental;
  seed = 0;
  useDataGenerator = true;
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

} // namespace Rorc
} // namespace AliceO2

