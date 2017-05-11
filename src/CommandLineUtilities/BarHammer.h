/// \file BarHammer.h
/// \brief Definition of the BarHammer class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <atomic>
#include "Common/BasicThread.h"
#include "Cru/Constants.h"
#include "ReadoutCard/ChannelMasterInterface.h"

namespace AliceO2 {
namespace roc {
namespace CommandLineUtilities {

class BarHammer : public AliceO2::Common::BasicThread
{
  public:
    void start(const std::shared_ptr<ChannelMasterInterface>& channelIn)
    {
      mChannel = channelIn;
      BasicThread::start([&](std::atomic<bool>* stopFlag) {
        auto channel = mChannel;

        if (channel->getCardType() != CardType::Cru) {
          throw std::runtime_error("BarHammer only supported for CRU");
          return;
        }

        if (!channel) {
          return;
        }

        int64_t hammerCount = 0;
        uint32_t writeCounter = 0;
        while (!stopFlag->load(std::memory_order_relaxed)) {
          for (int i = 0; i < MULTIPLIER; ++i) {
            channel->writeRegister(Cru::Registers::DEBUG_READ_WRITE, writeCounter);
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
    std::shared_ptr<ChannelMasterInterface> mChannel;
    std::atomic<int64_t> mHammerCount;
    static constexpr int64_t MULTIPLIER {10000};
};

} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2
