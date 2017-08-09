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
/* #include "mach/mt_chip.h" */
#include <mach/mt_freqhopping.h>
#include "mt-plat/mtk_rtc.h"
#include "mt_freqhopping_drv.h"


#ifdef CONFIG_ARM64
#define IOMEM(a)	((void __force __iomem *)((a)))
#endif

#define pminit_write(addr, val)         mt_reg_sync_writel((val), ((void *)(addr)))
#define pminit_read(addr)               __raw_readl(IOMEM(addr))
#ifndef DRV_WriteReg32
#define DRV_WriteReg32(addr, val)   \
	mt_reg_sync_writel(val, addr)
#endif

int __attribute__ ((weak)) mt_cpu_dormant_init(void) { return MT_CPU_DORMANT_BYPASS; }

#define TOPCK_LDVT
#ifdef TOPCK_LDVT
/***************************
*For TOPCKGen Meter LDVT Test
****************************/
unsigned int ckgen_meter(int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk26cali_0 = pminit_read(CLK26CALI_0);

	clk_dbg_cfg = pminit_read(CLK_DBG_CFG);

	/*sel ckgen_cksw[0] and enable freq meter sel ckgen[13:8], 01:hd_faxi_ck*/
	pminit_write(CLK_DBG_CFG, (ID << 8) | 0x01);

	clk_misc_cfg_0 = pminit_read(CLK_MISC_CFG_0);
	pminit_write(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));	/*select divider?dvt set zero*/

	clk26cali_1 = pminit_read(CLK26CALI_1);
	/*pminit_write(CLK26CALI_1, 0x00ff0000); */

	/*temp = pminit_read(CLK26CALI_0);*/
	pminit_write(CLK26CALI_0, 0x1000);
	pminit_write(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (pminit_read(CLK26CALI_0) & 0x10) {
		udelay(10);
		i++;
		if (i > 10000)
			break;
	}

	temp = pminit_read(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000)) / 1024;	/* Khz*/

	pminit_write(CLK_DBG_CFG, clk_dbg_cfg);
	/*pminit_write(CLK_MISC_CFG_0, clk_misc_cfg_0);*/
	/*pminit_write(CLK26CALI_0, clk26cali_0);*/
	/*pminit_write(CLK26CALI_1, clk26cali_1);*/

	pminit_write(CLK26CALI_0, 0x1010);
	udelay(50);
	pminit_write(CLK26CALI_0, 0x1000);
	pminit_write(CLK26CALI_0, 0x0000);
	if (i > 10)
		return 0;
	else
		return output;

}

unsigned int abist_meter(int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk26cali_0 = pminit_read(CLK26CALI_0);

	clk_dbg_cfg = pminit_read(CLK_DBG_CFG);

	/*sel abist_cksw and enable freq meter sel abist*/
	pminit_write(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFC0FFFC) | (ID << 16));

	clk_misc_cfg_0 = pminit_read(CLK_MISC_CFG_0);
	pminit_write(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF)|0x01000000);	/* select divider, WAIT CONFIRM*/

	clk26cali_1 = pminit_read(CLK26CALI_1);
	/*pminit_write(CLK26CALI_1, 0x00ff0000); // cycle count default 1024,[25:16]=3FF*/

	/*temp = pminit_read(CLK26CALI_0);*/
	pminit_write(CLK26CALI_0, 0x1000);
	pminit_write(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (pminit_read(CLK26CALI_0) & 0x10) {
		udelay(10);
		i++;
		if (i > 10000)
			break;
	}


	temp = pminit_read(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000)) / 1024 * 2;	/* Khz*/

	pminit_write(CLK_DBG_CFG, clk_dbg_cfg);
	/*pminit_write(CLK_MISC_CFG_0, clk_misc_cfg_0);*/
	/*pminit_write(CLK26CALI_0, clk26cali_0);*/
	/*pminit_write(CLK26CALI_1, clk26cali_1);*/


	pminit_write(CLK26CALI_0, 0x1010);
	udelay(50);
	pminit_write(CLK26CALI_0, 0x1000);
	pminit_write(CLK26CALI_0, 0x0000);

	if (i > 10)
		return 0;
	else
		return output;
}

const char *ckgen_array[] = {
	"hd_faxi_ck",
	"hf_fddrphycfg_ck",
	"hf_fmm_ck",
	"f_fpwm_ck",
	"hf_fvdec_ck",
	"hf_fmfg_ck",
	"hf_fcamtg_ck",
	"f_fuart_ck",
	"hf_fspi_ck",
	"hf_fmsdc50_0_hclk_ck", /*10*/
	"hf_fmsdc50_0_ck",
	"hf_fmsdc30_1_ck",
	"hf_fmsdc30_2_ck",
	"hf_fmsdc30_3_ck",
	"hf_faudio_ck",
	"hf_faud_intbus_ck",
	"hf_fpmicspi_ck",
	"b0",
	"hf_fatb_ck",
	"hf_fmjc_ck",/*20*/
	"hf_fdpi0_ck",
	"hf_fscam_ck",
	"hf_faud_1_ck",
	"hf_faud_2_ck",
	"fmem_ck_bfe_dcm_ch0_gt",
	"fmem_ck_aft_dcm_ch0",
	"f_fdisp_pwm_ck",
	"f_fssusb_top_sys_ck",
	"f_fssusb_top_xhci_ck",
	"f_fusb_top_ck",/*30*/
	"hg_fspm_ck",
	"hf_fbsi_spi_ck",
	"f_fi2c_ck",
	"hg_fdvfsp_ck",
	"hf_fvenc_ck",
	"f52m_mfg_ck",
	"hf_fimg_ck",
	"fmem_ck_bfe_dcm_ch1_gt",
	"fmem_ck_aft_dcm_ch1"
};

const char *ckgen_abist_array[] = {
	"AD_CSI0_DELAY_TSTCLK",
	"AD_CSI1_DELAY_TSTCLK",
	"AD_PSMCUPLL_CK",
	"AD_L1MCUPLL_CK",
	"AD_C2KCPPLL_CK",
	"AD_ICCPLL_CK",
	"AD_INTFPLL_CK",
	"AD_MD2GPLL_CK",
	"AD_IMCPLL_CK",
	"AD_C2KDSPPLL_CK",/*10*/
	"AD_BRPPLL_CK",
	"AD_DFEPLL_CK",
	"AD_EQPLL_CK",
	"AD_CMPPLL_CK",
	"AD_MDPLLGP_TST_CK",
	"AD_MDPLL_FS26M_CK",
	"AD_MDPLL_FS208M_CK",
	"b0",
	"b0",
	"AD_ARMPLL_L_CK",/*20*/
	"b0",
	"AD_ARMPLL_LL_CK",
	"AD_MAINPLL_CK",
	"AD_UNIVPLL_CK",
	"AD_MMPLL_CK",
	"AD_MSDCPLL_CK",
	"AD_VENCPLL_CK",
	"AD_APLL1_CK",
	"AD_APLL2_CK",
	"AD_APPLLGP_MON_FM_CK",/*30*/
	"AD_USB20_192M_CK",
	"AD_UNIV_192M_CK",
	"AD_SSUSB_192M_CK",
	"AD_TVDPLL_CK",
	"AD_DSI0_MPPLL_TST_CK",
	"AD_DSI0_LNTC_DSICLK",
	"b0",
	"AD_OSC_CK",
	"rtc32k_ck_i",
	"armpll_hd_fram_ck_big",/*40*/
	"armpll_hd_fram_ck_sml",
	"armpll_hd_fram_ck_bus",
	"msdc01_in_ck",
	"msdc02_in_ck",
	"msdc11_in_ck",
	"msdc12_in_ck",
	"b0",
	"b0",
	"AD_CCIPLL_CK",
	"AD_MPLL_208M_CK",/*50*/
	"AD_WBG_DIG_CK_CK_416M",
	"AD_WBG_B_DIG_CK_64M",
	"AD_WBG_W_DIG_CK_160M",
	"DA_USB20_48M_DIV_CK",
	"DA_UNIV_48M_DIV_CK",
	"DA_SSUSB_48M_DIV_CK",
	"DA_MPLL_52M_DIV_CK"
};


static int ckgen_meter_read(struct seq_file *m, void *v)
{
	int i;

	for (i = 1; i < 39; i++) {
		/*skip unknown case */
		if (i == 18)
			continue;

		seq_printf(m, "[%d] %s: %d\n", i, ckgen_array[i - 1], ckgen_meter(i));
	}
	return 0;
}

static ssize_t ckgen_meter_write(struct file *file, const char __user *buffer,
				 size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	int val;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtoint(desc, 10, &val) >= 0)
		pr_debug("ckgen_meter %d is %d\n", val, ckgen_meter(val));

	return count;
}


static int abist_meter_read(struct seq_file *m, void *v)
{
	int i;

	for (i = 1; i < 57; i++) {
		/*skip unknown case */
		if (i == 18 || i == 19 || i == 21 || i == 37 || i == 47 || i == 48)
			continue;

		seq_printf(m, "[%d] %s: %d\n", i, ckgen_abist_array[i - 1], abist_meter(i));
	}
	return 0;
}

static ssize_t abist_meter_write(struct file *file, const char __user *buffer,
				 size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	int val;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtoint(desc, 10, &val) >= 0)
		pr_debug("abist_meter %d is %d\n", val, abist_meter(val));

	return count;
}

static int proc_abist_meter_open(struct inode *inode, struct file *file)
{
	return single_open(file, abist_meter_read, NULL);
}

static const struct file_operations abist_meter_fops = {
	.owner = THIS_MODULE,
	.open = proc_abist_meter_open,
	.read = seq_read,
	.write = abist_meter_write,
};

static int proc_ckgen_meter_open(struct inode *inode, struct file *file)
{
	return single_open(file, ckgen_meter_read, NULL);
}

static const struct file_operations ckgen_meter_fops = {
	.owner = THIS_MODULE,
	.open = proc_ckgen_meter_open,
	.read = seq_read,
	.write = ckgen_meter_write,
};

#endif

/*********************************************************************
 * FUNCTION DEFINATIONS
 ********************************************************************/

static unsigned int mt_get_emi_freq(void)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk26cali_0 = pminit_read(CLK26CALI_0);
	/*sel abist_cksw and enable abist meter sel abist*/
	clk_dbg_cfg = pminit_read(CLK_DBG_CFG);
	pminit_write(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFFFFFFC) | (50 << 16));/*AD_MPLL_208M_CK*/

	clk_misc_cfg_0 = pminit_read(CLK_MISC_CFG_0);
	pminit_write(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));	/* select divider*/

	clk26cali_1 = pminit_read(CLK26CALI_1);
	/*pminit_write(CLK26CALI_1, 0x00ff0000); */

	/*temp = pminit_read(CLK26CALI_0);*/
	pminit_write(CLK26CALI_0, 0x1000);
	pminit_write(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (pminit_read(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = pminit_read(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000)) / 1024;	/* Khz*/

	pminit_write(CLK_DBG_CFG, clk_dbg_cfg);
	pminit_write(CLK_MISC_CFG_0, clk_misc_cfg_0);
	pminit_write(CLK26CALI_0, clk26cali_0);
	pminit_write(CLK26CALI_1, clk26cali_1);

	if (i > 10)
		return 0;
	else
		return output;

}
EXPORT_SYMBOL(mt_get_emi_freq);

unsigned int mt_get_bus_freq(void)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk26cali_0 = pminit_read(CLK26CALI_0);

	clk_dbg_cfg = pminit_read(CLK_DBG_CFG);
	pminit_write(CLK_DBG_CFG, (1 << 8) | 0x01);	/* AXI */

	clk_misc_cfg_0 = pminit_read(CLK_MISC_CFG_0);
	pminit_write(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));	/* select divider*/

	clk26cali_1 = pminit_read(CLK26CALI_1);
	/*pminit_write(CLK26CALI_1, 0x00ff0000); */

	/*temp = pminit_read(CLK26CALI_0);*/
	pminit_write(CLK26CALI_0, 0x1000);
	pminit_write(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (pminit_read(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = pminit_read(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000)) / 1024;	/* Khz*/

	pminit_write(CLK_DBG_CFG, clk_dbg_cfg);
	pminit_write(CLK_MISC_CFG_0, clk_misc_cfg_0);
	pminit_write(CLK26CALI_0, clk26cali_0);
	pminit_write(CLK26CALI_1, clk26cali_1);

	if (i > 10)
		return 0;
	else
		return output;


}
EXPORT_SYMBOL(mt_get_bus_freq);


static int cpu_speed_dump_read(struct seq_file *m, void *v)
{

	seq_printf(m, "%s(LL): %d Khz\n", ckgen_abist_array[21], abist_meter(22));
	seq_printf(m, "%s(L): %d Khz\n", ckgen_abist_array[19], abist_meter(20));
	seq_printf(m, "%s(CCI): %d Khz\n", ckgen_abist_array[48], abist_meter(49));

	return 0;
}

static int mm_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s :  %d Khz\n", ckgen_abist_array[24], abist_meter(25));
	return 0;
}

static int venc_speed_dump_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s :  %d Khz\n", ckgen_abist_array[26], abist_meter(27));
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
	.open = proc_cpu_open,
	.read = seq_read,
};

static int proc_mm_open(struct inode *inode, struct file *file)
{
	return single_open(file, mm_speed_dump_read, NULL);
}

static const struct file_operations mm_fops = {
	.owner = THIS_MODULE,
	.open = proc_mm_open,
	.read = seq_read,
};

static int proc_venc_open(struct inode *inode, struct file *file)
{
	return single_open(file, venc_speed_dump_read, NULL);
}

static const struct file_operations venc_fops = {
	.owner = THIS_MODULE,
	.open = proc_venc_open,
	.read = seq_read,
};

static int proc_emi_open(struct inode *inode, struct file *file)
{
	return single_open(file, emi_speed_dump_read, NULL);
}

static const struct file_operations emi_fops = {
	.owner = THIS_MODULE,
	.open = proc_emi_open,
	.read = seq_read,
};

static int proc_bus_open(struct inode *inode, struct file *file)
{
	return single_open(file, bus_speed_dump_read, NULL);
}

static const struct file_operations bus_fops = {
	.owner = THIS_MODULE,
	.open = proc_bus_open,
	.read = seq_read,
};

#if 0
static int proc_mmclk_open(struct inode *inode, struct file *file)
{
	return single_open(file, mmclk_speed_dump_read, NULL);
}

static const struct file_operations mmclk_fops = {
	.owner = THIS_MODULE,
	.open = proc_mmclk_open,
	.read = seq_read,
};

static int proc_mfgclk_open(struct inode *inode, struct file *file)
{
	return single_open(file, mfgclk_speed_dump_read, NULL);
}

static const struct file_operations mfgclk_fops = {
	.owner = THIS_MODULE,
	.open = proc_mfgclk_open,
	.read = seq_read,
};
#endif


static int __init mt_power_management_init(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *pm_init_dir = NULL;
	/* unsigned int code = mt_get_chip_hw_code(); */

	pm_power_off = mt_power_off;

	/* cpu dormant driver init */
	mt_cpu_dormant_init();

#if 0
	/* SPM driver init*/
	spm_module_init();

	/* Sleep driver init (for suspend)*/
	if (0x321 == code)
		slp_module_init();
	else if (0x335 == code)
		slp_module_init();
	else if (0x337 == code)
		slp_module_init();
	/* other unknown chip ID, error !!*/
#endif

	spm_module_init();
#if 0 /* disable it for bring-up */
	slp_module_init();
#endif
	mt_clkmgr_init();
	mt_freqhopping_init();

	/* mt_pm_log_init(); // power management log init */

	/* mt_dcm_init(); // dynamic clock management init */


	pm_init_dir = proc_mkdir("pm_init", NULL);
	/* pm_init_dir = proc_mkdir("pm_init", NULL); */
	if (!pm_init_dir) {
		pr_debug("[%s]: mkdir /proc/pm_init failed\n", __func__);
	} else {
		entry = proc_create("cpu_speed_dump", S_IRUGO, pm_init_dir, &cpu_fops);
		entry = proc_create("mm_speed_dump", S_IRUGO | S_IWGRP, pm_init_dir, &mm_fops);
		entry = proc_create("venc_speed_dump", S_IRUGO | S_IWGRP, pm_init_dir, &venc_fops);
		entry = proc_create("emi_speed_dump", S_IRUGO | S_IWGRP, pm_init_dir, &emi_fops);

#ifdef TOPCK_LDVT
		entry =
		    proc_create("abist_meter_test", S_IRUGO | S_IWUSR, pm_init_dir,
				&abist_meter_fops);
		entry =
		    proc_create("ckgen_meter_test", S_IRUGO | S_IWUSR, pm_init_dir,
				&ckgen_meter_fops);
#endif
	}

#endif


	return 0;
}

arch_initcall(mt_power_management_init);


#if !defined(MT_DORMANT_UT)
static int __init mt_pm_late_init(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
/*	mt_idle_init(); */
	clk_buf_init();
#endif
	return 0;
}

late_initcall(mt_pm_late_init);
#endif				/* #if !defined (MT_DORMANT_UT) */


MODULE_DESCRIPTION("MTK Power Management Init Driver");
MODULE_LICENSE("GPL");
