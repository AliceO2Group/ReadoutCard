/// \file Cru/Common.h
/// \brief Definition of common CRU utilities
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_COMMON_H_
#define ALICEO2_READOUTCARD_CRU_COMMON_H_

#include "Constants.h"
#include "Pda/PdaBar.h"

#include <iostream>

namespace AliceO2 {
namespace roc {
namespace Cru {

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

  bool operator== (const Link &dlink) const {
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
      enabled == dlink.enabled);
    }
  }
};

struct ReportInfo {
  std::vector<Link> linkList;
  uint32_t ttcClock;
  uint32_t downstreamData;
};

uint32_t getWrapperBaseAddress(int wrapper);
uint32_t getXcvrRegisterAddress(int wrapper, int bank, int link, int reg=0);
void atxcal0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress);
void txcal0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress);
void rxcal0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress);
void fpllref0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress, uint32_t refClock);
void fpllcal0(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t baseAddress, bool configCompensation=true);
uint32_t waitForBit(std::shared_ptr<Pda::PdaBar> pdaBar, uint32_t address, uint32_t position, uint32_t value);

} // namespace Cru
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_COMMON_H_
