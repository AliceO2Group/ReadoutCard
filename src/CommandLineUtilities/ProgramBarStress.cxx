
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file ProgramBarStress.cxx
/// \brief Tool that stress the Bar Accessor
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <chrono>
#include <thread>
#include <cstddef>
#include "Cru/Constants.h"
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"

using namespace o2::roc::CommandLineUtilities;
using namespace o2::roc;
namespace po = boost::program_options;

class ProgramBarStress : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Bar Stress", "Stress the Bar Accessor",
             "o2-roc-bar-stress --id 04:00.0 --channel=1 --address=0x0f00040 --value=0x18 \n"
             "\t--cycles 100000 --print-freq 10000 --sleep=1000" };
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()("cycles",
                          po::value<long long>(&mOptions.cycles)->default_value(100),
                          "Total bar writes to perform");
    Options::addOptionCardId(options);
    Options::addOptionRegisterAddress(options);
    Options::addOptionRegisterValue(options);
    Options::addOptionChannel(options);
  }

  int stress(BarInterface* bar, uint32_t address, uint32_t value, long long cycles)
  {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();

    for (long long i = 0;; i++) {
      bar->writeRegister(address / 4, value);
      bar->readRegister(address / 4);

      if (i == cycles || isSigInt()) {
        finish = std::chrono::high_resolution_clock::now();
        std::cout << "cyc: " << i * 2;
        std::cout << "time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count() * 1e-9 << std::endl;
        double throughput = ((i * 2) / (std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count() * 1e-9)); //throughput (ops/time) [ops/sec]
        std::cout << "Throughput: " << throughput << " ops/sec" << std::endl;
        std::cout << "Throughput: " << throughput * 32 / 1e6 << " Mbps/sec" << std::endl;

        double latency = ((std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count() * 1e-9) / (cycles * 2)); // latency (time/ops) [sec]
        std::cout << "Operation latency: " << latency << " sec " << std::endl;
        return i;
      }
    }
  }

  virtual void run(const boost::program_options::variables_map& map)
  {

    auto cardId = Options::getOptionCardId(map);
    int channelNumber = Options::getOptionChannel(map);
    uint32_t registerAddress = Options::getOptionRegisterAddress(map);
    uint32_t registerValue = Options::getOptionRegisterValue(map);
    auto params = o2::roc::Parameters::makeParameters(cardId, channelNumber);
    auto bar = o2::roc::ChannelFactory().getBar(params);

    std::cout << "Card ID: " << cardId << std::endl;
    std::cout << "BAR: " << channelNumber << std::endl;
    std::cout << "Total cycles (rd + wr): " << mOptions.cycles << std::endl;
    std::cout << "Total BAR operations: " << mOptions.cycles * 2 << std::endl;

    std::cout << std::endl
              << "Running operations..." << std::endl
              << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    long long cycles_run = stress(bar.get(), registerAddress, registerValue,
                                  mOptions.cycles);
    auto finish = std::chrono::high_resolution_clock::now();

    if (!cycles_run)
      std::cout << "Execution terminated because of error..." << std::endl;

    std::cout << "Total duration: " << std::chrono::duration_cast<std::chrono::seconds>(finish - start).count() << "s" << std::endl;
    std::cout << "Total BAR operations: " << cycles_run * 2 << std::endl;
  }

  struct OptionsStruct {
    long long cycles = 100;
    long long printFrequency = 10;
    int sleep = 0;
  } mOptions;

 private:
};

int main(int argc, char** argv)
{
  return ProgramBarStress().execute(argc, argv);
}
