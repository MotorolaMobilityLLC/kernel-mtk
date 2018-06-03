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

#include <linux/uaccess.h>
#include <mt-plat/charging.h>
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mach/mtk_pmic_wrap.h>
#if defined CONFIG_MTK_LEGACY
/*#include <mach/mtk_gpio.h> TBD*/
#endif
/*#include <mach/mtk_rtc.h> TBD*/
#if defined(CONFIG_MTK_SMART_BATTERY)
#include <mt-plat/battery_common.h>
#endif
#include <linux/time.h>
#include <mt-plat/mtk_boot.h>

#ifdef CONFIG_MTK_USB2JTAG_SUPPORT
#include <mt-plat/mtk_usb2jtag.h>
#endif


/* ============================================================ // */
/* extern function */
/* ============================================================ // */
void __attribute__((weak)) Charger_Detect_Init(void)
{
}

void __attribute__((weak)) Charger_Detect_Release(void)
{
}

bool is_dcp_type;

#if defined(CONFIG_POWER_EXT) || defined(CONFIG_FPGA_EARLY_PORTING)

int hw_charging_get_charger_type(void)
{
	return STANDARD_HOST;
}

#else

#if 0
static void hw_bc11_dump_register(void)
{

	battery_log(BAT_LOG_FULL, "Reg[0x%x]=0x%x,Reg[0x%x]=0x%x\n",
	MT6325_CHR_CON20, upmu_get_reg_value(MT6325_CHR_CON20),
	MT6325_CHR_CON21, upmu_get_reg_value(MT6325_CHR_CON21)
	);

}
#endif
static void hw_bc11_init(void)
{
	msleep(200);
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	/* add delay to make sure android init.rc finish */
	if (get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT
	    || get_boot_mode() == LOW_POWER_OFF_CHARGING_BOOT) {
		msleep(1000);
	}
#endif

#if defined(CONFIG_MTK_SMART_BATTERY)
	Charger_Detect_Init();
#endif
	/* RG_bc11_BIAS_EN=1 */
	bc11_set_register_value(PMIC_RG_BC11_BIAS_EN, 1);
	/* RG_bc11_VSRC_EN[1:0]=00 */
	bc11_set_register_value(PMIC_RG_BC11_VSRC_EN, 0);
	/* RG_bc11_VREF_VTH = [1:0]=00 */
	bc11_set_register_value(PMIC_RG_BC11_VREF_VTH, 0);
	/* RG_bc11_CMP_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0);
	/* RG_bc11_IPU_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_IPU_EN, 0);
	/* RG_bc11_IPD_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_IPD_EN, 0);
	/* bc11_RST=1 */
	bc11_set_register_value(PMIC_RG_BC11_RST, 1);
	/* bc11_BB_CTRL=1 */
	bc11_set_register_value(PMIC_RG_BC11_BB_CTRL, 1);

	msleep(50);
	/* mdelay(50); */
#if 0
	if (Enable_BATDRV_LOG == BAT_LOG_FULL) {
		battery_log(BAT_LOG_FULL, "hw_bc11_init() \r\n");
		hw_bc11_dump_register();
	}
#endif
}


static unsigned int hw_bc11_DCD(void)
{
	unsigned int wChargerAvail = 0;

	/* RG_bc11_IPU_EN[1.0] = 10 */
	bc11_set_register_value(PMIC_RG_BC11_IPU_EN, 0x2);
	/* RG_bc11_IPD_EN[1.0] = 01 */
	bc11_set_register_value(PMIC_RG_BC11_IPD_EN, 0x1);
	/* RG_bc11_VREF_VTH = [1:0]=01 */
	bc11_set_register_value(PMIC_RG_BC11_VREF_VTH, 0x1);
	/* RG_bc11_CMP_EN[1.0] = 10 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0x2);

	msleep(80);
	/* mdelay(80); */

	wChargerAvail = bc11_get_register_value(PMIC_RGS_BC11_CMP_OUT);
#if 0
	if (Enable_BATDRV_LOG == BAT_LOG_FULL) {
		battery_log(BAT_LOG_FULL, "hw_bc11_DCD() \r\n");
		hw_bc11_dump_register();
	}
#endif
	/* RG_bc11_IPU_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_IPU_EN, 0x0);
	/* RG_bc11_IPD_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_IPD_EN, 0x0);
	/* RG_bc11_CMP_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0x0);
	/* RG_bc11_VREF_VTH = [1:0]=00 */
	bc11_set_register_value(PMIC_RG_BC11_VREF_VTH, 0x0);


	return wChargerAvail;
}


static unsigned int hw_bc11_stepA1(void)
{
	unsigned int wChargerAvail = 0;

	/* RG_bc11_IPD_EN[1.0] = 01 */
	bc11_set_register_value(PMIC_RG_BC11_IPD_EN, 0x1);
	/* RG_bc11_VREF_VTH = [1:0]=00 */
	bc11_set_register_value(PMIC_RG_BC11_VREF_VTH, 0x0);
	/* RG_bc11_CMP_EN[1.0] = 01 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0x1);

	msleep(80);
	/* mdelay(80); */

	wChargerAvail = bc11_get_register_value(PMIC_RGS_BC11_CMP_OUT);
#if 0
	if (Enable_BATDRV_LOG == BAT_LOG_FULL) {
		battery_log(BAT_LOG_FULL, "hw_bc11_stepA1() \r\n");
		hw_bc11_dump_register();
	}
#endif
	/* RG_bc11_IPD_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_IPD_EN, 0x0);
	/* RG_bc11_CMP_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0x0);

	return wChargerAvail;
}


static unsigned int hw_bc11_stepA2(void)
{
	unsigned int wChargerAvail = 0;

	/* RG_bc11_VSRC_EN[1.0] = 10 */
	bc11_set_register_value(PMIC_RG_BC11_VSRC_EN, 0x2);
	/* RG_bc11_IPD_EN[1:0] = 01 */
	bc11_set_register_value(PMIC_RG_BC11_IPD_EN, 0x1);
	/* RG_bc11_VREF_VTH = [1:0]=00 */
	bc11_set_register_value(PMIC_RG_BC11_VREF_VTH, 0x0);
	/* RG_bc11_CMP_EN[1.0] = 01 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0x1);

	msleep(80);
	/* mdelay(80); */

	wChargerAvail = bc11_get_register_value(PMIC_RGS_BC11_CMP_OUT);

#if 0
	if (Enable_BATDRV_LOG == BAT_LOG_FULL) {
		battery_log(BAT_LOG_FULL, "hw_bc11_stepA2() \r\n");
		hw_bc11_dump_register();
	}
#endif
	/* RG_bc11_VSRC_EN[1:0]=00 */
	bc11_set_register_value(PMIC_RG_BC11_VSRC_EN, 0x0);
	/* RG_bc11_IPD_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_IPD_EN, 0x0);
	/* RG_bc11_CMP_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0x0);

	return wChargerAvail;
}


static unsigned int hw_bc11_stepB2(void)
{
	unsigned int wChargerAvail = 0;

	/* RG_bc11_IPU_EN[1:0]=10 */
	bc11_set_register_value(PMIC_RG_BC11_IPU_EN, 0x2);
	/* RG_bc11_VREF_VTH = [1:0]=01 */
	bc11_set_register_value(PMIC_RG_BC11_VREF_VTH, 0x1);
	/* RG_bc11_CMP_EN[1.0] = 01 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0x1);

	msleep(80);
	/* mdelay(80); */

	wChargerAvail = bc11_get_register_value(PMIC_RGS_BC11_CMP_OUT);

#if 0
	if (Enable_BATDRV_LOG == BAT_LOG_FULL) {
		battery_log(BAT_LOG_FULL, "hw_bc11_stepB2() \r\n");
		hw_bc11_dump_register();
	}
#endif


	if (!wChargerAvail) {
		/* RG_bc11_VSRC_EN[1.0] = 10 */
		/* mt6325_upmu_set_rg_bc11_vsrc_en(0x2); */
		bc11_set_register_value(PMIC_RG_BC11_VSRC_EN, 0x2);
	}
	/* RG_bc11_IPU_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_IPU_EN, 0x0);
	/* RG_bc11_CMP_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0x0);
	/* RG_bc11_VREF_VTH = [1:0]=00 */
	bc11_set_register_value(PMIC_RG_BC11_VREF_VTH, 0x0);


	return wChargerAvail;
}


static void hw_bc11_done(void)
{
	/* RG_bc11_VSRC_EN[1:0]=00 */
	bc11_set_register_value(PMIC_RG_BC11_VSRC_EN, 0x0);
	/* RG_bc11_VREF_VTH = [1:0]=0 */
	bc11_set_register_value(PMIC_RG_BC11_VREF_VTH, 0x0);
	/* RG_bc11_CMP_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_CMP_EN, 0x0);
	/* RG_bc11_IPU_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_IPU_EN, 0x0);
	/* RG_bc11_IPD_EN[1.0] = 00 */
	bc11_set_register_value(PMIC_RG_BC11_IPD_EN, 0x0);
	/* RG_bc11_BIAS_EN=0 */
	bc11_set_register_value(PMIC_RG_BC11_BIAS_EN, 0x0);

#if defined(CONFIG_MU3_PHY)
	Charger_Detect_Release();
#endif
#if 0
	if (Enable_BATDRV_LOG == BAT_LOG_FULL) {
		battery_log(BAT_LOG_FULL, "hw_bc11_done() \r\n");
		hw_bc11_dump_register();
	}
#endif

}

void hw_charging_enable_dp_voltage(int ison)
{
	static int cnt;

	if (ison == 1) {
		if (cnt == 0)
			cnt = 1;
		hw_bc11_init();
		bc11_set_register_value(PMIC_RG_BC11_VSRC_EN, 0x2);
	} else if (cnt == 1) {
		hw_bc11_done();
		cnt = 0;
	}
}

int hw_charging_get_charger_type(void)
{
#if 0
	return STANDARD_HOST;
	/* return STANDARD_CHARGER; //adaptor */
#else
	CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;

#ifdef CONFIG_MTK_USB2JTAG_SUPPORT
	if (usb2jtag_mode()) {
		pr_err("[USB2JTAG] in usb2jtag mode, skip charger detection\n");
		return STANDARD_HOST;
	}
#endif

	/********* Step initial  ***************/
	hw_bc11_init();

	/********* Step DCD ***************/
	if (hw_bc11_DCD() == 1) {
		/********* Step A1 ***************/
		if (hw_bc11_stepA1() == 1) {
			CHR_Type_num = APPLE_2_1A_CHARGER;
			/*battery_log(1, "step A1 : Apple 2.1A CHARGER!\r\n");*/
		} else {
			CHR_Type_num = NONSTANDARD_CHARGER;
			/*battery_log(1, "step A1 : Non STANDARD CHARGER!\r\n");*/
		}
	} else {
		/********* Step A2 ***************/
		if (hw_bc11_stepA2() == 1) {
			/********* Step B2 ***************/
			if (hw_bc11_stepB2() == 1) {
				is_dcp_type = true;
				CHR_Type_num = STANDARD_CHARGER;
				/*battery_log(1, "step B2 : STANDARD CHARGER!\r\n");*/
			} else {
				CHR_Type_num = CHARGING_HOST;
				/*battery_log(1, "step B2 :  Charging Host!\r\n");*/
			}
		} else {
			CHR_Type_num = STANDARD_HOST;
			/*battery_log(1, "step A2 : Standard USB Host!\r\n");*/
		}
	}

    /********* Finally setting *******************************/
	hw_bc11_done();

	return CHR_Type_num;
#endif
}
#endif
