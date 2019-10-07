// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file ExceptionInternal.h
/// \brief Definition of internal ReadoutCard exceptions and related functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_EXCEPTIONINTERNAL_H_
#define ALICEO2_SRC_READOUTCARD_EXCEPTIONINTERNAL_H_

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/exception/all.hpp>
#include <cstdint>
#include "Common/Exception.h"
#include "ReadoutCard/Exception.h"
#include "ReadoutCard/PciId.h"
#include "ReadoutCard/CardType.h"
#include "ReadoutCard/Parameters.h"
#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include "ReadoutCard/ParameterTypes/PciSequenceNumber.h"
#include "ReadoutCard/ParameterTypes/ResetLevel.h"
#include "ReadoutCard/ParameterTypes/DataSource.h"

namespace AliceO2
{
namespace roc
{
namespace ErrorInfo
{

using Message = AliceO2::Common::ErrorInfo::Message;
using FileName = AliceO2::Common::ErrorInfo::FileName;
using FilesystemType = AliceO2::Common::ErrorInfo::FilesystemType;

/// Helper macro for defining errinfo types for Boost exceptions
#define DEFINE_ERRINFO(name, type) \
  using name = ::boost::error_info<struct _##name, type>

DEFINE_ERRINFO(Address, uintptr_t);
DEFINE_ERRINFO(BarIndex, size_t);
DEFINE_ERRINFO(BarSize, size_t);
DEFINE_ERRINFO(CardId, ::AliceO2::roc::Parameters::CardIdType);
DEFINE_ERRINFO(CardType, ::AliceO2::roc::CardType::type);
DEFINE_ERRINFO(ChannelNumber, int);
DEFINE_ERRINFO(DdlResetMask, std::string);
DEFINE_ERRINFO(Directory, std::string);
DEFINE_ERRINFO(DiuCommand, int);
DEFINE_ERRINFO(DmaBufferPages, size_t);
DEFINE_ERRINFO(DmaBufferSize, size_t);
DEFINE_ERRINFO(DmaPageSize, size_t);
DEFINE_ERRINFO(FifoIndex, int);
DEFINE_ERRINFO(FifoSize, size_t);
DEFINE_ERRINFO(FileSize, size_t);
DEFINE_ERRINFO(GeneratorEventLength, size_t);
DEFINE_ERRINFO(Index, size_t);
DEFINE_ERRINFO(LinkId, uint32_t);
DEFINE_ERRINFO(DataSource, ::AliceO2::roc::DataSource::type);
DEFINE_ERRINFO(NamedMutexName, std::string);
DEFINE_ERRINFO(Offset, size_t);
DEFINE_ERRINFO(PageIndex, int);
DEFINE_ERRINFO(Pages, size_t);
DEFINE_ERRINFO(ParameterKey, std::string);
DEFINE_ERRINFO(PciAddress, ::AliceO2::roc::PciAddress);
DEFINE_ERRINFO(PciAddressBusNumber, int);
DEFINE_ERRINFO(PciAddressSlotNumber, int);
DEFINE_ERRINFO(PciAddressFunctionNumber, int);
DEFINE_ERRINFO(PciDeviceIndex, int);
DEFINE_ERRINFO(PciId, ::AliceO2::roc::PciId);
DEFINE_ERRINFO(PciIds, std::vector<::AliceO2::roc::PciId>);
DEFINE_ERRINFO(PciSequenceNumber, ::AliceO2::roc::PciSequenceNumber);
DEFINE_ERRINFO(PdaStatusCode, int);
DEFINE_ERRINFO(PossibleCauses, std::vector<std::string>);
DEFINE_ERRINFO(Range, size_t);
DEFINE_ERRINFO(ReadyFifoLength, int32_t);
DEFINE_ERRINFO(ReadyFifoStatus, int);
DEFINE_ERRINFO(ResetLevel, ::AliceO2::roc::ResetLevel::type);
DEFINE_ERRINFO(ScatterGatherEntrySize, size_t);
DEFINE_ERRINFO(SerialNumber, int);
DEFINE_ERRINFO(SharedBufferFile, std::string);
DEFINE_ERRINFO(SharedFifoFile, std::string);
DEFINE_ERRINFO(SharedLockFile, std::string);
DEFINE_ERRINFO(SharedObjectName, std::string);
DEFINE_ERRINFO(SharedStateFile, std::string);
DEFINE_ERRINFO(SiuCommand, int);
DEFINE_ERRINFO(String, std::string);
DEFINE_ERRINFO(StwExpected, std::string);
DEFINE_ERRINFO(StwReceived, std::string);

// Undefine macro for header safety (we don't want to pollute the global namespace with collision-prone names)
#undef DEFINE_ERRINFO
} // namespace ErrorInfo

/// Adds the given possible causes to the exception object.
/// Meant for catch & re-throw site usage, to avoid overwriting old ErrorInfo::PossibleCauses.
/// This is necessary, because adding an errinfo overwrites any previous one present.
void addPossibleCauses(boost::exception& exception, const std::vector<std::string>& possibleCauses);

} // namespace roc
} // namespace AliceO2

namespace boost
{

// These functions convert the errinfos to strings for diagnostic messages
std::string to_string(const AliceO2::roc::ErrorInfo::CardId& e);
std::string to_string(const AliceO2::roc::ErrorInfo::CardType& e);
std::string to_string(const AliceO2::roc::ErrorInfo::Message& e);
std::string to_string(const AliceO2::roc::ErrorInfo::DataSource& e);
std::string to_string(const AliceO2::roc::ErrorInfo::PciAddress& e);
std::string to_string(const AliceO2::roc::ErrorInfo::PciId& e);
std::string to_string(const AliceO2::roc::ErrorInfo::PciIds& e);
std::string to_string(const AliceO2::roc::ErrorInfo::PciSequenceNumber& e);
std::string to_string(const AliceO2::roc::ErrorInfo::PossibleCauses& e);
std::string to_string(const AliceO2::roc::ErrorInfo::ReadyFifoStatus& e);
std::string to_string(const AliceO2::roc::ErrorInfo::ResetLevel& e);

} // namespace boost

#endif // ALICEO2_SRC_READOUTCARD_EXCEPTIONINTERNAL_H_
