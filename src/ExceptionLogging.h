/// \file ExceptionLogging.h
/// \brief Definition of various exception logging utilities
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <boost/throw_exception.hpp>
#include "InfoLogger/InfoLogger.hxx"
#include "RORC/Exception.h"

namespace AliceO2 {
namespace Rorc {
namespace Util {

/// Log an exception to the InfoLogger
/// Used for macro THROW_LOGGED_EXCEPTION
inline void _logBoostException(const boost::exception& e, InfoLogger::InfoLogger& logger,
    InfoLogger::InfoLogger::Severity severity)
{
  logger << severity << boost::diagnostic_information(e) << InfoLogger::InfoLogger::endm;
}

/// Log an exception to the InfoLogger
/// Used for macro THROW_LOGGED_EXCEPTION
inline void _logStdException(const std::exception& e, InfoLogger::InfoLogger& logger,
    InfoLogger::InfoLogger::Severity severity)
{
  logger << severity << e.what() << InfoLogger::InfoLogger::endm;
}

#ifdef ALICEO2_RORC_DISABLE_EXCEPTION_LOGGING
#define THROW_LOGGED_EXCEPTION(_logger, _severity, _exception)\
  BOOST_THROW_EXCEPTION(_exception);
#else
#define THROW_LOGGED_EXCEPTION(_logger, _severity, _exception)\
  try {\
    BOOST_THROW_EXCEPTION(_exception);\
  }\
  catch(boost::exception& e) {\
    ::AliceO2::Rorc::Util::_logBoostException(e, _logger, _severity);\
    throw;\
  }\
  catch(std::exception& e) {\
    ::AliceO2::Rorc::Util::_logStdException(e, _logger, _severity);\
    throw;\
  }
#endif

} // namespace Util
} // namespace Rorc
} // namespace AliceO2
