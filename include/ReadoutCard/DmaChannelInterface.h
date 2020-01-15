// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DmaChannelInterface.h
/// \brief Definition of the DmaChannelInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_DMACHANNELINTERFACE_H_
#define ALICEO2_INCLUDE_READOUTCARD_DMACHANNELINTERFACE_H_

#include <cstdint>
#include <boost/optional.hpp>
#include <InfoLogger/InfoLogger.hxx>
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/ParameterTypes/ResetLevel.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"
#include "ReadoutCard/Superpage.h"

namespace AliceO2
{
namespace roc
{

/// Interface for objects that provide an interface to control and use a DMA channel.
class DmaChannelInterface
{
 public:
  virtual ~DmaChannelInterface()
  {
  }

  /// Starts DMA for the given channel
  /// Call this before pushing pages. May become unneeded in the future.
  virtual void startDma() = 0;

  /// Resets the channel. Requires the DMA to be stopped.
  /// \param resetLevel The depth of the reset
  virtual void resetChannel(ResetLevel::type resetLevel) = 0;

  /// Adds superpage to "transfer queue".
  /// A superpage represents a buffer that will be filled with multiple pages by the card.
  ///
  /// It *must* be contiguous in the card's bus address space. The two recommended ways to ensure this are:
  ///  * Make sure the superpage is contained within a hugepage (see README.md for more info on hugepages)
  ///  * Enable your machine's IOMMU. In this case, the channel buffer that you register when opening a channel will
  ///    be completely contiguous as far as the card is concerned.
  ///
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
  virtual bool pushSuperpage(Superpage superpage) = 0;

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

  // Returns true if the Transfer Queue is empty
  // i.e. no free pages to send to the card
  virtual bool isTransferQueueEmpty() = 0;

  // Returns true if the Ready Queue is full
  // i.e. queue filled by the card
  virtual bool isReadyQueueFull() = 0;

  // Returns the number of dropped packets, as reported by the BAR
  virtual int32_t getDroppedPackets() = 0;

  /// Stops DMA for the given channel.
  /// Called automatically on channel closure.
  /// This moves any remaining superpages to the "ready queue", even if they are not filled.
  virtual void stopDma() = 0;

  /// Returns the type of the card this DmaChannelInterface is controlling
  /// \return The card type
  virtual CardType::type getCardType() = 0;

  /// Sets the InfoLogger log level for this channel
  virtual void setLogLevel(InfoLogger::InfoLogger::Severity severity) = 0;

  /// Get the PCI address of this DMA channel
  /// Note: dummy card will always return 0:0.0
  virtual PciAddress getPciAddress() = 0;

  /// Get the NUMA node of this DMA channel
  /// The node number is retrieved from the `/sys/bus/pci/devices/[PCI address]/numa_node` sysfs file
  /// Note: dummy card will always return 0
  virtual int getNumaNode() = 0;

  // Optional features

  /// Request injection of an error into the data stream
  /// Currently, only the CRU backend supports this when using the internal data generator
  /// \return True if successful, false if no error was injected
  virtual bool injectError() = 0;

  /// Gets card serial number if available
  /// \return Serial number if available, else an empty optional
  virtual boost::optional<int32_t> getSerial() = 0;

  /// Gets card temperature in °C if available
  /// \return Temperature in °C if available, else an empty optional
  virtual boost::optional<float> getTemperature() = 0;

  /// Gets firmware version information
  /// \return A string containing firmware version information if available, else an empty optional
  virtual boost::optional<std::string> getFirmwareInfo() = 0;

  /// Gets card unique ID, such as an FPGA chip ID in the case of the CRU
  /// \return A string containing the unique ID
  virtual boost::optional<std::string> getCardId() = 0;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_DMACHANNELINTERFACE_H_
