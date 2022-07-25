
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
/// \file ProgramCleanup.cxx
/// \brief Utility that cleans up stale readout/readoutcard resources
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <pwd.h>
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"
#include "Pda/Util.h"
#include "ReadoutCard/CardFinder.h"
#include "ReadoutCard/InterprocessLock.h"

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
    std::cout << "2. Clean CRORC shared memory files under /dev/shm which match *_sp_info" << std::endl;
    std::cout << "3. Clean all hugepage resources under /var/lib/hugetlbfs/global/pagesize-{2MB, 1GB}/ which match readout* and o2-roc-bench-dma*" << std::endl;
    if (!mOptions.light) {
      std::cout << "4. Remove and reinsert the uio_pci_dma kernel module" << std::endl;
    }
    std::cout << std::endl;
    std::cout << "In case instances of readout.exe or o2-roc-bench-dma are running, roc-cleanup will fail." << std::endl;
    std::cout << std::endl;
    std::cout << "This tool is intended to be run with elevated privileges." << std::endl;
    std::cout << "Are you sure you want to continue? (yes/no)" << std::endl;
    std::string response;
    std::cin >> response;
    if (!algo::starts_with(response, "y")) {
      std::cout << "Terminated" << std::endl;
      return;
    }

    Logger::setFacility("ReadoutCard/cleanup");

    auto getUsername = []() -> std::string {
      struct passwd* passw = getpwuid(geteuid());
      if (passw) {
        return std::string(passw->pw_name);
      }
      return {};
    };

    Logger::get() << "`roc-cleanup` execution initiated by " << getUsername() << LogDebugOps_(4700) << endm;

    // Take and hold DMA locks during cleanup
    std::vector<std::unique_ptr<Interprocess::Lock>> dmaLocks;

    Logger::get() << "Grabbing PDA & DMA locks" << LogDebugDevel_(4701) << endm;
    try {
      auto cards = findCards();

      for (auto card : cards) {
        if (card.cardType == CardType::Cru) {
          std::string lockName("Alice_O2_RoC_DMA_" + card.pciAddress.toString() + "_lock");
          dmaLocks.push_back(std::make_unique<Interprocess::Lock>(lockName));
        } else if (card.cardType == CardType::Crorc) {
          for (int chan = 0; chan < 6; chan++) {
            std::string lockName("Alice_O2_RoC_DMA_" + card.pciAddress.toString() + "_chan" + std::to_string(chan) + "_lock");
            dmaLocks.push_back(std::make_unique<Interprocess::Lock>(lockName));
          }
        } // ignore an invalid card type
      }
    } catch (std::runtime_error& e) {
      Logger::get() << e.what() << LogErrorDevel_(4702) << endm;
      return;
    }

    Pda::freePdaDmaBuffers();

    Logger::get() << "Removing CRORC FIFO shared memory files" << LogDebugDevel_(4703) << endm;
    sysCheckRet("rm /dev/shm/*_sp_info");
    Logger::get() << "Removing readout 2MB hugepage mappings" << LogDebugDevel_(4704) << endm;
    sysCheckRet("rm /var/lib/hugetlbfs/global/pagesize-2MB/readout*");
    Logger::get() << "Removing readout 1GB hugepage mappings" << LogDebugDevel_(4705) << endm;
    sysCheckRet("rm /var/lib/hugetlbfs/global/pagesize-1GB/readout*");
    Logger::get() << "Removing o2-roc-bench-dma 2MB hugepage mappings" << LogDebugDevel_(4706) << endm;
    sysCheckRet("rm /var/lib/hugetlbfs/global/pagesize-2MB/roc-bench-dma*");
    Logger::get() << "Removing o2-roc-bench-dma 1GB hugepage mappings" << LogDebugDevel_(4707) << endm;
    sysCheckRet("rm /var/lib/hugetlbfs/global/pagesize-1GB/roc-bench-dma*");

    if (!mOptions.light) {
      Logger::get() << "Removing uio_pci_dma" << LogDebugDevel_(4708) << endm;
      sysCheckRet("modprobe -r uio_pci_dma");
      Logger::get() << "Reinserting uio_pci_dma" << LogDebugDevel_(4709) << endm;
      sysCheckRet("modprobe uio_pci_dma");
    }

    Logger::get() << "`roc-cleanup` execution finished" << LogDebugOps_(4710) << endm;
    return;
  }

 private:
  struct OptionsStruct {
    bool light = false;
  } mOptions;

  void sysCheckRet(const char* command)
  {
    int ret = system(command);
    if (ret && ret != 256) { // Don't log in case the file was not found
      Logger::get() << "Command: `" << command << "` failed with ret=" << ret << LogDebugDevel << endm;
    }
  }
};

int main(int argc, char** argv)
{
  return ProgramCleanup().execute(argc, argv);
}
