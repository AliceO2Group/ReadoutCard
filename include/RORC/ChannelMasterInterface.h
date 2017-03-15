/// \file ChannelMasterInterface.h
/// \brief Definition of the ChannelMasterInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_CHANNELMASTERINTERFACE_H_
#define ALICEO2_INCLUDE_RORC_CHANNELMASTERINTERFACE_H_

#include <cstdint>
#include <memory>
#include <boost/optional.hpp>
#include <boost/exception/all.hpp>
#include <InfoLogger/InfoLogger.hxx>
#include "RORC/Parameters.h"
#include "RORC/CardType.h"
#include "RORC/ResetLevel.h"
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

    /// Adds superpage to queue. A superpage represents a physically contiguous buffer that will be filled with multiple
    /// pages from the card.
    /// The user is responsible for making sure enqueued superpages do not overlap - the driver will dutifully overwrite
    /// your data if you tell it to do so.
    ///
    /// \param superpage Superpage to push
    virtual void pushSuperpage(Superpage superpage) = 0;

    /// Gets the superpage at the front of the queue (i.e. the oldest superpage)
    virtual Superpage getSuperpage() = 0;

    /// Tells the driver to stop keeping track of the superpage at the front of the queue (i.e. the oldest superpage)
    virtual Superpage popSuperpage() = 0;

    /// Call in a loop. Equivalent of old fillFifo(). May be replaced by internal driver thread at some point
    virtual void fillSuperpages() = 0;

    /// Gets the amount of superpages currently in the queue
    virtual int getSuperpageQueueCount() = 0;

    /// Gets the amount of superpages that can still be enqueued
    virtual int getSuperpageQueueAvailable() = 0;

    /// Gets the maximum amount of superpages allowed in the queue
    virtual int getSuperpageQueueCapacity() = 0;

    /// Returns the type of the RORC card this ChannelMaster is controlling
    /// \return The card type
    virtual CardType::type getCardType() = 0;

    /// Set the InfoLogger log level for this channel
    virtual void setLogLevel(InfoLogger::InfoLogger::Severity severity) = 0;

    /// Get card temperature in °C if available
    /// \return Temperature in °C if available, else an empty optional
    virtual boost::optional<float> getTemperature() = 0;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_CHANNELMASTERINTERFACE_H_
