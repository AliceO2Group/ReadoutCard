// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ChannelFactory.cxx
/// \brief Implementation of the ChannelFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ReadoutCard.h"
#include "ReadoutCard/ChannelFactory.h"
#include "Factory/ChannelFactoryUtils.h"

namespace AliceO2
{
namespace roc
{

using namespace ChannelFactoryUtils;
using namespace CardTypeTag;

ChannelFactory::ChannelFactory()
{
}

ChannelFactory::~ChannelFactory()
{
}

auto ChannelFactory::getDmaChannel(const Parameters& params) -> DmaChannelSharedPtr
{
  return dmaChannelFactoryHelper<DmaChannelInterface>(params);
}

auto ChannelFactory::getBar(const Parameters& params) -> BarSharedPtr
{
  return barFactoryHelper<BarInterface>(params);
}

} // namespace roc
} // namespace AliceO2
