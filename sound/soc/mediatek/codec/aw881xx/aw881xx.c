/*
 * aw881xx.c   aw881xx codec module
 *
 * Copyright (c) 2019 AWINIC Technology CO., LTD
 *
 *  Author: Nick Li <liweilei@awinic.com.cn>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * * Audio driver for various aw881xx I2C audio drivers.
 *
 *  Supported devices list:
 *              AW88162
 *              AW88163
 *              AW88164
 *              AW88194
 *              AW88194A
 *              AW88195
 */
#include <linux/module.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/debugfs.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/syscalls.h>
#include <sound/tlv.h>
#include <linux/uaccess.h>
#include "aw881xx.h"
#include "aw881xx_reg.h"
#include "aw881xx_monitor.h"
#include "aw881xx_cali.h"

/******************************************************
 *
 * Marco
 *
 ******************************************************/
#define AW881XX_I2C_NAME "aw881xx_smartpa"

#define AW881XX_DRIVER_VERSION "v0.1.15"

#define AW881XX_RATES SNDRV_PCM_RATE_8000_48000
#define AW881XX_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_S24_LE | \
			SNDRV_PCM_FMTBIT_S32_LE)

#define AW_I2C_RETRIES 5
#define AW_I2C_RETRY_DELAY 5	/* 5ms */
#define AW_READ_CHIPID_RETRIES 5
#define AW_READ_CHIPID_RETRY_DELAY 5	/* 5ms */

static uint16_t aw881xx_base_addr[] = {
	AW881XX_SPK_REG_ADDR,
	AW881XX_SPK_DSP_FW_ADDR,
	AW881XX_SPK_DSP_CFG_ADDR,
	AW881XX_VOICE_REG_ADDR,
	AW881XX_VOICE_DSP_FW_ADDR,
	AW881XX_VOICE_DSP_CFG_ADDR,
	AW881XX_FM_REG_ADDR,
	AW881XX_FM_DSP_FW_ADDR,
	AW881XX_FM_DSP_CFG_ADDR,
	AW881XX_RCV_REG_ADDR,
	AW881XX_RCV_DSP_FW_ADDR,
	AW881XX_RCV_DSP_CFG_ADDR,
};

/******************************************************
 *
 * Value
 *
 ******************************************************/
static char aw881xx_cfg_name[][AW881XX_CFG_NAME_MAX] = {
	/* spk */
	{"aw881xx_pid_xx_spk_reg.bin"},
	{"aw881xx_pid_01_spk_fw.bin"},
	{"aw881xx_pid_01_spk_cfg.bin"},
	/* voice */
	{"aw881xx_pid_xx_voice_reg.bin"},
	{"aw881xx_pid_01_voice_fw.bin"},
	{"aw881xx_pid_01_voice_cfg.bin"},
	/* fm */
	{"aw881xx_pid_xx_fm_reg.bin"},
	{"aw881xx_pid_01_fm_fw.bin"},
	{"aw881xx_pid_01_fm_cfg.bin"},
	/* rcv */
	{"aw881xx_pid_xx_rcv_reg.bin"},
	{"aw881xx_pid_01_rcv_fw.bin"},
	{"aw881xx_pid_01_rcv_cfg.bin"},
};

/******************************************************
 *
 * aw881xx distinguish between codecs and components by version
 *
 ******************************************************/
static const struct snd_soc_dapm_widget aw881xx_dapm_widgets[] = {
	/* playback */
	SND_SOC_DAPM_AIF_IN("AIF_RX", "Speaker_Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_OUTPUT("audio_out"),
};

static const struct snd_soc_dapm_route aw881xx_audio_map[] = {
	{"audio_out", NULL, "AIF_RX"},
};

#ifdef AW_KERNEL_VER_OVER_4_19_1
static const struct aw_componet_codec_ops aw_componet_codec_ops = {
	.aw_snd_soc_kcontrol_codec = snd_soc_kcontrol_component,
	.aw_snd_soc_codec_get_drvdata = snd_soc_component_get_drvdata,
	.aw_snd_soc_add_codec_controls = snd_soc_add_component_controls,
	.aw_snd_soc_unregister_codec = snd_soc_unregister_component,
	.aw_snd_soc_register_codec = snd_soc_register_component,
};
#else
static const struct aw_componet_codec_ops aw_componet_codec_ops = {
	.aw_snd_soc_kcontrol_codec = snd_soc_kcontrol_codec,
	.aw_snd_soc_codec_get_drvdata = snd_soc_codec_get_drvdata,
	.aw_snd_soc_add_codec_controls = snd_soc_add_codec_controls,
	.aw_snd_soc_unregister_codec = snd_soc_unregister_codec,
	.aw_snd_soc_register_codec = snd_soc_register_codec,
};
#endif

static aw_snd_soc_codec_t *aw_get_codec(struct snd_soc_dai *dai)
{
#ifdef AW_KERNEL_VER_OVER_4_19_1
	return dai->component;
#else
	return dai->codec;
#endif
}

/******************************************************
 *
 * aw881xx append suffix sound channel information
 *
 ******************************************************/
static void *aw881xx_devm_kstrdup(struct device *dev, char *buf)
{
	char *str = NULL;

	str = devm_kzalloc(dev, strlen(buf) + 1, GFP_KERNEL);
	if (!str) {
		pr_err("%s:devm_kzalloc %s failed\n", __func__, buf);
		return str;
	}
	memcpy(str, buf, strlen(buf));
	return str;
}

static int aw881xx_append_suffix(char *format, const char **change_name,
			struct aw881xx *aw881xx)
{
	char buf[64] = { 0 };

	if (!aw881xx->name_suffix)
		return 0;

	snprintf(buf, 64, format, *change_name, aw881xx->name_suffix);
	*change_name = aw881xx_devm_kstrdup(aw881xx->dev, buf);
	if (!(*change_name))
		return -ENOMEM;

	aw_dev_dbg(aw881xx->dev, "%s:change name :%s\n",
		__func__, *change_name);
	return 0;
}

/******************************************************
 *
 * aw881xx reg write/read
 *
 ******************************************************/
static int aw881xx_i2c_writes(struct aw881xx *aw881xx,
				uint8_t reg_addr, uint8_t *buf, uint16_t len)
{
	int ret = -1;
	uint8_t *data = NULL;

	data = kmalloc(len + 1, GFP_KERNEL);
	if (data == NULL) {
		aw_dev_err(aw881xx->dev, "%s: can not allocate memory\n",
				__func__);
		return -ENOMEM;
	}

	data[0] = reg_addr;
	memcpy(&data[1], buf, len);

	ret = i2c_master_send(aw881xx->i2c, data, len + 1);
	if (ret < 0)
		aw_dev_err(aw881xx->dev,
				"%s: i2c master send error\n", __func__);

	kfree(data);
	data = NULL;

	return ret;
}

static int aw881xx_i2c_reads(struct aw881xx *aw881xx,
				uint8_t reg_addr, uint8_t *data_buf,
				uint16_t data_len)
{
	int ret;
	struct i2c_msg msg[] = {
		[0] = {
				.addr = aw881xx->i2c->addr,
				.flags = 0,
				.len = sizeof(uint8_t),
				.buf = &reg_addr,
				},
		[1] = {
				.addr = aw881xx->i2c->addr,
				.flags = I2C_M_RD,
				.len = data_len,
				.buf = data_buf,
				},
	};

	ret = i2c_transfer(aw881xx->i2c->adapter, msg, ARRAY_SIZE(msg));
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: transfer failed.", __func__);
		return ret;
	} else if (ret != AW881XX_READ_MSG_NUM) {
		aw_dev_err(aw881xx->dev, "%s: transfer failed(size error).\n",
				__func__);
		return -ENXIO;
	}

	return 0;
}

static int aw881xx_i2c_write(struct aw881xx *aw881xx,
				uint8_t reg_addr, uint16_t reg_data)
{
	int ret = -1;
	uint8_t cnt = 0;
	uint8_t buf[2] = { 0 };

	buf[0] = (reg_data & 0xff00) >> 8;
	buf[1] = (reg_data & 0x00ff) >> 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = aw881xx_i2c_writes(aw881xx, reg_addr, buf, 2);
		if (ret < 0)
			aw_dev_err(aw881xx->dev,
					"%s: i2c_write cnt=%d error=%d\n",
					__func__, cnt, ret);
		else
			break;
		cnt++;
	}

	return ret;
}

static int aw881xx_i2c_read(struct aw881xx *aw881xx,
				uint8_t reg_addr, uint16_t *reg_data)
{
	int ret = -1;
	uint8_t cnt = 0;
	uint8_t buf[2] = { 0 };

	while (cnt < AW_I2C_RETRIES) {
		ret = aw881xx_i2c_reads(aw881xx, reg_addr, buf, 2);
		if (ret < 0) {
			aw_dev_err(aw881xx->dev,
					"%s: i2c_read cnt=%d error=%d\n",
					__func__, cnt, ret);
		} else {
			*reg_data = (buf[0] << 8) | (buf[1] << 0);
			break;
		}
		cnt++;
	}

	return ret;
}

static int aw881xx_i2c_write_bits(struct aw881xx *aw881xx,
					uint8_t reg_addr, uint16_t mask,
					uint16_t reg_data)
{
	int ret = -1;
	uint16_t reg_val = 0;

	ret = aw881xx_i2c_read(aw881xx, reg_addr, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: i2c read error, ret=%d\n",
				__func__, ret);
		return ret;
	}
	reg_val &= mask;
	reg_val |= reg_data;
	ret = aw881xx_i2c_write(aw881xx, reg_addr, reg_val);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: i2c read error, ret=%d\n",
				__func__, ret);
		return ret;
	}

	return 0;
}

int aw881xx_reg_writes(struct aw881xx *aw881xx,
				uint8_t reg_addr, uint8_t *buf, uint16_t len)
{
	int ret = -1;

	mutex_lock(&aw881xx->i2c_lock);
	ret = aw881xx_i2c_writes(aw881xx, reg_addr, buf, len);
	if (ret < 0)
		aw_dev_err(aw881xx->dev, "%s: aw881x_i2c_writes fail, ret=%d",
				__func__, ret);
	mutex_unlock(&aw881xx->i2c_lock);

	return ret;
}

int aw881xx_reg_write(struct aw881xx *aw881xx,
			uint8_t reg_addr, uint16_t reg_data)
{
	int ret = -1;

	mutex_lock(&aw881xx->i2c_lock);
	ret = aw881xx_i2c_write(aw881xx, reg_addr, reg_data);
	if (ret < 0)
		aw_dev_err(aw881xx->dev, "%s: aw881xx_i2c_write fail, ret=%d",
				__func__, ret);
	mutex_unlock(&aw881xx->i2c_lock);

	return ret;
}

int aw881xx_reg_read(struct aw881xx *aw881xx,
			uint8_t reg_addr, uint16_t *reg_data)
{
	int ret = -1;

	mutex_lock(&aw881xx->i2c_lock);
	ret = aw881xx_i2c_read(aw881xx, reg_addr, reg_data);
	if (ret < 0)
		aw_dev_err(aw881xx->dev, "%s: aw881xx_i2c_read fail, ret=%d",
				__func__, ret);
	mutex_unlock(&aw881xx->i2c_lock);

	return ret;
}

int aw881xx_reg_write_bits(struct aw881xx *aw881xx,
			uint8_t reg_addr, uint16_t mask, uint16_t reg_data)
{
	int ret = -1;

	mutex_lock(&aw881xx->i2c_lock);
	ret = aw881xx_i2c_write_bits(aw881xx, reg_addr, mask, reg_data);
	if (ret < 0)
		aw_dev_err(aw881xx->dev,
				"%s: aw881xx_i2c_write_bits fail, ret=%d",
				__func__, ret);
	mutex_unlock(&aw881xx->i2c_lock);

	return ret;
}

int aw881xx_dsp_write(struct aw881xx *aw881xx,
			uint16_t dsp_addr, uint16_t dsp_data)
{
	int ret = -1;

	mutex_lock(&aw881xx->i2c_lock);
	ret = aw881xx_i2c_write(aw881xx, AW881XX_REG_DSPMADD, dsp_addr);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: i2c write error, ret=%d\n",
				__func__, ret);
		goto dsp_write_err;
	}

	ret = aw881xx_i2c_write(aw881xx, AW881XX_REG_DSPMDAT, dsp_data);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: i2c write error, ret=%d\n",
				__func__, ret);
		goto dsp_write_err;
	}
	mutex_unlock(&aw881xx->i2c_lock);

	return ret;

 dsp_write_err:
	mutex_unlock(&aw881xx->i2c_lock);
	return ret;

}

int aw881xx_dsp_read(struct aw881xx *aw881xx,
			uint16_t dsp_addr, uint16_t *dsp_data)
{
	int ret = -1;

	mutex_lock(&aw881xx->i2c_lock);
	ret = aw881xx_i2c_write(aw881xx, AW881XX_REG_DSPMADD, dsp_addr);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: i2c write error, ret=%d\n",
				__func__, ret);
		goto dsp_read_err;
	}

	ret = aw881xx_i2c_read(aw881xx, AW881XX_REG_DSPMDAT, dsp_data);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: i2c read error, ret=%d\n",
				__func__, ret);
		goto dsp_read_err;
	}
	mutex_unlock(&aw881xx->i2c_lock);

	return ret;

 dsp_read_err:
	mutex_unlock(&aw881xx->i2c_lock);
	return ret;
}

/******************************************************
 *
 * aw881xx control
 *
 ******************************************************/
static void aw881xx_run_mute(struct aw881xx *aw881xx, bool mute)
{
	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	if (mute) {
		aw881xx_reg_write_bits(aw881xx, AW881XX_REG_PWMCTRL,
					AW881XX_BIT_PWMCTRL_HMUTE_MASK,
					AW881XX_BIT_PWMCTRL_HMUTE_ENABLE);
	} else {
		aw881xx_reg_write_bits(aw881xx, AW881XX_REG_PWMCTRL,
					AW881XX_BIT_PWMCTRL_HMUTE_MASK,
					AW881XX_BIT_PWMCTRL_HMUTE_DISABLE);
	}
}

static void aw881xx_run_pwd(struct aw881xx *aw881xx, bool pwd)
{
	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	if (pwd) {
		aw881xx_reg_write_bits(aw881xx, AW881XX_REG_SYSCTRL,
					AW881XX_BIT_SYSCTRL_PW_MASK,
					AW881XX_BIT_SYSCTRL_PW_PDN);
	} else {
		aw881xx_reg_write_bits(aw881xx, AW881XX_REG_SYSCTRL,
					AW881XX_BIT_SYSCTRL_PW_MASK,
					AW881XX_BIT_SYSCTRL_PW_ACTIVE);
	}
}

static void aw881xx_dsp_enable(struct aw881xx *aw881xx, bool dsp)
{
	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	if (dsp) {
		aw881xx_reg_write_bits(aw881xx, AW881XX_REG_SYSCTRL,
					AW881XX_BIT_SYSCTRL_DSP_MASK,
					AW881XX_BIT_SYSCTRL_DSP_WORK);
	} else {
		aw881xx_reg_write_bits(aw881xx, AW881XX_REG_SYSCTRL,
					AW881XX_BIT_SYSCTRL_DSP_MASK,
					AW881XX_BIT_SYSCTRL_DSP_BYPASS);
	}
}

static void aw881xx_memclk_select(struct aw881xx *aw881xx, unsigned char flag)
{
	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	if (flag == AW881XX_MEMCLK_PLL) {
		aw881xx_reg_write_bits(aw881xx, AW881XX_REG_SYSCTRL2,
					AW881XX_BIT_SYSCTRL2_MEMCLK_MASK,
					AW881XX_BIT_SYSCTRL2_MEMCLK_PLL);
	} else if (flag == AW881XX_MEMCLK_OSC) {
		aw881xx_reg_write_bits(aw881xx, AW881XX_REG_SYSCTRL2,
					AW881XX_BIT_SYSCTRL2_MEMCLK_MASK,
					AW881XX_BIT_SYSCTRL2_MEMCLK_OSC);
	} else {
		aw_dev_err(aw881xx->dev,
				"%s: unknown memclk config, flag=0x%x\n",
				__func__, flag);
	}
}

static int aw881xx_sysst_check(struct aw881xx *aw881xx)
{
	int ret = -1;
	unsigned char i;
	uint16_t reg_val = 0;

	for (i = 0; i < AW881XX_SYSST_CHECK_MAX; i++) {
		aw881xx_reg_read(aw881xx, AW881XX_REG_SYSST, &reg_val);
		if ((reg_val & (~AW881XX_BIT_SYSST_CHECK_MASK)) ==
			AW881XX_BIT_SYSST_CHECK) {
			ret = 0;
			return ret;
		} else {
			aw_dev_dbg(aw881xx->dev,
					"%s: check fail, cnt=%d, reg_val=0x%04x\n",
					__func__, i, reg_val);
			msleep(2);
		}
	}
	aw_dev_info(aw881xx->dev, "%s: check fail\n", __func__);

	return ret;
}

int aw881xx_get_sysint(struct aw881xx *aw881xx, uint16_t *sysint)
{
	int ret = -1;
	uint16_t reg_val = 0;

	ret = aw881xx_reg_read(aw881xx, AW881XX_REG_SYSINT, &reg_val);
	if (ret < 0)
		aw_dev_err(aw881xx->dev, "%s: read sysint fail, ret=%d\n",
				__func__, ret);
	else
		*sysint = reg_val;

	return ret;
}

int aw881xx_get_iis_status(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t reg_val = 0;

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_reg_read(aw881xx, AW881XX_REG_SYSST, &reg_val);
	if (reg_val & AW881XX_BIT_SYSST_PLLS)
		ret = 0;

	return ret;
}

int aw881xx_get_dsp_status(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t reg_val = 0;

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_reg_read(aw881xx, AW881XX_REG_WDT, &reg_val);
	if (reg_val)
		ret = 0;

	return ret;
}

int aw881xx_get_dsp_config(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t reg_val = 0;

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_reg_read(aw881xx, AW881XX_REG_SYSCTRL, &reg_val);
	if (reg_val & AW881XX_BIT_SYSCTRL_DSP_BYPASS)
		aw881xx->dsp_cfg = AW881XX_DSP_BYPASS;
	else
		aw881xx->dsp_cfg = AW881XX_DSP_WORK;

	return ret;
}

int aw881xx_get_hmute(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t reg_val = 0;

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_reg_read(aw881xx, AW881XX_REG_PWMCTRL, &reg_val);
	if (reg_val & AW881XX_BIT_PWMCTRL_HMUTE_ENABLE)
		ret = 1;
	else
		ret = 0;

	return ret;
}

static int aw881xx_get_icalk(struct aw881xx *aw881xx, int16_t *icalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t reg_icalk = 0;

	ret = aw881xx_reg_read(aw881xx, AW881XX_REG_EFRM, &reg_val);
	reg_icalk = (uint16_t)reg_val & AW881XX_EF_ISENSE_GAINERR_SLP_MASK;

	if (reg_icalk & AW881XX_EF_ISENSE_GAINERR_SLP_SIGN_MASK)
		reg_icalk = reg_icalk | AW881XX_EF_ISENSE_GAINERR_SLP_NEG;

	*icalk = (int16_t)reg_icalk;

	return ret;
}

static int aw881xx_get_vcalk(struct aw881xx *aw881xx, int16_t *vcalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t reg_vcalk = 0;

	ret = aw881xx_reg_read(aw881xx, AW881XX_REG_PRODUCT_ID, &reg_val);
	reg_val = reg_val >> AW881XX_EF_VSENSE_GAIN_SHIFT;

	reg_vcalk = (uint16_t)reg_val & AW881XX_EF_VSENSE_GAIN_MASK;

	if (reg_vcalk & AW881XX_EF_VSENSE_GAIN_SIGN_MASK)
		reg_vcalk = reg_vcalk | AW881XX_EF_VSENSE_GAIN_NEG;

	*vcalk = (int16_t)reg_vcalk;

	return ret;
}

static int aw881xx_dsp_set_vcalb(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t reg_val = 0;
	int vcalb;
	int icalk;
	int vcalk;
	int16_t icalk_val = 0;
	int16_t vcalk_val = 0;

	ret = aw881xx_get_icalk(aw881xx, &icalk_val);
	if (ret < 0)
		return ret;
	ret = aw881xx_get_vcalk(aw881xx, &vcalk_val);
	if (ret < 0)
		return ret;

	icalk = AW881XX_CABL_BASE_VALUE + AW881XX_ICABLK_FACTOR * icalk_val;
	vcalk = AW881XX_CABL_BASE_VALUE + AW881XX_VCABLK_FACTOR * vcalk_val;

	if (icalk == 0 || vcalk == 0) {
		aw_dev_err(aw881xx->dev, "%s: icalk=%d, vcalk=%d is error\n",
			__func__, icalk, vcalk);
		return -EINVAL;
	}

	vcalb = (AW881XX_VCAL_FACTOR * AW881XX_VSCAL_FACTOR /
		 AW881XX_ISCAL_FACTOR) * vcalk / icalk;

	reg_val = (uint16_t) vcalb;
	aw_dev_dbg(aw881xx->dev, "%s: icalk=%d, vcalk=%d, vcalb=%d, reg_val=%d\n",
			__func__, icalk, vcalk, vcalb, reg_val);

	ret = aw881xx_dsp_write(aw881xx, AW881XX_DSP_REG_VCALB, reg_val);

	return ret;
}

static int aw881xx_set_intmask(struct aw881xx *aw881xx, bool flag)
{
	int ret = -1;

	if (flag)
		ret = aw881xx_reg_write(aw881xx, AW881XX_REG_SYSINTM,
					aw881xx->intmask);
	else
		ret = aw881xx_reg_write(aw881xx, AW881XX_REG_SYSINTM,
					AW881XX_REG_SYSINTM_MASK);
	return ret;
}

static int aw881xx_start(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t sysint = 0;

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_run_pwd(aw881xx, false);
	ret = aw881xx_sysst_check(aw881xx);
	if (ret < 0) {
		aw881xx_run_mute(aw881xx, true);
	} else {
		aw881xx_run_mute(aw881xx, false);

		ret = aw881xx_get_sysint(aw881xx, &sysint);
		if (ret < 0)
			return ret;
		else
			aw_dev_info(aw881xx->dev, "%s: get_sysint=0x%04x\n",
				__func__, sysint);

		ret = aw881xx_get_sysint(aw881xx, &sysint);
		if (ret < 0)
			return ret;
		else
			aw_dev_info(aw881xx->dev, "%s: get_sysint=0x%04x\n",
				__func__, sysint);

		ret = aw881xx_set_intmask(aw881xx, true);
		if (ret < 0)
			return ret;

		if ((aw881xx->monitor.monitor_flag) &&
			((aw881xx->scene_mode == AW881XX_SPK_MODE) ||
				(aw881xx->scene_mode == AW881XX_VOICE_MODE) ||
				(aw881xx->scene_mode == AW881XX_FM_MODE))) {
			aw881xx_monitor_start(&aw881xx->monitor);
		}
	}

	return ret;
}

static void aw881xx_stop(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t sysint = 0;

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	ret = aw881xx_get_sysint(aw881xx, &sysint);
	if (ret < 0)
		aw_dev_err(aw881xx->dev, "%s: get_sysint fail, ret=%d\n",
			__func__, ret);
	else
		aw_dev_info(aw881xx->dev, "%s: get_sysint=0x%04x\n",
			__func__, sysint);

	aw881xx_set_intmask(aw881xx, false);
	aw881xx_run_mute(aw881xx, true);
	aw881xx_run_pwd(aw881xx, true);

	if (aw881xx->monitor.monitor_flag)
		aw881xx_monitor_stop(&aw881xx->monitor);
}

/*****************************************************
 *
 * check chip id
 *
 *****************************************************/
static void aw881xx_update_cfg_name(struct aw881xx *aw881xx)
{
	char aw881xx_head[] = { "aw881xx_pid_" };
	char buf[3] = { 0 };
	uint8_t i = 0, j = 0, j_shift = 0;
	uint8_t head_index = 0;
	uint16_t cfg_num = 0;

	memcpy(aw881xx->cfg_name, aw881xx_cfg_name, sizeof(aw881xx_cfg_name));
	head_index = sizeof(aw881xx_head) - 1;

	/*add product information */
	snprintf(buf, sizeof(buf), "%02x", aw881xx->pid);
	for (i = 0; i < sizeof(aw881xx->cfg_name) / AW881XX_CFG_NAME_MAX; i++)
		if (i % AW881XX_MODE_CFG_NUM_MAX)
			memcpy(aw881xx->cfg_name[i] + head_index, buf,
					sizeof(buf) - 1);

	/*add sound channel information */
	if (aw881xx->name_suffix) {
		head_index += AW881XX_ADD_CHAN_NAME_SHIFT;
		snprintf(buf, sizeof(buf), "_%s", aw881xx->name_suffix);
		for (i = 0; i < sizeof(aw881xx->cfg_name) / AW881XX_CFG_NAME_MAX; i++) {
			for (j = strlen(aw881xx->cfg_name[i]); j >= head_index; j--) {
				j_shift = j + AW881XX_ADD_CHAN_NAME_SHIFT;
				aw881xx->cfg_name[i][j_shift] = aw881xx->cfg_name[i][j];
			}
			memcpy(aw881xx->cfg_name[i] + head_index, buf,
				sizeof(buf) - 1);
		}
	}

	cfg_num = sizeof(aw881xx->cfg_name) / AW881XX_CFG_NAME_MAX;
	for (i = 0; i < cfg_num; i++)
		aw_dev_dbg(aw881xx->dev, "%s: id[%d], [%s]\n",
			__func__, i, aw881xx->cfg_name[i]);
}

static int aw881xx_read_dsp_pid(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t reg_val = 0;

	ret = aw881xx_dsp_read(aw881xx, AW881XX_REG_DSP_PID, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev,
				"%s: failed to read AW881XX_REG_DSP_PID: %d\n",
				__func__, ret);
		return -EIO;
	}

	switch (reg_val) {
	case AW881XX_DSP_PID_01:
		aw881xx->pid = AW881XX_PID_01;
		break;
	case AW881XX_DSP_PID_03:
		aw881xx->pid = AW881XX_PID_03;
		break;
	default:
		aw_dev_info(aw881xx->dev, "%s: unsupported dsp_pid=0x%04x\n",
				__func__, reg_val);
		return -EIO;
	}

	aw881xx_update_cfg_name(aw881xx);

	return 0;
}

static int aw881xx_read_product_id(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t product_id = 0;

	ret = aw881xx_reg_read(aw881xx, AW881XX_REG_PRODUCT_ID, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: failed to read REG_EFRL: %d\n",
			__func__, ret);
		return -EIO;
	}

	product_id = reg_val & (~AW881XX_BIT_PRODUCT_ID_MASK);
	switch (product_id) {
	case AW881XX_PID_01:
		aw881xx->pid = AW881XX_PID_01;
		break;
	case AW881XX_PID_03:
		aw881xx->pid = AW881XX_PID_03;
		break;
	default:
		aw881xx->pid = AW881XX_PID_03;
	}

	aw881xx_update_cfg_name(aw881xx);

	return 0;
}

static int aw881xx_read_chipid(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint8_t cnt = 0;
	uint16_t reg_val = 0;

	while (cnt < AW_READ_CHIPID_RETRIES) {
		ret = aw881xx_reg_read(aw881xx, AW881XX_REG_ID, &reg_val);
		if (ret < 0) {
			aw_dev_err(aw881xx->dev,
				"%s: failed to read chip id, ret=%d\n",
				__func__, ret);
			return -EIO;
		}
		switch (reg_val) {
		case AW881XX_CHIPID:
			aw881xx->chipid = AW881XX_CHIPID;
			aw881xx->flags |= AW881XX_FLAG_START_ON_MUTE;
			aw881xx->flags |= AW881XX_FLAG_SKIP_INTERRUPTS;

			aw881xx_read_product_id(aw881xx);

			aw_dev_info(aw881xx->dev,
				"%s: chipid=0x%04x, product_id=0x%02x\n",
				__func__, aw881xx->chipid, aw881xx->pid);
			return 0;
		default:
			aw_dev_info(aw881xx->dev,
				"%s: unsupported device revision (0x%x)\n",
				__func__, reg_val);
			break;
		}
		cnt++;

		msleep(AW_READ_CHIPID_RETRY_DELAY);
	}

	return -EINVAL;
}

/******************************************************
 *
 * aw881xx dsp
 *
 ******************************************************/
static int aw881xx_dsp_check(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t iis_check_max = 5;
	uint16_t i = 0;

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_run_pwd(aw881xx, false);
	aw881xx_memclk_select(aw881xx, AW881XX_MEMCLK_PLL);
	for (i = 0; i < iis_check_max; i++) {
		ret = aw881xx_get_iis_status(aw881xx);
		if (ret < 0) {
			aw_dev_err(aw881xx->dev,
					"%s: iis signal check error, reg=0x%x\n",
					__func__, reg_val);
			msleep(2);
		} else {
			if (aw881xx->dsp_cfg == AW881XX_DSP_WORK) {
				aw881xx_dsp_enable(aw881xx, false);
				aw881xx_dsp_enable(aw881xx, true);
				msleep(1);
				ret = aw881xx_get_dsp_status(aw881xx);
				if (ret < 0) {
					aw_dev_err(aw881xx->dev,
							"%s: dsp wdt status error=%d\n",
							__func__, ret);
				} else {
					return 0;
				}
			} else if (aw881xx->dsp_cfg == AW881XX_DSP_BYPASS) {
				return 0;
			} else {
				aw_dev_err(aw881xx->dev, "%s: unknown dsp cfg=%d\n",
						__func__, aw881xx->dsp_cfg);
				return -EINVAL;
			}
		}
	}
	return -EINVAL;
}

static void aw881xx_dsp_update_cali_re(struct aw881xx *aw881xx)
{
	if (aw881xx->cali_attr.cali_re != AW_ERRO_CALI_VALUE) {
		aw881xx_set_cali_re_to_dsp(&aw881xx->cali_attr);
	} else {
		aw_dev_info(aw881xx->dev, "%s: no set re=%d\n",
			__func__, aw881xx->cali_attr.cali_re);
	}
}

static void aw881xx_dsp_container_update(struct aw881xx *aw881xx,
					struct aw881xx_container *aw881xx_cont,
					uint16_t base)
{
	int i = 0;
#ifdef AW881XX_DSP_I2C_WRITES
	uint8_t tmp_val = 0;
	uint32_t tmp_len = 0;
#else
	uint16_t reg_val = 0;
#endif

	aw_dev_dbg(aw881xx->dev, "%s enter\n", __func__);

	mutex_lock(&aw881xx->i2c_lock);
#ifdef AW881XX_DSP_I2C_WRITES
	/* i2c writes */
	aw881xx_i2c_write(aw881xx, AW881XX_REG_DSPMADD, base);
	for (i = 0; i < aw881xx_cont->len; i += 2) {
		tmp_val = aw881xx_cont->data[i + 0];
		aw881xx_cont->data[i + 0] = aw881xx_cont->data[i + 1];
		aw881xx_cont->data[i + 1] = tmp_val;
	}
	for (i = 0; i < aw881xx_cont->len; i += AW881XX_MAX_RAM_WRITE_BYTE_SIZE) {
		if ((aw881xx_cont->len - i) < AW881XX_MAX_RAM_WRITE_BYTE_SIZE)
			tmp_len = aw881xx_cont->len - i;
		else
			tmp_len = AW881XX_MAX_RAM_WRITE_BYTE_SIZE;
		aw881xx_i2c_writes(aw881xx, AW881XX_REG_DSPMDAT,
				&aw881xx_cont->data[i], tmp_len);
	}
#else
	/* i2c write */
	aw881xx_i2c_write(aw881xx, AW881XX_REG_DSPMADD, base);
	for (i = 0; i < aw881xx_cont->len; i += 2) {
		reg_val = (aw881xx_cont->data[i + 1] << 8) + aw881xx_cont->data[i + 0];
		aw881xx_i2c_write(aw881xx, AW881XX_REG_DSPMDAT, reg_val);
	}
#endif
	mutex_unlock(&aw881xx->i2c_lock);
	aw_dev_dbg(aw881xx->dev, "%s: exit\n", __func__);
}

static void aw881xx_dsp_cfg_loaded(const struct firmware *cont, void *context)
{
	struct aw881xx *aw881xx = context;
	struct aw881xx_container *aw881xx_cfg = NULL;
	int ret = -1;

	if (!cont) {
		aw_dev_err(aw881xx->dev, "%s: failed to read %s\n",
				__func__, aw881xx->cfg_name[aw881xx->cfg_num]);
		release_firmware(cont);
		return;
	}

	aw_dev_info(aw881xx->dev,
			"%s: loaded %s - size: %zu\n", __func__,
			aw881xx->cfg_name[aw881xx->cfg_num], cont ? cont->size : 0);

	aw881xx_cfg = kzalloc(cont->size + sizeof(int), GFP_KERNEL);
	if (!aw881xx_cfg) {
		release_firmware(cont);
		aw_dev_err(aw881xx->dev, "%s: error allocating memory\n",
			   __func__);
		return;
	}
	aw881xx->dsp_cfg_len = cont->size;
	aw881xx_cfg->len = cont->size;
	memcpy(aw881xx_cfg->data, cont->data, cont->size);
	release_firmware(cont);

	if (aw881xx->work_flag == false) {
		kfree(aw881xx_cfg);
		aw881xx_cfg = NULL;
		aw_dev_info(aw881xx->dev, "%s: mode_flag = %d\n",
				__func__, aw881xx->work_flag);
		return;
	}

	mutex_lock(&aw881xx->lock);
	aw881xx_dsp_container_update(aw881xx, aw881xx_cfg,
					aw881xx_base_addr[aw881xx->cfg_num]);

	kfree(aw881xx_cfg);
	aw881xx_cfg = NULL;

	aw881xx_dsp_update_cali_re(aw881xx);
	aw881xx_dsp_set_vcalb(aw881xx);

	ret = aw881xx_dsp_check(aw881xx);
	if (ret < 0) {
		aw881xx->init = AW881XX_INIT_NG;
		aw881xx_run_mute(aw881xx, true);
		aw_dev_info(aw881xx->dev, "%s: fw/cfg update error\n",
				__func__);
	} else {
		aw_dev_info(aw881xx->dev, "%s: fw/cfg update complete\n",
				__func__);

		ret = aw881xx_start(aw881xx);
		if (ret < 0) {
			aw_dev_err(aw881xx->dev, "%s: start fail, ret=%d\n",
					__func__, ret);
		} else {
			aw_dev_info(aw881xx->dev, "%s: start success\n",
					__func__);
			aw881xx->init = AW881XX_INIT_OK;
		}
	}
	mutex_unlock(&aw881xx->lock);
}

static int aw881xx_load_dsp_cfg(struct aw881xx *aw881xx)
{
	aw_dev_info(aw881xx->dev, "%s enter\n", __func__);

	aw881xx->cfg_num++;

	return request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
					aw881xx->cfg_name[aw881xx->cfg_num],
					aw881xx->dev, GFP_KERNEL, aw881xx,
					aw881xx_dsp_cfg_loaded);
}

static void aw881xx_dsp_fw_loaded(const struct firmware *cont, void *context)
{
	struct aw881xx *aw881xx = context;
	struct aw881xx_container *aw881xx_cfg = NULL;
	int ret = -1;

	if (!cont) {
		aw_dev_err(aw881xx->dev, "%s: failed to read %s\n",
				__func__, aw881xx->cfg_name[aw881xx->cfg_num]);
		release_firmware(cont);
		return;
	}

	aw_dev_info(aw881xx->dev, "%s: loaded %s - size: %zu\n",
			__func__, aw881xx->cfg_name[aw881xx->cfg_num],
			cont ? cont->size : 0);

	aw881xx_cfg = kzalloc(cont->size + sizeof(int), GFP_KERNEL);
	if (!aw881xx_cfg) {
		release_firmware(cont);
		aw_dev_err(aw881xx->dev, "%s: error allocating memory\n",
				__func__);
		return;
	}
	aw881xx->dsp_fw_len = cont->size;
	aw881xx_cfg->len = cont->size;
	memcpy(aw881xx_cfg->data, cont->data, cont->size);
	release_firmware(cont);

	if (aw881xx->work_flag == true) {
		mutex_lock(&aw881xx->lock);
		aw881xx_dsp_container_update(aw881xx, aw881xx_cfg,
				aw881xx_base_addr[aw881xx->cfg_num]);
		mutex_unlock(&aw881xx->lock);
	}
	kfree(aw881xx_cfg);
	aw881xx_cfg = NULL;

	if (aw881xx->work_flag == false) {
		aw_dev_info(aw881xx->dev, "%s: mode_flag = %d\n",
				__func__, aw881xx->work_flag);
		return;
	}

	ret = aw881xx_load_dsp_cfg(aw881xx);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev,
				"%s: cfg loading requested failed: %d\n",
				__func__, ret);
	}
}

static int aw881xx_load_dsp_fw(struct aw881xx *aw881xx)
{
	int ret = -1;

	aw_dev_info(aw881xx->dev, "%s enter\n", __func__);

	aw881xx->cfg_num++;

	if (aw881xx->work_flag == true) {
		mutex_lock(&aw881xx->lock);
		aw881xx_run_mute(aw881xx, true);
		aw881xx_dsp_enable(aw881xx, false);
		aw881xx_memclk_select(aw881xx, AW881XX_MEMCLK_PLL);
		mutex_unlock(&aw881xx->lock);
	} else {
		aw_dev_info(aw881xx->dev, "%s: mode_flag = %d\n",
				__func__, aw881xx->work_flag);
		return ret;
	}

	return request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
					aw881xx->cfg_name[aw881xx->cfg_num],
					aw881xx->dev, GFP_KERNEL, aw881xx,
					aw881xx_dsp_fw_loaded);
}

static void aw881xx_get_cfg_shift(struct aw881xx *aw881xx)
{
	aw881xx->cfg_num = aw881xx->scene_mode * AW881XX_MODE_CFG_NUM_MAX;
	aw_dev_dbg(aw881xx->dev, "%s: cfg_num=%d\n",
			__func__, aw881xx->cfg_num);
}

static void aw881xx_update_dsp(struct aw881xx *aw881xx)
{
	uint16_t iis_check_max = 5;
	int i = 0;
	int ret = -1;

	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_get_cfg_shift(aw881xx);
	aw881xx_get_dsp_config(aw881xx);

	for (i = 0; i < iis_check_max; i++) {
		mutex_lock(&aw881xx->lock);
		ret = aw881xx_get_iis_status(aw881xx);
		mutex_unlock(&aw881xx->lock);
		if (ret < 0) {
			aw_dev_err(aw881xx->dev,
					"%s: get no iis signal, ret=%d\n",
					__func__, ret);
			msleep(2);
		} else {
			ret = aw881xx_load_dsp_fw(aw881xx);
			if (ret < 0) {
				aw_dev_err(aw881xx->dev,
						"%s: cfg loading requested failed: %d\n",
						__func__, ret);
			}
			break;
		}
	}
}

/******************************************************
 *
 * aw881xx reg config
 *
 ******************************************************/
static void aw881xx_reg_container_update(struct aw881xx *aw881xx,
					 struct aw881xx_container *aw881xx_cont)
{
	int i = 0;
	uint16_t reg_addr = 0;
	uint16_t reg_val = 0;

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	for (i = 0; i < aw881xx_cont->len; i += 4) {
		reg_addr = (aw881xx_cont->data[i + 1] << 8) +
			aw881xx_cont->data[i + 0];
		reg_val = (aw881xx_cont->data[i + 3] << 8) +
			aw881xx_cont->data[i + 2];
		aw_dev_dbg(aw881xx->dev, "%s: reg=0x%04x, val = 0x%04x\n",
				__func__, reg_addr, reg_val);
		if (reg_addr == AW881XX_REG_SYSINTM) {
			aw881xx->intmask = reg_val;
			reg_val = AW881XX_REG_SYSINTM_MASK;
		}
		aw881xx_reg_write(aw881xx, (uint8_t)reg_addr, (uint16_t)reg_val);
	}

	aw_dev_dbg(aw881xx->dev, "%s: exit\n", __func__);
}

static void aw881xx_reg_loaded(const struct firmware *cont, void *context)
{
	struct aw881xx *aw881xx = context;
	struct aw881xx_container *aw881xx_cfg = NULL;
	int ret = -1;
	uint16_t iis_check_max = 5;
	int i = 0;

	if (!cont) {
		aw_dev_err(aw881xx->dev, "%s: failed to read %s\n",
				__func__, aw881xx->cfg_name[aw881xx->cfg_num]);
		release_firmware(cont);
		return;
	}

	aw_dev_info(aw881xx->dev, "%s: loaded %s - size: %zu\n",
			__func__, aw881xx->cfg_name[aw881xx->cfg_num],
			cont ? cont->size : 0);

	aw881xx_cfg = kzalloc(cont->size + sizeof(int), GFP_KERNEL);
	if (!aw881xx_cfg) {
		release_firmware(cont);
		aw_dev_err(aw881xx->dev, "%s: error allocating memory\n",
				__func__);
		return;
	}
	aw881xx_cfg->len = cont->size;
	memcpy(aw881xx_cfg->data, cont->data, cont->size);
	release_firmware(cont);

	if (aw881xx->work_flag == true) {
		mutex_lock(&aw881xx->lock);
		aw881xx_reg_container_update(aw881xx, aw881xx_cfg);
		mutex_unlock(&aw881xx->lock);
	}
	kfree(aw881xx_cfg);
	aw881xx_cfg = NULL;

	if (aw881xx->work_flag == false) {
		aw_dev_info(aw881xx->dev, "%s: mode_flag = %d\n",
				__func__, aw881xx->work_flag);
		return;
	}

	for (i = 0; i < iis_check_max; i++) {
		mutex_lock(&aw881xx->lock);
		ret = aw881xx_get_iis_status(aw881xx);
		mutex_unlock(&aw881xx->lock);
		if (ret < 0) {
			aw_dev_err(aw881xx->dev,
					"%s: get no iis signal, ret=%d\n",
					__func__, ret);
			msleep(2);
		} else {
			aw881xx_read_dsp_pid(aw881xx);
			aw881xx_update_dsp(aw881xx);
			break;
		}
	}
}

static int aw881xx_load_reg(struct aw881xx *aw881xx)
{
	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	return request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
					aw881xx->cfg_name[aw881xx->cfg_num],
					aw881xx->dev, GFP_KERNEL, aw881xx,
					aw881xx_reg_loaded);
}

static void aw881xx_cold_start(struct aw881xx *aw881xx)
{
	int ret = -1;

	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_get_cali_re(&aw881xx->cali_attr);
	aw881xx_get_cfg_shift(aw881xx);

	ret = aw881xx_load_reg(aw881xx);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev,
				"%s: cfg loading requested failed: %d\n",
				__func__, ret);
	}
}

static void aw881xx_smartpa_cfg(struct aw881xx *aw881xx, bool flag)
{
	int ret = -1;

	aw_dev_info(aw881xx->dev, "%s: flag = %d\n", __func__, flag);

	aw881xx->work_flag = flag;
	if (flag == true && aw881xx->scene_mode != AW881XX_OFF_MODE) {
		if ((aw881xx->init == AW881XX_INIT_ST)
			|| (aw881xx->init == AW881XX_INIT_NG)) {
			aw_dev_info(aw881xx->dev, "%s: init = %d\n",
				__func__, aw881xx->init);
			aw881xx_cold_start(aw881xx);
		} else {
			ret = aw881xx_start(aw881xx);
			if (ret < 0)
				aw_dev_err(aw881xx->dev,
						"%s: start fail, ret=%d\n",
						__func__, ret);
			else
				aw_dev_info(aw881xx->dev, "%s: start success\n",
					__func__);
		}
	} else {
		aw881xx_stop(aw881xx);
	}
}

/******************************************************
 *
 * kcontrol
 *
 ******************************************************/
static const DECLARE_TLV_DB_SCALE(digital_gain, 0, 50, 0);

struct soc_mixer_control aw881xx_mixer = {
	.reg = AW881XX_REG_HAGCCFG7,
	.shift = AW881XX_VOL_REG_SHIFT,
	.min = AW881XX_VOLUME_MIN,
	.max = AW881XX_VOLUME_MAX,
};

static int aw881xx_volume_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;

	/* set kcontrol info */
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = mc->max - mc->min;

	return 0;
}

static int aw881xx_volume_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	uint16_t reg_val = 0;
	uint16_t vol_val = 0;
	unsigned int value = 0;

	aw881xx_reg_read(aw881xx, AW881XX_REG_HAGCCFG7, &reg_val);
	vol_val = (reg_val >> AW881XX_VOL_REG_SHIFT);

	value = (vol_val >> 4) * AW881XX_VOL_6DB_STEP +
		((vol_val & 0x0f) % AW881XX_VOL_6DB_STEP);

	ucontrol->value.integer.value[0] = value;

	return 0;
}

static int aw881xx_volume_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
	    (struct soc_mixer_control *)kcontrol->private_value;
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	unsigned int value = 0;
	uint16_t reg_val = 0;
	uint16_t vol_val = 0;

	/* value is right */
	value = ucontrol->value.integer.value[0];
	if (value > (mc->max - mc->min) || value < 0) {
		aw_dev_err(aw881xx->dev, "%s:value over range\n", __func__);
		return -EINVAL;
	}

	/* cal real value */
	vol_val = ((value / AW881XX_VOL_6DB_STEP) << 4) +
		(value % AW881XX_VOL_6DB_STEP);

	reg_val = (vol_val << AW881XX_VOL_REG_SHIFT);

	/* write value */
	aw881xx_reg_write_bits(aw881xx, AW881XX_REG_HAGCCFG7,
				(uint16_t) AW881XX_BIT_HAGCCFG7_VOL_MASK,
				reg_val);

	return 0;
}

static struct snd_kcontrol_new aw881xx_volume = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "aw881xx_rx_volume",
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |
		SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.tlv.p = (digital_gain),
	.info = aw881xx_volume_info,
	.get = aw881xx_volume_get,
	.put = aw881xx_volume_put,
	.private_value = (unsigned long)&aw881xx_mixer,
};

static const char *const mode_function[] = {
	"spk",
	"voice",
	"fm",
	"rcv",
	"off",
};

static int aw881xx_mode_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_dbg(aw881xx->dev, "%s: scene_mode=%d\n",
			__func__, aw881xx->scene_mode);

	ucontrol->value.integer.value[0] = aw881xx->scene_mode;

	return 0;
}

static int aw881xx_mode_set(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_dbg(aw881xx->dev, "%s: ucontrol->value.integer.value[0]=%ld\n",
			__func__, ucontrol->value.integer.value[0]);
	if (ucontrol->value.integer.value[0] == aw881xx->scene_mode)
		return 1;

	aw881xx->scene_mode = ucontrol->value.integer.value[0];

	aw881xx->init = AW881XX_INIT_ST;

	return 0;
}

static const struct soc_enum aw881xx_snd_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mode_function), mode_function),
};

static struct snd_kcontrol_new aw881xx_controls[] = {
	SOC_ENUM_EXT("aw881xx_mode_switch", aw881xx_snd_enum[0],
			aw881xx_mode_get, aw881xx_mode_set),
};

static void aw881xx_kcontrol_append_suffix(struct aw881xx *aw881xx,
				struct snd_kcontrol_new *src_control, int num)
{
	int i = 0, ret;
	struct snd_kcontrol_new *dst_control = NULL;

	dst_control = devm_kzalloc(aw881xx->dev,
					num * sizeof(struct snd_kcontrol_new),
					GFP_KERNEL);
	if (!dst_control) {
		aw_dev_err(aw881xx->dev, "%s:kcontrol kzalloc faild\n",
			__func__);
		return;
	}
	memcpy(dst_control, src_control, num * sizeof(struct snd_kcontrol_new));

	for (i = 0; i < num; i++) {
		ret = aw881xx_append_suffix("%s_%s",
				(const char **)&dst_control[i].name,
				aw881xx);
		if (ret < 0)
			return;
	}
	aw_componet_codec_ops.aw_snd_soc_add_codec_controls(aw881xx->codec,
							dst_control, num);
}

static void aw881xx_add_codec_controls(struct aw881xx *aw881xx)
{
	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_kcontrol_append_suffix(aw881xx,
					aw881xx_controls,
					ARRAY_SIZE(aw881xx_controls));
	aw881xx_kcontrol_append_suffix(aw881xx, &aw881xx_volume, 1);
}

/******************************************************
 *
 * Digital Audio Interface
 *
 ******************************************************/
static int aw881xx_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		mutex_lock(&aw881xx->lock);
		aw881xx_run_pwd(aw881xx, false);
		mutex_unlock(&aw881xx->lock);
	}

	return 0;
}

static int aw881xx_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	/*struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec); */

	aw_dev_info(codec->dev, "%s: fmt=0x%x\n", __func__, fmt);

	/* supported mode: regular I2S, slave, or PDM */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) !=
			SND_SOC_DAIFMT_CBS_CFS) {
			aw_dev_err(codec->dev,
					"%s: invalid codec master mode\n",
					__func__);
			return -EINVAL;
		}
		break;
	default:
		aw_dev_err(codec->dev, "%s: unsupported DAI format %d\n",
				__func__, fmt & SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}
	return 0;
}

static int aw881xx_set_dai_sysclk(struct snd_soc_dai *dai,
				  int clk_id, unsigned int freq, int dir)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_info(aw881xx->dev, "%s: freq=%d\n", __func__, freq);

	aw881xx->sysclk = freq;
	return 0;
}

static int aw881xx_update_hw_params(struct aw881xx *aw881xx)
{
	uint16_t reg_val = 0;
	uint32_t cco_mux_value;

	/* match rate */
	switch (aw881xx->rate) {
	case 8000:
		reg_val = AW881XX_BIT_I2SCTRL_SR_8K;
		cco_mux_value = AW881XX_I2S_CCO_MUX_8_16_32KHZ_VALUE;
		break;
	case 16000:
		reg_val = AW881XX_BIT_I2SCTRL_SR_16K;
		cco_mux_value = AW881XX_I2S_CCO_MUX_8_16_32KHZ_VALUE;
		break;
	case 32000:
		reg_val = AW881XX_BIT_I2SCTRL_SR_32K;
		cco_mux_value = AW881XX_I2S_CCO_MUX_8_16_32KHZ_VALUE;
		break;
	case 44100:
		reg_val = AW881XX_BIT_I2SCTRL_SR_44P1K;
		cco_mux_value = AW881XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		break;
	case 48000:
		reg_val = AW881XX_BIT_I2SCTRL_SR_48K;
		cco_mux_value = AW881XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		break;
	case 96000:
		reg_val = AW881XX_BIT_I2SCTRL_SR_96K;
		cco_mux_value = AW881XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		break;
	case 192000:
		reg_val = AW881XX_BIT_I2SCTRL_SR_192K;
		cco_mux_value = AW881XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		break;
	default:
		reg_val = AW881XX_BIT_I2SCTRL_SR_48K;
		cco_mux_value = AW881XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		aw_dev_err(aw881xx->dev, "%s: rate can not support\n",
				__func__);
		break;
	}
	aw881xx_reg_write_bits(aw881xx, AW881XX_REG_PLLCTRL1,
				AW881XX_I2S_CCO_MUX_MASK, cco_mux_value);
	/* set chip rate */
	aw881xx_reg_write_bits(aw881xx, AW881XX_REG_I2SCTRL,
					AW881XX_BIT_I2SCTRL_SR_MASK, reg_val);

	/* match bit width */
	switch (aw881xx->width) {
	case 16:
		reg_val = AW881XX_BIT_I2SCTRL_FMS_16BIT;
		break;
	case 20:
		reg_val = AW881XX_BIT_I2SCTRL_FMS_20BIT;
		break;
	case 24:
		reg_val = AW881XX_BIT_I2SCTRL_FMS_24BIT;
		break;
	case 32:
		reg_val = AW881XX_BIT_I2SCTRL_FMS_32BIT;
		break;
	default:
		reg_val = AW881XX_BIT_I2SCTRL_FMS_16BIT;
		aw_dev_err(aw881xx->dev, "%s: width can not support\n",
				__func__);
		break;
	}
	/* set width */
	aw881xx_reg_write_bits(aw881xx, AW881XX_REG_I2SCTRL,
					AW881XX_BIT_I2SCTRL_FMS_MASK, reg_val);

	return 0;
}

static int aw881xx_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		aw_dev_dbg(aw881xx->dev,
				"%s: requested rate: %d, sample size: %d\n",
				__func__, params_rate(params),
				snd_pcm_format_width(params_format(params)));
		return 0;
	}
	/* get rate param */
	aw881xx->rate = params_rate(params);
	aw_dev_dbg(aw881xx->dev, "%s: requested rate: %d, sample size: %d\n",
			__func__, aw881xx->rate,
			snd_pcm_format_width(params_format(params)));

	/* get bit width */
	aw881xx->width = params_width(params);
	aw_dev_dbg(aw881xx->dev, "%s: width = %d\n", __func__, aw881xx->width);

	mutex_lock(&aw881xx->lock);
	aw881xx_update_hw_params(aw881xx);
	mutex_unlock(&aw881xx->lock);

	return 0;
}

static int aw881xx_mute(struct snd_soc_dai *dai, int mute, int stream)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_info(aw881xx->dev, "%s: mute state=%d\n", __func__, mute);

	if (!(aw881xx->flags & AW881XX_FLAG_START_ON_MUTE))
		return 0;

	if (mute) {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			mutex_lock(&aw881xx->lock);
			aw881xx_smartpa_cfg(aw881xx, false);
			mutex_unlock(&aw881xx->lock);
		}
	} else {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			mutex_lock(&aw881xx->lock);
			aw881xx_smartpa_cfg(aw881xx, true);
			mutex_unlock(&aw881xx->lock);
		}
	}

	return 0;
}

static void aw881xx_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		aw881xx->rate = 0;
		mutex_lock(&aw881xx->lock);
		aw881xx_run_pwd(aw881xx, true);
		mutex_unlock(&aw881xx->lock);
	}
}

static const struct snd_soc_dai_ops aw881xx_dai_ops = {
	.startup = aw881xx_startup,
	.set_fmt = aw881xx_set_fmt,
	.set_sysclk = aw881xx_set_dai_sysclk,
	.hw_params = aw881xx_hw_params,
	.mute_stream = aw881xx_mute,
	.shutdown = aw881xx_shutdown,
};

static struct snd_soc_dai_driver aw881xx_dai[] = {
	{
	.name = "aw881xx-aif",
	.id = 1,
	.playback = {
			.stream_name = "Speaker_Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = AW881XX_RATES,
			.formats = AW881XX_FORMATS,
			},
	 .capture = {
			.stream_name = "Speaker_Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = AW881XX_RATES,
			.formats = AW881XX_FORMATS,
			},
	.ops = &aw881xx_dai_ops,
	.symmetric_rates = 1,
	},
};

/*****************************************************
 *
 * codec driver
 *
 *****************************************************/
static int aw881xx_probe(aw_snd_soc_codec_t *codec)
{
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx->codec = codec;

	aw881xx_add_codec_controls(aw881xx);

	if (codec->dev->of_node) {
		if (aw881xx->name_suffix)
			dev_set_name(aw881xx->dev, "%s_%s", "aw881xx_smartpa",
					aw881xx->name_suffix);
		else
			dev_set_name(aw881xx->dev, "%s", "aw881xx_smartpa");
	}

	aw_dev_info(aw881xx->dev, "%s: exit\n", __func__);

	return 0;
}

#ifdef AW_KERNEL_VER_OVER_4_19_1
static void aw881xx_remove(struct snd_soc_component *component)
{
	/*struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);*/

	aw_dev_info(component->dev, "%s: enter\n", __func__);

	return;
}
#else
static int aw881xx_remove(struct snd_soc_codec *codec)
{
	/*struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);*/

	aw_dev_info(codec->dev, "%s: enter\n", __func__);

	return 0;
}
#endif

static unsigned int aw881xx_codec_read(aw_snd_soc_codec_t *codec,
					unsigned int reg)
{
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	uint16_t value = 0;
	int ret = -1;

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	if (aw881xx_reg_access[reg] & REG_RD_ACCESS) {
		ret = aw881xx_reg_read(aw881xx, reg, &value);
		if (ret < 0) {
			aw_dev_err(aw881xx->dev, "%s: read register failed\n",
					__func__);
			return ret;
		}
	} else {
		aw_dev_dbg(aw881xx->dev, "%s: register 0x%x no read access\n",
				__func__, reg);
		return ret;
	}
	return ret;
}

static int aw881xx_codec_write(aw_snd_soc_codec_t *codec,
				unsigned int reg, unsigned int value)
{
	int ret = -1;
	struct aw881xx *aw881xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_dbg(aw881xx->dev, "%s: enter ,reg is 0x%x value is 0x%x\n",
			__func__, reg, value);

	if (aw881xx_reg_access[reg] & REG_WR_ACCESS)
		ret = aw881xx_reg_write(aw881xx, (uint8_t) reg,
					(uint16_t) value);
	else
		aw_dev_dbg(aw881xx->dev, "%s: register 0x%x no write access\n",
				__func__, reg);

	return ret;
}

#ifdef AW_KERNEL_VER_OVER_4_19_1
static struct snd_soc_component_driver soc_codec_dev_aw881xx = {
	.probe = aw881xx_probe,
	.remove = aw881xx_remove,
	.read = aw881xx_codec_read,
	.write = aw881xx_codec_write,
};

#else
static struct snd_soc_codec_driver soc_codec_dev_aw881xx = {
	.probe = aw881xx_probe,
	.remove = aw881xx_remove,
	.read = aw881xx_codec_read,
	.write = aw881xx_codec_write,
	.reg_cache_size = AW881XX_REG_MAX,
	.reg_word_size = 2,
	.component_driver = {
	.dapm_widgets = aw881xx_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(aw881xx_dapm_widgets),
	.dapm_routes = aw881xx_audio_map,
	.num_dapm_routes = ARRAY_SIZE(aw881xx_audio_map),
	},
};
#endif

/******************************************************
 *
 * irq
 *
 ******************************************************/
static void aw881xx_interrupt_setup(struct aw881xx *aw881xx)
{
	uint16_t reg_val = 0;

	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_reg_read(aw881xx, AW881XX_REG_SYSINTM, &reg_val);
	reg_val &= (~AW881XX_BIT_SYSINTM_PLLM);
	reg_val &= (~AW881XX_BIT_SYSINTM_OTHM);
	reg_val &= (~AW881XX_BIT_SYSINTM_OCDM);
	aw881xx_reg_write(aw881xx, AW881XX_REG_SYSINTM, reg_val);
}

static void aw881xx_interrupt_handle(struct aw881xx *aw881xx)
{
	uint16_t reg_val = 0;

	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_reg_read(aw881xx, AW881XX_REG_SYSST, &reg_val);
	aw_dev_info(aw881xx->dev, "%s: reg SYSST=0x%x\n", __func__, reg_val);

	aw881xx_reg_read(aw881xx, AW881XX_REG_SYSINT, &reg_val);
	aw_dev_info(aw881xx->dev, "%s: reg SYSINT=0x%x\n", __func__, reg_val);

	aw881xx_reg_read(aw881xx, AW881XX_REG_SYSINTM, &reg_val);
	aw_dev_info(aw881xx->dev, "%s: reg SYSINTM=0x%x\n", __func__, reg_val);
}

static irqreturn_t aw881xx_irq(int irq, void *data)
{
	struct aw881xx *aw881xx = data;

	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	aw881xx_interrupt_handle(aw881xx);

	aw_dev_info(aw881xx->dev, "%s: exit\n", __func__);

	return IRQ_HANDLED;
}

/*****************************************************
 *
 * device tree
 *
 *****************************************************/
static int aw881xx_parse_gpio_dt(struct aw881xx *aw881xx,
				 struct device_node *np)
{
	aw881xx->reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);
	if (aw881xx->reset_gpio < 0) {
		aw_dev_err(aw881xx->dev,
			"%s: no reset gpio provided, will not hw reset\n",
			__func__);
		return -EIO;
	} else {
		aw_dev_info(aw881xx->dev, "%s: reset gpio provided ok\n",
			__func__);
	}

	aw881xx->irq_gpio = of_get_named_gpio(np, "irq-gpio", 0);
	if (aw881xx->irq_gpio < 0)
		aw_dev_info(aw881xx->dev, "%s: no irq gpio provided.\n", __func__);
	else
		aw_dev_info(aw881xx->dev, "%s: irq gpio provided ok.\n", __func__);

	return 0;
}

static void aw881xx_parse_channel_dt(struct aw881xx *aw881xx,
					struct device_node *np)
{
	int ret = -1;
	const char *channel_value = NULL;

	ret = of_property_read_string(np, "sound-channel", &channel_value);
	if (ret < 0) {
		aw_dev_info(aw881xx->dev,
			"%s:read sound-channel failed,use default\n",
			__func__);
		goto chan_default;
	}
	aw_dev_info(aw881xx->dev,
		"%s: read sound-channel value is : %s\n",
		__func__, channel_value);

	if (!strcmp(channel_value, "left")) {
		aw881xx->name_suffix = "l";
	} else if (!strcmp(channel_value, "right")) {
		aw881xx->name_suffix = "r";
	} else {
		aw_dev_info(aw881xx->dev,
			"%s:not stereo channel,use default single track\n",
			__func__);
		goto chan_default;
	}
	return;

 chan_default:
	aw881xx->name_suffix = NULL;
}

static void aw881xx_parse_monitor_dt(struct aw881xx *aw881xx,
					struct device_node *np)
{
	int ret;
	struct aw881xx_monitor *monitor = &aw881xx->monitor;

	ret = of_property_read_u32(np, "monitor-flag", &monitor->monitor_flag);
	if (ret) {
		aw_dev_err(aw881xx->dev,
			"%s: find no monitor-flag, use default config\n",
			__func__);
		monitor->monitor_flag = AW881XX_MONITOR_DFT_FLAG;
	} else {
		aw_dev_info(aw881xx->dev, "%s: monitor-flag = %d\n",
			__func__, monitor->monitor_flag);
	}
	ret = of_property_read_u32(np, "monitor-timer-val",
				&monitor->monitor_timer_val);
	if (ret) {
		aw_dev_err(aw881xx->dev,
			"%s: find no monitor-timer-val, use default config\n",
			__func__);
		monitor->monitor_timer_val = AW881XX_MONITOR_TIMER_DFT_VAL;
	} else {
		aw_dev_info(aw881xx->dev, "%s: monitor-timer-val = %d\n",
			__func__, monitor->monitor_timer_val);
	}

}

static int aw881xx_parse_dt(struct device *dev, struct aw881xx *aw881xx,
				struct device_node *np)
{
	int ret;

	ret = aw881xx_parse_gpio_dt(aw881xx, np);
	aw881xx_parse_channel_dt(aw881xx, np);
	aw881xx_parse_monitor_dt(aw881xx, np);
	return ret;
}

static int aw881xx_hw_reset(struct aw881xx *aw881xx)
{
	dev_info(aw881xx->dev, "%s: enter\n", __func__);

	if (gpio_is_valid(aw881xx->reset_gpio)) {
		gpio_set_value_cansleep(aw881xx->reset_gpio, 0);
		msleep(1);
		gpio_set_value_cansleep(aw881xx->reset_gpio, 1);
		msleep(1);
	} else {
		aw_dev_err(aw881xx->dev, "%s: failed\n", __func__);
	}
	return 0;
}

/******************************************************
 *
 * sys group attribute: reg
 *
 ******************************************************/
static ssize_t aw881xx_reg_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	unsigned int databuf[2] = { 0 };

	if (2 == sscanf(buf, "%x %x", &databuf[0], &databuf[1]))
		aw881xx_reg_write(aw881xx, databuf[0], databuf[1]);

	return count;
}

static ssize_t aw881xx_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint8_t i = 0;
	uint16_t reg_val = 0;

	for (i = 0; i < AW881XX_REG_MAX; i++) {
		if (aw881xx_reg_access[i] & REG_RD_ACCESS) {
			aw881xx_reg_read(aw881xx, i, &reg_val);
			len += snprintf(buf + len, PAGE_SIZE - len,
					"reg:0x%02x=0x%04x\n", i, reg_val);
		}
	}
	return len;
}

static ssize_t aw881xx_rw_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	unsigned int databuf[2] = { 0 };

	if (2 == sscanf(buf, "%x %x", &databuf[0], &databuf[1])) {
		aw881xx->reg_addr = (uint8_t) databuf[0];
		aw881xx_reg_write(aw881xx, databuf[0], databuf[1]);
	} else if (1 == sscanf(buf, "%x", &databuf[0])) {
		aw881xx->reg_addr = (uint8_t) databuf[0];
	}

	return count;
}

static ssize_t aw881xx_rw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint16_t reg_val = 0;

	if (aw881xx_reg_access[aw881xx->reg_addr] & REG_RD_ACCESS) {
		aw881xx_reg_read(aw881xx, aw881xx->reg_addr, &reg_val);
		len += snprintf(buf + len, PAGE_SIZE - len,
				"reg:0x%02x=0x%04x\n", aw881xx->reg_addr,
				reg_val);
	}
	return len;
}

static ssize_t aw881xx_dsp_rw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint16_t reg_val = 0;

	mutex_lock(&aw881xx->i2c_lock);
	aw881xx_i2c_write(aw881xx, AW881XX_REG_DSPMADD, aw881xx->dsp_addr);
	aw881xx_i2c_read(aw881xx, AW881XX_REG_DSPMDAT, &reg_val);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"dsp:0x%04x=0x%04x\n", aw881xx->dsp_addr, reg_val);
	aw881xx_i2c_read(aw881xx, AW881XX_REG_DSPMDAT, &reg_val);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"dsp:0x%04x=0x%04x\n", aw881xx->dsp_addr + 1, reg_val);
	mutex_unlock(&aw881xx->i2c_lock);

	return len;
}

static ssize_t aw881xx_dsp_rw_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	unsigned int databuf[2] = { 0 };

	if (2 == sscanf(buf, "%x %x", &databuf[0], &databuf[1])) {
		aw881xx->dsp_addr = (unsigned int)databuf[0];
		aw881xx_dsp_write(aw881xx, databuf[0], databuf[1]);
		aw_dev_dbg(aw881xx->dev, "%s: get param: %x %x\n", __func__,
				databuf[0], databuf[1]);
	} else if (1 == sscanf(buf, "%x", &databuf[0])) {
		aw881xx->dsp_addr = (unsigned int)databuf[0];
		aw_dev_dbg(aw881xx->dev, "%s: get param: %x\n", __func__,
				databuf[0]);
	}

	return count;
}

static ssize_t aw881xx_update_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	unsigned int val = 0;
	int ret = 0;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	aw_dev_dbg(aw881xx->dev, "%s: value=%d\n", __func__, val);

	if (val) {
		aw881xx->init = AW881XX_INIT_ST;
		aw881xx_smartpa_cfg(aw881xx, false);
		aw881xx_smartpa_cfg(aw881xx, true);
	}
	return count;
}

static ssize_t aw881xx_update_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/*struct aw881xx *aw881xx = dev_get_drvdata(dev); */
	ssize_t len = 0;

	return len;
}

static ssize_t aw881xx_dsp_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	unsigned int val = 0;
	int ret = 0;

	ret = kstrtouint(buf, 0, &val);
	if (ret < 0)
		return ret;

	aw_dev_dbg(aw881xx->dev, "%s: value=%d\n", __func__, val);

	if (val) {
		aw881xx->init = AW881XX_INIT_ST;
		aw881xx_update_dsp(aw881xx);
	}
	return count;
}

static ssize_t aw881xx_dsp_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	unsigned int i = 0;
	uint16_t reg_val = 0;
	int ret = -1;

	aw881xx_get_dsp_config(aw881xx);
	if (aw881xx->dsp_cfg == AW881XX_DSP_BYPASS) {
		len += snprintf((char *)(buf + len), PAGE_SIZE - len,
				"%s: aw881xx dsp bypass\n", __func__);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len,
				"aw881xx dsp working\n");
		ret = aw881xx_get_iis_status(aw881xx);
		if (ret < 0) {
			len += snprintf((char *)(buf + len),
					PAGE_SIZE - len,
					"%s: aw881xx no iis signal\n",
					__func__);
		} else {
			aw_dev_dbg(aw881xx->dev, "%s: aw881xx_dsp_firmware:\n",
					__func__);
			mutex_lock(&aw881xx->i2c_lock);
			aw881xx_i2c_write(aw881xx, AW881XX_REG_DSPMADD,
					AW881XX_SPK_DSP_FW_ADDR);
			for (i = 0; i < aw881xx->dsp_fw_len; i += 2) {
				aw881xx_i2c_read(aw881xx,
						AW881XX_REG_DSPMDAT, &reg_val);
				aw_dev_dbg(aw881xx->dev,
					"%s: dsp_fw[0x%04x]:0x%02x,0x%02x\n",
					__func__, i, (reg_val >> 0) & 0xff,
					(reg_val >> 8) & 0xff);
			}
			aw_dev_dbg(aw881xx->dev, "\n");

			aw_dev_dbg(aw881xx->dev, "%s: aw881xx_dsp_cfg:\n",
					__func__);
			len += snprintf(buf + len, PAGE_SIZE - len,
					"aw881xx dsp config:\n");
			aw881xx_i2c_write(aw881xx, AW881XX_REG_DSPMADD,
					AW881XX_SPK_DSP_CFG_ADDR);
			for (i = 0; i < aw881xx->dsp_cfg_len; i += 2) {
				aw881xx_i2c_read(aw881xx,
						AW881XX_REG_DSPMDAT, &reg_val);
				len += snprintf(buf + len, PAGE_SIZE - len,
					"%02x,%02x,", (reg_val >> 0) & 0xff,
					(reg_val >> 8) & 0xff);
				aw_dev_dbg(aw881xx->dev,
					"%s: dsp_cfg[0x%04x]:0x%02x,0x%02x\n",
					__func__, i, (reg_val >> 0) & 0xff,
						(reg_val >> 8) & 0xff);
				if ((i / 2 + 1) % 8 == 0) {
					len += snprintf(buf + len,
							PAGE_SIZE - len, "\n");
				}
			}
			mutex_unlock(&aw881xx->i2c_lock);
			len += snprintf(buf + len, PAGE_SIZE - len, "\n");
		}
	}

	return len;
}

static ssize_t aw881xx_dsp_fw_ver_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	/*struct aw881xx *aw881xx = dev_get_drvdata(dev);*/

	return count;
}

static ssize_t aw881xx_dsp_fw_ver_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint16_t dsp_addr = 0x8c00;
	uint16_t temp_val = 0;
	uint32_t dsp_val = 0;

	for (dsp_addr = 0x8c00; dsp_addr < 0x9000; dsp_addr++) {
		dsp_val = 0;
		aw881xx_dsp_read(aw881xx, dsp_addr, &temp_val);
		dsp_val |= (temp_val << 0);
		aw881xx_dsp_read(aw881xx, dsp_addr + 1, &temp_val);
		dsp_val |= (temp_val << 16);
		if ((dsp_val & 0x7fffff00) == 0x7fffff00) {
			len += snprintf(buf + len, PAGE_SIZE - len,
					"dsp fw ver: v1.%d\n",
					0xff - (dsp_val & 0xff));
			break;
		}
	}

	return len;
}

static ssize_t aw881xx_spk_temp_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	int ret;
	int16_t reg_val;

	ret = aw881xx_reg_read(aw881xx, AW881XX_REG_ASR2, (uint16_t *)&reg_val);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: read addr:0x%x failed\n",
					__func__, AW881XX_REG_ASR2);
		return ret;
	}
	len += snprintf(buf + len, PAGE_SIZE - len,
			"Temp:%d\n", reg_val);

	return len;
}

static ssize_t aw881xx_diver_ver_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len,
			"driver version:%s\n", AW881XX_DRIVER_VERSION);

	return len;
}


static DEVICE_ATTR(reg, S_IWUSR | S_IRUGO,
			aw881xx_reg_show, aw881xx_reg_store);
static DEVICE_ATTR(rw, S_IWUSR | S_IRUGO,
			aw881xx_rw_show, aw881xx_rw_store);
static DEVICE_ATTR(dsp_rw, S_IWUSR | S_IRUGO,
			aw881xx_dsp_rw_show, aw881xx_dsp_rw_store);
static DEVICE_ATTR(update, S_IWUSR | S_IRUGO,
			aw881xx_update_show, aw881xx_update_store);
static DEVICE_ATTR(dsp, S_IWUSR | S_IRUGO,
			aw881xx_dsp_show, aw881xx_dsp_store);
static DEVICE_ATTR(dsp_fw_ver, S_IWUSR | S_IRUGO,
			aw881xx_dsp_fw_ver_show, aw881xx_dsp_fw_ver_store);
static DEVICE_ATTR(spk_temp, S_IRUGO,
			aw881xx_spk_temp_show, NULL);
static DEVICE_ATTR(driver_ver, S_IRUGO,
			aw881xx_diver_ver_show, NULL);


static struct attribute *aw881xx_attributes[] = {
	&dev_attr_reg.attr,
	&dev_attr_rw.attr,
	&dev_attr_dsp_rw.attr,
	&dev_attr_update.attr,
	&dev_attr_dsp.attr,
	&dev_attr_dsp_fw_ver.attr,
	&dev_attr_spk_temp.attr,
	&dev_attr_driver_ver.attr,
	NULL
};

static struct attribute_group aw881xx_attribute_group = {
	.attrs = aw881xx_attributes
};

/******************************************************
 *
 * i2c driver
 *
 ******************************************************/
static int aw881xx_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct snd_soc_dai_driver *dai = NULL;
	struct aw881xx *aw881xx = NULL;
	struct device_node *np = i2c->dev.of_node;
	const char *aw881xx_rst = "aw881xx_rst";
	const char *aw881xx_int = "aw881xx_int";
	const char *aw881xx_irq_name = "aw881xx";
	int irq_flags = 0;
	int ret = -1;

	aw_dev_info(&i2c->dev, "%s: enter\n", __func__);

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		aw_dev_err(&i2c->dev, "check_functionality failed\n");
		return -EIO;
	}

	aw881xx = devm_kzalloc(&i2c->dev, sizeof(struct aw881xx), GFP_KERNEL);
	if (aw881xx == NULL)
		return -ENOMEM;

	aw881xx->dev = &i2c->dev;
	aw881xx->i2c = i2c;

	i2c_set_clientdata(i2c, aw881xx);
	mutex_init(&aw881xx->lock);
	mutex_init(&aw881xx->i2c_lock);

	/* aw881xx rst & int */
	if (np) {
		ret = aw881xx_parse_dt(&i2c->dev, aw881xx, np);
		if (ret) {
			aw_dev_err(&i2c->dev,
				"%s:failed to parse device tree node\n",
				__func__);
			return ret;
		}
	} else {
		aw881xx->reset_gpio = -1;
		aw881xx->irq_gpio = -1;
	}

	if (gpio_is_valid(aw881xx->reset_gpio)) {
		ret = aw881xx_append_suffix("%s_%s", &aw881xx_rst, aw881xx);
		if (ret < 0)
			return ret;

		ret = devm_gpio_request_one(&i2c->dev, aw881xx->reset_gpio,
					GPIOF_OUT_INIT_LOW, aw881xx_rst);
		if (ret) {
			aw_dev_err(&i2c->dev, "%s: rst request failed\n",
				__func__);
			return ret;
		}
	}

	if (gpio_is_valid(aw881xx->irq_gpio)) {
		ret = aw881xx_append_suffix("%s_%s", &aw881xx_int, aw881xx);
		if (ret < 0)
			return ret;

		ret = devm_gpio_request_one(&i2c->dev, aw881xx->irq_gpio,
						GPIOF_DIR_IN, aw881xx_int);
		if (ret) {
			aw_dev_err(&i2c->dev, "%s: int request failed\n",
				__func__);
			return ret;
		}
	}

	/* hardware reset */
	aw881xx_hw_reset(aw881xx);

	/* aw881xx chip id */
	ret = aw881xx_read_chipid(aw881xx);
	if (ret < 0) {
		aw_dev_err(&i2c->dev, "%s: aw881xx_read_chipid failed ret=%d\n",
			__func__, ret);
		return ret;
	}

	/* aw881xx device name */
	if (i2c->dev.of_node) {
		if (aw881xx->name_suffix)
			dev_set_name(&i2c->dev, "%s_%s", "aw881xx_smartpa",
					aw881xx->name_suffix);
		else
			dev_set_name(&i2c->dev, "%s", "aw881xx_smartpa");
	} else
		aw_dev_err(&i2c->dev, "%s failed to set device name: %d\n",
				__func__, ret);

	/* register codec */
	dai = devm_kzalloc(&i2c->dev, sizeof(aw881xx_dai), GFP_KERNEL);
	if (!dai)
		return -ENOMEM;

	memcpy(dai, aw881xx_dai, sizeof(aw881xx_dai));
	ret = aw881xx_append_suffix("%s-%s", &dai->name, aw881xx);
	if (ret < 0)
		return ret;
	ret = aw881xx_append_suffix("%s_%s", &dai->playback.stream_name, aw881xx);
	if (ret < 0)
		return ret;
	ret = aw881xx_append_suffix("%s_%s", &dai->capture.stream_name, aw881xx);
	if (ret < 0)
		return ret;

	ret = aw_componet_codec_ops.aw_snd_soc_register_codec(&i2c->dev,
					&soc_codec_dev_aw881xx,
					dai, ARRAY_SIZE(aw881xx_dai));
	if (ret < 0) {
		aw_dev_err(&i2c->dev, "%s failed to register aw881xx: %d\n",
				__func__, ret);
		return ret;
	}

	/* aw881xx irq */
	if (gpio_is_valid(aw881xx->irq_gpio) &&
		!(aw881xx->flags & AW881XX_FLAG_SKIP_INTERRUPTS)) {

		ret = aw881xx_append_suffix("%s_%s", &aw881xx_irq_name, aw881xx);
		if (ret < 0)
			goto err_irq_append_suffix;
		aw881xx_interrupt_setup(aw881xx);
		/* register irq handler */
		irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
		ret = devm_request_threaded_irq(&i2c->dev,
						gpio_to_irq(aw881xx->irq_gpio),
						NULL, aw881xx_irq, irq_flags,
						aw881xx_irq_name, aw881xx);
		if (ret != 0) {
			aw_dev_err(&i2c->dev,
				"failed to request IRQ %d: %d\n",
				gpio_to_irq(aw881xx->irq_gpio), ret);
			goto err_irq;
		}
	} else {
		aw_dev_info(&i2c->dev,
				"%s skipping IRQ registration\n", __func__);
		/* disable feature support if gpio was invalid */
		aw881xx->flags |= AW881XX_FLAG_SKIP_INTERRUPTS;
	}

	dev_set_drvdata(&i2c->dev, aw881xx);
	ret = sysfs_create_group(&i2c->dev.kobj, &aw881xx_attribute_group);
	if (ret < 0) {
		aw_dev_info(&i2c->dev,
				"%s error creating sysfs attr files\n",
				__func__);
		goto err_sysfs;
	}
	aw881xx_cali_init(&aw881xx->cali_attr);
	aw881xx_monitor_init(&aw881xx->monitor);

	aw_dev_info(&i2c->dev, "%s: probe completed successfully!\n", __func__);

	return 0;

err_sysfs:
err_irq:
err_irq_append_suffix:
	aw_componet_codec_ops.aw_snd_soc_unregister_codec(&i2c->dev);
	return ret;
}

static int aw881xx_i2c_remove(struct i2c_client *i2c)
{
	pr_info("%s: enter\n", __func__);

	aw_componet_codec_ops.aw_snd_soc_unregister_codec(&i2c->dev);


	return 0;
}

static const struct i2c_device_id aw881xx_i2c_id[] = {
	{AW881XX_I2C_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, aw881xx_i2c_id);

static struct of_device_id aw881xx_dt_match[] = {
	{.compatible = "awinic,aw881xx_smartpa"},
	{.compatible = "awinic,aw881xx_smartpa_l"},
	{.compatible = "awinic,aw881xx_smartpa_r"},
	{},
};

static struct i2c_driver aw881xx_i2c_driver = {
	.driver = {
		.name = AW881XX_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(aw881xx_dt_match),
		},
	.probe = aw881xx_i2c_probe,
	.remove = aw881xx_i2c_remove,
	.id_table = aw881xx_i2c_id,
};

static int __init aw881xx_i2c_init(void)
{
	int ret = -1;

	pr_info("%s: aw881xx driver version %s\n", __func__, AW881XX_DRIVER_VERSION);

	ret = i2c_add_driver(&aw881xx_i2c_driver);
	if (ret)
		pr_err("%s: fail to add aw881xx device into i2c\n", __func__);

	return ret;
}

late_initcall(aw881xx_i2c_init);

static void __exit aw881xx_i2c_exit(void)
{
	i2c_del_driver(&aw881xx_i2c_driver);
}

module_exit(aw881xx_i2c_exit);

MODULE_DESCRIPTION("ASoC AW881XX Smart PA Driver");
MODULE_LICENSE("GPL v2");
