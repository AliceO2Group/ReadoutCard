/// \file PageManager.h
/// \brief Definition of the PageManager class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_RORC_SRC_PAGEMANAGER_H_
#define ALICEO2_RORC_SRC_PAGEMANAGER_H_

#include <iostream>
#include <queue>
#include <unordered_map>
#include <boost/circular_buffer.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/optional.hpp>
#include "RORC/Exception.h"

namespace AliceO2 {
namespace Rorc {

template<size_t FIRMWARE_QUEUE_CAPACITY>
class PageManager
{
  public:
    struct Page
    {
        int descriptorIndex; ///< Index for DMA FIFO descriptor table
        int bufferIndex; ///< Index for DMA buffer
    };

    enum class PageStatus
    {
      FREE,    ///< Page is free and may be used to push into
      PUSHING, ///< Page is being pushed into
      ARRIVED, ///< Page has been fully pushed
      IN_USE   ///< Page is in use by the client
    };

    void setAmountOfPages(size_t amount)
    {
      mMaxPages = amount;

      mQueueFree = Queue(typename Queue::container_type(amount));
      mQueueArrived = Queue(typename Queue::container_type(amount));

//      std::cout << "SETTING AMOUNT OF PAGES: " << amount << '\n';
      for (int i = 0; i < amount; ++i) {
        mQueueFree.push(Page{-1, i});
      }

      checkInvariant();
    }

    /// Check for arrived pages and free up FIFO slots that are no longer needed
    /// \tparam IsArrived Function to check if the page with the given descriptor index has been completely pushed
    /// \tparam ResetDescriptor Function reset the descriptor with the given index
    /// \return Amount of arrived pages
    template<class IsArrived, class ResetDescriptor>
    int handleArrivals(IsArrived isArrived, ResetDescriptor resetDescriptor)
    {
      checkInvariant();

      int arrived = 0;
      int queueSize = mQueuePushing.size();
      for (int i = 0; i < queueSize; ++i) {
        auto page = mQueuePushing.front();
        if (isArrived(page.descriptorIndex)) {
          resetDescriptor(page.descriptorIndex);
//          std::cout << "ARRIVED   bi:" << page.bufferIndex << " di:"<< page.descriptorIndex << '\n';
          arrived++;
          mQueuePushing.pop();
          mQueueArrived.push(page);
        } else {
          break;
        }
      }

      return arrived;
    }

    /// Push pages
    /// \tparam Push Function to push a single page with the given buffer index and descriptor index.
    /// \param pushLimit Limit on the amount of pages to push. If <= 0, will push as many pages as fit in the queue.
    /// \return The amount of pages that were pushed
    template<class Push>
    int pushPages(size_t pushLimit, Push push)
    {
      checkInvariant();

      size_t freeDescriptors = FIRMWARE_QUEUE_CAPACITY - mQueuePushing.size();
      size_t freePages = mQueueFree.size();
      size_t possibleToPush = std::min(freeDescriptors, freePages);
      int pushCount = (pushLimit <= 0) ? possibleToPush : std::min(pushLimit, possibleToPush);

      for (int i = 0; i < pushCount; ++i) {
        auto page = mQueueFree.front();
        mQueueFree.pop();

        page.descriptorIndex = getFifoHeadIndex(); // Assign new descriptor to page
        advanceFifoHead();

//        std::cout << "PUSH      bi:" << page.bufferIndex << " di:"<< page.descriptorIndex << '\n';
        push(page.bufferIndex, page.descriptorIndex);
        mQueuePushing.push(page);
      }
      return pushCount;
    }

    /// Gets a page with ARRIVED status and puts it in NONFREE status
    boost::optional<int> useArrivedPage()
    {
      checkInvariant();

      if (!mQueueArrived.empty()) {
        auto page = mQueueArrived.front();
        mQueueArrived.pop();
//        std::cout << "USING     bi:" << page.bufferIndex << '\n';

        auto iterator = mMapInUse.find(page.bufferIndex);
        if (iterator != mMapInUse.end()) {
          BOOST_THROW_EXCEPTION(Exception()
              << errinfo_rorc_error_message("Cannot free page: was already in use")
              << errinfo_rorc_fifo_index(page.descriptorIndex)
              << errinfo_rorc_page_index(page.bufferIndex));
        }

        mMapInUse[page.bufferIndex] = page;
        return page.bufferIndex;
      }
      return boost::none;
    }

    void freePage(int bufferIndex)
    {
      checkInvariant();

//      std::cout << "FREE      bi:" << bufferIndex << '\n';

      auto iterator = mMapInUse.find(bufferIndex);
      if (iterator == mMapInUse.end()) {
        BOOST_THROW_EXCEPTION(Exception()
            << errinfo_rorc_error_message("Cannot free page: was not in use")
            << errinfo_rorc_page_index(bufferIndex));
      }
      auto page = iterator->second;

      assert(bufferIndex == page.bufferIndex);

      mMapInUse.erase(bufferIndex);
      mQueueFree.push(page);
    }

  private:

    void checkInvariant()
    {
#ifndef NDEBUG
      auto free = mQueueFree.size();
      auto pushing = mQueuePushing.size();
      auto arrived = mQueueArrived.size();
      auto use = mMapInUse.size();
      auto total = free + pushing + arrived + use;

      auto print = [&]{
        std::cout
          << "\nFree     " << free
          << "\nPushing  " << pushing
          << "\nArrived  " << arrived
          << "\nInUse    " << use
          << "\nTotal    " << total
          << "\nMax      " << mMaxPages
          << '\n';
      };

      if (total != mMaxPages) {
        BOOST_THROW_EXCEPTION(std::runtime_error("invariant violated"));
      }

      if (pushing > FIRMWARE_QUEUE_CAPACITY) {
        BOOST_THROW_EXCEPTION(std::runtime_error("invariant violated"));
      }
#endif
    }

    int getFifoHeadIndex() const
    {
      return mFifoHead;
    }

    void advanceFifoHead()
    {
      mFifoHead = (mFifoHead + 1) % FIRMWARE_QUEUE_CAPACITY;
    }

    /// Underlying buffer for the queues
    using QueueBackend = boost::circular_buffer<Page>;

    /// Queue for readout page handles
    using Queue = std::queue<typename QueueBackend::value_type, QueueBackend>;

    /// Queue for pages in the firmware FIFO
    Queue mQueuePushing = Queue(typename Queue::container_type(FIRMWARE_QUEUE_CAPACITY));

    /// Pages that have arrived
    Queue mQueueArrived;

    /// Free pages in the buffer
    Queue mQueueFree;

    /// Pages that are in use
    std::unordered_map<int, Page> mMapInUse;

    int mFifoHead = 0;

    /// When the queue size is above this threshold, we do not fill the queue
    //int mFillThreshold = QUEUE_CAPACITY;

    size_t mMaxPages = 0;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_RORC_SRC_PAGEMANAGER_H_
