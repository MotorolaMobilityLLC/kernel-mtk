/**************************************************
*
Copyright (c) 2016, Analogix Semiconductor, Inc.

PKG Ver  : V1.0

Filename :

Project  : ANX7625

Created  : 02 Oct. 2016

Devices  : ANX7625

Toolchain: Android

Description:

Revision History:

***************************************************/
/*
 * Copyright(c) 2014, Analogix Semiconductor. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include "anx7625_driver.h"
#include "anx7625_private_interface.h"
#include "anx7625_public_interface.h"
#include  "anx7625_display.h"
#include "display.h"
#include "Flash.h"
#include "display_platform.h"

#ifdef ANX7625_MTK_PLATFORM
#ifdef CONFIG_MACH_MT6799
#include "mt6336.h"
#endif
#include "mt-plat/upmu_common.h"
#endif
#ifdef CONFIG_USB_C_SWITCH
#include <typec.h>
#endif
#ifdef DYNAMIC_CONFIG_MIPI
#include "../anx_extern/msm_dba.h"
#include "../anx_extern/msm_dba_internal.h"
void anx7625_notify_clients(struct msm_dba_device_info *dev,
	enum msm_dba_callback_event event);
#endif

/* Use device tree structure data when defined "CONFIG_OF"  */
/* #define CONFIG_OF */



static int create_sysfs_interfaces(struct device *dev);

/* to access global platform data */
static struct anx7625_platform_data *g_pdata;

atomic_t anx7625_power_status;
unsigned char cable_connected;
unsigned char alert_arrived;
unsigned char vbus_en;

struct i2c_client *anx7625_client;

struct anx7625_platform_data {
	int gpio_p_on;
	int gpio_reset;
	int gpio_cbl_det;
#ifdef SUP_INT_VECTOR
	int gpio_intr_comm;
#endif
#ifdef SUP_VBUS_CTL
	int gpio_vbus_ctrl;
#endif
	spinlock_t lock;
};

struct anx7625_data {
	struct anx7625_platform_data *pdata;
	struct delayed_work work;
	struct workqueue_struct *workqueue;
	struct mutex lock;
	struct wake_lock anx7625_lock;
#ifdef DYNAMIC_CONFIG_MIPI
	struct msm_dba_device_info dev_info;
#endif
};

/* to access global platform data */
static struct anx7625_data *the_chip_anx7625;


unsigned char debug_on;
static unsigned char auto_start = 1; /* auto update OCM FW*/

unsigned char hpd_status;

static unsigned char default_dpi_config = 0xff; /*1:720p  3:1080p*/
static unsigned char default_dsi_config = RESOLUTION_1080P60_DSI;
static unsigned char default_audio_config; /*I2S 2 channel*/

static unsigned char  last_read_DevAddr = 0xff;

#ifdef ANX7625_MTK_PLATFORM
/*USB Driver wrapper*/
static struct usbtypc_anx7625 *g_exttypec;
struct delayed_work usb_work;

struct usbtypc_anx7625 {
	struct pinctrl *pinctrl;
	struct usbc_pin_ctrl *pin_cfg;
	struct device *pinctrl_dev;
	struct i2c_client *i2c_hd;
	struct typec_switch_data *host_driver;
	struct typec_switch_data *device_driver;
};

/* Stat */
#define DISABLE 0
#define ENABLE 1

/* DriverType */
#define DEVICE_TYPE 1
#define HOST_TYPE	2
#define DONT_CARE_TYPE 3

static void trigger_driver(struct work_struct *work)
{
	int type = 0;
	int stat = cable_connected;
	struct usbtypc_anx7625 *typec = g_exttypec;

	type = ((ReadReg(OCM_SLAVE_I2C_ADDR, SYSTEM_STSTUS) &
		DATA_ROLE) == 0x20) ? HOST_TYPE : DEVICE_TYPE;

	TRACE("trigger_driver: type:%d, stat:%d\n", type, stat);

	if (type == DEVICE_TYPE && typec->device_driver) {
		struct typec_switch_data *drv = typec->device_driver;

		if ((stat == DISABLE) && (drv->disable)
				&& (drv->on == ENABLE)) {
			drv->disable(drv->priv_data);
			drv->on = DISABLE;

			TRACE("Device Disabe\n");
		} else if ((stat == ENABLE) && (drv->enable)
				&& (drv->on == DISABLE)) {
			drv->enable(drv->priv_data);
			drv->on = ENABLE;

			TRACE("Device Enable\n");
		} else {
			TRACE("No device driver to enable\n");
		}
	} else if (type == HOST_TYPE && typec->host_driver) {
		struct typec_switch_data *drv = typec->host_driver;

		if ((stat == DISABLE) && (drv->disable)
				&& (drv->on == ENABLE)) {
			drv->disable(drv->priv_data);
			drv->on = DISABLE;

			TRACE("Host Disable\n");
		} else if ((stat == ENABLE) &&
				(drv->enable) && (drv->on == DISABLE)) {
			drv->enable(drv->priv_data);
			drv->on = ENABLE;

			TRACE("Host Enable\n");
		} else {
			TRACE("No device driver to enable\n");
		}
	} else {
		TRACE("wrong way\n");
	}
}

int register_typec_switch_callback(struct typec_switch_data *new_driver)
{
	TRACE("Register driver %s %d\n",
		new_driver->name, new_driver->type);

	if (!g_exttypec)
		g_exttypec = kzalloc(sizeof(struct usbtypc_anx7625),
				GFP_KERNEL);

	if (new_driver->type == DEVICE_TYPE) {
		g_exttypec->device_driver = new_driver;
		g_exttypec->device_driver->on = 0;
		return 0;
	}

	if (new_driver->type == HOST_TYPE) {
		g_exttypec->host_driver = new_driver;
		g_exttypec->host_driver->on = 0;
		return 0;
	}

	return -EPERM;
}
EXPORT_SYMBOL_GPL(register_typec_switch_callback);

int unregister_typec_switch_callback(struct typec_switch_data *new_driver)
{
	TRACE("Unregister driver %s %d\n",
		new_driver->name, new_driver->type);

	if ((new_driver->type == DEVICE_TYPE) &&
			(g_exttypec->device_driver == new_driver))
		g_exttypec->device_driver = NULL;

	if ((new_driver->type == HOST_TYPE) &&
			(g_exttypec->host_driver == new_driver))
		g_exttypec->host_driver = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(unregister_typec_switch_callback);
#endif

/* software workaround for silicon bug MIS2-124 */
static void Reg_Access_Conflict_Workaround(unsigned char DevAddr)
{
	unsigned char RegAddr;
	int ret = 0;

	if (DevAddr != last_read_DevAddr) {
		switch (DevAddr) {
		case  0x54:
		case  0x72:
		default:
			RegAddr = 0x00;
			break;

		case  0x58:
			RegAddr = 0x00;
			break;

		case  0x70:
			RegAddr = 0xD1;
			break;

		case  0x7A:
			RegAddr = 0x60;
			break;

		case  0x7E:
			RegAddr = 0x39;
			break;

		case  0x84:
			RegAddr = 0x7F;
			break;
		}


		anx7625_client->addr = (DevAddr >> 1);
		ret = i2c_smbus_write_byte_data(anx7625_client, RegAddr, 0x00);
		if (ret < 0) {
			pr_err("%s %s: failed to write i2c addr=%x\n:%x",
				LOG_TAG, __func__, DevAddr, RegAddr);

		}
		last_read_DevAddr = DevAddr;
	}

}



/* anx7625 power status, sync with interface and cable detection thread */

inline unsigned char ReadReg(unsigned char DevAddr, unsigned char RegAddr)
{
	int ret = 0;

	Reg_Access_Conflict_Workaround(DevAddr);

	anx7625_client->addr = DevAddr >> 1;
	ret = i2c_smbus_read_byte_data(anx7625_client, RegAddr);
	if (ret < 0) {
		pr_err("%s %s: failed to read i2c addr=%x:%x\n", LOG_TAG,
			__func__, DevAddr, RegAddr);
	}
	return (uint8_t) ret;

}
unsigned char GetRegVal(unsigned char DevAddr, unsigned char RegAddr)
{
	return ReadReg(DevAddr, RegAddr);
}

int Read_Reg(uint8_t slave_addr, uint8_t offset, uint8_t *buf)
{
	int ret = 0;

	Reg_Access_Conflict_Workaround(slave_addr);

	anx7625_client->addr = (slave_addr >> 1);
	ret = i2c_smbus_read_byte_data(anx7625_client, offset);
	if (ret < 0) {
		pr_err("%s %s: failed to read i2c addr=%x:%x\n", LOG_TAG,
			__func__, slave_addr, offset);
		return ret;
	}
	*buf = (uint8_t) ret;

	return 0;
}


inline int ReadBlockReg(unsigned char DevAddr, u8 RegAddr, u8 len, u8 *dat)
{
	int ret = 0;

	Reg_Access_Conflict_Workaround(DevAddr);

	anx7625_client->addr = (DevAddr >> 1);
	ret = i2c_smbus_read_i2c_block_data(anx7625_client, RegAddr, len, dat);
	if (ret < 0) {
		pr_err("%s %s: failed to read i2c block addr=%x:%x\n", LOG_TAG,
			__func__, DevAddr, RegAddr);
		return -EPERM;
	}

	return (int)ret;
}


inline int WriteBlockReg(unsigned char DevAddr, u8 RegAddr, u8 len,
	const u8 *dat)
{
	int ret = 0;

	Reg_Access_Conflict_Workaround(DevAddr);

	anx7625_client->addr = (DevAddr >> 1);
	ret = i2c_smbus_write_i2c_block_data(anx7625_client, RegAddr, len, dat);
	if (ret < 0) {
		pr_err("%s %s: failed to write i2c block addr=%x:%x\n", LOG_TAG,
			__func__, DevAddr, RegAddr);
		return -EPERM;
	}

	return (int)ret;
}

inline void WriteReg(unsigned char DevAddr, unsigned char RegAddr,
	unsigned char RegVal)
{
	int ret = 0;

	Reg_Access_Conflict_Workaround(DevAddr);

	anx7625_client->addr = (DevAddr >> 1);
	ret = i2c_smbus_write_byte_data(anx7625_client, RegAddr, RegVal);
	if (ret < 0) {
		pr_err("%s %s: failed to write i2c addr=%x\n:%x", LOG_TAG,
			__func__, DevAddr, RegAddr);
	}
}

void MI2_power_on(void)
{
#ifndef ANX7625_MTK_PLATFORM

#ifdef CONFIG_OF
	struct anx7625_platform_data *pdata = g_pdata;
#else
	struct anx7625_platform_data *pdata = anx7625_client->
		dev.platform_data;
#endif

	/*power on pin enable */
	gpio_set_value(pdata->gpio_p_on, 1);
	usleep_range(10000, 11000);
	/*power reset pin enable */
	gpio_set_value(pdata->gpio_reset, 1);
	usleep_range(10000, 11000);
#else
	mhl_power_ctrl(1);
	reset_mhl_board(10, 10, 1);
#endif

	TRACE("%s %s: Anx7625 power on !\n", LOG_TAG, __func__);
}

void anx7625_hardware_reset(int enable)
{
#ifndef ANX7625_MTK_PLATFORM

#ifdef CONFIG_OF
	struct anx7625_platform_data *pdata = g_pdata;
#else
	struct anx7625_platform_data *pdata = anx7625_client->
		dev.platform_data;
#endif
	gpio_set_value(pdata->gpio_reset, enable);
#else
	reset_mhl_board(10, 10, enable);
#endif

}


void anx7625_power_standby(void)
{
#ifndef ANX7625_MTK_PLATFORM

#ifdef CONFIG_OF
	struct anx7625_platform_data *pdata = g_pdata;
#else
	struct anx7625_platform_data *pdata = anx7625_client->dev.platform_data;
#endif

	gpio_set_value(pdata->gpio_reset, 0);
	usleep_range(1000, 1100);
	gpio_set_value(pdata->gpio_p_on, 0);
	usleep_range(1000, 1100);
#else
	reset_mhl_board(1, 1, 0);
	mhl_power_ctrl(0);
#endif

	TRACE("%s %s: anx7625 power down\n", LOG_TAG, __func__);
}

/*configure DPR toggle*/
void ANX7625_DRP_Enable(void)
{
	/*reset main OCM*/
	WriteReg(RX_P0, OCM_DEBUG_REG_8, 1<<STOP_MAIN_OCM);
	/*config toggle.*/
	WriteReg(TCPC_INTERFACE, TCPC_ROLE_CONTROL, 0x45);
	WriteReg(TCPC_INTERFACE, TCPC_COMMAND, 0x99);
	WriteReg(TCPC_INTERFACE, ANALOG_CTRL_1, 0xA0);
	WriteReg(TCPC_INTERFACE, ANALOG_CTRL_1, 0xE0);

	TRACE("Enable DRP!");
}
/* basic configurations of ANX7625 */
void ANX7625_config(void)
{
	WriteReg(RX_P0, XTAL_FRQ_SEL, XTAL_FRQ_27M);

}


BYTE ANX7625_Chip_Located(void)
{
	BYTE c1, c2;

	MI2_power_on();
	Read_Reg(TCPC_INTERFACE, PRODUCT_ID_L, &c1);
	Read_Reg(TCPC_INTERFACE, PRODUCT_ID_H, &c2);
	anx7625_power_standby();
	if ((c1 == 0x25) && (c2 == 0x76)) {
		TRACE("ANX7625 is detected!\n");
		return 1;
	}
	TRACE("No ANX7625 found!\n");
	return 0;
}


#define FLASH_LOAD_STA 0x05
#define FLASH_LOAD_STA_CHK	(1<<7)

void anx7625_hardware_poweron(void)
{

	int retry_count, i;


	for (retry_count = 0; retry_count < 3; retry_count++) {

		MI2_power_on();

		ANX7625_config();


		for (i = 0; i < OCM_LOADING_TIME; i++) {
			/*Interface work? */
			if ((ReadReg(OCM_SLAVE_I2C_ADDR, FLASH_LOAD_STA)&
				FLASH_LOAD_STA_CHK) == FLASH_LOAD_STA_CHK) {
				TRACE("%s %s: interface initialization\n",
					LOG_TAG, __func__);


				chip_register_init();
				send_initialized_setting();
				mute_video_flag = 0;

	TRACE("Firmware version %02x%02x,Driver version %s\n",
		ReadReg(OCM_SLAVE_I2C_ADDR, OCM_FW_VERSION),
		ReadReg(OCM_SLAVE_I2C_ADDR, OCM_FW_REVERSION),
		ANX7625_DRV_VERSION);
				return;
			}
			usleep_range(1000, 1100);
		}
		anx7625_power_standby();

	}

}

#define RG_EN_OTG	 (0x1<<0x3)

void anx7625_vbus_control(bool on)
{
#ifdef SUP_VBUS_CTL
#ifdef ANX7625_MTK_PLATFORM
#ifdef CONFIG_MACH_MT6799
	if (vbus_en == on)
		goto skip;

	mt6336_ctrl_enable(mt6336_ctrl);

	/*
	 *  From Script_20160823.xlsx provided by Lynch Lin
	 */
	if (on) {
		uint8_t tmp;
		/*
		 * ==Initial Setting==
		 * WR	56	411	51	OTG CV and IOLP setting
		 *
		 * ==Enable OTG==
		 * 1. Check 0x612[4] (DA_QI_FLASH_MODE)
			and 0x0613[1] (DA_QI_VBUS_PLUGIN)
		 *
		 * 2. If they are both low, apply below settings
		 * 3. If any of them is high, do nothing
			(chip does not support OTG mode
			while under CHR or Flash mode)"
		 *
		 * WR	57	519	0D
		 * WR	57	520	00
		 * WR	57	55A	01
		 * WR	56	455	00
		 * WR	55	3C9	00
		 *
		 * WR	55	3CF	00
		 * WR	57	553	14
		 * WR	57	55F	E6
		 * WR	57	53D	47
		 * WR	57	529	8E
		 *
		 * WR	57	560	0C
		 * WR	56	40F	04
		 *
		 * WR	56	400	[3]=1'b1	Enable OTG
		 */
		mt6336_set_register_value(0x411, 0x55);

		tmp = mt6336_get_register_value(0x612);
		if (tmp & (0x1<<4)) {
			pr_err("At Flash mode, Can not turn on OTG\n");
			return;
		}

		tmp = mt6336_get_register_value(0x613);
		if (tmp & (0x1<<1)) {
			pr_err("VBUS present, Can not turn on OTG\n");
			/*return;*/
			goto skip;
		}

		mt6336_set_register_value(0x519, 0x0D);
		mt6336_set_register_value(0x520, 0x00);
		mt6336_set_register_value(0x55A, 0x01);
		mt6336_set_register_value(0x455, 0x00);
		mt6336_set_register_value(0x3C9, 0x00);

		mt6336_set_register_value(0x3CF, 0x00);
		mt6336_set_register_value(0x553, 0x14);
		mt6336_set_register_value(0x55F, 0xE6);
		mt6336_set_register_value(0x53D, 0x47);
		mt6336_set_register_value(0x529, 0x8E);

		mt6336_set_register_value(0x560, 0x0C);
		mt6336_set_register_value(0x40F, 0x04);

		tmp = mt6336_get_register_value(0x400);
		mt6336_set_register_value(0x400, (tmp | RG_EN_OTG));
	} else {
		uint8_t tmp;
		/*
		 * ==Disable OTG==
		 *
		 * WR	57	52A	88
		 * WR	57	553	14
		 * WR	57	519	3F
		 * WR	57	51E	02
		 *
		 * WR	57	520	04
		 * WR	57	55A	00
		 * WR	56	455	01
		 * WR	55	3C9	10
		 *
		 * WR	55	3CF	03
		 * WR	57	5AF	02
		 * WR	58	64E	02
		 * WR	56	402	03
		 *
		 * WR	57	529	88
		 *
		 * WR	56	400	[3]=1'b0	Disable OTG
		*/
		mt6336_set_register_value(0x52A, 0x88);
		mt6336_set_register_value(0x553, 0x14);
		mt6336_set_register_value(0x519, 0x3F);
		mt6336_set_register_value(0x51E, 0x02);

		mt6336_set_register_value(0x520, 0x04);
		mt6336_set_register_value(0x55A, 0x00);
		mt6336_set_register_value(0x455, 0x01);
		mt6336_set_register_value(0x3C9, 0x10);

		mt6336_set_register_value(0x3CF, 0x03);
		mt6336_set_register_value(0x5AF, 0x02);
		mt6336_set_register_value(0x64E, 0x02);
		mt6336_set_register_value(0x402, 0x03);

		mt6336_set_register_value(0x529, 0x88);

		tmp = mt6336_get_register_value(0x400);
		mt6336_set_register_value(0x400, (tmp & (~RG_EN_OTG)));
	}

	mt6336_ctrl_disable(mt6336_ctrl);

skip:

	if (vbus_en != on)
		pr_err("VBUS %s", (on ? "ON" : "OFF"));

	vbus_en = (on ? 1 : 0);


#elif defined CONFIG_USB_C_SWITCH
	struct usbtypc_anx7625 *typec = g_exttypec;
	struct typec_switch_data *drv = typec->host_driver;

	if (!drv || !(drv->vbus_enable) || !(drv->vbus_disable)) {
		pr_err("no function callback for VBUS");
		return;
	}

	pr_info("VBUS before %s, after %s",
			(vbus_en ? "ON" : "OFF"),
			(on ? "ON" : "OFF"));

	if (vbus_en == on)
		return;

	if (on)
		drv->vbus_enable(drv->priv_data);
	else
		drv->vbus_disable(drv->priv_data);

	vbus_en = (on ? 1 : 0);
#else /* CONFIG_USB_C_SWITCH end */

#ifdef CONFIG_OF
	struct anx7625_platform_data *pdata = g_pdata;
#else
	struct anx7625_platform_data *pdata = anx7625_client->
		dev.platform_data;
#endif

	if (on)
		gpio_set_value(pdata->gpio_vbus_ctrl, ENABLE_VBUS_OUTPUT);
	else
		gpio_set_value(pdata->gpio_vbus_ctrl, DISABLE_VBUS_OUTPUT);

#endif
#endif
#endif
}

static void anx7625_free_gpio(struct anx7625_data *platform)
{
#ifdef ANX7625_MTK_PLATFORM
#else
	gpio_free(platform->pdata->gpio_cbl_det);
	gpio_free(platform->pdata->gpio_reset);
	gpio_free(platform->pdata->gpio_p_on);
#ifdef SUP_INT_VECTOR
	gpio_free(platform->pdata->gpio_intr_comm);
#endif
#ifdef SUP_VBUS_CTL
	gpio_free(platform->pdata->gpio_vbus_ctrl);
#endif
#endif
}

static int anx7625_init_gpio(struct anx7625_data *platform)
{
#ifdef ANX7625_MTK_PLATFORM
#else
	int ret = 0;

	TRACE("%s %s: anx7625 init gpio\n", LOG_TAG, __func__);
	/*  gpio for chip power down  */
	ret = gpio_request(platform->pdata->gpio_p_on,
		"anx7625_p_on_ctl");
	if (ret) {
		pr_err("%s : failed to request gpio %d\n", __func__,
			platform->pdata->gpio_p_on);
		goto err0;
	}
	gpio_direction_output(platform->pdata->gpio_p_on, 0);
	/*  gpio for chip reset  */
	ret = gpio_request(platform->pdata->gpio_reset,
		"anx7625_reset_n");
	if (ret) {
		pr_err("%s : failed to request gpio %d\n", __func__,
			platform->pdata->gpio_reset);
		goto err1;
	}
	gpio_direction_output(platform->pdata->gpio_reset, 0);

	/*  gpio for cable detect  */
	ret = gpio_request(platform->pdata->gpio_cbl_det,
		"anx7625_cbl_det");
	if (ret) {
		pr_err("%s : failed to request gpio %d\n", __func__,
			platform->pdata->gpio_cbl_det);
		goto err2;
	}
	gpio_direction_input(platform->pdata->gpio_cbl_det);

#ifdef SUP_INT_VECTOR
	/*  gpio for chip interface communaction */
	ret = gpio_request(platform->pdata->gpio_intr_comm,
		"anx7625_intr_comm");
	if (ret) {
		pr_err("%s : failed to request gpio %d\n", __func__,
			platform->pdata->gpio_intr_comm);
		goto err3;
	}
	gpio_direction_input(platform->pdata->gpio_intr_comm);
#endif

#ifdef SUP_VBUS_CTL
	/*  gpio for vbus control  */
	ret = gpio_request(platform->pdata->gpio_vbus_ctrl,
		"anx7625_vbus_ctrl");
	if (ret) {
		pr_err("%s : failed to request gpio %d\n", __func__,
			platform->pdata->gpio_vbus_ctrl);
		goto err4;
	}
	gpio_direction_output(platform->pdata->gpio_vbus_ctrl,
		DISABLE_VBUS_OUTPUT);
#endif

	goto out;

#ifdef SUP_VBUS_CTL
err4:
	gpio_free(platform->pdata->gpio_vbus_ctrl);
#endif
#ifdef SUP_INT_VECTOR
err3:
	gpio_free(platform->pdata->gpio_intr_comm);
#endif
err2:
	gpio_free(platform->pdata->gpio_cbl_det);
err1:
	gpio_free(platform->pdata->gpio_reset);
err0:
	gpio_free(platform->pdata->gpio_p_on);

	return 1;
out:
#endif
	return 0;
}



void anx7625_main_process(void)
{
#ifdef ANX7625_MTK_PLATFORM
#else
	struct anx7625_data *platform = the_chip_anx7625;
#endif

#ifdef DYNAMIC_CONFIG_MIPI
	struct anx7625_data *td;

	td = the_chip_anx7625;
#endif

	TRACE("%s %s:cable_connected=%d power_status=%d\n",
		LOG_TAG, __func__, cable_connected,
		(unsigned int)atomic_read(&anx7625_power_status));


	/* do main loop, do what you want to do */
	if (auto_start) {
		auto_start = 0;
		mute_video_flag = 0;
		if (ANX7625_Chip_Located() == 0) {
			debug_on = 1;
			return;
		}
#ifdef SUP_VBUS_CTL
#ifdef ANX7625_MTK_PLATFORM

		/*Disable VBUS supply.*/
		vbus_en = 1;
		anx7625_vbus_control(0);
#endif
#endif

#if AUTO_UPDATE_OCM_FW
		burnhexauto();
#endif

		MI2_power_on();
		ANX7625_DRP_Enable();
		usleep_range(1000, 1100);
		anx7625_power_standby();

#ifdef ANX7625_MTK_PLATFORM
		Unmask_Slimport_Intr();
		Unmask_Slimport_Comm_Intr();
#endif
		/*dongle already inserted, trigger cable-det isr to check. */
		anx7625_cbl_det_isr(1, the_chip_anx7625);
		return;

	}

	if (atomic_read(&anx7625_power_status) == 0) {
		if (cable_connected == 1) {
			atomic_set(&anx7625_power_status, 1);
			anx7625_hardware_poweron();
			return;
		}
	} else {
		if (cable_connected == 0) {
			atomic_set(&anx7625_power_status, 0);
#ifdef SUP_VBUS_CTL
#ifdef ANX7625_MTK_PLATFORM
			/*Disable VBUS supply.*/
			anx7625_vbus_control(0);
			/*Disable USB driver*/
			schedule_delayed_work(&usb_work, 0);
#else
			gpio_set_value(platform->pdata->gpio_vbus_ctrl,
				DISABLE_VBUS_OUTPUT);
#endif
#endif


			if (hpd_status == 1)
				anx7625_stop_dp_work();

			ANX7625_DRP_Enable();
			usleep_range(1000, 1100);
			clear_sys_sta_bak();
			mute_video_flag = 0;
			anx7625_power_standby();
			return;
		}


		if (alert_arrived)
			anx7625_handle_intr_comm();

	}
}

static void anx7625_work_func(struct work_struct *work)
{
	struct anx7625_data *td = container_of(work, struct anx7625_data,
						work.work);

	mutex_lock(&td->lock);
	anx7625_main_process();
	mutex_unlock(&td->lock);

}



void anx7625_restart_work(int workqueu_timer)
{
	struct anx7625_data *td;

	td = the_chip_anx7625;
	if (td != NULL) {
		queue_delayed_work(td->workqueue, &td->work,
				msecs_to_jiffies(workqueu_timer));
	}

}


void anx7625_start_dp(void)
{


	/* hpd changed */
	TRACE("anx7625_start_dp: mute_flag: %d\n",
		(unsigned int)mute_video_flag);


	hpd_status = 1;
	DP_Process_Start();

#ifdef ANX7625_MTK_PLATFORM
#else

	if (delay_tab_id == 0) {
		if (default_dpi_config < 0x20)
			command_DPI_Configuration(default_dpi_config);
		else if (default_dsi_config < 0x20)
			command_DSI_Configuration(default_dsi_config);

		if (default_audio_config < 0x20)
			API_Configure_Audio_Input(default_audio_config);
	}
#endif

#ifdef DYNAMIC_CONFIG_MIPI
	/* Notify DBA framework connect event */
	if (td != NULL)
		anx7625_notify_clients(&td->dev_info,
						MSM_DBA_CB_HPD_CONNECT);
#endif


}



void anx7625_stop_dp_work(void)
{

#ifdef DYNAMIC_CONFIG_MIPI
	struct anx7625_data *td;

	td = the_chip_anx7625;
	/* Notify DBA framework disconnect event */
	if (td != NULL)
		anx7625_notify_clients(&td->dev_info,
						MSM_DBA_CB_HPD_DISCONNECT);
#endif

	/* hpd changed */
	TRACE("anx7625_stop_dp_work: mute_flag: %d\n",
		(unsigned int)mute_video_flag);

	DP_Process_Stop();
	hpd_status = 0;
	mute_video_flag = 0;

#ifdef ANX7625_MTK_PLATFORM
#else
	delay_tab_id = 0;
#endif

}


#ifdef CABLE_DET_PIN_HAS_GLITCH
static unsigned char confirmed_cable_det(void *data)
{
#ifdef ANX7625_MTK_PLATFORM
#else
	struct anx7625_data *platform = data;
#endif
	unsigned int count = 10;
	unsigned int cable_det_count = 0;
	u8 val = 0;

	do {
#ifndef ANX7625_MTK_PLATFORM
		val = gpio_get_value(platform->pdata->gpio_cbl_det);
#else
		val = confirm_mhl_irq_level();
#endif
		if (val == DONGLE_CABLE_INSERT)
			cable_det_count++;
			usleep_range(1000, 1100);
	} while (count--);

	if (cable_det_count > 7)
		return 1;
	else if (cable_det_count < 2)
		return 0;
	else
		return atomic_read(&anx7625_power_status);
}
#endif

irqreturn_t anx7625_cbl_det_isr(int irq, void *data)
{
#ifdef ANX7625_MTK_PLATFORM
#else
	struct anx7625_data *platform = data;

#endif
	if (debug_on)
		return IRQ_NONE;

#ifdef CABLE_DET_PIN_HAS_GLITCH
	cable_connected = confirmed_cable_det((void *)platform);
#else
#ifndef ANX7625_MTK_PLATFORM
	cable_connected = gpio_get_value(platform->pdata->
		gpio_cbl_det);
#else
	cable_connected = confirm_mhl_irq_level();
#endif
#endif

	TRACE("%s %s : cable plug pin status %d\n", LOG_TAG,
		__func__, cable_connected);


	if (cable_connected == DONGLE_CABLE_INSERT) {
		if (atomic_read(&anx7625_power_status) == 1)
			return IRQ_HANDLED;
		cable_connected = 1;
		anx7625_restart_work(1);
	} else {
		if (atomic_read(&anx7625_power_status) == 0)
			return IRQ_HANDLED;
		cable_connected = 0;
		anx7625_restart_work(1);
	}

	return IRQ_HANDLED;
}

#ifdef SUP_INT_VECTOR
irqreturn_t anx7625_intr_comm_isr(int irq, void *data)
{

	if (atomic_read(&anx7625_power_status) != 1)
		return IRQ_NONE;

	alert_arrived = 1;
	anx7625_restart_work(1);
	return IRQ_HANDLED;

}
void anx7625_handle_intr_comm(void)
{
	unsigned char c;

	c = ReadReg(TCPC_INTERFACE, INTR_ALERT_1);

	TRACE("%s %s : ======I=====alert=%02x\n",
		LOG_TAG, __func__, (uint)c);

	if (c & INTR_SOFTWARE_INT)
		handle_intr_vector();

	if (c & INTR_RECEIVED_MSG)
		/*Received interface message*/
		handle_msg_rcv_intr();

#ifdef ANX7625_MTK_PLATFORM
	if (cable_connected)
		schedule_delayed_work(&usb_work, 0);
#endif
	while (ReadReg(OCM_SLAVE_I2C_ADDR,
		INTERFACE_CHANGE_INT) != 0)
		handle_intr_vector();

	WriteReg(TCPC_INTERFACE, INTR_ALERT_1, 0xFF);

#ifdef ANX7625_MTK_PLATFORM
	if (confirm_mhl_comm_irq_level() == 0) {
#else
	if ((gpio_get_value(the_chip_anx7625->pdata->gpio_intr_comm)) == 0) {
#endif
		alert_arrived = 1;
		anx7625_restart_work(1);
		TRACE("%s %s : comm isr pin still low, re-enter\n",
			LOG_TAG, __func__);
	} else {
		alert_arrived = 0;
		TRACE("%s %s : comm isr pin cleared\n",
			LOG_TAG, __func__);
	}

}
#endif

#ifdef CONFIG_OF
static int anx7625_parse_dt(struct device *dev,
	struct anx7625_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	pdata->gpio_p_on =
		of_get_named_gpio_flags(np, "analogix,p-on-gpio",
		0, NULL);

	pdata->gpio_reset =
		of_get_named_gpio_flags(np, "analogix,reset-gpio",
		0, NULL);

	pdata->gpio_cbl_det =
		of_get_named_gpio_flags(np, "analogix,cbl-det-gpio",
		0, NULL);

#ifdef SUP_VBUS_CTL
	/*reuse previous unless gpio(v33_ctrl) for vbus control*/
	pdata->gpio_vbus_ctrl =
		of_get_named_gpio_flags(np, "analogix,v33-ctrl-gpio",
		0, NULL);
#endif
#ifdef SUP_INT_VECTOR
	pdata->gpio_intr_comm =
		of_get_named_gpio_flags(np, "analogix,intr-comm-gpio",
		0, NULL);
#endif
	TRACE("%s gpio p_on : %d, reset : %d,  gpio_cbl_det %d\n",
		LOG_TAG, pdata->gpio_p_on,
		pdata->gpio_reset, pdata->gpio_cbl_det);

	return 0;
}
#else
static int anx7625_parse_dt(struct device *dev,
	struct anx7625_platform_data *pdata)
{
	return -ENODEV;
}
#endif

#ifdef DYNAMIC_CONFIG_MIPI

static int anx7625_register_dba(struct anx7625_data *pdata)
{
	struct msm_dba_ops *client_ops;
	struct msm_dba_device_ops *dev_ops;

	if (!pdata)
		return -EINVAL;

	client_ops = &pdata->dev_info.client_ops;
	dev_ops = &pdata->dev_info.dev_ops;

	client_ops->power_on		= NULL;
	client_ops->video_on		= anx7625_mipi_timing_setting;
	client_ops->configure_audio = NULL;
	client_ops->hdcp_enable	 = NULL;
	client_ops->hdmi_cec_on	 = NULL;
	client_ops->hdmi_cec_write  = NULL;
	client_ops->hdmi_cec_read   = NULL;
	client_ops->get_edid_size   = NULL;
	client_ops->get_raw_edid	= anx7625_get_raw_edid;
	client_ops->check_hpd		= NULL;

	strlcpy(pdata->dev_info.chip_name, "anx7625",
			sizeof(pdata->dev_info.chip_name));

	pdata->dev_info.instance_id = 0;

	mutex_init(&pdata->dev_info.dev_mutex);

	INIT_LIST_HEAD(&pdata->dev_info.client_list);

	return msm_dba_add_probed_device(&pdata->dev_info);
}


void anx7625_notify_clients(struct msm_dba_device_info *dev,
		enum msm_dba_callback_event event)
{
	struct msm_dba_client_info *c;
	struct list_head *pos = NULL;

	TRACE("%s++\n", __func__);

	if (!dev) {
		pr_err("%s: invalid input\n", __func__);
		return;
	}

	list_for_each(pos, &dev->client_list) {
		c = list_entry(pos, struct msm_dba_client_info, list);

		TRACE("%s:notifying event %d to client %s\n", __func__,
			event, c->client_name);

		if (c && c->cb)
			c->cb(c->cb_data, event);
	}

	TRACE("%s--\n", __func__);
}

#endif

static int anx7625_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{

	struct anx7625_data *platform;
	struct anx7625_platform_data *pdata;
	int ret = 0;

#ifdef ANX7625_MTK_PLATFORM
	struct usbtypc_anx7625 *typec;
#else
	int cbl_det_irq = 0;
#endif

	TRACE("%s:\n", __func__);

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_I2C_BLOCK)) {
		pr_err("%s:anx7625's i2c bus doesn't support\n", __func__);
		ret = -ENODEV;
		goto exit;
	}

	platform = kzalloc(sizeof(struct anx7625_data), GFP_KERNEL);
	if (!platform) {
		/*pr_err("%s: failed to allocate driver data\n", __func__);*/
		ret = -ENOMEM;
		goto exit;
	}

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
					sizeof(struct anx7625_platform_data),
					GFP_KERNEL);
		if (!pdata) {
			pr_err("%s: Failed to allocate memory\n", __func__);
			return -ENOMEM;
		}

		client->dev.platform_data = pdata;

		/* device tree parsing function call */
		ret = anx7625_parse_dt(&client->dev, pdata);
		if (ret != 0)	/* if occurs error */
			goto err0;

		platform->pdata = pdata;
	} else {
		platform->pdata = client->dev.platform_data;
	}

	/* to access global platform data */
	g_pdata = platform->pdata;
	the_chip_anx7625 = platform;
	anx7625_client = client;
	anx7625_client->addr = (OCM_SLAVE_I2C_ADDR1 >> 1);

#ifdef ANX7625_MTK_PLATFORM
	INIT_DELAYED_WORK(&usb_work, trigger_driver);

	if (!g_exttypec) {
		typec = kzalloc(sizeof(struct usbtypc_anx7625), GFP_KERNEL);
		g_exttypec = typec;
	} else {
		typec = g_exttypec;
	}
#endif

	atomic_set(&anx7625_power_status, 0);

	mutex_init(&platform->lock);

	if (!platform->pdata) {
		ret = -EINVAL;
		goto err0;
	}
#ifdef ANX7625_MTK_PLATFORM
	register_slimport_eint();
	register_slimport_comm_eint();
#endif

	ret = anx7625_init_gpio(platform);
	if (ret) {
		pr_err("%s: failed to initialize gpio\n", __func__);
		goto err0;
	}

	INIT_DELAYED_WORK(&platform->work, anx7625_work_func);


	platform->workqueue =
		create_singlethread_workqueue("anx7625_work");
	if (platform->workqueue == NULL) {
		pr_err("%s: failed to create work queue\n", __func__);
		ret = -ENOMEM;
		goto err1;
	}
#ifdef ANX7625_MTK_PLATFORM
#else
	cbl_det_irq = gpio_to_irq(platform->pdata->gpio_cbl_det);
	if (cbl_det_irq < 0) {
		pr_err("%s : failed to get gpio irq\n", __func__);
		goto err1;
	}

	wake_lock_init(&platform->anx7625_lock, WAKE_LOCK_SUSPEND,
		"anx7625_wake_lock");

	ret = request_threaded_irq(cbl_det_irq, NULL, anx7625_cbl_det_isr,
				IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING
				| IRQF_ONESHOT, "anx7625-cbl-det", platform);
	if (ret < 0) {
		pr_err("%s : failed to request irq\n", __func__);
		goto err3;
	}

	ret = irq_set_irq_wake(cbl_det_irq, 1);
	if (ret < 0) {
		pr_err("%s : Request irq for cable detect", __func__);
		pr_err("interrupt wake set fail\n");
		goto err4;
	}

	ret = enable_irq_wake(cbl_det_irq);
	if (ret < 0) {
		pr_err("%s : Enable irq for cable detect", __func__);
		pr_err("interrupt wake enable fail\n");
		goto err4;
	}
#ifdef SUP_INT_VECTOR
	client->irq = gpio_to_irq(platform->pdata->gpio_intr_comm);
	if (client->irq < 0) {
		pr_err("%s : failed to get anx7625 gpio comm irq\n", __func__);
		goto err3;
	}

	ret = request_threaded_irq(client->irq, NULL, anx7625_intr_comm_isr,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"anx7625-intr-comm", platform);

	if (ret < 0) {
		pr_err("%s : failed to request interface irq\n", __func__);
		goto err4;
	}

	ret = irq_set_irq_wake(client->irq, 1);
	if (ret < 0) {
		pr_err("%s : Request irq for interface communaction", __func__);
		goto err4;
	}

	ret = enable_irq_wake(client->irq);
	if (ret < 0) {
		pr_err("%s : Enable irq for interface communaction", __func__);
		goto err4;
	}
#endif
#endif

	ret = create_sysfs_interfaces(&client->dev);
	if (ret < 0) {
		pr_err("%s : sysfs register failed", __func__);
		goto err4;
	}

	/*add work function*/
	queue_delayed_work(platform->workqueue, &platform->work,
		msecs_to_jiffies(100));


#ifdef DYNAMIC_CONFIG_MIPI

	/* Register msm dba device */
	ret = anx7625_register_dba(platform);
	if (ret) {
		pr_err("%s: Error registering with DBA %d\n",
			__func__, ret);
	}

#endif

	TRACE("anx7625_i2c_probe successfully %s %s end\n",
		LOG_TAG, __func__);
	goto exit;

err4:
	free_irq(client->irq, platform);
#ifdef ANX7625_MTK_PLATFORM
#else
err3:
	free_irq(cbl_det_irq, platform);
#endif
err1:
	anx7625_free_gpio(platform);
	destroy_workqueue(platform->workqueue);
err0:
	anx7625_client = NULL;
	kfree(platform);
exit:
	return ret;
}

static int anx7625_i2c_remove(struct i2c_client *client)
{
	struct anx7625_data *platform = i2c_get_clientdata(client);

	free_irq(client->irq, platform);
	anx7625_free_gpio(platform);
	destroy_workqueue(platform->workqueue);
	wake_lock_destroy(&platform->anx7625_lock);
	kfree(platform);
	return 0;
}
#ifdef ANX7625_MTK_PLATFORM
#else
static int anx7625_i2c_suspend(
	struct i2c_client *client, pm_message_t state)
{
	return 0;
}

static int anx7625_i2c_resume(struct i2c_client *client)
{
	return 0;
}
#endif
static const struct i2c_device_id anx7625_id[] = {
	{"anx7625", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, anx7625_id);

#ifdef CONFIG_OF
static const struct of_device_id anx_match_table[] = {
	{.compatible = "analogix,anx7625", },
#ifdef ANX7625_MTK_PLATFORM
	{.compatible = "mediatek,dp_switch_trans", },
#endif
	{},
};
#endif

static struct i2c_driver anx7625_driver = {
	.driver = {
		.name = "anx7625",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = anx_match_table,
#endif
	},
	.probe = anx7625_i2c_probe,
	.remove = anx7625_i2c_remove,
#ifdef ANX7625_MTK_PLATFORM
#else
	.suspend = anx7625_i2c_suspend,
	.resume = anx7625_i2c_resume,
#endif
	.id_table = anx7625_id,
};

static void __init anx7625_init_async(
	void *data, async_cookie_t cookie)
{
	int ret = 0;

#ifdef DEBUG_LOG_OUTPUT
	slimport_log_on = true;
#else
	slimport_log_on = false;
#endif

	TRACE("%s:\n", __func__);

#ifdef ANX7625_MTK_PLATFORM
	slimport_platform_init();
#endif
	ret = i2c_add_driver(&anx7625_driver);
	if (ret < 0)
		pr_err("%s: failed to register anx7625 i2c drivern",
			__func__);
}

static int __init anx7625_init(void)
{
#ifdef ANX7625_MTK_PLATFORM
#else
	async_schedule(anx7625_init_async, NULL);
#endif
	return 0;
}

static void __exit anx7625_exit(void)
{
	i2c_del_driver(&anx7625_driver);
}

int slimport_anx7625_init(void)
{
	TRACE("%s:\n", __func__);

#ifdef ANX7625_MTK_PLATFORM
	async_schedule(anx7625_init_async, NULL);
#endif
	return 0;
}





ssize_t anx7625_send_pd_cmd(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	int cmd;
	int result;

	/*result = sscanf(buf, "%d", &cmd);*/
	result = kstrtoint(buf, 10, &cmd);
	switch (cmd) {
	case TYPE_PWR_SRC_CAP:
		send_pd_msg(TYPE_PWR_SRC_CAP, 0, 0);
		break;

	case TYPE_DP_SNK_IDENTITY:
		send_pd_msg(TYPE_DP_SNK_IDENTITY, 0, 0);
		break;

	case TYPE_PSWAP_REQ:
		send_pd_msg(TYPE_PSWAP_REQ, 0, 0);
		break;
	case TYPE_DSWAP_REQ:
		send_pd_msg(TYPE_DSWAP_REQ, 0, 0);
		break;

	case TYPE_GOTO_MIN_REQ:
		send_pd_msg(TYPE_GOTO_MIN_REQ, 0, 0);
		break;

	case TYPE_PWR_OBJ_REQ:
		interface_send_request();
		break;
	case TYPE_ACCEPT:
		interface_send_accept();
		break;
	case TYPE_REJECT:
		interface_send_reject();
		break;
	case TYPE_SOFT_RST:
		send_pd_msg(TYPE_SOFT_RST, 0, 0);
		break;
	case TYPE_HARD_RST:
		send_pd_msg(TYPE_HARD_RST, 0, 0);
		break;

	}
	return count;
}


ssize_t anx7625_send_pswap(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 3, "%d\n", send_power_swap());
}

ssize_t anx7625_send_dswap(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 3, "%d\n", send_data_swap());
}

ssize_t anx7625_get_data_role(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 3, "%d\n", get_data_role());
}
ssize_t anx7625_get_power_role(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 3, "%d\n", get_power_role());
}

ssize_t anx7625_rd_reg(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int v_addr, cmd;
	int result;

	result = sscanf(buf, "%x  %x", &v_addr, &cmd);
	pr_info("reg[%x] = %x\n", cmd, ReadReg(v_addr, cmd));

	return count;

}

ssize_t anx7625_wr_reg(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int v_addr, cmd, val;
	int result;

	result = sscanf(buf, "%x  %x  %x", &v_addr, &cmd, &val);
	pr_info("c %x val %x\n", cmd, val);
	WriteReg(v_addr, cmd, val);
	pr_info("reg[%x] = %x\n", cmd, ReadReg(v_addr, cmd));
	return count;
}

ssize_t anx7625_dump_register(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int i = 0;
	int val;
	int result;
	unsigned char  pLine[100];

	memset(pLine, 0, 100);
	/*result = sscanf(buf, "%x", &val);*/
	result = kstrtoint(buf, 16, &val);

	pr_info(" dump register (0x%x)......\n", val);
	pr_info("	 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
	for (i = 0; i < 256; i++) {
		snprintf(&(pLine[(i%0x10)*3]), 4, "%02X ", ReadReg(val, i));
		if ((i & 0x0f) == 0x0f)
			pr_info("[%02x] %s\n", i - 0x0f, pLine);
	}
	pr_info("\ndown!\n");
	return count;
}


/* dump all registers */
/* Usage: dumpall */
static void dumpall(void)
{
	unsigned char DevAddr;
	char DevAddrString[6+2+1];  /* (6+2) characters + NULL terminator*/
	char addr_string[] = {
		0x54, 0x58, 0x70, 0x72, 0x7a, 0x7e, 0x84 };

	for (DevAddr = 0; DevAddr < sizeof(addr_string); DevAddr++) {
		snprintf(DevAddrString, 3, "%02x", addr_string[DevAddr]);
		anx7625_dump_register(NULL, NULL,
			DevAddrString, sizeof(DevAddrString));
	}

}


ssize_t anx7625_erase_hex(struct device *dev,
				struct device_attribute *attr, char *buf)
{

	MI2_power_on();

	command_erase_mainfw();

	return 1;
}

ssize_t anx7625_burn_hex(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	burnhex(0);

	return 1;
}


ssize_t anx7625_read_hex(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)

{
	int cmd, val;
	int result;

	result = sscanf(buf, "%x  %x", &cmd, &val);
	pr_info("readhex()\n");
	MI2_power_on();
	command_flash_read((unsigned int)cmd, (unsigned long)val);
	pr_info("\n");

	return count;
}

ssize_t anx7625_dpcd_read(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int addrh, addrm, addrl;
	int result;

	result = sscanf(buf, "%x %x %x", &addrh, &addrm, &addrl);
	if (result == 3) {
		unsigned char buff[2];

		sp_tx_aux_dpcdread_bytes(addrh, addrm, addrl, 1, buff);
		pr_info("aux_value = 0x%02x\n", (uint)buff[0]);
	} else {
		pr_info("input parameter error");
	}


	return count;
}
ssize_t anx7625_dpcd_write(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)


{
	int addrh, addrm, addrl, val;
	unsigned char buff[16];
	int result;

	result = sscanf(buf, "%x  %x  %x  %x",
		&addrh, &addrm, &addrl, &val);
	if (result == 4) {
		buff[0] = (unsigned char)val;
		sp_tx_aux_dpcdwrite_bytes(
			addrh, addrm, addrl, 1, buff);
	} else {
		pr_info("error input parameter.");
	}
	return count;
}

ssize_t anx7625_dump_edid(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	uint k, j;
	unsigned char   blocks_num;
	unsigned char edid_blocks[256*2];
	unsigned char  pLine[50];

	blocks_num = sp_tx_edid_read(edid_blocks);

	for (k = 0, j = 0; k < (128 * ((uint)blocks_num + 1)); k++) {
		if ((k&0x0f) == 0) {
			snprintf(&pLine[j], 14, "edid:[%02hhx] %02hhx ",
				(uint)(k / 0x10), (uint)edid_blocks[k]);
			j = j + 13;
		} else {
			snprintf(&pLine[j], 4, "%02hhx ", (uint)edid_blocks[k]);
			j = j + 3;

		}
		if ((k&0x0f) == 0x0f) {
			pr_info("%s\n", pLine);
			j = 0;
		}
	}

	return snprintf(buf, 5, "OK!\n");

}

void anx7625_dpi_config(int table_id)
{
	command_DPI_Configuration(table_id);
}


void anx7625_dsi_config(int table_id)
{
	command_DSI_Configuration(table_id);

}

void anx7625_audio_config(int table_id)
{
	command_Configure_Audio_Input(table_id);

}
void anx7625_dsc_config(int table_id, int ratio)
{
	pr_info("dsc configure table id %d, dsi config:%d\n",
		(uint)table_id, (uint)ratio);
	if (ratio == 0)
		command_DPI_DSC_Configuration(table_id);
	else
		command_DSI_DSC_Configuration(table_id);

}

ssize_t anx7625_debug(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int param[4];
	int result, i;
	char CommandName[20];

	memset(param, 0, sizeof(param));
	result = sscanf(buf, "%s %x %x %x %x", CommandName,
		param, param+1, param+2, param+3);
	pr_info("anx7625 cmd[%s", CommandName);
	for (i = 0; i < result - 1; i++)
		pr_info(" %x", param[i]);
	pr_info("]\n");

	if (strcmp(CommandName, "poweron") == 0) {
		pr_info("MI2_power_on\n");
		MI2_power_on();
	} else if (strcmp(CommandName, "powerdown") == 0) {
		anx7625_power_standby();
	} else if (strcmp(CommandName, "debugon") == 0) {
		debug_on = 1;
		pr_info("debug_on = %d\n", debug_on);
	} else if (strcmp(CommandName, "debugoff") == 0) {
		debug_on = 0;
		pr_info("debug_on = %d\n", debug_on);
	} else if (strcmp(CommandName, "erasehex") == 0) {
		if ((param[0] == 0) && (param[1] == 0))
			command_erase_mainfw();/*erase main fw*/
		else
			/*erase number of sector from index*/
			command_erase_sector(param[0], param[1]);

	} else if (strcmp(CommandName, "burnhex") == 0) {
		debug_on = 1;
		MI2_power_on();
		if (param[0] == 0) { /*update main OCM*/
			command_erase_mainfw();
			burnhex(0);
		} else if (param[0] == 1) { /*update secure OCM*/
			command_erase_securefw();
			burnhex(1);
		} else
			pr_info("Unknown parameter for burnhex.");
		debug_on = 0;
	} else if (strcmp(CommandName, "readhex") == 0) {
		if ((param[0] == 0) && (param[1] == 0))
			command_flash_read(0x1000, 0x100);
		else
			command_flash_read(param[0], param[1]);
	} else if (strcmp(CommandName, "dpidsiaudio") == 0) {
		default_dpi_config = param[0];
		default_dsi_config = param[1];
		default_audio_config = param[2];
		pr_info("default dpi:%d, default dsi:%d, default audio:%d\n",
			default_dpi_config, default_dsi_config,
			default_audio_config);
	} else if (strcmp(CommandName, "dpi") == 0) {
		default_dpi_config = param[0];
		command_Mute_Video(1);
		anx7625_dpi_config(param[0]);
	} else if (strcmp(CommandName, "dsi") == 0) {
		default_dsi_config = param[0];
		command_Mute_Video(1);
		anx7625_dsi_config(param[0]);
	} else if (strcmp(CommandName, "audio") == 0) {
		default_audio_config = param[0];
		anx7625_audio_config(param[0]);
	} else if (strcmp(CommandName, "dsc") == 0) {
		/*default_dpi_config = param[0];*/
		command_Mute_Video(1);
		anx7625_dsc_config(param[0], param[1]);
	} else if (strcmp(CommandName, "dpstart") == 0) {
		hpd_status = 1;
		DP_Process_Start();
	} else if (strcmp(CommandName, "show") == 0) {
		sp_tx_show_information();
	} else if (strcmp(CommandName, "dumpall") == 0) {
		dumpall();
	} else if (strcmp(CommandName, "mute") == 0) {
		command_Mute_Video(param[0]);
	} else {
		pr_info("Usage:\n");
		pr_info("  echo poweron > cmd             :");
		pr_info("			power on\n");
		pr_info("  echo powerdown > cmd           :");
		pr_info("			power off\n");
		pr_info("  echo debugon > cmd             :");
		pr_info("		debug on\n");
		pr_info("  echo debugoff > cmd            :");
		pr_info("			debug off\n");
		pr_info("  echo erasehex > cmd            :");
		pr_info("			erase main fw\n");
		pr_info("  echo burnhex [index] > cmd     :");
		pr_info("	burn FW into flash[0:Main OCM][1:Secure]\n");
		pr_info("  echo readhex [addr] [cnt]> cmd :");
		pr_info("			read bytes from flash\n");
		pr_info("  echo dpi [index] > cmd         :");
		pr_info("			configure dpi with table[index]\n");
		pr_info("  echo dsi [index] > cmd         :");
		pr_info("			configure dsi with table[index]\n");
		pr_info("  echo audio [index] > cmd       :");
		pr_info("			configure audio with table[index]\n");
		pr_info("  echo dpidsiaudio [dpi] [dsi] [audio]> cmd  :\n");
		pr_info("		configure default dpi dsi audio");
		pr_info("			dpi/dsi/audio function.\n");
		pr_info("  echo dsc [index][flag] > cmd         :");
		pr_info("			configure dsc with [index]&[flag]\n");
		pr_info("  echo dpstart > cmd         :");
		pr_info("			Start DP process\n");
		pr_info("  echo show > cmd            :");
		pr_info("			Show DP result information\n");
		pr_info("  echo dumpall > cmd            :");
		pr_info("			Dump anx7625 all register\n");
	}

	return count;
}

/* for debugging */
static struct device_attribute anx7625_device_attrs[] = {
	__ATTR(pdcmd,    S_IWUSR, NULL, anx7625_send_pd_cmd),
	__ATTR(rdreg,    S_IWUSR, NULL, anx7625_rd_reg),
	__ATTR(wrreg,    S_IWUSR, NULL, anx7625_wr_reg),
	__ATTR(dumpreg,  S_IWUSR, NULL, anx7625_dump_register),
	__ATTR(prole,    S_IRUGO, anx7625_get_power_role, NULL),
	__ATTR(drole,    S_IRUGO, anx7625_get_data_role, NULL),
	__ATTR(pswap,    S_IRUGO, anx7625_send_pswap, NULL),
	__ATTR(dswap,    S_IRUGO, anx7625_send_dswap, NULL),
	__ATTR(dpcdr,    S_IWUSR, NULL, anx7625_dpcd_read),
	__ATTR(dpcdw,    S_IWUSR, NULL, anx7625_dpcd_write),
	__ATTR(dumpedid, S_IRUGO, anx7625_dump_edid, NULL),
	__ATTR(cmd,      S_IWUSR, NULL, anx7625_debug)
};


static int create_sysfs_interfaces(struct device *dev)
{
	int i;

	TRACE("anx7625 create system fs interface ...\n");
	for (i = 0; i < ARRAY_SIZE(anx7625_device_attrs); i++)
		if (device_create_file(dev, &anx7625_device_attrs[i]))
			goto error;
	TRACE("success\n");
	return 0;
error:

	for (; i >= 0; i--)
		device_remove_file(dev, &anx7625_device_attrs[i]);
	pr_err("%s %s: anx7625 Unable to create interface",
		LOG_TAG, __func__);
	return -EINVAL;
}

module_init(anx7625_init);
module_exit(anx7625_exit);

MODULE_DESCRIPTION("USB PD anx7625 driver");
MODULE_AUTHOR("Li Zhen <zhenli@analogixsemi.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(ANX7625_DRV_VERSION);
