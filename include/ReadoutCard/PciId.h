// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file PciId.h
/// \brief Definition of the PciId struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_PCIID_H_
#define ALICEO2_INCLUDE_READOUTCARD_PCIID_H_

#include <string>

namespace AliceO2
{
namespace roc
{

/// Simple data holder class for a PCI ID, consisting of a device ID and vendor ID.
struct PciId {
  const std::string& getDeviceId() const
  {
    return device;
  }

  const std::string& getVendorId() const
  {
    return vendor;
  }

  std::string device;
  std::string vendor;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_PCIID_H_
