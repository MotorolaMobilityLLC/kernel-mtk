/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */


/* May be used */
#if 0
#include <mt-plat/mtk_rtc.h>
#include <mach/mtk_spm_mtcmos.h>
#endif

#include <generated/autoconf.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_irq.h>
#include <linux/sched.h>

#include <mt-plat/aee.h>
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_reboot.h>
#include <mach/mtk_pmic_wrap.h>
#include <mach/mtk_pmic.h>
#include "include/pmic.h"
#include "include/pmic_irq.h"
#include "include/pmic_throttling_dlpt.h"
#include "include/mtk_pmic_common.h"

#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
#include <mt-plat/mtk_boot_common.h>
/*#include <mt-plat/mtk_boot.h> TBD*/
/*#include <mt-plat/mtk_gpt.h> TBD*/
#endif

#if defined(CONFIG_MTK_SMART_BATTERY)
#include <mt-plat/battery_common.h>
/*#include <mach/mtk_battery_meter.h> TBD*/
/*#include <mt-plat/battery_meter.h> TBD*/
#endif

/*---IPI Mailbox define---*/
/*#define IPIMB*/

/* Global variable */
int g_pmic_irq;
unsigned int g_eint_pmic_num = 176;	/* TBD */
unsigned int g_cust_eint_mt_pmic_debounce_cn = 1;
unsigned int g_cust_eint_mt_pmic_type = 4;
unsigned int g_cust_eint_mt_pmic_debounce_en = 1;

/* PMIC extern variable */
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
static bool long_pwrkey_press;
static unsigned long timer_pre;
static unsigned long timer_pos;
#define LONG_PWRKEY_PRESS_TIME_UNIT     500     /*500ms */
#define LONG_PWRKEY_PRESS_TIME_US       1000000 /*500ms */
#endif

#define IRQ_HANDLER_READY 0

/* Interrupt Setting */
static struct pmic_sp_irq psc_irqs[][PMIC_INT_WIDTH] = {
	{
		PMIC_SP_IRQ_GEN(1, INT_PWRKEY),
		PMIC_SP_IRQ_GEN(1, INT_HOMEKEY),
		PMIC_SP_IRQ_GEN(1, INT_PWRKEY_R),
		PMIC_SP_IRQ_GEN(1, INT_HOMEKEY_R),
		PMIC_SP_IRQ_GEN(1, INT_NI_LBAT_INT),
		PMIC_SP_IRQ_GEN(1, INT_CHRDET),
		PMIC_SP_IRQ_GEN(1, INT_CHRDET_EDGE),
		PMIC_SP_IRQ_GEN(1, INT_VCDT_HV_DET),
		PMIC_SP_IRQ_GEN(1, INT_PCHR_CM_VINC),
		PMIC_SP_IRQ_GEN(1, INT_PCHR_CM_VDEC),
		PMIC_SP_IRQ_GEN(1, INT_WATCHDOG),
		PMIC_SP_IRQ_GEN(1, INT_VBATON_UNDET),
		PMIC_SP_IRQ_GEN(1, INT_BVALID_DET),
		PMIC_SP_IRQ_GEN(1, INT_OV),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
};

static struct pmic_sp_irq bm_irqs[][PMIC_INT_WIDTH] = {
	{
		PMIC_SP_IRQ_GEN(1, INT_FG_BAT0_H),
		PMIC_SP_IRQ_GEN(1, INT_FG_BAT0_L),
		PMIC_SP_IRQ_GEN(1, INT_FG_CUR_H),
		PMIC_SP_IRQ_GEN(1, INT_FG_CUR_L),
		PMIC_SP_IRQ_GEN(1, INT_FG_ZCV),
		PMIC_SP_IRQ_GEN(1, INT_FG_BAT1_H),
		PMIC_SP_IRQ_GEN(1, INT_FG_BAT1_L),
		PMIC_SP_IRQ_GEN(1, INT_FG_N_CHARGE_L),
		PMIC_SP_IRQ_GEN(1, INT_FG_IAVG_H),
		PMIC_SP_IRQ_GEN(1, INT_FG_IAVG_L),
		PMIC_SP_IRQ_GEN(1, INT_FG_TIME_H),
		PMIC_SP_IRQ_GEN(1, INT_FG_DISCHARGE),
		PMIC_SP_IRQ_GEN(1, INT_FG_CHARGE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
	{
		PMIC_SP_IRQ_GEN(1, INT_BATON_LV),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(1, INT_BATON_BAT_IN),
		PMIC_SP_IRQ_GEN(1, INT_BATON_BAT_OUT),
		PMIC_SP_IRQ_GEN(1, INT_BIF),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
};

static struct pmic_sp_irq sck_irqs[][PMIC_INT_WIDTH] = {
	{
		PMIC_SP_IRQ_GEN(1, INT_RTC),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
};

static struct pmic_sp_irq hk_irqs[][PMIC_INT_WIDTH] = {
	{
		PMIC_SP_IRQ_GEN(1, INT_THR_H),
		PMIC_SP_IRQ_GEN(1, INT_THR_L),
		PMIC_SP_IRQ_GEN(1, INT_BAT_H),
		PMIC_SP_IRQ_GEN(1, INT_BAT_L),
		PMIC_SP_IRQ_GEN(1, INT_BAT2_H),
		PMIC_SP_IRQ_GEN(1, INT_BAT2_L),
		PMIC_SP_IRQ_GEN(1, INT_BAT_TEMP_H),
		PMIC_SP_IRQ_GEN(1, INT_BAT_TEMP_L),
		PMIC_SP_IRQ_GEN(1, INT_AUXADC_IMP),
		PMIC_SP_IRQ_GEN(1, INT_NAG_C_DLTV),
		PMIC_SP_IRQ_GEN(1, INT_JEITA_HOT),
		PMIC_SP_IRQ_GEN(1, INT_JEITA_WARM),
		PMIC_SP_IRQ_GEN(1, INT_JEITA_COOL),
		PMIC_SP_IRQ_GEN(1, INT_JEITA_COLD),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
	{
		PMIC_SP_IRQ_GEN(1, INT_TYPEC_H_MAX),
		PMIC_SP_IRQ_GEN(1, INT_TYPEC_H_MIN),
		PMIC_SP_IRQ_GEN(1, INT_TYPEC_L_MAX),
		PMIC_SP_IRQ_GEN(1, INT_TYPEC_L_MIN),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
};

static struct pmic_sp_irq xpp_irqs[][PMIC_INT_WIDTH] = {
	{
		PMIC_SP_IRQ_GEN(1, INT_TYPE_C_SINK),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
};

static struct pmic_sp_irq buck_irqs[][PMIC_INT_WIDTH] = {
	{
		PMIC_SP_IRQ_GEN(1, INT_VPROC_OC),
		PMIC_SP_IRQ_GEN(1, INT_VCORE_OC),
		PMIC_SP_IRQ_GEN(1, INT_VMODEM_OC),
		PMIC_SP_IRQ_GEN(1, INT_VS1_OC),
		PMIC_SP_IRQ_GEN(1, INT_VS2_OC),
		PMIC_SP_IRQ_GEN(1, INT_VPA_OC),
		PMIC_SP_IRQ_GEN(1, INT_VCORE_PREOC),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
};

static struct pmic_sp_irq ldo_irqs[][PMIC_INT_WIDTH] = {
	{
		PMIC_SP_IRQ_GEN(1, INT_VFE28_OC),
		PMIC_SP_IRQ_GEN(1, INT_VXO22_OC),
		PMIC_SP_IRQ_GEN(1, INT_VRF18_OC),
		PMIC_SP_IRQ_GEN(1, INT_VRF12_OC),
		PMIC_SP_IRQ_GEN(1, INT_VMIPI_OC),
		PMIC_SP_IRQ_GEN(1, INT_VCN33_OC),
		PMIC_SP_IRQ_GEN(1, INT_VCN28_OC),
		PMIC_SP_IRQ_GEN(1, INT_VCN18_OC),
		PMIC_SP_IRQ_GEN(1, INT_VCAMA_OC),
		PMIC_SP_IRQ_GEN(1, INT_VCAMD_OC),
		PMIC_SP_IRQ_GEN(1, INT_VCAMIO_OC),
		PMIC_SP_IRQ_GEN(1, INT_VLDO28_OC),
		PMIC_SP_IRQ_GEN(1, INT_VA12_OC),
		PMIC_SP_IRQ_GEN(1, INT_VAUX18_OC),
		PMIC_SP_IRQ_GEN(1, INT_VAUD28_OC),
		PMIC_SP_IRQ_GEN(1, INT_VIO28_OC),
	},
	{
		PMIC_SP_IRQ_GEN(1, INT_VIO18_OC),
		PMIC_SP_IRQ_GEN(1, INT_VSRAM_PROC_OC),
		PMIC_SP_IRQ_GEN(1, INT_VSRAM_OTHERS_OC),
		PMIC_SP_IRQ_GEN(1, INT_VSRAM_GPU_OC),
		PMIC_SP_IRQ_GEN(1, INT_VDRAM_OC),
		PMIC_SP_IRQ_GEN(1, INT_VMC_OC),
		PMIC_SP_IRQ_GEN(1, INT_VMCH_OC),
		PMIC_SP_IRQ_GEN(1, INT_VEMC_OC),
		PMIC_SP_IRQ_GEN(1, INT_VSIM1_OC),
		PMIC_SP_IRQ_GEN(1, INT_VSIM2_OC),
		PMIC_SP_IRQ_GEN(1, INT_VIBR_OC),
		PMIC_SP_IRQ_GEN(1, INT_VUSB_OC),
		PMIC_SP_IRQ_GEN(1, INT_VBIF28_OC),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
};

static struct pmic_sp_irq aud_irqs[][PMIC_INT_WIDTH] = {
	{
		PMIC_SP_IRQ_GEN(1, INT_AUDIO),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(1, INT_ACCDET),
		PMIC_SP_IRQ_GEN(1, INT_ACCDET_EINT),
		PMIC_SP_IRQ_GEN(1, INT_ACCDET_EINT1),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
};

static struct pmic_sp_irq misc_irqs[][PMIC_INT_WIDTH] = {
	{
		PMIC_SP_IRQ_GEN(1, INT_EINT_RTC32K_1V8_1),
		PMIC_SP_IRQ_GEN(1, INT_SPI_CMD_ALERT),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
		PMIC_SP_IRQ_GEN(0, NO_USE),
	},
};

struct pmic_sp_interrupt sp_interrupts[] = {
	PMIC_SP_INTS_GEN(PSC_TOP, 1, psc_irqs, 2),
	PMIC_SP_INTS_GEN(BM_TOP, 2, bm_irqs, 4),
	PMIC_SP_INTS_GEN(SCK_TOP, 1, sck_irqs, 3),
	PMIC_SP_INTS_GEN(HK_TOP, 2, hk_irqs, 5),
	PMIC_SP_INTS_GEN(XPP_TOP, 1, xpp_irqs, 6),
	PMIC_SP_INTS_GEN(BUCK_TOP, 1, buck_irqs, 0),
	PMIC_SP_INTS_GEN(LDO_TOP, 2, ldo_irqs, 1),
	PMIC_SP_INTS_GEN(AUD_TOP, 1, aud_irqs, 7),
	PMIC_SP_INTS_GEN(MISC_TOP, 1, misc_irqs, 8),
};

unsigned int sp_interrupt_size = ARRAY_SIZE(sp_interrupts);

#if IRQ_HANDLER_READY
/* PWRKEY Int Handler */
void pwrkey_int_handler(void)
{
	PMICLOG("[pwrkey_int_handler] Press pwrkey %d\n",
		pmic_get_register_value(PMIC_PWRKEY_DEB));
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
	if (get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT)
		timer_pre = sched_clock();
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
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
		if (timer_pos - timer_pre >= LONG_PWRKEY_PRESS_TIME_UNIT * LONG_PWRKEY_PRESS_TIME_US)
			long_pwrkey_press = true;
		PMICLOG("timer_pos = %ld, timer_pre = %ld, timer_pos-timer_pre = %ld, long_pwrkey_press = %d\r\n",
			timer_pos, timer_pre, timer_pos - timer_pre, long_pwrkey_press);
		if (long_pwrkey_press) {	/*500ms */
			PMICLOG("Power Key Pressed during kernel power off charging, reboot OS\r\n");
			arch_reset(0, NULL);
		}
	}
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	kpd_pwrkey_pmic_handler(0x0);
#endif
}

/* Homekey Int Handler */
void homekey_int_handler(void)
{
	PMICLOG("[homekey_int_handler] Press homekey %d\n",
		pmic_get_register_value(PMIC_HOMEKEY_DEB));
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	kpd_pmic_rstkey_handler(0x1);
#endif
}

void homekey_int_handler_r(void)
{
	PMICLOG("[homekey_int_handler_r] Release homekey %d\n",
		pmic_get_register_value(PMIC_HOMEKEY_DEB));
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	kpd_pmic_rstkey_handler(0x0);
#endif
}

/* Chrdet Int Handler */
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
#endif /* IRQ_HANDLER_READY */

/* May be removed(TBD) */
/* Auxadc Int Handler */
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

/* General OC Int Handler */
void oc_int_handler(PMIC_IRQ_ENUM intNo, const char *int_name)
{
	PMICLOG("[%s] int name=%s\n", __func__, int_name);
	switch (intNo) {
	default:
		/* issue AEE exception and disable OC interrupt */
		/* TBD: dump_exception_reg */
		aee_kernel_warning("PMIC OC", "\nCRDISPATCH_KEY:PMIC OC\nOC Interrupt: %s", int_name);
		pmic_enable_interrupt(intNo, 0, "PMIC");
		pr_err(PMICTAG "[PMIC_INT] disable OC interrupt: %s\n", int_name);
		break;
	}
}

/*
 * PMIC Interrupt service
 */
static DEFINE_MUTEX(pmic_mutex);
struct task_struct *pmic_thread_handle;

#if !defined CONFIG_HAS_WAKELOCKS
struct wakeup_source pmicThread_lock;
#else
struct wake_lock pmicThread_lock;
#endif

void wake_up_pmic(void)
{
	PMICLOG("[%s]\n", __func__);
	if (pmic_thread_handle != NULL) {
		pmic_wake_lock(&pmicThread_lock);
		wake_up_process(pmic_thread_handle);
	} else {
		pr_err(PMICTAG "[%s] pmic_thread_handle not ready\n", __func__);
		return;
	}
}

irqreturn_t mt_pmic_eint_irq(int irq, void *desc)
{
	disable_irq_nosync(irq);
	PMICLOG("[mt_pmic_eint_irq] disable PMIC irq\n");
	wake_up_pmic();
	return IRQ_HANDLED;
}

static unsigned int get_spNo(PMIC_IRQ_ENUM intNo)
{
	if (intNo >= SP_PSC_TOP_START && intNo < SP_BM_TOP_START)
		return 0; /* SP_PSC_TOP */
	else if (intNo >= SP_BM_TOP_START && intNo < SP_SCK_TOP_START)
		return 1; /* SP_BM_TOP */
	else if (intNo >= SP_SCK_TOP_START && intNo < SP_HK_TOP_START)
		return 2; /* SP_SCK_TOP */
	else if (intNo >= SP_HK_TOP_START && intNo < SP_XPP_TOP_START)
		return 3; /* SP_HK_TOP */
	else if (intNo >= SP_XPP_TOP_START && intNo < SP_BUCK_TOP_START)
		return 4; /* SP_XPP_TOP */
	else if (intNo >= SP_BUCK_TOP_START && intNo < SP_LDO_TOP_START)
		return 5; /* SP_BUCK_TOP */
	else if (intNo >= SP_LDO_TOP_START && intNo < SP_AUD_TOP_START)
		return 6; /* SP_LDO_TOP */
	else if (intNo >= SP_AUD_TOP_START && intNo < SP_MISC_TOP_START)
		return 7; /* SP_AUD_TOP */
	return 99;
}

static unsigned int pmic_check_intNo(PMIC_IRQ_ENUM intNo,
	unsigned int *spNo, unsigned int *sp_conNo, unsigned int *sp_irqNo)
{
	if (intNo >= INT_ENUM_MAX)
		return 1;	/* fail intNo */

	*spNo = get_spNo(intNo);
	*sp_conNo = (intNo - sp_interrupts[*spNo].int_offset) / PMIC_INT_WIDTH;
	*sp_irqNo = intNo % PMIC_INT_WIDTH;
	return 0;
}


void pmic_enable_interrupt(PMIC_IRQ_ENUM intNo, unsigned int en, char *str)
{
	unsigned int spNo, sp_conNo, sp_irqNo;
	unsigned int enable_reg;

#if 0
	if (intNo >= INT_ENUM_MAX) {
		pr_err(PMICTAG "[%s] fail intNo=%d\n", __func__, intNo);
		return;
	}

	spNo = get_spNo(intNo);
	sp_conNo = (intNo - sp_interrupts[spNo].int_offset) / PMIC_INT_WIDTH;
	sp_irqNo = intNo % PMIC_INT_WIDTH;
#else
	if (pmic_check_intNo(intNo, &spNo, &sp_conNo, &sp_irqNo)) {
		pr_err(PMICTAG "[%s] fail intNo=%d\n", __func__, intNo);
		return;
	}
#endif
	/*sp_interrupts[spNo].sp_irqs[sp_conNo][sp_irqNo];*/

	enable_reg = sp_interrupts[spNo].enable + 0x6 * sp_conNo;
	PMICLOG("[%s] intNo=%d en=%d str=%s spNo=%d sp_conNo=%d sp_irqNo=%d, Reg[0x%x]=0x%x\n", __func__,
		intNo, en, str, spNo, sp_conNo, sp_irqNo, enable_reg, upmu_get_reg_value(enable_reg));
	if (en == 1)
		pmic_config_interface(enable_reg + 0x2, 0x1, 0x1, sp_irqNo);
	else if (en == 0)
		pmic_config_interface(enable_reg + 0x4, 0x1, 0x1, sp_irqNo);
	PMICLOG("[%s] after, Reg[0x%x]=0x%x\n", __func__,
		enable_reg, upmu_get_reg_value(enable_reg));
}

void pmic_mask_interrupt(PMIC_IRQ_ENUM intNo, char *str)
{
	unsigned int spNo, sp_conNo, sp_irqNo;
	unsigned int mask_reg;

	if (pmic_check_intNo(intNo, &spNo, &sp_conNo, &sp_irqNo)) {
		pr_err(PMICTAG "[%s] fail intNo=%d\n", __func__, intNo);
		return;
	}
	mask_reg = sp_interrupts[spNo].mask + 0x6 * sp_conNo;
	PMICLOG("[%s] intNo=%d str=%s spNo=%d sp_conNo=%d sp_irqNo=%d, Reg[0x%x]=0x%x\n", __func__,
		intNo, str, spNo, sp_conNo, sp_irqNo, mask_reg, upmu_get_reg_value(mask_reg));
	/* MASK_SET */
	pmic_config_interface(mask_reg + 0x2, 0x1, 0x1, sp_irqNo);
	PMICLOG("[%s] after, Reg[0x%x]=0x%x\n", __func__,
		mask_reg, upmu_get_reg_value(mask_reg));
}

void pmic_unmask_interrupt(PMIC_IRQ_ENUM intNo, char *str)
{
	unsigned int spNo, sp_conNo, sp_irqNo;
	unsigned int mask_reg;

	if (pmic_check_intNo(intNo, &spNo, &sp_conNo, &sp_irqNo)) {
		pr_err(PMICTAG "[%s] fail intNo=%d\n", __func__, intNo);
		return;
	}
	mask_reg = sp_interrupts[spNo].mask + 0x6 * sp_conNo;
	PMICLOG("[%s] intNo=%d str=%s spNo=%d sp_conNo=%d sp_irqNo=%d, Reg[0x%x]=0x%x\n", __func__,
		intNo, str, spNo, sp_conNo, sp_irqNo, mask_reg, upmu_get_reg_value(mask_reg));
	/* MASK_CLR */
	pmic_config_interface(mask_reg + 0x4, 0x1, 0x1, sp_irqNo);
	PMICLOG("[%s] after, Reg[0x%x]=0x%x\n", __func__,
		mask_reg, upmu_get_reg_value(mask_reg));
}

void pmic_register_interrupt_callback(PMIC_IRQ_ENUM intNo, void (EINT_FUNC_PTR) (void))
{
	unsigned int spNo, sp_conNo, sp_irqNo;

	if (pmic_check_intNo(intNo, &spNo, &sp_conNo, &sp_irqNo)) {
		pr_err(PMICTAG "[%s] fail intNo=%d\n", __func__, intNo);
		return;
	} else if (sp_interrupts[spNo].sp_irqs[sp_conNo][sp_irqNo].callback != NULL) {
		pr_err(PMICTAG "[%s] register callback conflict intNo=%d\n", __func__, intNo);
		return;
	}

	PMICLOG("[%s] intNo=%d\n", __func__, intNo);
	sp_interrupts[spNo].sp_irqs[sp_conNo][sp_irqNo].callback = EINT_FUNC_PTR;
}

#if ENABLE_ALL_OC_IRQ
/* register general oc interrupt handler */
void pmic_register_oc_interrupt_callback(PMIC_IRQ_ENUM intNo)
{
	unsigned int shift, no;

	shift = intNo / PMIC_INT_WIDTH;
	no = intNo % PMIC_INT_WIDTH;

	if (shift >= interrupts_size) {
		pr_err(PMICTAG "[%s] fail intno=%d\r\n", __func__, intNo);
		return;
	}
	interrupts[shift].interrupts[no].oc_callback = oc_int_handler;

	PMICLOG("[pmic_register_oc_interrupt_callback] intno=%d\r\n", intNo);
}

/* register and enable all oc interrupt */
void register_all_oc_interrupts(void)
{
	PMIC_IRQ_ENUM oc_interrupt = INT_VCORE_OC;

	for (; oc_interrupt <= INT_VXO22_OC; oc_interrupt++) {
		/* ignore pre_oc */
		if (oc_interrupt == INT_VCORE_PREOC)
			continue;
		pmic_register_oc_interrupt_callback(oc_interrupt);
		pmic_enable_interrupt(oc_interrupt, 1, "PMIC");
	}
}
#endif

static void pmic_sp_irq_handler(unsigned int spNo, unsigned int sp_conNo, unsigned int sp_int_status)
{
	unsigned int i;
	struct pmic_sp_irq *sp_irq;

	if (sp_int_status == 0)
		return; /* this subpack control has no interrupt triggered */

	pr_err(PMICTAG "[PMIC_INT] Reg[0x%x]=0x%x\n", (sp_interrupts[spNo].status + 0x6 * sp_conNo), sp_int_status);
	/* clear interrupt status in this subpack control */
	upmu_set_reg_value((sp_interrupts[spNo].status + 0x6 * sp_conNo), sp_int_status);
	for (i = 0; i < PMIC_INT_WIDTH; i++) {
		if (sp_int_status & (1 << i)) {
			sp_irq = &(sp_interrupts[spNo].sp_irqs[sp_conNo][i]);
			PMICLOG("[PMIC_INT][%s]\n", sp_irq->name);
			sp_irq->times++;
			if (sp_irq->callback != NULL)
				sp_irq->callback();
			if (sp_irq->oc_callback != NULL) {
				sp_irq->oc_callback(
					(sp_interrupts[spNo].int_offset + sp_conNo * PMIC_INT_WIDTH + i),
					sp_irq->name);
			}
		}
	}

}

static void pmic_int_handler(void)
{
	unsigned int spNo, sp_conNo;
	unsigned int status_reg;
	unsigned int top_int_status = 0, sp_int_status = 0;

	top_int_status = upmu_get_reg_value(MT6356_TOP_INT_STATUS0);

	for (spNo = 0; spNo < sp_interrupt_size; spNo++) {
		if (!(top_int_status & (1 << sp_interrupts[spNo].top_int_bit)))
			continue; /* this subpack has no interrupt triggered */
		for (sp_conNo = 0; sp_conNo < sp_interrupts[spNo].con_len; sp_conNo++) {
			status_reg = sp_interrupts[spNo].status + 0x6 * sp_conNo;
			sp_int_status = upmu_get_reg_value(status_reg);
			pmic_sp_irq_handler(spNo, sp_conNo, sp_int_status);
		}
	}
}

int pmic_thread_kthread(void *x)
{
	unsigned int spNo, sp_conNo;
	unsigned int status_reg;
	unsigned int sp_int_status = 0;
#ifdef IPIMB
#else
	unsigned int pwrap_eint_status = 0;
#endif
	struct sched_param param = {.sched_priority = 98 };

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);

	PMICLOG("[PMIC_INT] enter\n");

	pmic_enable_charger_detection_int(0);

	/* Run on a process content */
	while (1) {
		mutex_lock(&pmic_mutex);
#ifdef IPIMB
#else
		pwrap_eint_status = pmic_wrap_eint_status();
		PMICLOG("[PMIC_INT] pwrap_eint_status=0x%x\n", pwrap_eint_status);
#endif
		pmic_int_handler();
#ifdef IPIMB
#else
		pmic_wrap_eint_clr(0x0);
#endif
		for (spNo = 0; spNo < sp_interrupt_size; spNo++) {
			for (sp_conNo = 0; sp_conNo < sp_interrupts[spNo].con_len; sp_conNo++) {
				status_reg = sp_interrupts[spNo].status + 0x6 * sp_conNo;
				sp_int_status = upmu_get_reg_value(status_reg);
				PMICLOG("[PMIC_INT] after, Reg[0x%x]=0x%x\n",
					status_reg, sp_int_status);
			}
		}
		mutex_unlock(&pmic_mutex);
		pmic_wake_unlock(&pmicThread_lock);

		set_current_state(TASK_INTERRUPTIBLE);
		enable_irq(g_pmic_irq);
		schedule();
	}

	return 0;
}

static void irq_thread_init(void)
{
	/* create pmic irq thread handler*/
	pmic_thread_handle = kthread_create(pmic_thread_kthread, (void *)NULL, "pmic_thread");
	if (IS_ERR(pmic_thread_handle)) {
		pmic_thread_handle = NULL;
		pr_err(PMICTAG "[pmic_thread_kthread] creation fails\n");
	} else {
		PMICLOG("[pmic_thread_kthread] kthread_create Done\n");
	}
}

static void register_irq_handlers(void)
{
#if IRQ_HANDLER_READY
	pmic_register_interrupt_callback(INT_PWRKEY, pwrkey_int_handler);
	pmic_register_interrupt_callback(INT_HOMEKEY, homekey_int_handler);
	pmic_register_interrupt_callback(INT_PWRKEY_R, pwrkey_int_handler_r);
	pmic_register_interrupt_callback(INT_HOMEKEY_R, homekey_int_handler_r);

	pmic_register_interrupt_callback(INT_CHRDET_EDGE, chrdet_int_handler);
	pmic_register_interrupt_callback(INT_BAT_L, bat_l_int_handler);
	pmic_register_interrupt_callback(INT_BAT_H, bat_h_int_handler);

	pmic_register_interrupt_callback(INT_FG_CUR_H, fg_cur_h_int_handler);
	pmic_register_interrupt_callback(INT_FG_CUR_L, fg_cur_l_int_handler);
#endif
#if ENABLE_ALL_OC_IRQ
	register_all_oc_interrupts();
#endif
}

static void enable_pmic_irqs(void)
{
	pmic_enable_interrupt(INT_PWRKEY, 1, "PMIC");
	pmic_enable_interrupt(INT_HOMEKEY, 1, "PMIC");
	pmic_enable_interrupt(INT_PWRKEY_R, 1, "PMIC");
	pmic_enable_interrupt(INT_HOMEKEY_R, 1, "PMIC");

	pmic_enable_interrupt(INT_CHRDET_EDGE, 1, "PMIC");
}

void PMIC_EINT_SETTING(void)
{
	struct device_node *node = NULL;
	int ret = 0;
	unsigned int spNo, sp_conNo;
	unsigned int enable_reg;

	/* create pmic irq thread handler*/
	irq_thread_init();

	/* Disable all interrupt for initializing */
	for (spNo = 0; spNo < sp_interrupt_size; spNo++) {
		for (sp_conNo = 0; sp_conNo < sp_interrupts[spNo].con_len; sp_conNo++) {
			enable_reg = sp_interrupts[spNo].enable + 0x6 * sp_conNo;
			upmu_set_reg_value(enable_reg, 0);
		}
	}

	/* For all interrupt events, turn on interrupt module clock */
	pmic_set_register_value(PMIC_RG_INTRP_CK_PDN, 0);
	/* For BUCK PREOC related interrupt, please turn on intrp_pre_oc_ck (1MHz) */
	/* This clock is default on */
	/*pmic_set_register_value(RG_INTRP_PRE_OC_CK_PDN, 0); TBD*/

	register_irq_handlers();
	enable_pmic_irqs();

	node = of_find_compatible_node(NULL, NULL, "mediatek,pmic-eint");
	if (node) {
		/* no debounce setting */
		g_pmic_irq = irq_of_parse_and_map(node, 0);
		ret = request_irq(g_pmic_irq, (irq_handler_t)mt_pmic_eint_irq,
			IRQF_TRIGGER_NONE, "pmic-eint", NULL);
		if (ret > 0)
			pr_err(PMICTAG "EINT IRQ NOT AVAILABLE\n");
		enable_irq_wake(g_pmic_irq);
	} else
		pr_err(PMICTAG "can't find compatible node\n");

	PMICLOG("[CUST_EINT] CUST_EINT_MT_PMIC_MT6356_NUM=%d\n", g_eint_pmic_num);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_CN=%d\n", g_cust_eint_mt_pmic_debounce_cn);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_TYPE=%d\n", g_cust_eint_mt_pmic_type);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_EN=%d\n", g_cust_eint_mt_pmic_debounce_en);
}

MODULE_AUTHOR("Jeter Chen");
MODULE_DESCRIPTION("MT PMIC Interrupt Driver");
MODULE_LICENSE("GPL");
