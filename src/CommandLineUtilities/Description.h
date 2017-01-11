/// \file Description.h
/// \brief Definition of the Description class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

namespace AliceO2 {
namespace Rorc {
namespace CommandLineUtilities {

/// Contains some descriptive text about a RORC utility executable
struct Description
{
    Description(const std::string& name, const std::string& description, const std::string& usage)
        : name(name), description(description), usage(usage)
    {
    }

    /// Name of the utility
    const std::string name;

    /// Short description
    const std::string description;

    /// Usage example
    const std::string usage;
};

} // namespace CommandLineUtilities
} // namespace Rorc
} // namespace AliceO2
