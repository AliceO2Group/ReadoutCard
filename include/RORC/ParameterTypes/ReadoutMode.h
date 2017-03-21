/// \file ReadoutMode.h
/// \brief Definition of the ReadoutMode enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_READOUTMODE_H_
#define ALICEO2_INCLUDE_RORC_READOUTMODE_H_

#include <string>

namespace AliceO2 {
namespace Rorc {

/// Namespace for the RORC readout mode enum, and supporting functions
struct ReadoutMode
{
    /// Readout mode
    enum type
    {
      Continuous,
      //Triggered // Not yet supported
    };

    /// Converts a ReadoutMode to a string
    static std::string toString(const ReadoutMode::type& mode);

    /// Converts a string to a ReadoutMode
    static ReadoutMode::type fromString(const std::string& string);
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_READOUTMODE_H_
