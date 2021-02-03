/*
 * aw882xx.c   aw882xx codec module
 *
 * Copyright (c) 2019 AWINIC Technology CO., LTD
 *
 *  Author: Nick Li <liweilei@awinic.com.cn>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
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
#include <linux/debugfs.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/syscalls.h>
#include <sound/tlv.h>
#include <sound/control.h>
#include <linux/uaccess.h>
#include "aw882xx.h"
#include "aw882xx_reg.h"
#include "awinic_cali.h"
#include "awinic_monitor.h"
#include "awinic_dsp.h"


/******************************************************
 *
 * Marco
 *
 ******************************************************/


#define AW882XX_DRIVER_VERSION "v0.2.6"

#define AW882XX_RATES SNDRV_PCM_RATE_8000_48000
#define AW882XX_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_S24_LE | \
			SNDRV_PCM_FMTBIT_S32_LE)

#define AW_I2C_RETRIES			5	/* 5 times */
#define AW_I2C_RETRY_DELAY		5	/* 5 ms */
#define AW_READ_CHIPID_RETRIES		5	/* 5 times */
#define AW_READ_CHIPID_RETRY_DELAY	5	/* 5 ms */

/******************************************************
 *
 * copp enable
 *
 ******************************************************/
static DEFINE_MUTEX(g_aw_copp_lock);
unsigned int g_copp_en = 0;

/******************************************************
 * The prefix of the bin file name
 * The file suffix is added based on the channel
 * example : aw882xx_spk_reg_l.bin
 * notice : The mono file name suffix is ".bin"
 ******************************************************/
static char aw882xx_cfg_name[][AW882XX_CFG_NAME_MAX] = {
	{"aw882xx_spk_reg"},
	{"aw882xx_rcv_reg"},
};

static unsigned int aw882xx_mode_cfg_shift[AW882XX_MODE_SHIFT_MAX] = {
	AW882XX_MODE_SPK_SHIFT,
	AW882XX_MODE_RCV_SHIFT,
};

/******************************************************
 *
 * aw882xx distinguish between codecs and components by version
 *
 ******************************************************/

static const struct snd_soc_dapm_widget aw882xx_dapm_widgets[]={
 /* playback */
 SND_SOC_DAPM_AIF_IN("AIF_RX","Speaker_Playback",0,SND_SOC_NOPM,0,0),
 SND_SOC_DAPM_OUTPUT("audio_out"),
 /* capture for iv data */
 SND_SOC_DAPM_AIF_OUT("AIF_TX","Speaker_Capture",0,SND_SOC_NOPM,0,0),
 SND_SOC_DAPM_INPUT("iv_in"),
};

static const struct snd_soc_dapm_route aw882xx_audio_map[]={
 {"audio_out",NULL,"AIF_RX"},
 {"AIF_TX",NULL,"iv_in"},
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
 * aw882xx append suffix sound channel information
 *
 ******************************************************/
static void *aw882xx_devm_kstrdup(struct device *dev, char *buf)
{
	char *str = NULL;

	str = devm_kzalloc(dev, strlen(buf) + 1, GFP_KERNEL);
	if (str == NULL)
		return str;
	memcpy(str, buf, strlen(buf));
	return str;
}

int aw882xx_append_suffix(char *format, const char **change_name,
	struct aw882xx *aw882xx)
{
	char buf[64] = { 0 };

	if (!aw882xx->chan_info.name_suffix)
		return 0;

	snprintf(buf, sizeof(buf), format, *change_name, aw882xx->chan_info.name_suffix);
	*change_name = aw882xx_devm_kstrdup(aw882xx->dev, buf);
	if (!(*change_name)) {
		pr_err("%s: %s devm_kstrdup failed\n", __func__, buf);
		return -ENOMEM;
	}
	aw_dev_dbg(aw882xx->dev, "%s:change name :%s\n",
		__func__, *change_name);
	return 0;
}



/******************************************************
 *
 * aw882xx i2c write/read
 *
 ******************************************************/
static int aw882xx_i2c_writes(struct aw882xx *aw882xx,
	unsigned char reg_addr, unsigned char *buf, unsigned int len)
{
	int ret = -1;
	unsigned char *data = NULL;

	data = kmalloc(len+1, GFP_KERNEL);
	if (data == NULL) {
		aw_dev_err(aw882xx->dev, "%s: can not allocate memory\n",
			__func__);
		return -ENOMEM;
	}

	data[0] = reg_addr;
	memcpy(&data[1], buf, len);

	ret = i2c_master_send(aw882xx->i2c, data, len+1);
	if (ret < 0)
		aw_dev_err(aw882xx->dev,
			"%s: i2c master send error\n", __func__);

	kfree(data);
	data = NULL;

	return ret;
}

static int aw882xx_i2c_reads(struct aw882xx *aw882xx,
	unsigned char reg_addr, unsigned char *data_buf, unsigned int data_len)
{
	int ret;
	struct i2c_msg msg[] = {
		[0] = {
			.addr = aw882xx->i2c->addr,
			.flags = 0,
			.len = sizeof(uint8_t),
			.buf = &reg_addr,
			},
		[1] = {
			.addr = aw882xx->i2c->addr,
			.flags = I2C_M_RD,
			.len = data_len,
			.buf = data_buf,
			},
	};

	ret = i2c_transfer(aw882xx->i2c->adapter, msg, ARRAY_SIZE(msg));
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s: transfer failed.", __func__);
		return ret;
	} else if (ret != AW882XX_I2C_READ_MSG_NUM) {
		aw_dev_err(aw882xx->dev, "%s: transfer failed(size error).\n",
				__func__);
		return -ENXIO;
	}

	return 0;
}

int aw882xx_i2c_write(struct aw882xx *aw882xx,
	unsigned char reg_addr, unsigned int reg_data)
{
	int ret = -1;
	unsigned char cnt = 0;
	unsigned char buf[2];

	buf[0] = (reg_data&0xff00)>>8;
	buf[1] = (reg_data&0x00ff)>>0;

	while (cnt < AW_I2C_RETRIES) {
		ret = aw882xx_i2c_writes(aw882xx, reg_addr, buf, 2);
		if (ret < 0)
			aw_dev_err(aw882xx->dev, "%s: i2c_write cnt=%d error=%d\n",
				__func__, cnt, ret);
		else
			break;
		cnt++;
	}

	return ret;
}

int aw882xx_i2c_read(struct aw882xx *aw882xx,
	unsigned char reg_addr, unsigned int *reg_data)
{
	int ret = -1;
	unsigned char cnt = 0;
	unsigned char buf[2];

	while (cnt < AW_I2C_RETRIES) {
		ret = aw882xx_i2c_reads(aw882xx, reg_addr, buf, 2);
		if (ret < 0) {
			aw_dev_err(aw882xx->dev, "%s: i2c_read cnt=%d error=%d\n",
				__func__, cnt, ret);
		} else {
			*reg_data = (buf[0]<<8) | (buf[1]<<0);
			break;
		}
		cnt++;
	}

	return ret;
}

static int aw882xx_i2c_write_bits(struct aw882xx *aw882xx,
	unsigned char reg_addr, unsigned int mask, unsigned int reg_data)
{
	int ret = -1;
	unsigned int reg_val = 0;

	ret = aw882xx_i2c_read(aw882xx, reg_addr, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev,
			"%s: i2c read error, ret=%d\n", __func__, ret);
		return ret;
	}
	reg_val &= mask;
	reg_val |= reg_data;
	ret = aw882xx_i2c_write(aw882xx, reg_addr, reg_val);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev,
			"%s: i2c read error, ret=%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

/******************************************************
 *
 * aw882xx control
 *
 ******************************************************/
/*[7 : 4]: -6DB ; [3 : 0]: 0.5DB  real_value = value * 2 : 0.5db --> 1*/
uint32_t aw882xx_reg_val_to_db(uint32_t value)
{
	return ((value >> 4) * AW882XX_VOL_6DB_STEP + (value & 0x0f));
}

/*[7 : 4]: -6DB ; [3 : 0]: -0.5DB reg_value = value / step << 4 + value % step ; step = 6 * 2*/
static uint32_t aw882xx_db_val_to_reg(uint32_t value)
{
	return (((value / AW882XX_VOL_6DB_STEP) << 4)
		+ (value % AW882XX_VOL_6DB_STEP));
}

int aw882xx_set_volume(struct aw882xx *aw882xx, uint32_t value)
{
	uint32_t reg_value = 0;
	uint32_t real_value = aw882xx_db_val_to_reg(value);

	/* cal real value */
	aw882xx_i2c_read(aw882xx, AW882XX_HAGCCFG4_REG, &reg_value);

	aw_dev_dbg(aw882xx->dev, "%s: value %d , 0x%x\n",
			__func__, value, real_value);

	/*15 : 8] volume*/
	real_value = (real_value << 8) | (reg_value & 0x00ff);

	/* write value */
	aw882xx_i2c_write(aw882xx, AW882XX_HAGCCFG4_REG, real_value);

	return 0;
}

int aw882xx_get_volume(struct aw882xx *aw882xx, uint32_t *value)
{
	uint32_t reg_value = 0;
	uint32_t real_value = 0;

	/* read value */
	aw882xx_i2c_read(aw882xx, AW882XX_HAGCCFG4_REG, &reg_value);

	/*[15 : 8] volume*/
	real_value = reg_value >> 8;

	real_value = aw882xx_reg_val_to_db(real_value);
	*value = real_value;

	return 0;
}

static int aw882xx_fade_in_out(struct aw882xx *aw882xx, bool fade_in)
{
	int i = 0;
	uint32_t start_volume = 0;

	/*volume up*/
	if (fade_in) {
		for (i = AW_FADE_OUT_TARGET_VOL; i >= (int32_t)aw882xx->db_offset;
						i -= aw882xx->fade_step) {
			if (i < (int32_t)aw882xx->fade_step)
				i = aw882xx->db_offset;
			aw882xx_set_volume(aw882xx, i);
			usleep_range(1400, 1600);
		}
		if (i != (int32_t)aw882xx->db_offset)
			aw882xx_set_volume(aw882xx, aw882xx->db_offset);
	} else {
		/*volume down*/
		aw882xx_get_volume(aw882xx, &start_volume);
		for (i = start_volume; i <= AW_FADE_OUT_TARGET_VOL; i += aw882xx->fade_step) {
			aw882xx_set_volume(aw882xx, i);
			usleep_range(1400, 1600);
		}
		if (i != AW_FADE_OUT_TARGET_VOL)
			aw882xx_set_volume(aw882xx, AW_FADE_OUT_TARGET_VOL);
	}
	return 0;
}

static void aw882xx_run_mute(struct aw882xx *aw882xx, bool mute)
{
	aw_dev_dbg(aw882xx->dev, "%s: enter\n", __func__);

	if (mute) {
		aw882xx_fade_in_out(aw882xx, false);
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL2_REG,
				AW882XX_HMUTE_MASK,
				AW882XX_HMUTE_ENABLE_VALUE);
	} else {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL2_REG,
				AW882XX_HMUTE_MASK,
				AW882XX_HMUTE_DISABLE_VALUE);
		aw882xx_fade_in_out(aw882xx, true);
	}
}

static void aw882xx_run_i2s_tx(struct aw882xx *aw882xx, bool flag)
{
	aw_dev_dbg(aw882xx->dev, "%s: enter\n", __func__);

	if (flag) {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_I2SCFG1_REG,
				AW882XX_I2STXEN_MASK,
				AW882XX_I2STXEN_ENABLE_VALUE);
	} else {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_I2SCFG1_REG,
				AW882XX_I2STXEN_MASK,
				AW882XX_I2STXEN_DISABLE_VALUE);
	}
}

static void aw882xx_run_pwd(struct aw882xx *aw882xx, bool pwd)
{
	aw_dev_dbg(aw882xx->dev, "%s: enter\n", __func__);

	if (pwd) {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL_REG,
				AW882XX_PWDN_MASK,
				AW882XX_PWDN_POWER_DOWN_VALUE);
	} else {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL_REG,
				AW882XX_PWDN_MASK,
				AW882XX_PWDN_NORMAL_WORKING_VALUE);
	}
}

static int aw882xx_sysst_check(struct aw882xx *aw882xx)
{
	int ret = -1;
	unsigned char i;
	unsigned int reg_val = 0;

	for (i = 0; i < AW882XX_SYSST_CHECK_MAX; i++) {
		aw882xx_i2c_read(aw882xx, AW882XX_SYSST_REG, &reg_val);
		if ((((reg_val & (~AW882XX_SYSST_CHECK_MASK))) & AW882XX_SYSST_CHECK)
				== AW882XX_SYSST_CHECK) {
			ret = 0;
			break;
		} else {
			aw_dev_dbg(aw882xx->dev, "%s: check fail, cnt=%d, reg_val=0x%04x\n",
				__func__, i, reg_val);
			msleep(2);
		}
	}
	if (ret < 0)
		aw_dev_err(aw882xx->dev, "%s: check fail\n", __func__);

	return ret;
}

int aw882xx_get_sysint(struct aw882xx *aw882xx, uint16_t *sysint)
{
	int ret = -1;
	unsigned int reg_val = 0;

	ret = aw882xx_i2c_read(aw882xx, AW882XX_SYSINT_REG, &reg_val);
	if (ret < 0)
		aw_dev_err(aw882xx->dev, "%s: read sysint fail, ret=%d\n",
				__func__, ret);
	else
		*sysint = reg_val;

	return ret;
}

static int aw882xx_get_icalk(struct aw882xx *aw882xx, int16_t *icalk)
{
	int ret = -1;
	unsigned int reg_val = 0;
	uint16_t reg_icalk = 0;

	ret = aw882xx_i2c_read(aw882xx, AW882XX_EFRM1_REG, &reg_val);
	reg_icalk = (uint16_t)reg_val & AW882XX_EF_ISN_GESLP_MASK;

	if (reg_icalk & AW882XX_EF_ISN_GESLP_SIGN_MASK)
		reg_icalk = reg_icalk | AW882XX_EF_ISN_GESLP_NEG;

	*icalk = (int16_t)reg_icalk;

	return ret;
}

static int aw882xx_get_vcalk(struct aw882xx *aw882xx, int16_t *vcalk)
{
	int ret = -1;
	unsigned int reg_val = 0;
	uint16_t reg_vcalk = 0;

	ret = aw882xx_i2c_read(aw882xx, AW882XX_EFRH_REG, &reg_val);

	reg_vcalk = (uint16_t)reg_val & AW882XX_EF_VSN_GESLP_MASK;

	if (reg_vcalk & AW882XX_EF_VSN_GESLP_SIGN_MASK)
		reg_vcalk = reg_vcalk | AW882XX_EF_VSN_GESLP_NEG;

	*vcalk = (int16_t)reg_vcalk;

	return ret;
}

static int aw882xx_set_vcalb(struct aw882xx *aw882xx)
{
	int ret = -1;
	unsigned int reg_val;
	int vcalb;
	int icalk;
	int vcalk;
	int16_t icalk_val = 0;
	int16_t vcalk_val = 0;

	ret = aw882xx_get_icalk(aw882xx, &icalk_val);
	ret = aw882xx_get_vcalk(aw882xx, &vcalk_val);

	icalk = AW882XX_CABL_BASE_VALUE + AW882XX_ICABLK_FACTOR * icalk_val;
	vcalk = AW882XX_CABL_BASE_VALUE + AW882XX_VCABLK_FACTOR * vcalk_val;

	vcalb = AW882XX_VCAL_FACTOR * icalk / vcalk;

	reg_val = (unsigned int)vcalb;
	aw_dev_dbg(aw882xx->dev, "%s: icalk=%d, vcalk=%d, vcalb=%d, reg_val=%d\n",
		__func__, icalk, vcalk, vcalb, reg_val);

	ret = aw882xx_i2c_write(aw882xx, AW882XX_VTMCTRL3_REG, reg_val);

	return ret;
}

static int aw882xx_set_intmask(struct aw882xx *aw882xx, bool flag)
{
	int ret = -1;

	if (flag)
		ret = aw882xx_i2c_write(aw882xx, AW882XX_SYSINTM_REG,
					aw882xx->intmask);
	else
		ret = aw882xx_i2c_write(aw882xx, AW882XX_SYSINTM_REG,
					AW882XX_SYSINTM_DEFAULT);
	return ret;
}

static void aw882xx_set_cali_re_to_dsp(struct aw882xx *aw882xx)
{
	int ret;

	aw_dev_dbg(aw882xx->dev, "%s : cali re = 0x%x\n",
			__func__, aw882xx->cali.cali_re);

	if (aw882xx->cali.cali_re != AW_ERRO_CALI_VALUE) {
		ret = aw_write_data_to_dsp(INDEX_PARAMS_ID_RX_RE,
					&aw882xx->cali.cali_re,
					sizeof(int32_t),
					aw882xx->chan_info.channel);
		if (ret < 0)
			aw_dev_err(aw882xx->dev, "%s : set cali re to dsp failed\n",
				__func__);
	} else {
		aw_dev_err(aw882xx->dev, "%s : no set cali re to dsp re = %d\n",
			__func__, aw882xx->cali.cali_re);
	}

}

static void aw882xx_clear_sysint(struct aw882xx *aw882xx)
{
	uint16_t sysint = 0;
	int ret;

	ret = aw882xx_get_sysint(aw882xx, &sysint);
	if (ret < 0)
		aw_dev_err(aw882xx->dev, "%s: get_sysint fail, ret=%d\n",
			__func__, ret);
	else
		aw_dev_err(aw882xx->dev, "%s: get_sysint=0x%04x\n",
			__func__, sysint);

	ret = aw882xx_get_sysint(aw882xx, &sysint);
	if (ret < 0)
		aw_dev_err(aw882xx->dev, "%s: get_sysint fail, ret=%d\n",
			__func__, ret);
	else
		aw_dev_err(aw882xx->dev, "%s: get_sysint=0x%04x\n",
			__func__, sysint);
}

static int aw882xx_start(struct aw882xx *aw882xx)
{
	int ret;

	aw_dev_dbg(aw882xx->dev, "%s: enter\n", __func__);

	aw882xx_run_i2s_tx(aw882xx, true);
	aw882xx_run_pwd(aw882xx, false);
	msleep(2);
	ret = aw882xx_sysst_check(aw882xx);
	if (ret < 0) {
		aw882xx_run_mute(aw882xx, true);
		aw882xx_run_i2s_tx(aw882xx, false);
		aw882xx_run_pwd(aw882xx, true);
	} else {
		aw882xx_clear_sysint(aw882xx);
		aw882xx_set_intmask(aw882xx, true);
		aw882xx_run_mute(aw882xx, false);
		aw882xx_set_cali_re_to_dsp(aw882xx);
		aw882xx_monitor_start(&aw882xx->monitor);
	}

	return ret;
}

static void aw882xx_stop(struct aw882xx *aw882xx)
{
	int ret;
	uint16_t sysint = 0;

	aw_dev_dbg(aw882xx->dev, "%s: enter\n", __func__);

	aw882xx_monitor_stop(&aw882xx->monitor);
	aw882xx->is_power_on = AW882XX_PA_CLOSEING_ST;
	ret = aw882xx_get_sysint(aw882xx, &sysint);
	if (ret < 0)
		aw_dev_err(aw882xx->dev, "%s: get_sysint fail, ret=%d\n",
			__func__, ret);
	else
		aw_dev_err(aw882xx->dev, "%s: get_sysint=0x%04x\n",
			__func__, sysint);

	aw882xx_set_intmask(aw882xx, false);
	aw882xx_run_mute(aw882xx, true);
	aw882xx_run_i2s_tx(aw882xx, false);
	aw882xx_run_pwd(aw882xx, true);
	aw882xx->is_power_on = AW882XX_PA_CLOSE_ST;
}

/******************************************************
 *
 * aw882xx config
 *
 ******************************************************/
static void aw882xx_set_dither_en(struct aw882xx *aw882xx, uint32_t reg_val)
{
	uint32_t read_val;

	aw882xx_i2c_read(aw882xx, AW882XX_TESTCTRL2_REG, &read_val);
	read_val = read_val & AW882XX_DITHER_EN_MASK;

	read_val |= (reg_val & (~AW882XX_DITHER_EN_MASK));

	aw882xx_i2c_write(aw882xx, AW882XX_TESTCTRL2_REG, read_val);
	aw_dev_dbg(aw882xx->dev, "%s: set reg = 0x%04x, val = 0x%04x\n",
			__func__, AW882XX_TESTCTRL2_REG, read_val);
}

static int aw882xx_reg_container_update(struct aw882xx *aw882xx,
	struct aw882xx_container *aw882xx_cont)
{
	int i = 0;
	int reg_addr = 0;
	int reg_val = 0;
	int ret = -1;
	unsigned int sysctrl_val = 0;

	aw_dev_dbg(aw882xx->dev, "%s: enter\n", __func__);

	for (i = 0; i < aw882xx_cont->len; i += 4) {
		reg_addr = (aw882xx_cont->data[i+1]<<8) +
			aw882xx_cont->data[i+0];
		reg_val = (aw882xx_cont->data[i+3]<<8) +
			aw882xx_cont->data[i+2];
		aw_dev_dbg(aw882xx->dev, "%s: reg=0x%04x, val = 0x%04x\n",
			__func__, reg_addr, reg_val);
		if (reg_addr == AW882XX_SYSINTM_REG) {
			aw882xx->intmask = reg_val;
			reg_val = AW882XX_SYSINTM_DEFAULT;
		}

		if (reg_addr == AW882XX_TESTCTRL2_REG) {
			aw882xx_set_dither_en(aw882xx, (uint32_t)reg_val);
			continue;
		}

		ret = aw882xx_i2c_write(aw882xx,
			(unsigned char)reg_addr,
			(unsigned int)reg_val);
		if (ret < 0)
			break;
	}

	aw882xx_i2c_read(aw882xx, AW882XX_SYSCTRL2_REG, &sysctrl_val);
	aw882xx->hagce_val = (sysctrl_val & (~AW882XX_HAGCE_MASK));
	aw882xx->rmse_val = (sysctrl_val & (~AW882XX_RMSE_MASK));
	if (!aw882xx->hagce_enable) {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL2_REG,
			AW882XX_HAGCE_MASK, AW882XX_HAGCE_DISABLE_VALUE);
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL2_REG,
			AW882XX_RMSE_MASK, AW882XX_RMSE_DISABLE_VALUE);
	}
	aw_dev_dbg(aw882xx->dev, "%s: hagce val is 0x%04x, rmse val is 0x%04x",
			__func__, aw882xx->hagce_val, aw882xx->rmse_val);

	aw882xx_get_volume(aw882xx, &aw882xx->db_offset);

	aw_dev_dbg(aw882xx->dev, "%s: exit\n", __func__);

	return ret;
}

static void aw882xx_reg_loaded(const struct firmware *cont, void *context)
{
	int ret;
	struct aw882xx *aw882xx = context;
	struct aw882xx_container *aw882xx_cfg = NULL;
	struct aw882xx_chan_info *chan_info = &aw882xx->chan_info;

	if (!cont) {
		aw_dev_err(aw882xx->dev, "%s: failed to read %s\n", __func__,
			chan_info->bin_cfg_name[aw882xx->cfg_num]);
		release_firmware(cont);
		goto error;
	}

	aw_dev_err(aw882xx->dev, "%s: loaded %s - size: %zu\n", __func__,
		chan_info->bin_cfg_name[aw882xx->cfg_num],
		cont ? cont->size : 0);

	aw882xx_cfg = kzalloc(cont->size + sizeof(int), GFP_KERNEL);
	if (!aw882xx_cfg) {
		release_firmware(cont);
		aw_dev_err(aw882xx->dev,
			"%s: error allocating memory\n", __func__);
		goto error;
	}
	aw882xx_cfg->len = cont->size;
	memcpy(aw882xx_cfg->data, cont->data, cont->size);
	release_firmware(cont);

	mutex_lock(&aw882xx->lock);
	if (aw882xx->is_power_on != AW882XX_PA_OPENING_ST) {
		kfree(aw882xx_cfg);
		aw882xx_cfg = NULL;
		mutex_unlock(&aw882xx->lock);
		goto error;
	}

	ret = aw882xx_reg_container_update(aw882xx, aw882xx_cfg);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s: reg update fail\n", __func__);
		kfree(aw882xx_cfg);
		aw882xx_cfg = NULL;
		mutex_unlock(&aw882xx->lock);
		goto error;
	} else {
		aw_dev_err(aw882xx->dev, "%s: reg update sucess\n", __func__);
		aw882xx_run_mute(aw882xx, true);
		aw882xx_set_vcalb(aw882xx);
	}
	kfree(aw882xx_cfg);
	aw882xx_cfg = NULL;

	ret = aw882xx_start(aw882xx);
	if (ret < 0) {
		mutex_unlock(&aw882xx->lock);
		goto error;
	}
	aw882xx->init = AW882XX_INIT_OK;
	aw882xx->is_power_on = AW882XX_PA_OPEN_ST;
	mutex_unlock(&aw882xx->lock);

	return;


error:
	aw882xx->init = AW882XX_INIT_NG;
	aw882xx->is_power_on = AW882XX_PA_CLOSE_ST;
}

static int aw882xx_load_reg(struct aw882xx *aw882xx)
{
	aw_dev_err(aw882xx->dev, "%s: enter\n", __func__);

	return request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
		aw882xx->chan_info.bin_cfg_name[aw882xx->cfg_num],
		aw882xx->dev, GFP_KERNEL,
		aw882xx, aw882xx_reg_loaded);
}

static void aw882xx_get_cfg_shift(struct aw882xx *aw882xx)
{
	struct aw882xx_chan_info *chan_info = &aw882xx->chan_info;

	if (!chan_info->bin_cfg_name) {
		chan_info->bin_cfg_name = devm_kzalloc(aw882xx->dev,
				sizeof(aw882xx_cfg_name), GFP_KERNEL);
		if (!chan_info->bin_cfg_name) {
			aw_dev_err(aw882xx->dev, "%s: bin_cfg_name devm_kzalloc failed\n",
				__func__);
			return;
		}

		if (!chan_info->name_suffix) {
			snprintf(chan_info->bin_cfg_name[AW882XX_MODE_SPK_SHIFT],
				AW882XX_CFG_NAME_MAX, "%s.bin",
				aw882xx_cfg_name[AW882XX_MODE_SPK_SHIFT]);

			snprintf(chan_info->bin_cfg_name[AW882XX_MODE_RCV_SHIFT],
				AW882XX_CFG_NAME_MAX, "%s.bin",
				aw882xx_cfg_name[AW882XX_MODE_RCV_SHIFT]);
		} else {
			snprintf(chan_info->bin_cfg_name[AW882XX_MODE_SPK_SHIFT],
				AW882XX_CFG_NAME_MAX, "%s_%s.bin",
				aw882xx_cfg_name[AW882XX_MODE_SPK_SHIFT],
				chan_info->name_suffix);

			snprintf(chan_info->bin_cfg_name[AW882XX_MODE_RCV_SHIFT],
				AW882XX_CFG_NAME_MAX, "%s_%s.bin",
				aw882xx_cfg_name[AW882XX_MODE_RCV_SHIFT],
				chan_info->name_suffix);
		}
	}

	aw882xx->cfg_num = aw882xx_mode_cfg_shift[aw882xx->scene_mode];
	aw_dev_dbg(aw882xx->dev, "%s: cfg_num=%d\n",
		__func__, aw882xx->cfg_num);
}

static void aw882xx_cold_start(struct aw882xx *aw882xx)
{
	int ret = -1;

	aw_dev_err(aw882xx->dev, "%s: enter\n", __func__);

	aw882xx_get_cfg_shift(aw882xx);

	ret = aw882xx_load_reg(aw882xx);
	if (ret < 0)
		aw_dev_err(aw882xx->dev, "%s: cfg loading requested failed: %d\n",
			__func__, ret);
}

void aw882xx_smartpa_cfg(struct aw882xx *aw882xx, bool flag)
{
	int ret;

	aw_dev_err(aw882xx->dev, "%s: flag = %d, power status = %d\n",
		__func__, flag, aw882xx->is_power_on);

	mutex_lock(&aw882xx->lock);
	if (flag == true && aw882xx->aw882xx_pa_switch == AW882XX_ON_PA) {
		if (aw882xx->is_power_on == AW882XX_PA_CLOSE_ST) {
			aw882xx->is_power_on = AW882XX_PA_OPENING_ST;
			if ((aw882xx->init == AW882XX_INIT_ST) ||
				(aw882xx->init == AW882XX_INIT_NG)) {
				aw_dev_err(aw882xx->dev, "%s: init = %d\n",
					__func__, aw882xx->init);
				aw882xx_load_cali_re(&aw882xx->cali);
				aw882xx_cold_start(aw882xx);
			} else {
				ret = aw882xx_start(aw882xx);
				if (ret < 0) {
					aw882xx->is_power_on = AW882XX_PA_CLOSE_ST;
					aw882xx->init = AW882XX_INIT_NG;
				} else {
					aw882xx->is_power_on = AW882XX_PA_OPEN_ST;
					aw882xx->init = AW882XX_INIT_OK;
				}
			}
		}
	} else {
		aw882xx_stop(aw882xx);
	}
	mutex_unlock(&aw882xx->lock);
}

/******************************************************
 *
 * kcontrol
 *
 ******************************************************/
static const char *const mode_function[] = { "Spk", "Rcv"};
static const char *const pa_switch_function[] = { "On", "Off" };
static const char *const awinic_algo[] = { "Disable", "Enable" };
static const char *const hagc_status[] = {"Disable", "Reset"};
static const DECLARE_TLV_DB_SCALE(digital_gain, 0, 50, 0);

struct soc_mixer_control aw882xx_mixer = {
	.reg	= AW882XX_HAGCCFG4_REG,
	.shift	= AW882XX_VOL_START_BIT,
	.max	= AW882XX_VOLUME_MAX,
	.min	= AW882XX_VOLUME_MIN,
};

static int aw882xx_volume_info(struct snd_kcontrol *kcontrol,
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

static int aw882xx_volume_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	unsigned int reg_val = 0;
	/*unsigned int value = 0;*/
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *) kcontrol->private_value;

	aw882xx_i2c_read(aw882xx, AW882XX_HAGCCFG4_REG, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> mc->shift) &
		(AW882XX_VOL_MASK);
	return 0;
}

static int aw882xx_volume_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *) kcontrol->private_value;
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	unsigned int value = 0;
	unsigned int reg_value = 0;

	/* value is right */
	value = ucontrol->value.integer.value[0];
	if (value > (mc->max-mc->min)) {
		aw_dev_err(aw882xx->dev, "%s: value over range\n", __func__);
		return -ERANGE;
	}

	/* smartpa have clk */
	aw882xx_i2c_read(aw882xx, AW882XX_SYSST_REG, &reg_value);
	if (!(reg_value & AW882XX_PLLS_LOCKED_VALUE)) {
		aw_dev_err(aw882xx->dev, "%s: NO I2S CLK ,cat not write reg\n",
			__func__);
		return 0;
	}

	/* cal real value */
	value = (value << mc->shift) & (~AW882XX_VOL_MASK);
	aw882xx_i2c_read(aw882xx, AW882XX_HAGCCFG4_REG, &reg_value);
	value = value | (reg_value & 0x00ff);

	/* write value */
	aw882xx_i2c_write(aw882xx, AW882XX_HAGCCFG4_REG, value);

	return 0;
}

static struct snd_kcontrol_new aw882xx_volume = {
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "aw882xx_rx_volume",
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |
		SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.tlv.p = (digital_gain),
	.info = aw882xx_volume_info,
	.get = aw882xx_volume_get,
	.put = aw882xx_volume_put,
	.private_value = (unsigned long)&aw882xx_mixer,
};

static int aw882xx_mode_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_dbg(aw882xx->dev, "%s: aw882xx_scene_control=%d\n",
		__func__, aw882xx->scene_mode);

	ucontrol->value.integer.value[0] = aw882xx->scene_mode;

	return 0;
}

static int aw882xx_mode_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_dbg(aw882xx->dev, "%s: ucontrol->value.integer.value[0]=%ld\n",
		__func__, ucontrol->value.integer.value[0]);

	if (ucontrol->value.integer.value[0] == aw882xx->scene_mode)
		return 1;

	aw882xx->scene_mode = ucontrol->value.integer.value[0];

	aw882xx->init = AW882XX_INIT_ST;


	return 0;
}

static int aw882xx_rx_enable_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = -EINVAL;
	uint32_t ctrl_value = 0;

	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	ret = aw_get_module_enable(&ctrl_value, AW_RX_MODULE);
	if (ret)
		aw_dev_err(aw882xx->dev, "%s: dsp_msg error, ret=%d\n",
			__func__, ret);

	ucontrol->value.integer.value[0] = ctrl_value;

	aw_dev_dbg(aw882xx->dev, "%s: aw882xx_rx_enable %d\n",
		__func__, ctrl_value);
	return 0;
}

static int aw882xx_rx_enable_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = -EINVAL;
	uint32_t ctrl_value = 0;
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_dbg(aw882xx->dev, "%s: ucontrol->value.integer.value[0]=%ld\n",
		__func__, ucontrol->value.integer.value[0]);

	ctrl_value = ucontrol->value.integer.value[0];
	ret = aw_send_module_enable(&ctrl_value, AW_RX_MODULE);
	if (ret)
		aw_dev_err(aw882xx->dev, "%s: dsp_msg error, ret=%d\n",
			__func__, ret);

	return 0;
}

static int aw882xx_tx_enable_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = -EINVAL;
	uint32_t ctrl_value = 0;
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	ret = aw_get_module_enable(&ctrl_value, AW_TX_MODULE);
	if (ret)
		aw_dev_err(aw882xx->dev, "%s: dsp_msg error, ret=%d\n",
			__func__, ret);

	ucontrol->value.integer.value[0] = ctrl_value;

	aw_dev_dbg(aw882xx->dev, "%s: aw882xx_tx_enable %d\n",
		__func__, ctrl_value);
	return 0;
}

static int aw882xx_tx_enable_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = -EINVAL;
	uint32_t ctrl_value = 0;

	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_dbg(aw882xx->dev, "%s: ucontrol->value.integer.value[0]=%ld\n",
		__func__, ucontrol->value.integer.value[0]);

	ctrl_value = ucontrol->value.integer.value[0];

	ret = aw_send_module_enable(&ctrl_value, AW_TX_MODULE);
	if (ret)
		aw_dev_err(aw882xx->dev, "%s: dsp_msg error, ret=%d\n",
			__func__, ret);

	return 0;
}

static int aw882xx_pa_switch_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	int *pa_switch_ptr = &aw882xx->aw882xx_pa_switch;

	aw_dev_dbg(aw882xx->dev, "%s: aw882xx_pa_switch=%d\n",
			__func__, *pa_switch_ptr);

	ucontrol->value.integer.value[0] = *pa_switch_ptr;

	return 0;
}

static int aw882xx_pa_switch_set(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	int ret;
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	int *aw882xx_pa_switch = &aw882xx->aw882xx_pa_switch;

	aw_dev_dbg(aw882xx->dev, "%s: ucontrol->value.integer.value[0]=%ld\n",
			__func__, ucontrol->value.integer.value[0]);

	if (ucontrol->value.integer.value[0] == *aw882xx_pa_switch)
		return 1;

	*aw882xx_pa_switch = ucontrol->value.integer.value[0];

	mutex_lock(&aw882xx->lock);
	if (aw882xx->audio_stream_st == AW882XX_AUDIO_START) {
		if ((*aw882xx_pa_switch == AW882XX_OFF_PA) &&
			(aw882xx->is_power_on == AW882XX_PA_OPEN_ST))
			aw882xx_stop(aw882xx);
		else if ((*aw882xx_pa_switch == AW882XX_ON_PA) &&
			(aw882xx->is_power_on == AW882XX_PA_CLOSE_ST)) {
			aw882xx->is_power_on = AW882XX_PA_OPENING_ST;
			ret = aw882xx_start(aw882xx);
			if (ret < 0) {
				aw882xx->is_power_on = AW882XX_PA_CLOSE_ST;
				aw882xx->init = AW882XX_INIT_NG;
			} else {
				aw882xx->is_power_on = AW882XX_PA_OPEN_ST;
				aw882xx->init = AW882XX_INIT_OK;
			}
		}
	}
	mutex_unlock(&aw882xx->lock);

	return 0;
}

static int aw882xx_copp_en_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = -EINVAL;
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_dbg(aw882xx->dev, "%s: ucontrol->value.integer.value[0]=%ld\n",
		__func__, ucontrol->value.integer.value[0]);

	mutex_lock(&g_aw_copp_lock);
	g_copp_en = ucontrol->value.integer.value[0];
	ret = aw_dsp_copp_module_en(g_copp_en);
	if (ret)
		aw_dev_err(aw882xx->dev, "%s: dsp_msg error, ret=%d\n",
			__func__, ret);

	mutex_unlock(&g_aw_copp_lock);

	return 0;
}

static int aw882xx_copp_en_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	mutex_lock(&g_aw_copp_lock);
	ucontrol->value.integer.value[0] = g_copp_en;
	mutex_unlock(&g_aw_copp_lock);

	aw_dev_dbg(aw882xx->dev, "%s: ucontrol->value.integer.value[0]=%ld\n",
		__func__, ucontrol->value.integer.value[0]);
	return 0;
}

static int aw882xx_hagc_en_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = aw882xx->hagce_enable;
	return 0;
}

static int aw882xx_hagc_en_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	aw_snd_soc_codec_t *codec =
		aw_componet_codec_ops.aw_snd_soc_kcontrol_codec(kcontrol);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	if (aw882xx->hagce_enable == ucontrol->value.integer.value[0]) {
		aw_dev_err(aw882xx->dev, "%s: hagce enable nochange", __func__);
		return 0;
	}

	aw882xx->hagce_enable = ucontrol->value.integer.value[0];
	if (aw882xx->hagce_enable) {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL2_REG,
				AW882XX_HAGCE_MASK, aw882xx->hagce_val);
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL2_REG,
				AW882XX_RMSE_MASK, aw882xx->rmse_val);
	} else {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL2_REG,
			AW882XX_HAGCE_MASK, AW882XX_HAGCE_DISABLE_VALUE);
		aw882xx_i2c_write_bits(aw882xx, AW882XX_SYSCTRL2_REG,
			AW882XX_RMSE_MASK, AW882XX_RMSE_DISABLE_VALUE);
	}

	aw_dev_dbg(aw882xx->dev, "%s: ucontrol->value.integer.value[0]=%ld\n", __func__,
		ucontrol->value.integer.value[0]);
	return 0;
}

static const struct soc_enum aw882xx_snd_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mode_function), mode_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(pa_switch_function), pa_switch_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(awinic_algo), awinic_algo),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hagc_status), hagc_status),
};

static struct snd_kcontrol_new aw882xx_controls[] = {
	SOC_ENUM_EXT("aw882xx_mode_switch", aw882xx_snd_enum[0],
		aw882xx_mode_get, aw882xx_mode_set),
	SOC_ENUM_EXT("aw882xx_pa_switch", aw882xx_snd_enum[1],
		aw882xx_pa_switch_get, aw882xx_pa_switch_set),
	SOC_ENUM_EXT("aw882xx_rx_switch", aw882xx_snd_enum[2],
		aw882xx_rx_enable_get, aw882xx_rx_enable_set),
	SOC_ENUM_EXT("aw882xx_tx_switch", aw882xx_snd_enum[2],
		aw882xx_tx_enable_get, aw882xx_tx_enable_set),
	SOC_ENUM_EXT("aw882xx_copp_switch", aw882xx_snd_enum[2],
		aw882xx_copp_en_get, aw882xx_copp_en_set),
	SOC_ENUM_EXT("aw882xx_hagc_switch", aw882xx_snd_enum[3],
		aw882xx_hagc_en_get, aw882xx_hagc_en_set),
};

static void aw882xx_kcontrol_append_suffix(struct aw882xx *aw882xx,
	struct snd_kcontrol_new *src_control, int num)
{
	int i = 0, ret;
	struct snd_kcontrol_new *dst_control = NULL;

	dst_control = devm_kzalloc(aw882xx->dev,
		num * sizeof(struct snd_kcontrol_new), GFP_KERNEL);
	if (!dst_control) {
		aw_dev_err(aw882xx->dev, "kcontrol kzalloc faild\n");
		return;
	}
	memcpy(dst_control, src_control, num * sizeof(struct snd_kcontrol_new));

	for (i = 0; i < num; i++) {
		ret = aw882xx_append_suffix("%s_%s",
			(const char **)&dst_control[i].name, aw882xx);
		if (ret < 0)
			return;
	}
	aw_componet_codec_ops.aw_snd_soc_add_codec_controls(aw882xx->codec,
						dst_control, num);
}

void aw882xx_add_codec_controls(struct aw882xx *aw882xx)
{
	aw_dev_err(aw882xx->dev, "%s: enter\n", __func__);

	aw882xx_kcontrol_append_suffix(aw882xx,
		aw882xx_controls, ARRAY_SIZE(aw882xx_controls));
	aw882xx_kcontrol_append_suffix(aw882xx, &aw882xx_volume, 1);
}


/******************************************************
 *
 * Digital Audio Interface
 *
 ******************************************************/
static int aw882xx_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		aw_dev_err(aw882xx->dev, "%s: playback enter\n", __func__);
	else
		aw_dev_err(aw882xx->dev, "%s: capture enter\n", __func__);

	return 0;
}

static int aw882xx_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	/*struct aw882xx *aw882xx = aw_snd_soc_codec_get_drvdata(dai->codec);*/
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);

	aw_dev_err(codec->dev, "%s: fmt=0x%x\n", __func__, fmt);

	/* Supported mode: regular I2S, slave, or PDM */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) !=
			SND_SOC_DAIFMT_CBS_CFS) {
			aw_dev_err(codec->dev, "%s: invalid codec master mode\n",
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

static int aw882xx_set_dai_sysclk(struct snd_soc_dai *dai,
	int clk_id, unsigned int freq, int dir)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_err(aw882xx->dev, "%s: freq=%d\n", __func__, freq);

	aw882xx->sysclk = freq;
	return 0;
}

static int aw882xx_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	unsigned int rate = 0;
	uint32_t cco_mux_value;
	int reg_value = 0;
	int width = 0;


	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		aw_dev_dbg(aw882xx->dev, "%s: requested rate: %d, sample size: %d\n",
				__func__, rate,
				snd_pcm_format_width(params_format(params)));
		return 0;
	}
	/* get rate param */
	aw882xx->rate = rate = params_rate(params);
	aw_dev_dbg(aw882xx->dev, "%s: requested rate: %d, sample size: %d\n",
		__func__, rate, snd_pcm_format_width(params_format(params)));

	/* match rate */
	switch (rate) {
	case 8000:
		reg_value = AW882XX_I2SSR_8KHZ_VALUE;
		cco_mux_value = AW882XX_I2S_CCO_MUX_8_16_32KHZ_VALUE;
		break;
	case 16000:
		reg_value = AW882XX_I2SSR_16KHZ_VALUE;
		cco_mux_value = AW882XX_I2S_CCO_MUX_8_16_32KHZ_VALUE;
		break;
	case 32000:
		reg_value = AW882XX_I2SSR_32KHZ_VALUE;
		cco_mux_value = AW882XX_I2S_CCO_MUX_8_16_32KHZ_VALUE;
		break;
	case 44100:
		reg_value = AW882XX_I2SSR_44P1KHZ_VALUE;
		cco_mux_value = AW882XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		break;
	case 48000:
		reg_value = AW882XX_I2SSR_48KHZ_VALUE;
		cco_mux_value = AW882XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		break;
	case 96000:
		reg_value = AW882XX_I2SSR_96KHZ_VALUE;
		cco_mux_value = AW882XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		break;
	case 192000:
		reg_value = AW882XX_I2SSR_192KHZ_VALUE;
		cco_mux_value = AW882XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		break;
	default:
		reg_value = AW882XX_I2SSR_48KHZ_VALUE;
		cco_mux_value = AW882XX_I2S_CCO_MUX_EXC_8_16_32KHZ_VALUE;
		aw_dev_err(aw882xx->dev, "%s: rate can not support\n",
			__func__);
		break;
	}
	aw882xx_i2c_write_bits(aw882xx, AW882XX_PLLCTRL1_REG,
				AW882XX_I2S_CCO_MUX_MASK, cco_mux_value);

	/* set chip rate */
	if (-1 != reg_value) {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_I2SCTRL_REG,
				AW882XX_I2SSR_MASK, reg_value);
	}


	/* get bit width */
	width = params_width(params);
	aw_dev_dbg(aw882xx->dev, "%s: width = %d\n", __func__, width);
	switch (width) {
	case 16:
		reg_value = AW882XX_I2SFS_16_BITS_VALUE;
		break;
	case 20:
		reg_value = AW882XX_I2SFS_20_BITS_VALUE;
		break;
	case 24:
		reg_value = AW882XX_I2SFS_24_BITS_VALUE;
		break;
	case 32:
		reg_value = AW882XX_I2SFS_32_BITS_VALUE;
		break;
	default:
		reg_value = AW882XX_I2SFS_16_BITS_VALUE;
		aw_dev_err(aw882xx->dev,
			"%s: width can not support\n", __func__);
		break;
	}

	/* get width */
	if (-1 != reg_value) {
		aw882xx_i2c_write_bits(aw882xx, AW882XX_I2SCTRL_REG,
				AW882XX_I2SFS_MASK, reg_value);
	}

	return 0;
}

static int aw882xx_mute(struct snd_soc_dai *dai, int mute, int stream)
{
	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_err(aw882xx->dev, "%s: mute state=%d\n", __func__, mute);

	if (!(aw882xx->flags & AW882XX_FLAG_START_ON_MUTE))
		return 0;

	if (mute) {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			aw882xx_smartpa_cfg(aw882xx, false);
			aw882xx->audio_stream_st = AW882XX_AUDIO_STOP;
		}
	} else {
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			aw882xx_smartpa_cfg(aw882xx, true);
			aw882xx->audio_stream_st = AW882XX_AUDIO_START;
		}
	}

	return 0;
}

static void aw882xx_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{

	aw_snd_soc_codec_t *codec = aw_get_codec(dai);
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		aw882xx->rate = 0;

}

static const struct snd_soc_dai_ops aw882xx_dai_ops = {
	.startup = aw882xx_startup,
	.set_fmt = aw882xx_set_fmt,
	.set_sysclk = aw882xx_set_dai_sysclk,
	.hw_params = aw882xx_hw_params,
	.mute_stream = aw882xx_mute,
	.shutdown = aw882xx_shutdown,
};

static struct snd_soc_dai_driver aw882xx_dai[] = {
	{
		.name = "aw882xx-aif",
		.id = 1,
		.playback = {
			.stream_name = "Speaker_Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = AW882XX_RATES,
			.formats = AW882XX_FORMATS,
		},
		.capture = {
			.stream_name = "Speaker_Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = AW882XX_RATES,
			.formats = AW882XX_FORMATS,
		 },
		.ops = &aw882xx_dai_ops,
		.symmetric_rates = 1,
	},
};

/*****************************************************
 *
 * codec driver
 *
 *****************************************************/
static int aw882xx_probe(aw_snd_soc_codec_t *codec)
{
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	struct aw882xx_chan_info *chan_info = &aw882xx->chan_info;

	aw_dev_err(aw882xx->dev, "%s: enter\n", __func__);

	aw882xx->codec = codec;

	aw882xx_add_codec_controls(aw882xx);

	if (codec->dev->of_node) {
		if (chan_info->name_suffix)
			dev_set_name(codec->dev, "%s_%s", "aw882xx_smartpa",
				chan_info->name_suffix);
		else
			dev_set_name(codec->dev, "%s", "aw882xx_smartpa");
	}


	schedule_delayed_work(&aw882xx->monitor.load_fw_work,
			msecs_to_jiffies(AW_LOAD_MON_FW_DELAY_TIME));

	aw_dev_err(aw882xx->dev, "%s: exit\n", __func__);

	return 0;
}

#ifdef AW_KERNEL_VER_OVER_4_19_1
static void aw882xx_remove(struct snd_soc_component *component)
{
	/*struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);*/
	aw_dev_err(component->dev, "%s: enter\n", __func__);
}
#else
static int aw882xx_remove(aw_snd_soc_codec_t *codec)
{
	/*struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);*/
	aw_dev_err(codec->dev, "%s: enter\n", __func__);

	return 0;
}
#endif

static unsigned int aw882xx_codec_read(aw_snd_soc_codec_t *codec,
	unsigned int reg)
{
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);
	unsigned int value = 0;
	int ret = -1;

	aw_dev_dbg(aw882xx->dev, "%s: enter\n", __func__);

	if (aw882xx_reg_access[reg] & REG_RD_ACCESS) {
		ret = aw882xx_i2c_read(aw882xx, reg, &value);
		if (ret < 0)
			aw_dev_dbg(aw882xx->dev, "%s: read register failed\n",
				__func__);
	} else {
		aw_dev_dbg(aw882xx->dev, "%s: register 0x%x no read access\n",
			__func__, reg);
	}
	return ret;
}

static int aw882xx_codec_write(aw_snd_soc_codec_t *codec,
	unsigned int reg, unsigned int value)
{
	int ret = -1;
	struct aw882xx *aw882xx =
		aw_componet_codec_ops.aw_snd_soc_codec_get_drvdata(codec);

	aw_dev_dbg(aw882xx->dev, "%s: enter ,reg is 0x%x value is 0x%x\n",
		__func__, reg, value);

	if (aw882xx_reg_access[reg]&REG_WR_ACCESS) {
		ret = aw882xx_i2c_write(aw882xx, reg, value);
	} else {
		aw_dev_dbg(aw882xx->dev, "%s: register 0x%x no write access\n",
			__func__, reg);
	}

	return ret;
}

#ifdef AW_KERNEL_VER_OVER_4_19_1
static struct snd_soc_component_driver soc_codec_dev_aw882xx = {
	.probe = aw882xx_probe,
	.remove = aw882xx_remove,
	.read = aw882xx_codec_read,
	.write = aw882xx_codec_write,
};
#else
static struct snd_soc_codec_driver soc_codec_dev_aw882xx = {
	.probe = aw882xx_probe,
	.remove = aw882xx_remove,
	.read = aw882xx_codec_read,
	.write = aw882xx_codec_write,
	.reg_cache_size = AW882XX_REG_MAX,
	.reg_word_size = 2,
    .component_driver = {
    .dapm_widgets = aw882xx_dapm_widgets,
    .num_dapm_widgets = ARRAY_SIZE(aw882xx_dapm_widgets),
    .dapm_routes = aw882xx_audio_map,
    .num_dapm_routes = ARRAY_SIZE(aw882xx_audio_map),
    },
};
#endif

/******************************************************
 *
 * irq
 *
 ******************************************************/
static void aw882xx_interrupt_setup(struct aw882xx *aw882xx)
{
	unsigned int reg_val;

	aw_dev_err(aw882xx->dev, "%s: enter\n", __func__);

	aw882xx_i2c_read(aw882xx, AW882XX_SYSINTM_REG, &reg_val);
	reg_val &= (~AW882XX_UVLS_VDD_BELOW_2P8V_VALUE);
	reg_val &= (~AW882XX_NOCLKS_TRIG_VALUE);
	reg_val &= (~AW882XX_CLKS_TRIG_VALUE);
	aw882xx_i2c_write(aw882xx, AW882XX_SYSINTM_REG, reg_val);
}

static void aw882xx_interrupt_clear(struct aw882xx *aw882xx)
{
	unsigned int reg_val = 0;

	aw_dev_err(aw882xx->dev, "%s: enter\n", __func__);

	aw882xx_i2c_read(aw882xx, AW882XX_SYSST_REG, &reg_val);
	aw_dev_err(aw882xx->dev, "%s: reg SYSST=0x%x\n", __func__, reg_val);

	aw882xx_i2c_read(aw882xx, AW882XX_SYSINT_REG, &reg_val);
	aw_dev_err(aw882xx->dev, "%s: reg SYSINT=0x%x\n", __func__, reg_val);

	aw882xx_i2c_read(aw882xx, AW882XX_SYSINTM_REG, &reg_val);
	aw_dev_err(aw882xx->dev, "%s: reg SYSINTM=0x%x\n", __func__, reg_val);
}

static irqreturn_t aw882xx_irq(int irq, void *data)
{
	struct aw882xx *aw882xx = data;

	aw_dev_err(aw882xx->dev, "%s: enter\n", __func__);

	aw882xx_interrupt_clear(aw882xx);

	aw_dev_err(aw882xx->dev, "%s: exit\n", __func__);

	return IRQ_HANDLED;
}

/*****************************************************
 *
 * device tree
 *
 *****************************************************/
static void aw882xx_parse_gpio_dt(struct aw882xx *aw882xx,
	struct device_node *np)
{
	aw882xx->reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);
	if (aw882xx->reset_gpio < 0) {
		aw_dev_err(aw882xx->dev,
			"%s: no reset gpio provided, will not HW reset device\n",
			__func__);
	} else {
		aw_dev_err(aw882xx->dev, "%s: reset gpio provided ok\n",
			__func__);
	}
	aw882xx->irq_gpio = of_get_named_gpio(np, "irq-gpio", 0);
	if (aw882xx->irq_gpio < 0)
		aw_dev_err(aw882xx->dev, "%s: no irq gpio provided.\n",
			__func__);
	else
		aw_dev_err(aw882xx->dev, "%s: irq gpio provided ok.\n",
			__func__);
}

static void aw882xx_parse_channel_dt(struct aw882xx *aw882xx,
	struct device_node *np)
{
	int ret;
	const char *channel_value = NULL;
	struct aw882xx_chan_info *chan_info = &aw882xx->chan_info;

	chan_info->channel = AW882XX_CHANNLE_LEFT_MONO;
	chan_info->name_suffix = NULL;
	ret = of_property_read_string(np, "sound-channel", &channel_value);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev,
			"%s:read sound-channel failed,use default\n", __func__);
		return;
	}
	aw_dev_dbg(aw882xx->dev,
		"%s: read sound-channel value is : %s\n",
		__func__, channel_value);

	if (!strcmp(channel_value, "left")) {
		chan_info->name_suffix = "l";
	} else if (!strcmp(channel_value, "right")) {
		chan_info->channel = AW882XX_CHANNLE_RIGHT;
		chan_info->name_suffix = "r";
	} else {
		aw_dev_err(aw882xx->dev, "%s:not stereo channel,use default single track\n",
			__func__);
	}
}

static void aw882xx_parse_dt(struct device *dev, struct aw882xx *aw882xx,
		struct device_node *np)
{
	aw882xx_parse_gpio_dt(aw882xx, np);
	aw882xx_parse_channel_dt(aw882xx, np);
	aw882xx_parse_cali_mode_dt(&aw882xx->cali);
	aw882xx_parse_cali_way_dt(&aw882xx->cali);
	aw882xx_parse_monitor_dt(&aw882xx->monitor);
}

void aw882xx_hw_reset(struct aw882xx *aw882xx)
{
	aw_dev_err(aw882xx->dev, "%s: enter\n", __func__);

	if (gpio_is_valid(aw882xx->reset_gpio)) {
		gpio_set_value_cansleep(aw882xx->reset_gpio, 0);
		msleep(1);
		gpio_set_value_cansleep(aw882xx->reset_gpio, 1);
		msleep(2);
	} else {
		aw_dev_err(aw882xx->dev, "%s: failed\n", __func__);
	}

}

/*****************************************************
 *
 * check chip id
 *
 *****************************************************/
static int aw882xx_read_chipid(struct aw882xx *aw882xx)
{
	int ret = -1;
	unsigned int cnt = 0;
	unsigned int reg = 0;

	while (cnt < AW_READ_CHIPID_RETRIES) {
		ret = aw882xx_i2c_read(aw882xx, AW882XX_ID_REG, &reg);
		if (ret < 0) {
			aw_dev_err(aw882xx->dev,
				"%s: failed to read REG_ID: %d\n",
				__func__, ret);
			return -EIO;
		}
		switch (reg) {
		case AW882XX_ID:
			aw_dev_err(aw882xx->dev, "%s: aw882xx detected\n",
				__func__);
			aw882xx->flags |= AW882XX_FLAG_SKIP_INTERRUPTS;
			aw882xx->flags |= AW882XX_FLAG_START_ON_MUTE;
			aw882xx->chipid = AW882XX_ID;
			aw_dev_err(aw882xx->dev, "%s: aw882xx->flags=0x%x\n",
				__func__, aw882xx->flags);
			return 0;
		default:
			aw_dev_err(aw882xx->dev, "%s: unsupported device revision (0x%x)\n",
				__func__, reg);
			break;
		}
		cnt++;

		msleep(AW_READ_CHIPID_RETRY_DELAY);
	}

	return -EINVAL;
}

/******************************************************
 *
 * sys group attribute: reg
 *
 ******************************************************/
static ssize_t aw882xx_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	unsigned int databuf[2] = {0};

	if (2 == sscanf(buf, "%x %x", &databuf[0], &databuf[1]))
		aw882xx_i2c_write(aw882xx, databuf[0], databuf[1]);

	return count;
}

static ssize_t aw882xx_reg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	unsigned char i = 0;
	unsigned int reg_val = 0;

	for (i = 0; i < AW882XX_REG_MAX; i++) {
		if (aw882xx_reg_access[i]&REG_RD_ACCESS) {
			aw882xx_i2c_read(aw882xx, i, &reg_val);
			len += snprintf(buf+len, PAGE_SIZE-len,
				"reg:0x%02x=0x%04x\n", i, reg_val);
		}
	}

	return len;
}

static ssize_t aw882xx_rw_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	unsigned int databuf[2] = {0};

	if (2 == sscanf(buf, "%x %x", &databuf[0], &databuf[1])) {
		aw882xx->reg_addr = (unsigned char)databuf[0];
		aw882xx_i2c_write(aw882xx, databuf[0], databuf[1]);
	} else if (1 == sscanf(buf, "%x", &databuf[0])) {
		aw882xx->reg_addr = (unsigned char)databuf[0];
	}

	return count;
}

static ssize_t aw882xx_rw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	unsigned int reg_val = 0;

	if (aw882xx_reg_access[aw882xx->reg_addr] & REG_RD_ACCESS) {
		aw882xx_i2c_read(aw882xx, aw882xx->reg_addr, &reg_val);
		len += snprintf(buf+len, PAGE_SIZE-len,
			"reg:0x%02x=0x%04x\n", aw882xx->reg_addr, reg_val);
	}
	return len;
}

static ssize_t aw882xx_driver_ver_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len,
		"driver version:%s\n", AW882XX_DRIVER_VERSION);

	return len;
}

static ssize_t aw882xx_fade_step_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);

	unsigned int databuf[2] = {0};

	/*step 0 - 12*/
	if (1 == sscanf(buf, "%d", &databuf[0])) {
		if (databuf[0] > (AW882XX_VOLUME_STEP_DB * 2)) {
			aw_dev_err(aw882xx->dev, "%s: step overflow %d Db",
					__func__, databuf[0]);
			return count;
		}
		aw882xx->fade_step = databuf[0];
	}
	aw_dev_err(aw882xx->dev, "%s: set step %d.%d DB Done",
		__func__, GET_DB_INT(databuf[0]), GET_DB_DECIMAL(databuf[0]));

	return count;
}

static ssize_t aw882xx_fade_step_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len,
		"step: %d\n", aw882xx->fade_step);

	return len;
}


static DEVICE_ATTR(reg, S_IWUSR | S_IRUGO,
	aw882xx_reg_show, aw882xx_reg_store);
static DEVICE_ATTR(rw, S_IWUSR | S_IRUGO,
	aw882xx_rw_show, aw882xx_rw_store);
static DEVICE_ATTR(driver_ver, S_IRUGO,
	aw882xx_driver_ver_show, NULL);
static DEVICE_ATTR(fade_step, S_IWUSR | S_IRUGO,
	aw882xx_fade_step_show, aw882xx_fade_step_store);


static struct attribute *aw882xx_attributes[] = {
	&dev_attr_reg.attr,
	&dev_attr_rw.attr,
	&dev_attr_driver_ver.attr,
	&dev_attr_fade_step.attr,
	NULL
};

static struct attribute_group aw882xx_attribute_group = {
	.attrs = aw882xx_attributes,
};

/******************************************************
 *
 * i2c driver
 *
 ******************************************************/
int aw882xx_i2c_probe(struct i2c_client *i2c,
	const struct i2c_device_id *id)
{
	struct snd_soc_dai_driver *dai = NULL;
	struct aw882xx *aw882xx = NULL;
	struct device_node *np = i2c->dev.of_node;
	struct aw882xx_chan_info *chan_info = NULL;
	const char *aw882xx_rst = "aw882xx_rst";
	const char *aw882xx_int = "aw882xx_int";
	const char *aw882xx_irq_name = "aw882xx";
	int irq_flags = 0;
	int ret;

	aw_dev_err(&i2c->dev, "weichang  %s: enter\n", __func__);

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		aw_dev_err(&i2c->dev, "check_functionality failed\n");
		return -EIO;
	}

	aw882xx = devm_kzalloc(&i2c->dev, sizeof(struct aw882xx), GFP_KERNEL);
	if (aw882xx == NULL)
		return -ENOMEM;

	aw882xx->dev = &i2c->dev;
	aw882xx->i2c = i2c;
	chan_info = &aw882xx->chan_info;
	i2c_set_clientdata(i2c, aw882xx);
	mutex_init(&aw882xx->lock);
	aw882xx->hagce_enable = true;

	/* aw882xx rst & int */
	if (np) {
		aw882xx_parse_dt(&i2c->dev, aw882xx, np);
	} else {
		aw882xx->reset_gpio = -1;
		aw882xx->irq_gpio = -1;
	}

	if (gpio_is_valid(aw882xx->reset_gpio)) {
		ret = aw882xx_append_suffix("%s_%s", &aw882xx_rst, aw882xx);
		if (ret < 0)
			return ret;

		ret = devm_gpio_request_one(&i2c->dev, aw882xx->reset_gpio,
			GPIOF_OUT_INIT_LOW, aw882xx_rst);
		if (ret) {
			aw_dev_err(&i2c->dev, "%s: rst request failed\n",
				__func__);
			return ret;
		}
	}

	if (gpio_is_valid(aw882xx->irq_gpio)) {
		ret = aw882xx_append_suffix("%s_%s", &aw882xx_int, aw882xx);
		if (ret < 0)
			return ret;

		ret = devm_gpio_request_one(&i2c->dev, aw882xx->irq_gpio,
			GPIOF_DIR_IN, aw882xx_int);
		if (ret) {
			aw_dev_err(&i2c->dev, "%s: int request failed\n",
				__func__);
			return ret;
		}
	}

	/* hardware reset */
	aw882xx_hw_reset(aw882xx);

	/* aw882xx chip id */
	ret = aw882xx_read_chipid(aw882xx);
	if (ret < 0) {
		aw_dev_err(&i2c->dev, "%s: aw882xx_read_chipid failed ret=%d\n",
			__func__, ret);
		return ret;
	}

	/* aw882xx device name */
	if (np) {
		if (chan_info->name_suffix)
			dev_set_name(&i2c->dev, "%s_%s", "aw882xx_smartpa",
				chan_info->name_suffix);
		else
			dev_set_name(&i2c->dev, "%s", "aw882xx_smartpa");
	} else {
		aw_dev_err(&i2c->dev, "%s failed to set device name: %d\n",
			__func__, ret);
	}

	/* register codec */
	dai = devm_kzalloc(&i2c->dev, sizeof(aw882xx_dai), GFP_KERNEL);
	if (!dai)
		return -ENOMEM;

	memcpy(dai, aw882xx_dai, sizeof(aw882xx_dai));

	/*Change the DAI name according to the channel*/
	ret = aw882xx_append_suffix("%s-%s", &dai->name, aw882xx);
	if (ret < 0)
		return ret;
	ret = aw882xx_append_suffix("%s_%s",
		&dai->playback.stream_name, aw882xx);
	if (ret < 0)
		return ret;
	ret = aw882xx_append_suffix("%s_%s",
		&dai->capture.stream_name, aw882xx);
	if (ret < 0)
		return ret;

	aw_dev_err(aw882xx->dev, "%s: dai->name(%s)\n", __func__, dai->name);
	ret = aw_componet_codec_ops.aw_snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_aw882xx,
			dai, ARRAY_SIZE(aw882xx_dai));
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s failed to register aw882xx: %d\n",
			__func__, ret);
		return ret;
	}

	/* aw882xx irq */
	if (gpio_is_valid(aw882xx->irq_gpio) &&
		!(aw882xx->flags & AW882XX_FLAG_SKIP_INTERRUPTS)) {
		/* register irq handler */
		aw882xx_interrupt_setup(aw882xx);

		aw882xx_append_suffix("%s_%s", &aw882xx_irq_name, aw882xx);

		irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
		ret = devm_request_threaded_irq(&i2c->dev,
				gpio_to_irq(aw882xx->irq_gpio),
				NULL, aw882xx_irq, irq_flags,
				aw882xx_irq_name, aw882xx);
		if (ret != 0) {
			aw_dev_err(aw882xx->dev, "failed to request IRQ %d: %d\n",
				gpio_to_irq(aw882xx->irq_gpio), ret);
			goto err_irq;
		}
	} else {
		aw_dev_err(aw882xx->dev, "%s skipping IRQ registration\n",
			__func__);
		/* disable feature support if gpio was invalid */
		aw882xx->flags |= AW882XX_FLAG_SKIP_INTERRUPTS;
	}

	dev_set_drvdata(&i2c->dev, aw882xx);
	ret = sysfs_create_group(&i2c->dev.kobj, &aw882xx_attribute_group);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s error creating sysfs attr files\n",
			__func__);
		goto err_sysfs;
	}

	aw_cali_init(&aw882xx->cali);

	aw882xx_monitor_init(&aw882xx->monitor);

	aw882xx->fade_step = AW882XX_VOLUME_STEP_DB;
	aw882xx->aw882xx_pa_switch = AW882XX_ON_PA; /*can open*/
	aw882xx->is_power_on = AW882XX_PA_CLOSE_ST;
	aw_dev_dbg(aw882xx->dev, "%s: probe completed successfully!\n",
		__func__);

	return 0;


err_sysfs:
err_irq:
	aw_componet_codec_ops.aw_snd_soc_unregister_codec(&i2c->dev);

	return ret;
}

int aw882xx_i2c_remove(struct i2c_client *i2c)
{
	struct aw882xx *aw882xx = i2c_get_clientdata(i2c);

	aw_dev_err(aw882xx->dev, "%s: enter\n", __func__);

	aw_cali_deinit(&aw882xx->cali);
	aw882xx_monitor_deinit(&aw882xx->monitor);

	aw_componet_codec_ops.aw_snd_soc_unregister_codec(&i2c->dev);

	return 0;
}
#define AW882XX_I2C_NAME "aw882xx_smartpa"
static const struct i2c_device_id aw882xx_i2c_id[] = {
	{ AW882XX_I2C_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, aw882xx_i2c_id);

static struct of_device_id aw882xx_dt_match[] = {
	{ .compatible = "awinic,aw882xx_smartpa" },
	{ .compatible = "awinic,aw882xx_smartpa_l" },
	{ .compatible = "awinic,aw882xx_smartpa_r" },
	{ },
};

static struct i2c_driver aw882xx_i2c_driver = {
	.driver = {
		.name = AW882XX_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(aw882xx_dt_match),
	},
	.probe = aw882xx_i2c_probe,
	.remove = aw882xx_i2c_remove,
	.id_table = aw882xx_i2c_id,
};

EXPORT_SYMBOL(aw882xx_i2c_probe);
EXPORT_SYMBOL(aw882xx_i2c_remove);


static int __init aw882xx_i2c_init(void)
{
	int ret = -1;

	pr_err("%s: weichang  aw882xx driver version %s\n",
		__func__, AW882XX_DRIVER_VERSION);

	ret = i2c_add_driver(&aw882xx_i2c_driver);
	if (ret)
		pr_err("%s: fail to add aw882xx device into i2c\n", __func__);

	return ret;
}
module_init(aw882xx_i2c_init);


static void __exit aw882xx_i2c_exit(void)
{
	i2c_del_driver(&aw882xx_i2c_driver);
}
module_exit(aw882xx_i2c_exit);


MODULE_DESCRIPTION("ASoC AW882XX Smart PA Driver");
MODULE_LICENSE("GPL v2");
