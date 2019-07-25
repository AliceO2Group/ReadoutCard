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
#include <iostream>
#include "ReadoutCard/ChannelFactory.h"
#include "CommandLineUtilities/Common.h"
#include "CommandLineUtilities/Options.h"
#include "CommandLineUtilities/Program.h"

using namespace AliceO2::roc::CommandLineUtilities;
using namespace AliceO2::roc;
namespace algo = boost::algorithm;
namespace po = boost::program_options;

class ProgramCleanup : public Program
{
 public:
  virtual Description getDescription()
  {
    return { "Cleanup", "Cleans up ReadoutCard state", "roc-cleanup" };
  }

  virtual void addOptions(po::options_description&)
  {
  }

  virtual void run(const po::variables_map&)
  {
    std::cout << "\033[1;31m"
              << "!!! WARNING !!!"
              << "\033[0m" << std::endl;
    std::cout << std::endl;
    std::cout << "Execution of this tool will:" << std::endl;
    std::cout << "1. Clean all hugepage resources under /var/lib/hugetlbfs/global/pagesize-{2MB, 1GB}/ which match readout* and roc-bench-dma*" << std::endl;
    std::cout << "2. Remove and reinsert the uio_pci_dma kernel module" << std::endl;
    std::cout << std::endl;
    std::cout << "In case instances of readout.exe or roc-bench-dma are running, they will fail." << std::endl;
    std::cout << std::endl;
    std::cout << "This tool is intended to be run with elevated privileges." << std::endl;
    std::cout << "Are you sure you want to continue? (yes/no)" << std::endl;
    std::string response;
    std::cin >> response;
    if (!algo::starts_with(response, "y")) {
      std::cout << "Terminated" << std::endl;
      return;
    }

    std::cout << "Removing readout 2MB hugepage mappings" << std::endl;
    system("rm /var/lib/hugetlbfs/global/pagesize-2MB/readout*");
    std::cout << "Removing readout 1GB hugepage mappings" << std::endl;
    system("rm /var/lib/hugetlbfs/global/pagesize-1GB/readout*");
    std::cout << "Removing roc-bench-dma 2MB hugepage mappings" << std::endl;
    system("rm /var/lib/hugetlbfs/global/pagesize-2MB/roc-bench-dma*");
    std::cout << "Removing roc-bench-dma 1GB hugepage mappings" << std::endl;
    system("rm /var/lib/hugetlbfs/global/pagesize-1GB/roc-bench-dma*");

    std::cout << "Removing uio_pci_dma" << std::endl;
    system("modprobe -r uio_pci_dma");
    std::cout << "Reinserting uio_pci_dma" << std::endl;
    system("modprobe uio_pci_dma");
  }

 private:
};

int main(int argc, char** argv)
{
  return ProgramCleanup().execute(argc, argv);
}
