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

#ifndef __MTK_OCP_INTERNAL_H__
#define __MTK_OCP_INTERNAL_H__

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <mtk_ocp.h>


#define OCP_DVT		(0)
#define OCP_INTERRUPT_TEST	(0)
/* SRAM debugging */
#ifdef CONFIG_MTK_RAM_CONSOLE
#define CONFIG_OCP_AEE_RR_REC	1
#endif

#if defined(__MTK_OCP_C__) && OCP_FEATURE_ENABLED
#include <mt-plat/mtk_secure_api.h>
#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define mt_secure_call_ocp	mt_secure_call
#else
/* This is workaround for ocp use */
static noinline int mt_secure_call_ocp(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
	register u64 reg0 __asm__("x0") = function_id;
	register u64 reg1 __asm__("x1") = arg0;
	register u64 reg2 __asm__("x2") = arg1;
	register u64 reg3 __asm__("x3") = arg2;
	int ret = 0;

	asm volatile(
	"smc    #0\n"
		: "+r" (reg0)
		: "r" (reg1), "r" (reg2), "r" (reg3));

	ret = (int)reg0;
	return ret;
}
#endif
#endif

#if OCP_FEATURE_ENABLED
/*
 * BIT Operation
 */
#undef  BIT_OCP
#define BIT_OCP(_bit_)                    (unsigned)(1 << (_bit_))
#define BITS_OCP(_bits_, _val_)           ((((unsigned) -1 >> (31 - ((1) ? _bits_))) \
			& ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define BITMASK_OCP(_bits_)               (((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define GET_BITS_VAL_OCP(_bits_, _val_)   (((_val_) & (BITMASK_OCP(_bits_))) >> ((0) ? _bits_))

/**
 * Read/Write a field of a register.
 * @addr: Address of the register
 * @range: The field bit range in the form of MSB:LSB
 * @val:The value to be written to the field
 */
/* for ATF */
#define ocp_sec_read(addr)                       mt_secure_call_ocp(MTK_SIP_KERNEL_OCP_READ, addr, 0, 0)
#define ocp_sec_read_field(addr, range)          GET_BITS_VAL_OCP(range, ocp_sec_read(addr))
#define ocp_sec_write(addr, val)                 mt_secure_call_ocp(MTK_SIP_KERNEL_OCP_WRITE, addr, val, 0)
#define ocp_sec_write_field(addr, range, val)    ocp_sec_write(addr, \
			(ocp_sec_read(addr) & ~(BITMASK_OCP(range))) | BITS_OCP(range, val))
/* for IO MAP */
#define ocp_read(addr)                      __raw_readl(IOMEM(addr))
#define ocp_read_field(addr, range)         GET_BITS_VAL_OCP(range, ocp_read(addr))
#define ocp_write(addr, val)                 mt_reg_sync_writel((val), ((void *)addr))
#define ocp_write_field(addr, range, val)    ocp_write(addr, \
			(_ocp_read(addr) & ~(BITMASK_OCP(range))) | BITS_OCP(range, val))

/* log print */
#define TAG     "[mt_ocp]"
#define ocp_err(fmt, args...)	pr_debug(TAG fmt, ##args)
#define ocp_warn(fmt, args...)	pr_debug(TAG fmt, ##args)
#define ocp_info(fmt, args...)	pr_info(TAG fmt, ##args)
#define ocp_dbg(fmt, args...)				\
	do {						\
		if (ocp_info.debug)			\
			ocp_info(fmt, ##args);		\
	} while (0)

/* Lock */
#define ocp_lock(x)		spin_lock(&ocp_info.cl_setting[x].lock)
#define ocp_unlock(x)		spin_unlock(&ocp_info.cl_setting[x].lock)

/* for cl_setting check */
#define ocp_is_enable(x)	(ocp_info.cl_setting[x].is_enabled)
#define ocp_is_force_off(x)	(ocp_info.cl_setting[x].is_forced_off_by_user)
#define ocp_is_available(x)	(ocp_is_enable(x) && !ocp_is_force_off(x))
#define for_each_ocp_cluster(x)	for (x = 0; x < NR_OCP_CLUSTER; x++)
#define for_each_ocp_isr(x)	for (x = 0; x < NR_OCP_IRQ; x++)
#endif

/* enum */
/* Set OCPCFG bits {VoltBypassEn, LkgShare, LkgBypassEn, MHzBypassEn} = ConfigMode */
enum ocp_mode {
	NO_BYPASS = 0,
	MHZ_BYPASS,
	LKG_BYPASS,
	MHZ_LKG_BYPASS,
	LKG_SHARE = 4,
	SHARE_MHZ_BYPASS,
	SHARE_LKG_BYPASS,
	SHARE_MHZ_LKG_BYPASS,
	VOLT_BYPASS,
	VOLT_MHZ_BYPASS = 9,
	VOLT_LKG_BYPASS,
	VOLT_MHZ_LKG_BYPASS = 11,
	VOLT_BYPASS_LKG_SHARE,
	VOLT_MHZ_BYPASS_SHARE,
	VOLT_LKG_BYPASS_SHARE,
	VOLT_MHZ_LKG_BYPASS_SHARE = 15,

	NR_OCP_MODE,
};

enum ocp_int_select {
	IRQ_CLK_PCT_MIN = 0,
	IRQ_WA_MAX,
	IRQ_WA_MIN,

	NR_OCP_IRQ_TYPE,
};

enum ocp_value_select {
	CLK_AVG = 0,
	WA_AVG,
	TOP_RAW_LKG,
	CPU0_RAW_LKG,
	CPU1_RAW_LKG,
	CPU2_RAW_LKG,
	CPU3_RAW_LKG,
};


#define NR_OCP_IRQ		(3)
#define MAX_AUTO_CAPTURE_CNT	(60000)

#define OCP_ENABLE_DELAY_US	(1500)
#define OCP_CLK_PCT_MIN		(625)
#define OCP_CLK_PCT_MAX		(10000)
#define OCP_TARGET_MAX		(65535)
#define OCP_FREQ_MAX		(4095)
#define OCP_VOLTAGE_MAX		(1999)


/**
 * OCP registers
 */

/* Big */
#define BIG_OCP_BASE_ADDR	((unsigned int)(LITTLE_OCP_BASE_ADDR + BIG_OCP_OFFSET))
#define BIG_OCP_OFFSET		(0x2000)

#define MP2_OCPAPB00	(BIG_OCP_BASE_ADDR + 0x500)
#define MP2_OCPAPB01	(BIG_OCP_BASE_ADDR + 0x504)
#define MP2_OCPAPB02	(BIG_OCP_BASE_ADDR + 0x508)
#define MP2_OCPAPB03	(BIG_OCP_BASE_ADDR + 0x50C)
#define MP2_OCPAPB04	(BIG_OCP_BASE_ADDR + 0x510)
#define MP2_OCPAPB05	(BIG_OCP_BASE_ADDR + 0x514)
#define MP2_OCPAPB06	(BIG_OCP_BASE_ADDR + 0x518)
#define MP2_OCPAPB07	(BIG_OCP_BASE_ADDR + 0x51C)
#define MP2_OCPAPB08	(BIG_OCP_BASE_ADDR + 0x520)
#define MP2_OCPAPB09	(BIG_OCP_BASE_ADDR + 0x524)
#define MP2_OCPAPB0A	(BIG_OCP_BASE_ADDR + 0x528)
#define MP2_OCPAPB0B	(BIG_OCP_BASE_ADDR + 0x52C)
#define MP2_OCPAPB0C	(BIG_OCP_BASE_ADDR + 0x530)
#define MP2_OCPAPB0D	(BIG_OCP_BASE_ADDR + 0x534)
#define MP2_OCPAPB0E	(BIG_OCP_BASE_ADDR + 0x538)
#define MP2_OCPAPB0F	(BIG_OCP_BASE_ADDR + 0x53C)
#define MP2_OCPAPB10	(BIG_OCP_BASE_ADDR + 0x540)
#define MP2_OCPAPB11	(BIG_OCP_BASE_ADDR + 0x544)
#define MP2_OCPAPB12	(BIG_OCP_BASE_ADDR + 0x548)
#define MP2_OCPAPB13	(BIG_OCP_BASE_ADDR + 0x54C)
#define MP2_OCPAPB14	(BIG_OCP_BASE_ADDR + 0x550)
#define MP2_OCPAPB15	(BIG_OCP_BASE_ADDR + 0x554)
#define MP2_OCPAPB16	(BIG_OCP_BASE_ADDR + 0x558)

/* Little */
#define LITTLE_OCP_BASE_ADDR	((unsigned int)0x10200000)

#define MP0_OCPCPUPOST_CTRL0	(LITTLE_OCP_BASE_ADDR + 0x1030)
#define MP0_OCPCPUPOST_CTRL1	(LITTLE_OCP_BASE_ADDR + 0x1034)
#define MP0_OCPCPUPOST_CTRL2	(LITTLE_OCP_BASE_ADDR + 0x1038)
#define MP0_OCPCPUPOST_CTRL3	(LITTLE_OCP_BASE_ADDR + 0x103C)
#define MP0_OCPCPUPOST_CTRL4	(LITTLE_OCP_BASE_ADDR + 0x1040)
#define MP0_OCPCPUPOST_CTRL5	(LITTLE_OCP_BASE_ADDR + 0x1044)
#define MP0_OCPCPUPOST_CTRL6	(LITTLE_OCP_BASE_ADDR + 0x1048)
#define MP0_OCPCPUPOST_CTRL7	(LITTLE_OCP_BASE_ADDR + 0x104C)

#define MP0_OCPNCPUPOST_CTRL	(LITTLE_OCP_BASE_ADDR + 0x1070)

#define MP0_OCPAPB00	(LITTLE_OCP_BASE_ADDR + 0x1500)
#define MP0_OCPAPB01	(LITTLE_OCP_BASE_ADDR + 0x1504)
#define MP0_OCPAPB02	(LITTLE_OCP_BASE_ADDR + 0x1508)
#define MP0_OCPAPB03	(LITTLE_OCP_BASE_ADDR + 0x150C)
#define MP0_OCPAPB04	(LITTLE_OCP_BASE_ADDR + 0x1510)
#define MP0_OCPAPB05	(LITTLE_OCP_BASE_ADDR + 0x1514)
#define MP0_OCPAPB06	(LITTLE_OCP_BASE_ADDR + 0x1518)
#define MP0_OCPAPB07	(LITTLE_OCP_BASE_ADDR + 0x151C)
#define MP0_OCPAPB08	(LITTLE_OCP_BASE_ADDR + 0x1520)
#define MP0_OCPAPB09	(LITTLE_OCP_BASE_ADDR + 0x1524)
#define MP0_OCPAPB0A	(LITTLE_OCP_BASE_ADDR + 0x1528)
#define MP0_OCPAPB0B	(LITTLE_OCP_BASE_ADDR + 0x152C)
#define MP0_OCPAPB0C	(LITTLE_OCP_BASE_ADDR + 0x1530)
#define MP0_OCPAPB0D	(LITTLE_OCP_BASE_ADDR + 0x1534)
#define MP0_OCPAPB0E	(LITTLE_OCP_BASE_ADDR + 0x1538)
#define MP0_OCPAPB0F	(LITTLE_OCP_BASE_ADDR + 0x153C)
#define MP0_OCPAPB10	(LITTLE_OCP_BASE_ADDR + 0x1540)
#define MP0_OCPAPB11	(LITTLE_OCP_BASE_ADDR + 0x1544)
#define MP0_OCPAPB12	(LITTLE_OCP_BASE_ADDR + 0x1548)
#define MP0_OCPAPB13	(LITTLE_OCP_BASE_ADDR + 0x154C)
#define MP0_OCPAPB14	(LITTLE_OCP_BASE_ADDR + 0x1550)
#define MP0_OCPAPB15	(LITTLE_OCP_BASE_ADDR + 0x1554)
#define MP0_OCPAPB16	(LITTLE_OCP_BASE_ADDR + 0x1558)
#define MP0_OCPAPB17	(LITTLE_OCP_BASE_ADDR + 0x155C)
#define MP0_OCPAPB18	(LITTLE_OCP_BASE_ADDR + 0x1560)
#define MP0_OCPAPB19	(LITTLE_OCP_BASE_ADDR + 0x1564)
#define MP0_OCPAPB1A	(LITTLE_OCP_BASE_ADDR + 0x1568)
#define MP0_OCPAPB1B	(LITTLE_OCP_BASE_ADDR + 0x156C)
#define MP0_OCPAPB1C	(LITTLE_OCP_BASE_ADDR + 0x1570)
#define MP0_OCPAPB1D	(LITTLE_OCP_BASE_ADDR + 0x1574)
#define MP0_OCPAPB1E	(LITTLE_OCP_BASE_ADDR + 0x1578)
#define MP0_OCPAPB1F	(LITTLE_OCP_BASE_ADDR + 0x157C)
#define MP0_OCPAPB20	(LITTLE_OCP_BASE_ADDR + 0x1580)
#define MP0_OCPAPB21	(LITTLE_OCP_BASE_ADDR + 0x1584)
#define MP0_OCPAPB22	(LITTLE_OCP_BASE_ADDR + 0x1588)
#define MP0_OCPAPB23	(LITTLE_OCP_BASE_ADDR + 0x158C)
#define MP0_OCPAPB24	(LITTLE_OCP_BASE_ADDR + 0x1590)
#define MP0_OCPAPB25	(LITTLE_OCP_BASE_ADDR + 0x1594)

#define MP0_OCP_GENERAL_CTRL	(LITTLE_OCP_BASE_ADDR + 0x17FC)

#define MP1_OCPCPUPOST_CTRL0	(LITTLE_OCP_BASE_ADDR + 0x3030)
#define MP1_OCPCPUPOST_CTRL1	(LITTLE_OCP_BASE_ADDR + 0x3034)
#define MP1_OCPCPUPOST_CTRL2	(LITTLE_OCP_BASE_ADDR + 0x3038)
#define MP1_OCPCPUPOST_CTRL3	(LITTLE_OCP_BASE_ADDR + 0x303C)
#define MP1_OCPCPUPOST_CTRL4	(LITTLE_OCP_BASE_ADDR + 0x3040)
#define MP1_OCPCPUPOST_CTRL5	(LITTLE_OCP_BASE_ADDR + 0x3044)
#define MP1_OCPCPUPOST_CTRL6	(LITTLE_OCP_BASE_ADDR + 0x3048)
#define MP1_OCPCPUPOST_CTRL7	(LITTLE_OCP_BASE_ADDR + 0x304C)

#define MP1_OCPNCPUPOST_CTRL	(LITTLE_OCP_BASE_ADDR + 0x3070)

#define MP1_OCPAPB00	(LITTLE_OCP_BASE_ADDR + 0x3500)
#define MP1_OCPAPB01	(LITTLE_OCP_BASE_ADDR + 0x3504)
#define MP1_OCPAPB02	(LITTLE_OCP_BASE_ADDR + 0x3508)
#define MP1_OCPAPB03	(LITTLE_OCP_BASE_ADDR + 0x350C)
#define MP1_OCPAPB04	(LITTLE_OCP_BASE_ADDR + 0x3510)
#define MP1_OCPAPB05	(LITTLE_OCP_BASE_ADDR + 0x3514)
#define MP1_OCPAPB06	(LITTLE_OCP_BASE_ADDR + 0x3518)
#define MP1_OCPAPB07	(LITTLE_OCP_BASE_ADDR + 0x351C)
#define MP1_OCPAPB08	(LITTLE_OCP_BASE_ADDR + 0x3520)
#define MP1_OCPAPB09	(LITTLE_OCP_BASE_ADDR + 0x3524)
#define MP1_OCPAPB0A	(LITTLE_OCP_BASE_ADDR + 0x3528)
#define MP1_OCPAPB0B	(LITTLE_OCP_BASE_ADDR + 0x352C)
#define MP1_OCPAPB0C	(LITTLE_OCP_BASE_ADDR + 0x3530)
#define MP1_OCPAPB0D	(LITTLE_OCP_BASE_ADDR + 0x3534)
#define MP1_OCPAPB0E	(LITTLE_OCP_BASE_ADDR + 0x3538)
#define MP1_OCPAPB0F	(LITTLE_OCP_BASE_ADDR + 0x353C)
#define MP1_OCPAPB10	(LITTLE_OCP_BASE_ADDR + 0x3540)
#define MP1_OCPAPB11	(LITTLE_OCP_BASE_ADDR + 0x3544)
#define MP1_OCPAPB12	(LITTLE_OCP_BASE_ADDR + 0x3548)
#define MP1_OCPAPB13	(LITTLE_OCP_BASE_ADDR + 0x354C)
#define MP1_OCPAPB14	(LITTLE_OCP_BASE_ADDR + 0x3550)
#define MP1_OCPAPB15	(LITTLE_OCP_BASE_ADDR + 0x3554)
#define MP1_OCPAPB16	(LITTLE_OCP_BASE_ADDR + 0x3558)
#define MP1_OCPAPB17	(LITTLE_OCP_BASE_ADDR + 0x355C)
#define MP1_OCPAPB18	(LITTLE_OCP_BASE_ADDR + 0x3560)
#define MP1_OCPAPB19	(LITTLE_OCP_BASE_ADDR + 0x3564)
#define MP1_OCPAPB1A	(LITTLE_OCP_BASE_ADDR + 0x3568)
#define MP1_OCPAPB1B	(LITTLE_OCP_BASE_ADDR + 0x356C)
#define MP1_OCPAPB1C	(LITTLE_OCP_BASE_ADDR + 0x3570)
#define MP1_OCPAPB1D	(LITTLE_OCP_BASE_ADDR + 0x3574)
#define MP1_OCPAPB1E	(LITTLE_OCP_BASE_ADDR + 0x3578)
#define MP1_OCPAPB1F	(LITTLE_OCP_BASE_ADDR + 0x357C)
#define MP1_OCPAPB20	(LITTLE_OCP_BASE_ADDR + 0x3580)
#define MP1_OCPAPB21	(LITTLE_OCP_BASE_ADDR + 0x3584)
#define MP1_OCPAPB22	(LITTLE_OCP_BASE_ADDR + 0x3588)
#define MP1_OCPAPB23	(LITTLE_OCP_BASE_ADDR + 0x358C)
#define MP1_OCPAPB24	(LITTLE_OCP_BASE_ADDR + 0x3590)
#define MP1_OCPAPB25	(LITTLE_OCP_BASE_ADDR + 0x3594)

#define MP1_OCP_GENERAL_CTRL	(LITTLE_OCP_BASE_ADDR + 0x37FC)


/* OCP SMC */
#ifdef CONFIG_ARM64
#define MTK_SIP_KERNEL_OCP_WRITE	0xC20003D0
#define MTK_SIP_KERNEL_OCP_READ		0xC20003D1

#define MTK_SIP_KERNEL_OCPENDIS		0xC20003D2
#define MTK_SIP_KERNEL_OCPTARGET	0xC20003D3
#define MTK_SIP_KERNEL_OCPFREQPCT	0xC20003D4
#define MTK_SIP_KERNEL_OCPVOLTAGE	0xC20003D5
#define MTK_SIP_KERNEL_OCPBIGINTCLR	0xC20003D6
#define MTK_SIP_KERNEL_OCPMP0INTCLR	0xC20003D7
#define MTK_SIP_KERNEL_OCPMP1INTCLR	0xC20003D8
#define MTK_SIP_KERNEL_OCPBIGINTENDIS	0xC20003D9
#define MTK_SIP_KERNEL_OCPMP0INTENDIS	0xC20003DA
#define MTK_SIP_KERNEL_OCPMP1INTENDIS	0xC20003DB
#define MTK_SIP_KERNEL_OCPINTLIMIT	0xC20003DC
#define MTK_SIP_KERNEL_OCPLKGMONENDIS	0xC20003DD
#define MTK_SIP_KERNEL_OCPVALUESTATUS	0xC20003DE
#define MTK_SIP_KERNEL_OCPIRQHOLDOFF	0xC20003DF
#define MTK_SIP_KERNEL_OCPCONFIGMODE	0xC20003E0
#define MTK_SIP_KERNEL_OCPLEAKAGE	0xC20003E1
#else
#define MTK_SIP_KERNEL_OCP_WRITE	0x820003D0
#define MTK_SIP_KERNEL_OCP_READ		0x820003D1

#define MTK_SIP_KERNEL_OCPENDIS		0x820003D2
#define MTK_SIP_KERNEL_OCPTARGET	0x820003D3
#define MTK_SIP_KERNEL_OCPFREQPCT	0x820003D4
#define MTK_SIP_KERNEL_OCPVOLTAGE	0x820003D5
#define MTK_SIP_KERNEL_OCPBIGINTCLR	0x820003D6
#define MTK_SIP_KERNEL_OCPMP0INTCLR	0x820003D7
#define MTK_SIP_KERNEL_OCPMP1INTCLR	0x820003D8
#define MTK_SIP_KERNEL_OCPBIGINTENDIS	0x820003D9
#define MTK_SIP_KERNEL_OCPMP0INTENDIS	0x820003DA
#define MTK_SIP_KERNEL_OCPMP1INTENDIS	0x820003DB
#define MTK_SIP_KERNEL_OCPINTLIMIT	0x820003DC
#define MTK_SIP_KERNEL_OCPLKGMONENDIS	0x820003DD
#define MTK_SIP_KERNEL_OCPVALUESTATUS	0x820003DE
#define MTK_SIP_KERNEL_OCPIRQHOLDOFF	0x820003DF
#define MTK_SIP_KERNEL_OCPCONFIGMODE	0x820003E0
#define MTK_SIP_KERNEL_OCPLEAKAGE	0x820003E1
#endif


struct ocp_status {
	unsigned int temp;

	unsigned int clk_avg;
	unsigned int wa_avg;
	unsigned int top_raw_lkg;
	unsigned int cpu0_raw_lkg;
	unsigned int cpu1_raw_lkg;
	unsigned int cpu2_raw_lkg;
	unsigned int cpu3_raw_lkg;
};

struct ocp_cluster_setting {
	bool is_enabled;
	bool is_forced_off_by_user;
	enum ocp_mode mode;
	unsigned int target;
	spinlock_t lock;
	unsigned int irq_num[NR_OCP_IRQ_TYPE];
#if OCP_INTERRUPT_TEST
	struct work_struct work;
#endif
	struct ocp_status *status;
};

struct ocp_data {
	unsigned int debug;
	void __iomem  *ocp_base;
	struct ocp_cluster_setting cl_setting[NR_OCP_CLUSTER];
	const struct dev_pm_ops pm_ops;
	struct platform_device pdev;
	struct platform_driver pdrv;
	/* for HQA */
	struct hrtimer auto_capture_timer;
	enum ocp_cluster auto_capture_cluster;
	unsigned int auto_capture_total_cnt;
	unsigned int auto_capture_cur_cnt;
};

/* SRAM debugging */
extern void aee_rr_rec_ocp_target_limit(int id, u32 val);
extern u32 aee_rr_curr_ocp_target_limit(int id);
extern void aee_rr_rec_ocp_enable(u8 val);
extern u8 aee_rr_curr_ocp_enable(void);


#endif /* __MTK_OCP_INTERNAL_H__ */

