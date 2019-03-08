// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

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
