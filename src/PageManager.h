/// \file PageManager.h
/// \brief Definition of the PageManager class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_RORC_SRC_PAGEMANAGER_H_
#define ALICEO2_RORC_SRC_PAGEMANAGER_H_

#include <iostream>
#include <boost/circular_buffer.hpp>
#include <boost/dynamic_bitset.hpp>
#include <queue>
#include "RORC/Exception.h"

namespace AliceO2 {
namespace Rorc {

template<size_t QUEUE_CAPACITY>
class PageManager
{
  public:
    struct FifoPage
    {
        int descriptorIndex; ///< Index for CRU DMA descriptor table
        int bufferIndex; ///< Index for mPageAddresses
    };

    enum class PageStatus
    {
      FREE,    ///< Page is free and may be used to push into
      PUSHING, ///< Page is being pushed into
      ARRIVED, ///< Page has been fully pushed
      IN_USE  ///< Page is in use by the client
    };

    /// Underlying buffer for the ReadoutQueue
    using QueueBackend = boost::circular_buffer<FifoPage>;

    /// Queue for readout page handles
    using ReadoutQueue = std::queue<typename QueueBackend::value_type, QueueBackend>;

    void setAmountOfPages(size_t amount)
    {
      mFifoPageStatus.resize(amount, PageStatus::FREE);
    }

    /// Free up FIFO slots that are no longer needed
    /// \tparam IsArrived Function to check if the page with the given descriptor index has been completely pushed
    /// \tparam ResetDescriptor Function reset the descriptor with the given index
    template<class IsArrived, class ResetDescriptor>
    void freeQueueSlots(IsArrived isArrived, ResetDescriptor resetDescriptor)
    {
      int queueSize = mQueue.size();
      for (int i = 0; i < queueSize; ++i) {
        auto page = mQueue.front();
        if (isArrived(page.descriptorIndex)) {
          resetDescriptor(page.descriptorIndex);
//          std::cout << "ARRIVED   bi:" << page.bufferIndex << " di:"<< page.descriptorIndex << '\n';
          setFifoPageStatus(page, PageStatus::ARRIVED);
          mQueue.pop();
        } else {
          break;
        }
      }
    }

    /// Push pages
    /// \tparam Push Function to push a single page with the given buffer index and descriptor index.
    /// \param pushLimit Limit on the amount of pages to push. If <= 0, will push as many pages as fit in the queue.
    /// \return The amount of pages that were pushed
    template<class Push>
    int pushPages(size_t pushLimit, Push push)
    {
      size_t free = QUEUE_CAPACITY - mQueue.size();
      int toPush = (pushLimit <= 0) ? free : std::min(pushLimit, free);
      int pushCount = 0;

      for (int i = 0; i < toPush; ++i) {
        if (auto freeBufferIndex = findBufferIndexWithStatus(PageStatus::FREE, mFreePageHint)) {
          pushCount++;
          mFreePageHint++;
          FifoPage page;
          page.bufferIndex = *freeBufferIndex;
          page.descriptorIndex = getFifoHeadIndex();
//          std::cout << "PUSH      bi:" << page.bufferIndex << " di:"<< page.descriptorIndex << '\n';
          push(page.bufferIndex, page.descriptorIndex);
          setFifoPageStatus(page, PageStatus::PUSHING);
          mQueue.push(page);
          advanceFifoHead();
        } else {
          // No free pages...
          break;
        }
      }
      return pushCount;
    }

    boost::optional<int> findBufferIndexWithStatus(PageStatus status) const
    {
      int size = mFifoPageStatus.size();
      for (int i = 0; i < size; ++i) {
        if (getFifoPageStatus(i) == status) {
          return i;
        }
      }
      return boost::none;
    }

    boost::optional<int> findBufferIndexWithStatus(PageStatus status, int hintIndex) const
    {
      // Check if hinted index has the right status
      if (getFifoPageStatus(hintIndex) == status) {
        return hintIndex;
      }

      // Search starting from the index
      int size = mFifoPageStatus.size();
      for (int i = hintIndex + 1; i < size; ++i) {
        if (getFifoPageStatus(i) == status) {
          return i;
        }
      }

      // Wrap around if needed
      for (int i = 0; i < hintIndex; ++i) {
        if (getFifoPageStatus(i) == status) {
          return i;
        }
      }
      return boost::none;
    }

    /// Gets a page with ARRIVED status and puts it in NONFREE status
    boost::optional<int> useArrivedPage()
    {
      if (auto bufferIndex = findBufferIndexWithStatus(PageStatus::ARRIVED)) {
//        std::cout << "USING     bi:" << *bufferIndex << '\n';
        setFifoPageStatus(*bufferIndex, PageStatus::IN_USE);
        return *bufferIndex;
      }
      return boost::none;
    }

    void freePage(int bufferIndex)
    {
      if (getFifoPageStatus(bufferIndex) != PageStatus::IN_USE) {
        BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message("Cannot free page: Invalid page status"));
      }
//      std::cout << "FREE      bi:" << bufferIndex << '\n';
      setFifoPageStatus(bufferIndex, PageStatus::FREE);
      mFreePageHint = bufferIndex;
    }

    PageStatus getFifoPageStatus(const FifoPage& page) const
    {
      return getFifoPageStatus(page.bufferIndex);
    }

    PageStatus getFifoPageStatus(int bufferIndex) const
    {
      if (bufferIndex >= mFifoPageStatus.size()) {
        BOOST_THROW_EXCEPTION(Exception() << errinfo_rorc_error_message("Invalid page index"));
      }
      return mFifoPageStatus[bufferIndex];
    }

    void setFifoPageStatus(const FifoPage& page, PageStatus status)
    {
      setFifoPageStatus(page.bufferIndex, status);
    }

    void setFifoPageStatus(int bufferIndex, PageStatus status)
    {
      mFifoPageStatus[bufferIndex] = status;
    }

  private:

    int getFifoHeadIndex() const
    {
      return mFifoHead;
    }

    void advanceFifoHead()
    {
      mFifoHead = (mFifoHead + 1) % QUEUE_CAPACITY;
    }

    /// Queue for pages in the firmware FIFO
    ReadoutQueue mQueue = ReadoutQueue(typename ReadoutQueue::container_type(QUEUE_CAPACITY));

    /// A map to keep track of which pages are arrived, free (can push into) and not-free (in use by client)
    std::vector<PageStatus> mFifoPageStatus;

    int mFifoHead = 0;

    /// When the queue size is above this threshold, we do not fill the queue
    //int mFillThreshold = QUEUE_CAPACITY;

    /// Hint where to look for a free page
    int mFreePageHint = 0;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_RORC_SRC_PAGEMANAGER_H_
