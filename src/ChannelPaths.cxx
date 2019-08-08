// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ChannelPaths.cxx
/// \brief Implementation of the ChannelPaths class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelPaths.h"
#include <boost/format.hpp>

namespace b = boost;

namespace AliceO2
{
namespace roc
{
namespace
{
static const char* DIR_SHAREDMEM = "/dev/shm/";
static const char* FORMAT = "%s/AliceO2_RoC_%s_Channel_%i%s";
} // namespace

ChannelPaths::ChannelPaths(PciAddress pciAddress, int channel) : mPciAddress(pciAddress), mChannel(channel)
{
}

std::string ChannelPaths::makePath(std::string fileName, const char* directory) const
{
  return b::str(b::format(FORMAT) % directory % mPciAddress.toString() % mChannel % fileName);
}

std::string ChannelPaths::lock() const
{
  return makePath(".lock", DIR_SHAREDMEM);
}

std::string ChannelPaths::fifo() const
{
  return makePath("_fifo", DIR_SHAREDMEM);
}

std::string ChannelPaths::namedMutex() const
{
  return b::str(b::format("AliceO2_RoC_%s_Channel_%i_Mutex") % mPciAddress.toString() % mChannel);
}

} // namespace roc
} // namespace AliceO2
