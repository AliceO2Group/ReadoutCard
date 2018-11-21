/// \file Ttc.h
/// \brief Definition of the Ttc class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_TTC_H_
#define ALICEO2_READOUTCARD_CRU_TTC_H_

#include "Pda/PdaBar.h"

namespace AliceO2 {
namespace roc {

class Ttc {
  public: 
    Ttc(std::shared_ptr<Pda::PdaBar> pdaBar);

    void setClock(uint32_t clock, bool devkit=false); 
    void configurePlls(uint32_t clock);
    void setRefGen(uint32_t refGenId, int frequency=240);
    void resetFpll();
    void configurePonTx(uint32_t onuAddress);
    void calibrateTtc();
    void selectDownstreamData(uint32_t downstreamData);

  private:
    std::shared_ptr<Pda::PdaBar> mPdaBar;
};
} // namespace roc
} // namespace AliceO2


#endif // ALICEO2_READOUTCARD_CRU_TTC_H_
