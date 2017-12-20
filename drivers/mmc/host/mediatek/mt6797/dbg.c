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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mmc/host.h>
#include <linux/seq_file.h>
#include <mach/mt_gpt.h>
#include <asm/io.h>
/* for fpga early porting */
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/scatterlist.h>
#include <linux/mm_types.h>
/* end for fpga early porting */
#include "mt_sd.h"
#include "dbg.h"
#include "autok.h"
#ifndef FPGA_PLATFORM
#include <mt-plat/upmu_common.h>
#endif
#ifdef MTK_MSDC_BRINGUP_DEBUG
#include <mach/mt_pmic_wrap.h>
#endif

#define MTK_EMMC_CMD_DEBUG

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

#define	EMMC_HS400	((UHS_DDR50) + 1)	/* 0x7 Host supports EMMC HS400 mode */
#define CAPS_SPEED_NULL ((EMMC_HS400) + 1)
#define CAPS_DRIVE_NULL ((DRIVER_TYPE_D) + 1)
static char cmd_buf[256];
struct task_struct *rw_thread = NULL;
int sdio_cd_result = 1;

/* for driver profile */
#define TICKS_ONE_MS  (13000)
u32 gpt_enable = 0;
u32 sdio_pro_enable = 0;	/* make sure gpt is enabled */
static unsigned long long sdio_pro_time = 30;	/* no more than 30s */
static unsigned long long sdio_profiling_start;
struct sdio_profile sdio_perfomance = { 0 };

/*#define MTK_MSDC_ERROR_TUNE_DEBUG*/

#ifdef MSDC_DMA_ADDR_DEBUG
struct dma_addr msdc_latest_dma_address[MAX_BD_PER_GPD];

struct dma_addr *msdc_get_dma_address(int host_id)
{
	struct bd_t *bd;
	int i = 0;
	int mode = -1;
	struct msdc_host *host;
	void __iomem *base;

	if (host_id < 0 || host_id >= HOST_MAX_NUM) {
		pr_err("[%s] invalid host_id %d\n", __func__, host_id);
		return NULL;
	}

	if (!mtk_msdc_host[host_id]) {
		pr_err("[%s] msdc%d does not exist\n", __func__, host_id);
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
EXPORT_SYMBOL(msdc_get_dma_address);

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
void msdc_select_card_type(struct mmc_host *host)
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
#define pr_register(m, OFFSET, REG)     \
	seq_printf(m, "R[%x]=0x%.8x\n", OFFSET, MSDC_READ32(REG))

void msdc_dump_register_debug(struct seq_file *m, u32 id, void __iomem *base)
{
	pr_register(m, OFFSET_MSDC_CFG,                 MSDC_CFG);
	pr_register(m, OFFSET_MSDC_IOCON,               MSDC_IOCON);
	pr_register(m, OFFSET_MSDC_PS,                  MSDC_PS);
	pr_register(m, OFFSET_MSDC_INT,                 MSDC_INT);
	pr_register(m, OFFSET_MSDC_INTEN,               MSDC_INTEN);
	pr_register(m, OFFSET_MSDC_FIFOCS,              MSDC_FIFOCS);
	seq_printf(m, "R[%x]=not read\n", OFFSET_MSDC_TXDATA);
	seq_printf(m, "R[%x]=not read\n", OFFSET_MSDC_RXDATA);
	pr_register(m, OFFSET_SDC_CFG,                  SDC_CFG);
	pr_register(m, OFFSET_SDC_CMD,                  SDC_CMD);
	pr_register(m, OFFSET_SDC_ARG,                  SDC_ARG);
	pr_register(m, OFFSET_SDC_STS,                  SDC_STS);
	pr_register(m, OFFSET_SDC_RESP0,                SDC_RESP0);
	pr_register(m, OFFSET_SDC_RESP1,                SDC_RESP1);
	pr_register(m, OFFSET_SDC_RESP2,                SDC_RESP2);
	pr_register(m, OFFSET_SDC_RESP3,                SDC_RESP3);
	pr_register(m, OFFSET_SDC_BLK_NUM,              SDC_BLK_NUM);
	pr_register(m, OFFSET_SDC_VOL_CHG,              SDC_VOL_CHG);
	pr_register(m, OFFSET_SDC_CSTS,                 SDC_CSTS);
	pr_register(m, OFFSET_SDC_CSTS_EN,              SDC_CSTS_EN);
	pr_register(m, OFFSET_SDC_DCRC_STS,             SDC_DCRC_STS);
	pr_register(m, OFFSET_EMMC_CFG0,                EMMC_CFG0);
	pr_register(m, OFFSET_EMMC_CFG1,                EMMC_CFG1);
	pr_register(m, OFFSET_EMMC_STS,                 EMMC_STS);
	pr_register(m, OFFSET_EMMC_IOCON,               EMMC_IOCON);
	pr_register(m, OFFSET_SDC_ACMD_RESP,            SDC_ACMD_RESP);
	pr_register(m, OFFSET_SDC_ACMD19_TRG,           SDC_ACMD19_TRG);
	pr_register(m, OFFSET_SDC_ACMD19_STS,           SDC_ACMD19_STS);
	pr_register(m, OFFSET_MSDC_DMA_SA_HIGH,         MSDC_DMA_SA_HIGH);
	pr_register(m, OFFSET_MSDC_DMA_SA,              MSDC_DMA_SA);
	pr_register(m, OFFSET_MSDC_DMA_CA,              MSDC_DMA_CA);
	pr_register(m, OFFSET_MSDC_DMA_CTRL,            MSDC_DMA_CTRL);
	pr_register(m, OFFSET_MSDC_DMA_CFG,             MSDC_DMA_CFG);
	pr_register(m, OFFSET_MSDC_DMA_LEN,             MSDC_DMA_LEN);
	pr_register(m, OFFSET_MSDC_DBG_SEL,             MSDC_DBG_SEL);
	pr_register(m, OFFSET_MSDC_DBG_OUT,             MSDC_DBG_OUT);
	pr_register(m, OFFSET_MSDC_PATCH_BIT0,          MSDC_PATCH_BIT0);
	pr_register(m, OFFSET_MSDC_PATCH_BIT1,          MSDC_PATCH_BIT1);
	pr_register(m, OFFSET_MSDC_PATCH_BIT2,          MSDC_PATCH_BIT2);

	if ((id != 2) && (id != 3))
		goto skip_sdio_tune_reg;

	pr_register(m, OFFSET_DAT0_TUNE_CRC,            DAT0_TUNE_CRC);
	pr_register(m, OFFSET_DAT0_TUNE_CRC,            DAT1_TUNE_CRC);
	pr_register(m, OFFSET_DAT0_TUNE_CRC,            DAT2_TUNE_CRC);
	pr_register(m, OFFSET_DAT0_TUNE_CRC,            DAT3_TUNE_CRC);
	pr_register(m, OFFSET_CMD_TUNE_CRC,             CMD_TUNE_CRC);
	pr_register(m, OFFSET_SDIO_TUNE_WIND,           SDIO_TUNE_WIND);

skip_sdio_tune_reg:
	pr_register(m, OFFSET_MSDC_PAD_TUNE0,           MSDC_PAD_TUNE0);
	pr_register(m, OFFSET_MSDC_PAD_TUNE1,           MSDC_PAD_TUNE1);
	pr_register(m, OFFSET_MSDC_DAT_RDDLY0,          MSDC_DAT_RDDLY0);
	pr_register(m, OFFSET_MSDC_DAT_RDDLY1,          MSDC_DAT_RDDLY1);
	pr_register(m, OFFSET_MSDC_DAT_RDDLY2,          MSDC_DAT_RDDLY2);
	pr_register(m, OFFSET_MSDC_DAT_RDDLY3,          MSDC_DAT_RDDLY3);
	pr_register(m, OFFSET_MSDC_HW_DBG,              MSDC_HW_DBG);
	pr_register(m, OFFSET_MSDC_VERSION,             MSDC_VERSION);

	if (id != 0)
		goto skip_emmc50_reg;

	pr_register(m, OFFSET_EMMC50_PAD_DS_TUNE,       EMMC50_PAD_DS_TUNE);
	pr_register(m, OFFSET_EMMC50_PAD_CMD_TUNE,      EMMC50_PAD_CMD_TUNE);
	pr_register(m, OFFSET_EMMC50_PAD_DAT01_TUNE,    EMMC50_PAD_DAT01_TUNE);
	pr_register(m, OFFSET_EMMC50_PAD_DAT23_TUNE,    EMMC50_PAD_DAT23_TUNE);
	pr_register(m, OFFSET_EMMC50_PAD_DAT45_TUNE,    EMMC50_PAD_DAT45_TUNE);
	pr_register(m, OFFSET_EMMC50_PAD_DAT67_TUNE,    EMMC50_PAD_DAT67_TUNE);
	pr_register(m, OFFSET_EMMC51_CFG0,              EMMC51_CFG0);
	pr_register(m, OFFSET_EMMC50_CFG0,              EMMC50_CFG0);
	pr_register(m, OFFSET_EMMC50_CFG1,              EMMC50_CFG1);
	pr_register(m, OFFSET_EMMC50_CFG2,              EMMC50_CFG2);
	pr_register(m, OFFSET_EMMC50_CFG3,              EMMC50_CFG3);
	pr_register(m, OFFSET_EMMC50_CFG4,              EMMC50_CFG4);

skip_emmc50_reg:
	return;
}

#ifdef MTK_EMMC_CMD_DEBUG
#define dbg_max_cnt (500)
struct dbg_run_host_log {
	unsigned long long time_sec;
	unsigned long long time_usec;
	int type;
	int cmd;
	int arg;
	int skip;
};
static struct dbg_run_host_log dbg_run_host_log_dat[dbg_max_cnt];
static int dbg_host_cnt;
static int is_lock_init;
static spinlock_t cmd_dump_lock;

static unsigned int printk_cpu_test = UINT_MAX;
struct timeval cur_tv;

/* type 0: cmd, type 1 rsp */
void dbg_add_host_log(struct mmc_host *mmc, int type, int cmd, int arg)
{
	unsigned long long t;
	unsigned long long nanosec_rem;
	unsigned long flags;
	static int last_cmd, last_arg, skip;
	int l_skip = 0;
	struct msdc_host *host = mmc_priv(mmc);

	/* only log msdc0 */
	if (!host || host->id != 0)
		return;

	if (!is_lock_init) {
		spin_lock_init(&cmd_dump_lock);
		is_lock_init = 1;
	}

	spin_lock_irqsave(&cmd_dump_lock, flags);
	if (type == 1) {
		/*skip log if last cmd rsp are the same*/
		if (last_cmd == cmd && last_arg == arg) {
			skip++;
			if (dbg_host_cnt == 0)
				dbg_host_cnt = dbg_max_cnt;
			/*remove type = 0, command*/
			dbg_host_cnt--;
			goto end;
		}
		last_cmd = cmd;
		last_arg = arg;
		l_skip = skip;
		skip = 0;
	}
	t = cpu_clock(printk_cpu_test);
	nanosec_rem = do_div(t, 1000000000)/1000;
	do_gettimeofday(&cur_tv);
	dbg_run_host_log_dat[dbg_host_cnt].time_sec = t;
	dbg_run_host_log_dat[dbg_host_cnt].time_usec = nanosec_rem;
	dbg_run_host_log_dat[dbg_host_cnt].type = type;
	dbg_run_host_log_dat[dbg_host_cnt].cmd = cmd;
	dbg_run_host_log_dat[dbg_host_cnt].arg = arg;
	dbg_run_host_log_dat[dbg_host_cnt].skip = l_skip;
	dbg_host_cnt++;
	if (dbg_host_cnt >= dbg_max_cnt)
		dbg_host_cnt = 0;
end:
	spin_unlock_irqrestore(&cmd_dump_lock, flags);
}

void mmc_cmd_dump(struct seq_file *m, struct mmc_host *mmc, u32 latest_cnt)
{
	int i, j;
	int tag = -1;
	int is_read, is_rel, is_fprg;
	unsigned long flags;
	unsigned long long time_sec, time_usec;
	int type, cmd, arg, skip, cnt;
	struct msdc_host *host;
	u32 dump_cnt;

	if (!mmc || !mmc->card)
		return;
	/* only dump msdc0 */
	host = mmc_priv(mmc);
	if (!host || host->id != 0)
		return;
	if (!is_lock_init) {
		spin_lock_init(&cmd_dump_lock);
		is_lock_init = 1;
	}

	spin_lock_irqsave(&cmd_dump_lock, flags);
	dump_cnt = min_t(u32, latest_cnt, dbg_max_cnt);

	i = dbg_host_cnt - 1;
	if (i < 0)
		i = dbg_max_cnt - 1;

	for (j = 0; j < dump_cnt; j++) {
		time_sec = dbg_run_host_log_dat[i].time_sec;
		time_usec = dbg_run_host_log_dat[i].time_usec;
		type = dbg_run_host_log_dat[i].type;
		cmd = dbg_run_host_log_dat[i].cmd;
		arg = dbg_run_host_log_dat[i].arg;
		skip = dbg_run_host_log_dat[i].skip;
		if (cmd == 44 && !type) {
			cnt = arg & 0xffff;
			tag = (arg >> 16) & 0xf;
			is_read = (arg >> 30) & 0x1;
			is_rel = (arg >> 31) & 0x1;
			is_fprg = (arg >> 24) & 0x1;
			MSDC_PRINFO_PROC_MSG(m,
			"%04d [%5llu.%06llu]%2d %2d %08x id=%02d %s cnt=%d %d %d\n",
				j, time_sec, time_usec,
				type, cmd, arg, tag,
				is_read ? "R" : "W",
				cnt, is_rel, is_fprg);
		} else if ((cmd == 46 || cmd == 47) && !type) {
			tag = (arg >> 16) & 0xf;
			MSDC_PRINFO_PROC_MSG(m, "%04d [%5llu.%06llu]%2d %2d %08x id=%02d\n",
				j, time_sec, time_usec,
				type, cmd, arg, tag);
		} else
			MSDC_PRINFO_PROC_MSG(m, "%04d [%5llu.%06llu]%2d %2d %08x (%d)\n",
				j, time_sec, time_usec,
				type, cmd, arg, skip);
		i--;
		if (i < 0)
			i = dbg_max_cnt - 1;
	}
	spin_unlock_irqrestore(&cmd_dump_lock, flags);
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	MSDC_PRINFO_PROC_MSG(m, "areq_cnt:%d, task_id_index %08lx, cq_wait_rdy:%d\n",
			atomic_read(&mmc->areq_cnt), mmc->task_id_index, atomic_read(&mmc->cq_wait_rdy));
#endif
}

#else
void dbg_add_host_log(struct mmc_host *mmc, int type, int cmd, int arg)
{
}

void mmc_cmd_dump(struct seq_file *m, struct mmc_host *mmc, u32 latest_cnt)
{
}
#endif

static void msdc_dump_host_state(struct seq_file *m, struct msdc_host *host)
{
	/* need porting */
}

static void msdc_proc_dump(struct seq_file *m, u32 id)
{
	struct msdc_host *host = mtk_msdc_host[id];

	if (host == NULL) {
		pr_info("====== Null msdc%d, dump skipped ======\n", id);
		return;
	}

	msdc_dump_host_state(m, host);
	mmc_cmd_dump(m, host->mmc, 500);
}

void msdc_hang_detect_dump(u32 id)
{
	struct msdc_host *host = mtk_msdc_host[id];

	if (host == NULL) {
		pr_info("====== Null msdc%d, dump skipped ======\n", id);
		return;
	}

	msdc_dump_host_state(NULL, host);
	mmc_cmd_dump(NULL, host->mmc, 50);
}
EXPORT_SYMBOL(msdc_hang_detect_dump);

void msdc_cmdq_status_print(struct seq_file *m, struct msdc_host *host)
{
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	struct mmc_host *mmc = host->mmc;

	if (!mmc)
		return;

	seq_puts(m, "===============================\n");
	seq_printf(m, "cmdq support : %s\n",
		mmc->card->ext_csd.cmdq_support ? "yes":"no");
	seq_printf(m, "cmdq mode    : %s\n",
		mmc->card->ext_csd.cmdq_mode_en ? "enable" : "disable");
	seq_printf(m, "cmdq depth   : %d\n",
		mmc->card->ext_csd.cmdq_depth);
	seq_puts(m, "===============================\n");
	seq_printf(m, "task_id_index: %08lx\n",
		mmc->task_id_index);
	seq_printf(m, "cq_wait_rdy  : %d\n",
		atomic_read(&mmc->cq_wait_rdy));
	seq_printf(m, "cq_tuning_now: %d\n",
		atomic_read(&mmc->cq_tuning_now));

#else
	seq_puts(m, "driver not supported\n");
#endif
}

void msdc_cmdq_func(struct seq_file *m, struct msdc_host *host, const int num)
{
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	void __iomem *base = NULL;
	int a, b;

	if (host == NULL)
		return;

	base = host->base;
#endif

	if (!host || !host->mmc || !host->mmc->card)
		return;

	switch (num) {
	case 0:
		msdc_cmdq_status_print(m, host);
		break;
#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	case 1:
		seq_puts(m, "force enable cmdq\n");
		host->mmc->card->ext_csd.cmdq_support = 1;
		host->mmc->cmdq_support_changed = 1;
		break;
	case 2:
		mmc_cmd_dump(m, host->mmc, 500);
		break;
	case 3:
		MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, a);
		MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, a+1);
		MSDC_GET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, b);
		seq_printf(m, "force MSDC_PAD_TUNE0_CMDRDLY %d -> %d\n", a, b);
		break;
	case 4:
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, a);
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, a+1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY1, b);
		seq_printf(m, "force MSDC_EMMC50_PAD_DS_TUNE_DLY1 %d -> %d\n", a, b);
		break;
	case 5:
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY2, a);
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY2, a+1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY2, b);
		seq_printf(m, "force MSDC_EMMC50_PAD_DS_TUNE_DLY2 %d -> %d\n", a, b);
		break;
	case 6:
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, a);
		MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, a+1);
		MSDC_GET_FIELD(EMMC50_PAD_DS_TUNE, MSDC_EMMC50_PAD_DS_TUNE_DLY3, b);
		seq_printf(m, "force MSDC_EMMC50_PAD_DS_TUNE_DLY3  %d -> %d\n", a, b);
		break;
#endif
	default:
		seq_printf(m, "unknown function id %d\n", num);
		break;
	}
}

void msdc_set_host_mode_speed(struct seq_file *m, struct msdc_host *host,
		int spd_mode, int cmdq)
{
	/* Clear HS400, HS200 timing */
	host->mmc->caps2 &=
		~(MMC_CAP2_HS400_1_8V | MMC_CAP2_HS200_1_8V_SDR);
	/* Clear other timing */
	host->mmc->caps &= ~(MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED |
			     MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
			     MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 |
			     MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR);
	switch (spd_mode) {
	case MMC_TIMING_LEGACY:
		seq_puts(m, "[SD_Debug]host  support MMC_TIMING_LEGACY\n");
		break;
	case MMC_TIMING_MMC_HS:
		host->mmc->caps |= MMC_CAP_MMC_HIGHSPEED;
		seq_puts(m, "[SD_Debug]host  support MMC_TIMING_MMC_HS\n");
		break;
	case MMC_TIMING_SD_HS:
		host->mmc->caps |= MMC_CAP_SD_HIGHSPEED;
		seq_puts(m, "[SD_Debug]host  support MMC_TIMING_SD_HS\n");
		break;
	case MMC_TIMING_UHS_SDR12:
		host->mmc->caps |= MMC_CAP_UHS_SDR12;
		seq_puts(m, "[SD_Debug]host  support MMC_CAP_UHS_SDR12\n");
		break;
	case MMC_TIMING_UHS_SDR25:
		host->mmc->caps |= MMC_CAP_UHS_SDR25;
		host->mmc->caps |= MMC_CAP_UHS_SDR12;
		seq_puts(m, "[SD_Debug]host  support MMC_CAP_UHS_SDR12\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_SDR25\n");
		break;
	case MMC_TIMING_UHS_SDR50:
		host->mmc->caps |= MMC_CAP_UHS_SDR50;
		host->mmc->caps |= MMC_CAP_UHS_SDR25;
		host->mmc->caps |= MMC_CAP_UHS_SDR12;
		seq_puts(m, "[SD_Debug]host  support MMC_CAP_UHS_SDR12\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_SDR25\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_SDR50\n");
		break;
	case MMC_TIMING_UHS_SDR104:
		host->mmc->caps |= MMC_CAP_UHS_SDR104;
		host->mmc->caps |= MMC_CAP_UHS_DDR50;
		host->mmc->caps |= MMC_CAP_UHS_SDR50;
		host->mmc->caps |= MMC_CAP_UHS_SDR25;
		host->mmc->caps |= MMC_CAP_UHS_SDR12;
		seq_puts(m, "[SD_Debug]host  support MMC_CAP_UHS_SDR12\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_SDR25\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_SDR50\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_DDR50\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_SDR104\n");
		break;
	case MMC_TIMING_UHS_DDR50:
		host->mmc->caps |= MMC_CAP_UHS_DDR50;
		host->mmc->caps |= MMC_CAP_UHS_SDR50;
		host->mmc->caps |= MMC_CAP_UHS_SDR25;
		host->mmc->caps |= MMC_CAP_UHS_SDR12;
		seq_puts(m, "[SD_Debug]host  support MMC_CAP_UHS_SDR12\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_SDR25\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_SDR50\n");
		seq_puts(m, "[SD_Debug]              MMC_CAP_UHS_DDR50\n");
		break;
	case MMC_TIMING_MMC_DDR52:
		host->mmc->caps |= MMC_CAP_1_8V_DDR;
		host->mmc->caps |= MMC_CAP_MMC_HIGHSPEED;
		seq_puts(m, "[SD_Debug]host  support MMC_TIMING_MMC_HS\n");
		seq_puts(m, "[SD_Debug]              MMC_TIMING_MMC_DDR52\n");
		break;
	case MMC_TIMING_MMC_HS200:
		host->mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
		host->mmc->caps |= MMC_CAP_1_8V_DDR;
		host->mmc->caps |= MMC_CAP_MMC_HIGHSPEED;
		seq_puts(m, "[SD_Debug]host  support MMC_TIMING_MMC_HS\n");
		seq_puts(m, "[SD_Debug]              MMC_TIMING_MMC_DDR52\n");
		seq_puts(m, "[SD_Debug]              MMC_TIMING_MMC_HS200\n");
		break;
	case MMC_TIMING_MMC_HS400:
		host->mmc->caps2 |= MMC_CAP2_HS400_1_8V;
		host->mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
		host->mmc->caps |= MMC_CAP_1_8V_DDR;
		host->mmc->caps |= MMC_CAP_MMC_HIGHSPEED;
		seq_puts(m, "[SD_Debug]host  support MMC_TIMING_MMC_HS\n");
		seq_puts(m, "[SD_Debug]              MMC_TIMING_MMC_DDR52\n");
		seq_puts(m, "[SD_Debug]              MMC_TIMING_MMC_HS200\n");
		seq_puts(m, "[SD_Debug]              MMC_TIMING_MMC_HS400\n");
		break;
	default:
		seq_printf(m, "[SD_Debug]invalid speed mode:%d\n",
			spd_mode);
		break;
	}

#ifdef CONFIG_MTK_EMMC_CQ_SUPPORT
	if (cmdq) {
		seq_puts(m, "[SD_Debug] enable command queue feature\n");
		host->mmc->card->ext_csd.cmdq_support = 1;
		host->mmc->cmdq_support_changed = 1;
	} else {
		seq_puts(m, "[SD_Debug] disable command queue feature\n");
		host->mmc->card->ext_csd.cmdq_support = 0;
		host->mmc->cmdq_support_changed = 1;
		host->mmc->card->ext_csd.cmdq_mode_en = 0;
	}
#else
	seq_puts(m, "[SD_Debug] not support command queue feature yet\n");
#endif

	/*
	 * support hw reset operation
	 */
	host->mmc->caps |= MMC_CAP_HW_RESET;
	msdc_select_card_type(host->mmc);
	mmc_claim_host(host->mmc);
	/* Must set mmc_host ios.time = MMC_TIMING_LEGACY,
	 * or clock will not be setted to 400K before mmc_init_card
	 * CMD1 will timeout
	 */
	host->mmc->ios.timing = MMC_TIMING_LEGACY;
	host->mmc->ios.clock = 26000;
	if (mmc_hw_reset(host->mmc))
		seq_puts(m, "[SD_Debug] Reinit card failed, Can not switch speed mode\n");
	mmc_release_host(host->mmc);
	host->mmc->caps &= ~MMC_CAP_HW_RESET;
}
void msdc_get_host_mode_speed(struct seq_file *m,
		struct msdc_host *host, int spd_mode)
{
	seq_printf(m, "[SD_Debug]msdc[%d] supports:\n", host->id);

	if (host->mmc->caps & MMC_CAP_MMC_HIGHSPEED)
		seq_puts(m, "[SD_Debug]      MMC_TIMING_MMC_HS\n");
	if (host->mmc->caps & MMC_CAP_SD_HIGHSPEED)
		seq_puts(m, "[SD_Debug]      MMC_TIMING_SD_HS\n");
	if (host->mmc->caps & MMC_CAP_UHS_SDR12)
		seq_puts(m, "[SD_Debug]      MMC_CAP_UHS_SDR12\n");
	if (host->mmc->caps & MMC_CAP_UHS_SDR25)
		seq_puts(m, "[SD_Debug]      MMC_CAP_UHS_SDR25\n");
	if (host->mmc->caps & MMC_CAP_UHS_SDR104)
		seq_puts(m, "[SD_Debug]      MMC_CAP_UHS_SDR104\n");
	if (host->mmc->caps & MMC_CAP_UHS_DDR50)
		seq_puts(m, "[SD_Debug]      MMC_CAP_UHS_DDR50\n");
	if (host->mmc->caps & MMC_CAP_1_8V_DDR)
		seq_puts(m, "[SD_Debug]      MMC_CAP_MMC_DDR52\n");
	if (host->mmc->caps2 & MMC_CAP2_HS200_1_8V_SDR)
		seq_puts(m, "[SD_Debug]      MMC_TIMING_MMC_HS200\n");
	if (host->mmc->caps2 & MMC_CAP2_HS400_1_8V)
		seq_puts(m, "[SD_Debug]      MMC_TIMING_MMC_HS400\n");

	seq_puts(m, "[SD_Debug] Command queue feature is disable\n");

}

int msdc_reinit(struct msdc_host *host)
{
	struct mmc_host *mmc;
	struct mmc_card *card;
	int ret = -1;

	if (!host) {
		pr_err("msdc_host is NULL\n");
		return -1;
	}
	mmc = host->mmc;
	if (!mmc) {
		ERR_MSG("mmc is NULL");
		return -1;
	}
	card = mmc->card;
	if (card == NULL)
		ERR_MSG("mmc->card is NULL");
	if (host->block_bad_card)
		ERR_MSG("Need block this bad SD card from re-initialization");


	if (host->hw->host_function == MSDC_EMMC)
		return -1;

	if (host->hw->host_function != MSDC_SD)
		goto skip_reinit2;

	if (!(host->mmc->caps & MMC_CAP_NONREMOVABLE) || (host->block_bad_card != 0))
		goto skip_reinit1;
	mmc_claim_host(mmc);
	mmc->ios.timing = MMC_TIMING_LEGACY;
	msdc_ops_set_ios(mmc, &mmc->ios);
	/* power reset sdcard */
	/* ret = mmc->bus_ops->reset(mmc); */
	mmc_release_host(mmc);

	ERR_MSG("Reinit %s", ret == 0 ? "success" : "fail");

skip_reinit1:
	if (!(host->mmc->caps & MMC_CAP_NONREMOVABLE) && (host->mmc->card)
		&& mmc_card_present(host->mmc->card)
		&& (!mmc_card_removed(host->mmc->card))
		&& (host->block_bad_card == 0))
		ret = 0;
skip_reinit2:
	return ret;
}

static void msdc_set_field(struct seq_file *m, void __iomem *address,
	unsigned int start_bit, unsigned int len, unsigned int value)
{
	unsigned long field;

	if (start_bit > 31 || start_bit < 0 || len > 31 || len <= 0
		|| (start_bit + len) > 32) {
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

static void msdc_init_gpt(void)
{
#if 0
	GPT_CONFIG config;

	config.num = GPT6;
	config.mode = GPT_FREE_RUN;
	config.clkSrc = GPT_CLK_SRC_SYS;
	config.clkDiv = GPT_CLK_DIV_1;	/* 13MHz GPT6 */

	if (GPT_Config(config) == FALSE)
		return;

	GPT_Start(GPT6);
#endif
}

u32 msdc_time_calc(u32 old_L32, u32 old_H32, u32 new_L32, u32 new_H32)
{
	u32 ret = 0;

	if (new_H32 == old_H32) {
		ret = new_L32 - old_L32;
	} else if (new_H32 == (old_H32 + 1)) {
		if (new_L32 > old_L32) {
			pr_notice("msdc old_L<0x%x> new_L<0x%x>\n",
				old_L32, new_L32);
		}
		ret = (0xffffffff - old_L32);
		ret += new_L32;
	} else {
		pr_notice("msdc old_H<0x%x> new_H<0x%x>\n",
			old_H32, new_H32);
	}

	return ret;
}

void msdc_sdio_profile(struct sdio_profile *result)
{
	struct cmd_profile *cmd;
	u32 i;

	pr_notice("sdio === performance dump ===\n");
	pr_notice("sdio === total execute tick<%d> time<%dms> Tx<%dB> Rx<%dB>\n",
		result->total_tc, result->total_tc / TICKS_ONE_MS,
		result->total_tx_bytes, result->total_rx_bytes);

	/* CMD52 Dump */
	cmd = &result->cmd52_rx;
	pr_notice("sdio === CMD52 Rx <%d>times tick<%d> Max<%d> Min<%d> Aver<%d>\n",
		cmd->count, cmd->tot_tc,
		cmd->max_tc, cmd->min_tc, cmd->tot_tc / cmd->count);
	cmd = &result->cmd52_tx;
	pr_notice("sdio === CMD52 Tx <%d>times tick<%d> Max<%d> Min<%d> Aver<%d>\n",
		cmd->count, cmd->tot_tc,
		cmd->max_tc, cmd->min_tc, cmd->tot_tc / cmd->count);

	/* CMD53 Rx bytes + block mode */
	for (i = 0; i < 512; i++) {
		cmd = &result->cmd53_rx_byte[i];
		if (cmd->count == 0)
			continue;
		pr_notice("sdio<%6d><%3dB>_Rx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n",
			cmd->count, i, cmd->tot_tc, cmd->max_tc, cmd->min_tc,
			cmd->tot_tc / cmd->count, cmd->tot_bytes,
			(cmd->tot_bytes / 10) * 13 / (cmd->tot_tc / 10));
	}
	for (i = 0; i < 100; i++) {
		cmd = &result->cmd53_rx_blk[i];
		if (cmd->count == 0)
			continue;
		pr_notice("sdio<%6d><%3d>B_Rx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n",
			cmd->count, i, cmd->tot_tc, cmd->max_tc, cmd->min_tc,
			cmd->tot_tc / cmd->count, cmd->tot_bytes,
			(cmd->tot_bytes / 10) * 13 / (cmd->tot_tc / 10));
	}

	/* CMD53 Tx bytes + block mode */
	for (i = 0; i < 512; i++) {
		cmd = &result->cmd53_tx_byte[i];
		if (cmd->count == 0)
			continue;
		pr_notice("sdio<%6d><%3dB>_Tx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n",
			 cmd->count, i, cmd->tot_tc, cmd->max_tc, cmd->min_tc,
			 cmd->tot_tc / cmd->count, cmd->tot_bytes,
			 (cmd->tot_bytes / 10) * 13 / (cmd->tot_tc / 10));
	}
	for (i = 0; i < 100; i++) {
		cmd = &result->cmd53_tx_blk[i];
		if (cmd->count == 0)
			continue;
		pr_notice("sdio<%6d><%3d>B_Tx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n",
			 cmd->count, i, cmd->tot_tc, cmd->max_tc, cmd->min_tc,
			 cmd->tot_tc / cmd->count, cmd->tot_bytes,
			 (cmd->tot_bytes / 10) * 13 / (cmd->tot_tc / 10));
	}

	pr_notice("sdio === performance dump done ===\n");
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

#define COMPARE_ADDRESS_MMC		0x402000
#define COMPARE_ADDRESS_SD		0x2000
#define COMPARE_ADDRESS_SDIO		0x0
#define COMPARE_ADDRESS_SD_COMBO	0x2000

#define MSDC_MULTI_BUF_LEN  (4*4*1024) /*16KB write/read/compare*/

static DEFINE_MUTEX(sd_lock);
static DEFINE_MUTEX(emmc_lock);

u8 read_write_state = 0;	/* 0:stop, 1:read, 2:write */

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

	u8 wData_len;
	u8 *wData;
	u8 wData_emmc[16] = {
		0x67, 0x45, 0x23, 0x01,
		0xef, 0xcd, 0xab, 0x89,
		0xce, 0x8a, 0x46, 0x02,
		0xde, 0x9b, 0x57, 0x13
	};

	u8 wData_sd[200] = {
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

	if (type == MMC_TYPE_MMC) {
		wData = wData_emmc;
		wData_len = 16;
	} else if (type == MMC_TYPE_SD) {
		wData = wData_sd;
		wData_len = 200;
	} else
		return -1;

	if (host_num >= HOST_MAX_NUM || host_num < 0) {
		pr_err("[%s]invalid host id: %d\n", __func__, host_num);
		return -1;
	}

	/*allock memory for test buf*/
	multi_rwbuf = kzalloc((MSDC_MULTI_BUF_LEN), GFP_KERNEL);
	if (multi_rwbuf == NULL) {
		result = -1;
		goto free;
	}
	rPtr = wPtr = (u8 *)multi_rwbuf;

	host_ctl = mtk_msdc_host[host_num];
	if (!host_ctl) {
		pr_err(" host_ctl in host[%d]\n", host_num);
		result = -1;
		goto free;
	}
	mmc = host_ctl->mmc;
	if (!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card) {
		pr_err(" No card initialized in host[%d]\n", host_num);
		result = -1;
		goto free;
	}

	if (!is_card_present(host_ctl)) {
		pr_err("  [%s]: card is removed!\n", __func__);
		result = -1;
		goto free;
	}

	mmc_claim_host(mmc);

#ifdef CONFIG_MTK_EMMC_SUPPORT
	if (!g_ett_tune && (host_ctl->hw->host_function == MSDC_EMMC))
		msdc_switch_part(host_ctl, 0);
#endif

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

	msdc_cmd.arg = address;

	BUG_ON(!host_ctl->mmc->card);

	msdc_stop.opcode = MMC_STOP_TRANSMISSION;
	msdc_stop.arg = 0;
	msdc_stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	msdc_data.stop = &msdc_stop;

	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		/* pr_notice("this device use byte address!!\n"); */
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
	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		result = msdc_cmd.error;

	if (msdc_data.error)
		result = msdc_data.error;
	else
		result = 0;

free:
	kfree(multi_rwbuf);

	return result;
}

int multi_rw_compare(struct seq_file *m, int host_num,
		uint address, int count, uint type, int multi_thread)
{
	int i = 0, j = 0;
	int error = 0;
	struct mutex *rw_mutex;

	if (type == MMC_TYPE_SD)
		rw_mutex = &sd_lock;
	else if (type == MMC_TYPE_MMC)
		rw_mutex = &emmc_lock;
	else
		return 0;

	for (i = 0; i < count; i++) {
		/* pr_notice("== cpu[%d] pid[%d]: start %d time compare ==\n",
			task_cpu(current), current->pid, i); */

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
			mutex_unlock(rw_mutex);
			break;
		}

		/* read */
		for (j = 0; j < 1; j++) {
			error = multi_rw_compare_core(host_num, 1, address,
				type, 1);
			if (error) {
				if (!multi_thread)
					seq_printf(m, "[%s]: failed to read data, error=%d\n",
					__func__, error);
				else
					pr_err("[%s]: failed to read data, error=%d\n",
					__func__, error);
				break;
			}
		}
		if (!multi_thread)
			seq_printf(m, "== cpu[%d] pid[%d]: %s %d time compare ==\n",
				task_cpu(current), current->pid,
				(error ? "FAILED" : "FINISH"), i);
		else
			pr_err("== cpu[%d] pid[%d]: %s %d time compare ==\n",
				task_cpu(current), current->pid,
				(error ? "FAILED" : "FINISH"), i);

		mutex_unlock(rw_mutex);
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
 * so do not to access the first 2GB region */
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
/*
 * 2012-03-25
 * the SMP thread function
 * do read after write the memory card, and bit by bit comparison
 */
static int write_read_thread(void *ptr)
{
	struct write_read_data *data = (struct write_read_data *)ptr;
	struct seq_file *m = data->m;

	if (1 == data->host_id) {
		pr_err("sd thread start\n");
		multi_rw_compare(m, data->host_id, data->start_address,
			data->count, MMC_TYPE_SD, 1);
		pr_err("sd thread %d end\n", current->pid);
	} else if (0 == data->host_id) {
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
	seq_puts(m, "\n   CLOCK control:\n");
	seq_printf(m, "   *set clk src:       echo %x 0 [host_id] [clk_src] > msdc_debug\n",
		SD_TOOL_CLK_SRC_SELECT);
	seq_printf(m, "   *get clk src:       echo %x 1 [host_id] > msdc_debug\n",
		SD_TOOL_CLK_SRC_SELECT);
	seq_puts(m, "      [clk_src]       msdc0: 0:26M, 1:800M, 2:400M, 3:200M, 4:182M, 5:136M, 6:156M, 7:48M, 8:91M\n");
	seq_puts(m, "	  [clk_src]  msdc1/2/3: 0:26M, 1:208M, 2:200M, 3:182M, 4:182M, 5:136M, 6:156M, 7:48M, 8:91M\n");
	seq_puts(m, "\n   REGISTER control:\n");
	seq_printf(m, "        write register:    echo %x 0 [host_id] [register_offset] [value] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_printf(m, "        read register:     echo %x 1 [host_id] [register_offset] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_printf(m, "        write mask:        echo %x 2 [host_id] [register_offset] [start_bit] [len] [value] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_printf(m, "        read mask:         echo %x 3 [host_id] [register_offset] [start_bit] [len] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_printf(m, "     dump all:          echo %x 4 [host_id] > msdc_debug\n",
		SD_TOOL_REG_ACCESS);
	seq_puts(m, "\n   DRVING control:\n");
	seq_printf(m, "        set driving:       echo %x 1 [host_id] [clk_drv] [cmd_drv] [dat_drv] [rst_drv] [ds_drv] [voltage] > msdc_debug\n",
		SD_TOOL_SET_DRIVING);
	seq_puts(m, "            [SD voltage]        1: 18v, 0: 33v\n");
	seq_printf(m, "        get driving:       echo %x 0 [host_id]  > msdc_debug\n",
		SD_TOOL_SET_DRIVING);
	seq_puts(m, "\n   DESENSE control:\n");
	seq_printf(m, "          write register:    echo %x 0 [value] > msdc_debug\n",
		SD_TOOL_DESENSE);
	seq_printf(m, "          read register:     echo %x 1 > msdc_debug\n",
		SD_TOOL_DESENSE);
	seq_printf(m, "          write mask:        echo %x 2 [start_bit] [len] [value] > msdc_debug\n",
		SD_TOOL_DESENSE);
	seq_printf(m, "          read mask:         echo %x 3 [start_bit] [len] > msdc_debug\n",
		SD_TOOL_DESENSE);
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
		    "                               8: MMC_TIMING_MMC_DDR52	9: MMC_TIMING_MMC_HS200	A: MMC_TIMING_MMC_HS400\n"
		    "		 [cmdq]             0: disable cmdq feature\n"
		    "                               1: enable cmdq feature\n");
	seq_printf(m, "\n   DMA viloation:         echo %x [host_id] [ops]> msdc_debug\n",
		SD_TOOL_DMA_STATUS);
	seq_puts(m, "          [ops]              0:get latest dma address, 1:start violation test\n");
	seq_printf(m, "\n   SET Slew Rate:         echo %x [host_id] [clk] [cmd] [dat]> msdc_debug\n",
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
	seq_printf(m, "\n   eMMC ETT Tune:         echo %x [type] [start_voltage], [end_voltage] > /proc/msdc_debug\n",
		MMC_ETT_TUNE);
	seq_puts(m, "            [type]             0:tune cmd  1:tune read  2:tune write  3:tune HS400\n");
	seq_puts(m, "            [start_voltage]    ?mV\n");
	seq_puts(m, "            [end_voltage]      ?mV, we try ETT from higher voltage to lower voltage\n");
	seq_printf(m, "\n   CRC Stress Test:       echo %x [action_id]> /proc/msdc_debug\n",
		MMC_CRC_STRESS);
	seq_puts(m, "            [action_id]        0:disable 1:enable\n");
	seq_printf(m, "\n   Enable AXI Modules:    echo %x [action_id][module_id]> /proc/msdc_debug\n",
		ENABLE_AXI_MODULE);
	seq_puts(m, "            [action_id]        0:disable 1:enable\n");
	seq_printf(m, "\n   Dump ext_csd register:       echo %x [host_id]> /proc/msdc_debug\n",
			MMC_DUMP_EXT_CSD);
	seq_printf(m, "\n   Dump csd register:       echo %x [host_id]> /proc/msdc_debug\n",
			MMC_DUMP_CSD);
	seq_puts(m, "\n   NOTE: All input data is Hex number!\n");

	seq_puts(m, "\n=============================================\n\n");

	return 0;
}

void msdc_hw_parameter_debug(struct msdc_hw *hw, struct seq_file *m, void *v)
{
	seq_printf(m, "hw->clk_src = %x\n", hw->clk_src);
	seq_printf(m, "hw->cmd_edge = %x\n", hw->cmd_edge);
	seq_printf(m, "hw->rdata_edge = %x\n", hw->rdata_edge);
	seq_printf(m, "hw->wdata_edge = %x\n", hw->wdata_edge);
	seq_printf(m, "hw->clk_drv = %x\n", hw->clk_drv);
	seq_printf(m, "hw->cmd_drv = %x\n", hw->cmd_drv);
	seq_printf(m, "hw->dat_drv = %x\n", hw->dat_drv);
	seq_printf(m, "hw->rst_drv = %x\n", hw->rst_drv);
	seq_printf(m, "hw->ds_drv = %x\n", hw->ds_drv);
	seq_printf(m, "hw->data_pins = %x\n", (unsigned int)hw->data_pins);
	seq_printf(m, "hw->data_offset = %x\n", (unsigned int)hw->data_offset);
	seq_printf(m, "hw->flags = %x\n", (unsigned int)hw->flags);
	seq_printf(m, "hw->dat0rddly = %x\n", hw->dat0rddly);
	seq_printf(m, "hw->dat1rddly = %x\n", hw->dat1rddly);
	seq_printf(m, "hw->dat2rddly = %x\n", hw->dat2rddly);
	seq_printf(m, "hw->dat3rddly = %x\n", hw->dat3rddly);
	seq_printf(m, "hw->dat4rddly = %x\n", hw->dat4rddly);
	seq_printf(m, "hw->dat5rddly = %x\n", hw->dat5rddly);
	seq_printf(m, "hw->dat6rddly = %x\n", hw->dat6rddly);
	seq_printf(m, "hw->dat7rddly = %x\n", hw->dat7rddly);
	seq_printf(m, "hw->datwrddly = %x\n", hw->datwrddly);
	seq_printf(m, "hw->cmdrrddly = %x\n", hw->cmdrrddly);
	seq_printf(m, "hw->cmdrddly = %x\n", hw->cmdrddly);
	seq_printf(m, "hw->host_function = %x\n", (u32)hw->host_function);
	seq_printf(m, "hw->boot = %x\n", hw->boot);
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
	uint type, address;

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
	struct bd_t  *bd;

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
		pr_err("gpd addr:0x%lx\n", (ulong)(host->dma.gpd_addr));
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
		pr_err("bd addr:0x%lx\n", (ulong)(host->dma.bd_addr));
		for (i = 0; i < host->dma.sglen; i++) {
			pr_err("the %d BD\n", i);
			pr_err("        eol:0x%x, rsv0:0x%x, chksum:0x%x, rsv1:0x%x,blkpad:0x%x,dwpad:0x%x,rsv2:0x%x\n",
				bd->eol, bd->rsv0, bd->chksum, bd->rsv1,
				bd->blkpad, bd->dwpad, bd->rsv2);
			pr_err("        nexth4:0x%x, ptrh4:0x%x, next:0x%x, ptr:0x%x, buflen:0x%x, rsv3:0x%x\n",
				(unsigned int)bd->nexth4,
				(unsigned int)bd->ptrh4, (unsigned int)bd->next,
				(unsigned int)bd->ptr, bd->buflen, bd->rsv3);
		}
	}
}
static void msdc_dump_csd(struct seq_file *m, struct msdc_host *host)
{
	struct mmc_csd *csd = &host->mmc->card->csd;
	u32 *resp = host->mmc->card->raw_csd;
	int i;
	unsigned int csd_struct;
	static const char const *sd_csd_ver[] = {"v1.0", "v2.0"};
	static const char const *mmc_csd_ver[] = {"v1.0", "v1.1", "v1.2", "Ver. in EXT_CSD"};
	static const char const *mmc_cmd_cls[] = {"basic", "stream read", "block read",
		"stream write", "block write", "erase", "write prot", "lock card",
		"app-spec", "I/O", "rsv.", "rsv."};
	static const char const *sd_cmd_cls[] = {"basic", "rsv.", "block read",
		"rsv.", "block write", "erase", "write prot", "lock card",
		"app-spec", "I/O", "switch", "rsv."};

	if (mmc_card_sd(host->mmc->card)) {
		csd_struct = UNSTUFF_BITS(resp, 126, 2);
		seq_printf(m, "[CSD] CSD %s\n", sd_csd_ver[csd_struct]);
		seq_printf(m, "[CSD] TACC_NS: %d ns, TACC_CLKS: %d clks\n", csd->tacc_ns, csd->tacc_clks);
		if (csd_struct == 1) {
			seq_puts(m, "[CSD] Read/Write Blk Len = 512bytes\n");
		} else {
			seq_printf(m, "[CSD] Read Blk Len = %d, Write Blk Len = %d\n",
					1 << csd->read_blkbits, 1 << csd->write_blkbits);
		}
		seq_puts(m, "[CSD] CMD Class:");
		for (i = 0; i < 12; i++) {
			if ((csd->cmdclass >> i) & 0x1)
				seq_printf(m, "'%s' ", sd_cmd_cls[i]);
		}
		seq_puts(m, "\n");
		seq_printf(m, "[CSD] MAX frequence: %dHZ\n", csd->max_dtr);
	} else {
		csd_struct = UNSTUFF_BITS(resp, 126, 2);
		seq_printf(m, "[CSD] CSD %s\n", mmc_csd_ver[csd_struct]);
		seq_printf(m, "[CSD] MMCA Spec v%d\n", csd->mmca_vsn);
		seq_printf(m, "[CSD] TACC_NS: %d ns, TACC_CLKS: %d clks\n", csd->tacc_ns, csd->tacc_clks);
		seq_printf(m, "[CSD] Read Blk Len = %d, Write Blk Len = %d\n",
				1 << csd->read_blkbits, 1 << csd->write_blkbits);
		seq_puts(m, "[CSD] CMD Class:");
		for (i = 0; i < 12; i++) {
			if ((csd->cmdclass >> i) & 0x1)
				seq_printf(m, "'%s' ", mmc_cmd_cls[i]);
		}
		seq_puts(m, "\n");
		seq_printf(m, "[CSD] MAX frequence: %dHZ\n", csd->max_dtr);
	}
}

void dbg_msdc_dump_clock_sts(struct seq_file *m, struct msdc_host *host)
{

	if (topckgen_reg_base) {
		/* CLK_CFG_3 control msdc clock source PLL */
		seq_printf(m, " CLK_CFG_3 register address is 0x%p\n\n",
			topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET);
		seq_puts(m, " bit[9~8]=01b,     bit[15]=0b\n");
		seq_puts(m, " bit[19~16]=0001b, bit[23]=0b\n");
		seq_puts(m, " bit[26~24]=0010b, bit[31]=0b\n");
		seq_printf(m, " Read value is       0x%x\n",
			MSDC_READ32(topckgen_reg_base + MSDC_CLK_CFG_3_OFFSET));
	}
	if (apmixed_reg_base) {
		/* bit0 is enables PLL, 0: disable 1: enable */
		seq_printf(m, " MSDCPLL_CON0_OFFSET register address is 0x%p\n\n",
			apmixed_reg_base + MSDCPLL_CON0_OFFSET);
		seq_puts(m, " bit[0]=1b\n");
		seq_printf(m, " Read value is       0x%x\n",
			MSDC_READ32(apmixed_reg_base + MSDCPLL_CON0_OFFSET));

		seq_printf(m, " MSDCPLL_CON1_OFFSET register address is 0x%p\n\n",
			apmixed_reg_base + MSDCPLL_CON1_OFFSET);
		seq_printf(m, " Read value is       0x%x\n",
			MSDC_READ32(apmixed_reg_base + MSDCPLL_CON1_OFFSET));

		seq_printf(m, " MSDCPLL_CON2_OFFSET register address is 0x%p\n\n",
			apmixed_reg_base + MSDCPLL_CON2_OFFSET);
		seq_printf(m, " Read value is       0x%x\n",
			MSDC_READ32(apmixed_reg_base + MSDCPLL_CON2_OFFSET));

		seq_printf(m, " MSDCPLL_PWR_CON0 register address is 0x%p\n\n",
			apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET);
		seq_puts(m, " bit[0]=1b\n");
		seq_printf(m, " Read value is       0x%x\n",
			MSDC_READ32(apmixed_reg_base + MSDCPLL_PWR_CON0_OFFSET));
	}
}
void msdc_dump_ext_csd(struct seq_file *m, struct msdc_host *host)
{
	u8 ext_csd[512];
	u32 tmp;
	static const char const *rev[] = {
		"4.0", "4.1", "4.2", "4.3", "Obsolete", "4.41", "4.5", "5.0", "5.1"};

	mmc_claim_host(host->mmc);

	if (mmc_send_ext_csd(host->mmc->card, ext_csd)) {
		seq_puts(m, "mmc_send_ext_csd failed\n");
		mmc_release_host(host->mmc);
		return;
	}
	mmc_release_host(host->mmc);

	seq_puts(m, "===========================================================\n");
	seq_printf(m, "[EXT_CSD] EXT_CSD rev.              : v1.%d (MMCv%s)\n",
			ext_csd[EXT_CSD_REV], rev[ext_csd[EXT_CSD_REV]]);
	seq_printf(m, "[EXT_CSD] CSD struct rev.           : v1.%d\n",
			ext_csd[EXT_CSD_STRUCTURE]);
	/* seq_printf(m, "[EXT_CSD] Supported command sets    : %xh\n", ext_csd[EXT_CSD_S_CMD_SET]); */
	seq_printf(m, "[EXT_CSD] HPI features              : %xh\n",
			ext_csd[EXT_CSD_HPI_FEATURES]);
	seq_printf(m, "[EXT_CSD] Cache control             : %xh\n",
			ext_csd[EXT_CSD_CACHE_CTRL]);
	seq_printf(m, "[EXT_CSD] BG operations support     : %xh\n",
			ext_csd[EXT_CSD_BKOPS_SUPPORT]);
	seq_printf(m, "[EXT_CSD] BG operations status      : %xh\n",
			ext_csd[EXT_CSD_BKOPS_STATUS]);
	/*
	memcpy(&tmp, &ext_csd[EXT_CSD_CORRECT_PRG_SECTS_NUM], 4);
	seq_printf(m, "[EXT_CSD] Correct prg. sectors      : %xh\n", tmp);
	seq_printf(m, "[EXT_CSD] 1st init time after part. : %d ms\n", ext_csd[EXT_CSD_INI_TIMEOUT_AP] * 100);
	seq_printf(m, "[EXT_CSD] Min. write perf.(DDR,52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_DDR_W_8_52]);
	seq_printf(m, "[EXT_CSD] Min. read perf. (DDR,52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_DDR_R_8_52]);
	*/
	seq_printf(m, "[EXT_CSD] TRIM timeout: %d ms\n",
			ext_csd[EXT_CSD_TRIM_MULT] & 0xFF * 300);
	seq_printf(m, "[EXT_CSD] Secure feature support: %xh\n",
			ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT]);
	seq_printf(m, "[EXT_CSD] Secure erase timeout  : %d ms\n",
			300 * ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] *
			ext_csd[EXT_CSD_SEC_ERASE_MULT]);
	seq_printf(m, "[EXT_CSD] Secure trim timeout   : %d ms\n", 300 *
			ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] *
			ext_csd[EXT_CSD_SEC_TRIM_MULT]);
	/* seq_printf(m, "[EXT_CSD] Access size           : %d bytes\n", ext_csd[EXT_CSD_ACC_SIZE] * 512); */
	seq_printf(m, "[EXT_CSD] HC erase unit size    : %d kbytes\n",
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 512);
	seq_printf(m, "[EXT_CSD] HC erase timeout      : %d ms\n",
			ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] * 300);
	/* seq_printf(m, "[EXT_CSD] HC write prot grp size: %d kbytes\n", 512 *
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] *
			ext_csd[EXT_CSD_HC_WP_GPR_SIZE]); */
	seq_printf(m, "[EXT_CSD] HC erase grp def.     : %xh\n",
			ext_csd[EXT_CSD_ERASE_GROUP_DEF]);
	seq_printf(m, "[EXT_CSD] Reliable write sect count: %xh\n",
			ext_csd[EXT_CSD_REL_WR_SEC_C]);
	/* seq_printf(m, "[EXT_CSD] Sleep current (VCC) : %xh\n", ext_csd[EXT_CSD_S_C_VCC]);
	seq_printf(m, "[EXT_CSD] Sleep current (VCCQ): %xh\n", ext_csd[EXT_CSD_S_C_VCCQ]); */
	seq_printf(m, "[EXT_CSD] Sleep/awake timeout : %d ns\n",
			100 * (2 << ext_csd[EXT_CSD_S_A_TIMEOUT]));
	memcpy(&tmp, &ext_csd[EXT_CSD_SEC_CNT], 4);
	seq_printf(m, "[EXT_CSD] Sector count : %xh\n", tmp);
	/* seq_printf(m, "[EXT_CSD] Min. WR Perf.  (52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_W_8_52]);
	seq_printf(m, "[EXT_CSD] Min. Read Perf.(52MH,8b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_R_8_52]);
	seq_printf(m, "[EXT_CSD] Min. WR Perf.  (26MH,8b,52MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_W_8_26_4_25]);
	seq_printf(m, "[EXT_CSD] Min. Read Perf.(26MH,8b,52MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_R_8_26_4_25]);
	seq_printf(m, "[EXT_CSD] Min. WR Perf.  (26MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_W_4_26]);
	seq_printf(m, "[EXT_CSD] Min. Read Perf.(26MH,4b): %xh\n", ext_csd[EXT_CSD_MIN_PERF_R_4_26]); */
	seq_printf(m, "[EXT_CSD] Power class: %x\n",
			ext_csd[EXT_CSD_POWER_CLASS]);
	seq_printf(m, "[EXT_CSD] Power class(DDR,52MH,3.6V): %xh\n",
			ext_csd[EXT_CSD_PWR_CL_DDR_52_360]);
	seq_printf(m, "[EXT_CSD] Power class(DDR,52MH,1.9V): %xh\n",
			ext_csd[EXT_CSD_PWR_CL_DDR_52_195]);
	seq_printf(m, "[EXT_CSD] Power class(26MH,3.6V)    : %xh\n",
			ext_csd[EXT_CSD_PWR_CL_26_360]);
	seq_printf(m, "[EXT_CSD] Power class(52MH,3.6V)    : %xh\n",
			ext_csd[EXT_CSD_PWR_CL_52_360]);
	seq_printf(m, "[EXT_CSD] Power class(26MH,1.9V)    : %xh\n",
			ext_csd[EXT_CSD_PWR_CL_26_195]);
	seq_printf(m, "[EXT_CSD] Power class(52MH,1.9V)    : %xh\n",
			ext_csd[EXT_CSD_PWR_CL_52_195]);
	seq_printf(m, "[EXT_CSD] Part. switch timing    : %xh\n",
			ext_csd[EXT_CSD_PART_SWITCH_TIME]);
	seq_printf(m, "[EXT_CSD] Out-of-INTR busy timing: %xh\n",
			ext_csd[EXT_CSD_OUT_OF_INTERRUPT_TIME]);
	seq_printf(m, "[EXT_CSD] Card type       : %xh\n",
			ext_csd[EXT_CSD_CARD_TYPE]);
	/* seq_printf(m, "[EXT_CSD] Command set     : %xh\n", ext_csd[EXT_CSD_CMD_SET]);
	seq_printf(m, "[EXT_CSD] Command set rev.: %xh\n", ext_csd[EXT_CSD_CMD_SET_REV]); */
	seq_printf(m, "[EXT_CSD] HS timing       : %xh\n",
			ext_csd[EXT_CSD_HS_TIMING]);
	seq_printf(m, "[EXT_CSD] Bus width       : %xh\n",
			ext_csd[EXT_CSD_BUS_WIDTH]);
	seq_printf(m, "[EXT_CSD] Erase memory content : %xh\n",
			ext_csd[EXT_CSD_ERASED_MEM_CONT]);
	/*seq_printf(m, "[EXT_CSD] Partition config      : %xh\n",
			ext_csd[EXT_CSD_PART_CFG]); */
	/* seq_printf(m, "[EXT_CSD] Boot partition size   : %d kbytes\n",
			ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128);*/
	/* seq_printf(m, "[EXT_CSD] Boot information      : %xh\n", ext_csd[EXT_CSD_BOOT_INFO]);
	seq_printf(m, "[EXT_CSD] Boot config protection: %xh\n", ext_csd[EXT_CSD_BOOT_CONFIG_PROT]);
	seq_printf(m, "[EXT_CSD] Boot bus width        : %xh\n", ext_csd[EXT_CSD_BOOT_BUS_WIDTH]); */
	seq_printf(m, "[EXT_CSD] Boot area write prot  : %xh\n",
			ext_csd[EXT_CSD_BOOT_WP]);
	/*seq_printf(m, "[EXT_CSD] User area write prot  : %xh\n", ext_csd[EXT_CSD_USR_WP]);
	seq_printf(m, "[EXT_CSD] FW configuration      : %xh\n", ext_csd[EXT_CSD_FW_CONFIG]); */
	/*seq_printf(m, "[EXT_CSD] RPMB size : %d kbytes\n",
			ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128); */
	/* seq_printf(m, "[EXT_CSD] Write rel. setting  : %xh\n", ext_csd[EXT_CSD_WR_REL_SET]); */
	seq_printf(m, "[EXT_CSD] Write rel. parameter: %xh\n",
			ext_csd[EXT_CSD_WR_REL_PARAM]);
	seq_printf(m, "[EXT_CSD] Start background ops : %xh\n",
			ext_csd[EXT_CSD_BKOPS_START]);
	seq_printf(m, "[EXT_CSD] Enable background ops: %xh\n",
			ext_csd[EXT_CSD_BKOPS_EN]);
	seq_printf(m, "[EXT_CSD] H/W reset function   : %xh\n",
			ext_csd[EXT_CSD_RST_N_FUNCTION]);
	seq_printf(m, "[EXT_CSD] HPI management       : %xh\n",
			ext_csd[EXT_CSD_HPI_MGMT]);
	/* memcpy(&tmp, &ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT], 4);
	seq_printf(m, "[EXT_CSD] Max. enhanced area size : %xh (%d kbytes)\n",
			tmp & 0x00FFFFFF, (tmp & 0x00FFFFFF) * 512 *
			ext_csd[EXT_CSD_HC_WP_GPR_SIZE] * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]); */
	seq_printf(m, "[EXT_CSD] Part. support  : %xh\n",
			ext_csd[EXT_CSD_PARTITION_SUPPORT]);
	seq_printf(m, "[EXT_CSD] Part. attribute: %xh\n",
			ext_csd[EXT_CSD_PARTITION_ATTRIBUTE]);
	seq_printf(m, "[EXT_CSD] Part. setting  : %xh\n",
			ext_csd[EXT_CSD_PARTITION_SETTING_COMPLETED]);
	/* seq_printf(m, "[EXT_CSD] General purpose 1 size : %xh (%d kbytes)\n",
			(ext_csd[EXT_CSD_GP1_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] << 16),
			(ext_csd[EXT_CSD_GP1_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_GP1_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_GP1_SIZE_MULT + 2] << 16) * 512 *
			ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	seq_printf(m, "[EXT_CSD] General purpose 2 size : %xh (%d kbytes)\n",
			(ext_csd[EXT_CSD_GP2_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] << 16),
			(ext_csd[EXT_CSD_GP2_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_GP2_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_GP2_SIZE_MULT + 2] << 16) * 512 *
			ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	seq_printf(m, "[EXT_CSD] General purpose 3 size : %xh (%d kbytes)\n",
			(ext_csd[EXT_CSD_GP3_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] << 16),
			(ext_csd[EXT_CSD_GP3_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_GP3_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_GP3_SIZE_MULT + 2] << 16) * 512 *
			ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	seq_printf(m, "[EXT_CSD] General purpose 4 size : %xh (%d kbytes)\n",
			(ext_csd[EXT_CSD_GP4_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] << 16),
			(ext_csd[EXT_CSD_GP4_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_GP4_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_GP4_SIZE_MULT + 2] << 16) * 512 *
			ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);*/
	/* seq_printf(m, "[EXT_CSD] Enh. user area size : %xh (%d kbytes)\n",
			(ext_csd[EXT_CSD_ENH_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_ENH_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_ENH_SIZE_MULT + 2] << 16),
			(ext_csd[EXT_CSD_ENH_SIZE_MULT + 0] |
			 ext_csd[EXT_CSD_ENH_SIZE_MULT + 1] << 8 |
			 ext_csd[EXT_CSD_ENH_SIZE_MULT + 2] << 16) * 512 *
			ext_csd[EXT_CSD_HC_WP_GPR_SIZE] *
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]);
	seq_printf(m, "[EXT_CSD] Enh. user area start: %xh\n",
			(ext_csd[EXT_CSD_ENH_START_ADDR + 0] |
			 ext_csd[EXT_CSD_ENH_START_ADDR + 1] << 8 |
			 ext_csd[EXT_CSD_ENH_START_ADDR + 2] << 16 |
			 ext_csd[EXT_CSD_ENH_START_ADDR + 3]) << 24);
	seq_printf(m, "[EXT_CSD] Bad block mgmt mode: %xh\n", ext_csd[EXT_CSD_BADBLK_MGMT]); */
	seq_puts(m, "===========================================================\n");
}
static void msdc_check_emmc_cache_status(struct seq_file *m,
		struct msdc_host *host)
{
	BUG_ON(!host);
	BUG_ON(!host->mmc);
	BUG_ON(!host->mmc->card);

	if (!mmc_card_mmc(host->mmc->card))
		seq_printf(m, "msdc%d: is not a eMMC card\n", host->id);

	if (0 == host->mmc->card->ext_csd.cache_size)
		seq_printf(m, "msdc%d:card don't support cache feature\n",
				host->id);
	seq_printf(m, "msdc%d: Current eMMC Cache status: %s, Cache size:%dKB\n",
		host->id,
		host->mmc->card->ext_csd.cache_ctrl ? "Enable" : "Disable",
		host->mmc->card->ext_csd.cache_size/8);
}


static void msdc_enable_emmc_cache(struct seq_file *m,
		struct msdc_host *host, int enable)
{
	int err;
	u8 c_ctrl;

	msdc_check_emmc_cache_status(m, host);

	mmc_get_card(host->mmc->card);

	c_ctrl = host->mmc->card->ext_csd.cache_ctrl;

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
	mmc_put_card(host->mmc->card);
}
#ifdef MTK_MSDC_ERROR_TUNE_DEBUG
#define MTK_MSDC_ERROR_NONE	(0)
#define MTK_MSDC_ERROR_CMD_TMO	(0x1)
#define MTK_MSDC_ERROR_CMD_CRC	(0x1 << 1)
#define MTK_MSDC_ERROR_DAT_TMO	(0x1 << 2)
#define MTK_MSDC_ERROR_DAT_CRC	(0x1 << 3)
#define MTK_MSDC_ERROR_ACMD_TMO	(0x1 << 4)
#define MTK_MSDC_ERROR_ACMD_CRC	(0x1 << 5)
unsigned int g_err_tune_dbg_count = 0;
unsigned int g_err_tune_dbg_host = 0;
unsigned int g_err_tune_dbg_cmd = 0;
unsigned int g_err_tune_dbg_arg = 0;
unsigned int g_err_tune_dbg_error = MTK_MSDC_ERROR_NONE;

static void msdc_error_tune_debug_print(struct seq_file *m, int p1, int p2, int p3, int p4, int p5)
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

	if (g_err_tune_dbg_cmd == cmd->opcode) {
		if ((cmd->opcode != MMC_SWITCH)
		 || ((cmd->opcode == MMC_SWITCH) &&
		     (g_err_tune_dbg_arg == ((cmd->arg >> 16) & 0xff)))) {
			if (g_err_tune_dbg_error & MTK_MSDC_ERROR_CMD_TMO) {
				*intsts = MSDC_INT_CMDTMO;
				g_err_tune_dbg_count--;
			} else if (g_err_tune_dbg_error
				 & MTK_MSDC_ERROR_CMD_CRC) {
				*intsts = MSDC_INT_RSPCRCERR;
				g_err_tune_dbg_count--;
			}
			pr_err("[%s]: got the error cmd:%d, arg=%d, dbg error=%d, cmd->error=%d, count=%d\n",
				__func__, g_err_tune_dbg_cmd,
				g_err_tune_dbg_arg, g_err_tune_dbg_error,
				cmd->error, g_err_tune_dbg_count);
		}
	}

#ifdef MTK_MSDC_USE_CMD23
	if ((g_err_tune_dbg_cmd == MMC_SET_BLOCK_COUNT) &&
	    sbc &&
	    (host->autocmd & MSDC_AUTOCMD23)) {
		if (g_err_tune_dbg_error & MTK_MSDC_ERROR_ACMD_TMO) {
			*intsts = MSDC_INT_ACMDTMO;
		} else if (g_err_tune_dbg_error
			& MTK_MSDC_ERROR_ACMD_CRC) {
			*intsts = MSDC_INT_ACMDCRCERR;
		} else {
			return;
		}
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

void msdc_error_tune_debug3(struct msdc_host *host,
		struct mmc_command *cmd, u32 *intsts)
{
	if (!g_err_tune_dbg_error ||
	    (g_err_tune_dbg_count <= 0) ||
	    (g_err_tune_dbg_host != host->id))
		return;

	if (g_err_tune_dbg_cmd != cmd->opcode)
		return;

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
}
#endif /*ifdef MTK_MSDC_ERROR_TUNE_DEBUG*/
int g_count = 0;
/* ========== driver proc interface =========== */
static int msdc_debug_proc_show(struct seq_file *m, void *v)
{
	int cmd = -1;
	int sscanf_num;
	int p1, p2, p3, p4, p5, p6, p7, p8;
	int id, zone;
	int mode, size;
	int thread_num, compare_count, multi_address;
	void __iomem *base = NULL;
	ulong data_for_wr;
	unsigned int offset = 0;
	unsigned int reg_value;
	int spd_mode = MMC_TIMING_LEGACY;
	struct msdc_host *host = NULL;
	char enable_str[512] = "disable";
	int cmdq;
#ifdef MSDC_DMA_ADDR_DEBUG
	struct dma_addr *dma_address, *p_dma_address;
#endif
	int dma_status;

	p1 = p2 = p3 = p4 = p5 = p6 = p7 = p8 = -1;

	cmd_buf[g_count] = '\0';
	seq_printf(m, "Debug Command:  %s\n", cmd_buf);

	sscanf_num = sscanf(cmd_buf, "%x %x %x %x %x %x %x %x %x", &cmd,
		&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);

	/* clear buffer */
	g_count = 0;
	cmd_buf[g_count] = '\0';

	switch (cmd) {
	case SD_TOOL_ZONE:
		id = p1;
		host = mtk_msdc_host[id];
		zone = p2;	/* zone &= 0x3ff; */
		seq_printf(m, "[SD_Debug] msdc host<%d> debug zone<0x%.8x>\n",
				id, zone);
		if (host) {
			seq_printf(m, "[SD_Debug] msdc host<%d> caps<0x%.8x>\n",
					id, host->mmc->caps);
			seq_printf(m, "[SD_Debug] msdc host<%d> caps2<0x%.8x>\n",
					id, host->mmc->caps2);
		}

		if (id > HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (id == HOST_MAX_NUM) {
			sd_debug_zone[0] = sd_debug_zone[1] = zone;
			sd_debug_zone[2] = zone;
			sd_debug_zone[3] = zone;
		} else {
			sd_debug_zone[id] = zone;
		}
		break;
	case SD_TOOL_DMA_SIZE:
		id = p2;
		mode = p3;
		size = p4;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (p1 == 0) {
			drv_mode[id] = mode;
			dma_size[id] = size;
			seq_printf(m, "[SD_Debug] msdc host[%d] mode<%d> size<%d>\n",
				 id, drv_mode[id], dma_size[id]);

		} else {
			seq_printf(m, "[SD_Debug] msdc host[%d] mode<%d> size<%d>\n",
				 id, drv_mode[id], dma_size[id]);
		}
		break;
	case SD_TOOL_CLK_SRC_SELECT:
		id = p2;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (p1 == 0) {
			if (p3 >= 0 && p3 < CLK_SRC_MAX_NUM) {
				msdc_clock_src[id] = p3;
				seq_printf(m, "[SD_Debug] msdc%d's clk source changed to %d\n",
					id, msdc_clock_src[id]);
				seq_puts(m, "[SD_Debug] to enable the above settings, suspend and resume the phone again\n");
			} else {
				seq_printf(m, "[SD_Debug] invalid clock src id:%d, check /proc/msdc_help\n",
					p3);
			}
		} else if (p1 == 1) {
			seq_printf(m, "[SD_Debug] msdc%d's pll source is %d\n",
				id, msdc_clock_src[id]);
		}
		break;
	case SD_TOOL_REG_ACCESS:
		id = p2;
		offset = (unsigned int)p3;

		if (id >= HOST_MAX_NUM || id < 0 || mtk_msdc_host[id] == NULL)
			goto invalid_host_id;

		host = mtk_msdc_host[id];
		base = host->base;
		if ((offset == 0x18 || offset == 0x1C) && p1 != 4)
			seq_puts(m, "[SD_Debug] Err: Accessing TXDATA and RXDATA is forbidden\n");

		if (msdc_clk_enable(host)) {
			seq_puts(m, "[SD_Debug] msdc_clk_enable failed\n");
			return 1;
		}
		if (p1 == 0) {
			if (offset > 0x228) {
				seq_puts(m, "invalid register offset\n");
				break;
			}
			reg_value = p4;
			seq_printf(m, "[SD_Debug][MSDC Reg]Original:0x%p+0x%x (0x%x)\n",
				base, offset, MSDC_READ32(base + offset));
			MSDC_WRITE32(base + offset, reg_value);
			seq_printf(m, "[SD_Debug][MSDC Reg]Modified:0x%p+0x%x (0x%x)\n",
				base, offset, MSDC_READ32(base + offset));
		} else if (p1 == 1) {
			seq_printf(m, "[SD_Debug][MSDC Reg]Reg:0x%p+0x%x (0x%x)\n",
					base, offset,
					MSDC_READ32(base + offset));
		} else if (p1 == 2) {
			msdc_set_field(m, base + offset, p4, p5, p6);
		} else if (p1 == 3) {
			msdc_get_field(m, base + offset, p4, p5, p6);
		} else if (p1 == 4) {
			msdc_dump_register_debug(m, host->id, base);
		} else if (p1 == 5) {
			msdc_dump_register_debug(m, host->id, base);
		}

		msdc_clk_disable(host);

		break;
	case SD_TOOL_SET_DRIVING:
		id = p2;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];
		if (p1 == 1) {
			if ((unsigned char)p3 > 7 || (unsigned char)p4 > 7 ||
					(unsigned char)p5 > 7 || (unsigned char)p6 > 7 ||
					(unsigned char)p7 > 7) {
				seq_puts(m, "[SD_Debug]Some drving value was invalid(invalid:0~7)\n");
				break;
			}
			if (id == 0) {
				host->hw->clk_drv = (unsigned char)p3;
				host->hw->cmd_drv = (unsigned char)p4;
				host->hw->dat_drv = (unsigned char)p5;
				host->hw->rst_drv = (unsigned char)p6;
				host->hw->ds_drv = (unsigned char)p7;
				msdc_set_driving(host, host->hw, 0);
				seq_printf(m, "[SD_Debug] eMMC set driving: clk_drv=%d, cmd_drv=%d, dat_drv=%d, rst_drv=%d, ds_drv=%d\n",
				host->hw->clk_drv, host->hw->cmd_drv,
				host->hw->dat_drv, host->hw->rst_drv,
				host->hw->ds_drv);

			} else if (id == 1 && p8 == 1) {
				/* SD card 1.8v Driving is different */
				host->hw->clk_drv_sd_18 = (unsigned char)p3;
				host->hw->cmd_drv_sd_18 = (unsigned char)p4;
				host->hw->dat_drv_sd_18 = (unsigned char)p5;
				msdc_set_driving(host, host->hw, 1);
				seq_printf(m, "[SD_Debug] SD30 set driving: clk_drv=%d, cmd_drv=%d, dat_drv=%d, rst_drv=%d, ds_drv=%d\n",
				host->hw->clk_drv, host->hw->cmd_drv,
				host->hw->dat_drv, host->hw->rst_drv,
				host->hw->ds_drv);

			} else if (id == 1 && p8 == 0) {
				host->hw->clk_drv = (unsigned char)p3;
				host->hw->cmd_drv = (unsigned char)p4;
				host->hw->dat_drv = (unsigned char)p5;
				msdc_set_driving(host, host->hw, 0);
				seq_printf(m, "[SD_Debug] SD20 set driving: clk_drv=%d, cmd_drv=%d, dat_drv=%d, rst_drv=%d, ds_drv=%d\n",
				host->hw->clk_drv, host->hw->cmd_drv,
				host->hw->dat_drv, host->hw->rst_drv,
				host->hw->ds_drv);

			}
		} else {
			seq_printf(m, "[SD_Debug] Get driving: clk_drv=%d, cmd_drv=%d, dat_drv=%d, rst_drv=%d, ds_drv=%d\n",
				host->hw->clk_drv, host->hw->cmd_drv,
				host->hw->dat_drv, host->hw->rst_drv,
				host->hw->ds_drv);
			if (id == 1)
				seq_printf(m, "[SD_Debug]              clk_drv_sd_18=%d, cmd_drv_sd_18=%d, dat_drv_sd_18=%d\n",
					host->hw->clk_drv_sd_18,
					host->hw->cmd_drv_sd_18,
					host->hw->dat_drv_sd_18);
		}
		break;
	case SD_TOOL_ENABLE_SLEW_RATE:
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];
		if ((unsigned char)p2 > 1 || (unsigned char)p3 > 1
				|| (unsigned char)p4 > 1) {
			seq_puts(m, "[SD_Debug]Some sr value was invalid(correct:0(disable),1(enable))\n");
		} else {
			msdc_set_sr(host, p2, p3, p4);
			seq_printf(m, "[SD_Debug]msdc%d, clk_sr=%d, cmd_sr=%d, dat_sr=%d\n",
				id, p2, p3, p4);
		}
		break;
	case SD_TOOL_SET_RDTDSEL:
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];
		if ((p2 < 0) || (p2 > 2)) {
			seq_puts(m, "[SD_Debug]invalid option ( set rd:0, set td:1, get td/rd: 2)\n");
		} else {
			if (p2 == 0) {
				msdc_set_rdsel_dbg(host, p3);
				seq_printf(m, "[SD_Debug]msdc%d, set rd=%d\n",
					id, p3);
			} else if (p2 == 1) { /* set td:1 */
				msdc_set_tdsel_dbg(host, p3);
				seq_printf(m, "[SD_Debug]msdc%d, set td=%d\n",
					id, p3);
			} else if (p2 == 2) { /* get td/rd:2 */
				msdc_get_rdsel_dbg(host, &p3); /* get rd */
				msdc_get_tdsel_dbg(host, &p4); /* get td */
				seq_printf(m, "[SD_Debug]msdc%d, rd : 0x%x, td : 0x%x\n",
					id, p3, p4);
			}
		}
		break;
	case SD_TOOL_ENABLE_SMT:
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];
		msdc_set_smt(host, p2);
		seq_printf(m, "[SD_Debug]smt=%d\n", p2);
		break;
	case RW_BIT_BY_BIT_COMPARE:
		id = p1;
		compare_count = p2;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (compare_count < 0) {
			seq_printf(m, "[SD_Debug]: bad compare count: %d\n",
				compare_count);
			break;
		}

		if (id == 0) { /* for msdc0 */
			multi_rw_compare(m, 0, COMPARE_ADDRESS_MMC,
				compare_count, MMC_TYPE_MMC, 0);
			/*  test the address 0 of eMMC */
		} else {
			multi_rw_compare(m, id, COMPARE_ADDRESS_SD,
				compare_count, MMC_TYPE_SD, 0);
		}
		break;
	case MSDC_READ_WRITE:
		id = p1;
		mode = p2;	/* 0:stop, 1:read, 2:write */
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (mode > 2 || mode < 0) {
			seq_printf(m, "[SD_Debug]: bad mode: %d\n", mode);
			break;
		}
		if (mode == read_write_state) {
			seq_printf(m, "[SD_Debug]: same operation mode=%d.\n",
				read_write_state);
			break;
		}
		if (mode == 1 && read_write_state == 2) {
			seq_puts(m, "[SD_Debug]: cannot read in write state, please stop first\n");
			break;
		}
		if (mode == 2 && read_write_state == 1) {
			seq_puts(m, "[SD_Debug]: cannot write in read state, please stop first\n");
			break;
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
		break;
	case SD_TOOL_MSDC_HOST_MODE:
		id = p2;
		host = mtk_msdc_host[id];
		spd_mode = p3;
		cmdq = p4;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (p1 == 0)
			msdc_get_host_mode_speed(m, host, spd_mode);
		else
			msdc_set_host_mode_speed(m, host, spd_mode, cmdq);

		break;
	case SD_TOOL_DMA_STATUS:
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		if (p2 == 0) {
			static const char const *str[] = {
				"No data transaction or the device is not present until now\n",
				"DMA mode is disabled Now\n",
				"Write from SD to DRAM in DMA mode\n",
				"Write from DRAM to SD in DMA mode\n"
			};
			dma_status = msdc_get_dma_status(id);
			seq_printf(m, ">>>> msdc%d: dma_status=%d, ", id, dma_status);
			seq_printf(m, "%s", str[dma_status+1]);
			if (dma_status == -1)
				break;
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
			seq_puts(m, "please enable MSDC_DMA_ADDR_DEBUG at mt_sd.h if you want dump dma address\n");
#endif
		} else if (p2 == 1) {
			seq_printf(m, ">>>> msdc%d: start dma violation test\n", id);
			g_dma_debug[id] = 1;
			multi_rw_compare(m, id, COMPARE_ADDRESS_SD, 3,
				MMC_TYPE_SD, 0);
		}
		break;
	case MMC_EDC_EMMC_CACHE:
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
			msdc_check_emmc_cache_status(m, host);
			break;
		default:
			seq_puts(m, "ERROR:3rd parameter is wrong, please see the msdc_help\n");
			break;
		}
		break;
	case MMC_DUMP_GPD:
		seq_puts(m, "==== MSDC DUMP GPD/BD ====\n");
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		else
			msdc_dump_gpd_bd(id);
		break;
	case SD_TOOL_SDIO_PROFILE:
		if (p1 == 1) {	/* enable profile */
			if (gpt_enable == 0) {
				msdc_init_gpt();
				gpt_enable = 1;
			}
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
		break;
	case SMP_TEST_ON_ONE_HOST:
		id = p1;
		thread_num = p2;
		compare_count = p3;
		multi_address = p4;
		smp_test_on_hosts(m, thread_num, id, compare_count, multi_address);
		break;
	case SMP_TEST_ON_ALL_HOST:
		thread_num = p1;
		compare_count = p2;
		multi_address = p3;
		smp_test_on_hosts(m, thread_num, HOST_MAX_NUM, compare_count,
			multi_address);
		break;
	case MMC_REGISTER_READ:
		seq_printf(m, "p1 = 0x%x\n", p1);
		seq_printf(m, "regiser: 0x%x = 0x%x\n", p1, MSDC_READ32((ulong) p1));
		break;
#ifdef MTK_IO_PERFORMANCE_DEBUG
	case MMC_PERF_DEBUG:
		/* 1 enable; 0 disable */
		g_mtk_mmc_perf_dbg = p1;
		g_mtk_mmc_dbg_range = p2;

		if (2 == g_mtk_mmc_dbg_range) {
			g_dbg_range_start = p3;
			g_dbg_range_end = p3 + p4;
			g_check_read_write = p5;
		}
		seq_printf(m, "g_mtk_mmc_perf_dbg = 0x%x, g_mtk_mmc_dbg_range = 0x%x, start = 0x%x, end = 0x%x\n",
			g_mtk_mmc_perf_dbg, g_mtk_mmc_dbg_range,
			g_dbg_range_start, g_dbg_range_end);
		break;
	case MMC_PERF_DEBUG_PRINT:
		int i, j, k, num = 0;

		if (p1 == 0) {
			g_mtk_mmc_clear = 0;
			break;
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
				seq_printf(m, "%d. %llu %s\n", j, g_mmcqd_buf[i][j],
					g_time_mark[j]);
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
		break;
#endif
#ifdef MTK_MMC_PERFORMANCE_TEST
	case MMC_PERF_TEST:
		/* 1 enable; 0 disable */
		g_mtk_mmc_perf_test = p1;
		break;
#endif

#ifdef MTK_MSDC_ERROR_TUNE_DEBUG
	case MMC_ERROR_TUNE:
		msdc_error_tune_debug_print(m, p1, p2, p3, p4, p5);
		break;
#endif
	case ENABLE_AXI_MODULE:
		if (p1)
			strncpy(enable_str, "enable", 7);
		else
			strncpy(enable_str, "disable", 8);
		seq_printf(m, "==== %s AXI MODULE ====\n", enable_str);
		if (p2 == 0 || p2 == 5) {
			seq_printf(m, "%s %s transaction on AXI bus\n",
				enable_str, "NFI");
			/* NFI_SW_RST */
			MSDC_SET_FIELD(pericfg_reg_base, (0x1 << 14),
				(p1 ? 0x0 : 0x1));
		}
		if (p2 == 1 || p2 == 5) {
			seq_printf(m, "%s %s transaction on AXI bus\n",
				enable_str, "SD");
			/* MSDC1_SW_RST */
			MSDC_SET_FIELD(pericfg_reg_base, (0x1 << 20),
				(p1 ? 0x0 : 0x1));
		}
		if (p2 == 2 || p2 == 5) {
			seq_printf(m, "%s %s transaction on AXI bus\n",
				enable_str, "USB");
			/* USB_SW_RST */
			MSDC_SET_FIELD(pericfg_reg_base, (0x1 << 28),
				(p1 ? 0x0 : 0x1));
		}
		if (p2 == 3 || p2 == 5) {
			seq_printf(m, "%s %s transaction on AXI bus\n",
				enable_str, "PERI");
			/* PERI_AXI */
			MSDC_SET_FIELD(pericfg_reg_base + 0x210, (0x3 << 8),
				(p1 ? 0x3 : 0x2));
		}
		if (p2 == 4 || p2 == 5) {
			seq_printf(m, "%s %s transaction on AXI bus\n",
				enable_str, "AUDIO");
			/* AUDIO_RST */
			MSDC_SET_FIELD(infracfg_ao_reg_base + 0x40, (0x1 << 5),
				(p1 ? 0x0 : 0x1));
		}
		seq_printf(m, "disable AXI modules, reg[0x10003000]=0x%x, reg[0x10003210]=0x%x, reg[0x10001040]=0x%x\n",
			MSDC_READ32(pericfg_reg_base),
			MSDC_READ32(pericfg_reg_base + 0x210),
			MSDC_READ32(infracfg_ao_reg_base + 0x40));
		break;
	case MMC_CRC_STRESS:
		seq_puts(m, "==== CRC Stress Test ====\n");
		if (0 == p1) {
			g_reset_tune = 0;
		} else {
			g_reset_tune = 1;
			base = mtk_msdc_host[0]->base;
			MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
				MSDC_EMMC50_PAD_DS_TUNE_DLY1, 0x1c);
			MSDC_SET_FIELD(EMMC50_PAD_DS_TUNE,
				MSDC_EMMC50_PAD_DS_TUNE_DLY3, 0xe);
		}
		break;
	case MMC_DUMP_EXT_CSD:
		id = p1;
		if (id != 0)
			goto invalid_host_id;

		host = mtk_msdc_host[id];
		msdc_dump_ext_csd(m, host);
		break;
	case MMC_DUMP_CSD:
		id = p1;
		if (id > 1)
			goto invalid_host_id;

		host = mtk_msdc_host[id];
		msdc_dump_csd(m, host);
		seq_puts(m, "[SD_Debug]: Dump clock status\n");
		dbg_msdc_dump_clock_sts(m, host);
		break;
	case DO_AUTOK_OFFLINE_TUNE_TX:
		host = mtk_msdc_host[p1]; /*p1 = id */
		mmc_claim_host(host->mmc);
		#if 0
		do_autok_offline_tune_tx = 1;
		host->mmc->ops->execute_tuning(host->mmc,
			MMC_SEND_TUNING_BLOCK_HS200);
		do_autok_offline_tune_tx = 0;
		#else
		autok_offline_tuning_TX(host);
		#endif
		mmc_release_host(host->mmc);
		break;
	case MMC_CMDQ_STATUS:
		seq_puts(m, "==== eMMC CMDQ Feature ====\n");
		id = p1;
		if (id >= HOST_MAX_NUM || id < 0)
			goto invalid_host_id;
		host = mtk_msdc_host[id];
		msdc_cmdq_func(m, host, p2);
		break;
	default:
		/* default dump info for aee */
		seq_puts(m, "==== msdc debug info for aee ====\n");
		msdc_proc_dump(m, 0);
		break;
	}
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


/*Used by ETT for SDIO 2.0 device, maybe it can be removed*/
static int msdc_tune_flag_proc_read_show(struct seq_file *m, void *data)
{
	seq_printf(m, "0x%X\n", sdio_tune_flag);
	return 0;
}

/*Used by ETT for SDIO 2.0 device, maybe it can be removed*/
static int msdc_debug_proc_read_FT_show(struct seq_file *m, void *data)
{
#if defined(CONFIG_MTK_WCN_CMB_SDIO_SLOT)
	int msdc_id = 0;
	void __iomem *base;
	unsigned char  cmd_edge;
	unsigned char  data_edge;
	unsigned char  clk_drv1 = 0, clk_drv2 = 0;
	unsigned char  cmd_drv1 = 0, cmd_drv2 = 0;
	unsigned char  dat_drv1 = 0, dat_drv2 = 0;
	u32 cur_rxdly0;
	u8 u8_dat0, u8_dat1, u8_dat2, u8_dat3;
	u8 u8_wdat, u8_cmddat;
	u8 u8_DDLSEL;

	switch (CONFIG_MTK_WCN_CMB_SDIO_SLOT) {
	case 0:
		if (mtk_msdc_host[0]) {
			base = mtk_msdc_host[0]->base;
			msdc_id = 0;
		}
		break;
	case 1:
		if (mtk_msdc_host[1]) {
			base = mtk_msdc_host[1]->base;
			msdc_id = 1;
		}
		break;
	case 2:
		if (mtk_msdc_host[2]) {
			base = mtk_msdc_host[2]->base;
			msdc_id = 2;
		}
		break;
	case 3:
		if (mtk_msdc_host[3]) {
			base = mtk_msdc_host[3]->base;
			msdc_id = 3;
		}
		break;
	default:
		pr_err("Invalid Host number: %d\n", CONFIG_MTK_WCN_CMB_SDIO_SLOT);
		break;
	}
	if (msdc_clk_enable(host)) {
		seq_puts(m, "[SD_Debug] msdc_clk_enable failed\n");
		return 1;
	}
	MSDC_GET_FIELD((base+0x04), MSDC_IOCON_RSPL, cmd_edge);
	MSDC_GET_FIELD((base+0x04), MSDC_IOCON_R_D_SMPL, data_edge);
/*
	MSDC_GET_FIELD((base + 0xe0), MSDC_PAD_CTL0_CLKDRVN, clk_drv1);
	MSDC_GET_FIELD((base + 0xe0), MSDC_PAD_CTL0_CLKDRVP, clk_drv2);

	MSDC_GET_FIELD((base + 0xe4), MSDC_PAD_CTL1_CMDDRVN, cmd_drv1);
	MSDC_GET_FIELD((base + 0xe4), MSDC_PAD_CTL1_CMDDRVP, cmd_drv2);

	MSDC_GET_FIELD((base + 0xe8), MSDC_PAD_CTL2_DATDRVN, dat_drv1);
	MSDC_GET_FIELD((base + 0xe8), MSDC_PAD_CTL2_DATDRVP, dat_drv2);*/

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, u8_DDLSEL);
	cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
	u8_dat0 = (cur_rxdly0 >> 24) & 0x1F;
	u8_dat1 = (cur_rxdly0 >> 16) & 0x1F;
	u8_dat2 = (cur_rxdly0 >>  8) & 0x1F;
	u8_dat3 = (cur_rxdly0 >>  0) & 0x1F;

	MSDC_GET_FIELD((base + 0xf0), MSDC_PAD_TUNE0_DATWRDLY, u8_wdat);
	MSDC_GET_FIELD((base + 0xf0), MSDC_PAD_TUNE0_CMDRRDLY, u8_cmddat);

	seq_puts(m, "\n=========================================\n");

#if defined(CONFIG_MTK_WCN_CMB_SDIO_SLOT)
	seq_puts(m, "(1) WCN SDIO SLOT is at msdc<%d>\n",
		CONFIG_MTK_WCN_CMB_SDIO_SLOT);
#endif

	seq_puts(m, "-----------------------------------------\n");
	seq_puts(m, "(2) clk settings\n");
	seq_puts(m, "Only using internal clock\n");

	seq_puts(m, "-----------------------------------------\n");
	seq_puts(m, "(3) settings of driving current\n");
	if ((clk_drv1 == clk_drv2) && (cmd_drv1 == cmd_drv2)
		&& (dat_drv1 == dat_drv2) && (clk_drv2 == cmd_drv1)
		&& (cmd_drv2 == dat_drv1)) {
		seq_printf(m, "driving current is <%d>\n", clk_drv1);
	} else {
		seq_printf(m, "clk_drv1<%d>  clk_drv2<%d>  cmd_drv1<%d>  cmd_drv2<%d>  dat_drv1<%d>  dat_drv2<%d>\n",
			clk_drv1, clk_drv2, cmd_drv1, cmd_drv2,
			dat_drv1, dat_drv2);
	}

	seq_puts(m, "-----------------------------------------\n");
	seq_puts(m, "(4) edge settings\n");
	seq_puts(m, "cmd_edge is %s\n", (cmd_edge ? "falling" : "rising"));
	seq_puts(m, "cmd_edge is %s\n", (data_edge ? "falling" : "rising"));

	seq_puts(m, "-----------------------------------------\n");
	seq_puts(m, "(5) data delay info\n");
	seq_printf(m, "Read (MSDC_DAT_RDDLY0) is <0x%x> and (MSDC_IOCON_DDLSEL) is <0x%x>\n",
		cur_rxdly0, u8_DDLSEL);
	seq_printf(m, "data0<0x%x>  data1<0x%x>  data2<0x%x>  data3<0x%x>\n",
		u8_dat0, u8_dat1, u8_dat2, u8_dat3);
	seq_printf(m, "Write is <0x%x>\n", u8_wdat);
	seq_printf(m, "Cmd is <0x%x>\n", u8_cmddat);
	seq_puts(m, "=========================================\n\n");

	return 0;

#else
	seq_puts(m, "\n=========================================\n");
	seq_puts(m, "There is no WCN SDIO SLOT.\n");
	seq_puts(m, "=========================================\n\n");
	return 0;
#endif
}

/*Used by ETT for SDIO 2.0 device, maybe it can be removed*/
static ssize_t msdc_debug_proc_write_FT(struct file *file,
	const char __user *buf, size_t count, loff_t *data)
{
	int ret;
	int i_case = 0, i_par1 = -1, i_par2 = -1;
#if defined(CONFIG_MTK_WCN_CMB_SDIO_SLOT)
	int i_clk = 0;
	int i_driving = 0, i_edge = 0, i_data = 0, i_delay = 0;
	u32 cur_rxdly0;
	u8 u8_dat0, u8_dat1, u8_dat2, u8_dat3;
	void __iomem *base;
#endif
	int sscanf_num;

	if (count == 0)
		return -1;
	if (count > 255)
		count = 255;

	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;

	cmd_buf[count] = '\0';
	pr_err("[SD_Debug]msdc Write %s\n", cmd_buf);

	sscanf_num = sscanf(cmd_buf, "%d %d %d ", &i_case, &i_par1, &i_par2);
	if (sscanf_num < 3)
		return -1;

	if (i_par2 == -1)
		return -1;

	pr_err("i_case=%d i_par1=%d i_par2=%d\n", i_case, i_par1, i_par2);

#if defined(CONFIG_MTK_WCN_CMB_SDIO_SLOT)
	if (CONFIG_MTK_WCN_CMB_SDIO_SLOT == 0 && mtk_msdc_host[0])
		base = mtk_msdc_host[0]->base;
	if (CONFIG_MTK_WCN_CMB_SDIO_SLOT == 1 && mtk_msdc_host[1])
		base = mtk_msdc_host[1]->base;
	if (CONFIG_MTK_WCN_CMB_SDIO_SLOT == 2 && mtk_msdc_host[2])
		base = mtk_msdc_host[2]->base;
	if (CONFIG_MTK_WCN_CMB_SDIO_SLOT == 3 && mtk_msdc_host[3])
		base = mtk_msdc_host[3]->base;
#else
	return -1;
#endif

#if defined(CONFIG_MTK_WCN_CMB_SDIO_SLOT)
	if (i_case == 1) { /*set clk*/
		if (!((i_par1 == 0) || (i_par1 == 1)))
			return -1;
		i_clk = i_par1;

		pr_err("i_clk=%d\n", i_clk);
	} else if (i_case == 2) { /*set driving current*/
		if (!((i_par1 >= 0) && (i_par1 <= 7)))
			return -1;
		i_driving = i_par1;

		pr_err("i_driving=%d\n", i_driving);
	} else if (i_case == 3) { /*set data delay*/
		if (!((i_par1 >= 0) && (i_par1 <= 3)))
			return -1;
		if (!((i_par2 >= 0) && (i_par2 <= 31)))
			return -1;
		i_data = i_par1;
		i_delay = i_par2;

		cur_rxdly0 = MSDC_READ32(MSDC_DAT_RDDLY0);
		u8_dat0 = (cur_rxdly0 >> 24) & 0x1F;
		u8_dat1 = (cur_rxdly0 >> 16) & 0x1F;
		u8_dat2 = (cur_rxdly0 >>  8) & 0x1F;
		u8_dat3 = (cur_rxdly0 >>  0) & 0x1F;

		if (i_data == 0) {
			u8_dat0 = i_delay;
		} else if (i_data == 1) {
			u8_dat1 = i_delay;
		} else if (i_data == 2) {
			u8_dat2 = i_delay;
		} else if (i_data == 3) {
			u8_dat3 = i_delay;
		} else if (i_data == 4) { /*write data*/
			MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY,
				i_delay);
		} else if (i_data == 5)  { /*cmd data*/
			MSDC_SET_FIELD(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY,
				i_delay);
		} else {
			return -1;
		}

		cur_rxdly0 = ((u8_dat0 & 0x1F) << 24) | ((u8_dat1 & 0x1F) << 16)
			| ((u8_dat2 & 0x1F) << 8) | ((u8_dat3 & 0x1F) << 0);
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
		MSDC_WRITE32(MSDC_DAT_RDDLY0, cur_rxdly0);

		pr_err("i_data=%d i_delay=%d\n", i_data, i_delay);
	} else if (i_case == 4) {/*set edge*/
		if (!((i_par1 == 0) || (i_par1 == 1)))
			return -1;
		i_edge = i_par1;

		MSDC_SET_FIELD((base+0x04), MSDC_IOCON_RSPL, i_edge);
		MSDC_SET_FIELD((base+0x04), MSDC_IOCON_R_D_SMPL, i_edge);

		pr_err("i_edge=%d\n", i_edge);
	} else {
		return -1;
	}

	return 1;
#endif
}

#ifdef ONLINE_TUNING_DVTTEST

static int msdc_debug_proc_read_DVT_show(struct seq_file *m, void *data)
{
	return 0;
}

static ssize_t msdc_debug_proc_write_DVT(struct file *file,
	const char __user *buf, size_t count, loff_t *data)
{
	int ret;
	int i_msdc_id = 0;
	int sscanf_num;

	struct msdc_host *host;
	if (count == 0)
		return -1;
	if (count >= 255)
		count = 255;
	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;

	cmd_buf[count] = '\0';
	pr_err("[SD_Debug]msdc Write %s\n", cmd_buf);

	/* sscanf_num = sscanf(cmd_buf, "%d", &i_msdc_id);*/
	i_msdc_id = kstrtoint(cmd_buf);
	if (sscanf_num < 1)
		return -1;

	if ((i_msdc_id < 0) || (i_msdc_id >= HOST_MAX_NUM)) {
		pr_err("[SD_Debug]invalid msdc host_id\n");
		return -1;
	}

	host = mtk_msdc_host[i_msdc_id];
	if (host) {
		pr_err("[SD_Debug] Start Online Tuning DVT test\n");
		mt_msdc_online_tuning_test(host, 0, 0, 0);
		pr_err("[SD_Debug] Finish Online Tuning DVT test\n");
	}
	return count;
}
#endif  /* ONLINE_TUNING_DVTTEST*/

static int msdc_tune_proc_read_show(struct seq_file *m, void *data)
{
	seq_puts(m, "\n=========================================\n");
	seq_printf(m, "sdio_enable_tune: 0x%.8x\n", sdio_enable_tune);
	seq_printf(m, "sdio_iocon_dspl: 0x%.8x\n", sdio_iocon_dspl);
	seq_printf(m, "sdio_iocon_w_dspl: 0x%.8x\n", sdio_iocon_w_dspl);
	seq_printf(m, "sdio_iocon_rspl: 0x%.8x\n", sdio_iocon_rspl);
	seq_printf(m, "sdio_pad_tune_rrdly: 0x%.8x\n", sdio_pad_tune_rrdly);
	seq_printf(m, "sdio_pad_tune_rdly: 0x%.8x\n", sdio_pad_tune_rdly);
	seq_printf(m, "sdio_pad_tune_wrdly: 0x%.8x\n", sdio_pad_tune_wrdly);
	seq_printf(m, "sdio_dat_rd_dly0_0: 0x%.8x\n", sdio_dat_rd_dly0_0);
	seq_printf(m, "sdio_dat_rd_dly0_1: 0x%.8x\n", sdio_dat_rd_dly0_1);
	seq_printf(m, "sdio_dat_rd_dly0_2: 0x%.8x\n", sdio_dat_rd_dly0_2);
	seq_printf(m, "sdio_dat_rd_dly0_3: 0x%.8x\n", sdio_dat_rd_dly0_3);
	seq_printf(m, "sdio_dat_rd_dly1_0: 0x%.8x\n", sdio_dat_rd_dly1_0);
	seq_printf(m, "sdio_dat_rd_dly1_1: 0x%.8x\n", sdio_dat_rd_dly1_1);
	seq_printf(m, "sdio_dat_rd_dly1_2: 0x%.8x\n", sdio_dat_rd_dly1_2);
	seq_printf(m, "sdio_dat_rd_dly1_3: 0x%.8x\n", sdio_dat_rd_dly1_3);
	seq_printf(m, "sdio_clk_drv: 0x%.8x\n", sdio_clk_drv);
	seq_printf(m, "sdio_cmd_drv: 0x%.8x\n", sdio_cmd_drv);
	seq_printf(m, "sdio_data_drv: 0x%.8x\n", sdio_data_drv);
	seq_printf(m, "sdio_tune_flag: 0x%.8x\n", sdio_tune_flag);
	seq_puts(m, "=========================================\n\n");

	return 0;
}

static ssize_t msdc_tune_proc_write(struct file *file, const char __user *buf,
	size_t count, loff_t *data)
{
#ifdef CONFIG_SDIOAUTOK_SUPPORT
	int ret;
	int cmd, p1, p2;

	if (count == 0)
		return -1;
	if (count > 255)
		count = 255;

	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;

	cmd_buf[count] = '\0';
	pr_err("msdc Write %s\n", cmd_buf);

	if (3  == sscanf(cmd_buf, "%x %x %x", &cmd, &p1, &p2)) {
		switch (cmd) {
		case 0:
			if (p1 && p2) {
				/*sdio_enable_tune = 1;*/
				ettagent_init();
			} else {
				/*sdio_enable_tune = 0;*/
				ettagent_exit();
			}
			break;
		case 1:/*Cmd and Data latch edge*/
			sdio_iocon_rspl = p1&0x1;
			sdio_iocon_dspl = p2&0x1;
			break;
		case 2:/*Cmd Pad/Async*/
			sdio_pad_tune_rrdly = p1&0x1F;
			sdio_pad_tune_rdly = p2&0x1F;
			break;
		case 3:
			sdio_dat_rd_dly0_0 = p1&0x1F;
			sdio_dat_rd_dly0_1 = p2&0x1F;
			break;
		case 4:
			sdio_dat_rd_dly0_2 = p1&0x1F;
			sdio_dat_rd_dly0_3 = p2&0x1F;
			break;
		case 5:/*Write data edge/delay*/
			sdio_iocon_w_dspl = p1&0x1;
			sdio_pad_tune_wrdly = p2&0x1F;
			break;
		case 6:
			sdio_dat_rd_dly1_2 = p1&0x1F;
			sdio_dat_rd_dly1_3 = p2&0x1F;
			break;
		case 7:
			sdio_clk_drv = p1&0x7;
			break;
		case 8:
			sdio_cmd_drv = p1&0x7;
			sdio_data_drv = p2&0x7;
			break;
		}
	}
#endif
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

static int msdc_FT_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_debug_proc_read_FT_show,
		inode->i_private);
}

static const struct file_operations msdc_FT_fops = {
	.open = msdc_FT_open,
	.write = msdc_debug_proc_write_FT,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifdef ONLINE_TUNING_DVTTEST
static int msdc_DVT_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_debug_proc_read_DVT_show,
		inode->i_private);
}

static const struct file_operations msdc_DVT_fops = {
	.open = msdc_DVT_open,
	.write = msdc_debug_proc_write_DVT,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif				/* ONLINE_TUNING_DVTTEST */

static int msdc_tune_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_tune_proc_read_show, inode->i_private);
}

static const struct file_operations msdc_tune_fops = {
	.open = msdc_tune_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = msdc_tune_proc_write,
};

static int msdc_tune_flag_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_tune_flag_proc_read_show,
		inode->i_private);
}

static const struct file_operations msdc_tune_flag_fops = {
	.open = msdc_tune_flag_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifdef MSDC_HQA
u32 sdio_vio18_flag, sdio_vcore1_flag, sdio_vcore2_flag;
u32 vio18_reg, vcore1_reg, vcore2_reg;

static ssize_t msdc_voltage_proc_write(struct file *file,
	const char __user *buf, size_t count, loff_t *data)
{
	int ret;
	int sscanf_num;

	if (count == 0)
		return -1;
	if (count >= 255)
		count = 255;
	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;

	cmd_buf[count] = '\0';
	pr_err("[SD_Debug]msdc Write %s\n", cmd_buf);

	sscanf_num = sscanf(cmd_buf, "%d %d %d", &sdio_vio18_flag,
		&sdio_vcore1_flag, &sdio_vcore2_flag);

	if (sscanf_num < 3)
		return -1;

	if (sdio_vio18_flag >= 1730 && sdio_vio18_flag <= 1880) {
		/*0.01V per step*/
		if (sdio_vio18_flag > 1800)
			vio18_reg = 8-(sdio_vio18_flag-1800)/10;
		else
			vio18_reg = (1800-sdio_vio18_flag)/10;
		pmic_config_interface(REG_VIO18_CAL, vio18_reg,
			VIO18_CAL_MASK, VIO18_CAL_SHIFT);
	}

	/*Vcore2 is VCORE_AO*/
	if (sdio_vcore2_flag > 900 && sdio_vcore2_flag < 1200) {
		/*0.00625V per step*/
		/*Originally divied by 12.5, to avoid floating-point division,
		 amplify numerator and denominator by 4*/
		vcore2_reg = ((sdio_vcore2_flag-600)<<2)/25;
		pmic_config_interface(REG_VCORE_VOSEL_SW, vcore2_reg,
			VCORE_VOSEL_SW_MASK, VCORE_VOSEL_SW_SHIFT);
		pmic_config_interface(REG_VCORE_VOSEL_HW, vcore2_reg,
			VCORE_VOSEL_HW_MASK, VCORE_VOSEL_HW_SHIFT);
	}

	return count;
}

static int msdc_voltage_flag_proc_read_show(struct seq_file *m, void *data)
{
	seq_printf(m, "vio18: 0x%d 0x%X\n", sdio_vio18_flag, vio18_reg);
	seq_printf(m, "vcore1: 0x%d 0x%X\n", sdio_vcore1_flag,  vcore1_reg);
	seq_printf(m, "vcore2: 0x%d 0x%X\n", sdio_vcore2_flag, vcore2_reg);
	return 0;
}

static int msdc_voltage_flag_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_voltage_flag_proc_read_show,
		inode->i_private);
}

static const struct file_operations msdc_voltage_flag_fops = {
	.open = msdc_voltage_flag_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = msdc_voltage_proc_write,
};
#endif

#define PERM
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

#ifdef MSDC_DMA_ADDR_DEBUG
	msdc_init_dma_latest_address();
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(msdc_debug_proc_init);
