/// \file ChannelPaths.h
/// \brief Definition of the ChannelPaths class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <string>
#include "RORC/ParameterTypes/PciAddress.h"

namespace AliceO2 {
namespace Rorc {

/// Namespace for functions to generate paths for filesystem object used by the Channel classes
class ChannelPaths
{
  public:

    /// Constructs the ChannelPaths object with the given parameters
    /// \param pciAddress PCI address of the card
    /// \param channel Channel of the card
    ChannelPaths(PciAddress pciAddress, int channel);

    /// Generates a path for the channel file lock
    /// \return The path
    std::string lock() const;

    /// Generates a path for the channel's shared memory FIFO object. It will be in hugetlbfs.
    /// \return The path
    std::string fifo() const;

    /// Generates a name for the channel's mutex
    /// \return The name
    std::string namedMutex() const;

  private:

    std::string makePath(std::string fileName, const char* directory) const;

    const PciAddress mPciAddress;
    const int mChannel;
};

} // namespace Rorc
} // namespace AliceO2
