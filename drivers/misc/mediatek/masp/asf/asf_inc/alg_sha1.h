/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _SHA1_H
#define _SHA1_H

#include "sec_cust_struct.h"

#define SHA1_LEN 20

typedef struct {
	unsigned long to[2];
	unsigned long st[5];
	unsigned char buf[64];
} sha1_ctx;

typedef struct {
	unsigned int state[5];
	unsigned int count[2];
	unsigned char buffer[64];
} SHA1_CTX;

/**************************************************************************
 *  EXPORT FUNCTIONS
 **************************************************************************/
void sha1(const unsigned char *input, int ilen, unsigned char output[20]);

/**************************************************************************
 *  EXPORT VARIABLES
 **************************************************************************/
extern unsigned char sha1sum[HASH_LEN];


#endif
