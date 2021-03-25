// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file NullDmaBufferProvider.h
/// \brief Definition of the NullDmaBufferProvider.h class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_DMABUFFERPROVIDER_NULLDMABUFFERPROVIDER_H_
#define O2_READOUTCARD_SRC_DMABUFFERPROVIDER_NULLDMABUFFERPROVIDER_H_

#include "DmaBufferProvider/DmaBufferProviderInterface.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include "ExceptionInternal.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaDmaBuffer.h"

namespace o2
{
namespace roc
{

/// Implementation of the DmaBufferProviderInterface for a zero-size "null" or "dummy" buffer
class NullDmaBufferProvider : public DmaBufferProviderInterface
{
 public:
  virtual ~NullDmaBufferProvider() = default;

  /// Get starting userspace address of the DMA buffer
  virtual uintptr_t getAddress() const
  {
    return 0;
  }

  /// Get total size of the DMA buffer
  virtual size_t getSize() const
  {
    return 0;
  }

  /// Amount of entries in the scatter-gather list
  virtual size_t getScatterGatherListSize() const
  {
    return 0;
  }

  /// Get size of an entry of the scatter-gather list
  virtual size_t getScatterGatherEntrySize(int) const
  {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("No scatter-gather list provided"));
  }

  /// Get userspace address of an entry of the scatter-gather list
  virtual uintptr_t getScatterGatherEntryAddress(int) const
  {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("No scatter-gather list provided"));
  }

  /// Function for getting the bus address that corresponds to the user address + given offset
  virtual uintptr_t getBusOffsetAddress(size_t) const
  {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("No bus addresses provided"));
  }
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_DMABUFFERPROVIDER_NULLDMABUFFERPROVIDER_H_
