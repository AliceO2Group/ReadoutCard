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
#include "Swt/Swt.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
using namespace AliceO2::InfoLogger;
namespace po = boost::program_options;

constexpr uint32_t SWT_WR_WORD_L = 0x0f00040 / 4;

class ProgramBarStress: public Program
{
  public:

  virtual Description getDescription()
  {
    return {"Bar Stress", "Stress the Bar Accessor", 
      "roc-bar-stress --id 42:00.0 --cycles 100000 --print-freq 10000"};
  }

  virtual void addOptions(boost::program_options::options_description& options)
  {
    options.add_options()
      ("cycles",
       po::value<long long>(&mOptions.cycles)->default_value(100),
       "Total bar writes to perform")
      ("print-freq",
       po::value<long long>(&mOptions.printFrequency)->default_value(10),
       "Print every #print-freq cycles");
    Options::addOptionCardId(options);
  }

  int stress(BarInterface *bar, long long cycles, long long printFrequency) 
  {

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();

    for (long long i=0;; i++){
      bar->writeRegister(SWT_WR_WORD_L, 0x42);
      
      if (i && ((i%printFrequency == 0) || (i == cycles))){
        finish = std::chrono::high_resolution_clock::now();
        getLogger() << "loops [" << i-printFrequency+1 << " - " << i  << "]: " <<
          std::chrono::duration_cast<std::chrono::nanoseconds> (finish - start).count() << "ns" << InfoLogger::endm;

        if (i == cycles || isSigInt()){ //sigInt only stops at cycles = printFrequency * X
          double throughput = (printFrequency /(std::chrono::duration_cast<std::chrono::nanoseconds> (finish - start).count() * 1e-9)); //throughput (ops/time) [ops/sec]
          getLogger() << "Throughput :" << throughput << " ops/sec" << InfoLogger::endm;

          double latency = ((std::chrono::duration_cast<std::chrono::nanoseconds> (finish - start).count() * 1e-9) / printFrequency); // latency (time/ops) [sec]
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
    getLogger() << "Total BAR write operations: " << mOptions.cycles << InfoLogger::endm;
    getLogger() << "Print frequency: " << mOptions.printFrequency << InfoLogger::endm;

    std::shared_ptr<BarInterface>
      bar0 = ChannelFactory().getBar(cardId, 0);
    std::shared_ptr<BarInterface>
      bar2 = ChannelFactory().getBar(cardId, 2);  

    if(isVerbose())
      getLogger() << "Resetting card..." << InfoLogger::endm;
    bar0->writeRegister(Cru::Registers::RESET_CONTROL.index, 0x1);

    if(isVerbose())
      getLogger() << "Running operations..." << InfoLogger::endm;
  
    auto start = std::chrono::high_resolution_clock::now();
    long long cycles_run = stress(bar2.get(), mOptions.cycles, mOptions.printFrequency);
    auto finish = std::chrono::high_resolution_clock::now();
    
    if (!cycles_run)
      getLogger() << "Execution terminated because of error..." << InfoLogger::endm;
    
    getLogger() << "Total duration: " << std::chrono::duration_cast<std::chrono::seconds> (finish-start).count() << "s" << InfoLogger::endm;
    getLogger() << "Total bar operations: " << cycles_run << InfoLogger::endm;
  }

  struct OptionsStruct 
  {
    long long cycles = 100;
    long long printFrequency = 10;
  }mOptions;

  private:
};

int main(int argc, char** argv)
{
  return ProgramBarStress().execute(argc, argv);
}
