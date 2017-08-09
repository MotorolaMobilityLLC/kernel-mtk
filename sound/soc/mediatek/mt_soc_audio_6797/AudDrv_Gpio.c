/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudDrv_Gpio.c
 *
 * Project:
 * --------
 *   MT6797  Audio Driver GPIO
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * George
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include "AudDrv_Gpio.h"
#ifndef CONFIG_PINCTRL_MT6797
#include <mt-plat/mt_gpio.h>
#endif

#ifdef CONFIG_PINCTRL_MT6797
struct pinctrl *pinctrlaud;

enum audio_system_gpio_type {
	GPIO_AUD_CLK_MOSI_OFF,
	GPIO_AUD_CLK_MOSI_ON,
	GPIO_AUD_DAT_MISO_OFF,
	GPIO_AUD_DAT_MISO_ON,
	GPIO_AUD_DAT_MOSI_OFF,
	GPIO_AUD_DAT_MOSI_ON,
	GPIO_VOW_DAT_MISO_OFF,
	GPIO_VOW_DAT_MISO_ON,
	GPIO_VOW_CLK_MISO_OFF,
	GPIO_VOW_CLK_MISO_ON,
	GPIO_ANC_DAT_MOSI_OFF,
	GPIO_ANC_DAT_MOSI_ON,
	GPIO_SMARTPA_MODE0,
	GPIO_SMARTPA_MODE1,
	GPIO_SMARTPA_MODE3,
	GPIO_TDM_MODE0,
	GPIO_TDM_MODE1,
/*	GPIO_I2S_MODE0,
	GPIO_I2S_MODE1,
	GPIO_EXTAMP_HIGH,
	GPIO_EXTAMP_LOW,
	GPIO_EXTAMP2_HIGH,
	GPIO_EXTAMP2_LOW,
	GPIO_RCVSPK_HIGH,
	GPIO_RCVSPK_LOW,*/
	GPIO_HPDEPOP_HIGH,
	GPIO_HPDEPOP_LOW,
	GPIO_NUM
};

struct audio_gpio_attr {
	const char *name;
	bool gpio_prepare;
	struct pinctrl_state *gpioctrl;
};

static struct audio_gpio_attr aud_gpios[GPIO_NUM] = {
	[GPIO_AUD_CLK_MOSI_OFF] = {"aud_clk_mosi_off", false, NULL},
	[GPIO_AUD_CLK_MOSI_ON] = {"aud_clk_mosi_on", false, NULL},
	[GPIO_AUD_DAT_MISO_OFF] = {"aud_dat_miso_off", false, NULL},
	[GPIO_AUD_DAT_MISO_ON] = {"aud_dat_miso_on", false, NULL},
	[GPIO_AUD_DAT_MOSI_OFF] = {"aud_dat_mosi_off", false, NULL},
	[GPIO_AUD_DAT_MOSI_ON] = {"aud_dat_mosi_on", false, NULL},
	[GPIO_VOW_DAT_MISO_OFF] = {"vow_dat_miso_off", false, NULL},
	[GPIO_VOW_DAT_MISO_ON] = {"vow_dat_miso_on", false, NULL},
	[GPIO_VOW_CLK_MISO_OFF] = {"vow_clk_miso_off", false, NULL},
	[GPIO_VOW_CLK_MISO_ON] = {"vow_clk_miso_on", false, NULL},
	[GPIO_ANC_DAT_MOSI_OFF] = {"anc_dat_mosi_off", false, NULL},
	[GPIO_ANC_DAT_MOSI_ON] = {"anc_dat_mosi_on", false, NULL},


	[GPIO_SMARTPA_MODE0] = {"aud_smartpa_mode0", false, NULL},
	[GPIO_SMARTPA_MODE1] = {"aud_smartpa_mode1", false, NULL},
	[GPIO_SMARTPA_MODE3] = {"aud_smartpa_mode3", false, NULL},
	[GPIO_TDM_MODE0] = {"aud_tdm_mode0", false, NULL},
	[GPIO_TDM_MODE1] = {"aud_tdm_mode1", false, NULL},

/*	[GPIO_I2S_MODE0] = {"audi2s1_mode0", false, NULL},
	[GPIO_I2S_MODE1] = {"audi2s1_mode1", false, NULL},
	[GPIO_EXTAMP_HIGH] = {"audextamp_high", false, NULL},
	[GPIO_EXTAMP_LOW] = {"audextamp_low", false, NULL},
	[GPIO_EXTAMP2_HIGH] = {"audextamp2_high", false, NULL},
	[GPIO_EXTAMP2_LOW] = {"audextamp2_low", false, NULL},
	[GPIO_RCVSPK_HIGH] = {"audcvspk_high", false, NULL},
	[GPIO_RCVSPK_LOW] = {"audcvspk_low", false, NULL},*/
	[GPIO_HPDEPOP_HIGH] = {"hpdepop-pullhigh", false, NULL},
	[GPIO_HPDEPOP_LOW] = {"hpdepop-pulllow", false, NULL},
};
#endif


void AudDrv_GPIO_probe(void *dev)
{
#ifdef CONFIG_PINCTRL_MT6797
	int ret;
	int i = 0;

	pr_warn("%s\n", __func__);

	pinctrlaud = devm_pinctrl_get(dev);
	if (IS_ERR(pinctrlaud)) {
		ret = PTR_ERR(pinctrlaud);
		pr_err("Cannot find pinctrlaud!\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(aud_gpios); i++) {
		aud_gpios[i].gpioctrl = pinctrl_lookup_state(pinctrlaud, aud_gpios[i].name);
		if (IS_ERR(aud_gpios[i].gpioctrl)) {
			ret = PTR_ERR(aud_gpios[i].gpioctrl);
			pr_err("%s pinctrl_lookup_state %s fail %d\n", __func__, aud_gpios[i].name,
			       ret);
		} else {
			aud_gpios[i].gpio_prepare = true;
		}
	}
#endif
}

static int AudDrv_GPIO_Select(enum audio_system_gpio_type _type)
{
	int ret = 0;

	if (_type < 0 || _type >= GPIO_NUM) {
		pr_err("%s(), error, invaild gpio type %d\n", __func__, _type);
		return -EINVAL;
	}

	if (!aud_gpios[_type].gpio_prepare) {
		pr_err("%s(), error, gpio type %d not prepared\n",
		       __func__, _type);
		BUG();
	}

	ret = pinctrl_select_state(pinctrlaud,
				   aud_gpios[_type].gpioctrl);
	if (ret)
		pr_err("%s(), error, can not set gpio type %d\n",
		       __func__, _type);

	return ret;
}

static int set_aud_clk_mosi(bool _enable)
{
/*
 * scp also need this gpio on mt6797,
 * don't switch gpio if they exist.
 */
#ifndef CONFIG_MTK_TINYSYS_SCP_SUPPORT
	static int aud_clk_mosi_counter;

	if (_enable) {
		aud_clk_mosi_counter++;
		if (aud_clk_mosi_counter == 1)
			return AudDrv_GPIO_Select(GPIO_AUD_CLK_MOSI_ON);
	} else {
		if (aud_clk_mosi_counter > 0) {
			aud_clk_mosi_counter--;
		} else {
			aud_clk_mosi_counter = 0;
			pr_warn("%s(), counter %d <= 0\n",
				__func__, aud_clk_mosi_counter);
		}

		if (aud_clk_mosi_counter == 0)
			return AudDrv_GPIO_Select(GPIO_AUD_CLK_MOSI_OFF);
	}
#endif
	return 0;
}

static int set_aud_dat_mosi(bool _enable)
{
	if (_enable)
		return AudDrv_GPIO_Select(GPIO_AUD_DAT_MOSI_ON);
	else
		return AudDrv_GPIO_Select(GPIO_AUD_DAT_MOSI_OFF);
}

static int set_aud_dat_miso(bool _enable, Soc_Aud_Digital_Block _usage)
{
	static bool adda_enable;
	static bool vow_enable;
	static bool anc_enable;

	switch (_usage) {
	case Soc_Aud_Digital_Block_ADDA_UL:
		adda_enable = _enable;
		break;
	case Soc_Aud_Digital_Block_ADDA_VOW:
		vow_enable = _enable;
		break;
	case Soc_Aud_Digital_Block_ADDA_ANC:
		anc_enable = _enable;
		break;
	default:
		return -EINVAL;
	}

	if (vow_enable)
		return AudDrv_GPIO_Select(GPIO_VOW_DAT_MISO_ON);
	else if (adda_enable || anc_enable)
		return AudDrv_GPIO_Select(GPIO_AUD_DAT_MISO_ON);
	else
		return AudDrv_GPIO_Select(GPIO_AUD_DAT_MISO_OFF);
}

static int set_vow_clk_miso(bool _enable)
{
	if (_enable)
		return AudDrv_GPIO_Select(GPIO_VOW_CLK_MISO_ON);
	else
		return AudDrv_GPIO_Select(GPIO_VOW_CLK_MISO_OFF);
}

static int set_anc_dat_mosi(bool _enable)
{
	if (_enable)
		return AudDrv_GPIO_Select(GPIO_ANC_DAT_MOSI_ON);
	else
		return AudDrv_GPIO_Select(GPIO_ANC_DAT_MOSI_OFF);
}

int AudDrv_GPIO_Request(bool _enable, Soc_Aud_Digital_Block _usage)
{
	switch (_usage) {
	case Soc_Aud_Digital_Block_ADDA_DL:
		set_aud_clk_mosi(_enable);
		set_aud_dat_mosi(_enable);
		break;
	case Soc_Aud_Digital_Block_ADDA_UL:
		set_aud_clk_mosi(_enable);
		set_aud_dat_miso(_enable, _usage);
		break;
	case Soc_Aud_Digital_Block_ADDA_VOW:
		set_vow_clk_miso(_enable);
		set_aud_dat_miso(_enable, _usage);
		break;
	case Soc_Aud_Digital_Block_ADDA_ANC:
		set_aud_dat_miso(_enable, _usage);
		set_anc_dat_mosi(_enable);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

int AudDrv_GPIO_SMARTPA_Select(int mode)
{
	int retval = 0;

#ifndef CONFIG_PINCTRL_MT6797
	switch (mode) {
	case 0:
		mt_set_gpio_mode(69 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_mode(70 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_mode(71 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_mode(72 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_mode(73 | 0x80000000, GPIO_MODE_00);
		break;
	case 1:
		mt_set_gpio_mode(69 | 0x80000000, GPIO_MODE_01);
		mt_set_gpio_mode(70 | 0x80000000, GPIO_MODE_01);
		mt_set_gpio_mode(71 | 0x80000000, GPIO_MODE_01);
		mt_set_gpio_mode(72 | 0x80000000, GPIO_MODE_01);
		mt_set_gpio_mode(73 | 0x80000000, GPIO_MODE_01);
		break;
	case 3:
		mt_set_gpio_mode(69 | 0x80000000, GPIO_MODE_03);
		mt_set_gpio_mode(70 | 0x80000000, GPIO_MODE_03);
		mt_set_gpio_mode(71 | 0x80000000, GPIO_MODE_03);
		mt_set_gpio_mode(72 | 0x80000000, GPIO_MODE_03);
		mt_set_gpio_mode(73 | 0x80000000, GPIO_MODE_03);
		break;
	default:
		pr_err("%s(), invalid mode = %d", __func__, mode);
		retval = -1;
	}
#else
	switch (mode) {
	case 0:
		AudDrv_GPIO_Select(GPIO_SMARTPA_MODE0);
		break;
	case 1:
		AudDrv_GPIO_Select(GPIO_SMARTPA_MODE1);
		break;
	case 3:
		AudDrv_GPIO_Select(GPIO_SMARTPA_MODE3);
		break;
	default:
		pr_err("%s(), invalid mode = %d", __func__, mode);
		retval = -1;
	}
#endif
	return retval;
}

int AudDrv_GPIO_TDM_Select(int mode)
{
	int retval = 0;

#ifndef CONFIG_PINCTRL_MT6797
	switch (mode) {
	case 0:
		mt_set_gpio_mode(135 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_mode(136 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_mode(138 | 0x80000000, GPIO_MODE_00);
		break;
	case 1:
		mt_set_gpio_mode(135 | 0x80000000, GPIO_MODE_01);
		mt_set_gpio_mode(136 | 0x80000000, GPIO_MODE_01);
		mt_set_gpio_mode(138 | 0x80000000, GPIO_MODE_01);
		break;
	default:
		pr_err("%s(), invalid mode = %d", __func__, mode);
		retval = -1;
}
#else
	switch (mode) {
	case 0:
		AudDrv_GPIO_Select(GPIO_TDM_MODE0);
		break;
	case 1:
		AudDrv_GPIO_Select(GPIO_TDM_MODE1);
		break;
	default:
		pr_err("%s(), invalid mode = %d", __func__, mode);
		retval = -1;
	}
#endif
	return retval;
}


int AudDrv_GPIO_I2S_Select(int bEnable)
{
	int retval = 0;

#if 0
	if (bEnable == 1) {
		if (aud_gpios[GPIO_I2S_MODE1].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_I2S_MODE1].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_I2S_MODE1] pins\n");
		}
	} else {
		if (aud_gpios[GPIO_I2S_MODE0].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_I2S_MODE0].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_I2S_MODE0] pins\n");
		}

	}
#endif
	return retval;
}

int AudDrv_GPIO_EXTAMP_Select(int bEnable, int mode)
{
	int retval = 0;

#if 0
	int extamp_mode;
	int i;

	if (bEnable == 1) {
		if (mode == 1)
			extamp_mode = 1;
		else if (mode == 2)
			extamp_mode = 2;
		else
			extamp_mode = 3;	/* default mode is 3 */

		if (aud_gpios[GPIO_EXTAMP_HIGH].gpio_prepare) {
			for (i = 0; i < extamp_mode; i++) {
				retval = pinctrl_select_state(pinctrlaud,
						aud_gpios[GPIO_EXTAMP_LOW].gpioctrl);
				if (retval)
					pr_err("could not set aud_gpios[GPIO_EXTAMP_LOW] pins\n");
				udelay(2);
				retval = pinctrl_select_state(pinctrlaud,
						aud_gpios[GPIO_EXTAMP_HIGH].gpioctrl);
				if (retval)
					pr_err("could not set aud_gpios[GPIO_EXTAMP_HIGH] pins\n");
				udelay(2);
			}
		}
	} else {
		if (aud_gpios[GPIO_EXTAMP_LOW].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_EXTAMP_LOW].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_EXTAMP_LOW] pins\n");
		}

	}
#endif
	return retval;
}

int AudDrv_GPIO_EXTAMP2_Select(int bEnable, int mode)
{
	int retval = 0;

#if 0
	int extamp_mode;
	int i;

	if (bEnable == 1) {
		if (mode == 1)
			extamp_mode = 1;
		else if (mode == 2)
			extamp_mode = 2;
		else
			extamp_mode = 3;	/* default mode is 3 */

		if (aud_gpios[GPIO_EXTAMP2_HIGH].gpio_prepare) {
			for (i = 0; i < extamp_mode; i++) {
				retval = pinctrl_select_state(pinctrlaud,
						aud_gpios[GPIO_EXTAMP2_LOW].gpioctrl);
				if (retval)
					pr_err("could not set aud_gpios[GPIO_EXTAMP2_LOW] pins\n");
				udelay(2);
				retval = pinctrl_select_state(pinctrlaud,
						aud_gpios[GPIO_EXTAMP2_HIGH].gpioctrl);
				if (retval)
					pr_err("could not set aud_gpios[GPIO_EXTAMP2_HIGH] pins\n");
				udelay(2);
			}
		}
	} else {
		if (aud_gpios[GPIO_EXTAMP2_LOW].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_EXTAMP2_LOW].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_EXTAMP2_LOW] pins\n");
		}

	}
#endif
	return retval;
}

int AudDrv_GPIO_RCVSPK_Select(int bEnable)
{
	int retval = 0;

#if 0
	if (bEnable == 1) {
		if (aud_gpios[GPIO_RCVSPK_HIGH].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_RCVSPK_HIGH].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_RCVSPK_HIGH] pins\n");
		}
	} else {
		if (aud_gpios[GPIO_RCVSPK_LOW].gpio_prepare) {
			retval =
			    pinctrl_select_state(pinctrlaud, aud_gpios[GPIO_RCVSPK_LOW].gpioctrl);
			if (retval)
				pr_err("could not set aud_gpios[GPIO_RCVSPK_LOW] pins\n");
		}

	}
#endif
	return retval;
}

int AudDrv_GPIO_HPDEPOP_Select(int bEnable)
{
	int retval = 0;

	if (bEnable == 1)
		AudDrv_GPIO_Select(GPIO_HPDEPOP_LOW);
	else
		AudDrv_GPIO_Select(GPIO_HPDEPOP_HIGH);

	return retval;
}
