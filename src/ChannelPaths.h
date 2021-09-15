
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
/// \file ChannelPaths.h
/// \brief Definition of the ChannelPaths class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_CHANNELPATHS_H_
#define O2_READOUTCARD_SRC_CHANNELPATHS_H_

#include <string>
#include "ReadoutCard/ParameterTypes/PciAddress.h"

namespace o2
{
namespace roc
{

/// Namespace for functions to generate paths for filesystem object used by the Channel classes
class ChannelPaths
{
 public:
  /// Constructs the ChannelPaths object with the given parameters
  /// \param pciAddress PCI address of the card
  /// \param channel Channel of the card
  ChannelPaths(PciAddress pciAddress, int channel);

  /// Generates a path for the channel file lock
  /// \return The path
  std::string lock() const;

  /// Generates a path for the channel's shared memory FIFO object. It will be in hugetlbfs.
  /// \return The path
  std::string fifo() const;

  /// Generates a name for the channel's mutex
  /// \return The name
  std::string namedMutex() const;

 private:
  std::string makePath(std::string fileName, const char* directory) const;

  const PciAddress mPciAddress;
  const int mChannel;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_CHANNELPATHS_H_
