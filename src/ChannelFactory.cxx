///
/// \file ChannelFactory.cxx
/// \author Pascal Boeschoten
///

#include "RORC/ChannelFactory.h"
#include <dirent.h>
#include <boost/range/iterator_range.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "RorcException.h"
#include "RorcDeviceFinder.h"
#include "DummyChannelMaster.h"
#include "DummyChannelSlave.h"
#ifdef ALICEO2_RORC_PDA_ENABLED
#  include <pda.h>
#  include "CrorcChannelMaster.h"
#  include "CrorcChannelSlave.h"
#  include "CruChannelMaster.h"
#  include "CruChannelSlave.h"
#  include "ChannelPaths.h"
#endif

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

namespace AliceO2 {
namespace Rorc {

ChannelFactory::ChannelFactory()
{
}

ChannelFactory::~ChannelFactory()
{
}

void makeParentDirectories(const bfs::path& path)
{
  /// TODO Implement using boost::filesystem
  system(b::str(b::format("mkdir -p %s") % path.parent_path()).c_str());
}

/// Similar to the "touch" Linux command
void touchFile(const bfs::path& path)
{
  std::ofstream ofs(path.c_str(), std::ios::app);
}

std::shared_ptr<ChannelMasterInterface> ChannelFactory::getMaster(int serialNumber, int channelNumber,
    const ChannelParameters& params)
{
#ifdef ALICEO2_RORC_PDA_ENABLED
  if (serialNumber == DUMMY_SERIAL_NUMBER) {
    return std::make_shared<DummyChannelMaster>(serialNumber, channelNumber, params);
  } else {
    // Find the PCI device
    RorcDeviceFinder finder(serialNumber);

    switch (finder.getCardType()) {
      case RorcDeviceFinder::CardType::CRORC: {
        // TODO Should this filesystem stuff be done by the ChannelMaster object itself?
        makeParentDirectories(ChannelPaths::pages(serialNumber, channelNumber));
        makeParentDirectories(ChannelPaths::state(serialNumber, channelNumber));
        makeParentDirectories(ChannelPaths::fifo(serialNumber, channelNumber));
        makeParentDirectories(ChannelPaths::lock(serialNumber, channelNumber));
        touchFile(ChannelPaths::lock(serialNumber, channelNumber));
        return std::make_shared<CrorcChannelMaster>(serialNumber, channelNumber, params);
      }
      case RorcDeviceFinder::CardType::CRU: {
        // TODO Remove throw when CruChannelMaster is in usable state
        BOOST_THROW_EXCEPTION(RorcException()
            << errinfo_rorc_generic_message("CRU not yet supported")
            << errinfo_rorc_serial_number(serialNumber)
            << errinfo_rorc_channel_number(channelNumber));
        return std::make_shared<CruChannelMaster>(serialNumber, channelNumber, params);
      }
      default: {
        BOOST_THROW_EXCEPTION(RorcException()
            << errinfo_rorc_generic_message("Unknown card type")
            << errinfo_rorc_serial_number(serialNumber)
            << errinfo_rorc_channel_number(channelNumber));
      }
    }
  }
#else
#pragma message("PDA not enabled, Alice02::Rorc::ChannelFactory::getMaster() will always return a dummy implementation")
  return std::make_shared<DummyChannelMaster>();
#endif
}


std::shared_ptr<ChannelSlaveInterface> ChannelFactory::getSlave(int serialNumber, int channelNumber)
{
#ifdef ALICEO2_RORC_PDA_ENABLED
  if (serialNumber == DUMMY_SERIAL_NUMBER) {
    return std::make_shared<DummyChannelSlave>(serialNumber, channelNumber);
  } else {
    // Find the PCI device
    RorcDeviceFinder finder(serialNumber);

    switch (finder.getCardType()) {
      case RorcDeviceFinder::CardType::CRORC: {
        return std::make_shared<CrorcChannelSlave>(serialNumber, channelNumber);
      }
      case RorcDeviceFinder::CardType::CRU: {
        return std::make_shared<CruChannelSlave>(serialNumber, channelNumber);
      }
      default: {
        BOOST_THROW_EXCEPTION(RorcException()
            << errinfo_rorc_generic_message("Unknown card type")
            << errinfo_rorc_serial_number(serialNumber)
            << errinfo_rorc_channel_number(channelNumber));
      }
    }
  }
#else
#pragma message("PDA not enabled, Alice02::Rorc::ChannelFactory::getSlave() will always return a dummy implementation")
  return std::make_shared<DummyChannelMaster>();
#endif
}

} // namespace Rorc
} // namespace AliceO2
