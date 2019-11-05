/// \file PatternPlayer.h
/// \brief Definition of the PatternPlayer class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_PATTERNPLAYER_H_
#define ALICEO2_READOUTCARD_CRU_PATTERNPLAYER_H_

#include "Pda/PdaBar.h"
#include <boost/multiprecision/cpp_int.hpp>

using namespace boost::multiprecision;

namespace AliceO2
{
namespace roc
{

class PatternPlayer
{
 public:
  PatternPlayer(std::shared_ptr<Pda::PdaBar> pdaBar);

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

 private:
  std::shared_ptr<Pda::PdaBar> mPdaBar;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_PATTERNPLAYER_H_
