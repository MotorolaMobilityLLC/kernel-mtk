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

#ifndef SEC_AUTH_H
#define SEC_AUTH_H

/**************************************************************************
 * AUTH DATA STRUCTURE
**************************************************************************/
#define SIGNATURE_SIZE                      (128)
#define RSA_KEY_SIZE                        (128)

typedef struct {
	unsigned char content[SIGNATURE_SIZE];
} _signature;

/**************************************************************************
*  EXPORT FUNCTION
**************************************************************************/
extern int lib_init_key(unsigned char *nKey, unsigned int nKey_len, unsigned char *eKey,
			unsigned int eKey_len);
extern int lib_sign(unsigned char *data_buf, unsigned int data_len, unsigned char *sig_buf, unsigned int sig_len);
extern int lib_hash(unsigned char *data_buf, unsigned int data_len, unsigned char *hash_buf, unsigned int hash_len);
extern int lib_verify(unsigned char *data_buf, unsigned int data_len, unsigned char *sig_buf,
		      unsigned int sig_len);
extern int sec_auth_test(void);

#endif				/* SEC_AUTH_H */
