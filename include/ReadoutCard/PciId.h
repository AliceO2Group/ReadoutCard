
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
/// \file PciId.h
/// \brief Definition of the PciId struct.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_PCIID_H_
#define O2_READOUTCARD_INCLUDE_PCIID_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <string>

namespace o2
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
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_PCIID_H_
