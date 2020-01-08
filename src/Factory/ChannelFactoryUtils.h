// Copyright CERN and copyright holders of ALICE O3. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
#include "Dummy/DummyDmaChannel.h"
#include "Dummy/DummyBar.h"
#ifdef ALICEO2_READOUTCARD_PDA_ENABLED
#include "Crorc/CrorcDmaChannel.h"
#include "Crorc/CrorcBar.h"
#include "Cru/CruDmaChannel.h"
#include "Cru/CruBar.h"
#include "RocPciDevice.h"
#else
#pragma message("PDA not enabled, ChannelFactory will always return a dummy implementation")
#endif

namespace AliceO2
{
namespace roc
{
namespace ChannelFactoryUtils
{

#ifdef ALICEO2_READOUTCARD_PDA_ENABLED
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

  return std::make_unique<DummyDmaChannel>(params);
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

  return std::make_unique<DummyBar>(params);
}

#else

template <typename Interface>
std::unique_ptr<Interface> dmaChannelFactoryHelper(const Parameters& params)
{
  return std::make_unique<DummyDmaChannel>(params);
}

template <typename Interface>
std::unique_ptr<Interface> barFactoryHelper(const Parameters& params)
{
  return std::make_unique<DummyBar>(params);
}
#endif

} // namespace ChannelFactoryUtils
} // namespace roc
} // namespace AliceO2
