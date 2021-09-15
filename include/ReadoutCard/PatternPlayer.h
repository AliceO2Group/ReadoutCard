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

/// \file PatternPlayer.h
/// \brief Definition of the PatternPlayer class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_PATTERNPLAYER_H_
#define O2_READOUTCARD_INCLUDE_PATTERNPLAYER_H_

#include "ReadoutCard/NamespaceAlias.h"
#include "ReadoutCard/Cru.h"
#include "ReadoutCard/BarInterface.h"
#include <boost/multiprecision/cpp_int.hpp>

using namespace boost::multiprecision;

namespace o2
{
namespace roc
{

class PatternPlayer
{
 public:
  struct Info {
    uint128_t syncPattern = 0x0;
    uint128_t resetPattern = 0x0;
    uint128_t idlePattern = 0x0;
    uint32_t syncLength = 1;
    uint32_t syncDelay = 0;
    uint32_t resetLength = 1;
    uint32_t resetTriggerSelect = 30;
    uint32_t syncTriggerSelect = 29;
    bool syncAtStart = false;
    bool triggerSync = false;
    bool triggerReset = false;
  };

  PatternPlayer(std::shared_ptr<BarInterface> bar);
  void play(PatternPlayer::Info info);

 private:
  void configure(bool startConfig);
  void setIdlePattern(uint128_t pattern);
  void setSyncPattern(uint128_t pattern);
  void configureSync(uint32_t length = 1, uint32_t delay = 0);
  void setResetPattern(uint128_t pattern);
  void configureReset(uint32_t length = 1);
  void selectPatternTrigger(uint32_t syncTrigger = 3, uint32_t resetTrigger = 3);
  void enableSyncAtStart(bool enable = false);
  void triggerSync();
  void triggerReset();

  std::shared_ptr<BarInterface> mBar;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_PATTERNPLAYER_H_
