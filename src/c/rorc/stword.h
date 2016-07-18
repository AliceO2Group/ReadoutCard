#pragma once

typedef union {
  struct {
    unsigned char dest   :  4;
    unsigned char code   :  4;
    unsigned char trid   :  4;
    unsigned long param  : 19;
    unsigned char error  :  1;
  } part;
  unsigned long stw;
} stword_t;

