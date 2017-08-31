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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *    pmic_irq.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 *   This Module defines PMIC functions
 *
 * Author:
 * -------
 * Jimmy-YJ Huang
 *
 ****************************************************************************/
#include <generated/autoconf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#endif
#include <linux/uaccess.h>

#include <mt-plat/upmu_common.h>
#include "pmic.h"
#include "pmic_adb_dbg.h"

#include <mach/mtk_pmic_wrap.h>
#include <mt-plat/mtk_rtc.h>

#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
#include <mt-plat/mtk_boot.h>
#include <mt-plat/mtk_boot_common.h>
/*#include <mach/system.h> TBD*/
#include <mt-plat/mtk_gpt.h>
#endif

#if defined(CONFIG_MTK_SMART_BATTERY)
#include <mt-plat/battery_meter.h>
#include <mt-plat/battery_common.h>
#include <mach/mtk_battery_meter.h>
#endif
#include <mt6311.h>
#include <mach/mtk_pmic.h>
#include <mt-plat/mtk_reboot.h>
#include <mt-plat/aee.h>

/*****************************************************************************
 * Global variable
 ******************************************************************************/
int g_pmic_irq;
unsigned int g_eint_pmic_num = 150;
unsigned int g_cust_eint_mt_pmic_debounce_cn = 1;
unsigned int g_cust_eint_mt_pmic_type = 4;
unsigned int g_cust_eint_mt_pmic_debounce_en = 1;

/*****************************************************************************
 * PMIC extern variable
 ******************************************************************************/
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
static bool long_pwrkey_press;
static unsigned long timer_pre;
static unsigned long timer_pos;
#define LONG_PWRKEY_PRESS_TIME_UNIT     500     /*500ms */
#define LONG_PWRKEY_PRESS_TIME_US       1000000 /*500ms */
#endif

/*****************************************************************************
 * PMIC extern function
 ******************************************************************************/

#ifndef CONFIG_KEYBOARD_MTK /* fix kpd build error only */
void kpd_pwrkey_pmic_handler(unsigned long pressed)
{}
void kpd_pmic_rstkey_handler(unsigned long pressed)
{}
#endif

/*****************************************************************************
 * interrupt Setting
 ******************************************************************************/
static struct pmic_interrupt_bit interrupt_status0[] = {
	PMIC_S_INT_GEN(RG_INT_STATUS_PWRKEY),
	PMIC_S_INT_GEN(RG_INT_STATUS_HOMEKEY),
	PMIC_S_INT_GEN(RG_INT_STATUS_PWRKEY_R),
	PMIC_S_INT_GEN(RG_INT_STATUS_HOMEKEY_R),
	PMIC_S_INT_GEN(RG_INT_STATUS_THR_H),
	PMIC_S_INT_GEN(RG_INT_STATUS_THR_L),
	PMIC_S_INT_GEN(RG_INT_STATUS_BAT_H),
	PMIC_S_INT_GEN(RG_INT_STATUS_BAT_L),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(RG_INT_STATUS_RTC),
	PMIC_S_INT_GEN(RG_INT_STATUS_AUDIO),
	PMIC_S_INT_GEN(RG_INT_STATUS_MAD),
	PMIC_S_INT_GEN(RG_INT_STATUS_ACCDET),
	PMIC_S_INT_GEN(RG_INT_STATUS_ACCDET_EINT),
	PMIC_S_INT_GEN(RG_INT_STATUS_ACCDET_NEGV),
	PMIC_S_INT_GEN(RG_INT_STATUS_NI_LBAT_INT),
};

static struct pmic_interrupt_bit interrupt_status1[] = {
	PMIC_S_INT_GEN(RG_INT_STATUS_VCORE_OC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VGPU_OC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VSRAM_MD_OC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VMODEM_OC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VM1_OC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VS1_OC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VS2_OC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VPA_OC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VCORE_PREOC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VGPU_PREOC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VSRAM_MD_PREOC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VMODEM_PREOC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VM1_PREOC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VS1_PREOC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VS2_PREOC),
	PMIC_S_INT_GEN(RG_INT_STATUS_LDO_OC),
};

static struct pmic_interrupt_bit interrupt_status2[] = {
	PMIC_S_INT_GEN(RG_INT_STATUS_JEITA_HOT),
	PMIC_S_INT_GEN(RG_INT_STATUS_JEITA_WARM),
	PMIC_S_INT_GEN(RG_INT_STATUS_JEITA_COOL),
	PMIC_S_INT_GEN(RG_INT_STATUS_JEITA_COLD),
	PMIC_S_INT_GEN(RG_INT_STATUS_AUXADC_IMP),
	PMIC_S_INT_GEN(RG_INT_STATUS_NAG_C_DLTV),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(RG_INT_STATUS_OV),
	PMIC_S_INT_GEN(RG_INT_STATUS_BVALID_DET),
	PMIC_S_INT_GEN(RG_INT_STATUS_RGS_BATON_HV),
	PMIC_S_INT_GEN(RG_INT_STATUS_VBATON_UNDET),
	PMIC_S_INT_GEN(RG_INT_STATUS_WATCHDOG),
	PMIC_S_INT_GEN(RG_INT_STATUS_PCHR_CM_VDEC),
	PMIC_S_INT_GEN(RG_INT_STATUS_CHRDET),
	PMIC_S_INT_GEN(RG_INT_STATUS_PCHR_CM_VINC),
};

static struct pmic_interrupt_bit interrupt_status3[] = {
	PMIC_S_INT_GEN(RG_INT_STATUS_FG_BAT_H),
	PMIC_S_INT_GEN(RG_INT_STATUS_FG_BAT_L),
	PMIC_S_INT_GEN(RG_INT_STATUS_FG_CUR_H),
	PMIC_S_INT_GEN(RG_INT_STATUS_FG_CUR_L),
	PMIC_S_INT_GEN(RG_INT_STATUS_FG_ZCV),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
};

struct pmic_interrupts interrupts[] = {
	PMIC_M_INTS_GEN(MT6351_INT_STATUS0, MT6351_INT_CON0, MT6351_INT_CON0_SET,
			MT6351_INT_CON0_CLR, interrupt_status0),
	PMIC_M_INTS_GEN(MT6351_INT_STATUS1, MT6351_INT_CON1, MT6351_INT_CON1_SET,
			MT6351_INT_CON1_CLR, interrupt_status1),
	PMIC_M_INTS_GEN(MT6351_INT_STATUS2, MT6351_INT_CON2, MT6351_INT_CON2_SET,
			MT6351_INT_CON2_CLR, interrupt_status2),
	PMIC_M_INTS_GEN(MT6351_INT_STATUS3, MT6351_INT_CON3, MT6351_INT_CON3_SET,
			MT6351_INT_CON3_CLR, interrupt_status3),
};

int interrupts_size = ARRAY_SIZE(interrupts);

/*****************************************************************************
 * PWRKEY Int Handler
 ******************************************************************************/
void pwrkey_int_handler(void)
{

	PMICLOG("[pwrkey_int_handler] Press pwrkey %d\n",
		pmic_get_register_value(PMIC_PWRKEY_DEB));

#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
	if (get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT)
		timer_pre = sched_clock();
#endif
#if defined(CONFIG_FPGA_EARLY_PORTING)
#else
	kpd_pwrkey_pmic_handler(0x1);
#endif
}

void pwrkey_int_handler_r(void)
{
	PMICLOG("[pwrkey_int_handler_r] Release pwrkey %d\n",
		pmic_get_register_value(PMIC_PWRKEY_DEB));
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
	if (get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT && timer_pre != 0) {
		timer_pos = sched_clock();
		if (timer_pos - timer_pre >=
		    LONG_PWRKEY_PRESS_TIME_UNIT * LONG_PWRKEY_PRESS_TIME_US) {
			long_pwrkey_press = true;
		}
		PMICLOG
		    ("timer_pos = %ld, timer_pre = %ld, timer_pos-timer_pre = %ld, long_pwrkey_press = %d\r\n",
		     timer_pos, timer_pre, timer_pos - timer_pre, long_pwrkey_press);
		if (long_pwrkey_press) {	/*500ms */
			PMICLOG
			    ("Power Key Pressed during kernel power off charging, reboot OS\r\n");
			arch_reset(0, NULL);
		}
	}
#endif
#if defined(CONFIG_FPGA_EARLY_PORTING)
#else
	kpd_pwrkey_pmic_handler(0x0);
#endif
}

/*****************************************************************************
 * Homekey Int Handler
 ******************************************************************************/
void homekey_int_handler(void)
{
	PMICLOG("[homekey_int_handler] Press homekey %d\n",
		pmic_get_register_value(PMIC_HOMEKEY_DEB));
#if defined(CONFIG_FPGA_EARLY_PORTING)
#else
	kpd_pmic_rstkey_handler(0x1);
#endif
}

void homekey_int_handler_r(void)
{
	PMICLOG("[homekey_int_handler_r] Release homekey %d\n",
		pmic_get_register_value(PMIC_HOMEKEY_DEB));
#if defined(CONFIG_FPGA_EARLY_PORTING)
#else
	kpd_pmic_rstkey_handler(0x0);
#endif
}

/*****************************************************************************
 * Chrdet Int Handler
 ******************************************************************************/
void chrdet_int_handler(void)
{
	PMICLOG("[chrdet_int_handler]CHRDET status = %d....\n",
		pmic_get_register_value(PMIC_RGS_CHRDET));

#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if (!upmu_get_rgs_chrdet()) {
		int boot_mode = 0;

		boot_mode = get_boot_mode();

		if (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT
		    || boot_mode == LOW_POWER_OFF_CHARGING_BOOT) {
			PMICLOG("[chrdet_int_handler] Unplug Charger/USB\n");
			mt_power_off();
		}
	}
#endif
	pmic_set_register_value(PMIC_RG_USBDL_RST, 1);
#if defined(CONFIG_MTK_SMART_BATTERY)
	do_chrdet_int_task();
#endif

}

/*****************************************************************************
 * Auxadc Int Handler
 ******************************************************************************/
void auxadc_imp_int_handler_r(void)
{
	PMICLOG("auxadc_imp_int_handler_r() =%d\n",
		pmic_get_register_value(PMIC_AUXADC_ADC_OUT_IMP));
	/*clear IRQ */
	pmic_set_register_value(PMIC_AUXADC_CLR_IMP_CNT_STOP, 1);
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_CLR, 1);
	/*restore to initial state */
	pmic_set_register_value(PMIC_AUXADC_CLR_IMP_CNT_STOP, 0);
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_CLR, 0);
	/*turn off interrupt */
	pmic_set_register_value(PMIC_RG_INT_EN_AUXADC_IMP, 0);

}

/*****************************************************************************
 * Ldo OC Int Handler
 ******************************************************************************/
#define HANDLE_LDO_OC_IRQ 0 /* not register and handle LDO OC irq */
#if HANDLE_LDO_OC_IRQ
void ldo_oc_int_handler(void)
{
	if (pmic_get_register_value(PMIC_OC_STATUS_VMCH))
		msdc_sd_power_off();
}
#endif
/*****************************************************************************
 * General OC Int Handler
 ******************************************************************************/
void oc_int_handler(PMIC_IRQ_ENUM intNo, const char *int_name)
{
	static unsigned int vcore_pre_oc_times;
	static unsigned int vgpu_pre_oc_times;

	PMICLOG("[general_oc_int_handler] int name=%s\n", int_name);
	if (intNo == INT_VCORE_PREOC) {
		/* keep OC interrupt and keep tracking */
		pr_err(PMICTAG "[PMIC_INT] PMIC OC: %s (%d times)\n", int_name, ++vcore_pre_oc_times);
	} else if (intNo == INT_VGPU_PREOC) {
		/* keep OC interrupt and keep tracking */
		pr_err(PMICTAG "[PMIC_INT] PMIC OC: %s (%d times)\n", int_name, ++vgpu_pre_oc_times);
	} else {
		/* issue AEE exception and disable OC interrupt */
		kernel_dump_exception_reg();
		aee_kernel_warning("PMIC OC", "\nCRDISPATCH_KEY:PMIC OC\nOC Interrupt: %s", int_name);
		pmic_enable_interrupt(intNo, 0, "PMIC");
		pr_err(PMICTAG "[PMIC_INT] disable OC interrupt: %s\n", int_name);
	}
}
/*****************************************************************************
 * Low battery call back function
 ******************************************************************************/
#define LBCB_NUM 16

#ifndef DISABLE_LOW_BATTERY_PROTECT
#define LOW_BATTERY_PROTECT
#endif

#ifdef LOW_BATTERY_PROTECT
/* ex. 3400/5400*4096*/
#define BAT_HV_THD   (POWER_INT0_VOLT*4096/5400)	/*ex: 3400mV*/
#define BAT_LV_1_THD (POWER_INT1_VOLT*4096/5400)	/*ex: 3250mV*/
#define BAT_LV_2_THD (POWER_INT2_VOLT*4096/5400)	/*ex: 3000mV*/
#endif

void bat_h_int_handler(void)
{
	g_lowbat_int_bottom = 0;

	PMICLOG("[bat_h_int_handler]....\n");

	/*sub-task*/
#ifdef LOW_BATTERY_PROTECT
	g_low_battery_level = 0;
	exec_low_battery_callback(LOW_BATTERY_LEVEL_0);

#if 0
	lbat_max_en_setting(0);
	mdelay(1);
	lbat_min_en_setting(1);
#else
	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MIN, BAT_LV_1_THD);

	lbat_min_en_setting(0);
	lbat_max_en_setting(0);
	mdelay(1);
	lbat_min_en_setting(1);
#endif

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_AUXADC_LBAT3, upmu_get_reg_value(MT6351_AUXADC_LBAT3),
		MT6351_AUXADC_LBAT4, upmu_get_reg_value(MT6351_AUXADC_LBAT4),
		MT6351_INT_CON0, upmu_get_reg_value(MT6351_INT_CON0)
	    );
#endif

}

void bat_l_int_handler(void)
{
	PMICLOG("[bat_l_int_handler]....\n");

	/*sub-task*/
#ifdef LOW_BATTERY_PROTECT
	g_low_battery_level++;
	if (g_low_battery_level > 2)
		g_low_battery_level = 2;

	if (g_low_battery_level == 1)
		exec_low_battery_callback(LOW_BATTERY_LEVEL_1);
	else if (g_low_battery_level == 2) {
		exec_low_battery_callback(LOW_BATTERY_LEVEL_2);
		g_lowbat_int_bottom = 1;
	} else
		PMICLOG("[bat_l_int_handler]err,g_low_battery_level=%d\n", g_low_battery_level);

#if 0
	lbat_min_en_setting(0);
	mdelay(1);
	lbat_max_en_setting(1);
#else

	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MIN, BAT_LV_2_THD);

	lbat_min_en_setting(0);
	lbat_max_en_setting(0);
	mdelay(1);
	if (g_low_battery_level < 2)
		lbat_min_en_setting(1);
	lbat_max_en_setting(1);
#endif

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_AUXADC_LBAT3, upmu_get_reg_value(MT6351_AUXADC_LBAT3),
		MT6351_AUXADC_LBAT4, upmu_get_reg_value(MT6351_AUXADC_LBAT4),
		MT6351_INT_CON0, upmu_get_reg_value(MT6351_INT_CON0)
	    );
#endif

}

/*****************************************************************************
 * Battery OC call back function
 ******************************************************************************/

#ifndef DISABLE_BATTERY_OC_PROTECT
#define BATTERY_OC_PROTECT
#endif

void fg_cur_h_int_handler(void)
{
	PMICLOG("[fg_cur_h_int_handler]....\n");

	/*sub-task*/
#ifdef BATTERY_OC_PROTECT
	g_battery_oc_level = 0;
	exec_battery_oc_callback(BATTERY_OC_LEVEL_0);
	bat_oc_h_en_setting(0);
	bat_oc_l_en_setting(0);
	mdelay(1);
	bat_oc_l_en_setting(1);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_PMIC_FG_CUR_HTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_HTH_ADDR),
		MT6351_PMIC_FG_CUR_LTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_LTH_ADDR),
		MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR, upmu_get_reg_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR)
	    );
#endif
}

void fg_cur_l_int_handler(void)
{
	PMICLOG("[fg_cur_l_int_handler]....\n");

	/*sub-task*/
#ifdef BATTERY_OC_PROTECT
	g_battery_oc_level = 1;
	exec_battery_oc_callback(BATTERY_OC_LEVEL_1);
	bat_oc_h_en_setting(0);
	bat_oc_l_en_setting(0);
	mdelay(1);
	bat_oc_h_en_setting(1);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_PMIC_FG_CUR_HTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_HTH_ADDR),
		MT6351_PMIC_FG_CUR_LTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_LTH_ADDR),
		MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR, upmu_get_reg_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR)
	    );
#endif
}

/*****************************************************************************
 * PMIC Interrupt service
 ******************************************************************************/
static DEFINE_MUTEX(pmic_mutex);
struct task_struct *pmic_thread_handle;

#if !defined CONFIG_HAS_WAKELOCKS
struct wakeup_source pmicThread_lock;
#else
struct wake_lock pmicThread_lock;
#endif

void wake_up_pmic(void)
{
	PMICLOG("[wake_up_pmic]\r\n");
	if (pmic_thread_handle != NULL)
		wake_up_process(pmic_thread_handle);

#if !defined CONFIG_HAS_WAKELOCKS
	__pm_stay_awake(&pmicThread_lock);
#else
	wake_lock(&pmicThread_lock);
#endif
}
EXPORT_SYMBOL(wake_up_pmic);

irqreturn_t mt_pmic_eint_irq(int irq, void *desc)
{
	PMICLOG("[mt_pmic_eint_irq] receive interrupt\n");
	disable_irq_nosync(irq);
	wake_up_pmic();
	return IRQ_HANDLED;
}

void pmic_enable_interrupt(PMIC_IRQ_ENUM intNo, unsigned int en, char *str)
{
	unsigned int shift, no;

	shift = intNo / PMIC_INT_WIDTH;
	no = intNo % PMIC_INT_WIDTH;

	if (shift >= ARRAY_SIZE(interrupts)) {
		PMICLOG("[pmic_enable_interrupt] fail intno=%d \r\n", intNo);
		return;
	}

	PMICLOG("[pmic_enable_interrupt] intno=%d en=%d str=%s shf=%d no=%d [0x%x]=0x%x\r\n",
		intNo, en, str, shift, no, interrupts[shift].en,
		upmu_get_reg_value(interrupts[shift].en));

	if (en == 1)
		pmic_config_interface(interrupts[shift].set, 0x1, 0x1, no);
	else if (en == 0)
		pmic_config_interface(interrupts[shift].clear, 0x1, 0x1, no);

	PMICLOG("[pmic_enable_interrupt] after [0x%x]=0x%x\r\n",
		interrupts[shift].en, upmu_get_reg_value(interrupts[shift].en));

}

void pmic_register_interrupt_callback(PMIC_IRQ_ENUM intNo, void (EINT_FUNC_PTR) (void))
{
	unsigned int shift, no;

	shift = intNo / PMIC_INT_WIDTH;
	no = intNo % PMIC_INT_WIDTH;

	if (shift >= ARRAY_SIZE(interrupts)) {
		PMICLOG("[pmic_register_interrupt_callback] fail intno=%d\r\n", intNo);
		return;
	}

	PMICLOG("[pmic_register_interrupt_callback] intno=%d\r\n", intNo);

	interrupts[shift].interrupts[no].callback = EINT_FUNC_PTR;

}

#define ENABLE_ALL_OC_IRQ 0

/* register general oc interrupt handler */
void pmic_register_oc_interrupt_callback(PMIC_IRQ_ENUM intNo)
{
	unsigned int shift, no;

	shift = intNo / PMIC_INT_WIDTH;
	no = intNo % PMIC_INT_WIDTH;

	if (shift >= ARRAY_SIZE(interrupts)) {
		pr_err(PMICTAG "[pmic_register_oc_interrupt_callback] fail intno=%d\r\n", intNo);
		return;
	}

	PMICLOG("[pmic_register_oc_interrupt_callback] intno=%d\r\n", intNo);

	interrupts[shift].interrupts[no].oc_callback = oc_int_handler;

}

/* register and enable all oc interrupt */
void register_all_oc_interrupts(void)
{
	PMIC_IRQ_ENUM oc_interrupt = INT_VCORE_OC;

	for (; oc_interrupt <= INT_VS2_PREOC; oc_interrupt++) {
		if (oc_interrupt == INT_VPA_OC) {
			/* Skip VPA OC.
			 * ALPS02729620: 3G
			 * ALPS02728523: 4G
			 * ALPS02728059: SRLTE
			 */
			PMICLOG("[%s] skip VPA OC\r\n", __func__);
		} else {
			pmic_register_oc_interrupt_callback(oc_interrupt);
			pmic_enable_interrupt(oc_interrupt, 1, "PMIC");
		}
	}
}

void PMIC_EINT_SETTING(struct device_node *np)
{
	struct device_node *pmic_irq_node = NULL;
	int ret = 0;

	upmu_set_reg_value(MT6351_INT_CON0, 0);
	upmu_set_reg_value(MT6351_INT_CON1, 0);
	upmu_set_reg_value(MT6351_INT_CON2, 0);
	upmu_set_reg_value(MT6351_INT_CON3, 0);

	/*enable pwrkey/homekey interrupt */
	upmu_set_reg_value(MT6351_INT_CON0_SET, 0xf);

	/*for all interrupt events, turn on interrupt module clock */
	pmic_set_register_value(PMIC_RG_INTRP_CK_PDN, 0);

	/*For BUCK OC related interrupt, please turn on pwmoc_6m_ck (6MHz) */
	pmic_set_register_value(PMIC_RG_PWMOC_6M_CK_PDN, 0);

	pmic_register_interrupt_callback(0, pwrkey_int_handler);
	pmic_register_interrupt_callback(1, homekey_int_handler);
	pmic_register_interrupt_callback(2, pwrkey_int_handler_r);
	pmic_register_interrupt_callback(3, homekey_int_handler_r);

	pmic_register_interrupt_callback(6, bat_h_int_handler);
	pmic_register_interrupt_callback(7, bat_l_int_handler);
#if HANDLE_LDO_OC_IRQ
	pmic_register_interrupt_callback(31, ldo_oc_int_handler);
#endif
	pmic_register_interrupt_callback(46, chrdet_int_handler);

	pmic_register_interrupt_callback(50, fg_cur_h_int_handler);
	pmic_register_interrupt_callback(51, fg_cur_l_int_handler);

	pmic_enable_interrupt(0, 1, "PMIC");
	pmic_enable_interrupt(1, 1, "PMIC");
	pmic_enable_interrupt(2, 1, "PMIC");
	pmic_enable_interrupt(3, 1, "PMIC");
#ifdef LOW_BATTERY_PROTECT
	/* move to lbat_xxx_en_setting */
#else
	/* pmic_enable_interrupt(6, 1, "PMIC"); */
	/* pmic_enable_interrupt(7, 1, "PMIC"); */
#endif
	/*pmic_enable_interrupt(31, 1, "PMIC");*/
	pmic_enable_interrupt(46, 1, "PMIC");
#ifdef BATTERY_OC_PROTECT
	/* move to bat_oc_x_en_setting */
#else
	/* pmic_enable_interrupt(50, 1, "PMIC"); */
	/* pmic_enable_interrupt(51, 1, "PMIC"); */
#endif

#if ENABLE_ALL_OC_IRQ
	register_all_oc_interrupts();
#endif

	pmic_irq_node = of_get_child_by_name(np, "pmic_irq");
	if (pmic_irq_node) {
		g_pmic_irq = irq_of_parse_and_map(pmic_irq_node, 0);
		ret = request_irq(g_pmic_irq, (irq_handler_t) mt_pmic_eint_irq, IRQF_TRIGGER_NONE, "pmic-eint", NULL);
		if (ret > 0)
			PMICLOG("EINT IRQ LINENNOT AVAILABLE\n");
		enable_irq_wake(g_pmic_irq); /* let pmic irq can wake up suspend */
	} else
		PMICLOG("can't find compatible node\n");

	PMICLOG("[CUST_EINT] CUST_EINT_MT_PMIC_MT6351_NUM=%d\n", g_eint_pmic_num);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_CN=%d\n", g_cust_eint_mt_pmic_debounce_cn);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_TYPE=%d\n", g_cust_eint_mt_pmic_type);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_EN=%d\n", g_cust_eint_mt_pmic_debounce_en);

}

static void pmic_int_handler(void)
{
	unsigned char i, j;
	unsigned int ret;

	for (i = 0; i < ARRAY_SIZE(interrupts); i++) {
		unsigned int int_status_val = 0;

		int_status_val = upmu_get_reg_value(interrupts[i].address);
		pr_err("[PMIC_INT] addr[0x%x]=0x%x\n", interrupts[i].address, int_status_val);

		for (j = 0; j < PMIC_INT_WIDTH; j++) {
			if ((int_status_val) & (1 << j)) {
				PMICLOG("[PMIC_INT][%s]\n", interrupts[i].interrupts[j].name);
				interrupts[i].interrupts[j].times++;

				if (interrupts[i].interrupts[j].callback != NULL)
					interrupts[i].interrupts[j].callback();
				if (interrupts[i].interrupts[j].oc_callback != NULL) {
					interrupts[i].interrupts[j].oc_callback((i * PMIC_INT_WIDTH + j),
										interrupts[i].interrupts[j].name);
				}
				ret = pmic_config_interface(interrupts[i].address, 0x1, 0x1, j);
			}
		}
	}
}

int pmic_thread_kthread(void *x)
{
	unsigned int i;
	unsigned int int_status_val = 0;
	unsigned int pwrap_eint_status = 0;
	struct sched_param param = {.sched_priority = 98 };

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);

	PMICLOG("[PMIC_INT] enter\n");

	pmic_enable_charger_detection_int(0);

	/* Run on a process content */
	while (1) {
		mutex_lock(&pmic_mutex);

		pwrap_eint_status = pmic_wrap_eint_status();
		PMICLOG("[PMIC_INT] pwrap_eint_status=0x%x\n", pwrap_eint_status);

		pmic_int_handler();

		pmic_wrap_eint_clr(0x0);
		/*PMICLOG("[PMIC_INT] pmic_wrap_eint_clr(0x0);\n");*/

		for (i = 0; i < ARRAY_SIZE(interrupts); i++) {
			int_status_val = upmu_get_reg_value(interrupts[i].address);
			PMICLOG("[PMIC_INT] after ,int_status_val[0x%x]=0x%x\n",
				interrupts[i].address, int_status_val);
		}


		mdelay(1);

		mutex_unlock(&pmic_mutex);
#if !defined CONFIG_HAS_WAKELOCKS
		__pm_relax(&pmicThread_lock);
#else
		wake_unlock(&pmicThread_lock);
#endif

		set_current_state(TASK_INTERRUPTIBLE);
		if (g_pmic_irq != 0)
			enable_irq(g_pmic_irq);
		schedule();
	}

	return 0;
}


MODULE_AUTHOR("Jimmy-YJ Huang");
MODULE_DESCRIPTION("MT PMIC Interrupt Driver");
MODULE_LICENSE("GPL");
