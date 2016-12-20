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
/* system includes */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/sched/rt.h>
#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/bitops.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/suspend.h>
#include <linux/topology.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mt_io.h>
#include <mt-plat/aee.h>

#include "sspm_ipi.h"
#include "mt_cpufreq_internal.h"
#include "mt_cpufreq_hybrid.h"

#define DEFINE_FOPS_RO(fname)						\
static int fname##_open(struct inode *inode, struct file *file)		\
{									\
	return single_open(file, fname##_show, inode->i_private);	\
}									\
static const struct file_operations fname##_fops = {			\
	.open		= fname##_open,					\
	.read		= seq_read,					\
	.llseek		= seq_lseek,					\
	.release	= single_release,				\
}

#ifdef CONFIG_HYBRID_CPU_DVFS
spinlock_t cpudvfs_lock;
static struct task_struct *Ripi_cpu_dvfs_task;
struct ipi_action ptpod_act;
uint32_t ptpod_buf[4];
int Ripi_cpu_dvfs_thread(void *data)
{
	int i, ret;
	struct mt_cpu_dvfs *p;
	unsigned long flags;
	uint32_t adata;
	uint32_t pwdata[4];
	struct cpufreq_freqs freqs;

	/* cpufreq_info("CPU DVFS received thread\n"); */
	ptpod_act.data = (void *)ptpod_buf;
	ret = sspm_ipi_recv_registration_ex(IPI_ID_CPU_DVFS, &cpudvfs_lock, &ptpod_act);

	if (ret != 0) {
		cpufreq_err("Error: ipi_recv_registration CPU DVFS error: %d\n", ret);
		do {
			msleep(1000);
		} while (!kthread_should_stop());
		return (-1);
	}
	/* cpufreq_info("sspm_ipi_recv_registration IPI_ID_CPU_DVFS pass!!(%d)\n", ret); */

	/* an endless loop in which we are doing our work */
	i = 0;
	do {
		i++;
		/* cpufreq_info("sspm_ipi_recv_wait IPI_ID_CPU_DVFS\n"); */
		sspm_ipi_recv_wait(IPI_ID_CPU_DVFS);
		/* cpufreq_info("Info: CPU DVFS thread received ID=%d, i=%d\n", ptpod_act.id, i); */
		spin_lock_irqsave(&cpudvfs_lock, flags);
		memcpy(pwdata, ptpod_buf, sizeof(pwdata));
		spin_unlock_irqrestore(&cpudvfs_lock, flags);

		cpufreq_lock(flags);
		for_each_cpu_dvfs_only(i, p) {
#if 0
			cpufreq_info("DVFS - Received %s: (old | new freq:%d %d, limit:%d, base:%d)\n",
				cpu_dvfs_get_name(p), cpu_dvfs_get_freq_by_idx(p, p->idx_opp_tbl),
				cpu_dvfs_get_freq_by_idx(p, (int)(pwdata[0] >> (8*i)) & 0xF),
				cpu_dvfs_get_freq_by_idx(p, (int)((pwdata[1] >> (8*i+4)) & 0xF)),
				cpu_dvfs_get_freq_by_idx(p, (int)((pwdata[1] >> (8*i)) & 0xF)));
#endif
			if (p->idx_opp_tbl != (int)((pwdata[0] >> (8*i)) & 0xF)) {
				freqs.old = cpu_dvfs_get_freq_by_idx(p, p->idx_opp_tbl);
				freqs.new = cpu_dvfs_get_freq_by_idx(p, (int)(pwdata[0] >> (8*i)) & 0xF);
				p->idx_opp_tbl = (int)(pwdata[0] >> (8*i) & 0xF);
				if (p->mt_policy) {
					freqs.cpu = p->mt_policy->cpu;
					cpufreq_freq_transition_begin(p->mt_policy, &freqs);
					cpufreq_freq_transition_end(p->mt_policy, &freqs, 0);
					p->mt_policy->min =
						cpu_dvfs_get_freq_by_idx(p, (int)((pwdata[1] >> (8*i)) & 0xF));
					p->mt_policy->max =
						cpu_dvfs_get_freq_by_idx(p, (int)((pwdata[1] >> (8*i+4)) & 0xF));
				}
			} else {
				if (p->mt_policy) {
					p->mt_policy->min =
						cpu_dvfs_get_freq_by_idx(p, (int)((pwdata[1] >> (8*i)) & 0xF));
					p->mt_policy->max =
						cpu_dvfs_get_freq_by_idx(p, (int)((pwdata[1] >> (8*i+4)) & 0xF));
				}
			}
		}
		cpufreq_unlock(flags);

		/* cpufreq_info("Info: CPU DVFS Task send back ack data=%08X\n", adata); */
		ret = sspm_ipi_send_ack(IPI_ID_CPU_DVFS, &adata);
		if (ret != 0)
			cpufreq_err("Error: ipi_send_ack CPU DVFS error: %d\n", ret);

		/* cpufreq_info("Info: CPU DVFS Task send back Done\n"); */
	} while (!kthread_should_stop());
	return 0;
}

int has_dvfs;
int dvfs_to_spm2_command(u32 cmd, struct cdvfs_data *cdvfs_d)
{
#define OPT				(0) /* reserve for extensibility */
#define DVFS_D_LEN		(4) /* # of cmd + arg0 + arg1 + ... */
	unsigned int len = DVFS_D_LEN;
	int ack_data;
	unsigned int ret = 0;

	/*pr_debug("#@# %s(%d) cmd %x\n", __func__, __LINE__, cmd);*/

	switch (cmd) {
	case IPI_DVFS_INIT_PTBL:
		cdvfs_d->cmd = cmd;

		pr_err("I'd like to initialize sspm DVFS, segment code = %d\n", cdvfs_d->u.set_fv.arg[0]);

		/* ret = sspm_ipi_send_sync(iSPEED_DEV_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data); */
		ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;

	case IPI_DVFS_INIT:
		cdvfs_d->cmd = cmd;

		pr_err("I'd like to initialize sspm DVFS, segment code = %d\n", cdvfs_d->u.set_fv.arg[0]);

		/* ret = sspm_ipi_send_sync(iSPEED_DEV_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data); */
		ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;

	case IPI_SET_DVFS:
		cdvfs_d->cmd = cmd;

		cpufreq_ver("I'd like to set cluster%d to freq%d\n", cdvfs_d->u.set_fv.arg[0],
			cdvfs_d->u.set_fv.arg[1]);

		/* ret = sspm_ipi_send_sync(iSPEED_DEV_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data); */
		ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);
		has_dvfs = 1;
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;

	case IPI_SET_CLUSTER_ON_OFF:
		cdvfs_d->cmd = cmd;

		pr_err("I'd like to set cluster%d ON/OFF state to %d)\n",
			cdvfs_d->u.set_fv.arg[0], cdvfs_d->u.set_fv.arg[1]);

		if (is_in_suspend()) {
			ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, IPI_OPT_LOCK_POLLING, cdvfs_d, len, &ack_data);
			pr_err("Send with IPI_OPT_LOCK_POLLING\n");
		} else
			ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);

		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;

	case IPI_SET_FREQ:
		cdvfs_d->cmd = cmd;

		pr_err("I'd like to set cluster%d freq to %d)\n",
			cdvfs_d->u.set_fv.arg[0], cdvfs_d->u.set_fv.arg[1]);

		/* ret = sspm_ipi_send_sync(iSPEED_DEV_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data); */
		ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);
		has_dvfs = 1;
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;
#if 0
	case IPI_SET_VOLT:
		cdvfs_d->cmd = cmd;

		pr_err("I'd like to set cluster%d volt to %d)\n",
			cdvfs_d->u.set_fv.arg[0], cdvfs_d->u.set_fv.arg[1]);

		/* ret = sspm_ipi_send_sync(iSPEED_DEV_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data); */
		ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;
#endif
	case IPI_GET_VOLT:
		cdvfs_d->cmd = cmd;

		pr_err("I'd like to get volt from Buck%d\n", cdvfs_d->u.set_fv.arg[0]);

		/* ret = sspm_ipi_send_sync(iSPEED_DEV_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data); */
		ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);
		pr_err("Get volt = %d\n", ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		ret = ack_data;
		break;

	case IPI_GET_FREQ:
		cdvfs_d->cmd = cmd;

		pr_err("I'd like to get freq from pll%d\n", cdvfs_d->u.set_fv.arg[0]);
		if (is_in_suspend()) {
			ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, IPI_OPT_LOCK_POLLING, cdvfs_d, len, &ack_data);
			pr_err("Send with IPI_OPT_LOCK_POLLING\n");
		} else
			ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);

		pr_err("Get freq = %d\n", ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		ret = ack_data;
		break;

	case IPI_PAUSE_DVFS:
		cdvfs_d->cmd = cmd;

		pr_err("I'd like to set dvfs enable status to %d\n", cdvfs_d->u.set_fv.arg[0]);

		/* ret = sspm_ipi_send_sync(iSPEED_DEV_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data); */
		ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;

	case IPI_TURBO_MODE:
		cdvfs_d->cmd = cmd;

		cpufreq_info("I'd like to set turbo mode to %d(%d, %d)\n",
			cdvfs_d->u.set_fv.arg[0], cdvfs_d->u.set_fv.arg[1], cdvfs_d->u.set_fv.arg[2]);

		if (is_in_suspend()) {
			ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, IPI_OPT_LOCK_POLLING, cdvfs_d, len, &ack_data);
			pr_err("Send with IPI_OPT_LOCK_POLLING\n");
		} else
			ret = sspm_ipi_send_sync(IPI_ID_CPU_DVFS, OPT, cdvfs_d, len, &ack_data);

		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync ret %d\n", __func__, __LINE__, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;

	default:
		pr_err("#@# %s(%d) cmd(%d) wrong!!!\n", __func__, __LINE__, cmd);
		break;
	}

	return ret;
}

#include <linux/of_address.h>
u32 *g_dbg_repo;
static u32 cmcu_probe_done;
void __iomem *log_repo;
static void __iomem *csram_base;
static void __iomem *cspm_base;
#define csram_read(offs)		__raw_readl(csram_base + (offs))
#define csram_write(offs, val)		mt_reg_sync_writel(val, csram_base + (offs))
static struct hrtimer nfy_trig_timer;
static struct task_struct *dvfs_nfy_task;
static atomic_t dvfs_nfy_req = ATOMIC_INIT(0);

/* #define CSRAM_BASE		0x00100000 */
#define CSRAM_BASE		0x0012a000
#define CSRAM_SIZE		0x3000		/* 12K bytes */
#define ENTRY_EACH_LOG		7
#define OFFS_TBL_S		0x0010
#define OFFS_TBL_E		0x0250
#define PVT_TBL_SIZE    (OFFS_TBL_E - OFFS_TBL_S)
#define OFFS_DATA_S		0x02a0
#define OFFS_LOG_S		0x03d0
#define OFFS_LOG_E		0x0ec0
/* #define OFFS_LOG_E		0x2ff8 */
#define DBG_REPO_S		CSRAM_BASE
#define DBG_REPO_E		(DBG_REPO_S + CSRAM_SIZE)
#define DBG_REPO_TBL_S		(DBG_REPO_S + OFFS_TBL_S)
#define DBG_REPO_TBL_E		(DBG_REPO_S + OFFS_TBL_E)
#define DBG_REPO_DATA_S		(DBG_REPO_S + OFFS_DATA_S)
#define DBG_REPO_DATA_E		(DBG_REPO_S + OFFS_LOG_S)
#define DBG_REPO_LOG_S		(DBG_REPO_S + OFFS_LOG_S)
#define DBG_REPO_LOG_E		(DBG_REPO_S + OFFS_LOG_E)
#define DBG_REPO_SIZE		(DBG_REPO_E - DBG_REPO_S)
#define DBG_REPO_NUM		(DBG_REPO_SIZE / sizeof(u32))
#define REPO_I_DATA_S		(OFFS_DATA_S / sizeof(u32))
#define REPO_I_LOG_S		(OFFS_LOG_S / sizeof(u32))
#define REPO_GUARD0		0x55aa55aa
#define REPO_GUARD1		0xaa55aa55
#ifdef USE_16_OPP
#define NR_FREQ       16
#else
#define NR_FREQ       4
#endif
#define MAX_LOG_FETCH		20

static u32 dbg_repo_bak[DBG_REPO_NUM];
/* log_box[MAX_LOG_FETCH] is also used to save last log entry */
static struct cpu_dvfs_log_box log_box_parsed[1 + MAX_LOG_FETCH] __nosavedata;
static unsigned int next_log_offs __nosavedata = OFFS_LOG_S;
static dvfs_notify_t notify_dvfs_change_cb;

void parse_time_log_content(unsigned int *local_buf, int idx)
{
	struct cpu_dvfs_log *log_box = (struct cpu_dvfs_log *)local_buf;

	log_box_parsed[idx].time_stamp = ((unsigned long long)log_box->time_stamp_h_log << 32) |
		(unsigned long long)(log_box->time_stamp_l_log);

	cpufreq_ver("log_box[%d]->time_stamp_log = %llu\n",
		idx, log_box_parsed[idx].time_stamp);
}

void parse_log_content(unsigned int *local_buf, int idx)
{
	struct cpu_dvfs_log *log_box = (struct cpu_dvfs_log *)local_buf;
	struct mt_cpu_dvfs *p;
	int i;

	for_each_cpu_dvfs(i, p) {
		log_box_parsed[idx].cluster_opp_cfg[i].freq_idx = log_box->cluster_opp_cfg[i].opp_idx_log;
		if (cpu_dvfs_is(p, MT_CPU_DVFS_LL)) {
			log_box_parsed[idx].cluster_opp_cfg[i].limit_idx = log_box->h_ll_limit;
			log_box_parsed[idx].cluster_opp_cfg[i].base_idx = log_box->l_ll_limit;
		} else if (cpu_dvfs_is(p, MT_CPU_DVFS_L)) {
			log_box_parsed[idx].cluster_opp_cfg[i].limit_idx = log_box->h_l_limit;
			log_box_parsed[idx].cluster_opp_cfg[i].base_idx = log_box->l_l_limit;
		} else if (cpu_dvfs_is(p, MT_CPU_DVFS_B)) {
			log_box_parsed[idx].cluster_opp_cfg[i].limit_idx = log_box->h_b_limit;
			log_box_parsed[idx].cluster_opp_cfg[i].base_idx = log_box->l_b_limit;
		}
	}

#if 0
	for_each_cpu_dvfs(i, p) {
		cpufreq_info("[%s]log_box->vproc_ll_log = 0x%x\n",
			cpu_dvfs_get_name(p), log_box->cluster_opp_cfg[i].vproc_log);
		cpufreq_info("[%s]log_box->opp_ll_log = 0x%x\n",
			cpu_dvfs_get_name(p), log_box->cluster_opp_cfg[i].opp_idx_log);
		cpufreq_info("[%s]log_box->wfi_ll_log = 0x%x\n",
			cpu_dvfs_get_name(p), log_box->cluster_opp_cfg[i].wfi_log);
		cpufreq_info("[%s]log_box->vsram_ll_log = 0x%x\n",
			cpu_dvfs_get_name(p), log_box->cluster_opp_cfg[i].vsram_log);
	}

	cpufreq_info("log_box->pause_bit = 0x%x\n", log_box->pause_bit);
	cpufreq_info("log_box->b_en = 0x%x\n", log_box->b_en);
	cpufreq_info("log_box->h_b_limit = 0x%x\n", log_box->h_b_limit);
	cpufreq_info("log_box->l_b_limit = 0x%x\n", log_box->l_b_limit);
	cpufreq_info("log_box->l_en = 0x%x\n", log_box->l_en);
	cpufreq_info("log_box->h_l_limit = 0x%x\n", log_box->h_l_limit);
	cpufreq_info("log_box->l_l_limit = 0x%x\n", log_box->l_l_limit);
	cpufreq_info("log_box->ll_en = 0x%x\n", log_box->ll_en);
	cpufreq_info("log_box->h_ll_limit = 0x%x\n", log_box->h_ll_limit);
	cpufreq_info("log_box->l_ll_limit = 0x%x\n", log_box->l_ll_limit);
#endif
}

static void fetch_dvfs_log_and_notify(void)
{
	int i, j;
	unsigned int buf[ENTRY_EACH_LOG] = {0};
	unsigned int next_log_offs_bk = 0;

#if 0
	cpufreq_ver("DVFS - Do thread task Begin!\n");

	if (log_box_parsed[MAX_LOG_FETCH].time_stamp == 0 &&
					next_log_offs <= OFFS_LOG_S) {
		return;
	}
#endif

	if (has_dvfs == 0)
		return;

	log_box_parsed[0] = log_box_parsed[MAX_LOG_FETCH];

	for (i = 1; i <= MAX_LOG_FETCH; i++) {

		next_log_offs_bk = next_log_offs;
		buf[0] = csram_read(next_log_offs_bk);
		next_log_offs_bk += 4;
		if (next_log_offs_bk >= OFFS_LOG_E)
			next_log_offs_bk = OFFS_LOG_S;
		buf[1] = csram_read(next_log_offs_bk);
		parse_time_log_content(buf, i);

		cpufreq_ver("log_box_parsed[%d] = %llu < log_box_parsed[%d] = %llu\n",
			i, log_box_parsed[i].time_stamp, (i-1), log_box_parsed[i-1].time_stamp);

		if (log_box_parsed[i].time_stamp < log_box_parsed[i-1].time_stamp)
			break;

		for (j = 0; j < ENTRY_EACH_LOG; j++) {
			if (j == 0) {
				next_log_offs_bk = next_log_offs;
				cpufreq_ver("backup next_log_offs_bk = 0x%x, next_log_offs = 0x%x\n",
					next_log_offs_bk, next_log_offs);
			}
			buf[j] = csram_read(next_log_offs);
			cpufreq_ver("DVFS receive - buf[%d] = addr[0x%x] = 0x%x!\n",
				j, next_log_offs, buf[j]);

			next_log_offs += 4;
			if (next_log_offs >= OFFS_LOG_E)
				next_log_offs = OFFS_LOG_S;
		}
		parse_log_content(buf, i);
	}

	if (log_box_parsed[0].time_stamp == 0 && i >= 2)
		log_box_parsed[0] = log_box_parsed[1];

	if (i >= 2 && i <= MAX_LOG_FETCH)
		log_box_parsed[MAX_LOG_FETCH] = log_box_parsed[i - 1];

	if (notify_dvfs_change_cb && log_box_parsed[0].time_stamp > 0)
		notify_dvfs_change_cb(log_box_parsed, i);

	/* cpufreq_info("DVFS - Do thread task Done!\n"); */
	}

/* #ifdef CPUHVFS_HW_GOVERNOR */
#if 1
void cpuhvfs_register_dvfs_notify(dvfs_notify_t callback)
{
	notify_dvfs_change_cb = callback;
}
#endif

static inline void start_notify_trigger_timer(u32 intv_ms)
{
	hrtimer_start(&nfy_trig_timer, ms_to_ktime(intv_ms), HRTIMER_MODE_REL);
}

static inline void stop_notify_trigger_timer(void)
{
	hrtimer_cancel(&nfy_trig_timer);
}

static inline void kick_kthread_to_notify(void)
{
	atomic_inc(&dvfs_nfy_req);
	wake_up_process(dvfs_nfy_task);
}

static int dvfs_nfy_task_fn(void *data)
{
	while (1) {
		set_current_state(TASK_UNINTERRUPTIBLE);

		if (atomic_read(&dvfs_nfy_req) <= 0) {
			schedule();
			continue;
		}

		__set_current_state(TASK_RUNNING);

		fetch_dvfs_log_and_notify();
		atomic_dec(&dvfs_nfy_req);
	}

	return 0;
}

#define DVFS_NOTIFY_INTV	20		/* ms */
static enum hrtimer_restart nfy_trig_timer_fn(struct hrtimer *timer)
{
	kick_kthread_to_notify();

	hrtimer_forward_now(timer, ms_to_ktime(DVFS_NOTIFY_INTV));

	return HRTIMER_RESTART;
}

static int create_resource_for_dvfs_notify(void)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 2 };	/* lower than WDK */

	/* init hrtimer */
	hrtimer_init(&nfy_trig_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	nfy_trig_timer.function = nfy_trig_timer_fn;

	/* create kthread */
	dvfs_nfy_task = kthread_create(dvfs_nfy_task_fn, NULL, "dvfs_log_dump");

	if (IS_ERR(dvfs_nfy_task))
		return PTR_ERR(dvfs_nfy_task);

	sched_setscheduler_nocheck(dvfs_nfy_task, SCHED_FIFO, &param);
	get_task_struct(dvfs_nfy_task);

	return 0;
}

static int _mt_cmcu_pdrv_probe(struct platform_device *pdev)
{
	cspm_base = of_iomap(pdev->dev.of_node, 0);
	if (!cspm_base)
		return -ENOMEM;

	csram_base = of_iomap(pdev->dev.of_node, 1);

	if (!csram_base)
		return -ENOMEM;

	cmcu_probe_done = 1;

	return 0;
}

static int _mt_cmcu_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cmcu_of_match[] = {
	{ .compatible = "mediatek,mt6797-cmcu", },
	{}
};
#endif

static struct platform_driver _mt_cmcu_pdrv = {
	.probe = _mt_cmcu_pdrv_probe,
	.remove = _mt_cmcu_pdrv_remove,
	.driver = {
		   .name = "cmcu",
		   .owner = THIS_MODULE,
		   .of_match_table	= of_match_ptr(cmcu_of_match),
	},
};

int cpuhvfs_set_init_ptbl(void)
{
	struct cdvfs_data cdvfs_d;

	/* seg code */
	cdvfs_d.u.set_fv.arg[0] = 0;
	dvfs_to_spm2_command(IPI_DVFS_INIT_PTBL, &cdvfs_d);

	return 0;
}


int cpuhvfs_set_init_sta(void)
{
	struct cdvfs_data cdvfs_d;

	/* seg code */
	cdvfs_d.u.set_fv.arg[0] = 0;
	dvfs_to_spm2_command(IPI_DVFS_INIT, &cdvfs_d);

	return 0;
}

int cpuhvfs_set_cluster_on_off(int cluster_id, int state)
{
	struct cdvfs_data cdvfs_d;

	/* Cluster, ON:1/OFF:0 */
	cdvfs_d.u.set_fv.arg[0] = cluster_id;
	cdvfs_d.u.set_fv.arg[1] = state;

	dvfs_to_spm2_command(IPI_SET_CLUSTER_ON_OFF, &cdvfs_d);

	return 0;
}

int cpuhvfs_set_mix_max(int cluster_id, int base, int limit)
{
	return 0;
}

int cpuhvfs_set_dvfs(int cluster_id, unsigned int freq)
{
	struct cdvfs_data cdvfs_d;

	/* Cluster, Freq */
	cdvfs_d.u.set_fv.arg[0] = cluster_id;
	cdvfs_d.u.set_fv.arg[1] = freq;

	dvfs_to_spm2_command(IPI_SET_DVFS, &cdvfs_d);

	return 0;
}

int cpuhvfs_get_volt(int buck_id)
{
	struct cdvfs_data cdvfs_d;
	int ret = 0;

	/* Cluster, Volt */
	cdvfs_d.u.set_fv.arg[0] = buck_id;

	ret = dvfs_to_spm2_command(IPI_GET_VOLT, &cdvfs_d);

	return ret;
}

int cpuhvfs_get_freq(int pll_id)
{
	struct cdvfs_data cdvfs_d;
	int ret = 0;

	/* Cluster, Freq */
	cdvfs_d.u.set_fv.arg[0] = pll_id;

	ret = dvfs_to_spm2_command(IPI_GET_FREQ, &cdvfs_d);

	return ret;
}

int cpuhvfs_set_volt(int cluster_id, unsigned int volt)
{
#if 0
	struct cdvfs_data cdvfs_d;

	cdvfs_d.u.set_fv.arg[0] = cluster_id;
	cdvfs_d.u.set_fv.arg[1] = volt;

	dvfs_to_spm2_command(IPI_SET_VOLT, &cdvfs_d);
#endif
	return 0;
}

int cpuhvfs_set_freq(int cluster_id, unsigned int freq)
{
	struct cdvfs_data cdvfs_d;

	cdvfs_d.u.set_fv.arg[0] = cluster_id;
	cdvfs_d.u.set_fv.arg[1] = freq;

	dvfs_to_spm2_command(IPI_SET_FREQ, &cdvfs_d);

	return 0;
}

int cpuhvfs_pause_dvfsp_running(void)
{
	struct cdvfs_data cdvfs_d;

	/* set on = 0 */
	/* cdvfs_d.u.set_fv.arg[0] = PAUSE_IDLE; */
	cdvfs_d.u.set_fv.arg[0] = PAUSE_SUSPEND_LL;

	dvfs_to_spm2_command(IPI_PAUSE_DVFS, &cdvfs_d);

	/* stop_notify_trigger_timer(); */

	return 0;
}

void cpuhvfs_unpause_dvfsp_to_run(void)
{
	struct cdvfs_data cdvfs_d;

	/* set on = 1 */
	cdvfs_d.u.set_fv.arg[0] = UNPAUSE;

	dvfs_to_spm2_command(IPI_PAUSE_DVFS, &cdvfs_d);

	/* start_notify_trigger_timer(DVFS_NOTIFY_INTV); */
}

#if 0
int cpuhvfs_dvfsp_suspend(void)
{
	cpuhvfs_pause_dvfsp_running(PAUSE_SUSPEND);
	return 0;
}

void cpuhvfs_dvfsp_resume(unsigned int on_cluster)
{
	cpuhvfs_set_cluster_on_off(on_cluster, 1);
	cpuhvfs_unpause_dvfsp_to_run(PAUSE_SUSPEND);
}
#endif

int cpuhvfs_set_turbo_mode(int turbo_mode, int freq_step, int volt_step)
{
	struct cdvfs_data cdvfs_d;

	/* Turbo, ON:1/OFF:0 */
	cdvfs_d.u.set_fv.arg[0] = turbo_mode;
	cdvfs_d.u.set_fv.arg[1] = freq_step;
	cdvfs_d.u.set_fv.arg[2] = volt_step;

	dvfs_to_spm2_command(IPI_TURBO_MODE, &cdvfs_d);

	return 0;
}

/*
* Module driver
*/
#define ARRAY_ROW_SIZE 4
#ifdef USE_16_OPP
unsigned int fyTbl[][ARRAY_ROW_SIZE] = {
/* Freq, Vproc, post_div, clk_div */
	{1638, 0x5C, 1, 2},/* (LL) */
	{1560, 0x57, 1, 2},
	{1495, 0x52, 1, 2},
	{1417, 0x4D, 1, 2},
	{1339, 0x48, 1, 2},
	{1248, 0x43, 1, 2},
	{1144, 0x3E, 1, 2},
	{1040, 0x39, 1, 2},
	{936, 0x34, 1, 2},
	{845, 0x30, 1, 2},
	{754, 0x2C, 1, 2},
	{663, 0x28, 2, 2},
	{585, 0x24, 2, 2},
	{507, 0x20, 2, 2},
	{429, 0x1C, 2, 2},
	{221, 0x18, 2, 2},

	{2340, 0x5C, 1, 1},/* (L) */
	{2262, 0x57, 1, 1},
	{2184, 0x52, 1, 1},
	{2106, 0x4D, 1, 1},
	{2015, 0x48, 1, 1},
	{1872, 0x43, 1, 1},
	{1742, 0x3E, 1, 1},
	{1599, 0x39, 1, 1},
	{1443, 0x34, 2, 1},
	{1300, 0x30, 2, 1},
	{1170, 0x2C, 2, 1},
	{1040, 0x28, 2, 1},
	{923, 0x24, 2, 1},
	{793, 0x20, 2, 1},
	{533, 0x1C, 2, 2},
	{442, 0x18, 2, 2},

	{2548, 0x5C, 1, 1},/* (B) */
	{2444, 0x57, 1, 1},
	{2327, 0x52, 1, 1},
	{2210, 0x4D, 1, 1},
	{2093, 0x48, 1, 1},
	{1911, 0x43, 1, 1},
	{1716, 0x3E, 1, 1},
	{1534, 0x39, 1, 1},
	{1417, 0x38, 2, 1},
	{1339, 0x38, 2, 1},
	{1274, 0x38, 2, 1},
	{1196, 0x38, 2, 1},
	{1040, 0x38, 2, 1},
	{897, 0x38, 2, 1},
	{533, 0x38, 2, 2},
	{442, 0x38, 2, 2},

	{1391, 0x5C, 1, 2},/* (CCI) */
	{1326, 0x57, 1, 2},
	{1274, 0x52, 1, 2},
	{1209, 0x4D, 1, 2},
	{1144, 0x48, 1, 2},
	{1066, 0x43, 1, 2},
	{988, 0x3E, 1, 2},
	{910, 0x39, 1, 2},
	{819, 0x34, 1, 2},
	{741, 0x30, 2, 2},
	{663, 0x2C, 2, 2},
	{598, 0x28, 2, 2},
	{520, 0x24, 2, 2},
	{442, 0x20, 2, 2},
	{390, 0x1C, 2, 2},
	{221, 0x18, 2, 2},
};
#else
unsigned int fyTbl[][ARRAY_ROW_SIZE] = {
/* Freq, Vproc, post_div, clk_div */
	{1638, 0x5C, 1, 2},/* (LL) */
	{1248, 0x43, 1, 2},
	{845, 0x30, 1, 2},
	{507, 0x20, 2, 2},

	{2340, 0x5C, 1, 1},/* (L) */
	{1872, 0x43, 1, 1},
	{1300, 0x30, 2, 1},
	{793, 0x20, 2, 2},

	{2548, 0x5C, 1, 1},/* (B) */
	{1911, 0x43, 1, 1},
	{1339, 0x38, 2, 1},
	{897, 0x38, 2, 1},

	{1391, 0x5C, 1, 2},/* (CCI) */
	{1066, 0x43, 1, 2},
	{741, 0x30, 2, 2},
	{442, 0x20, 2, 2},
};
#endif

u32 *recordRef;
static unsigned int *recordTbl;

void cpuhvfs_pvt_tbl_create(void)
{
	int i;

	recordRef = ioremap_nocache(DBG_REPO_TBL_S, PVT_TBL_SIZE);
	cpufreq_info("DVFS - @(Record)%s----->(%p)\n", __func__, recordRef);
	memset_io((u8 *)recordRef, 0x00, PVT_TBL_SIZE);

	recordTbl = &fyTbl[0][0];

	for (i = 0; i < NR_FREQ; i++) {
		/* Freq, Vproc, post_div, clk_div */
		/* LL [31:16] = Vproc, [15:0] = Freq */
		recordRef[i] =
			((*(recordTbl + (i * ARRAY_ROW_SIZE) + 1) & 0xFF) << 16) |
			(*(recordTbl + (i * ARRAY_ROW_SIZE)) & 0xFFFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i, recordRef[i]);
		/* LL [31:16] = postdiv, [15:0] = clkdiv */
		recordRef[i + NR_FREQ] =
			((*(recordTbl + (i * ARRAY_ROW_SIZE) + 3) & 0xFF) << 16) |
			(*(recordTbl + (i * ARRAY_ROW_SIZE) + 2) & 0xFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + NR_FREQ, recordRef[i + NR_FREQ]);
		/* L [31:16] = Vproc, [15:0] = Freq */
		recordRef[i + 36] =
			((*(recordTbl + ((NR_FREQ * 1) + i) * ARRAY_ROW_SIZE + 1) & 0xFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 1) + i) * ARRAY_ROW_SIZE) & 0xFFFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 36, recordRef[i + 36]);
		/* L [31:16] = postdiv, [15:0] = clkdiv */
		recordRef[i + 36 + NR_FREQ] =
			((*(recordTbl + ((NR_FREQ * 1) + i) * ARRAY_ROW_SIZE + 3) & 0xFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 1) + i) * ARRAY_ROW_SIZE + 2) & 0xFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 36 + NR_FREQ, recordRef[i + 36 + NR_FREQ]);
		/* CCI [31:16] = Vproc, [15:0] = Freq */
		recordRef[i + 72] =
			((*(recordTbl + ((NR_FREQ * 2) + i) * ARRAY_ROW_SIZE + 1) & 0xFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 2) + i) * ARRAY_ROW_SIZE) & 0xFFFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 72, recordRef[i + 72]);
		/* CCI [31:16] = postdiv, [15:0] = clkdiv */
		recordRef[i + 72 + NR_FREQ] =
			((*(recordTbl + ((NR_FREQ * 2) + i) * ARRAY_ROW_SIZE + 3) & 0xFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 2) + i) * ARRAY_ROW_SIZE + 2) & 0xFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 72 + NR_FREQ, recordRef[i + 72 + NR_FREQ]);
		/* BIG [31:16] = Vproc, [15:0] = Freq */
		recordRef[i + 108] =
			((*(recordTbl + ((NR_FREQ * 3) + i) * ARRAY_ROW_SIZE + 1) & 0xFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 3) + i) * ARRAY_ROW_SIZE) & 0xFFFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 108, recordRef[i + 108]);
		/* BIG [31:16] = postdiv, [15:0] = clkdiv */
		recordRef[i + 108 + NR_FREQ] =
			((*(recordTbl + ((NR_FREQ * 3) + i) * ARRAY_ROW_SIZE + 3) & 0xFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 3) + i) * ARRAY_ROW_SIZE + 2) & 0xFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 108 + NR_FREQ, recordRef[i + 108 + NR_FREQ]);
	}
	recordRef[i*2] = 0xffffffff;
	recordRef[i*2+36] = 0xffffffff;
	recordRef[i*2+72] = 0xffffffff;
	recordRef[i*2+108] = 0xffffffff;
	mb(); /* SRAM writing */
}

static int dbg_repo_show(struct seq_file *m, void *v)
{
	int i;
	u32 *repo = m->private;
	char ch;

	for (i = 0; i < DBG_REPO_NUM; i++) {
		if (i >= REPO_I_LOG_S && (i - REPO_I_LOG_S) % ENTRY_EACH_LOG >= (ENTRY_EACH_LOG - 2))
			ch = ':';	/* timestamp */
		else
			ch = '.';

		seq_printf(m, "%4d%c%08x%c", i, ch, repo[i], i % 4 == 3 ? '\n' : ' ');
	}

	return 0;
}

DEFINE_FOPS_RO(dbg_repo);

static int create_cpuhvfs_debug_fs(void)
{
	struct dentry *root;

	/* create /sys/kernel/debug/cpuhvfs */
	root = debugfs_create_dir("cpuhvfs", NULL);
	if (!root)
		return -EPERM;

	debugfs_create_file("dbg_repo", 0444, root, g_dbg_repo, &dbg_repo_fops);
	debugfs_create_file("dbg_repo_bak", 0444, root, dbg_repo_bak, &dbg_repo_fops);

	return 0;
}

int cpuhvfs_module_init(void)
{
	int r;

	if (!log_repo) {
		pr_err("FAILED TO PRE-INIT CPUHVFS\n");
		return -ENODEV;
	}

	r = create_cpuhvfs_debug_fs();
	if (r) {
		pr_err("FAILED TO CREATE DEBUG FILESYSTEM (%d)\n", r);
		return r;
	}

	/* SW Governor Report */
	spin_lock_init(&cpudvfs_lock);
	Ripi_cpu_dvfs_task = kthread_run(Ripi_cpu_dvfs_thread, NULL, "ipi_cpu_dvfs_rtask");

	return 0;
}

static int __init cmcu_module_init(void)
{
	int r;

	r = platform_driver_register(&_mt_cmcu_pdrv);
	if (r)
		pr_err("fail to register cmcu driver @ %s()\n", __func__);

	if (!cmcu_probe_done) {
		pr_err("FAILED TO PROBE CMCU DEVICE\n");
		return -ENODEV;
	}

	/* HW Governor Report */
	r = create_resource_for_dvfs_notify();
	if (r) {
		cpufreq_err("FAILED TO CREATE RESOURCE FOR NOTIFY (%d)\n", r);
		return r;
	}

	log_repo = csram_base;

	return 0;
}

static void init_cpuhvfs_debug_repo(void)
{
	u32 __iomem *dbg_repo = csram_base;

	/* backup debug repo for later analysis */
	memcpy_fromio(dbg_repo_bak, dbg_repo, DBG_REPO_SIZE);

	dbg_repo[0] = REPO_GUARD0;
	dbg_repo[1] = REPO_GUARD1;
	dbg_repo[2] = REPO_GUARD0;
	dbg_repo[3] = REPO_GUARD1;

	/* Clean 0x00100000(CSRAM_BASE) + 0x02a0 count END_SRAM - (DBG_REPO_S + OFFS_DATA_S)*/
	memset_io((void __iomem *)dbg_repo + DBG_REPO_DATA_S - DBG_REPO_S,
		  0,
		  DBG_REPO_E - DBG_REPO_DATA_S);

	dbg_repo[REPO_I_DATA_S] = REPO_GUARD0;
	g_dbg_repo = dbg_repo;
}

static int cpuhvfs_pre_module_init(void)
{
	int r;

	r = cmcu_module_init();
	if (r) {
		cpufreq_err("FAILED TO INIT DVFS SSPM (%d)\n", r);
		return r;
	}

	init_cpuhvfs_debug_repo();

	cpuhvfs_pvt_tbl_create();

	cpuhvfs_set_init_ptbl();

	return 0;
}
fs_initcall(cpuhvfs_pre_module_init);
#endif
MODULE_DESCRIPTION("Hybrid CPU DVFS Driver v0.1");
MODULE_LICENSE("GPL");
