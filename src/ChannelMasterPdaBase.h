/// \file ChannelMasterPdaBase.h
/// \brief Definition of the ChannelMasterPdaBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <boost/scoped_ptr.hpp>
#include "ChannelMasterBase.h"
#include "PageAddress.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDmaBuffer.h"
#include "RORC/ChannelMasterInterface.h"
#include "RORC/Exception.h"
#include "RORC/MemoryMappedFile.h"
#include "RORC/Parameters.h"
#include "RorcDevice.h"

namespace AliceO2 {
namespace Rorc {

/// Partially implements the ChannelMasterInterface. It takes care of PDA-based functionality that is common to the
/// C-RORC and CRU implementations.
class ChannelMasterPdaBase: public ChannelMasterBase
{
  public:
    using AllowedChannels = std::set<int>;

    /// Constructor for the ChannelMaster object
    /// \param cardType Type of the card
    /// \param parameters Parameters of the channel
    /// \param allowedChannels Channels allowed by this card type
    /// \param fifoSize Size of the Firmware FIFO in the DMA buffer
    ChannelMasterPdaBase(CardType::type cardType, const Parameters& parameters, const AllowedChannels& allowedChannels,
        size_t fifoSize);

    ~ChannelMasterPdaBase();

    virtual void startDma() final override;
    virtual void stopDma() final override;
    void resetChannel(ResetLevel::type resetLevel) final override;
    virtual uint32_t readRegister(int index) final override;
    virtual void writeRegister(int index, uint32_t value) final override;

  protected:
    /// Namespace for describing the state of the DMA
    struct DmaState
    {
        /// The state of the DMA
        enum type
        {
          UNKNOWN = 0, STOPPED = 1, STARTED = 2
        };
    };

    /// Template method called by startDma() to do device-specific (CRORC, RCU...) actions
    /// Note: subclasses should not call getLockGuard() in this function, as the ChannelMaster will have the lock
    /// already.
    virtual void deviceStartDma() = 0;

    /// Template method called by stopDma() to do device-specific (CRORC, RCU...) actions
    /// Note: subclasses should not call getLockGuard() in this function, as the ChannelMaster will have the lock
    /// already.
    virtual void deviceStopDma() = 0;

    /// Template method called by resetChannel() to do device-specific (CRORC, RCU...) actions
    /// Note: subclasses should not call getLockGuard() in this function, as the ChannelMaster will have the lock
    /// already.
    virtual void deviceResetChannel(ResetLevel::type resetLevel) = 0;

    /// Function for getting the bus address that corresponds to the user address + given offset
    uintptr_t getBusOffsetAddress(size_t offset);

    /// The size of the shared state data file. It should be over-provisioned, since subclasses may also allocate their
    /// own shared data in this file.
    static size_t getSharedDataSize()
    {
      // 4KiB ought to be enough for anybody, and is also the standard x86 page size.
      // Since shared memory needs to be a multiple of page size, this should work out.
      return 4l * 1024l;
    }

    /// Name for the shared data object in the shared state file
    static std::string getSharedDataName()
    {
      return "ChannelMasterSharedData";
    }

    uintptr_t getFifoAddressBus() const
    {
      return mFifoAddressBus;
    }

    uintptr_t getFifoAddressUser() const
    {
      return mFifoAddressUser;
    }

    uintptr_t getBarUserspace() const
    {
      return mPdaBar->getUserspaceAddress();
    }

    volatile uint32_t* getBarUserspaceU32() const
    {
      return mPdaBar->getUserspaceAddressU32();
    }

    const Pda::PdaDmaBuffer& getPdaDmaBuffer() const
    {
      return *(mPdaDmaBuffer.get());
    }

    Pda::PdaBar& getPdaBar()
    {
      return *(mPdaBar.get());
    }

    Pda::PdaBar* getPdaBarPtr()
    {
      return mPdaBar.get();
    }

    const RorcDevice& getRorcDevice() const
    {
      return *(mRorcDevice.get());
    }

    DmaState::type getDmaState() const
    {
      return mDmaState;
    }

  private:

    /// Current state of the DMA
    DmaState::type mDmaState;

    /// PDA device objects
    boost::scoped_ptr<RorcDevice> mRorcDevice;

    /// PDA BAR object
    boost::scoped_ptr<Pda::PdaBar> mPdaBar;

    /// PDA DMABuffer object for the pages
    boost::scoped_ptr<Pda::PdaDmaBuffer> mPdaDmaBuffer;

    /// Userspace address of FIFO in DMA buffer
    uintptr_t mFifoAddressUser;

    /// Bus address of FIFO in DMA buffer
    uintptr_t mFifoAddressBus;
};

} // namespace Rorc
} // namespace AliceO2
