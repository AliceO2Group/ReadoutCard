///
/// \file Exception.h
/// \author Pascal Boeschoten
///
/// Utilities for RORC exceptions
///

#pragma once

#include <stdexcept>
#include <string>
#include <sstream>
#include <boost/exception/all.hpp>
#include <cstdint>

namespace AliceO2 {
namespace Rorc {

struct AliceO2RorcException : virtual boost::exception, virtual std::exception {};

/// Helper macro for defining errinfo types for Boost exceptions
#define DEFINE_ERRINFO(name, type) \
  using errinfo_aliceO2_rorc_##name = boost::error_info<struct errinfo_aliceO2_rorc_##name##_, type>

/// Helper macro for defining exception types
#define DEFINE_EXCEPTION(name) \
  struct name##Exception : virtual AliceO2RorcException {}

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

// Exception definitions
DEFINE_EXCEPTION(MemoryMap);
DEFINE_EXCEPTION(InvalidParameter);
DEFINE_EXCEPTION(FileLock);
DEFINE_EXCEPTION(Crorc);
DEFINE_EXCEPTION(CrorcArmDataGenerator);
DEFINE_EXCEPTION(CrorcArmDdl);
DEFINE_EXCEPTION(CrorcInitDiu);
DEFINE_EXCEPTION(CrorcCheckLink);
DEFINE_EXCEPTION(CrorcSiuCommand);
DEFINE_EXCEPTION(CrorcDiuCommand);
DEFINE_EXCEPTION(CrorcSiuLoopback);
DEFINE_EXCEPTION(CrorcFreeFifo);
DEFINE_EXCEPTION(CrorcStartDataGenerator);
DEFINE_EXCEPTION(CrorcStartTrigger);
DEFINE_EXCEPTION(CrorcStopTrigger);
DEFINE_EXCEPTION(CrorcDataArrival);
DEFINE_EXCEPTION(Cru);

// Undefine macros for header safety
#undef DEFINE_ERRINFO
#undef DEFINE_EXCEPTION

} // namespace Rorc
} // namespace AliceO2

#define ALICEO2_RORC_THROW_EXCEPTION(message) \
    BOOST_THROW_EXCEPTION(AliceO2RorcException() << errinfo_aliceO2_rorc_generic_message(message))
