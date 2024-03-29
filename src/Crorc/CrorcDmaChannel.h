
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
/// \file CrorcDmaChannel.h
/// \brief Definition of the CrorcDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_SRC_CRORC_CRORCDMACHANNEL_H_
#define O2_READOUTCARD_SRC_CRORC_CRORCDMACHANNEL_H_

#include <mutex>
#include <unordered_map>
#include <boost/scoped_ptr.hpp>
#include "DmaChannelPdaBase.h"
#include "CrorcBar.h"
#include "ReadoutCard/Parameters.h"
#include "folly/ProducerConsumerQueue.h"

namespace o2
{
namespace roc
{

/// Extends DmaChannel object, and provides device-specific functionality
/// Note: the functions prefixed with "crorc" are translated from the functions of the C interface (src/c/rorc/...")
class CrorcDmaChannel final : public DmaChannelPdaBase
{
 public:
  CrorcDmaChannel(const Parameters& parameters);
  virtual ~CrorcDmaChannel() override;

  virtual CardType::type getCardType() override;

  virtual bool injectError() override
  {
    return false;
  }
  virtual boost::optional<int32_t> getSerial() override;
  virtual boost::optional<std::string> getFirmwareInfo() override;

  virtual bool pushSuperpage(Superpage superpage) override;

  virtual int getTransferQueueAvailable() override;
  virtual int getReadyQueueSize() override;

  virtual Superpage getSuperpage() override;
  virtual Superpage popSuperpage() override;
  virtual void fillSuperpages() override;
  virtual bool isTransferQueueEmpty() override;
  virtual bool isReadyQueueFull() override;
  virtual int32_t getDroppedPackets() override;
  virtual bool areSuperpageFifosHealthy() override;

  AllowedChannels allowedChannels();

 protected:
  virtual void deviceStartDma() override;
  virtual void deviceStopDma() override;
  virtual void deviceResetChannel(ResetLevel::type resetLevel = ResetLevel::InternalSiu) override;

 private:
  /// Superpage size supported by the CRORC backend
  static constexpr size_t SUPERPAGE_SIZE = 1 * 1024 * 1024;

  /// DMA page size
  static constexpr size_t DMA_PAGE_SIZE = 8 * 1024;

  /// Max amount of superpages in the transfer queue (i.e. pending transfer).
  static constexpr size_t TRANSFER_QUEUE_CAPACITY = 128;
  static constexpr size_t TRANSFER_QUEUE_CAPACITY_ALLOCATIONS = TRANSFER_QUEUE_CAPACITY + 1; // folly Queue needs + 1

  /// Max amount of superpages in the intermediate queue (i.e. pushed superpage).
  /// CRORC FW only handles a single superpage at a time
  static constexpr size_t INTERMEDIATE_QUEUE_CAPACITY = 1;
  static constexpr size_t INTERMEDIATE_QUEUE_CAPACITY_ALLOCATIONS = INTERMEDIATE_QUEUE_CAPACITY + 1;

  /// Max amount of superpages in the ready queue (i.e. finished transfer).
  /// This is an arbitrary size, can easily be increased if more headroom is needed.
  static constexpr size_t READY_QUEUE_CAPACITY = TRANSFER_QUEUE_CAPACITY;
  static constexpr size_t READY_QUEUE_CAPACITY_ALLOCATIONS = TRANSFER_QUEUE_CAPACITY_ALLOCATIONS;

  /// Minimum number of superpages needed to bootstrap DMA
  //static constexpr size_t DMA_START_REQUIRED_SUPERPAGES = 1;
  //static constexpr size_t DMA_START_REQUIRED_SUPERPAGES = READYFIFO_ENTRIES;

  using SuperpageQueue = folly::ProducerConsumerQueue<Superpage>;

  /// Enables data receiving in the RORC
  void startDataReceiving();

  /// Initializes and starts the data generator
  void startDataGenerator();

  CrorcBar* getBar()
  {
    return crorcBar.get();
  }

  /// Memory mapped file for the Superpage info buffer
  boost::scoped_ptr<MemoryMappedFile> mSuperpageInfoFile;

  /// PDA DMABuffer FIFO object for the Superpage info buffer
  boost::scoped_ptr<Pda::PdaDmaBuffer> mPdaDmaBufferFifo;

  /// Userspace address of Superpage info in DMA buffer
  uintptr_t mSuperpageInfoAddressUser;

  /// Bus address of Superpage info in DMA buffer
  uintptr_t mSuperpageInfoAddressBus;

  /// BAR used for DMA engine and configuration
  std::shared_ptr<CrorcBar> crorcBar;

  /// Queue for superpages that are pushed from the Readout thread
  SuperpageQueue mTransferQueue{ TRANSFER_QUEUE_CAPACITY_ALLOCATIONS };

  /// Queue for the superpage that is pushed to the firmware
  SuperpageQueue mIntermediateQueue{ INTERMEDIATE_QUEUE_CAPACITY_ALLOCATIONS };

  /// Queue for superpages that are filled
  SuperpageQueue mReadyQueue{ READY_QUEUE_CAPACITY_ALLOCATIONS };

  /// Address of DMA buffer in userspace
  uintptr_t mDmaBufferUserspace = 0;

  // These variables are configuration parameters

  /// DMA page size
  const size_t mPageSize;

  /// Allows sending the RDYRX and EOBTR commands.
  bool mRDYRX;

  /// Allows sending the STBRD and EOBTR commands for FEE configuration
  const bool mSTBRD;

  /// Gives the data source
  const DataSource::type mDataSource;

  /// Enables the data generator
  bool mGeneratorEnabled;

  /// Check the SuperpageInfo buffer for superpage availability
  bool isASuperpageAvailable();

  /// A convenience struct to access the Superpage info DMA buffer
  struct SuperpageInfo {
    volatile uint32_t size;  // size of the written superpage in bytes (24 bits)
    volatile uint32_t count; // incrementing counter that signals a completed write (8 bits)
  };

  const size_t kSuperpageInfoSize = 2 * sizeof(uint32_t);

  SuperpageInfo* getSuperpageInfoUser()
  {
    return reinterpret_cast<SuperpageInfo*>(mSuperpageInfoAddressUser);
  }

  // Counter for the available (from the fw) superpages
  uint32_t mSPAvailCount = 0xff;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_CRORC_CRORCDMACHANNEL_H_
