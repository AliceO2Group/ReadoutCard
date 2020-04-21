// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CrorcDmaChannel.h
/// \brief Definition of the CrorcDmaChannel class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_CRORC_CRORCDMACHANNEL_H_
#define ALICEO2_SRC_READOUTCARD_CRORC_CRORCDMACHANNEL_H_

#include <mutex>
#include <unordered_map>
#include <boost/scoped_ptr.hpp>
#include "DmaChannelPdaBase.h"
#include "CrorcBar.h"
#include "ReadoutCard/Parameters.h"
#include "ReadyFifo.h"
#include "folly/ProducerConsumerQueue.h"

namespace AliceO2
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
  static constexpr size_t TRANSFER_QUEUE_CAPACITY = MAX_SUPERPAGE_DESCRIPTORS;
  static constexpr size_t TRANSFER_QUEUE_CAPACITY_ALLOCATIONS = TRANSFER_QUEUE_CAPACITY + 1; // folly Queue needs + 1

  /// Max amount of superpages in the ready queue (i.e. finished transfer).
  /// This is an arbitrary size, can easily be increased if more headroom is needed.
  static constexpr size_t READY_QUEUE_CAPACITY = TRANSFER_QUEUE_CAPACITY;
  static constexpr size_t READY_QUEUE_CAPACITY_ALLOCATIONS = TRANSFER_QUEUE_CAPACITY_ALLOCATIONS;

  /// Minimum number of superpages needed to bootstrap DMA
  //static constexpr size_t DMA_START_REQUIRED_SUPERPAGES = 1;
  //static constexpr size_t DMA_START_REQUIRED_SUPERPAGES = READYFIFO_ENTRIES;

  using SuperpageQueue = folly::ProducerConsumerQueue<Superpage>;

  /// Namespace for enum describing the status of a page's arrival
  struct DataArrivalStatus {
    enum type {
      NoneArrived,
      PartArrived,
      WholeArrived,
    };
  };

  ReadyFifo* getReadyFifoUser()
  {
    return reinterpret_cast<ReadyFifo*>(mReadyFifoAddressUser);
  }

  /// Enables data receiving in the RORC
  void startDataReceiving();

  /// Initializes and starts the data generator
  void startDataGenerator();

  /// Pushes a page to the CRORC's Free FIFO
  /// \param readyFifoIndex Index of the Ready FIFO to write the page's transfer status to
  /// \param pageBusAddress Address on the bus to push the page to
  void pushFreeFifoPage(int readyFifoIndex, uintptr_t pageBusAddress, int pageSize);

  /// Check if data has arrived
  DataArrivalStatus::type dataArrived(int index);

  /// Get front index of FIFO
  int getFreeFifoFront() const
  {
    return (mFreeFifoBack + mFreeFifoSize) % READYFIFO_ENTRIES;
  };

  CrorcBar* getBar()
  {
    return crorcBar.get();
  }

  /// Starts pending DMA with given superpage for the initial pages
  void startPendingDma();

  /// BAR used for DMA engine and configuration
  std::shared_ptr<CrorcBar> crorcBar;

  /// Memory mapped file for the ReadyFIFO
  boost::scoped_ptr<MemoryMappedFile> mBufferFifoFile;

  /// PDA DMABuffer object for the ReadyFIFO
  boost::scoped_ptr<Pda::PdaDmaBuffer> mPdaDmaBufferFifo;

  /// Userspace address of FIFO in DMA buffer
  uintptr_t mReadyFifoAddressUser;

  /// Bus address of FIFO in DMA buffer
  uintptr_t mReadyFifoAddressBus;

  /// Front index of the firmware FIFO (WRITE)
  int mFreeFifoFront = 0;

  /// Back index of the firmware FIFO (READ)
  int mFreeFifoBack = 0;

  /// Amount of elements in the firmware FIFO
  int mFreeFifoSize = 0;

  /// Queue for superpages that are pushed to the firmware FIFO
  SuperpageQueue mTransferQueue{ TRANSFER_QUEUE_CAPACITY_ALLOCATIONS };

  /// Queue for superpages that are filled
  SuperpageQueue mReadyQueue{ READY_QUEUE_CAPACITY_ALLOCATIONS };

  /// Address of DMA buffer in userspace
  uintptr_t mDmaBufferUserspace = 0;

  /// Indicates deviceStartDma() was called, but DMA was not actually started yet. We do this because we need a
  /// superpage to actually start.
  bool mPendingDmaStart = false;

  // These variables are configuration parameters

  /// DMA page size
  const size_t mPageSize;

  /// Allows sending the RDYRX and EOBTR commands.
  bool mRDYRX;

  /// Allows sending the STBRD and EOBTR commands for FEE configuration
  const bool mSTBRD;

  /// Enforces that the data reading is carried out with the Start Block Read (STBRD) command
  /// XXX Not sure if this should be a parameter...
  const bool mUseFeeAddress;

  /// Gives the data source
  const DataSource::type mDataSource;

  /// Enables the data generator
  bool mGeneratorEnabled;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_CRORC_CRORCDMACHANNEL_H_
