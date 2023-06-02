
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
/// \file Crorc/Common.h
/// \brief Definition of common CRORC structs and utilities
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRORC_COMMON_H_
#define O2_READOUTCARD_CRORC_COMMON_H_

namespace o2
{
namespace roc
{
namespace Crorc
{

enum LinkStatus {
  Down,
  Up,
};

struct Link {
  LinkStatus status = LinkStatus::Down;
  float opticalPower = 0.0;
  uint32_t orbitSor = 0;
};

struct ReportInfo {
  std::map<int, Link> linkMap;
  uint16_t crorcId;
  uint16_t timeFrameLength;
  bool timeFrameDetectionEnabled;
  bool qsfpEnabled;
  bool dynamicOffset;
};

struct PacketMonitoringInfo {
  uint32_t acquisitionRate;
  uint32_t packetsReceived;
};

} // namespace Crorc
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRORC_COMMON_H_
