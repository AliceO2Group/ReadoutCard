// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file BarInterfaceBase.h
/// \brief Definition of the BarInterfaceBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_BARINTERFACEBASE_H_
#define ALICEO2_SRC_READOUTCARD_BARINTERFACEBASE_H_

#include <boost/optional/optional_io.hpp>
#include <memory>
#include "RocPciDevice.h"
#include "Pda/PdaBar.h"
#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/Logger.h"
#include "ReadoutCard/Parameters.h"

namespace AliceO2
{
namespace roc
{

/// Partially implements the BarInterface
class BarInterfaceBase : public BarInterface
{
 public:
  BarInterfaceBase(const Parameters& parameters, std::unique_ptr<RocPciDevice> rocPciDevice);
  BarInterfaceBase(std::shared_ptr<Pda::PdaBar> bar);
  virtual ~BarInterfaceBase();

  virtual uint32_t readRegister(int index) override;
  virtual void writeRegister(int index, uint32_t value) override;
  virtual void modifyRegister(int index, int position, int width, uint32_t value) override;

  virtual int getIndex() const override
  {
    return mPdaBar->getIndex();
  }

  virtual size_t getSize() const override
  {
    return mPdaBar->getSize();
  }

  /// Default implementation for optional function
  virtual boost::optional<int32_t> getSerial() override
  {
    return {};
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

  /// Default implementation for optional function
  virtual uint32_t getDroppedPackets(int /*endpoint*/) override
  {
    return 0;
  }

  /// Default implementation for optional function
  virtual uint32_t getTotalPacketsPerSecond(int /*endpoint*/) override
  {
    return 0;
  }

  /// Default implementation for optional function
  virtual uint32_t getCTPClock() override
  {
    return 0;
  }

  /// Default implementation for optional function
  virtual uint32_t getLocalClock() override
  {
    return 0;
  }

  /// Default implementation for optional function
  virtual int32_t getLinks() override
  {
    return 0;
  }

  /// Default implementation for optional function
  virtual int32_t getLinksPerWrapper(int /*wrapper*/) override
  {
    return 0;
  }

  virtual int getEndpointNumber() override
  {
    return -1;
  }

 protected:
  /// BAR index
  int mBarIndex;

  /// PDA device objects
  std::unique_ptr<RocPciDevice> mRocPciDevice;

  /// PDA BAR object ptr
  std::shared_ptr<Pda::PdaBar> mPdaBar;

  /// Convenience function for InfoLogger
  void log(const std::string& logMessage, ILMessageOption = LogInfoOps);

 private:
  /// Inheriting classes must implement this to check whether a given read is safe.
  /// If it is not safe, it should throw an UnsafeReadAccess exception
  //virtual void checkReadSafe(int index) = 0;
  /// Inheriting classes must implement this to check whether a given write is safe.
  /// If it is not safe, it should throw an UnsafeWriteAccess exception
  //virtual void checkWriteSafe(int index, uint32_t value) = 0;

  std::string mLoggerPrefix;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_BARINTERFACEBASE_H_
