
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
/// \file BarHammer.h
/// \brief Definition of the BarHammer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_CLI_BARHAMMER_H
#define O2_READOUTCARD_CLI_BARHAMMER_H

#include <atomic>
#include "Common/BasicThread.h"
#include "Cru/Constants.h"
#include "ReadoutCard/BarInterface.h"

namespace o2
{
namespace roc
{
namespace CommandLineUtilities
{

/// This class is for benchmarking the BAR. It "hammers" the BAR with repeated writes.
/// It will store the amount of writes since the start, which can be used to calculate "throughput".
class BarHammer : public AliceO2::Common::BasicThread
{
 public:
  void start(const std::shared_ptr<BarInterface>& channelIn)
  {
    mChannel = channelIn;
    BasicThread::start([&](std::atomic<bool>* stopFlag) {
      auto channel = mChannel;
      if (!channel) {
        return;
      }

      int64_t hammerCount = 0;
      uint32_t writeCounter = 0;
      while (!stopFlag->load(std::memory_order_relaxed)) {
        for (int i = 0; i < MULTIPLIER; ++i) {
          //channel->writeRegister(Cru::Registers::DEBUG_READ_WRITE.index, writeCounter);
          writeCounter++;
        }
        hammerCount++;
      }
      mHammerCount.store(hammerCount, std::memory_order_relaxed);
    });
  }

  double getCount()
  {
    return double(mHammerCount.load(std::memory_order_relaxed)) * double(MULTIPLIER);
  }

 private:
  std::shared_ptr<BarInterface> mChannel;
  std::atomic<int64_t> mHammerCount;
  static constexpr int64_t MULTIPLIER{ 10000 };
};

} // namespace CommandLineUtilities
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_CLI_BARHAMMER_H
