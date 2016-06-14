///
/// \file CardDummy.h
/// \author Pascal Boeschoten
///

#pragma once

#include "RORC/CardInterface.h"

namespace AliceO2 {
namespace Rorc {

/// A dummy implementation of the CardInterface. This exists so that the readout module may be built even if the all
/// the dependencies of the 'real' card implementation are not met (this mainly concerns the PDA driver library).
/// In the future, a dummy implementation could be a simulated card. Currently, methods of this implementation do
/// nothing besides print which method was called and with what parameters. Returned values are static and should not
/// be used.
class CardDummy : public CardInterface
{
  public:
    CardDummy();
    virtual ~CardDummy();

    virtual void startDma(int channel);
    virtual void stopDma(int channel);
    virtual void openChannel(int channel, const ChannelParameters& channelParameters);
    virtual void closeChannel(int channel);
    virtual void resetCard(int channel, ResetLevel::type resetLevel);
    virtual uint32_t readRegister(int channel, int index);
    virtual void writeRegister(int channel, int index, uint32_t value);
    virtual int getNumberOfChannels();
    virtual volatile void* getMappedMemory(int channel);
    virtual PageVector getMappedPages(int channel);
    virtual PageHandle pushNextPage(int channel);
    virtual bool isPageArrived(int channel, const PageHandle& handle);
    virtual Page getPage(int channel, const PageHandle& handle);
    virtual void markPageAsRead(int channel, const PageHandle& handle);
    virtual int getNumberOfPages(int channel);

};

} // namespace Rorc
} // namespace AliceO2
