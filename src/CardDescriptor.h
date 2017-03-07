/// \file CardDescriptor.h
/// \brief Definition of the CardDescriptor struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "RORC/CardType.h"
#include "RORC/PciAddress.h"
#include "RORC/PciId.h"

namespace AliceO2 {
namespace Rorc {

/// Data holder for basic information about a card
struct CardDescriptor
{
    CardType::type cardType;
    int serialNumber;
    PciId pciId;
    PciAddress pciAddress;
};

} // namespace Rorc
} // namespace AliceO2
