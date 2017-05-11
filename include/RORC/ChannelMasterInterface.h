/// \file ChannelMasterInterface.h
/// \brief Definition of the ChannelMasterInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_CHANNELMASTERINTERFACE_H_
#define ALICEO2_INCLUDE_RORC_CHANNELMASTERINTERFACE_H_

#include <cstdint>
#include <boost/optional.hpp>
#include <InfoLogger/InfoLogger.hxx>
#include "RORC/Parameters.h"
#include "RORC/CardType.h"
#include "RORC/ParameterTypes/ResetLevel.h"
#include "RORC/RegisterReadWriteInterface.h"
#include "RORC/Superpage.h"

namespace AliceO2 {
namespace Rorc {

/// Pure abstract interface for objects that obtain a master lock on a channel and provides an interface to control
/// and use that channel.
class ChannelMasterInterface: public virtual RegisterReadWriteInterface
{
  public:
    using MasterSharedPtr = std::shared_ptr<ChannelMasterInterface>;

    virtual ~ChannelMasterInterface()
    {
    }

    /// Starts DMA for the given channel
    /// Call this before pushing pages. May become unneeded in the future.
    virtual void startDma() = 0;

    /// Stops DMA for the given channel
    /// Called automatically on channel closure.
    virtual void stopDma() = 0;

    /// Resets the channel. Requires the DMA to be stopped.
    /// \param resetLevel The depth of the reset
    virtual void resetChannel(ResetLevel::type resetLevel) = 0;

    /// Adds superpage to "transfer queue".
    /// A superpage represents a physically contiguous buffer that will be filled with multiple pages from the card.
    /// The user is responsible for making sure enqueued superpages do not overlap - the driver will dutifully overwrite
    /// your data if you tell it to do so.
    ///
    /// This method will not necessarily already start the actual transfer of data.
    /// The driver may delay it until fillSuperpages() is called, for example.
    /// When the transfer into a superpage is ready, the driver will move it to the "ready queue".
    /// At that point, it may be inspected with getSuperpage() and popped with popSuperpage().
    ///
    /// Note that this method, getSuperpage() and popSuperpage() take and return *copies* of the Superpage struct.
    /// While the user "owns" the superpage, they cannot change anything about the superpage information given to the
    /// driver once it is pushed.
    ///
    /// \param superpage Superpage to push
    virtual void pushSuperpage(Superpage superpage) = 0;

    /// Gets the superpage at the front of the "ready queue". Does not pop it.
    /// Note that it returns a copy of the Superpage's values.
    virtual Superpage getSuperpage() = 0;

    /// Pops and returns the superpage at the front of the "ready queue".
    virtual Superpage popSuperpage() = 0;

    /// Handles internal driver business. Call in a loop. May be replaced by internal driver thread at some point.
    virtual void fillSuperpages() = 0;

    /// Gets the amount of superpages that can still be pushed into the "transfer queue" using pushSuperpage()
    virtual int getTransferQueueAvailable() = 0;

    /// Gets the amount of superpages currently in the "ready queue". If there is more than one available, the front
    /// superpage can be inspected with getSuperpage() or popped with popSuperpage().
    virtual int getReadyQueueSize() = 0;

    /// Returns the type of the card this ChannelMaster is controlling
    /// \return The card type
    virtual CardType::type getCardType() = 0;

    /// Sets the InfoLogger log level for this channel
    virtual void setLogLevel(InfoLogger::InfoLogger::Severity severity) = 0;

    /// Gets card temperature in °C if available
    /// \return Temperature in °C if available, else an empty optional
    virtual boost::optional<float> getTemperature() = 0;

    /// Gets firmware version information
    /// \return A string containing firmware version information if available, else an empty optional
    virtual boost::optional<std::string> getFirmwareInfo() = 0;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_CHANNELMASTERINTERFACE_H_
