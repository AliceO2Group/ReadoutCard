// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramFirmwareCheck.cxx
/// \brief Tool that checks the FW compatibility of the ReadoutCards.
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "ReadoutCard/FirmwareChecker.h"
#include "RocPciDevice.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
namespace po = boost::program_options;

class ProgramFirmwareCheck : public Program
{
 public:
  ProgramFirmwareCheck() : Program()
  {
  }

  virtual Description getDescription()
  {
    return { "Firmware Check", "Check firmware compatibility of the ReadoutCard(s)",
             "roc-fw-check\n" };
  }

  virtual void addOptions(boost::program_options::options_description&)
  {
  }

  virtual void run(const boost::program_options::variables_map&)
  {
    auto cardsFound = RocPciDevice::findSystemDevices();
    for (auto const& card : cardsFound) {
      auto params = Parameters::makeParameters(card.pciAddress, 2);
      FirmwareChecker().checkFirmwareCompatibility(params);
    }
    return;
  }
};

int main(int argc, char** argv)
{
  return ProgramFirmwareCheck().execute(argc, argv);
}
