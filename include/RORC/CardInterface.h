///
/// \file CardInterface.h
/// \author Pascal Boeschoten
///

#pragma once

#include <vector>
#include <chrono>
#include <cstdint>
#include "RORC/Page.h"
#include "RORC/ChannelParameters.h"

namespace AliceO2 {
namespace Rorc {

using PageVector = std::vector<Page>;

/// A handle to index a page
/// TODO Make more private
/// TODO Move to separate file? Or make it a CardInterface member class?
class PageHandle
{
  public:
    inline PageHandle(int index = -1)
        : index(index)
    {
    }

    int index;
};

/// High-level C++ interface class for the RORC API
class CardInterface
{
  public:
    virtual ~CardInterface() 
    {
    }

    /// Opens a DMA channel
    /// \param channel The channel number
    /// \param channelParameters The configuration parameters for the channel
    virtual void openChannel(int channel, const ChannelParameters& channelParameters) = 0;

    /// Closes a DMA channel
    /// \param channel The channel number
    virtual void closeChannel(int channel) = 0;

    /// Start DMA for the given channel
    /// This must be called before pushing pages
    /// \param channel The channel number
    virtual void startDma(int channel) = 0;

    /// Stop DMA for the given channel
    /// This should probably be called before closing a channel that has started DMA
    /// \param channel The channel number
    virtual void stopDma(int channel) = 0;

    /// Reset the card
    /// \param channel The channel number
    /// \param resetLevel The depth of the reset
    virtual void resetCard(int channel, ResetLevel::type resetLevel) = 0;

    /// Reads a BAR register. The registers are indexed by 32 bits
    /// \param channel The channel number
    /// \param channel The index of the register
    virtual uint32_t readRegister(int channel, int index) = 0;

    /// Writes a BAR register
    /// \param channel The channel number
    /// \param index The index of the register
    /// \param value The value to be written into the register
    virtual void writeRegister(int channel, int index, uint32_t value) = 0;

    /// Push page and return handle to the pushed page.
    /// \param channel The channel number
    /// \return A handle to the page that can be used with other functions to check when it has arrived, and then to
    ///         access it.
    virtual PageHandle pushNextPage(int channel) = 0;

    /// Check if the page has arrived
    /// \param channel The channel number
    /// \handle The handle of the page returned from pushNextPage()
    /// \return True if the page has arrived, else false
    virtual bool isPageArrived(int channel, const PageHandle& handle) = 0;

    /// Get a page
    /// \param channel The channel number
    /// \handle The handle of the page returned from pushNextPage()
    /// \return A Page object containing the address of the page
    virtual Page getPage(int channel, const PageHandle& handle) = 0;

    /// Mark a page as read, so it can be written to again
    /// \param channel The channel number
    /// \handle The handle of the page returned from pushNextPage()
    virtual void markPageAsRead(int channel, const PageHandle& handle) = 0;

    /// Get the number of pages allocated
    /// \param channel The channel number
    /// \return The number of pages allocated
    virtual int getNumberOfPages(int channel) = 0;

    /// Get the number of DMA channels available on the card
    /// \return The number of DMA channels available on the card
    virtual int getNumberOfChannels() = 0;

    /// Get a pointer to the memory mapped userspace memory
    /// \param channel The channel number
    /// \return A pointer to the memory mapped userspace memory
    virtual volatile void* getMappedMemory(int channel) = 0;

    /// Returns a vector of pointers to the start of each page in userspace
    /// \param channel The channel number
    /// \return Vector of pointers to the start of each page in userspace
    virtual PageVector getMappedPages(int channel) = 0;

    //// Interface channel accessor, for convenient access without having to specify the channel number
    // TODO Finish when CardInterface is more stable? Or remove?
    class ChannelAccessor
    {
      public:
        ChannelAccessor(int channel, CardInterface* cardInterface)
            : channel(channel), interface(cardInterface)
        {
        }

        inline void openChannel(const ChannelParameters& channelParameters)
        {
          interface->openChannel(channel, channelParameters);
        }

        /// Closes a DMA channel
        inline void closeChannel()
        {
          interface->closeChannel(channel);
        }

        inline void startDma()
        {
          interface->startDma(channel);
        }

        inline void stopDma()
        {
          interface->stopDma(channel);
        }

        inline uint32_t readRegister(int index)
        {
          return interface->readRegister(channel, index);
        }

        inline void writeRegister(int index, uint32_t value)
        {
          interface->writeRegister(channel, index, value);
        }

      private:
        const int channel;
        CardInterface* interface;
    };

    inline ChannelAccessor getChannelAccessor(int channel)
    {
      return ChannelAccessor(channel, this);
    }
};

} // namespace Rorc
} // namespace AliceO2
