/// \file ChannelMasterInterface.h
/// \brief Definition of the ChannelMasterInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstdint>
#include <memory>
#include <boost/optional.hpp>
#include <boost/exception/all.hpp>
#include "RORC/Parameters.h"
#include "RORC/Page.h"
#include "RORC/CardType.h"
#include "RORC/ResetLevel.h"
#include "RORC/RegisterReadWriteInterface.h"

namespace AliceO2 {
namespace Rorc {

/// Pure abstract interface for objects that obtain a master lock on a channel and provides an interface to control
/// and use that channel.
class ChannelMasterInterface: public virtual RegisterReadWriteInterface
{
  public:
    using MasterSharedPtr = std::shared_ptr<ChannelMasterInterface>;

    /// A class that represents a page
    class Page
    {
      public:
        Page(volatile void* address, int size, int id, const MasterSharedPtr& channel)
            : mAddress(address), mSize(size), mId(id), mChannel(channel)
        {
        }

        volatile void* getAddress() const
        {
          return mAddress;
        }

        volatile uint32_t* getAddressU32() const
        {
          return reinterpret_cast<volatile uint32_t*>(getAddress());
        }

        /// Get the size of the page
        int getSize() const
        {
          return mSize;
        }

        /// Get the page's ID. Used internally.
        int getId() const
        {
          return mId;
        }

        ~Page()
        {
          mChannel->freePage(*this);
        }

      private:
        volatile void* mAddress; ///< Userspace address of the page's memory
        int mSize; ///< Size of the page
        int mId; ///< ID used for internal bookkeeping
        MasterSharedPtr mChannel;
    };

    using PageSharedPtr = std::shared_ptr<Page>;

    virtual ~ChannelMasterInterface()
    {
    }

    /// Starts DMA for the given channel
    /// This must be called before pushing pages
    virtual void startDma() = 0;

    /// Stops DMA for the given channel
    virtual void stopDma() = 0;

    /// Resets the channel
    /// \param resetLevel The depth of the reset
    virtual void resetCard(ResetLevel::type resetLevel) = 0;

    /// Returns the type of the RORC card this ChannelMaster is controlling
    /// \return The card type
    virtual CardType::type getCardType() = 0;

    /// Fills the card's FIFO
    /// Will block if the buffer is full: does not allow you to push new data into a page that was not acknowledged with
    /// acknowledgePage()
    /// \param maxFill Maximum amount of pages to push. If <= 0, will push as many as possible
    /// \return Amount of pages pushed
    virtual int fillFifo(int maxFill = -1) = 0;

    /// Returns the amount of pages that are currently available (may be popped) in the DMA buffer
    virtual int getAvailableCount() = 0;

    /// Pops a page from the channel's DMA buffer and returns it wrapped in a shared_ptr
    /// Note that the Page object keeps a reference to the channel, so it can free itself automatically on destruction
    static PageSharedPtr popPage(const MasterSharedPtr& channel)
    {
      return channel->popPageInternal(channel);
    }

    /// Indicates the client is no longer using the page. Called by the Page class's destructor.
    virtual void freePage(const Page& page) = 0;

  private:
    /// Pops and gets a page from the DMA buffer.
    /// If a page is not available, returns an empty optional.
    virtual PageSharedPtr popPageInternal(const MasterSharedPtr& channel) = 0;
};

} // namespace Rorc
} // namespace AliceO2
