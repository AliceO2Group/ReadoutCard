/// \file ChannelMasterInterface.h
/// \brief Definition of the ChannelMasterInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <cstdint>
#include <boost/optional.hpp>
#include <boost/exception/all.hpp>
#include "RORC/Parameters.h"
#include "RORC/PageHandle.h"
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

    /// Return the type of the RORC card this ChannelMaster is controlling
    /// \return The card type
    virtual CardType::type getCardType() = 0;

    /// Fills the card's FIFO
    /// Will block if the buffer is full: does not allow you to push new data into a page that was not acknowledged with
    /// acknowledgePage()
    /// \param maxFill Maximum amount of pages to push. If <= 0, will push as many as possible
    /// \return Amount of pages pushed
    virtual int fillFifo(int maxFill = -1) = 0;
    struct Page
    {
        volatile void* const userspace;
        int const index;

        volatile void* getAddress() const
        {
          return userspace;
        }

        volatile uint32_t* getAddressU32() const
        {
          return reinterpret_cast<volatile uint32_t*>(userspace);
        }
    };

    /// Get access to pages in sequential order, one by one
    /// If a page is not available, returns an empty optional.
    /// If a page has not been acked yet, it will be returned again.
    virtual boost::optional<Page> getPage() = 0;

    /// Indicate we're done with the page. This assumes pages are read out by the user in sequential order.
    /// Effectively, this represents an increment of the internal circular buffer's tail.
    /// If acknowledgePage() is not called, getPage() will keep returning the same page
    virtual void freePage(const Page& page) = 0;

    void acknowledgePage(const boost::optional<Page>& page)
    {
      freePage(page.get());
    }
};

} // namespace Rorc
} // namespace AliceO2
