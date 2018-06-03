/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <generated/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/seq_file.h>
#include <linux/power_supply.h>
#include <linux/time.h>
#include <linux/uaccess.h>

#include <mt-plat/mtk_battery.h>
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mt-plat/mtk_boot.h>
#include <mt-plat/charger_type.h>
#include <pmic.h>
#include "mt6336.h"

#if !defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_30_SUPPORT)
#define __SW_CHRDET_IN_PROBE_PHASE__
#endif
/*#define __CHRDET_RG_DUMP__*/

static CHARGER_TYPE g_chr_type;
static struct mt_charger *g_mt_charger;
#ifdef __SW_CHRDET_IN_PROBE_PHASE__
static struct work_struct chr_work;
#endif

#if !defined(CONFIG_MTK_FPGA)
static struct mt6336_ctrl *core_ctrl;
static bool first_connect = true;
#endif

static DEFINE_MUTEX(chrdet_lock);

#if  defined(CONFIG_MTK_FPGA)
/*****************************************************************************
 * FPGA
 ******************************************************************************/
int hw_charging_get_charger_type(void)
{
	return STANDARD_HOST;
}

#else
/*****************************************************************************
 * EVB / Phone
 ******************************************************************************/
static unsigned short bc12_set_register_value(MT6336_PMU_FLAGS_LIST_ENUM flagname, unsigned int val)
{
	return mt6336_set_flag_register_value(flagname, val);
}

static unsigned short bc12_get_register_value(MT6336_PMU_FLAGS_LIST_ENUM flagname)
{
	return mt6336_get_flag_register_value(flagname);
}

static bool check_rdm_dwm_requirement(void)
{
	unsigned short var1 = bc12_get_register_value(MT6336_PMIC_CID);
	unsigned short var2 = bc12_get_register_value(MT6336_PMIC_HWCID);

	if ((var1 == 0x36) && (var2 == 0x20))
		return true;

	return false;
}


static void hw_bc12_init(void)
{
	int timeout = 200;

	if (first_connect == true) {
		/* add make sure USB Ready */
		if (is_usb_rdy() == false) {
			pr_err("CDP, block\n");
			while (is_usb_rdy() == false && timeout > 0) {
				msleep(100);
				timeout--;
			}
			if (timeout == 0)
				pr_err("CDP, timeout\n");
			else
				pr_err("CDP, free\n");
		} else
			pr_err("CDP, PASS\n");
		first_connect = false;
	}

	/* RG_BC12_BB_CTRL = 1 */
	bc12_set_register_value(MT6336_RG_A_BC12_BB_CTRL, 1);
	/* RG_BC12_RST = 1 */
	bc12_set_register_value(MT6336_RG_BC12_RST, 1);
	/* RG_A_BC12_IBIAS_EN = 1 */
	bc12_set_register_value(MT6336_RG_A_BC12_IBIAS_EN, 1);
	/* Delay 10ms */
	usleep_range(10000, 20000);
	/* RG_A_BC12_VSRC_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VSRC_EN, 0);
	/* RG_A_BC12_VREF_VTH_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VREF_VTH_EN, 0);
	/* RG_A_BC12_CMP_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_CMP_EN, 0);
	/* RG_A_BC12_IPU_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPU_EN, 0);
	/* RG_A_BC12_IPD_EN[1:0]= 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 0);
	/* RG_A_BC12_IPD_HALF_EN = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_HALF_EN, 0);
#if defined(CONFIG_PROJECT_PHY)
	Charger_Detect_Init();
#endif
	/* VBUS asserted, wait > 300ms (min.) */
	msleep(300);
}


static unsigned int hw_bc12_step_1(void)
{
	unsigned int wChargerAvail = 0;
	unsigned int tmp = 0;

	/* RG_A_BC12_VSRC_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VSRC_EN, 0);
	/* RG_A_BC12_VREF_VTH_EN[1:0] = 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_VREF_VTH_EN, 1);
	/* RG_A_BC12_CMP_EN[1:0] = 10 */
	bc12_set_register_value(MT6336_RG_A_BC12_CMP_EN, 2);
	/* RG_A_BC12_IPU_EN[1:0] = 10 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPU_EN, 2);
	/* RG_A_BC12_IPD_EN[1:0]= 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 1);
	/* RG_A_BC12_IPD_HALF_EN = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_HALF_EN, 0);
	/**/
	if (check_rdm_dwm_requirement()) {
		tmp = bc12_get_register_value(MT6336_RG_A_ANABASE_RSV);
		bc12_set_register_value(MT6336_RG_A_ANABASE_RSV, (tmp | 0x1));
	}

	/* Delay 10ms */
	usleep_range(10000, 20000);

	/* RGS_BC12_CMP_OUT */
	/* Check BC 1.2 comparator output
	 * 0: Jump to 2-a
	 * 1: Jump to 2-b
	 */
	wChargerAvail = bc12_get_register_value(MT6336_AD_QI_BC12_CMP_OUT);
#if defined(__CHRDET_RG_DUMP__)
	pr_err("%s(%d): %d", __func__, bc12_get_register_value(MT6336_PMIC_HWCID), wChargerAvail);
#endif

	/**/
	if (check_rdm_dwm_requirement()) {
		tmp = bc12_get_register_value(MT6336_RG_A_ANABASE_RSV);
		bc12_set_register_value(MT6336_RG_A_ANABASE_RSV, (tmp & ~0x1));
	}


	/* Delay 10ms */
	usleep_range(10000, 20000);

	return wChargerAvail;
}

static unsigned int hw_bc12_step_2a(void)
{
	unsigned int wChargerAvail = 0;
	unsigned int tmp = 0;

	/* RG_A_BC12_VSRC_EN[1:0] = 10 */
	bc12_set_register_value(MT6336_RG_A_BC12_VSRC_EN, 2);
	/* RG_A_BC12_VREF_VTH_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VREF_VTH_EN, 0);
	/* RG_A_BC12_CMP_EN[1:0] = 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_CMP_EN, 1);
	/* RG_A_BC12_IPU_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPU_EN, 0);
	/* RG_A_BC12_IPD_EN[1:0]= 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 1);
	/* RG_A_BC12_IPD_HALF_EN = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_HALF_EN, 0);
	/**/
	if (check_rdm_dwm_requirement()) {
		tmp = bc12_get_register_value(MT6336_RG_A_ANABASE_RSV);
		bc12_set_register_value(MT6336_RG_A_ANABASE_RSV, (tmp | 0x1));
	}

	/* Delay 40ms */
	msleep(40);

	/* RGS_BC12_CMP_OUT */
	/* Check BC 1.2 comparator output
	 * 0: SDP
	 * 1: Jump to 3-a
	 */
	wChargerAvail = bc12_get_register_value(MT6336_AD_QI_BC12_CMP_OUT);
#if defined(__CHRDET_RG_DUMP__)
	pr_err("%s(%d): %d", __func__, bc12_get_register_value(MT6336_PMIC_HWCID), wChargerAvail);
#endif
	/**/
	if (check_rdm_dwm_requirement()) {
		tmp = bc12_get_register_value(MT6336_RG_A_ANABASE_RSV);
		bc12_set_register_value(MT6336_RG_A_ANABASE_RSV, (tmp & ~0x1));
	}


	/* Delay 20ms */
	msleep(20);

	return wChargerAvail;
}

static unsigned int hw_bc12_step_2b1(void)
{
	unsigned int wChargerAvail = 0;
	/* RG_A_BC12_VSRC_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VSRC_EN, 0);
	/* RG_A_BC12_VREF_VTH_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VREF_VTH_EN, 0);
	/* RG_A_BC12_CMP_EN[1:0] = 10 */
	bc12_set_register_value(MT6336_RG_A_BC12_CMP_EN, 2);
	/* RG_A_BC12_IPU_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPU_EN, 0);
	/* RG_A_BC12_IPD_EN[1:0]= 10 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 2);
	/* RG_A_BC12_IPD_HALF_EN = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_HALF_EN, 0);
	/* Delay 10ms */
	usleep_range(10000, 20000);

	/* RGS_BC12_CMP_OUT */
	/* Latch output OUT1 */
	wChargerAvail = bc12_get_register_value(MT6336_AD_QI_BC12_CMP_OUT);
#if defined(__CHRDET_RG_DUMP__)
	pr_err("%s(%d): %d\n", __func__, bc12_get_register_value(MT6336_PMIC_HWCID), wChargerAvail);
#endif

	/* Delay 10ms */
	usleep_range(10000, 20000);

	return wChargerAvail;
}

static unsigned int hw_bc12_step_2b2(void)
{
	unsigned int wChargerAvail = 0;

	/* RG_A_BC12_VSRC_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VSRC_EN, 0);
	/* RG_A_BC12_VREF_VTH_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VREF_VTH_EN, 0);
	/* RG_A_BC12_CMP_EN[1:0] = 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_CMP_EN, 1);
	/* RG_A_BC12_IPU_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPU_EN, 0);
	/* RG_A_BC12_IPD_EN[1:0]= 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 1);
	/* RG_A_BC12_IPD_HALF_EN = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_HALF_EN, 0);
	/* Delay 10ms */
	usleep_range(10000, 20000);

	/* RGS_BC12_CMP_OUT */
	/* Latch output OUT2 */
	wChargerAvail = bc12_get_register_value(MT6336_AD_QI_BC12_CMP_OUT);
#if defined(__CHRDET_RG_DUMP__)
	pr_err("%s(%d): %d\n", __func__, bc12_get_register_value(MT6336_PMIC_HWCID), wChargerAvail);
#endif

	/* Delay 10ms */
	usleep_range(10000, 20000);

	return wChargerAvail;
}

#if 0
static unsigned int hw_bc12_step_2b3(void)
{
	unsigned int wChargerAvail = 0;
	/* Check OUT1 and OUT2
	 * 1. (OUT1, OUT2) = 00: Jump to 3b
	 * 2. (OUT1, OUT2) = 01: IPhone 5V-1A, Jump to END
	 * 3. (OUT1, OUT2) = others: IPad2 5V-2A or IPad4 5V-2.4A, Jump to END"
	 */

	return wChargerAvail;
}
#endif

static unsigned int hw_bc12_step_3a(void)
{
	unsigned int wChargerAvail = 0;

	/* RG_A_BC12_VSRC_EN[1:0] = 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_VSRC_EN, 1);
	/* RG_A_BC12_VREF_VTH_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VREF_VTH_EN, 0);
	/* RG_A_BC12_CMP_EN[1:0] = 10 */
	bc12_set_register_value(MT6336_RG_A_BC12_CMP_EN, 2);
	/* RG_A_BC12_IPU_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPU_EN, 0);
	/* RG_A_BC12_IPD_EN[1:0]= 10 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 2);
	/* RG_A_BC12_IPD_HALF_EN = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_HALF_EN, 0);
	/* Delay 40ms */
	msleep(40);

	/* RGS_BC12_CMP_OUT */
	/* Check BC 1.2 comparator output
	 * 0: CDP
	 * 1: DCP
	 */
	wChargerAvail = bc12_get_register_value(MT6336_AD_QI_BC12_CMP_OUT);
#if defined(__CHRDET_RG_DUMP__)
	pr_err("%s(%d): %d\n", __func__, bc12_get_register_value(MT6336_PMIC_HWCID), wChargerAvail);
#endif

	/* Delay 20ms */
	msleep(20);

	return wChargerAvail;
}

static unsigned int hw_bc12_step_3b(void)
{
	unsigned int wChargerAvail = 0;

	/* RG_A_BC12_VSRC_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VSRC_EN, 0);
	/* RG_A_BC12_VREF_VTH_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VREF_VTH_EN, 0);
	/* RG_A_BC12_CMP_EN[1:0] = 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_CMP_EN, 1);
	/* RG_A_BC12_IPU_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPU_EN, 0);
	/* RG_A_BC12_IPD_EN[1:0]= 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 1);
	/* RG_A_BC12_IPD_HALF_EN = 1 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_HALF_EN, 1);
	/* Delay 10ms */
	usleep_range(10000, 20000);

	/* RGS_BC12_CMP_OUT */
	/* Check BC 1.2 comparator output
	 * 0: Jump to 4
	 * 1: Non-STD Charger
	 */
	wChargerAvail = bc12_get_register_value(MT6336_AD_QI_BC12_CMP_OUT);
#if defined(__CHRDET_RG_DUMP__)
	pr_err("%s(%d): %d\n", __func__, bc12_get_register_value(MT6336_PMIC_HWCID), wChargerAvail);
#endif

	/* Delay 10ms */
	usleep_range(10000, 20000);

	return wChargerAvail;
}

static unsigned int hw_bc12_step_4(void)
{
	unsigned int wChargerAvail = 0;

	/* RG_A_BC12_VSRC_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VSRC_EN, 0);
	/* RG_A_BC12_VREF_VTH_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VREF_VTH_EN, 0);
	/* RG_A_BC12_CMP_EN[1:0] = 01 */
	bc12_set_register_value(MT6336_RG_A_BC12_CMP_EN, 1);
	/* RG_A_BC12_IPU_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPU_EN, 0);
	/* RG_A_BC12_IPD_EN[1:0]= 10 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 2);
	/* RG_A_BC12_IPD_HALF_EN = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_HALF_EN, 0);
	/* Delay 10ms */
	usleep_range(10000, 20000);
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 0);
	/* Delay 10ms */
	usleep_range(10000, 20000);

	/* RGS_BC12_CMP_OUT */
	wChargerAvail = bc12_get_register_value(MT6336_AD_QI_BC12_CMP_OUT);
#if defined(__CHRDET_RG_DUMP__)
	pr_err("%s(%d): %d\n", __func__, bc12_get_register_value(MT6336_PMIC_HWCID), wChargerAvail);
#endif

	/* Delay 10ms */
	usleep_range(10000, 20000);

	return wChargerAvail;
}


static void hw_bc12_done(void)
{
#if defined(CONFIG_PROJECT_PHY)
	Charger_Detect_Release();
#endif
	/* RG_BC12_BB_CTRL = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_BB_CTRL, 0);
	/* RG_BC12_RST = 1 */
	bc12_set_register_value(MT6336_RG_BC12_RST, 1);
	/* RG_A_BC12_IBIAS_EN = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_IBIAS_EN, 0);
	/* RG_A_BC12_VSRC_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VSRC_EN, 0);
	/* RG_A_BC12_VREF_VTH_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_VREF_VTH_EN, 0);
	/* RG_A_BC12_CMP_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_CMP_EN, 0);
	/* RG_A_BC12_IPU_EN[1:0] = 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPU_EN, 0);
	/* RG_A_BC12_IPD_EN[1:0]= 00 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_EN, 0);
	/* RG_A_BC12_IPD_HALF_EN = 0 */
	bc12_set_register_value(MT6336_RG_A_BC12_IPD_HALF_EN, 0);
}

static void dump_charger_name(CHARGER_TYPE type)
{
	switch (type) {
	case CHARGER_UNKNOWN:
		pr_err("charger type: %d, CHARGER_UNKNOWN\n", type);
		break;
	case STANDARD_HOST:
		pr_err("charger type: %d, Standard USB Host\n", type);
		break;
	case CHARGING_HOST:
		pr_err("charger type: %d, Charging USB Host\n", type);
		break;
	case NONSTANDARD_CHARGER:
		pr_err("charger type: %d, Non-standard Charger\n", type);
		break;
	case STANDARD_CHARGER:
		pr_err("charger type: %d, Standard Charger\n", type);
		break;
	case APPLE_2_1A_CHARGER:
		pr_err("charger type: %d, APPLE_2_1A_CHARGER\n", type);
		break;
	case APPLE_1_0A_CHARGER:
		pr_err("charger type: %d, APPLE_1_0A_CHARGER\n", type);
		break;
	case APPLE_0_5A_CHARGER:
		pr_err("charger type: %d, APPLE_0_5A_CHARGER\n", type);
		break;
	default:
		pr_err("charger type: %d, Not Defined!!!\n", type);
		break;
	}
}

int hw_charging_get_charger_type(void)
{
	unsigned int out = 0;

	CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;

	hw_bc12_init();

	if (hw_bc12_step_1()) {
		if (hw_bc12_step_2b1())
			out = 0x02;
		if (hw_bc12_step_2b2())
			out |= 0x01;
		switch (out) {
		/* Check OUT1 and OUT2
		 * 1. (OUT1, OUT2) = 00: Jump to 3b
		 * 2. (OUT1, OUT2) = 01: IPhone 5V-1A, Jump to END
		 * 3. (OUT1, OUT2) = others: IPad2 5V-2A or IPad4 5V-2.4A, Jump to END"
		 */
		case 0:
			if (hw_bc12_step_3b()) {
				CHR_Type_num = NONSTANDARD_CHARGER;
			} else {
				pr_err("charger type: DP/DM are floating! force to Non-STD\n");
				CHR_Type_num = NONSTANDARD_CHARGER;
			}
			break;
		case 1:
			CHR_Type_num = APPLE_1_0A_CHARGER;
			break;
		case 2:
		case 3:
			CHR_Type_num = APPLE_2_1A_CHARGER;
			break;
		}
	} else {
		if (hw_bc12_step_2a()) {
			if (hw_bc12_step_3a())
				if (hw_bc12_step_4()) {
					pr_err("charger type: SS 5V/2A. force to DCP\n");
					CHR_Type_num = STANDARD_CHARGER;
				} else
					CHR_Type_num = STANDARD_CHARGER;
			else
				CHR_Type_num = CHARGING_HOST;
		} else
			CHR_Type_num = STANDARD_HOST;
	}

	dump_charger_name(CHR_Type_num);

	hw_bc12_done();

	return CHR_Type_num;
}


/************************************************/
/* Power Supply
*************************************************/

/**
 * struct gpio_charger_platform_data - platform_data for gpio_charger devices
 * @name:		Name for the chargers power_supply device
 * @type:		Type of the charger
 * @gpio:		GPIO which is used to indicate the chargers status
 * @gpio_active_low:	Should be gset to 1 if the GPIO is active low otherwise 0
 * @supplied_to:	Array of battery names to which this chargers supplies power
 * @num_supplicants:	Number of entries in the supplied_to array
 */
struct mt_charger_platform_data {
	const char *name;
	enum power_supply_type type;

	char **supplied_to;
	size_t num_supplicants;
};


struct mt_charger {
	/* unsigned int irq; */

	struct power_supply_desc charger;
	struct power_supply *charger_psy;
	struct power_supply_desc usb;
	struct power_supply *usb_psy;
};

/*****************************************************************************
 * Charger Detection
 ******************************************************************************/
void do_charger_detect(void)
{
	if (!mt_usb_is_device()) {
		g_chr_type = CHARGER_UNKNOWN;
		pr_err("charger type: UNKNOWN, Now is usb host mode. Skip detection!!!\n");
		return;
	}
#ifdef CONFIG_USB_C_SWITCH_MT6336
	if (is_otg_en()) {
		g_chr_type = CHARGER_UNKNOWN;
		pr_err("charger type: UNKNOWN, Now is TYPEC sink mode. Skip detection!!!\n");
		return;
	}
#endif

	/* enable ctrl to lock power, keeping MT6336 in normal mode */
	mt6336_ctrl_enable(core_ctrl);
	mutex_lock(&chrdet_lock);

	if (upmu_get_rgs_chrdet() == true) {
		pr_err("charger IN\n");
		g_chr_type = hw_charging_get_charger_type();
	} else {
		pr_err("charger OUT\n");
		g_chr_type = CHARGER_UNKNOWN;
	}

	/* usb */
	if ((g_chr_type == STANDARD_HOST) || (g_chr_type == CHARGING_HOST))
		mt_usb_connect();
	else
		mt_usb_disconnect();

	mtk_charger_int_handler();
	fg_charger_in_handler();


	if (g_mt_charger) {
		power_supply_changed(g_mt_charger->charger_psy);
		power_supply_changed(g_mt_charger->usb_psy);
	}

	mutex_unlock(&chrdet_lock);
	/* enter low power mode*/
	mt6336_ctrl_disable(core_ctrl);
}


/*****************************************************************************
 * PMIC Int Handler
 ******************************************************************************/
void chrdet_int_handler(void)
{
	/*
	 * pr_err("[chrdet_int_handler]CHRDET status = %d....\n",
	 *	pmic_get_register_value(PMIC_RGS_CHRDET));
	 */
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if (!upmu_get_rgs_chrdet()) {
		int boot_mode = 0;

		boot_mode = get_boot_mode();

		if (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT
		    || boot_mode == LOW_POWER_OFF_CHARGING_BOOT) {
			pr_err("[chrdet_int_handler] Unplug Charger/USB\n");
			mt_power_off();
		}
	}
#endif
	pmic_set_register_value(PMIC_RG_USBDL_RST, 1);
	do_charger_detect();
}

#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_30_SUPPORT
int typec_chrdet_int_handler(int enable)
{
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if (enable == 0) {
		int boot_mode = 0;

		boot_mode = get_boot_mode();

		if (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT
		    || boot_mode == LOW_POWER_OFF_CHARGING_BOOT) {
			pr_err("[chrdet_int_handler] Unplug Charger/USB\n");
			mt_power_off();
			return -1;
		}
	}
#endif

	/* enable ctrl to lock power, keeping MT6336 in normal mode */
	mt6336_ctrl_enable(core_ctrl);

	pmic_set_register_value(PMIC_RG_USBDL_RST, 1);

	if (enable) {
		pr_err("charger IN\n");
		g_chr_type = hw_charging_get_charger_type();
	} else {
		pr_err("charger OUT\n");
		g_chr_type = CHARGER_UNKNOWN;
	}

	mtk_charger_int_handler();
	fg_charger_in_handler();

	if (g_mt_charger) {
		power_supply_changed(g_mt_charger->charger_psy);
		power_supply_changed(g_mt_charger->usb_psy);
	}

	/* enter low power mode*/
	mt6336_ctrl_disable(core_ctrl);

	return 0;
}
#endif

/************************************************/
/* Power Supply Functions
*************************************************/
static int mt_charger_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	/* struct mt_charger *mt_charger = container_of(psy, struct mt_charger, charger); */
	/* const struct mt_charger_platform_data *pdata = mt_charger->pdata; */

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 0;
		/* force to 1 in all charger type */
		if (g_chr_type != CHARGER_UNKNOWN)
			val->intval = 1;
		/* reset to 0 if charger type is USB */
		if ((g_chr_type == STANDARD_HOST) || (g_chr_type == CHARGING_HOST))
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mt_usb_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	/* struct mt_charger *mt_charger = container_of(psy, struct mt_charger, usb); */
	/* const struct mt_charger_platform_data *pdata = mt_charger->pdata; */

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if ((g_chr_type == STANDARD_HOST) || (g_chr_type == CHARGING_HOST))
			val->intval = 1;
		else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static enum power_supply_property mt_charger_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property mt_usb_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

#ifdef __SW_CHRDET_IN_PROBE_PHASE__
static void do_charger_detection_work(struct work_struct *data)
{
	if (upmu_get_rgs_chrdet() == true)
		do_charger_detect();
}
#endif

static int mt_charger_probe(struct platform_device *pdev)
{
	/* const struct mt_charger_platform_data *pdata = pdev->dev.platform_data; */
	struct mt_charger *mt_charger;
	struct power_supply_desc *charger;
	struct power_supply_desc *usb;
	int ret;

	/*
	 * if (!pdata) {
	 *	dev_err(&pdev->dev, "No platform data\n");
	 *	return -EINVAL;
	 * }
	 */

	mt_charger = devm_kzalloc(&pdev->dev, sizeof(*mt_charger), GFP_KERNEL);
	g_mt_charger = mt_charger;
	if (!mt_charger)
		return -ENOMEM;

	charger = &mt_charger->charger;
	/* charger->name = "mt-chr"; */
	charger->name = "ac";
	charger->type = POWER_SUPPLY_TYPE_MAINS;
	charger->properties = mt_charger_properties;
	charger->num_properties = ARRAY_SIZE(mt_charger_properties);
	charger->get_property = mt_charger_get_property;
	/* charger->supplied_to = pdata->supplied_to; */
	/* charger->num_supplicants = pdata->num_supplicants; */

	usb = &mt_charger->usb;
	/* usb->name = "mt-usb"; */
	usb->name = "usb";
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->properties = mt_usb_properties;
	usb->num_properties = ARRAY_SIZE(mt_usb_properties);
	usb->get_property = mt_usb_get_property;
	/* usb->supplied_to = pdata->supplied_to; */
	/* usb->num_supplicants = pdata->num_supplicants; */


	/* mt_charger->pdata = pdata; */

	mt_charger->charger_psy = power_supply_register(&pdev->dev, charger, NULL);
	if (IS_ERR(mt_charger->charger_psy)) {
		dev_err(&pdev->dev, "Failed to register power supply: %ld\n",
			PTR_ERR(mt_charger->charger_psy));
		ret = PTR_ERR(mt_charger->charger_psy);
		return ret;
	}

	mt_charger->usb_psy = power_supply_register(&pdev->dev, usb, NULL);
	if (IS_ERR(mt_charger->usb_psy)) {
		dev_err(&pdev->dev, "Failed to register power supply: %ld\n",
			PTR_ERR(mt_charger->usb_psy));
		ret = PTR_ERR(mt_charger->usb_psy);
		return ret;
	}

	platform_set_drvdata(pdev, mt_charger);
	device_init_wakeup(&pdev->dev, 1);

	core_ctrl = mt6336_ctrl_get("mt6336_chrdet");
	mutex_init(&chrdet_lock);

#ifdef __SW_CHRDET_IN_PROBE_PHASE__
	/* do charger detect here to prevent HW miss interrupt*/
	INIT_WORK(&chr_work, do_charger_detection_work);
	schedule_work(&chr_work);
#endif

#if !defined(CONFIG_MTK_PUMP_EXPRESS_PLUS_30_SUPPORT)
	pmic_register_interrupt_callback(INT_CHRDET_EDGE, chrdet_int_handler);
	pmic_enable_interrupt(INT_CHRDET_EDGE, 1, "PMIC");
#else
	register_charger_det_callback(typec_chrdet_int_handler);
#endif
	return 0;
}

static int mt_charger_remove(struct platform_device *pdev)
{
	struct mt_charger *mt_charger = platform_get_drvdata(pdev);

	power_supply_unregister(mt_charger->charger_psy);
	power_supply_unregister(mt_charger->usb_psy);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mt_charger_suspend(struct device *dev)
{
	/* struct mt_charger *mt_charger = dev_get_drvdata(dev); */
	return 0;
}

static int mt_charger_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mt_charger *mt_charger = platform_get_drvdata(pdev);

	power_supply_changed(mt_charger->charger_psy);
	power_supply_changed(mt_charger->usb_psy);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mt_charger_pm_ops,
		mt_charger_suspend, mt_charger_resume);

static const struct of_device_id mt_charger_match[] = {
	{ .compatible = "mediatek,mt-charger", },
	{ },
};
static struct platform_driver mt_charger_driver = {
	.probe = mt_charger_probe,
	.remove = mt_charger_remove,
	.driver = {
		.name = "mt-charger-det",
		.owner = THIS_MODULE,
		.pm = &mt_charger_pm_ops,
		.of_match_table = mt_charger_match,
	},
};


/* Legacy api to prevent build error */
bool upmu_is_chr_det(void)
{
	if (upmu_get_rgs_chrdet())
		return true;

	return false;
}

/* Legacy api to prevent build error */
bool pmic_chrdet_status(void)
{
	if (upmu_is_chr_det())
		return true;

	pr_err("[pmic_chrdet_status] No charger\r\n");
	return false;
}

CHARGER_TYPE mt_get_charger_type(void)
{
	return g_chr_type;
}

static s32 __init mt_charger_det_init(void)
{
	return platform_driver_register(&mt_charger_driver);
}

static void __exit mt_charger_det_exit(void)
{
	platform_driver_unregister(&mt_charger_driver);
}

late_initcall(mt_charger_det_init);
module_exit(mt_charger_det_exit);

MODULE_DESCRIPTION("mt-charger-detection");
MODULE_AUTHOR("MediaTek");
MODULE_LICENSE("GPL v2");
/*module_platform_driver(mt_charger_driver);*/


#endif
