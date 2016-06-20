///
/// \file CardPdaCrorc.h
/// \author Pascal Boeschoten
///

#pragma once

#include "CardPdaBase.h"

namespace AliceO2 {
namespace Rorc {

/// CRORC implementation
class CardPdaCrorc: public CardPdaBase
{
  public:
    struct DataArrivalStatus
    {
        enum type
        {
          NONE_ARRIVED = 0, // == RORC_DATA_BLOCK_NOT_ARRIVED;
          PART_ARRIVED = 1, // == RORC_NOT_END_OF_EVENT_ARRIVED
          WHOLE_ARRIVED = 2 // == RORC_LAST_BLOCK_OF_EVENT_ARRIVED
        };
    };

    CardPdaCrorc(DeviceOperator* deviceOperator, PciDevice* pciDevice, int serialNumber);
    virtual ~CardPdaCrorc();
    virtual void startDma(int channel);
    virtual void stopDma(int channel);
    virtual void resetCard(int channel, ResetLevel::type resetLevel);
    virtual uint32_t readRegister(int channel, int index);
    virtual void writeRegister(int channel, int index, uint32_t value);
    virtual int getNumberOfChannels();
    virtual PageVector getMappedPages(int channel);

    virtual PageHandle pushNextPage(int channel);
    virtual bool isPageArrived(int channel, const PageHandle& handle);
    virtual Page getPage(int channel, const PageHandle& handle);
    virtual void markPageAsRead(int channel, const PageHandle& handle);

  protected:
    virtual void validateChannelParameters(const ChannelParameters& parameters);
    virtual void deviceOpenDmaChannel(int channel);
    virtual void deviceCloseDmaChannel(int channel);

  private:
    /// Initializes the readyFifo wrapper object
    void initializeReadyFifo(ChannelData& cd);

    /// Initializes the RORC's freeFifo by pushing 128 pages.
    void initializeFreeFifo(ChannelData& cd, int pagesToPush);

    /// Pushes a page to the RORC's freeFifo
    void pushFreeFifoPage(ChannelData& cd, int fifoIndex);

    /// Returns the arrival status of a page
    CardPdaCrorc::DataArrivalStatus::type dataArrived(ChannelData& cd, int index);

    /// Arms the DDL with the given reset mask
    void armDdl(ChannelData& cd, int32_t resetMask);

    /// Enables the RORC data receiving
    void startDataReceiving(ChannelData& cd);

    /// Gets the starting address of the page data in userspace
    volatile void* getDataStartAddress(ChannelData& cd);

    /// Arms the data generator
    void armDataGenerator(ChannelData& cd, const GeneratorParameters& gen);

    /// Starts the data generator
    void startDataGenerator(ChannelData& cd, int maxEvents);

    /// Not entirely sure what this is for
    long long int loopPerUsec;

    /// Not entirely sure what this is for
    double pciLoopPerUsec;
};

} // namespace Rorc
} // namespace AliceO2
