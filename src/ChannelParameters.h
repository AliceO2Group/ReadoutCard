/// \file ChannelParameters.h
/// \brief Definition of the ChannelParameters class and associated enums.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <chrono>
#include <string>
#include <cstdint>
#include <tuple>
#include <set>
#include <boost/program_options/variables_map.hpp>
#include <boost/lexical_cast.hpp>
#include "RORC/LoopbackMode.h"
#include "RORC/ResetLevel.h"
#include "RORC/GeneratorPattern.h"

namespace AliceO2 {
namespace Rorc {

/// DMA related parameters
struct DmaParameters
{
    DmaParameters();

    /// Size in bytes of the pages that the RORC must push.
    size_t pageSize;

    /// Size in bytes of the host's DMA buffer.
    size_t bufferSize;

    /// Instead of allocating the DMA buffer in kernel memory, allocate it in userspace shared memory.
    /// Note: at the moment, this option is just for testing, but shared memory will probably become the default, or
    /// even the only option in the future.
    bool useSharedMemory;

    inline bool operator==(const DmaParameters& other) const
    {
      auto tie = [](decltype(other) x) {
        return std::tie(x.bufferSize, x.pageSize, x.useSharedMemory);
      };
      return tie(*this) == tie(other);
    }
};

/// Generator related parameters
struct GeneratorParameters
{
    GeneratorParameters();

    /// If data generator is used, loopbackMode parameter is needed in this case
    bool useDataGenerator;

    /// Gives the type of loopback
    LoopbackMode::type loopbackMode;

    /// Data pattern parameter for the data generator
    GeneratorPattern::type pattern;

    /// Initial value of the first data in a data block
    uint32_t initialValue;

    /// Sets the second word of each fragment when the data generator is used
    uint32_t initialWord;

    /// Random seed parameter in case the data generator is set to produce random data
    int seed;

    /// Maximum number of events
    /// TODO Change to maximum number of pages
    int maximumEvents;

    /// Length of data written to each page
    size_t dataSize;

    inline bool operator==(const GeneratorParameters& other) const
    {
      auto tie = [](decltype(other) x) {
        return std::tie(x.dataSize, x.initialValue, x.initialWord, x.loopbackMode, x.maximumEvents, x.pattern, x.seed,
            x.useDataGenerator);
      };
      return tie(*this) == tie(other);
    }
};

/// Parameters for a RORC channel
struct ChannelParameters
{
    ChannelParameters();

    DmaParameters dma;
    GeneratorParameters generator;

    /// Defines that the received fragment contains the Common Data Header
    /// XXX Not sure if this should be a parameter...
    bool ddlHeader;

    /// Prevents sending the RDYRX and EOBTR commands. TODO This switch is implicitly set when data generator or the
    /// STBRD command is used
    /// XXX Not sure if this should be a parameter...
    bool noRDYRX;

    /// Enforces that the data reading is carried out with the Start Block Read (STBRD) command
    /// XXX Not sure if this should be a parameter...
    bool useFeeAddress;

    /// Reset level on initialization of channel
    ResetLevel::type initialResetLevel;

    inline bool operator==(const ChannelParameters& other) const
    {
      auto tie = [](decltype(other) x) {
        return std::tie(x.ddlHeader, x.dma, x.generator, x.initialResetLevel, x.noRDYRX, x.useFeeAddress);
      };
      return tie(*this) == tie(other);
    }
};

} // namespace Rorc
} // namespace AliceO2
