// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file BarHammer.h
/// \brief Definition of the BarHammer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_BARHAMMER_H
#define ALICEO2_READOUTCARD_BARHAMMER_H

#include <atomic>
#include "Common/BasicThread.h"
#include "Cru/Constants.h"
#include "ReadoutCard/BarInterface.h"

namespace AliceO2
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
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_BARHAMMER_H
