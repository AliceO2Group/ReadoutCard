/// \file ChannelMasterInterface.h
/// \brief Definition of the ChannelMasterInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstdint>
#include "RORC/PageHandle.h"
#include "RORC/Page.h"
#include "RORC/ChannelParameters.h"
#include "RORC/CardType.h"
#include "RORC/RegisterReadWriteInterface.h"

namespace AliceO2 {
namespace Rorc {

/// Pure abstract interface for objects that obtain a master lock on a channel and provides an interface to control
/// and use that channel.
class ChannelMasterInterface: public virtual RegisterReadWriteInterface
{
  public:

    virtual ~ChannelMasterInterface()
    {
    }

    /// Start DMA for the given channel
    /// This must be called before pushing pages
    virtual void startDma() = 0;

    /// Stop DMA for the given channel
    virtual void stopDma() = 0;

    /// Reset the channel
    /// \param resetLevel The depth of the reset
    virtual void resetCard(ResetLevel::type resetLevel) = 0;

    /// Start pushing a page from the card to the host's DMA buffer and return a handle for the page.
    /// \return A handle to the page that can be used with other functions to check when it has arrived, and then to
    ///   access it.
    virtual PageHandle pushNextPage() = 0;

    /// Check if the page has arrived from the card to the host's DMA buffer
    /// \param handle The handle of the page returned from pushNextPage()
    /// \return True if the page has arrived, else false
    virtual bool isPageArrived(const PageHandle& handle) = 0;

    /// Get a page
    /// \param handle The handle of the page returned from pushNextPage()
    /// \return A Page object containing the address and size of the page
    virtual Page getPage(const PageHandle& handle) = 0;

    /// Mark a page as read, so it can be written to again
    /// \param handle The handle of the page returned from pushNextPage()
    virtual void markPageAsRead(const PageHandle& handle) = 0;

    /// Return the type of the RORC card this ChannelMaster is controlling
    /// \return The card type
    virtual CardType::type getCardType() = 0;
};

} // namespace Rorc
} // namespace AliceO2
