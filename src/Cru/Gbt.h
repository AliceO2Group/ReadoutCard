// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Gbt.h
/// \brief Definition of the Gbt class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_GBT_H_
#define ALICEO2_READOUTCARD_CRU_GBT_H_

#include "Pda/PdaBar.h"
#include "Common.h"
#include "Constants.h"
#include "I2c.h"

namespace AliceO2 {
namespace roc {

class Gbt {
  
  using Link = Cru::Link;
  using LinkStatus = Cru::LinkStatus;

  public: 
    //Gbt(std::shared_ptr<Pda::PdaBar> pdaBar, std::vector<Link> &mLinkList, int wrapperCount);
    Gbt(std::shared_ptr<Pda::PdaBar> pdaBar, std::map<int, Link> &mLinkMap, int wrapperCount);
    void setMux(int link, uint32_t mux);
    void setInternalDataGenerator(Link link, uint32_t value);
    void setTxMode(Link link, uint32_t mode);
    void setRxMode(Link link, uint32_t mode);
    void setLoopback(Link link, uint32_t enabled);
    void calibrateGbt();
    void getGbtModes();
    void getGbtMuxes();
    void getLoopbacks();
    LinkStatus getStickyBit(Link link);
    uint32_t getRxClockFrequency(Link link);
    uint32_t getTxClockFrequency(Link link);

  private:
    uint32_t getSourceSelectAddress(Link link);
    uint32_t getStatusAddress(Link link);
    uint32_t getClearErrorAddress(Link link);
    uint32_t getRxControlAddress(Link link);
    uint32_t getTxControlAddress(Link link);
    uint32_t getAtxPllRegisterAddress(int wrapper, uint32_t reg);

    void atxcal(uint32_t baseAddress=0x0);
    void cdrref(uint32_t refClock);
    void txcal();
    void rxcal();

    void resetStickyBit(Link link);

    std::shared_ptr<Pda::PdaBar> mPdaBar;
    std::map<int, Link> &mLinkMap;
    int mWrapperCount;
};
} // namespace roc
} // namespace AliceO2


#endif // ALICEO2_READOUTCARD_CRU_GBT_H_
