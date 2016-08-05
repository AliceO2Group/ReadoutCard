///
/// \file ChannelUtilityInterface.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <vector>
#include <cstdint>
#include <ostream>

namespace AliceO2 {
namespace Rorc {

/// This is a library-private interface that ChannelMasters also implement
/// They provide the functions used in the utility programs, essentially delegating the implementation.
/// This interface was introduced after the utilities started using hacky solutions to access things that should be
/// more or less private to the ChannelMaster.
/// With this interface, the utility functionality is more maintainable and we respect the privacy of the ChannelMaster.
class ChannelUtilityInterface
{
  public:

    virtual ~ChannelUtilityInterface()
    {
    }

    virtual std::vector<uint32_t> utilityCopyFifo() = 0;
    virtual void utilityPrintFifo(std::ostream& os) = 0;
    virtual void utilitySetLedState(bool state) = 0;
    virtual void utilitySanityCheck(std::ostream& os) = 0;
    virtual void utilityCleanupState() = 0;
    virtual int utilityGetFirmwareVersion() = 0;
};

} // namespace Rorc
} // namespace AliceO2
