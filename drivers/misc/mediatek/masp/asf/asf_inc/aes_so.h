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

#ifndef H_AES_BROM
#define H_AES_BROM

#define AES_ENCRYPT     1
#define AES_DECRYPT     0

typedef struct {
	int nr;
	unsigned long *rk;
	unsigned long buf[68];
} a_ctx;

/**************************************************************************
 *  EXPORT FUNCTION
 **************************************************************************/
extern int aes_so_enc(unsigned char *in_buf, unsigned int in_len, unsigned char *out_buf,
		      unsigned int out_len);
extern int aes_so_dec(unsigned char *in_buf, unsigned int in_len, unsigned char *out_buf,
		      unsigned int out_len);
extern int aes_so_init_key(unsigned char *key_buf, unsigned int key_len);
extern int aes_so_init_vector(void);

#endif
