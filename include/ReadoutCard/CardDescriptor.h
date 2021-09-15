
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
/// \file CardDescriptor.h
/// \brief Definition of the CardDescriptor struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_CARDDESCRIPTOR_H_
#define O2_READOUTCARD_INCLUDE_CARDDESCRIPTOR_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <boost/optional.hpp>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include "ReadoutCard/ParameterTypes/SerialId.h"
#include "ReadoutCard/PciId.h"

namespace o2
{
namespace roc
{

/// Data holder for basic information about a card
struct CardDescriptor {
  CardType::type cardType;
  SerialId serialId;
  PciId pciId;
  PciAddress pciAddress;
  int32_t numaNode;
  int sequenceId;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_CARDDESCRIPTOR_H_
