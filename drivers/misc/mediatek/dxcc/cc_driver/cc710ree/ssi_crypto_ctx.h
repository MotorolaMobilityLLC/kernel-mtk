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


#ifndef _SSI_CRYPTO_CTX_H_
#define _SSI_CRYPTO_CTX_H_

#ifdef __KERNEL__
#include <linux/types.h>
#define INT32_MAX 0x7FFFFFFFL
#else
#include <stdint.h>
#endif


#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* SeP context size */
#ifndef SEP_CTX_SIZE_LOG2
#if (SEP_SUPPORT_SHA > 256)
#define SEP_CTX_SIZE_LOG2 8
#else
#define SEP_CTX_SIZE_LOG2 7
#endif
#endif
#define SEP_CTX_SIZE (1<<SEP_CTX_SIZE_LOG2)
#define SASI_DRV_CTX_SIZE_WORDS (SEP_CTX_SIZE >> 2)

#define SASI_DRV_DES_IV_SIZE 8
#define SASI_DRV_DES_BLOCK_SIZE 8

#define SASI_DRV_DES_ONE_KEY_SIZE 8
#define SASI_DRV_DES_DOUBLE_KEY_SIZE 16
#define SASI_DRV_DES_TRIPLE_KEY_SIZE 24
#define SASI_DRV_DES_KEY_SIZE_MAX SASI_DRV_DES_TRIPLE_KEY_SIZE

#define SEP_AES_IV_SIZE 16
#define SEP_AES_IV_SIZE_WORDS (SEP_AES_IV_SIZE >> 2)

#define SEP_AES_BLOCK_SIZE 16
#define SEP_AES_BLOCK_SIZE_WORDS 4

#define SEP_AES_128_BIT_KEY_SIZE 16
#define SEP_AES_128_BIT_KEY_SIZE_WORDS	(SEP_AES_128_BIT_KEY_SIZE >> 2)
#define SEP_AES_192_BIT_KEY_SIZE 24
#define SEP_AES_192_BIT_KEY_SIZE_WORDS	(SEP_AES_192_BIT_KEY_SIZE >> 2)
#define SEP_AES_256_BIT_KEY_SIZE 32
#define SEP_AES_256_BIT_KEY_SIZE_WORDS	(SEP_AES_256_BIT_KEY_SIZE >> 2)
#define SEP_AES_KEY_SIZE_MAX			SEP_AES_256_BIT_KEY_SIZE
#define SEP_AES_KEY_SIZE_WORDS_MAX		(SEP_AES_KEY_SIZE_MAX >> 2)

#define SEP_MD5_DIGEST_SIZE 16
#define SEP_SHA1_DIGEST_SIZE 20
#define SEP_SHA224_DIGEST_SIZE 28
#define SEP_SHA256_DIGEST_SIZE 32
#define SEP_SHA256_DIGEST_SIZE_IN_WORDS 8
#define SEP_SHA384_DIGEST_SIZE 48
#define SEP_SHA512_DIGEST_SIZE 64

#define SEP_SHA1_BLOCK_SIZE 64
#define SEP_SHA1_BLOCK_SIZE_IN_WORDS 16
#define SEP_MD5_BLOCK_SIZE 64
#define SEP_MD5_BLOCK_SIZE_IN_WORDS 16
#define SEP_SHA224_BLOCK_SIZE 64
#define SEP_SHA256_BLOCK_SIZE 64
#define SEP_SHA256_BLOCK_SIZE_IN_WORDS 16
#define SEP_SHA1_224_256_BLOCK_SIZE 64
#define SEP_SHA384_BLOCK_SIZE 128
#define SEP_SHA512_BLOCK_SIZE 128

#if (SEP_SUPPORT_SHA > 256)
#define SEP_DIGEST_SIZE_MAX SEP_SHA512_DIGEST_SIZE
#define SEP_HASH_BLOCK_SIZE_MAX SEP_SHA512_BLOCK_SIZE /*1024b*/
#else /* Only up to SHA256 */
#define SEP_DIGEST_SIZE_MAX SEP_SHA256_DIGEST_SIZE
#define SEP_HASH_BLOCK_SIZE_MAX SEP_SHA256_BLOCK_SIZE /*512b*/
#endif

#define SEP_HMAC_BLOCK_SIZE_MAX SEP_HASH_BLOCK_SIZE_MAX

#define SEP_MULTI2_SYSTEM_KEY_SIZE 		32
#define SEP_MULTI2_DATA_KEY_SIZE 		8
#define SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE 	(SEP_MULTI2_SYSTEM_KEY_SIZE + SEP_MULTI2_DATA_KEY_SIZE)
#define	SEP_MULTI2_BLOCK_SIZE					8
#define	SEP_MULTI2_IV_SIZE					8
#define	SEP_MULTI2_MIN_NUM_ROUNDS				8
#define	SEP_MULTI2_MAX_NUM_ROUNDS				128


#define SASI_DRV_ALG_MAX_BLOCK_SIZE SEP_HASH_BLOCK_SIZE_MAX


enum drv_engine_type {
	DRV_ENGINE_NULL = 0,
	DRV_ENGINE_AES = 1,
	DRV_ENGINE_DES = 2,
	DRV_ENGINE_HASH = 3,
	DRV_ENGINE_RC4 = 4,
	DRV_ENGINE_DOUT = 5,
	DRV_ENGINE_RESERVE32B = INT32_MAX,
};

enum ssi_drv_crypto_alg {
	DRV_CRYPTO_ALG_NULL = -1,
	DRV_CRYPTO_ALG_AES  = 0,
	DRV_CRYPTO_ALG_DES  = 1,
	DRV_CRYPTO_ALG_HASH = 2,
	DRV_CRYPTO_ALG_C2   = 3,
	DRV_CRYPTO_ALG_HMAC = 4,
	DRV_CRYPTO_ALG_AEAD = 5,
	DRV_CRYPTO_ALG_BYPASS = 6,
	DRV_CRYPTO_ALG_NUM = 7,
	DRV_CRYPTO_ALG_RESERVE32B = INT32_MAX
};

enum sep_crypto_direction {
	SEP_CRYPTO_DIRECTION_NULL = -1,
	SEP_CRYPTO_DIRECTION_ENCRYPT = 0,
	SEP_CRYPTO_DIRECTION_DECRYPT = 1,
	SEP_CRYPTO_DIRECTION_DECRYPT_ENCRYPT = 3,
	SEP_CRYPTO_DIRECTION_RESERVE32B = INT32_MAX
};

enum sep_cipher_mode {
	SEP_CIPHER_NULL_MODE = -1,
	SEP_CIPHER_ECB = 0,
	SEP_CIPHER_CBC = 1,
	SEP_CIPHER_CTR = 2,
	SEP_CIPHER_CBC_MAC = 3,
	SEP_CIPHER_XTS = 4,
	SEP_CIPHER_XCBC_MAC = 5,
	SEP_CIPHER_OFB = 6,
	SEP_CIPHER_CMAC = 7,
	SEP_CIPHER_CCM = 8,
	SEP_CIPHER_CBC_CTS = 11,
	SEP_CIPHER_GCTR = 12,
	SEP_CIPHER_RESERVE32B = INT32_MAX
};

enum sep_hash_mode {
	SEP_HASH_NULL = -1,
	SEP_HASH_SHA1 = 0,
	SEP_HASH_SHA256 = 1,
	SEP_HASH_SHA224 = 2,
	SEP_HASH_SHA512 = 3,
	SEP_HASH_SHA384 = 4,
	SEP_HASH_MD5 = 5,
	SEP_HASH_CBC_MAC = 6, 
	SEP_HASH_XCBC_MAC = 7,
	SEP_HASH_CMAC = 8,
	SEP_HASH_MODE_NUM = 9,
	SEP_HASH_RESERVE32B = INT32_MAX
};

enum drv_hash_hw_mode {
	SEP_HASH_HW_MD5 = 0,
	SEP_HASH_HW_SHA1 = 1,
	SEP_HASH_HW_SHA256 = 2,
	SEP_HASH_HW_SHA224 = 10,
	SEP_HASH_HW_SHA512 = 4,
	SEP_HASH_HW_SHA384 = 12,
	SEP_HASH_HW_GHASH = 6,
	SEP_HASH_HW_RESERVE32B = INT32_MAX
};

enum drv_multi2_mode {
	SEP_MULTI2_NULL = -1,
	SEP_MULTI2_ECB = 0,
	SEP_MULTI2_CBC = 1,
	SEP_MULTI2_OFB = 2,
	SEP_MULTI2_RESERVE32B = INT32_MAX
};


/* drv_crypto_key_type[1:0] is mapped to cipher_do[1:0] */
/* drv_crypto_key_type[2] is mapped to cipher_config2 */
enum drv_crypto_key_type {
	DRV_NULL_KEY = -1,
	DRV_USER_KEY = 0,		/* 0x000 */
	DRV_ROOT_KEY = 1,		/* 0x001 */
	DRV_PROVISIONING_KEY = 2,	/* 0x010 */
	DRV_SESSION_KEY = 3,		/* 0x011 */
	DRV_APPLET_KEY = 4,		/* NA */
	DRV_PLATFORM_KEY = 5,		/* 0x101 */
	DRV_CUSTOMER_KEY = 6,		/* 0x110 */
	DRV_END_OF_KEYS = INT32_MAX,
};

enum drv_crypto_padding_type {
	DRV_PADDING_NONE = 0,
	DRV_PADDING_PKCS7 = 1,
	DRV_PADDING_RESERVE32B = INT32_MAX
};

/*******************************************************************/
/***************** DESCRIPTOR BASED CONTEXTS ***********************/
/*******************************************************************/

 /* Generic context ("super-class") */
struct drv_ctx_generic {
	enum ssi_drv_crypto_alg alg;
} __attribute__((__may_alias__));


struct drv_ctx_hash {
	enum ssi_drv_crypto_alg alg; /* DRV_CRYPTO_ALG_HASH */
	enum sep_hash_mode mode;
	uint8_t digest[SEP_DIGEST_SIZE_MAX];
	/* reserve to end of allocated context size */
	uint8_t reserved[SEP_CTX_SIZE - 2 * sizeof(uint32_t) -
			SEP_DIGEST_SIZE_MAX];
};

/* !!!! drv_ctx_hmac should have the same structure as drv_ctx_hash except
   k0, k0_size fields */
struct drv_ctx_hmac {
	enum ssi_drv_crypto_alg alg; /* DRV_CRYPTO_ALG_HMAC */
	enum sep_hash_mode mode;
	uint8_t digest[SEP_DIGEST_SIZE_MAX];
	uint32_t k0[SEP_HMAC_BLOCK_SIZE_MAX/sizeof(uint32_t)];
	uint32_t k0_size;
	/* reserve to end of allocated context size */
	uint8_t reserved[SEP_CTX_SIZE - 3 * sizeof(uint32_t) -
			SEP_DIGEST_SIZE_MAX - SEP_HMAC_BLOCK_SIZE_MAX];
};

struct drv_ctx_cipher {
	enum ssi_drv_crypto_alg alg; /* DRV_CRYPTO_ALG_AES */
	enum sep_cipher_mode mode;
	enum sep_crypto_direction direction;
	enum drv_crypto_key_type crypto_key_type;
	enum drv_crypto_padding_type padding_type;
	uint32_t key_size; /* numeric value in bytes   */
	uint32_t data_unit_size; /* required for XTS */
	/* block_state is the AES engine block state.
	*  It is used by the host to pass IV or counter at initialization.
	*  It is used by SeP for intermediate block chaining state and for
	*  returning MAC algorithms results.           */
	uint8_t block_state[SEP_AES_BLOCK_SIZE];
	uint8_t key[SEP_AES_KEY_SIZE_MAX];
	uint8_t xex_key[SEP_AES_KEY_SIZE_MAX];
	/* reserve to end of allocated context size */
	uint32_t reserved[SASI_DRV_CTX_SIZE_WORDS - 7 -
		SEP_AES_BLOCK_SIZE/sizeof(uint32_t) - 2 *
		(SEP_AES_KEY_SIZE_MAX/sizeof(uint32_t))];
};

/* authentication and encryption with associated data class */
struct drv_ctx_aead {
	enum ssi_drv_crypto_alg alg; /* DRV_CRYPTO_ALG_AES */
	enum sep_cipher_mode mode;
	enum sep_crypto_direction direction;
	uint32_t key_size; /* numeric value in bytes   */
	uint32_t nonce_size; /* nonce size (octets) */
	uint32_t header_size; /* finit additional data size (octets) */
	uint32_t text_size; /* finit text data size (octets) */
	uint32_t tag_size; /* mac size, element of {4, 6, 8, 10, 12, 14, 16} */
	/* block_state1/2 is the AES engine block state */
	uint8_t block_state[SEP_AES_BLOCK_SIZE];
	uint8_t mac_state[SEP_AES_BLOCK_SIZE]; /* MAC result */
	uint8_t nonce[SEP_AES_BLOCK_SIZE]; /* nonce buffer */
	uint8_t key[SEP_AES_KEY_SIZE_MAX];
	/* reserve to end of allocated context size */
	uint32_t reserved[SASI_DRV_CTX_SIZE_WORDS - 8 -
		3 * (SEP_AES_BLOCK_SIZE/sizeof(uint32_t)) -
		SEP_AES_KEY_SIZE_MAX/sizeof(uint32_t)];
};

/*******************************************************************/
/***************** MESSAGE BASED CONTEXTS **************************/
/*******************************************************************/


/* Get the address of a @member within a given @ctx address
   @ctx: The context address
   @type: Type of context structure
   @member: Associated context field */
#define GET_CTX_FIELD_ADDR(ctx, type, member) (ctx + offsetof(type, member))

#endif /* _SEP_CTX_H_ */

