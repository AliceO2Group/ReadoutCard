/// \file ExceptionInternal.h
/// \brief Definition of internal RORC exceptions and related functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/exception/all.hpp>
#include <cstdint>
#include "RORC/Exception.h"
#include "RORC/PciId.h"
#include "RORC/CardType.h"
#include "RORC/ParameterTypes/PciAddress.h"
#include "RORC/ParameterTypes/ResetLevel.h"
#include "RORC/ParameterTypes/GeneratorPattern.h"
#include "RORC/ParameterTypes/LoopbackMode.h"

namespace AliceO2 {
namespace Rorc {
namespace ErrorInfo {

/// Helper macro for defining errinfo types for Boost exceptions
#define DEFINE_ERRINFO(name, type) \
  using name = ::boost::error_info<struct _##name, type>

DEFINE_ERRINFO(Address, uintptr_t);
DEFINE_ERRINFO(BarIndex, size_t);
DEFINE_ERRINFO(BarSize, size_t);
DEFINE_ERRINFO(CardType, ::AliceO2::Rorc::CardType::type);
DEFINE_ERRINFO(ChannelNumber, int);
DEFINE_ERRINFO(DdlResetMask, std::string);
DEFINE_ERRINFO(Directory, std::string);
DEFINE_ERRINFO(DiuCommand, int);
DEFINE_ERRINFO(DmaBufferPages, size_t);
DEFINE_ERRINFO(DmaBufferSize, size_t);
DEFINE_ERRINFO(DmaPageSize, size_t);
DEFINE_ERRINFO(Message, std::string);
DEFINE_ERRINFO(FifoIndex, int);
DEFINE_ERRINFO(FifoSize, size_t);
DEFINE_ERRINFO(FileName, std::string);
DEFINE_ERRINFO(FileSize, size_t);
DEFINE_ERRINFO(FilesystemType, std::string);
DEFINE_ERRINFO(GeneratorEventLength, size_t);
DEFINE_ERRINFO(GeneratorPattern, int);
DEFINE_ERRINFO(GeneratorSeed, int);
DEFINE_ERRINFO(Index, size_t);
DEFINE_ERRINFO(LoopbackMode, ::AliceO2::Rorc::LoopbackMode::type);
DEFINE_ERRINFO(NamedMutexName, std::string);
DEFINE_ERRINFO(Offset, size_t);
DEFINE_ERRINFO(PageIndex, int);
DEFINE_ERRINFO(Pages, size_t);
DEFINE_ERRINFO(ParameterKey, std::string);
DEFINE_ERRINFO(PciAddress, ::AliceO2::Rorc::PciAddress);
DEFINE_ERRINFO(PciAddressBusNumber, int);
DEFINE_ERRINFO(PciAddressSlotNumber, int);
DEFINE_ERRINFO(PciAddressFunctionNumber, int);
DEFINE_ERRINFO(PciDeviceIndex, int);
DEFINE_ERRINFO(PciId, ::AliceO2::Rorc::PciId);
DEFINE_ERRINFO(PciIds, std::vector<::AliceO2::Rorc::PciId>);
DEFINE_ERRINFO(PdaStatusCode, int);
DEFINE_ERRINFO(PossibleCauses, std::vector<std::string>);
DEFINE_ERRINFO(Range, size_t);
DEFINE_ERRINFO(ReadyFifoLength, int32_t);
DEFINE_ERRINFO(ReadyFifoStatus, int);
DEFINE_ERRINFO(ResetLevel, ::AliceO2::Rorc::ResetLevel::type);
DEFINE_ERRINFO(ScatterGatherEntrySize, size_t);
DEFINE_ERRINFO(SerialNumber, int);
DEFINE_ERRINFO(SharedBufferFile, std::string);
DEFINE_ERRINFO(SharedFifoFile, std::string);
DEFINE_ERRINFO(SharedLockFile, std::string);
DEFINE_ERRINFO(SharedObjectName, std::string);
DEFINE_ERRINFO(SharedStateFile, std::string);
DEFINE_ERRINFO(SiuCommand, int);
DEFINE_ERRINFO(StatusCode, int);

// Undefine macro for header safety (we don't want to pollute the global namespace with collision-prone names)
#undef DEFINE_ERRINFO
} // namespace ErrorInfo

/// Adds the given possible causes to the exception object.
/// Meant for catch & re-throw site usage, to avoid overwriting old ErrorInfo::PossibleCauses.
/// This is necessary, because adding an errinfo overwrites any previous one present.
void addPossibleCauses(boost::exception& exception, const std::vector<std::string>& possibleCauses);

} // namespace Rorc
} // namespace AliceO2


namespace boost {

// These functions convert the errinfos to strings for diagnostic messages
std::string to_string(const AliceO2::Rorc::ErrorInfo::CardType& e);
std::string to_string(const AliceO2::Rorc::ErrorInfo::Message& e);
std::string to_string(const AliceO2::Rorc::ErrorInfo::LoopbackMode& e);
std::string to_string(const AliceO2::Rorc::ErrorInfo::PciAddress& e);
std::string to_string(const AliceO2::Rorc::ErrorInfo::PciId& e);
std::string to_string(const AliceO2::Rorc::ErrorInfo::PciIds& e);
std::string to_string(const AliceO2::Rorc::ErrorInfo::PossibleCauses& e);
std::string to_string(const AliceO2::Rorc::ErrorInfo::ReadyFifoStatus& e);
std::string to_string(const AliceO2::Rorc::ErrorInfo::ResetLevel& e);
std::string to_string(const AliceO2::Rorc::ErrorInfo::StatusCode& e);

}
