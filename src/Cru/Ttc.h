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

/// \file Ttc.h
/// \brief Definition of the Ttc class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_TTC_H_
#define O2_READOUTCARD_CRU_TTC_H_

#include "Common.h"
#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/InterprocessLock.h"

namespace o2
{
namespace roc
{

class Ttc
{
  using LinkStatus = Cru::LinkStatus;
  using OnuStickyStatus = Cru::OnuStickyStatus;
  using OnuStatus = Cru::OnuStatus;
  using FecStatus = Cru::FecStatus;

 public:
  Ttc(std::shared_ptr<BarInterface> bar, int serial = -1, int endpoint = -1);

  void calibrateTtc();
  void setClock(uint32_t clock);
  void resetFpll();
  bool configurePonTx(uint32_t onuAddress);
  void selectDownstreamData(uint32_t downstreamData);
  uint32_t getPllClock();
  uint32_t getDownstreamData();
  uint32_t getHbTriggerLtuCount();
  uint32_t getPhyTriggerLtuCount();
  uint32_t getTofTriggerLtuCount();
  uint32_t getCalTriggerLtuCount();
  std::pair<uint32_t, uint32_t> getEoxSoxLtuCount();

  void resetCtpEmulator(bool doReset);
  void setEmulatorTriggerMode(Cru::TriggerMode mode);
  void doManualPhyTrigger();
  void setEmulatorContinuousMode();
  void setEmulatorIdleMode();
  void setEmulatorStandaloneFlowControl(bool allow = false);
  void setEmulatorBCMAX(uint32_t bcmax);
  void setEmulatorHBMAX(uint32_t hbmax);
  void setEmulatorPrescaler(uint32_t hbkeep, uint32_t hbdrop);
  void setEmulatorPHYSDIV(uint32_t physdiv);
  void setEmulatorCALDIV(uint32_t caldiv);
  void setEmulatorHCDIV(uint32_t hcdiv);
  void setEmulatorORBITINIT(uint32_t orbitInit);
  void setFixedBCTrigger(std::vector<uint32_t> FBCTVector);

  OnuStatus onuStatus(bool monitoring = false);
  FecStatus fecStatus();

 private:
  void configurePlls(uint32_t clock);
  void setRefGen(int frequency = 240);
  OnuStickyStatus getOnuStickyStatus(bool monitoring = false);
  uint32_t getPonQuality();
  int getPonQualityStatus();
  double getPonRxPower();

  std::shared_ptr<BarInterface> mBar;
  std::unique_ptr<Interprocess::Lock> mI2cLock;

  int mSerial;
  int mEndpoint;
  static constexpr uint32_t MAX_BCID = 3564 - 1;
};
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_TTC_H_
