///
/// \file CardPdaBase.h
/// \author Pascal Boeschoten
///

#pragma once

#include <vector>
#include <memory>
#include <pda.h>
#include "RORC/CardInterface.h"
#include "SglWrapper.h"
#include "BarWrapper.h"
#include "ReadyFifoWrapper.h"

namespace AliceO2 {
namespace Rorc {

/// Abstract class for common (non card-specific) PDA-based functions and data
class CardPdaBase : public CardInterface
{
  public:
    CardPdaBase(DeviceOperator* deviceOperator, PciDevice* pciDevice, int serialNumber, int amountOfChannels);
    virtual ~CardPdaBase();
    virtual void openChannel(int channel, const ChannelParameters& channelParameters) final;
    virtual void closeChannel(int channel) final;
    virtual int getNumberOfPages(int channel) final;
    virtual volatile void* getMappedMemory(int channel) final;

    class ChannelData
    {
      public:
        ChannelData(int channel = -1, const ChannelParameters& parameters = ChannelParameters());

        /// The number of this channel
        int channel;

        /// Wrapper around PDA scatter-gather list
        std::unique_ptr<SglWrapper> sglWrapper;

        /// Wrapper around the software FIFO
        std::unique_ptr<ReadyFifoWrapper> fifo;

        /// Wrapper around the the PCI BAR
        BarWrapper bar;

        /// PDA structs for the allocated buffers
        DMABuffer* dmaBuffer;

        /// Userspace address of the mapped buffers
        volatile uint32_t* mappedBuffer;

        /// Array to keep track of read pages (false: wasn't read out, true: was read out).
        /// TODO Make a bit more descriptive / refactor. Maybe merge into FifoWrapper?
        std::vector<bool> pageWasReadOut;

        const ChannelParameters& params() const {
          return parameters;
        }

      private:
        /// Configuration parameters
        ChannelParameters parameters;
    };

  protected:
    ChannelData& getChannelData(int channel);

    /// Template method called by openDmaChannel()
    virtual void validateChannelParameters(const ChannelParameters& parameters) = 0;

    /// Template method called by openDmaChannel() to do device-specific (CRORC, RCU...) actions
    virtual void deviceOpenDmaChannel(int channel) = 0;

    /// Template method called by closeDmaChannel() to do device-specific (CRORC, RCU...) actions
    virtual void deviceCloseDmaChannel(int channel) = 0;

    DMABuffer_SGNode* getScatterGatherList(int channel);

    void initBar(ChannelData& cd);
    void initDmaBuffer(ChannelData& cd);
    void initDmaBufferMap(ChannelData& cd);
    void initScatterGatherList(ChannelData& cd);

    bool isChannelOpen(int channel);


    /// PDA device operator
    DeviceOperator* const deviceOperator;

    /// PDA struct for the device
    PciDevice* const pciDevice;

    /// Serial number of the card
    const int serialNumber;


  private:
    void setChannelOpen(int channel, bool open);

    // Flags for keeping track of channel open/closed state
    std::vector<bool> channelOpen;

    // Channel data
    std::vector<ChannelData> channelDataVector;
};

} // namespace Rorc
} // namespace AliceO2
