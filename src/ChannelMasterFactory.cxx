///
/// \file ChannelMasterFactory.cxx
/// \author Pascal Boeschoten
///

#include "RORC/ChannelMasterFactory.h"
#include <dirent.h>
#include <boost/range/iterator_range.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "RorcException.h"
#include "RorcDeviceFinder.h"
#include "DummyChannelMaster.h"
#ifdef ALICEO2_RORC_PDA_ENABLED
#  include <pda.h>
#  include "CrorcChannelMaster.h"
#  include "CruChannelMaster.h"
#  include "ChannelPaths.h"
#endif

namespace b = boost;
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

namespace AliceO2 {
namespace Rorc {

ChannelMasterFactory::ChannelMasterFactory()
{
}

ChannelMasterFactory::~ChannelMasterFactory()
{
}

void makeParentDirectories(const bfs::path& path)
{
  system(b::str(b::format("mkdir -p %s") % path.parent_path()).c_str());
}

// Similar to the "touch" Linux command
void touchFile(const bfs::path& path)
{
  std::ofstream ofs(path.c_str(), std::ios::app);
}

std::shared_ptr<ChannelMasterInterface> ChannelMasterFactory::getChannel(int serialNumber, int channelNumber,
    const ChannelParameters& params)
{
#ifdef ALICEO2_RORC_PDA_ENABLED
  if (serialNumber == DUMMY_SERIAL_NUMBER) {
    return std::make_shared<DummyChannelMaster>(serialNumber, channelNumber, params);
  } else {
    RorcDeviceFinder finder(serialNumber);
    auto cardType = finder.getCardType();

    if (cardType == RorcDeviceFinder::CardType::UNKNOWN) {
      ALICEO2_RORC_THROW_EXCEPTION("Unknown card type");
    }

    PdaDevice device(finder.getPciVendorId(), finder.getPciDeviceId());

    if (cardType == RorcDeviceFinder::CardType::CRORC) {
      // TODO Should this filesystem stuff be done by the ChannelMaster object itself?
      makeParentDirectories(ChannelPaths::pages(serialNumber, channelNumber));
      makeParentDirectories(ChannelPaths::state(serialNumber, channelNumber));
      makeParentDirectories(ChannelPaths::fifo(serialNumber, channelNumber));
      makeParentDirectories(ChannelPaths::lock(serialNumber, channelNumber));
      touchFile(ChannelPaths::lock(serialNumber, channelNumber));

      return std::make_shared<CrorcChannelMaster>(serialNumber, channelNumber, params);
    } else if (cardType == RorcDeviceFinder::CardType::CRU) {
      // TODO instantiate CRU ChannelMaster
      ALICEO2_RORC_THROW_EXCEPTION("CRU not yet supported");
    } else {
      ALICEO2_RORC_THROW_EXCEPTION("Unrecognized card type");
    }
  }
#else
#pragma message("PDA not enabled, Alice02::Rorc::ChannelMasterFactory::getCardFromSerialNumber() will always return dummy implementation")
  return std::make_shared<CardDummy>();
#endif
}

} // namespace Rorc
} // namespace AliceO2
