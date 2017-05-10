/// \file TestSuperpageQueue.cxx
/// \brief Test of the SuperpageQueue class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#define BOOST_TEST_MODULE RORC_TestPageManager
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <array>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <boost/test/unit_test.hpp>
#include <boost/optional.hpp>
#include "SuperpageQueue.h"
#include "RORC/ChannelMasterInterface.h"

using namespace ::AliceO2::Rorc;

namespace {

constexpr size_t MAX_SUPERPAGES = 4;
using Queue = SuperpageQueue<MAX_SUPERPAGES>;
using Id = Queue::Id;
using Entry = Queue::SuperpageQueueEntry;

BOOST_AUTO_TEST_CASE(Capacity)
{
  Queue queue;

  for (size_t i = 0; i < MAX_SUPERPAGES; ++i) {
    queue.addToQueue(Entry());
  }

  BOOST_CHECK_THROW(queue.addToQueue(Entry()), std::exception);
}

BOOST_AUTO_TEST_CASE(Lifecycle)
{
  Queue queue;
  std::array<Id, MAX_SUPERPAGES> ids;

  for (size_t i = 0; i < MAX_SUPERPAGES; ++i) {
    Entry entry;
    entry.busAddress = i;
    entry.maxPages = 1;
    ids[i] = queue.addToQueue(entry);
  }

  for (size_t i = 0; i < MAX_SUPERPAGES; ++i) {
    Id id = i;
    Entry& entry = queue.getPushingFrontEntry();
    BOOST_CHECK(entry.busAddress == i);
    // We 'push' all pages into this superpage
    entry.pushedPages = 1;
    // So we can remove it from the pushing & arrivals queues
    BOOST_CHECK(queue.removeFromPushingQueue() == id);
    // Then we move from arrivals to filled...
    entry.superpage.ready = true;
    BOOST_CHECK(queue.moveFromArrivalsToFilledQueue() == id);
    // Finally, we remove it from the filled queue
    BOOST_CHECK(queue.removeFromFilledQueue().busAddress == i);
  }

  BOOST_CHECK_THROW(queue.removeFromFilledQueue(), std::exception);
}

} // Anonymous namespace
