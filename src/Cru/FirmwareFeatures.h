// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file FirmwareFeatures.h
/// \brief Definition of the FirmwareFeatures struct
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_CRU_FIRMWAREFEATURES_H_
#define O2_READOUTCARD_CRU_FIRMWAREFEATURES_H_

namespace o2
{
namespace roc
{

struct FirmwareFeatures {
  /// Is the card's firmware a "standalone design"?
  bool standalone = false;

  /// Is a serial number available?
  bool serial = false;

  /// Is firmware information available?
  bool firmwareInfo = false;

  /// Is the special register for loopback at BAR2 0x8000020 enabled?
  bool dataSelection = false;

  /// Is the temperature sensor enabled?
  bool temperature = false;

  /// Is the Arria 10 FPGA chip ID available?
  bool chipId = false;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_FIRMWAREFEATURES_H_
