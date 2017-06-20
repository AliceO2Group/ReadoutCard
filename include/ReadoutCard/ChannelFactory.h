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
    // static constexpr int DUMMY_SERIAL_NUMBER = -1; ///< Deprecated, use getDummySerialNumber() instead

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
    /// \param parameters Parameters for the channel
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
