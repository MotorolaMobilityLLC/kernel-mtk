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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <crypto/algapi.h>
#include <crypto/internal/skcipher.h>
#include <crypto/aes.h>
#include <crypto/ctr.h>
#include <crypto/des.h>

#include "ssi_config.h"
#include "ssi_driver.h"
#include "ssi_lli_defs.h"
#include "ssi_buffer_mgr.h"
#include "ssi_cipher.h"
#include "ssi_request_mgr.h"
#include "ssi_sysfs.h"
#include "ssi_fips_local.h"

#define MAX_ABLKCIPHER_SEQ_LEN 6

#define template_ablkcipher	template_u.ablkcipher
#define template_sblkcipher	template_u.blkcipher

#define SSI_MIN_AES_XTS_SIZE 0x10
#define SSI_MAX_AES_XTS_SIZE 0x2000

struct ssi_blkcipher_handle {
	struct list_head blkcipher_alg_list;
	/* This field is used for the secure key dropped flows */
	uint32_t  dropped_buffer;
	dma_addr_t  dropped_buffer_dma_addr;
};

struct ssi_ablkcipher_ctx {
	struct ssi_drvdata *drvdata;
	uint8_t *key;
	dma_addr_t key_dma_addr;
	int keylen;
	int key_round_number;
	int cipher_mode;
	int flow_mode;
	int is_secure_key;
	int is_aes_ctr_prot;
	unsigned int gpr1_value;
	struct blkcipher_req_ctx *sync_ctx;
};

static void ssi_ablkcipher_complete(struct device *dev, void *ssi_req, void __iomem *cc_base);
DEFINE_SEMAPHORE(secure_key_sem);


static int validate_keys_sizes(struct ssi_ablkcipher_ctx *ctx_p, uint32_t size) {
	switch (ctx_p->flow_mode){
	case S_DIN_to_AES:
		switch (size){
		case SEP_AES_128_BIT_KEY_SIZE:
		case SEP_AES_192_BIT_KEY_SIZE:
			if (likely(ctx_p->cipher_mode != SEP_CIPHER_XTS))
				return 0;
			break;
		case SEP_AES_256_BIT_KEY_SIZE:
			return 0;
		case (SEP_AES_192_BIT_KEY_SIZE*2):
		case (SEP_AES_256_BIT_KEY_SIZE*2):
			if (likely(ctx_p->cipher_mode == SEP_CIPHER_XTS))
				return 0;
			break;
		default:
			break;
		}
	case S_DIN_to_DES:
		if (likely(size == DES3_EDE_KEY_SIZE ||
		    size == DES_KEY_SIZE))
			return 0;
		break;
#if SSI_CC_HAS_MULTI2
	case S_DIN_to_MULTI2:
		if (likely(size == SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE))
			return 0;
		break;
#endif
	default:
		break;

	}
	return -EINVAL;
}


static int validate_data_size(struct ssi_ablkcipher_ctx *ctx_p, unsigned int size) {
	switch (ctx_p->flow_mode){
	case S_DIN_to_AES:
		switch (ctx_p->cipher_mode){
		case SEP_CIPHER_XTS:
			if ((size >= SSI_MIN_AES_XTS_SIZE) &&
			    (size <= SSI_MAX_AES_XTS_SIZE) && 
			    IS_ALIGNED(size, AES_BLOCK_SIZE))
				return 0;
			break;
		case SEP_CIPHER_CBC_CTS:
			if (likely(size >= AES_BLOCK_SIZE))
				return 0;
			break;
		case SEP_CIPHER_OFB:
		case SEP_CIPHER_CTR:
				return 0;
		case SEP_CIPHER_ECB:
		case SEP_CIPHER_CBC:
			if (likely(IS_ALIGNED(size, AES_BLOCK_SIZE)))
				return 0;
			break;
		default:
			break;
		}
		break;
	case S_DIN_to_DES:
		if (likely(IS_ALIGNED(size, DES_BLOCK_SIZE)))
				return 0;
		break;
#if SSI_CC_HAS_MULTI2
	case S_DIN_to_MULTI2:
		switch (ctx_p->cipher_mode) {
		case SEP_MULTI2_CBC:
			if (likely(IS_ALIGNED(size, SEP_MULTI2_BLOCK_SIZE)))
				return 0;
			break;
		case SEP_MULTI2_OFB:
			return 0;
		default:
			break;
		}
		break;
#endif /*SSI_CC_HAS_MULTI2*/
	default:
		break;

	}
	return -EINVAL;
}

static unsigned int get_max_keysize(struct crypto_tfm *tfm)
{
	struct ssi_crypto_alg *ssi_alg = container_of(tfm->__crt_alg, struct ssi_crypto_alg, crypto_alg);

	if ((ssi_alg->crypto_alg.cra_flags & CRYPTO_ALG_TYPE_MASK) == CRYPTO_ALG_TYPE_ABLKCIPHER) {
		return ssi_alg->crypto_alg.cra_ablkcipher.max_keysize;
	}

	if ((ssi_alg->crypto_alg.cra_flags & CRYPTO_ALG_TYPE_MASK) == CRYPTO_ALG_TYPE_BLKCIPHER) {
		return ssi_alg->crypto_alg.cra_blkcipher.max_keysize;
	}

	return 0;
}

static int ssi_blkcipher_init(struct crypto_tfm *tfm)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct crypto_alg *alg = tfm->__crt_alg;
	struct ssi_crypto_alg *ssi_alg =
			container_of(alg, struct ssi_crypto_alg, crypto_alg);
	struct device *dev;
	int rc = 0;
	unsigned int max_key_buf_size = get_max_keysize(tfm);

	SSI_LOG_DEBUG("Initializing context @%p for %s\n", ctx_p,
						crypto_tfm_alg_name(tfm));

	CHECK_AND_RETURN_UPON_FIPS_TEE_ERROR();
	ctx_p->cipher_mode = ssi_alg->cipher_mode;
	ctx_p->flow_mode = ssi_alg->flow_mode;
	ctx_p->is_secure_key = ssi_alg->is_secure_key;
	ctx_p->drvdata = ssi_alg->drvdata;
	dev = &ctx_p->drvdata->plat_dev->dev;

	/* Allocate key buffer, cache line aligned */
	ctx_p->key = kmalloc(max_key_buf_size, GFP_KERNEL|GFP_DMA);
	if (!ctx_p->key) {
		SSI_LOG_ERR("Allocating key buffer in context failed\n");
		rc = -ENOMEM;
	}
	SSI_LOG_DEBUG("Allocated key buffer in context ctx_p->key=@%p\n",
								ctx_p->key);

	/* Map key buffer */
	ctx_p->key_dma_addr = dma_map_single(dev, (void *)ctx_p->key,
					     max_key_buf_size, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, ctx_p->key_dma_addr)) {
		SSI_LOG_ERR("Mapping Key %u B at va=%pK for DMA failed\n",
			max_key_buf_size, ctx_p->key);
		return -ENOMEM;
	}
	SSI_UPDATE_DMA_ADDR_TO_48BIT(ctx_p->key_dma_addr, max_key_buf_size);
	SSI_LOG_DEBUG("Mapped key %u B at va=%pK to dma=0x%llX\n",
		max_key_buf_size, ctx_p->key,
		(unsigned long long)ctx_p->key_dma_addr);

	return rc;
}

static void ssi_blkcipher_exit(struct crypto_tfm *tfm)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct device *dev = &ctx_p->drvdata->plat_dev->dev;
	unsigned int max_key_buf_size = get_max_keysize(tfm);

	SSI_LOG_DEBUG("Clearing context @%p for %s\n",
		crypto_tfm_ctx(tfm), crypto_tfm_alg_name(tfm));

	/* Unmap key buffer */
	SSI_RESTORE_DMA_ADDR_TO_48BIT(ctx_p->key_dma_addr);
	dma_unmap_single(dev, ctx_p->key_dma_addr, max_key_buf_size,
								DMA_TO_DEVICE);
	SSI_LOG_DEBUG("Unmapped key buffer key_dma_addr=0x%llX\n", 
		(unsigned long long)ctx_p->key_dma_addr);

	/* Free key buffer in context */
	kfree(ctx_p->key);
	SSI_LOG_DEBUG("Free key buffer in context ctx_p->key=@%p\n", ctx_p->key);
}


typedef struct tdes_keys{
        u8      key1[DES_KEY_SIZE];
        u8      key2[DES_KEY_SIZE];
        u8      key3[DES_KEY_SIZE];
}tdes_keys_t;

static const u8 zero_buff[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                               0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                               0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                               0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

/* The function verifies that tdes keys are not weak.*/
static int ssi_fips_verify_3des_keys(const u8 *key, unsigned int keylen)
{
        tdes_keys_t *tdes_key = (tdes_keys_t*)key;

	/* verify key1 != key2 and key3 != key2*/
        if (unlikely( (memcmp((u8*)tdes_key->key1, (u8*)tdes_key->key2, sizeof(tdes_key->key1)) == 0) || 
		      (memcmp((u8*)tdes_key->key3, (u8*)tdes_key->key2, sizeof(tdes_key->key3)) == 0) )) {
                return -ENOEXEC;
        }

        return 0;
}

/* The function verifies that xts keys are not weak.*/
static int ssi_fips_verify_xts_keys(const u8 *key, unsigned int keylen)
{
        /* Weak key is define as key that its first half (128/256 lsb) equals its second half (128/256 msb) */
        int singleKeySize = keylen >> 1;

	if (unlikely(memcmp(key, &key[singleKeySize], singleKeySize) == 0)) {
		return -ENOEXEC;
	}

        return 0;
}

static int ssi_blkcipher_setkey(struct crypto_tfm *tfm, 
				const u8 *key, 
				unsigned int keylen)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct device *dev = &ctx_p->drvdata->plat_dev->dev;
	u32 tmp[DES_EXPKEY_WORDS];
	unsigned int max_key_buf_size = get_max_keysize(tfm);

	DECL_CYCLE_COUNT_RESOURCES;

	SSI_LOG_DEBUG("Setting key in context @%p for %s. keylen=%u\n",
		ctx_p, crypto_tfm_alg_name(tfm), keylen);
	dump_byte_array("key", (uint8_t *)key, keylen);

	CHECK_AND_RETURN_UPON_FIPS_ERROR();
	
	/* STAT_PHASE_0: Init and sanity checks */
	START_CYCLE_COUNT();

#if SSI_CC_HAS_MULTI2
	/*last byte of key buffer is round number and should not be a part of key size*/
	if (ctx_p->flow_mode == S_DIN_to_MULTI2) {
		keylen -=1;
	}
#endif /*SSI_CC_HAS_MULTI2*/

	if (unlikely(validate_keys_sizes(ctx_p,keylen) != 0)) {
		SSI_LOG_ERR("Unsupported key size %d.\n", keylen);
		crypto_tfm_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	// verify weak keys
	if (ctx_p->flow_mode == S_DIN_to_DES) {
		if (unlikely(!des_ekey(tmp, key)) &&
		(crypto_tfm_get_flags(tfm) & CRYPTO_TFM_REQ_WEAK_KEY)) {
			tfm->crt_flags |= CRYPTO_TFM_RES_WEAK_KEY;
			return -EINVAL;
		}
	}
	if ((ctx_p->cipher_mode == SEP_CIPHER_XTS) &&
		ssi_fips_verify_xts_keys(key, keylen) != 0) {
		return -EINVAL;
	}
	if ((ctx_p->flow_mode == S_DIN_to_DES) &&
		(keylen == DES3_EDE_KEY_SIZE) &&
		ssi_fips_verify_3des_keys(key, keylen) != 0) {
		return -EINVAL;
	}

	END_CYCLE_COUNT(STAT_OP_TYPE_SETKEY, STAT_PHASE_0);

	/* STAT_PHASE_1: Copy key to ctx */
	START_CYCLE_COUNT();
	SSI_RESTORE_DMA_ADDR_TO_48BIT(ctx_p->key_dma_addr);
	dma_sync_single_for_cpu(dev, ctx_p->key_dma_addr,
					max_key_buf_size, DMA_TO_DEVICE);
#if SSI_CC_HAS_MULTI2
	if (ctx_p->flow_mode == S_DIN_to_MULTI2) {
		memcpy(ctx_p->key, key, SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE);
		ctx_p->key_round_number = key[SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE];
		if (ctx_p->key_round_number < SEP_MULTI2_MIN_NUM_ROUNDS ||
		    ctx_p->key_round_number > SEP_MULTI2_MAX_NUM_ROUNDS) {
			crypto_tfm_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
			return -EINVAL;
		}
	} else
#endif /*SSI_CC_HAS_MULTI2*/
	{
		memcpy(ctx_p->key, key, keylen);
		if (keylen == 24)
			memset(ctx_p->key + 24, 0, SEP_AES_KEY_SIZE_MAX - 24);
	}
	dma_sync_single_for_device(dev, ctx_p->key_dma_addr,
					max_key_buf_size, DMA_TO_DEVICE);
	SSI_UPDATE_DMA_ADDR_TO_48BIT(ctx_p->key_dma_addr ,max_key_buf_size);
	ctx_p->keylen = keylen;

	END_CYCLE_COUNT(STAT_OP_TYPE_SETKEY, STAT_PHASE_1);

	return 0;
}


#if SSI_CC_HAS_SEC_KEY
static int ssi_blkcipher_secure_key_setkey(struct crypto_tfm *tfm,
					   const u8 *key, unsigned int keylen)
{
	struct ssi_ablkcipher_ctx *ctx_p =
				crypto_tfm_ctx(tfm);
	struct device *dev = &ctx_p->drvdata->plat_dev->dev;

	u32 skConfig = 0;
	DECL_CYCLE_COUNT_RESOURCES;

	SSI_LOG_DEBUG("Setting key in context @%p for %s. keylen=%u\n",
		ctx_p, crypto_tfm_alg_name(tfm), keylen);

	CHECK_AND_RETURN_UPON_FIPS_ERROR();
	if (keylen != DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES) {
		return -EINVAL;
	}

	/* STAT_PHASE_0: Init and sanity checks */
	START_CYCLE_COUNT();
	/* No sanity checks*/
	END_CYCLE_COUNT(STAT_OP_TYPE_SETKEY, STAT_PHASE_0);
	memcpy(&skConfig,
	       &key[DX_SECURE_KEY_RESTRICT_CONFIG_OFFSET * sizeof(u32)],
	       sizeof(u32));
	/* STAT_PHASE_1: Copy key to ctx */
	START_CYCLE_COUNT();
	/* The secure key is working only with one key size */
	SSI_RESTORE_DMA_ADDR_TO_48BIT(ctx_p->key_dma_addr);
	dma_sync_single_for_cpu(dev, ctx_p->key_dma_addr,
					MAX_KEY_BUF_SIZE, DMA_TO_DEVICE);
	memcpy(ctx_p->key, key, MAX_KEY_BUF_SIZE);
	dma_sync_single_for_device(dev, ctx_p->key_dma_addr,
					MAX_KEY_BUF_SIZE, DMA_TO_DEVICE);
	SSI_UPDATE_DMA_ADDR_TO_48BIT(ctx_p->key_dma_addr, MAX_KEY_BUF_SIZE);

	/*Get the keysize for the IV loading descriptor */
	switch(SASI_REG_FLD_GET(0, SECURE_KEY_RESTRICT, KEY_TYPE, skConfig)) {
	case DX_SECURE_KEY_AES_KEY128 :
		ctx_p->keylen = SEP_AES_128_BIT_KEY_SIZE;
		break;
	case DX_SECURE_KEY_AES_KEY256:
		ctx_p->keylen = SEP_AES_256_BIT_KEY_SIZE;
		break;
#if SSI_CC_HAS_MULTI2
	case DX_SECURE_KEY_MULTI2:
		/*The keylen is not use in the multi2 IV loading desciptor.
		  The field is init to value that is converted to 0 in the
		  descriptor */
		ctx_p->keylen = SEP_AES_128_BIT_KEY_SIZE;
		break;
#endif /*SSI_CC_HAS_MULTI2*/
	case DX_SECURE_KEY_BYPASS:
		break;
	default:
		return -EINVAL;
	}

	switch(SASI_REG_FLD_GET(0, SECURE_KEY_RESTRICT, MODE, skConfig)) {
	case DX_SECURE_KEY_CIPHER_CTR_NONCE_PROT:
	case DX_SECURE_KEY_CIPHER_CTR_NONCE_CTR_PROT_NSP:
		ctx_p->is_aes_ctr_prot = 1;
		break;
	default:
		ctx_p->is_aes_ctr_prot = 0;
	}
	END_CYCLE_COUNT(STAT_OP_TYPE_SETKEY, STAT_PHASE_1);
	return 0;
}
#endif /*SSI_CC_HAS_SEC_KEY*/

void
ssi_blkcipher_create_setup_desc(
	int cipher_mode,
	int flow_mode,
	int direction,
	dma_addr_t key_dma_addr,
	unsigned int key_len,
	dma_addr_t iv_dma_addr,
	unsigned int ivsize,
	unsigned int nbytes,
	HwDesc_s desc[],
	unsigned int *seq_size)
{
	switch (cipher_mode) {
	case SEP_CIPHER_CBC:
	case SEP_CIPHER_CBC_CTS:
	case SEP_CIPHER_CTR:
	case SEP_CIPHER_OFB:
		/* Load cipher state */
		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
				     iv_dma_addr, ivsize,
				     AXI_ID, NS_BIT);
		HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], direction);
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
		HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], cipher_mode);
		if ((cipher_mode == SEP_CIPHER_CTR) ||
		    (cipher_mode == SEP_CIPHER_OFB) ) {
			HW_DESC_SET_SETUP_MODE(&desc[*seq_size],
					       SETUP_LOAD_STATE1);
		} else {
			HW_DESC_SET_SETUP_MODE(&desc[*seq_size],
					       SETUP_LOAD_STATE0);
		}
		(*seq_size)++;
		/*FALLTHROUGH*/
	case SEP_CIPHER_ECB:
		/* Load key */
		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], cipher_mode);
		HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], direction);
		if (flow_mode == S_DIN_to_AES) {
			HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
					     key_dma_addr,
					     ((key_len == 24) ? AES_MAX_KEY_SIZE : key_len),
					     AXI_ID, NS_BIT);
			HW_DESC_SET_KEY_SIZE_AES(&desc[*seq_size], key_len);
		} else {
			/*des*/
			HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
					     key_dma_addr, key_len,
					     AXI_ID, NS_BIT);
			HW_DESC_SET_KEY_SIZE_DES(&desc[*seq_size], key_len);
		}
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
		HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_KEY0);
		(*seq_size)++;
		break;
	case SEP_CIPHER_XTS:
		/* Load AES key */
		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], cipher_mode);
		HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], direction);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
				     key_dma_addr, key_len/2,
				     AXI_ID, NS_BIT);
		HW_DESC_SET_KEY_SIZE_AES(&desc[*seq_size], key_len/2);
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
		HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_KEY0);
		(*seq_size)++;

		/* load XEX key */
		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], cipher_mode);
		HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], direction);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
				     (key_dma_addr+key_len/2), key_len/2,
				     AXI_ID, NS_BIT);
		HW_DESC_SET_XEX_DATA_UNIT_SIZE(&desc[*seq_size], nbytes);
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
		HW_DESC_SET_KEY_SIZE_AES(&desc[*seq_size], key_len/2);
		HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_XEX_KEY);
		(*seq_size)++;

		/* Set state */
		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_STATE1);
		HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], cipher_mode);
		HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], direction);
		HW_DESC_SET_KEY_SIZE_AES(&desc[*seq_size], key_len/2);
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
				     iv_dma_addr, SEP_AES_BLOCK_SIZE,
				     AXI_ID, NS_BIT);
		(*seq_size)++;
		break;
	default:
		SSI_LOG_ERR("Unsupported cipher mode (%d)\n", cipher_mode);
		BUG();
	}
}

#if SSI_CC_HAS_MULTI2
static inline void ssi_blkcipher_create_multi2_setup_desc(
	struct crypto_tfm *tfm,
	struct blkcipher_req_ctx *req_ctx,
	unsigned int ivsize,
	HwDesc_s desc[],
	unsigned int *seq_size)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);

	int direction = req_ctx->gen_ctx.op_type;
	/* Load system key */
	HW_DESC_INIT(&desc[*seq_size]);
	HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], ctx_p->cipher_mode);
	HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], direction);
	HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI, ctx_p->key_dma_addr,
						SEP_MULTI2_SYSTEM_KEY_SIZE,
						AXI_ID, NS_BIT);
	HW_DESC_SET_FLOW_MODE(&desc[*seq_size], ctx_p->flow_mode);
	HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_KEY0);
	(*seq_size)++;

	/* load data key */
	HW_DESC_INIT(&desc[*seq_size]);
	HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
					(ctx_p->key_dma_addr +
						SEP_MULTI2_SYSTEM_KEY_SIZE),
				SEP_MULTI2_DATA_KEY_SIZE, AXI_ID, NS_BIT);
	HW_DESC_SET_MULTI2_NUM_ROUNDS(&desc[*seq_size],
						ctx_p->key_round_number);
	HW_DESC_SET_FLOW_MODE(&desc[*seq_size], ctx_p->flow_mode);
	HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], ctx_p->cipher_mode);
	HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], direction);
	HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_STATE0 );
	(*seq_size)++;


	/* Set state */
	HW_DESC_INIT(&desc[*seq_size]);
	HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
			     req_ctx->gen_ctx.iv_dma_addr,
			     ivsize, AXI_ID, NS_BIT);
	HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], direction);
	HW_DESC_SET_FLOW_MODE(&desc[*seq_size], ctx_p->flow_mode);
	HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], ctx_p->cipher_mode);
	HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_STATE1);
	(*seq_size)++;

}
#endif /*SSI_CC_HAS_MULTI2*/

#if SSI_CC_HAS_SEC_KEY
static inline void
ssi_blkcipher_create_secure_key_setup_desc(
	struct crypto_tfm *tfm,
	struct blkcipher_req_ctx *req_ctx,
	unsigned int ivsize,
	HwDesc_s desc[],
	unsigned int *seq_size)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);

	int direction = req_ctx->gen_ctx.op_type;

	/* Load system key */
	HW_DESC_INIT(&desc[*seq_size]);
	HW_DESC_SET_DIN_TYPE(&desc[*seq_size], NO_DMA,
			     ctx_p->key_dma_addr,
			     DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			     AXI_ID, NS_BIT);
	HW_DESC_STOP_QUEUE(&desc[*seq_size]);
	(*seq_size)++;

	/* Load cipher state */
	HW_DESC_INIT(&desc[*seq_size]);
	HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
			     req_ctx->gen_ctx.iv_dma_addr,
			     ivsize, AXI_ID, NS_BIT);
	HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], direction);
	HW_DESC_SET_FLOW_MODE(&desc[*seq_size], ctx_p->flow_mode);
	HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], ctx_p->cipher_mode);
	/*The state is loaded in any case of multi2 or AES CTR/OFB
	  with SETUP_LOAD_STATE1 all other case with SETUP_LOAD_STATE0*/
	if ((ctx_p->flow_mode == S_DIN_to_MULTI2) ||
	    (ctx_p->cipher_mode == SEP_CIPHER_CTR) ||
	    (ctx_p->cipher_mode == SEP_CIPHER_OFB)) {
		HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_STATE1);
	} else {
		HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_STATE0);
	}
	HW_DESC_SET_KEY_SIZE_AES(&desc[*seq_size], ctx_p->keylen);
	(*seq_size)++;

}

static inline void
ssi_blkcipher_create_secure_bypass_setup_desc(
	struct crypto_tfm *tfm,
	HwDesc_s desc[],
	unsigned int *seq_size)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);

	/* Load system key */
	HW_DESC_INIT(&desc[*seq_size]);
	HW_DESC_SET_DIN_TYPE(&desc[*seq_size], NO_DMA,
			     ctx_p->key_dma_addr,
			     DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			     AXI_ID, NS_BIT);
	HW_DESC_STOP_QUEUE(&desc[*seq_size]);
	(*seq_size)++;
}

/* Load key to disable the secure key restrictions */
static inline void
ssi_blkcipher_create_disable_secure_key_setup_desc(
	struct crypto_tfm *tfm,
	struct blkcipher_req_ctx *req_ctx,
	HwDesc_s desc[],
	unsigned int *seq_size)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	unsigned int flow_mode = ctx_p->flow_mode;

	switch (flow_mode) {
	case BYPASS:
		flow_mode = S_DIN_to_AES;
		break;
	default:
		break;
	}


	HW_DESC_INIT(&desc[*seq_size]);
	HW_DESC_SET_CIPHER_MODE(&desc[*seq_size], ctx_p->cipher_mode);
	HW_DESC_SET_CIPHER_CONFIG0(&desc[*seq_size], req_ctx->gen_ctx.op_type);
	HW_DESC_SET_DIN_CONST(&desc[*seq_size], 0, SEP_AES_128_BIT_KEY_SIZE);
	HW_DESC_SET_KEY_SIZE_AES(&desc[*seq_size], SEP_AES_128_BIT_KEY_SIZE);
	HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
	HW_DESC_SET_SETUP_MODE(&desc[*seq_size], SETUP_LOAD_KEY0);
	(*seq_size)++;
}
#endif /*SSI_CC_HAS_SEC_KEY*/

static inline void
ssi_blkcipher_create_data_desc(
	struct crypto_tfm *tfm,
	struct blkcipher_req_ctx *req_ctx,
	struct scatterlist *dst, struct scatterlist *src,
	unsigned int nbytes,
	void *areq,
	HwDesc_s desc[],
	unsigned int *seq_size)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	unsigned int flow_mode = ctx_p->flow_mode;

	switch (ctx_p->flow_mode) {
	case S_DIN_to_AES:
		flow_mode = DIN_AES_DOUT;
		break;
	case S_DIN_to_DES:
		flow_mode = DIN_DES_DOUT;
		break;
#if SSI_CC_HAS_MULTI2
	case S_DIN_to_MULTI2:
		flow_mode = DIN_MULTI2_DOUT;
		break;
#endif /*SSI_CC_HAS_MULTI2*/
	default:
		SSI_LOG_ERR("invalid flow mode, flow_mode = %d \n", flow_mode);
		return;
	}
	/* Process */
	if (likely(req_ctx->dma_buf_type == SSI_DMA_BUF_DLLI)){
		SSI_LOG_DEBUG(" data params addr 0x%llX length 0x%X \n",
			     (unsigned long long)sg_dma_address(src),
			     nbytes);
		SSI_LOG_DEBUG(" data params addr 0x%llX length 0x%X \n",
			     (unsigned long long)sg_dma_address(dst),
			     nbytes);
		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
				     sg_dma_address(src),
				     nbytes, AXI_ID, NS_BIT);
		HW_DESC_SET_DOUT_DLLI(&desc[*seq_size],
				      sg_dma_address(dst),
				      nbytes,
				      AXI_ID, NS_BIT, (areq == NULL)? 0:1);
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
		(*seq_size)++;
	} else {
		/* bypass */
		SSI_LOG_DEBUG(" bypass params addr 0x%llX "
			     "length 0x%X addr 0x%08X\n",
			(unsigned long long)req_ctx->mlli_params.mlli_dma_addr,
			req_ctx->mlli_params.mlli_len,
			(unsigned int)ctx_p->drvdata->mlli_sram_addr);
		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
				     req_ctx->mlli_params.mlli_dma_addr,
				     req_ctx->mlli_params.mlli_len,
				     AXI_ID, NS_BIT);
		HW_DESC_SET_DOUT_SRAM(&desc[*seq_size],
				      ctx_p->drvdata->mlli_sram_addr,
				      req_ctx->mlli_params.mlli_len);
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], BYPASS);
		(*seq_size)++;

		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_MLLI,
			ctx_p->drvdata->mlli_sram_addr,
				     req_ctx->in_mlli_nents, AXI_ID, NS_BIT);
		if (req_ctx->out_nents == 0) {
			SSI_LOG_DEBUG(" din/dout params addr 0x%08X "
				     "addr 0x%08X\n",
			(unsigned int)ctx_p->drvdata->mlli_sram_addr,
			(unsigned int)ctx_p->drvdata->mlli_sram_addr);
			HW_DESC_SET_DOUT_MLLI(&desc[*seq_size],
			ctx_p->drvdata->mlli_sram_addr,
					      req_ctx->in_mlli_nents,
					       AXI_ID, NS_BIT,(areq == NULL)? 0:1);
		} else {
			SSI_LOG_DEBUG(" din/dout params "
				     "addr 0x%08X addr 0x%08X\n",
				(unsigned int)ctx_p->drvdata->mlli_sram_addr,
				(unsigned int)ctx_p->drvdata->mlli_sram_addr +
				(uint32_t)LLI_ENTRY_BYTE_SIZE *
							req_ctx->in_nents);
			HW_DESC_SET_DOUT_MLLI(&desc[*seq_size],
				(ctx_p->drvdata->mlli_sram_addr +
				LLI_ENTRY_BYTE_SIZE *
						req_ctx->in_mlli_nents),
				req_ctx->out_mlli_nents, AXI_ID, NS_BIT,(areq == NULL)? 0:1);
		}
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
		(*seq_size)++;
	}
}

#if SSI_CC_HAS_SEC_KEY
static inline void
ssi_blkcipher_create_secure_key_mlli_desc(
	struct crypto_tfm *tfm,
	struct blkcipher_req_ctx *req_ctx,
	HwDesc_s desc[],
	unsigned int *seq_size)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);

	if (likely(req_ctx->dma_buf_type == SSI_DMA_BUF_MLLI)) {
		/* bypass */
		SSI_LOG_DEBUG(" bypass params addr 0x%llX "
			     "length 0x%X addr 0x%08X\n",
			(unsigned long long)req_ctx->mlli_params.mlli_dma_addr,
			req_ctx->mlli_params.mlli_len,
			(unsigned int)ctx_p->drvdata->mlli_sram_addr);
		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
				     req_ctx->mlli_params.mlli_dma_addr,
				     req_ctx->mlli_params.mlli_len,
				     AXI_ID, NS_BIT);
		HW_DESC_SET_DOUT_SRAM(&desc[*seq_size],
				      ctx_p->drvdata->mlli_sram_addr,
				      req_ctx->mlli_params.mlli_len);
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], BYPASS);
		(*seq_size)++;
	}
}

static inline void
ssi_blkcipher_create_secure_key_data_desc(
	struct crypto_tfm *tfm,
	struct blkcipher_req_ctx *req_ctx,
	struct scatterlist *dst, struct scatterlist *src,
	unsigned int nbytes,
	void *areq,
	HwDesc_s desc[],
	unsigned int *seq_size)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct ssi_blkcipher_handle *handle =
					ctx_p->drvdata->blkcipher_handle;
	unsigned int flow_mode = ctx_p->flow_mode;

	switch (ctx_p->flow_mode) {
	case S_DIN_to_AES:
		flow_mode = DIN_AES_DOUT;
		break;
#if SSI_CC_HAS_MULTI2
	case S_DIN_to_MULTI2:
		flow_mode = DIN_MULTI2_DOUT;
		break;
#endif
	default:
		SSI_LOG_ERR("invalid flow mode, flow_mode = %d \n", flow_mode);
		return;
	}

	/* Process */
	HW_DESC_INIT(&desc[*seq_size]);
	HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
	if (likely(req_ctx->dma_buf_type == SSI_DMA_BUF_DLLI)){
		SSI_LOG_DEBUG(" data params addr 0x%llX length 0x%X \n",
			     (unsigned long long)sg_dma_address(src),
			     nbytes);
		SSI_LOG_DEBUG(" data params addr 0x%llX length 0x%X \n",
			     (unsigned long long)sg_dma_address(dst),
			     nbytes);
		if (likely(!ctx_p->is_aes_ctr_prot)) {
			HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
					     sg_dma_address(src),
					     nbytes, AXI_ID, NS_BIT);
			HW_DESC_SET_DOUT_DLLI(&desc[*seq_size],
					      sg_dma_address(dst),
					      nbytes,
					      AXI_ID, NS_BIT, (areq == NULL)? 0:1);
		} else {
			/* In case of counter protection the operation
			is done in the SEP so the stop queue and NO DMA
			should be set */
			HW_DESC_SET_DIN_TYPE(&desc[*seq_size], NO_DMA,
					     sg_dma_address(src),
					     nbytes, AXI_ID, NS_BIT);
			HW_DESC_SET_DOUT_TYPE(&desc[*seq_size], NO_DMA,
					      sg_dma_address(dst),
					      nbytes,
					      AXI_ID, NS_BIT);
		}
	} else {
		/*The MLLI table was already loaded before */
		if (likely((req_ctx->sec_dir != SSI_NO_DMA_IS_SECURE))) {
			if (req_ctx->sec_dir == SSI_SRC_DMA_IS_SECURE) {
				HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
						     sg_dma_address(src),
						     nbytes, AXI_ID,
									NS_BIT);
				HW_DESC_SET_DOUT_MLLI(&desc[*seq_size],
					ctx_p->drvdata->mlli_sram_addr,
					req_ctx->out_mlli_nents, AXI_ID,
						      NS_BIT,
						      (areq == NULL)? 0:(!ctx_p->is_aes_ctr_prot));
			} else {
				HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_MLLI,
					ctx_p->drvdata->mlli_sram_addr,
					req_ctx->in_mlli_nents, AXI_ID,
									NS_BIT);
				HW_DESC_SET_DOUT_DLLI(&desc[*seq_size],
						      sg_dma_address(dst),
						      nbytes,
						      AXI_ID, NS_BIT,
						      (areq == NULL)? 0:(!ctx_p->is_aes_ctr_prot));
			}
		} else {
			/* In case of counter protection the flow is done in the
			   SEP and it can be done MLLI to MLLI*/
			HW_DESC_SET_DIN_TYPE(&desc[*seq_size], NO_DMA,
				ctx_p->drvdata->mlli_sram_addr,
				req_ctx->in_mlli_nents, AXI_ID, NS_BIT);
			if (req_ctx->out_nents == 0) {
				HW_DESC_SET_DOUT_TYPE(&desc[*seq_size], NO_DMA,
					ctx_p->drvdata->mlli_sram_addr,
					req_ctx->out_mlli_nents, AXI_ID, NS_BIT);
			} else {
				HW_DESC_SET_DOUT_TYPE(&desc[*seq_size],  NO_DMA,
				(ctx_p->drvdata->mlli_sram_addr +
				LLI_ENTRY_BYTE_SIZE *
						req_ctx->in_mlli_nents),
				req_ctx->out_mlli_nents, AXI_ID, NS_BIT);
			}
		}
	}
	if (unlikely(ctx_p->is_aes_ctr_prot)) {
		HW_DESC_STOP_QUEUE(&desc[*seq_size]);
	}
	(*seq_size)++;
	if (ctx_p->is_aes_ctr_prot) {
		/* dummy completion descriptor */
		HW_DESC_INIT(&desc[*seq_size]);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
				     handle->dropped_buffer_dma_addr,
				     sizeof(handle->dropped_buffer),
				     AXI_ID, NS_BIT);
		HW_DESC_SET_DOUT_DLLI(&desc[*seq_size],
				      handle->dropped_buffer_dma_addr,
				      sizeof(handle->dropped_buffer),
				      AXI_ID, NS_BIT, (areq == NULL)? 0:1);
		HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
		(*seq_size)++;
	}

}

static inline void
ssi_blkcipher_create_secure_bypass_data_desc(
	struct crypto_tfm *tfm,
	struct blkcipher_req_ctx *req_ctx,
	struct scatterlist *dst, struct scatterlist *src,
	unsigned int nbytes,
	void *areq,
	HwDesc_s desc[],
	unsigned int *seq_size)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct ssi_blkcipher_handle *handle =
					ctx_p->drvdata->blkcipher_handle;
	unsigned int flow_mode = ctx_p->flow_mode;

	switch (ctx_p->flow_mode) {
	case BYPASS:
		break;
	default:
		SSI_LOG_ERR("invalid flow mode, flow_mode = %d \n", flow_mode);
		return;
	}

	/* Process */
	HW_DESC_INIT(&desc[*seq_size]);
	if (likely(req_ctx->dma_buf_type == SSI_DMA_BUF_DLLI)){
		SSI_LOG_DEBUG(" data params addr 0x%llX length 0x%X \n",
			     (unsigned long long)sg_dma_address(src),
			     nbytes);
		SSI_LOG_DEBUG(" data params addr 0x%llX length 0x%X \n",
			     (unsigned long long)sg_dma_address(dst),
			     nbytes);
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], NO_DMA,
				     sg_dma_address(src),
				     nbytes, AXI_ID, NS_BIT);
        HW_DESC_SET_DOUT_TYPE(&desc[*seq_size], NO_DMA,
                      sg_dma_address(dst),
                      nbytes,
                      AXI_ID, NS_BIT);
	} else {
		/*The MLLI table was already loaded before */
		HW_DESC_SET_DIN_TYPE(&desc[*seq_size], NO_DMA,
			ctx_p->drvdata->mlli_sram_addr,
			req_ctx->in_mlli_nents, AXI_ID, NS_BIT);
		/* The Dout is holding the type of the input
		as the input must be NO_DMA for the special descriptor
		and the output must be DLLI due ot the restrictions
		*/
        HW_DESC_SET_DOUT_TYPE(&desc[*seq_size], NO_DMA,
                      sg_dma_address(dst),
                      nbytes,
                      AXI_ID, NS_BIT);
	}
	HW_DESC_STOP_QUEUE(&desc[*seq_size]);
	HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
	(*seq_size)++;

	/* dummy completion register */
	HW_DESC_INIT(&desc[*seq_size]);
	HW_DESC_SET_DIN_TYPE(&desc[*seq_size], DMA_DLLI,
			     handle->dropped_buffer_dma_addr,
			     sizeof(handle->dropped_buffer), AXI_ID, NS_BIT);
	HW_DESC_SET_DOUT_DLLI(&desc[*seq_size],
			      handle->dropped_buffer_dma_addr,
			      sizeof(handle->dropped_buffer),
			      AXI_ID, NS_BIT, (areq == NULL)? 0:1);
	HW_DESC_SET_FLOW_MODE(&desc[*seq_size], flow_mode);
	(*seq_size)++;
}

#endif /*SSI_CC_HAS_SEC_KEY*/


static int ssi_blkcipher_complete(struct device *dev,
					struct ssi_ablkcipher_ctx *ctx_p,
					struct blkcipher_req_ctx *req_ctx,
					struct scatterlist *dst, struct scatterlist *src,
					void *info, /* req info */
					unsigned int ivsize,
					void *areq,
					void __iomem *cc_base)
{
#if SSI_CC_HAS_SEC_KEY
	unsigned int mem_violation = 0 ,general_violation = 0;
#endif
	int completion_error = 0;
	uint32_t inflight_counter;
	DECL_CYCLE_COUNT_RESOURCES;

	START_CYCLE_COUNT();
	ssi_buffer_mgr_unmap_blkcipher_request(dev, req_ctx, ivsize, src, dst);
	info = req_ctx->backup_info;
	END_CYCLE_COUNT(STAT_OP_TYPE_GENERIC, STAT_PHASE_4);


	/*Set the inflight couter value to local variable*/
	inflight_counter =  ctx_p->drvdata->inflight_counter;
	/*Decrease the inflight counter*/
	if(ctx_p->flow_mode == BYPASS && ctx_p->drvdata->inflight_counter > 0)
		ctx_p->drvdata->inflight_counter--;

#if SSI_CC_HAS_SEC_KEY
	if (ctx_p->is_secure_key == 1) {
		/*read violation indicators FILTER_DROPPED_MEM_CNT and DSCRPTR_FILTER_DROPPED_CNT */
		/* in case of error will be incremented */
		mem_violation = READ_REGISTER(cc_base + SASI_REG_OFFSET(CRY_KERNEL, DSCRPTR_FILTER_DROPPED_MEM_CNT));
		general_violation = READ_REGISTER(cc_base + SASI_REG_OFFSET(CRY_KERNEL, DSCRPTR_FILTER_DROPPED_CNT));

		/*For BYPASS operation perform only reading of violation read registers to avoid errors in others special modes*/
		if (ctx_p->flow_mode == BYPASS) {
			goto end_secure_complete;
		}

		/*In case of ROM error in blob parser, GPR1 will be incremented */
		if (ctx_p->gpr1_value != READ_REGISTER(cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_HOST_GPR1))) {
			completion_error = EINVAL;
			goto end_secure_complete;
		}

		/*Check values of memory violation indicator*/
		if (((ctx_p->is_aes_ctr_prot == 1) && (mem_violation > MIN_VALID_VALUE_OF_MEMCNT_FOR_SPECIAL_MODES)) ||
		    ((ctx_p->flow_mode == BYPASS) && (mem_violation > inflight_counter))||
		    ((ctx_p->is_aes_ctr_prot != 1) && (ctx_p->flow_mode != BYPASS) && (mem_violation > 0))) {
			completion_error = EFAULT;
			goto end_secure_complete;
		}

		/*Check values of general violation indicator*/
		if (((ctx_p->is_aes_ctr_prot == 1)&&(general_violation > MIN_VALID_VALUE_OF_GENCNT_FOR_SPECIAL_MODES)) ||
		    ((ctx_p->flow_mode == BYPASS) && (general_violation > (inflight_counter * MIN_VALID_VALUE_OF_GENCNT_FOR_SPECIAL_MODES))) ||
		    ((ctx_p->is_aes_ctr_prot != 1)&& (ctx_p->flow_mode != BYPASS) && (general_violation > 0))) {
			completion_error = EPERM;
			goto end_secure_complete;
		}
	}

end_secure_complete:
	if(ctx_p->is_secure_key == 1) {
		up(&secure_key_sem);
	}
#endif
	if(areq){
		ablkcipher_request_complete(areq, completion_error);
		return 0;
	}
	return completion_error;
}

static int ssi_blkcipher_process(
	struct crypto_tfm *tfm,
	struct blkcipher_req_ctx *req_ctx,
	struct scatterlist *dst, struct scatterlist *src,
	unsigned int nbytes,
	void *info, //req info
	unsigned int ivsize,
	void *areq, 
	enum sep_crypto_direction direction)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct device *dev = &ctx_p->drvdata->plat_dev->dev;
	HwDesc_s desc[MAX_ABLKCIPHER_SEQ_LEN];
	struct ssi_crypto_req ssi_req = {0};
	int rc, seq_len = 0,cts_restore_flag = 0;
	DECL_CYCLE_COUNT_RESOURCES;

	SSI_LOG_DEBUG("%s areq=%p info=%p nbytes=%d\n",
		((direction==SEP_CRYPTO_DIRECTION_ENCRYPT)?"Encrypt":"Decrypt"),
		     areq, info, nbytes);

	CHECK_AND_RETURN_UPON_FIPS_ERROR();
	/* STAT_PHASE_0: Init and sanity checks */
	START_CYCLE_COUNT();
	
	/* TODO: check data length according to mode */
	if (unlikely(validate_data_size(ctx_p, nbytes))) {
		SSI_LOG_ERR("Unsupported data size %d.\n", nbytes);
		crypto_tfm_set_flags(tfm, CRYPTO_TFM_RES_BAD_BLOCK_LEN);
		return -EINVAL;
	}
	if (nbytes == 0) {
		/* No data to process is valid */
		return 0;
	}
        /*For CTS in case of data size aligned to 16 use CBC mode*/
	if (((nbytes % AES_BLOCK_SIZE) == 0) && (ctx_p->cipher_mode == SEP_CIPHER_CBC_CTS)){

		ctx_p->cipher_mode = SEP_CIPHER_CBC;
		cts_restore_flag = 1;
	}

#if SSI_CC_HAS_SEC_KEY
	if (ctx_p->is_secure_key == 1){
		down(&secure_key_sem);
		/*save the initial value of GPR1 in the context*/
		ctx_p->gpr1_value = READ_REGISTER(ctx_p->drvdata->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_HOST_GPR1));

	}
#endif
	/* Setup DX request structure */

	ssi_req.user_cb = (void *)ssi_ablkcipher_complete;
	ssi_req.user_arg = (void *)areq;

#ifdef ENABLE_CYCLE_COUNT
	ssi_req.op_type = (direction == SEP_CRYPTO_DIRECTION_DECRYPT) ?
		STAT_OP_TYPE_DECODE : STAT_OP_TYPE_ENCODE;

#endif

	/* Setup request context */
	req_ctx->gen_ctx.op_type = direction;
	
	END_CYCLE_COUNT(ssi_req.op_type, STAT_PHASE_0);

	/* STAT_PHASE_1: Map buffers */
	START_CYCLE_COUNT();
	
	rc = ssi_buffer_mgr_map_blkcipher_request(ctx_p->drvdata, req_ctx, ivsize, nbytes, info, src, dst);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("map_request() failed\n");
		if (ctx_p->is_secure_key == 1){
		    up(&secure_key_sem);
		}
		goto exit_process;
	}

	END_CYCLE_COUNT(ssi_req.op_type, STAT_PHASE_1);

	/* STAT_PHASE_2: Create sequence */
	START_CYCLE_COUNT();

	/* Setup processing */
#if SSI_CC_HAS_SEC_KEY
	if (ctx_p->is_secure_key == 0) {
#endif /*SSI_CC_HAS_SEC_KEY*/
#if SSI_CC_HAS_MULTI2
		if (ctx_p->flow_mode == S_DIN_to_MULTI2) {
			ssi_blkcipher_create_multi2_setup_desc(tfm,
							       req_ctx,
							       ivsize,
							       desc,
							       &seq_len);
		} else
#endif /*SSI_CC_HAS_MULTI2*/
		{
			ssi_blkcipher_create_setup_desc(ctx_p->cipher_mode,
							ctx_p->flow_mode,
							req_ctx->gen_ctx.op_type,
							ctx_p->key_dma_addr,
							ctx_p->keylen,
							req_ctx->gen_ctx.iv_dma_addr,
							ivsize,
							nbytes,
							desc,
							&seq_len);
		}
		/* Data processing */
		ssi_blkcipher_create_data_desc(tfm,
                                      req_ctx, 
                                      dst, src,
                                      nbytes,
                                      areq,
                                      desc, &seq_len);
#if SSI_CC_HAS_SEC_KEY
	} else {
		ssi_blkcipher_create_secure_key_mlli_desc(tfm, req_ctx, desc, &seq_len);
		ssi_blkcipher_create_secure_key_setup_desc(tfm, req_ctx, ivsize, desc, &seq_len);
		/* Data processing */
		ssi_blkcipher_create_secure_key_data_desc(tfm,
                                                 req_ctx,
                                                 dst, src,
                                                 nbytes,
                                                 areq,
                                                 desc, &seq_len);
		ssi_blkcipher_create_disable_secure_key_setup_desc(tfm,
								   req_ctx,
								   desc,
								   &seq_len);
	}
#endif /*SSI_CC_HAS_SEC_KEY*/

	/* do we need to generate IV? */
	if (req_ctx->is_giv == true) {
		ssi_req.ivgen_dma_addr[0] = req_ctx->gen_ctx.iv_dma_addr;
		ssi_req.ivgen_dma_addr_len = 1;
		/* set the IV size (8/16 B long)*/
		ssi_req.ivgen_size = ivsize;
	}
	END_CYCLE_COUNT(ssi_req.op_type, STAT_PHASE_2);

	/* STAT_PHASE_3: Lock HW and push sequence */
	START_CYCLE_COUNT();
	rc = ssi_send_request(ctx_p->drvdata, &ssi_req, desc, seq_len, (areq == NULL)? 0:1);
	if(areq != NULL) {
		if (unlikely(rc != -EINPROGRESS)) {
			/* Failed to send the request or request completed synchronously */
			ssi_buffer_mgr_unmap_blkcipher_request(dev, req_ctx, ivsize, src, dst);
			if (ctx_p->is_secure_key == 1){
				up(&secure_key_sem);
			}
		}

		END_CYCLE_COUNT(ssi_req.op_type, STAT_PHASE_3);
	} else {
		if (rc != 0) {
			ssi_buffer_mgr_unmap_blkcipher_request(dev, req_ctx, ivsize, src, dst);
			if (ctx_p->is_secure_key == 1){
				up(&secure_key_sem);
			}
			END_CYCLE_COUNT(ssi_req.op_type, STAT_PHASE_3);            
		} else {
			END_CYCLE_COUNT(ssi_req.op_type, STAT_PHASE_3);
			rc = ssi_blkcipher_complete(dev, ctx_p, req_ctx, dst, src, info, ivsize, NULL, ctx_p->drvdata->cc_base);
		} 
	}

exit_process:
	if (cts_restore_flag != 0)
		ctx_p->cipher_mode = SEP_CIPHER_CBC_CTS;
	return rc;
}
#if SSI_CC_HAS_SEC_KEY
static int ssi_blkcipher_process_secure_bypass(
    struct crypto_tfm *tfm,
    struct blkcipher_req_ctx *req_ctx,
    struct scatterlist *dst, struct scatterlist *src,
    unsigned int nbytes,
    void *info, //req info
    unsigned int ivsize,
    void *areq)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct device *dev = &ctx_p->drvdata->plat_dev->dev;
	HwDesc_s desc[MAX_ABLKCIPHER_SEQ_LEN];
	struct ssi_crypto_req ssi_req = {0};
	int rc, seq_len = 0;
	DECL_CYCLE_COUNT_RESOURCES;

	SSI_LOG_DEBUG("areq=%p info=%p nbytes=%d\n",
		     areq, info, nbytes);

	CHECK_AND_RETURN_UPON_FIPS_ERROR();
	/* STAT_PHASE_0: Init and sanity checks */
	START_CYCLE_COUNT();
	
	if (nbytes == 0) {
		/* No data to process is valid */
		return 0;
	}

	down(&secure_key_sem);
	if (areq != NULL) {
		/* Setup DX request structure */
		ssi_req.user_cb = (void *)ssi_ablkcipher_complete;
		ssi_req.user_arg = (void *)areq;
		/*save the initial value of GPR1 in the context*/
		ctx_p->gpr1_value = READ_REGISTER(ctx_p->drvdata->cc_base + SASI_REG_OFFSET(HOST_RGF, HOST_SEP_HOST_GPR1));
		/*Increase the inflight counter*/
		ctx_p->drvdata->inflight_counter ++;
		ssi_req.user_arg = (void *)areq;

#ifdef ENABLE_CYCLE_COUNT
		ssi_req.op_type = STAT_OP_TYPE_DECODE;

#endif
	}
	
	END_CYCLE_COUNT(SEP_CRYPTO_DIRECTION_DECRYPT, STAT_PHASE_0);

	/* STAT_PHASE_1: Map buffers */
	START_CYCLE_COUNT();
	
	rc = ssi_buffer_mgr_map_blkcipher_request(ctx_p->drvdata, req_ctx, ivsize, nbytes, info, src, dst);
	if (unlikely(rc != 0)) {
		SSI_LOG_ERR("map_request() failed\n");
		goto exit_process;
	}
	if(req_ctx->sec_dir == SSI_SRC_DMA_IS_SECURE) {
		rc = -ENOMEM;
		goto free_mem;
	}
	END_CYCLE_COUNT(SEP_CRYPTO_DIRECTION_DECRYPT, STAT_PHASE_1);

	/* STAT_PHASE_2: Create sequence */
	START_CYCLE_COUNT();

	/* Setup processing */
	ssi_blkcipher_create_secure_key_mlli_desc(tfm, req_ctx, desc, &seq_len);
	ssi_blkcipher_create_secure_bypass_setup_desc(tfm, desc, &seq_len);
	/* Data processing */
	ssi_blkcipher_create_secure_bypass_data_desc(tfm,
                                                req_ctx,
                                                dst, src,
                                                nbytes,
                                                areq,
                                                desc, &seq_len);
	ssi_blkcipher_create_disable_secure_key_setup_desc(tfm,
							   req_ctx,
							   desc,
							   &seq_len);

	END_CYCLE_COUNT(SEP_CRYPTO_DIRECTION_DECRYPT, STAT_PHASE_2);

	/* STAT_PHASE_3: Lock HW and push sequence */
	START_CYCLE_COUNT();
	
	rc = ssi_send_request(ctx_p->drvdata, &ssi_req, desc, seq_len, (areq == NULL)? 0:1);
free_mem:
	if (areq != NULL) {
		if (unlikely(rc != -EINPROGRESS)) {
			/* Failed to send the request or request completed synchronously */
			ssi_buffer_mgr_unmap_blkcipher_request(dev, req_ctx, ivsize, src, dst);
			up(&secure_key_sem);
		}

		END_CYCLE_COUNT(ssi_req.op_type, STAT_PHASE_3);
	} else {
		if (rc != 0) {
			ssi_buffer_mgr_unmap_blkcipher_request(dev, req_ctx, ivsize, src, dst);
			up(&secure_key_sem);
			END_CYCLE_COUNT(ssi_req.op_type, STAT_PHASE_3);
		} else {
			END_CYCLE_COUNT(ssi_req.op_type, STAT_PHASE_3);
			rc = ssi_blkcipher_complete(dev, ctx_p, req_ctx, dst, src, info, ivsize, NULL, ctx_p->drvdata->cc_base);
		}
	}
	
exit_process:
	
	return rc;
}
#endif//SSI_CC_HAS_SEC_KEY

static void ssi_ablkcipher_complete(struct device *dev, void *ssi_req, void __iomem *cc_base)
{
	struct ablkcipher_request *areq = (struct ablkcipher_request *)ssi_req;
	struct blkcipher_req_ctx *req_ctx = ablkcipher_request_ctx(areq);
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(areq);
	struct ssi_ablkcipher_ctx *ctx_p = crypto_ablkcipher_ctx(tfm);
	unsigned int ivsize = crypto_ablkcipher_ivsize(tfm);

	CHECK_AND_RETURN_VOID_UPON_FIPS_ERROR();

	ssi_blkcipher_complete(dev, ctx_p, req_ctx, areq->dst, areq->src, areq->info, ivsize, areq, cc_base);
}



static int ssi_sblkcipher_init(struct crypto_tfm *tfm)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);

	/* Allocate sync ctx buffer */
	ctx_p->sync_ctx = kmalloc(sizeof(struct blkcipher_req_ctx), GFP_KERNEL|GFP_DMA);
	if (!ctx_p->sync_ctx) {
		SSI_LOG_ERR("Allocating sync ctx buffer in context failed\n");
		return -ENOMEM;
	}
	SSI_LOG_DEBUG("Allocated sync ctx buffer in context ctx_p->sync_ctx=@%p\n",
								ctx_p->sync_ctx);

	return ssi_blkcipher_init(tfm);
}


static void ssi_sblkcipher_exit(struct crypto_tfm *tfm)
{
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	
	kfree(ctx_p->sync_ctx);
	SSI_LOG_DEBUG("Free sync ctx buffer in context ctx_p->sync_ctx=@%p\n", ctx_p->sync_ctx);

	ssi_blkcipher_exit(tfm);
}


static int ssi_sblkcipher_encrypt(struct blkcipher_desc *desc,
                        struct scatterlist *dst, struct scatterlist *src,
                        unsigned int nbytes)
{
	struct crypto_blkcipher *blk_tfm = desc->tfm;
	struct crypto_tfm *tfm = crypto_blkcipher_tfm(blk_tfm);
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct blkcipher_req_ctx *req_ctx = ctx_p->sync_ctx;
	unsigned int ivsize = crypto_blkcipher_ivsize(blk_tfm);

	req_ctx->backup_info = desc->info;
	req_ctx->is_giv = false;

	return ssi_blkcipher_process(tfm, req_ctx, dst, src, nbytes, desc->info, ivsize, NULL, SEP_CRYPTO_DIRECTION_ENCRYPT);
}

static int ssi_sblkcipher_decrypt(struct blkcipher_desc *desc,
                        struct scatterlist *dst, struct scatterlist *src,
                        unsigned int nbytes)
{
	struct crypto_blkcipher *blk_tfm = desc->tfm;
	struct crypto_tfm *tfm = crypto_blkcipher_tfm(blk_tfm);
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct blkcipher_req_ctx *req_ctx = ctx_p->sync_ctx;
	unsigned int ivsize = crypto_blkcipher_ivsize(blk_tfm);

	req_ctx->backup_info = desc->info;
	req_ctx->is_giv = false;

	return ssi_blkcipher_process(tfm, req_ctx, dst, src, nbytes, desc->info, ivsize, NULL, SEP_CRYPTO_DIRECTION_DECRYPT);
}

#if SSI_CC_HAS_SEC_KEY
static int ssi_sblkcipher_process_secure_bypass(struct blkcipher_desc *desc,
                        struct scatterlist *dst, struct scatterlist *src,
                        unsigned int nbytes)
{
	struct crypto_blkcipher *blk_tfm = desc->tfm;
	struct crypto_tfm *tfm = crypto_blkcipher_tfm(blk_tfm);
	struct ssi_ablkcipher_ctx *ctx_p = crypto_tfm_ctx(tfm);
	struct blkcipher_req_ctx *req_ctx = ctx_p->sync_ctx;
	unsigned int ivsize = crypto_blkcipher_ivsize(blk_tfm);

	req_ctx->backup_info = desc->info;
	req_ctx->is_giv = false;

	return ssi_blkcipher_process_secure_bypass(tfm, req_ctx, 
                                              dst, src, 
                                              nbytes, desc->info, 
                                              ivsize, NULL);
}
#endif//SSI_CC_HAS_SEC_KEY

/* Async wrap functions */

static int ssi_ablkcipher_init(struct crypto_tfm *tfm)
{
	struct ablkcipher_tfm *ablktfm = &tfm->crt_ablkcipher;
	
	ablktfm->reqsize = sizeof(struct blkcipher_req_ctx);

	return ssi_blkcipher_init(tfm);
}


static int ssi_ablkcipher_setkey(struct crypto_ablkcipher *tfm, 
				const u8 *key, 
				unsigned int keylen)
{
	return ssi_blkcipher_setkey(crypto_ablkcipher_tfm(tfm), key, keylen);
}

#if SSI_CC_HAS_SEC_KEY
static int ssi_ablkcipher_secure_key_setkey(struct crypto_ablkcipher *tfm,
					   const u8 *key, unsigned int keylen)
{
	return ssi_blkcipher_secure_key_setkey(crypto_ablkcipher_tfm(tfm), key, keylen);
}

static int ssi_ablkcipher_process_secure_bypass(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *ablk_tfm = crypto_ablkcipher_reqtfm(req);
	struct crypto_tfm *tfm = crypto_ablkcipher_tfm(ablk_tfm);
	struct blkcipher_req_ctx *req_ctx = ablkcipher_request_ctx(req);
	unsigned int ivsize = crypto_ablkcipher_ivsize(ablk_tfm);

	return ssi_blkcipher_process_secure_bypass(tfm, req_ctx, 
                                              req->dst, req->src, 
                                              req->nbytes, req->info, 
                                              ivsize, (void *) req);
}

#endif /*SSI_CC_HAS_SEC_KEY*/

static int ssi_ablkcipher_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *ablk_tfm = crypto_ablkcipher_reqtfm(req);
	struct crypto_tfm *tfm = crypto_ablkcipher_tfm(ablk_tfm);
	struct blkcipher_req_ctx *req_ctx = ablkcipher_request_ctx(req);
	unsigned int ivsize = crypto_ablkcipher_ivsize(ablk_tfm);

	req_ctx->backup_info = req->info;
	req_ctx->is_giv = false;

	return ssi_blkcipher_process(tfm, req_ctx, req->dst, req->src, req->nbytes, req->info, ivsize, (void *)req, SEP_CRYPTO_DIRECTION_ENCRYPT);
}

static int ssi_ablkcipher_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *ablk_tfm = crypto_ablkcipher_reqtfm(req);
	struct crypto_tfm *tfm = crypto_ablkcipher_tfm(ablk_tfm);
	struct blkcipher_req_ctx *req_ctx = ablkcipher_request_ctx(req);
	unsigned int ivsize = crypto_ablkcipher_ivsize(ablk_tfm);

	req_ctx->backup_info = req->info;
	req_ctx->is_giv = false;
	return ssi_blkcipher_process(tfm, req_ctx, req->dst, req->src, req->nbytes, req->info, ivsize, (void *)req, SEP_CRYPTO_DIRECTION_DECRYPT);
}


static int ssi_ablkcipher_givencrypt(struct skcipher_givcrypt_request *req)
{
	struct crypto_ablkcipher *ablk_tfm = skcipher_givcrypt_reqtfm(req);
    
	struct ablkcipher_request *subreq = skcipher_givcrypt_reqctx(req);
	struct blkcipher_req_ctx *req_ctx = ablkcipher_request_ctx(subreq);
	unsigned int ivsize = crypto_ablkcipher_ivsize(ablk_tfm);
	struct crypto_ablkcipher *ablk_gen_tfm;
	struct crypto_tfm *tfm;
    
	int rc;

	CHECK_AND_RETURN_UPON_FIPS_ERROR();

	/*set the correct tfm and req params*/
	ablkcipher_request_set_tfm(subreq, skcipher_geniv_cipher(ablk_tfm));
	ablkcipher_request_set_crypt(subreq, req->creq.src, req->creq.dst,
				     req->creq.nbytes, req->creq.info);
	ablkcipher_request_set_callback(subreq, req->creq.base.flags,
					req->creq.base.complete,
					req->creq.base.data);
	/* Backup orig. IV */
	req_ctx->backup_info = subreq->info;
	/* request IV points to payload's header */
	subreq->info = req->giv;
	/*set the giv flag */
	req_ctx->is_giv = true;

	ablk_gen_tfm = crypto_ablkcipher_reqtfm(subreq);
	tfm = crypto_ablkcipher_tfm(ablk_gen_tfm);

	rc = ssi_blkcipher_process(tfm, req_ctx, subreq->dst, subreq->src, subreq->nbytes, subreq->info, ivsize, (void *)subreq, SEP_CRYPTO_DIRECTION_ENCRYPT);
	if (rc != -EINPROGRESS)
		subreq->info = req_ctx->backup_info;

	return rc;
}

/* DX Block cipher alg */
static struct ssi_alg_template blkcipher_algs[] = {
/* Sync Template */
#if SSI_CC_HAS_AES_XTS
	{
		.name = "xts(aes)",
		.driver_name = "xts-aes-dx-sync",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE * 2,
			.max_keysize = AES_MAX_KEY_SIZE * 2,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_XTS,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = true,
	},
#endif /*SSI_CC_HAS_AES_XTS*/
	{
		.name = "ecb(aes)",
		.driver_name = "ecb-aes-dx-sync",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = 0,
			},
		.cipher_mode = SEP_CIPHER_ECB,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = true,
	},
	{
		.name = "cbc(aes)",
		.driver_name = "cbc-aes-dx-sync",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CBC,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = true,
	},
	{
		.name = "ofb(aes)",
		.driver_name = "ofb-aes-dx-sync",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_OFB,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = true,
	},
#if SSI_CC_HAS_AES_CTS
	{
		.name = "cts1(cbc(aes))",
		.driver_name = "cts1-cbc-aes-dx-sync",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CBC_CTS,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = true,
	},
#endif
	{
		.name = "ctr(aes)",
		.driver_name = "ctr-aes-dx-sync",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CTR,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = true,
	},
	{
		.name = "cbc(des3_ede)",
		.driver_name = "cbc-3des-dx-sync",
		.blocksize = DES3_EDE_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.ivsize = DES3_EDE_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CBC,
		.flow_mode = S_DIN_to_DES,
		.is_secure_key = 0,
        .synchronous = true,
	},
	{
		.name = "ecb(des3_ede)",
		.driver_name = "ecb-3des-dx-sync",
		.blocksize = DES3_EDE_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.ivsize = 0,
			},
		.cipher_mode = SEP_CIPHER_ECB,
		.flow_mode = S_DIN_to_DES,
		.is_secure_key = 0,
        .synchronous = true,
	},
	{
		.name = "cbc(des)",
		.driver_name = "cbc-des-dx-sync",
		.blocksize = DES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.ivsize = DES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CBC,
		.flow_mode = S_DIN_to_DES,
		.is_secure_key = 0,
        .synchronous = true,
	},
	{
		.name = "ecb(des)",
		.driver_name = "ecb-des-dx-sync",
		.blocksize = DES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.ivsize = 0,
			},
		.cipher_mode = SEP_CIPHER_ECB,
		.flow_mode = S_DIN_to_DES,
		.is_secure_key = 0,
        .synchronous = true,
	},
#if SSI_CC_HAS_MULTI2
	{
		.name = "cbc(multi2)",
		.driver_name = "cbc-multi2-dx-sync",
		.blocksize = SEP_MULTI2_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE + 1,
			.max_keysize = SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE + 1,
			.ivsize = SEP_MULTI2_IV_SIZE,
			},
		.cipher_mode = SEP_MULTI2_CBC,
		.flow_mode = S_DIN_to_MULTI2,
		.is_secure_key = 0,
        .synchronous = true,
	},
	{
		.name = "ofb(multi2)",
		.driver_name = "ofb-multi2-dx-sync",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_encrypt,
			.min_keysize = SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE + 1,
			.max_keysize = SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE + 1,
			.ivsize = SEP_MULTI2_IV_SIZE,
			},
		.cipher_mode = SEP_MULTI2_OFB,
		.flow_mode = S_DIN_to_MULTI2,
		.is_secure_key = 0,
        .synchronous = true,
	},
#endif /*SSI_CC_HAS_MULTI2*/
#if SSI_CC_HAS_SEC_KEY
	{
		.name = "cbc(saes)",
		.driver_name = "cbc-saes-dx-sync",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_secure_key_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CBC,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 1,
        .synchronous = true,
	},
#if SSI_CC_HAS_AES_CTS
	{
		.name = "cts1(cbc(saes))",
		.driver_name = "cts1-cbc-saes-dx-sync",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_secure_key_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CBC_CTS,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 1,
        .synchronous = true,
	},
#endif /*SSI_CC_HAS_AES_CTS*/
	{
		.name = "ctr(saes)",
		.driver_name = "ctr-saes-dx-sync",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_secure_key_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CTR,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 1,
        .synchronous = true,
	},
#if SSI_CC_HAS_MULTI2
	{
		.name = "cbc(smulti2)",
		.driver_name = "cbc-smulti2-dx-sync",
		.blocksize = SEP_MULTI2_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_secure_key_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_decrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = SEP_MULTI2_IV_SIZE,
			},
		.cipher_mode = SEP_MULTI2_CBC,
		.flow_mode = S_DIN_to_MULTI2,
		.is_secure_key = 1,
        .synchronous = true,
	},
	{
		.name = "ofb(smulti2)",
		.driver_name = "ofb-smulti2-dx-sync",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_secure_key_setkey,
			.encrypt = ssi_sblkcipher_encrypt,
			.decrypt = ssi_sblkcipher_encrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = SEP_MULTI2_IV_SIZE,
			},
		.cipher_mode = SEP_MULTI2_OFB,
		.flow_mode = S_DIN_to_MULTI2,
		.is_secure_key = 1,
        .synchronous = true,
	},
#endif /*SSI_CC_HAS_MULTI2*/
	{
		.name = "sbypass",
		.driver_name = "sbypass-dx-sync",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_BLKCIPHER,
		.template_sblkcipher = {
			.setkey = ssi_blkcipher_secure_key_setkey,
			.encrypt = ssi_sblkcipher_process_secure_bypass,
			.decrypt = ssi_sblkcipher_process_secure_bypass,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			},
		.cipher_mode = SEP_CIPHER_NULL_MODE,
		.flow_mode = BYPASS,
		.is_secure_key = 1,
        .synchronous = true,
	},
#endif /*SSI_CC_HAS_SEC_KEY*/
/* Async template */
#if SSI_CC_HAS_AES_XTS
	{
		.name = "xts(aes)",
		.driver_name = "xts-aes-dx",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE * 2,
			.max_keysize = AES_MAX_KEY_SIZE * 2,
			.ivsize = AES_BLOCK_SIZE,
			.geniv = "dx-ablkcipher-gen-iv",
			},
		.cipher_mode = SEP_CIPHER_XTS,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = false,
	},
#endif /*SSI_CC_HAS_AES_XTS*/
	{
		.name = "ecb(aes)",
		.driver_name = "ecb-aes-dx",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = 0,
			},
		.cipher_mode = SEP_CIPHER_ECB,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = false,
	},
	{
		.name = "cbc(aes)",
		.driver_name = "cbc-aes-dx",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
			.geniv = "dx-ablkcipher-gen-iv",
			},
		.cipher_mode = SEP_CIPHER_CBC,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = false,
	},
	{
		.name = "ofb(aes)",
		.driver_name = "ofb-aes-dx",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
			.geniv = "dx-ablkcipher-gen-iv",
			},
		.cipher_mode = SEP_CIPHER_OFB,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = false,
	},
#if SSI_CC_HAS_AES_CTS
	{
		.name = "cts1(cbc(aes))",
		.driver_name = "cts1-cbc-aes-dx",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
			.geniv = "dx-ablkcipher-gen-iv",
			},
		.cipher_mode = SEP_CIPHER_CBC_CTS,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = false,
	},
#endif
	{
		.name = "ctr(aes)",
		.driver_name = "ctr-aes-dx",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
			.geniv = "dx-ablkcipher-gen-iv",
			},
		.cipher_mode = SEP_CIPHER_CTR,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 0,
        .synchronous = false,
	},
	{
		.name = "cbc(des3_ede)",
		.driver_name = "cbc-3des-dx",
		.blocksize = DES3_EDE_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.ivsize = DES3_EDE_BLOCK_SIZE,
			.geniv = "dx-ablkcipher-gen-iv",
			},
		.cipher_mode = SEP_CIPHER_CBC,
		.flow_mode = S_DIN_to_DES,
		.is_secure_key = 0,
        .synchronous = false,
	},
	{
		.name = "ecb(des3_ede)",
		.driver_name = "ecb-3des-dx",
		.blocksize = DES3_EDE_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.ivsize = 0,
			},
		.cipher_mode = SEP_CIPHER_ECB,
		.flow_mode = S_DIN_to_DES,
		.is_secure_key = 0,
        .synchronous = false,
	},
	{
		.name = "cbc(des)",
		.driver_name = "cbc-des-dx",
		.blocksize = DES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.ivsize = DES_BLOCK_SIZE,
			.geniv = "dx-ablkcipher-gen-iv",
			},
		.cipher_mode = SEP_CIPHER_CBC,
		.flow_mode = S_DIN_to_DES,
		.is_secure_key = 0,
        .synchronous = false,
	},
	{
		.name = "ecb(des)",
		.driver_name = "ecb-des-dx",
		.blocksize = DES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.ivsize = 0,
			},
		.cipher_mode = SEP_CIPHER_ECB,
		.flow_mode = S_DIN_to_DES,
		.is_secure_key = 0,
        .synchronous = false,
	},
#if SSI_CC_HAS_MULTI2
	{
		.name = "cbc(multi2)",
		.driver_name = "cbc-multi2-dx",
		.blocksize = SEP_MULTI2_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE + 1,
			.max_keysize = SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE + 1,
			.ivsize = SEP_MULTI2_IV_SIZE,
			.geniv = "dx-ablkcipher-gen-iv",
			},
		.cipher_mode = SEP_MULTI2_CBC,
		.flow_mode = S_DIN_to_MULTI2,
		.is_secure_key = 0,
        .synchronous = false,
	},
	{
		.name = "ofb(multi2)",
		.driver_name = "ofb-multi2-dx",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_encrypt,
			.min_keysize = SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE + 1,
			.max_keysize = SEP_MULTI2_SYSTEM_N_DATA_KEY_SIZE + 1,
			.ivsize = SEP_MULTI2_IV_SIZE,
			.geniv = "dx-ablkcipher-gen-iv",
			},
		.cipher_mode = SEP_MULTI2_OFB,
		.flow_mode = S_DIN_to_MULTI2,
		.is_secure_key = 0,
        .synchronous = false,
	},
#endif /*SSI_CC_HAS_MULTI2*/
#if SSI_CC_HAS_SEC_KEY
	{
		.name = "cbc(saes)",
		.driver_name = "cbc-saes-dx",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_secure_key_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CBC,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 1,
        .synchronous = false,
	},
#if SSI_CC_HAS_AES_CTS
	{
		.name = "cts1(cbc(saes))",
		.driver_name = "cts1-cbc-saes-dx",
		.blocksize = AES_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_secure_key_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CBC_CTS,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 1,
        .synchronous = false,
	},
#endif /*SSI_CC_HAS_AES_CTS*/
	{
		.name = "ctr(saes)",
		.driver_name = "ctr-saes-dx",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_secure_key_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = AES_BLOCK_SIZE,
			},
		.cipher_mode = SEP_CIPHER_CTR,
		.flow_mode = S_DIN_to_AES,
		.is_secure_key = 1,
        .synchronous = false,
	},
#if SSI_CC_HAS_MULTI2
	{
		.name = "cbc(smulti2)",
		.driver_name = "cbc-smulti2-dx",
		.blocksize = SEP_MULTI2_BLOCK_SIZE,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_secure_key_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_decrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = SEP_MULTI2_IV_SIZE,
			},
		.cipher_mode = SEP_MULTI2_CBC,
		.flow_mode = S_DIN_to_MULTI2,
		.is_secure_key = 1,
        .synchronous = false,
	},
	{
		.name = "ofb(smulti2)",
		.driver_name = "ofb-smulti2-dx",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_secure_key_setkey,
			.encrypt = ssi_ablkcipher_encrypt,
			.decrypt = ssi_ablkcipher_encrypt,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.ivsize = SEP_MULTI2_IV_SIZE,
			},
		.cipher_mode = SEP_MULTI2_OFB,
		.flow_mode = S_DIN_to_MULTI2,
		.is_secure_key = 1,
        .synchronous = false,
	},
#endif /*SSI_CC_HAS_MULTI2*/
	{
		.name = "sbypass",
		.driver_name = "sbypass-dx",
		.blocksize = 1,
		.type = CRYPTO_ALG_TYPE_ABLKCIPHER,
		.template_ablkcipher = {
			.setkey = ssi_ablkcipher_secure_key_setkey,
			.encrypt = ssi_ablkcipher_process_secure_bypass,
			.decrypt = ssi_ablkcipher_process_secure_bypass,
			.min_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			.max_keysize = DX_SECURE_KEY_PACKAGE_BUF_SIZE_IN_BYTES,
			},
		.cipher_mode = SEP_CIPHER_NULL_MODE,
		.flow_mode = BYPASS,
		.is_secure_key = 1,
        .synchronous = false,
	},
#endif /*SSI_CC_HAS_SEC_KEY*/
};

static 
struct ssi_crypto_alg *ssi_ablkcipher_create_alg(struct ssi_alg_template *template)
{
	struct ssi_crypto_alg *t_alg;
	struct crypto_alg *alg;

	t_alg = kzalloc(sizeof(struct ssi_crypto_alg), GFP_KERNEL);
	if (!t_alg) {
		SSI_LOG_ERR("failed to allocate t_alg\n");
		return ERR_PTR(-ENOMEM);
	}

	alg = &t_alg->crypto_alg;

	snprintf(alg->cra_name, CRYPTO_MAX_ALG_NAME, "%s", template->name);
	snprintf(alg->cra_driver_name, CRYPTO_MAX_ALG_NAME, "%s",
		 template->driver_name);
	alg->cra_module = THIS_MODULE;
	alg->cra_priority = SSI_CRA_PRIO;
	alg->cra_blocksize = template->blocksize;
	alg->cra_alignmask = 0;
	alg->cra_ctxsize = sizeof(struct ssi_ablkcipher_ctx);
	
	alg->cra_init = template->synchronous? ssi_sblkcipher_init:ssi_ablkcipher_init;
	alg->cra_exit = template->synchronous? ssi_sblkcipher_exit:ssi_blkcipher_exit;
	alg->cra_type = template->synchronous? &crypto_blkcipher_type:&crypto_ablkcipher_type;
	if(template->synchronous) {
		alg->cra_blkcipher = template->template_sblkcipher;
		alg->cra_flags = CRYPTO_ALG_KERN_DRIVER_ONLY |
				template->type;
	} else {
		alg->cra_ablkcipher = template->template_ablkcipher;
		alg->cra_flags = CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY |
				template->type;
	}
	

	t_alg->cipher_mode = template->cipher_mode;
	t_alg->flow_mode = template->flow_mode;
	t_alg->is_secure_key = template->is_secure_key;

	return t_alg;
}

static struct crypto_template ssi_ablkcipher_giv_tmpl;

int ssi_ablkcipher_free(struct ssi_drvdata *drvdata)
{
	struct ssi_crypto_alg *t_alg, *n;
	struct ssi_blkcipher_handle *blkcipher_handle = 
						drvdata->blkcipher_handle;
	struct device *dev;
	dev = &drvdata->plat_dev->dev;

	if (blkcipher_handle != NULL) {
		/* Remove registered algs */
		list_for_each_entry_safe(t_alg, n,
				&blkcipher_handle->blkcipher_alg_list,
					 entry) {
			crypto_unregister_alg(&t_alg->crypto_alg);
			list_del(&t_alg->entry);
			kfree(t_alg);
		}
		if (blkcipher_handle->dropped_buffer_dma_addr != 0) {
			SSI_RESTORE_DMA_ADDR_TO_48BIT(
				blkcipher_handle->dropped_buffer_dma_addr);
			dma_unmap_single(dev,
			blkcipher_handle->dropped_buffer_dma_addr,
			sizeof(blkcipher_handle->dropped_buffer),
			DMA_TO_DEVICE);
			blkcipher_handle->dropped_buffer_dma_addr = 0;
		}
		kfree(blkcipher_handle);
		drvdata->blkcipher_handle = NULL;

		crypto_unregister_template(&ssi_ablkcipher_giv_tmpl);
	}
	return 0;
}



int ssi_ablkcipher_alloc(struct ssi_drvdata *drvdata)
{
	struct ssi_blkcipher_handle *ablkcipher_handle;
	struct ssi_crypto_alg *t_alg;
	int rc = -ENOMEM;
	int alg;

	ablkcipher_handle = kmalloc(sizeof(struct ssi_blkcipher_handle),
		GFP_KERNEL);
	if (ablkcipher_handle == NULL)
		return -ENOMEM;

	drvdata->blkcipher_handle = ablkcipher_handle;
	ablkcipher_handle->dropped_buffer_dma_addr = dma_map_single(
		&drvdata->plat_dev->dev,
		&ablkcipher_handle->dropped_buffer,
		sizeof(ablkcipher_handle->dropped_buffer),
		DMA_TO_DEVICE);
	if (ablkcipher_handle->dropped_buffer_dma_addr == 0) {
		SSI_LOG_ERR("Mapping dummy DMA failed\n");
		goto fail0;
	}
	SSI_UPDATE_DMA_ADDR_TO_48BIT(ablkcipher_handle->dropped_buffer_dma_addr,
				    sizeof(ablkcipher_handle->dropped_buffer));
#if SSI_CC_HAS_SEC_KEY
	/*set 32 low bits*/
	WRITE_REGISTER(drvdata->cc_base + 
		       SASI_REG_OFFSET(CRY_KERNEL,
			       DSCRPTR_FILTER_DROPPED_ADDRESS),
		       (ablkcipher_handle->dropped_buffer_dma_addr & 
								UINT32_MAX));
	/*set 32 high bits*/
	WRITE_REGISTER(drvdata->cc_base + 
		       SASI_REG_OFFSET(CRY_KERNEL,
			       DSCRPTR_FILTER_DROPPED_ADDRESS_HIGH),
		       (ablkcipher_handle->dropped_buffer_dma_addr >> 32));
#endif

	INIT_LIST_HEAD(&ablkcipher_handle->blkcipher_alg_list);

	/* Linux crypto */
	SSI_LOG_DEBUG("Number of algorithms = %zu\n", ARRAY_SIZE(blkcipher_algs));
	for (alg = 0; alg < ARRAY_SIZE(blkcipher_algs); alg++) {
		if (blkcipher_algs[alg].synchronous)
			continue;
		SSI_LOG_DEBUG("creating %s\n", blkcipher_algs[alg].driver_name);
		t_alg = ssi_ablkcipher_create_alg(&blkcipher_algs[alg]);
		if (IS_ERR(t_alg)) {
			rc = PTR_ERR(t_alg);
			SSI_LOG_ERR("%s alg allocation failed\n",
				 blkcipher_algs[alg].driver_name);
			goto fail0;
		}
		t_alg->drvdata = drvdata;

		SSI_LOG_DEBUG("registering %s\n", blkcipher_algs[alg].driver_name);
		rc = crypto_register_alg(&t_alg->crypto_alg);
		SSI_LOG_DEBUG("%s alg registration rc = %x\n",
			t_alg->crypto_alg.cra_driver_name, rc);
		if (unlikely(rc != 0)) {
			SSI_LOG_ERR("%s alg registration failed\n",
				t_alg->crypto_alg.cra_driver_name);
			kfree(t_alg);
			goto fail0;
		} else {
			list_add_tail(&t_alg->entry, 
				      &ablkcipher_handle->blkcipher_alg_list);
			SSI_LOG_DEBUG("Registered %s\n", 
					t_alg->crypto_alg.cra_driver_name);
		}
	}
	rc = crypto_register_template(&ssi_ablkcipher_giv_tmpl);
	if (rc != 0) {
		goto fail0;
	}
	return 0;

fail0:
	ssi_ablkcipher_free(drvdata);
	return rc;
}

static int ssi_ablkcipher_giv_init(struct crypto_tfm *tfm)
{
	CHECK_AND_RETURN_UPON_FIPS_TEE_ERROR();
	tfm->crt_ablkcipher.reqsize = sizeof(struct ablkcipher_request) + sizeof(struct blkcipher_req_ctx);
	return skcipher_geniv_init(tfm);
}

static void ssi_ablkcipher_giv_exit(struct crypto_tfm *tfm)
{
	return skcipher_geniv_exit(tfm);
}

static struct crypto_instance *ssi_ablkcipher_giv_alloc(struct rtattr **tb)
{
	struct crypto_instance *inst;

	inst = skcipher_geniv_alloc(&ssi_ablkcipher_giv_tmpl, tb, 0, 0);
	if (IS_ERR(inst))
		return inst;


	inst->alg.cra_ablkcipher.givencrypt = ssi_ablkcipher_givencrypt;

	inst->alg.cra_init = ssi_ablkcipher_giv_init;
	inst->alg.cra_exit = ssi_ablkcipher_giv_exit;


	return inst;

}

static void ssi_ablkcipher_giv_free(struct crypto_instance *inst)
{
	skcipher_geniv_free(inst);
}

static struct crypto_template ssi_ablkcipher_giv_tmpl = {
	.name = "dx-ablkcipher-gen-iv",
	.alloc = ssi_ablkcipher_giv_alloc,
	.free = ssi_ablkcipher_giv_free,
	.module = THIS_MODULE,
};

