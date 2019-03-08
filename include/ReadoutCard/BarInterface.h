// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file BarInterface.h
/// \brief Definition of the BarInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_BARINTERFACE_H_
#define ALICEO2_INCLUDE_READOUTCARD_BARINTERFACE_H_

#include <cstdint>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"
#include "ReadoutCard/Parameters.h"
#include "boost/optional.hpp"

namespace AliceO2 {
namespace roc {

/// Provides access to a BAR of a readout card.
///
/// Registers are read and written in 32-bit chunks.
/// Inherits from RegisterReadWriteInterface and implements the read & write methods.
///
/// Access to 'dangerous' registers may be restricted: UnsafeReadAccess and UnsafeWriteAccess exceptions may be thrown.
///
/// To instantiate an implementation, use the ChannelFactory::getBar() method.
class BarInterface: public virtual RegisterReadWriteInterface
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
    
    virtual int32_t getDroppedPackets() = 0;
    
    virtual uint32_t getCTPClock() = 0;
    
    virtual uint32_t getLocalClock() = 0;

    virtual int32_t getLinks() = 0;

    virtual int32_t getLinksPerWrapper(int wrapper) = 0;

    virtual int getEndpointNumber() = 0;
    
    virtual void configure() = 0;
    
    virtual void reconfigure() = 0;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_BARINTERFACE_H_
