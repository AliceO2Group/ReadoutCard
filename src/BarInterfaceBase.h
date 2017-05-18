/// \file BarInterfaceBase.h
/// \brief Definition of the BarInterfaceBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <memory>
#include "RocPciDevice.h"
#include "Pda/PdaBar.h"
#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/Parameters.h"

namespace AliceO2 {
namespace roc {

/// Partially implements the BarInterface
class BarInterfaceBase: public BarInterface
{
  public:

    BarInterfaceBase(const Parameters& parameters);
    virtual ~BarInterfaceBase();

    virtual uint32_t readRegister(int index) override;
    virtual void writeRegister(int index, uint32_t value) override;

    virtual int getIndex() const override
    {
      return mPdaBar->getIndex();
    }

    virtual size_t getSize() const override
    {
      return mPdaBar->getSize();
    }

  protected:
    /// BAR index
    int mBarIndex;

    /// PDA device objects
    std::unique_ptr<RocPciDevice> mRocPciDevice;

    /// PDA BAR object
    std::unique_ptr<Pda::PdaBar> mPdaBar;

  private:
    /// Inheriting classes must implement this to check whether a given read is safe.
    /// If it is not safe, it should throw an UnsafeReadAccess exception
    virtual void checkReadSafe(int index) = 0;
    /// Inheriting classes must implement this to check whether a given write is safe.
    /// If it is not safe, it should throw an UnsafeWriteAccess exception
    virtual void checkWriteSafe(int index, uint32_t value) = 0;
};

} // namespace roc
} // namespace AliceO2
