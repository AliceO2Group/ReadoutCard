// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CardDescriptor.h
/// \brief Definition of the CardDescriptor struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_CARDDESCRIPTOR_H_
#define ALICEO2_SRC_READOUTCARD_CARDDESCRIPTOR_H_

#include <boost/optional.hpp>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include "ReadoutCard/PciId.h"

namespace AliceO2 {
namespace roc {

/// Data holder for basic information about a card
struct CardDescriptor
{
    CardType::type cardType;
    boost::optional<int> serialNumber;
    PciId pciId;
    PciAddress pciAddress;
    int32_t numaNode;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_CARDDESCRIPTOR_H_
