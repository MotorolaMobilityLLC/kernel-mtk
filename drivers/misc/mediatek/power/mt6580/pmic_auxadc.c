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
/* #include <linux/aee.h> TBD */
/* #include <linux/xlog.h> TBD */
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
/* #include <linux/earlysuspend.h> TBD */
#include <linux/seq_file.h>

#include <asm/uaccess.h>

/* #include <mach/upmu_common.h> */
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
/* #include <mach/mt_pm_ldo.h> */
/* #include <mach/eint.h> TBD */
#include <mach/mt_pmic_wrap.h>
/* #include <mach/mt_gpio.h> TBD */
/* #include <mach/mtk_rtc.h> TBD */
#include <mach/mt_spm_mtcmos.h>
#include <mach/mt_clkbuf_ctl.h>

/*#include <mach/battery_common.h> TBD */
#include <linux/time.h>

/* #include <cust_pmic.h> TBD */
/* #include <cust_battery_meter.h> TBD */

/* extern void __iomem *spm_base; TBD */

/*
 * PMIC-AUXADC related define
 */
#define VOLTAGE_FULL_RANGE	1800
#define VOLTAGE_FULL_RANGE_6311	3200
#define ADC_PRECISE		32768	/* 15 bits */
#define ADC_PRECISE_CH7		131072	/* 17 bits */
#define ADC_PRECISE_6311	4096	/* 12 bits */

/*
 * PMIC-AUXADC global variable
 */
#define PMIC_RD32(addr)            __raw_readl((void *)addr)
#define PMICTAG                "[Auxadc] "
#define PMICLOG(fmt, arg...)   pr_debug(PMICTAG fmt, ##arg)

#define TURN_OFF                0
#define TURN_ON                 1

signed int count_time_out = 15;
struct wake_lock pmicAuxadc_irq_lock;
static DEFINE_SPINLOCK(pmic_adc_lock);
static DEFINE_MUTEX(pmic_adc_mutex);

void pmic_auxadc_init(void)
{
	wake_lock_init(&pmicAuxadc_irq_lock, WAKE_LOCK_SUSPEND, "pmicAuxadc irq wakelock");
	PMICLOG("****[pmic_auxadc_init] DONE\n");
}

unsigned int pmic_is_auxadc_busy(void)
{
	unsigned int ret = 0;
	unsigned int int_status_val_0 = 0;

	ret = pmic_read_interface(0x73a, (&int_status_val_0), 0x7FFF, 0x1);
	return int_status_val_0;
}

void pmic_turn_on_clock(unsigned int enable)
{
#if 0
	unsigned int reg = 0xFFFF;
#endif
	/* PMICLOG("Before Clock Dump = %x\n",PMIC_RD32(spm_base+0x620)); */
	if (enable == TURN_ON)
		clk_buf_ctrl(CLK_BUF_AUDIO, 1);
	else
		clk_buf_ctrl(CLK_BUF_AUDIO, 0);
#if 0
	pmic_read_interface(0x108, (&reg), 0xFFFF, 0x0);
	PMICLOG("0x108 = %x\n", reg);
	pmic_read_interface(0x10E, (&reg), 0xFFFF, 0x0);
	PMICLOG("0x10E = %x\n", reg);
	pmic_read_interface(0x120, (&reg), 0xFFFF, 0x0);
	PMICLOG("0x120 = %x\n", reg);
	pmic_read_interface(0x126, (&reg), 0xFFFF, 0x0);
	PMICLOG("0x126 = %x\n", reg);
	PMICLOG("After Clock Dump = %x\n", PMIC_RD32(spm_base + 0x620));
#endif
}

void PMIC_IMM_PollingAuxadcChannel(void)
{
	/* pr_info("[PMIC][PMIC_IMM_PollingAuxadcChannel] before:%d ",upmu_get_rg_adc_deci_gdly()); */


	if (pmic_get_register_value(PMIC_RG_ADC_DECI_GDLY) == 1) {
		while (pmic_get_register_value(PMIC_RG_ADC_DECI_GDLY) == 1) {
			unsigned long flags;

			spin_lock_irqsave(&pmic_adc_lock, flags);
			if (pmic_is_auxadc_busy() == 0)
				pmic_set_register_value_nolock(PMIC_RG_ADC_DECI_GDLY, 0);
			spin_unlock_irqrestore(&pmic_adc_lock, flags);
		}
	}
	/* pr_info("[PMIC][PMIC_IMM_PollingAuxadcChannel] after:%d ",upmu_get_rg_adc_deci_gdly()); */
}

signed int PMIC_IMM_GetCurrent(void)
{
	signed int ret = 0;
	int count = 0;
	signed int batsns, isense;
	signed int ADC_I_SENSE = 1;	/* 1 measure time */
	signed int ADC_BAT_SENSE = 1;	/* 1 measure time */
	signed int ICharging = 0;
	signed int adc_reg_val = 0;
	/*
	   0 : BATON2 **
	   1 : CH6
	   2 : THR SENSE2 **
	   3 : THR SENSE1
	   4 : VCDT
	   5 : BATON1
	   6 : ISENSE
	   7 : BATSNS
	   8 : ACCDET
	   9-16 : audio
	 */
	pmic_set_register_value(PMIC_RG_VBUF_EN, 1);
	pmic_turn_on_clock(TURN_ON);
	udelay(30);
	wake_lock(&pmicAuxadc_irq_lock);
	mutex_lock(&pmic_adc_mutex);
	/* set 0 */
	ret =
	    pmic_read_interface(MT6350_AUXADC_CON22, &adc_reg_val, MT6350_PMIC_RG_AP_RQST_LIST_MASK,
				MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
	adc_reg_val = adc_reg_val & (~(3 << 6));
	ret =
	    pmic_config_interface(MT6350_AUXADC_CON22, adc_reg_val,
				  MT6350_PMIC_RG_AP_RQST_LIST_MASK,
				  MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);

	/* set 1 */
	ret =
	    pmic_read_interface(MT6350_AUXADC_CON22, &adc_reg_val, MT6350_PMIC_RG_AP_RQST_LIST_MASK,
				MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
	adc_reg_val = adc_reg_val | (3 << 6);
	ret =
	    pmic_config_interface(MT6350_AUXADC_CON22, adc_reg_val,
				  MT6350_PMIC_RG_AP_RQST_LIST_MASK,
				  MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
#if 0
	/* set 0 */
	ret =
	    pmic_read_interface(MT6350_AUXADC_CON22, &adc_reg_val, MT6350_PMIC_RG_AP_RQST_LIST_MASK,
				MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
	adc_reg_val = adc_reg_val & (~(1 << 7));
	ret =
	    pmic_config_interface(MT6350_AUXADC_CON22, adc_reg_val,
				  MT6350_PMIC_RG_AP_RQST_LIST_MASK,
				  MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);

	/* set 1 */
	ret =
	    pmic_read_interface(MT6350_AUXADC_CON22, &adc_reg_val, MT6350_PMIC_RG_AP_RQST_LIST_MASK,
				MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
	adc_reg_val = adc_reg_val | (1 << 7);
	ret =
	    pmic_config_interface(MT6350_AUXADC_CON22, adc_reg_val,
				  MT6350_PMIC_RG_AP_RQST_LIST_MASK,
				  MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
#endif
	while (pmic_get_register_value(PMIC_RG_ADC_RDY_ISENSE) != 1) {
		/* msleep(1); TBD */
		usleep_range(1000, 2000);
		if ((count++) > count_time_out) {
			PMICLOG("[PMIC_IMM_GetCurrent] isense Time out!\n");
			break;
		}
	}
	isense = pmic_get_register_value(PMIC_RG_ADC_OUT_ISENSE);
	while (pmic_get_register_value(PMIC_RG_ADC_RDY_BATSNS) != 1) {
		/* msleep(1); */
		usleep_range(1000, 2000);
		if ((count++) > count_time_out) {
			PMICLOG("[PMIC_IMM_GetCurrent] batsns Time out!\n");
			break;
		}
	}
	batsns = pmic_get_register_value(PMIC_RG_ADC_OUT_BATSNS);


	ADC_BAT_SENSE = (batsns * 3 * VOLTAGE_FULL_RANGE) / 32768;
	ADC_I_SENSE = (isense * 3 * VOLTAGE_FULL_RANGE) / 32768;

#ifdef CONFIG_SMART_BATTERY
	ICharging = (ADC_I_SENSE - ADC_BAT_SENSE + g_I_SENSE_offset) * 1000 / CUST_R_SENSE;
#endif

	pmic_turn_on_clock(TURN_OFF);
	wake_unlock(&pmicAuxadc_irq_lock);
	mutex_unlock(&pmic_adc_mutex);

	PMICLOG
	    ("[PMIC_IMM_GetCurrent(share lock)] raw batsns:%d raw isense:%d batsns:%d isense:%d iCharging:%d cnt:%d\n",
	     batsns, isense, ADC_BAT_SENSE, ADC_I_SENSE, ICharging, count);

	return ICharging;

}

/*
 * PMIC-AUXADC
 */
unsigned int PMIC_IMM_GetOneChannelValue(pmic_adc_ch_list_enum dwChannel, int deCount, int trimd)
{
	signed int ret = 0;
	signed int ret_data;
	signed int r_val_temp = 0;
	signed int adc_result = 0;
	int count = 0;
	signed int adc_reg_val = 0;
	/*
	   0 : BATON2 **
	   1 : CH6
	   2 : THR SENSE2 **
	   3 : THR SENSE1
	   4 : VCDT
	   5 : BATON1
	   6 : ISENSE
	   7 : BATSNS
	   8 : ACCDET
	   9-16 : audio
	 */

	/* do not support BATON2 and THR SENSE2 for sw workaround */
	if (dwChannel == 0 || dwChannel == 2)
		return 0;

	if (dwChannel > 15)
		return -1;
	pmic_turn_on_clock(TURN_ON);
	udelay(30);
	PMIC_IMM_PollingAuxadcChannel();


	wake_lock(&pmicAuxadc_irq_lock);
	mutex_lock(&pmic_adc_mutex);

	if (dwChannel < 9) {
		pmic_set_register_value(PMIC_RG_VBUF_EN, 1);

		/* set 0 */
		ret =
		    pmic_read_interface(MT6350_AUXADC_CON22, &adc_reg_val,
					MT6350_PMIC_RG_AP_RQST_LIST_MASK,
					MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
		adc_reg_val = adc_reg_val & (~(1 << dwChannel));
		ret =
		    pmic_config_interface(MT6350_AUXADC_CON22, adc_reg_val,
					  MT6350_PMIC_RG_AP_RQST_LIST_MASK,
					  MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);

		/* set 1 */
		ret =
		    pmic_read_interface(MT6350_AUXADC_CON22, &adc_reg_val,
					MT6350_PMIC_RG_AP_RQST_LIST_MASK,
					MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
		adc_reg_val = adc_reg_val | (1 << dwChannel);
		ret =
		    pmic_config_interface(MT6350_AUXADC_CON22, adc_reg_val,
					  MT6350_PMIC_RG_AP_RQST_LIST_MASK,
					  MT6350_PMIC_RG_AP_RQST_LIST_SHIFT);
	} else if (dwChannel >= 9 && dwChannel <= 16) {
		ret =
		    pmic_read_interface(MT6350_AUXADC_CON23, &adc_reg_val,
					MT6350_PMIC_RG_AP_RQST_LIST_RSV_MASK,
					MT6350_PMIC_RG_AP_RQST_LIST_RSV_SHIFT);
		adc_reg_val = adc_reg_val & (~(1 << (dwChannel - 9)));
		ret =
		    pmic_config_interface(MT6350_AUXADC_CON23, adc_reg_val,
					  MT6350_PMIC_RG_AP_RQST_LIST_RSV_MASK,
					  MT6350_PMIC_RG_AP_RQST_LIST_RSV_SHIFT);

		/* set 1 */
		ret =
		    pmic_read_interface(MT6350_AUXADC_CON23, &adc_reg_val,
					MT6350_PMIC_RG_AP_RQST_LIST_RSV_MASK,
					MT6350_PMIC_RG_AP_RQST_LIST_RSV_SHIFT);
		adc_reg_val = adc_reg_val | (1 << (dwChannel - 9));
		ret =
		    pmic_config_interface(MT6350_AUXADC_CON23, adc_reg_val,
					  MT6350_PMIC_RG_AP_RQST_LIST_RSV_MASK,
					  MT6350_PMIC_RG_AP_RQST_LIST_RSV_SHIFT);
	}

	udelay(300);

	switch (dwChannel) {
	case 0:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_BATON2) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_BATON2);
		break;
	case 1:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_CH6) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_CH6);
		break;
	case 2:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_THR_SENSE2) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_THR_SENSE2);
		break;
	case 3:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_THR_SENSE1) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_THR_SENSE1);
		break;
	case 4:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_VCDT) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_VCDT);
		break;
	case 5:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_BATON1) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_BATON1);
		break;
	case 6:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_ISENSE) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_ISENSE);
		break;
	case 7:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_BATSNS) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_BATSNS);
		break;
	case 8:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_CH5) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_CH5);
		break;
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		while (pmic_get_register_value(PMIC_RG_ADC_RDY_INT) != 1) {
			/* msleep(1); */
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_RG_ADC_OUT_INT);
		break;


	default:
		PMICLOG("[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
		pmic_turn_on_clock(TURN_OFF);
		wake_unlock(&pmicAuxadc_irq_lock);
		mutex_unlock(&pmic_adc_mutex);
		return -1;
		break;
	}

	switch (dwChannel) {
	case 0:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 1:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 2:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 3:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 4:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 5:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 6:
		r_val_temp = 4;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 7:
		r_val_temp = 4;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 8:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	default:
		PMICLOG("[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
		pmic_turn_on_clock(TURN_OFF);
		wake_unlock(&pmicAuxadc_irq_lock);
		mutex_unlock(&pmic_adc_mutex);
		return -1;
		break;
	}
	pmic_turn_on_clock(TURN_OFF);
	wake_unlock(&pmicAuxadc_irq_lock);
	mutex_unlock(&pmic_adc_mutex);
#if 0
	{
		unsigned int ret = 0, clock = 0, pmic_cid = 0;

		ret = pmic_read_interface(0x100, (&pmic_cid), 0xFFFF, 0x0);
		PMICLOG("[AUXADC] pmic_cid = %x\n", pmic_cid);
		ret = pmic_read_interface(MT6350_TOP_CKPDN0, (&clock), 0xFFFF, 0x0);
		PMICLOG("[AUXADC] clock0 = %x\n", clock);
		ret = pmic_read_interface(MT6350_TOP_CKPDN1, (&clock), 0xFFFF, 0x0);
		PMICLOG("[AUXADC] clock1 = %x\n", clock);
		ret = pmic_read_interface(MT6350_TOP_CKPDN2, (&clock), 0xFFFF, 0x0);
		PMICLOG("[AUXADC] clock2 = %x\n", clock);
	}
#endif
	PMICLOG("[AUXADC] ch=%d raw=%d data=%d\n", dwChannel, ret_data, adc_result);

	/* return ret_data; */

	return adc_result;

}
