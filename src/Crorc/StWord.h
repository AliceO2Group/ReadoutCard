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