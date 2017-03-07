/// \file SuperpageQueue.h
/// \brief Definition of the SuperpageQueue class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_RORC_SRC_SUPERPAGEQUEUE_H_
#define ALICEO2_RORC_SRC_SUPERPAGEQUEUE_H_

#include <iostream>
#include <unordered_map>
#include <boost/circular_buffer.hpp>
#include "ExceptionInternal.h"

namespace AliceO2 {
namespace Rorc {

/// Queue to handle superpages
template <size_t MAX_SUPERPAGES>
class SuperpageQueue {
  public:
    using Offset = size_t;

    /// This struct wraps the SuperpageStatus and adds some internally used variables
    struct SuperpageQueueEntry
    {
        SuperpageStatus status;
        uintptr_t busAddress;
        int pushedPages; ///< Amount of pages that have been pushed (not necessarily arrived)
    };

    using Queue = boost::circular_buffer<Offset>;
    using Registry = std::unordered_map<Offset, SuperpageQueueEntry>;

    /// Gets offset of youngest superpage
    Offset getBackSuperpage()
    {
      if (!mPushing.empty()) {
        return mPushing.back();
      }

      if (!mArrivals.empty()) {
        return mArrivals.back();
      }

      if (!mFilled.empty()) {
        return mFilled.back();
      }

      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not get back superpage, queues were empty"));
    }

    /// Gets offset of oldest superpage
    Offset getFrontSuperpage()
    {
      if (!mFilled.empty()) {
        return mFilled.front();
      }

      if (!mArrivals.empty()) {
        return mArrivals.front();
      }

      if (!mPushing.empty()) {
        return mPushing.front();
      }

      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not get front superpage, queues were empty"));
    }

    /// Gets status of oldest superpage
    SuperpageStatus getFrontSuperpageStatus()
    {
      if (mRegistry.empty()) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not get superpage status, queue was empty"));
      }

      return mRegistry.at(getFrontSuperpage()).status;
    }

    /// Add a superpage to the queue
    /// When a superpage is initially added, it is put into the internal pushing and arrivals queues
    void addToQueue(const SuperpageQueueEntry& entry)
    {
      if (isFull()) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, queue full"));
      }

      if (!mRegistry.insert(std::make_pair(entry.status.offset, entry)).second) {
        // Key was already present
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Could not enqueue superpage, offset already in use")
            << ErrorInfo::Offset(entry.status.offset));
      }

      mPushing.push_back(entry.status.offset);
      mArrivals.push_back(entry.status.offset);

    //  printf("Enqueued superpage\n  o=%lu pqs=%lu aqs=%lu fqs=%lu\n", entry.status.offset, mPushing.size(), mArrivals.size(),
    //      mFilled.size());
    }

    /// Removes a superpage that has been pushed completely from the pushing queue
    void removeFromPushingQueue()
    {
      if (mPushing.empty()) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Could not remove from pushing queue, pushing queue was empty"));
      }

      auto offset = mPushing.front();
    //  printf("Removing from pushing queue\n  o=%lu\n", offset);

      const auto& entry = mRegistry.at(offset);

      if (entry.pushedPages != entry.status.maxPages) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not remove from pushing queue"));
      }

      mPushing.pop_front();
    }

    /// Moves a superpage that has had all pushed pages completely arrived from the internal arrivals queue to filled
    /// queue
    void moveFromArrivalsToFilledQueue()
    {
      if (mArrivals.empty()) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Could not move from arrivals to filled, arrivals was empty"));
      }

      auto offset = mArrivals.front();
    //  printf("Moving from arrivals to filled\n  o=%lu\n", offset);

      const auto& entry = mRegistry.at(offset);
      if (entry.status.confirmedPages != entry.status.maxPages) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Could not move arrivals to filled, superpage was not filled"));
      }

      mFilled.push_back(offset);
      mArrivals.pop_front();
    }


    /// Removes a superpage that's completely filled from the filled queue, ending the 'lifecycle' of the superpage
    auto removeFromFilledQueue() -> SuperpageQueueEntry
    {
      if (mFilled.empty()) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not pop superpage, filled queue was empty"));
      }

      Offset offset = mFilled.front();
      SuperpageQueueEntry entry = mRegistry.at(offset);
      if (mRegistry.erase(offset) == 0) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not pop superpage, did not find element"));
      }

      mFilled.pop_front();

    //  printf("Popped superpage\n  o=%lu pqs=%lu aqs=%lu fqs=%lu\n", offset, mPushing.size(), mArrivals.size(),
    //      mFilled.size());

      return entry;
    }


    int getQueueCount() const
    {
      return mRegistry.size();
    }

    int getQueueAvailable() const
    {
      return MAX_SUPERPAGES - mRegistry.size();
    }

    int getQueueCapacity() const
    {
      return MAX_SUPERPAGES;
    }

    int isEmpty() const
    {
      return mRegistry.empty();
    }

    int isFull() const
    {
      return mRegistry.size() == MAX_SUPERPAGES;
    }

    const Registry& getRegistry() const
    {
      return mRegistry;
    }

    const Queue& getPushing() const
    {
      return mPushing;
    }
    const Queue& getArrivals() const
    {
      return mArrivals;
    }

    const Queue& getFilled() const
    {
      return mFilled;
    }

    SuperpageQueueEntry& getEntry(Offset offset)
    {
      return mRegistry.at(offset);
    }

    void clear()
    {
      mRegistry.clear();
      mPushing.clear();
      mArrivals.clear();
      mFilled.clear();
    };

  private:

    /// Registry for superpages
    /// The queues contain an offset that's used as a key for this registry.
    Registry mRegistry;

    /// Queue for superpages that can be pushed into
    Queue mPushing { MAX_SUPERPAGES };

    /// Queue for superpages that must be checked for arrivals
    Queue mArrivals { MAX_SUPERPAGES };

    /// Queue for superpages that are filled
    Queue mFilled { MAX_SUPERPAGES };
};


} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_RORC_SRC_SUPERPAGEQUEUE_H_
