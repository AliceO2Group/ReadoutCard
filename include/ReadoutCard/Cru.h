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

#include "Register.h"

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
static constexpr uint32_t GBT_MUX_TTCUP(0x3);
static constexpr uint32_t GBT_MUX_UL(0x4);

static constexpr uint32_t GBT_MODE_GBT(0x0);
static constexpr uint32_t GBT_MODE_WB(0x1);

static constexpr uint32_t GBT_PACKET(0x1);
static constexpr uint32_t GBT_STREAMING(0x0);

namespace ScRegisters
{
static constexpr Register SC_BASE_INDEX(0x00000000);

static constexpr Register SCA_WR_DATA(0x00000000);
static constexpr Register SCA_WR_CMD(0x00000004);
static constexpr Register SCA_WR_CTRL(0x00000008);

static constexpr Register SCA_RD_DATA(0x00000010);
static constexpr Register SCA_RD_CMD(0x00000014);
static constexpr Register SCA_RD_CTRL(0x00000018);
static constexpr Register SCA_RD_MON(0x0000001c); //unused

static constexpr Register SCA_MFT_PSU_DATA(0x00f00000);
static constexpr Register SCA_MFT_PSU_CMD(0x00f00004);
static constexpr Register SCA_MFT_PSU_CTRL(0x00f00008);
static constexpr Register SCA_MFT_PSU_RESET(0x00f0000c);
static constexpr Register SCA_MFT_PSU_MASTER_SLAVE(0x00f0003c);

static constexpr Register SC_LINK(0x00000078);
static constexpr Register SC_RESET(0x0000007c);

static constexpr Register SWT_WR_WORD_L(0x00000040);
static constexpr Register SWT_WR_WORD_M(0x00000044);
static constexpr Register SWT_WR_WORD_H(0x00000048);

static constexpr Register SWT_RD_WORD_L(0x00000050);
static constexpr Register SWT_RD_WORD_M(0x00000054);
static constexpr Register SWT_RD_WORD_H(0x00000058);

static constexpr Register SWT_CMD(0x0000004c); //unused
static constexpr Register SWT_MON(0x0000005c);
static constexpr Register SWT_WORD_MON(0x00000060); //unused

static constexpr Register IC_WR_CFG(0x00000024);
static constexpr Register IC_WR_DATA(0x00000020);
static constexpr Register IC_WR_CMD(0x00000028);
static constexpr Register IC_RD_DATA(0x00000030);
} // namespace ScRegisters

} // namespace Cru
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_READOUTCARD_CRU_H_
