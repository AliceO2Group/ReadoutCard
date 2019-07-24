// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Cru.h
/// \brief Definitions of CRU related constants
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_INCLUDE_READOUTCARD_CRU_H_
#define ALICEO2_INCLUDE_READOUTCARD_CRU_H_

#include <cstdint>

namespace AliceO2
{
namespace roc
{
namespace Cru
{

static constexpr uint32_t CLOCK_TTC(0x0);
static constexpr uint32_t CLOCK_LOCAL(0x2);

static constexpr uint32_t DATA_CTP(0x0);
static constexpr uint32_t DATA_PATTERN(0x1);
static constexpr uint32_t DATA_MIDTRG(0x2);

static constexpr uint32_t GBT_MUX_TTC(0x0);
static constexpr uint32_t GBT_MUX_DDG(0x1);
static constexpr uint32_t GBT_MUX_SWT(0x2);

static constexpr uint32_t GBT_MODE_GBT(0x0);
static constexpr uint32_t GBT_MODE_WB(0x1);

static constexpr uint32_t GBT_PACKET(0x1);
static constexpr uint32_t GBT_CONTINUOUS(0x0);

} // namespace Cru
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CRU_H_
