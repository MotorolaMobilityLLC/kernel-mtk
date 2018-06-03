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

#include <mt-plat/sync_write.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <asm/div64.h>
#include <mtk_dramc.h>

#include "emi_mbw.h"
#include "emi_bwl.h"

static bool dump_latency_status;

unsigned long long last_time_ns;
long long LastWordAllCount;

#ifdef ENABLE_RUNTIME_BM
#define RUNTIME_PERIOD	1
static struct timer_list bm_timer;
static unsigned int bm_bw_total;
static unsigned int bm_bw_apmcu;
static unsigned int bm_bw_mdmcu;
static unsigned int bm_bw_mm;
static unsigned int bm_cycle_cnt;
static unsigned int bm_lat_apmcu0;
static unsigned int bm_lat_apmcu1;
static unsigned int bm_lat_mm0;
static unsigned int bm_lat_mm1;
static unsigned int bm_lat_mdmcu;
static unsigned int bm_lat_mdhw;
static unsigned int bm_lat_peri;
static unsigned int bm_lat_gpu;
static unsigned int bm_cnt_apmcu0;
static unsigned int bm_cnt_apmcu1;
static unsigned int bm_cnt_mm0;
static unsigned int bm_cnt_mm1;
static unsigned int bm_cnt_mdmcu;
static unsigned int bm_cnt_mdhw;
static unsigned int bm_cnt_peri;
static unsigned int bm_cnt_gpu;
static unsigned int bm_data_rate;

void del_bm_timer(void)
{
	del_timer_sync(&bm_timer);
}

void add_bm_timer(void)
{
	mod_timer(&bm_timer, jiffies + msecs_to_jiffies(RUNTIME_PERIOD));
}

void bm_timer_callback(unsigned long data)
{
	int emi_dcm_status;
	void __iomem *CEN_EMI_BASE;

	CEN_EMI_BASE = mt_cen_emi_base_get();

	if (CEN_EMI_BASE == NULL) {
		mod_timer(&bm_timer, jiffies + msecs_to_jiffies(RUNTIME_PERIOD));
		return;
	}

	if (!is_dump_latency()) {
		emi_dcm_status = BM_GetEmiDcm();
		BM_SetEmiDcm(0xff);
		writel(readl(EMI_BMEN) & 0xfffffffc, EMI_BMEN);
		BM_SetEmiDcm(emi_dcm_status);
		return;
	}

	/* backup and disable EMI DCM */
	emi_dcm_status = BM_GetEmiDcm();
	BM_SetEmiDcm(0xff);

	/* pause BM */
	writel(readl(EMI_BMEN) | 0x2, EMI_BMEN);

	/* get BM result */
	bm_data_rate = get_dram_data_rate();
	bm_cycle_cnt = readl(EMI_BCNT);
	bm_bw_total = readl(EMI_WSCT);
	bm_bw_apmcu = readl(EMI_WSCT2);
	bm_bw_mm = readl(EMI_WSCT3);
	bm_bw_mdmcu = readl(EMI_WSCT4);

	bm_lat_apmcu0 = readl(EMI_TTYPE1);
	bm_lat_apmcu1 = readl(EMI_TTYPE2);
	bm_lat_mm1 = readl(EMI_TTYPE3);
	bm_lat_mdmcu = readl(EMI_TTYPE4);
	bm_lat_mdhw = readl(EMI_TTYPE5);
	bm_lat_mm0 = readl(EMI_TTYPE6);
	bm_lat_peri = readl(EMI_TTYPE7);
	bm_lat_gpu = readl(EMI_TTYPE8);

	bm_cnt_apmcu0 = readl(EMI_TTYPE9);
	bm_cnt_apmcu1 = readl(EMI_TTYPE10);
	bm_cnt_mm1 = readl(EMI_TTYPE11);
	bm_cnt_mdmcu = readl(EMI_TTYPE12);
	bm_cnt_mdhw = readl(EMI_TTYPE13);
	bm_cnt_mm0 = readl(EMI_TTYPE14);
	bm_cnt_peri = readl(EMI_TTYPE15);
	bm_cnt_gpu = readl(EMI_TTYPE16);

#if 0
	pr_debug("[EMI BM] bm_data_rate %d\n", bm_data_rate);
	pr_debug("[EMI BM] bm_cycle_cnt %d\n", bm_cycle_cnt);
	pr_debug("[EMI BM] bm_bw_total %d\n", bm_bw_total);
	pr_debug("[EMI BM] bm_bw_apmcu %d\n", bm_bw_apmcu);
	pr_debug("[EMI BM] bm_bw_mm %d\n", bm_bw_mm);
	pr_debug("[EMI BM] bm_bw_mdmcu %d\n", bm_bw_mdmcu);
	pr_debug("[EMI BM] bm_lat/cnt_apmcu0 %d/%d\n", bm_lat_apmcu0, bm_cnt_apmcu0);
	pr_debug("[EMI BM] bm_lat/cnt_apmcu1 %d/%d\n", bm_lat_apmcu1, bm_cnt_apmcu1);
	pr_debug("[EMI BM] bm_lat/cnt_mm0 %d/%d\n", bm_lat_mm0, bm_cnt_mm0);
	pr_debug("[EMI BM] bm_lat/cnt_mm1 %d/%d\n", bm_lat_mm1, bm_cnt_mm1);
	pr_debug("[EMI BM] bm_lat/cnt_mdmcu %d/%d\n", bm_lat_mdmcu, bm_cnt_mdmcu);
	pr_debug("[EMI BM] bm_lat/cnt_mdhw %d/%d\n", bm_lat_mdhw, bm_cnt_mdhw);
	pr_debug("[EMI BM] bm_lat/cnt_peri %d/%d\n", bm_lat_peri, bm_cnt_peri);
	pr_debug("[EMI BM] bm_lat/cnt_gpu %d/%d\n", bm_lat_gpu, bm_cnt_gpu);
#endif

	/* clear BM */
	writel(readl(EMI_BMEN) & 0xfffffffc, EMI_BMEN);

	/* setup BM */
	writel(0x00ff0000, EMI_BMEN);
	writel(0x00240003, EMI_MSEL);
	writel(0x00000018, EMI_MSEL2);
	writel(0x02000000, EMI_BMEN2);
	writel(0xffffffff, EMI_BMRW0);

	/* enable BM */
	writel(readl(EMI_BMEN) | 0x1, EMI_BMEN);

	/* restore EMI DCM */
	BM_SetEmiDcm(emi_dcm_status);

	mod_timer(&bm_timer, jiffies + msecs_to_jiffies(RUNTIME_PERIOD));
}
#endif

void dump_last_bm(char *buf, unsigned int leng)
{
#ifdef ENABLE_RUNTIME_BM
	snprintf(buf, leng,
		"[EMI BM] ddr data rate: %d\n"
		"EMI_BCNT: 0x%x\n"
		"EMI_WSCT(total): 0x%x\n"
		"EMI_WSCT2(apmcu): 0x%x\n"
		"EMI_WSCT3(mm): 0x%x\n"
		"EMI_WSCT4(mdmcu): 0x%x\n"
		"EMI_TTYPE1(lat_apmcu1): 0x%x\n"
		"EMI_TTYPE2(lat_apmcu2): 0x%x\n"
		"EMI_TTYPE3(lat_mm1): 0x%x\n"
		"EMI_TTYPE4(lat_mdmcu): 0x%x\n"
		"EMI_TTYPE5(lat_mdhw): 0x%x\n"
		"EMI_TTYPE6(lat_mm0): 0x%x\n"
		"EMI_TTYPE7(lat_peri): 0x%x\n"
		"EMI_TTYPE8(lat_gpu): 0x%x\n"
		"EMI_TTYPE9(cnt_apmcu0): 0x%x\n"
		"EMI_TTYPE8(cnt_apmcu1): 0x%x\n"
		"EMI_TTYPE8(cnt_mm1): 0x%x\n"
		"EMI_TTYPE8(cnt_mdmcu): 0x%x\n"
		"EMI_TTYPE8(cnt_mdhw): 0x%x\n"
		"EMI_TTYPE8(cnt_mm0): 0x%x\n"
		"EMI_TTYPE8(cnt_peri): 0x%x\n"
		"EMI_TTYPE8(cnt_gpu): 0x%x\n",
		bm_data_rate, bm_cycle_cnt, bm_bw_total, bm_bw_apmcu, bm_bw_mm, bm_bw_mdmcu,
		bm_lat_apmcu0, bm_lat_apmcu1, bm_lat_mm1, bm_lat_mdmcu, bm_lat_mdhw,
		bm_lat_mm0, bm_lat_peri, bm_lat_gpu, bm_cnt_apmcu0, bm_cnt_apmcu1,
		bm_cnt_mm1, bm_cnt_mdmcu, bm_cnt_mdhw, bm_cnt_mm0, bm_cnt_peri, bm_cnt_gpu);
#else
	snprintf(buf, leng, "[EMI BM] disable runtime BM\n");
#endif
}

/***********************************************
 * register / unregister g_pGetMemBW CB
 ***********************************************/
static getmembw_func g_pGetMemBW; /* not initialise statics to 0 or NULL */

void mt_getmembw_registerCB(getmembw_func pCB)
{
	if (pCB == NULL) {
		/* reset last time & word all count */
		last_time_ns = sched_clock();
		LastWordAllCount = 0;
		pr_err("[get_mem_bw] register CB is a null function\n");
	}	else {
		pr_err("[get_mem_bw] register CB successful\n");
	}

	g_pGetMemBW = pCB;
}
EXPORT_SYMBOL(mt_getmembw_registerCB);

unsigned long long get_mem_bw(void)
{
	unsigned long long throughput;
	long long WordAllCount;
	unsigned long long current_time_ns, time_period_ns;
	int count;
	long long value;
	int emi_dcm_status;

#if DISABLE_FLIPPER_FUNC
	return 0;
#endif

	if (g_pGetMemBW)
		return g_pGetMemBW();

	emi_dcm_status = BM_GetEmiDcm();
	/* pr_err("[get_mem_bw]emi_dcm_status = %d\n", emi_dcm_status); */
	current_time_ns = sched_clock();
	time_period_ns = current_time_ns - last_time_ns;
	/* pr_err("[get_mem_bw]last_time=%llu, current_time=%llu,
	 * period=%llu\n", last_time_ns, current_time_ns, time_period_ns);
	 */

	/* disable_infra_dcm(); */
	BM_SetEmiDcm(0xff);	/* disable EMI dcm */

	BM_Pause();
	WordAllCount = BM_GetWordAllCount();
	if (WordAllCount == 0) {
		LastWordAllCount = 0;
	} else if (WordAllCount == BM_ERR_OVERRUN) {
		pr_debug("[get_mem_bw] BM_ERR_OVERRUN\n");
		WordAllCount = 0;
		LastWordAllCount = 0;
		BM_Enable(0);	/* stop EMI monitors will reset all counters */
		BM_Enable(1);	/* start EMI monitor counting */
	}

	WordAllCount -= LastWordAllCount;
	throughput = (WordAllCount * 8 * 1000);

	if (time_period_ns >= 0xFFFFFFFF) { /* uint32_t overflow */
		do_div(time_period_ns, 10000000);
		do_div(throughput, 10000000);
		pr_debug("[get_mem_bw] time_period_ns overflow lst\n");
		/* pr_err("[get_mem_bw] time_period_ns overflow 1st\n"); */

		if (time_period_ns >= 0xFFFFFFFF) { /* uint32_t overflow */
			do_div(time_period_ns, 1000);
			do_div(throughput, 1000);
			pr_debug("[get_mem_bw] time_period_ns overflow 2nd\n");
			/* pr_err("[get_mem_bw] time_period overflow 2nd\n"); */
		}
	}

	do_div(throughput, time_period_ns);
	/* pr_err("[get_mem_bw]Total MEMORY THROUGHPUT =%llu(MB/s),
	 * WordAllCount_delta = 0x%llx, LastWordAllCount = 0x%llx\n",
	 * throughput, WordAllCount, LastWordAllCount);
	 */

	/* stopping EMI monitors will reset all counters */
	BM_Enable(0);

	value = BM_GetWordAllCount();
	count = 100;
	if ((value != 0) && (value > 0xB0000000)) {
		do {
			value = BM_GetWordAllCount();
			if (value != 0) {
				count--;
				BM_Enable(1);
				BM_Enable(0);
			} else
				break;

		} while (count > 0);
	}
	LastWordAllCount = value;

	/* pr_err("[get_mem_bw]loop count = %d, last_word_all_count = 0x%llx\n", count, LastWordAllCount); */

	/* start EMI monitor counting */
	BM_Enable(1);
	last_time_ns = sched_clock();

	/* restore_infra_dcm();*/
	BM_SetEmiDcm(emi_dcm_status);

	/*pr_err("[get_mem_bw]throughput = %llx\n", throughput);*/

	return throughput;
}

static int mem_bw_suspend_callback(struct device *dev)
{
	/*pr_err("[get_mem_bw]mem_bw_suspend_callback\n");*/
	LastWordAllCount = 0;
	BM_Pause();
	return 0;
}

static int mem_bw_resume_callback(struct device *dev)
{
	/* pr_err("[get_mem_bw]mem_bw_resume_callback\n"); */
	BM_Continue();
	return 0;
}

const struct dev_pm_ops mt_mem_bw_pm_ops = {
	.suspend = mem_bw_suspend_callback,
	.resume = mem_bw_resume_callback,
	.restore_early = NULL,
};

struct platform_device mt_mem_bw_pdev = {
	.name = "mt-mem_bw",
	.id = -1,
};

static struct platform_driver mt_mem_bw_pdrv = {
	.probe = NULL,
	.remove = NULL,
	.driver = {
		   .name = "mt-mem_bw",
		   .pm = &mt_mem_bw_pm_ops,
		   .owner = THIS_MODULE,
		   },
};

int mbw_init(void)
{
	int ret = 0;

	BM_Init();

	/* register platform device/driver */
	ret = platform_device_register(&mt_mem_bw_pdev);
	if (ret) {
		pr_err("fail to register mem_bw device @ %s()\n", __func__);
		goto out;
	}

	ret = platform_driver_register(&mt_mem_bw_pdrv);
	if (ret) {
		pr_err("fail to register mem_bw driver @ %s()\n", __func__);
		platform_device_unregister(&mt_mem_bw_pdev);
	}

	enable_dump_latency();

out:
	return ret;
}

void enable_dump_latency(void)
{
	dump_latency_status = true;
}

void disable_dump_latency(void)
{
	dump_latency_status = false;
}

bool is_dump_latency(void)
{
	return dump_latency_status;
}

static int __init dvfs_bwct_init(void)
{
	unsigned int dram_type;
	unsigned int ch_num;

	dram_type = get_dram_type();
	ch_num = get_ch_num();

	pr_err("[EMI] set BWCT for DRAM type(%d), ch(%d)\n",
		dram_type, ch_num);

	switch (ch_num) {
	case 1:
		BM_SetBW(0x05000405);
		break;
	case 2:
		BM_SetBW(0x0a000705);
		break;
	default:
		pr_err("[EMI] unsupported channel number\n");
		return -1;
	}

	return 0;
}

late_initcall(dvfs_bwct_init);

#ifdef ENABLE_RUNTIME_BM
static int __init runtime_bm_init(void)
{
	setup_deferrable_timer_on_stack(&bm_timer, bm_timer_callback, 0);
	if (mod_timer(&bm_timer, jiffies + msecs_to_jiffies(RUNTIME_PERIOD)))
		pr_debug("[EMI MBW] Error in BM mod_timer\n");

	return 0;
}
late_initcall(runtime_bm_init);
#endif
