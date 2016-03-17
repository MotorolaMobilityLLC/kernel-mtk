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

#ifndef _CIPHERIMG_H
#define _CIPHERIMG_H

/**************************************************************************
 *  AES
 **************************************************************************/
typedef enum {
	AES_VER_LEGACY = 0,
	AES_VER_SO
} AES_VER;

/**************************************************************************
 *  EXPORTED FUNCTIONS
 **************************************************************************/
extern int sec_aes_init(void);
extern int lib_aes_enc(unsigned char *input_buf, unsigned int input_len, unsigned char *output_buf,
		       unsigned int output_len);
extern int lib_aes_dec(unsigned char *input_buf, unsigned int input_len, unsigned char *output_buf,
		       unsigned int output_len);
extern int lib_aes_init_key(unsigned char *key_buf, unsigned int key_len, AES_VER ver);
extern int lib_aes_init_vector(AES_VER ver);


#endif	 /*_CIPHERIMG_H*/
