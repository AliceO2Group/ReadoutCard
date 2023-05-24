
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
/// \file Cru/Common.h
/// \brief Definition of common CRU utilities
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_COMMON_H_
#define O2_READOUTCARD_CRU_COMMON_H_

#include "Constants.h"
#include "ExceptionInternal.h"
#include "ReadoutCard/BarInterface.h"

namespace o2
{
namespace roc
{
namespace Cru
{

enum LinkStatus {
  Down,
  Up,
  UpWasDown
};

constexpr const char* LinkStatusToString(LinkStatus status)
{
  switch (status) {
    case LinkStatus::Down:
      return "DOWN";
    case LinkStatus::Up:
      return "UP";
    case LinkStatus::UpWasDown:
      return "UP (was DOWN)";
    default:
      throw std::invalid_argument("Invalid LinkStatus provided for string conversion");
  }
}

struct Link {
  int dwrapper = -1;
  int wrapper = -1;
  int bank = -1;
  uint32_t id = 0xffffffff;
  uint32_t dwrapperId = 0xffffffff;
  uint32_t globalId = 0xffffffff;
  uint32_t baseAddress;
  GbtMux::type gbtMux = GbtMux::type::Ttc;
  GbtMode::type gbtTxMode = GbtMode::type::Gbt;
  GbtMode::type gbtRxMode = GbtMode::type::Gbt;
  bool loopback = false;
  DatapathMode::type datapathMode = DatapathMode::type::Packet;
  bool enabled = false;
  LinkStatus stickyBit = LinkStatus::Down;
  float opticalPower = 0.0;
  float txFreq = 0x0; //In MHz
  float rxFreq = 0x0; //In MHz
  uint32_t allowRejection = 0;
  uint32_t systemId = 0x0;
  uint32_t feeId = 0x0;
  /*uint32_t systemId = 0x1ff; // invalid, used as "unset" placeholder
  uint32_t feeId = 0x1f; // invalid, used as "unset" placeholder*/
  uint32_t glitchCounter = 0;
  uint32_t fecCounter = 0;
  uint32_t pktProcessed = 0;
  uint32_t pktErrorProtocol = 0;
  uint32_t pktErrorCheck1 = 0;
  uint32_t pktErrorCheck2 = 0;
  uint32_t pktErrorOversize = 0;
  uint32_t orbitSor = 0;

  bool operator==(const Link& dlink) const
  {
    if (enabled == dlink.enabled && enabled == false) {
      return true;
    } else {
      return (dwrapper == dlink.dwrapper &&
              wrapper == dlink.wrapper &&
              bank == dlink.bank &&
              id == dlink.id &&
              dwrapperId == dlink.dwrapperId &&
              baseAddress == dlink.baseAddress &&
              gbtMux == dlink.gbtMux &&
              gbtTxMode == dlink.gbtTxMode &&
              gbtRxMode == dlink.gbtRxMode &&
              loopback == dlink.loopback &&
              datapathMode == dlink.datapathMode &&
              enabled == dlink.enabled &&
              allowRejection == dlink.allowRejection &&
              systemId == dlink.systemId &&
              feeId == dlink.feeId);
    }
  }

  bool operator!=(const Link& dlink) const
  {
    return !operator==(dlink);
  }
};

struct ReportInfo {
  std::map<int, Link> linkMap;
  uint32_t ttcClock;
  uint32_t downstreamData;
  uint32_t ponStatusRegister;
  uint32_t onuAddress;
  uint16_t cruId;
  bool dynamicOffset;
  uint32_t triggerWindowSize;
  bool gbtEnabled;
  bool userLogicEnabled;
  bool runStatsEnabled;
  bool userAndCommonLogicEnabled;
  uint16_t timeFrameLength;
  bool dmaStatus;
};

struct OnuStickyStatus {
  LinkStatus upstreamStatus;
  LinkStatus downstreamStatus;
  uint32_t stickyValue;
  uint32_t stickyValuePrev;
};

struct OnuStatus {
  uint32_t onuAddress;
  bool rx40Locked;
  bool phaseGood;
  bool rxLocked;
  bool operational;
  bool mgtTxReady;
  bool mgtRxReady;
  bool mgtTxPllLocked;
  bool mgtRxPllLocked;
  OnuStickyStatus stickyStatus;
  uint32_t ponQuality;
  int ponQualityStatus;
  double ponRxPower;
};

struct FecStatus {
  bool clearFecCrcError;
  bool latchFecCrcError;
  bool slowControlFramingLocked;
  uint8_t fecSingleErrorCount;
  uint8_t fecDoubleErrorCount;
  uint8_t crcErrorCount;
};

struct LinkPacketInfo {
  uint32_t accepted;
  uint32_t rejected;
  uint32_t forced;
};

struct WrapperPacketInfo {
  uint32_t dropped;
  uint32_t totalPacketsPerSec;
};

struct PacketMonitoringInfo {
  std::map<int, LinkPacketInfo> linkPacketInfoMap;
  std::map<int, WrapperPacketInfo> wrapperPacketInfoMap;
};

struct TriggerMonitoringInfo {
  uint64_t hbCount;
  double hbRate;
  uint64_t phyCount;
  double phyRate;
  uint64_t tofCount;
  double tofRate;
  uint64_t calCount;
  double calRate;
  uint64_t eoxCount;
  uint64_t soxCount;
};

enum TriggerMode {
  Manual,
  Periodic,
  Continuous,
  Fixed,
  Hc,
  Cal,
};

struct CtpInfo {
  uint32_t bcMax;
  uint32_t hbDrop;
  uint32_t hbKeep;
  uint32_t hbMax;

  TriggerMode triggerMode;
  uint32_t triggerFrequency;

  bool generateEox;
  bool generateSingleTrigger;

  uint32_t orbitInit;
};

struct UserLogicInfo {
  uint32_t eventSize;
  bool random;
  uint32_t systemId;
  uint32_t linkId;
};

struct LoopbackStats {
  bool pllLock;
  bool rxLockedToData;
  bool dataLayerUp;
  bool gbtPhyUp;
  uint32_t rxDataErrorCount;
  uint32_t fecErrorCount;
};

uint32_t getWrapperBaseAddress(int wrapper);
uint32_t getXcvrRegisterAddress(int wrapper, int bank, int link, int reg = 0);
void atxcal0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress);
void txcal0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress);
void rxcal0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress);
void fpllref0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress, uint32_t refClock);
void fpllcal0(std::shared_ptr<BarInterface> bar, uint32_t baseAddress, bool configCompensation = true);
void fpllref(std::map<int, Link> linkMap, std::shared_ptr<BarInterface> bar, uint32_t refClock, uint32_t baseAddress = 0);
void fpllcal(std::map<int, Link> linkMap, std::shared_ptr<BarInterface> bar, uint32_t baseAddress = 0, bool configCompensation = true);
uint32_t getBankPllRegisterAddress(int wrapper, int bank);
uint32_t waitForBit(std::shared_ptr<BarInterface> bar, uint32_t address, uint32_t position, uint32_t value);

} // namespace Cru
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_COMMON_H_
