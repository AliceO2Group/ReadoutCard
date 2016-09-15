/// \file ParameterMap.h
/// \brief Definition of the ParameterMap and associated methods
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <map>
#include <set>
#include <string>

namespace AliceO2 {
namespace Rorc {
namespace Parameters {

using Map = std::map<std::string, std::string>;

namespace Keys
{
  inline std::string dmaBufferSize() { return "dma_buffer_size"; }
  inline std::string dmaPageSize() { return "dma_page_size"; }
  inline std::string generatorEnabled() { return "generator_enabled"; }
  inline std::string generatorLoopbackMode() { return "generator_loopback_mode"; }
  inline std::string generatorDataSize() { return "generator_data_size"; }

  inline std::set<std::string> all() {
    return {
      dmaBufferSize(),
      dmaPageSize(),
      generatorEnabled(),
      generatorLoopbackMode(),
      generatorDataSize()};
  }
}

} // namespace Parameters
} // namespace Rorc
} // namespace AliceO2
