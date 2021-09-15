
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
/// \file ChannelFactory.cxx
/// \brief Implementation of the ChannelFactory class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/ReadoutCard.h"
#include "ReadoutCard/ChannelFactory.h"
#include "Factory/ChannelFactoryUtils.h"

namespace o2
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
} // namespace o2
