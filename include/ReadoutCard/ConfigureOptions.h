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
/// \file ConfigureOptions.h
/// \brief Definition of the ConfigureOptions struct.
///
/// \author Sylvain Chapeland (sylvain.chapeland@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_CONFIGUREOPTIONS_H_
#define O2_READOUTCARD_INCLUDE_CONFIGUREOPTIONS_H_

#include "ReadoutCard/NamespaceAlias.h"

namespace o2
{
namespace roc
{

/// Options controlling how a card configuration is applied (not what is configured: see Parameters)
struct ConfigureOptions {
  /// Rewrite all settings, even if the card state already matches
  bool force = false;

  /// Clock (TTC) configuration block: run it, skip it, or run only it
  enum class ClockConfig { Yes,
                           No,
                           Only };
  ClockConfig clockConfig = ClockConfig::Yes;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_CONFIGUREOPTIONS_H_
