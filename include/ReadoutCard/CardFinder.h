// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CardFinder.h
/// \brief Definitions for the CardFinder methods.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CARDFINDER_H_
#define ALICEO2_INCLUDE_READOUTCARD_CARDFINDER_H_

#include "ReadoutCard/CardDescriptor.h"

namespace AliceO2
{
namespace roc
{

std::vector<CardDescriptor> findCards(); //Possibly extend with card types (crorc, cru)

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CARDFINDER_H_
