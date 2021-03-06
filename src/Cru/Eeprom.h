/// \file Eeprom.h
/// \brief Definition of the Eeprom class
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef O2_READOUTCARD_CRU_EEPROM_H_
#define O2_READOUTCARD_CRU_EEPROM_H_

#include "Pda/PdaBar.h"

namespace o2
{
namespace roc
{

class Eeprom
{
 public:
  Eeprom(std::shared_ptr<Pda::PdaBar> pdaBar);

  boost::optional<int32_t> getSerial();

 private:
  std::string readContent();
  std::shared_ptr<Pda::PdaBar> mPdaBar;
};
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CRU_EEPROM_H_
