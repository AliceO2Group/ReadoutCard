///
/// \file ChannelParameters.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <chrono>
#include <string>
#include <cstdint>
#include <tuple>
#include <boost/program_options/variables_map.hpp>

namespace AliceO2 {
namespace Rorc {

/// Namespace for the RORC reset level enum, and supporting functions
struct ResetLevel
{
    enum type
    {
      NOTHING = 0, RORC = 1, RORC_DIU = 2, RORC_DIU_SIU = 3,
    };

    /// Converts a ResetLevel to a string
    static std::string toString(const ResetLevel::type& level);

    /// Converts a string to a ResetLevel
    static ResetLevel::type fromString(const std::string& string);

    /// Returns true if the reset level includes external resets (SIU and/or DIU)
    static bool includesExternal(const ResetLevel::type& level);
};

/// Namespace for the RORC loopback mode enum, and supporting functions
struct LoopbackMode
{
    /// Loopback mode
    enum type
    {
      NONE = 0, DIU = 1, SIU = 2, RORC = 3
    };

    /// Converts a LoopbackMode to a string
    static std::string toString(const LoopbackMode::type& mode);

    /// Converts a string to a LoopbackMode
    static LoopbackMode::type fromString(const std::string& string);

    /// Returns true if the loopback mode is external (SIU and/or DIU)
    static bool isExternal(const LoopbackMode::type& mode);
};

/// Namespace for the RORC generator pattern enum
struct GeneratorPattern
{
  enum type
  {
    CONSTANT = 1,
    ALTERNATING = 2,
    FLYING_0 = 3,
    FLYING_1 = 4,
    INCREMENTAL = 5,
    DECREMENTAL = 6,
    RANDOM = 7
  };
};

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
      return tie((*this)) == tie(other);
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
      return tie((*this)) == tie(other);
    }
};

/// Parameters for a RORC channel
class ChannelParameters
{
  public:
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
      return tie((*this)) == tie(other);
    }
};

} // namespace Rorc
} // namespace AliceO2
