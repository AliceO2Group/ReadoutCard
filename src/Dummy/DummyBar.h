/// \file DummyBar.h
/// \brief Definition of the DummyBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ReadoutCard/BarInterface.h"
#include <array>

namespace AliceO2 {
namespace roc {

/// A dummy implementation of the BarInterface.
/// This exists so that the ReadoutCard module may be built even if the all the dependencies of the 'real' card
/// implementation are not met (this mainly concerns the PDA driver library).
/// In the future, a dummy implementation could be a simulated card. Currently, methods of this implementation do
/// nothing besides print which method was called. Returned values are static and should not be used.
class DummyBar final : public BarInterface
{
  public:

    DummyBar(const Parameters& parameters);
    virtual ~DummyBar();
    virtual uint32_t readRegister(int index) override;
    virtual void writeRegister(int index, uint32_t value) override;

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

  private:
    int mBarIndex;
};

} // namespace roc
} // namespace AliceO2
