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
#include <mt-plat/mtk_io.h>
#include <mt-plat/aee.h>

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#include "sspm_ipi.h"
#endif

#include "mtk_cpufreq_internal.h"
#include "mtk_cpufreq_hybrid.h"

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
#define NR_FREQ       16

static u32 dbg_repo_bak[DBG_REPO_NUM];
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
unsigned int fyTbl[][ARRAY_ROW_SIZE] = {
/* Freq, Vproc, post_div, clk_div */
	{1508, 0x32A, 1, 2},/* (LL) */
	{1443, 0x316, 1, 2},
	{1391, 0x307, 1, 2},
	{1326, 0x2F3, 1, 2},
	{1261, 0x2DF, 1, 2},
	{1196, 0x2CB, 1, 2},
	{1118, 0x2B7, 1, 2},
	{1053, 0x2A3, 1, 2},
	{975, 0x28A, 1, 2},
	{910, 0x276, 1, 2},
	{819, 0x270, 1, 2},
	{728, 0x270, 2, 2},
	{637, 0x270, 2, 2},
	{546, 0x270, 2, 2},
	{455, 0x270, 2, 2},
	{338, 0x270, 2, 2},

	{1703, 0x32A, 1, 1},/* (L) */
	{1625, 0x316, 1, 1},
	{1560, 0x307, 1, 1},
	{1469, 0x2F3, 1, 1},
	{1391, 0x2DF, 1, 1},
	{1313, 0x2CB, 1, 1},
	{1222, 0x2B7, 1, 1},
	{1144, 0x2A3, 1, 1},
	{1040, 0x28A, 2, 1},
	{949, 0x276, 2, 1},
	{858, 0x270, 2, 1},
	{754, 0x270, 2, 1},
	{663, 0x270, 2, 1},
	{559, 0x270, 2, 1},
	{468, 0x270, 2, 2},
	{338, 0x270, 2, 2},

	{1976, 0x32A, 1, 1},/* (B) */
	{1872, 0x316, 1, 1},
	{1781, 0x307, 1, 1},
	{1677, 0x2F3, 1, 1},
	{1560, 0x2DF, 1, 1},
	{1443, 0x2CB, 1, 1},
	{1339, 0x2B7, 1, 1},
	{1222, 0x2A3, 1, 1},
	{1079, 0x28A, 2, 1},
	{962, 0x276, 2, 1},
	{858, 0x270, 2, 1},
	{767, 0x270, 2, 1},
	{663, 0x270, 2, 1},
	{572, 0x270, 2, 1},
	{468, 0x270, 2, 2},
	{338, 0x270, 2, 2},

	{949, 0x32A, 1, 2},/* (CCI) */
	{910, 0x316, 1, 2},
	{871, 0x307, 1, 2},
	{819, 0x2F3, 1, 2},
	{767, 0x2DF, 1, 2},
	{728, 0x2CB, 1, 2},
	{676, 0x2B7, 1, 2},
	{624, 0x2A3, 1, 2},
	{559, 0x28A, 1, 2},
	{520, 0x276, 2, 2},
	{455, 0x270, 2, 2},
	{390, 0x270, 2, 2},
	{325, 0x270, 2, 2},
	{273, 0x270, 2, 2},
	{208, 0x270, 2, 2},
	{130, 0x270, 2, 2},
};

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
			((*(recordTbl + (i * ARRAY_ROW_SIZE) + 1) & 0xFFF) << 16) |
			(*(recordTbl + (i * ARRAY_ROW_SIZE)) & 0xFFFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i, recordRef[i]);
		/* LL [31:16] = postdiv, [15:0] = clkdiv */
		recordRef[i + NR_FREQ] =
			((*(recordTbl + (i * ARRAY_ROW_SIZE) + 3) & 0xFF) << 16) |
			(*(recordTbl + (i * ARRAY_ROW_SIZE) + 2) & 0xFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + NR_FREQ, recordRef[i + NR_FREQ]);
		/* L [31:16] = Vproc, [15:0] = Freq */
		recordRef[i + 36] =
			((*(recordTbl + ((NR_FREQ * 1) + i) * ARRAY_ROW_SIZE + 1) & 0xFFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 1) + i) * ARRAY_ROW_SIZE) & 0xFFFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 36, recordRef[i + 36]);
		/* L [31:16] = postdiv, [15:0] = clkdiv */
		recordRef[i + 36 + NR_FREQ] =
			((*(recordTbl + ((NR_FREQ * 1) + i) * ARRAY_ROW_SIZE + 3) & 0xFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 1) + i) * ARRAY_ROW_SIZE + 2) & 0xFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 36 + NR_FREQ, recordRef[i + 36 + NR_FREQ]);
		/* CCI [31:16] = Vproc, [15:0] = Freq */
		recordRef[i + 72] =
			((*(recordTbl + ((NR_FREQ * 2) + i) * ARRAY_ROW_SIZE + 1) & 0xFFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 2) + i) * ARRAY_ROW_SIZE) & 0xFFFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 72, recordRef[i + 72]);
		/* CCI [31:16] = postdiv, [15:0] = clkdiv */
		recordRef[i + 72 + NR_FREQ] =
			((*(recordTbl + ((NR_FREQ * 2) + i) * ARRAY_ROW_SIZE + 3) & 0xFFF) << 16) |
			(*(recordTbl + ((NR_FREQ * 2) + i) * ARRAY_ROW_SIZE + 2) & 0xFF);
		cpufreq_info("DVFS - recordRef[%d] = 0x%x\n", i + 72 + NR_FREQ, recordRef[i + 72 + NR_FREQ]);
		/* BIG [31:16] = Vproc, [15:0] = Freq */
		recordRef[i + 108] =
			((*(recordTbl + ((NR_FREQ * 3) + i) * ARRAY_ROW_SIZE + 1) & 0xFFF) << 16) |
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
