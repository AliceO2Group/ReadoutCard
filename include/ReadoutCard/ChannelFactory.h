// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ChannelFactory.h
/// \brief Definition of the ChannelFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CHANNELFACTORY_H_
#define ALICEO2_INCLUDE_READOUTCARD_CHANNELFACTORY_H_


#include "ReadoutCard/Parameters.h"
#include <memory>
#include <string>
#include "ReadoutCard/DmaChannelInterface.h"
#include "ReadoutCard/BarInterface.h"

namespace AliceO2 {
namespace roc {

/// Factory class for creating objects to access and control card channels
class ChannelFactory
{
  public:
    using DmaChannelSharedPtr = std::shared_ptr<DmaChannelInterface>;
    using BarSharedPtr = std::shared_ptr<BarInterface>;

    ChannelFactory();
    virtual ~ChannelFactory();

    /// Get an object to access a DMA channel with the given serial number and channel number.
    /// Passing 'DUMMY_SERIAL_NUMBER' as serial number returns a dummy implementation
    /// \param parameters Parameters for the channel
    DmaChannelSharedPtr getDmaChannel(const Parameters &parameters);

    /// Get an object to access a BAR with the given card ID and channel number.
    /// Passing 'DUMMY_SERIAL_NUMBER' as serial number returns a dummy implementation
    /// \param parameters Parameters for the channel
    BarSharedPtr getBar(const Parameters &parameters);

    /// Get an object to access a BAR with the given card ID and channel number.
    /// Passing 'DUMMY_SERIAL_NUMBER' as serial number returns a dummy implementation
    /// \param cardId ID of the card
    /// \param channel Channel number to open
    BarSharedPtr getBar(const Parameters::CardIdType& cardId, const Parameters::ChannelNumberType& channel)
    {
      return getBar(Parameters::makeParameters(cardId, channel));
    }

    static int getDummySerialNumber()
    {
      return -1;
    }
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CHANNELFACTORY_H_
