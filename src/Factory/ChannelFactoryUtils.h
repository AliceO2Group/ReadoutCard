
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
/// \file ChannelFactoryUtils.h
/// \brief Definition of helper functions for Channel factories
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <map>
#include <algorithm>
#include "ExceptionInternal.h"
#include "ReadoutCard/CardDescriptor.h"
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/Parameters.h"
#include "Crorc/CrorcDmaChannel.h"
#include "Crorc/CrorcBar.h"
#include "Cru/CruDmaChannel.h"
#include "Cru/CruBar.h"
#include "RocPciDevice.h"

namespace o2
{
namespace roc
{
namespace ChannelFactoryUtils
{

inline std::unique_ptr<RocPciDevice> findCard(const Parameters::CardIdType& id)
{
  return std::make_unique<RocPciDevice>(id);
}

template <typename Interface>
std::unique_ptr<Interface> dmaChannelFactoryHelper(const Parameters& params)
{
  auto id = params.getCardIdRequired();
  auto rocPciDevice = findCard(id);
  auto cardDescriptor = rocPciDevice->getCardDescriptor();

  if (cardDescriptor.cardType == CardType::Cru) {
    return std::make_unique<CruDmaChannel>(params);
  } else if (cardDescriptor.cardType == CardType::Crorc) {
    return std::make_unique<CrorcDmaChannel>(params);
  }

  BOOST_THROW_EXCEPTION(DeviceFinderException() << ErrorInfo::Message("Unknown card type"));
}

template <typename Interface>
std::unique_ptr<Interface> barFactoryHelper(const Parameters& params)
{
  auto id = params.getCardIdRequired();
  auto rocPciDevice = findCard(id);
  auto cardDescriptor = rocPciDevice->getCardDescriptor();

  if (cardDescriptor.cardType == CardType::Cru) {
    return std::make_unique<CruBar>(params, std::move(rocPciDevice));
  } else if (cardDescriptor.cardType == CardType::Crorc) {
    return std::make_unique<CrorcBar>(params, std::move(rocPciDevice));
  }

  BOOST_THROW_EXCEPTION(DeviceFinderException() << ErrorInfo::Message("Unknown card type"));
}

} // namespace ChannelFactoryUtils
} // namespace roc
} // namespace o2
