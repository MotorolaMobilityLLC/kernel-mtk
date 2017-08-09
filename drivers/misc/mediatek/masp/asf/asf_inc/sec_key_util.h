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

#ifndef SEC_KEY_UTIL_H
#define SEC_KEY_UTIL_H

/**************************************************************************
 * EXPORT FUNCTION
 **************************************************************************/
extern void sec_decode_key(unsigned char *key, unsigned int key_len, unsigned char *seed,
			   unsigned int seed_len);

#endif				/* SEC_KEY_UTIL_H */
