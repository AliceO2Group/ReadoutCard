// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramSwtStress.cxx
/// \brief Tool that stress the Bar Accessor with SWT transactions
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
#include "Swt/Swt.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

class ProgramSwtStress: public Program
{
  public:

  virtual Description getDescription()
  {
    return {"Swt Stress", "Stress the Bar Accessor with SWT transactions", 
      "roc-swt-stress --id 42:00.0 --gbt-link 0 --cycles 100000 --print-freq 10000 --errorcheck"};
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()
      ("gbt-link",
       po::value<uint32_t>(&mOptions.gbtLink)->default_value(0),
       "GBT link over which the bar writes will be performed. CRU is 0-17")
      ("cycles",
       po::value<long long>(&mOptions.cycles)->default_value(100),
       "Cycles of SWT writes(/reads) to perform")
      ("print-freq",
       po::value<long long>(&mOptions.printFrequency)->default_value(10),
       "Print every #print-freq cycles")
      ("errorcheck",
       po::bool_switch(&mOptions.errorCheck),
       "Perform data validation");
    Options::addOptionCardId(options);
  }

  int stress(Swt *swt, long long cycles, long long printFrequency, bool errorCheck) 
  {

    SwtWord swtWordWr = SwtWord(0x0, 0x0, 0x0);
    SwtWord swtWordRd = SwtWord(0x0, 0x0, 0x0);

    /* swtWordCheck for testing */
    // SwtWord swtWordCheck = SwtWord(0x42, 0x42, 0x42);

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();

    for (long long i=0;; i++){
      swtWordWr.setLow((0x1 + i)%0xffffffff);
      swtWordWr.setMed((0x2 + i)%0xffffffff); 
      swtWordWr.setHigh((0x3 + i)%0xffff);
      
      uint32_t mon = swt->write(swtWordWr);
      if (isVerbose())
        getLogger() << "WR MON: 0x" << std::setfill('0') << std::hex << mon << InfoLogger::endm;
     
      if (errorCheck){
        uint32_t mon = swt->read(swtWordRd);
        if (swtWordRd != swtWordWr){
          getLogger() << "SWT validation failed" << InfoLogger::endm;
          getLogger() << "Read: " << swtWordRd << " | Expected: " << swtWordWr << InfoLogger::endm;
          return -1;
        }
        
        if (isVerbose()){
          getLogger() << "RD MON: 0x" << std::setfill('0') << std::hex << mon << InfoLogger::endm;
          getLogger() << "Read swtWord: " << swtWordRd << InfoLogger::endm;
        }
      }

      if (i && ((i%printFrequency == 0) || (i == cycles))){
        finish = std::chrono::high_resolution_clock::now();
        getLogger() << "loops [" << i-printFrequency+1 << " - " << i  << "]: " <<
          std::chrono::duration_cast<std::chrono::nanoseconds> (finish - start).count() << "ns" << InfoLogger::endm;

        if (i == cycles || isSigInt()){ //sigInt only stops at cycles = printFrequency * X
          double throughput = (barOps /(std::chrono::duration_cast<std::chrono::nanoseconds> (finish - start).count() * 1e-9)); //throughput (ops/time) [ops/sec]
          getLogger() << "Throughput :" << throughput << " ops/sec" << InfoLogger::endm;

          double latency = ((std::chrono::duration_cast<std::chrono::nanoseconds> (finish - start).count() * 1e-9) / barOps); // latency (time/ops) [sec]
          getLogger() << "Operation latency: " << latency << " sec " << InfoLogger::endm;
          return i;
        }

        start = std::chrono::high_resolution_clock::now();
      }
    }
  }

  virtual void run(const boost::program_options::variables_map& map) 
  {

    auto cardId = Options::getOptionCardId(map);

    getLogger() << "Card ID: " << cardId << InfoLogger::endm;
    getLogger() << "GBT Link: " << mOptions.gbtLink << InfoLogger::endm;
    getLogger() << "Cycles of SWT write(/read) operations: " << mOptions.cycles << InfoLogger::endm;
    getLogger() << "Print frequency: " << mOptions.printFrequency << InfoLogger::endm;
    getLogger() << "Error Check enabled: " << mOptions.errorCheck << InfoLogger::endm;

    /* bar(Writes | Reads | Ops) every #printFrequency cycles */
    barWrites = (SWT_WRITE_BAR_WRITES + SWT_READ_BAR_WRITES*mOptions.errorCheck)*mOptions.printFrequency;
    barReads = (SWT_WRITE_BAR_READS + SWT_READ_BAR_READS*mOptions.errorCheck)*mOptions.printFrequency;
    barOps = barWrites + barReads;

    getLogger() << "Logging time every " << barOps << " bar operations, of which:" << InfoLogger::endm;
    getLogger() << "barWrites: " << barWrites << " | barReads: " << barReads << InfoLogger::endm;
      
    std::shared_ptr<BarInterface>
      bar0 = ChannelFactory().getBar(cardId, 0);
    std::shared_ptr<BarInterface>
      bar2 = ChannelFactory().getBar(cardId, 2);  

    if(isVerbose())
      getLogger() << "Resetting card..." << InfoLogger::endm;
    bar0->writeRegister(Cru::Registers::RESET_CONTROL.index, 0x1);

    if(isVerbose())
      getLogger() << "Initializing SWT..." << InfoLogger::endm;
    auto swt = Swt(*bar2, mOptions.gbtLink);

    if(isVerbose())
      getLogger() << "Running operations..." << InfoLogger::endm;
  
    auto start = std::chrono::high_resolution_clock::now();
    long long cycles_run = stress(&swt, mOptions.cycles, mOptions.printFrequency, mOptions.errorCheck);
    auto finish = std::chrono::high_resolution_clock::now();
    
    if (!cycles_run)
      getLogger() << "Execution terminated because of error..." << InfoLogger::endm;
    
    getLogger() << "Total duration: " << std::chrono::duration_cast<std::chrono::seconds> (finish-start).count() << "s" << InfoLogger::endm;
    getLogger() << "Total bar operations: " << barOps * (cycles_run/mOptions.printFrequency) << InfoLogger::endm;
    getLogger() << "Total bar writes " << barWrites * (cycles_run/mOptions.printFrequency) << InfoLogger::endm;
    getLogger() << "Total bar reads: " << barReads * (cycles_run/mOptions.printFrequency) << InfoLogger::endm;
  }

  struct OptionsStruct 
  {
    uint32_t gbtLink = 0;
    long long cycles = 100;
    long long printFrequency = 10;
    bool errorCheck = true;
  }mOptions;

  long long barOps = 0;
  long long barWrites = 0;
  long long barReads = 0;

  private:
};

int main(int argc, char** argv)
{
  return ProgramSwtStress().execute(argc, argv);
}
