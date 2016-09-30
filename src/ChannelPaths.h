/// \file ChannelPaths.h
/// \brief Definition of the ChannelPaths class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <boost/filesystem/path.hpp>
#include "RORC/CardType.h"

namespace AliceO2 {
namespace Rorc {

/// Namespace for functions to generate paths for filesystem object used by the Channel classes
class ChannelPaths
{
  public:

    /// Constructs the ChannelPaths object with the given parameters
    /// \param cardType Type of the card
    /// \param serial Serial number of the card
    /// \param channel Channel of the card
    ChannelPaths(CardType::type cardType, int serial, int channel);

    /// Generates a path for the shared memory DMA buffer for the pages to be pushed into
    /// \return The path
    boost::filesystem::path pages() const;

    /// Generates a path for the channel's shared memory state object
    /// \return The path
    boost::filesystem::path state() const;

    /// Generates a path for the channel file lock
    /// \return The path
    boost::filesystem::path lock() const;

    /// Generates a path for the channel's shared memory FIFO object
    /// \return The path
    boost::filesystem::path fifo() const;

    /// Generates a name for the channel's mutex
    /// \return The name
    std::string namedMutex() const;

  private:

    const CardType::type mCardType;
    const int mSerial;
    const int mChannel;
};

} // namespace Rorc
} // namespace AliceO2
