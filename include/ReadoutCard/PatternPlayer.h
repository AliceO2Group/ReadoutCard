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
    uint128_t pat0= 0x0;
    uint128_t pat1 = 0x0;
    uint128_t pat2 = 0x0;
    uint128_t pat3 = 0x0;
    uint32_t pat1Length = 1;
    uint32_t pat1Delay = 0;
    uint32_t pat2Length = 1;
    uint32_t pat3Length = 1;
    uint32_t pat1TriggerSelect = 29;
    uint32_t pat2TriggerSelect = 30;
    uint32_t pat3TriggerSelect = 0;
    uint32_t pat2TriggerTF = 0;

    bool exePat1AtStart = false;
    bool exePat1Now = false;
    bool exePat2Now = false;
  };

  PatternPlayer(std::shared_ptr<BarInterface> bar);
  void play(PatternPlayer::Info info);
  PatternPlayer::Info read();

 private:
  void configure(bool startConfig);
  void exePat1AtStart(bool enable = false);
  void exePat1();
  void exePat2();

  std::shared_ptr<BarInterface> mBar;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_PATTERNPLAYER_H_
