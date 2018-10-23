/// \file BarInterfaceBase.h
/// \brief Definition of the BarInterfaceBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_BARINTERFACEBASE_H_
#define ALICEO2_SRC_READOUTCARD_BARINTERFACEBASE_H_

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
    BarInterfaceBase(std::shared_ptr<Pda::PdaBar> bar);
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

    /// Default implementation for optional function
    virtual boost::optional<float> getTemperature() override
    {
      return {};
    }

    /// Default implementation for optional function
    virtual boost::optional<std::string> getFirmwareInfo() override
    {
      return {};
    }

    /// Default implementation for optional function
    virtual boost::optional<std::string> getCardId() override
    {
      return {};
    }


  protected:
    /// BAR index
    int mBarIndex;

    /// PDA device objects
    std::unique_ptr<RocPciDevice> mRocPciDevice;

    /// PDA BAR object ptr
    std::shared_ptr<Pda::PdaBar> mPdaBar;

    // PDA BAR object
    Pda::PdaBar mlPdaBar;

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

#endif // ALICEO2_SRC_READOUTCARD_BARINTERFACEBASE_H_
