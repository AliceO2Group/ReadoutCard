/// \file ChannelMasterPdaBase.cxx
/// \brief Implementation of the ChannelMasterPdaBase class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ChannelMasterPdaBase.h"
#include <boost/filesystem/path.hpp>
#include "Pda/Pda.h"
#include "Utilities/SmartPointer.h"
#include "Utilities/Util.h"
#include "Visitor.h"

namespace AliceO2 {
namespace Rorc {
namespace {

CardDescriptor getDescriptor(const Parameters& parameters)
{
  return Visitor::apply<CardDescriptor>(parameters.getCardIdRequired(),
      [&](int serial) {return RorcDevice(serial).getCardDescriptor();},
      [&](const PciAddress& address) {return RorcDevice(address).getCardDescriptor();});
}


/// Throws if the file system type of the given file/directory is not one of the given valid types
void assertFileSystemType(std::string path, const std::set<std::string>& validTypes, std::string name)
{
  bool found;
  std::string type;
  std::tie(found, type) = Utilities::isFileSystemTypeAnyOf(path, validTypes);

  if (!found) {
    std::ostringstream oss;
    oss << "File-backed shared memory for '" << name << "' file system type invalid (supported: ";
    for (auto i = validTypes.begin(); i != validTypes.end(); i++) {
      oss << *i;
      if (i != validTypes.end()) {
        oss << ",";
      }
    }
    oss << ")";

    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message(oss.str())
        << ErrorInfo::FileName(path)
        << ErrorInfo::FilesystemType(type));
  }
}
}

ChannelMasterPdaBase::ChannelMasterPdaBase(const Parameters& parameters,
    const AllowedChannels& allowedChannels, size_t fifoSize)
    : ChannelMasterBase(getDescriptor(parameters), parameters, allowedChannels), mDmaState(DmaState::STOPPED)
{
  // Initialize PDA & DMA objects
  Utilities::resetSmartPtr(mRorcDevice, getSerialNumber());

  log("Initializing BAR", InfoLogger::InfoLogger::Debug);
  Utilities::resetSmartPtr(mPdaBar, mRorcDevice->getPciDevice(), getChannelNumber());

  // Create and register our internal FIFO buffer
  log("Initializing FIFO DMA buffer", InfoLogger::InfoLogger::Debug);
  {
    // Note: since the CRU needs a contiguous FIFO that's bigger than the default memory page size of 4KB, we use a
    // 2 MB hugepage for the FIFO. Only the actually used size (= fifoSize) will be registered with PDA.
    constexpr size_t FIFO_ALLOCATION_SIZE = 2 * 1024 * 1024;

    // If for some weird reason a card needs more than 2MB in the future, this will throw...
    if (fifoSize > FIFO_ALLOCATION_SIZE) {
      BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("FIFO size exceeded 2 MB"));
    }

    // Check file system type to make sure we're using hugetlbfs
    assertFileSystemType(boost::filesystem::path(boost::filesystem::path(getPaths().fifo())).parent_path().string(),
        {"hugetlbfs"}, "fifo");

    // Now we can create and register the buffer
    // Note: if resizing the file fails, we might've accidentally put the file in a hugetlbfs mount with 1 GB page size
    Utilities::resetSmartPtr(mBufferFifoFile, getPaths().fifo(), FIFO_ALLOCATION_SIZE);
    Utilities::resetSmartPtr(mPdaDmaBufferFifo, mRorcDevice->getPciDevice(), mBufferFifoFile->getAddress(), fifoSize,
        getPdaDmaBufferIndexFifo(getChannelNumber()));

    const auto& entry = mPdaDmaBufferFifo->getScatterGatherList().at(0);
    if (entry.size < fifoSize) {
      // Something must've failed at some point
      BOOST_THROW_EXCEPTION(Exception()
          << ErrorInfo::Message("Scatter gather list entry for internal FIFO was too small")
          << ErrorInfo::ScatterGatherEntrySize(entry.size)
          << ErrorInfo::FifoSize(fifoSize));
    }
    mFifoAddressUser = entry.addressUser;
    mFifoAddressBus = entry.addressBus;
  }

  // Register user's page data buffer
  log("Initializing memory-mapped DMA buffer", InfoLogger::InfoLogger::Debug);
  Utilities::resetSmartPtr(mPdaDmaBuffer, mRorcDevice->getPciDevice(), getBufferProvider().getAddress(),
      getBufferProvider().getSize(), getPdaDmaBufferIndexPages(getChannelNumber(), 0));
}

ChannelMasterPdaBase::~ChannelMasterPdaBase()
{
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMasterPdaBase::startDma()
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    log("Unknown DMA state");
  } else if (mDmaState == DmaState::STARTED) {
    log("DMA already started. Ignoring startDma() call");
  } else {
    log("Starting DMA", InfoLogger::InfoLogger::Debug);
    deviceStartDma();
  }
  mDmaState = DmaState::STARTED;
}

// Checks DMA state and forwards call to subclass if necessary
void ChannelMasterPdaBase::stopDma()
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    log("Unknown DMA state");
  } else if (mDmaState == DmaState::STOPPED) {
    log("Warning: DMA already stopped. Ignoring stopDma() call");
  } else {
    log("Stopping DMA", InfoLogger::InfoLogger::Debug);
    deviceStopDma();
  }
  mDmaState = DmaState::STOPPED;
}

void ChannelMasterPdaBase::resetChannel(ResetLevel::type resetLevel)
{
  CHANNELMASTER_LOCKGUARD();

  if (mDmaState == DmaState::UNKNOWN) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Reset channel failed: DMA in unknown state"));
  }
  if (mDmaState != DmaState::STOPPED) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Reset channel failed: DMA was not stopped"));
  }

  log("Resetting channel", InfoLogger::InfoLogger::Debug);
  deviceResetChannel(resetLevel);
}

uint32_t ChannelMasterPdaBase::readRegister(int index)
{
  return mPdaBar->getRegister<uint32_t>(index * sizeof(uint32_t));
}

void ChannelMasterPdaBase::writeRegister(int index, uint32_t value)
{
  mPdaBar->setRegister<uint32_t>(index * sizeof(uint32_t), value);
}

uintptr_t ChannelMasterPdaBase::getBusOffsetAddress(size_t offset)
{
  const auto& list = getPdaDmaBuffer().getScatterGatherList();

  auto userBase = list.at(0).addressUser;
  auto userWithOffset = userBase + offset;

  // First we find the SGL entry that contains our address
  for (int i = 0; i < list.size(); ++i) {
    auto entryUserStartAddress = list[i].addressUser;
    auto entryUserEndAddress = entryUserStartAddress + list[i].size;

    if ((userWithOffset >= entryUserStartAddress) && (userWithOffset < entryUserEndAddress)) {
      // This is the entry we need
      // We now need to calculate the difference from the start of this entry to the given offset. We make use of the
      // fact that the userspace addresses will be contiguous
      auto entryOffset = userWithOffset - entryUserStartAddress;
      auto offsetBusAddress = list[i].addressBus + entryOffset;
      return offsetBusAddress;
    }
  }

  BOOST_THROW_EXCEPTION(Exception()
      << ErrorInfo::Message("Physical offset address out of range")
      << ErrorInfo::Offset(offset));
}



} // namespace Rorc
} // namespace AliceO2
