/// \file Cru.h
/// \brief Definitions of CRU related constants
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CRU_H_
#define ALICEO2_INCLUDE_READOUTCARD_CRU_H_

#include <cstdint>

namespace AliceO2 {
namespace roc {
namespace Cru {

static constexpr uint32_t CLOCK_TTC(0x0);
static constexpr uint32_t CLOCK_LOCAL(0x2);

static constexpr uint32_t DATA_CTP(0x0);
static constexpr uint32_t DATA_PATTERN(0x1);
static constexpr uint32_t DATA_MIDTRG(0x2);

static constexpr uint32_t GBT_MUX_TTC(0x0);
static constexpr uint32_t GBT_MUX_DDG(0x1);
static constexpr uint32_t GBT_MUX_SC(0x2);

static constexpr uint32_t GBT_MODE_GBT(0x0);
static constexpr uint32_t GBT_MODE_WB(0x1);

static constexpr uint32_t GBT_PACKET(0x1);
static constexpr uint32_t GBT_CONTINUOUS(0x0);

} // namespace Cru
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CRU_H_
