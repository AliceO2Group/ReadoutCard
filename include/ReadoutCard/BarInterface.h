
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file BarInterface.h
/// \brief Definition of the BarInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_BARINTERFACE_H_
#define O2_READOUTCARD_INCLUDE_BARINTERFACE_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <cstdint>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"
#include "ReadoutCard/Parameters.h"
#include "boost/optional.hpp"

namespace o2
{
namespace roc
{

/// Provides access to a BAR of a readout card.
///
/// Registers are read and written in 32-bit chunks.
/// Inherits from RegisterReadWriteInterface and implements the read & write methods.
///
/// Access to 'dangerous' registers may be restricted: UnsafeReadAccess and UnsafeWriteAccess exceptions may be thrown.
///
/// To instantiate an implementation, use the ChannelFactory::getBar() method.
class BarInterface : public virtual RegisterReadWriteInterface
{
 public:
  virtual ~BarInterface()
  {
  }

  /// Get the index of this BAR
  virtual int getIndex() const = 0;

  /// Get the size of this BAR in bytes
  virtual size_t getSize() const = 0;

  /// Returns the type of the card
  /// \return The card type
  virtual CardType::type getCardType() = 0;

  virtual boost::optional<int32_t> getSerial() = 0;

  virtual boost::optional<float> getTemperature() = 0;

  virtual boost::optional<std::string> getFirmwareInfo() = 0;

  virtual boost::optional<std::string> getCardId() = 0;

  virtual uint32_t getDroppedPackets(int endpoint) = 0;

  virtual uint32_t getTotalPacketsPerSecond(int endpoint) = 0;

  virtual uint32_t getCTPClock() = 0;

  virtual uint32_t getLocalClock() = 0;

  virtual int32_t getLinks() = 0;

  virtual int32_t getLinksPerWrapper(int wrapper) = 0;

  virtual int getEndpointNumber() = 0;

  virtual void configure(bool force) = 0;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_BARINTERFACE_H_
