/// \file TestPageManager.cxx
/// \brief Test of the PageManager class
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
#include "PageManager.h"
#include "RORC/ChannelMasterInterface.h"

using namespace ::AliceO2::Rorc;

namespace {

constexpr size_t FIFO_CAPACITY = 128; ///< Firmware FIFO capacity
using Manager = PageManager<FIFO_CAPACITY>;
using Fifo = std::array<bool, FIFO_CAPACITY>;

constexpr int MAGIC_OFFSET = 0xabc0000;

struct Page
{
    void* userspace;
    int index;
};

int handleArrivals(Fifo& fifo, Manager& manager)
{
  auto isArrived = [&](int descriptorIndex) {
    return fifo[descriptorIndex];
  };

  auto resetDescriptor = [&](int descriptorIndex) {
    fifo[descriptorIndex] = false;
  };

  return manager.handleArrivals(isArrived, resetDescriptor);
}

int pushPages(Fifo& fifo, Manager& manager, int amount)
{
  auto push = [&](int bufferIndex, int descriptorIndex) {
  //    auto& pageAddress = getPageAddresses()[bufferIndex];
  //    auto sourceAddress = reinterpret_cast<volatile void*>((descriptorIndex % NUM_OF_FW_BUFFERS) * DMA_PAGE_SIZE);
  //    mFifoUser->setDescriptor(descriptorIndex, DMA_PAGE_SIZE_32, sourceAddress, pageAddress.bus);
  //    bar(Register::DMA_COMMAND) = 0x1; // Is this the right location..? Or should it be in the freeing?
  };

  return manager.pushPages(amount, push);
}

auto getPage(Fifo& fifo, Manager& manager) -> boost::optional<Page>
{
  if (auto page = manager.useArrivedPage()) {
    int bufferIndex = *page;
    return Page{reinterpret_cast<void*>(bufferIndex + MAGIC_OFFSET), bufferIndex};
  }
  return boost::none;
}

void setFifoArrived(Fifo& fifo)
{
  for (bool& b : fifo) {
    b = true;
  }
}

BOOST_AUTO_TEST_CASE(PageManagerCapacityTest)
{
  constexpr size_t BUFFER_NUMBER_OF_PAGES = 511;

  PageManager<FIFO_CAPACITY> manager;
  manager.setAmountOfPages(BUFFER_NUMBER_OF_PAGES);
  Fifo fifo;

  auto checkArrivals = [&](int amount) {
    for (int i = 0; i < amount; ++i) {
      boost::optional<Page> page = getPage(fifo, manager);
      BOOST_REQUIRE(page.is_initialized());
    }
  };

  const size_t pushCycles = BUFFER_NUMBER_OF_PAGES / FIFO_CAPACITY;
  const size_t rest = BUFFER_NUMBER_OF_PAGES % FIFO_CAPACITY;

  // Push most of pages in full FIFO cycles
  for (int i = 0; i < pushCycles; ++i) {
    BOOST_REQUIRE(pushPages(fifo, manager, FIFO_CAPACITY) == FIFO_CAPACITY);
    setFifoArrived(fifo);
    BOOST_REQUIRE(handleArrivals(fifo, manager) == FIFO_CAPACITY);
    checkArrivals(FIFO_CAPACITY);
  }

  // Push rest of pages
  BOOST_REQUIRE(pushPages(fifo, manager, FIFO_CAPACITY) == rest);
  setFifoArrived(fifo);
  BOOST_REQUIRE(handleArrivals(fifo, manager) == rest);
  checkArrivals(rest);
}

BOOST_AUTO_TEST_CASE(PageManagerRandomFreeTest)
{
  constexpr size_t BUFFER_NUMBER_OF_PAGES = 511;

  PageManager<FIFO_CAPACITY> manager;
  manager.setAmountOfPages(BUFFER_NUMBER_OF_PAGES);
  Fifo fifo;

  std::vector<int> pages;

  auto getPageIndexes = [&](int amount) {
    for (int i = 0; i < amount; ++i) {
      boost::optional<Page> page = getPage(fifo, manager);
      BOOST_REQUIRE(page.is_initialized());
      pages.push_back(page->index);
    }
  };

  // Fill up the buffer
  for (int i = 0; i < (BUFFER_NUMBER_OF_PAGES/FIFO_CAPACITY + 1); ++i) {
    pushPages(fifo, manager, FIFO_CAPACITY);
    setFifoArrived(fifo);
    handleArrivals(fifo, manager);
  }
  getPageIndexes(BUFFER_NUMBER_OF_PAGES);

  // Free some pages randomly
  constexpr size_t FREE_AMOUNT = 100;
  {
    std::default_random_engine generator;
    for (int i = 0; i < FREE_AMOUNT; ++i) {
      std::uniform_int_distribution<size_t> distribution(0, pages.size() - 1);
      size_t index = distribution(generator);
      manager.freePage(pages[index]);
      pages.erase(pages.begin() + index);
    }
  }

  // Try to reuse freed pages
  pushPages(fifo, manager, FREE_AMOUNT);
  setFifoArrived(fifo);
  handleArrivals(fifo, manager);
  getPageIndexes(FREE_AMOUNT);

  // Should be full
  BOOST_REQUIRE(pushPages(fifo, manager, 1) == 0);
}

BOOST_AUTO_TEST_CASE(PageManagerEmptyTest)
{
  constexpr size_t BUFFER_NUMBER_OF_PAGES = 511;

  PageManager<FIFO_CAPACITY> manager;
  manager.setAmountOfPages(BUFFER_NUMBER_OF_PAGES);
  Fifo fifo;

  BOOST_CHECK_THROW(manager.freePage(0), Exception);
  BOOST_CHECK_THROW(manager.freePage(3459873), Exception);
  BOOST_REQUIRE(not manager.useArrivedPage().is_initialized());
  BOOST_CHECK_NO_THROW(manager.handleArrivals(
      [](auto){return true;},
      [](auto){}));
  BOOST_CHECK_NO_THROW(manager.pushPages(12345,
      [](auto, auto){}));
}

} // Anonymous namespace
