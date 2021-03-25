// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file RegisterReadWriteInterface.h
/// \brief Definition of the RegisterReadWriteInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_REGISTERREADWRITEINTERFACE_H_
#define O2_READOUTCARD_INCLUDE_REGISTERREADWRITEINTERFACE_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <cstdint>

namespace o2
{
namespace roc
{

/// A simple interface for reading and writing 32-bit registers.
class RegisterReadWriteInterface
{
 public:
  virtual ~RegisterReadWriteInterface()
  {
  }

  /// Reads a BAR register. The registers are indexed per 32 bits
  /// \param index The index of the register
  /// \throw May throw an UnsafeReadAccess exception
  virtual uint32_t readRegister(int index) = 0;

  /// Writes a BAR register
  /// \param index The index of the register
  /// \param value The value to be written into the register
  /// \throw May throw an UnsafeWriteAccess exception
  virtual void writeRegister(int index, uint32_t value) = 0;

  /// Modifies a BAR register
  /// \param index The index of the register
  /// \param position The position where the value will be written
  /// \param width The width of the value to be written
  /// \param value The value to be written into the register
  /// \throw May throw an UnsafeWriteAccess exception
  virtual void modifyRegister(int index, int position, int width, uint32_t value) = 0;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_REGISTERREADWRITEINTERFACE_H_
