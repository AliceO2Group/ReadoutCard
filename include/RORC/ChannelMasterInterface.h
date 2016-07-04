///
/// \file ChannelMasterInterface.h
/// \author Pascal Boeschoten
///

#pragma once

#include <cstdint>
#include "RORC/Page.h"
#include "RORC/ChannelParameters.h"

namespace AliceO2 {
namespace Rorc {

/// Obtains a master lock on a channel and provides an interface to control and use the channel
class ChannelMasterInterface
{
  public:
    /// A handle that refers to a page. Used by some of the API calls.
    class PageHandle
    {
      public:
        inline PageHandle(int index = -1) : index(index)
        {
        }

        int index;
    };

    virtual ~ChannelMasterInterface()
    {
    }

    /// Start DMA for the given channel
    /// This must be called before pushing pages
    /// TODO Should this be done automatically?
    virtual void startDma() = 0;

    /// Stop DMA for the given channel
    /// TODO Should this be done automatically?
    virtual void stopDma() = 0;

    /// Reset the channel
    /// \param resetLevel The depth of the reset
    virtual void resetCard(ResetLevel::type resetLevel) = 0;

    /// Reads a BAR register. The registers are indexed per 32 bits
    /// \param index The index of the register
    virtual uint32_t readRegister(int index) = 0;

    /// Writes a BAR register
    /// \param index The index of the register
    /// \param value The value to be written into the register
    virtual void writeRegister(int index, uint32_t value) = 0;

    /// Push page and return handle to the pushed page.
    /// \return A handle to the page that can be used with other functions to check when it has arrived, and then to
    ///         access it.
    virtual PageHandle pushNextPage() = 0;

    /// Check if the page has arrived
    /// \handle The handle of the page returned from pushNextPage()
    /// \return True if the page has arrived, else false
    virtual bool isPageArrived(const PageHandle& handle) = 0;

    /// Get a page
    /// \handle The handle of the page returned from pushNextPage()
    /// \return A Page object containing the address of the page
    virtual Page getPage(const PageHandle& handle) = 0;

    /// Mark a page as read, so it can be written to again
    /// \handle The handle of the page returned from pushNextPage()
    virtual void markPageAsRead(const PageHandle& handle) = 0;
};

} // namespace Rorc
} // namespace AliceO2
