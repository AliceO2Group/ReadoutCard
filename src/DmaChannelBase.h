
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
/// \file DmaChannelBase.h
/// \brief Definition of the DmaChannelBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_DMACHANNELBASE_H_
#define O2_READOUTCARD_SRC_DMACHANNELBASE_H_

#include <set>
#include <vector>
#include <memory>
#include <mutex>
#include <boost/optional.hpp>
#include "ChannelPaths.h"
#include "ExceptionInternal.h"
#include "Pda/PdaLock.h"
#include "ReadoutCard/CardDescriptor.h"
#include "ReadoutCard/DmaChannelInterface.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/Logger.h"
#include "ReadoutCard/InterprocessLock.h"
#include "ReadoutCard/Parameters.h"
#include "Utilities/Util.h"

namespace o2
{
namespace roc
{

/// Partially implements the DmaChannelInterface. It provides:
/// - Interprocess synchronization
/// - Creation of files and directories related to the channel
/// - Logging facilities
class DmaChannelBase : public DmaChannelInterface
{
 public:
  using AllowedChannels = std::set<int>;

  /// Constructor for the DmaChannel object
  /// \param cardDescriptor Card descriptor
  /// \param parameters Parameters of the channel
  /// \param allowedChannels Channels allowed by this card type
  DmaChannelBase(CardDescriptor cardDescriptor, Parameters& parameters,
                 const AllowedChannels& allowedChannels);
  virtual ~DmaChannelBase();

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

 protected:
  /// Namespace for enum describing the initialization state of the shared data
  struct InitializationState {
    enum type {
      UNKNOWN = 0,
      UNINITIALIZED = 1,
      INITIALIZED = 2
    };
  };

  int getChannelNumber() const
  {
    return mChannelNumber;
  }

  int getSerialNumber() const
  {
    return mCardDescriptor.serialId.getSerial();
  }

  int getEndpointNumber() const
  {
    return mCardDescriptor.serialId.getEndpoint();
  }

  const CardDescriptor& getCardDescriptor() const
  {
    return mCardDescriptor;
  }

  ChannelPaths getPaths()
  {
    return { getCardDescriptor().pciAddress, getChannelNumber() };
  }

  /// Convenience function for InfoLogger
  void log(const std::string& logMessage, ILMessageOption = LogInfoDevel);

 private:
  /// Check if the channel number is valid
  void checkChannelNumber(const AllowedChannels& allowedChannels);

  /// Check the validity of basic parameters
  void checkParameters(Parameters& parameters);

  /// Type of the card
  const CardDescriptor mCardDescriptor;

  /// DMA channel number
  const int mChannelNumber;

  /// Lock that guards against both inter- and intra-process ownership
  std::unique_ptr<Interprocess::Lock> mInterprocessLock;

  std::string mLoggerPrefix;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_DMACHANNELBASE_H_
