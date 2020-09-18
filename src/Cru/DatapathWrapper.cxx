// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Cru/DatapathWrapper.cxx
/// \brief Implementation of Datapath Wrapper
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#include "Constants.h"
#include "DatapathWrapper.h"
#include "Utilities/Util.h"

namespace AliceO2
{
namespace roc
{

DatapathWrapper::DatapathWrapper(std::shared_ptr<Pda::PdaBar> pdaBar) : mPdaBar(pdaBar)
{
}

/// Set links with a bitmask
void DatapathWrapper::setLinksEnabled(uint32_t dwrapper, uint32_t mask)
{
  uint32_t address = getDatapathWrapperBaseAddress(dwrapper) +
                     Cru::Registers::DWRAPPER_GREGS.index +
                     Cru::Registers::DWRAPPER_ENREG.index;
  mPdaBar->writeRegister(address / 4, mask);
}

/// Set particular link's enabled bit
void DatapathWrapper::setLinkEnabled(Link link)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DWRAPPER_GREGS.address +
                     Cru::Registers::DWRAPPER_ENREG.address;
  mPdaBar->modifyRegister(address / 4, link.dwrapperId, 1, 0x1);
}

/// Set particular link's enabled bit
void DatapathWrapper::setLinkDisabled(Link link)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DWRAPPER_GREGS.address +
                     Cru::Registers::DWRAPPER_ENREG.address;
  mPdaBar->modifyRegister(address / 4, link.dwrapperId, 1, 0x0);
}

/// Get particular link's enabled bit
bool DatapathWrapper::getLinkEnabled(Link link)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DWRAPPER_GREGS.address +
                     Cru::Registers::DWRAPPER_ENREG.address;
  uint32_t enabled = mPdaBar->readRegister(address / 4);
  return ((enabled >> link.dwrapperId) & 0x1);
}

/// Set Datapath Mode
void DatapathWrapper::setDatapathMode(Link link, uint32_t mode)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DATAPATHLINK_OFFSET.address +
                     Cru::Registers::DATALINK_OFFSET.address * link.dwrapperId +
                     Cru::Registers::DATALINK_CONTROL.address;

  uint32_t val = 0;
  val |= 0x1FC; //=RAWMAXLEN
  val |= (mode << 31);

  mPdaBar->writeRegister(address / 4, val);
}

/// Get Datapath Mode
DatapathMode::type DatapathWrapper::getDatapathMode(Link link)
{

  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DATAPATHLINK_OFFSET.address +
                     Cru::Registers::DATALINK_OFFSET.address * link.dwrapperId +
                     Cru::Registers::DATALINK_CONTROL.address;

  uint32_t value = mPdaBar->readRegister(address / 4); //1 = packet | 0 = streaming
  DatapathMode::type mode;
  if ((value >> 31) == 0x1) {
    mode = DatapathMode::type::Packet;
  } else {
    mode = DatapathMode::type::Streaming;
  }
  return mode;
}

/// Set Packet Arbitration
/*void DatapathWrapper::setPacketArbitration(int wrapperCount, int arbitrationMode)
{
  uint32_t value = 0x0;
  value |= (arbitrationMode << 15);

  for (int i = 0; i < wrapperCount; i++) {
    uint32_t address = getDatapathWrapperBaseAddress(i) +
                       Cru::Registers::DWRAPPER_GREGS.address +
                       Cru::Registers::DWRAPPER_DATAGEN_CONTROL.address;

    mPdaBar->writeRegister(address / 4, value);
  }
}*/

/// Set Flow Control
void DatapathWrapper::setFlowControl(int wrapper, int allowReject)
{
  uint32_t value = 0;
  value |= (allowReject << 0);

  uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                     Cru::Registers::FLOW_CONTROL_OFFSET.address +
                     Cru::Registers::FLOW_CONTROL_REGISTER.address;

  mPdaBar->writeRegister(address / 4, value);
}

uint32_t DatapathWrapper::getFlowControl(int wrapper)
{
  uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                     Cru::Registers::FLOW_CONTROL_OFFSET.address +
                     Cru::Registers::FLOW_CONTROL_REGISTER.address;
  return mPdaBar->readRegister(address / 4);
}

uint32_t DatapathWrapper::getDatapathWrapperBaseAddress(int wrapper)
{
  if (wrapper == 0) {
    return Cru::Registers::DWRAPPER_BASE0.address;
  } else if (wrapper == 1) {
    return Cru::Registers::DWRAPPER_BASE1.address;
  }

  return 0x0;
}

void DatapathWrapper::resetDataGeneratorPulse()
{
  // Resets data generator
  for (int wrapper = 0; wrapper <= 1; wrapper++) {
    uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                       Cru::Registers::DWRAPPER_GREGS.address +
                       Cru::Registers::DWRAPPER_DATAGEN_CONTROL.address;
    mPdaBar->modifyRegister(address / 4, 0, 1, 0x1);
    mPdaBar->modifyRegister(address / 4, 0, 1, 0x0);
  }
}

void DatapathWrapper::useDataGeneratorSource(bool enable)
{
  // Sets datagenerator as bigfifo input source
  for (int wrapper = 0; wrapper <= 1; wrapper++) {
    uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                       Cru::Registers::DWRAPPER_GREGS.address +
                       Cru::Registers::DWRAPPER_DATAGEN_CONTROL.address;
    if (enable) {
      mPdaBar->modifyRegister(address / 4, 31, 1, 0x1);
    } else {
      mPdaBar->modifyRegister(address / 4, 31, 1, 0x0);
    }
  }
}

void DatapathWrapper::enableDataGenerator(bool enable)
{
  // Enables data generation
  for (int wrapper = 0; wrapper <= 1; wrapper++) {
    uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                       Cru::Registers::DWRAPPER_GREGS.address +
                       Cru::Registers::DWRAPPER_DATAGEN_CONTROL.address;
    if (enable) {
      mPdaBar->modifyRegister(address / 4, 1, 1, 0x1);
    } else {
      mPdaBar->modifyRegister(address / 4, 1, 1, 0x0);
    }
  }
}

void DatapathWrapper::setDynamicOffset(int wrapper, bool enable)
{
  // Enable dynamic offset setting of the RDH (instead of fixed 0x2000)
  uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                     Cru::Registers::DWRAPPER_GREGS.address +
                     Cru::Registers::DWRAPPER_ENREG.address;
  if (enable) {
    mPdaBar->modifyRegister(address / 4, 31, 1, 0x1);
  } else {
    mPdaBar->modifyRegister(address / 4, 31, 1, 0x0);
  }
}

bool DatapathWrapper::getDynamicOffsetEnabled(int wrapper)
{
  // Enable dynamic offset setting of the RDH (instead of fixed 0x2000)
  uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                     Cru::Registers::DWRAPPER_GREGS.address +
                     Cru::Registers::DWRAPPER_ENREG.address;

  uint32_t value = mPdaBar->readRegister(address / 4);
  if (Utilities::getBit(value, 31) == 0x1) {
    return true;
  }
  return false;
}

uint32_t DatapathWrapper::getDroppedPackets(int wrapper)
{
  uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                     Cru::Registers::DWRAPPER_GREGS.address +
                     Cru::Registers::DWRAPPER_DROPPED_PACKETS.address;

  return mPdaBar->readRegister(address / 4);
}

uint32_t DatapathWrapper::getTotalPacketsPerSecond(int wrapper)
{
  uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                     Cru::Registers::DWRAPPER_GREGS.address +
                     Cru::Registers::DWRAPPER_TOTAL_PACKETS_PER_SEC.address;

  return mPdaBar->readRegister(address / 4);
}

uint32_t DatapathWrapper::getAcceptedPackets(Link link)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DATAPATHLINK_OFFSET.address +
                     Cru::Registers::DATALINK_OFFSET.address * link.dwrapperId +
                     Cru::Registers::DATALINK_PACKETS_ACCEPTED.address;

  return mPdaBar->readRegister(address / 4);
}

uint32_t DatapathWrapper::getRejectedPackets(Link link)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DATAPATHLINK_OFFSET.address +
                     Cru::Registers::DATALINK_OFFSET.address * link.dwrapperId +
                     Cru::Registers::DATALINK_PACKETS_REJECTED.address;

  return mPdaBar->readRegister(address / 4);
}

uint32_t DatapathWrapper::getForcedPackets(Link link)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DATAPATHLINK_OFFSET.address +
                     Cru::Registers::DATALINK_OFFSET.address * link.dwrapperId +
                     Cru::Registers::DATALINK_PACKETS_FORCED.address;

  return mPdaBar->readRegister(address / 4);
}

/// size in gbt words
void DatapathWrapper::setTriggerWindowSize(int wrapper, uint32_t size)
{
  if (size > 65535) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("BAD TRIGSIZE, should be less or equal to 65536") << ErrorInfo::ConfigValue(size));
  }
  uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                     Cru::Registers::DWRAPPER_GREGS.address +
                     Cru::Registers::DWRAPPER_TRIGGER_SIZE.address;

  mPdaBar->writeRegister(address / 4, size);
}

/// size in gbt words
uint32_t DatapathWrapper::getTriggerWindowSize(int wrapper)
{
  uint32_t address = getDatapathWrapperBaseAddress(wrapper) +
                     Cru::Registers::DWRAPPER_GREGS.address +
                     Cru::Registers::DWRAPPER_TRIGGER_SIZE.address;

  return mPdaBar->readRegister(address / 4);
}

void DatapathWrapper::toggleUserAndCommonLogic(bool enable, int wrapper)
{
  uint32_t value = enable ? 0x1 : 0x0;
  uint32_t address = (wrapper == 0) ? Cru::Registers::DWRAPPER_BASE0.index : Cru::Registers::DWRAPPER_BASE1.index;

  mPdaBar->modifyRegister(address, 30, 1, value);
}

bool DatapathWrapper::getUserAndCommonLogicEnabled(int wrapper)
{
  uint32_t address = (wrapper == 0) ? Cru::Registers::DWRAPPER_BASE0.index : Cru::Registers::DWRAPPER_BASE1.index;
  return (Utilities::getBit(mPdaBar->readRegister(address), 30) == 0x1);
}

void DatapathWrapper::setSystemId(Link link, uint32_t systemId)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DATAPATHLINK_OFFSET.address +
                     Cru::Registers::DATALINK_OFFSET.address * link.dwrapperId +
                     Cru::Registers::DATALINK_IDS.address;
  mPdaBar->modifyRegister(address / 4, 16, 8, systemId);
}

uint32_t DatapathWrapper::getSystemId(Link link)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DATAPATHLINK_OFFSET.address +
                     Cru::Registers::DATALINK_OFFSET.address * link.dwrapperId +
                     Cru::Registers::DATALINK_IDS.address;

  return (mPdaBar->readRegister(address / 4) & 0xff0000) >> 16;
}

void DatapathWrapper::setFeeId(Link link, uint32_t feeId)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DATAPATHLINK_OFFSET.address +
                     Cru::Registers::DATALINK_OFFSET.address * link.dwrapperId +
                     Cru::Registers::DATALINK_IDS.address;
  mPdaBar->modifyRegister(address / 4, 0, 16, feeId);
}

uint32_t DatapathWrapper::getFeeId(Link link)
{
  uint32_t address = getDatapathWrapperBaseAddress(link.dwrapper) +
                     Cru::Registers::DATAPATHLINK_OFFSET.address +
                     Cru::Registers::DATALINK_OFFSET.address * link.dwrapperId +
                     Cru::Registers::DATALINK_IDS.address;

  return mPdaBar->readRegister(address / 4) & 0xffff;
}

} // namespace roc
} // namespace AliceO2
