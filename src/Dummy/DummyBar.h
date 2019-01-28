/// \file DummyBar.h
/// \brief Definition of the DummyBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_DUMMY_DUMMYBAR_H_
#define ALICEO2_SRC_READOUTCARD_DUMMY_DUMMYBAR_H_

#include "ReadoutCard/BarInterface.h"
#include <array>

namespace AliceO2 {
namespace roc {

/// A dummy implementation of the BarInterface.
/// This exists so that the ReadoutCard module may be built even if the all the dependencies of the 'real' card
/// implementation are not met (this mainly concerns the PDA driver library).
/// In the future, a dummy implementation could be a simulated card. Currently, methods of this implementation do
/// nothing besides print which method was called. Returned values are static and should not be used.
class DummyBar : public BarInterface
{
  public:

    DummyBar(const Parameters& parameters);
    virtual ~DummyBar();
    virtual uint32_t readRegister(int index) override;
    virtual void writeRegister(int index, uint32_t value) override;
    virtual void modifyRegister(int index, int position, int width, uint32_t value) override;

    virtual int getIndex() const override
    {
      return mBarIndex;
    }

    virtual size_t getSize() const override
    {
      return 4*1024;
    }

    virtual CardType::type getCardType() override
    {
      return CardType::Dummy;
    }

    virtual boost::optional<int32_t> getSerial() override
    {
      return {};
    }
    
    virtual boost::optional<float> getTemperature() override
    {
      return {};
    }

    virtual boost::optional<std::string> getFirmwareInfo() override
    {
      return {};
    }

    virtual boost::optional<std::string> getCardId() override
    {
      return {};
    }

    virtual int32_t getDroppedPackets() override
    {
      return 0;
    }

    virtual uint32_t getCTPClock() override
    {
      return 0;
    }

    virtual uint32_t getLocalClock() override
    {
      return 0;
    }

    virtual int32_t getLinks() override
    {
      return 0;
    }

    virtual int32_t getLinksPerWrapper(int /*wrapper*/) override
    {
      return 0;
    }

    void configure() override;

    void reconfigure() override;
  private:
    int mBarIndex;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_DUMMY_DUMMYBAR_H_
