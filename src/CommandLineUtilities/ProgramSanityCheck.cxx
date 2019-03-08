// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramSanityCheck.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Utility that performs some basic sanity checks

#include <iostream>
#include <thread>
#include <chrono>
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/Exception.h"
#include <boost/format.hpp>
#include "CommandLineUtilities/Program.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using std::cout;
using std::endl;

namespace b = boost;

namespace {
class ProgramSanityCheck: public Program
{
  public:

    virtual Description getDescription()
    {
      return {"Sanity Check", "Does some basic sanity checks on the card",
          "roc-sanity-check --id=12345 --channel=0"};
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      Options::addOptionSerialNumber(options);
      Options::addOptionChannel(options);
      Options::addOptionCardId(options);
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      auto cardId = Options::getOptionCardId(map);
      const int channelNumber = Options::getOptionChannel(map);

      cout << "Warning: if the card is in a bad state, this program may result in a crash and reboot of the host\n";
      cout << "  To proceed, type 'y'\n";
      cout << "  To abort, type anything else or give SIGINT (usually Ctrl-c)\n";
      int c = getchar();
      if (c != 'y' || isSigInt()) {
        return;
      }

      auto params = AliceO2::roc::Parameters::makeParameters(cardId, channelNumber);
      //auto channel = AliceO2::roc::ChannelUtilityFactory().getUtility(params);
      //channel->utilitySanityCheck(cout);
    }
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramSanityCheck().execute(argc, argv);
}
