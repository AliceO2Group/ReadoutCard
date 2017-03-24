#pragma once

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

