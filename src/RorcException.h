///
/// \file RorcException.h
/// \author Pascal Boeschoten
///
/// Exceptions for the RORC module
///

#pragma once

#include <stdexcept>
#include <string>
#include <sstream>
#include <boost/exception/all.hpp>
#include <cstdint>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

namespace AliceO2 {
namespace Rorc {

/// Helper macro for defining errinfo types for Boost exceptions
#define DEFINE_ERRINFO(name, type) \
  using errinfo_rorc_##name = boost::error_info<struct errinfo_rorc_##name##_, type>

// errinfo definitions
DEFINE_ERRINFO(generic_message, std::string);
DEFINE_ERRINFO(possible_causes, std::string);
DEFINE_ERRINFO(readyfifo_status, std::string);
DEFINE_ERRINFO(readyfifo_length, int32_t);
DEFINE_ERRINFO(filename, std::string);
DEFINE_ERRINFO(filesize, size_t);
DEFINE_ERRINFO(directory, std::string);
DEFINE_ERRINFO(serial_number, int);
DEFINE_ERRINFO(channel_number, int);
DEFINE_ERRINFO(status_code, int);
DEFINE_ERRINFO(status_code_string, std::string);
DEFINE_ERRINFO(ddl_reset_mask, std::string);
DEFINE_ERRINFO(page_index, int);
DEFINE_ERRINFO(fifo_index, int);
DEFINE_ERRINFO(reset_level, int);
DEFINE_ERRINFO(reset_level_string, std::string);
DEFINE_ERRINFO(loopback_mode, int);
DEFINE_ERRINFO(loopback_mode_string, std::string);
DEFINE_ERRINFO(siu_command, int);
DEFINE_ERRINFO(diu_command, int);
DEFINE_ERRINFO(generator_pattern, int);
DEFINE_ERRINFO(generator_seed, int);
DEFINE_ERRINFO(generator_event_length, size_t);

// Undefine macro for header safety
#undef DEFINE_ERRINFO

// RORC exception definitions
struct RorcException : virtual boost::exception, virtual std::exception {};

// General exception definitions
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

} // namespace Rorc
} // namespace AliceO2

#define ALICEO2_RORC_THROW_EXCEPTION(message) \
    BOOST_THROW_EXCEPTION(RorcException() << errinfo_rorc_generic_message(message))
