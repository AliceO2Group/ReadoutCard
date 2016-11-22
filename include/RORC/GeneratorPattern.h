/// \file GeneratorPattern.h
/// \brief Definition of the GeneratorPattern enum and supporting functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_GENERATORPATTERN_H_
#define ALICEO2_INCLUDE_RORC_GENERATORPATTERN_H_

#include <string>

namespace AliceO2 {
namespace Rorc {

/// Namespace for the RORC generator pattern enum
struct GeneratorPattern
{
  enum type
  {
    Unknown = 0,
    Constant = 1,
    Alternating = 2,
    Flying0 = 3,
    Flying1 = 4,
    Incremental = 5,
    Decremental = 6,
    Random = 7
  };
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_GENERATORPATTERN_H_
