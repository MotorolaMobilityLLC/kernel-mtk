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

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mt-plat/mtk_gpt.h>
#include <linux/io.h>
#include <linux/scatterlist.h>

#ifndef FPGA_PLATFORM
#include <mt-plat/upmu_common.h>
#endif
#ifdef MTK_MSDC_BRINGUP_DEBUG
#include <mach/mtk_pmic_wrap.h>
#endif

#include "mtk_sd.h"
#include <mmc/core/core.h>
#include "dbg.h"
#include "autok_dvfs.h"

#ifdef CONFIG_MTK_ENG_BUILD
#define MTK_EMMC_CQ_DEBUG
#endif

#ifdef MTK_IO_PERFORMANCE_DEBUG
unsigned int g_mtk_mmc_perf_dbg;
unsigned int g_mtk_mmc_dbg_range;
unsigned int g_dbg_range_start;
unsigned int g_dbg_range_end;
unsigned int g_mtk_mmc_dbg_flag;
unsigned int g_dbg_req_count;
unsigned int g_dbg_raw_count;
unsigned int g_dbg_write_count;
unsigned int g_dbg_raw_count_old;
unsigned int g_mtk_mmc_clear;
int g_check_read_write;
int g_i;
unsigned long long g_req_buf[4000][30] = { {0} };
unsigned long long g_req_write_buf[4000][30] = { {0} };
unsigned long long g_req_write_count[4000] = { 0 };

unsigned long long g_mmcqd_buf[400][300] = { {0} };

char *g_time_mark[] = {
	"--start fetch request",
	"--end fetch request",
	"--start dma map this request",
	"--end dma map this request",
	"--start request",
	"--DMA start",
	"--DMA transfer done",
	"--start dma unmap request",
	"--end dma unmap request",
	"--end of request",
};

char *g_time_mark_vfs_write[] = {
	"--in vfs_write",
	"--before generic_segment_checks",
	"--after generic_segment_checks",
	"--after vfs_check_frozen",
	"--after generic_write_checks",
	"--after file_remove_suid",
	"--after file_update_time",
	"--after generic_file_direct_write",
	"--after generic_file_buffered_write",
	"--after filemap_write_and_wait_range",
	"--after invalidate_mapping_pages",
	"--after 2nd generic_file_buffered_write",
	"--before generic_write_sync",
	"--after generic_write_sync",
	"--out vfs_write"
};
#endif

/* for get transfer time with each trunk size, default not open */
#ifdef MTK_MMC_PERFORMANCE_TEST
unsigned int g_mtk_mmc_perf_test;
#endif

/* for a type command, e.g. CMD53, 2 blocks */
struct cmd_profile {
	u32 max_tc;             /* Max tick count */
	u32 min_tc;
	u32 tot_tc;             /* total tick count */
	u32 tot_bytes;
	u32 count;              /* the counts of the command */
};

/* dump when total_tc and total_bytes */
struct sdio_profile {
	u32 total_tc;           /* total tick count of CMD52 and CMD53 */
	u32 total_tx_bytes;     /* total bytes of CMD53 Tx */
	u32 total_rx_bytes;     /* total bytes of CMD53 Rx */

	/*CMD52 */
	struct cmd_profile cmd52_tx;
	struct cmd_profile cmd52_rx;

	/*CMD53 in byte unit */
	struct cmd_profile cmd53_tx_byte[512];
	struct cmd_profile cmd53_rx_byte[512];

	/*CMD53 in block unit */
	struct cmd_profile cmd53_tx_blk[100];
	struct cmd_profile cmd53_rx_blk[100];
};

static char cmd_buf[256];

/* for driver profile */
#define TICKS_ONE_MS  (1000000)
u32 sdio_pro_enable;
static unsigned long long sdio_pro_time = 30;	/* no more than 30s */
static unsigned long long sdio_profiling_start;
struct sdio_profile sdio_perfomance = { 0 };

/*#define MTK_MSDC_ERROR_TUNE_DEBUG*/

#ifdef MSDC_DMA_ADDR_DEBUG
struct dma_addr msdc_latest_dma_address[MAX_BD_PER_GPD];

static struct dma_addr *msdc_get_dma_address(int host_id)
{
	struct bd_t *bd;
	int i = 0;
	int mode = -1;
	struct msdc_host *host;
	void __iomem *base;

	if (host_id < 0 || host_id >= HOST_MAX_NUM || !mtk_msdc_host[host_id]) {
		pr_err("[%s] invalid host_id %d\n", __func__, host_id);
		return NULL;
	}

	host = mtk_msdc_host[host_id];
	base = host->base;
	/* spin_lock(&host->lock); */
	MSDC_GET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, mode);
	if (mode == 1) {
		pr_crit("Desc.DMA\n");
		bd = host->dma.bd;
		i = 0;
		while (i < MAX_BD_PER_GPD) {
			msdc_latest_dma_address[i].start_address =
				(u32) bd[i].ptr;
			msdc_latest_dma_address[i].size = bd[i].buflen;
			msdc_latest_dma_address[i].end = bd[i].eol;
			if (i > 0)
				msdc_latest_dma_address[i - 1].next =
					&msdc_latest_dma_address[i];

			if (bd[i].eol)
				break;
			i++;
		}
	} else if (mode == 0) {
		pr_crit("Basic DMA\n");
		msdc_latest_dma_address[0].start_address =
			MSDC_READ32(MSDC_DMA_SA);
		msdc_latest_dma_address[0].size = MSDC_READ32(MSDC_DMA_LEN);
		msdc_latest_dma_address[0].end = 1;
	}
	/* spin_unlock(&host->lock); */

	return msdc_latest_dma_address;

}

static void msdc_init_dma_latest_address(void)
{
	struct dma_addr *ptr, *prev;
	int bdlen = MAX_BD_PER_GPD;

	memset(msdc_latest_dma_address, 0, sizeof(struct dma_addr) * bdlen);
	ptr = msdc_latest_dma_address + bdlen - 1;
	while (ptr != msdc_latest_dma_address) {
		prev = ptr - 1;
		prev->next = (void *)(msdc_latest_dma_address
			+ (ptr - msdc_latest_dma_address));
		ptr = prev;
	}

}
#endif

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
#if 0 /* def MTK_EMMC_CQ_DEBUG */
#define dbg_max_cnt (500)
struct dbg_run_host_log {
	unsigned long long time_sec;
	unsigned long long time_usec;
	int type;
	int cmd;
	int arg;
};
static struct dbg_run_host_log dbg_run_host_log_dat[dbg_max_cnt];
static int dbg_host_cnt;
static int is_lock_init;
static spinlock_t cmd_dump_lock;

static unsigned int printk_cpu_test = UINT_MAX;
struct timeval cur_tv;

void dbg_add_host_log(struct mmc_host *mmc, int type, int cmd, int arg)
{
	unsigned long long t;
	unsigned long long nanosec_rem;
	unsigned long flags;

	if (!is_lock_init) {
		spin_lock_init(&cmd_dump_lock);
		is_lock_init = 1;
	}

	spin_lock_irqsave(&cmd_dump_lock, flags);
	t = cpu_clock(printk_cpu_test);
	nanosec_rem = do_div(t, 1000000000)/1000;
	do_gettimeofday(&cur_tv);
	dbg_run_host_log_dat[dbg_host_cnt].time_sec = t;
	dbg_run_host_log_dat[dbg_host_cnt].time_usec = nanosec_rem;
	dbg_run_host_log_dat[dbg_host_cnt].type = type;
	dbg_run_host_log_dat[dbg_host_cnt].cmd = cmd;
	dbg_run_host_log_dat[dbg_host_cnt].arg = arg;
	dbg_host_cnt++;
	if (dbg_host_cnt >= dbg_max_cnt)
		dbg_host_cnt = 0;
	spin_unlock_irqrestore(&cmd_dump_lock, flags);
}

void mmc_cmd_dump(struct mmc_host *mmc)
{
	int i;
	int tag = -1;
	unsigned long flags;
	struct msdc_host *host;

	if (!mmc || !mmc->card)
		return;

	host = mmc_priv(mmc);

	if (!is_lock_init) {
		spin_lock_init(&cmd_dump_lock);
		is_lock_init = 1;
	}

	pr_err("-------------------------------------------------------------------------------\n");
	spin_lock_irqsave(&cmd_dump_lock, flags);
	for (i = dbg_host_cnt; i < dbg_max_cnt; i++) {
		if (dbg_run_host_log_dat[i].cmd == 44
			&& (dbg_run_host_log_dat[i].type == 0)) {
			tag = (dbg_run_host_log_dat[i].arg >> 16) & 0xf;
			pr_err("%d [%5llu.%06llu]%2d %2d 0x%08x tag=%d type=%s %s %s\n",
				i, dbg_run_host_log_dat[i].time_sec,
				dbg_run_host_log_dat[i].time_usec,
				dbg_run_host_log_dat[i].type,
				dbg_run_host_log_dat[i].cmd,
				dbg_run_host_log_dat[i].arg, tag,
				((dbg_run_host_log_dat[i].arg >> 30) & 0x1) ? "read" : "write",
				!((dbg_run_host_log_dat[i].arg >> 30) & 0x1) &&
				((dbg_run_host_log_dat[i].arg >> 31) & 0x1) ? "rel" : NULL,
				!((dbg_run_host_log_dat[i].arg >> 30) & 0x1) &&
				((dbg_run_host_log_dat[i].arg >> 24) & 0x1) ? "fprg" : NULL
				);
		} else if ((dbg_run_host_log_dat[i].cmd == 46
			|| dbg_run_host_log_dat[i].cmd == 47)
			&& !dbg_run_host_log_dat[i].type) {
			tag = (dbg_run_host_log_dat[i].arg >> 16) & 0xf;
			pr_err("%d [%5llu.%06llu]%2d %2d 0x%08x tag=%d\n",
				i, dbg_run_host_log_dat[i].time_sec,
				dbg_run_host_log_dat[i].time_usec,
				dbg_run_host_log_dat[i].type,
				dbg_run_host_log_dat[i].cmd,
				dbg_run_host_log_dat[i].arg, tag);
		} else
			pr_err("%d [%5llu.%06llu]%2d %2d 0x%08x\n",
				i, dbg_run_host_log_dat[i].time_sec,
				dbg_run_host_log_dat[i].time_usec,
				dbg_run_host_log_dat[i].type,
				dbg_run_host_log_dat[i].cmd,
				dbg_run_host_log_dat[i].arg);
	}

	for (i = 0; i < dbg_host_cnt; i++) {
		if (dbg_run_host_log_dat[i].cmd == 44
			&& !dbg_run_host_log_dat[i].type) {
			tag = (dbg_run_host_log_dat[i].arg >> 16) & 0xf;
			pr_err("%d [%5llu.%06llu]%2d %2d 0x%08x tag=%d type=%s %s %s\n",
				i, dbg_run_host_log_dat[i].time_sec,
				dbg_run_host_log_dat[i].time_usec,
				dbg_run_host_log_dat[i].type,
				dbg_run_host_log_dat[i].cmd,
				dbg_run_host_log_dat[i].arg, tag,
				((dbg_run_host_log_dat[i].arg >> 30) & 0x1) ? "read" : "write",
				!((dbg_run_host_log_dat[i].arg >> 30) & 0x1) &&
				((dbg_run_host_log_dat[i].arg >> 31) & 0x1) ? "rel" : NULL,
				!((dbg_run_host_log_dat[i].arg >> 30) & 0x1) &&
				((dbg_run_host_log_dat[i].arg >> 24) & 0x1) ? "fprg" : NULL
				);
		} else if ((dbg_run_host_log_dat[i].cmd == 46
			|| dbg_run_host_log_dat[i].cmd == 47)
			&& !dbg_run_host_log_dat[i].type) {
			tag = (dbg_run_host_log_dat[i].arg >> 16) & 0xf;
			pr_err("%d [%5llu.%06llu]%2d %2d 0x%08x tag=%d\n",
				i, dbg_run_host_log_dat[i].time_sec,
				dbg_run_host_log_dat[i].time_usec,
				dbg_run_host_log_dat[i].type,
				dbg_run_host_log_dat[i].cmd,
				dbg_run_host_log_dat[i].arg, tag);
		} else
			pr_err("%d [%5llu.%06llu]%2d %2d 0x%08x\n",
				i, dbg_run_host_log_dat[i].time_sec,
				dbg_run_host_log_dat[i].time_usec,
				dbg_run_host_log_dat[i].type,
				dbg_run_host_log_dat[i].cmd,
				dbg_run_host_log_dat[i].arg);
	}
	spin_unlock_irqrestore(&cmd_dump_lock, flags);
	msdc_cmdq_status_print(host);
}
#else
void dbg_add_host_log(struct mmc_host *mmc, int type, int cmd, int arg)
{
}

void mmc_cmd_dump(struct mmc_host *mmc)
{
}
#endif

#ifdef CONFIG_MTK_EMMC_CQ_MET_USR_DEF
void emmc_cq_state(void)
{
	MET_PRINTK("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		dbg_cq_state[0], dbg_cq_state[1], dbg_cq_state[2],
		dbg_cq_state[3], dbg_cq_state[4], dbg_cq_state[5],
		dbg_cq_state[6], dbg_cq_state[7], dbg_cq_state[8],
		dbg_cq_state[9], dbg_cq_state[10], dbg_cq_state[11],
		dbg_cq_state[12], dbg_cq_state[13], dbg_cq_state[14],
		dbg_cq_state[15], dbg_cq_state[16], dbg_cq_state[17],
		dbg_cq_state[18], dbg_cq_state[19], dbg_cq_state[20],
		dbg_cq_state[21], dbg_cq_state[22], dbg_cq_state[23],
		dbg_cq_state[24], dbg_cq_state[25], dbg_cq_state[26],
		dbg_cq_state[27], dbg_cq_state[28], dbg_cq_state[29],
		dbg_cq_state[30], dbg_cq_state[31]);
}

void emmc_cq_state_log(struct mmc_host *mmc, unsigned int idx,
	unsigned int val)
{
	dbg_cq_state[idx] = val;
	emmc_cq_state();
}

void emmc_cq_state_pr(struct mmc_host *mmc, unsigned int idx,
	unsigned int val)
{
	dbg_cq_state[idx] = val;
}
#else
void emmc_cq_state_log(struct mmc_host *mmc, unsigned int idx,
	unsigned int val)
{
}

void emmc_cq_state_pr(struct mmc_host *mmc, unsigned int idx,
	unsigned int val)
{
}
#endif
#endif

void msdc_cmdq_status_print(struct msdc_host *host)
{
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	struct mmc_host *mmc = host->mmc;

	if (!mmc)
		return;

	pr_err("===============================\n");
	pr_err("cmdq support : %s\n",
		mmc->card->ext_csd.cmdq_support ? "yes":"no");
	pr_err("cmdq mode    : %s\n",
		mmc->card->ext_csd.cmdq_mode_en ? "enable" : "disable");
	pr_err("cmdq depth   : %d\n", mmc->card->ext_csd.cmdq_depth);
	pr_err("===============================\n");
	pr_err("areq_cnt     : %d\n", atomic_read(&mmc->areq_cnt));
	pr_err("task_id_index: %08lx\n", mmc->task_id_index);
	pr_err("cq_wait_rdy  : %d\n", atomic_read(&mmc->cq_wait_rdy));
	pr_err("cq_rdy_cnt  : %d\n", atomic_read(&mmc->cq_rdy_cnt));
	pr_err("cq_tuning_now: %d\n", atomic_read(&mmc->cq_tuning_now));

#else
	pr_err("driver not supported\n");
#endif
}

void msdc_cmdq_func(struct msdc_host *host, const int num)
{
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	void __iomem *base;
	int a, b;
#endif

	if (!host || !host->mmc || !host->mmc->card)
		return;

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	base = host->base;
#endif

	switch (num) {
	case 0:
		msdc_cmdq_status_print(host);
		break;
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	case 1:
		pr_err("force enable cmdq\n");
		host->mmc->card->ext_csd.cmdq_support = 1;
		host->mmc->cmdq_support_changed = 1;
		break;
	case 2:
		mmc_cmd_dump(host->mmc);
		break;
	case 3:
		MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, a);
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, a+1);
		MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, b);
		pr_err("force MSDC_PAD_TUNE0_CMDRDLY %d -> %d\n", a, b);
		break;
	case 4:
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, a);
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, a+1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, b);
		pr_err("force MSDC_EMMC50_PAD_DS_TUNE_DLY1 %d -> %d\n", a, b);
		break;
	case 5:
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY2, a);
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY2, a+1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY2, b);
		pr_err("force MSDC_EMMC50_PAD_DS_TUNE_DLY2 %d -> %d\n", a, b);
		break;
	case 6:
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, a);
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, a+1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, b);
		pr_err("force MSDC_EMMC50_PAD_DS_TUNE_DLY3  %d -> %d\n", a, b);
		break;
#endif
	default:
		pr_err("unknown function id %d\n", num);
		break;
	}
}

/* Clone from core/mmc.c since it is static in mmc.c */
static void msdc_select_card_type(struct mmc_host *host)
{
	struct mmc_card *card = host->card;
	u8 card_type = card->ext_csd.raw_card_type;
	u32 caps = host->caps, caps2 = host->caps2;
	unsigned int hs_max_dtr = 0, hs200_max_dtr = 0;
	unsigned int avail_type = 0;

	if (caps & MMC_CAP_MMC_HIGHSPEED &&
	    card_type & EXT_CSD_CARD_TYPE_HS_26) {
		hs_max_dtr = MMC_HIGH_26_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS_26;
	}

	if (caps & MMC_CAP_MMC_HIGHSPEED &&
	    card_type & EXT_CSD_CARD_TYPE_HS_52) {
		hs_max_dtr = MMC_HIGH_52_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS_52;
	}

	if (caps & MMC_CAP_1_8V_DDR &&
	    card_type & EXT_CSD_CARD_TYPE_DDR_1_8V) {
		hs_max_dtr = MMC_HIGH_DDR_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_DDR_1_8V;
	}

	if (caps & MMC_CAP_1_2V_DDR &&
	    card_type & EXT_CSD_CARD_TYPE_DDR_1_2V) {
		hs_max_dtr = MMC_HIGH_DDR_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_DDR_1_2V;
	}

	if (caps2 & MMC_CAP2_HS200_1_8V_SDR &&
	    card_type & EXT_CSD_CARD_TYPE_HS200_1_8V) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS200_1_8V;
	}

	if (caps2 & MMC_CAP2_HS200_1_2V_SDR &&
	    card_type & EXT_CSD_CARD_TYPE_HS200_1_2V) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS200_1_2V;
	}

	if (caps2 & MMC_CAP2_HS400_1_8V &&
	    card_type & EXT_CSD_CARD_TYPE_HS400_1_8V) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS400_1_8V;
	}

	if (caps2 & MMC_CAP2_HS400_1_2V &&
	    card_type & EXT_CSD_CARD_TYPE_HS400_1_2V) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
		avail_type |= EXT_CSD_CARD_TYPE_HS400_1_2V;
	}

	card->ext_csd.hs_max_dtr = hs_max_dtr;
	card->ext_csd.hs200_max_dtr = hs200_max_dtr;
	card->mmc_avail_type = avail_type;
}

void msdc_get_host_mode_speed(struct seq_file *m, struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);

	seq_printf(m, "[SD_Debug]msdc[%d] supports:\n", host->id);

	if (mmc->caps & MMC_CAP_MMC_HIGHSPEED)
		seq_puts(m, "[SD_Debug]      MMC_TIMING_MMC_HS\n");
	if (mmc->caps & MMC_CAP_SD_HIGHSPEED)
		seq_puts(m, "[SD_Debug]      MMC_TIMING_SD_HS\n");
	if (mmc->caps & MMC_CAP_UHS_SDR12)
		seq_puts(m, "[SD_Debug]      MMC_CAP_UHS_SDR12\n");
	if (mmc->caps & MMC_CAP_UHS_SDR25)
		seq_puts(m, "[SD_Debug]      MMC_CAP_UHS_SDR25\n");
	if (mmc->caps & MMC_CAP_UHS_SDR104)
		seq_puts(m, "[SD_Debug]      MMC_CAP_UHS_SDR104\n");
	if (mmc->caps & MMC_CAP_UHS_DDR50)
		seq_puts(m, "[SD_Debug]      MMC_CAP_UHS_DDR50\n");
	if (mmc->caps & MMC_CAP_1_8V_DDR)
		seq_puts(m, "[SD_Debug]      MMC_CAP_MMC_DDR52\n");
	if (mmc->caps2 & MMC_CAP2_HS200_1_8V_SDR)
		seq_puts(m, "[SD_Debug]      MMC_TIMING_MMC_HS200\n");
	if (mmc->caps2 & MMC_CAP2_HS400_1_8V)
		seq_puts(m, "[SD_Debug]      MMC_TIMING_MMC_HS400\n");

	seq_puts(m, "[SD_Debug] Command queue feature is disable\n");

}

void msdc_set_host_mode_speed(struct seq_file *m,
	struct mmc_host *mmc, int spd_mode)
{
	struct msdc_host *host = mmc_priv(mmc);

	/* Clear HS400, HS200 timing */
	mmc->caps2 &=
		~(MMC_CAP2_HS400_1_8V | MMC_CAP2_HS200_1_8V_SDR);
	/* Clear other timing */
	mmc->caps &= ~(MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED |
			     MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
			     MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 |
			     MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR);
	switch (spd_mode) {
	case MMC_TIMING_LEGACY:
		break;
	case MMC_TIMING_MMC_HS:
		mmc->caps |= MMC_CAP_MMC_HIGHSPEED;
		break;
	case MMC_TIMING_SD_HS:
		mmc->caps |= MMC_CAP_SD_HIGHSPEED;
		break;
	case MMC_TIMING_UHS_SDR12:
		mmc->caps |= MMC_CAP_UHS_SDR12;
		break;
	case MMC_TIMING_UHS_SDR25:
		mmc->caps |= MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR12;
		break;
	case MMC_TIMING_UHS_SDR50:
		mmc->caps |= MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR25 |
			MMC_CAP_UHS_SDR12;
		break;
	case MMC_TIMING_UHS_SDR104:
		mmc->caps |= MMC_CAP_UHS_SDR104 | MMC_CAP_UHS_DDR50 |
			MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR25 |
			MMC_CAP_UHS_SDR12;
		break;
	case MMC_TIMING_UHS_DDR50:
		mmc->caps |= MMC_CAP_UHS_DDR50 | MMC_CAP_UHS_SDR25 |
			MMC_CAP_UHS_SDR12;
		break;
	case MMC_TIMING_MMC_DDR52:
		mmc->caps |= MMC_CAP_1_8V_DDR | MMC_CAP_MMC_HIGHSPEED;
		break;
	case MMC_TIMING_MMC_HS200:
		mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
		mmc->caps |= MMC_CAP_1_8V_DDR | MMC_CAP_MMC_HIGHSPEED;
		break;
	case MMC_TIMING_MMC_HS400:
		mmc->caps2 |= MMC_CAP2_HS400_1_8V | MMC_CAP2_HS200_1_8V_SDR;
		mmc->caps |= MMC_CAP_1_8V_DDR | MMC_CAP_MMC_HIGHSPEED;
		break;
	default:
		seq_printf(m, "[SD_Debug]invalid speed mode:%d\n", spd_mode);
		break;
	}

	if (!mmc->card || mmc_card_sd(mmc->card))
		return;

	msdc_select_card_type(mmc);

	/* For suppressing msdc_dump_info() caused by
	 * cmd13 do in mmc_reset()@mmc.c
	 */
	g_emmc_mode_switch = 1;

	if (mmc->card->ext_csd.cache_ctrl) {
		mmc_flush_cache(mmc->card);
		msdc_cache_ctrl(host, 0, NULL);
	}

	/* Since only mmc_hw_reset() is public,
	 * so use mmc_hw_reset() to reinit card
	 */
	mmc->caps |= MMC_CAP_HW_RESET;

	/* Must set mmc_host ios.time = MMC_TIMING_LEGACY,
	 * or clock will not be setted to 400K before mmc_init_card
	 * CMD1 will timeout
	 */
	mmc->ios.timing = MMC_TIMING_LEGACY;
	mmc->ios.clock = 260000;
	msdc_ops_set_ios(mmc, &mmc->ios);

	if (mmc_hw_reset(mmc))
		seq_puts(m, "[SD_Debug] Reinit card failed, Can not switch speed mode\n");

	g_emmc_mode_switch = 0;

	mmc->caps &= ~MMC_CAP_HW_RESET;

}

static void msdc_set_field(struct seq_file *m, void __iomem *address,
	unsigned int start_bit, unsigned int len, unsigned int value)
{
	unsigned long field;

	if (start_bit > 31 || start_bit < 0 || len > 31 || len <= 0
	 || (start_bit + len > 32)) {
		seq_puts(m, "[SD_Debug]invalid reg field range or length\n");
	} else {
		field = ((1 << len) - 1) << start_bit;
		value &= (1 << len) - 1;
		seq_printf(m, "[SD_Debug]Original:0x%p (0x%x)\n",
			address, MSDC_READ32(address));
		MSDC_SET_FIELD(address, field, value);
		seq_printf(m, "[SD_Debug]Modified:0x%p (0x%x)\n",
			address, MSDC_READ32(address));
	}
}

static void msdc_get_field(struct seq_file *m, void __iomem *address,
	unsigned int start_bit, unsigned int len, unsigned int value)
{
	unsigned long field;

	if (start_bit > 31 || start_bit < 0 || len > 31 || len <= 0
	 || (start_bit + len > 32)) {
		seq_puts(m, "[SD_Debug]invalid reg field range or length\n");
	} else {
		field = ((1 << len) - 1) << start_bit;
		MSDC_GET_FIELD(address, field, value);
		seq_printf(m, "[SD_Debug]Reg:0x%p start_bit(%d)len(%d)(0x%x)\n",
			address, start_bit, len, value);
	}
}

void msdc_sdio_profile(struct sdio_profile *result)
{
	struct cmd_profile *cmd;
	u32 i;

	pr_debug("sdio === performance dump ===\n");
	pr_debug("sdio === total execute tick<%d> time<%dms> Tx<%dB> Rx<%dB>\n",
		result->total_tc, result->total_tc / TICKS_ONE_MS,
		result->total_tx_bytes, result->total_rx_bytes);

	/* CMD52 Dump */
	cmd = &result->cmd52_rx;
	pr_debug("sdio === CMD52 Rx <%d>times tick<%d> Max<%d> Min<%d> Aver<%d>\n",
		cmd->count, cmd->tot_tc,
		cmd->max_tc, cmd->min_tc, cmd->tot_tc / cmd->count);
	cmd = &result->cmd52_tx;
	pr_debug("sdio === CMD52 Tx <%d>times tick<%d> Max<%d> Min<%d> Aver<%d>\n",
		cmd->count, cmd->tot_tc,
		cmd->max_tc, cmd->min_tc, cmd->tot_tc / cmd->count);

	/* CMD53 Rx bytes + block mode */
	for (i = 0; i < 512; i++) {
		cmd = &result->cmd53_rx_byte[i];
		if (cmd->count == 0)
			continue;
		pr_debug("sdio<%6d><%3dB>_Rx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n",
			cmd->count, i, cmd->tot_tc, cmd->max_tc, cmd->min_tc,
			cmd->tot_tc / cmd->count, cmd->tot_bytes,
			(cmd->tot_bytes / 10) * 13 / (cmd->tot_tc / 10));
	}
	for (i = 0; i < 100; i++) {
		cmd = &result->cmd53_rx_blk[i];
		if (cmd->count == 0)
			continue;
		pr_debug("sdio<%6d><%3d>B_Rx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n",
			cmd->count, i, cmd->tot_tc, cmd->max_tc, cmd->min_tc,
			cmd->tot_tc / cmd->count, cmd->tot_bytes,
			(cmd->tot_bytes / 10) * 13 / (cmd->tot_tc / 10));
	}

	/* CMD53 Tx bytes + block mode */
	for (i = 0; i < 512; i++) {
		cmd = &result->cmd53_tx_byte[i];
		if (cmd->count == 0)
			continue;
		pr_debug("sdio<%6d><%3dB>_Tx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n",
			 cmd->count, i, cmd->tot_tc, cmd->max_tc, cmd->min_tc,
			 cmd->tot_tc / cmd->count, cmd->tot_bytes,
			 (cmd->tot_bytes / 10) * 13 / (cmd->tot_tc / 10));
	}
	for (i = 0; i < 100; i++) {
		cmd = &result->cmd53_tx_blk[i];
		if (cmd->count == 0)
			continue;
		pr_debug("sdio<%6d><%3d>B_Tx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n",
			 cmd->count, i, cmd->tot_tc, cmd->max_tc, cmd->min_tc,
			 cmd->tot_tc / cmd->count, cmd->tot_bytes,
			 (cmd->tot_bytes / 10) * 13 / (cmd->tot_tc / 10));
	}

	pr_debug("sdio === performance dump done ===\n");
}

/* ========= sdio command table =========== */
void msdc_performance(u32 opcode, u32 sizes, u32 bRx, u32 ticks)
{
	struct sdio_profile *result = &sdio_perfomance;
	struct cmd_profile *cmd;
	u32 block;
	long long endtime;

	if (sdio_pro_enable == 0)
		return;

	if (opcode == 52) {
		cmd = bRx ? &result->cmd52_rx : &result->cmd52_tx;
	} else if (opcode == 53) {
		if (sizes < 512) {
			cmd = bRx ? &result->cmd53_rx_byte[sizes] :
				&result->cmd53_tx_byte[sizes];
		} else {
			block = sizes / 512;
			if (block >= 99) {
				pr_err("cmd53 error blocks\n");
				while (1)
					;
			}
			cmd = bRx ? &result->cmd53_rx_blk[block] :
				&result->cmd53_tx_blk[block];
		}
	} else {
		return;
	}

	/* update the members */
	if (ticks > cmd->max_tc)
		cmd->max_tc = ticks;

	if (cmd->min_tc == 0 || ticks < cmd->min_tc)
		cmd->min_tc = ticks;

	cmd->tot_tc += ticks;
	cmd->tot_bytes += sizes;
	cmd->count++;

	if (bRx)
		result->total_rx_bytes += sizes;
	else
		result->total_tx_bytes += sizes;

	result->total_tc += ticks;
#if 0
	/* dump when total_tc > 30s */
	if (result->total_tc >= sdio_pro_time * TICKS_ONE_MS * 1000) {
		msdc_sdio_profile(result);
		memset(result, 0, sizeof(struct sdio_profile));
	}
#endif

	endtime = sched_clock();
	if ((endtime - sdio_profiling_start) >=
		sdio_pro_time * 1000000000) {
		msdc_sdio_profile(result);
		memset(result, 0, sizeof(struct sdio_profile));
		sdio_profiling_start = endtime;
	}

}

void sdio_get_time(struct mmc_request *mrq, struct timespec *time_now)
{
	if (mrq->cmd->opcode == 52 || mrq->cmd->opcode == 53)
		get_monotonic_boottime(time_now);
}

void sdio_calc_time(struct mmc_request *mrq, struct timespec *time_start)
{
	struct timespec time_end;
	u32 ticks = 0, sizes = 0, bRx = 0;

	if (mrq->cmd->opcode == 52 || mrq->cmd->opcode == 53) {
		get_monotonic_boottime(&time_end);
		if (time_end.tv_sec == time_start->tv_sec) {
			ticks = time_end.tv_nsec == time_start->tv_nsec;
		} else if (time_end.tv_sec > time_start->tv_sec) {
			ticks = (time_end.tv_sec - time_start->tv_sec)
				* 1000000000UL
				+ time_end.tv_nsec - time_start->tv_nsec;
		} else {
			pr_err("Shall not happen\n");
			return;
		}

		if (mrq->cmd->data) {
			sizes = mrq->cmd->data->blocks *
				mrq->cmd->data->blksz;
			bRx = mrq->cmd->data->flags & MMC_DATA_READ ?
				1 : 0;
		} else {
			bRx = mrq->cmd->arg & 0x80000000 ? 1 : 0;
		}

		if (!mrq->cmd->error)
			msdc_performance(mrq->cmd->opcode, sizes, bRx, ticks);
	}
}

#define COMPARE_ADDRESS_MMC             0x402000
#define COMPARE_ADDRESS_SD              0x2000
#define COMPARE_ADDRESS_SDIO            0x0
#define COMPARE_ADDRESS_SD_COMBO        0x2000

#define MSDC_MULTI_BUF_LEN  (4*4*1024) /*16KB write/read/compare*/

static DEFINE_MUTEX(sd_lock);
static DEFINE_MUTEX(emmc_lock);

u8 read_write_state; /* 0:stop, 1:read, 2:write */

static u8 wData_emmc[16] = {
	0x67, 0x45, 0x23, 0x01,
	0xef, 0xcd, 0xab, 0x89,
	0xce, 0x8a, 0x46, 0x02,
	0xde, 0x9b, 0x57, 0x13
};

static u8 wData_sd[200] = {
	0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,    /*worst1*/
	0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
	0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,

	0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,    /*worst2*/
	0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,

	0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff,    /*worst3*/
	0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,

	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,    /*worst4*/
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,

	0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,    /*worst5*/
	0x80, 0x80, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x7f,
	0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x7f, 0x40, 0x40,
	0x04, 0xfb, 0x04, 0x04, 0x04, 0xfb, 0xfb, 0xfb,
	0x04, 0xfb, 0xfb, 0xfb, 0x02, 0x02, 0x02, 0xfd,
	0x02, 0x02, 0x02, 0xfd, 0xfd, 0xfd, 0x02, 0xfd,
	0xfd, 0xfd, 0x01, 0x01, 0x01, 0xfe, 0x01, 0x01,
	0x01, 0xfe, 0xfe, 0xfe, 0x01, 0xfe, 0xfe, 0xfe,
	0x80, 0x80, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x7f,
	0x7f, 0x7f, 0x80, 0x7f, 0x7f, 0x7f, 0x40, 0x40,
	0x40, 0x40, 0x80, 0x7f, 0x7f, 0x7f, 0x40, 0x40,
	0x20, 0xdf, 0x20, 0x20, 0x20, 0xdf, 0xdf, 0xdf,
	0x10, 0x10, 0x10, 0xef, 0xef, 0x10, 0xef, 0xef,
};

/*
  * @read, bit0: 1:read/0:write; bit1: 0:compare/1:not compare
*/
static int multi_rw_compare_core(int host_num, int read, uint address,
	uint type, uint compare)
{
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_command msdc_stop;

	u32 *multi_rwbuf = NULL;
	u8 *wPtr = NULL, *rPtr = NULL;

	struct mmc_request msdc_mrq;
	struct msdc_host *host_ctl;
	struct mmc_host *mmc;
	int result = 0, forIndex = 0;
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	int is_cmdq_en;
	int ret;
#endif

	u8 wData_len;
	u8 *wData;

	if (host_num >= HOST_MAX_NUM || host_num < 0) {
		pr_err("[%s]invalid host id: %d\n", __func__, host_num);
		return -1;
	}

	host_ctl = mtk_msdc_host[host_num];

	if (!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card) {
		pr_err(" No card initialized in host[%d]\n", host_num);
		result = -1;
		goto free;
	}

	if (type == MMC_TYPE_MMC) {
		wData = wData_emmc;
		wData_len = 16;
	} else if (type == MMC_TYPE_SD) {
		wData = wData_sd;
		wData_len = 200;
	} else
		return -1;

	/*allock memory for test buf*/
	multi_rwbuf = kzalloc((MSDC_MULTI_BUF_LEN), GFP_KERNEL);
	if (multi_rwbuf == NULL) {
		result = -1;
		return result;
	}
	rPtr = wPtr = (u8 *)multi_rwbuf;

	if (!is_card_present(host_ctl)) {
		pr_err("  [%s]: card is removed!\n", __func__);
		result = -1;
		goto free;
	}

	mmc = host_ctl->mmc;

	mmc_claim_host(mmc);

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	is_cmdq_en = false;
	if (host_num == 0 && host_ctl->mmc->card->ext_csd.cmdq_mode_en) {
		/* cmdq enabled, turn it off first */
		pr_debug("[MSDC_DBG] cmdq enabled, turn it off\n");
		ret = mmc_blk_cmdq_switch(host_ctl->mmc->card, 0);
		if (ret) {
			pr_debug("[MSDC_DBG] turn off cmdq en failed\n");
			mmc_release_host(host_ctl->mmc);
			result = -1;
			goto free;
		} else
			is_cmdq_en = true;
	}
#endif

	if (host_ctl->hw->host_function == MSDC_EMMC)
		msdc_switch_part(host_ctl, 0);

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));
	memset(&msdc_stop, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;
	msdc_data.blocks = MSDC_MULTI_BUF_LEN / 512;

	if (read) {
		/* init read command */
		msdc_data.flags = MMC_DATA_READ;
		msdc_cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
	} else {
		/* init write command */
		msdc_data.flags = MMC_DATA_WRITE;
		msdc_cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		/* init write buffer */
		for (forIndex = 0; forIndex < MSDC_MULTI_BUF_LEN; forIndex++)
			*(wPtr + forIndex) = wData[forIndex % wData_len];
	}

#ifdef CONFIG_MTK_HW_FDE_AES
	if (fde_aes_check_cmd(FDE_AES_EN_RAW, fde_aes_get_raw(), host_num)) {
		fde_aes_set_msdc_id(host_num & 0xff);
		if (read)
			fde_aes_set_fde(0);
		else
			fde_aes_set_fde(1);
	}
#endif

	msdc_cmd.arg = address;

	msdc_stop.opcode = MMC_STOP_TRANSMISSION;
	msdc_stop.arg = 0;
	msdc_stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	msdc_data.stop = &msdc_stop;

	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		/* pr_err("this device use byte address!!\n"); */
		msdc_cmd.arg <<= 9;
	}
	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

	sg_init_one(&msdc_sg, multi_rwbuf, MSDC_MULTI_BUF_LEN);

	mmc_set_data_timeout(&msdc_data, mmc->card);
	mmc_wait_for_req(mmc, &msdc_mrq);

	if (compare == 0 || !read)
		goto skip_check;

	/* compare */
	for (forIndex = 0; forIndex < MSDC_MULTI_BUF_LEN; forIndex++) {
		if (rPtr[forIndex] != wData[forIndex % wData_len]) {
			pr_err("index[%d]\tW_buffer[0x%x]\tR_buffer[0x%x]\tfailed\n",
				forIndex, wData[forIndex % wData_len],
				rPtr[forIndex]);
			result = -1;
		}
	}

skip_check:
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	if (is_cmdq_en) {
		pr_debug("[MSDC_DBG] turn on cmdq\n");
		ret = mmc_blk_cmdq_switch(host_ctl->mmc->card, 1);
		if (ret)
			pr_debug("[MSDC_DBG] turn on cmdq en failed\n");
		else
			is_cmdq_en = false;
	}
#endif

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		result = msdc_cmd.error;

	if (msdc_data.error)
		result = msdc_data.error;

free:
	kfree(multi_rwbuf);

	return result;
}

int multi_rw_compare(struct seq_file *m, int host_num,
		uint address, int count, uint type, int multi_thread)
{
	int i = 0;
	int error = 0;
	struct mutex *rw_mutex;

	if (type == MMC_TYPE_SD)
		rw_mutex = &sd_lock;
	else if (type == MMC_TYPE_MMC)
		rw_mutex = &emmc_lock;
	else
		return 0;

	for (i = 0; i < count; i++) {
		mutex_lock(rw_mutex);
		/* write */
		error = multi_rw_compare_core(host_num, 0, address, type, 0);
		if (error) {
			if (!multi_thread)
				seq_printf(m, "[%s]: failed to write data, error=%d\n",
					__func__, error);
			else
				pr_err("[%s]: failed to write data, error=%d\n",
				__func__, error);
			goto skip_read;
		}

		/* read */
		error = multi_rw_compare_core(host_num, 1, address,
			type, 1);
		if (error) {
			if (!multi_thread)
				seq_printf(m, "[%s]: failed to read data, error=%d\n",
				__func__, error);
			else
				pr_err("[%s]: failed to read data, error=%d\n",
				__func__, error);
		}

skip_read:
		if (!multi_thread)
			seq_printf(m, "== cpu[%d] pid[%d]: %s %d time compare ==\n",
				task_cpu(current), current->pid,
				(error ? "FAILED" : "FINISH"), i);
		else
			pr_err("== cpu[%d] pid[%d]: %s %d time compare ==\n",
				task_cpu(current), current->pid,
				(error ? "FAILED" : "FINISH"), i);

		mutex_unlock(rw_mutex);
		if (error)
			break;
	}

	if (i == count) {
		if (!multi_thread)
			seq_printf(m, "pid[%d]: success to compare data for %d times\n",
				current->pid, count);
		else
			pr_err("pid[%d]: success to compare data for %d times\n",
				current->pid, count);
	}

	return error;
}

#define MAX_THREAD_NUM_FOR_SMP 20

/* make the test can run on 4GB card */
static uint smp_address_on_sd[MAX_THREAD_NUM_FOR_SMP] = {
	0x2000,
	0x80000,
	0x100000,
	0x180000,
	0x200000,		/* 1GB */
	0x202000,
	0x280000,
	0x300000,
	0x380000,
	0x400000,		/* 2GB */
	0x402000,
	0x480000,
	0x500000,
	0x580000,
	0x600000,
	0x602000,		/* 3GB */
	0x660000,		/* real total size of 4GB sd card is < 4GB */
	0x680000,
	0x6a0000,
	0x6b0000,
};

/* cause the system run on the emmc storage,
 * so do not to access the first 2GB region
 */
static uint smp_address_on_mmc[MAX_THREAD_NUM_FOR_SMP] = {
	0x402000,
	0x410000,
	0x520000,
	0x530000,
	0x640000,
	0x452000,
	0x460000,
	0x470000,
	0x480000,
	0x490000,
	0x4a2000,
	0x4b0000,
	0x5c0000,
	0x5d0000,
	0x6e0000,
	0x602000,
	0x660000,		/* real total size of 4GB sd card is < 4GB */
	0x680000,
	0x6a0000,
	0x6b0000,
};

struct write_read_data {
	int host_id;		/* target host you want to do SMP test on. */
	uint start_address;	/* Address of memcard you want to write/read */
	int count;		/* times you want to do read after write */
	struct seq_file *m;
};

static struct write_read_data wr_data[HOST_MAX_NUM][MAX_THREAD_NUM_FOR_SMP];
/* 2012-03-25
 * the SMP thread function
 * do read after write the memory card, and bit by bit comparison
 */

struct task_struct *rw_thread;

static int write_read_thread(void *ptr)
{
	struct write_read_data *data = (struct write_read_data *)ptr;
	struct seq_file *m = data->m;

	if (data->host_id == 1) {
		pr_err("sd thread start\n");
		multi_rw_compare(m, data->host_id, data->start_address,
			data->count, MMC_TYPE_SD, 1);
		pr_err("sd thread %d end\n", current->pid);
	} else if (data->host_id == 0) {
		pr_err("emmc thread %d start\n", current->pid);
		multi_rw_compare(m, data->host_id, data->start_address,
			data->count, MMC_TYPE_MMC, 1);
		pr_err("emmc thread %d end\n", current->pid);
	}
	return 0;
}

/*
 * 2012-03-25
 * function:         do SMP test on all MSDC hosts
 * thread_num:       number of thread to be triggerred on this host.
 * count:            times you want to do read after writein each thread.
 * multi_address:    whether do read/write the same/different address of
 *                       the memory card in each thread.
 */
static int smp_test_on_hosts(struct seq_file *m, int thread_num,
		int host_id, int count, int multi_address)
{
	int i = 0, j = 0;
	int id_start, id_end;
	int ret = 0;
	uint start_address, type;
	char thread_name[128];
	struct msdc_host *host_ctl;

	seq_printf(m, "=======================[%s] start ===========================\n\n",
		__func__);
	seq_printf(m, " each host run %d thread, each thread run %d RW comparison\n",
		thread_num, count);
	if (thread_num > MAX_THREAD_NUM_FOR_SMP) {
		seq_printf(m, " too much thread for SMP test, thread_num=%d\n",
			thread_num);
		ret = -1;
		goto out;
	}
	if (host_id > HOST_MAX_NUM || host_id < 0) {
		seq_printf(m, " Invalid host id %d\n", host_id);
		ret = -1;
		goto out;
	}
	if (host_id == HOST_MAX_NUM) {
		id_start = 0;
		id_end = HOST_MAX_NUM;
	} else {
		id_start = host_id;
		id_end = host_id+1;
	}


	for (i = id_start; i < id_end; i++) {
		host_ctl = mtk_msdc_host[i];
		if (!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card) {
			seq_printf(m, " MSDC[%d], no card is initialized\n", i);
			continue;
		}
		type = host_ctl->mmc->card->type;
		if (type == MMC_TYPE_MMC) {
			if (!multi_address)
				start_address = COMPARE_ADDRESS_MMC;
			else
				start_address = smp_address_on_mmc[i];
		} else if (type == MMC_TYPE_SD) {
			if (!multi_address)
				start_address = COMPARE_ADDRESS_MMC;
			else
				start_address = smp_address_on_sd[i];
		} else if (type == MMC_TYPE_SDIO) {
			seq_printf(m, " MSDC[%d], SDIO:\n", i);
			seq_puts(m, "   please manually run wifi application instead of write/read SDIO card\n");
			continue;
		} else {
			seq_printf(m, " MSDC[%d], unkwonn card type\n ", i);
			continue;
		}
		for (j = 0; j < thread_num; j++) {
			wr_data[i][j].host_id = i;
			wr_data[i][j].count = count;
			wr_data[i][j].start_address = start_address;
			wr_data[i][j].m = m;

			sprintf(thread_name, "msdc_H%d_T%d", i, j);
			kthread_run(write_read_thread, &wr_data[i][j],
				thread_name);
			seq_printf(m, "	start thread: %s, at address: 0x%x\n",
				 thread_name, wr_data[i][j].start_address);
		}
	}
 out:
	seq_printf(m, "=======================[%s] end ===========================\n\n",
		__func__);
	return ret;
}

static int msdc_help_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, "\n===============[msdc_help]================\n");

	seq_printf(m, "\n   LOG control:        echo %x [host_id] [debug_zone] > msdc_debug\n",
		SD_TOOL_ZONE);
	seq_printf(m, "          [debug_zone] DMA:0x%x, CMD:0x%x, RSP:0x%x, INT:0x%x, CFG:0x%x, FUC:0x%x,\n",
		DBG_EVT_DMA, DBG_EVT_CMD, DBG_EVT_RSP, DBG_EVT_INT, DBG_EVT_CFG,
		DBG_EVT_FUC);
	seq_printf(m, "                        OPS:0x%x, FIO:0x%x, WRN:0x%x, PWR:0x%x, CLK:0x%x, RW:0x%x, NRW:0x%x, CHE:0x%x\n",
		DBG_EVT_OPS, DBG_EVT_FIO, DBG_EVT_WRN, DBG_EVT_PWR, DBG_EVT_CLK,
		DBG_EVT_RW, DBG_EVT_NRW, DBG_EVT_CHE);
	seq_puts(m, "\n   DMA mode:\n");
	seq_printf(m, "   *set DMA mode:   echo %x 0 [host_id] [dma_mode] [dma_size] > msdc_debug\n",
		SD_TOOL_DMA_SIZE);
	seq_printf(m, "   *get DMA mode:   echo %x 1 [host_id] > msdc_debug\n",
		SD_TOOL_DMA_SIZE);
	seq_puts(m, "        [dma_mode]   0:PIO, 1:DMA, 2:SIZE_DEP\n");
	seq_puts(m, "        [dma_size]   valid for SIZE_DEP mode, the min size can trigger the DMA mode\n");
	seq_printf(m, "\n   SDIO profile:  echo %x [enable] [time] > msdc_debug\n",
		SD_TOOL_SDIO_PROFILE);
	seq_puts(m, "\n   REGISTER control:\n");
	seq_printf(m, "        write register:    echo %x 0 [host_id] [register_offset] [value] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_printf(m, "        read register:     echo %x 1 [host_id] [register_offset] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_printf(m, "        write mask:        echo %x 2 [host_id] [register_offset] [start_bit] [len] [value] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_printf(m, "        read mask:         echo %x 3 [host_id] [register_offset] [start_bit] [len] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_printf(m, "        dump all:          echo %x 4 [host_id] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_puts(m, "\n   DRVING control:\n");
	seq_printf(m, "        set driving:       echo %x [host_id] [clk_drv] [cmd_drv] [dat_drv] [rst_drv] [ds_drv] [voltage] > msdc_debug\n",
		SD_TOOL_SET_DRIVING);
	seq_puts(m, "            [voltage]           1: 18v, 0: 33v\n");
	seq_printf(m, "        get driving:       echo %x 0 [host_id]  > msdc_debug\n",
		SD_TOOL_SET_DRIVING);
	seq_printf(m, "\n   RW_COMPARE test:       echo %x [host_id] [compare_count] > msdc_debug\n",
		RW_BIT_BY_BIT_COMPARE);
	seq_puts(m, "          [compare_count]    how many time you want to \"write=>read=>compare\"\n");
	seq_printf(m, "\n   SMP_ON_ONE_HOST test:  echo %x [host_id] [thread_num] [compare_count] [multi_address] > msdc_debug\n",
		SMP_TEST_ON_ONE_HOST);
	seq_puts(m, "          [thread_num]       how many R/W comparision thread you want to run at host_id\n");
	seq_puts(m, "          [compare_count]    how many time you want to \"write=>read=>compare\" in each thread\n");
	seq_puts(m, "          [multi_address]    whether read/write different address in each thread, 0:No, 1:Yes\n");
	seq_printf(m, "\n   SMP_ON_ALL_HOST test:  echo %x [thread_num] [compare_count] [multi_address] > msdc_debug\n",
		SMP_TEST_ON_ALL_HOST);
	seq_puts(m, "          [thread_num]       how many R/W comparision thread you want to run at each host\n");
	seq_puts(m, "          [compare_count]    how many time you want to \"write=>read=>compare\" in each thread\n");
	seq_puts(m, "          [multi_address]    whether read/write different address in each thread, 0:No, 1:Yes\n");
	seq_puts(m, "\n   SPEED_MODE control:\n");
	seq_printf(m, "          set speed mode:    echo %x 1 [host_id] [speed_mode] [cmdq]> msdc_debug\n",
		SD_TOOL_MSDC_HOST_MODE);
	seq_printf(m, "          get speed mode:    echo %x 0 [host_id]\n",
		SD_TOOL_MSDC_HOST_MODE);
	seq_puts(m, "            [speed_mode]       0: MMC_TIMING_LEGACY	1: MMC_TIMING_MMC_HS	2: MMC_TIMING_SD_HS	 3: MMC_TIMING_UHS_SDR12\n"
		    "                               4: MMC_TIMING_UHS_SDR25	5: MMC_TIMING_UHS_SDR50	6: MMC_TIMING_UHS_SDR104 7: MMC_TIMING_UHS_DDR50\n"
		    "                               8: MMC_TIMING_MMC_DDR52	9: MMC_TIMING_MMC_HS200	A: MMC_TIMING_MMC_HS400\n");
	seq_printf(m, "\n   DMA viloation:         echo %x [host_id] [ops]> msdc_debug\n",
		SD_TOOL_DMA_STATUS);
	seq_puts(m, "          [ops]              0:get latest dma address, 1:start violation test\n");
	seq_printf(m, "\n   SET Slew Rate:         echo %x [host_id] [clk] [cmd] [dat] [rst] [ds]> msdc_debug\n",
		SD_TOOL_ENABLE_SLEW_RATE);
	seq_puts(m, "\n   TD/RD SEL:\n");
	seq_printf(m, "          set rdsel:             echo %x [host_id] 0 [value] > msdc_debug\n",
		SD_TOOL_SET_RDTDSEL);
	seq_printf(m, "          set tdsel:             echo %x [host_id] 1 [value] > msdc_debug\n",
		SD_TOOL_SET_RDTDSEL);
	seq_printf(m, "          get tdsel/rdsel:       echo %x [host_id] 2 > msdc_debug\n",
		SD_TOOL_SET_RDTDSEL);
	seq_puts(m, "            [value]              rdsel: 0x0<<4 ~ 0x3f<<4,    tdsel: 0x0~0xf\n");
	seq_printf(m, "\n   EMMC/SD RW test:       echo %x [host_id] [mode] > msdc_debug\n",
		MSDC_READ_WRITE);
	seq_puts(m, "          [mode]               mode 0:stop, 1:read, 2:write\n");
	seq_printf(m, "\n   Error tune debug:       echo %x [host_id] [cmd_id] [arg] [error_type] [count] > msdc_debug\n",
		MMC_ERROR_TUNE);
	seq_puts(m, "            [cmd_id]           0: CMD0, 1: CMD1, 2: CMD2......\n");
	seq_puts(m, "            [arg]              for CMD6, arg means ext_csd index......\n");
	seq_puts(m, "            [error]            0: disable error tune debug, 1: cmd timeout, 2: cmd crc, 4: dat timeout, 8: dat crc, 16: acmd timeout, 32: acmd crc\n");
	seq_puts(m, "            [count]            error count\n");
	seq_printf(m, "\n   eMMC Cache Control: echo %x [host_id] [action_id] > /proc/msdc_debug\n",
		MMC_EDC_EMMC_CACHE);
	seq_puts(m, "            [action_id]        0:Disable cache 1:Enable cache 2:check cache status\n");
	seq_printf(m, "\n   eMMC Dump GPD/BD:      echo %x [host_id] > /proc/msdc_debug\n",
		MMC_DUMP_GPD);
	seq_puts(m, "            [type]             0:tune cmd  1:tune read  2:tune write  3:tune HS400\n");
	seq_puts(m, "            [start_voltage]    ?mV\n");
	seq_puts(m, "            [end_voltage]      ?mV, we try ETT from higher voltage to lower voltage\n");
	seq_printf(m, "\n   CRC Stress Test:       echo %x [action_id]> /proc/msdc_debug\n",
		MMC_CRC_STRESS);
	seq_puts(m, "            [action_id]        0:disable 1:enable\n");
	seq_printf(m, "\n   SDIO AutoK Result :    echo %x [host_id][vcore][rw]> /proc/msdc_debug\n",
		SDIO_AUTOK_RESULT);
	seq_puts(m, "            [host_id]  2:sdio\n");
	seq_puts(m, "            [vcore]    0:low  1:high\n");
	seq_puts(m, "            [rw]       0:read  1:write\n");


	seq_puts(m, "\n   NOTE: All input data is Hex number!\n");

	seq_puts(m, "\n=============================================\n\n");

	return 0;
}

/*
  *data: bit0~4:id, bit4~7: mode
*/
static int rwThread(void *data)
{
	int error, i = 0;
	ulong p = (ulong) data;
	int id = p & 0x3;
	int mode = (p >> 4) & 0x3;
	int read;
	uint type = 0, address = 0;

	pr_err("[****SD_rwThread****]id=%d, mode=%d\n", id, mode);

	if (mode == 1)
		read = 1;
	else if (mode == 2)
		read = 0;
	else
		return -1;

	while (read_write_state != 0) {
		if (read_write_state == 1)
			p = 0x3;
		else if (read_write_state == 2)
			p = 0;

		if (id == 0) {
			type = MMC_TYPE_MMC;
			address = COMPARE_ADDRESS_MMC;
		} else if (id < HOST_MAX_NUM) {
			type = MMC_TYPE_SD;
			address = COMPARE_ADDRESS_SD;
		}

		error = multi_rw_compare_core(id, read, address, type, 0);
		if (error) {
			pr_err("[%s]: failed data id0, error=%d\n",
				__func__, error);
				break;
		}

		i++;
		if (i == 10000) {
			pr_err("[***rwThread %s***]",
				read_write_state == 1 ? "read" : "write");
			i = 0;
		}
	}
	pr_err("[SD_Debug]rwThread exit\n");
	return 0;
}

void msdc_dump_gpd_bd(int id)
{
	struct msdc_host *host;
	int i = 0;
	struct gpd_t *gpd;
	struct bd_t *bd;

	if (id < 0 || id >= HOST_MAX_NUM) {
		pr_err("[%s]: invalid host id: %d\n", __func__, id);
		return;
	}

	host = mtk_msdc_host[id];
	if (host == NULL) {
		pr_err("[%s]: host0 or host0->dma is NULL\n", __func__);
		return;
	}
	gpd = host->dma.gpd;
	bd  = host->dma.bd;

	pr_err("================MSDC GPD INFO ==================\n");
	if (gpd == NULL) {
		pr_err("GPD is NULL\n");
	} else {
#ifdef CONFIG_ARM64
		pr_err("gbd addr:0x%llx\n", host->dma.gpd_addr);
#else
		pr_err("gbd addr:0x%x\n", (unsigned int)host->dma.gpd_addr);
#endif
		pr_err("hwo:0x%x, bdp:0x%x, rsv0:0x%x, chksum:0x%x,intr:0x%x,rsv1:0x%x,nexth4:0x%x,ptrh4:0x%x\n",
			gpd->hwo, gpd->bdp, gpd->rsv0, gpd->chksum,
			gpd->intr, gpd->rsv1, (unsigned int)gpd->nexth4,
			(unsigned int)gpd->ptrh4);
		pr_err("next:0x%x, ptr:0x%x, buflen:0x%x, extlen:0x%x, arg:0x%x,blknum:0x%x,cmd:0x%x\n",
			(unsigned int)gpd->next, (unsigned int)gpd->ptr,
			gpd->buflen, gpd->extlen, gpd->arg, gpd->blknum,
			gpd->cmd);
	}
	pr_err("================MSDC BD INFO ===================\n");
	if (bd == NULL) {
		pr_err("BD is NULL\n");
	} else {
#ifdef CONFIG_ARM64
		pr_err("bd addr:0x%llx\n", host->dma.bd_addr);
#else
		pr_err("bd addr:0x%x\n", (unsigned int)host->dma.bd_addr);
#endif
		for (i = 0; i < host->dma.sglen; i++) {
			pr_err("%4d BD eol:0x%x, rsv0:0x%x, chksum:0x%x, rsv1:0x%x,blkpad:0x%x,dwpad:0x%x,rsv2:0x%x\n",
				i, bd->eol, bd->rsv0, bd->chksum, bd->rsv1,
				bd->blkpad, bd->dwpad, bd->rsv2);
			pr_err("        nexth4:0x%x, ptrh4:0x%x, next:0x%x, ptr:0x%x, buflen:0x%x, rsv3:0x%x\n",
				(unsigned int)bd->nexth4,
				(unsigned int)bd->ptrh4, (unsigned int)bd->next,
				(unsigned int)bd->ptr, bd->buflen, bd->rsv3);
			bd++;
		}
	}
}

/* FIX ME: Move to user space */
static int msdc_check_emmc_cache_status(struct seq_file *m,
		struct msdc_host *host)
{
	struct mmc_card *card;

	if (!host || !host->mmc || !host->mmc->card) {
		seq_puts(m, "host or mmc or card is NULL\n");
		return -1;
	}

	card = host->mmc->card;
	if (!mmc_card_mmc(card)) {
		seq_printf(m, "msdc%d: is not a eMMC card\n", host->id);
		return -2;
	}

	if (card->ext_csd.cache_size == 0) {
		seq_printf(m, "msdc%d:card don't support cache feature\n",
			host->id);
		return -1;
	}

	seq_printf(m, "msdc%d: Current eMMC Cache status: %s, Cache size:%dKB\n",
		host->id,
		host->mmc->card->ext_csd.cache_ctrl ? "Enable" : "Disable",
		host->mmc->card->ext_csd.cache_size/8);

	return card->ext_csd.cache_ctrl;
}

/* FIX ME: Move to user space */
static void msdc_enable_emmc_cache(struct seq_file *m,
		struct msdc_host *host, int enable)
{
	int err;
	u8 c_ctrl;
	struct mmc_card *card;

	if (msdc_check_emmc_cache_status(m, host) < 0)
		return;

	card = host->mmc->card;

	(void)mmc_get_card(card);

	c_ctrl = card->ext_csd.cache_ctrl;

	if (c_ctrl == enable)
		seq_printf(m, "msdc%d:cache has already been %s state,\n",
			host->id, enable ? "enable" : "disable");
	else {
		err = msdc_cache_ctrl(host, enable, NULL);
		if (err)
			seq_printf(m, "msdc%d: Cache is supported, but %s failed\n",
				host->id, enable ? "enable" : "disable");
		else
			seq_printf(m, "msdc%d: %s cache successfully\n",
				host->id, enable ? "enable" : "disable");
	}
	mmc_put_card(card);
}

#ifdef MTK_MSDC_ERROR_TUNE_DEBUG
#define MTK_MSDC_ERROR_NONE	(0)
#define MTK_MSDC_ERROR_CMD_TMO	(0x1)
#define MTK_MSDC_ERROR_CMD_CRC	(0x1 << 1)
#define MTK_MSDC_ERROR_DAT_TMO	(0x1 << 2)
#define MTK_MSDC_ERROR_DAT_CRC	(0x1 << 3)
#define MTK_MSDC_ERROR_ACMD_TMO	(0x1 << 4)
#define MTK_MSDC_ERROR_ACMD_CRC	(0x1 << 5)
unsigned int g_err_tune_dbg_count;
unsigned int g_err_tune_dbg_host;
unsigned int g_err_tune_dbg_cmd;
unsigned int g_err_tune_dbg_arg;
unsigned int g_err_tune_dbg_error = MTK_MSDC_ERROR_NONE;

static void msdc_error_tune_debug_print(struct seq_file *m, int p1, int p2,
	int p3, int p4, int p5)
{
	g_err_tune_dbg_host = p1;
	g_err_tune_dbg_cmd = p2;
	g_err_tune_dbg_arg = p3;
	g_err_tune_dbg_error = p4;
	g_err_tune_dbg_count = p5;
	if (g_err_tune_dbg_count &&
	    (g_err_tune_dbg_error != MTK_MSDC_ERROR_NONE)) {
		seq_puts(m, "===================MSDC error debug start =======================\n");
		seq_printf(m, "host:%d, cmd=%d, arg=%d, error=%d, count=%d\n",
			g_err_tune_dbg_host, g_err_tune_dbg_cmd,
			g_err_tune_dbg_arg, g_err_tune_dbg_error,
			g_err_tune_dbg_count);
	} else {
		g_err_tune_dbg_host = 0;
		g_err_tune_dbg_cmd = 0;
		g_err_tune_dbg_arg = 0;
		g_err_tune_dbg_error = MTK_MSDC_ERROR_NONE;
		g_err_tune_dbg_count = 0;
		seq_printf(m, "host:%d, cmd=%d, arg=%d, error=%d, count=%d\n",
			g_err_tune_dbg_host, g_err_tune_dbg_cmd,
			g_err_tune_dbg_arg, g_err_tune_dbg_error,
			g_err_tune_dbg_count);
		seq_puts(m, "=====================MSDC error debug end =======================\n");
	}
}

void msdc_error_tune_debug1(struct msdc_host *host, struct mmc_command *cmd,
	struct mmc_command *sbc, u32 *intsts)
{
	if (!g_err_tune_dbg_error ||
	    (g_err_tune_dbg_count <= 0) ||
	    (g_err_tune_dbg_host != host->id))
		return;

#ifdef MTK_MSDC_USE_CMD23
	if (g_err_tune_dbg_cmd != cmd->opcode)
		goto check_sbc;
#endif

	if ((g_err_tune_dbg_cmd != MMC_SWITCH)
	 || ((g_err_tune_dbg_cmd == MMC_SWITCH) &&
	     (g_err_tune_dbg_arg == ((cmd->arg >> 16) & 0xff)))) {
		if (g_err_tune_dbg_error & MTK_MSDC_ERROR_CMD_TMO) {
			*intsts = MSDC_INT_CMDTMO;
			g_err_tune_dbg_count--;
		} else if (g_err_tune_dbg_error & MTK_MSDC_ERROR_CMD_CRC) {
			*intsts = MSDC_INT_RSPCRCERR;
			g_err_tune_dbg_count--;
		}
		pr_err("[%s]: got the error cmd:%d, arg=%d, dbg error=%d, cmd->error=%d, count=%d\n",
			__func__, g_err_tune_dbg_cmd, g_err_tune_dbg_arg,
			g_err_tune_dbg_error, cmd->error, g_err_tune_dbg_count);
	}

#ifdef MTK_MSDC_USE_CMD23
check_sbc:
	if ((g_err_tune_dbg_cmd == MMC_SET_BLOCK_COUNT) &&
	    sbc &&
	    (host->autocmd & MSDC_AUTOCMD23)) {
		if (g_err_tune_dbg_error & MTK_MSDC_ERROR_ACMD_TMO)
			*intsts = MSDC_INT_ACMDTMO;
		else if (g_err_tune_dbg_error & MTK_MSDC_ERROR_ACMD_CRC)
			*intsts = MSDC_INT_ACMDCRCERR;
		else
			return;
		pr_err("[%s]: got the error cmd:%d, dbg error=%d, sbc->error=%d, count=%d\n",
			__func__, g_err_tune_dbg_cmd, g_err_tune_dbg_error,
			sbc->error, g_err_tune_dbg_count);
	}
#endif

}

void msdc_error_tune_debug2(struct msdc_host *host, struct mmc_command *stop,
	u32 *intsts)
{
	void __iomem *base = host->base;

	if (!g_err_tune_dbg_error ||
	    (g_err_tune_dbg_count <= 0) ||
	    (g_err_tune_dbg_host != host->id))
		return;

	if (g_err_tune_dbg_cmd == (MSDC_READ32(SDC_CMD) & 0x3f)) {
		if (g_err_tune_dbg_error & MTK_MSDC_ERROR_DAT_TMO) {
			*intsts = MSDC_INT_DATTMO;
			g_err_tune_dbg_count--;
		} else if (g_err_tune_dbg_error & MTK_MSDC_ERROR_DAT_CRC) {
			*intsts = MSDC_INT_DATCRCERR;
			g_err_tune_dbg_count--;
		}
		pr_err("[%s]: got the error cmd:%d, dbg error 0x%x, data->error=%d, count=%d\n",
			__func__, g_err_tune_dbg_cmd, g_err_tune_dbg_error,
			host->data->error, g_err_tune_dbg_count);
	}
	if ((g_err_tune_dbg_cmd == MMC_STOP_TRANSMISSION) &&
	    stop &&
	    (host->autocmd & MSDC_AUTOCMD12)) {
		if (g_err_tune_dbg_error & MTK_MSDC_ERROR_ACMD_TMO) {
			*intsts = MSDC_INT_ACMDTMO;
			g_err_tune_dbg_count--;
		} else if (g_err_tune_dbg_error & MTK_MSDC_ERROR_ACMD_CRC) {
			*intsts = MSDC_INT_ACMDCRCERR;
			g_err_tune_dbg_count--;
		}
		pr_err("[%s]: got the error cmd:%d, dbg error 0x%x, stop->error=%d, host->error=%d, count=%d\n",
			__func__, g_err_tune_dbg_cmd, g_err_tune_dbg_error,
			stop->error, host->error, g_err_tune_dbg_count);
	}
}
#endif /*ifdef MTK_MSDC_ERROR_TUNE_DEBUG*/

static u16 sdio_setting_offsets[] = {
	OFFSET_MSDC_CFG,
	OFFSET_SDC_STS,

	OFFSET_MSDC_IOCON,
	OFFSET_MSDC_PATCH_BIT0,
	OFFSET_MSDC_PATCH_BIT1,
	OFFSET_MSDC_PATCH_BIT2,
	OFFSET_MSDC_PAD_TUNE0,
	OFFSET_MSDC_PAD_TUNE1,
	OFFSET_MSDC_DAT_RDDLY0,
	OFFSET_MSDC_DAT_RDDLY1,
	OFFSET_MSDC_DAT_RDDLY2,
	OFFSET_MSDC_DAT_RDDLY3,

	OFFSET_MSDC_IOCON_1,
	OFFSET_MSDC_PATCH_BIT0_1,
	OFFSET_MSDC_PATCH_BIT1_1,
	OFFSET_MSDC_PATCH_BIT2_1,
	OFFSET_MSDC_PAD_TUNE0_1,
	OFFSET_MSDC_PAD_TUNE1_1,
	OFFSET_MSDC_DAT_RDDLY0_1,
	OFFSET_MSDC_DAT_RDDLY1_1,
	OFFSET_MSDC_DAT_RDDLY2_1,
	OFFSET_MSDC_DAT_RDDLY3_1,

	OFFSET_MSDC_IOCON_2,
	OFFSET_MSDC_PATCH_BIT0_2,
	OFFSET_MSDC_PATCH_BIT1_2,
	OFFSET_MSDC_PATCH_BIT2_2,
	OFFSET_MSDC_PAD_TUNE0_2,
	OFFSET_MSDC_PAD_TUNE1_2,
	OFFSET_MSDC_DAT_RDDLY0_2,
	OFFSET_MSDC_DAT_RDDLY1_2,
	OFFSET_MSDC_DAT_RDDLY2_2,
	OFFSET_MSDC_DAT_RDDLY3_2,

	0xFFFF /*as mark of end */
};

static void msdc_dump_sdio_setting(struct msdc_host *host, struct seq_file *m)
{
	void __iomem *base = host->base;
	int i;

	MSDC_GET_FIELD(SDC_STS, SDC_STS_DVFS_LEVEL, i);
	seq_printf(m, "SDIO : DVFS_LEVEL : %d\n", i);

	for (i = 0; sdio_setting_offsets[i] != 0xFFFF; i++) {
		seq_printf(m, "R[%x]=0x%.8x\n", sdio_setting_offsets[i],
			MSDC_READ32(base + sdio_setting_offsets[i]));
	}
}

static void msdc_dump_autok_setting(struct msdc_host *host, struct seq_file *m)
{
#ifdef MSDC_BRING_UP
	int i, j;
	int bit_pos, byte_pos, start;
	char buf[65];

	seq_printf(m, "[AUTOK]VER : 0x%02x%02x%02x%02x\r\n",
		host->autok_res[0][AUTOK_VER3],
		host->autok_res[0][AUTOK_VER2],
		host->autok_res[0][AUTOK_VER1],
		host->autok_res[0][AUTOK_VER0]);

	for (i = 0; i < AUTOK_VCORE_NUM; i++) {
		start = CMD_SCAN_R0;
		for (j = 0; j < 64; j++) {
			bit_pos = j % 8;
			byte_pos = j / 8 + start;
			if (host->autok_res[i][byte_pos] & (1 << bit_pos))
				buf[j] = 'X';
			else
				buf[j] = 'O';
		}
		buf[j] = '\0';
		seq_printf(m, "[AUTOK]CMD Rising \t: %s\r\n", buf);

		start = CMD_SCAN_F0;
		for (j = 0; j < 64; j++) {
			bit_pos = j % 8;
			byte_pos = j / 8 + start;
			if (host->autok_res[i][byte_pos] & (1 << bit_pos))
				buf[j] = 'X';
			else
				buf[j] = 'O';
		}
		buf[j] = '\0';
		seq_printf(m, "[AUTOK]CMD Falling \t: %s\r\n", buf);

		start = DAT_SCAN_R0;
		for (j = 0; j < 64; j++) {
			bit_pos = j % 8;
			byte_pos = j / 8 + start;
			if (host->autok_res[i][byte_pos] & (1 << bit_pos))
				buf[j] = 'X';
			else
				buf[j] = 'O';
		}
		buf[j] = '\0';
		seq_printf(m, "[AUTOK]DAT Rising \t: %s\r\n", buf);

		start = DAT_SCAN_F0;
		for (j = 0; j < 64; j++) {
			bit_pos = j % 8;
			byte_pos = j / 8 + start;
			if (host->autok_res[i][byte_pos] & (1 << bit_pos))
				buf[j] = 'X';
			else
				buf[j] = 'O';
		}
		buf[j] = '\0';
		seq_printf(m, "[AUTOK]DAT Falling \t: %s\r\n", buf);

		start = DS_SCAN_0;
		for (j = 0; j < 64; j++) {
			bit_pos = j % 8;
			byte_pos = j / 8 + start;
			if (host->autok_res[i][byte_pos] & (1 << bit_pos))
				buf[j] = 'X';
			else
				buf[j] = 'O';
		}
		buf[j] = '\0';
		seq_printf(m, "[AUTOK]DS Window \t: %s\r\n", buf);

		start = D_DATA_SCAN_0;
		for (j = 0; j < 32; j++) {
			bit_pos = j % 8;
			byte_pos = j / 8 + start;
			if (host->autok_res[i][byte_pos] & (1 << bit_pos))
				buf[j] = 'X';
			else
				buf[j] = 'O';
		}
		buf[j] = '\0';
		seq_printf(m, "[AUTOK]Device Data RX \t: %s\r\n", buf);

		start = H_DATA_SCAN_0;
		for (j = 0; j < 32; j++) {
			bit_pos = j % 8;
			byte_pos = j / 8 + start;
			if (host->autok_res[i][byte_pos] & (1 << bit_pos))
				buf[j] = 'X';
			else
				buf[j] = 'O';
		}
		buf[j] = '\0';
		seq_printf(m, "[AUTOK]Host   Data TX \t: %s\r\n", buf);

		seq_printf(m, "[AUTOK]CMD [EDGE:%d CMD_FIFO_EDGE:%d DLY1:%d DLY2:%d]\r\n",
			host->autok_res[i][0], host->autok_res[i][1], host->autok_res[i][5], host->autok_res[i][7]);
		seq_printf(m, "[AUTOK]DAT [RDAT_EDGE:%d RD_FIFO_EDGE:%d WD_FIFO_EDGE:%d]\r\n",
			host->autok_res[i][2], host->autok_res[i][3], host->autok_res[i][4]);
		seq_printf(m, "[AUTOK]DAT [LATCH_CK:%d DLY1:%d DLY2:%d]\r\n",
			host->autok_res[i][13], host->autok_res[i][9], host->autok_res[i][11]);
		seq_printf(m, "[AUTOK]DS  [DLY1:%d DLY2:%d DLY3:%d]\r\n",
			host->autok_res[i][14], host->autok_res[i][16], host->autok_res[i][18]);
		seq_printf(m, "[AUTOK]DAT [TX SEL:%d]\r\n", host->autok_res[i][20]);
	}
#endif
}

int g_count;
/* ========== driver proc interface =========== */
static int msdc_debug_proc_show(struct seq_file *m, void *v)
{
	int cmd = -1;
	int sscanf_num;
	int p1, p2, p3, p4, p5, p6, p7, p8;
	int id, zone, vcore;
	int mode;
	int thread_num, compare_count, multi_address;
	void __iomem *base = NULL;
	ulong data_for_wr;
	unsigned int offset = 0;
	unsigned int reg_value;
	int spd_mode = MMC_TIMING_LEGACY;
	struct msdc_host *host = NULL;
#ifdef MSDC_DMA_ADDR_DEBUG
	struct dma_addr *dma_address, *p_dma_address;
#endif
	int dma_status;
	u8 *res;

	p1 = p2 = p3 = p4 = p5 = p6 = p7 = p8 = -1;

	cmd_buf[g_count] = '\0';
	seq_printf(m, "Debug Command:  %s\n", cmd_buf);

	sscanf_num = sscanf(cmd_buf, "%x %x %x %x %x %x %x %x %x", &cmd,
		&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);

	if (cmd == -1) {
		seq_puts(m, "Please run: echo cmd [p1] [p2] [p3] [p4] [p5] [p6] [p7] [p8]> /proc/msdc_debug\n");
		seq_puts(m, "For help run: cat /proc/msdc_help\n");
		return 0;
	}

	if (cmd == SD_TOOL_ZONE) {
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;

		host = mtk_msdc_host[id];
		zone = p2;	/* zone &= 0x3ff; */
		seq_printf(m, "[SD_Debug] msdc host<%d> debug zone<0x%.8x>\n",
				id, zone);

		sd_debug_zone[id] = zone;
		if (host) {
			seq_printf(m, "[SD_Debug] msdc host<%d> caps<0x%.8x>\n",
					id, host->mmc->caps);
			seq_printf(m, "[SD_Debug] msdc host<%d> caps2<0x%.8x>\n",
					id, host->mmc->caps2);
		}
	} else if (cmd == SD_TOOL_DMA_SIZE) {
		id = p2;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (p1 == 0) {
			drv_mode[id] = p3;
			dma_size[id] = p4;
		}
		seq_printf(m, "[SD_Debug] msdc host[%d] mode<%d> size<%d>\n",
			 id, drv_mode[id], dma_size[id]);
	} else if ((cmd == SD_TOOL_REG_ACCESS)
		|| (cmd == SD_TOOL_TOP_REG_ACCESS)) {
		id = p2;
		offset = (unsigned int)p3;

		if (id >= HOST_MAX_NUM || id < 0 || mtk_msdc_host[id] == NULL)
			goto invalid_host_id;

		host = mtk_msdc_host[id];
		if (cmd == SD_TOOL_REG_ACCESS) {
			base = host->base;
			if ((offset == 0x18 || offset == 0x1C) && p1 != 4) {
				seq_puts(m, "[SD_Debug] Err: Accessing TXDATA and RXDATA is forbidden\n");
				goto out;
			}
		} else {
			base = host->base_top;
		}

		msdc_clk_enable(host);

		mmc_claim_host(host->mmc);

		if (p1 == 0) {
			if (offset > 0x1000) {
				seq_puts(m, "invalid register offset\n");
				goto out;
			}
			reg_value = p4;
			seq_printf(m, "[SD_Debug][MSDC Reg]Original:0x%p+0x%x (0x%x)\n",
				base, offset, MSDC_READ32(base + offset));
			MSDC_WRITE32(base + offset, reg_value);
			seq_printf(m, "[SD_Debug][MSDC Reg]Modified:0x%p+0x%x (0x%x)\n",
				base, offset, MSDC_READ32(base + offset));
		} else if (p1 == 1) {
			seq_printf(m, "[SD_Debug][MSDC Reg]Reg:0x%p+0x%x (0x%x)\n",
				base, offset, MSDC_READ32(base + offset));
		} else if (p1 == 2) {
			msdc_set_field(m, base + offset, p4, p5, p6);
		} else if (p1 == 3) {
			msdc_get_field(m, base + offset, p4, p5, p6);
		} else if (p1 == 4) {
			msdc_dump_register_core(host, m);
		} else if (p1 == 5) {
			msdc_dump_info(host->id);
		}

		mmc_release_host(host->mmc);

		/* prevent clock off before device ready */
		/*msdc_clk_disable(host);*/
	} else if (cmd == SD_TOOL_SET_DRIVING) {
		char *device_str, *get_set_str;

		id = p2;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];

		if (id == 0) {
			device_str = "eMMC";
		} else if (id == 1) {
			if (p8 == 1)
				device_str = "SD30";
			else
				device_str = "SD20";
		} else {
			device_str = "SDIO";
		}

		if (p1 == 1) {
			get_set_str = "set";
			if ((unsigned char)p3 > 7 || (unsigned char)p4 > 7 ||
			    (unsigned char)p5 > 7 || (unsigned char)p6 > 7 ||
			    (unsigned char)p7 > 7) {
				seq_puts(m, "[SD_Debug]Some drving value was invalid(invalid:0~7)\n");
				goto out;
			}

			host->hw->driving_applied->clk_drv = (unsigned char)p3;
			host->hw->driving_applied->cmd_drv = (unsigned char)p4;
			host->hw->driving_applied->dat_drv = (unsigned char)p5;
			host->hw->driving_applied->rst_drv = (unsigned char)p6;
			host->hw->driving_applied->ds_drv = (unsigned char)p7;
			msdc_set_driving(host, host->hw->driving_applied);
		} else {
			get_set_str = "get";
		}

		seq_printf(m, "[SD_Debug] %s %s driving: clk_drv=%d, cmd_drv=%d, dat_drv=%d, rst_drv=%d, ds_drv=%d\n",
			device_str, get_set_str,
			host->hw->driving_applied->clk_drv,
			host->hw->driving_applied->cmd_drv,
			host->hw->driving_applied->dat_drv,
			host->hw->driving_applied->rst_drv,
			host->hw->driving_applied->ds_drv);
	} else if (cmd == SD_TOOL_ENABLE_SLEW_RATE) {
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];
		if ((unsigned char)p2 > 1 || (unsigned char)p3 > 1
		 || (unsigned char)p4 > 1 || (unsigned char)p5 > 1
		 || (unsigned char)p6 > 1) {
			seq_puts(m, "[SD_Debug]Some sr value was invalid(correct:0(disable),1(enable))\n");
		} else {
			msdc_set_sr(host, p2, p3, p4, p5, p6);
			seq_printf(m, "[SD_Debug]msdc%d, clk_sr=%d, cmd_sr=%d, dat_sr=%d, rst=%d, ds=%d\n",
				id, p2, p3, p4, p5, p6);
		}
	} else if (cmd == SD_TOOL_SET_RDTDSEL) {
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];
		if ((p2 < 0) || (p2 > 2)) {
			seq_puts(m, "[SD_Debug]invalid option ( set rd:0, set td:1, get td/rd: 2)\n");
		} else {
			/* p3 is checked in msdc_set_tdsel/msdc_set_rdsel */
			if (p2 == 0) {
				msdc_set_rdsel(host, MSDC_TDRDSEL_CUST, p3);
				seq_printf(m, "[SD_Debug]msdc%d, set rd=%d\n",
					id, p3);
			} else if (p2 == 1) { /* set td:1 */
				msdc_set_tdsel(host, MSDC_TDRDSEL_CUST, p3);
				seq_printf(m, "[SD_Debug]msdc%d, set td=%d\n",
					id, p3);
			} else if (p2 == 2) { /* get td/rd:2 */
				msdc_get_rdsel(host, &p3); /* get rd */
				msdc_get_tdsel(host, &p4); /* get td */
				seq_printf(m, "[SD_Debug]msdc%d, rd : 0x%x, td : 0x%x\n",
					id, p3, p4);
			}
		}
	} else if (cmd == SD_TOOL_ENABLE_SMT) {
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];
		msdc_set_smt(host, p2);
		seq_printf(m, "[SD_Debug]smt=%d\n", p2);
	} else if (cmd == RW_BIT_BY_BIT_COMPARE) {
		id = p1;
		compare_count = p2;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (compare_count < 0) {
			seq_printf(m, "[SD_Debug]: bad compare count: %d\n",
				compare_count);
			goto out;
		}

		if (id == 0) { /* for msdc0 */
			multi_rw_compare(m, 0, COMPARE_ADDRESS_MMC,
				compare_count, MMC_TYPE_MMC, 0);
			/*  test the address 0 of eMMC */
		} else {
			multi_rw_compare(m, id, COMPARE_ADDRESS_SD,
				compare_count, MMC_TYPE_SD, 0);
		}
	} else if (cmd == MSDC_READ_WRITE) {
		id = p1;
		mode = p2;	/* 0:stop, 1:read, 2:write */
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (mode > 2 || mode < 0) {
			seq_printf(m, "[SD_Debug]: bad mode: %d\n", mode);
			goto out;
		}
		if (mode == read_write_state) {
			seq_printf(m, "[SD_Debug]: same operation mode=%d.\n",
				read_write_state);
			goto out;
		}
		if (mode == 1 && read_write_state == 2) {
			seq_puts(m, "[SD_Debug]: cannot read in write state, please stop first\n");
			goto out;
		}
		if (mode == 2 && read_write_state == 1) {
			seq_puts(m, "[SD_Debug]: cannot write in read state, please stop first\n");
			goto out;
		}
		read_write_state = mode;

		seq_printf(m, "[SD_Debug]: host id: %d, mode: %d.\n", id, mode);
		if ((mode == 0) && (rw_thread)) {
			kthread_stop(rw_thread);
			rw_thread = NULL;
			seq_puts(m, "[SD_Debug]: stop read/write thread\n");
		} else {
			seq_puts(m, "[SD_Debug]: start read/write thread\n");
			data_for_wr = (id & 0x3) | ((mode & 0x3) << 4);
			rw_thread = kthread_create(rwThread,
				(void *)data_for_wr, "msdc_rw_thread");
			wake_up_process(rw_thread);
		}
	} else if (cmd == SD_TOOL_MSDC_HOST_MODE) {
		id = p2;
		host = mtk_msdc_host[id];
		spd_mode = p3;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (p1 == 1) {
			mmc_claim_host(host->mmc);
			msdc_set_host_mode_speed(m, host->mmc, spd_mode);
			mmc_release_host(host->mmc);
		}
		msdc_get_host_mode_speed(m, host->mmc);
	} else if (cmd == SD_TOOL_DMA_STATUS) {
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (p2 == 0) {
			static char const * const str[] = {
				"No data transaction or the device is not present until now\n",
				"DMA mode is disabled Now\n",
				"Write from SD to DRAM in DMA mode\n",
				"Write from DRAM to SD in DMA mode\n"
			};
			dma_status = msdc_get_dma_status(id);
			seq_printf(m, ">>>> msdc%d: dma_status=%d, ", id,
				dma_status);
			seq_printf(m, "%s", str[dma_status+1]);
			if (dma_status == -1)
				goto out;
#ifdef MSDC_DMA_ADDR_DEBUG
			dma_address = msdc_get_dma_address(id);
			if (dma_address) {
				seq_printf(m, ">>>> msdc%d:\n", id);
				p_dma_address = dma_address;
				while (p_dma_address) {
					seq_printf(m, ">>>>     addr=0x%x, size=%d\n",
						p_dma_address->start_address,
						p_dma_address->size);
					if (p_dma_address->end)
						break;
					p_dma_address = p_dma_address->next;
				}
			} else {
				seq_printf(m, ">>>> msdc%d: BD count=0\n", id);
			}
#else
			seq_puts(m, "please enable MSDC_DMA_ADDR_DEBUG at mtk_sd.h if you want dump dma address\n");
#endif
		} else if (p2 == 1) {
			seq_printf(m, ">>>> msdc%d: start dma violation test\n",
				id);
			g_dma_debug[id] = 1;
			multi_rw_compare(m, id, COMPARE_ADDRESS_SD, 3,
				MMC_TYPE_SD, 0);
		}
	} else if (cmd == MMC_EDC_EMMC_CACHE) {
		seq_puts(m, "==== MSDC Cache Feature Test ====\n");
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;

		host = mtk_msdc_host[id];

		switch (p2) {
		case 0:
			msdc_enable_emmc_cache(m, host, 0);
			break;
		case 1:
			msdc_enable_emmc_cache(m, host, 1);
			break;
		case 2:
		default:
			msdc_check_emmc_cache_status(m, host);
			break;
		}
	} else if (cmd == MMC_DUMP_GPD) {
		seq_puts(m, "==== MSDC DUMP GPD/BD ====\n");
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		else
			msdc_dump_gpd_bd(id);
	} else if (cmd == SD_TOOL_SDIO_PROFILE) {
		if (p1 == 1) {	/* enable profile */
			sdio_pro_enable = 1;
			if (p2 == 0)
				p2 = 1;
			if (p2 >= 30)
				p2 = 30;
			sdio_pro_time = p2;
		} else if (p1 == 0) {
			/* todo */
			sdio_pro_enable = 0;
		}
	} else if (cmd == SMP_TEST_ON_ONE_HOST) {
		id = p1;
		thread_num = p2;
		compare_count = p3;
		multi_address = p4;
		smp_test_on_hosts(m, thread_num, id, compare_count,
			multi_address);
	} else if (cmd == SMP_TEST_ON_ALL_HOST) {
		thread_num = p1;
		compare_count = p2;
		multi_address = p3;
		smp_test_on_hosts(m, thread_num, HOST_MAX_NUM, compare_count,
			multi_address);
#ifdef MTK_IO_PERFORMANCE_DEBUG
	} else if (cmd == MMC_PERF_DEBUG) {
		/* 1 enable; 0 disable */
		g_mtk_mmc_perf_dbg = p1;
		g_mtk_mmc_dbg_range = p2;

		if (g_mtk_mmc_dbg_range == 2) {
			g_dbg_range_start = p3;
			g_dbg_range_end = p3 + p4;
			g_check_read_write = p5;
		}
		seq_printf(m, "g_mtk_mmc_perf_dbg = 0x%x, g_mtk_mmc_dbg_range = 0x%x, start = 0x%x, end = 0x%x\n",
			g_mtk_mmc_perf_dbg, g_mtk_mmc_dbg_range,
			g_dbg_range_start, g_dbg_range_end);
	} else if (cmd == MMC_PERF_DEBUG_PRINT) {
		int i, j, k, num = 0;

		if (p1 == 0) {
			g_mtk_mmc_clear = 0;
			goto out;
		}
		seq_printf(m, "msdc g_dbg_req_count<%d>\n", g_dbg_req_count);
		for (i = 1; i <= g_dbg_req_count; i++) {
			seq_printf(m, "analysis: %s 0x%x %d block, PGh %d\n",
				(g_check_read_write == 18 ? "read" : "write"),
				(unsigned int)g_mmcqd_buf[i][298],
				(unsigned int)g_mmcqd_buf[i][299],
				(unsigned int)(g_mmcqd_buf[i][297] * 2));
			if (g_check_read_write == 18) {
				for (j = 1; j <= g_mmcqd_buf[i][296] * 2;
					j++) {
					seq_printf(m, "page %d:\n", num+1);
					for (k = 0; k < 5; k++) {
						seq_printf(m, "%d %llu\n",
							k, g_req_buf[num][k]);
					}
					num += 1;
				}
			}
			seq_puts(m, "----------------------------------\n");
			for (j = 0; j < sizeof(g_time_mark)/sizeof(char *);
				j++) {
				seq_printf(m, "%d. %llu %s\n", j,
					g_mmcqd_buf[i][j], g_time_mark[j]);
			}
			seq_puts(m, "==================================\n");
		}
		if (g_check_read_write == 25) {
			seq_printf(m, "msdc g_dbg_write_count<%d>\n",
				g_dbg_write_count);
			for (i = 1; i <= g_dbg_write_count; i++) {
				seq_puts(m, "**********************************\n");
				seq_printf(m, "write count: %llu\n",
					g_req_write_count[i]);
				for (j = 0; j < sizeof(g_time_mark_vfs_write)/
					sizeof(char *); j++) {
					seq_printf(m, "%d. %llu %s\n", j,
						g_req_write_buf[i][j],
						g_time_mark_vfs_write[j]);
				}
			}
			seq_puts(m, "**********************************\n");
		}
		g_mtk_mmc_clear = 0;
#endif
#ifdef MTK_MMC_PERFORMANCE_TEST
	} else if (cmd == MMC_PERF_TEST) {
		/* 1 enable; 0 disable */
		g_mtk_mmc_perf_test = p1;
#endif

#ifdef MTK_MSDC_ERROR_TUNE_DEBUG
	} else if (cmd == MMC_ERROR_TUNE) {
		msdc_error_tune_debug_print(m, p1, p2, p3, p4, p5);
#endif
	} else if (cmd == MMC_CRC_STRESS) {
#ifdef CONFIG_MTK_EMMC_SUPPORT
		/* FIX ME, move to user space */
		seq_puts(m, "==== CRC Stress Test ====\n");
		base = mtk_msdc_host[0]->base;
		if (p1 == 0) {
			/* do nothing */
		} else if (p1 == 1)  {
			MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
				MSDC_EMMC50_PAD_DS_TUNE_DLY1, 0x1c);
			MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
				MSDC_EMMC50_PAD_DS_TUNE_DLY3, 0xe);
			if (host->base_top) {
				void __iomem *base_top = host->base_top;

				MSDC_SET_FIELD(TOP_EMMC50_PAD_DS_TUNE,
					PAD_DS_DLY1, 0x1c);
				MSDC_SET_FIELD(TOP_EMMC50_PAD_DS_TUNE,
					PAD_DS_DLY3, 0xe);
			}
		} else if (p1 == 2)  {
			MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, 1);
			MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY,
				p2);
			pr_err("[****MMC_CRC_STRESS****] CMDRDLY<%d>\n", p2);
		}
#endif
	} else if (cmd == DO_AUTOK_OFFLINE_TUNE_TX) {
		host = mtk_msdc_host[p1]; /*p1 = id */
		mmc_claim_host(host->mmc);
		#if 0
		do_autok_offline_tune_tx = 1;
		host->mmc->ops->execute_tuning(host->mmc,
			MMC_SEND_TUNING_BLOCK_HS200);
		do_autok_offline_tune_tx = 0;
		#else
		/*autok_offline_tuning_TX(host);*/
		#endif
		mmc_release_host(host->mmc);
	} else if (cmd == SDIO_AUTOK_RESULT) {
		id = p1;
		vcore = p2;
		mode = p3;
		host = mtk_msdc_host[id];
		/* pr_err("[****AutoK test****]msdc host_id<%d> vcore<%d> mode<%d>\n",
		 *	id, vcore, mode);
		 */

		if ((vcore < 0) || (vcore >= AUTOK_VCORE_NUM))
			vcore = AUTOK_VCORE_LEVEL0;
		res = host->autok_res[vcore];

		if (mode == 0) {
			if (sdio_autok_res_apply(host, vcore) != 0)
				pr_err("sdio autok result not exist!\n");
		} else if (mode == 1) {
			sdio_autok_res_save(host, vcore, res);
		} else if (mode == 2) {
			msdc_dump_sdio_setting(host, m);
		} else if (mode == 3) {
			msdc_dump_autok_setting(host, m);
			msdc_dump_autok(host);
		}
	} else if (cmd == MMC_CMDQ_STATUS) {
		seq_puts(m, "==== eMMC CMDQ Feature ====\n");
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];
		msdc_cmdq_func(host, p2);
	}

out:
	return 0;

invalid_host_id:
	seq_printf(m, "[SD_Debug]invalid host id: %d\n", id);
	return 1;
}

static ssize_t msdc_debug_proc_write(struct file *file, const char *buf,
	size_t count, loff_t *data)
{
	int ret;

	if (count == 0)
		return -1;
	if (count > 255)
		count = 255;
	g_count = count;
	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;
	return count;
}

static int msdc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_debug_proc_show, inode->i_private);
}

static const struct file_operations msdc_proc_fops = {
	.open = msdc_proc_open,
	.write = msdc_debug_proc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int msdc_help_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_help_proc_show, inode->i_private);
}
static const struct file_operations msdc_help_fops = {
	.open = msdc_help_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifndef USER_BUILD_KERNEL
#define PROC_PERM		0660
#else
#define PROC_PERM		0440
#endif

int msdc_debug_proc_init(void)
{
	struct proc_dir_entry *prEntry;
	kuid_t uid;
	kgid_t gid;
#ifdef MSDC_HQA
	struct proc_dir_entry *voltage_flag;
#endif
	uid = make_kuid(&init_user_ns, 0);
	gid = make_kgid(&init_user_ns, 1001);

	prEntry = proc_create("msdc_debug", PROC_PERM, NULL, &msdc_proc_fops);

	if (prEntry)
		proc_set_user(prEntry, uid, gid);
	else
		pr_err("[%s]: failed to create /proc/msdc_debug\n", __func__);

	prEntry = proc_create("msdc_help", PROC_PERM, NULL, &msdc_help_fops);

	if (!prEntry)
		pr_err("[%s]: failed to create /proc/msdc_help\n", __func__);

#ifdef MSDC_HQA
	voltage_flag = proc_create("msdc_voltage_flag", PROC_PERM, NULL,
		&msdc_voltage_flag_fops);

	if (voltage_flag)
		proc_set_user(voltage_flag, uid, gid);
	else
		pr_err("[%s]: failed to create /proc/msdc_voltage_flag\n",
			__func__);
#endif

#ifdef MSDC_DMA_ADDR_DEBUG
	msdc_init_dma_latest_address();
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(msdc_debug_proc_init);
