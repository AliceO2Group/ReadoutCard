/// \file PdaDmaBufferProvider.h
/// \brief Definition of the PdaDmaBufferProvider class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_DMABUFFERPROVIDER_PDADMABUFFERPROVIDER_H_
#define ALICEO2_SRC_READOUTCARD_DMABUFFERPROVIDER_PDADMABUFFERPROVIDER_H_

#include <cstddef>
#include <cstdint>
#include <vector>
#include "DmaBufferProvider/DmaBufferProviderInterface.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaDmaBuffer.h"

namespace AliceO2 {
namespace roc {

/// Implementation of the DmaBufferProviderInterface for in-memory DMA buffers registered with PDA
class PdaDmaBufferProvider : public DmaBufferProviderInterface
{
  public:
    PdaDmaBufferProvider(Pda::PdaDevice::PdaPciDevice pciDevice, void* userBufferAddress, size_t userBufferSize,
        int dmaBufferId, bool requireHugepage)
        : mAddress(userBufferAddress), mSize(userBufferSize), mPdaBuffer(pciDevice, userBufferAddress, userBufferSize,
            dmaBufferId, requireHugepage)
    {

    }

    virtual ~PdaDmaBufferProvider() = default;

    /// Get starting userspace address of the DMA buffer
    virtual uintptr_t getAddress() const
    {
      return reinterpret_cast<uintptr_t>(mAddress);
    }

    /// Get total size of the DMA buffer
    virtual size_t getSize() const
    {
      return mSize;
    }

    /// Amount of entries in the scatter-gather list
    virtual size_t getScatterGatherListSize() const
    {
      return mPdaBuffer.getScatterGatherList().size();
    }

    /// Get size of an entry of the scatter-gather list
    virtual size_t getScatterGatherEntrySize(int index) const
    {
      return mPdaBuffer.getScatterGatherList().at(index).size;
    }

    /// Get userspace address of an entry of the scatter-gather list
    virtual uintptr_t getScatterGatherEntryAddress(int index) const
    {
      return mPdaBuffer.getScatterGatherList().at(index).addressUser;
    }

    /// Function for getting the bus address that corresponds to the user address + given offset
    virtual uintptr_t getBusOffsetAddress(size_t offset) const
    {
      return mPdaBuffer.getBusOffsetAddress(offset);
    }

  private:
    void* mAddress;
    size_t mSize;
    Pda::PdaDmaBuffer mPdaBuffer;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_DMABUFFERPROVIDER_PDADMABUFFERPROVIDER_H_