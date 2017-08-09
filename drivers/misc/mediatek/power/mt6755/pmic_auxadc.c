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

#include <asm/uaccess.h>

#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
/*#include <mach/eint.h> TBD*/
#include <mach/mt_pmic_wrap.h>
#if defined CONFIG_MTK_LEGACY
/*#include <mach/mt_gpio.h> TBD*/
#endif
/*#include <mach/mtk_rtc.h> TBD*/
#include <mach/mt_spm_mtcmos.h>

/*#include <mach/battery_common.h> TBD*/
#include <linux/time.h>
/*#include <mach/pmic_mt6328_sw.h>*/
/*#include <mach/battery_meter.h> TBD*/

/*#include <cust_pmic.h> TBD*/
/*#include <cust_battery_meter.h> TBD*/


/*
 * PMIC-AUXADC related define
 */
#define VOLTAGE_FULL_RANGE	1800
#define VOLTAGE_FULL_RANGE_6311	3200
#define ADC_PRECISE		32768	/* 15 bits*/
#define ADC_PRECISE_CH7		131072	/* 17 bits*/
#define ADC_PRECISE_6311	4096	/* 12 bits*/

/*
 * PMIC-AUXADC global variable
 */

#define PMICTAG                "[Auxadc] "
#if defined PMIC_DEBUG
#define PMICLOG2(fmt, arg...)   pr_debug(PMICTAG fmt, ##arg)
#else
#define PMICLOG2(fmt, arg...)
#endif

signed int count_time_out = 15;
struct wake_lock pmicAuxadc_irq_lock;
/*static DEFINE_SPINLOCK(pmic_adc_lock);*/
static DEFINE_MUTEX(pmic_adc_mutex);

void pmic_auxadc_init(void)
{
	/*signed int adc_busy;*/
	wake_lock_init(&pmicAuxadc_irq_lock, WAKE_LOCK_SUSPEND, "pmicAuxadc irq wakelock");

	pmic_set_register_value(PMIC_AUXADC_AVG_NUM_LARGE, 6);	/* 1.3ms */
	pmic_set_register_value(PMIC_AUXADC_AVG_NUM_SMALL, 2);	/* 0.8ms */

	pmic_set_register_value(PMIC_AUXADC_AVG_NUM_SEL, 0x83);	/* 0.8ms */

	pmic_set_register_value(PMIC_AUXADC_VBUF_EN, 0x1);

	PMICLOG2("****[pmic_auxadc_init] DONE\n");
}

void pmic_auxadc_lock(void)
{
	wake_lock(&pmicAuxadc_irq_lock);
	mutex_lock(&pmic_adc_mutex);
}

void pmic_auxadc_unlock(void)
{
	wake_unlock(&pmicAuxadc_irq_lock);
	mutex_unlock(&pmic_adc_mutex);
}

signed int PMIC_IMM_GetCurrent(void)
{
	signed int ret = 0;
	int count = 0;
	signed int batsns, isense;
	signed int ADC_I_SENSE = 1;	/* 1 measure time*/
	signed int ADC_BAT_SENSE = 1;	/* 1 measure time*/
	signed int ICharging = 0;

	pmic_set_register_value(PMIC_AUXADC_CK_AON, 1);
	pmic_set_register_value(PMIC_AUXADC_VBUF_EN, 1);
	pmic_set_register_value(PMIC_RG_CLKSQ_EN_AUX_AP_MODE, 1);

	wake_lock(&pmicAuxadc_irq_lock);
	mutex_lock(&pmic_adc_mutex);
	ret = pmic_config_interface(MT6351_AUXADC_RQST0_SET, 0x3, 0xffff, 0);

	while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH0_BY_AP) != 1) {
		/*msleep(1);*/
		usleep_range(1000, 2000);
		if ((count++) > count_time_out) {
			PMICLOG2("[PMIC_IMM_GetCurrent] batsns Time out!\n");
			break;
		}
	}
	batsns = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH0_BY_AP);

	while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH1_BY_AP) != 1) {
		/*msleep(1);*/
		usleep_range(1000, 2000);
		if ((count++) > count_time_out) {
			PMICLOG2("[PMIC_IMM_GetCurrent] isense Time out!\n");
			break;
		}
	}
	isense = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH1_BY_AP);


	ADC_BAT_SENSE = (batsns * 3 * VOLTAGE_FULL_RANGE) / 32768;
	ADC_I_SENSE = (isense * 3 * VOLTAGE_FULL_RANGE) / 32768;

/*
	ICharging =
	    (ADC_I_SENSE - ADC_BAT_SENSE +
	     g_I_SENSE_offset) * 1000 / batt_meter_cust_data.cust_r_sense;

 TBD*/

	wake_unlock(&pmicAuxadc_irq_lock);
	mutex_unlock(&pmic_adc_mutex);

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
	unsigned int busy;
	/*
	   PMIC_AUX_BATSNS_AP =         0x000,
	   PMIC_AUX_ISENSE_AP,
	   PMIC_AUX_VCDT_AP,
	   PMIC_AUX_BATON_AP,
	   PMIC_AUX_CH4,
	   PMIC_AUX_VACCDET_AP,
	   PMIC_AUX_CH6,
	   PMIC_AUX_TSX,
	   PMIC_AUX_CH8,
	   PMIC_AUX_CH9,
	   PMIC_AUX_CH10,
	   PMIC_AUX_CH11,
	   PMIC_AUX_CH12,
	   PMIC_AUX_CH13,
	   PMIC_AUX_CH14,
	   PMIC_AUX_CH15,
	   BATSNS 3v-4.5v
	   ISENSE 1.5-4.5v
	   BATON  0-1.8v
	   VCDT   4v-14v
	   ACCDET 1.8v
	   GPS    1.8v

	 */

	/*upmu_set_reg_value(0x28c,0x0002); */

	pmic_set_register_value(PMIC_AUXADC_CK_AON, 1);
	/* mt6351 change to hw control, but reserve sw control register */
	/* pmic_set_register_value(PMIC_AUXADC_VBUF_EN,1); //use ch4 only, pmic temp or dcxo temp */
	pmic_set_register_value(PMIC_RG_CLKSQ_EN_AUX_AP_MODE, 1);	/* enable 26MHz */

	/*upmu_set_reg_value(0x0a44,0x010a); */
#if defined PMIC_DVT_TC_EN
	/* only used for PMIC_DVT */
	pmic_set_register_value(PMIC_STRUP_AUXADC_START_SEL, 1);
	pmic_set_register_value(PMIC_STRUP_AUXADC_RSTB_SW, 1);
	pmic_set_register_value(PMIC_STRUP_AUXADC_RSTB_SEL, 1);
	/* END only used for PMIC_DVT */
#endif
	wake_lock(&pmicAuxadc_irq_lock);
	mutex_lock(&pmic_adc_mutex);
	/*ret=pmic_config_interface(MT6351_TOP_CLKSQ_SET,(1<<2),0xffff,0); */
	ret = pmic_config_interface(MT6351_AUXADC_RQST0_SET, (1 << dwChannel), 0xffff, 0);


	busy = upmu_get_reg_value(MT6351_AUXADC_STA0);
	udelay(50);

	switch (dwChannel) {
	case 0:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH0_BY_AP) != 1) {
			/*msleep(1);*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH0_BY_AP);
		break;
	case 1:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH1_BY_AP) != 1) {
			/*msleep(1);*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH1_BY_AP);
		break;
	case 2:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH2) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH2);
		break;
	case 3:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH3) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH3);
		break;
	case 4:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH4) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH4);
		break;
	case 5:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH5) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH5);
		break;
	case 6:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH6) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH6);
		break;
	case 7:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH7_BY_AP) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH7_BY_AP);
		break;
	case 8:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH8) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH8);
		break;
	case 9:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH9) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH9);
		break;
	case 10:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH10) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH10);
		break;
	case 11:
		/* MT6351 ch3 bug, wei-lin request us to add below code */
		if (g_pmic_pad_vbif28_vol == 0x1) {
			ret = pmic_set_register_value(PMIC_BATON_TDET_EN, 0);
			ret = pmic_set_register_value(PMIC_TOP_CKPDN_CON2_CLR, 0x70);
			ret = pmic_config_interface(MT6351_LDO_VBIF28_CON0, 0xda6a, 0xffff, 0);
			/* mt6351 sw workaround, PAD_VBIF28 SWITCH TURN ON */
			pmic_set_register_value(PMIC_RG_ADCIN_VSEN_MUX_EN, 1);
			mdelay(3); /* delay 1~3ms */
		}

		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH11) != 1) {
			usleep_range(1000, 1200);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH11);
		break;
	case 12:
	case 13:
	case 14:
	case 15:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH12_15) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH12_15);
		break;


	default:
		PMICLOG2("[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
		wake_unlock(&pmicAuxadc_irq_lock);
		mutex_unlock(&pmic_adc_mutex);
		return -1;
		break;
	}

	switch (dwChannel) {
	case 0:
		r_val_temp = 3;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 1:
		r_val_temp = 3;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 2:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 3:
		r_val_temp = 2;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 4:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 5:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 6:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 7:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 8:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 9:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 10:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 11:
		r_val_temp = 2;
		g_pmic_pad_vbif28_vol = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		/* mt6351 sw workaround, PAD_VBIF28 SWITCH TURN ON */
		pmic_set_register_value(PMIC_RG_ADCIN_VSEN_MUX_EN, 0);
		ret = pmic_set_register_value(PMIC_RG_VBIF28_EN, 0);
/*		ret = pmic_config_interface(MT6351_LDO_VBIF28_CON0,0xda6a,0xffff,0); */
		ret = pmic_set_register_value(PMIC_TOP_CKPDN_CON2_SET, 0x70);
		mdelay(3); /* delay 1~3ms */
		ret = pmic_set_register_value(PMIC_BATON_TDET_EN, 0x1);
		PMICLOG2("[AUXADC] VBIF28_def_volt(%x, %x, %x)\n", g_pmic_pad_vbif28_vol,
			ret_data, pmic_get_register_value(PMIC_BATON_TDET_EN));
		break;
	case 12:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 13:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 14:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 15:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 16:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	default:
		PMICLOG2("[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
		wake_unlock(&pmicAuxadc_irq_lock);
		mutex_unlock(&pmic_adc_mutex);
		return -1;
		break;
	}

	wake_unlock(&pmicAuxadc_irq_lock);
	mutex_unlock(&pmic_adc_mutex);

	/*PMICLOG2("[AUXADC] ch=%d raw=%d data=%d\n", dwChannel, ret_data,adc_result);*/

	/*return ret_data;*/
	return adc_result;

}

unsigned int PMIC_IMM_GetOneChannelValueMD(unsigned char dwChannel, int deCount, int trimd)
{
	signed int ret = 0;
	signed int ret_data;
	signed int r_val_temp = 0;
	signed int adc_result = 0;
	int count = 0;
	/*
	   CH0: BATSNS
	   CH1: ISENSE
	   CH4: PMIC TEMP
	   CH7: TSX by MD
	   CH8: TSX by GPS

	 */

	if (dwChannel != 0 && dwChannel != 1 && dwChannel != 4 && dwChannel != 7 && dwChannel != 8)
		return -1;


	wake_lock(&pmicAuxadc_irq_lock);

	mutex_lock(&pmic_adc_mutex);
	ret = pmic_config_interface(MT6351_TOP_CLKSQ_SET, (1 << 3), 0xffff, 0);
	ret = pmic_config_interface(MT6351_AUXADC_RQST1_SET, (1 << dwChannel), 0xffff, 0);
	mutex_unlock(&pmic_adc_mutex);

	udelay(10);

	switch (dwChannel) {
	case 0:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH0_BY_MD) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[PMIC_IMM_GetOneChannelValueMD] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH0_BY_MD);
		break;
	case 1:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH1_BY_MD) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[PMIC_IMM_GetOneChannelValueMD] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH1_BY_MD);
		break;
	case 4:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH4_BY_MD) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[PMIC_IMM_GetOneChannelValueMD] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH4_BY_MD);
		break;
	case 7:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH7_BY_MD) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[PMIC_IMM_GetOneChannelValueMD] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH7_BY_MD);
		break;
	case 8:
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH7_BY_GPS) != 1) {
			/*msleep(1)*/
			usleep_range(1000, 2000);
			if ((count++) > count_time_out) {
				PMICLOG2("[PMIC_IMM_GetOneChannelValueMD] (%d) Time out!\n",
					dwChannel);
				break;
			}
		}
		ret_data = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_CH7_BY_GPS);
		break;


	default:
		PMICLOG2("[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
		wake_unlock(&pmicAuxadc_irq_lock);
		return -1;
		break;
	}

	switch (dwChannel) {
	case 0:
		r_val_temp = 3;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 1:
		r_val_temp = 3;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 4:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 4096;
		break;
	case 7:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	case 8:
		r_val_temp = 1;
		adc_result = (ret_data * r_val_temp * VOLTAGE_FULL_RANGE) / 32768;
		break;
	default:
		PMICLOG2("[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
		wake_unlock(&pmicAuxadc_irq_lock);
		return -1;
		break;
	}

	wake_unlock(&pmicAuxadc_irq_lock);

	PMICLOG2("[AUXADC] PMIC_IMM_GetOneChannelValueMD ch=%d raw=%d data=%d\n", dwChannel,
		ret_data, adc_result);

	return ret_data;
	/*return adc_result;*/

}
