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

#define __MT_EEM2_C__



#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
/* #include <mach/mt_boot.h> */
#include "mt_eem2.h"

#ifdef USING_XLOG
#include <linux/xlog.h>

#define TAG     "xxx2"

#define eem2_err(fmt, args...)       \
	pr_err(ANDROID_LOG_ERROR, TAG, fmt, ##args)
#define eem2_warn(fmt, args...)      \
	pr_warn(ANDROID_LOG_WARN, TAG, fmt, ##args)
#define eem2_info(fmt, args...)      \
	pr_info(ANDROID_LOG_INFO, TAG, fmt, ##args)
#define eem2_dbg(fmt, args...)       \
	pr_debug(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define eem2_ver(fmt, args...)       \
	pr_notice(ANDROID_LOG_VERBOSE, TAG, fmt, ##args)

#else   /* USING_XLOG */

#define TAG     "[xxx2] "

#define eem2_err(fmt, args...)       \
	pr_err(TAG fmt, ##args)
#define eem2_warn(fmt, args...)      \
	pr_warn(TAG fmt, ##args)
#define eem2_info(fmt, args...)      \
	pr_info(TAG fmt, ##args)
#define eem2_dbg(fmt, args...)       \
	pr_debug(TAG fmt, ##args)
#define eem2_ver(fmt, args...)       \
	pr_notice(TAG fmt, ##args)

#endif  /* USING_XLOG */


#ifdef CONFIG_OF
void __iomem *eem2_base; /* 0x10220000 */
static unsigned long eem2_phy_base;
static struct device_node *node;
#endif

#define EEM2_BASEADDR			((eem2_base) + 0x600)
#define EEM2_CTRL_REG_0			(EEM2_BASEADDR + 0x78)
#define EEM2_CTRL_REG_1			(EEM2_BASEADDR + 0x7c)
#define EEM2_CTRL_REG_2			(EEM2_BASEADDR + 0x138)
#define EEM2_CTRL_REG_3			(EEM2_BASEADDR + 0x13c)

#define EEM2_BASEADDR_PHYS		((eem2_phy_base) + 0x600)
#define EEM2_CTRL_REG_0_PHYS		(EEM2_BASEADDR_PHYS + 0x78)
#define EEM2_CTRL_REG_1_PHYS		(EEM2_BASEADDR_PHYS + 0x7c)
#define EEM2_CTRL_REG_2_PHYS		(EEM2_BASEADDR_PHYS + 0x138)
#define EEM2_CTRL_REG_3_PHYS		(EEM2_BASEADDR_PHYS + 0x13c)

/* EEM2_BIG_BASEADDR           (0x10220000) */
#define EEM2_BIG_BASEADDR		(eem2_base + 0x2400)
#define EEM2_BIG_CTRL_REG_0		(EEM2_BIG_BASEADDR + 0x10) /* 0x10 */
#define EEM2_BIG_CTRL_REG_1		(EEM2_BIG_BASEADDR + 0x14) /* 0x14 */
#define EEM2_BIG_CTRL_REG_2		(EEM2_BIG_BASEADDR + 0x18) /* 0x18 */
#define EEM2_BIG_CTRL_REG_3		(EEM2_BIG_BASEADDR + 0x1C) /* 0x1C */
#define EEM2_BIG_CTRL_REG_4		(EEM2_BIG_BASEADDR + 0x20) /* 0x20 */
#define EEM2_DET_CPU_ENABLE_ADDR	(EEM2_BIG_BASEADDR + 0x24) /* 0x24 */

#define EEM2_BIG_BASEADDR_PHYS		((eem2_phy_base) + 0x2400)
#define EEM2_BIG_CTRL_REG_0_PHYS	(EEM2_BIG_BASEADDR_PHYS + 0x10) /* 0x10 */
#define EEM2_BIG_CTRL_REG_1_PHYS	(EEM2_BIG_BASEADDR_PHYS + 0x14) /* 0x14 */
#define EEM2_BIG_CTRL_REG_2_PHYS	(EEM2_BIG_BASEADDR_PHYS + 0x18) /* 0x18 */
#define EEM2_BIG_CTRL_REG_3_PHYS	(EEM2_BIG_BASEADDR_PHYS + 0x1C) /* 0x1C */
#define EEM2_BIG_CTRL_REG_4_PHYS	(EEM2_BIG_BASEADDR_PHYS + 0x20) /* 0x20 */
#define EEM2_DET_CPU_ENABLE_ADDR_PHYS	(EEM2_BIG_BASEADDR_PHYS + 0x24) /* 0x24 */


/*
 * BIT Operation
 */
#undef  BIT
#define BIT(_bit_)                    (unsigned)(1 << (_bit_))
#define BITS(_bits_, _val_)\
	((((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))

#define BITMASK(_bits_)\
	(((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))

#define GET_BITS_VAL(_bits_, _val_)   (((_val_) & (_BITMASK_(_bits_))) >> ((0) ? _bits_))



/* #define eem2_read(addr)          (*(volatile unsigned int *)(addr)) */
/* #define eem2_write(addr, val)    (*(volatile unsigned int *)(addr) = (unsigned int)(val)) */
/**
 * Read/Write a field of a register.
 * @addr:       Address of the register
 * @range:      The field bit range in the form of MSB:LSB
 * @val:        The value to be written to the field
 */
#define READ_REGISTER_UINT32(reg)	(*(volatile unsigned int * const)(reg))
#define INREG32(x)	READ_REGISTER_UINT32((unsigned int *)((void *)(x)))
#define DRV_Reg32(addr)	INREG32(addr)
#define eem2_read(addr)	DRV_Reg32(addr)

#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
	#define eem2_write(addr, val)    mcusys_smc_write_phy(addr##_PHYS, val)
	#define eem2_write_field(addr, range, val)\
		mcusys_smc_write_phy(addr##_PHYS, ((eem2_read(addr) & ~(BITMASK(range))) | BITS(range, val)))
#else
	#define eem2_write(addr, val)    mt_reg_sync_writel(val, addr)
	#define eem2_write_field(addr, range, val)\
		eem2_write(addr, ((eem2_read(addr) & ~(BITMASK(range))) | BITS(range, val)))
#endif


/* For L LL */
static struct EEM2_data eem2_data;
static struct EEM2_trig eem2_trig;
static int eem2_lo_enable;
static unsigned int eem2_ctrl_lo[2];

/* For big */
static struct EEM2_big_data eem2_big_data;
static struct EEM2_big_trig eem2_big_trig;
static unsigned int eem2_big_regs[4];
static unsigned int eem2_big_lo_enable;
static unsigned int eem2_big_initialized;

/* static CHIP_SW_VER ver = CHIP_SW_VER_01; */

static void eem2_reset_data(struct EEM2_data *data)
{
	memset_io((void *)data, 0, sizeof(struct EEM2_data));
}



static void eem2_big_reset_data(struct EEM2_big_data *data)
{
	memset_io((void *)data, 0, sizeof(struct EEM2_big_data));
}



static void eem2_reset_trig(struct EEM2_trig *trig)
{
	memset_io((void *)trig, 0, sizeof(struct EEM2_trig));
}



static void eem2_big_reset_trig(struct EEM2_big_trig *trig)
{
	memset_io((void *)trig, 0, sizeof(struct EEM2_big_trig));
}



static int eem2_set_rampstart(struct EEM2_data *data, unsigned int rampstart)
{
	if (rampstart & ~(0x3)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"3\"\n");
		return -1;
	}
	data->RAMPSTART = rampstart;
	return 0;
}



static int eem2_big_set_rampstart(struct EEM2_big_data *data, unsigned int rampstart)
{
	if (rampstart & ~(0x3)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"3\"\n",
			__func__,
			rampstart);
		return -1;
	}
	data->BIG_RAMPSTART = rampstart;
	return 0;
}



static int eem2_set_rampstep(struct EEM2_data *data, unsigned int rampstep)
{
	if (rampstep & ~(0xF)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}
	data->RAMPSTEP = rampstep;
	return 0;
}



static int eem2_big_set_rampstep(struct EEM2_big_data *data, unsigned int rampstep)
{
	if (rampstep & ~(0xF)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"15\"\n",
			__func__,
			rampstep);
		return -1;
	}
	data->BIG_RAMPSTEP = rampstep;
	return 0;
}



static int eem2_set_delay(struct EEM2_data *data, unsigned int delay)
{
	if (delay & ~(0xF)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}
	data->DELAY = delay;
	return 0;
}



static int eem2_big_set_delay(struct EEM2_big_data *data, unsigned int delay)
{
	if (delay & ~(0xF)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"15\"\n",
			__func__,
			delay);
		return -1;
	}
	data->BIG_DELAY = delay;
	return 0;
}



static int eem2_set_autoStopBypass_enable(struct EEM2_data *data, unsigned int autostop_enable)
{
	if (autostop_enable & ~(0x1)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"1\"\n");
		return -1;
	}
	data->AUTO_STOP_BYPASS_ENABLE = autostop_enable;
	return 0;
}



static int eem2_big_set_autoStopBypass_enable(
	struct EEM2_big_data *data, unsigned int autostop_enable)
{
	if (autostop_enable & ~(0x1)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"1\"\n",
			__func__,
			autostop_enable);
		return -1;
	}
	data->BIG_AUTOSTOPENABLE = autostop_enable;
	return 0;
}



static int eem2_set_triggerPulDelay(struct EEM2_data *data, unsigned int triggerPulDelay)
{
	if (triggerPulDelay & ~(0x1)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"1\"\n");
		return -1;
	}
	data->TRIGGER_PUL_DELAY = triggerPulDelay;
	return 0;
}



static int eem2_big_set_triggerPulDelay(struct EEM2_big_data *data, unsigned int triggerPulDelay)
{
	if (triggerPulDelay & ~(0x1)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"1\"\n",
			__func__,
			triggerPulDelay);
		return -1;
	}
	data->BIG_TRIGGER_PUL_DELAY = triggerPulDelay;
	return 0;
}



static int eem2_set_ctrl_enable(struct EEM2_data *data, unsigned int ctrlEnable)
{
	if (ctrlEnable & ~(0x1)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"1\"\n");
		return -1;
	}
	data->CTRL_ENABLE = ctrlEnable;
	return 0;
}



static int eem2_set_det_enable(struct EEM2_data *data, unsigned int detEnable)
{
	if (detEnable & ~(0x1)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"1\"\n");
		return -1;
	}
	data->DET_ENABLE = detEnable;
	return 0;
}



static int eem2_big_set_nocpuenable(struct EEM2_big_data *data, unsigned int noCpuEnable)
{
	if (noCpuEnable & ~(0x1)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"1\"\n",
			__func__,
			noCpuEnable);
		return -1;
	}
	data->BIG_NOCPUENABLE = noCpuEnable;
	return 0;
}



static int eem2_big_set_cpuenable(struct EEM2_big_data *data, unsigned int cpuEnable)
{
	if (cpuEnable & ~(0x3)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"3\"\n",
			__func__,
			cpuEnable);
		return -1;
	}
	data->BIG_CPUENABLE = cpuEnable;
	return 0;
}



static int eem2_set_mp0_nCORERESET(struct EEM2_trig *trig, unsigned int mp0_nCoreReset)
{
	if (mp0_nCoreReset & ~(0xF)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}
	trig->mp0_nCORE_RESET = mp0_nCoreReset;
	return 0;
}



static int eem2_set_mp0_STANDBYWFE(struct EEM2_trig *trig, unsigned int mp0_StandbyWFE)
{
	if (mp0_StandbyWFE & ~(0xF)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"15\"\n",
			__func__,
			mp0_StandbyWFE);
		 return -1;
	}
	trig->mp0_STANDBY_WFE = mp0_StandbyWFE;
	return 0;
}



int eem2_set_mp0_STANDBYWFI(struct EEM2_trig *trig, unsigned int mp0_StandbyWFI)
{
	if (mp0_StandbyWFI & ~(0xF)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"15\"\n",
			__func__,
			mp0_StandbyWFI);
		return -1;
	}
	trig->mp0_STANDBY_WFI = mp0_StandbyWFI;
	return 0;
}



int eem2_set_mp0_STANDBYWFIL2(struct EEM2_trig *trig, unsigned int mp0_StandbyWFIL2)
{
	if (mp0_StandbyWFIL2 & ~(0x1)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"1\"\n",
			__func__,
			mp0_StandbyWFIL2);
		return -1;
	}
	trig->mp0_STANDBY_WFIL2 = mp0_StandbyWFIL2;
	return 0;
}



int eem2_set_mp1_nCORERESET(struct EEM2_trig *trig, unsigned int mp1_nCoreReset)
{
	if (mp1_nCoreReset & ~(0xF)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}
	trig->mp1_nCORE_RESET = mp1_nCoreReset;
	return 0;
}



int eem2_set_mp1_STANDBYWFE(struct EEM2_trig *trig, unsigned int mp1_StandbyWFE)
{
	if (mp1_StandbyWFE & ~(0xF)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"15\"\n");
	return -1;
	}
	trig->mp1_STANDBY_WFE = mp1_StandbyWFE;
	return 0;
}



int eem2_set_mp1_STANDBYWFI(struct EEM2_trig *trig, unsigned int mp1_StandbyWFI)
{
	if (mp1_StandbyWFI & ~(0xF)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}
	trig->mp1_STANDBY_WFI = mp1_StandbyWFI;
	return 0;
}



int eem2_set_mp1_STANDBYWFIL2(struct EEM2_trig *trig, unsigned int mp1_StandbyWFIL2)
{
	if (mp1_StandbyWFIL2 & ~(0x1)) {
		eem2_err("bad argument!! argument should be \"0\" ~ \"1\"\n");
		return -1;
	}
	trig->mp1_STANDBY_WFIL2 = mp1_StandbyWFIL2;
	return 0;
}



static int eem2_big_set_NoCPU(struct EEM2_big_trig *trig, unsigned int noCpu_lo_setting)
{
	if (noCpu_lo_setting & ~(0x33338)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"0x33338\"\n",
			__func__,
			noCpu_lo_setting);
		return -1;
	}
	trig->ctrl_regs[0] = (noCpu_lo_setting << 12);
	return 0;
}



static int eem2_big_set_CPU0(struct EEM2_big_trig *trig, unsigned int cpu0_lo_setting)
{
	if (cpu0_lo_setting & ~(0xF)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"F\"\n",
			__func__,
			cpu0_lo_setting);
		return -1;
	}
	trig->ctrl_regs[1] = (cpu0_lo_setting << 28);
	return 0;
}



static int eem2_big_set_CPU1(struct EEM2_big_trig *trig, unsigned int cpu1_lo_setting)
{
	if (cpu1_lo_setting & ~(0xF)) {
		eem2_err("[%s] bad argument!! (%d), argument should be \"0\" ~ \"F\"\n",
			__func__,
			cpu1_lo_setting);
		return -1;
	}
	trig->ctrl_regs[2] = (cpu1_lo_setting << 28);
	return 0;
}



static void eem2_apply(struct EEM2_data *data, struct EEM2_trig *trig)
{
	volatile unsigned int val_0 =
		BITS(13:12, data->RAMPSTART) |
		BITS(11:8, data->RAMPSTEP) |
		BITS(7:4, data->DELAY) |
		BITS(3:3, data->AUTO_STOP_BYPASS_ENABLE) |
		BITS(2:2, data->TRIGGER_PUL_DELAY) |
		BITS(1:1, data->CTRL_ENABLE) |
		BITS(0:0, data->DET_ENABLE);

	volatile unsigned int val_1 =
		BITS(31:28,   trig->mp0_nCORE_RESET) |
		BITS(27:24,   trig->mp0_STANDBY_WFE) |
		BITS(23:20,   trig->mp0_STANDBY_WFI) |
		BITS(19:19, trig->mp0_STANDBY_WFIL2) |
		BITS(18:15,   trig->mp1_nCORE_RESET) |
		BITS(14:11,   trig->mp1_STANDBY_WFE) |
		BITS(10:7,   trig->mp1_STANDBY_WFI) |
		BITS(6:6, trig->mp1_STANDBY_WFIL2);

	eem2_ctrl_lo[0] = val_0;
	eem2_ctrl_lo[1] = val_1;
	eem2_err("%s() set value[0]=0x%x, value[1]=0x%x\n", __func__, eem2_ctrl_lo[0], eem2_ctrl_lo[1]);
	/* for Everest cluster 0 */
	eem2_write(EEM2_CTRL_REG_0, val_0);
	eem2_write(EEM2_CTRL_REG_1, val_1 & 0xfff80000);

	/* for Everest cluster 1 */
	eem2_write(EEM2_CTRL_REG_2, val_0);
	eem2_write(EEM2_CTRL_REG_3, (val_1 << 13));

	/* Apply software reset that apply the EEM2 LO value to system */
	eem2_write_field(EEM2_CTRL_REG_0, 31:31, 1);
	udelay(1000);
	eem2_write_field(EEM2_CTRL_REG_0, 31:31, 0);

	eem2_write_field(EEM2_CTRL_REG_2, 31:31, 1);
	udelay(1000);
	eem2_write_field(EEM2_CTRL_REG_2, 31:31, 0);
	eem2_err("%s() read value[0]=0x%x, value[1]=0x%x\n", __func__, eem2_ctrl_lo[0], eem2_ctrl_lo[1]);
}



static void eem2_big_apply(struct EEM2_big_data *data, struct EEM2_big_trig *trig)
{
	volatile unsigned int val_2 =
		BITS(16:16, data->BIG_TRIGGER_PUL_DELAY) |
		BITS(15:14, data->BIG_RAMPSTART) |
		BITS(13:10, data->BIG_DELAY) |
		BITS(9:6, data->BIG_RAMPSTEP) |
		BITS(5:5, data->BIG_AUTOSTOPENABLE) |
		BITS(4:4, data->BIG_NOCPUENABLE) |
		BITS(3:0, data->BIG_CPUENABLE);

	volatile unsigned int nocpu = trig->ctrl_regs[0];
	volatile unsigned int cpu0 = trig->ctrl_regs[1];
	volatile unsigned int cpu1 = trig->ctrl_regs[2];

	eem2_big_regs[0] = nocpu;
	eem2_big_regs[1] = cpu0;
	eem2_big_regs[2] = cpu1;
	eem2_big_regs[3] = val_2;

	eem2_write(EEM2_DET_CPU_ENABLE_ADDR, val_2);
	eem2_write(EEM2_BIG_CTRL_REG_0, nocpu);
	eem2_write(EEM2_BIG_CTRL_REG_1, cpu0);
	eem2_write(EEM2_BIG_CTRL_REG_2, cpu1);
}



/* config_LO_CTRL(EEM2_RAMPSTART_3, 9, 13, 0, 0, 1, 1, 0xF, 0xF, 0xF, 1, 0xF, 0xF, 0xF, 1) All on and worst case */
/* config_LO_CTRL(EEM2_RAMPSTART_3, 1, 1, 0, 0, 1, 1, 0xF, 0xF, 0xF, 1, 0xF, 0xF, 0xF, 1) All on and best case */
/* config_LO_CTRL(EEM2_RAMPSTART_3, 1, 1, 0, 0, 1, 1, 0x8, 0x8, 0x8, 1, 0xF, 0xF, 0xF, 1) 4 core on and best case */
/* config_LO_CTRL(EEM2_RAMPSTART_3, 0, 0, 0, 0, 0, 0, 0x0, 0x0, 0x0, 0, 0x0, 0x0, 0x0, 0) For all off */
static void config_LO_CTRL(
	unsigned int rampStart,
	unsigned int rampStep,
	unsigned int delay,
	unsigned int autoStopEnable,
	unsigned int triggerPulDelay,
	unsigned int ctrlEnable,
	unsigned int detEnable,
	unsigned int mp0_nCoreReset,
	unsigned int mp0_StandbyWFE,
	unsigned int mp0_StandbyWFI,
	unsigned int mp0_StandbyWFIL2,
	unsigned int mp1_nCoreReset,
	unsigned int mp1_StandbyWFE,
	unsigned int mp1_StandbyWFI,
	unsigned int mp1_StandbyWFIL2
	)
{
	eem2_reset_data(&eem2_data);
	smp_mb();
	eem2_set_rampstart(&eem2_data, rampStart);
	eem2_set_rampstep(&eem2_data, rampStep);
	eem2_set_delay(&eem2_data, delay);
	eem2_set_autoStopBypass_enable(&eem2_data, autoStopEnable);
	eem2_set_triggerPulDelay(&eem2_data, triggerPulDelay);
	eem2_set_ctrl_enable(&eem2_data, ctrlEnable);
	eem2_set_det_enable(&eem2_data, detEnable);

	eem2_reset_trig(&eem2_trig);
	smp_mb();
	eem2_set_mp0_nCORERESET(&eem2_trig, mp0_nCoreReset);
	eem2_set_mp0_STANDBYWFE(&eem2_trig, mp0_StandbyWFE);
	eem2_set_mp0_STANDBYWFI(&eem2_trig, mp0_StandbyWFI);
	eem2_set_mp0_STANDBYWFIL2(&eem2_trig, mp0_StandbyWFIL2);
	eem2_set_mp1_nCORERESET(&eem2_trig, mp1_nCoreReset);
	eem2_set_mp1_STANDBYWFE(&eem2_trig, mp1_StandbyWFE);
	eem2_set_mp1_STANDBYWFI(&eem2_trig, mp1_StandbyWFI);
	eem2_set_mp1_STANDBYWFIL2(&eem2_trig, mp1_StandbyWFIL2);
	smp_mb();
	eem2_apply(&eem2_data, &eem2_trig);
}



static void config_big_LO_CTRL(unsigned int triggerPulDelay,
	unsigned int rampStart,
	unsigned int delay,
	unsigned int rampStep,
	unsigned int autoStopEnable,
	unsigned int noCpuEnable,
	unsigned int cpuEnable,
	unsigned int noCpu_lo_setting,
	unsigned int cpu0_lo_setting, unsigned int cpu1_lo_setting)
{
	eem2_big_reset_data(&eem2_big_data);
	smp_mb();
	eem2_big_set_triggerPulDelay(&eem2_big_data, triggerPulDelay);
	eem2_big_set_rampstart(&eem2_big_data, rampStart);
	eem2_big_set_delay(&eem2_big_data, delay);
	eem2_big_set_rampstep(&eem2_big_data, rampStep);
	eem2_big_set_autoStopBypass_enable(&eem2_big_data, autoStopEnable);
	eem2_big_set_nocpuenable(&eem2_big_data, noCpuEnable);
	eem2_big_set_cpuenable(&eem2_big_data, cpuEnable);

	eem2_big_reset_trig(&eem2_big_trig);
	smp_mb();
	eem2_big_set_NoCPU(&eem2_big_trig, noCpu_lo_setting);
	eem2_big_set_CPU0(&eem2_big_trig, cpu0_lo_setting);
	eem2_big_set_CPU1(&eem2_big_trig, cpu1_lo_setting);
	smp_mb();
	eem2_big_apply(&eem2_big_data, &eem2_big_trig);
}



static void enable_LO(void)
{
	int i;

	if ((eem2_ctrl_lo[0] & 0x03) == 0) {
		/* For >= 4 core on and best case; */
		config_LO_CTRL(EEM2_RAMPSTART_3, 1, 1, 0, 0, 1, 1, 0x8, 0x8, 0x8, 0, 0xF, 0xF, 0xF, 0);
	} else {
		config_LO_CTRL(
			(eem2_ctrl_lo[0]>>12) & 0x03,
			(eem2_ctrl_lo[0]>>8) & 0x0F,
			(eem2_ctrl_lo[0]>>4) & 0x0F,
			(eem2_ctrl_lo[0]>>3) & 0x01,
			(eem2_ctrl_lo[0]>>2) & 0x01,
			(eem2_ctrl_lo[0]>>1) & 0x01,
			eem2_ctrl_lo[0] & 0x01,
			(eem2_ctrl_lo[1]>>28) & 0x0f,
			(eem2_ctrl_lo[1]>>24) & 0x0f,
			(eem2_ctrl_lo[1]>>20) & 0x0f,
			(eem2_ctrl_lo[1]>>19) & 0x01,
			(eem2_ctrl_lo[1]>>15) & 0x0f,
			(eem2_ctrl_lo[1]>>11) & 0x0f,
			(eem2_ctrl_lo[1]>>7) & 0x0f,
			(eem2_ctrl_lo[1]>>6) & 0x01
		);
	}

	for (i = 0; i < EEM2_REG_NUM; i++) {
		eem2_dbg("[%d]=[%x]\n", i, eem2_read(EEM2_CTRL_REG_0 + (i << 2)));
		eem2_dbg("[%d]=[%x]\n", i + 2, eem2_read(EEM2_CTRL_REG_2 + (i << 2)));
	}
}



static void enable_big_LO(void)
{
	if ((eem2_big_regs[3] & 0x03) == 0)
		config_big_LO_CTRL(0, EEM2_RAMPSTART_3, 1, 1, 0, 1, 3, 0x33338, 0xF, 0xF);
	else {
		config_big_LO_CTRL((eem2_big_regs[3] >> 16) & 0x01,
			(eem2_big_regs[3] >> 14) & 0x03,
			(eem2_big_regs[3] >> 10) & 0x0F,
			(eem2_big_regs[3] >> 6) & 0x0F,
			(eem2_big_regs[3] >> 5) & 0x01,
			(eem2_big_regs[3] >> 4) & 0x01,
			eem2_big_regs[3] & 0x0F,
			eem2_big_regs[0] >> 12,
			eem2_big_regs[1] >> 28, eem2_big_regs[2] >> 28);
	}
}



static void disable_LO(void)
{
	config_LO_CTRL(0, 0, 0, 0, 0, 0, 0, 0x0, 0x0, 0x0, 0, 0x0, 0x0, 0x0, 0);/* => For all off */
}



static void disable_big_LO(void)
{
	config_big_LO_CTRL(0, 0, 0, 0, 0, 0, 0, 0x0, 0x0, 0x0); /* => For all big off */
}



void turn_on_LO(void)
{
	if (0 == eem2_lo_enable)
		return;

	enable_LO();
}



void turn_on_big_LO(void)
{
	enable_big_LO();
}



void turn_off_LO(void)
{
	if (0 == eem2_lo_enable)
		return;

	disable_LO();
}



void turn_off_big_LO(void)
{
	disable_big_LO();
}



/* Device infrastructure */
static int eem2_remove(struct platform_device *pdev)
{
	return 0;
}



static int eem2_probe(struct platform_device *pdev)
{
	return 0;
}



static int eem2_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	kthread_stop(eem2_thread);
	*/

	return 0;
}



static int eem2_resume(struct platform_device *pdev)
{
	/*
	eem2_thread = kthread_run(eem2_thread_handler, 0, "eem2 xxx");
	if (IS_ERR(eem2_thread))
	{
		pr_debug("[%s]: failed to create eem2 xxx thread\n", __func__);
	}
	*/

	return 0;
}



#ifdef CONFIG_OF
static const struct of_device_id mt_eem2_of_match[] = {
	{ .compatible = "mediatek,mcucfg", },
	{},
};
#endif
static struct platform_driver eem2_driver = {
	.remove     = eem2_remove,
	.shutdown   = NULL,
	.probe      = eem2_probe,
	.suspend    = eem2_suspend,
	.resume     = eem2_resume,
	.driver     = {
		.name   = "mt-eem2",
		#ifdef CONFIG_OF
		.of_match_table = mt_eem2_of_match,
		#endif
	},
};



#ifdef CONFIG_PROC_FS
static char *_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	return buf;

out:
	free_page((unsigned long)buf);

	return NULL;
}



/* eem2_lo_enable */
static int eem2_lo_enable_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "eem2_lo_enable = %d\n", eem2_lo_enable);

	return 0;
}



static ssize_t eem2_lo_enable_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	int val = 0;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &val)) {
		if (val == 1) {
			enable_LO();
			eem2_lo_enable = 1;
		} else {
			eem2_lo_enable = 0;
			disable_LO();
		}
	}

	free_page((unsigned long)buf);

	return count;
}



/* eem2_big_lo_enable */
static int eem2_big_lo_enable_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "eem2_big_lo_enable = %d\n", eem2_big_lo_enable);

	return 0;
}



static ssize_t eem2_big_lo_enable_proc_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *pos)
{
	int val = 0;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &val)) {
		eem2_big_initialized = 0;
		if (0 == val)
			eem2_big_lo_enable = 0;
		else
			eem2_big_lo_enable = 1;
	}
	free_page((unsigned long)buf);

	return count;
}



/* eem2_ctrl_lo_0 */
static int eem2_ctrl_lo_0_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "eem2_ctrl_lo_0 = %x\n", eem2_ctrl_lo[0]);
	seq_printf(m, "[print by register] eem2_ctrl_lo_0 = %x\n", eem2_read(EEM2_CTRL_REG_0));
	seq_printf(m, "[print by register] eem2_ctrl_lo_2 = %x\n", eem2_read(EEM2_CTRL_REG_2));
	return 0;
}



static ssize_t eem2_ctrl_lo_0_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 16, eem2_ctrl_lo))
		config_LO_CTRL(
			(eem2_ctrl_lo[0]>>12) & 0x03,
			(eem2_ctrl_lo[0]>>8) & 0x0F,
			(eem2_ctrl_lo[0]>>4) & 0x0F,
			(eem2_ctrl_lo[0]>>3) & 0x01,
			(eem2_ctrl_lo[0]>>2) & 0x01,
			(eem2_ctrl_lo[0]>>1) & 0x01,
			eem2_ctrl_lo[0] & 0x01,
			(eem2_ctrl_lo[1]>>28) & 0x0f,
			(eem2_ctrl_lo[1]>>24) & 0x0f,
			(eem2_ctrl_lo[1]>>20) & 0x0f,
			(eem2_ctrl_lo[1]>>19) & 0x01,
			(eem2_ctrl_lo[1]>>15) & 0x0f,
			(eem2_ctrl_lo[1]>>11) & 0x0f,
			(eem2_ctrl_lo[1]>>7) & 0x0f,
			(eem2_ctrl_lo[1]>>6) & 0x01
		  );

	free_page((unsigned long)buf);

	return count;
}



/* eem2_ctrl_lo_1*/
static int eem2_ctrl_lo_1_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "eem2_ctrl_lo_1 = %x\n", eem2_ctrl_lo[1]);
	seq_printf(m, "[print by register] eem2_ctrl_lo_1 = %x\n", eem2_read(EEM2_CTRL_REG_1));
	seq_printf(m, "[print by register] eem2_ctrl_lo_3 = %x\n", eem2_read(EEM2_CTRL_REG_3));
	return 0;
}



static ssize_t eem2_ctrl_lo_1_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 16, (eem2_ctrl_lo + 1))) {
		eem2_ctrl_lo[1] = eem2_ctrl_lo[1] << 4;
		config_LO_CTRL(
			(eem2_ctrl_lo[0]>>12) & 0x03,
			(eem2_ctrl_lo[0]>>8) & 0x0F,
			(eem2_ctrl_lo[0]>>4) & 0x0F,
			(eem2_ctrl_lo[0]>>3) & 0x01,
			(eem2_ctrl_lo[0]>>2) & 0x01,
			(eem2_ctrl_lo[0]>>1) & 0x01,
			eem2_ctrl_lo[0] & 0x01,
			(eem2_ctrl_lo[1]>>28) & 0x0f,
			(eem2_ctrl_lo[1]>>24) & 0x0f,
			(eem2_ctrl_lo[1]>>20) & 0x0f,
			(eem2_ctrl_lo[1]>>19) & 0x01,
			(eem2_ctrl_lo[1]>>15) & 0x0f,
			(eem2_ctrl_lo[1]>>11) & 0x0f,
			(eem2_ctrl_lo[1]>>7) & 0x0f,
			(eem2_ctrl_lo[1]>>6) & 0x01
			);
	}

	free_page((unsigned long)buf);
	return count;
}



/* eem2_big_ctrl_lo */
static int eem2_big_ctrl_lo_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "eem2_big_ctrl_lo = %08lx\n", (unsigned long)eem2_big_regs[3]);
	seq_printf(m, "[print by register] eem2_big_ctrl_lo = %x\n", eem2_read(EEM2_DET_CPU_ENABLE_ADDR));
	return 0;
}



static ssize_t eem2_big_ctrl_lo_proc_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 16, (eem2_big_regs + 3))) /* 0xc453 */
		eem2_dbg("write success (0x%x)", *(eem2_big_regs + 3));

	free_page((unsigned long)buf);

	return count;
}



/* eem2_big_ctrl_trig_0 */
static int eem2_big_ctrl_trig_0_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "eem2_big_ctrl_trig_0 = %x\n", eem2_big_regs[0]);
	seq_printf(m, "[Reg]eem2_big_ctrl_trig_0 = %08lx\n",
		(unsigned long)eem2_read(EEM2_BIG_CTRL_REG_0));
	return 0;
}



static ssize_t eem2_big_ctrl_trig_0_proc_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 16, eem2_big_regs)) { /* 0x33338 */
		eem2_big_regs[0] = eem2_big_regs[0] << 12;
		eem2_dbg("write success (0x%x)", *(eem2_big_regs));
	}
	free_page((unsigned long)buf);

	return count;
}



/* eem2_big_ctrl_trig_1 */
static int eem2_big_ctrl_trig_1_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "eem2_big_ctrl_trig_1 = %x\n", eem2_big_regs[1]);
	seq_printf(m, "[Reg]eem2_big_ctrl_trig_1 = %08lx\n",
		(unsigned long)eem2_read(EEM2_BIG_CTRL_REG_1));
	return 0;
}



static ssize_t eem2_big_ctrl_trig_1_proc_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 16, (eem2_big_regs + 1))) { /* 0xF */
		eem2_big_regs[1] = eem2_big_regs[1] << 28;
		eem2_dbg("write success (0x%x)", *(eem2_big_regs + 1));
	}
	free_page((unsigned long)buf);

	return count;
}



 /* eem2_big_ctrl_trig_2 */
static int eem2_big_ctrl_trig_2_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "eem2_big_ctrl_trig_2 = %x\n", eem2_big_regs[2]);
	seq_printf(m, "[Reg]eem2_big_ctrl_trig_2 = %08lx\n",
		(unsigned long)eem2_read(EEM2_BIG_CTRL_REG_2));
	return 0;
}



static ssize_t eem2_big_ctrl_trig_2_proc_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 16, (eem2_big_regs + 2))) {/* 0xF */
		eem2_big_regs[2] = eem2_big_regs[2] << 28;
		eem2_dbg("write success (0x%x)", *(eem2_big_regs + 2));
	}
	free_page((unsigned long)buf);

	return count;
}



/* eem2_dump */
static int eem2_dump_proc_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < EEM2_REG_NUM; i++) {
		seq_printf(m, "0x%lx = 0x%x\n",
			(EEM2_CTRL_REG_0_PHYS + (i << 2)),
			eem2_read(EEM2_CTRL_REG_0 + (i << 2)));
		seq_printf(m, "0x%lx = 0x%x\n",
			(EEM2_CTRL_REG_2_PHYS + (i << 2)),
			eem2_read(EEM2_CTRL_REG_2 + (i << 2)));
	}

	return 0;
}



/* eem2_big_dump */
static int eem2_big_dump_proc_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < EEM2_BIG_REG_NUM; i++) {
		seq_printf(m, "%08lx\n",
			(unsigned long)eem2_read(EEM2_BIG_CTRL_REG_0 + (i << 2)));
	}

	return 0;
}



#define PROC_FOPS_RW(name)                          \
	static int name ## _proc_open(struct inode *inode, struct file *file)   \
	{                                   \
		return single_open(file, name ## _proc_show, PDE_DATA(inode));  \
	}                                   \
	static const struct file_operations name ## _proc_fops = {      \
		.owner          = THIS_MODULE,                  \
		.open           = name ## _proc_open,               \
		.read           = seq_read,                 \
		.llseek         = seq_lseek,                    \
		.release        = single_release,               \
		.write          = name ## _proc_write,              \
	}

#define PROC_FOPS_RO(name)                          \
	static int name ## _proc_open(struct inode *inode, struct file *file)   \
	{                                   \
		return single_open(file, name ## _proc_show, PDE_DATA(inode));  \
	}                                   \
	static const struct file_operations name ## _proc_fops = {      \
		.owner          = THIS_MODULE,                  \
		.open           = name ## _proc_open,               \
		.read           = seq_read,                 \
		.llseek         = seq_lseek,                    \
		.release        = single_release,               \
	}

#define PROC_ENTRY(name)    {__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(eem2_lo_enable);
PROC_FOPS_RW(eem2_big_lo_enable);
PROC_FOPS_RW(eem2_ctrl_lo_0);
PROC_FOPS_RW(eem2_ctrl_lo_1);
PROC_FOPS_RW(eem2_big_ctrl_lo);
PROC_FOPS_RW(eem2_big_ctrl_trig_0);
PROC_FOPS_RW(eem2_big_ctrl_trig_1);
PROC_FOPS_RW(eem2_big_ctrl_trig_2);
PROC_FOPS_RO(eem2_dump);
PROC_FOPS_RO(eem2_big_dump);



static int _create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(eem2_lo_enable),
		PROC_ENTRY(eem2_big_lo_enable),
		PROC_ENTRY(eem2_ctrl_lo_0),
		PROC_ENTRY(eem2_ctrl_lo_1),
		PROC_ENTRY(eem2_big_ctrl_lo),
		PROC_ENTRY(eem2_big_ctrl_trig_0),
		PROC_ENTRY(eem2_big_ctrl_trig_1),
		PROC_ENTRY(eem2_big_ctrl_trig_2),
		PROC_ENTRY(eem2_dump),
		PROC_ENTRY(eem2_big_dump),
	};

	dir = proc_mkdir("eem2", NULL);

	if (!dir) {
		eem2_err("fail to create /proc/eem2 @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			eem2_err("%s(), create /proc/eem2/%s failed\n", __func__, entries[i].name);
	}

	return 0;
}
#endif /* CONFIG_PROC_FS */



void eem2_pre_iomap(void)
{
	struct resource r;

#ifdef CONFIG_OF
	if (eem2_base == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,mcucfg");

		if (node) {
			/* Setup IO addresses */
			eem2_base = of_iomap(node, 0);
			eem2_err("[VIRTUAL] eem2_base=0x%p\n", (void *)eem2_base);
			if (of_address_to_resource(node, 0, &r)) {
				eem2_err("%s(), error: cannot get phys addr", __func__);
			} else {
				eem2_phy_base = r.start;
				eem2_err("[PHYSICAL] eem2_phy_base=0x%p\n", (void *)eem2_phy_base);
			}
		} else {
			eem2_err("%s(), error: cannot find node ", __func__);
		}
	}
#endif
}



int eem2_pre_init(void)
{
	/* eem2_err("EEM2_base = 0x%16p\n", (void *)eem2_base);*/
	if (eem2_big_initialized == 1)
		return 0;

	/* EEM2_BIG_LO */
	if (1 == eem2_big_lo_enable)
		turn_on_big_LO();
	else
		turn_off_big_LO();
	eem2_big_initialized = 1;
	return 1;
}



/*
 * Module driver
 */
static int __init eem2_init(void)
{
	int err = 0;
	struct resource r;

	if (eem2_base == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,mcucfg");
		if (node) {
			/* Setup IO addresses */
			eem2_base = of_iomap(node, 0);
			eem2_err("[VIRTUAL] eem2_base=0x%p\n", (void *)eem2_base);
			if (of_address_to_resource(node, 0, &r)) {
				eem2_err("%s(), error: cannot get phys addr", __func__);
			} else {
				eem2_phy_base = r.start;
				eem2_err("[PHYSICAL] eem2_phy_base=0x%p\n", (void *)eem2_phy_base);
			}
		} else {
			eem2_err("%s(), error: cannot find node ", __func__);
		}
	} else {
		eem2_err("[VIRTUAL] eem2_base=0x%p\n", (void *)eem2_base);
		eem2_err("[PHYSICAL] eem2_phy_base=0x%p\n", (void *)eem2_phy_base);
	}

	err = platform_driver_register(&eem2_driver);
	if (err) {
		eem2_err("%s(), eem2 driver callback register failed..\n", __func__);
		return err;
	}

	/* ver = mt_get_chip_sw_ver(); */
	turn_on_LO();

#ifdef CONFIG_PROC_FS
	/* init proc */
	if (_create_procfs()) {
		err = -ENOMEM;
		goto out;
	}
#endif /* CONFIG_PROC_FS */

out:
	return err;
}



static void __exit eem2_exit(void)
{
	eem2_dbg("xxx2 de-initialization\n");
}



module_init(eem2_init);
module_exit(eem2_exit);

MODULE_DESCRIPTION("MediaTek EEM2 Driver v0.1");
MODULE_LICENSE("GPL");

#undef __MT_EEM2_C__
