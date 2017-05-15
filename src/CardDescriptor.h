/// \file CardDescriptor.h
/// \brief Definition of the CardDescriptor struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "ReadoutCard/CardType.h"
#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include "ReadoutCard/PciId.h"

namespace AliceO2 {
namespace roc {

/// Data holder for basic information about a card
struct CardDescriptor
{
    CardType::type cardType;
    int serialNumber;
    PciId pciId;
    PciAddress pciAddress;
};

} // namespace roc
} // namespace AliceO2
