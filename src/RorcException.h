///
/// \file RorcException.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Exceptions for the RORC module
///

#pragma once

#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <boost/exception/all.hpp>
#include <cstdint>
#include "RORC/ChannelParameters.h"
#include "RORC/PciId.h"

namespace AliceO2 {
namespace Rorc {

/// Helper macro for defining errinfo types for Boost exceptions
#define DEFINE_ERRINFO(name, type) \
  using errinfo_rorc_##name = boost::error_info<struct errinfo_rorc_##name##_, type>

// errinfo definitions
DEFINE_ERRINFO(generic_message, std::string);
DEFINE_ERRINFO(possible_causes, std::vector<std::string>);
DEFINE_ERRINFO(readyfifo_status, std::string);
DEFINE_ERRINFO(readyfifo_length, int32_t);
DEFINE_ERRINFO(filename, std::string);
DEFINE_ERRINFO(filesize, size_t);
DEFINE_ERRINFO(directory, std::string);
DEFINE_ERRINFO(serial_number, int);
DEFINE_ERRINFO(channel_number, int);
DEFINE_ERRINFO(status_code, int);
DEFINE_ERRINFO(pda_status_code, int);
DEFINE_ERRINFO(ddl_reset_mask, std::string);
DEFINE_ERRINFO(page_index, int);
DEFINE_ERRINFO(fifo_index, int);
DEFINE_ERRINFO(reset_level, ResetLevel::type);
DEFINE_ERRINFO(loopback_mode, LoopbackMode::type);
DEFINE_ERRINFO(siu_command, int);
DEFINE_ERRINFO(diu_command, int);
DEFINE_ERRINFO(generator_pattern, int);
DEFINE_ERRINFO(generator_seed, int);
DEFINE_ERRINFO(generator_event_length, size_t);
DEFINE_ERRINFO(pci_id, PciId);
DEFINE_ERRINFO(pci_device_index, int);

// Undefine macro for header safety
#undef DEFINE_ERRINFO

// RORC exception definitions
struct RorcException : virtual boost::exception, virtual std::exception {};

// General exception definitions
struct RorcPdaException : virtual RorcException {};
struct MemoryMapException : virtual RorcException {};
struct InvalidParameterException : virtual RorcException {};
struct FileLockException : virtual RorcException {};
struct DeviceFinderException : virtual RorcException {};

// C-RORC exception definitions
struct CrorcException : virtual RorcException {};
struct CrorcArmDataGeneratorException : virtual CrorcException {};
struct CrorcArmDdlException : virtual CrorcException {};
struct CrorcInitDiuException : virtual CrorcException {};
struct CrorcCheckLinkException : virtual CrorcException {};
struct CrorcSiuCommandException : virtual CrorcException {};
struct CrorcDiuCommandException : virtual CrorcException {};
struct CrorcSiuLoopbackException : virtual CrorcException {};
struct CrorcFreeFifoException : virtual CrorcException {};
struct CrorcStartDataGeneratorException : virtual CrorcException {};
struct CrorcStartTriggerException : virtual CrorcException {};
struct CrorcStopTriggerException : virtual CrorcException {};
struct CrorcDataArrivalException : virtual CrorcException {};

// CRU exception definitions
struct CruException : virtual RorcException {};

// Utility exception definition
struct UtilException : virtual boost::exception, virtual std::exception {};
struct InvalidOptionValueException : virtual UtilException {};
struct OptionRequiredException : virtual UtilException {};

/// Adds the given possible causes to the exception object.
/// Meant for catch & re-throw site usage, to avoid overwriting old errinfo_rorc_possible_causes.
/// This is necessary, because adding an errinfo overwrites any previous one present.
void addPossibleCauses(boost::exception& exception, const std::vector<std::string>& possibleCauses);

} // namespace Rorc
} // namespace AliceO2


namespace boost {

// These functions convert the errinfos to strings for diagnostic messages
std::string to_string(const ::AliceO2::Rorc::errinfo_rorc_generic_message& e);
std::string to_string(const ::AliceO2::Rorc::errinfo_rorc_possible_causes& e);
std::string to_string(const ::AliceO2::Rorc::errinfo_rorc_loopback_mode& e);
std::string to_string(const ::AliceO2::Rorc::errinfo_rorc_reset_level& e);
std::string to_string(const ::AliceO2::Rorc::errinfo_rorc_status_code& e);
std::string to_string(const ::AliceO2::Rorc::errinfo_rorc_pci_id& e);

}

/// Helper macro for throwing a generic exception
#define ALICEO2_RORC_THROW_EXCEPTION(message) \
    BOOST_THROW_EXCEPTION(::AliceO2::Rorc::RorcException() << ::AliceO2::Rorc::errinfo_rorc_generic_message(message))
