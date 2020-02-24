// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Crorc/Common.h
/// \brief Definition of common CRORC structs and utilities
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRORC_COMMON_H_
#define ALICEO2_READOUTCARD_CRORC_COMMON_H_

namespace AliceO2
{
namespace roc
{
namespace Crorc
{

enum LinkStatus {
  Up,
  Down,
};

struct Link {
  LinkStatus status = LinkStatus::Down;
  float opticalPower = 0.0;
};

struct ReportInfo {
  std::map<int, Link> linkMap;
  uint16_t crorcId;
  bool qsfpEnabled;
  bool dynamicOffset;
};

} // namespace Crorc
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRORC_COMMON_H_
