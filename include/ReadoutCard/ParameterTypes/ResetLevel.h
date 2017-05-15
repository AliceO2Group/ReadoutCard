/// \file ResetLevel.h
/// \brief Definition of the ResetLevel enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_RESETLEVEL_H_
#define ALICEO2_INCLUDE_READOUTCARD_RESETLEVEL_H_

#include <string>

namespace AliceO2 {
namespace roc {

/// Namespace for the RORC reset level enum, and supporting functions
struct ResetLevel
{
    enum type
    {
      Nothing = 0, ///< No reset
      Internal = 1, ///< Reset internally only
      InternalDiu = 2, ///< Reset internally, and the DIU
      InternalDiuSiu = 3, ///< Reset internally, the DIU, and the SIU
    };

    /// Converts a ResetLevel to a string
    static std::string toString(const ResetLevel::type& level);

    /// Converts a string to a ResetLevel
    static ResetLevel::type fromString(const std::string& string);

    /// Returns true if the reset level includes external resets (SIU and/or DIU)
    static bool includesExternal(const ResetLevel::type& level);
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_RESETLEVEL_H_
