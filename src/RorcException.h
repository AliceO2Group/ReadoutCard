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
#define ALICEO2_RORC_DEFINE_ERRINFO(name, type) \
  using errinfo_aliceO2_rorc_##name = boost::error_info<struct errinfo_aliceO2_rorc_##name##_, type>

/// Helper macro for defining exception types
#define ALICEO2_RORC_DEFINE_EXCEPTION(name) \
  struct name##Exception : virtual AliceO2RorcException {}

// errinfo definitions
ALICEO2_RORC_DEFINE_ERRINFO(generic_message, std::string);
ALICEO2_RORC_DEFINE_ERRINFO(possible_causes, std::string);
ALICEO2_RORC_DEFINE_ERRINFO(readyfifo_status, std::string);
ALICEO2_RORC_DEFINE_ERRINFO(readyfifo_length, int32_t);
ALICEO2_RORC_DEFINE_ERRINFO(filename, std::string);
ALICEO2_RORC_DEFINE_ERRINFO(filesize, size_t);
ALICEO2_RORC_DEFINE_ERRINFO(directory, std::string);
ALICEO2_RORC_DEFINE_ERRINFO(serial_number, int);
ALICEO2_RORC_DEFINE_ERRINFO(channel_number, int);
ALICEO2_RORC_DEFINE_ERRINFO(status_code, int);
ALICEO2_RORC_DEFINE_ERRINFO(status_string, std::string);
ALICEO2_RORC_DEFINE_ERRINFO(ddl_reset_mask, std::string);
ALICEO2_RORC_DEFINE_ERRINFO(page_index, int);
ALICEO2_RORC_DEFINE_ERRINFO(fifo_index, int);

// Exception definitions
ALICEO2_RORC_DEFINE_EXCEPTION(MemoryMap);
ALICEO2_RORC_DEFINE_EXCEPTION(InvalidParameter);
ALICEO2_RORC_DEFINE_EXCEPTION(FileLock);
ALICEO2_RORC_DEFINE_EXCEPTION(Crorc);
ALICEO2_RORC_DEFINE_EXCEPTION(Cru);

// Undefine macros for header safety
#undef ALICEO2_RORC_DEFINE_ERRINFO
#undef ALICEO2_RORC_DEFINE_EXCEPTION

} // namespace Rorc
} // namespace AliceO2

#define ALICEO2_RORC_THROW_EXCEPTION(message) \
    BOOST_THROW_EXCEPTION(AliceO2RorcException() << errinfo_aliceO2_rorc_generic_message(message))
