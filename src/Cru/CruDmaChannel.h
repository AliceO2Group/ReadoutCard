
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
/// \file CruDmaChannel.h
/// \brief Definition of the CruDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_CRUDMACHANNEL_H_
#define O2_READOUTCARD_CRU_CRUDMACHANNEL_H_

#include "DmaChannelPdaBase.h"
#include <memory>
#include <deque>
//#define BOOST_CB_ENABLE_DEBUG 1
#include <boost/circular_buffer.hpp>
#include "Cru/CruBar.h"
#include "Cru/FirmwareFeatures.h"
#include "ReadoutCard/Parameters.h"
#include "folly/ProducerConsumerQueue.h"

namespace o2
{
namespace roc
{

/// Extends DmaChannel object, and provides device-specific functionality
class CruDmaChannel final : public DmaChannelPdaBase
{
 public:
  CruDmaChannel(const Parameters& parameters);
  virtual ~CruDmaChannel() override;

  virtual CardType::type getCardType() override;

  virtual bool pushSuperpage(Superpage) override;

  virtual int getTransferQueueAvailable() override;
  virtual int getReadyQueueSize() override;

  virtual Superpage getSuperpage() override;
  virtual Superpage popSuperpage() override;
  virtual void fillSuperpages() override;
  virtual bool isTransferQueueEmpty() override;
  virtual bool isReadyQueueFull() override;
  virtual int32_t getDroppedPackets() override;
  virtual bool areSuperpageFifosHealthy() override;

  virtual bool injectError() override;
  virtual boost::optional<int32_t> getSerial() override;
  virtual boost::optional<float> getTemperature() override;
  virtual boost::optional<std::string> getFirmwareInfo() override;
  virtual boost::optional<std::string> getCardId() override;
  AllowedChannels allowedChannels();

  virtual int32_t getCounterFirstOrbit() override;

 protected:
  virtual void deviceStartDma() override;
  virtual void deviceStopDma() override;
  virtual void deviceResetChannel(ResetLevel::type resetLevel) override;

 private:
  /// Max amount of superpages per link.
  /// This may not exceed the limit determined by the firmware capabilities.
  size_t mLinkQueueCapacity;

  /// Max amount of superpages in the ready queue.
  /// This is an arbitrary size, can easily be increased if more headroom is needed.
  size_t mReadyQueueCapacity;

  /// Queue for one link
  using SuperpageQueue = folly::ProducerConsumerQueue<Superpage>;

  /// Index into mLinks
  using LinkIndex = uint32_t;

  /// ID for a link
  using LinkId = uint32_t;

  /// Struct for keeping track of one link's counter and superpages
  struct Link {
    /// The link's FEE ID
    LinkId id = 0;

    /// The amount of superpages received from this link
    uint32_t superpageCounter = 0;

    /// The superpage queue
    std::shared_ptr<SuperpageQueue> queue;
  };

  void resetCru();
  void setBufferReady();
  void setBufferNonReady();

  auto getBar()
  {
    return cruBar.get();
  }

  auto getBar2()
  {
    return cruBar2.get();
  }

  /// Gets index of next link to push
  LinkIndex getNextLinkIndex();

  /// Push a superpage to a link
  void pushSuperpageToLink(Link& link, const Superpage& superpage);

  /// Mark the front superpage of a link ready and transfer it to the ready queue
  void transferSuperpageFromLinkToReady(Link& link, bool reclaim = false);

  /// Enable debug mode by writing to the appropriate CRU register
  void enableDebugMode();

  /// Reset debug mode to the state it was in prior to the start of execution
  void resetDebugMode();

  /// Reclaims superpages pushed to the CRU but not filled on DMA stop
  void reclaimSuperpages();

  /// BAR 0 is needed for DMA engine interaction and various other functions
  std::shared_ptr<CruBar> cruBar;

  /// BAR 2 is needed to read serial number, temperature, etc.
  std::shared_ptr<CruBar> cruBar2;

  /// Features of the firmware
  FirmwareFeatures mFeatures;

  /// Vector of objects representing links
  std::vector<Link> mLinks;

  /// To keep track of how many slots are available in the link queues (in mLinks) in total
  size_t mLinkQueuesTotalAvailable;

  /// Queue for superpages that have been transferred and are waiting for popping by the user
  std::unique_ptr<SuperpageQueue> mReadyQueue;

  // These variables are configuration parameters

  /// Gives the data source
  const DataSource::type mDataSource;

  /// Flag to know if we should reset the debug register after we fiddle with it
  bool mDebugRegisterReset = false;

  /// DMA page size, as specified when opening the channel
  const size_t mDmaPageSize;

  bool mFirstSPPushed = false;

  /// Empty Superpage FIFO counters per link
  std::unordered_map<int, uint32_t> mEmptySPFifoCounters;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_CRUDMACHANNEL_H_
