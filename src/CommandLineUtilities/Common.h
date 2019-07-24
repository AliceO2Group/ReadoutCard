// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Common.h
/// \brief Definition of common functions for the ReadoutCard utilities.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_COMMON_H
#define ALICEO2_READOUTCARD_COMMON_H

#include <boost/program_options.hpp>

namespace AliceO2
{
namespace roc
{
namespace CommandLineUtilities
{
namespace Common
{

/// Create a string showing individual bits of the given 32-bit value
std::string make32bitString(uint32_t bits);

/// Create a string showing the given 32-bit value in hexadecimal format
std::string make32hexString(uint32_t bits);

/// Create a string representation of a register address
std::string makeRegisterAddressString(int address);

/// Create a string representation of a register address and its value
std::string makeRegisterString(int address, uint32_t value);

} // namespace Common
} // namespace CommandLineUtilities
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_COMMON_H
