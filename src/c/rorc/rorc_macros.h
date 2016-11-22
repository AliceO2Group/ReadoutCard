/// \file rorc_macros.h
///
/// Private rorc macros

#pragma once

#define incr15(a) (((a) + 1) & 0xf)
#define ST_DEST(fw)  ((unsigned short)( (fw) & 0xf))               // 4 bits  0- 3
#define mask(a,b) ((a) & (b))
#define rorcFWVersMajor(fw) ((fw >> 20) & 0xf)
#define rorcFWVersMinor(fw) ((fw >> 13) & 0x7f)
#define rorcFFSize(fw) ((fw & 0xff000000) >> 18)  /* (x >> 24) * 64 */

