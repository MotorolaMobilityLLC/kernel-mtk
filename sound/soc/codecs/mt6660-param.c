/*
 *  sound/soc/codecs/mt6660-param.c
 *  Driver to Mediatek MT6660 SPKAMP Param
 *
 *  Copyright (C) 2018 Mediatek Inc.
 *  cy_huang <cy_huang@richtek.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <sound/soc.h>
/* param security purpose */
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <crypto/akcipher.h>
#include <crypto/algapi.h>
#include <linux/crc32.h>
#include <crypto/hash.h>

#include "mt6660.h"

struct mt6660_param_drvdata {
	struct device *dev;
	struct mt6660_chip *chip;
	void *param;
	int param_size;
};

struct sec_header {
	char tag[8];
	u16 BHV;
	u16 MSHV;
	u32 TBS;
	char ICTagString[16];
	char BinDesc[96];
	char AuthorInfo[80];
	u8 date[16];
	u32 EHO;
	u32 EHS;
	u32 EDO;
	u32 EDS;
	u8 pub_key[144];
	u8 signature[128];
} __packed;

/* rsa 1024 sig */
#define SIG_SIZE	(128)
/* rsa 1024 pub key */
#define PUBKEY_SIZE	(140)
/* SHA256 Digest */
#define DIG_SIZE	(32)

static int mt6660_param_header_digest(struct device *dev, const void *in,
				      int in_cnt, void *out, int out_cnt)
{
	struct crypto_shash *tfm;
	int size, ret = 0;

	dev_dbg(dev, "%s ++\n", __func__);
	if (!dev || !in || !out || in_cnt == 0 || out_cnt != DIG_SIZE)
		return -EINVAL;
	/* sha256 digest gen */
	tfm = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	{
		SHASH_DESC_ON_STACK(desc, tfm);
		desc->tfm = tfm;
		desc->flags = CRYPTO_TFM_REQ_MAY_SLEEP;
		size = offsetof(struct sec_header, pub_key);
		ret = crypto_shash_digest(desc, in, size, out);
		if (ret < 0)
			dev_err(dev, "%s: gen digest fail\n", __func__);
		shash_desc_zero(desc);
	}
	crypto_free_shash(tfm);
	dev_dbg(dev, "%s --\n", __func__);
	return ret;
}

struct crypto_result {
	struct completion completion;
	int err;
};

static const u8 RSA_digest_info_SHA256[] = {
	0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
	0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
	0x00, 0x04, 0x20
};

static void mt6660_param_key_verify_done(struct crypto_async_request *req,
					 int err)
{
	struct crypto_result *compl = req->data;

	if (err == -EINPROGRESS)
		return;
	compl->err = err;
	complete(&compl->completion);
}

static int pkcs_1_v1_5_decode_emsa(struct device *dev, const u8 *in, int inlen)
{
	const u8 *asn1_template = RSA_digest_info_SHA256;
	int asn1_size = ARRAY_SIZE(RSA_digest_info_SHA256);
	int hash_size = 32;
	unsigned int t_offset, ps_end, ps_start, i;

	if (!dev || !in || !inlen)
		return -EINVAL;
	if (inlen < 2 + 1 + asn1_size + hash_size)
		return -EINVAL;

	ps_start = 2;
	if (in[0] != 0x00 && in[1] != 0x01) {
		dev_err(dev, "[in[0] == %02u], [in[1] = %02u]\n", in[0], in[1]);
		return -EBADMSG;
	}

	t_offset = inlen - (asn1_size + hash_size);
	ps_end = t_offset - 1;
	if (in[ps_end] != 0x00) {
		dev_err(dev, "[in[T-1] == %02u]\n", in[ps_end]);
		return -EBADMSG;
	}
	for (i = ps_start; i < ps_end; i++) {
		if (in[i] != 0xff) {
			dev_err(dev, "[in[PS%x] == %02u]\n", i - 2, in[i]);
			return -EBADMSG;
		}
	}
	if (crypto_memneq(asn1_template, in + t_offset, asn1_size) != 0) {
		dev_err(dev, "[in[T] ASN.1 mismatch]\n");
		return -EBADMSG;
	}
	return t_offset + asn1_size;
}

static int mt6660_param_sig_output(struct device *dev,
				   const struct sec_header *header,
				   const void *dig, int dig_size)
{
	struct crypto_result compl;
	struct crypto_akcipher *tfm;
	struct akcipher_request *req;
	struct scatterlist sig_sg, digest_sg;
	void *output;
	unsigned int outlen;
	int ret = 0;

	dev_dbg(dev, "%s ++\n", __func__);
	if (!dev || !header || !dig || dig_size < DIG_SIZE)
		return -EINVAL;
	tfm = crypto_alloc_akcipher("rsa", 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	req = akcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		goto err_free_tfm;
	ret = crypto_akcipher_set_pub_key(tfm, header->pub_key, PUBKEY_SIZE);
	if (ret < 0)
		goto err_free_req;
	outlen = crypto_akcipher_maxsize(tfm);
	output = devm_kzalloc(dev, outlen, GFP_KERNEL);
	if (!output)
		goto err_free_req;
	sg_init_one(&sig_sg, header->signature, SIG_SIZE);
	sg_init_one(&digest_sg, output, outlen);
	akcipher_request_set_crypt(req, &sig_sg, &digest_sg, SIG_SIZE, outlen);
	init_completion(&compl.completion);
	akcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG |
				      CRYPTO_TFM_REQ_MAY_SLEEP,
				      mt6660_param_key_verify_done, &compl);
	ret = crypto_akcipher_verify(req);
	if (ret == -EINPROGRESS) {
		wait_for_completion(&compl.completion);
		ret = compl.err;
	}
	if (ret < 0)
		goto out_free_output;
	ret = pkcs_1_v1_5_decode_emsa(dev, output, req->dst_len);
	if (ret < 0)
		goto out_free_output;
	if (memcmp(dig, output + ret, DIG_SIZE) != 0)
		ret = -EKEYREJECTED;
	else
		ret = 0;
	dev_dbg(dev, "%s result [%d] --\n", __func__, ret);
out_free_output:
	devm_kfree(dev, output);
err_free_req:
	akcipher_request_free(req);
err_free_tfm:
	crypto_free_akcipher(tfm);
	return ret;
}

static int mt6660_param_header_check_signature(struct device *dev,
					       const char *buf, size_t cnt)
{
	const struct sec_header *header = (const struct sec_header *)buf;
	u8 digest[DIG_SIZE];
	int size, ret;

	if (cnt < sizeof(*header))
		return -EINVAL;
	/* sha256 digest gen */
	memset(digest, 0, DIG_SIZE);
	size = offsetof(struct sec_header, pub_key);
	ret = mt6660_param_header_digest(dev, header, size, digest, DIG_SIZE);
	if (ret < 0) {
		dev_err(dev, "digest fail\n");
		return ret;
	}
	return mt6660_param_sig_output(dev, header, digest, DIG_SIZE);
}

static int mt6660_param_header_validate(struct device *dev,
					const char *buf, size_t cnt)
{
	const struct sec_header *header = (const struct sec_header *)buf;
	u32 size = sizeof(*header);
	int ret;

	/* check signature pub_key digest */
	ret = mt6660_param_header_check_signature(dev, buf, cnt);
	if (ret < 0) {
		dev_err(dev, "check header signature fail\n");
		return ret;
	}
	/* check bin size match */
	size += (header->EDO + header->EDS);
	if (size != header->TBS || header->TBS != cnt) {
		dev_err(dev, "check size fail\n");
		return -EINVAL;
	}
	return 0;
}

static int mt6660_param_data_validate(struct device *dev,
				      const char *buf, size_t cnt)
{
	const struct sec_header *header = (const struct sec_header *)buf;
	u8 *data;
	int offset = sizeof(*header);
	u32 crc, calc_crc, data_len;

	if (header->EDS < 4) {
		dev_warn(dev, "ignore param data check\n");
		return 0;
	}
	offset += (header->EDO);
	crc = *(u32 *)(buf + offset);
	data_len = header->EDS - 4;
	data = (u8 *)(buf + offset + 4);
	calc_crc = crc32_be(-1, data, data_len);
	if (calc_crc != crc) {
		dev_err(dev, "crc not match, crc 0x%08x, calc_crc = 0x%08x\n",
			crc, calc_crc);
		return -EINVAL;
	}
	return 0;
}

static int mt6660_param_data_trigger(struct device *dev,
				     const char *buf, size_t count)
{
	struct mt6660_param_drvdata *p_drvdata = dev_get_drvdata(dev);
	const struct sec_header *header = (const struct sec_header *)buf;
	void *data;
	u32 data_len;
	int ret;

	/* copy buf to private variable */
	if (p_drvdata->param) {
		devm_kfree(dev, p_drvdata->param);
		p_drvdata->param = NULL;
		p_drvdata->param_size = 0;
	}
	if (!p_drvdata->param) {
		p_drvdata->param = devm_kzalloc(dev, count, GFP_KERNEL);
		if (!p_drvdata->param)
			return -ENOMEM;
		p_drvdata->param_size = count;
		memcpy(p_drvdata->param, buf, count);
	}
	data = (void *)(buf + 512 + header->EDO + 4);
	data_len = header->EDS - 4;
	ret = mt6660_codec_trigger_param_write(p_drvdata->chip, data, data_len);
	if (ret < 0) {
		dev_err(dev, "trigger parameter write fail\n");
		return ret;
	}
	return 0;
}

static ssize_t mt6660_param_file_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct mt6660_param_drvdata *p_drvdata = dev_get_drvdata(dev);

	if (!p_drvdata->param)
		return scnprintf(buf, PAGE_SIZE, "no proprietary param\n");
	memcpy(buf, p_drvdata->param, p_drvdata->param_size);
	return p_drvdata->param_size;
}

static ssize_t mt6660_param_file_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int ret = 0;

	ret = mt6660_param_header_validate(dev, buf, count);
	if (ret < 0) {
		dev_err(dev, "parameter check header fail\n");
		return ret;
	}
	ret = mt6660_param_data_validate(dev, buf, count);
	if (ret < 0) {
		dev_err(dev, "parameter check data fail\n");
		return ret;
	}
	ret = mt6660_param_data_trigger(dev, buf, count);
	if (ret < 0) {
		dev_err(dev, "parameter trigger data fail\n");
		return ret;
	}
	return count;
}

static const DEVICE_ATTR(prop_params, 0644, mt6660_param_file_show,
			 mt6660_param_file_store);

static int mt6660_param_probe(struct platform_device *pdev)
{
	struct mt6660_chip *chip = dev_get_drvdata(pdev->dev.parent);
	struct mt6660_param_drvdata *p_drvdata = NULL;
	int ret = 0;

	dev_info(&pdev->dev, "%s: ++\n", __func__);
	p_drvdata = devm_kzalloc(&pdev->dev, sizeof(*p_drvdata), GFP_KERNEL);
	if (!p_drvdata)
		return -ENOMEM;
	/* drvdata initialize */
	p_drvdata->dev = &pdev->dev;
	p_drvdata->chip = chip;
	platform_set_drvdata(pdev, p_drvdata);
	ret = device_create_file(&pdev->dev, &dev_attr_prop_params);
	if (ret < 0)
		goto cfile_fail;
	dev_info(&pdev->dev, "%s: --\n", __func__);
	return 0;
cfile_fail:
	devm_kfree(&pdev->dev, p_drvdata);
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static int mt6660_param_remove(struct platform_device *pdev)
{
	struct mt6660_param_drvdata *p_drvdata = platform_get_drvdata(pdev);

	dev_dbg(p_drvdata->dev, "%s: ++\n", __func__);
	dev_dbg(p_drvdata->dev, "%s: --\n", __func__);
	return 0;
}

static const struct platform_device_id mt6660_param_pdev_id[] = {
	{ "mt6660-param", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, mt6660_param_pdev_id);

static struct platform_driver mt6660_param_driver = {
	.driver = {
		.name = "mt6660-param",
		.owner = THIS_MODULE,
	},
	.probe = mt6660_param_probe,
	.remove = mt6660_param_remove,
	.id_table = mt6660_param_pdev_id,
};
module_platform_driver(mt6660_param_driver);
