/// \file LoopbackMode.h
/// \brief Definition of the LoopbackMode enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_LOOPBACKMODE_H_
#define ALICEO2_INCLUDE_RORC_LOOPBACKMODE_H_

#include <string>

namespace AliceO2 {
namespace Rorc {

/// Namespace for the RORC loopback mode enum, and supporting functions
struct LoopbackMode
{
    /// Loopback mode
    enum type
    {
      None = 0, Diu = 1, Siu = 2, Rorc = 3
    };

    /// Converts a LoopbackMode to a string
    static std::string toString(const LoopbackMode::type& mode);

    /// Converts a string to a LoopbackMode
    static LoopbackMode::type fromString(const std::string& string);

    /// Returns true if the loopback mode is external (SIU and/or DIU)
    static bool isExternal(const LoopbackMode::type& mode);
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_LOOPBACKMODE_H_
