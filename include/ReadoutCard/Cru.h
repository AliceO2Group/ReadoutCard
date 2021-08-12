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

#ifndef O2_READOUTCARD_INCLUDE_CRU_H_
#define O2_READOUTCARD_INCLUDE_CRU_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <cstdint>

#include "Register.h"

namespace o2
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
static constexpr uint32_t GBT_MUX_TTCUP(0x3);
static constexpr uint32_t GBT_MUX_UL(0x4);

static constexpr uint32_t GBT_MODE_GBT(0x0);
static constexpr uint32_t GBT_MODE_WB(0x1);

static constexpr uint32_t GBT_PACKET(0x1);
static constexpr uint32_t GBT_STREAMING(0x0);

namespace ScRegisters
{
static constexpr Register SC_BASE_INDEX(0x00f00000);

static constexpr Register SCA_WR_DATA(0x00f00000);
static constexpr Register SCA_WR_CMD(0x00f00004);
static constexpr Register SCA_WR_CTRL(0x00f00008);

static constexpr Register SCA_RD_DATA(0x00f00010);
static constexpr Register SCA_RD_CMD(0x00f00014);
static constexpr Register SCA_RD_CTRL(0x00f00018);
static constexpr Register SCA_RD_MON(0x00f0001c);

static constexpr Register SCA_MFT_PSU_DATA(0x00f00000);
static constexpr Register SCA_MFT_PSU_CMD(0x00f00004);
static constexpr Register SCA_MFT_PSU_CTRL(0x00f00008);
static constexpr Register SCA_MFT_PSU_RESET(0x00f0000c);
static constexpr Register SCA_MFT_PSU_MASTER_SLAVE(0x00f0003c);
static constexpr Register SCA_MFT_PSU_ID(0x00f00080);

static constexpr Register SC_LINK(0x00f00078);
static constexpr Register SC_RESET(0x00f0007c);

static constexpr Register SWT_WR_WORD_L(0x00f00040);
static constexpr Register SWT_WR_WORD_M(0x00f00044);
static constexpr Register SWT_WR_WORD_H(0x00f00048);

static constexpr Register SWT_RD_WORD_L(0x00f00050);
static constexpr Register SWT_RD_WORD_M(0x00f00054);
static constexpr Register SWT_RD_WORD_H(0x00f00058);

static constexpr Register SWT_CMD(0x00f0004c);
static constexpr Register SWT_MON(0x00f0005c);
static constexpr Register SWT_WORD_MON(0x00f00060);
} // namespace ScRegisters

} // namespace Cru
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_CRU_H_
