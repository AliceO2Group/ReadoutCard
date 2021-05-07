// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ProgramCleanup.cxx
/// \brief Utility that cleans up stale readout/readoutcard resources
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Pda/Util.h"
#include "ReadoutCard/CardDescriptor.h"

using namespace o2::roc::CommandLineUtilities;
using namespace o2::roc;
namespace algo = boost::algorithm;
namespace po = boost::program_options;
namespace bfs = boost::filesystem;

class ProgramCleanup : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Cleanup", "Cleans up ReadoutCard state", "o2-roc-cleanup" };
  }

  virtual void addOptions(po::options_description& options)
  {
    options.add_options()("light",
                          po::bool_switch(&mOptions.light)->default_value(false),
                          "Flag to run a \"light\" o2-roc-cleanup, skipping PDA module removal");
  }

  virtual void run(const po::variables_map&)
  {
    std::cout << "\033[1;31m"
              << "!!! WARNING !!!"
              << "\033[0m" << std::endl;
    std::cout << std::endl;
    std::cout << "Execution of this tool will:" << std::endl;
    std::cout << "1. Free PDA DMA buffers" << std::endl;
    std::cout << "2. Clean CRORC FIFO shared memory files under /dev/shm which match AliceO2_ROC_*" << std::endl;
    std::cout << "3. Clean all hugepage resources under /var/lib/hugetlbfs/global/pagesize-{2MB, 1GB}/ which match readout* and o2-roc-bench-dma*" << std::endl;
    if (!mOptions.light) {
      std::cout << "4. Remove and reinsert the uio_pci_dma kernel module" << std::endl;
    }
    std::cout << std::endl;
    std::cout << "In case instances of readout.exe or o2-roc-bench-dma are running, they will fail." << std::endl;
    std::cout << std::endl;
    std::cout << "This tool is intended to be run with elevated privileges." << std::endl;
    std::cout << "Are you sure you want to continue? (yes/no)" << std::endl;
    std::string response;
    std::cin >> response;
    if (!algo::starts_with(response, "y")) {
      std::cout << "Terminated" << std::endl;
      return;
    }

    Pda::freePdaDmaBuffers();

    std::cout << "Removing CRORC FIFO shared memory files" << std::endl;
    sysCheckRet("rm /dev/shm/AliceO2_RoC_*");
    std::cout << "Removing readout 2MB hugepage mappings" << std::endl;
    sysCheckRet("rm /var/lib/hugetlbfs/global/pagesize-2MB/readout*");
    std::cout << "Removing readout 1GB hugepage mappings" << std::endl;
    sysCheckRet("rm /var/lib/hugetlbfs/global/pagesize-1GB/readout*");
    std::cout << "Removing o2-roc-bench-dma 2MB hugepage mappings" << std::endl;
    sysCheckRet("rm /var/lib/hugetlbfs/global/pagesize-2MB/roc-bench-dma*");
    std::cout << "Removing o2-roc-bench-dma 1GB hugepage mappings" << std::endl;
    sysCheckRet("rm /var/lib/hugetlbfs/global/pagesize-1GB/roc-bench-dma*");

    if (!mOptions.light) {
      std::cout << "Removing uio_pci_dma" << std::endl;
      sysCheckRet("modprobe -r uio_pci_dma");
      std::cout << "Reinserting uio_pci_dma" << std::endl;
      sysCheckRet("modprobe uio_pci_dma");
    }
  }

 private:
  struct OptionsStruct {
    bool light = false;
  } mOptions;

  void sysCheckRet(const char* command)
  {
    int ret = system(command);
    if (ret) {
      std::cerr << "Command: `" << command << "` failed with ret=" << ret << std::endl;
    }
  }
};

int main(int argc, char** argv)
{
  return ProgramCleanup().execute(argc, argv);
}
