// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Si5344-RevD-TFC_40-Registers.h
/// \brief Definition of register map needed for CRU configuration
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

std::vector<std::pair<uint32_t, uint32_t>> getSi5344RegisterMap()
{
  std::vector<std::pair<uint32_t, uint32_t>> registerMap = {
    /*# Si538x/4x Registers Script
# 
# Part: Si5344
# Project File: C:\Users\cachemi\Desktop\PLLs\PCIe40_LLI_programming_data\tfc_si5344_input0_40MHz\Si5344-RevD-TFC_40-Project.slabtimeproj
# Design ID: TFC_40
# Includes Pre/Post Download Control Register Writes: Yes
# Die Revision: B1
# Creator: ClockBuilder Pro v2.20 [2017-11-21]
# Created On: 2018-01-20 22:52:21 GMT+01:00
    Address,Data
# 
# Start configuration preamble*/
    std::make_pair(0x0B24, 0xC0),
    std::make_pair(0x0B25, 0x00),
    std::make_pair(0x0540, 0x01),
    /*# End configuration preamble
# 
# Delay 300 msec
#    Delay is worst case time for device to complete any calibration
#    that is running due to device state change previous to this script
#    being processed.
# 
# Start configuration registers*/
    std::make_pair(0x0006, 0x00),
    std::make_pair(0x0007, 0x00),
    std::make_pair(0x0008, 0x00),
    std::make_pair(0x000B, 0x68),
    std::make_pair(0x0016, 0x02),
    std::make_pair(0x0017, 0xDC),
    std::make_pair(0x0018, 0xEE),
    std::make_pair(0x0019, 0xDD),
    std::make_pair(0x001A, 0xDF),
    std::make_pair(0x002B, 0x02),
    std::make_pair(0x002C, 0x01),
    std::make_pair(0x002D, 0x01),
    std::make_pair(0x002E, 0x44),
    std::make_pair(0x002F, 0x00),
    std::make_pair(0x0030, 0x00),
    std::make_pair(0x0031, 0x00),
    std::make_pair(0x0032, 0x00),
    std::make_pair(0x0033, 0x00),
    std::make_pair(0x0034, 0x00),
    std::make_pair(0x0035, 0x00),
    std::make_pair(0x0036, 0x44),
    std::make_pair(0x0037, 0x00),
    std::make_pair(0x0038, 0x00),
    std::make_pair(0x0039, 0x00),
    std::make_pair(0x003A, 0x00),
    std::make_pair(0x003B, 0x00),
    std::make_pair(0x003C, 0x00),
    std::make_pair(0x003D, 0x00),
    std::make_pair(0x003F, 0x11),
    std::make_pair(0x0040, 0x04),
    std::make_pair(0x0041, 0x0C),
    std::make_pair(0x0042, 0x00),
    std::make_pair(0x0043, 0x00),
    std::make_pair(0x0044, 0x00),
    std::make_pair(0x0045, 0x0C),
    std::make_pair(0x0046, 0x32),
    std::make_pair(0x0047, 0x00),
    std::make_pair(0x0048, 0x00),
    std::make_pair(0x0049, 0x00),
    std::make_pair(0x004A, 0x32),
    std::make_pair(0x004B, 0x00),
    std::make_pair(0x004C, 0x00),
    std::make_pair(0x004D, 0x00),
    std::make_pair(0x004E, 0x05),
    std::make_pair(0x004F, 0x00),
    std::make_pair(0x0050, 0x0F),
    std::make_pair(0x0051, 0x03),
    std::make_pair(0x0052, 0x00),
    std::make_pair(0x0053, 0x00),
    std::make_pair(0x0054, 0x00),
    std::make_pair(0x0055, 0x03),
    std::make_pair(0x0056, 0x00),
    std::make_pair(0x0057, 0x00),
    std::make_pair(0x0058, 0x00),
    std::make_pair(0x0059, 0x01),
    std::make_pair(0x005A, 0xF0),
    std::make_pair(0x005B, 0x00),
    std::make_pair(0x005C, 0xBE),
    std::make_pair(0x005D, 0x00),
    std::make_pair(0x005E, 0x00),
    std::make_pair(0x005F, 0x00),
    std::make_pair(0x0060, 0x00),
    std::make_pair(0x0061, 0x00),
    std::make_pair(0x0062, 0x00),
    std::make_pair(0x0063, 0x00),
    std::make_pair(0x0064, 0x00),
    std::make_pair(0x0065, 0x00),
    std::make_pair(0x0066, 0x00),
    std::make_pair(0x0067, 0x00),
    std::make_pair(0x0068, 0x00),
    std::make_pair(0x0069, 0x00),
    std::make_pair(0x0092, 0x02),
    std::make_pair(0x0093, 0xA0),
    std::make_pair(0x0095, 0x00),
    std::make_pair(0x0096, 0x80),
    std::make_pair(0x0098, 0x60),
    std::make_pair(0x009A, 0x02),
    std::make_pair(0x009B, 0x60),
    std::make_pair(0x009D, 0x08),
    std::make_pair(0x009E, 0x40),
    std::make_pair(0x00A0, 0x20),
    std::make_pair(0x00A2, 0x00),
    std::make_pair(0x00A9, 0xB5),
    std::make_pair(0x00AA, 0x61),
    std::make_pair(0x00AB, 0x00),
    std::make_pair(0x00AC, 0x00),
    std::make_pair(0x00E5, 0x21),
    std::make_pair(0x00EA, 0x0A),
    std::make_pair(0x00EB, 0x60),
    std::make_pair(0x00EC, 0x00),
    std::make_pair(0x00ED, 0x00),
    std::make_pair(0x0102, 0x01),
    std::make_pair(0x0112, 0x06),
    std::make_pair(0x0113, 0x09),
    std::make_pair(0x0114, 0x3E),
    std::make_pair(0x0115, 0x18),
    std::make_pair(0x0117, 0x06),
    std::make_pair(0x0118, 0x09),
    std::make_pair(0x0119, 0x3B),
    std::make_pair(0x011A, 0x28),
    std::make_pair(0x0126, 0x06),
    std::make_pair(0x0127, 0x09),
    std::make_pair(0x0128, 0x3B),
    std::make_pair(0x0129, 0x28),
    std::make_pair(0x012B, 0x06),
    std::make_pair(0x012C, 0x09),
    std::make_pair(0x012D, 0x3B),
    std::make_pair(0x012E, 0x28),
    std::make_pair(0x013F, 0x00),
    std::make_pair(0x0140, 0x00),
    std::make_pair(0x0141, 0x40),
    std::make_pair(0x0142, 0xFF),
    std::make_pair(0x0206, 0x00),
    std::make_pair(0x0208, 0x19),
    std::make_pair(0x0209, 0x00),
    std::make_pair(0x020A, 0x00),
    std::make_pair(0x020B, 0x00),
    std::make_pair(0x020C, 0x00),
    std::make_pair(0x020D, 0x00),
    std::make_pair(0x020E, 0x01),
    std::make_pair(0x020F, 0x00),
    std::make_pair(0x0210, 0x00),
    std::make_pair(0x0211, 0x00),
    std::make_pair(0x0212, 0x00),
    std::make_pair(0x0213, 0x00),
    std::make_pair(0x0214, 0x00),
    std::make_pair(0x0215, 0x00),
    std::make_pair(0x0216, 0x00),
    std::make_pair(0x0217, 0x00),
    std::make_pair(0x0218, 0x00),
    std::make_pair(0x0219, 0x00),
    std::make_pair(0x021A, 0x00),
    std::make_pair(0x021B, 0x00),
    std::make_pair(0x021C, 0x00),
    std::make_pair(0x021D, 0x00),
    std::make_pair(0x021E, 0x00),
    std::make_pair(0x021F, 0x00),
    std::make_pair(0x0220, 0x00),
    std::make_pair(0x0221, 0x00),
    std::make_pair(0x0222, 0x00),
    std::make_pair(0x0223, 0x00),
    std::make_pair(0x0224, 0x00),
    std::make_pair(0x0225, 0x00),
    std::make_pair(0x0226, 0x00),
    std::make_pair(0x0227, 0x00),
    std::make_pair(0x0228, 0x00),
    std::make_pair(0x0229, 0x00),
    std::make_pair(0x022A, 0x00),
    std::make_pair(0x022B, 0x00),
    std::make_pair(0x022C, 0x00),
    std::make_pair(0x022D, 0x00),
    std::make_pair(0x022E, 0x00),
    std::make_pair(0x022F, 0x00),
    std::make_pair(0x0231, 0x0B),
    std::make_pair(0x0232, 0x0B),
    std::make_pair(0x0233, 0x0B),
    std::make_pair(0x0234, 0x0B),
    std::make_pair(0x0235, 0x00),
    std::make_pair(0x0236, 0x00),
    std::make_pair(0x0237, 0x4C),
    std::make_pair(0x0238, 0x3C),
    std::make_pair(0x0239, 0xAB),
    std::make_pair(0x023A, 0x00),
    std::make_pair(0x023B, 0x00),
    std::make_pair(0x023C, 0x00),
    std::make_pair(0x023D, 0xC8),
    std::make_pair(0x023E, 0xAF),
    std::make_pair(0x0250, 0x00),
    std::make_pair(0x0251, 0x00),
    std::make_pair(0x0252, 0x00),
    std::make_pair(0x0253, 0x00),
    std::make_pair(0x0254, 0x00),
    std::make_pair(0x0255, 0x00),
    std::make_pair(0x025C, 0x00),
    std::make_pair(0x025D, 0x00),
    std::make_pair(0x025E, 0x00),
    std::make_pair(0x025F, 0x00),
    std::make_pair(0x0260, 0x00),
    std::make_pair(0x0261, 0x00),
    std::make_pair(0x026B, 0x54),
    std::make_pair(0x026C, 0x46),
    std::make_pair(0x026D, 0x43),
    std::make_pair(0x026E, 0x5F),
    std::make_pair(0x026F, 0x34),
    std::make_pair(0x0270, 0x30),
    std::make_pair(0x0271, 0x00),
    std::make_pair(0x0272, 0x00),
    std::make_pair(0x028A, 0x00),
    std::make_pair(0x028B, 0x00),
    std::make_pair(0x028C, 0x00),
    std::make_pair(0x028D, 0x00),
    std::make_pair(0x028E, 0x00),
    std::make_pair(0x028F, 0x00),
    std::make_pair(0x0290, 0x00),
    std::make_pair(0x0291, 0x00),
    std::make_pair(0x0294, 0xB0),
    std::make_pair(0x0296, 0x02),
    std::make_pair(0x0297, 0x02),
    std::make_pair(0x0299, 0x02),
    std::make_pair(0x029D, 0xFA),
    std::make_pair(0x029E, 0x01),
    std::make_pair(0x029F, 0x00),
    std::make_pair(0x02A9, 0xCC),
    std::make_pair(0x02AA, 0x04),
    std::make_pair(0x02AB, 0x00),
    std::make_pair(0x02B7, 0xFF),
    std::make_pair(0x0302, 0x00),
    std::make_pair(0x0303, 0x00),
    std::make_pair(0x0304, 0x00),
    std::make_pair(0x0305, 0x00),
    std::make_pair(0x0306, 0x0E),
    std::make_pair(0x0307, 0x00),
    std::make_pair(0x0308, 0x00),
    std::make_pair(0x0309, 0x00),
    std::make_pair(0x030A, 0x00),
    std::make_pair(0x030B, 0x80),
    std::make_pair(0x030C, 0x00),
    std::make_pair(0x030D, 0x00),
    std::make_pair(0x030E, 0x00),
    std::make_pair(0x030F, 0x00),
    std::make_pair(0x0310, 0x00),
    std::make_pair(0x0311, 0x00),
    std::make_pair(0x0312, 0x00),
    std::make_pair(0x0313, 0x00),
    std::make_pair(0x0314, 0x00),
    std::make_pair(0x0315, 0x00),
    std::make_pair(0x0316, 0x00),
    std::make_pair(0x0317, 0x00),
    std::make_pair(0x0318, 0x00),
    std::make_pair(0x0319, 0x00),
    std::make_pair(0x031A, 0x00),
    std::make_pair(0x031B, 0x00),
    std::make_pair(0x031C, 0x00),
    std::make_pair(0x031D, 0x00),
    std::make_pair(0x031E, 0x00),
    std::make_pair(0x031F, 0x00),
    std::make_pair(0x0320, 0x00),
    std::make_pair(0x0321, 0x00),
    std::make_pair(0x0322, 0x00),
    std::make_pair(0x0323, 0x00),
    std::make_pair(0x0324, 0x00),
    std::make_pair(0x0325, 0x00),
    std::make_pair(0x0326, 0x00),
    std::make_pair(0x0327, 0x00),
    std::make_pair(0x0328, 0x00),
    std::make_pair(0x0329, 0x00),
    std::make_pair(0x032A, 0x00),
    std::make_pair(0x032B, 0x00),
    std::make_pair(0x032C, 0x00),
    std::make_pair(0x032D, 0x00),
    std::make_pair(0x0338, 0x00),
    std::make_pair(0x0339, 0x1F),
    std::make_pair(0x033B, 0x00),
    std::make_pair(0x033C, 0x00),
    std::make_pair(0x033D, 0x00),
    std::make_pair(0x033E, 0x00),
    std::make_pair(0x033F, 0x00),
    std::make_pair(0x0340, 0x00),
    std::make_pair(0x0341, 0x00),
    std::make_pair(0x0342, 0x00),
    std::make_pair(0x0343, 0x00),
    std::make_pair(0x0344, 0x00),
    std::make_pair(0x0345, 0x00),
    std::make_pair(0x0346, 0x00),
    std::make_pair(0x0347, 0x00),
    std::make_pair(0x0348, 0x00),
    std::make_pair(0x0349, 0x00),
    std::make_pair(0x034A, 0x00),
    std::make_pair(0x034B, 0x00),
    std::make_pair(0x034C, 0x00),
    std::make_pair(0x034D, 0x00),
    std::make_pair(0x034E, 0x00),
    std::make_pair(0x034F, 0x00),
    std::make_pair(0x0350, 0x00),
    std::make_pair(0x0351, 0x00),
    std::make_pair(0x0352, 0x00),
    std::make_pair(0x0359, 0x00),
    std::make_pair(0x035A, 0x00),
    std::make_pair(0x035B, 0x00),
    std::make_pair(0x035C, 0x00),
    std::make_pair(0x035D, 0x00),
    std::make_pair(0x035E, 0x00),
    std::make_pair(0x035F, 0x00),
    std::make_pair(0x0360, 0x00),
    std::make_pair(0x0487, 0x00),
    std::make_pair(0x0508, 0x13),
    std::make_pair(0x0509, 0x22),
    std::make_pair(0x050A, 0x0C),
    std::make_pair(0x050B, 0x0B),
    std::make_pair(0x050C, 0x07),
    std::make_pair(0x050D, 0x3F),
    std::make_pair(0x050E, 0x16),
    std::make_pair(0x050F, 0x2A),
    std::make_pair(0x0510, 0x09),
    std::make_pair(0x0511, 0x08),
    std::make_pair(0x0512, 0x07),
    std::make_pair(0x0513, 0x3F),
    std::make_pair(0x0515, 0x00),
    std::make_pair(0x0516, 0x00),
    std::make_pair(0x0517, 0x00),
    std::make_pair(0x0518, 0x00),
    std::make_pair(0x0519, 0x48),
    std::make_pair(0x051A, 0x03),
    std::make_pair(0x051B, 0x00),
    std::make_pair(0x051C, 0x00),
    std::make_pair(0x051D, 0x00),
    std::make_pair(0x051E, 0x00),
    std::make_pair(0x051F, 0x80),
    std::make_pair(0x0521, 0x2B),
    std::make_pair(0x052A, 0x01),
    std::make_pair(0x052B, 0x01),
    std::make_pair(0x052C, 0x87),
    std::make_pair(0x052D, 0x03),
    std::make_pair(0x052E, 0x19),
    std::make_pair(0x052F, 0x19),
    std::make_pair(0x0531, 0x00),
    std::make_pair(0x0532, 0x10),
    std::make_pair(0x0533, 0x04),
    std::make_pair(0x0534, 0x00),
    std::make_pair(0x0535, 0x00),
    std::make_pair(0x0536, 0x04),
    std::make_pair(0x0537, 0x00),
    std::make_pair(0x0538, 0x00),
    std::make_pair(0x0539, 0x00),
    std::make_pair(0x053A, 0x02),
    std::make_pair(0x053B, 0x03),
    std::make_pair(0x053C, 0x00),
    std::make_pair(0x053D, 0x11),
    std::make_pair(0x053E, 0x06),
    std::make_pair(0x0589, 0x0E),
    std::make_pair(0x058A, 0x00),
    std::make_pair(0x059B, 0xFA),
    std::make_pair(0x059D, 0x13),
    std::make_pair(0x059E, 0x24),
    std::make_pair(0x059F, 0x0C),
    std::make_pair(0x05A0, 0x0B),
    std::make_pair(0x05A1, 0x07),
    std::make_pair(0x05A2, 0x3F),
    std::make_pair(0x05A6, 0x0B),
    std::make_pair(0x0802, 0x35),
    std::make_pair(0x0803, 0x05),
    std::make_pair(0x0804, 0x00),
    std::make_pair(0x0805, 0x00),
    std::make_pair(0x0806, 0x00),
    std::make_pair(0x0807, 0x00),
    std::make_pair(0x0808, 0x00),
    std::make_pair(0x0809, 0x00),
    std::make_pair(0x080A, 0x00),
    std::make_pair(0x080B, 0x00),
    std::make_pair(0x080C, 0x00),
    std::make_pair(0x080D, 0x00),
    std::make_pair(0x080E, 0x00),
    std::make_pair(0x080F, 0x00),
    std::make_pair(0x0810, 0x00),
    std::make_pair(0x0811, 0x00),
    std::make_pair(0x0812, 0x00),
    std::make_pair(0x0813, 0x00),
    std::make_pair(0x0814, 0x00),
    std::make_pair(0x0815, 0x00),
    std::make_pair(0x0816, 0x00),
    std::make_pair(0x0817, 0x00),
    std::make_pair(0x0818, 0x00),
    std::make_pair(0x0819, 0x00),
    std::make_pair(0x081A, 0x00),
    std::make_pair(0x081B, 0x00),
    std::make_pair(0x081C, 0x00),
    std::make_pair(0x081D, 0x00),
    std::make_pair(0x081E, 0x00),
    std::make_pair(0x081F, 0x00),
    std::make_pair(0x0820, 0x00),
    std::make_pair(0x0821, 0x00),
    std::make_pair(0x0822, 0x00),
    std::make_pair(0x0823, 0x00),
    std::make_pair(0x0824, 0x00),
    std::make_pair(0x0825, 0x00),
    std::make_pair(0x0826, 0x00),
    std::make_pair(0x0827, 0x00),
    std::make_pair(0x0828, 0x00),
    std::make_pair(0x0829, 0x00),
    std::make_pair(0x082A, 0x00),
    std::make_pair(0x082B, 0x00),
    std::make_pair(0x082C, 0x00),
    std::make_pair(0x082D, 0x00),
    std::make_pair(0x082E, 0x00),
    std::make_pair(0x082F, 0x00),
    std::make_pair(0x0830, 0x00),
    std::make_pair(0x0831, 0x00),
    std::make_pair(0x0832, 0x00),
    std::make_pair(0x0833, 0x00),
    std::make_pair(0x0834, 0x00),
    std::make_pair(0x0835, 0x00),
    std::make_pair(0x0836, 0x00),
    std::make_pair(0x0837, 0x00),
    std::make_pair(0x0838, 0x00),
    std::make_pair(0x0839, 0x00),
    std::make_pair(0x083A, 0x00),
    std::make_pair(0x083B, 0x00),
    std::make_pair(0x083C, 0x00),
    std::make_pair(0x083D, 0x00),
    std::make_pair(0x083E, 0x00),
    std::make_pair(0x083F, 0x00),
    std::make_pair(0x0840, 0x00),
    std::make_pair(0x0841, 0x00),
    std::make_pair(0x0842, 0x00),
    std::make_pair(0x0843, 0x00),
    std::make_pair(0x0844, 0x00),
    std::make_pair(0x0845, 0x00),
    std::make_pair(0x0846, 0x00),
    std::make_pair(0x0847, 0x00),
    std::make_pair(0x0848, 0x00),
    std::make_pair(0x0849, 0x00),
    std::make_pair(0x084A, 0x00),
    std::make_pair(0x084B, 0x00),
    std::make_pair(0x084C, 0x00),
    std::make_pair(0x084D, 0x00),
    std::make_pair(0x084E, 0x00),
    std::make_pair(0x084F, 0x00),
    std::make_pair(0x0850, 0x00),
    std::make_pair(0x0851, 0x00),
    std::make_pair(0x0852, 0x00),
    std::make_pair(0x0853, 0x00),
    std::make_pair(0x0854, 0x00),
    std::make_pair(0x0855, 0x00),
    std::make_pair(0x0856, 0x00),
    std::make_pair(0x0857, 0x00),
    std::make_pair(0x0858, 0x00),
    std::make_pair(0x0859, 0x00),
    std::make_pair(0x085A, 0x00),
    std::make_pair(0x085B, 0x00),
    std::make_pair(0x085C, 0x00),
    std::make_pair(0x085D, 0x00),
    std::make_pair(0x085E, 0x00),
    std::make_pair(0x085F, 0x00),
    std::make_pair(0x0860, 0x00),
    std::make_pair(0x0861, 0x00),
    std::make_pair(0x090E, 0x02),
    std::make_pair(0x0943, 0x00),
    std::make_pair(0x0949, 0x01),
    std::make_pair(0x094A, 0x01),
    std::make_pair(0x094E, 0x49),
    std::make_pair(0x094F, 0x02),
    std::make_pair(0x095E, 0x00),
    std::make_pair(0x0A02, 0x00),
    std::make_pair(0x0A03, 0x01),
    std::make_pair(0x0A04, 0x01),
    std::make_pair(0x0A05, 0x01),
    std::make_pair(0x0A14, 0x00),
    std::make_pair(0x0A1A, 0x00),
    std::make_pair(0x0A20, 0x00),
    std::make_pair(0x0A26, 0x00),
    std::make_pair(0x0B44, 0x2F),
    std::make_pair(0x0B46, 0x00),
    std::make_pair(0x0B47, 0x0E),
    std::make_pair(0x0B48, 0x0E),
    std::make_pair(0x0B4A, 0x0E),
    std::make_pair(0x0B57, 0xF0),
    std::make_pair(0x0B58, 0x00),
    /*# End configuration registers
# 
# Start configuration postamble*/
    std::make_pair(0x0514, 0x01),
    std::make_pair(0x001C, 0x01),
    std::make_pair(0x0540, 0x00),
    std::make_pair(0x0B24, 0xC3),
    std::make_pair(0x0B25, 0x02)
    /*# End configuration postamble*/
  };
  return registerMap;
}
