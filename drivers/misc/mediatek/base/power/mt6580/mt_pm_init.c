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

#include <linux/pm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <mach/irqs.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_clkbuf_ctl.h>
#include "mt_cpufreq.h"
#include "mt_gpufreq.h"
#include <mt-plat/sync_write.h>
#include <mt_spm.h>
#include "mt_sleep.h"
#include "mt_cpuidle.h"

#include "mt-plat/mtk_rtc.h"

#define pminit_write(addr, val)         mt_reg_sync_writel((val), ((void *)(addr)))
#define pminit_read(addr)               __raw_readl(IOMEM(addr))
#define TOPCK_LDVT

#ifdef TOPCK_LDVT

#define DEBUG_FQMTR
#ifdef DEBUG_FQMTR

#ifdef __KERNEL__
#define FQMTR_TAG     "Power/swap"

#define fqmtr_err(fmt, args...)       \
	pr_debug(fmt, ##args)
#define fqmtr_warn(fmt, args...)      \
	pr_debug(fmt, ##args)
#define fqmtr_info(fmt, args...)      \
	pr_debug(fmt, ##args)
#define fqmtr_dbg(fmt, args...)       \
	pr_debug(fmt, ##args)
#define fqmtr_ver(fmt, args...)       \
	pr_debug(fmt, ##args)

#define fqmtr_read(addr)            __raw_readl(IOMEM(addr))
#define fqmtr_write(addr, val)      mt_reg_sync_writel(val, addr)

#else

#define fqmtr_err(fmt, args...)       \
	print(fmt, ##args)
#define fqmtr_warn(fmt, args...)      \
	print(fmt, ##args)
#define fqmtr_info(fmt, args...)      \
	print(fmt, ##args)
#define fqmtr_dbg(fmt, args...)       \
	print(fmt, ##args)
#define fqmtr_ver(fmt, args...)       \
	print(fmt, ##args)

#define fqmtr_read(addr)            DRV_Reg32(addr)
#define fqmtr_write(addr, val)      DRV_WriteReg32(addr, val)

#endif

#define FREQ_MTR_CTRL_REG               (FREQ_MTR_CTRL)
#define FREQ_MTR_CTRL_RDATA		(FREQ_MTR_CTRL + 0x4)
#define RG_FQMTR_CKDIV_GET(x)           (((x) >> 28) & 0x3)
#define RG_FQMTR_CKDIV_SET(x)           (((x) & 0x3) << 28)
#define RG_FQMTR_FIXCLK_SEL_GET(x)      (((x) >> 24) & 0x3)
#define RG_FQMTR_FIXCLK_SEL_SET(x)      (((x) & 0x3) << 24)
#define RG_FQMTR_MONCLK_SEL_GET(x)      (((x) >> 16) & 0x3f)
#define RG_FQMTR_MONCLK_SEL_SET(x)      (((x) & 0x3f) << 16)
#define RG_FQMTR_MONCLK_EN_GET(x)       (((x) >> 15) & 0x1)
#define RG_FQMTR_MONCLK_EN_SET(x)       (((x) & 0x1) << 15)
#define RG_FQMTR_MONCLK_RST_GET(x)      (((x) >> 14) & 0x1)
#define RG_FQMTR_MONCLK_RST_SET(x)      (((x) & 0x1) << 14)
#define RG_FQMTR_MONCLK_WINDOW_GET(x)   (((x) >> 0) & 0xfff)
#define RG_FQMTR_MONCLK_WINDOW_SET(x)   (((x) & 0xfff) << 0)
#define RG_FQMTR_CKDIV_DIV_2    0
#define RG_FQMTR_CKDIV_DIV_4    1
#define RG_FQMTR_CKDIV_DIV_8    2
#define RG_FQMTR_CKDIV_DIV_16   3
#define RG_FQMTR_FIXCLK_26MHZ   0
#define RG_FQMTR_FIXCLK_32KHZ   2

enum rg_fqmtr_monclk {
	RG_FQMTR_MONCLK_PDN = 0,
	RG_FQMTR_MONCLK_MAINPLL_DIV8,
	RG_FQMTR_MONCLK_MAINPLL_DIV11,
	RG_FQMTR_MONCLK_MAINPLL_DIV12,
	RG_FQMTR_MONCLK_MAINPLL_DIV20,
	RG_FQMTR_MONCLK_MAINPLL_DIV7,
	RG_FQMTR_MONCLK_MAINPLL_DIV16,
	RG_FQMTR_MONCLK_MAINPLL_DIV24,
	RG_FQMTR_MONCLK_NFI2X,
	RG_FQMTR_MONCLK_WHPLL,
	RG_FQMTR_MONCLK_WPLL,
	RG_FQMTR_MONCLK_26MHZ,
	RG_FQMTR_MONCLK_USB_48MHZ,
	RG_FQMTR_MONCLK_EMI1X ,
	RG_FQMTR_MONCLK_AP_INFRA_FAST_BUS,
	RG_FQMTR_MONCLK_SMI_MMSYS,
	RG_FQMTR_MONCLK_UART0,
	RG_FQMTR_MONCLK_UART1,
	RG_FQMTR_MONCLK_GPU,
	RG_FQMTR_MONCLK_MSDC0,
	RG_FQMTR_MONCLK_MSDC1,
	RG_FQMTR_MONCLK_AD_DSI0_LNTC_DSICLK,
	RG_FQMTR_MONCLK_AD_MPPLL_TST_CK,
	RG_FQMTR_MONCLK_AP_PLLLGP_TST_CK,
	RG_FQMTR_MONCLK_52MHZ,
	RG_FQMTR_MONCLK_ARMPLL,
	RG_FQMTR_MONCLK_32KHZ,
	RG_FQMTR_MONCLK_AD_MEMPLL_MONCLK,
	RG_FQMTR_MONCLK_AD_MEMPLL2_MONCLK,
	RG_FQMTR_MONCLK_AD_MEMPLL3_MONCLK,
	RG_FQMTR_MONCLK_AD_MEMPLL4_MONCLK,
	RG_FQMTR_MONCLK_RESERVED,
	RG_FQMTR_MONCLK_CAM_SENINF,
	RG_FQMTR_MONCLK_SCAM,
	RG_FQMTR_MONCLK_PWM_MMSYS,
	RG_FQMTR_MONCLK_DDRPHYCFG,
	RG_FQMTR_MONCLK_PMIC_SPI,
	RG_FQMTR_MONCLK_SPI,
	RG_FQMTR_MONCLK_104MHZ,
	RG_FQMTR_MONCLK_USB_78MHZ,
	RG_FQMTR_MONCLK_MAX,
};

const char *rg_fqmtr_monclk_name[] = {
	[RG_FQMTR_MONCLK_PDN]                   = "power done",
	[RG_FQMTR_MONCLK_MAINPLL_DIV8]          = "mainpll div8",
	[RG_FQMTR_MONCLK_MAINPLL_DIV11]         = "mainpll div11",
	[RG_FQMTR_MONCLK_MAINPLL_DIV12]         = "mainpll div12",
	[RG_FQMTR_MONCLK_MAINPLL_DIV20]         = "mainpll div20",
	[RG_FQMTR_MONCLK_MAINPLL_DIV7]          = "mainpll div7",
	[RG_FQMTR_MONCLK_MAINPLL_DIV16]         = "mainpll div16",
	[RG_FQMTR_MONCLK_MAINPLL_DIV24]         = "mainpll div24",
	[RG_FQMTR_MONCLK_NFI2X]                 = "nfi2x",
	[RG_FQMTR_MONCLK_WHPLL]                 = "whpll",
	[RG_FQMTR_MONCLK_WPLL]                  = "wpll",
	[RG_FQMTR_MONCLK_26MHZ]                 = "26MHz",
	[RG_FQMTR_MONCLK_USB_48MHZ]             = "USB 48MHz",
	[RG_FQMTR_MONCLK_EMI1X]                 = "emi1x",
	[RG_FQMTR_MONCLK_AP_INFRA_FAST_BUS]     = "AP infra fast bus",
	[RG_FQMTR_MONCLK_SMI_MMSYS]             = "smi mmsys",
	[RG_FQMTR_MONCLK_UART0]                 = "uart0",
	[RG_FQMTR_MONCLK_UART1]                 = "uart1",
	[RG_FQMTR_MONCLK_GPU]                   = "gpu",
	[RG_FQMTR_MONCLK_MSDC0]                 = "msdc0",
	[RG_FQMTR_MONCLK_MSDC1]                 = "msdc1",
	[RG_FQMTR_MONCLK_AD_DSI0_LNTC_DSICLK]   = "AD_DSI0_LNTC_DSICLK(mipi)",
	[RG_FQMTR_MONCLK_AD_MPPLL_TST_CK]       = "AD_MPPLL_TST_CK(mipi)",
	[RG_FQMTR_MONCLK_AP_PLLLGP_TST_CK]      = "AP_PLLLGP_TST_CK",
	[RG_FQMTR_MONCLK_52MHZ]                 = "52MHz",
	[RG_FQMTR_MONCLK_ARMPLL]                = "ARMPLL",
	[RG_FQMTR_MONCLK_32KHZ]                 = "32Khz",
	[RG_FQMTR_MONCLK_AD_MEMPLL_MONCLK]      = "AD_MEMPLL_MONCLK",
	[RG_FQMTR_MONCLK_AD_MEMPLL2_MONCLK]     = "AD_MEMPLL_2MONCLK",
	[RG_FQMTR_MONCLK_AD_MEMPLL3_MONCLK]     = "AD_MEMPLL_3MONCLK",
	[RG_FQMTR_MONCLK_AD_MEMPLL4_MONCLK]     = "AD_MEMPLL_4MONCLK",
	[RG_FQMTR_MONCLK_RESERVED]              = "Reserved",
	[RG_FQMTR_MONCLK_CAM_SENINF]            = "CAM seninf",
	[RG_FQMTR_MONCLK_SCAM]                  = "SCAM",
	[RG_FQMTR_MONCLK_PWM_MMSYS]             = "PWM mmsys",
	[RG_FQMTR_MONCLK_DDRPHYCFG]             = "ddrphycfg",
	[RG_FQMTR_MONCLK_PMIC_SPI]              = "PMIC SPI",
	[RG_FQMTR_MONCLK_SPI]                   = "SPI",
	[RG_FQMTR_MONCLK_104MHZ]                = "104MHz",
	[RG_FQMTR_MONCLK_USB_78MHZ]             = "USB 78MHz",
	[RG_FQMTR_MONCLK_MAX]                   = "MAX",
};

#define RG_FQMTR_EN     1
#define RG_FQMTR_RST    1

#define RG_FRMTR_WINDOW     0x100

unsigned int do_fqmtr_ctrl(int fixclk, int monclk_sel)
{
	u32 value = 0;

	BUG_ON(!((fixclk == RG_FQMTR_FIXCLK_26MHZ) | (fixclk == RG_FQMTR_FIXCLK_32KHZ)));

	if (monclk_sel == RG_FQMTR_MONCLK_ARMPLL)
		fqmtr_write(TEST_DBG_CTRL, ((fqmtr_read(TEST_DBG_CTRL) & ~0xff) | 0x1));

	fqmtr_write(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_RST_SET(RG_FQMTR_RST));
	fqmtr_write(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_RST_SET(!RG_FQMTR_RST));
	fqmtr_write(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_WINDOW_SET(RG_FRMTR_WINDOW) |
		RG_FQMTR_MONCLK_SEL_SET(monclk_sel) |
		RG_FQMTR_FIXCLK_SEL_SET(fixclk)	|
		RG_FQMTR_MONCLK_EN_SET(RG_FQMTR_EN));

	while (fqmtr_read(FREQ_MTR_CTRL_RDATA) & 0x80000000) {
		value++;
		if (value > 2000)
			break;
	}
	value = fqmtr_read(FREQ_MTR_CTRL_RDATA);

	fqmtr_write(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_RST_SET(RG_FQMTR_RST));
	fqmtr_write(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_RST_SET(!RG_FQMTR_RST));

	if (monclk_sel == RG_FQMTR_MONCLK_ARMPLL)
		value *= 2;
	if (fixclk == RG_FQMTR_FIXCLK_26MHZ)
		return ((26 * value) / (RG_FRMTR_WINDOW + 1));
	return ((32000 * value) / (RG_FRMTR_WINDOW + 1));

}

void dump_fqmtr(void)
{
	int i = 0;
	unsigned int ret;

	for (i = 0; i < RG_FQMTR_MONCLK_MAX; i++) {
		if (i == RG_FQMTR_MONCLK_RESERVED)
			continue;
		ret = do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, i);
		fqmtr_dbg("%s - %d MHz\n", rg_fqmtr_monclk_name[i], ret);
	}
}
#endif /* DEBUG_FQMTR */

static int fqmtr_read_fs(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < RG_FQMTR_MONCLK_MAX; i++) {
		if (i == RG_FQMTR_MONCLK_RESERVED)
			continue;
		seq_printf(m, "%d:%s - %d\n", i, rg_fqmtr_monclk_name[i], do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, i));
	}

	return 0;
}

static int proc_fqmtr_open(struct inode *inode, struct file *file)
{
	return single_open(file, fqmtr_read_fs, NULL);
}
static const struct file_operations fqmtr_fops = {
	.owner = THIS_MODULE,
	.open  = proc_fqmtr_open,
	.read  = seq_read,
};

#endif

/*********************************************************************
 * FUNCTION DEFINATIONS
 ********************************************************************/

unsigned int mt_get_emi_freq(void)
{
	return do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, RG_FQMTR_MONCLK_EMI1X);
}
EXPORT_SYMBOL(mt_get_emi_freq);

unsigned int mt_get_bus_freq(void)
{
	return do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, RG_FQMTR_MONCLK_AP_INFRA_FAST_BUS);
}
EXPORT_SYMBOL(mt_get_bus_freq);

unsigned int mt_get_cpu_freq(void)
{
	return do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, RG_FQMTR_MONCLK_ARMPLL);

}
EXPORT_SYMBOL(mt_get_cpu_freq);

unsigned int mt_get_mmclk_freq(void)
{
	return do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, RG_FQMTR_MONCLK_EMI1X);
}
EXPORT_SYMBOL(mt_get_mmclk_freq);

unsigned int mt_get_mfgclk_freq(void)
{
	return do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, RG_FQMTR_MONCLK_GPU);
}
EXPORT_SYMBOL(mt_get_mfgclk_freq);

static int cpu_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", mt_get_cpu_freq());
	return 0;
}

static int emi_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", mt_get_emi_freq());
	return 0;
}

static int bus_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", mt_get_bus_freq());
	return 0;
}

static int mmclk_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", mt_get_mmclk_freq());
	return 0;
}

static int mfgclk_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", mt_get_mfgclk_freq());
	return 0;
}

static int proc_cpu_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpu_speed_dump_read, NULL);
}
static const struct file_operations cpu_fops = {
	.owner = THIS_MODULE,
	.open  = proc_cpu_open,
	.read  = seq_read,
};

static int proc_emi_open(struct inode *inode, struct file *file)
{
	return single_open(file, emi_speed_dump_read, NULL);
}
static const struct file_operations emi_fops = {
	.owner = THIS_MODULE,
	.open  = proc_emi_open,
	.read  = seq_read,
};

static int proc_bus_open(struct inode *inode, struct file *file)
{
	return single_open(file, bus_speed_dump_read, NULL);
}
static const struct file_operations bus_fops = {
	.owner = THIS_MODULE,
	.open  = proc_bus_open,
	.read  = seq_read,
};

static int proc_mmclk_open(struct inode *inode, struct file *file)
{
	return single_open(file, mmclk_speed_dump_read, NULL);
}
static const struct file_operations mmclk_fops = {
	.owner = THIS_MODULE,
	.open  = proc_mmclk_open,
	.read  = seq_read,
};

static int proc_mfgclk_open(struct inode *inode, struct file *file)
{
	return single_open(file, mfgclk_speed_dump_read, NULL);
}
static const struct file_operations mfgclk_fops = {
	.owner = THIS_MODULE,
	.open  = proc_mfgclk_open,
	.read  = seq_read,
};

static int __init mt_power_management_init(void)
{
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *pm_init_dir = NULL;

	pm_power_off = mt_power_off;
	/* cpu dormant driver init */
	mt_cpu_dormant_init();

	spm_module_init();
	slp_module_init();
	mt_clkmgr_init();

	pm_init_dir = proc_mkdir("pm_init", NULL);
	pm_init_dir = proc_mkdir("pm_init", NULL);
	if (!pm_init_dir) {
		pr_err("[%s]: mkdir /proc/pm_init failed\n", __func__);
	} else {
		entry = proc_create("cpu_speed_dump", S_IRUGO, pm_init_dir, &cpu_fops);

		entry = proc_create("emi_speed_dump", S_IRUGO, pm_init_dir, &emi_fops);

		entry = proc_create("bus_speed_dump", S_IRUGO, pm_init_dir, &bus_fops);

		entry = proc_create("mmclk_speed_dump", S_IRUGO, pm_init_dir, &mmclk_fops);

		entry = proc_create("mfgclk_speed_dump", S_IRUGO, pm_init_dir, &mfgclk_fops);
#ifdef TOPCK_LDVT
		entry = proc_create("fqmtr_test", S_IRUGO, pm_init_dir, &fqmtr_fops);
#endif
	}
	return 0;
}
arch_initcall(mt_power_management_init);

static int __init mt_pm_late_init(void)
{
	clk_buf_init();
	return 0;
}

late_initcall(mt_pm_late_init);

MODULE_DESCRIPTION("MTK Power Management Init Driver");
MODULE_LICENSE("GPL");
