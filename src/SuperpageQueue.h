/// \file SuperpageQueue.h
/// \brief Definition of the SuperpageQueue class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_RORC_SRC_SUPERPAGEQUEUE_H_
#define ALICEO2_RORC_SRC_SUPERPAGEQUEUE_H_

#include <iostream>
#include <unordered_map>
#include <boost/circular_buffer.hpp>
#include "RORC/Superpage.h"
#include "ExceptionInternal.h"

namespace AliceO2 {
namespace Rorc {

/// Queue to handle superpages
/// We keep this header-only to make it inlineable, since these are all very short and simple functions.
template <size_t MAX_SUPERPAGES>
class SuperpageQueue {
  public:
    using Id = uint8_t;
    using Queue = boost::circular_buffer<Id>;

    /// This struct wraps the SuperpageStatus and adds some internally used variables
    struct SuperpageQueueEntry
    {
        bool isPushed() const
        {
          return pushedPages == maxPages;
        }

        int getUnpushedPages() const
        {
          return maxPages - pushedPages;
        }

        Superpage superpage;
        uintptr_t busAddress;
        int pushedPages; ///< Amount of pages that have been pushed (not necessarily arrived)
        int maxPages; ///< Amount of pages that can be pushed
    };

    /// Gets ID of youngest superpage
    Id getBackSuperpageId()
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

    /// Gets ID of oldest superpage
    Id getFrontSuperpageId()
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
    Superpage getFrontSuperpage()
    {
      if (getQueueCount() == 0) {
      //if (mRegistry.empty()) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not get superpage status, queue was empty"));
      }

      return getEntry(getFrontSuperpageId()).superpage;
    }

    /// Add a superpage to the queue
    /// When a superpage is initially added, it is put into the internal pushing and arrivals queues
    /// \return ID of the added superpage
    Id addToQueue(const SuperpageQueueEntry& entry)
    {
      if (isFull()) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, queue full"));
      }

      auto id = mNextId;

#ifndef NDEBUG
      if (isValidEntry(mRegistry.at(id))) {
        BOOST_THROW_EXCEPTION(
            Exception() << ErrorInfo::Message("Could not enqueue superpage, would overwrite index ID already in use")
                << ErrorInfo::Index(id) << ErrorInfo::FifoSize(getQueueCount()));
      }
#endif

//      printf("Enqueued superpage\n  o=%lu pqs=%lu aqs=%lu fqs=%lu\n", entry.status.getOffset(), mPushing.size(), mArrivals.size(),
//                mFilled.size());

      mRegistry[id] = entry; // We don't use getEntry() because it checks for entry validity
      mNextId = (mNextId + 1) % MAX_SUPERPAGES;
      mNumberOfEntries++;

      mPushing.push_back(id);
      mArrivals.push_back(id);
      return id;
    }

    /// Removes a superpage that has been pushed completely from the pushing queue
    /// \return ID of the removed superpage
    Id removeFromPushingQueue()
    {
      if (mPushing.empty()) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Could not remove from pushing queue, pushing queue was empty"));
      }

      auto id = mPushing.front();
//      printf("Removing from pushing queue\n  o=%lu\n", id);

      const auto& entry = getEntry(id);

      if (!entry.isPushed()) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Could not remove from pushing queue, entry was not completely pushed"));
      }

      mPushing.pop_front();
      return id;
    }

    /// Moves a superpage that has had all pushed pages completely arrived from the internal arrivals queue to filled
    /// queue
    Id moveFromArrivalsToFilledQueue()
    {
      if (mArrivals.empty()) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Could not move from arrivals to filled, arrivals was empty"));
      }

      auto id = mArrivals.front();
//      printf("Moving from arrivals to filled\n  o=%d\n", int(id));

      const auto& entry = getEntry(id);
      if (!entry.superpage.isFilled()) {
        BOOST_THROW_EXCEPTION(Exception()
            << ErrorInfo::Message("Could not move arrivals to filled, superpage was not filled"));
      }

      mFilled.push_back(id);
      mArrivals.pop_front();
      return id;
    }


    /// Removes a superpage that's completely filled from the filled queue, ending the 'lifecycle' of the superpage
    SuperpageQueueEntry removeFromFilledQueue()
    {
      if (mFilled.empty()) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not pop superpage, filled queue was empty"));
      }

      auto id = mFilled.front();
      SuperpageQueueEntry entry = getEntry(id);
      resetEntry(getEntry(id));
      mNumberOfEntries--;
      mFilled.pop_front();

//      printf("Popped superpage\n  o=%lu pqs=%lu aqs=%lu fqs=%lu\n", id, mPushing.size(), mArrivals.size(),
//          mFilled.size());

      return entry;
    }


    int getQueueCount() const
    {
      return mNumberOfEntries;
    }

    int getQueueAvailable() const
    {
      return MAX_SUPERPAGES - getQueueCount();
    }

    int getQueueCapacity() const
    {
      return MAX_SUPERPAGES;
    }

    int isEmpty() const
    {
      return getQueueCount() == 0;
    }

    int isFull() const
    {
      return getQueueCount() == MAX_SUPERPAGES;
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

    SuperpageQueueEntry& getPushingFrontEntry()
    {
      return getEntry(getPushing().front());
    }

    SuperpageQueueEntry& getArrivalsFrontEntry()
    {
      return getEntry(getArrivals().front());
    }

    SuperpageQueueEntry& getEntry(Id id)
    {
#ifndef NDEBUG
      if (!isValidEntry(mRegistry.at(id))) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Invalid entry") << ErrorInfo::Index(id));
      }
#endif
      return mRegistry[id];
    }

    void resetEntry(SuperpageQueueEntry& entry)
    {
      entry.pushedPages = PUSHED_PAGES_INVALID;
    }

    bool isValidEntry(const SuperpageQueueEntry& entry) const
    {
      return entry.pushedPages != PUSHED_PAGES_INVALID;
    }

    void clear()
    {
      for (auto& e : mRegistry) {
        resetEntry(e);
      }
      mPushing.clear();
      mArrivals.clear();
      mFilled.clear();
      mNumberOfEntries = 0;
      mNextId = 0;
    };

  private:

    static constexpr int PUSHED_PAGES_INVALID = -1;
    int mNumberOfEntries = 0;
    Id mNextId = 0;

    /// Registry for superpages
    /// The queues contain an ID that's used as a key for this registry.
    std::array<SuperpageQueueEntry, MAX_SUPERPAGES> mRegistry;

    /// Queue for superpages that can be pushed into
    Queue mPushing { MAX_SUPERPAGES };

    /// Queue for superpages that must be checked for arrivals
    Queue mArrivals { MAX_SUPERPAGES };

    /// Queue for superpages that are filled
    Queue mFilled { MAX_SUPERPAGES };

    static_assert(MAX_SUPERPAGES <= (size_t(std::numeric_limits<Id>::max()) + 1),
        "Id type can't handle amount of entries");
};


} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_RORC_SRC_SUPERPAGEQUEUE_H_
