
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
/// \file StWord.h
/// \brief Definition of the StWord union
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_CRORC_STWORD_H_
#define O2_READOUTCARD_SRC_CRORC_STWORD_H_

#include <cstdint>

union StWord {
  struct {
    unsigned char dest : 4;
    unsigned char code : 4;
    unsigned char trid : 4;
    unsigned long param : 19;
    unsigned char error : 1;
  } part;
  uint32_t stw;
};

#endif // O2_READOUTCARD_SRC_CRORC_STWORD_H_
