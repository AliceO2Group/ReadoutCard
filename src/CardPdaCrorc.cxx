///
/// \file CardPdaCrorc.cxx
/// \author Pascal Boeschoten
///

#include "CardPdaCrorc.h"
#include <iostream>
#include <iomanip>
#include <bitset>
#include <boost/lexical_cast.hpp>
#include "c/interface/header.h"
#include "c/rorc/rorc.h"
#include "RorcException.h"

using std::cout;
using std::endl;

namespace AliceO2 {
namespace Rorc {

constexpr int CRORC_NUMBER_OF_CHANNELS = 6;

CardPdaCrorc::CardPdaCrorc(DeviceOperator* deviceOperator, PciDevice* pciDevice, int serialNumber)
    : CardPdaBase(deviceOperator, pciDevice, serialNumber, CRORC_NUMBER_OF_CHANNELS), loopPerUsec(0), pciLoopPerUsec(0)
{
}

CardPdaCrorc::~CardPdaCrorc()
{
}

void CardPdaCrorc::validateChannelParameters(const ChannelParameters& ps)
{
  if (ps.dma.bufferSize % (2 * 1024 * 1024) != 0) {
    ALICEO2_RORC_THROW_EXCEPTION("Parameter 'dma.bufferSize' not a multiple of 2 mebibytes");
  }

  if (ps.generator.dataSize > ps.dma.pageSize) {
    ALICEO2_RORC_THROW_EXCEPTION("Parameter 'generator.dataSize' greater than 'dma.pageSize'");
  }

  if ((ps.dma.bufferSize % ps.dma.pageSize) != 0) {
    ALICEO2_RORC_THROW_EXCEPTION("DMA buffer size not a multiple of 'dma.pageSize'");
  }

  // TODO Implement more checks

//  if (ps.generator.seed ???) {
//    THROW_RUNTIME_ERROR("Parameter 'generator.seed' invalid");
//  }
}

void CardPdaCrorc::deviceOpenDmaChannel(int channel)
{
  auto& cd = getChannelData(channel);

  // Find DIU version, required for armDdl()
  ddlFindDiuVersion(cd.bar.getUserspaceAddressU32());
}

void CardPdaCrorc::deviceCloseDmaChannel(int channel)
{
}

void CardPdaCrorc::startDma(int channel)
{
  auto& cd = getChannelData(channel);

  // For random sleep time after every received data block.
  if(SLEEP_TIME || LOAD_TIME){
    srand(time(NULL));
  }

  // Initializing the software FIFO
  initializeReadyFifo(cd);

  // Resetting the card,according to the RESET LEVEL parameter
  resetCard(channel, cd.params().initialResetLevel);

  // Setting the card to be able to receive data
  startDataReceiving(cd);

  // Initializing the firmware FIFO, pushing (entries) pages
  initializeFreeFifo(cd, cd.params().fifo.entries);

//  cout << "SGL Lengths:" << endl;
//  for (int i = 0; i < cd.sglWrapper->nodes.size(); ++i) {
//    cout << "  " << i << " -> " << std::setw(5) << cd.sglWrapper->nodes[i]->length / (1024 * 1024) << " MiB" << endl;
//  }

  if (cd.params().generator.useDataGenerator) {
    // Initializing the data generator according to the LOOPBACK parameter
    armDataGenerator(cd, cd.params().generator);

    // Starting the data generator
    startDataGenerator(cd, cd.params().generator.maximumEvents);
  } else {
    if (!cd.params().noRDYRX) {

      uint64_t timeout = pciLoopPerUsec * DDL_RESPONSE_TIME;
      uint64_t respCycle = pciLoopPerUsec * DDL_RESPONSE_TIME;
      auto userAddress = cd.bar.getUserspaceAddressU32();

      // Clearing SIU/DIU status.
      if (rorcCheckLink(userAddress) != RORC_STATUS_OK) {
        printf("SIU not seen. Can not clear SIU status.\n");
      } else {
        if (ddlReadSiu(userAddress, 0, timeout) != -1) {
          printf("SIU status cleared.\n");
        }
      }

      if (ddlReadDiu(userAddress, 0, timeout) != -1) {
        printf("DIU status cleared.\n");
      }

      // RDYRX command to FEE
      stword_t stw;
      int returnCode = rorcStartTrigger(userAddress, respCycle, &stw);

      if (returnCode == RORC_LINK_NOT_ON) {
        ALICEO2_RORC_THROW_EXCEPTION("Error: LINK IS DOWN, RDYRX command can not be sent");
      }

      if (returnCode == RORC_STATUS_ERROR) {
        ALICEO2_RORC_THROW_EXCEPTION("Error: RDYRX command can not be sent");
      }

      if (returnCode == RORC_NOT_ACCEPTED) {
        ALICEO2_RORC_THROW_EXCEPTION(" No reply arrived for RDYRX in timeout"); // %lld usec\n", (unsigned long long) DDL_RESPONSE_TIME);
      } else {
        printf(" FEE accepted the RDYDX command. Its reply: 0x%08lx\n", stw.stw);
      }
    }
  }
}

int rorcStopDataGenerator(volatile uint32_t* buff)
{
  rorcWriteReg(buff, C_CSR, DRORC_CMD_STOP_DG);
  return (RORC_STATUS_OK);
}

void CardPdaCrorc::stopDma(int channel)
{
  auto& cd = getChannelData(channel);
  auto userAddress = cd.bar.getUserspaceAddressU32();

  // Stopping receiving data
  if (cd.params().generator.useDataGenerator) {
    rorcStopDataGenerator(userAddress);
    rorcStopDataReceiver(userAddress);
  } else if(!cd.params().noRDYRX) {
    // Sending EOBTR to FEE.
    stword_t stw;
    uint64_t timeout = pciLoopPerUsec * DDL_RESPONSE_TIME;
    int returnCode = rorcStopTrigger(userAddress, timeout, &stw);

    if (returnCode == RORC_LINK_NOT_ON) {
      ALICEO2_RORC_THROW_EXCEPTION("Error: LINK IS DOWN, EOBTR command can not be sent");
    }

    if (returnCode == RORC_STATUS_ERROR) {
      ALICEO2_RORC_THROW_EXCEPTION("Error: EOBTR command can not be sent");
    }

    printf(" EOBTR command sent to the FEE\n");

    if (returnCode != RORC_NOT_ACCEPTED) {
      printf(" FEE accepted the EOBTR command. Its reply: 0x%08lx\n", stw.stw);
    }
  }
}

PageHandle CardPdaCrorc::pushNextPage(int channel)
{
  auto& cd = getChannelData(channel);
  auto handle = PageHandle(cd.fifo->getWriteIndex());

  // Check if page is available to write to
  if (cd.pageWasReadOut[handle.index] == false) {
    ALICEO2_RORC_THROW_EXCEPTION("Pushing page would overwrite");
  }

  cd.pageWasReadOut[handle.index] = false;
  pushFreeFifoPage(cd, handle.index);
  cd.fifo->advanceWriteIndex();

  return handle;
}

void handleRangeCheck(CardPdaBase::ChannelData& cd, const PageHandle& handle)
{
  if (handle.index < 0 || handle.index > cd.params().fifo.entries) {
    ALICEO2_RORC_THROW_EXCEPTION("PageHandle index out of range");
  }
}

bool CardPdaCrorc::isPageArrived(int channel, const PageHandle& handle)
{
  auto& cd = getChannelData(channel);
  handleRangeCheck(cd, handle);
  return dataArrived(cd, handle.index) != DataArrivalStatus::NONE_ARRIVED;
}

Page CardPdaCrorc::getPage(int channel, const PageHandle& handle)
{
  auto& cd = getChannelData(channel);
  handleRangeCheck(cd, handle);
  return Page(cd.sglWrapper->pages[handle.index].userAddress);
}

void CardPdaCrorc::markPageAsRead(int channel, const PageHandle& handle)
{
  auto& cd = getChannelData(channel);
  handleRangeCheck(cd, handle);

  if (cd.pageWasReadOut[handle.index]) {
    ALICEO2_RORC_THROW_EXCEPTION("Page was already marked as read");
  }

  cd.pageWasReadOut[handle.index] = true;
}

uint32_t CardPdaCrorc::readRegister(int channel, int index)
{
  return getChannelData(channel).bar[index];
}

void CardPdaCrorc::writeRegister(int channel, int index, uint32_t value)
{
  getChannelData(channel).bar[index] = value;
}

void CardPdaCrorc::armDdl(ChannelData& cd, int resetMask)
{
  if (rorcArmDDL(cd.bar.getUserspaceAddressU32(), resetMask) != RORC_STATUS_OK) {
    std::stringstream ss;
    ss << "Failed to reset channel " << cd.channel << " using reset mask 0x"
      << std::hex << std::setw(4) << std::setfill('0') << uint16_t(resetMask >> 16) << "." 
      << std::hex << std::setw(4) << std::setfill('0') << uint16_t(resetMask);
    ALICEO2_RORC_THROW_EXCEPTION(ss.str());
  }
}

int CardPdaCrorc::getNumberOfChannels()
{
  return CRORC_NUMBER_OF_CHANNELS;
}

void CardPdaCrorc::resetCard(int channel, ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::NOTHING) {
    return;
  }

  auto& cd = getChannelData(channel);
  auto loopbackMode = cd.params().generator.loopbackMode;

  if (resetLevel == ResetLevel::RORC_ONLY) {
    armDdl(cd, RORC_RESET_RORC);
  }

  if (LoopbackMode::isExternal(loopbackMode)) {
    armDdl(cd, RORC_RESET_DIU);

    if ((resetLevel == ResetLevel::RORC_DIU_SIU) && (loopbackMode != LoopbackMode::EXTERNAL_DIU))
    {
      // Wait a little before SIU reset.
      usleep(100000);
      // Reset SIU.
      armDdl(cd, RORC_RESET_SIU);
      armDdl(cd, RORC_RESET_DIU);
    }

    armDdl(cd, RORC_RESET_RORC);
  }

  // Wait a little after reset.
  usleep(100000);
}

volatile void* CardPdaCrorc::getDataStartAddress(ChannelData& cd)
{
  return cd.sglWrapper->nodes[0]->u_pointer + cd.params().fifo.getFullOffset();
}

void CardPdaCrorc::initializeReadyFifo(ChannelData& cd)
{
  auto userAddress = (cd.sglWrapper->nodes[0]->u_pointer + cd.params().fifo.softwareOffset);
  auto deviceAddress = (cd.sglWrapper->nodes[0]->d_pointer + cd.params().fifo.softwareOffset);
  cd.fifo.reset(new ReadyFifoWrapper(userAddress, deviceAddress, cd.params().fifo.entries));
  cd.fifo->resetAll();
}

void CardPdaCrorc::initializeFreeFifo(ChannelData& cd, int pagesToPush)
{
  // Pushing a given number of pages to the firmware FIFO.
  for(int i = 0; i < pagesToPush; ++i){
    pushFreeFifoPage(cd, i);
  }
}

void CardPdaCrorc::pushFreeFifoPage(ChannelData& cd, int fifoIndex)
{
  auto pageAddress = reinterpret_cast<uint64_t>(cd.sglWrapper->pages[fifoIndex].busAddress);
  rorcPushRxFreeFifo(cd.bar.getUserspaceAddress(), pageAddress, (cd.params().dma.pageSize / 4), fifoIndex);
}

CardPdaCrorc::DataArrivalStatus::type CardPdaCrorc::dataArrived(ChannelData& cd, int index)
{
  auto status = cd.fifo->getEntry(index).status;

  if (status == -1) {
    return DataArrivalStatus::NONE_ARRIVED;
  } else if (status == 0) {
    return DataArrivalStatus::PART_ARRIVED;
  } else {
    return DataArrivalStatus::WHOLE_ARRIVED;
  }
}

void CardPdaCrorc::startDataReceiving(ChannelData& cd)
{
  auto barAddress = cd.bar.getUserspaceAddressU32();

  setLoopPerSec(&loopPerUsec, &pciLoopPerUsec, barAddress);
  ddlFindDiuVersion(barAddress);

  // Preparing the card.
  if (LoopbackMode::EXTERNAL_SIU == cd.params().generator.loopbackMode) {
    resetCard(cd.channel, ResetLevel::RORC_DIU_SIU);

    if (rorcCheckLink(barAddress) != RORC_STATUS_OK) {
      ALICEO2_RORC_THROW_EXCEPTION("SIU not seen. Can not clear SIU status");
    } else {
      if (ddlReadSiu(barAddress, 0, DDL_RESPONSE_TIME) == -1) {
        ALICEO2_RORC_THROW_EXCEPTION("SIU read error");
      }
    }
    if (ddlReadDiu(barAddress, 0, DDL_RESPONSE_TIME) == -1) {
      ALICEO2_RORC_THROW_EXCEPTION("DIU read error");
    }
  }

  rorcReset(barAddress, RORC_RESET_FF);

  // Checking if firmware FIFO is empty.
  if (rorcCheckRxFreeFifo(barAddress) != RORC_FF_EMPTY){
    ALICEO2_RORC_THROW_EXCEPTION("Firmware FIFO is not empty");
  }

  rorcStartDataReceiver(barAddress, reinterpret_cast<unsigned long>(cd.fifo->getDeviceAddress()));
}

void CardPdaCrorc::armDataGenerator(ChannelData& cd, const GeneratorParameters& gen)
{
  auto barAddress = cd.bar.getUserspaceAddress();
  int ret, rounded_len;
  stword_t stw;

  if (LoopbackMode::NONE == gen.loopbackMode) {
    ret = rorcStartTrigger(barAddress, DDL_RESPONSE_TIME * pciLoopPerUsec, &stw);

    if (ret == RORC_LINK_NOT_ON) {
      ALICEO2_RORC_THROW_EXCEPTION("Error: LINK IS DOWN, RDYRX command could not be sent");
    }
    if (ret == RORC_STATUS_ERROR) {
      ALICEO2_RORC_THROW_EXCEPTION("Error: RDYRX command could not be sent");
    }
    if (ret == RORC_NOT_ACCEPTED) {
      ALICEO2_RORC_THROW_EXCEPTION("No reply arrived for RDYRX within timeout"); // %d usec\n", DDL_RESPONSE_TIME);
    } else {
      printf("FEE accepted the RDYDX command. Its reply: 0x%08lx\n\n", stw.stw);
    }
  }

  if (rorcArmDataGenerator(barAddress, gen.initialValue, gen.initialWord, gen.pattern, gen.dataSize / 4, gen.seed,
      &rounded_len) != RORC_STATUS_OK) {
    ALICEO2_RORC_THROW_EXCEPTION("Failed to arm data generator");
  }

  if (LoopbackMode::INTERNAL_RORC == gen.loopbackMode) {
    rorcParamOn(barAddress, PRORC_PARAM_LOOPB);
    usleep(100000);
  }

  if (LoopbackMode::EXTERNAL_SIU == gen.loopbackMode) {
    ret = ddlSetSiuLoopBack(barAddress, DDL_RESPONSE_TIME * pciLoopPerUsec, &stw);
    if (ret != RORC_STATUS_OK) {
      ALICEO2_RORC_THROW_EXCEPTION("SIU loopback error");
    }
    usleep(100000);
    if (rorcCheckLink(barAddress) != RORC_STATUS_OK) {
      ALICEO2_RORC_THROW_EXCEPTION("SIU not seen, can not clear SIU status");
    } else {
      if (ddlReadSiu(barAddress, 0, DDL_RESPONSE_TIME) == -1) {
        ALICEO2_RORC_THROW_EXCEPTION("SIU read error");
      }
    }
    if (ddlReadDiu(barAddress, 0, DDL_RESPONSE_TIME) == -1) {
      ALICEO2_RORC_THROW_EXCEPTION("DIU read error");
    }
  }
}

void CardPdaCrorc::startDataGenerator(ChannelData& cd, int maxEvents)
{
  rorcStartDataGenerator(cd.bar.getUserspaceAddress(), maxEvents);
}

PageVector CardPdaCrorc::getMappedPages(int channel)
{
  auto& cd = getChannelData(channel);
  PageVector pages;

  for (auto& sglPage : cd.sglWrapper->pages) {
    pages.emplace_back(sglPage.userAddress);
  }

  return pages;
}

} // namespace Rorc
} // namespace AliceO2
