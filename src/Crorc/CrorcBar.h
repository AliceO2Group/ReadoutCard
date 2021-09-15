
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
/// \file CrorcBar.h
/// \brief Definition of the CrorcBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_SRC_CRORC_CRORCBAR_H_
#define O2_READOUTCARD_SRC_CRORC_CRORCBAR_H_

#include "BarInterfaceBase.h"
#include "Crorc/Common.h"
#include "Crorc/Constants.h"
#include "Utilities/Util.h"

namespace o2
{
namespace roc
{

class CrorcBar final : public BarInterfaceBase
{
 public:
  CrorcBar(const Parameters& parameters, std::unique_ptr<RocPciDevice> rocPciDevice);
  CrorcBar(std::shared_ptr<Pda::PdaBar> bar);
  virtual ~CrorcBar();
  //virtual void checkReadSafe(int index) override;
  //virtual void checkWriteSafe(int index, uint32_t value) override;

  virtual CardType::type getCardType() override
  {
    return CardType::Crorc;
  }

  virtual boost::optional<int32_t> getSerial() override;
  virtual boost::optional<std::string> getFirmwareInfo() override;
  virtual int getEndpointNumber() override;

  boost::optional<int32_t> getSerialNumber();
  void setSerial(int serial);

  void configure(bool force = false) override;
  Crorc::ReportInfo report(bool forConfig = false);

  void resetDevice(bool withSiu);
  void flushSuperpages();
  void startDataReceiver(uintptr_t readyFifoBusAddress);
  void stopDataReceiver();
  void startDataGenerator();
  void stopDataGenerator();
  void startTrigger(uint32_t command);
  void stopTrigger();
  bool isLinkUp(int barIndex);
  bool checkLinkUp();
  void assertLinkUp();
  void pushRxFreeFifo(uintptr_t blockAddress, uint32_t blockLength, uint32_t readyFifoIndex);
  void setLoopback();
  void setDiuLoopback();
  void setSiuLoopback();
  Crorc::PacketMonitoringInfo monitorPackets();

 private:
  std::map<int, Crorc::Link> initializeLinkMap();

  bool arch64()
  {
    return (sizeof(void*) > 4);
  }

  void setQsfpEnabled();
  bool getQsfpEnabled();
  void setDynamicOffsetEnabled(bool enabled);
  bool getDynamicOffsetEnabled();
  void setCrorcId(uint16_t crorcId);
  uint16_t getCrorcId();
  void setTimeFrameLength(uint16_t timeFrameLength);
  uint16_t getTimeFrameLength();
  void setTimeFrameDetectionEnabled(bool enabled);
  bool getTimeFrameDetectionEnabled();
  void getOpticalPowers(std::map<int, Crorc::Link>& linkMap);

  void resetSiu();
  void resetCard();
  void sendDdlCommand(uint32_t address, uint32_t command);

  uint16_t mCrorcId;
  bool mDynamicOffset;
  std::set<uint32_t> mLinkMask;
  uint16_t mTimeFrameLength;
  bool mTimeFrameDetectionEnabled;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_CRORC_CRORCBAR_H_
