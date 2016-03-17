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
#include "sec_aes.h"
#include "sec_cipher_header.h"
#include "sec_typedef.h"
#include "sec_error.h"
#include "sec_rom_info.h"
#include "sec_secroimg.h"
#include "sec_key_util.h"
/* v1. legacy aes. W1128/32 MP */
#include "aes_legacy.h"
/* v2. so aes. W1150 MP */
#include "aes_so.h"
#include "sec_boot_lib.h"

/**************************************************************************
 *  DEFINITIONS
 **************************************************************************/
#define MOD                         "SEC AES"
#define SML_SCRAMBLE_SEED           "78ABD4569EA41795"

/**************************************************************************
*  DEBUG CONTROL
**************************************************************************/
#define SEC_AES_DEBUG_LOG           (0)
/*#define SMSG                        printk*/

/**************************************************************************
 *  LOCAL VARIABLE
 **************************************************************************/
static unsigned char bAesKeyInit = FALSE;

/**************************************************************************
 *  EXTERNAL FUNCTION
 **************************************************************************/

/**************************************************************************
 *  DUMP HEADER
 **************************************************************************/
void sec_dump_img_header(const CIPHER_HEADER *img_header)
{
	SMSG(true, "[%s] header magic_number  = %d\n", MOD, img_header->magic_number);
	SMSG(true, "[%s] header cust_name     = %s\n", MOD, img_header->cust_name);
	SMSG(true, "[%s] header image_version = %d\n", MOD, img_header->image_version);
	SMSG(true, "[%s] header image_length  = %d\n", MOD, img_header->image_length);
	SMSG(true, "[%s] header image_offset  = %d\n", MOD, img_header->image_offset);
	SMSG(true, "[%s] header cipher_length = %d\n", MOD, img_header->cipher_length);
	SMSG(true, "[%s] header cipher_offset = %d\n", MOD, img_header->cipher_offset);
}

/**************************************************************************
 *  IMPORT KEY
 **************************************************************************/
int sec_aes_import_key(void)
{
	int ret = SEC_OK;
	unsigned char key[AES_KEY_SIZE] = { 0 };
	AES_VER aes_ver = AES_VER_LEGACY;
	unsigned int key_len = 0;

	/* avoid re-init aes key
	   if re-init key again, key value will be decoded twice .. */
	if (TRUE == bAesKeyInit) {
		SMSG(true, "[%s] reset aes vector\n", MOD);
		/* initialize internal crypto engine */
		ret = lib_aes_init_vector(rom_info.m_SEC_CTRL.m_sec_aes_legacy ?
						(AES_VER_LEGACY) : (AES_VER_SO));
		if (SEC_OK != ret)
			goto _end;
		return ret;
	}

	bAesKeyInit = TRUE;

	if (0 != mcmp(rom_info.m_id, RI_NAME, RI_NAME_LEN)) {
		SMSG(true, "[%s] error. key not found\n", MOD);
		ret = ERR_AES_KEY_NOT_FOUND;
		goto _end;
	}


	/* -------------------------- */
	/* check aes type             */
	/* -------------------------- */
	if (TRUE == rom_info.m_SEC_CTRL.m_sec_aes_legacy) {
		aes_ver = AES_VER_LEGACY;
		key_len = 32;
	} else {
		aes_ver = AES_VER_SO;
		key_len = 16;
	}


	/* -------------------------- */
	/* get sml aes key            */
	/* -------------------------- */
	if (FALSE == rom_info.m_SEC_CTRL.m_sml_aes_key_ac_en) {
		sec_decode_key(rom_info.m_SEC_KEY.sml_aes_key,
			       sizeof(rom_info.m_SEC_KEY.sml_aes_key),
			       rom_info.m_SEC_KEY.crypto_seed,
			       sizeof(rom_info.m_SEC_KEY.crypto_seed));
		dump_buf(rom_info.m_SEC_KEY.sml_aes_key, 4);
		mcpy(key, rom_info.m_SEC_KEY.sml_aes_key, sizeof(key));
	} else {
		SMSG(true, "\n[%s] AC enabled\n", MOD);
		dump_buf(secroimg.m_andro.sml_aes_key, 4);
		sec_decode_key(secroimg.m_andro.sml_aes_key,
			       sizeof(secroimg.m_andro.sml_aes_key),
			       (unsigned char *) SML_SCRAMBLE_SEED, sizeof(SML_SCRAMBLE_SEED));
		dump_buf(secroimg.m_andro.sml_aes_key, 4);
		mcpy(key, secroimg.m_andro.sml_aes_key, sizeof(key));
	}

	ret = lib_aes_init_key(key, key_len, aes_ver);
	/* initialize internal crypto engine */
	if (SEC_OK != ret)
		goto _end;
_end:
	return ret;
}

/**************************************************************************
 *  IMAGE CIPHER AES INIT
 **************************************************************************/
int sec_aes_init(void)
{
	int ret = SEC_OK;

	ret = sec_aes_import_key();
	/* init key */
	if (SEC_OK != ret)
		goto _exit;
_exit:
	return ret;
}


/**************************************************************************
 *  AES TEST FUNCTION
 **************************************************************************/
/* Note: this function is only for aes test */
int sec_aes_test(void)
{
	int ret = SEC_OK;
	unsigned char buf[CIPHER_BLOCK_SIZE] = "AES_TEST";

	SMSG(true, "\n[%s] SW AES test\n", MOD);

	/* -------------------------- */
	/* sec aes encrypt test       */
	/* -------------------------- */
	SMSG(true, "[%s] input      = 0x%x,0x%x,0x%x,0x%x\n", MOD, buf[0], buf[1], buf[2], buf[3]);
	ret = lib_aes_enc(buf, CIPHER_BLOCK_SIZE, buf, CIPHER_BLOCK_SIZE);
	if (SEC_OK != ret) {
		SMSG(true, "[%s] error (0x%x)\n", MOD, ret);
		goto _exit;
	}
	SMSG(true, "[%s] cipher     = 0x%x,0x%x,0x%x,0x%x\n", MOD, buf[0], buf[1], buf[2], buf[3]);

	/* -------------------------- */
	/* sec aes decrypt test       */
	/* -------------------------- */
	ret = lib_aes_dec(buf, CIPHER_BLOCK_SIZE, buf, CIPHER_BLOCK_SIZE);
	if (SEC_OK != ret) {
		SMSG(true, "[%s] error (0x%x)\n", MOD, ret);
		goto _exit;
	}
	SMSG(true, "[%s] plain text = 0x%x,0x%x,0x%x,0x%x\n", MOD, buf[0], buf[1], buf[2], buf[3]);

_exit:
	return ret;
}
