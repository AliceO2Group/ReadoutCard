// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CruBar.h
/// \brief Definition of the CruBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_CRUBAR_H_
#define O2_READOUTCARD_CRU_CRUBAR_H_

#include <cstddef>
#include <set>
#include <map>
#include <boost/optional/optional.hpp>
#include "BarInterfaceBase.h"
#include "Common.h"
#include "Cru/Constants.h"
#include "Cru/FirmwareFeatures.h"
#include "ExceptionInternal.h"
#include "Pda/PdaBar.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/PatternPlayer.h"
#include "Utilities/Util.h"

namespace o2
{
namespace roc
{

class CruBar final : public BarInterfaceBase
{
  using Link = Cru::Link;

 public:
  CruBar(const Parameters& parameters, std::unique_ptr<RocPciDevice> rocPciDevice);
  CruBar(std::shared_ptr<Pda::PdaBar> bar);
  virtual ~CruBar();
  //virtual void checkReadSafe(int index) override;
  //virtual void checkWriteSafe(int index, uint32_t value) override;

  virtual CardType::type getCardType() override
  {
    return CardType::Cru;
  }

  virtual boost::optional<int32_t> getSerial() override;
  virtual boost::optional<float> getTemperature() override;
  virtual boost::optional<std::string> getFirmwareInfo() override;
  virtual boost::optional<std::string> getCardId() override;
  virtual uint32_t getDroppedPackets(int endpoint) override;
  virtual uint32_t getTotalPacketsPerSecond(int endpoint) override;
  virtual uint32_t getCTPClock() override;
  virtual uint32_t getLocalClock() override;
  virtual int32_t getLinks() override;
  virtual int32_t getLinksPerWrapper(int wrapper) override;
  virtual int getEndpointNumber() override;

  void pushSuperpageDescriptor(uint32_t link, uint32_t pages, uintptr_t busAddress);
  uint32_t getSuperpageCount(uint32_t link);
  uint32_t getSuperpageSize(uint32_t link);
  uint32_t getSuperpageFifoEmptyCounter(uint32_t link);
  void startDmaEngine();
  void stopDmaEngine();
  void resetDataGeneratorCounter();
  void resetCard();
  void dataGeneratorInjectError();
  void setDataSource(uint32_t source);
  FirmwareFeatures getFirmwareFeatures();

  static FirmwareFeatures convertToFirmwareFeatures(uint32_t reg);

  void resetInternalCounters();
  void setWrapperCount();
  void configure(bool force = false) override;
  Cru::ReportInfo report(bool forConfig = false);
  Cru::PacketMonitoringInfo monitorPackets();
  Cru::TriggerMonitoringInfo monitorTriggers(bool updateable = false);
  void emulateCtp(Cru::CtpInfo);
  void patternPlayer(PatternPlayer::Info patternPlayerInfo);

  void enableDataTaking();
  void disableDataTaking();

  void setDebugModeEnabled(bool enabled);
  bool getDebugModeEnabled();

  void toggleUserLogicLink(bool mUserLogicEnabled);
  void toggleRunStatsLink(bool mRunStatsLinkEnabled);
  boost::optional<std::string> getUserLogicVersion();

  Cru::OnuStatus reportOnuStatus();
  Cru::FecStatus reportFecStatus();

  void controlUserLogic(uint32_t eventSize, bool random);
  Cru::UserLogicInfo reportUserLogic();

  std::map<int, Cru::LoopbackStats> getGbtLoopbackStats(bool reset);

  std::map<int, Link> initializeLinkMap();

  std::shared_ptr<Pda::PdaBar> getPdaBar()
  {
    return mPdaBar;
  }

 private:
  boost::optional<int32_t> getSerialNumber();
  uint32_t getTemperatureRaw();
  boost::optional<float> convertTemperatureRaw(uint32_t registerValue);
  boost::optional<float> getTemperatureCelsius();
  uint32_t getFirmwareCompileInfo();
  uint32_t getFirmwareGitHash();
  uint32_t getFirmwareDateEpoch();
  uint32_t getFirmwareDate();
  uint32_t getFirmwareTime();
  uint32_t getFpgaChipHigh();
  uint32_t getFpgaChipLow();
  uint32_t getPonStatusRegister();
  uint32_t getOnuAddress();
  bool checkPonUpstreamStatusExpected(uint32_t ponUpstreamRegister, uint32_t onuAddress);
  bool checkClockConsistent(std::map<int, Link> linkMap);
  void populateLinkMap(std::map<int, Link>& linkMap);

  uint32_t getDdgBurstLength();
  void checkConfigParameters();

  void setCruId(uint16_t cruId);
  uint16_t getCruId();

  void setVirtualLinksIds(uint16_t systemId);

  uint16_t getTimeFrameLength();
  void setTimeFrameLength(uint16_t timeFrameLength);

  FirmwareFeatures parseFirmwareFeatures();
  FirmwareFeatures mFeatures;

  uint32_t mAllowRejection;
  Clock::type mClock;
  uint16_t mCruId;
  DatapathMode::type mDatapathMode;
  DownstreamData::type mDownstreamData;
  GbtMode::type mGbtMode;
  GbtMux::type mGbtMux;
  uint32_t mLoopback;
  int mWrapperCount = 0;
  std::set<uint32_t> mLinkMask;
  std::map<int, Link> mLinkMap;
  std::map<uint32_t, uint32_t> mRegisterMap;
  std::map<uint32_t, GbtMux::type> mGbtMuxMap;
  bool mPonUpstream;
  uint32_t mOnuAddress;
  bool mDynamicOffset;
  uint32_t mTriggerWindowSize;
  bool mGbtEnabled;
  bool mUserLogicEnabled;
  bool mRunStatsEnabled;
  bool mUserAndCommonLogicEnabled;
  uint32_t mSystemId;
  uint32_t mFeeId;
  std::map<uint32_t, uint32_t> mFeeIdMap;
  GbtPatternMode::type mGbtPatternMode;
  GbtCounterType::type mGbtCounterType;
  GbtStatsMode::type mGbtStatsMode;
  uint32_t mGbtHighMask;
  uint32_t mGbtMedMask;
  uint32_t mGbtLowMask;
  bool mGbtLoopbackReset;
  uint32_t mTimeFrameLength;

  int mSerial;
  int mEndpoint;

  /// Per-link counter to verify superpage sizes received are valid
  uint32_t mSuperpageSizeIndexCounter[Cru::MAX_LINKS] = { 0 };
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_CRUBAR_H_
