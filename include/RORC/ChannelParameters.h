///
/// \file ChannelParameters.h
/// \author Pascal Boeschoten
///

#pragma once

#include <chrono>
#include <cstdint>
#include <boost/program_options/variables_map.hpp>

namespace AliceO2 {
namespace Rorc {

namespace ResetLevel
{
    /// Reset Level enumeration
    enum type
    {
      NOTHING = 0, RORC_ONLY = 1, RORC_DIU = 2, RORC_DIU_SIU = 3,
    };

    bool includesExternal(const ResetLevel::type& mode);
}

namespace LoopbackMode
{
    /// Loopback mode
    enum type
    {
      NONE = 0, EXTERNAL_DIU = 1, EXTERNAL_SIU = 2, INTERNAL_RORC = 3
    };

    bool isExternal(const LoopbackMode::type& mode);
}

namespace GeneratorPattern
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
}

/// DMA related parameters
struct DmaParameters
{
    DmaParameters();

    /// Size in bytes of the pages that the RORC must push.
    size_t pageSize;

    /// Size in bytes of the host's DMA buffer.
    int bufferSize;

    /// Instead of allocating the DMA buffer in kernel memory, allocate it in userspace shared memory.
    /// Note: at the moment, this option is just for testing, but shared memory will probably become the default, or
    /// even the only option in the future.
    bool useSharedMemory;
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
};

/// Timing related parameters
struct TimingParameters
{
    /// Defines the waiting period after each received fragment
    std::chrono::milliseconds sleepTime;

    /// Defines the waiting period before each time a new page is pushed
    std::chrono::milliseconds loadTime;

    /// Defines the waiting period for command responses
    std::chrono::milliseconds waitTime;
};

class ChannelParameters
{
  public:
    ChannelParameters();

    DmaParameters dma;
    GeneratorParameters generator;
    TimingParameters timing;

    /// Defines that the received fragment contains the Common Data Header
    bool ddlHeader;

    /// Prevents sending the RDYRX and EOBTR commands. TODO This switch is implicitly set when data generator or the STBRD command is used
    bool noRDYRX;

    /// Enforces that the data reading is carried out with the Start Block Read (STBRD) command
    bool useFeeAddress;

    /// Reset level on initialization of channel
    ResetLevel::type initialResetLevel;

    static ChannelParameters fromProgramOptions(const boost::program_options::variables_map& variablesMap);
};

} // namespace Rorc
} // namespace AliceO2
