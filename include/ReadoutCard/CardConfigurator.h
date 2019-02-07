/// \file CardConfigurator.h
/// \brief Definition of the CardConfigurator class. 
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)


#ifndef ALICEO2_SRC_READOUTCARD_CARDCONFIGURATOR_H_
#define ALICEO2_SRC_READOUTCARD_CARDCONFIGURATOR_H_

#include "ReadoutCard/Parameters.h"

namespace AliceO2 {
namespace roc {

class CardConfigurator
{
  public:
    CardConfigurator(Parameters::CardIdType cardId, std::string pathToConfigFile, bool forceConfigure=false);
    CardConfigurator(Parameters& parameters, bool forceConfigure=false);

  private:
    void parseConfigFile(std::string pathToConfigFile, Parameters& parameters);
};

} // namespace AliceO2
} // namespace roc

#endif // ALICEO2_SRC_READOUTCARD_CARDCONFIGURATOR_H_
