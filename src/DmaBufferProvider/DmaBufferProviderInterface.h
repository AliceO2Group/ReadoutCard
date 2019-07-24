// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DmaBufferProviderInterface.h
/// \brief Definition of the BufferProviderInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_DMABUFFERPROVIDER_DMABUFFERPROVIDERINTERFACE_H_
#define ALICEO2_SRC_READOUTCARD_DMABUFFERPROVIDER_DMABUFFERPROVIDERINTERFACE_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace AliceO2
{
namespace roc
{

/// Interface class for providing a DMA buffer with scatter-gather list.
class DmaBufferProviderInterface
{
 public:
  virtual ~DmaBufferProviderInterface()
  {
  }

  /// Get starting userspace address of the DMA buffer
  virtual uintptr_t getAddress() const = 0;

  /// Get total size of the DMA buffer
  virtual size_t getSize() const = 0;

  /// Amount of entries in the scatter-gather list
  virtual size_t getScatterGatherListSize() const = 0;

  /// Get size of an entry of the scatter-gather list
  virtual size_t getScatterGatherEntrySize(int index) const = 0;

  /// Get userspace address of an entry of the scatter-gather list
  virtual uintptr_t getScatterGatherEntryAddress(int index) const = 0;

  /// Gets the bus address that corresponds to the userspace address + given offset
  virtual uintptr_t getBusOffsetAddress(size_t offset) const = 0;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_DMABUFFERPROVIDER_DMABUFFERPROVIDERINTERFACE_H_
