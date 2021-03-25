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

#ifndef O2_READOUTCARD_INCLUDE_CHANNELFACTORY_H_
#define O2_READOUTCARD_INCLUDE_CHANNELFACTORY_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <memory>
#include <string>
#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/DmaChannelInterface.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ParameterTypes/SerialId.h"

namespace o2
{
namespace roc
{

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
  DmaChannelSharedPtr getDmaChannel(const Parameters& parameters);

  /// Get an object to access a BAR with the given card ID and channel number.
  /// Passing 'DUMMY_SERIAL_NUMBER' as serial number returns a dummy implementation
  /// \param parameters Parameters for the channel
  BarSharedPtr getBar(const Parameters& parameters);

  /// Get an object to access a BAR with the given card ID and channel number.
  /// Passing 'DUMMY_SERIAL_NUMBER' as serial number returns a dummy implementation
  /// \param cardId ID of the card
  /// \param channel Channel number to open
  BarSharedPtr getBar(const Parameters::CardIdType& cardId, const Parameters::ChannelNumberType& channel)
  {
    return getBar(Parameters::makeParameters(cardId, channel));
  }

  static SerialId getDummySerialId()
  {
    return { SERIAL_DUMMY, ENDPOINT_DUMMY };
  }
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_CHANNELFACTORY_H_
