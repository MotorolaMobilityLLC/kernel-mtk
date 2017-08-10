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
/*
 * @file    mt_idvfs.c
 * @brief   Driver for CPU iDVFS
 *
 */
#define	__MT_IDVFS_C__

/* system includes */
#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>	/* spin lock */
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>	/* for suspend,resume */
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/cpu.h>		/* cpu_online */
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <mach/irqs.h>

/* local includes */
#include "../../../power/mt6797/da9214.h"
#include "mt_ptp.h"
#include "mt_cpufreq.h"
#include "mt_idvfs.h"
#include "mt_otp.h"
#include "mt_ocp.h"
#include "mach/mt_thermal.h"
#include "mt_freqhopping_drv.h"  /* mt6797_0x1001AXXX_lock */
#include <mt-plat/sync_write.h>  /* mt_reg_sync_writel */
#include <mt-plat/mt_io.h>       /* reg read,srite */
#include <mt-plat/aee.h>         /* ram console */

/* IDVFS ADDR */ /* TODO: include other head file */
#ifdef CONFIG_OF
static void __iomem         *idvfsapb_base;          /* 0x11017000 0x1000, i2c idvfsapb ctrl reg */
static void __iomem         *idvfspin_base;          /* 0x10005000 0x1000, DFD,UDI pinmux reg */
static void __iomem         *idvfsclk_base;          /* 0x10000000 0x1000, clock meter reg */
static void __iomem         *idvfsprob_base;         /* 0x10001000 0x2000, GPU, L-LL, BIG sarm ldo probe reg */
static void __iomem         *idvfsefus_base;         /* 0x10206000 0x1000, Big eFUSE register */
static int                   idvfs_low_freq_cnt;     /* for into low freq counter */
static int                   idvfs_irq_number;       /* 331 */
static char                  idvfs_i2c_debug = 0xd9; /* for i2c ctrl */
static struct                clk *idvfs_i2c6_clk;    /* i2c6 clk ctrl */
/* define IDVFS_BASE_ADDR    ((unsigned long)idvfs_base) */
#else
#include "mach/mt_reg_base.h"
/* (0x10222000) */
/* #define IDVFS_BASE_ADDR IDVFS_BASEADDR */
#endif
#else
#include <linux/stringify.h>
#include "typedefs.h"
#include "da9214.h"
#include "mt_idvfs.h"
#endif

#ifdef __KERNEL__
#define IDVFSAPB_BASE			(idvfsapb_base)
#define IDVFSPIN_BASE			(idvfspin_base)
#define IDVFSCLK_BASE			(idvfsclk_base)
#define IDVFSPROB_BASE			(idvfsprob_base)
#define IDVFSEFUS_BASE			(idvfsefus_base)
#else
#define IDVFSAPB_BASE			0x11017000
#define IDVFSPIN_BASE			0x10005000
#define IDVFSCLK_BASE			0x10000000
#define IDVFSPROB_BASE			0x10001000
#endif

/* 0x11017000 0x1000, i2c idvfsapb ctrl reg */
#define IDVFSAPB_SW_ADDR		(IDVFSAPB_BASE+0x00)
#define IDVFSAPB_SW_DATA		(IDVFSAPB_BASE+0x04)
#define IDVFSAPB_SW_WR			(IDVFSAPB_BASE+0x08)
#define IDVFSAPB_SW_RD			(IDVFSAPB_BASE+0x0c)
#define IDVFSAPB_SW_PRDATA		(IDVFSAPB_BASE+0x10)
#define IDVFSAPB_HW_PMIC_ADDR	(IDVFSAPB_BASE+0x14)
#define IDVFSAPB_HW_DATA_ADDR	(IDVFSAPB_BASE+0x18)
#define IDVFSAPB_HW_STAR_ADDR	(IDVFSAPB_BASE+0x1c)
#define IDVFSAPB_HW_STATUS		(IDVFSAPB_BASE+0x20)
#define IDVFSAPB_HW_START_DATA	(IDVFSAPB_BASE+0x24)
/* i2c dual ctrl register offset only w/o base addr */
#define IDVFSAPB_I2C_DUAL_PORT_DATA		(0x80)
#define IDVFSAPB_I2C_DUAL_SLAVE_ADDR	(0x84)
#define IDVFSAPB_I2C_DUAL_INT_MASK		(0x88)
#define IDVFSAPB_I2C_DUAL_INT_STATUS	(0x8c)
#define IDVFSAPB_I2C_DUAL_CTRL			(0x90)
#define IDVFSAPB_I2C_DUAL_LENGTH		(0x94)
#define IDVFSAPB_I2C_DUAL_TR_LENGTH		(0x98)
#define IDVFSAPB_I2C_DUAL_TIMING		(0xa0) /* i2c speed slect */
#define IDVFSAPB_I2C_DUAL_START			(0xa4)

/* 0x10005000 0x1000, DFD,UDI pinmux reg */
#define IDVFSPIN_UDI_JTAG1		(IDVFSPIN_BASE+0x330)
#define IDVFSPIN_UDI_JTAG2		(IDVFSPIN_BASE+0x340)
#define IDVFSPIN_UDI_SDCARD		(IDVFSPIN_BASE+0x400)
#define IDVFSPIN_DFD			(IDVFSPIN_BASE+0x500)

/* 0x10000000 0x1000, clock meter reg */

/* 0x10001000 0x2000, GPU, L-LL, BIG sarm ldo probe reg */

static int func_lv_mask_idvfs = 100;
#define IDVFS_DREQ_ENABLE		0
#define IDVFS_OCP_ENABLE		1
#define IDVFS_OTP_ENABLE		1
#define IDVFS_CCF_I2C6			1
#define IDVFS_INTERRUPT_ENABLE	0
#define IDVFS_FMAX_DEFAULT		2900 /* org = 2500Mhz, now = 2900MHz(116%) */
#define IDVFS_FMIN_DEFAULT		300  /* org = 505Mhz, now = 300MHz(12%) */
#define IDVFS_FREQMz_STORE		750  /* org = 750Mhz, or 1100Mhz workaround by OCP issue */
#define IDVFS_CTRL_REG_DEFAULT	0x0010a203

/* for ram console print flag */
#ifdef CONFIG_MTK_RAM_CONSOLE
	#define CONFIG_IDVFS_AEE_RR_REC 1
#endif

#if CONFIG_IDVFS_AEE_RR_REC
#define AEE_RR_REC(name, value)		aee_rr_rec_##name(value)
#define AEE_RR_CURR(name)			aee_rr_curr_##name()
#else
#define AEE_RR_REC(name, value)
#define AEE_RR_CURR()
#endif

#if 1
	/* set Vproc = 1000mv for 750MHz, due to PTP VBOOT */
#define IDVFS_VPROC_STORE		100000
	/* set Vsram = 1100mv for 750MHz */
	/* When next iDVFS enable Freq = 750MHz(default),
	so the Vproc need parking to 880mv, Vsarm parking to 1005mv(default). */
#define IDVFS_VSRAM_STORE		110000
#else
	/* set Vproc = 880mv for 750MHz, when next big on use */
#define IDVFS_VPROC_STORE		88000
	/* set Vsram = 1100mv for 750MHz */
	/* When next iDVFS enable Freq = 750MHz(default),
	so the Vproc need parking to 880mv, Vsarm parking to 1005mv(default). */
#define IDVFS_VSRAM_STORE		105000
#endif

/*
 * bit operation
 */
#define BIT_IDVFS(bit)		(1U << (bit))

#define MSB_IDVFS(range)	(1 ? range)
#define LSB_IDVFS(range)	(0 ? range)
/**
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r: Range in the form of MSB:LSB
 */
#define BITMASK_IDVFS(r) \
	(((unsigned) -1 >> (31 - MSB_IDVFS(r))) & ~((1U << LSB_IDVFS(r)) - 1))

#define GET_BITS_VAL_IDVFS(_bits_, _val_) \
	(((_val_) & (BITMASK_IDVFS(_bits_))) >> ((0) ? _bits_))

/**
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r: Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS_IDVFS(r, val)	((val << LSB_IDVFS(r)) & BITMASK_IDVFS(r))
/*
 * REG ACCESS
 */
#define _idvfs_read(addr)               __raw_readl(IOMEM(addr))
#define _idvfs_write(addr, val)         mt_reg_sync_writel((val), ((void *)addr))
#define _idvfs_read_field(addr, range) \
		GET_BITS_VAL_IDVFS(range, _idvfs_read(addr))
#define _idvfs_write_field(addr, range, val) \
		_idvfs_write(addr, (_idvfs_read(addr) & ~(BITMASK_IDVFS(range))) | BITS_IDVFS(range, val))

/*
 * LOG
 */
#define	IDVFS_TAG	  "[mt_idvfs] "
#ifdef __KERNEL__
#ifdef USING_XLOG
	#include <linux/xlog.h>
	#define	idvfs_emerg(fmt, args...)	  pr_err(ANDROID_LOG_ERROR,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_alert(fmt, args...)	  pr_err(ANDROID_LOG_ERROR,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_crit(fmt,	args...)	  pr_err(ANDROID_LOG_ERROR,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_error(fmt, args...)	  pr_err(ANDROID_LOG_ERROR,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_warning(fmt, args...)	  pr_warn(ANDROID_LOG_WARN,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_notice(fmt, args...)	  pr_info(ANDROID_LOG_INFO,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_info(fmt,	args...)	  pr_info(ANDROID_LOG_INFO,	IDVFS_TAG, fmt,	##args)
	#define	idvfs_debug(fmt, args...)	  pr_debug(ANDROID_LOG_DEBUG, IDVFS_TAG, fmt, ##args)
	#define idvfs_ver(fmt, args...)	\
		do {	\
			if (func_lv_mask_idvfs > 0) {\
				pr_info(ANDROID_LOG_INFO,	IDVFS_TAG, fmt,	##args);	\
				func_lv_mask_idvfs = (func_lv_mask_idvfs == 1) ? 1 :	\
									((func_lv_mask_idvfs == 2) ? 0 :	\
									(func_lv_mask_idvfs - 1)); }	\
		} while (0)
#else
	#define	idvfs_emerg(fmt, args...)	  pr_emerg(IDVFS_TAG fmt, ##args)
	#define	idvfs_alert(fmt, args...)	  pr_alert(IDVFS_TAG fmt, ##args)
	#define	idvfs_crit(fmt,	args...)	  pr_crit(IDVFS_TAG	fmt, ##args)
	#define	idvfs_error(fmt, args...)	  pr_err(IDVFS_TAG fmt,	##args)
	#define	idvfs_warning(fmt, args...)	  pr_warn(IDVFS_TAG	fmt, ##args)
	#define	idvfs_notice(fmt, args...)	  pr_notice(IDVFS_TAG fmt, ##args)
	#define	idvfs_info(fmt,	args...)	  pr_info(IDVFS_TAG	fmt, ##args)
	#define	idvfs_debug(fmt, args...)	  pr_debug(IDVFS_TAG fmt, ##args)
	#define idvfs_ver(fmt, args...)	\
		do {	\
			if (func_lv_mask_idvfs > 0) {\
				pr_info(IDVFS_TAG	fmt, ##args);	\
				func_lv_mask_idvfs = (func_lv_mask_idvfs == 1) ? 1 :	\
									((func_lv_mask_idvfs == 2) ? 0 :	\
									 (func_lv_mask_idvfs - 1)); }	\
		} while (0)
#endif

	/* For Interrupt use	*/
	#if	0 /*EN_ISR_LOG*/
		#define	idvfs_isr_info(fmt,	args...)  idvfs_debug(fmt, ##args)
	#else
		#define	idvfs_isr_info(fmt,	args...)
	#endif
#else
	#define	idvfs_emerg(fmt, args...)		printf(IDVFS_TAG fmt, ##args)
	#define	idvfs_alert(fmt, args...)		printf(IDVFS_TAG fmt, ##args)
	#define	idvfs_crit(fmt,	args...)		printf(IDVFS_TAG fmt, ##args)
	#define	idvfs_error(fmt, args...)		printf(IDVFS_TAG fmt, ##args)
	#define	idvfs_warning(fmt, args...)		printf(IDVFS_TAG fmt, ##args)
	#define	idvfs_notice(fmt, args...)		printf(IDVFS_TAG fmt, ##args)
	#define	idvfs_info(fmt,	args...)		printf(IDVFS_TAG fmt, ##args)
	#define	idvfs_debug(fmt, args...)		printf(IDVFS_TAG fmt, ##args)
	#define idvfs_ver(fmt, args...)			printf(IDVFS_TAG fmt, ##args)
#endif

#define DA9214_MV_TO_STEP(volt_mv)			((volt_mv -	300) / 10)
#define DA9214_STEP_TO_MV(step)				(((step & 0x7f) * 10) + 300)

/* static unsigned int func_lv_mask_idvfs	= (FUNC_LV_MODULE |	FUNC_LV_CPUFREQ
 | FUNC_LV_API | FUNC_LV_LOCAL | FUNC_LV_HELP); */
/* static unsigned int func_lv_mask_idvfs	= 0; */

static struct CHANNEL_STATUS channel_status[IDVFS_CHANNEL_NP] = {
		[IDVFS_CHANNEL_SWP] = {
			.name = __stringify(IDVFS_CHANNEL_SWP),
			.ch_number = IDVFS_CHANNEL_SWP,
			.status = 0,
			.percentage = 0
		},

		[IDVFS_CHANNEL_OTP] = {
			.name = __stringify(IDVFS_CHANNEL_OTP),
			.ch_number = IDVFS_CHANNEL_OTP,
			.status = 0,
			.percentage = 0
		},

		[IDVFS_CHANNEL_OCP] = {
			.name = __stringify(IDVFS_CHANNEL_OCP),
			.ch_number = IDVFS_CHANNEL_OCP,
			.status = 0,
			.percentage = 0
		},
	};

/*
idvfs_status, status machine
0: disable finish
1: enable finish
2: enable start
3: disable start
4: SWREQ start
5: disable and wait SWREQ finish
6: SWREQ finish can into disable
*/

static struct IDVFS_INIT_OPT idvfs_init_opt = {
	.idvfs_status = 0,
	.freq_max = IDVFS_FMAX_DEFAULT,
	.freq_min = IDVFS_FMIN_DEFAULT,
	.freq_cur = 750,
	.i2c_speed = 0,
	.ocp_endis = IDVFS_OCP_ENABLE,
	.otp_endis = IDVFS_OTP_ENABLE,
	.idvfs_ctrl_reg = IDVFS_CTRL_REG_DEFAULT,
	.idvfs_enable_cnt = 0,
	.idvfs_swreq_cnt = 0,
	.channel = channel_status,
	};

/* protect idvfs struct */
/* spin_lock(&idvfs_struct_lock);
   spin_unlock(&idvfs_struct_lock); */
DEFINE_SPINLOCK(idvfs_struct_lock);

/* iDVFSAPB function ****************************************************** */
/* return 0: Ok, -1: timeout */
static int iDVFSAPB_Write(unsigned int sw_pAddr, unsigned int sw_pWdata)
{
	unsigned int i = 0;

	_idvfs_write(IDVFSAPB_SW_ADDR, ((unsigned int)(sw_pAddr)));  /* addr, soft reset */
	_idvfs_write(IDVFSAPB_SW_DATA, ((unsigned int)(sw_pWdata))); /* data */
	_idvfs_write(IDVFSAPB_SW_WR, ((unsigned int)(0x01)));

	while (i++ < 10) {
		if (0x01 == (_idvfs_read(IDVFSAPB_HW_STATUS) & 0x1f))
			return 0;
		udelay(5);
		idvfs_ver("wait 5usec count = %u.\n", i);
	}

	idvfs_error("[iDVFSAPB_Write] Addr 0x%x = 0x%x, and timeout 50us.\n", sw_pAddr, sw_pWdata);
	return -1;
}

/* #ifndef __KERNEL__ zzz */
static unsigned int iDVFSAPB_Read(unsigned int sw_pAddr)
{
	unsigned int i = 0;

	/* chekc iDVFS disable */
	_idvfs_write(IDVFSAPB_SW_ADDR, ((unsigned int)(sw_pAddr)));
	_idvfs_write(IDVFSAPB_SW_RD, ((unsigned int)(0x01)));

	while (i++ < 10) {
		if (0x01 == (_idvfs_read(IDVFSAPB_HW_STATUS) & 0x1f))
			return _idvfs_read(IDVFSAPB_SW_PRDATA);
		udelay(5);
		idvfs_ver("wait 5usec count = %u.\n", i);
	}

	idvfs_error("[iDVFSAPB_Read] Addr 0x%x, wait timeout 50usec.\n", sw_pAddr);
	return -1;
}

#ifndef __KERNEL__
/* it's only for DA9214 PMIC */
int iDVFSAPB_DA9214_write(unsigned int sw_pAddr, unsigned int sw_pWdata)
{
	unsigned int i = 0;

	/* iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_CTRL, 0x20); */
	/* iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_LENGTH, 0x0102); */
	/* iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_TR_LENGTH, 0x0001); */

	/* slave addr: 0xd0(da8214), 0xd6(mt6313), I2C control slave addr: 0x84 */
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_PORT_DATA, (sw_pAddr & 0xff));
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_PORT_DATA, (sw_pWdata & 0xff));
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_START, 0x0001);

	while (i++ < 10) {
		if (0x00 == iDVFSAPB_Read(IDVFSAPB_I2C_DUAL_START)) {
			/* clear IRQ status */
			iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_INT_STATUS, 0x0001);
			return 0;
		}
		udelay(5);
		idvfs_ver("wait 5usec count = %u.\n", i);
	}

	idvfs_error("[iDVFSAPB_DA9214_writ] Addr 0x%x = 0x%x, wait timeout 50usec.\n", sw_pAddr, sw_pWdata);
	return -1;
}

/* it's only for DA9214 PMIC but not work when iDVFS enable!!! */
static int iDVFSAPB_DA9214_read(unsigned int sw_pAddr)
{
	unsigned int i = 0;

	/* dir change */
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_CTRL, 0x0010);
	/* transfer_aux_len and transfer_len */
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_LENGTH, 0x0101);
	/* 2 transac: 1byte TX then 1 byte RX */
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_TR_LENGTH, 0x0002);
	/* slave addr: 0xd0(da8214), 0xd6(mt6313), I2C control slave addr: 0x84 */
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_PORT_DATA, (sw_pAddr & 0xff));
	/* slave addr: 0xd0(da8214), 0xd6(mt6313), I2C control slave addr: 0x84 */
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_START, 0x0001);

	while (i++ < 10) {
		if (0x00 == iDVFSAPB_Read(IDVFSAPB_I2C_DUAL_START))
			return iDVFSAPB_Read(IDVFSAPB_I2C_DUAL_PORT_DATA);
		udelay(5);
		idvfs_ver("wait 5usec count = %u.\n", i);
	}

	idvfs_error("[iDVFSAPB_DA9214_read] Addr 0x%x, wait timeout 50usec.\n", sw_pAddr);
	return -1;
}

static int iDVFSAPB_pmic_manual_big(int	volt_mv) /* it's only for DA9214 PMIC  */
{
	int i;

	/* voltage range 300 ~ 1200mv */
	if ((volt_mv > 1200) || (volt_mv < 300))
		return -1;

	iDVFSAPB_DA9214_write(0xd9, (DA9214_MV_TO_STEP(volt_mv) | 0x0080));
}
#endif

int iDVFSAPB_init(void) /* it's only for DA9214 PMIC, return 0: 400K, 1:3.4M */
{
	/* unsigned char i2c_spd_reg; */

	/* check PMIC support I2C speed when first init */
#if 0
	if (idvfs_init_opt.i2c_speed == 0) {
		/* check DA9214 i2c speed */
		/* bit 6 R/W PM_IF_HSM Enables continuous */
		da9214_config_interface(0x0, 0x2, 0xF, 0); /* select to page 2,3 */
		da9214_read_interface(0x06, &i2c_spd_reg, 0xff, 0);
		da9214_config_interface(0x0, 0x1, 0xF, 0); /* back to page 1 */

		spin_lock(&idvfs_struct_lock);
		idvfs_init_opt.i2c_speed = ((i2c_spd_reg & 0x40) ? 3400 : 400);
		spin_unlock(&idvfs_struct_lock);
		idvfs_ver("iDVFSAPB: First init to get DA9214 HSM mode reg = 0x%x, Speed = %uKHz.\n",
				i2c_spd_reg, idvfs_init_opt.i2c_speed);
	}
#else
	/* due to preloader alread setting speed, and don't switch page when kernel initial */
	spin_lock(&idvfs_struct_lock);
	idvfs_init_opt.i2c_speed = 3400;
	spin_unlock(&idvfs_struct_lock);
#endif

	/* PMIC i2c pseed and set iDVFSAPB ctrl 3.4M = 0x1001, 400K = 0x1303 */
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_TIMING, ((idvfs_init_opt.i2c_speed == 3400) ? 0x1001 : 0x1303));
	/* I2C control slave addr reg: 0x84, PMIC slave addr: 0xd0(da8214), 0xd6(mt6313) */
	iDVFSAPB_Write(IDVFSAPB_I2C_DUAL_SLAVE_ADDR, 0x00d0);
	idvfs_ver("iDVFSAPB: Timming ctrl(0xa0) = 0x%x.\n",
		iDVFSAPB_Read(IDVFSAPB_I2C_DUAL_TIMING));
	idvfs_ver("iDVFSAPB: Slave address(0x84) = 0x%x.(DA9214(0xd0) or MT6313(0xd6))\n",
		iDVFSAPB_Read(IDVFSAPB_I2C_DUAL_SLAVE_ADDR));

	/* HW pmic volt ctrl reg addr, DA9214: L/LL = 0xd7, Big = 0xd9, MT6313: L/LL = 0x??, Big = 0x96? */
	_idvfs_write(IDVFSAPB_HW_PMIC_ADDR, ((unsigned int)(0xd9)));

	/* return 400 or 3400 */
	return idvfs_init_opt.i2c_speed;
}

/* iDVFSAPB function *********************************************************************** */

/* Freq meter ****************************************************************************** */
unsigned int _mt_get_cpu_freq_idvfs(unsigned int num)
{
	int output = 0, i = 0;
	unsigned int temp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	/* 0x10000000 0x1000, clock meter reg */
	/* CLK_DBG_CFG=0x1000010C */
	clk_dbg_cfg = _idvfs_read(IDVFSCLK_BASE + 0x10c);
	/* sel abist_cksw and enable freq meter sel abist */
	_idvfs_write(IDVFSCLK_BASE + 0x10c, (clk_dbg_cfg & 0xFFFFFFFE) | (num << 16));
	/* sel abist_cksw and enable freq meter sel abist */
	/* DRV_WriteReg32(0x1000010c, (clk_dbg_cfg & 0xFFFFFFFC) | (num << 16)); */

	/* CLK_MISC_CFG_0=0x10000104 */
	clk_misc_cfg_0 = _idvfs_read(IDVFSCLK_BASE + 0x104);
	/* select divider, WAIT CONFIRM */
	_idvfs_write(IDVFSCLK_BASE + 0x104, (clk_misc_cfg_0 & 0x01FFFFFF));

	/* CLK26CALI_1=0x10000224 */
	clk26cali_1 = _idvfs_read(IDVFSCLK_BASE + 0x224);
	/* cycle count default 1024,[25:16]=3FF */
	/* DRV_WriteReg32(CLK26CALI_1, 0x00ff0000); */

	/* temp = DRV_Reg32(CLK26CALI_0); */
	_idvfs_write(IDVFSCLK_BASE + 0x220, 0x1000); /* CLK26CALI_0=0x10000220 */
	_idvfs_write(IDVFSCLK_BASE + 0x220, 0x1010);


	/* wait frequency meter finish */
	while (_idvfs_read(IDVFSCLK_BASE + 0x220) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = _idvfs_read(IDVFSCLK_BASE + 0x224) & 0xFFFF;
	/* Khz */
	output = (((temp * 26000)) / 1024 * 2);

	_idvfs_write(IDVFSCLK_BASE + 0x10c, clk_dbg_cfg);
	_idvfs_write(IDVFSCLK_BASE + 0x104, clk_misc_cfg_0);
	_idvfs_write(IDVFSCLK_BASE + 0x220, 0x1010);
	_idvfs_write(IDVFSCLK_BASE + 0x220, 0x1000);
	/* idvfs_write(0x10000220, clk26cali_0); */
	/* idvfs_write(0x10000224, clk26cali_1); */

	/* cpufreq_dbg("CLK26CALI_1 = 0x%x, CPU freq = %u KHz\n", temp, output); */

	if (i > 10) {
		/* cpufreq_dbg("meter not finished!\n"); */
		return 0;
	} else {
		return output;
	}
}
/* Freq meter *********************************************************************** */

/* iDVFSAPB function ***********************************************************************  */
/* iDVFS function code */
/* if return 0:ok, -1:invalid parameter, -2: iDVFS is enable */
/* int BigiDVFSEnable(unsigned int Fmax, unsigned int cur_vproc_mv_x100, unsigned int cur_vsram_mv_x100) */
int BigiDVFSEnable_hp(void) /* for cpu hot plug call */
{

#ifndef ENABLE_IDVFS
	/* iDVFS not enable */
	return -1;
#else /* ENABLE_IDVFS */

	unsigned char ret_val = 0;
	unsigned int cur_vproc_mv_x100, cur_vsram_mv_x100;

	/* for back legacy DVFS mode debug */
	if (disable_idvfs_flag)
		return -6;

	/* sync when ptp1 enable then enable iDVFS */
	if (infoIdvfs == 0xff) {
		/* true eFuse enable ptp */
		idvfs_ver("[****]iDVFS Start Enable: chk ptp1 init ok!\n");
	} else {
		/* if (infoIdvfs == 0x55) { */
			/* empty eFuse, and init Big ptp by temp eFuse */
			/* eem_init_det_tmp(); */
			/* idvfs_ver("[****]iDVFS Start Enable: empty ptp1 eFuse, disable iDVFS.\n"); */
			/* swithc eFuse enable ptp finish */
			/* infoIdvfs = 0xff; */
			/* break; */
		/* } */
		idvfs_ver("iDVFS not enable and wait ptp1 enable, infoIdvfs = 0x%x.\n", infoIdvfs);
		return -5;
	}

	/* idvfs status manchine */
	/* 0: disable finish, 1: enable finish, 2: enable start, 3: disable start */
	spin_lock(&idvfs_struct_lock);
	if (idvfs_init_opt.idvfs_status == 1) {
		spin_unlock(&idvfs_struct_lock);
		return 0;
	} else if (idvfs_init_opt.idvfs_status == 0) {
		idvfs_init_opt.idvfs_status = 2;
		AEE_RR_REC(idvfs_state_manchine, 2);
	} else {
		/* should be not enable and disable start at the same time */
		BUG_ON(1);
	}
	spin_unlock(&idvfs_struct_lock);

#if 0
	/* check pos div only 0 or 1 */
	if (((SEC_BIGIDVFS_READ(0x102224a0) & 0x00007000) >> 12) >= 2) {
		idvfs_error("iDVFS enable pos div = %u, (only 0 or 1).\n",
				((SEC_BIGIDVFS_READ(0x102224a0) & 0x00007000) >> 12));
		/* pos div error */
		spin_lock(&idvfs_struct_lock);
		idvfs_init_opt.idvfs_status = 0;
		spin_unlock(&idvfs_struct_lock);
		AEE_RR_REC(idvfs_state_manchine, 0);
		return -3;
	}
#endif

	/* initial clk div = 1 */
	/* 01001: 3/4, 10001:4/5, 11001: 5/6, 11100:2/6, 01000: 4/4 */
	mt6797_0x1001AXXX_reg_set(0x274, 0x1f, 0x8);
	udelay(2);

#if IDVFS_CCF_I2C6
	/* I2CV6 (APPM I2C) clock CCF control enable */
	if (clk_enable(idvfs_i2c6_clk)) {
		idvfs_error("I2C6 CLK Ctrl enable fail.\n");
		spin_lock(&idvfs_struct_lock);
		idvfs_init_opt.idvfs_status = 0;
		spin_unlock(&idvfs_struct_lock);
		AEE_RR_REC(idvfs_state_manchine, 0);
		return -4;
	}
#endif

	/* move to prob init */
	iDVFSAPB_init();

	/* get current vproc volt */
	da9214_read_interface(0xd9, &ret_val, 0x7f, 0);
	cur_vproc_mv_x100 = ((DA9214_STEP_TO_MV(ret_val & 0x7f)) * 100);

	/* before BigiDVFS enable aee pmic status */
	/* da9214_read_interface(0xd9, &ret_val, 0x7f, 0); */ /* due to before read */
	AEE_RR_REC(idvfs_curr_volt, ((AEE_RR_CURR(idvfs_curr_volt) & 0xff00) | ret_val));
	da9214_read_interface(0x5e, &ret_val, 0xff, 0);
	AEE_RR_REC(idvfs_curr_volt, ((AEE_RR_CURR(idvfs_curr_volt) & 0x00ff) | (ret_val << 8)));

	/* fix sarm = 1180mv, change to 1200mv due to t-mode */
	BigiDVFSSRAMLDOSet(120000);
	cur_vsram_mv_x100 = 120000;
	udelay(20); /* wait LDO set stable */

	idvfs_ver("iDVFS enable cur Vproc = %u(mv_x100), Vsarm = %u(mv_x100).\n",
			cur_vproc_mv_x100, cur_vsram_mv_x100);

#if IDVFS_DREQ_ENABLE
	/* BigDREQHWEn(1130, 1030); DREQ ON */
	SEC_BIGIDVFS_WRITE(0x102222b8, 0xc1500);
	SEC_BIGIDVFS_WRITE(0x102222b8, 0xc1501);
	SEC_BIGIDVFS_WRITE(0x102222b8, 0xc1505);
	udelay(2); /* wait DREQ stable */
#endif

	/* auto set AVG status, enable at ATF code */
	/* BigiDVFSSWAvg(0, 1); */

	/* call smc function_id = SMC_IDVFS_BigiDVFSEnable(Fmax, Vproc_mv_x100, Vsram_mv_x100) */
	SEC_BIGIDVFSENABLE(idvfs_init_opt.idvfs_ctrl_reg, cur_vproc_mv_x100, cur_vsram_mv_x100);
	/* ram console, 0x00107203 is ATF define */
	AEE_RR_REC(idvfs_ctrl_reg, (idvfs_init_opt.idvfs_ctrl_reg | 0x1));

	/* enable sw channel status and clear oct/otpl channel status by struct */
	spin_lock(&idvfs_struct_lock);
	idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].status = 1;
	idvfs_init_opt.channel[IDVFS_CHANNEL_OTP].status = 0;
	idvfs_init_opt.channel[IDVFS_CHANNEL_OCP].status = 0;

	/* normal start percentage 30% */
	idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage = 3000;
	idvfs_init_opt.freq_cur = 750;
	spin_unlock(&idvfs_struct_lock);
	AEE_RR_REC(idvfs_swreq_curr_pct_x100, 3000);
	AEE_RR_REC(idvfs_swreq_next_pct_x100, 3000);

	/* idvfs_ver("iDVFS Enable OCP/OTP channel.\n"); */
	/* cfg or init OCP then enable OCP channel */
	if (idvfs_init_opt.ocp_endis)
		Cluster2_OCP_ON();

	/* cfg or init OTP then enable OTP channel */
	/* mark channel enable, otp will chk itself */
	/* BigiDVFSChannel(2, 1); */
	if (idvfs_init_opt.otp_endis)
		BigOTPEnable();

	/* set Freq to target init freq */
	/* if (idvfs_init_opt.freq_init != 750) {
		BigIDVFSFreq(idvfs_init_opt.freq_init * 4));
		idvfs_init_opt.freq_cur = idvfs_init_opt.freq_init;
	} */
	/* default 100% freq start and enable sw channel */
	/* BigiDVFSSWAvgStatus(); */

	/* enable struct idvfs_status = 1, 1: enable finish */
	spin_lock(&idvfs_struct_lock);
	idvfs_init_opt.idvfs_status = 1;
	++idvfs_init_opt.idvfs_enable_cnt;
	spin_unlock(&idvfs_struct_lock);
	AEE_RR_REC(idvfs_state_manchine, 1);
	AEE_RR_REC(idvfs_enable_cnt, idvfs_init_opt.idvfs_enable_cnt);

	idvfs_ver("[****]iDVFS enable success. Fmax = %u MHz, Fcur = %uMHz.\n",
	IDVFS_FMAX_DEFAULT, idvfs_init_opt.freq_cur);

	return 0;

#endif /* ENABLE_IDVFS */
}

/* return 0: Ok, -1: timeout */
/* define BigiDVFSDisable_hp(void) */
/* int BigiDVFSDisable(void) */
int BigiDVFSDisable_hp(void) /* chg for hot plug */
{
#ifndef ENABLE_IDVFS
	/* iDVFS not enable */
	return -1;
#else
	int i;
	/* static void __iomem *reg_base; */
	/* unsigned char ret_val = 0; */

	/* for back legacy DVFS mode debug */
	if (disable_idvfs_flag)
		return -6;

	/* idvfs status manchine */
	/* 0: disable finish, 1: enable finish, 2: enable start, 3: disable start */
	spin_lock(&idvfs_struct_lock);
	if (idvfs_init_opt.idvfs_status == 0) {
		spin_unlock(&idvfs_struct_lock);
		return 0;
	} else if (idvfs_init_opt.idvfs_status == 1) {
		idvfs_init_opt.idvfs_status = 3;
		AEE_RR_REC(idvfs_state_manchine, 3);
	} else {
		/* should be not enable and disable start at the same time */
		BUG_ON(1);
	}
	spin_unlock(&idvfs_struct_lock);

	/* before BigiDVFS disable aee pmic status */
	/* da9214_read_interface(0xd9, &ret_val, 0x7f, 0);
	AEE_RR_REC(idvfs_curr_volt, ((AEE_RR_CURR(idvfs_curr_volt) & 0xff00) | ret_val));
	da9214_read_interface(0x5e, &ret_val, 0xff, 0);
	AEE_RR_REC(idvfs_curr_volt, ((AEE_RR_CURR(idvfs_curr_volt) & 0x00ff) | (ret_val << 8))); */

	idvfs_ver("[****]iDVFS start disable.\n");
	/* idvfs_ver("iDVFS disable OCP/OTP channel.\n"); */
	/* disable OCP channel */
	/* enable depend on .ocp_endis, but desable by .status
	   due to adb command direct disable .ocp_endis ctrl */
	/* BigiDVFSChannel(1, 0); */
	/* BigOCPDisable(); */

#if 0
	/* check cpu8 into WFI */
	i = 0;
	/* reg_base = ioremap(0x10220000, 0x1000); */
	idvfs_ver("Check all big core in WFI.\n");
	/* writel(reg_base + 0x2400, 0x1b); */
	SEC_BIGIDVFS_WRITE(0x10222400, 0x1b);
	while (1) {
		/* bit[1:0] = core8/9 WFI status bit */
		/* if ((readl(reg_base + 0x2404) & 0x03) == 0x03) */
		if ((SEC_BIGIDVFS_READ(0x10222404) & 0x03) == 0x03)
			break;

	/* total wait 50*1000 = 50ms */
		udelay(50);
	if (i++ >= 1000)
		BUG_ON(1);
	}
	/* iounmap(reg_base); */
	idvfs_ver("All big core in WFI success.\n");
#endif

	/* down to 30% = 750MHz(IDVFS_FREQMz_STORE) for disable */
	/* BigIDVFSFreq(3000), ()IDVFS_FREQMz_STORE / 4) * 100 = 30 */
	idvfs_ver("iDVFS disable force setting FreqREQ = 30%%, 750MHz(IDVFS_FREQMz_STORE)\n");
	/* SEC_BIGIDVFS_WRITE(0x10222498, ((IDVFS_FREQMz_STORE / 25) << 12)); */
	SEC_BIGIDVFS_SWREQ(((IDVFS_FREQMz_STORE / 25) << 12));
	spin_lock(&idvfs_struct_lock);
	idvfs_init_opt.freq_cur = 750;
	spin_unlock(&idvfs_struct_lock);

	/* force disable otp/ocp channel first */
	SEC_BIGIDVFS_WRITE(0x10222470, (SEC_BIGIDVFS_READ(0x10222470) & 0xfffffff3));
	spin_lock(&idvfs_struct_lock);
	idvfs_init_opt.channel[IDVFS_CHANNEL_OTP].status = 0;
	idvfs_init_opt.channel[IDVFS_CHANNEL_OCP].status = 0;
	spin_unlock(&idvfs_struct_lock);

	/* wait 750Mhz ready */
	udelay(120);

	if (idvfs_init_opt.ocp_endis)
		Cluster2_OCP_OFF();

	/* disable OTP channel */
	/* wait otp move to itself disable channel */
	/* BigiDVFSChannel(2, 0); */
	if (idvfs_init_opt.otp_endis)
		BigOTPDisable();

	/* call smc */ /* function_id = SMC_IDVFS_BigiDVFSDisable */
	SEC_BIGIDVFSDISABLE();
	AEE_RR_REC(idvfs_ctrl_reg, 0x0);

	/* When next iDVFS enable Freq = 750MHz(default)
	 chk touch SRAMLDO must before cluster rst active */
	/* set Vsram = 1100mv for 750MHz */
	BigiDVFSSRAMLDOSet(IDVFS_VSRAM_STORE);

	/* set Vproc = 1000mv for 750MHz(IDVFS_FREQMz_STORE), due to PTP VBOOT */
	da9214_vosel_buck_b(IDVFS_VPROC_STORE);

	udelay(20); /* settle time Max(pmic, sarm) */

#if IDVFS_DREQ_ENABLE
	/* if dreq enable */
	/* BigDREQSWEn(0); */
#endif

#if IDVFS_CCF_I2C6
	/* I2CV6 (APPM I2C) clock CCF control disable */
	clk_disable(idvfs_i2c6_clk);
#endif

	/* clear all channel status by struct */
	spin_lock(&idvfs_struct_lock);
	for (i = 0; i < IDVFS_CHANNEL_NP; i++) {
		idvfs_init_opt.channel[i].status = 0;
		idvfs_init_opt.channel[i].percentage = 0;
	}

	/* disable struct, 0: disable finish */
	idvfs_init_opt.idvfs_status = 0;
	spin_unlock(&idvfs_struct_lock);
	AEE_RR_REC(idvfs_state_manchine, 0);

	idvfs_ver("[****]iDVFS disable success.\n");
	return 0;

#endif
}

/* return 0:Ok, -1: Invalid Parameter */
int BigiDVFSChannel(unsigned int Channelm, unsigned int EnDis)
{
	if ((Channelm >= 3) || (EnDis >= 2)) {
		/* iDVFS disable stage */
		idvfs_ver("iDVFS channel = %d, EnDis = %d, out of range\n", Channelm, EnDis);
		return -1;
	}

	/* call smc */
	/* function_id = SMC_IDVFS_BigiDVFSChannel */
	/* rc = SEC_BIGIDVFSCHANNEL(Channelm, EnDis); */
	/* setting register */
	switch (Channelm) {
	case 0:
		/* SW channel, bit[1] */
		SEC_BIGIDVFS_WRITE(0x10222470,
			(SEC_BIGIDVFS_READ(0x10222470) & 0xfffffffd) | (EnDis << 1));
		AEE_RR_REC(idvfs_ctrl_reg,
			((AEE_RR_CURR(idvfs_ctrl_reg) & 0xfffffffd) | (EnDis << 1)));
		break;
	case 1:
		/* OCP channel, bit[2] */
		if (idvfs_init_opt.ocp_endis) {
			SEC_BIGIDVFS_WRITE(0x10222470,
				(SEC_BIGIDVFS_READ(0x10222470) & 0xfffffffb) | (EnDis << 2));
			AEE_RR_REC(idvfs_ctrl_reg,
				((AEE_RR_CURR(idvfs_ctrl_reg) & 0xfffffffb) | (EnDis << 1)));
		}
		break;
	default: /* case 2: */
		/* OTP channel, bit[3] */
		if (idvfs_init_opt.otp_endis) {
			SEC_BIGIDVFS_WRITE(0x10222470,
				(SEC_BIGIDVFS_READ(0x10222470) & 0xfffffff7) | (EnDis << 3));
			AEE_RR_REC(idvfs_ctrl_reg,
				((AEE_RR_CURR(idvfs_ctrl_reg) & 0xfffffff7) | (EnDis << 1)));
		}
		break;
	}
	/* setting struct */
	spin_lock(&idvfs_struct_lock);
	idvfs_init_opt.channel[Channelm].status = EnDis;
	spin_unlock(&idvfs_struct_lock);
	idvfs_ver("iDVFS channel %s select success, EnDis = %u.\n",
		idvfs_init_opt.channel[Channelm].name, EnDis);
	return 0;
}

int BigiDVFSChannelGet(unsigned int Channelm)
{
	/* return struct value, enable must be through BigiDVFSChannel API */
	return idvfs_init_opt.channel[Channelm].status;
}

/* return pct_x100, input 7+12 type pct*/
unsigned int GetDecInterger(unsigned int hexpct)
{
	unsigned int i, pct_x100 = 0;

	/* get freq_swreq integer */
	pct_x100 = (hexpct >> 12) * 10000;

	/* get freq_swreq decimal */
	for (i = 0 ; i < 12 ; i++) {
		if (hexpct & (1 << (11 - i)))
			pct_x100 += (5000 / (1 << i));
	}

	/* scaling 100, adj 0.00t if t >= 4 add 1 */
	pct_x100 = (pct_x100 / 100) + (((pct_x100 % 100) >= 40) ? 1 : 0);

	return pct_x100;
}

/* return 0 Ok. -1 Error. Invalid parameter. No action taken. */
/* This function enables SW to set the frequency of maximum unthrottled operating */
/* frequency percentage. The percentage is relative to the FMaxMHz set when */
/* enabled. The IDVFS SW channel must be enabled before using this function. */
int BigIDVFSFreq(unsigned int Freqpct_x100)
{
	unsigned int i, freq_swreq, temp_pct_x100;

	/* check freqpct */
	/* ATF clemp 20.41% ~ 116% percentage */
	/* ((100% * 100 scaling) / IDVFS_FMAX_DEFAULT) = 4*/
	/* if ((Freqpct_x100 < (100)) || (Freqpct_x100 > (10000))) { */
	Freqpct_x100 =
		((Freqpct_x100 <= (idvfs_init_opt.freq_min * 4)) ? (idvfs_init_opt.freq_min * 4) :
		((Freqpct_x100 >= (idvfs_init_opt.freq_max * 4)) ? (idvfs_init_opt.freq_max * 4) :
		  Freqpct_x100));

	/* get freq_swreq integer */
	freq_swreq = (Freqpct_x100 / 100) << 12;

	/* get freq_swreq decimal */
	temp_pct_x100 = (Freqpct_x100 % 100);
	for (i = 0 ; (i < 12) && (temp_pct_x100 != 0) ; i++) {
		temp_pct_x100 <<= 1;
		if (temp_pct_x100 >= 100) {
			temp_pct_x100 -= 100;
			freq_swreq |= (1 << (11 - i));
		}
	}

	/* for ram console printf */
	AEE_RR_REC(idvfs_curr_volt,
		((AEE_RR_CURR(idvfs_curr_volt) & 0xff00) | ((SEC_BIGIDVFS_READ(0x102224c8) & 0xff00) >> 8)));

	/* leave lower 505MHz */
	if ((Freqpct_x100 >= 2000) && (idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage < 2000)) {
		idvfs_ver("Low Freq leave %uMHz.\n",
			(idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage / 4));
		/* 01001: 3/4, 10001:4/5, 11001: 5/6, 11100:2/6, 01000: 4/4 */
		mt6797_0x1001AXXX_reg_set(0x274, 0x1f, 0x8);
		udelay(2);
		/* get freq_swreq integer, swavg only 7+8 bit need shift to 7+12 bit */
		AEE_RR_REC(idvfs_swavg_curr_pct_x100, idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage);
	} else
		AEE_RR_REC(idvfs_swavg_curr_pct_x100, (GetDecInterger((((SEC_BIGIDVFS_READ(0x102224cc) >> 16)
			& 0x7fff) << 4))));

	/* call smc */
	/* function_id = SMC_IDVFS_BigiDVFSFreq */
	/* rc = SEC_BIGIDVFSFREQ(temp_pct_x100); */
	/* Frepct_x100 = 100(1%) ~ 10000(100%) */
	/* swreq = cur/max */

	/* into ATF for new setting */
	/* SEC_BIGIDVFS_WRITE(0x10222498, freq_swreq); */
	SEC_BIGIDVFS_SWREQ(freq_swreq);

	/* into lower 505MHz */
	if ((Freqpct_x100 < 2000) && (idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage > 2000)) {
		idvfs_low_freq_cnt++;
		idvfs_ver("Low Freq into %uMHz, count = %u.\n",
		(Freqpct_x100 / 4), idvfs_low_freq_cnt);
		/* for iDVFS stable */
		/* ARMPLLDIV_CKDIV = 0x1001a274 */
		/* 10010:3/5 , 11010:4/6, 01000: 4/4 */
		/* 507*3/5=304.2MHz,1217%, 507*4/6=338MHz,1352% */
		if (Freqpct_x100 < 1300)
			mt6797_0x1001AXXX_reg_set(0x274, 0x1f, 0x12);
		else
			mt6797_0x1001AXXX_reg_set(0x274, 0x1f, 0x1a);
		udelay(2);
	}

	idvfs_ver("Set Freq: SWP_cur_pct_x100 = %u, SWP_new_pct_x100 = %u, freq_swreq = 0x%x.\n",
				idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage, Freqpct_x100, freq_swreq);

	spin_lock(&idvfs_struct_lock);
	++idvfs_init_opt.idvfs_swreq_cnt;
	idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage = Freqpct_x100;
	spin_unlock(&idvfs_struct_lock);

	/* for ram console printf */
	AEE_RR_REC(idvfs_swreq_cnt, idvfs_init_opt.idvfs_swreq_cnt);
	AEE_RR_REC(idvfs_swreq_curr_pct_x100, idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage);
	AEE_RR_REC(idvfs_swreq_next_pct_x100, Freqpct_x100);

	return 0;
}

/* To fix FreqSWREQ */
int BigIDVFSFreqMaxMin(unsigned int maxpct_x100, unsigned int minpct_x100)
{
	/* Clamp min 12.17% ~ Turbo max 116% */
	maxpct_x100 =  ((maxpct_x100 <= 1200) ? 1200 :
					((maxpct_x100 >= 11600) ? 11600 : maxpct_x100));
	minpct_x100 = (minpct_x100 > maxpct_x100) ? maxpct_x100 : minpct_x100;

	spin_lock(&idvfs_struct_lock);
	idvfs_init_opt.freq_max = (maxpct_x100 / 4);
	idvfs_init_opt.freq_min = (minpct_x100 / 4);
	spin_unlock(&idvfs_struct_lock);
	return 0;
}

/* return 0:. Ok, -1: Parameter invalid */
int BigiDVFSSWAvg(unsigned int Length, unsigned int EnDis)
{
	if ((Length < 0) || (Length > 7)) {
		idvfs_error("iDVFS SWAvg: length must be 0~7 or Endis invalid.\n");
		return -1;
	}

	if ((EnDis < 0) || (EnDis > 1)) {
		idvfs_error("iDVFS SWAvg: Enab out of range..\n");
		return -2;
	}

	/* call smc */ /* function_id = SMC_IDVFS_BigiDVFSSWAvg */
	/* rc = SEC_BIGIDVFSSWAVG(Length, EnDis); */
	SEC_BIGIDVFS_WRITE(0x102224cc, ((Length << 4) | (EnDis)));

	idvfs_ver("iDVFS SWAvg: setting SWAvg success.\n");
	return 0;
}

/* return value of freq percentage x 100, 0(0%) ~ 10000(100%), -1: SWAvg not enabled */
int BigiDVFSSWAvgStatus(void)
{
	unsigned int freqpct_x100, sw_avgfreq = 0;

	/* iDVFS not enable, here is critical spin lock */
	spin_lock(&idvfs_struct_lock);
	if (idvfs_init_opt.idvfs_status != 1) {
		spin_unlock(&idvfs_struct_lock);
		return 0;
	}
	spin_unlock(&idvfs_struct_lock);

	/* call smc, function_id = SMC_IDVFS_BigiDVFSSWAvgStatus */
	sw_avgfreq = ((SEC_BIGIDVFS_READ(0x102224cc) >> 16) & 0x7fff);
	/* get freq_swreq integer, swavg only 7+8 bit need shift to 7+12 bit */
	freqpct_x100 = GetDecInterger(sw_avgfreq << 4);

	if ((idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage < 2000) &&
		(freqpct_x100 <= 2100))
		freqpct_x100 = idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage;

	if (freqpct_x100 == 0) {
		spin_lock(&idvfs_struct_lock);
		idvfs_init_opt.freq_cur = 0;
		spin_unlock(&idvfs_struct_lock);
		idvfs_ver("iDVFS or SW AVG not enable, SWAVG = 0.\n");
	} else {
		spin_lock(&idvfs_struct_lock);
		idvfs_init_opt.freq_cur = (freqpct_x100 / 4);
		spin_unlock(&idvfs_struct_lock);
		idvfs_ver("Get Freq: SWP_cur_pct_x100 = %u, Phy_get_pct_x100 = %u, sw_avgfreq = 0x%x.\n",
				idvfs_init_opt.channel[IDVFS_CHANNEL_SWP].percentage, freqpct_x100, sw_avgfreq);
		/* may be add prt OTP/OCP infor */
	}
	return freqpct_x100;
}

int BigiDVFSPllSetFreq(unsigned int Freq)
{
	/* chk freq range */
	if ((Freq < 250) || (Freq > 3000)) {
		idvfs_error("Output Freq = %u out of 250 ~ 3000MHz range.\n", Freq);
		return -1;
	}

	/* if iDVFS enable change to iDVFS mode setting Freq */
	if (idvfs_init_opt.idvfs_status == 1)
		return BigIDVFSFreq(Freq * 4);

	/* if big cluster offline then return */
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return -1;

	SEC_BIGIDVFSPLLSETFREQ(Freq);
	idvfs_ver("Legacy iDVFS setting PLL Freq success. Freq = %uMHz.\n", Freq);
	spin_lock(&idvfs_struct_lock);
	idvfs_init_opt.freq_cur = Freq;
	spin_unlock(&idvfs_struct_lock);
	return 0;
}

unsigned int BigiDVFSPllGetFreq(void)
{
	unsigned int freq = 0, pos_div = 0;

	/* iDVFS mode use SWAVG to get Freq */
	if (idvfs_init_opt.idvfs_status == 1) {
		/* call smc, function_id = SMC_IDVFS_BigiDVFSSWAvgStatus */
		/* get freq_swreq integer, swavg only 7+8 bit need shift to 7+12 bit */
		return GetDecInterger(((SEC_BIGIDVFS_READ(0x102224cc) >> 16) & 0x7fff) << 4);
	}

	/* if big cluster offline then return  */
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return 0;

	/* quick response freq */
	/* return idvfs_init_opt.freq_cur; */

	/* default fcur = 1500MHz */
	freq = (((unsigned long long)(SEC_BIGIDVFS_READ(0x102224a4) & 0x7fffffff) * 26L) / (1L << 24));
	pos_div = (1 << ((SEC_BIGIDVFS_READ(0x102224a0) >> 12) & 0x7));

	idvfs_ver("Legacy iDVFS get PLL Freq = %uMHz, pos_div = %u, output Freq = %uMHZ.\n",
				freq, pos_div, (freq / pos_div));

	return (freq / pos_div);
}

#if 0
int BigiDVFSPllDisable(void)
{
	if ((SEC_BIGIDVFS_READ(0x102224a0) & 0x1) == 0) {
		idvfs_error("PLL already disable.\n");
		return -1;
	}

	/* idvfs_write_field(0x102224a0, 0:0, 0); */
	SEC_BIGIDVFS_WRITE(0x102224a0, (SEC_BIGIDVFS_READ(0x102224a0) & 0xfffffffe));
	udelay(1);
	/* idvfs_write_field(0x102224a0, 8:8, 0); */
	SEC_BIGIDVFS_WRITE(0x102224a0, (SEC_BIGIDVFS_READ(0x102224a0) & 0xfffffeff));
	udelay(1);

	idvfs_ver("Legacy iDVFS PLL disable success.\n");
	return 0;
}
#endif

/* bit[31:16] = SRAM LDO cal, bit[15:0] = eFuse cal */
unsigned int BigiDVFSSRAMLDOEFUSE(void)
{
	/* check big cluster online */
	/* 0x10206000 0x1000, Big eFUSE register */
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return (_idvfs_read(IDVFSEFUS_BASE + 0x66C) & 0xffff);
	else
		return (((SEC_BIGIDVFS_READ(0x102222b4) & 0xffff) << 16) |
				(_idvfs_read(IDVFSEFUS_BASE + 0x66C) & 0xffff));
}

int BigiDVFSSRAMLDOSet(unsigned int mVolts_x100)
{
	int	rc = 0;

	if ((mVolts_x100 < 50000) || (mVolts_x100 > 120000)) {
		idvfs_error("Error: SRAM LDO volte = %umv, out of range 500mv~1200mv.\n", mVolts_x100);
		return -1; /* out of range */
	}

	rc = SEC_BIGIDVFSSRAMLDOSET(mVolts_x100);
	/* due to iDVFS fix don't need recoder */
	AEE_RR_REC(idvfs_sram_ldo, (mVolts_x100 / 100));

	if (rc >= 0)
		idvfs_ver("SRAM LDO setting = %u(x100mv) success.\n", mVolts_x100);
	else
		idvfs_error("iDVFS set SRAM LDO volt fail and return rc = %u.\n", rc);

	return rc;
}

int BigiDVFSSRAMLDODisable(void)
{
	/* disable SRAMLDO volt, wait fix */
	/* idvfs_write_field(0x102222b0, 7:4, 0); */
	AEE_RR_REC(idvfs_sram_ldo, 0);

	return 0;
}

unsigned int BigiDVFSSRAMLDOGet(void)
{
	int vosel = 0, volt;

	/* if all big offline return store voltage */
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return IDVFS_VSRAM_STORE;

	vosel = (SEC_BIGIDVFS_READ(0x102222b0) & 0xf);

	switch (vosel) {
	case 0:
		volt = 105000;
		break;
	case 1:
		volt = 60000;
		break;
	case 2:
		volt = 70000;
		break;
	default:
		volt = (((vosel - 3) * 2500) + 90000);
		break;
	}

	/* idvfs_ver("SRAM LDO get = %u(x100mv).\n", volt); */
	return volt;
}

int BigiDVFSPOSDIVSet(unsigned int pos_div)
{

	/* if big cluster offline then return */
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return -1;

	if ((pos_div > 7) || (pos_div < 0)) {
		idvfs_error("iDVFS Pos Div set = %u, out of range 0 ~ 7.\n", pos_div);
		return -1;
	}

	/* idvfs_write_field(0x102224a0, 0:0, 0), set ARMPLL disable */
	SEC_BIGIDVFS_WRITE(0x102224a0, (SEC_BIGIDVFS_READ(0x102224a0) & 0xfffffffe));
	/* idvfs_write_field(0x102224a0, 14:12, pos_div), set ARMPLL_CON0 bit[12:14] for pos_div */
	SEC_BIGIDVFS_WRITE(0x102224a0, ((SEC_BIGIDVFS_READ(0x102224a0) & 0xffff0fff) | (pos_div << 12)));
	/* idvfs_write_field(0x102224a0, 0:0, 1), set ARMPLL enable */
	SEC_BIGIDVFS_WRITE(0x102224a0, (SEC_BIGIDVFS_READ(0x102224a0) | 0x1));
	return 0;
}

unsigned int BigiDVFSPOSDIVGet(void)
{
	/* if big cluster offline then return */
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return 0;
	else
		return ((SEC_BIGIDVFS_READ(0x102224a0) >> 12) & 0x7);	  /* 0~7 */
}

int BigiDVFSPLLSetPCM(unsigned int freq)	/* <1000 ~ = 3000(MHz), with our pos div value */
{
	/* if big cluster offline then return */
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return -1;

	if ((freq > 3000) || (freq <= 1000)) {
		idvfs_error("Source PLL Freq = %u out of 1000 ~ 3000MHz range.\n", freq);
		return -1;
	}

	/* idvfs_write_field(0x102224a0, 0:0, 0), set ARMPLL disable */
	SEC_BIGIDVFS_WRITE(0x102224a0, (SEC_BIGIDVFS_READ(0x102224a0) & 0xfffffffe));
	udelay(1);
	/* idvfs_write_field(0x102224a0, 0:0, 1), set ARMPLL enable */
	SEC_BIGIDVFS_WRITE(0x102224a0, (SEC_BIGIDVFS_READ(0x102224a0) | 0x1));
	udelay(1);

	/* pllsdm= cur freq = max freq */
	SEC_BIGIDVFS_WRITE(0x102224a4, ((SEC_BIGIDVFS_READ(0x102224a4) & 0x80000000) |
				(((unsigned	long long)(1L << 24) * (unsigned long long)freq) / 26)));
	udelay(1);

	/* idvfs_write_field(0x102224a4, 31:31, 1), toggle 1 */
	SEC_BIGIDVFS_WRITE(0x102224a4, (SEC_BIGIDVFS_READ(0x102224a4) | 0x80000000));
	udelay(1);
	/* idvfs_write_field(0x102224a4, 31:31, 0), toggle 0 */
	SEC_BIGIDVFS_WRITE(0x102224a4, (SEC_BIGIDVFS_READ(0x102224a4) & 0x7fffffff));
	udelay(1);

	/* idvfs_ver("Bigpll setting ARMPLL_CON0 = 0x%x,
	ARMPLL_CON1_PTP3 = 0x%x, Freq = %uMHz, POS_DIV = %u.\n",
	ptp3_reg_read(ARMPLL_CON0_PTP3), ptp3_reg_read(ARMPLL_CON1_PTP3),
	freq, rg_armpll_posdiv_r()); */

	return 0;
}

unsigned int BigiDVFSPLLGetPCW(void) /* <1000 ~ = 3000(MHz), with our pos div value */
{
	unsigned int freq;

	/* if big cluster offline then return */
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		return 0;

	/* default fcur = 1500MHz */
	freq = (((unsigned long long)(SEC_BIGIDVFS_READ(0x102224a4) & 0x7fffffff) * 26L) / (1L << 24));

	idvfs_ver("Get PLL PCW without pos_div and clk_div freq = %u.\n", freq);
	return freq;
}

#ifdef __KERNEL__ /* __KERNEL__ */

/* for iDVFS interrupt ISR handler */
#if IDVFS_INTERRUPT_ENABLE
int BigiDVFSISRHandler(void)
{
	if ((SEC_BIGIDVFS_READ(0x10222470) & 0x00004000) != 0) {
		idvfs_ver("iDVFS Vporc timeout interrupt. iDVFS ctrl = 0x%x, clear irq now....\n",
				SEC_BIGIDVFS_READ(0x10222470));
		WARN_ON(1);

		/* clear interrupt status */
		/* idvfs_write_field(0x10222470, 11:11, 1); */
		SEC_BIGIDVFS_WRITE(0x10222470, (SEC_BIGIDVFS_READ(0x10222470) | 0x800));
		/* idvfs_write_field(0x10222470, 11:11, 0); */
		SEC_BIGIDVFS_WRITE(0x10222470, (SEC_BIGIDVFS_READ(0x10222470) & 0xfffff7ff));

		/* force clear and start */
		/* idvfs_write_field(0x10222470, 0:0, 0); */
		SEC_BIGIDVFS_WRITE(0x10222470, (SEC_BIGIDVFS_READ(0x10222470) & 0xfffffffe));
		udelay(1);
		/* idvfs_write_field(0x10222470, 0:0, 1); */
		SEC_BIGIDVFS_WRITE(0x10222470, (SEC_BIGIDVFS_READ(0x10222470) | 0x1));
		udelay(1);

		/* wait interrupt status cleawr */
		/* while(); */
	}

	return 0;
}

/* iDVFS ISR Handler */
static irqreturn_t idvfs_big_isr(int irq, void *dev_id)
{
	/* FUNC_ENTER(FUNC_LV_MODULE); */

	/* print status */
	idvfs_ver("interrupt number: 331 for idvfs or otp interrupt trigger.\n");
	BigOTPISRHandler();
	BigiDVFSISRHandler();

	/* FUNC_EXIT(FUNC_LV_MODULE); */
	return IRQ_HANDLED;
}
#endif

#if defined(CONFIG_IDVFS_AEE_RR_REC) && !defined(EARLY_PORTING)
static void _mt_idvfs_aee_init(void)
{
	aee_rr_rec_idvfs_ctrl_reg(0);
	aee_rr_rec_idvfs_enable_cnt(0);
	aee_rr_rec_idvfs_swreq_cnt(0);
	aee_rr_rec_idvfs_curr_volt(0);
	aee_rr_rec_idvfs_swavg_curr_pct_x100(0);
	aee_rr_rec_idvfs_swreq_curr_pct_x100(0);
	aee_rr_rec_idvfs_swreq_next_pct_x100(0);
	aee_rr_rec_idvfs_sram_ldo(0);
	aee_rr_rec_idvfs_state_manchine(0);
}
#endif

static int idvfs_probe(struct platform_device *pdev)
{
	int err = 0;

	idvfs_ver("IDVFS Probe Initial.\n");
	idvfs_irq_number = 0;

#if IDVFS_INTERRUPT_ENABLE
	/*get idvfs irq num*/
	idvfs_irq_number = irq_of_parse_and_map(pdev->dev.of_node, 0);
	idvfs_ver("idvfs irq = %u.\n", idvfs_irq_number);

	/* get iDVFS IRQ */
	err = request_irq(idvfs_irq_number, idvfs_big_isr, IRQF_TRIGGER_HIGH, "idvfs_big_isp", NULL);
	if (err) {
		idvfs_error("iDVFS IRQ register failed: idvfs_isr_big (%u)\n", err);
		WARN_ON(1);
		return err;
	}
	idvfs_ver("iDVFS request irq ISR finish IRQ number = %u.\n", idvfs_irq_number);
#endif

	/* get CCF I2C6 register */
	idvfs_i2c6_clk = devm_clk_get(&pdev->dev, "i2c");
	err = clk_prepare(idvfs_i2c6_clk);
	if (err) {
		idvfs_error("FAILED TO PREPARE I2C CLOCK (%u). iDVFS only 750MHz.\n", err);
		spin_lock(&idvfs_struct_lock);
		idvfs_init_opt.idvfs_status = 0;
		spin_unlock(&idvfs_struct_lock);
		AEE_RR_REC(idvfs_state_manchine, 0);
		WARN_ON(1);
		return err;
	}
	idvfs_ver("Parepare I2C6 CLK Ctrl done. idvfs_i2c6_clk = 0x%lx.\n", (unsigned long)idvfs_i2c6_clk);

#if defined(CONFIG_IDVFS_AEE_RR_REC) && !defined(EARLY_PORTING)
	_mt_idvfs_aee_init();
	idvfs_ver("iDVFS ram console init.\n");
#endif

	return 0;
}

/* Device infrastructure */
static int idvfs_remove(struct platform_device *pdev)
{
	return 0;
}

static int idvfs_suspend(struct	platform_device *pdev, pm_message_t state)
{
	/*
	kthread_stop(idvfs_thread);
	*/

	idvfs_ver("IDVFS suspend\n");
	return 0;
}


static int idvfs_resume(struct platform_device *pdev)
{
	/*
	idvfs_thread = kthread_run(idvfs_thread_handler, 0, "idvfs xxx");
	if (IS_ERR(idvfs_thread))
	{
		printk("[%s]: failed to create idvfs xxx thread\n", __func__);
	}
	*/

	idvfs_ver("IDVFS resume\n");

	/* iDVFSAPB init depend on suspend power */
	/* iDVFSAPB_init */

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_idvfs_of_match[] = {
	{ .compatible = "mediatek,idvfs", },
	{},
};
#endif

static struct platform_driver idvfs_pdrv = {
	.probe		= idvfs_probe,
	.remove		= idvfs_remove,
	.shutdown	= NULL,
	.suspend	= idvfs_suspend,
	.resume		= idvfs_resume,
	.driver		= {
		.name		= "mt_idvfs_driver",
#ifdef CONFIG_OF
		.of_match_table = mt_idvfs_of_match,
#endif
	},
};

/* ----------------------------------------------------------------------------- */

#ifdef CONFIG_PROC_FS

static char *_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;
	if (count >= PAGE_SIZE)
		goto out;
	if (copy_from_user(buf,	buffer,	count))
		goto out;

	buf[count] = '\0';
	return buf;

out:
	free_page((unsigned	long)buf);
	return NULL;
}

/* idvfs_debug */
static int idvfs_debug_proc_show(struct seq_file *m, void *v)
{
	unsigned char ret_val;

	da9214_read_interface(idvfs_i2c_debug, &ret_val, 0xff, 0);
	seq_printf(m, "IDVFS debug (log level) = %u. I2C_reg[0x%x] = 0x%x.\n",
		func_lv_mask_idvfs, idvfs_i2c_debug, ret_val);
	return 0;
}

static ssize_t idvfs_debug_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);
	unsigned int dbg_lv;

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &dbg_lv))
		func_lv_mask_idvfs = dbg_lv;
	else
		idvfs_error("echo dbg_lv (dec) > /proc/idvfs/idvfs_debug\n");

	free_page((unsigned long)buf);
	return count;
}

/* dvt_test */
static int dvt_test_proc_show(struct seq_file *m, void *v)
{
	unsigned int i, Leakage = 0 , Total = 0, ClkPct = 0;
	unsigned char ret_val = 0;
	/* unsigned int armplldiv_mon_en; */
	/* seq_printf(m, "empty.\n"); */

	/* get freq cur */
	BigiDVFSSWAvgStatus();

	/* print struct IDVFS_INIT_OPT */
	seq_puts(m, "*** Only for debug message, for real time info pls check iDVFS log.\n");
	seq_printf(m, "IDVFS_INIT_OPT\n"
			"idvfs_status = %u\n"
			"freq_max = %u MHz\n"
			"freq_min = %u MHz\n"
			"freq_cur = %u MHz\n"
			"i2c_spd  = %u KHz\n"
			"OCP/OTP = %s/%s\n"
			"idvfs Enable/SWREQ cnt/lowF cnt = %u/%u/%u\n"
			"Idvfs_ctrl_reg = 0x%08x\n",
			idvfs_init_opt.idvfs_status,
			idvfs_init_opt.freq_max,
			idvfs_init_opt.freq_min,
			(idvfs_init_opt.idvfs_status) ? idvfs_init_opt.freq_cur : BigiDVFSPllGetFreq(),
			idvfs_init_opt.i2c_speed,
			(idvfs_init_opt.ocp_endis) ? "Enable" : "Disable",
			(idvfs_init_opt.otp_endis) ? "Enable" : "Disable",
			idvfs_init_opt.idvfs_enable_cnt,
			idvfs_init_opt.idvfs_swreq_cnt,
			idvfs_low_freq_cnt,
			idvfs_init_opt.idvfs_ctrl_reg);

#if 0
	/* enable all freq meter and get freq */
	/*!! Don't read/write 0x1-0-0-1-a-2-8-4 due to need patch for this register */
	/* armplldiv_mon_en = idvfs_read(0x1-0-0-1-a-2-8-4); */
	/* add for enable all monitor channel */
	/* idvfs_write(0x1-0-0-1-a-2-8-4, 0xffffffff); */
	seq_printf(m, "Big Freq meter = %uKHZ.\n", _mt_get_cpu_freq_idvfs(37));	/* or 46 */
	seq_printf(m, "LL Freq meter  = %uKHZ.\n", _mt_get_cpu_freq_idvfs(34));	/* 42 */
	seq_printf(m, "L Freq meter   = %uKHZ.\n", _mt_get_cpu_freq_idvfs(35));	/* 43 */
	seq_printf(m, "CCI Freq meter = %uKHZ.\n", _mt_get_cpu_freq_idvfs(36));	/* 44 */
	seq_printf(m, "Back Freq meter= %uKHZ.\n", _mt_get_cpu_freq_idvfs(45));
	/* idvfs_write(0x1-0-0-1-a-2-8-4, armplldiv_mon_en); */
#endif

	seq_puts(m, "================= 2015/11/10 Ver 5.1 ==================\n");

	/* if big cluster offline then return */
	if ((cpu_online(8) == 0) && (cpu_online(9) == 0))
		goto DVT_TEST_END;

	/* get otp and ocp percentage */
	if (idvfs_init_opt.channel[IDVFS_CHANNEL_OCP].status) {
		BigOCPCapture(1, 1, 0, 15);
		BigOCPCaptureStatus(&Leakage, &Total, &ClkPct);
		spin_lock(&idvfs_struct_lock);
		idvfs_init_opt.channel[IDVFS_CHANNEL_OCP].percentage = ClkPct;
		spin_unlock(&idvfs_struct_lock);
	}
	if (idvfs_init_opt.channel[IDVFS_CHANNEL_OTP].status) {
		spin_lock(&idvfs_struct_lock);
		idvfs_init_opt.channel[IDVFS_CHANNEL_OTP].percentage = BigOTPGetFreqpct();
		spin_unlock(&idvfs_struct_lock);
	}

	/* print channel status and name */
	for (i = 0; i < IDVFS_CHANNEL_NP; i++) {
		seq_printf(m, "Ch[%u]-%s, Pcent_x100 = %u, EnDIS = %u.",
		idvfs_init_opt.channel[i].ch_number,
		idvfs_init_opt.channel[i].name,
		idvfs_init_opt.channel[i].percentage,
		idvfs_init_opt.channel[i].status);
		if (i == IDVFS_CHANNEL_SWP)
			seq_puts(m, "\n");
		if (i == IDVFS_CHANNEL_OCP)
			seq_printf(m, " (L_PWR = %umW, T_TWR = %umW)\n", Leakage, Total);
		if (i == IDVFS_CHANNEL_OTP)
			seq_printf(m, " (Big thermal = %u)\n", get_immediate_big_wrap());
	}
	seq_printf(m, "iDVFS ctrl = 0x%x.\n", SEC_BIGIDVFS_READ(0x10222470));
	seq_printf(m, "iDVFS debugout = 0x%x.\n", SEC_BIGIDVFS_READ(0x102224c8));
	seq_printf(m, "SW AVG status = %u(%%_x100), Freq = %uMHz.\n",
				(idvfs_init_opt.freq_cur * 4), idvfs_init_opt.freq_cur);
	ret_val = ((SEC_BIGIDVFS_READ(0x10222470) & 0xf000) >> 12);
	switch (ret_val) {
	case 7:
		seq_printf(m, "Debug Freq = %uMHz.(POS_DIV=1 or 2)\n",
		(unsigned int)((((unsigned long long)(SEC_BIGIDVFS_READ(0x102224c8) & 0x7fffffff)) * 26) >> 24));
		break;
	case 8:
		seq_printf(m, "Debug POS_DIV = %u.\n", (SEC_BIGIDVFS_READ(0x102224c8) & 0x7));
		break;
	default: /* case 10: */
		i = SEC_BIGIDVFS_READ(0x102224c8);
		seq_printf(m, "Debug cur_vsram[7:0] = %u, cur_vproc[15:8] = %umv, next_vproc[15:8] = %umv.\n",
		(i & 0xff), ((((i & 0xff00) >> 8) * 10) + 300), ((((i & 0xff0000) >> 16) * 10) + 300));
		break;
	}

#if IDVFS_DREQ_ENABLE
	seq_printf(m, "Big DREQGet = %s.\n", ((BigDREQGet() == 0) ? "Open" : "Short"));
#endif

DVT_TEST_END:
	da9214_read_interface(0xd9, &ret_val, 0x7f, 0);
	seq_printf(m, "Big Vproc = %umv.\n", DA9214_STEP_TO_MV((ret_val & 0x7f)));
	seq_printf(m, "Big Vsram = %umv.\n", (BigiDVFSSRAMLDOGet()/100));
	seq_printf(m, "Big Vsram LDO_Cal/eFuse = 0x%x.\n", BigiDVFSSRAMLDOEFUSE());

	/* switch bank 0; */
	/* idvfs_write(0x1100b400, 0x003f0000); */
	/* seq_printf(m, "DCVALUES = 0x%x.\n", idvfs_read(0x1100b240)); */
	/* seq_printf(m, "EEMEN    = 0x%x.\n", idvfs_read(0x1100b238)); */

	/* reg dump */
	/* for(i = 0; i < 24; i++) */
	/* seq_printf(m, "Reg 0x%x = 0x%x.\n", (i + 0x470), idvfs_read(0x10222470 + i)); */

	/* cpu 8/9 online info */
	/* seq_printf(m, "CPU 8/9 online = %u / %u\n", cpu_online(8), cpu_online(9)); */
	seq_puts(m, "======================================================\n");

	return 0;
}

static ssize_t dvt_test_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = (char *)_copy_from_user_for_proc(buffer, count);
	int rc = 0xff, err = 0;
	unsigned int func_code, func_para[4];
	unsigned char ret_val = 0;

	if (!buf)
		return -EINVAL;

	/* get scanf length and parameter */
	err = sscanf(buf, "%u %u %u %u %u", &func_code, &func_para[0], &func_para[1], &func_para[2], &func_para[3]);
	if (err > 0) {
#if 0
		/* print input information */
		switch (err) {
		case 2:
			idvfs_info("Input = %u, Para = %u.\n", err, func_para[0]);
			break;
		case 3:
			idvfs_info("Input = %u, Para = %u, %u.\n", err, func_para[0], func_para[1]);
			break;
		default: /* case 4: */
			idvfs_info("Input = %u, Para = %u, %u, %u.\n", err, func_para[0], func_para[1], func_para[2]);
			break;
		}
#endif

		/* switch function code */
		switch (func_code) {
		case 0:
			/* FMax = 500 ~ 3000, Vproc_mv_x100 = 50000~120000, VSram_mv_x100 = 50000~120000 */
			if ((err == 4) || (err == 2))
				rc = BigiDVFSEnable_hp();
			break;
		case 1:
			/* Disable iDVFS */
			if (err == 1)
				rc = BigiDVFSDisable_hp();
			break;
		case 2:
			/* Manual ctrl freq */
			/* echo 2 precent [max_precent] > dvt_test */
			if (err == 2)
				rc = BigIDVFSFreq(func_para[0]);
			break;
		case 3:
			/* Channel = 0(SW), 1(OCP), 2(OTP), EnDis = 0/1 */
			if (err == 3) {
				if ((cpu_online(8) == 1) || (cpu_online(9) == 1)) {
					rc = BigiDVFSChannel(func_para[0], func_para[1]);
					if (idvfs_init_opt.ocp_endis)
						Cluster2_OCP_OFF();
					if (idvfs_init_opt.otp_endis)
						BigOTPDisable();
				}
				spin_lock(&idvfs_struct_lock);
				if (func_para[0] == 1)
					idvfs_init_opt.ocp_endis = func_para[1];
				if (func_para[0] == 2)
					idvfs_init_opt.otp_endis = func_para[1];
				spin_unlock(&idvfs_struct_lock);
			}
			break;
		case 4:
			/* Length = 0~7, EnDis = 0/1 */
			if (err == 3)
				rc = BigiDVFSSWAvg(func_para[0], func_para[1]);
			break;
		case 5:
			/* switch debugout mode = 0 ~ 15 */
			if (err == 2)
				rc = SEC_BIGIDVFS_WRITE(0x10222470,
						((SEC_BIGIDVFS_READ(0x10222470) & 0xffff0fff) | (func_para[0] << 12)));
			break;
		case 6:
			/* Fmax, FMin, range 2054(20.54%) ~ 11600(116%) */
			if (err == 3) {
				rc = BigIDVFSFreqMaxMin(func_para[0], func_para[1]);
				if (func_para[0] == func_para[1])
					BigIDVFSFreq(func_para[0]);
			}
			break;
		case 7:
			/* Set L/LL Vproc, Vsarm mv_x100*/
			if (err == 3) {
				rc = da9214_vosel_buck_a(func_para[0]);
				/* set_cur_volt_sram_l(func_para[1]) */
			}
			break;
		case 8:
			/* Set Big Vproc, Vsram by mv_x100 */
			if (err == 3) {
				/*rc = da9214_vosel_buck_b(func_para[0]); */
				/* if(idvfs_init_opt.i2c_speed == 0)
					idvfs_init_opt.i2c_speed = iDVFSAPB_init();
				iDVFSAPB_DA9214_write(0xd9, DA9214_MV_TO_STEP(func_para[0] / 100)); */
				da9214_read_interface(0xd9, &ret_val, 0x7f, 0);
				ret_val = DA9214_STEP_TO_MV(ret_val & 0x7f);
				if (ret_val > ((BigiDVFSSRAMLDOGet() / 100) - 100)) {
					if ((cpu_online(8) == 1) || (cpu_online(9) == 1))
						BigiDVFSSRAMLDOSet(func_para[1]);
					da9214_config_interface(0xd9,
						((DA9214_MV_TO_STEP(func_para[0] / 100)) | 0x80), 0xff, 0);
				} else {
					da9214_config_interface(0xd9,
						((DA9214_MV_TO_STEP(func_para[0] / 100)) | 0x80), 0xff, 0);
					if ((cpu_online(8) == 1) || (cpu_online(9) == 1))
						BigiDVFSSRAMLDOSet(func_para[1]);
				}
			}
			rc = 0;
			break;
		case 9:
			/* reserve for ptp1, don't remove case 9, when ptp1 enable then unrun this command */
			/* if (err == 1) */
			/* eem_init_det_tmp(); */
			/* new for set idvfs_init_opt.idvfs_ctrl_reg */
			if (err == 2) {
				err = sscanf(buf, "%u %x ", &func_code, &func_para[0]);
				spin_lock(&idvfs_struct_lock);
				idvfs_init_opt.idvfs_ctrl_reg = func_para[0];
				spin_unlock(&idvfs_struct_lock);
			}
			rc = 0;
			break;
		case 10:
			/* case 10: org idvfsapb, chg to Freq setting */
			if (err == 2)
				rc = BigiDVFSPllSetFreq(func_para[0]);
			break;
		case 11:
			/* send i2c6 PMIC command */
			err = sscanf(buf, "%u %x %x %x %x", &func_code,
					&func_para[0], &func_para[1], &func_para[2], &func_para[3]);
			if (err == 5) {
				/* da9214_config_interface(reg, val, filter, shift) */
				da9214_config_interface(func_para[0], func_para[1], func_para[2], func_para[3]);
				idvfs_i2c_debug = (unsigned char)func_para[0];
			}
			break;
		case 12:
			/* case 12: bug on test */
			if (err == 2)
				if (func_para[0] == 11072)
					BUG_ON(1);
			break;
		case 13:
			if (err == 2)
				infoIdvfs = func_para[0];
			rc = 0;
			break;
		case 15:
			/* iDVFSAPB stress test command */
			break;
		case 16:
			/* DFD download, default 0x00011110 */
			if (err == 1)
				_idvfs_write(IDVFSPIN_DFD, 0x00033330);
			rc = 0;
			break;
		case 17:
			/* UDI to PMUX Mode 3(JTAG2), default 0x00011110 */
			if (err == 1) {
				_idvfs_write(IDVFSPIN_UDI_JTAG1, 0x00331111);
				_idvfs_write(IDVFSPIN_UDI_JTAG2, 0x01100333);
			}
			rc = 0;
			break;
		case 18:
			/* UDI to PMUX Mode 4(SDCARD), default 0x00011110 */
			if (err == 1)
				_idvfs_write(IDVFSPIN_UDI_SDCARD, 0x04414440);
			rc = 0;
			break;
		default: /* case 19: */
			/* AUXPIMX swithc */
			/* 0x10001000 0x2000, GPU, L-LL, BIG sarm ldo probe reg */
			if (err == 3) {
				if (func_para[0] == 0) {
					if (func_para[1] == 0) {
						/* disable pinmux */
						_idvfs_write_field(IDVFSPROB_BASE + 0x1510, 30:30, 0x1);
						/* L/LL CPU AGPIO prob out volt disable */
						_idvfs_write(IDVFSPROB_BASE + 0xfac, 0x000000ff);
					} else if (func_para[1] == 1) {
						/* enable pinmux */
						/* L/LL CPU AGPIO enable */
						_idvfs_write_field(IDVFSPROB_BASE + 0x1510, 30:30, 0x0);
						/* L/LL CPU AGPIO prob out volt enable */
						_idvfs_write(IDVFSPROB_BASE + 0xfac, 0x0000ff00);
					}
				} else if (func_para[0] == 1) {
					if (func_para[1] == 0) {
						/* disable pinmux */
						_idvfs_write_field(IDVFSPROB_BASE + 0x1890, 6:6, 0x1);
						/* Big CPU AGPIO prob out volt disable */
						SEC_BIGIDVFS_WRITE(0x102222b0,
							SEC_BIGIDVFS_READ(0x102222b0) & 0x7fffffff);
					} else if (func_para[1] == 1) {
						/* enable pinmux */
						_idvfs_write_field(IDVFSPROB_BASE + 0x1890, 6:6, 0x0);
						/* Big CPU AGPIO enable */
						SEC_BIGIDVFS_WRITE(0x102222b0,
							SEC_BIGIDVFS_READ(0x102222b0) | 0x80000000);
					}
				} else if (func_para[0] == 2) {
					if (func_para[1] == 0) {
						/* disable pinmux */
						_idvfs_write_field(IDVFSPROB_BASE + 0x1120, 9:9, 0x1);
						/* GPU AGPIO prob out volt disable */
						_idvfs_write(IDVFSPROB_BASE + 0xfd4, 0x000000ff);
					} else if (func_para[1] == 1) {
						/* enable pinmux */
						/* GPU AGPIO enable */
						_idvfs_write_field(IDVFSPROB_BASE + 0x1120, 9:9, 0x0);
						/* GPU AGPIO prob out volt enable */
						_idvfs_write(IDVFSPROB_BASE + 0xfd4, 0x0000ff00);
					}
				}
			}
			rc = 0;
			break;
		}

		if (rc == 0xff)
			idvfs_error("Error input parameter or function code.\n");
		else if	(rc	>= 0)
			idvfs_ver("Function code = %u, return success. return value = %u.\n", func_code, rc);
		else
			idvfs_ver("Function code = %u, fail and return value = %u.\n", func_code, rc);
	}

	free_page((unsigned	long)buf);
	return count;
}

#define PROC_FOPS_RW(name)							\
	static int name	## _proc_open(struct inode *inode, struct file *file)	\
	{									\
		return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
	}									\
	static const struct	file_operations	name ##	_proc_fops = {		\
		.owner			= THIS_MODULE,					\
		.open			= name ## _proc_open,				\
		.read			= seq_read,					\
		.llseek			= seq_lseek,					\
		.release		= single_release,				\
		.write			= name ## _proc_write,				\
	}

#define PROC_FOPS_RO(name)							\
	static int name	## _proc_open(struct inode *inode, struct file *file)	\
	{									\
		return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
	}									\
	static const struct	file_operations	name ##	_proc_fops = {		\
		.owner			= THIS_MODULE,					\
		.open			= name ## _proc_open,				\
		.read			= seq_read,					\
		.llseek			= seq_lseek,					\
		.release		= single_release,				\
	}

#define	PROC_ENTRY(name)	{__stringify(name),	&name ## _proc_fops}

PROC_FOPS_RW(idvfs_debug);
PROC_FOPS_RW(dvt_test);

static int _create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int	i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] =	{
		PROC_ENTRY(idvfs_debug),
		PROC_ENTRY(dvt_test),
	};

	dir = proc_mkdir("idvfs", NULL);

	if (!dir) {
		idvfs_error("fail to create /proc/idvfs @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			idvfs_error("%s(), create /proc/idvfs/%s failed\n", __func__, entries[i].name);
	}

	return 0;
}
#endif /* CONFIG_PROC_FS */

/*
 * Module driver
 */
static int __init idvfs_init(void)
{
	/* ver = mt_get_chip_sw_ver(); */
	/* ptp2_lo_enable = 1; */
	/* turn_on_LO();  */

	int err = 0;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,idvfs");
	if (!node) {
		idvfs_error("error: cannot find node IDVFS_NODE!\n");
		BUG();
	}

	/* Setup IO addresses and printf */
	idvfsapb_base = of_iomap(node, 0); /* 0x11017000 0x1000, i2c idvfsapb ctrl reg */
	idvfspin_base = of_iomap(node, 1); /* 0x10005000 0x1000, DFD,UDI pinmux reg */
	idvfsclk_base = of_iomap(node, 2); /* 0x10000000 0x1000, clock meter reg */
	idvfsprob_base = of_iomap(node, 3); /* 0x10001000 0x2000, GPU, L-LL, BIG sarm ldo probe reg */
	idvfsefus_base = of_iomap(node, 4); /* 0x10206000 0x1000, Big eFUSE register */

	idvfs_ver("idvfsapb_base = 0x%lx.\n", (unsigned long)idvfsapb_base);
	idvfs_ver("idvfspin_base = 0x%lx.\n", (unsigned long)idvfspin_base);
	idvfs_ver("idvfsclk_base = 0x%lx.\n", (unsigned long)idvfsclk_base);
	idvfs_ver("idvfsprob_base = 0x%lx.\n", (unsigned long)idvfsprob_base);
	idvfs_ver("idvfsefus_base = 0x%lx.\n", (unsigned long)idvfsefus_base);
	if (!idvfsapb_base || !idvfspin_base || !idvfsclk_base || !idvfsprob_base || !idvfsefus_base) {
		idvfs_error("idvfs get some base NULL.\n");
		BUG();
	}

	/*get idvfs irq num*/
	/* idvfs_irq_number = irq_of_parse_and_map(node, 0); */
	/* idvfs_ver("idvfs irq = %u.\n", idvfs_irq_number);*/

	/* register platform driver */
	err = platform_driver_register(&idvfs_pdrv);
	if (err) {
		idvfs_error("fail to register IDVFS driver @ %s()\n", __func__);
		goto out;
	}

#ifdef CONFIG_PROC_FS
	/* init proc */
	if (_create_procfs()) {
		err = -ENOMEM;
		goto out;
	}
#endif /* CONFIG_PROC_FS */

	/* enable iDVFSAPB function */
	/* iDVFSAPB_init(); */

out:
	return err;
}

static void __exit idvfs_exit(void)
{
	idvfs_ver("IDVFS de-initialization\n");
	platform_driver_unregister(&idvfs_pdrv);
}

module_init(idvfs_init);
module_exit(idvfs_exit);

MODULE_DESCRIPTION("MediaTek iDVFS Driver v0.1");
MODULE_LICENSE("GPL");
#endif /* __KERNEL__ */
#undef __MT_IDVFS_C__
