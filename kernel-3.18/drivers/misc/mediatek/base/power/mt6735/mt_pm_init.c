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
/* #include <linux/xlog.h> */

#include <asm/io.h>
#include <asm/uaccess.h>

/* #include "mach/irqs.h" */
#include <mt-plat/sync_write.h>
/* #include "mach/mt_reg_base.h" */
/* #include "mach/mt_typedefs.h" */
#include "mt_spm.h"
#include "mt_sleep.h"
#include "mt_dcm.h"
#include "mt_clkmgr.h"
#include "mt_cpufreq.h"
#include "mt_gpufreq.h"
/* #include "mach/mt_sleep.h" */
/* #include "mach/mt_dcm.h" */
#include <mach/mt_clkmgr.h>
/* #include "mach/mt_cpufreq.h" */
/* #include "mach/mt_gpufreq.h" */
#include "mt_cpuidle.h"
#include "mt_clkbuf_ctl.h"
/* #include "mach/mt_clkbuf_ctl.h" */
/* #include "mach/mt_chip.h" */
#include "mt-plat/mtk_rtc.h"

#define pminit_write(addr, val)         mt_reg_sync_writel((val), ((void *)(addr)))
#define pminit_read(addr)               __raw_readl(IOMEM(addr))
#ifndef DRV_WriteReg32
#define DRV_WriteReg32(addr, val)   \
	mt_reg_sync_writel(val, addr)
#endif

#define TOPCK_LDVT
#ifdef TOPCK_LDVT
/***************************
*For TOPCKGen Meter LDVT Test
****************************/
static unsigned int ckgen_meter(int val)
{
	int output = 0;
	int i = 0;
	unsigned int temp, clk26cali_0, clk_cfg_9, clk_misc_cfg_1;

	clk26cali_0 = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80); /* enable fmeter_en */

	clk_misc_cfg_1 = readl(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0x00FFFFFF); /* select divider */

	clk_cfg_9 = readl(CLK_CFG_9);
	DRV_WriteReg32(CLK_CFG_9, (val << 16)); /* select ckgen_cksw */

	temp = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x10); /* start fmeter */

	/* wait frequency meter finish */
	while (readl(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = readl(CLK26CALI_2) & 0xFFFF;

	output = (temp * 26000) / 1024; /* Khz */

	DRV_WriteReg32(CLK_CFG_9, clk_cfg_9);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);

	if (i > 10)
		return 0;
	else
		return output;
}

static unsigned int abist_meter(int val)
{
	int output = 0;
	int i = 0;
	unsigned int temp, clk26cali_0, clk_cfg_8, clk_misc_cfg_1;

	clk26cali_0 = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80); /* enable fmeter_en */

	clk_misc_cfg_1 = readl(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0xFFFFFF00); /* select divider */

	clk_cfg_8 = readl(CLK_CFG_8);
	DRV_WriteReg32(CLK_CFG_8, (val << 8)); /* select abist_cksw */

	temp = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x1); /* start fmeter */

	/* wait frequency meter finish */
	while (readl(CLK26CALI_0) & 0x1) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = readl(CLK26CALI_1) & 0xFFFF;

	output = (temp * 26000) / 1024; /* Khz */

	DRV_WriteReg32(CLK_CFG_8, clk_cfg_8);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);

	if (i > 10)
		return 0;
	else
		return output;
}


#if defined(CONFIG_ARCH_MT6753)
const char *ckgen_array[] = {
	"hf_faxi_ck", "hd_faxi_ck", "hf_fdpi0_ck", "hf_fddrphycfg_ck", "hf_fmm_ck",
	"f_fpwm_ck", "hf_fvdec_ck", "hf_fmfg_ck", "hf_fcamtg_ck", "f_fuart_ck",
	"hf_fspi_ck", "f_fusb20_ck", "hf_fmsdc30_0_ck", "hf_fmsdc30_1_ck", "hf_fmsdc30_2_ck",
	"hf_faudio_ck", "hf_faud_intbus_ck", "hf_fpmicspi_ck", "f_frtc_ck", "f_f26m_ck",
	"f_f32k_md1_ck", "f_frtc_conn_ck", "hf_fmsdc30_3_ck", "hg_fmipicfg_ck", "NULL",
	"hd_qaxidcm_ck", "NULL", "hf_fscam_ck", "f_fckbus_scan", " f_fckrtc_scan",
	"hf_fatb_ck", "hf_faud_1_ck", "hf_faud_2_ck", "hf_fmsdc50_0_ck", "hf_firda_ck",
	" hf_firtx_ck", "hf_fdisppwm_ck", "hs_fmfg13m_ck"
};
#else
const char *ckgen_array[] = {
	"hf_faxi_ck", "hd_faxi_ck", "hf_fdpi0_ck", "hf_fddrphycfg_ck", "hf_fmm_ck",
	"f_fpwm_ck", "hf_fvdec_ck", "hf_fmfg_ck", "hf_fcamtg_ck", "f_fuart_ck",
	"hf_fspi_ck", "f_fusb20_ck", "hf_fmsdc30_0_ck", "hf_fmsdc30_1_ck", "hf_fmsdc30_2_ck",
	"hf_faudio_ck", "hf_faud_intbus_ck", "hf_fpmicspi_ck", "f_frtc_ck", "f_f26m_ck",
	"f_f32k_md1_ck", "f_frtc_conn_ck", "f_fusb20p1_ck", "hg_fmipicfg_ck", "NULL",
	"hd_qaxidcm_ck", "NULL", "hf_fscam_ck", "f_fckbus_scan", " f_fckrtc_scan",
	"hf_fatb_ck", "hf_faud_1_ck", "hf_faud_2_ck", "hf_fmsdc50_0_ck", "hf_firda_ck",
	" hf_firtx_ck", "hf_fdisppwm_ck", "hs_fmfg13m_ck"
};
#endif

static int ckgen_meter_read(struct seq_file *m, void *v)
{
	int i;

	for (i = 1; i < 39; i++)
		seq_printf(m, "%s: %d\n", ckgen_array[i-1], ckgen_meter(i));

	return 0;
}

static ssize_t ckgen_meter_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	/*int val;*/

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	/*if (sscanf(desc, "%d", &val) == 1)*/
	/*if (kstrtoint(desc, "%d", &val) == 1)
		pr_debug("ckgen_meter %d is %d\n", val, ckgen_meter(val));*/

	return count;
}


static int abist_meter_read(struct seq_file *m, void *v)
{
	int i;

	for (i = 1; i < 59; i++)
		seq_printf(m, "%d\n", abist_meter(i));

	return 0;
}
static ssize_t abist_meter_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	/*int val;*/

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	/*if (sscanf(desc, "%d", &val) == 1)*/
	/*if (kstrtoint(desc, "%d", &val) == 1)
		pr_debug("abist_meter %d is %d\n", val, abist_meter(val)); */

	return count;
}

static int proc_abist_meter_open(struct inode *inode, struct file *file)
{
	return single_open(file, abist_meter_read, NULL);
}
static const struct file_operations abist_meter_fops = {
	.owner = THIS_MODULE,
	.open  = proc_abist_meter_open,
	.read  = seq_read,
	.write = abist_meter_write,
};

static int proc_ckgen_meter_open(struct inode *inode, struct file *file)
{
	return single_open(file, ckgen_meter_read, NULL);
}
static const struct file_operations ckgen_meter_fops = {
	.owner = THIS_MODULE,
	.open  = proc_ckgen_meter_open,
	.read  = seq_read,
	.write = ckgen_meter_write,
};

#endif

/*********************************************************************
 * FUNCTION DEFINATIONS
 ********************************************************************/

static unsigned int mt_get_emi_freq(void)
{
	int output = 0;
	int i = 0;
	unsigned int temp, clk26cali_0, clk_cfg_8, clk_misc_cfg_1;

	clk26cali_0 = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80); /* enable fmeter_en */

	clk_misc_cfg_1 = readl(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0xFFFFFF00); /* select divider */

	clk_cfg_8 = readl(CLK_CFG_8);
	DRV_WriteReg32(CLK_CFG_8, (14 << 8)); /* select abist_cksw */

	temp = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x1); /* start fmeter */

	/* wait frequency meter finish */
	while (readl(CLK26CALI_0) & 0x1) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = readl(CLK26CALI_1) & 0xFFFF;

	output = (temp * 26000) / 1024; /* Khz */

	DRV_WriteReg32(CLK_CFG_8, clk_cfg_8);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);

	if (i > 10)
		return 0;
	else
		return output;
}
EXPORT_SYMBOL(mt_get_emi_freq);

unsigned int mt_check_bus_freq(void)
{
	unsigned int axi_bus_sel = 0;
	unsigned int bus_clk = 273000;

	axi_bus_sel = (readl(CLK_CFG_0) & 0x00000007);
	switch (axi_bus_sel) {
	case 0: /* 26M */
		bus_clk = 26000;
		break;
	case 1: /* SYSPLL1_D2: 273M */
		bus_clk = 273000;
		break;
	case 2: /* SYSPLL_D5: 218.4M */
		bus_clk = 218400;
		break;
	case 3: /* SYSPLL1_D4: 136.5M */
		bus_clk = 136500;
		break;
	case 4: /* UNIVPLL_D5: 249.6M */
		bus_clk = 249600;
		break;
	case 5: /* UNIVPLL2_D2: 208M */
		bus_clk = 208000;
		break;
	default:
		break;
	}
		return bus_clk; /* Khz */
}
EXPORT_SYMBOL(mt_check_bus_freq);

unsigned int mt_get_bus_freq(void)
{
	int output = 0;
	int i = 0;
	unsigned int temp, clk26cali_0, clk_cfg_9, clk_misc_cfg_1;

	clk26cali_0 = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80); /* enable fmeter_en */

	clk_misc_cfg_1 = readl(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0x00FFFFFF); /* select divider */

	clk_cfg_9 = readl(CLK_CFG_9);
	DRV_WriteReg32(CLK_CFG_9, (1 << 16)); /* select ckgen_cksw */

	temp = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x10); /* start fmeter */

	/* wait frequency meter finish */
	while (readl(CLK26CALI_0) & 0x10) {
		udelay(10);
		i++;
		if (i > 1000)
			break;
	}

	temp = readl(CLK26CALI_2) & 0xFFFF;

	output = (temp * 26000) / 1024; /* Khz */

	DRV_WriteReg32(CLK_CFG_9, clk_cfg_9);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);

	if (i > 1000)
		return 0;
	else
		return output;
}
EXPORT_SYMBOL(mt_get_bus_freq);

static unsigned int mt_get_cpu_freq(void)
{
	int output = 0;
	int i = 0;
	unsigned int temp, clk26cali_0, clk_cfg_8, clk_misc_cfg_1;

	clk26cali_0 = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80); /* enable fmeter_en */

	clk_misc_cfg_1 = readl(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0xFFFF0300); /* select divider */

	clk_cfg_8 = readl(CLK_CFG_8);
	DRV_WriteReg32(CLK_CFG_8, (39 << 8)); /* select abist_cksw */

	temp = readl(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x1); /* start fmeter */

	/* wait frequency meter finish */
	while (readl(CLK26CALI_0) & 0x1) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = readl(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000) / 1024) * 4; /* Khz */

	DRV_WriteReg32(CLK_CFG_8, clk_cfg_8);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);

	if (i > 10)
		return 0;
	else
		return output;
}
EXPORT_SYMBOL(mt_get_cpu_freq);

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

#if 0
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
#endif

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

#if 0
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
#endif


static int __init mt_power_management_init(void)
{
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *pm_init_dir = NULL;
	/* unsigned int code = mt_get_chip_hw_code(); */

	pm_power_off = mt_power_off;

#if !defined(CONFIG_MTK_FPGA)

	/* cpu dormant driver init */
	mt_cpu_dormant_init();

	spm_module_init();
	slp_module_init();
	mt_clkmgr_init();

	/* mt_pm_log_init(); // power management log init */

	/* mt_dcm_init(); // dynamic clock management init */


	pm_init_dir = proc_mkdir("pm_init", NULL);
	/* pm_init_dir = proc_mkdir("pm_init", NULL); */
	if (!pm_init_dir) {
		pr_debug("[%s]: mkdir /proc/pm_init failed\n", __func__);
	} else {
		entry = proc_create("cpu_speed_dump", S_IRUGO, pm_init_dir, &cpu_fops);

		/* entry = proc_create("bigcpu_speed_dump", S_IRUGO, pm_init_dir, &bigcpu_fops); */

		entry = proc_create("emi_speed_dump", S_IRUGO, pm_init_dir, &emi_fops);

		entry = proc_create("bus_speed_dump", S_IRUGO, pm_init_dir, &bus_fops);

		/* entry = proc_create("mmclk_speed_dump", S_IRUGO, pm_init_dir, &mmclk_fops); */

		/* entry = proc_create("mfgclk_speed_dump", S_IRUGO, pm_init_dir, &mfgclk_fops); */
#ifdef TOPCK_LDVT
		entry = proc_create("abist_meter_test", S_IRUGO|S_IWUSR, pm_init_dir, &abist_meter_fops);
		entry = proc_create("ckgen_meter_test", S_IRUGO|S_IWUSR, pm_init_dir, &ckgen_meter_fops);
#endif
	}

#endif

	return 0;
}

arch_initcall(mt_power_management_init);


#if !defined(MT_DORMANT_UT)
static int __init mt_pm_late_init(void)
{
#ifndef CONFIG_MTK_FPGA
/*	mt_idle_init(); */
	clk_buf_init();
#endif
	return 0;
}

late_initcall(mt_pm_late_init);
#endif /* #if !defined (MT_DORMANT_UT) */


MODULE_DESCRIPTION("MTK Power Management Init Driver");
MODULE_LICENSE("GPL");
