/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <asm/mman.h>
#include <linux/dmapool.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <asm/types.h>
#include <linux/wait.h>
#include <linux/timer.h>

#define DRIVER_NAME "mtk_mdla"

#include <linux/interrupt.h>
#include "gsm.h"
#include "mdla_pmu.h"
#include <linux/spinlock.h>

#include "mdla_hw_reg.h"
#include "mdla_ioctl.h"
#include "mdla_ion.h"
#include "mdla_debug.h"
#ifndef MTK_MDLA_FPGA_PORTING
/*dvfs porting++*/
#define ENABLE_PMQOS

#include <linux/clk.h>
#include <linux/pm_qos.h>
#include <helio-dvfsrc-opp.h>
#include "mdla_dvfs.h"
#include "apu_dvfs.h"
#include <linux/regulator/consumer.h>
#include "vpu_dvfs.h"


#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source mdla_wake_lock[MTK_MDLA_CORE];
#else
struct wake_lock mdla_wake_lock[MTK_MDLA_CORE];
#endif


/* opp, mW */
struct MDLA_OPP_INFO mdla_power_table[MDLA_OPP_NUM] = {
	{MDLA_OPP_0, 336 * 4},
	{MDLA_OPP_1, 250 * 4},
	{MDLA_OPP_2, 221 * 4},
	{MDLA_OPP_3, 208 * 4},
	{MDLA_OPP_4, 140 * 4},
	{MDLA_OPP_5, 120 * 4},
	{MDLA_OPP_6, 114 * 4},
	{MDLA_OPP_7, 84 * 4},
	{MDLA_OPP_8, 336 * 4},
	{MDLA_OPP_9, 250 * 4},
	{MDLA_OPP_10, 221 * 4},
	{MDLA_OPP_11, 208 * 4},
	{MDLA_OPP_12, 140 * 4},
	{MDLA_OPP_13, 120 * 4},
	{MDLA_OPP_14, 114 * 4},
	{MDLA_OPP_15, 84 * 4},

};
#define CMD_WAIT_TIME_MS    (3 * 1000)
#define OPP_WAIT_TIME_MS    (300)
#define PWR_KEEP_TIME_MS    (500)
#define OPP_KEEP_TIME_MS    (500)
#define POWER_ON_MAGIC		(2)
#define OPPTYPE_VCORE		(0)
#define OPPTYPE_DSPFREQ		(1)
#define OPPTYPE_VVPU		(2)
#define OPPTYPE_VMDLA		(3)

/* clock */
static struct clk *clk_top_dsp_sel;
#if 0
static struct clk *clk_top_dsp1_sel;
static struct clk *clk_top_dsp2_sel;
#endif
static struct clk *clk_top_dsp3_sel;
static struct clk *clk_top_ipu_if_sel;
#if 0
static struct clk *clk_apu_core0_jtag_cg;
static struct clk *clk_apu_core0_axi_m_cg;
static struct clk *clk_apu_core0_apu_cg;
static struct clk *clk_apu_core1_jtag_cg;
static struct clk *clk_apu_core1_axi_m_cg;
static struct clk *clk_apu_core1_apu_cg;
#endif
static struct clk *clk_apu_mdla_apb_cg;
static struct clk *clk_apu_mdla_cg_b0;
static struct clk *clk_apu_mdla_cg_b1;
static struct clk *clk_apu_mdla_cg_b2;
static struct clk *clk_apu_mdla_cg_b3;
static struct clk *clk_apu_mdla_cg_b4;
static struct clk *clk_apu_mdla_cg_b5;
static struct clk *clk_apu_mdla_cg_b6;
static struct clk *clk_apu_mdla_cg_b7;
static struct clk *clk_apu_mdla_cg_b8;
static struct clk *clk_apu_mdla_cg_b9;
static struct clk *clk_apu_mdla_cg_b10;
static struct clk *clk_apu_mdla_cg_b11;
static struct clk *clk_apu_mdla_cg_b12;
static struct clk *clk_apu_conn_apu_cg;
static struct clk *clk_apu_conn_ahb_cg;
static struct clk *clk_apu_conn_axi_cg;
static struct clk *clk_apu_conn_isp_cg;
static struct clk *clk_apu_conn_cam_adl_cg;
static struct clk *clk_apu_conn_img_adl_cg;
static struct clk *clk_apu_conn_emi_26m_cg;
static struct clk *clk_apu_conn_vpu_udi_cg;
static struct clk *clk_apu_vcore_ahb_cg;
static struct clk *clk_apu_vcore_axi_cg;
static struct clk *clk_apu_vcore_adl_cg;
static struct clk *clk_apu_vcore_qos_cg;
static struct clk *clk_apu_vcore_qos_cg;
static struct clk *clk_top_clk26m;
static struct clk *clk_top_univpll_d3_d8;
static struct clk *clk_top_univpll_d3_d4;
static struct clk *clk_top_mainpll_d2_d4;
static struct clk *clk_top_univpll_d3_d2;
static struct clk *clk_top_mainpll_d2_d2;
static struct clk *clk_top_univpll_d2_d2;
static struct clk *clk_top_mainpll_d3;
static struct clk *clk_top_univpll_d3;
static struct clk *clk_top_mmpll_d7;
static struct clk *clk_top_mmpll_d6;
static struct clk *clk_top_adsppll_d5;
static struct clk *clk_top_tvdpll_ck;
static struct clk *clk_top_tvdpll_mainpll_d2_ck;
static struct clk *clk_top_univpll_d2;
static struct clk *clk_top_adsppll_d4;
static struct clk *clk_top_mainpll_d2;
static struct clk *clk_top_mmpll_d4;



/* mtcmos */
static struct clk *mtcmos_dis;

//static struct clk *mtcmos_vpu_vcore_dormant;
static struct clk *mtcmos_vpu_vcore_shutdown;
//static struct clk *mtcmos_vpu_conn_dormant;
static struct clk *mtcmos_vpu_conn_shutdown;
#if 0
static struct clk *mtcmos_vpu_core0_dormant;
static struct clk *mtcmos_vpu_core0_shutdown;
static struct clk *mtcmos_vpu_core1_dormant;
static struct clk *mtcmos_vpu_core1_shutdown;
#endif
//static struct clk *mtcmos_vpu_core2_dormant;
static struct clk *mtcmos_vpu_core2_shutdown;

/* smi */
static struct clk *clk_mmsys_gals_ipu2mm;
static struct clk *clk_mmsys_gals_ipu12mm;
static struct clk *clk_mmsys_gals_comm0;
static struct clk *clk_mmsys_gals_comm1;
static struct clk *clk_mmsys_smi_common;
#if 0
/*direct link*/
static struct clk *clk_mmsys_ipu_dl_txck;
static struct clk *clk_mmsys_ipu_dl_rx_ck;
#endif
/* workqueue */
struct my_struct_t {
	int core;
	struct delayed_work my_work;
};
static struct workqueue_struct *wq;
static void mdla_power_counter_routine(struct work_struct *);
static struct my_struct_t power_counter_work[MTK_MDLA_CORE];

/* static struct workqueue_struct *opp_wq; */
static void mdla_opp_keep_routine(struct work_struct *);
static DECLARE_DELAYED_WORK(opp_keep_work, mdla_opp_keep_routine);


/* power */
static struct mutex power_mutex[MTK_MDLA_CORE];
static bool is_power_on[MTK_MDLA_CORE];
static bool is_power_debug_lock;
static struct mutex power_counter_mutex[MTK_MDLA_CORE];
static int power_counter[MTK_MDLA_CORE];
static struct mutex opp_mutex;
static bool force_change_vcore_opp[MTK_MDLA_CORE];
static bool force_change_vvpu_opp[MTK_MDLA_CORE];
static bool force_change_vmdla_opp[MTK_MDLA_CORE];
static bool force_change_dsp_freq[MTK_MDLA_CORE];
static bool force_change_dsp_if_freq[MTK_MDLA_CORE];
static bool change_freq_first[MTK_MDLA_CORE];
static bool opp_keep_flag;
static uint8_t max_vcore_opp;
//static uint8_t max_vvpu_opp;
static uint8_t max_vmdla_opp;
static uint8_t max_dsp_freq;

/* dvfs */
static struct mdla_dvfs_opps opps;
#ifdef ENABLE_PMQOS
static struct pm_qos_request mdla_qos_bw_request[MTK_MDLA_CORE];
static struct pm_qos_request mdla_qos_vcore_request[MTK_MDLA_CORE];
//static struct pm_qos_request mdla_qos_vvpu_request[MTK_MDLA_CORE];
static struct pm_qos_request mdla_qos_vmdla_request[MTK_MDLA_CORE];
#endif

/*regulator id*/
static struct regulator *vvpu_reg_id;
static struct regulator *vmdla_reg_id;


/*dvfs porting--*/
#endif
#define  DEVICE_NAME "mdlactl"
#define  CLASS_NAME  "mdla"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen, Wen");
MODULE_DESCRIPTION("MDLA driver");
MODULE_VERSION("0.1");

static void *apu_mdla_cmde_mreg_top;
static void *apu_mdla_config_top;
void *apu_mdla_biu_top;
void *apu_mdla_gsm_top;

int mdla_irq;

/* TODO: move these global control vaiables into device specific data.
 * to support multiple instance (multiple MDLA).
 */
static int majorNumber;
static int numberOpens;
static struct class *mdlactlClass;
static struct device *mdlactlDevice;
static struct platform_device *mdlactlPlatformDevice;
static u32 cmd_id;
static u32 max_cmd_id;
static u32 cmd_list_len;
struct work_struct mdla_queue;

#define UINT32_MAX (0xFFFFFFFF)
static u32 last_cmd_id0 = UINT32_MAX;
static u32 last_cmd_id1 = UINT32_MAX;
static u32 fail_cmd_id0 = UINT32_MAX;
static u32 fail_cmd_id1 = UINT32_MAX;

DEFINE_MUTEX(cmd_list_lock);
static DECLARE_WAIT_QUEUE_HEAD(mdla_cmd_queue);
static LIST_HEAD(cmd_list);

struct mtk_mdla_local {
	int irq;
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};
struct command_entry {
	struct list_head list;
	void *kva;    /* Virtual Address for Kernel */
	u32 mva;      /* Physical Address for Device */
	u32 count;
	u32 id;
	u64 khandle;  /* ion kernel handle */
	u8 type;      /* allocate memory type */
};

static int mdla_open(struct inode *, struct file *);
static int mdla_release(struct inode *, struct file *);
static long mdla_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int mdla_wait_command(u32 id);
static int mdla_mmap(struct file *filp, struct vm_area_struct *vma);
static int mdla_process_command(struct command_entry *run_cmd_data);
static void mdla_timeup(unsigned long data);

#define MDLA_TIMEOUT_DEFAULT 1000 /* ms */
u32 mdla_timeout = MDLA_TIMEOUT_DEFAULT;
static DEFINE_TIMER(mdla_timer, mdla_timeup, 0, 0);

static const struct file_operations fops = {
	.open = mdla_open,
	.unlocked_ioctl = mdla_ioctl,
	.mmap = mdla_mmap,
	.release = mdla_release,
};

unsigned int mdla_cfg_read(u32 offset)
{
	return ioread32(apu_mdla_config_top + offset);
}

static void mdla_cfg_write(u32 value, u32 offset)
{
	iowrite32(value, apu_mdla_config_top + offset);
}

unsigned int mdla_reg_read(u32 offset)
{
	return ioread32(apu_mdla_cmde_mreg_top + offset);
}

static void mdla_reg_write(u32 value, u32 offset)
{
	iowrite32(value, apu_mdla_cmde_mreg_top + offset);
}

static void mdla_reset(void)
{
	mdla_debug("%s: MDLA RESET\n", __func__);  // TODO: remove debug
	mdla_cfg_write(0x03f, MDLA_SW_RST);
	mdla_cfg_write(0x000, MDLA_SW_RST);
	mdla_cfg_write(0xffffffff, MDLA_CG_CLR);
	mdla_reg_write(MDLA_IRQ_MASK & ~(MDLA_IRQ_SWCMD_DONE),
		MREG_TOP_G_INTP2);
}

static void mdla_timeup(unsigned long data)
{

	// TODO: remove debug
	mdla_debug("%s: MDLA Timeout: %u ms, max_cmd_id: %u, cmd_id0: %u, cmd_id1: %u\n",
		__func__, mdla_timeout, max_cmd_id, last_cmd_id0, last_cmd_id1);
	mdla_dump_reg();
	mdla_reset();

	fail_cmd_id0 = last_cmd_id0;
	fail_cmd_id1 = last_cmd_id1;

	if (last_cmd_id1 == UINT32_MAX)
		max_cmd_id = last_cmd_id0;
	else
		max_cmd_id = last_cmd_id1;

	/* wake up those who were waiting on id0 or id1. */
	wake_up_interruptible_all(&mdla_cmd_queue);
	/* resume execution of the command queue */
	schedule_work(&mdla_queue);
}

static int mdla_mmap(struct file *filp, struct vm_area_struct *vma)
{

	unsigned long offset = vma->vm_pgoff;
	unsigned long size = vma->vm_end - vma->vm_start;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, offset, size,
			vma->vm_page_prot)) {
		pr_info("%s: remap_pfn_range error: %p\n",
			__func__, vma);
		return -EAGAIN;
	}
	return 0;
}

static void mdla_start_queue(struct work_struct *work)
{
	struct command_entry *run_cmd_data;

	mutex_lock(&cmd_list_lock);

	while (max_cmd_id >= last_cmd_id0) {
		cmd_list_len--;
		last_cmd_id0 = last_cmd_id1;
		last_cmd_id1 = UINT32_MAX;
		if (!list_empty(&cmd_list)) {
			run_cmd_data = list_first_entry(&cmd_list,
					struct command_entry, list);
			list_del(&run_cmd_data->list);
			last_cmd_id1 = run_cmd_data->id + run_cmd_data->count
					- 1;
			mdla_process_command(run_cmd_data);
			kfree(run_cmd_data);
		}
	}
	if ((cmd_list_len == 0) && timer_pending(&mdla_timer)) {
		// TODO: remove debug
		mdla_debug("%s: del_timer().\n", __func__);
		del_timer(&mdla_timer);
	}
	mutex_unlock(&cmd_list_lock);
	wake_up_interruptible_all(&mdla_cmd_queue);
	mdla_debug(
			"mdla_interrupt max_cmd_id: [%d] last_cmd_id0: [%d] last_cmd_id1: [%d]\n",
			max_cmd_id, last_cmd_id0, last_cmd_id1);
}

static irqreturn_t mdla_interrupt(int irq, void *dev_id)
{
	max_cmd_id = mdla_reg_read(MREG_TOP_G_FIN0);
	mdla_reg_write(MDLA_IRQ_MASK, MREG_TOP_G_INTP0);
	schedule_work(&mdla_queue);
	return IRQ_HANDLED;
}
#ifndef MTK_MDLA_FPGA_PORTING
/*dvfs porting ++*/
static inline int Map_MDLA_Freq_Table(int freq_opp)
{
	int freq_value = 0;

	switch (freq_opp) {
	case 0:
	default:
		freq_value = 788;
		break;
	case 1:
		freq_value = 700;
		break;
	case 2:
		freq_value = 624;
		break;
	case 3:
		freq_value = 606;
		break;
	case 4:
		freq_value = 594;
		break;
	case 5:
		freq_value = 546;
		break;
	case 6:
		freq_value = 525;
		break;
	case 7:
		freq_value = 450;
		break;
	case 8:
		freq_value = 416;
		break;
	case 9:
		freq_value = 364;
		break;
	case 10:
		freq_value = 312;
		break;
	case 11:
		freq_value = 273;
		break;
	case 12:
		freq_value = 208;
		break;
	case 13:
		freq_value = 137;
		break;
	case 14:
		freq_value = 52;
		break;
	case 15:
		freq_value = 26;
		break;
	}

	return freq_value;
}

static int mdla_set_clock_source(struct clk *clk, uint8_t step)
{
	struct clk *clk_src;

	LOG_DBG("mdla scc(%d)", step);
	/* set dsp frequency - 0:788 MHz, 1:700 MHz, 2:606 MHz, 3:594 MHz*/
	/* set dsp frequency - 4:560 MHz, 5:525 MHz, 6:450 MHz, 7:416 MHz*/
	/* set dsp frequency - 8:364 MHz, 9:312 MHz, 10:273 MH, 11:208 MH*/
	/* set dsp frequency - 12:137 MHz, 13:104 MHz, 14:52 MHz, 15:26 MHz*/

	switch (step) {
	case 0:
		clk_src = clk_top_mmpll_d4;
		break;
	case 1:
		clk_src = clk_top_adsppll_d4;
		break;
	case 2:
		clk_src = clk_top_univpll_d2;
		break;
	case 3:
		clk_src = clk_top_tvdpll_mainpll_d2_ck;
		break;
	case 4:
		clk_src = clk_top_tvdpll_ck;
		break;
	case 5:
		clk_src = clk_top_mainpll_d2;
		break;
	case 6:
		clk_src = clk_top_mmpll_d6;
		break;
	case 7:
		clk_src = clk_top_mmpll_d7;
		break;
	case 8:
		clk_src = clk_top_univpll_d3;
		break;
	case 9:
		clk_src = clk_top_mainpll_d3;
		break;
	case 10:
		clk_src = clk_top_univpll_d2_d2;
		break;
	case 11:
		clk_src = clk_top_mainpll_d2_d2;
		break;
	case 12:
		clk_src = clk_top_univpll_d3_d2;
		break;
	case 13:
		clk_src = clk_top_mainpll_d2_d4;
		break;
	case 14:
		clk_src = clk_top_univpll_d3_d8;
		break;
	case 15:
		clk_src = clk_top_clk26m;
		break;
	default:
		LOG_ERR("wrong freq step(%d)", step);
		return -EINVAL;
	}

	return clk_set_parent(clk, clk_src);
}
/*set CONN hf_fdsp_ck, VCORE hf_fipu_if_ck*/
static int mdla_if_set_clock_source(struct clk *clk, uint8_t step)
{
	struct clk *clk_src;

	LOG_DBG("mdla scc(%d)", step);
	/* set dsp frequency - 0:624 MHz, 1:624 MHz, 2:606 MHz, 3:594 MHz*/
	/* set dsp frequency - 4:560 MHz, 5:525 MHz, 6:450 MHz, 7:416 MHz*/
	/* set dsp frequency - 8:364 MHz, 9:312 MHz, 10:273 MH, 11:208 MH*/
	/* set dsp frequency - 12:137 MHz, 13:104 MHz, 14:52 MHz, 15:26 MHz*/
	switch (step) {
	case 0:
		clk_src = clk_top_univpll_d2;/*624MHz*/
		break;
	case 1:
		clk_src = clk_top_univpll_d2;/*624MHz*/
		break;
	case 2:
		clk_src = clk_top_tvdpll_mainpll_d2_ck;
		break;
	case 3:
		clk_src = clk_top_tvdpll_ck;
		break;
	case 4:
		clk_src = clk_top_adsppll_d5;
		break;
	case 5:
		clk_src = clk_top_mmpll_d6;
		break;
	case 6:
		clk_src = clk_top_mmpll_d7;
		break;
	case 7:
		clk_src = clk_top_univpll_d3;
		break;
	case 8:
		clk_src = clk_top_mainpll_d3;
		break;
	case 9:
		clk_src = clk_top_univpll_d2_d2;
		break;
	case 10:
		clk_src = clk_top_mainpll_d2_d2;
		break;
	case 11:
		clk_src = clk_top_univpll_d3_d2;
		break;
	case 12:
		clk_src = clk_top_mainpll_d2_d4;
		break;
	case 13:
		clk_src = clk_top_univpll_d3_d4;
		break;
	case 14:
		clk_src = clk_top_univpll_d3_d8;
		break;
	case 15:
		clk_src = clk_top_clk26m;
		break;
	default:
		LOG_ERR("wrong freq step(%d)", step);
		return -EINVAL;
	}

	return clk_set_parent(clk, clk_src);
}


#if 0
static int mdla_get_hw_vcore_opp(int core)
{
#ifdef MTK_MDLA_FPGA_PORTING
	LOG_DBG("[mdla_%d] FPGA always return 0\n", core);

	return 0;
#else
	int opp_value = 0;
	int get_vcore_value = 0;

	get_vcore_value = (int)vcorefs_get_curr_vcore();
	if (get_vcore_value >= 800000)
		opp_value = 0;
	else
		opp_value = 1;

	LOG_DBG("[mdla_%d] vcore(%d->%d)\n", core, get_vcore_value, opp_value);

	return opp_value;
#endif
}
#endif

int mdla_get_opp(void)
{
	LOG_DBG("[mdla_%d] vvpu(%d->%d)\n", core, get_vvpu_value, opp_value);
	return opps.mdlacore.index;
}

static int mdla_get_hw_vvpu_opp(int core)
{
	int opp_value = 0;
	int get_vvpu_value = 0;

	get_vvpu_value = (int)regulator_get_voltage(vvpu_reg_id);
	if (get_vvpu_value >= 825000)
		opp_value = 0;
	else if (get_vvpu_value > 725000)
		opp_value = 0;
	else if (get_vvpu_value > 650000)
		opp_value = 1;
	else
		opp_value = 2;

	LOG_DBG("[mdla_%d] vvpu(%d->%d)\n", core, get_vvpu_value, opp_value);

	return opp_value;
}

static int mdla_get_hw_vmdla_opp(int core)
{
	int opp_value = 0;
	int get_vmdla_value = 0;

	get_vmdla_value = (int)regulator_get_voltage(vmdla_reg_id);
	if (get_vmdla_value >= 825000)
		opp_value = 0;
	else if (get_vmdla_value > 725000)
		opp_value = 0;
	else if (get_vmdla_value > 650000)
		opp_value = 1;
	else
		opp_value = 2;

	LOG_DBG("[mdla_%d] vmdla(%d->%d)\n", core, get_vmdla_value, opp_value);

	return opp_value;
}

static void mdla_dsp_if_freq_check(int core, uint8_t vmdla_index)
{
	uint8_t vvpu_opp;

	vvpu_opp = get_vpu_opp();
	switch (vmdla_index) {
	case 0:
		opps.dsp.index = 0;
		opps.ipu_if.index = 0;
		break;
	case 1:
		opps.dsp.index = 0;
		opps.ipu_if.index = 0;
		break;
	case 2:
		opps.dsp.index = 0;
		opps.ipu_if.index = 0;
		break;
	case 3:
	case 4:
	case 5:
	case 6:
		opps.dsp.index = 5;
		opps.ipu_if.index = 5;
		if (vvpu_opp >= 5) {
			/*dsp.index,ipu_if.index would not be changed*/
			force_change_dsp_if_freq[core] = false;
		} else {
		/*dsp.index,ipu_if.index need to be changed*/
			force_change_dsp_if_freq[core] = true;
		}
		break;
	case 7:
		opps.dsp.index = 6;
		opps.ipu_if.index = 6;
		if (vvpu_opp >= 6)
			force_change_dsp_if_freq[core] = false;
		else
			force_change_dsp_if_freq[core] = true;
		break;
	case 8:
		opps.dsp.index = 7;
		opps.ipu_if.index = 7;
		if (vvpu_opp >= 7)
			force_change_dsp_if_freq[core] = false;
		else
			force_change_dsp_if_freq[core] = true;
		break;
	case 9:
		opps.dsp.index = 9;
		opps.ipu_if.index = 9;
		if (vvpu_opp >= 9)
			force_change_dsp_if_freq[core] = false;
		else
			force_change_dsp_if_freq[core] = true;
		break;
	case 10:
		opps.dsp.index = 10;
		opps.ipu_if.index = 10;
		if (vvpu_opp >= 10)
			force_change_dsp_if_freq[core] = false;
		else
			force_change_dsp_if_freq[core] = true;
		break;
	case 11:
		opps.dsp.index = 11;
		opps.ipu_if.index = 11;
		if (vvpu_opp >= 11)
			force_change_dsp_if_freq[core] = false;
		else
			force_change_dsp_if_freq[core] = true;

		break;
	case 12:
		opps.dsp.index = 12;
		opps.ipu_if.index = 12;
		if (vvpu_opp >= 12)
			force_change_dsp_if_freq[core] = false;
		else
			force_change_dsp_if_freq[core] = true;
		break;
	case 13:
		opps.dsp.index = 13;
		opps.ipu_if.index = 13;
		if (vvpu_opp >= 13)
			force_change_dsp_if_freq[core] = false;
		else
			force_change_dsp_if_freq[core] = true;
		break;
	case 14:
		opps.dsp.index = 14;
		opps.ipu_if.index = 14;
		if (vvpu_opp >= 14)
			force_change_dsp_if_freq[core] = false;
		else
			force_change_dsp_if_freq[core] = true;
		break;
	case 15:
		opps.dsp.index = 15;
		opps.ipu_if.index = 15;
		if (vvpu_opp >= 15)
			force_change_dsp_if_freq[core] = false;
		else
			force_change_dsp_if_freq[core] = true;
		break;
	default:
		LOG_ERR("wrong vmdla_index(%d)", vmdla_index);
	}


}


/* expected range, vmdla_index: 0~15 */
/* expected range, freq_index: 0~15 */
//static void mdla_opp_check(int core, uint8_t vcore_index, uint8_t freq_index)
static void mdla_opp_check(int core, uint8_t vmdla_index, uint8_t freq_index)
{
	int i = 0;
	bool freq_check = false;
	int log_freq = 0, log_max_freq = 0;
	//int get_vcore_opp = 0;
	//int get_vvpu_opp = 0;
	int get_vmdla_opp = 0;

	if (is_power_debug_lock) {
		force_change_vcore_opp[core] = false;
		force_change_vvpu_opp[core] = false;
		force_change_vmdla_opp[core] = false;
		force_change_dsp_freq[core] = false;
		force_change_dsp_if_freq[core] = false;
		goto out;
	}

	log_freq = Map_MDLA_Freq_Table(freq_index);

	LOG_DBG("opp_check + (%d/%d/%d), ori vmdla(%d)\n", core,
			vmdla_index, freq_index, opps.vmdla.index);

	mutex_lock(&opp_mutex);
	change_freq_first[core] = false;
	log_max_freq = Map_MDLA_Freq_Table(max_dsp_freq);
	/* vcore opp */
	//get_vcore_opp = mdla_get_hw_vcore_opp(core);
	/* vmdla opp*/
	get_vmdla_opp = mdla_get_hw_vmdla_opp(core);
#if 0
	if ((vcore_index == 0xFF) || (vcore_index == get_vcore_opp)) {

		LOG_DBG("no need, vcore opp(%d), hw vore opp(%d)\n",
				vcore_index, get_vcore_opp);

		force_change_vcore_opp[core] = false;
		opps.vcore.index = vcore_index;
	} else {
		/* opp down, need change freq first*/
		if (vcore_index > get_vcore_opp)
			change_freq_first[core] = true;

		if (vcore_index < max_vcore_opp) {
			LOG_INF("mdla bound vcore opp(%d) to %d",
					vcore_index, max_vcore_opp);

			vcore_index = max_vcore_opp;
		}

		if (vcore_index >= opps.count) {
			LOG_ERR("wrong vcore opp(%d), max(%d)",
					vcore_index, opps.count - 1);

		} else if ((vcore_index < opps.vcore.index) ||
				((vcore_index > opps.vcore.index) &&
					(!opp_keep_flag))) {
			opps.vcore.index = vcore_index;
			force_change_vcore_opp[core] = true;
			freq_check = true;
		}
	}
#else
if ((vmdla_index == 0xFF) || (vmdla_index == get_vmdla_opp)) {

	LOG_DBG("no need, vmdla opp(%d), hw vore opp(%d)\n",
			vmdla_index, get_vmdla_opp);

	force_change_vmdla_opp[core] = false;
	opps.vmdla.index = vmdla_index;
} else {
	/* opp down, need change freq first*/
	if (vmdla_index > get_vmdla_opp)
		change_freq_first[core] = true;

	if (vmdla_index < max_vmdla_opp) {
		LOG_INF("mdla bound vmdla opp(%d) to %d",
				vmdla_index, max_vmdla_opp);

		vmdla_index = max_vmdla_opp;
	}

	if (vmdla_index >= opps.count) {
		LOG_ERR("wrong vmdla opp(%d), max(%d)",
				vmdla_index, opps.count - 1);

	} else if ((vmdla_index < opps.vmdla.index) ||
			((vmdla_index > opps.vmdla.index) &&
				(!opp_keep_flag))) {
		opps.vmdla.index = vmdla_index;
		force_change_vmdla_opp[core] = true;
		freq_check = true;
	}
}

#endif
	/* dsp freq opp */
	if (freq_index == 0xFF) {
		LOG_DBG("no request, freq opp(%d)", freq_index);
		force_change_dsp_freq[core] = false;
		force_change_dsp_if_freq[core] = false;
	} else {
		if (freq_index < max_dsp_freq) {
			LOG_INF("mdla bound dsp freq(%dMHz) to %dMHz",
					log_freq, log_max_freq);
			freq_index = max_dsp_freq;
		}

		if ((opps.mdlacore.index != freq_index) || (freq_check)) {
			/* freq_check for all vcore adjust related operation
			 * in acceptable region
			 */

			/* vcore not change and dsp change */
			//if ((force_change_vcore_opp[core] == false) &&
			if ((force_change_vmdla_opp[core] == false) &&
				(freq_index > opps.mdlacore.index) &&
				(opp_keep_flag)) {
				force_change_dsp_freq[core] = false;
					LOG_INF("%s(%d) %s (%d/%d_%d/%d)\n",
						__func__,
						core,
						"dsp keep high",
						force_change_vmdla_opp[core],
						freq_index,
						opps.mdlacore.index,
						opp_keep_flag);
			} else {
				opps.mdlacore.index = freq_index;
					/*To FIX*/
				if (opps.vmdla.index == 1 &&
						opps.mdlacore.index < 3) {
					/* adjust 0~3 to 4~7 for real table
					 * if needed
					 */
					opps.mdlacore.index = 3;
				}
				if (opps.vmdla.index == 2 &&
						opps.mdlacore.index < 9) {
					/* adjust 0~3 to 4~7 for real table
					 * if needed
					 */
					opps.mdlacore.index = 9;
				}

				opps.dsp.index = 15;
				opps.ipu_if.index = 15;
				for (i = 0 ; i < MTK_MDLA_CORE ; i++) {
					LOG_DBG("%s %s[%d].%s(%d->%d)\n",
						__func__,
						"opps.mdlacore",
						core,
						"index",
						opps.mdlacore.index,
						opps.dsp.index);

					/* interface should be the max freq of
					 * mdla cores
					 */
					if ((opps.mdlacore.index <
						opps.dsp.index) &&
						(opps.mdlacore.index >=
						max_dsp_freq)) {
#if 0
						opps.dsp.index =
							opps.mdlacore.index;

						opps.ipu_if.index =
							opps.mdlacore.index;
#endif
			mdla_dsp_if_freq_check(core, opps.mdlacore.index);
					}
				}
				force_change_dsp_freq[core] = true;

				opp_keep_flag = true;
				mod_delayed_work(wq, &opp_keep_work,
					msecs_to_jiffies(OPP_KEEP_TIME_MS));
			}
		} else {
			/* vcore not change & dsp not change */
				LOG_INF("opp_check(%d) vcore/dsp no change\n",
						core);

			opp_keep_flag = true;

			mod_delayed_work(wq, &opp_keep_work,
					msecs_to_jiffies(OPP_KEEP_TIME_MS));
		}
	}
	mutex_unlock(&opp_mutex);
out:
	LOG_INF("%s(%d)(%d/%d_%d)(%d/%d)(%d.%d.%d)(%d/%d)(%d/%d/%d/%d/%d)%d\n",
		"opp_check",
		core,
		is_power_debug_lock,
		vmdla_index,
		freq_index,
		opps.vmdla.index,
		get_vmdla_opp,
		opps.dsp.index,
		opps.mdlacore.index,
		opps.ipu_if.index,
		max_vmdla_opp,
		max_dsp_freq,
		freq_check,
		force_change_vmdla_opp[core],
		force_change_dsp_freq[core],
		force_change_dsp_if_freq[core],
		change_freq_first[core],
		opp_keep_flag);
}

static bool mdla_change_opp(int core, int type)
{
#ifdef MTK_MDLA_FPGA_PORTING
	LOG_INF("[mdla_%d] %d Skip at FPGA", core, type);

	return true;
#else
	int ret;


	switch (type) {
	/* vcore opp */
	case OPPTYPE_VCORE:
		LOG_INF("[mdla_%d] wait for changing vcore opp", core);
#if 0
		ret = wait_to_do_change_vcore_opp(core);
		if (ret) {
			LOG_ERR("[mdla_%d] timeout to %s, ret=%d\n",
				core,
				"wait_to_do_change_vcore_opp",
				ret);
			goto out;
		}
		LOG_DBG("[mdla_%d] to do vcore opp change", core);
		mutex_lock(&(mdla_service_cores[core].state_mutex));
		mdla_service_cores[core].state = VCT_VCORE_CHG;
		mutex_unlock(&(mdla_service_cores[core].state_mutex));
		mutex_lock(&opp_mutex);
		mdla_trace_begin("vcore:request");
		#ifdef ENABLE_PMQOS
		switch (opps.vcore.index) {
		case 0:
			pm_qos_update_request(&mdla_qos_vcore_request[core],
								VCORE_OPP_0);
			break;
		case 1:
			pm_qos_update_request(&mdla_qos_vcore_request[core],
								VCORE_OPP_1);
			break;
		case 2:
		default:
			pm_qos_update_request(&mdla_qos_vcore_request[core],
								VCORE_OPP_2);
			break;
		}
		#else
		ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL,
							opps.vcore.index);
		#endif
		mdla_trace_end();
		mutex_lock(&(mdla_service_cores[core].state_mutex));
		mdla_service_cores[core].state = VCT_BOOTUP;
		mutex_unlock(&(mdla_service_cores[core].state_mutex));
		if (ret) {
			LOG_ERR("[mdla_%d]fail to request vcore, step=%d\n",
					core, opps.vcore.index);
			goto out;
		}

		LOG_INF("[mdla_%d] cgopp vmdla=%d\n",
				core,
				regulator_get_voltage(vmdla_reg_id));

		force_change_vcore_opp[core] = false;
		mutex_unlock(&opp_mutex);
		//wake_up_interruptible(&waitq_do_core_executing);
#endif
		break;
	/* dsp freq opp */
	case OPPTYPE_DSPFREQ:
		mutex_lock(&opp_mutex);
		LOG_INF("[mdla] %s setclksrc(%d/%d/%d)\n",
				__func__,
				opps.dsp.index,
				opps.mdlacore.index,
				opps.ipu_if.index);
		if (force_change_dsp_if_freq[core]) {

		ret = mdla_if_set_clock_source(clk_top_dsp_sel, opps.dsp.index);
		if (ret) {
			LOG_ERR("[mdla]fail to set dsp freq, %s=%d, %s=%d\n",
				"step", opps.dsp.index,
				"ret", ret);
			goto out;
		}
		ret = mdla_if_set_clock_source(clk_top_ipu_if_sel,
							opps.ipu_if.index);
		if (ret) {
			LOG_ERR("[mdla]%s, %s=%d, %s=%d\n",
					"fail to set ipu_if freq",
					"step", opps.ipu_if.index,
					"ret", ret);
			goto out;
		}
			}
			ret = mdla_set_clock_source(clk_top_dsp3_sel,
						opps.mdlacore.index);
			if (ret) {
				LOG_ERR("[mdla]%s%d freq, %s=%d, %s=%d\n",
					"fail to set dsp_",
					"step", opps.mdlacore.index,
					"ret", ret);
				goto out;
			}



		force_change_dsp_freq[core] = false;
		force_change_dsp_if_freq[core] = false;
		mutex_unlock(&opp_mutex);
		break;
	/* vmdla opp */
	case OPPTYPE_VMDLA:
		LOG_INF("[mdla_%d] wait for changing vmdla opp", core);
#if 0
		ret = wait_to_do_change_vcore_opp(core);
		if (ret) {
			LOG_ERR("[mdla_%d] timeout to %s, ret=%d\n",
				core,
				"wait_to_do_change_vcore_opp",
				ret);
			goto out;
		}
#endif
		LOG_DBG("[mdla_%d] to do vmdla opp change", core);
#if 0
		mutex_lock(&(mdla_service_cores[core].state_mutex));
		mdla_service_cores[core].state = VCT_VCORE_CHG;
		mutex_unlock(&(mdla_service_cores[core].state_mutex));
#endif
		mutex_lock(&opp_mutex);
		mdla_trace_begin("vcore:request");
		#ifdef ENABLE_PMQOS
		switch (opps.vmdla.index) {
		case 0:
			pm_qos_update_request(&mdla_qos_vmdla_request[core],
								VMDLA_OPP_0);
			pm_qos_update_request(&mdla_qos_vcore_request[core],
								VCORE_OPP_0);
			break;
		case 1:
			pm_qos_update_request(&mdla_qos_vmdla_request[core],
								VMDLA_OPP_1);
			pm_qos_update_request(&mdla_qos_vcore_request[core],
								VCORE_OPP_1);
			break;
		case 2:
		default:
			pm_qos_update_request(&mdla_qos_vmdla_request[core],
								VMDLA_OPP_2);
			pm_qos_update_request(&mdla_qos_vcore_request[core],
								VCORE_OPP_2);
			break;
		}
		#else
		ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL,
							opps.vcore.index);
		#endif
		mdla_trace_end();
#if 0
		mutex_lock(&(mdla_service_cores[core].state_mutex));
		mdla_service_cores[core].state = VCT_BOOTUP;
		mutex_unlock(&(mdla_service_cores[core].state_mutex));
#endif
		if (ret) {
			LOG_ERR("[mdla_%d]fail to request vcore, step=%d\n",
					core, opps.vcore.index);
			goto out;
		}

		LOG_INF("[mdla_%d] cgopp vmdla=%d\n",
				core,
				regulator_get_voltage(vmdla_reg_id));

		force_change_vcore_opp[core] = false;
		force_change_vvpu_opp[core] = false;
		force_change_vmdla_opp[core] = false;
		mutex_unlock(&opp_mutex);
		//wake_up_interruptible(&waitq_do_core_executing);
		break;
	default:
		LOG_INF("unexpected type(%d)", type);
		break;
	}

out:
	return true;
#endif
}

int32_t mdla_thermal_en_throttle_cb(uint8_t vcore_opp, uint8_t mdla_opp)
{
	int i = 0;
	int ret = 0;
	int vmdla_opp_index = 0;
	int mdla_freq_index = 0;

	#if 0
	bool mdla_down = true;

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		mutex_lock(&power_counter_mutex[i]);
		if (power_counter[i] > 0)
			vpu_down = false;
		mutex_unlock(&power_counter_mutex[i]);
	}
	if (vpu_down) {
		LOG_INF("[vpu] all vpu are off currently, do nothing\n");
		return ret;
	}
	#endif

	#if 0
	if ((int)vpu_opp < 4) {
		vcore_opp_index = 0;
		vpu_freq_index = vpu_opp;
	} else {
		vcore_opp_index = 1;
		vpu_freq_index = vpu_opp;
	}
	#else
	if (mdla_opp < MDLA_MAX_NUM_OPPS) {
		vmdla_opp_index = opps.vmdla.opp_map[mdla_opp];
		mdla_freq_index = opps.dsp.opp_map[mdla_opp];
	} else {
		LOG_ERR("mdla_thermal_en wrong opp(%d)\n", mdla_opp);
		return -1;
	}
	#endif
	LOG_INF("%s, opp(%d)->(%d/%d)\n",
	__func__, mdla_opp, vmdla_opp_index, mdla_freq_index);

	mutex_lock(&opp_mutex);
	max_dsp_freq = mdla_freq_index;
	max_vmdla_opp = vmdla_opp_index;
	mutex_unlock(&opp_mutex);
	for (i = 0 ; i < MTK_MDLA_CORE ; i++) {
		mutex_lock(&opp_mutex);

		/* force change for all core under thermal request */
		opp_keep_flag = false;

		mutex_unlock(&opp_mutex);
		mdla_opp_check(i, vmdla_opp_index, mdla_freq_index);
	}

	for (i = 0 ; i < MTK_MDLA_CORE ; i++) {
		if (force_change_dsp_freq[i]) {
			/* force change freq while running */
			switch (mdla_freq_index) {
			case 0:
			default:
				LOG_INF("thermal force bound freq @788MHz\n");
				break;
			case 1:
				LOG_INF("thermal force bound freq @ 700MHz\n");
				break;
			case 2:
				LOG_INF("thermal force bound freq @624MHz\n");
				break;
			case 3:
				LOG_INF("thermal force bound freq @606MHz\n");
				break;
			case 4:
				LOG_INF("thermal force bound freq @594MHz\n");
				break;
			case 5:
				LOG_INF("thermal force bound freq @546MHz\n");
				break;
			case 6:
				LOG_INF("thermal force bound freq @525MHz\n");
				break;
			case 7:
				LOG_INF("thermal force bound freq @450MHz\n");
				break;
			case 8:
				LOG_INF("thermal force bound freq @416MHz\n");
				break;
			case 9:
				LOG_INF("thermal force bound freq @364MHz\n");
				break;
			case 10:
				LOG_INF("thermal force bound freq @312MHz\n");
				break;
			case 11:
				LOG_INF("thermal force bound freq @273MHz\n");
				break;
			case 12:
				LOG_INF("thermal force bound freq @208MHz\n");
				break;
			case 13:
				LOG_INF("thermal force bound freq @137MHz\n");
				break;
			case 14:
				LOG_INF("thermal force bound freq @52MHz\n");
				break;
			case 15:
				LOG_INF("thermal force bound freq @26MHz\n");
				break;
			}
			/*mdla_change_opp(i, OPPTYPE_DSPFREQ);*/
		}
		if (force_change_vmdla_opp[i]) {
			/* vcore change should wait */
			LOG_INF("thermal force bound vcore opp to %d\n",
					vmdla_opp_index);
			/* vcore only need to change one time from
			 * thermal request
			 */
			/*if (i == 0)*/
			/*	mdla_change_opp(i, OPPTYPE_VCORE);*/
		}
	}

	return ret;
}

int32_t mdla_thermal_dis_throttle_cb(void)
{
	int ret = 0;

	LOG_INF("%s +\n", __func__);
	mutex_lock(&opp_mutex);
	max_vcore_opp = 0;
	max_vmdla_opp = 0;
	max_dsp_freq = 0;
	mutex_unlock(&opp_mutex);
	LOG_INF("%s -\n", __func__);

	return ret;
}

static int mdla_prepare_regulator_and_clock(struct device *pdev)
{
	int ret = 0;
/*enable Vmdla Vmdla*/
/*--Get regulator handle--*/
	vvpu_reg_id = regulator_get(pdev, "vpu");
	if (!vvpu_reg_id) {
		ret = -ENOENT;
	    LOG_ERR("regulator_get vvpu_reg_id failed\n");
	}
	vmdla_reg_id = regulator_get(pdev, "mt6360,bulk1");
	if (!vmdla_reg_id) {
		ret = -ENOENT;
		LOG_ERR("regulator_get vmdla_reg_id failed\n");
	}
#ifdef MTK_MDLA_FPGA_PORTING
	LOG_INF("%s skip at FPGA\n", __func__);
#else
#define PREPARE_MDLA_MTCMOS(clk) \
	{ \
		clk = devm_clk_get(pdev, #clk); \
		if (IS_ERR(clk)) { \
			ret = -ENOENT; \
			LOG_ERR("can not find mtcmos: %s\n", #clk); \
		} \
	}
	PREPARE_MDLA_MTCMOS(mtcmos_dis);
	PREPARE_MDLA_MTCMOS(mtcmos_vpu_vcore_shutdown);
	PREPARE_MDLA_MTCMOS(mtcmos_vpu_conn_shutdown);
	PREPARE_MDLA_MTCMOS(mtcmos_vpu_core2_shutdown);

#undef PREPARE_MDLA_MTCMOS

#define PREPARE_MDLA_CLK(clk) \
	{ \
		clk = devm_clk_get(pdev, #clk); \
		if (IS_ERR(clk)) { \
			ret = -ENOENT; \
			LOG_ERR("can not find clock: %s\n", #clk); \
		} else if (clk_prepare(clk)) { \
			ret = -EBADE; \
			LOG_ERR("fail to prepare clock: %s\n", #clk); \
		} \
	}

	PREPARE_MDLA_CLK(clk_mmsys_gals_ipu2mm);
	PREPARE_MDLA_CLK(clk_mmsys_gals_ipu12mm);
	PREPARE_MDLA_CLK(clk_mmsys_gals_comm0);
	PREPARE_MDLA_CLK(clk_mmsys_gals_comm1);
	PREPARE_MDLA_CLK(clk_mmsys_smi_common);
	PREPARE_MDLA_CLK(clk_apu_vcore_ahb_cg);
	PREPARE_MDLA_CLK(clk_apu_vcore_axi_cg);
	PREPARE_MDLA_CLK(clk_apu_vcore_adl_cg);
	PREPARE_MDLA_CLK(clk_apu_vcore_qos_cg);
	PREPARE_MDLA_CLK(clk_apu_conn_apu_cg);
	PREPARE_MDLA_CLK(clk_apu_conn_ahb_cg);
	PREPARE_MDLA_CLK(clk_apu_conn_axi_cg);
	PREPARE_MDLA_CLK(clk_apu_conn_isp_cg);
	PREPARE_MDLA_CLK(clk_apu_conn_cam_adl_cg);
	PREPARE_MDLA_CLK(clk_apu_conn_img_adl_cg);
	PREPARE_MDLA_CLK(clk_apu_conn_emi_26m_cg);
	PREPARE_MDLA_CLK(clk_apu_conn_vpu_udi_cg);
#if 0
	PREPARE_VPU_CLK(clk_apu_core0_jtag_cg);
	PREPARE_VPU_CLK(clk_apu_core0_axi_m_cg);
	PREPARE_VPU_CLK(clk_apu_core0_apu_cg);
	PREPARE_VPU_CLK(clk_apu_core1_jtag_cg);
	PREPARE_VPU_CLK(clk_apu_core1_axi_m_cg);
	PREPARE_VPU_CLK(clk_apu_core1_apu_cg);
#endif
	PREPARE_MDLA_CLK(clk_apu_mdla_apb_cg);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b0);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b1);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b2);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b3);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b4);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b5);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b6);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b7);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b8);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b9);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b10);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b11);
	PREPARE_MDLA_CLK(clk_apu_mdla_cg_b12);
	PREPARE_MDLA_CLK(clk_top_dsp_sel);
#if 0
	PREPARE_MDLA_CLK(clk_top_dsp1_sel);
	PREPARE_MDLA_CLK(clk_top_dsp2_sel);
#endif
	PREPARE_MDLA_CLK(clk_top_dsp3_sel);
	PREPARE_MDLA_CLK(clk_top_ipu_if_sel);
	PREPARE_MDLA_CLK(clk_top_clk26m);
	PREPARE_MDLA_CLK(clk_top_univpll_d3_d8);
	PREPARE_MDLA_CLK(clk_top_univpll_d3_d4);
	PREPARE_MDLA_CLK(clk_top_mainpll_d2_d4);
	PREPARE_MDLA_CLK(clk_top_univpll_d3_d2);
	PREPARE_MDLA_CLK(clk_top_mainpll_d2_d2);
	PREPARE_MDLA_CLK(clk_top_univpll_d2_d2);
	PREPARE_MDLA_CLK(clk_top_mainpll_d3);
	PREPARE_MDLA_CLK(clk_top_univpll_d3);
	PREPARE_MDLA_CLK(clk_top_mmpll_d7);
	PREPARE_MDLA_CLK(clk_top_mmpll_d6);
	PREPARE_MDLA_CLK(clk_top_adsppll_d5);
	PREPARE_MDLA_CLK(clk_top_tvdpll_ck);
	PREPARE_MDLA_CLK(clk_top_tvdpll_mainpll_d2_ck);
	PREPARE_MDLA_CLK(clk_top_univpll_d2);
	PREPARE_MDLA_CLK(clk_top_adsppll_d4);
	PREPARE_MDLA_CLK(clk_top_mainpll_d2);
	PREPARE_MDLA_CLK(clk_top_mmpll_d4);

#undef PREPARE_MDLA_CLK

#endif
	return ret;
}
static int mdla_enable_regulator_and_clock(int core)
{
#ifdef MTK_MDLA_FPGA_PORTING
	LOG_INF("%s skip at FPGA\n", __func__);

	is_power_on[core] = true;
	force_change_vcore_opp[core] = false;
	force_change_vmdla_opp[core] = false;
	force_change_dsp_freq[core] = false;
	force_change_dsp_if_freq[core] = false;
	return 0;
#else
	int ret = 0;
	//int get_vcore_opp = 0;
	int get_vmdla_opp = 0;
	//bool adjust_vcore = false;
	bool adjust_vmdla = false;

	LOG_INF("[mdla_%d] en_rc + (%d)\n", core, is_power_debug_lock);
	mdla_trace_begin("%s");

	if (is_power_debug_lock)
		goto clk_on;
#if 0
	get_vcore_opp = mdla_get_hw_vcore_opp(core);
	if (opps.vcore.index != get_vcore_opp)
		adjust_vcore = true;
#else
	/*--enable regulator--*/
	ret = regulator_enable(vmdla_reg_id);
	if (ret) {
	LOG_ERR("regulator_enable vmdla_reg_id failed\n");
	goto out;
	}
	get_vmdla_opp = mdla_get_hw_vmdla_opp(core);
	if (opps.vmdla.index != get_vmdla_opp)
		adjust_vmdla = true;
#endif
	mdla_trace_begin("vcore:request");
	//if (adjust_vcore) {
	if (adjust_vmdla) {
		LOG_DBG("[mdla_%d] en_rc wait for changing vcore opp", core);
#if 0
		ret = wait_to_do_change_vcore_opp(core);
		if (ret) {

			/* skip change vcore in these time */
			LOG_WRN("[mdla_%d] timeout to %s(%d/%d), ret=%d\n",
				core,
				"wait_to_do_change_vcore_opp",
				opps.vcore.index,
				//get_vcore_opp,
				ret);

			ret = 0;
			goto clk_on;
		}
#endif
		LOG_DBG("[mdla_%d] en_rc to do vmdla opp change", core);
#ifdef ENABLE_PMQOS
		switch (opps.vmdla.index) {
		case 0:
			pm_qos_update_request(&mdla_qos_vmdla_request[core],
								VMDLA_OPP_0);
			pm_qos_update_request(&mdla_qos_vcore_request[core],
								VCORE_OPP_0);
			break;
		case 1:
			pm_qos_update_request(&mdla_qos_vmdla_request[core],
								VMDLA_OPP_1);
			pm_qos_update_request(&mdla_qos_vcore_request[core],
								VCORE_OPP_1);
				break;
		case 2:
		default:
			pm_qos_update_request(&mdla_qos_vmdla_request[core],
								VMDLA_OPP_2);
			pm_qos_update_request(&mdla_qos_vcore_request[core],
								VCORE_OPP_2);
			break;
		}
#else
		ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL,
							opps.vcore.index);
#endif
	}
	mdla_trace_end();
	if (ret) {
		LOG_ERR("[mdla_%d]fail to request vcore, step=%d\n",
				core, opps.vcore.index);
		goto out;
	}
	LOG_INF("[mdla_%d] adjust(%d,%d) result vmdla=%d\n",
			core,
			adjust_vmdla,
			opps.vmdla.index,
			regulator_get_voltage(vmdla_reg_id));

	LOG_DBG("[mdla_%d] en_rc setmmdvfs(%d) done\n", core, opps.vcore.index);

clk_on:
#define ENABLE_MDLA_MTCMOS(clk) \
{ \
	if (clk != NULL) { \
		if (clk_prepare_enable(clk)) \
			LOG_ERR("fail to prepare&enable mtcmos:%s\n", #clk); \
	} else { \
		LOG_WRN("mtcmos not existed: %s\n", #clk); \
	} \
}

#define ENABLE_MDLA_CLK(clk) \
	{ \
		if (clk != NULL) { \
			if (clk_enable(clk)) \
				LOG_ERR("fail to enable clock: %s\n", #clk); \
		} else { \
			LOG_WRN("clk not existed: %s\n", #clk); \
		} \
	}

	mdla_trace_begin("clock:enable_source");
	ENABLE_MDLA_CLK(clk_top_dsp_sel);
	ENABLE_MDLA_CLK(clk_top_ipu_if_sel);
	ENABLE_MDLA_CLK(clk_top_dsp3_sel);
	mdla_trace_end();

	mdla_trace_begin("mtcmos:enable");
	ENABLE_MDLA_MTCMOS(mtcmos_dis);
	ENABLE_MDLA_MTCMOS(mtcmos_vpu_vcore_shutdown);
	ENABLE_MDLA_MTCMOS(mtcmos_vpu_conn_shutdown);
	ENABLE_MDLA_MTCMOS(mtcmos_vpu_core2_shutdown);

	mdla_trace_end();
	udelay(500);

	mdla_trace_begin("clock:enable");
	ENABLE_MDLA_CLK(clk_mmsys_gals_ipu2mm);
	ENABLE_MDLA_CLK(clk_mmsys_gals_ipu12mm);
	ENABLE_MDLA_CLK(clk_mmsys_gals_comm0);
	ENABLE_MDLA_CLK(clk_mmsys_gals_comm1);
	ENABLE_MDLA_CLK(clk_mmsys_smi_common);
	#if 0
	/*ENABLE_VPU_CLK(clk_ipu_adl_cabgen);*/
	/*ENABLE_VPU_CLK(clk_ipu_conn_dap_rx_cg);*/
	/*ENABLE_VPU_CLK(clk_ipu_conn_apb2axi_cg);*/
	/*ENABLE_VPU_CLK(clk_ipu_conn_apb2ahb_cg);*/
	/*ENABLE_VPU_CLK(clk_ipu_conn_ipu_cab1to2);*/
	/*ENABLE_VPU_CLK(clk_ipu_conn_ipu1_cab1to2);*/
	/*ENABLE_VPU_CLK(clk_ipu_conn_ipu2_cab1to2);*/
	/*ENABLE_VPU_CLK(clk_ipu_conn_cab3to3);*/
	/*ENABLE_VPU_CLK(clk_ipu_conn_cab2to1);*/
	/*ENABLE_VPU_CLK(clk_ipu_conn_cab3to1_slice);*/
	#endif


	ENABLE_MDLA_CLK(clk_apu_vcore_ahb_cg);
	ENABLE_MDLA_CLK(clk_apu_vcore_axi_cg);
	ENABLE_MDLA_CLK(clk_apu_vcore_adl_cg);
	ENABLE_MDLA_CLK(clk_apu_vcore_qos_cg);
	ENABLE_MDLA_CLK(clk_apu_conn_apu_cg);
	ENABLE_MDLA_CLK(clk_apu_conn_ahb_cg);
	ENABLE_MDLA_CLK(clk_apu_conn_axi_cg);
	ENABLE_MDLA_CLK(clk_apu_conn_isp_cg);
	ENABLE_MDLA_CLK(clk_apu_conn_cam_adl_cg);
	ENABLE_MDLA_CLK(clk_apu_conn_img_adl_cg);
	ENABLE_MDLA_CLK(clk_apu_conn_emi_26m_cg);
	ENABLE_MDLA_CLK(clk_apu_conn_vpu_udi_cg);
	switch (core) {
	case 0:
	default:
		ENABLE_MDLA_CLK(clk_apu_mdla_apb_cg);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b0);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b1);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b2);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b3);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b4);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b5);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b6);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b7);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b8);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b9);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b10);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b11);
		ENABLE_MDLA_CLK(clk_apu_mdla_cg_b12);
		break;
	}
	mdla_trace_end();

#undef ENABLE_MDLA_MTCMOS
#undef ENABLE_MDLA_CLK

	LOG_INF("[mdla_%d] en_rc setclksrc(%d/%d/%d)\n",
			core,
			opps.dsp.index,
			opps.mdlacore.index,
			opps.ipu_if.index);

	ret = mdla_if_set_clock_source(clk_top_dsp_sel, opps.dsp.index);
	if (ret) {
		LOG_ERR("[mdla_%d]fail to set dsp freq, step=%d, ret=%d\n",
				core, opps.dsp.index, ret);
		goto out;
	}

	ret = mdla_set_clock_source(clk_top_dsp3_sel, opps.mdlacore.index);
	if (ret) {
		LOG_ERR("[mdla_%d]fail to set mdla freq, step=%d, ret=%d\n",
				core, opps.mdlacore.index, ret);
		goto out;
	}

	ret = mdla_if_set_clock_source(clk_top_ipu_if_sel, opps.ipu_if.index);
	if (ret) {
		LOG_ERR("[mdla_%d]fail to set ipu_if freq, step=%d, ret=%d\n",
				core, opps.ipu_if.index, ret);
		goto out;
	}

out:
	mdla_trace_end();
	is_power_on[core] = true;
	force_change_vcore_opp[core] = false;
	force_change_vmdla_opp[core] = false;
	force_change_dsp_freq[core] = false;
	force_change_dsp_if_freq[core] = false;
	LOG_DBG("[mdla_%d] en_rc -\n", core);
	return ret;
#endif
}

static int mdla_disable_regulator_and_clock(int core)
{
	int ret = 0;

#ifdef MTK_MDLA_FPGA_PORTING
	LOG_INF("%s skip at FPGA\n", __func__);

	is_power_on[core] = false;
	if (!is_power_debug_lock)
		opps.mdlacore.index = 7;

	return ret;
#else
	unsigned int smi_bus_vpu_value = 0x0;

	/* check there is un-finished transaction in bus before
	 * turning off vpu power
	 */
#ifdef MTK_VPU_SMI_DEBUG_ON
	smi_bus_vpu_value = vpu_read_smi_bus_debug(core);

	LOG_INF("[vpu_%d] dis_rc 1 (0x%x)\n", core, smi_bus_vpu_value);

	if ((int)smi_bus_vpu_value != 0) {
		mdelay(1);
		smi_bus_vpu_value = vpu_read_smi_bus_debug(core);

		LOG_INF("[vpu_%d] dis_rc again (0x%x)\n", core,
				smi_bus_vpu_value);

		if ((int)smi_bus_vpu_value != 0) {
			smi_debug_bus_hanging_detect_ext2(0x1ff, 1, 0, 1);
			vpu_aee_warn("VPU SMI CHECK",
				"core_%d fail to check smi, value=%d\n",
				core,
				smi_bus_vpu_value);
		}
	}
#else
	LOG_INF("[mdla_%d] dis_rc + (0x%x)\n", core, smi_bus_vpu_value);
#endif

#define DISABLE_MDLA_CLK(clk) \
	{ \
		if (clk != NULL) { \
			clk_disable(clk); \
		} else { \
			LOG_WRN("clk not existed: %s\n", #clk); \
		} \
	}
	switch (core) {
	case 0:
	default:
		DISABLE_MDLA_CLK(clk_apu_mdla_apb_cg);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b0);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b1);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b2);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b3);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b4);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b5);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b6);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b7);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b8);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b9);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b10);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b11);
		DISABLE_MDLA_CLK(clk_apu_mdla_cg_b12);
		break;
	}
	#if 0
	DISABLE_VPU_CLK(clk_ipu_adl_cabgen);
	DISABLE_VPU_CLK(clk_ipu_conn_dap_rx_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_apb2axi_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_apb2ahb_cg);
	DISABLE_VPU_CLK(clk_ipu_conn_ipu_cab1to2);
	DISABLE_VPU_CLK(clk_ipu_conn_ipu1_cab1to2);
	DISABLE_VPU_CLK(clk_ipu_conn_ipu2_cab1to2);
	DISABLE_VPU_CLK(clk_ipu_conn_cab3to3);
	DISABLE_VPU_CLK(clk_ipu_conn_cab2to1);
	DISABLE_VPU_CLK(clk_ipu_conn_cab3to1_slice);
	#endif
	DISABLE_MDLA_CLK(clk_apu_conn_apu_cg);
	DISABLE_MDLA_CLK(clk_apu_conn_ahb_cg);
	DISABLE_MDLA_CLK(clk_apu_conn_axi_cg);
	DISABLE_MDLA_CLK(clk_apu_conn_isp_cg);
	DISABLE_MDLA_CLK(clk_apu_conn_cam_adl_cg);
	DISABLE_MDLA_CLK(clk_apu_conn_img_adl_cg);
	DISABLE_MDLA_CLK(clk_apu_conn_emi_26m_cg);
	DISABLE_MDLA_CLK(clk_apu_conn_vpu_udi_cg);
	DISABLE_MDLA_CLK(clk_apu_vcore_ahb_cg);
	DISABLE_MDLA_CLK(clk_apu_vcore_axi_cg);
	DISABLE_MDLA_CLK(clk_apu_vcore_adl_cg);
	DISABLE_MDLA_CLK(clk_apu_vcore_qos_cg);
	DISABLE_MDLA_CLK(clk_mmsys_gals_ipu2mm);
	DISABLE_MDLA_CLK(clk_mmsys_gals_ipu12mm);
	DISABLE_MDLA_CLK(clk_mmsys_gals_comm0);
	DISABLE_MDLA_CLK(clk_mmsys_gals_comm1);
	DISABLE_MDLA_CLK(clk_mmsys_smi_common);
	LOG_DBG("[mdla_%d] dis_rc flag4\n", core);

#define DISABLE_MDLA_MTCMOS(clk) \
	{ \
		if (clk != NULL) { \
			clk_disable_unprepare(clk); \
		} else { \
			LOG_WRN("mtcmos not existed: %s\n", #clk); \
		} \
	}
	DISABLE_MDLA_MTCMOS(mtcmos_vpu_core2_shutdown);
	DISABLE_MDLA_MTCMOS(mtcmos_vpu_conn_shutdown);
	DISABLE_MDLA_MTCMOS(mtcmos_vpu_vcore_shutdown);
	DISABLE_MDLA_MTCMOS(mtcmos_dis);


	DISABLE_MDLA_CLK(clk_top_dsp_sel);
	DISABLE_MDLA_CLK(clk_top_ipu_if_sel);
	DISABLE_MDLA_CLK(clk_top_dsp3_sel);

#undef DISABLE_MDLA_MTCMOS
#undef DISABLE_MDLA_CLK
#ifdef ENABLE_PMQOS
	pm_qos_update_request(&mdla_qos_vmdla_request[core], VMDLA_OPP_UNREQ);
	pm_qos_update_request(&mdla_qos_vcore_request[core], VCORE_OPP_UNREQ);
#else
	ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL,
						MMDVFS_FINE_STEP_UNREQUEST);
#endif
	if (ret) {
		LOG_ERR("[mdla_%d]fail to unrequest vcore!\n", core);
		goto out;
	}
	LOG_DBG("[mdla_%d] disable result vmdla=%d\n",
		core, regulator_get_voltage(vmdla_reg_id));
out:
	/*--disable regulator--*/
	ret = regulator_disable(vmdla_reg_id);
	if (ret)
	LOG_ERR("regulator_disable vmdla_reg_id failed\n");
	is_power_on[core] = false;
	if (!is_power_debug_lock)
		opps.mdlacore.index = 15;
	LOG_INF("[mdla_%d] dis_rc -\n", core);
	return ret;
#endif
}

static void mdla_unprepare_regulator_and_clock(void)
{

#ifdef MTK_MDLA_FPGA_PORTING
	LOG_INF("%s skip at FPGA\n", __func__);
#else
#define UNPREPARE_MDLA_CLK(clk) \
	{ \
		if (clk != NULL) { \
			clk_unprepare(clk); \
			clk = NULL; \
		} \
	}
#if 0
	UNPREPARE_MDLA_CLK(clk_apu_core0_jtag_cg);
	UNPREPARE_MDLA_CLK(clk_apu_core0_axi_m_cg);
	UNPREPARE_MDLA_CLK(clk_apu_core0_apu_cg);
	UNPREPARE_MDLA_CLK(clk_apu_core1_jtag_cg);
	UNPREPARE_MDLA_CLK(clk_apu_core1_axi_m_cg);
	UNPREPARE_MDLA_CLK(clk_apu_core1_apu_cg);
#endif
	UNPREPARE_MDLA_CLK(clk_apu_mdla_apb_cg);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b0);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b1);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b2);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b3);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b4);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b5);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b6);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b7);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b8);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b9);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b10);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b11);
	UNPREPARE_MDLA_CLK(clk_apu_mdla_cg_b12);
	UNPREPARE_MDLA_CLK(clk_apu_conn_apu_cg);
	UNPREPARE_MDLA_CLK(clk_apu_conn_ahb_cg);
	UNPREPARE_MDLA_CLK(clk_apu_conn_axi_cg);
	UNPREPARE_MDLA_CLK(clk_apu_conn_isp_cg);
	UNPREPARE_MDLA_CLK(clk_apu_conn_cam_adl_cg);
	UNPREPARE_MDLA_CLK(clk_apu_conn_img_adl_cg);
	UNPREPARE_MDLA_CLK(clk_apu_conn_emi_26m_cg);
	UNPREPARE_MDLA_CLK(clk_apu_conn_vpu_udi_cg);
	UNPREPARE_MDLA_CLK(clk_apu_vcore_ahb_cg);
	UNPREPARE_MDLA_CLK(clk_apu_vcore_axi_cg);
	UNPREPARE_MDLA_CLK(clk_apu_vcore_adl_cg);
	UNPREPARE_MDLA_CLK(clk_apu_vcore_qos_cg);
	UNPREPARE_MDLA_CLK(clk_mmsys_gals_ipu2mm);
	UNPREPARE_MDLA_CLK(clk_mmsys_gals_ipu12mm);
	UNPREPARE_MDLA_CLK(clk_mmsys_gals_comm0);
	UNPREPARE_MDLA_CLK(clk_mmsys_gals_comm1);
	UNPREPARE_MDLA_CLK(clk_mmsys_smi_common);
	UNPREPARE_MDLA_CLK(clk_top_dsp_sel);
#if 0
	UNPREPARE_MDLA_CLK(clk_top_dsp1_sel);
	UNPREPARE_MDLA_CLK(clk_top_dsp2_sel);
#endif
	UNPREPARE_MDLA_CLK(clk_top_dsp3_sel);
	UNPREPARE_MDLA_CLK(clk_top_ipu_if_sel);
	UNPREPARE_MDLA_CLK(clk_top_clk26m);
	UNPREPARE_MDLA_CLK(clk_top_univpll_d3_d8);
	UNPREPARE_MDLA_CLK(clk_top_univpll_d3_d4);
	UNPREPARE_MDLA_CLK(clk_top_mainpll_d2_d4);
	UNPREPARE_MDLA_CLK(clk_top_univpll_d3_d2);
	UNPREPARE_MDLA_CLK(clk_top_mainpll_d2_d2);
	UNPREPARE_MDLA_CLK(clk_top_univpll_d2_d2);
	UNPREPARE_MDLA_CLK(clk_top_mainpll_d3);
	UNPREPARE_MDLA_CLK(clk_top_univpll_d3);
	UNPREPARE_MDLA_CLK(clk_top_mmpll_d7);
	UNPREPARE_MDLA_CLK(clk_top_mmpll_d6);
	UNPREPARE_MDLA_CLK(clk_top_adsppll_d5);
	UNPREPARE_MDLA_CLK(clk_top_tvdpll_ck);
	UNPREPARE_MDLA_CLK(clk_top_tvdpll_mainpll_d2_ck);
	UNPREPARE_MDLA_CLK(clk_top_univpll_d2);
	UNPREPARE_MDLA_CLK(clk_top_adsppll_d4);
	UNPREPARE_MDLA_CLK(clk_top_mainpll_d2);
	UNPREPARE_MDLA_CLK(clk_top_mmpll_d4);



#undef UNPREPARE_MDLA_CLK
#endif
}

static int mdla_get_power(int core)
{
	int ret = 0;

	LOG_DBG("[mdla_%d/%d] gp +\n", core, power_counter[core]);
	mutex_lock(&power_counter_mutex[core]);
	power_counter[core]++;
	ret = mdla_boot_up(core);
	mutex_unlock(&power_counter_mutex[core]);
	LOG_DBG("[mdla_%d/%d] gp + 2\n", core, power_counter[core]);
	if (ret == POWER_ON_MAGIC) {
		mutex_lock(&opp_mutex);
		if (change_freq_first[core]) {
			LOG_INF("[mdla_%d] change freq first(%d)\n",
					core, change_freq_first[core]);
			/*mutex_unlock(&opp_mutex);*/
			/*mutex_lock(&opp_mutex);*/
			if (force_change_dsp_freq[core]) {
				mutex_unlock(&opp_mutex);
				/* force change freq while running */
				LOG_DBG("mdla_%d force change dsp freq", core);
				mdla_change_opp(core, OPPTYPE_DSPFREQ);
			} else {
				mutex_unlock(&opp_mutex);
			}

			mutex_lock(&opp_mutex);
			//if (force_change_vcore_opp[core]) {
			if (force_change_vmdla_opp[core]) {
				mutex_unlock(&opp_mutex);
				/* vcore change should wait */
				LOG_DBG("mdla_%d force change vmdla opp", core);
				//mdla_change_opp(core, OPPTYPE_VCORE);
				mdla_change_opp(core, OPPTYPE_VMDLA);
			} else {
				mutex_unlock(&opp_mutex);
			}
		} else {
			/*mutex_unlock(&opp_mutex);*/
			/*mutex_lock(&opp_mutex);*/
			//if (force_change_vcore_opp[core]) {
			if (force_change_vmdla_opp[core]) {
				mutex_unlock(&opp_mutex);
				/* vcore change should wait */
				LOG_DBG("mdla_%d force change vcore opp", core);
				//mdla_change_opp(core, OPPTYPE_VCORE);
				mdla_change_opp(core, OPPTYPE_VMDLA);
			} else {
				mutex_unlock(&opp_mutex);
			}

			mutex_lock(&opp_mutex);
			if (force_change_dsp_freq[core]) {
				mutex_unlock(&opp_mutex);
				/* force change freq while running */
				LOG_DBG("mdla_%d force change dsp freq", core);
				mdla_change_opp(core, OPPTYPE_DSPFREQ);
			} else {
				mutex_unlock(&opp_mutex);
			}
		}
	}
	LOG_DBG("[mdla_%d/%d] gp -\n", core, power_counter[core]);

	if (ret == POWER_ON_MAGIC)
		return 0;
	else
		return ret;
}

static void mdla_put_power(int core, enum mdlaPowerOnType type)
{
	LOG_DBG("[mdla_%d/%d] pp +\n", core, power_counter[core]);
	mutex_lock(&power_counter_mutex[core]);
	if (--power_counter[core] == 0) {
		switch (type) {
		case VPT_PRE_ON:
			LOG_DBG("[mdla_%d] VPT_PRE_ON\n", core);
			mod_delayed_work(wq,
				&(power_counter_work[core].my_work),
				msecs_to_jiffies(10 * PWR_KEEP_TIME_MS));
			break;
		case VPT_IMT_OFF:
			LOG_INF("[mdla_%d] VPT_IMT_OFF\n", core);
			mod_delayed_work(wq,
				&(power_counter_work[core].my_work),
				msecs_to_jiffies(0));
			break;
		case VPT_ENQUE_ON:
		default:
			LOG_DBG("[mdla_%d] VPT_ENQUE_ON\n", core);
			mod_delayed_work(wq,
				&(power_counter_work[core].my_work),
				msecs_to_jiffies(PWR_KEEP_TIME_MS));
			break;
		}
	}
	mutex_unlock(&power_counter_mutex[core]);
	LOG_DBG("[mdla_%d/%d] pp -\n", core, power_counter[core]);
}

int mdla_set_power(struct mdla_power *power)
{
	int ret = 0;
	uint8_t vcore_opp_index = 0xFF;
	uint8_t vmdla_opp_index = 0xFF;
	uint8_t dsp_freq_index = 0xFF;
	int i = 0, core = -1;

	for (i = 0 ; i < MTK_MDLA_CORE ; i++) {
		/*LOG_DBG("debug i(%d), (0x1 << i) (0x%x)", i, (0x1 << i));*/
		if (power->core == (0x1 << i)) {
			core = i;
			break;
		}
	}

	if (core >= MTK_MDLA_CORE || core < 0) {
		LOG_ERR("wrong core index (0x%x/%d/%d)",
				power->core, core, MTK_MDLA_CORE);
		ret = -1;
		return ret;
	}

	LOG_INF("[mdla_%d] set power opp:%d\n",
			core, power->opp_step);

	if (power->opp_step == 0xFF) {
		vcore_opp_index = 0xFF;
		vmdla_opp_index = 0xFF;
		dsp_freq_index = 0xFF;
	} else {
		if (power->opp_step < MDLA_MAX_NUM_OPPS &&
							power->opp_step >= 0) {
			vcore_opp_index = opps.vcore.opp_map[power->opp_step];
			vmdla_opp_index = opps.vmdla.opp_map[power->opp_step];
			dsp_freq_index =
				opps.mdlacore.opp_map[power->opp_step];
		} else {
			LOG_ERR("wrong opp step (%d)", power->opp_step);
			ret = -1;
			return ret;
		}
	}
	mdla_opp_check(core, vmdla_opp_index, dsp_freq_index);
	//user->power_opp = power->opp_step;

	ret = mdla_get_power(core);
#if 0
	mutex_lock(&(mdla_service_cores[core].state_mutex));
	mdla_service_cores[core].state = VCT_IDLE;
	mutex_unlock(&(mdla_service_cores[core].state_mutex));
#endif
	/* to avoid power leakage, power on/off need be paired */
	mdla_put_power(core, VPT_PRE_ON);
	LOG_INF("[mdla_%d] %s -\n", core, __func__);
	return ret;
}

static void mdla_power_counter_routine(struct work_struct *work)
{
	int core = 0;
	struct my_struct_t *my_work_core =
			container_of(work, struct my_struct_t, my_work.work);

	core = my_work_core->core;
	LOG_INF("mdla_%d counterR (%d)+\n", core, power_counter[core]);

	mutex_lock(&power_counter_mutex[core]);
	if (power_counter[core] == 0)
		mdla_shut_down(core);
	else
		LOG_DBG("mdla_%d no need this time.\n", core);
	mutex_unlock(&power_counter_mutex[core]);

	LOG_INF("mdla_%d counterR -", core);
}
int mdla_quick_suspend(int core)
{
	LOG_DBG("[mdla_%d] q_suspend +\n", core);
	mutex_lock(&power_counter_mutex[core]);
	//LOG_INF("[mdla_%d] q_suspend (%d/%d)\n", core,
		//power_counter[core], mdla_service_cores[core].state);

	if (power_counter[core] == 0) {
		mutex_unlock(&power_counter_mutex[core]);

			mod_delayed_work(wq,
				&(power_counter_work[core].my_work),
				msecs_to_jiffies(0));
	} else {
		mutex_unlock(&power_counter_mutex[core]);
	}

	return 0;
}


static void mdla_opp_keep_routine(struct work_struct *work)
{
	LOG_INF("%s flag (%d) +\n", __func__, opp_keep_flag);
	mutex_lock(&opp_mutex);
	opp_keep_flag = false;
	mutex_unlock(&opp_mutex);
	LOG_INF("%s flag (%d) -\n", __func__, opp_keep_flag);
}
static int mdla_init_hw(int core, struct platform_device *pdev)
{
	int ret, i;
	//int param;
#if 0
	struct vpu_shared_memory_param mem_param;

	LOG_INF("[vpu] core : %d\n", core);

	vpu_service_cores[core].vpu_base = device->mdla_base[core];
	mdla_service_cores[core].bin_base = device->bin_base;

#ifdef MTK_VPU_EMULATOR
	mdla_request_emulator_irq(device->irq_num[core], mdla_isr_handler);
#else
	if (!m4u_client)
		m4u_client = m4u_create_client();
	if (!ion_client)
		ion_client = ion_client_create(g_ion_device, "mdla");

	ret = mdla_map_mva_of_bin(core, device->bin_pa);
	if (ret) {
		LOG_ERR("[mdla_%d]fail to map binary data!\n", core);
		goto out;
	}

	ret = request_irq(device->irq_num[core],
		(irq_handler_t)mdla_ISR_CB_TBL[core].isr_fp,
		device->irq_trig_level,
		(const char *)mdla_ISR_CB_TBL[core].device_name, NULL);
	if (ret) {
		LOG_ERR("[mdla_%d]fail to request mdla irq!\n", core);
		goto out;
	}
#endif
#endif

	if (core == 0) {
		//init_waitqueue_head(&cmd_wait);
		//mutex_init(&lock_mutex);
		//init_waitqueue_head(&lock_wait);
		mutex_init(&opp_mutex);
		//init_waitqueue_head(&waitq_change_vcore);
		//init_waitqueue_head(&waitq_do_core_executing);
		//is_locked = false;
		max_vcore_opp = 0;
		max_vmdla_opp = 0;
		max_dsp_freq = 0;
		opp_keep_flag = false;
		//mdla_dev = device;
		is_power_debug_lock = false;

		for (i = 0 ; i < MTK_MDLA_CORE ; i++) {
			mutex_init(&(power_mutex[i]));
			mutex_init(&(power_counter_mutex[i]));
			power_counter[i] = 0;
			power_counter_work[i].core = i;
			is_power_on[i] = false;
			force_change_vcore_opp[i] = false;
			force_change_vmdla_opp[i] = false;
			force_change_dsp_freq[i] = false;
			force_change_dsp_if_freq[i] = false;
			change_freq_first[i] = false;
			//exception_isr_check[i] = false;

			INIT_DELAYED_WORK(&(power_counter_work[i].my_work),
						mdla_power_counter_routine);

			#ifdef CONFIG_PM_WAKELOCKS
			if (i == 0) {
				wakeup_source_init(&(mdla_wake_lock[i]),
							"mdla_wakelock_0");
			} else {
				wakeup_source_init(&(mdla_wake_lock[i]),
							"mdla_wakelock_1");
			}
			#else
			if (i == 0) {
				wake_lock_init(&(mdla_wake_lock[i]),
							WAKE_LOCK_SUSPEND,
							"mdla_wakelock_0");
			} else {
				wake_lock_init(&(mdla_wake_lock[i]),
							WAKE_LOCK_SUSPEND,
							"mdla_wakelock_1");
			}
			#endif
			}
#if 0
		/* define the steps and OPP */
#define DEFINE_VPU_STEP(step, m, v0, v1, v2, v3, v4, v5, v6, v7) \
			{ \
				opps.step.index = m - 1; \
				opps.step.count = m; \
				opps.step.values[0] = v0; \
				opps.step.values[1] = v1; \
				opps.step.values[2] = v2; \
				opps.step.values[3] = v3; \
				opps.step.values[4] = v4; \
				opps.step.values[5] = v5; \
				opps.step.values[6] = v6; \
				opps.step.values[7] = v7; \
			}


#define DEFINE_VPU_OPP(i, v0, v1, v2, v3, v4) \
		{ \
			opps.vcore.opp_map[i]  = v0; \
			opps.dsp.opp_map[i]    = v1; \
			opps.dspcore[0].opp_map[i]   = v2; \
			opps.dspcore[1].opp_map[i]   = v3; \
			opps.ipu_if.opp_map[i] = v4; \
		}


		DEFINE_VPU_STEP(vcore,  2, 800000, 725000, 0, 0, 0, 0, 0, 0);

		DEFINE_VPU_STEP(dsp,    8,
					525000, 450000, 416000, 364000,
					312000, 273000, 208000, 182000);

		DEFINE_VPU_STEP(dspcore[0],   8,
					525000, 450000, 416000, 364000,
					312000, 273000, 208000, 182000);

		DEFINE_VPU_STEP(dspcore[1],   8,
					525000, 450000, 416000, 364000,
					312000, 273000, 208000, 182000);

		DEFINE_VPU_STEP(ipu_if, 8,
					525000, 450000, 416000, 364000,
					312000, 273000, 208000, 182000);

		/* default freq */
		DEFINE_VPU_OPP(0, 0, 0, 0, 0, 0);
		DEFINE_VPU_OPP(1, 0, 1, 1, 1, 1);
		DEFINE_VPU_OPP(2, 0, 2, 2, 2, 2);
		DEFINE_VPU_OPP(3, 0, 3, 3, 3, 3);
		DEFINE_VPU_OPP(4, 1, 4, 4, 4, 4);
		DEFINE_VPU_OPP(5, 1, 5, 5, 5, 5);
		DEFINE_VPU_OPP(6, 1, 6, 6, 6, 6);
		DEFINE_VPU_OPP(7, 1, 7, 7, 7, 7);

		/* default low opp */
		opps.count = 8;
		opps.index = 4; /* user space usage*/
		opps.vcore.index = 1;
		opps.dsp.index = 4;
		opps.dspcore[0].index = 4;
		opps.dspcore[1].index = 4;
		opps.ipu_if.index = 4;

#undef DEFINE_VPU_OPP
#undef DEFINE_VPU_STEP
#else
			/* define the steps and OPP */
#define DEFINE_APU_STEP(step, m, v0, v1, v2, v3, \
						v4, v5, v6, v7, v8, v9, \
						v10, v11, v12, v13, v14, v15) \
				{ \
					opps.step.index = m - 1; \
					opps.step.count = m; \
					opps.step.values[0] = v0; \
					opps.step.values[1] = v1; \
					opps.step.values[2] = v2; \
					opps.step.values[3] = v3; \
					opps.step.values[4] = v4; \
					opps.step.values[5] = v5; \
					opps.step.values[6] = v6; \
					opps.step.values[7] = v7; \
					opps.step.values[8] = v8; \
					opps.step.values[9] = v9; \
					opps.step.values[10] = v10; \
					opps.step.values[11] = v11; \
					opps.step.values[12] = v12; \
					opps.step.values[13] = v13; \
					opps.step.values[14] = v14; \
					opps.step.values[15] = v15; \
				}


#define DEFINE_APU_OPP(i, v0, v1, v2, v3, v4, v5, v6) \
			{ \
				opps.vvpu.opp_map[i]  = v0; \
				opps.vmdla.opp_map[i]  = v1; \
				opps.dsp.opp_map[i]    = v2; \
				opps.dspcore[0].opp_map[i]	 = v3; \
				opps.dspcore[1].opp_map[i]	 = v4; \
				opps.ipu_if.opp_map[i] = v5; \
				opps.mdlacore.opp_map[i]    = v6; \
			}

			DEFINE_APU_STEP(vcore, 3, 825000,
				725000, 650000, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0);
			DEFINE_APU_STEP(vvpu, 3, 825000,
				725000, 650000, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0);
			DEFINE_APU_STEP(vmdla, 3, 825000,
				725000, 650000, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0);
			DEFINE_APU_STEP(dsp, 16, 624000,
				624000, 606000, 594000,
				560000, 525000, 450000,
				416000, 364000, 312000,
				273000, 208000, 137000,
				104000, 52000, 26000);
			DEFINE_APU_STEP(dspcore[0], 16, 700000,
				624000, 606000, 594000,
				560000, 525000, 450000,
				416000, 364000, 312000,
				273000, 208000, 137000,
				104000, 52000, 26000);
			DEFINE_APU_STEP(dspcore[1], 16, 700000,
				624000, 606000, 594000,
				560000, 525000, 450000,
				416000, 364000, 312000,
				273000, 208000, 137000,
				104000, 52000, 26000);
			DEFINE_APU_STEP(mdlacore, 16, 788000,
				700000, 624000, 606000,
				594000, 546000, 525000,
				450000, 416000, 364000,
				312000, 273000, 208000,
				137000, 52000, 26000);
			DEFINE_APU_STEP(ipu_if, 16, 624000,
				624000, 606000, 594000,
				560000, 525000, 450000,
				416000, 364000, 312000,
				273000, 208000, 137000,
				104000, 52000, 26000);

			/* default freq */
			DEFINE_APU_OPP(0, 0, 0, 0, 0, 0, 0, 0);
			DEFINE_APU_OPP(1, 0, 0, 1, 1, 1, 1, 1);
			DEFINE_APU_OPP(2, 0, 0, 2, 2, 2, 2, 2);
			DEFINE_APU_OPP(3, 0, 1, 3, 3, 3, 3, 3);
			DEFINE_APU_OPP(4, 0, 1, 4, 4, 4, 4, 4);
			DEFINE_APU_OPP(5, 1, 1, 5, 5, 5, 5, 5);
			DEFINE_APU_OPP(6, 1, 1, 6, 6, 6, 6, 6);
			DEFINE_APU_OPP(7, 1, 1, 7, 7, 7, 7, 7);
			DEFINE_APU_OPP(8, 1, 1, 8, 8, 8, 8, 8);
			DEFINE_APU_OPP(9, 2, 2, 9, 9, 9, 9, 9);
			DEFINE_APU_OPP(10, 2, 2, 10, 10, 10, 10, 10);
			DEFINE_APU_OPP(11, 2, 2, 11, 11, 11, 11, 11);
			DEFINE_APU_OPP(12, 2, 2, 12, 12, 12, 12, 12);
			DEFINE_APU_OPP(13, 2, 2, 13, 13, 13, 13, 13);
			DEFINE_APU_OPP(14, 2, 2, 14, 14, 14, 14, 14);
			DEFINE_APU_OPP(15, 2, 2, 15, 15, 15, 15, 15);


			/* default low opp */
			opps.count = 16;
			opps.index = 5; /* user space usage*/
			opps.vcore.index = 1;
			opps.vmdla.index = 1;
			opps.dsp.index = 5;
			opps.dspcore[0].index = 5;
			opps.dspcore[1].index = 5;
			opps.ipu_if.index = 5;
			opps.mdlacore.index = 3;
#undef DEFINE_APU_OPP
#undef DEFINE_APU_STEP

#endif
		ret = mdla_prepare_regulator_and_clock(&pdev->dev);
		if (ret) {
			LOG_ERR("[mdla_%d]fail to prepare regulator or clk!\n",
					core);
			goto out;
		}

		/* pmqos  */
		#ifdef ENABLE_PMQOS
		for (i = 0 ; i < MTK_MDLA_CORE ; i++) {
			pm_qos_add_request(&mdla_qos_bw_request[i],
						PM_QOS_MM_MEMORY_BANDWIDTH,
						PM_QOS_DEFAULT_VALUE);

			pm_qos_add_request(&mdla_qos_vcore_request[i],
						PM_QOS_VCORE_OPP,
						PM_QOS_VCORE_OPP_DEFAULT_VALUE);

		    pm_qos_add_request(&mdla_qos_vmdla_request[i],
					    PM_QOS_VMDLA_OPP,
					    PM_QOS_VMDLA_OPP_DEFAULT_VALUE);
		}
		#endif

	}
	return 0;

out:
#if 0
	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		if (mdla_service_cores[i].srvc_task != NULL) {
			kfree(mdla_service_cores[i].srvc_task);
			mdla_service_cores[i].srvc_task = NULL;
		}

		if (mdla_service_cores[i].work_buf)
			mdla_free_shared_memory(mdla_service_cores[i].work_buf);
	}
#endif
	return ret;
}

static int mdla_uninit_hw(void)
{
	int i;

	for (i = 0 ; i < MTK_MDLA_CORE ; i++) {
		cancel_delayed_work(&(power_counter_work[i].my_work));
#if 0
		if (mdla_service_cores[i].srvc_task != NULL) {
			kthread_stop(mdla_service_cores[i].srvc_task);
			kfree(mdla_service_cores[i].srvc_task);
			mdla_service_cores[i].srvc_task = NULL;
		}

		if (mdla_service_cores[i].work_buf) {
			mdla_free_shared_memory(mdla_service_cores[i].work_buf);
			mdla_service_cores[i].work_buf = NULL;
		}
#endif
	}
	cancel_delayed_work(&opp_keep_work);
	/* pmqos  */
	#ifdef ENABLE_PMQOS
	for (i = 0 ; i < MTK_MDLA_CORE ; i++) {
		pm_qos_remove_request(&mdla_qos_bw_request[i]);
		pm_qos_remove_request(&mdla_qos_vcore_request[i]);
		pm_qos_remove_request(&mdla_qos_vmdla_request[i]);
	}
	#endif

	mdla_unprepare_regulator_and_clock();
#if 0
	if (m4u_client) {
		m4u_destroy_client(m4u_client);
		m4u_client = NULL;
	}

	if (ion_client) {
		ion_client_destroy(ion_client);
		ion_client = NULL;
	}
#endif
	if (wq) {
		flush_workqueue(wq);
		destroy_workqueue(wq);
		wq = NULL;
	}

	return 0;
}

int mdla_boot_up(int core)
{
	int ret = 0;

	LOG_DBG("[mdla_%d] boot_up +\n", core);
	mutex_lock(&power_mutex[core]);
	LOG_DBG("[mdla_%d] is_power_on(%d)\n", core, is_power_on[core]);
	if (is_power_on[core]) {
		mutex_unlock(&power_mutex[core]);
		//mutex_lock(&(mdla_service_cores[core].state_mutex));
		//mdla_service_cores[core].state = VCT_BOOTUP;
		//mutex_unlock(&(mdla_service_cores[core].state_mutex));
		//wake_up_interruptible(&waitq_change_vcore);
		LOG_DBG("[mdla_%d] already power on\n", core);
		return POWER_ON_MAGIC;
	}
	LOG_DBG("[mdla_%d] boot_up flag2\n", core);

	mdla_trace_begin("%s", __func__);
	//mutex_lock(&(mdla_service_cores[core].state_mutex));
	//mdla_service_cores[core].state = VCT_BOOTUP;
	//mutex_unlock(&(mdla_service_cores[core].state_mutex));
	//wake_up_interruptible(&waitq_change_vcore);

	ret = mdla_enable_regulator_and_clock(core);
	if (ret) {
		LOG_ERR("[mdla_%d]fail to enable regulator or clock\n", core);
		goto out;
	}
#ifdef MET_POLLING_MODE
	ret = mdla_profile_state_set(core, 1);
	if (ret) {
		LOG_ERR("[mdla_%d] fail to mdla_profile_state_set 1\n", core);
		goto out;
	}
#endif

out:
#if 0 /* control on/off outside the via get_power/put_power */
	if (ret) {
		ret = mdla_disable_regulator_and_clock(core);
		if (ret) {
			LOG_ERR("[mdla_%d]fail to disable regulator and clk\n",
					core);
			goto out;
		}
	}
#endif
	mdla_trace_end();
	mutex_unlock(&power_mutex[core]);
	return ret;
}

int mdla_shut_down(int core)
{
	int ret = 0;

	LOG_DBG("[mdla_%d] shutdown +\n", core);
	mutex_lock(&power_mutex[core]);
	if (!is_power_on[core]) {
		mutex_unlock(&power_mutex[core]);
		return 0;
	}
#if 0
	mutex_lock(&(mdla_service_cores[core].state_mutex));
	switch (mdla_service_cores[core].state) {
	case VCT_SHUTDOWN:
	case VCT_IDLE:
	case VCT_NONE:
#ifdef MTK_mdla_FPGA_PORTING
	case VCT_BOOTUP:
#endif
		mdla_service_cores[core].current_algo = 0;
		mdla_service_cores[core].state = VCT_SHUTDOWN;
		mutex_unlock(&(mdla_service_cores[core].state_mutex));
		break;
#ifndef MTK_VPU_FPGA_PORTING
	case VCT_BOOTUP:
#endif
	case VCT_EXECUTING:
	case VCT_VCORE_CHG:
		mutex_unlock(&(mdla_service_cores[core].state_mutex));
		goto out;
		/*break;*/
	}
#endif
#ifdef MET_POLLING_MODE
	ret = mdla_profile_state_set(core, 0);
	if (ret) {
		LOG_ERR("[mdla_%d] fail to mdla_profile_state_set 0\n", core);
		goto out;
	}
#endif

	mdla_trace_begin("%s", __func__);
	ret = mdla_disable_regulator_and_clock(core);
	if (ret) {
		LOG_ERR("[mdla_%d]fail to disable regulator and clock\n", core);
		goto out;
	}

	//wake_up_interruptible(&waitq_change_vcore);
out:
	mdla_trace_end();
	mutex_unlock(&power_mutex[core]);
	LOG_DBG("[mdla_%d] shutdown -\n", core);
	return ret;
}

int mdla_dump_opp_table(struct seq_file *s)
{
	int i;

#define LINE_BAR "  +-----+----------+----------+------------+-----------+-----------+\n"
	mdla_print_seq(s, LINE_BAR);
	mdla_print_seq(s, "  |%-5s|%-10s|%-10s|%-10s|%-10s|%-12s|\n",
				"OPP", "VCORE(uV)", "VVPU(uV)",
				"VMDLA(uV)", "DSP(KHz)",
				"IPU_IF(KHz)");
	mdla_print_seq(s, LINE_BAR);

	for (i = 0; i < opps.count; i++) {
		mdla_print_seq(s,
"  |%-5d|[%d]%-7d|[%d]%-7d|[%d]%-7d|[%d]%-7d|[%d]%-9d\n",
			i,
			opps.vcore.opp_map[i],
			opps.vcore.values[opps.vcore.opp_map[i]],
			opps.vvpu.opp_map[i],
			opps.vvpu.values[opps.vvpu.opp_map[i]],
			opps.vmdla.opp_map[i],
			opps.vmdla.values[opps.vmdla.opp_map[i]],
			opps.dsp.opp_map[i],
			opps.dsp.values[opps.dsp.opp_map[i]],
			opps.ipu_if.opp_map[i],
			opps.ipu_if.values[opps.ipu_if.opp_map[i]]);
	}

	mdla_print_seq(s, LINE_BAR);
#undef LINE_BAR

	return 0;
}

int mdla_dump_power(struct seq_file *s)
{
	int vvpu_opp = 0;
	int vmdla_opp = 0;

	vvpu_opp = mdla_get_hw_vvpu_opp(0);
	vmdla_opp = mdla_get_hw_vmdla_opp(0);


mdla_print_seq(s, "%s(rw): %s[%d/%d], %s[%d/%d]\n",
			"dvfs_debug",
			"vvpu", opps.vvpu.index, vvpu_opp,
			"vmdla", opps.vmdla.index, vmdla_opp);
mdla_print_seq(s, "%s[%d], %s[%d], %s[%d], %s[%d]\n",
			"dsp", opps.dsp.index,
			"ipu_if", opps.ipu_if.index,
			"dsp1", opps.dspcore[0].index,
			"dsp2", opps.dspcore[1].index);


	mdla_print_seq(s, "is_power_debug_lock(rw): %d\n", is_power_debug_lock);

	return 0;
}

int mdla_set_power_parameter(uint8_t param, int argc, int *args)
{
	int ret = 0;

	switch (param) {
	case MDLA_POWER_PARAM_FIX_OPP:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:1, received:%d\n",
					argc);
			goto out;
		}

		switch (args[0]) {
		case 0:
			is_power_debug_lock = false;
			break;
		case 1:
			is_power_debug_lock = true;
			break;
		default:
			if (ret) {
				LOG_ERR("invalid argument, received:%d\n",
						(int)(args[0]));
				goto out;
			}
			ret = -EINVAL;
			goto out;
		}
		break;
	case MDLA_POWER_PARAM_DVFS_DEBUG:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:1, received:%d\n",
					argc);
			goto out;
		}

		ret = args[0] >= opps.count;
		if (ret) {
			LOG_ERR("opp step(%d) is out-of-bound, count:%d\n",
					(int)(args[0]), opps.count);
			goto out;
		}

		opps.vcore.index = opps.vcore.opp_map[args[0]];
		opps.vvpu.index = opps.vvpu.opp_map[args[0]];
		opps.vmdla.index = opps.vmdla.opp_map[args[0]];
		opps.dsp.index = opps.dsp.opp_map[args[0]];
		opps.ipu_if.index = opps.ipu_if.opp_map[args[0]];
		opps.dspcore[0].index = opps.dspcore[0].opp_map[args[0]];
		opps.dspcore[1].index = opps.dspcore[1].opp_map[args[0]];

		is_power_debug_lock = true;

		break;
	case MDLA_POWER_PARAM_JTAG:
		#if 0
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:1, received:%d\n",
					argc);
			goto out;
		}

		is_jtag_enabled = args[0];
		ret = vpu_hw_enable_jtag(is_jtag_enabled);
		#endif
		break;
	case MDLA_POWER_PARAM_LOCK:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:1, received:%d\n",
					argc);
			goto out;
		}

		is_power_debug_lock = args[0];

		break;
	default:
		LOG_ERR("unsupport the power parameter:%d\n", param);
		break;
	}

out:
	return ret;
}

/*dvfs porting --*/
#endif
static int mdla_probe(struct platform_device *pdev)
{
	struct resource *r_irq; /* Interrupt resources */
	struct resource *apu_mdla_command; /* IO mem resources */
	struct resource *apu_mdla_config; /* IO mem resources */
	struct resource *apu_mdla_biu; /* IO mem resources */
	struct resource *apu_mdla_gsm; /* IO mem resources */
	int rc = 0;
	struct device *dev = &pdev->dev;

	mdlactlPlatformDevice = pdev;

	dev_info(dev, "Device Tree Probing\n");

	/* Get iospace for MDLA Config */
	apu_mdla_config = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!apu_mdla_config) {
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}

	/* Get iospace for MDLA Command */
	apu_mdla_command = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!apu_mdla_command) {
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}
	/* Get iospace for MDAL PMU */
	apu_mdla_biu = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!apu_mdla_command) {
		dev_err(dev, "apu_mdla_biu address\n");
		return -ENODEV;
	}

	/* Get iospace GSM */
	apu_mdla_gsm = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (!apu_mdla_gsm) {
		dev_err(dev, "apu_mdla_biu address\n");
		return -ENODEV;
	}

	apu_mdla_config_top = ioremap_nocache(apu_mdla_config->start,
			apu_mdla_config->end - apu_mdla_config->start + 1);
	if (!apu_mdla_config_top) {
		dev_err(dev, "mtk_mdla: Could not allocate iomem\n");
		rc = -EIO;
		return rc;
	}

	apu_mdla_cmde_mreg_top = ioremap_nocache(apu_mdla_command->start,
			apu_mdla_command->end - apu_mdla_command->start + 1);
	if (!apu_mdla_cmde_mreg_top) {
		dev_err(dev, "mtk_mdla: Could not allocate iomem\n");
		rc = -EIO;
		return rc;
	}

	apu_mdla_biu_top = ioremap_nocache(apu_mdla_biu->start,
			apu_mdla_biu->end - apu_mdla_biu->start + 1);
	if (!apu_mdla_biu_top) {
		dev_err(dev, "mtk_mdla: Could not allocate iomem\n");
		rc = -EIO;
		return rc;
	}

	apu_mdla_gsm_top = ioremap_nocache(apu_mdla_gsm->start,
			apu_mdla_gsm->end - apu_mdla_gsm->start + 1);
	if (!apu_mdla_gsm_top) {
		dev_err(dev, "mtk_mdla: Could not allocate iomem\n");
		rc = -EIO;
		return rc;
	}

	/* Get IRQ for the device */
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!r_irq) {
		dev_info(dev, "no IRQ found\n");
		return rc;
	}
	dev_info(dev, "platform_get_resource irq: %d\n", (int)r_irq->start);

	mdla_irq = r_irq->start;
	rc = request_irq(mdla_irq, mdla_interrupt, IRQF_TRIGGER_LOW,
	DRIVER_NAME, dev);
	dev_info(dev, "request_irq\n");
	if (rc) {
		rc = request_irq(mdla_irq, mdla_interrupt, IRQF_TRIGGER_HIGH,
				DRIVER_NAME, dev);
		if (rc) {
			dev_err(dev, "mtk_mdla: Could not allocate interrupt %d.\n",
					mdla_irq);
			return rc;
		}
	}
#ifndef MTK_MDLA_FPGA_PORTING
	mdla_init_hw(0, pdev);
#endif
	dev_info(dev, "apu_mdla_config_top at 0x%08lx mapped to 0x%08lx\n",
			(unsigned long __force)apu_mdla_config->start,
			(unsigned long __force)apu_mdla_config->end);
	dev_info(dev, "apu_mdla_command at 0x%08lx mapped to 0x%08lx, irq=%d\n",
			(unsigned long __force)apu_mdla_command->start,
			(unsigned long __force)apu_mdla_command->end,
			(int)r_irq->start);
	dev_info(dev, "apu_mdla_biu_top at 0x%08lx mapped to 0x%08lx\n",
			(unsigned long __force)apu_mdla_biu->start,
			(unsigned long __force)apu_mdla_biu->end);

	mdla_ion_init();

	return 0;

}

static int mdla_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
#ifndef MTK_MDLA_FPGA_PORTING
	mdla_uninit_hw();
#endif
	free_irq(mdla_irq, dev);
	iounmap(apu_mdla_config_top);
	iounmap(apu_mdla_cmde_mreg_top);
	iounmap(apu_mdla_biu_top);
	iounmap(apu_mdla_gsm_top);
	mdla_ion_exit();
	platform_set_drvdata(pdev, NULL);

	return 0;
}
static const struct of_device_id mdla_of_match[] = {
	{ .compatible = "mediatek,mdla", },
	{ .compatible = "mtk,mdla", },
	{ /* end of list */},
};

MODULE_DEVICE_TABLE(of, mdla_of_match);
static struct platform_driver mdla_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mdla_of_match,
	},
	.probe = mdla_probe,
	.remove = mdla_remove,
};

static int mdlactl_init(void)
{
	int ret;

	ret = platform_driver_register(&mdla_driver);
	if (ret != 0)
		return ret;

	numberOpens = 0;
	cmd_id = 1;
	max_cmd_id = 1;
	cmd_list_len = 0;

	// Try to dynamically allocate a major number for the device
	//  more difficult but worth it
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	/* TODO: replace with alloc_chrdev_region() and
	 * unregister_chrdev_region()
	 * see examples at drivers/misc/mediatek/vpu/1.0/ vpu_reg_chardev,
	 * vpu_unreg_chardev
	 */

	if (majorNumber < 0) {
		pr_warn("MDLA failed to register a major number\n");
		return majorNumber;
	}
	mdla_debug("MDLA: registered correctly with major number %d\n",
			majorNumber);

	// Register the device class
	mdlactlClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(mdlactlClass)) {  // Check for error and clean up if there is
		unregister_chrdev(majorNumber, DEVICE_NAME);
		pr_warn("Failed to register device class\n");
		return PTR_ERR(mdlactlClass);
	}
	// Register the device driver
	mdlactlDevice = device_create(mdlactlClass, NULL, MKDEV(majorNumber, 0),
	NULL, DEVICE_NAME);
	if (IS_ERR(mdlactlDevice)) {  // Clean up if there is an error
		unregister_chrdev(majorNumber, DEVICE_NAME);
		pr_warn("Failed to create the device\n");
		return PTR_ERR(mdlactlDevice);
	}

	// Init DMA from of
	of_dma_configure(mdlactlDevice, NULL);

	// Set DMA mask
	if (dma_get_mask(mdlactlDevice) != DMA_BIT_MASK(32)) {
		ret = dma_set_mask_and_coherent(mdlactlDevice,
					DMA_BIT_MASK(32));
		if (ret)
			pr_warn("MDLA: set DMA mask failed: %d\n", ret);
	}

	mdla_reset();
	mdla_dump_reg();

	pmu_init();
	INIT_WORK(&mdla_queue, mdla_start_queue);
	mdla_debugfs_init();

	return 0;
}

static int mdla_open(struct inode *inodep, struct file *filep)
{
	numberOpens++;
	mdla_debug("MDLA: Device has been opened %d time(s)\n", numberOpens);

	return 0;
}

static int mdla_release(struct inode *inodep, struct file *filep)
{
	mdla_debug("MDLA: Device successfully closed\n");

	return 0;
}

static int mdla_wait_command(u32 id)
{
	mdla_debug("%s: enter id:[%d]\n", __func__, id);
	// @suppress("Type cannot be resolved")
	wait_event_interruptible(mdla_cmd_queue, max_cmd_id >= id);
	if ((id == fail_cmd_id0) ||	(id == fail_cmd_id1)) {
		mdla_debug("mdla_wait_command exit <timeout> [%d]\n", id);
		return -EIO;
	}

	mdla_debug("%s; exit\n", __func__);

	return 0;
}

static void mdla_mark_command_id(struct command_entry *run_cmd_data)
{
	int i;
	u32 count = run_cmd_data->count;
	u32 *cmd = run_cmd_data->kva;

	run_cmd_data->id = cmd_id;

	/* Patch Command buffer 0x150
	 * CODA: 0x150,	mreg_cmd_swcmd_id, 32, 31, 0, mreg_cmd_swcmd_id,
	 * RW, PRIVATE, 32'h0,, This SW command ID
	 */

	for (i = 0; i < count; i++) {
		mdla_debug("%s: add command: %d\n", __func__, cmd_id);
		cmd[84 + (i * 96)] = cmd_id++;
	}

	// TODO: remove debug logs
	mdla_debug("%s: kva=%p, mva=0x%08x, phys_to_dma=%p\n",
			__func__,
			run_cmd_data->kva,
			run_cmd_data->mva,
			phys_to_dma(mdlactlDevice, run_cmd_data->mva));

#ifdef CONFIG_MTK_MDLA_ION
	if (run_cmd_data->khandle) {
		mdla_ion_sync(run_cmd_data->khandle);
		return;
	}
#endif
	dma_sync_single_for_device(mdlactlDevice,
			phys_to_dma(mdlactlDevice, run_cmd_data->mva),
			count * 0x180, DMA_TO_DEVICE);
}

static int mdla_process_command(struct command_entry *run_cmd_data)
{
	dma_addr_t addr = run_cmd_data->mva;
	u32 count = run_cmd_data->count;
	u32 *cmd = (u32 *) run_cmd_data->kva;
	int ret = 0;
	mdla_debug("%s: id: [%d], count: [%d]\n", cmd[84],
		__func__, run_cmd_data->count);
#ifndef MTK_MDLA_FPGA_PORTING
	/* step1, enable clocks and boot-up if needed */
	ret = mdla_get_power(0);
	if (ret) {
		LOG_ERR("[mdla] fail to get power!\n");
		return ret;
	}
#endif
	if (mdla_timeout) {
		// TODO: remove debug
		mdla_debug("%s: mod_timer(), cmd id=%u, [%u]\n",
			__func__, run_cmd_data->id, cmd[84]);
		mod_timer(&mdla_timer,
			jiffies + msecs_to_jiffies(mdla_timeout));
	}

	/* Issue command */
	mdla_reg_write(addr, MREG_TOP_G_CDMA1);
	mdla_reg_write(count, MREG_TOP_G_CDMA2);
	mdla_reg_write(run_cmd_data->id, MREG_TOP_G_CDMA3);
/*Todo mdla_put_power after cmd end */
	return ret;
}

static int mdla_run_command_async(struct ioctl_run_cmd *run_cmd_data)
{
	u32 id;
	struct command_entry *cmd = kmalloc(sizeof(struct command_entry),
	GFP_KERNEL);

	cmd->mva = run_cmd_data->mva;

	if (is_gsm_addr(cmd->mva))
		cmd->kva = gsm_mva_to_virt(cmd->mva);
	else if (run_cmd_data->khandle) {
		cmd->kva = (void *)run_cmd_data->kva;
		mdla_debug("%s: <ion> kva=%p, mva=%08x\n",
				__func__,
				cmd->kva,
				cmd->mva);
	} else {
		cmd->kva = phys_to_virt(cmd->mva);
		mdla_debug("%s: <dram> kva: phys_to_virt(mva:%08x) = %p\n",
			__func__, cmd->mva, cmd->kva);
	}

	cmd->count = run_cmd_data->count;
	cmd->khandle = run_cmd_data->khandle;
	cmd->type = run_cmd_data->type;

	mutex_lock(&cmd_list_lock);

	/* Increate cmd_id*/
	mdla_mark_command_id(cmd);

	id = cmd_id - 1;
	if (cmd_list_len == 0) {
		mdla_debug("cmd_list_len == 0\n");
		last_cmd_id0 = id;
		last_cmd_id1 = UINT32_MAX;
		mdla_process_command(cmd);
		kfree(cmd);
	} else if (cmd_list_len == 1) {
		mdla_debug("cmd_list_len == 1\n");
		last_cmd_id1 = id;
		mdla_process_command(cmd);
		kfree(cmd);
	} else {
		mdla_debug("cmd_list_len == %d\n", cmd_list_len);
		list_add_tail(&cmd->list, &cmd_list);
	}
	cmd_list_len++;
	mutex_unlock(&cmd_list_lock);

	return id;
}

static void mdla_dram_alloc(struct ioctl_malloc *malloc_data)
{
	dma_addr_t phyaddr = 0;

	malloc_data->kva = dma_alloc_coherent(mdlactlDevice, malloc_data->size,
			&phyaddr, GFP_KERNEL);
	malloc_data->mva = dma_to_phys(mdlactlDevice, phyaddr);
	mdla_debug("%s: kva:%p, mva:%x\n", // TODO: remove debug
		__func__, malloc_data->kva, malloc_data->mva);
}

static void mdla_dram_free(struct ioctl_malloc *malloc_data)
{
	mdla_debug("%s: kva:%p, mva:%x\n", // TODO: remove debug
		__func__, malloc_data->kva, malloc_data->mva);
	dma_free_coherent(mdlactlDevice, malloc_data->size,
			(void *) malloc_data->kva, malloc_data->mva);
}

static void mdla_gsm_alloc(struct ioctl_malloc *malloc_data)
{
	malloc_data->kva = gsm_alloc(malloc_data->size);
	malloc_data->mva = gsm_virt_to_mva(malloc_data->kva);
	mdla_debug("%s: kva:%p, mva:%x\n", // TODO: remove debug
		__func__, malloc_data->kva, malloc_data->mva);
}
static void mdla_gsm_free(struct ioctl_malloc *malloc_data)
{
	if (malloc_data->kva)
		gsm_release(malloc_data->kva, malloc_data->size);
	mdla_debug("%s: kva:%p, mva:%x\n", // TODO: remove debug
		__func__, malloc_data->kva, malloc_data->mva);
}

static long mdla_ioctl(struct file *filp, unsigned int command,
		unsigned long arg)
{
	long retval;
	struct ioctl_malloc malloc_data;
	struct ioctl_run_cmd run_cmd_data;
	struct ioctl_perf perf_data;
	u32 id;

	switch (command) {
	case IOCTL_MALLOC:

		if (copy_from_user(&malloc_data, (void *) arg,
			sizeof(malloc_data))) {
			retval = -EFAULT;
			return retval;
		}

		if (malloc_data.type == MEM_DRAM)
			mdla_dram_alloc(&malloc_data);
		else if (malloc_data.type == MEM_GSM)
			mdla_gsm_alloc(&malloc_data);

		if (copy_to_user((void *) arg, &malloc_data,
			sizeof(malloc_data)))
			return -EFAULT;
		mdla_debug("ioctl_malloc size:[%x] pva:[%x] kva:[%p]\n",
				malloc_data.size,
				malloc_data.mva,
				malloc_data.kva);
		if (malloc_data.kva == NULL)
			return -ENOMEM;
		break;
	case IOCTL_FREE:

		if (copy_from_user(&malloc_data, (void *) arg,
				sizeof(malloc_data))) {
			retval = -EFAULT;
			return retval;
		}

		if (malloc_data.type == MEM_DRAM)
			mdla_dram_free(&malloc_data);
		else if (malloc_data.type == MEM_GSM)
			mdla_gsm_free(&malloc_data);
		mdla_debug("ioctl_free size:[%x] pa:[%x] pva:[%p]\n",
				malloc_data.size, malloc_data.mva,
				malloc_data.kva);
		break;
	case IOCTL_RUN_CMD_SYNC:
		if (copy_from_user(&run_cmd_data, (void *) arg,
				sizeof(run_cmd_data))) {
			retval = -EFAULT;
			return retval;
		}

		mdla_debug("%s: RUN_CMD_SYNC: kva=%p, mva=0x%08x, phys_to_virt=%p\n",
				__func__,
				(void *)run_cmd_data.kva,
				run_cmd_data.mva,
				phys_to_virt(run_cmd_data.mva));
		id = mdla_run_command_async(&run_cmd_data);
		retval = mdla_wait_command(id);
		return retval;

	case IOCTL_RUN_CMD_ASYNC:
		if (copy_from_user(&run_cmd_data, (void *) arg,
				sizeof(run_cmd_data))) {
			retval = -EFAULT;
			return retval;
		}

		mdla_debug("%s: RUN_CMD_ASYNC: kva=%p, mva=0x%08x, phys_to_virt=%p\n",
				__func__,
				(void *)run_cmd_data.kva,
				run_cmd_data.mva,
				phys_to_virt(run_cmd_data.mva));
		id = mdla_run_command_async(&run_cmd_data);
		run_cmd_data.id = id;
		if (copy_to_user((void *) arg, &run_cmd_data,
			sizeof(run_cmd_data)))
			return -EFAULT;
		break;
	case IOCTL_WAIT_CMD:
		if (copy_from_user(&run_cmd_data, (void *) arg,
				sizeof(run_cmd_data))) {
			retval = -EFAULT;
			return retval;
		}
		retval = mdla_wait_command(run_cmd_data.id);
		return retval;

	case IOCTL_PERF_SET_EVENT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.handle = pmu_set_perf_event(
			perf_data.interface, perf_data.event);
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_GET_EVENT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.event = pmu_get_perf_event(perf_data.handle);
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_GET_CNT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.counter = pmu_get_perf_counter(perf_data.handle);

		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;

	case IOCTL_PERF_UNSET_EVENT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		pmu_unset_perf_event(perf_data.handle);
		break;
	case IOCTL_PERF_GET_START:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.start = pmu_get_perf_start();
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_GET_END:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.end = pmu_get_perf_end();
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_GET_CYCLE:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.start = pmu_get_perf_cycle();
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_RESET_CNT:
		pmu_reset_counter();
		break;
	case IOCTL_PERF_RESET_CYCLE:
		pmu_reset_cycle();
		break;
	case IOCTL_PERF_SET_MODE:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		pmu_perf_set_mode(perf_data.mode);
		break;
	case IOCTL_ION_KMAP:
		return mdla_ion_kmap(arg);
	case IOCTL_ION_KUNMAP:
		return mdla_ion_kunmap(arg);
	default:
		return -EINVAL;
	}
	return 0;
}

static void mdlactl_exit(void)
{
	mdla_debugfs_exit();
	platform_driver_unregister(&mdla_driver);
	device_destroy(mdlactlClass, MKDEV(majorNumber, 0));
	class_destroy(mdlactlClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	mdla_debug("MDLA: Goodbye from the LKM!\n");
}

module_init(mdlactl_init);
module_exit(mdlactl_exit);
