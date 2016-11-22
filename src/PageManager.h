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
#include "ExceptionInternal.h"

namespace AliceO2 {
namespace Rorc {

/// CRTP base class for PageManager implementations
template <class Derived>
class PageManagerBase
{
  public:
    /// Set the amount of pages that fit in the buffer
    /// This method effectively resets the manager
    void setAmountOfPages(size_t amount)
    {
      _this()._setAmountOfPages(amount);
    }

    /// Check for arrived pages and free up FIFO slots that are no longer needed
    /// \param isArrived Function to check if the page with the given descriptor index has been completely pushed
    /// \param resetDescriptor Function reset the descriptor with the given index
    /// \return Amount of arrived pages
    template<class IsArrived, class ResetDescriptor>
    int handleArrivals(IsArrived isArrived, ResetDescriptor resetDescriptor)
    {
      return _this()._handleArrivals(isArrived, resetDescriptor);
    }

    /// Push pages
    /// \param pushLimit Limit on the amount of pages to push. If <= 0, will push as many pages as fit in the queue.
    /// \param push Function to push a single page with the given buffer index and descriptor index.
    /// \return The amount of pages that were pushed
    template<class Push>
    int pushPages(size_t pushLimit, Push push)
    {
      return _this()._pushPages(pushLimit, push);
    }

    /// Gets a page with ARRIVED status and puts it in NONFREE status
    /// \return The page ID
    boost::optional<int> useArrivedPage()
    {
      return _this()._useArrivedPage();
    }

    void freePage(int bufferIndex)
    {
      _this()._freePage(bufferIndex);
    }

    int getArrivedCount()
    {
      return _this()._getArrivedCount();
    }

  private:
    Derived& _this()
    {
      return *static_cast<Derived*>(this);
    }
};

/// Standard implementation of PageManagerBase
template<size_t FIRMWARE_QUEUE_CAPACITY>
class PageManager : public PageManagerBase<PageManager<FIRMWARE_QUEUE_CAPACITY>>
{
  public:
    struct Page
    {
        int descriptorIndex; ///< Index for DMA FIFO descriptor table
        int bufferIndex; ///< Index for DMA buffer
    };

    void _setAmountOfPages(size_t amount)
    {
      mMaxPages = amount;
      mFifoHead = 0;
      mQueuePushing = Queue(typename Queue::container_type(FIRMWARE_QUEUE_CAPACITY));
      mQueueFree = Queue(typename Queue::container_type(amount));
      mQueueArrived = Queue(typename Queue::container_type(amount));

//      std::cout << "SETTING AMOUNT OF PAGES: " << amount << '\n';
      for (int i = 0; i < amount; ++i) {
        mQueueFree.push(Page{-1, i});
      }

      checkInvariant();
    }

    template<class IsArrived, class ResetDescriptor>
    int _handleArrivals(IsArrived isArrived, ResetDescriptor resetDescriptor)
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

    template<class Push>
    int _pushPages(size_t pushLimit, Push push)
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
    /// \return The page ID
    boost::optional<int> _useArrivedPage()
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

    void _freePage(int bufferIndex)
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

    int _getArrivedCount()
    {
      return mQueueArrived.size();
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
        print();
        BOOST_THROW_EXCEPTION(std::runtime_error("invariant violated"));
      }

      if (pushing > FIRMWARE_QUEUE_CAPACITY) {
        print();
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
    Queue mQueuePushing;

    /// Pages that have arrived
    Queue mQueueArrived;

    /// Free pages in the buffer
    Queue mQueueFree;

    /// Pages that are in use
    std::unordered_map<int, Page> mMapInUse;

    /// Current head of the firmware FIFO
    int mFifoHead = 0;

    size_t mMaxPages = 0;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_RORC_SRC_PAGEMANAGER_H_
