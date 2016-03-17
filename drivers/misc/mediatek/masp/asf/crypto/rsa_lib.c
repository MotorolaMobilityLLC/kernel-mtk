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

#include "sec_osal_light.h"
#include "sec_typedef.h"
#include "rsa_def.h"
#include "alg_sha1.h"
#include "bgn_export.h"
#include "sec_cust_struct.h"
#include "sec_auth.h"
#include "sec_error.h"
#include "sec_rom_info.h"
#include "sec_boot_lib.h"
#include "sec_sign_header.h"
#include "sec_key_util.h"
#include "sec_log.h"

/**************************************************************************
 *  MODULE NAME
 **************************************************************************/
#define MOD                         "AUTHEN"

/**************************************************************************
*  LOCAL VARIABLE
**************************************************************************/
unsigned char bRsaKeyInit = false;
unsigned char bRsaImgKeyInit = false;
CUST_SEC_INTER g_cus_inter;

/**************************************************************************
 *  RSA SML KEY INIT
 **************************************************************************/
int lib_init_key(unsigned char *nKey, unsigned int nKey_len, unsigned char *eKey, unsigned int eKey_len)
{
	int ret = SEC_OK;

	/* ------------------------------ */
	/* avoid re-init aes key
	   if re-init key again, key value will be decoded twice .. */
	/* ------------------------------ */
	if (true == bRsaKeyInit)
		return ret;

	bRsaKeyInit = true;

	if (0 != mcmp(rom_info.m_id, RI_NAME, RI_NAME_LEN)) {
		SMSG(true, "[%s] error. key not found\n", MOD);
		ret = ERR_RSA_KEY_NOT_FOUND;
		goto _end;
	}

	/* ------------------------------ */
	/* clean rsa variable             */
	/* ------------------------------ */
	memset(&rsa, 0, sizeof(rsa_ctx));

	/* ------------------------------ */
	/* init RSA module / exponent key */
	/* ------------------------------ */
	rsa.len = RSA_KEY_SIZE;

	/* ------------------------------ */
	/* decode key                     */
	/* ------------------------------ */
	sec_decode_key(nKey, nKey_len,
		       rom_info.m_SEC_KEY.crypto_seed, sizeof(rom_info.m_SEC_KEY.crypto_seed));

	/* ------------------------------ */
	/* init mpi library               */
	/* ------------------------------ */
	bgn_read_str(&rsa.N, 16, (char *)nKey, nKey_len);
	bgn_read_str(&rsa.E, 16, (char *)eKey, eKey_len);

	/* ------------------------------ */
	/* debugging                      */
	/* ------------------------------ */
	dump_buf(nKey, 0x4);

_end:

	return ret;

}

/**************************************************************************
 *  SIGNING
 **************************************************************************/
int lib_sign(unsigned char *data_buf, unsigned int data_len, unsigned char *sig_buf, unsigned int sig_len)
{
	return 0;
}


/**************************************************************************
 *  HASHING
 **************************************************************************/
int lib_hash(unsigned char *data_buf, unsigned int data_len, unsigned char *hash_buf, unsigned int hash_len)
{
	if (HASH_LEN != hash_len) {
		SMSG(true, "hash length is wrong (%d)\n", hash_len);
		goto _err;
	}

	/* hash the plain text */
	sha1(data_buf, data_len, hash_buf);
	return 0;
_err:
	return -1;
}


/**************************************************************************
 *  VERIFY SIGNATURE
 **************************************************************************/
int lib_verify(unsigned char *data_buf, unsigned int data_len, unsigned char *sig_buf, unsigned int sig_len)
{

	if (RSA_KEY_LEN != sig_len) {
		SMSG(true, "signature length is wrong (%d)\n", sig_len);
		goto _err;
	}

	SMSG(true, "[%s] 0x%x,0x%x,0x%x,0x%x\n", MOD, data_buf[0], data_buf[1], data_buf[2],
	     data_buf[3]);

	/* hash the plain text */
	sha1(data_buf, data_len, sha1sum);

	/* verify this signature */
	SMSG(true, "[%s] verify signature", MOD);
	if (rsa_verify(&rsa, HASH_LEN, sha1sum, sig_buf) != 0) {
		SMSG(true, " ... failed\n");
		goto _err;
	}
	SMSG(true, " ... pass\n");

	return 0;

_err:

	return -1;
}
