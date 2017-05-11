/// \file ChannelSlaveInterface.h
/// \brief Definition of the ChannelSlaveInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CHANNELSLAVEINTERFACE_H_
#define ALICEO2_INCLUDE_READOUTCARD_CHANNELSLAVEINTERFACE_H_

#include <cstdint>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"
#include "ReadoutCard/Parameters.h"

namespace AliceO2 {
namespace roc {

/// Provides a limited-access interface to a RORC channel
/// TODO
///   - Register access restricted
///   - Possibly read-only access to pages?
class ChannelSlaveInterface: public virtual RegisterReadWriteInterface
{
  public:
    virtual ~ChannelSlaveInterface()
    {
    }

    /// Return the type of the RORC card this ChannelSlave is controlling
    /// \return The card type
    virtual CardType::type getCardType() = 0;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CHANNELSLAVEINTERFACE_H_
