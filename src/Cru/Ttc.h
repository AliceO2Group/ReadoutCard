/// \file Ttc.h
/// \brief Definition of the Ttc class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_TTC_H_
#define ALICEO2_READOUTCARD_CRU_TTC_H_

#include "Pda/PdaBar.h"

namespace AliceO2
{
namespace roc
{

class Ttc
{
 public:
  Ttc(std::shared_ptr<Pda::PdaBar> pdaBar);

  void calibrateTtc();
  void setClock(uint32_t clock);
  void resetFpll();
  bool configurePonTx(uint32_t onuAddress);
  void selectDownstreamData(uint32_t downstreamData);
  uint32_t getPllClock();
  uint32_t getDownstreamData();

 private:
  void configurePlls(uint32_t clock);
  void setRefGen(int frequency = 240);

  std::shared_ptr<Pda::PdaBar> mPdaBar;
};
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_TTC_H_
