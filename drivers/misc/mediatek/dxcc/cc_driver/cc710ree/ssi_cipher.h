/*****************************************************************************
* Copyright (C) 2015 ARM Limited or its affiliates.	                     *
* This program is free software; you can redistribute it and/or modify it    *
* under the terms of the GNU General Public License as published by the Free *
* Software Foundation; either version 2 of the License, or (at your option)  * 
* any later version.							     *
* This program is distributed in the hope that it will be useful, but 	     *
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
* for more details.							     *	
* You should have received a copy of the GNU General Public License along    *
* with this program; if not, write to the Free Software Foundation, 	     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.        *
******************************************************************************/

/* \file ssi_cipher.h
   ARM CryptoCell Cipher Crypto API
 */

#ifndef __SSI_CIPHER_H__
#define __SSI_CIPHER_H__

#include <linux/kernel.h>
#include <crypto/algapi.h>
#include "ssi_driver.h"
#include "ssi_buffer_mgr.h"

#if SSI_CC_HAS_SEC_KEY
#define MAX_KEY_BUF_SIZE (DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES)
#define MIN_VALID_VALUE_OF_MEMCNT_FOR_SPECIAL_MODES 1
#define MIN_VALID_VALUE_OF_GENCNT_FOR_SPECIAL_MODES 2
#else
#define MAX_KEY_BUF_SIZE AES_MAX_KEY_SIZE
#endif

struct blkcipher_req_ctx {
	struct async_gen_req_ctx gen_ctx;
	enum ssi_req_dma_buf_type dma_buf_type;
	uint32_t in_nents;
	uint32_t in_mlli_nents;
	uint32_t out_nents;
	uint32_t out_mlli_nents;
	uint8_t *backup_info; /*store iv for generated IV flow*/
	bool is_giv;
	struct mlli_params mlli_params;
	enum ssi_secure_dir_type sec_dir;
};



int ssi_ablkcipher_alloc(struct ssi_drvdata *drvdata);

int ssi_ablkcipher_free(struct ssi_drvdata *drvdata);


void ssi_blkcipher_create_setup_desc(
	int cipher_mode,
	int flow_mode,
	int direction,
	dma_addr_t key_dma_addr,
	unsigned int key_len,
	dma_addr_t iv_dma_addr,
	unsigned int ivsize,
	unsigned int nbytes,
	HwDesc_s desc[],
	unsigned int *seq_size);


#endif /*__SSI_CIPHER_H__*/
