#pragma once

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "stdint.h"
#include "linux/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void elapsed(struct timeval *tv2, struct timeval *tv1, int *dsec, int *dusec);
int roundPowerOf2(int number);
int logi2(unsigned int number);
int trim(char *string);

#ifdef __cplusplus
} /* extern "C" */
#endif

