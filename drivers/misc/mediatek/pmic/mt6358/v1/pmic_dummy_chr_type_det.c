/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2016 MediaTek Inc.
 */


#include <generated/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
//#include <linux/pm_wakeup.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/seq_file.h>
#include <linux/power_supply.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>

#include <mt-plat/v1/mtk_battery.h>
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mt-plat/mtk_boot.h>
#include <mt-plat/v1/charger_type.h>

#define __SW_CHRDET_IN_PROBE_PHASE__

static enum charger_type g_chr_type;
#ifdef __SW_CHRDET_IN_PROBE_PHASE__
static struct work_struct chr_work;
#endif

static DEFINE_MUTEX(chrdet_lock);

static struct power_supply *chrdet_psy;
static int chrdet_inform_psy_changed(enum charger_type chg_type,
				bool chg_online)
{
	int ret = 0;
	union power_supply_propval propval;

	pr_debug("charger type: %s: online = %d, type = %d\n",
		__func__, chg_online, chg_type);

	/* Inform chg det power supply */
	if (chg_online) {
		propval.intval = chg_online;
		ret = power_supply_set_property(chrdet_psy,
			POWER_SUPPLY_PROP_ONLINE, &propval);
		if (ret < 0)
			pr_debug("%s: psy online failed, ret = %d\n",
				__func__, ret);

		propval.intval = chg_type;
		ret = power_supply_set_property(chrdet_psy,
			POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
		if (ret < 0)
			pr_debug("%s: psy type failed, ret = %d\n",
				__func__, ret);

		return ret;
	}

	propval.intval = chg_type;
	ret = power_supply_set_property(chrdet_psy,
		POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
	if (ret < 0)
		pr_debug("%s: psy type failed, ret(%d)\n",
			__func__, ret);

	propval.intval = chg_online;
	ret = power_supply_set_property(chrdet_psy,
		POWER_SUPPLY_PROP_ONLINE, &propval);
	if (ret < 0)
		pr_debug("%s: psy online failed, ret(%d)\n",
			__func__, ret);
	return ret;
}

#ifdef CONFIG_MOTO_CHG_BQ25601_SUPPORT
extern int bq25601_start_chg_type_detect(void);
#endif

#ifdef CONFIG_MOTO_CHG_RT9471_SUPPORT
extern int rt9471_start_chg_type_detect(void);
extern int rt9471_flag;
#endif

#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
extern int wt6670f_start_detection(void);
extern int wt6670f_get_protocol(void);
extern void bq2597x_set_psy(void);
#endif

int hw_charging_get_charger_type(void)
{
#ifdef CONFIG_MOTO_CHG_BQ25601_SUPPORT
	int chg_type = 0;

    #ifdef CONFIG_MOTO_CHG_RT9471_SUPPORT
        if(rt9471_flag == 0){
    #endif
	    Charger_Detect_Init();
	    chg_type = bq25601_start_chg_type_detect();
	    Charger_Detect_Release();
	    pr_err("[%s] BQ25601 charge type is  0x%x\n",__func__,chg_type);
	    switch (chg_type) {
		case 0x20:
			return STANDARD_HOST;//SDP
		case 0x40:
			return CHARGING_HOST;//CDP
		case 0x60:
			return STANDARD_CHARGER;//DCP
		case 0xc0:
			return NONSTANDARD_CHARGER;//FC
		default:
			break;
	    }
    #ifdef CONFIG_MOTO_CHG_RT9471_SUPPORT
	} else {
            Charger_Detect_Init();
            chg_type = rt9471_start_chg_type_detect();
	    pr_err("[%s] RT9471 charge type is  0x%x\n",__func__,chg_type);
            Charger_Detect_Release();
            return chg_type;
	}
    #endif
#elif defined(CONFIG_MOTO_CHG_WT6670F_SUPPORT)
        int chg_type = 0;
        Charger_Detect_Init();
        wt6670f_start_detection();
        msleep(3000);
        chg_type = wt6670f_get_protocol();
        pr_err("[%s] WT6670F charge type is  0x%x\n",__func__,chg_type);
        if((chg_type != 0x8) && (chg_type != 0x9)){
            Charger_Detect_Release();
	}

        switch (chg_type) {
            case 0x1:
                return NONSTANDARD_CHARGER;//FC
                break;
            case 0x2:
                return STANDARD_HOST;//SDP
                break;
            case 0x3:
                return CHARGING_HOST;//CDP
                break;
            case 0x4:
            case 0x8://QC3P_18W
            case 0x9://QC3P_27W
                return STANDARD_CHARGER;//DCP
                break;
            default:
                break;
        }
#endif
	
	return STANDARD_HOST;
}

/*****************************************************************************
 * Charger Detection
 ******************************************************************************/
void __attribute__((weak)) mtk_pmic_enable_chr_type_det(bool en)
{
}

void do_charger_detect(void)
{
	if (!mt_usb_is_device()) {
		g_chr_type = CHARGER_UNKNOWN;
		pr_debug("charger type: UNKNOWN, Now is usb host mode. Skip detection!!!\n");
		return;
	}

	mutex_lock(&chrdet_lock);

	if (pmic_get_register_value(PMIC_RGS_CHRDET)) {
		pr_info("charger type: charger IN\n");
		g_chr_type = hw_charging_get_charger_type();
#ifdef CONFIG_MOTO_CHG_WT6670F_SUPPORT
                bq2597x_set_psy();
#endif
		chrdet_inform_psy_changed(g_chr_type, 1);
	} else {
		pr_info("charger type: charger OUT\n");
		g_chr_type = CHARGER_UNKNOWN;
		chrdet_inform_psy_changed(g_chr_type, 0);
	}

	mutex_unlock(&chrdet_lock);
}



/*****************************************************************************
 * PMIC Int Handler
 ******************************************************************************/
void chrdet_int_handler(void)
{
	/*
	 * pr_info("[chrdet_int_handler]CHRDET status = %d....\n",
	 *	pmic_get_register_value(PMIC_RGS_CHRDET));
	 */
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if (!pmic_get_register_value(PMIC_RGS_CHRDET)) {
		int boot_mode = 0;

		boot_mode = get_boot_mode();

		if (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT ||
		    boot_mode == LOW_POWER_OFF_CHARGING_BOOT) {
			pr_info("[%s] Unplug Charger/USB\n", __func__);
#ifndef CONFIG_TCPC_CLASS
			orderly_poweroff(true);
#else
			return;
#endif
		}
	}
#endif
	do_charger_detect();
}


/************************************************
 * Charger Probe Related
 ************************************************/
#ifdef __SW_CHRDET_IN_PROBE_PHASE__
static void do_charger_detection_work(struct work_struct *data)
{
	if (pmic_get_register_value(PMIC_RGS_CHRDET))
		do_charger_detect();
}
#endif

static int __init pmic_chrdet_init(void)
{
	mutex_init(&chrdet_lock);
	chrdet_psy = power_supply_get_by_name("charger");
	if (!chrdet_psy) {
		pr_debug("%s: get power supply failed\n", __func__);
		return -EINVAL;
	}

#ifdef __SW_CHRDET_IN_PROBE_PHASE__
	/* do charger detect here to prevent HW miss interrupt*/
	INIT_WORK(&chr_work, do_charger_detection_work);
	schedule_work(&chr_work);
#endif

	pmic_register_interrupt_callback(INT_CHRDET_EDGE, chrdet_int_handler);
	pmic_enable_interrupt(INT_CHRDET_EDGE, 1, "PMIC");

	return 0;
}

late_initcall(pmic_chrdet_init);
