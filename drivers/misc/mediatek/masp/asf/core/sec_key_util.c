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

#include "sec_typedef.h"
#include "sec_boot.h"

/**************************************************************************
 *  DEFINITIONS
 **************************************************************************/
#define MOD                         "SEC_KEY_UTIL"

/**************************************************************************
 *  KEY SECRET
 **************************************************************************/
#define ENCODE_MAGIC                (0x1)

void sec_decode_key(unsigned char *key, unsigned int key_len, unsigned char *seed, unsigned int seed_len)
{
	unsigned int i = 0;

	for (i = 0; i < key_len; i++) {
		key[i] -= seed[i % seed_len];
		key[i] -= ENCODE_MAGIC;
	}
}
