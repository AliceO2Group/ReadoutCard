/// \file Cru/DataFormat.h
/// \brief Definitions of CRU data format functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_DATAFORMAT
#define ALICEO2_READOUTCARD_CRU_DATAFORMAT

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Utilities/Util.h"

namespace AliceO2
{
namespace roc
{
namespace Cru
{
namespace DataFormat
{
namespace
{
  uint32_t getWord(const char* data, int i)
  {
    uint32_t word = 0;
    memcpy(&word, &data[sizeof(word)*i], sizeof(word));
    return word;
  }
} // Anonymous namespace

uint32_t getLinkId(const char* data)
{
  return Utilities::getBits(getWord(data, 2), 4, 4+8-1);
}

uint32_t getEventSize(const char* data)
{
  return Utilities::getBits(getWord(data, 3), 4, 4+12-1);
}

} // namespace DataFormat
} // namespace Cru
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_DATAFORMAT
