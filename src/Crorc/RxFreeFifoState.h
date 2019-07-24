// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file RxFreeFifoState.h
/// \brief Definition of the RxFreeFifoState enum
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_CRORC_RXFREEFIFOSTATE_H_
#define ALICEO2_SRC_READOUTCARD_CRORC_RXFREEFIFOSTATE_H_

enum class RxFreeFifoState {
  EMPTY,
  NOT_EMPTY,
  FULL
};

#endif // ALICEO2_SRC_READOUTCARD_CRORC_RXFREEFIFOSTATE_H_
