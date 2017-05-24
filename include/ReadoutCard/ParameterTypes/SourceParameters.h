/// \file BufferParameters.h
/// \brief Definition of the BufferParameters
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_SOURCEPARAMETERS_H_
#define ALICEO2_INCLUDE_READOUTCARD_SOURCEPARAMETERS_H_

#include <string>
#include <boost/variant.hpp>
#include "ReadoutCard/ParameterTypes/GeneratorPattern.h"
#include "ReadoutCard/ParameterTypes/LoopbackMode.h"
#include "LoopbackMode.h"

namespace AliceO2 {
namespace roc {
namespace source_parameters {

struct Trigger
{
};

struct Continuous
{
};

struct Generator
{
    GeneratorPattern::type pattern;
    LoopbackMode::type loopback;

    struct FixedSize
    {
        size_t size;
    };
    struct RandomSize
    {
        size_t min;
        size_t max;
    };

    boost::variant<FixedSize, RandomSize> size;
};

} // namespace source_parameters
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_SOURCEPARAMETERS_H_
