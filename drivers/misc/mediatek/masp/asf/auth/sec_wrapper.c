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

#include "sec_wrapper.h"
#include "sec_auth.h"

int sec_init_key(unsigned char *nKey, unsigned int nKey_len,
		 unsigned char *eKey, unsigned int eKey_len)
{
	return lib_init_key(nKey, nKey_len, eKey, eKey_len);
}

int sec_hash(unsigned char *data_buf, unsigned int data_len,
	     unsigned char *hash_buf, unsigned int hash_len)
{
	return lib_hash(data_buf, data_len, hash_buf, hash_len);
}

int sec_verify(unsigned char *data_buf, unsigned int data_len,
	       unsigned char *sig_buf, unsigned int sig_len)
{
	return lib_verify(data_buf, data_len, sig_buf, sig_len);
}
