// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

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
#include "ReadoutCard/DmaChannelInterface.h"

using namespace ::o2::roc;

namespace
{

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
    entry.superpage.setReady(true);
    BOOST_CHECK(queue.moveFromArrivalsToFilledQueue() == id);
    // Finally, we remove it from the filled queue
    BOOST_CHECK(queue.removeFromFilledQueue().busAddress == i);
  }

  BOOST_CHECK_THROW(queue.removeFromFilledQueue(), std::exception);
}

} // Anonymous namespace
