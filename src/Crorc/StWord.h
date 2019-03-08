// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file StWord.h
/// \brief Definition of the StWord union
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_CRORC_STWORD_H_
#define ALICEO2_SRC_READOUTCARD_CRORC_STWORD_H_

#include <cstdint>

union StWord {
  struct {
    unsigned char dest   :  4;
    unsigned char code   :  4;
    unsigned char trid   :  4;
    unsigned long param  : 19;
    unsigned char error  :  1;
  } part;
  uint32_t stw;
};

#endif // ALICEO2_SRC_READOUTCARD_CRORC_STWORD_H_
