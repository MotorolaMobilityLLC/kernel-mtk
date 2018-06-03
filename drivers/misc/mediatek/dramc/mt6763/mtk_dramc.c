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

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>
#include <linux/memblock.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>
/* #include <mach/mtk_clkmgr.h> */
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <asm/setup.h>
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mt-plat/mtk_io.h>
#include <mt-plat/dma.h>
#include <mt-plat/sync_write.h>

#include "mtk_dramc.h"
#include "dramc.h"

#ifdef DRAM_HQA
#if defined(HQA_LPDDR4)
#include <fan53526.h>
#elif defined(HQA_LPDDR4X)
#include <fan53526.h>
#include <fan53528buc08x.h>
#endif
#endif

#ifdef CONFIG_OF_RESERVED_MEM
#define DRAM_R0_DUMMY_READ_RESERVED_KEY "reserve-memory-dram_r0_dummy_read"
#define DRAM_R1_DUMMY_READ_RESERVED_KEY "reserve-memory-dram_r1_dummy_read"
#include <linux/of_reserved_mem.h>
#include <mt-plat/mtk_memcfg.h>
#endif

#include <mt-plat/aee.h>
#include <mt-plat/mtk_chip.h>

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355) && defined(DRAM_HQA)
#include <linux/regulator/consumer.h>
#endif

void __iomem *DRAMC_AO_CHA_BASE_ADDR;
void __iomem *DRAMC_AO_CHB_BASE_ADDR;
void __iomem *DRAMC_NAO_CHA_BASE_ADDR;
void __iomem *DRAMC_NAO_CHB_BASE_ADDR;
void __iomem *DDRPHY_CHA_BASE_ADDR;
void __iomem *DDRPHY_CHB_BASE_ADDR;
#define DRAM_RSV_SIZE 0x1000

#ifdef SW_TX_TRACKING
static unsigned int mr18_cur;
static unsigned int mr19_cur;
#endif

#ifdef LAST_DRAMC
static void *(*get_emi_base)(void);
#endif

#define DRAMC_GET_EMI_WORKAROUND /* FIXME, waiting for mt_emi_base_get() porting done */

#ifdef DRAMC_GET_EMI_WORKAROUND
static void __iomem *EMI_BASE_ADDR; /* not initialise statics to 0 or NULL */
#endif

static DEFINE_MUTEX(dram_dfs_mutex);
unsigned char No_DummyRead;
unsigned int DRAM_TYPE;
unsigned int CH_NUM;
unsigned int CBT_MODE;

/*extern bool spm_vcorefs_is_dvfs_in_porgress(void);*/
#define Reg_Sync_Writel(addr, val)   writel(val, IOMEM(addr))
#define Reg_Readl(addr) readl(IOMEM(addr))
static unsigned int dram_rank_num;
phys_addr_t dram_rank0_addr, dram_rank1_addr;


struct dram_info *g_dram_info_dummy_read, *get_dram_info;
struct dram_info dram_info_dummy_read;

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355) && defined(DRAM_HQA)
struct regulator *_reg_VCORE;
struct regulator *_reg_VDRAM1;
struct regulator *_reg_VDRAM2;
#endif

#define DRAMC_RSV_TAG "[DRAMC_RSV]"
#define dramc_rsv_aee_warn(string, args...) do {\
	pr_err("[ERR]"string, ##args); \
	aee_kernel_warning(DRAMC_RSV_TAG, "[ERR]"string, ##args);  \
} while (0)

static unsigned int check_DRAM_size(void)
{
	int ret = 0;
	phys_addr_t max_dram_size = get_max_DRAM_size();

	if (max_dram_size > 0x100000000ULL)    /* dram size = 6GB or 5GB*/
		ret = 1;
	else if (max_dram_size == 0x100000000ULL)	/* dram size = 4GB*/
		ret = 2;
	return ret;
}

/* Return 0 if success, -1 if failure */
static int __init dram_dummy_read_fixup(void)
{
	int ret = 0;

	ret = acquire_buffer_from_memory_lowpower(&dram_rank1_addr);

	/* Success to acquire memory */
	if (ret == 0) {
		pr_info("%s: %pa\n", __func__, &dram_rank1_addr);
		return 0;
	}

	/* error occurs */
	pr_alert("%s: failed to acquire last 1 page(%d)\n", __func__, ret);
	return -1;
}

static int __init dt_scan_dram_info(unsigned long node,
const char *uname, int depth, void *data)
{
	char *type = (char *)of_get_flat_dt_prop(node, "device_type", NULL);
	const __be32 *reg, *endp;
	unsigned long l;

	/* We are scanning "memory" nodes only */
	if (type == NULL) {
		/*
		* The longtrail doesn't have a device_type on the
		* /memory node, so look for the node called /memory@0.
		*/
		if (depth != 1 || strcmp(uname, "memory@0") != 0)
			return 0;
	} else if (strcmp(type, "memory") != 0)
		return 0;

	reg = (const __be32 *)of_get_flat_dt_prop(node,
	(const char *)"reg", (int *)&l);
	if (reg == NULL)
		return 0;

	endp = reg + (l / sizeof(__be32));
	if (node) {
		get_dram_info =
		(struct dram_info *)of_get_flat_dt_prop(node,
		"orig_dram_info", NULL);
		if (get_dram_info == NULL) {
			No_DummyRead = 1;
			return 0;
		}

		g_dram_info_dummy_read = &dram_info_dummy_read;
		dram_info_dummy_read.rank_num = get_dram_info->rank_num;
		dram_rank_num = get_dram_info->rank_num;
		pr_err("[DRAMC] dram info dram rank number = %d\n",
		g_dram_info_dummy_read->rank_num);

		if ((dram_rank_num == SINGLE_RANK) || (check_DRAM_size() == 1)) {
			dram_info_dummy_read.rank_info[0].start = dram_rank0_addr;
			dram_info_dummy_read.rank_info[1].start = dram_rank0_addr;
			pr_err("[DRAMC] dram info dram rank0 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[0].start);
		} else if (dram_rank_num == DUAL_RANK) {
			/* No dummy read address for rank1, try to fix it up */
			if (dram_rank1_addr == 0 && dram_dummy_read_fixup() != 0) {
				No_DummyRead = 1;
				dramc_rsv_aee_warn("dram dummy read reserve fail on rank1 !!!\n");
			}

			dram_info_dummy_read.rank_info[0].start = dram_rank0_addr;
			if (check_DRAM_size() == 2)
				dram_info_dummy_read.rank_info[1].start = 0xC0000000;
			else
				dram_info_dummy_read.rank_info[1].start = dram_rank1_addr;
			pr_err("[DRAMC] dram info dram rank0 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[0].start);
			pr_err("[DRAMC] dram info dram rank1 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[1].start);
		} else {
			No_DummyRead = 1;
			pr_err("[DRAMC] dram info dram rank number incorrect !!!\n");
		}
	}

	return node;
}

void *mt_dramc_cha_base_get(void)
{
	return DRAMC_AO_CHA_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_cha_base_get);

void *mt_dramc_chb_base_get(void)
{
	return DRAMC_AO_CHB_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_chb_base_get);

void *mt_dramc_nao_cha_base_get(void)
{
	return DRAMC_NAO_CHA_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_nao_cha_base_get);

void *mt_dramc_nao_chb_base_get(void)
{
	return DRAMC_NAO_CHB_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_nao_chb_base_get);

void *mt_ddrphy_cha_base_get(void)
{
	return DDRPHY_CHA_BASE_ADDR;
}
EXPORT_SYMBOL(mt_ddrphy_cha_base_get);

void *mt_ddrphy_chb_base_get(void)
{
	return DDRPHY_CHB_BASE_ADDR;
}
EXPORT_SYMBOL(mt_ddrphy_chb_base_get);

#ifdef SW_TX_TRACKING
static tx_result read_dram_mode_reg(
unsigned int mr_index, unsigned int *mr_value,
void __iomem *dramc_ao_chx_base, void __iomem *dramc_nao_chx_base)
{
	unsigned int response;
	unsigned int time_cnt;
	unsigned int temp;

	/* assign MR index */
	temp = Reg_Readl(DRAMC_AO_MRS) & ~(0x1FFF<<8);
	Reg_Sync_Writel(DRAMC_AO_MRS, temp | (mr_index<<8));

	/* fire MRR by MRREN 0->1 */
	temp = Reg_Readl(DRAMC_AO_SPCMD);
	Reg_Sync_Writel(DRAMC_AO_SPCMD, temp | 0x2);

	/* wait MRR finish response or timeout handling */
	time_cnt = 100;
	do {
		udelay(1);
		response = Reg_Readl(DRAMC_NAO_SPCMDRESP) & 0x2;
		time_cnt--;
	} while ((response == 0) && (time_cnt > 0));
	if (time_cnt == 0)
		return TX_TIMEOUT_MRR_ENABLE;

	/* Read out MR value or timeout handling */
	time_cnt = 10;
	do {
		udelay(1);
		*mr_value = Reg_Readl(DRAMC_NAO_MRR_STATUS) & 0xFFFF;
		time_cnt--;
	} while ((*mr_value == 0) && (time_cnt > 0));
#if 0
	if (time_cnt == 0)
		pr_warn("[DRAMC] read mode reg time out 2\n");
#endif

	/* set MRR fire bit MRREN to 0 for next MRR */
	temp = Reg_Readl(DRAMC_AO_SPCMD);
	Reg_Sync_Writel(DRAMC_AO_SPCMD, temp & ~0x2);

	/* wait for the ready response */
	time_cnt = 100;
	do {
		udelay(1);
		response = Reg_Readl(DRAMC_NAO_SPCMDRESP) & 0x2;
		time_cnt--;
	} while ((response == 2) && (time_cnt > 0));
	if (time_cnt == 0)
		return TX_TIMEOUT_MRR_DISABLE;

	return TX_DONE;
}

static tx_result start_dram_dqs_osc(void __iomem *dramc_ao_chx_base, void __iomem *dramc_nao_chx_base)
{
	unsigned int response;
	unsigned int time_cnt;
	unsigned int temp;

	temp = Reg_Readl(DRAMC_AO_SPCMD) | (0x1<<10);
	Reg_Sync_Writel(DRAMC_AO_SPCMD, temp);

	time_cnt = 100;
	do {
		udelay(1);
		response = Reg_Readl(DRAMC_NAO_SPCMDRESP) & (0x1<<10);
		time_cnt--;
	} while ((response == 0) && (time_cnt > 0));

	if (time_cnt == 0)
		return TX_TIMEOUT_DQSOSC;

	temp = Reg_Readl(DRAMC_AO_SPCMD) & ~(0x1<<10);
	Reg_Sync_Writel(DRAMC_AO_SPCMD, temp);

	return TX_DONE;
}

static tx_result auto_dram_dqs_osc(unsigned int rank,
void __iomem *dramc_ao_chx_base, void __iomem *dramc_nao_chx_base)
{
	unsigned int backup_mrs, backup_pd_ctrl, backup_ckectrl;
	unsigned int temp;
	tx_result res;

	backup_mrs = Reg_Readl(DRAMC_AO_MRS);
	backup_pd_ctrl = Reg_Readl(DRAMC_AO_PD_CTRL);
	backup_ckectrl = Reg_Readl(DRAMC_AO_CKECTRL);

	/* disable DQS OSC 2 ranks simutaneously and specify rank index */
	temp = Reg_Readl(DRAMC_AO_RKCFG) & ~(0x1<<11);
	Reg_Sync_Writel(DRAMC_AO_RKCFG, temp);
	temp = Reg_Readl(DRAMC_AO_MRS) & ~(0x3<<28);
	Reg_Sync_Writel(DRAMC_AO_MRS, temp | (rank<<28) | (0x80000000));

	/* switch DQS OSC control to SW mode */
	temp = Reg_Readl(DRAMC_AO_DQSOSCR);
	Reg_Sync_Writel(DRAMC_AO_DQSOSCR, temp | (0x1<<28));
	temp = Reg_Readl(DRAMC_AO_SLP4_TESTMODE);
	Reg_Sync_Writel(DRAMC_AO_SLP4_TESTMODE,  temp | (0x1<<28));

	/* set DRAMC clock free run and CKE always on */
	temp = Reg_Readl(DRAMC_AO_PD_CTRL);
	Reg_Sync_Writel(DRAMC_AO_PD_CTRL, temp | (0x1<<26));
	if (rank == 0) {
		temp = Reg_Readl(DRAMC_AO_CKECTRL) & ~(0x1<<7);
		Reg_Sync_Writel(DRAMC_AO_CKECTRL, temp | (0x1<<6));
	} else {
		Reg_Sync_Writel(DRAMC_AO_CKECTRL, temp & ~(0x1<<5));
		Reg_Sync_Writel(DRAMC_AO_CKECTRL, temp | (0x1<<4));
	}

	res = start_dram_dqs_osc(dramc_ao_chx_base, dramc_nao_chx_base);
	if (res != TX_DONE)
		return res;
	udelay(1);
	temp = Reg_Readl(DRAMC_AO_MRS) & ~(0x3<<26);
	Reg_Sync_Writel(DRAMC_AO_MRS, temp | (rank<<26));
	res = read_dram_mode_reg(18, &mr18_cur, dramc_ao_chx_base, dramc_nao_chx_base);
	if (res != TX_DONE)
		return res;
	res = read_dram_mode_reg(19, &mr19_cur, dramc_ao_chx_base, dramc_nao_chx_base);
	if (res != TX_DONE)
		return res;

#if 0 /* print message for debugging */
	/* byte 0 */
	dqs_cnt = (mr18_cur & 0xFF) | ((mr19_cur & 0xFF) << 8);
	if (dqs_cnt != 0)
		dqs_osc[0] = mr23_value*16000000/(dqs_cnt * frequency); /* sagy: our frequency is double data rate */
	else
		dqs_osc[0] = 0;
	/* byte 1 */
	dqs_cnt = (mr18_cur >> 8) | (mr19_cur & 0xFF00);
	if (dqs_cnt != 0)
		dqs_osc[1] = mr23_value*16000000/(dqs_cnt * frequency); /* sagy: our frequency is double data rate */
	else
		dqs_osc[1] = 0;

	pr_err("[DRAMC] Rank %d, (LSB)MR18= 0x%x, (MSB)MR19= 0x%x, tDQSOscB0 = %d ps tDQSOscB1 = %d ps\n",
		rank, mr18_cur, mr19_cur, dqs_osc[0], dqs_osc[1]);
#endif

	Reg_Sync_Writel(DRAMC_AO_MRS, backup_mrs);
	Reg_Sync_Writel(DRAMC_AO_PD_CTRL, backup_pd_ctrl);
	Reg_Sync_Writel(DRAMC_AO_CKECTRL, backup_ckectrl);

	return TX_DONE;
}

static tx_result dramc_tx_tracking(int channel)
{
	void __iomem *dramc_ao_chx_base;
	void __iomem *dramc_nao_chx_base;
	void __iomem *ddrphy_chx_base;

	unsigned int shu_level;
	unsigned int shu_index;
	unsigned int shu_offset_dramc, shu_offset_ddrphy;
	unsigned int dqsosc_inc, dqsosc_dec;
	unsigned int pi_orig[3][2][2]; /* [shuffle][rank][byte] */
	unsigned int pi_new[3][2][2]; /* [shuffle][rank][byte] */
	unsigned int pi_adjust;
	unsigned int mr1819_base[2][2];
	unsigned int mr1819_cur[2];
	unsigned int mr1819_delta;
	unsigned int mr4_on_off;
	unsigned int response;
	unsigned int time_cnt;
	unsigned int temp;
	unsigned int rank, byte;
	unsigned int tx_freq_ratio[3];
	unsigned int pi_adj, max_pi_adj[3];
	tx_result res;

	if (channel == 0) {
		dramc_ao_chx_base = DRAMC_AO_CHA_BASE_ADDR;
		dramc_nao_chx_base = DRAMC_NAO_CHA_BASE_ADDR;
		ddrphy_chx_base = DDRPHY_CHA_BASE_ADDR;
	} else {
		dramc_ao_chx_base = DRAMC_AO_CHB_BASE_ADDR;
		dramc_nao_chx_base = DRAMC_NAO_CHB_BASE_ADDR;
		ddrphy_chx_base = DDRPHY_CHB_BASE_ADDR;
	}

	shu_level = (Reg_Readl(DRAMC_AO_SHUSTATUS) >> 1) & 0x3;
	if (shu_level == 0) {
		tx_freq_ratio[0] = 0x8;
		tx_freq_ratio[1] = 0x7;
		tx_freq_ratio[2] = 0x4;
	} else if (shu_level == 1) {
		tx_freq_ratio[0] = 0xa;
		tx_freq_ratio[1] = 0x8;
		tx_freq_ratio[2] = 0x5;
	} else {
		tx_freq_ratio[0] = 0x10;
		tx_freq_ratio[1] = 0xd;
		tx_freq_ratio[2] = 0x8;
	}
	max_pi_adj[0] = 10;
	max_pi_adj[1] = 8;
	max_pi_adj[2] = 4;

	shu_offset_dramc = 0x600 * shu_level;
	dqsosc_inc = (Reg_Readl(DRAMC_AO_DQSOSC_PRD + shu_offset_dramc) >> 16) & 0xFF;
	dqsosc_dec = (Reg_Readl(DRAMC_AO_DQSOSC_PRD + shu_offset_dramc) >> 24) & 0xFF;

	/* mr1819_base[rank][byte] */
	mr1819_base[0][0] = (Reg_Readl(DRAMC_AO_SHU1RK0_DQSOSC + shu_offset_dramc) >>  0) & 0xFFFF;
	mr1819_base[1][0] = (Reg_Readl(DRAMC_AO_SHU1RK1_DQSOSC + shu_offset_dramc) >>  0) & 0xFFFF;
	if (CBT_MODE == BYTE_MODE) {
		mr1819_base[0][1] = (Reg_Readl(DRAMC_AO_SHU1RK0_DQSOSC + shu_offset_dramc) >> 16) & 0xFFFF;
		mr1819_base[1][1] = (Reg_Readl(DRAMC_AO_SHU1RK1_DQSOSC + shu_offset_dramc) >> 16) & 0xFFFF;
	} else { /* normal mode */
		mr1819_base[0][1] = mr1819_base[0][0];
		mr1819_base[1][1] = mr1819_base[1][0];
	}

	/* pi_orig[shuffle][rank][byte] */
	for (shu_index = 0; shu_index < 3; shu_index++) {
		shu_offset_dramc = 0x600 * shu_index;
		pi_orig[shu_index][0][0] = (Reg_Readl(DRAMC_AO_SHU1RK0_PI + shu_offset_dramc) >> 8) & 0x3F;
		pi_orig[shu_index][0][1] = (Reg_Readl(DRAMC_AO_SHU1RK0_PI + shu_offset_dramc) >> 0) & 0x3F;
		pi_orig[shu_index][1][0] = (Reg_Readl(DRAMC_AO_SHU1RK1_PI + shu_offset_dramc) >> 8) & 0x3F;
		pi_orig[shu_index][1][1] = (Reg_Readl(DRAMC_AO_SHU1RK1_PI + shu_offset_dramc) >> 0) & 0x3F;
	}

	temp = Reg_Readl(DRAMC_AO_SPCMDCTRL);
	mr4_on_off = (temp >> 29) & 0x1;
	Reg_Sync_Writel(DRAMC_AO_SPCMDCTRL, temp | (1<<29));
	for (rank = 0; rank < 2; rank++) {
		res = auto_dram_dqs_osc(rank, dramc_ao_chx_base, dramc_nao_chx_base);
		if (res != TX_DONE)
			return res;
		mr1819_cur[0] = (mr18_cur & 0xFF) | ((mr19_cur & 0xFF) << 8);
		if (CBT_MODE == BYTE_MODE)
			mr1819_cur[1] = (mr18_cur >> 8) | (mr19_cur & 0xFF00);
		else /* Normal Mode */
			mr1819_cur[1] = mr1819_cur[0];

		/* inc: mr1819_cur > mr1819_base, PI- */
		/* dec: mr1819_cur < mr1819_base, PI+ */
		for (byte = 0; byte < 2; byte++) {
			if (mr1819_cur[byte] >= mr1819_base[rank][byte]) {
				mr1819_delta = mr1819_cur[byte] - mr1819_base[rank][byte];
				pi_adjust = mr1819_delta / dqsosc_inc;
				for (shu_index = 0; shu_index < 3; shu_index++) {
					pi_adj = pi_adjust * tx_freq_ratio[shu_index] / tx_freq_ratio[shu_level];
					if (pi_adj > max_pi_adj[shu_index])
						return TX_FAIL_VARIATION;
					pi_new[shu_index][rank][byte] =
						(pi_orig[shu_index][rank][byte] - pi_adj) & 0x3F;
#if 0 /* print message for debugging */
pr_err("[DRAMC], CH%d RK%d B%d, shu=%d base=%X cur=%X delta=%d INC=%d PI=0x%x Adj=%d newPI=0x%x\n",
channel, rank, byte, shu_index, mr1819_base[rank][byte], mr1819_cur[byte],
mr1819_delta, dqsosc_inc, pi_orig[shu_index][rank][byte],
(pi_adjust * tx_freq_ratio[shu_index] / tx_freq_ratio[shu_level]),
pi_new[shu_index][rank][byte]);
#endif
				}
			} else {
				mr1819_delta = mr1819_base[rank][byte] - mr1819_cur[byte];
				pi_adjust = mr1819_delta / dqsosc_dec;
				for (shu_index = 0; shu_index < 3; shu_index++) {
					pi_adj = pi_adjust * tx_freq_ratio[shu_index] / tx_freq_ratio[shu_level];
					if (pi_adj > max_pi_adj[shu_index])
						return TX_FAIL_VARIATION;
					pi_new[shu_index][rank][byte] =
						(pi_orig[shu_index][rank][byte] + pi_adj) & 0x3F;
#if 0 /* print message for debugging */
pr_err("[DRAMC], CH%d RK%d B%d, shu=%d base=%X cur=%X delta=%d DEC=%d PI=0x%x Adj=%d newPI=0x%x\n",
channel, rank, byte, shu_index, mr1819_base[rank][byte], mr1819_cur[byte],
mr1819_delta, dqsosc_dec, pi_orig[shu_index][rank][byte],
(pi_adjust * tx_freq_ratio[shu_index] / tx_freq_ratio[shu_level]),
pi_new[shu_index][rank][byte]);
#endif
				}
			}
		}
	}

	temp = Reg_Readl(DRAMC_AO_DQSOSCR);
	Reg_Sync_Writel(DRAMC_AO_DQSOSCR, temp | (0x1<<5));
	Reg_Sync_Writel(DRAMC_AO_DQSOSCR, temp | (0x3<<5));

	for (shu_index = 0; shu_index < 3; shu_index++) {
		shu_offset_ddrphy = 0x500 * shu_index;
		temp = Reg_Readl(DDRPHY_SHU1_R0_B0_DQ7 + shu_offset_ddrphy) & ~((0x3F << 8) | (0x3F << 16));
		Reg_Sync_Writel(DDRPHY_SHU1_R0_B0_DQ7 + shu_offset_ddrphy, temp | (pi_new[shu_index][0][0] << 16)
										| (pi_new[shu_index][0][0] << 8));
		temp = Reg_Readl(DDRPHY_SHU1_R0_B1_DQ7 + shu_offset_ddrphy) & ~((0x3F << 8) | (0x3F << 16));
		Reg_Sync_Writel(DDRPHY_SHU1_R0_B1_DQ7 + shu_offset_ddrphy, temp | (pi_new[shu_index][0][1] << 16)
										| (pi_new[shu_index][0][1] << 8));
		temp = Reg_Readl(DDRPHY_SHU1_R1_B0_DQ7 + shu_offset_ddrphy) & ~((0x3F << 8) | (0x3F << 16));
		Reg_Sync_Writel(DDRPHY_SHU1_R1_B0_DQ7 + shu_offset_ddrphy, temp | (pi_new[shu_index][1][0] << 16)
										| (pi_new[shu_index][1][0] << 8));
		temp = Reg_Readl(DDRPHY_SHU1_R1_B1_DQ7 + shu_offset_ddrphy) & ~((0x3F << 8) | (0x3F << 16));
		Reg_Sync_Writel(DDRPHY_SHU1_R1_B1_DQ7 + shu_offset_ddrphy, temp | (pi_new[shu_index][1][1] << 16)
										| (pi_new[shu_index][1][1] << 8));
	}

	time_cnt = 100;
	do {
		udelay(1);
		response = Reg_Readl(DRAMC_NAO_MISC_STATUSA) & (1 << 29);
		time_cnt--;
	} while ((response == 0) && (time_cnt > 0));
	if (time_cnt == 0) {
		pr_err("[DRAMC] write DDRPHY time out\n");
		return TX_TIMEOUT_DDRPHY;
	}

	temp = Reg_Readl(DRAMC_AO_DQSOSCR);
	Reg_Sync_Writel(DRAMC_AO_DQSOSCR, temp & ~(0x1<<5));
	Reg_Sync_Writel(DRAMC_AO_DQSOSCR, temp & ~(0x3<<5));

	temp = Reg_Readl(DRAMC_AO_SPCMDCTRL) & ~(1<<29);
	Reg_Sync_Writel(DRAMC_AO_SPCMDCTRL, temp | (mr4_on_off<<29));

	return TX_DONE;
}

void dump_tx_log(tx_result res)
{
	switch (res) {
	case TX_TIMEOUT_MRR_ENABLE:
		pr_err("[DRAMC] TX MRR enable timeout\n");
		break;
	case TX_TIMEOUT_MRR_DISABLE:
		pr_err("[DRAMC] TX MRR disable timeout\n");
		break;
	case TX_TIMEOUT_DQSOSC:
		pr_err("[DRAMC] TX DQS OSC timeout\n");
		break;
	case TX_TIMEOUT_DDRPHY:
		pr_err("[DRAMC] TX DDRPHY update timeout\n");
		break;
	case TX_FAIL_DATA_RATE:
		pr_err("[DRAMC] TX read data rate fail\n");
		break;
	case TX_FAIL_VARIATION:
		pr_err("[DRAMC] TX variation is too large\n");
		break;
	default:
		pr_err("[DRAMC] TX unknown error\n");
		break;
	}
}
#endif

#ifdef LAST_DRAMC
static int __init set_single_channel_test_angent(int channel)
{
	void __iomem *dramc_ao_base;
	void __iomem *emi_base;
	unsigned int bit_scramble, bit_xor, bit_shift;
	unsigned int emi_cona, emi_conf;
	unsigned int channel_position;
	unsigned int temp;
	phys_addr_t test_agent_base = dram_rank0_addr;

	emi_base = get_emi_base();
	if (emi_base == NULL) {
		pr_err("[LastDRAMC] can't find EMI base\n");
		return -1;
	}
	emi_cona = readl(IOMEM(emi_base+0x000));
	emi_conf = readl(IOMEM(emi_base+0x028))>>8;

	channel_position = (emi_cona>>2)&0x3;
	if (channel_position == 0x3)
		channel_position = 12;
	else
		channel_position += 7;

	if (channel == 0)
		dramc_ao_base = DRAMC_AO_CHA_BASE_ADDR;
	else
		dramc_ao_base = DRAMC_AO_CHB_BASE_ADDR;

	/* calculate DRAM base address (test_agent_base) */
	/* pr_err("[LastDRAMC] reserved address before emi: %llx\n", test_agent_base); */
	for (bit_scramble = 11; bit_scramble < 17; bit_scramble++) {
		bit_xor = (emi_conf >> (4*(bit_scramble-11))) & 0xf;
		bit_xor &= test_agent_base >> 16;
		for (bit_shift = 0; bit_shift < 4; bit_shift++)
			test_agent_base ^= ((bit_xor>>bit_shift)&0x1) << bit_scramble;
	}
	test_agent_base -= 0x40000000;
	/* pr_err("[LastDRAMC] reserved address after emi: %llx\n", test_agent_base); */
	if ((emi_cona&0x300) != 0) {
		/* pr_err("[LastDRAMC] two channels\n"); */
		temp = (test_agent_base & (0x1ffffffff << (channel_position+1))) >> 1;
		test_agent_base = temp | (test_agent_base & (0x1ffffffff >> (33-channel_position)));
	}
	/* pr_err("[LastDRAMC] reserved address after emi: %llx\n", test_agent_base); */

	/* set base address for test agent */
	temp = Reg_Readl(dramc_ao_base+0x94) & 0xF;
	if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X))
		temp |= (test_agent_base>>1) & 0xFFFFFFF0;
	else if (DRAM_TYPE == TYPE_LPDDR3)
		temp |= (test_agent_base) & 0xFFFFFFF0;
	else {
		pr_err("[LastDRAMC] undefined DRAM type\n");
		return -1;
	}
	Reg_Sync_Writel(dramc_ao_base+0x94, temp);

	/* write test pattern */

	return 0;
}

static int __init last_dramc_test_agent_init(void)
{
	void __iomem *emi_base;
	unsigned int emi_cona;

	get_emi_base = (void *)symbol_get(mt_emi_base_get);
	if (get_emi_base == NULL) {
		pr_err("[LastDRAMC] mt_emi_base_get is NULL\n");
		return 0;
	}

	emi_base = get_emi_base();
	if (emi_base == NULL) {
		pr_err("[LastDRAMC] can't find EMI base\n");
		return 0;
	}
	emi_cona = readl(IOMEM(emi_base+0x000));

	set_single_channel_test_angent(0);
	if ((emi_cona&0x300) != 0)
		set_single_channel_test_angent(1);

	symbol_put(mt_emi_base_get);
	get_emi_base = NULL;

	return 0;
}

late_initcall(last_dramc_test_agent_init);
#endif

#ifdef CONFIG_MTK_DRAMC_PASR
#define __ETT__ 0
int enter_pasr_dpd_config(unsigned char segment_rank0,
			   unsigned char segment_rank1)
{
	unsigned int rank_pasr_segment[2];
	unsigned int iRankIdx = 0, iChannelIdx = 0, cnt = 1000;
	unsigned int u4value_64 = 0;
	unsigned int u4value_38 = 0;
	void __iomem *u4rg_64; /* MR4 ZQCS */
	void __iomem *u4rg_38; /* MIOCKCTRLOFF */
	void __iomem *u4rg_5C; /* MRS */
	void __iomem *u4rg_60; /* MRWEN */
	void __iomem *u4rg_88; /* MRW_RESPONSE */
#if !__ETT__
	unsigned long save_flags;

	pr_warn("[DRAMC0] PASR r0 = 0x%x  r1 = 0x%x\n", (segment_rank0 & 0xFF), (segment_rank1 & 0xFF));
	local_irq_save(save_flags);
	if (acquire_dram_ctrl() != 0) {
		pr_warn("[DRAMC0] can NOT get SPM HW SEMAPHORE!\n");
		local_irq_restore(save_flags);
		return -1;
	}
	/* pr_warn("[DRAMC0] get SPM HW SEMAPHORE!\n"); */
#endif
	rank_pasr_segment[0] = segment_rank0 & 0xFF; /* for rank0 */
	rank_pasr_segment[1] = segment_rank1 & 0xFF; /* for rank1 */
	/* pr_warn("[DRAMC0] PASR r0 = 0x%x  r1 = 0x%x\n", rank_pasr_segment[0], rank_pasr_segment[1]); */

/* #if PASR_TEST_SCENARIO == PASR_SUPPORT_2_CHANNEL*/
	for (iChannelIdx = 0; iChannelIdx < 2; iChannelIdx++) {
		if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X)) {
			if (iChannelIdx == 0) { /*Channel-A*/
				u4rg_64 = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x64);
				u4rg_38 = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x38);
				u4rg_5C = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x5C);
				u4rg_60 = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x60);
				u4rg_88 = IOMEM(DRAMC_NAO_CHA_BASE_ADDR + 0x88);
			} else { /*Channel-B*/
				u4rg_64 = IOMEM(DRAMC_AO_CHB_BASE_ADDR + 0x64);
				u4rg_38 = IOMEM(DRAMC_AO_CHB_BASE_ADDR + 0x38);
				u4rg_5C = IOMEM(DRAMC_AO_CHB_BASE_ADDR + 0x5C);
				u4rg_60 = IOMEM(DRAMC_AO_CHB_BASE_ADDR + 0x60);
				u4rg_88 = IOMEM(DRAMC_NAO_CHB_BASE_ADDR + 0x88);
			}
		} else if (DRAM_TYPE == TYPE_LPDDR3) {/* #else PASR_TEST_SCENARIO == PASR_SUPPORT_1_CHANNEL LPDDR3 */
			if (iChannelIdx == 1)
				break;

			u4rg_64 = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x64);
			u4rg_38 = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x38);
			u4rg_5C = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x5C);
			u4rg_60 = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x60);
			u4rg_88 = IOMEM(DRAMC_NAO_CHA_BASE_ADDR + 0x88);
		} else {
		break; }

		u4value_64 = readl(u4rg_64);
		u4value_38 = readl(u4rg_38);

		/* Disable MR4 => 0x64[29] = 1 */
		writel(readl(u4rg_64) | 0x20000000, u4rg_64);
		/* Disable ZQCS => LPDDR4: 0x64[30] = 0 LPDDR3: 0x64[31] = 0 */
		writel(readl(u4rg_64) & 0x3FFFFFFF, u4rg_64);
#if !__ETT__
		mb(); /* flush memory */
#endif
		udelay(2);
		writel(readl(u4rg_38) | 0x04000000, u4rg_38); /* MIOCKCTRLOFF = 1 */
		writel(readl(u4rg_38) & 0xFFFFFFFD, u4rg_38); /* DCMEN2 = 0 */
		writel(readl(u4rg_38) & 0xBFFFFFFF, u4rg_38); /* PHYCLKDYNGEN = 0 */

		for (iRankIdx = 0; iRankIdx < 2; iRankIdx++) {
			writel(((iRankIdx << 24) | rank_pasr_segment[iRankIdx] | (0x00000011 << 8)), u4rg_5C);
			writel(readl(u4rg_60) | 0x00000001, u4rg_60);
			cnt = 1000;
			do {
				if (cnt-- == 0) {
					if (iRankIdx == 0)
						pr_warn("[DRAMC0] CHA PASR MRW fail!\n");
					else
						pr_warn("[DRAMC0] CHB PASR MRW fail!\n");
#if !__ETT__
					if (release_dram_ctrl() != 0)
						pr_warn("[DRAMC0] release SPM HW SEMAPHORE fail!\n");
					/* pr_warn("[DRAMC0] release SPM HW SEMAPHORE success!\n"); */
					local_irq_restore(save_flags);
#endif
					return -1;
				}
				udelay(1);
			} while ((readl(u4rg_88) & 0x00000001) == 0x0);
			writel(readl(u4rg_60) & 0xfffffffe, u4rg_60);
		}
		writel(u4value_64, u4rg_64);
		writel(u4value_38, u4rg_38);
		writel(0, u4rg_5C);
}
#if !__ETT__
	if (release_dram_ctrl() != 0)
		pr_warn("[DRAMC0] release SPM HW SEMAPHORE fail!\n");
	/* pr_warn("[DRAMC0] release SPM HW SEMAPHORE success!\n"); */
	local_irq_restore(save_flags);
#endif
	return 0;
}

int exit_pasr_dpd_config(void)
{
	int ret;

	ret = enter_pasr_dpd_config(0, 0);

	return ret;
}
#else
int enter_pasr_dpd_config(unsigned char segment_rank0, unsigned char segment_rank1)
{
	return 0;
}

int exit_pasr_dpd_config(void)
{
	return 0;
}
#endif

#define MEM_TEST_SIZE 0x2000
#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5
int Binning_DRAM_complex_mem_test(void)
{
	unsigned char *MEM8_BASE;
	unsigned short *MEM16_BASE;
	unsigned int *MEM32_BASE;
	unsigned int *MEM_BASE;
	unsigned long mem_ptr;
	unsigned char pattern8;
	unsigned short pattern16;
	unsigned int i, j, size, pattern32;
	unsigned int value;
	unsigned int len = MEM_TEST_SIZE;
	void *ptr;
	int ret = 1;

	ptr = vmalloc(MEM_TEST_SIZE);

	if (!ptr) {
		/*pr_err("fail to vmalloc\n");*/
		/*ASSERT(0);*/
		ret = -24;
		goto fail;
	}

	MEM8_BASE = (unsigned char *)ptr;
	MEM16_BASE = (unsigned short *)ptr;
	MEM32_BASE = (unsigned int *)ptr;
	MEM_BASE = (unsigned int *)ptr;
	/*pr_warn("Test DRAM start address 0x%lx\n", (unsigned long)ptr);*/
	pr_warn("Test DRAM start address %p\n", ptr);
	pr_warn("Test DRAM SIZE 0x%x\n", MEM_TEST_SIZE);
	size = len >> 2;

	/* === Verify the tied bits (tied high) === */
	for (i = 0; i < size; i++)
		MEM32_BASE[i] = 0;

	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0) {
			vfree(ptr);
			/* return -1; */
			ret = -1;
			goto fail;
		} else
			MEM32_BASE[i] = 0xffffffff;
	}

	/* === Verify the tied bits (tied low) === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffffffff) {
			vfree(ptr);
			/* return -2; */
			ret = -2;
			goto fail;
		} else
			MEM32_BASE[i] = 0x00;
	}

	/* === Verify pattern 1 (0x00~0xff) === */
	pattern8 = 0x00;
	for (i = 0; i < len; i++)
		MEM8_BASE[i] = pattern8++;
	pattern8 = 0x00;
	for (i = 0; i < len; i++) {
		if (MEM8_BASE[i] != pattern8++) {
			vfree(ptr);
			/* return -3; */
			ret = -3;
			goto fail;
		}
	}

	/* === Verify pattern 2 (0x00~0xff) === */
	pattern8 = 0x00;
	for (i = j = 0; i < len; i += 2, j++) {
		if (MEM8_BASE[i] == pattern8)
			MEM16_BASE[j] = pattern8;
		if (MEM16_BASE[j] != pattern8) {
			vfree(ptr);
			/* return -4; */
			ret = -4;
			goto fail;
		}
		pattern8 += 2;
	}

	/* === Verify pattern 3 (0x00~0xffff) === */
	pattern16 = 0x00;
	for (i = 0; i < (len >> 1); i++)
		MEM16_BASE[i] = pattern16++;
	pattern16 = 0x00;
	for (i = 0; i < (len >> 1); i++) {
		if (MEM16_BASE[i] != pattern16++) {
			vfree(ptr);
			/* return -5; */
			ret = -5;
			goto fail;
		}
	}

	/* === Verify pattern 4 (0x00~0xffffffff) === */
	pattern32 = 0x00;
	for (i = 0; i < (len >> 2); i++)
		MEM32_BASE[i] = pattern32++;
	pattern32 = 0x00;
	for (i = 0; i < (len >> 2); i++) {
		if (MEM32_BASE[i] != pattern32++) {
			vfree(ptr);
			/* return -6; */
			ret = -6;
			goto fail;
		}
	}

	/* === Pattern 5: Filling memory range with 0x44332211 === */
	for (i = 0; i < size; i++)
		MEM32_BASE[i] = 0x44332211;

	/* === Read Check then Fill Memory with a5a5a5a5 Pattern === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0x44332211) {
			vfree(ptr);
			/* return -7; */
			ret = -7;
			goto fail;
		} else {
			MEM32_BASE[i] = 0xa5a5a5a5;
		}
	}

	/* === Read Check then Fill Memory with */
	/* 00 Byte Pattern at offset 0h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a5a5) {
			vfree(ptr);
			/* return -8; */
			ret = -8;
			goto fail;
		} else {
			MEM8_BASE[i * 4] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with */
	/* 00 Byte Pattern at offset 2h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a500) {
			vfree(ptr);
			/* return -9; */
			ret = -9;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 2] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with */
	/* 00 Byte Pattern at offset 1h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa500a500) {
			vfree(ptr);
			/* return -10; */
			ret = -10;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 1] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with */
	/* 00 Byte Pattern at offset 3h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5000000) {
			vfree(ptr);
			/* return -11; */
			ret = -11;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 3] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with ffff */
	/* Word Pattern at offset 1h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0x00000000) {
			vfree(ptr);
			/* return -12; */
			ret = -12;
			goto fail;
		} else {
			MEM16_BASE[i * 2 + 1] = 0xffff;
		}
	}

	/* === Read Check then Fill Memory with ffff */
	/* Word Pattern at offset 0h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffff0000) {
			vfree(ptr);
			/* return -13; */
			ret = -13;
			goto fail;
		} else {
			MEM16_BASE[i * 2] = 0xffff;
		}
	}
    /*===  Read Check === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffffffff) {
			vfree(ptr);
			/* return -14; */
			ret = -14;
			goto fail;
		}
	}

    /************************************************
    * Additional verification
    ************************************************/
	/* === stage 1 => write 0 === */

	for (i = 0; i < size; i++)
		MEM_BASE[i] = PATTERN1;

	/* === stage 2 => read 0, write 0xF === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];

		if (value != PATTERN1) {
			vfree(ptr);
			/* return -15; */
			ret = -15;
			goto fail;
		}
		MEM_BASE[i] = PATTERN2;
	}

	/* === stage 3 => read 0xF, write 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN2) {
			vfree(ptr);
			/* return -16; */
			ret = -16;
			goto fail;
		}
		MEM_BASE[i] = PATTERN1;
	}

	/* === stage 4 => read 0, write 0xF === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN1) {
			vfree(ptr);
			/* return -17; */
			ret = -17;
			goto fail;
		}
		MEM_BASE[i] = PATTERN2;
	}

	/* === stage 5 => read 0xF, write 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN2) {
			vfree(ptr);
			/* return -18; */
			ret = -18;
			goto fail;
		}
		MEM_BASE[i] = PATTERN1;
	}

	/* === stage 6 => read 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN1) {
			vfree(ptr);
			/* return -19; */
			ret = -19;
			goto fail;
		}
	}

	/* === 1/2/4-byte combination test === */
	mem_ptr = (unsigned long)MEM_BASE;
	while (mem_ptr < ((unsigned long)MEM_BASE + (size << 2))) {
		*((unsigned char *)mem_ptr) = 0x78;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x56;
		mem_ptr += 1;
		*((unsigned short *)mem_ptr) = 0x1234;
		mem_ptr += 2;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned short *)mem_ptr) = 0x5678;
		mem_ptr += 2;
		*((unsigned char *)mem_ptr) = 0x34;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x12;
		mem_ptr += 1;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned char *)mem_ptr) = 0x78;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x56;
		mem_ptr += 1;
		*((unsigned short *)mem_ptr) = 0x1234;
		mem_ptr += 2;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned short *)mem_ptr) = 0x5678;
		mem_ptr += 2;
		*((unsigned char *)mem_ptr) = 0x34;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x12;
		mem_ptr += 1;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
	}
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != 0x12345678) {
			vfree(ptr);
			/* return -20; */
			ret = -20;
			goto fail;
		}
	}

	/* === Verify pattern 1 (0x00~0xff) === */
	pattern8 = 0x00;
	MEM8_BASE[0] = pattern8;
	for (i = 0; i < size * 4; i++) {
		unsigned char waddr8, raddr8;

		waddr8 = i + 1;
		raddr8 = i;
		if (i < size * 4 - 1)
			MEM8_BASE[waddr8] = pattern8 + 1;
		if (MEM8_BASE[raddr8] != pattern8) {
			vfree(ptr);
			/* return -21; */
			ret = -21;
			goto fail;
		}
		pattern8++;
	}

	/* === Verify pattern 2 (0x00~0xffff) === */
	pattern16 = 0x00;
	MEM16_BASE[0] = pattern16;
	for (i = 0; i < size * 2; i++) {
		if (i < size * 2 - 1)
			MEM16_BASE[i + 1] = pattern16 + 1;
		if (MEM16_BASE[i] != pattern16) {
			vfree(ptr);
			/* return -22; */
			ret = -22;
			goto fail;
		}
		pattern16++;
	}
	/* === Verify pattern 3 (0x00~0xffffffff) === */
	pattern32 = 0x00;
	MEM32_BASE[0] = pattern32;
	for (i = 0; i < size; i++) {
		if (i < size - 1)
			MEM32_BASE[i + 1] = pattern32 + 1;
		if (MEM32_BASE[i] != pattern32) {
			vfree(ptr);
			/* return -23; */
			ret = -23;
			goto fail;
		}
		pattern32++;
	}
	pr_warn("complex R/W mem test pass\n");

fail:
	vfree(ptr);
	return ret;
}

unsigned int ucDram_Register_Read(unsigned int u4reg_addr)
{
	return 0; /* for mbw dummy use */
}

unsigned int lpDram_Register_Read(unsigned int Reg_base, unsigned int Offset)
{
	if ((Reg_base == DRAMC_NAO_CHA) && (Offset < 0x1000))
		return readl(IOMEM(DRAMC_NAO_CHA_BASE_ADDR + Offset));
	else if ((Reg_base == DRAMC_NAO_CHB) && (Offset < 0x1000))
		return readl(IOMEM(DRAMC_NAO_CHB_BASE_ADDR + Offset));
	else if ((Reg_base == DRAMC_AO_CHA) && (Offset < 0x1000))
		return readl(IOMEM(DRAMC_AO_CHA_BASE_ADDR + Offset));
	else if ((Reg_base == DRAMC_AO_CHB) && (Offset < 0x1000))
		return readl(IOMEM(DRAMC_AO_CHB_BASE_ADDR + Offset));
	else if ((Reg_base == PHY_AO_CHA) && (Offset < 0x1000))
		return readl(IOMEM(DDRPHY_CHA_BASE_ADDR + Offset));
	else if ((Reg_base == PHY_AO_CHB) && (Offset < 0x1000))
		return readl(IOMEM(DDRPHY_CHB_BASE_ADDR + Offset));
	else
		return 0;
}

/************************************************
* CL#46077
*************************************************/
unsigned int get_dram_data_rate(void)
{
	unsigned int u4ShuLevel, u4SDM_PCW, u4PREDIV, u4POSDIV, u4CKDIV4, u4VCOFreq, u4DataRate = 0;
	int channels;

	channels = get_emi_ch_num();
	u4ShuLevel = get_shuffle_status();

	u4SDM_PCW = readl(IOMEM(DDRPHY_CHA_BASE_ADDR + 0xd94 + 0x500 * u4ShuLevel)) >> 16;
	u4PREDIV = (readl(IOMEM(DDRPHY_CHA_BASE_ADDR + 0xda0 + 0x500 * u4ShuLevel)) & 0x000c0000) >> 18;
	u4POSDIV = readl(IOMEM(DDRPHY_CHA_BASE_ADDR + 0xda0 + 0x500 * u4ShuLevel)) & 0x00000007;
	u4CKDIV4 = (readl(IOMEM(DDRPHY_CHA_BASE_ADDR + 0xd18 + 0x500 * u4ShuLevel)) & 0x08000000) >> 27;

	u4VCOFreq = ((52>>u4PREDIV)*(u4SDM_PCW>>8))>>u4POSDIV;

	u4DataRate = u4VCOFreq>>u4CKDIV4;

	/* pr_err("[DRAMC Driver] PCW=0x%X, u4PREDIV=%d, u4POSDIV=%d, CKDIV4=%d, DataRate=%d\n", */
	/* u4SDM_PCW, u4PREDIV, u4POSDIV, u4CKDIV4, u4DataRate); */

	if (DRAM_TYPE == TYPE_LPDDR3) {
		if (u4DataRate == 1859)
			u4DataRate = 1866;
		else
			u4DataRate = 0;
	} else if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X)) {
		if (channels == 1) {
			if (u4DataRate == 3198)
				u4DataRate = 3200;
			else
				u4DataRate = 0;
		} else {
			if (u4DataRate == 3198)
				u4DataRate = 3200;
			else if (u4DataRate == 1599)
				u4DataRate = 1600;
			else
				u4DataRate = 0;
		}
	} else
		u4DataRate = 0;

	return u4DataRate;
}

unsigned int read_dram_temperature(unsigned char channel)
{
	unsigned int value = 0;

	if (channel == CHANNEL_A) {
		value =
		(readl(IOMEM(DRAMC_NAO_CHA_BASE_ADDR + 0x3b8)) >> 8) & 0x7;
	}	else if (channel == CHANNEL_B) {
		value =
		(readl(IOMEM(DRAMC_NAO_CHB_BASE_ADDR + 0x3b8)) >> 8) & 0x7;
		}

	return value;
}

unsigned int get_shuffle_status(void)
{
	return (readl(PDEF_DRAMC0_CHA_REG_0E4) & 0x6) >> 1;
	/* HPM = 0, LPM = 1, ULPM = 2; */

}

int get_ddr_type(void)
{
	return DRAM_TYPE;

}

int get_emi_ch_num(void)
{
	void __iomem *emi_base;
	unsigned int emi_cona;

	if (CH_NUM)
		return CH_NUM;

#ifdef DRAMC_GET_EMI_WORKAROUND
	emi_base = EMI_BASE_ADDR;
#else
	get_emi_base = (void *)symbol_get(mt_emi_base_get);
	if (get_emi_base == NULL) {
		pr_err("[get_emi_ch_num] mt_emi_base_get is NULL\n");
		return 0;
	}

	emi_base = get_emi_base();
	symbol_put(mt_emi_base_get);
	get_emi_base = NULL;
#endif

	emi_cona = readl(IOMEM(emi_base+0x000));

	switch ((emi_cona >> 8) & 0x3) {
	case 0:
		CH_NUM = 1;
		break;
	case 1:
		CH_NUM = 2;
		break;
	case 2:
		CH_NUM = 4;
		break;
	default:
		pr_err("[LastDRAMC] invalid channel num (emi_cona = 0x%x)\n", emi_cona);
	}
	return CH_NUM;
}

int dram_steps_freq(unsigned int step)
{
	int freq = -1;
	int channels;

	channels = get_emi_ch_num();

	switch (step) {
	case 0:
		if (DRAM_TYPE == TYPE_LPDDR3)
			/*freq = get_dram_data_rate();*/	/* DDR1800 or DDR1866 */
			freq = 1866;
		else if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X)) {
			if (channels == 1)
				freq = 3733;
			else
				freq = 3200;
		}
		break;
	case 1:
		if (DRAM_TYPE == TYPE_LPDDR3)
			freq = 1600;
		else if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X)) {
			if (channels == 1)
				freq = 3200;
			else
				freq = 3200;
		}
		break;
	case 2:
		if (DRAM_TYPE == TYPE_LPDDR3)
			freq = 1600;
		else if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X)) {
			if (channels == 1)
				freq = 3200;
			else
				freq = 2400;
		}
		break;
	case 3:
		if (DRAM_TYPE == TYPE_LPDDR3)
			freq = 1200;
		else if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X)) {
			if (channels == 1)
				freq = 2400;
			else
				freq = 1600;
		}
		break;
	default:
		return -1;
	}
	return freq;
}

int dram_can_support_fh(void)
{
/* FIXME: open it when multi-freq ready */
#if 0
	if (No_DummyRead)
		return 0;
	else
		return 1;
#else
	return 0;
#endif
}

#ifdef CONFIG_OF_RESERVED_MEM
int dram_dummy_read_reserve_mem_of_init(struct reserved_mem *rmem)
{
	phys_addr_t rptr = 0;
	unsigned int rsize = 0;

	rptr = rmem->base;
	rsize = (unsigned int)rmem->size;

	if (strstr(DRAM_R0_DUMMY_READ_RESERVED_KEY, rmem->name)) {
		if (rsize < DRAM_RSV_SIZE) {
			pr_err("[DRAMC] Can NOT reserve memory for Rank0\n");
			No_DummyRead = 1;
			return 0;
		}
		dram_rank0_addr = rptr;
		dram_rank_num++;
		pr_err("[dram_dummy_read_reserve_mem_of_init] dram_rank0_addr = %pa, size = 0x%x\n",
				&dram_rank0_addr, rsize);
	}

	if (strstr(DRAM_R1_DUMMY_READ_RESERVED_KEY, rmem->name)) {
		if (rsize < DRAM_RSV_SIZE) {
			pr_err("[DRAMC] Can NOT reserve memory for Rank1\n");
			No_DummyRead = 1;
			return 0;
		}
		dram_rank1_addr = rptr;
		dram_rank_num++;
		pr_err("[dram_dummy_read_reserve_mem_of_init] dram_rank1_addr = %pa, size = 0x%x\n",
				&dram_rank1_addr, rsize);
	}

	return 0;
}
RESERVEDMEM_OF_DECLARE(dram_reserve_r0_dummy_read_init,
DRAM_R0_DUMMY_READ_RESERVED_KEY,
			dram_dummy_read_reserve_mem_of_init);
RESERVEDMEM_OF_DECLARE(dram_reserve_r1_dummy_read_init,
DRAM_R1_DUMMY_READ_RESERVED_KEY,
			dram_dummy_read_reserve_mem_of_init);
#endif
static ssize_t complex_mem_test_show(struct device_driver *driver, char *buf)
{
	int ret;

	ret = Binning_DRAM_complex_mem_test();
	if (ret > 0)
		return snprintf(buf, PAGE_SIZE, "MEM Test all pass\n");
	else
		return snprintf(buf, PAGE_SIZE, "MEM TEST failed %d\n", ret);
}

static ssize_t complex_mem_test_store(struct device_driver *driver,
const char *buf, size_t count)
{
	/*snprintf(buf, "do nothing\n");*/
	return count;
}

static ssize_t read_dram_data_rate_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRAM data rate = %d\n",
	get_dram_data_rate());
}

static ssize_t read_dram_data_rate_store(struct device_driver *driver,
const char *buf, size_t count)
{
	return count;
}


DRIVER_ATTR(emi_clk_mem_test, 0664,
complex_mem_test_show, complex_mem_test_store);

DRIVER_ATTR(read_dram_data_rate, 0664,
read_dram_data_rate_show, read_dram_data_rate_store);

/*DRIVER_ATTR(dram_dfs, 0664, dram_dfs_show, dram_dfs_store);*/

static int dram_probe(struct platform_device *pdev)
{
	unsigned int i;
	struct resource *res;
	void __iomem *base_temp[6];
	struct device_node *node = NULL;

	pr_debug("[DRAMC] module probe.\n");

	for (i = 0; i < 6; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		base_temp[i] = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(base_temp[i])) {
			pr_err("[DRAMC] unable to map %d base\n", i);
			return -EINVAL;
		}
	}

	DRAMC_AO_CHA_BASE_ADDR = base_temp[0];
	DRAMC_AO_CHB_BASE_ADDR = base_temp[1];

	DRAMC_NAO_CHA_BASE_ADDR = base_temp[2];
	DRAMC_NAO_CHB_BASE_ADDR = base_temp[3];

	DDRPHY_CHA_BASE_ADDR = base_temp[4];
	DDRPHY_CHB_BASE_ADDR = base_temp[5];

	pr_warn("[DRAMC]get DRAMC_AO_CHA_BASE_ADDR @ %p\n", DRAMC_AO_CHA_BASE_ADDR);
	pr_warn("[DRAMC]get DRAMC_AO_CHB_BASE_ADDR @ %p\n", DRAMC_AO_CHB_BASE_ADDR);

	pr_warn("[DRAMC]get DDRPHY_CHA_BASE_ADDR @ %p\n", DDRPHY_CHA_BASE_ADDR);
	pr_warn("[DRAMC]get DDRPHY_CHB_BASE_ADDR @ %p\n", DDRPHY_CHB_BASE_ADDR);

	pr_warn("[DRAMC]get DRAMC_NAO_CHA_BASE_ADDR @ %p\n", DRAMC_NAO_CHA_BASE_ADDR);
	pr_warn("[DRAMC]get DRAMC_NAO_CHB_BASE_ADDR @ %p\n", DRAMC_NAO_CHB_BASE_ADDR);

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	if (node) {
		SLEEP_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get SLEEP_BASE_ADDR @ %p\n",
		SLEEP_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find SLEEP_BASE_ADDR compatible node\n");
		return -1;
	}

	DRAM_TYPE = (readl(PDEF_DRAMC0_CHA_REG_010) & 0x1C00) >> 10;
	pr_err("[DRAMC Driver] dram type =%d\n", get_ddr_type());

	CBT_MODE = (readl(PDEF_DRAMC0_CHA_REG_010) & 0x2000) >> 13;
	pr_err("[DRAMC Driver] cbt mode =%d\n", CBT_MODE);

	pr_err("[DRAMC Driver] Dram Data Rate = %d\n", get_dram_data_rate());
	pr_err("[DRAMC Driver] shuffle_status = %d\n", get_shuffle_status());

#if 0
	if ((DRAM_TYPE == TYPE_LPDDR4) || (DRAM_TYPE == TYPE_LPDDR4X)) {
		low_freq_counter = 10;
		setup_deferrable_timer_on_stack(&zqcs_timer, zqcs_timer_callback, 0);
		if (mod_timer(&zqcs_timer, jiffies + msecs_to_jiffies(280)))
			pr_err("[DRAMC Driver] Error in ZQCS mod_timer\n");
	}
#endif
	if (dram_can_support_fh())
		pr_err("[DRAMC Driver] dram can support DFS\n");
	else
		pr_err("[DRAMC Driver] dram can not support DFS\n");

	return 0;
}

static int dram_remove(struct platform_device *dev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dram_of_ids[] = {
	{.compatible = "mediatek,dramc",},
	{}
};
#endif

static struct platform_driver dram_test_drv = {
	.probe = dram_probe,
	.remove = dram_remove,
	.driver = {
		.name = "emi_clk_test",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = dram_of_ids,
#endif
		},
};

static struct timer_list zqcs_timer;
static unsigned char low_freq_counter;

void zqcs_timer_callback(unsigned long data)
{
	unsigned int Response, TimeCnt, CHCounter, RankCounter;
	void __iomem *u4rg_24;
	void __iomem *u4rg_38;
	void __iomem *u4rg_5C;
	void __iomem *u4rg_60;
	void __iomem *u4rg_88;
	unsigned long save_flags;
#ifdef SW_TX_TRACKING
	tx_result res[2];
#endif

	local_irq_save(save_flags);
	if (acquire_dram_ctrl() != 0) {
		pr_warn("[DRAMC] can NOT get SPM HW SEMAPHORE!\n");
		mod_timer(&zqcs_timer, jiffies + msecs_to_jiffies(280));
		local_irq_restore(save_flags);
		return;
	}

  /* CH0_Rank0 --> CH1Rank0 */
	for (RankCounter = 0; RankCounter < 2; RankCounter++) {
		for (CHCounter = 0; CHCounter < 2; CHCounter++) {
			TimeCnt = 100;

			if (CHCounter == 0) {
				u4rg_24 = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x24);
				u4rg_38 = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x38);
				u4rg_5C = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x5C);
				u4rg_60 = IOMEM(DRAMC_AO_CHA_BASE_ADDR + 0x60);
				u4rg_88 = IOMEM(DRAMC_NAO_CHA_BASE_ADDR + 0x88);
			} else if (CHCounter == 1) {
				u4rg_24 = IOMEM(DRAMC_AO_CHB_BASE_ADDR + 0x24);
				u4rg_38 = IOMEM(DRAMC_AO_CHB_BASE_ADDR + 0x38);
				u4rg_5C = IOMEM(DRAMC_AO_CHB_BASE_ADDR + 0x5C);
				u4rg_60 = IOMEM(DRAMC_AO_CHB_BASE_ADDR + 0x60);
				u4rg_88 = IOMEM(DRAMC_NAO_CHB_BASE_ADDR + 0x88);
			}
			writel(readl(u4rg_38) & 0xBFFFFFFF, u4rg_38); /* DMPHYCLKDYNGEN */
			writel(readl(u4rg_38) | 0x4000000, u4rg_38); /* DMMIOCKCTRLOFF */
			writel(readl(u4rg_24) | 0x40, u4rg_24); /* DMCKEFIXON */
			writel(readl(u4rg_24) | 0x10, u4rg_24); /* DMCKE1FIXON */

			if (RankCounter == 0)
				writel(readl(u4rg_5C) & 0xCFFFFFFF, u4rg_5C); /* Rank 0 */
			else if (RankCounter == 1) {
				writel((readl(u4rg_5C) & 0xCFFFFFFF) | 0x10000000,
				u4rg_5C); /* Rank 1 */
			}
			writel(readl(u4rg_60) | 0x10, u4rg_60); /* for ZQCal Start */

			do {
				Response = readl(u4rg_88) & 0x10;
				TimeCnt--;
				udelay(1);  /* Wait tZQCAL(min) 1us for next polling */
			} while ((Response == 0) && (TimeCnt > 0));

			writel(readl(u4rg_60) & 0xFFFFFFEF, u4rg_60); /* ZQCal Stop */

			if (TimeCnt == 0) { /* time out */
				writel(readl(u4rg_38) | 0x40000000, u4rg_38); /* DMPHYCLKDYNGEN */
				writel(readl(u4rg_24) & 0xFFFFFFBF, u4rg_24); /* DMCKEFIXON */
				writel(readl(u4rg_24) & 0xFFFFFFEF, u4rg_24); /* DMCKE1FIXON */
				writel(readl(u4rg_38) & 0xFBFFFFFF, u4rg_38); /* DMMIOCKCTRLOFF */
				if (release_dram_ctrl() != 0)
					pr_warn("[DRAMC] release SPM HW SEMAPHORE fail!\n");
				mod_timer(&zqcs_timer, jiffies + msecs_to_jiffies(280));
				local_irq_restore(save_flags);
				pr_err("CA%x Rank%x ZQCal Start time out\n", CHCounter, RankCounter);
				return;
			}

			udelay(1);

			TimeCnt = 100;
			writel(readl(u4rg_60) | 0x40, u4rg_60); /* for ZQCal latch */

			do {
				Response = readl(u4rg_88) & 0x40;
				TimeCnt--;
				udelay(1);  /* Wait tZQCAL(min) 1us for next polling */
			} while ((Response == 0) && (TimeCnt > 0));

			writel(readl(u4rg_60) & 0xFFFFFFBF, u4rg_60); /* ZQ latch Stop*/

			writel(readl(u4rg_38) | 0x40000000, u4rg_38); /* DMPHYCLKDYNGEN */
			writel(readl(u4rg_24) & 0xFFFFFFBF, u4rg_24); /* DMCKEFIXON */
			writel(readl(u4rg_24) & 0xFFFFFFEF, u4rg_24); /* DMCKE1FIXON */
			writel(readl(u4rg_38) & 0xFBFFFFFF, u4rg_38); /* DMMIOCKCTRLOFF */
			if (TimeCnt == 0) { /* time out */
				if (release_dram_ctrl() != 0)
					pr_warn("[DRAMC] release SPM HW SEMAPHORE fail!\n");
			mod_timer(&zqcs_timer, jiffies + msecs_to_jiffies(280));
			local_irq_restore(save_flags);
			pr_err("CA%x Rank%x ZQCal latch time out\n", CHCounter, RankCounter);
			return;
			}
			udelay(1);
		}
	}

#ifdef SW_TX_TRACKING
	res[0] = TX_DONE;
	res[1] = TX_DONE;
	if ((get_dram_data_rate() == 3200) || (low_freq_counter >= 10))	{
		res[0] = dramc_tx_tracking(0);
		res[1] = dramc_tx_tracking(1);
		low_freq_counter = 0;
	} else {
		low_freq_counter++;
		}
#endif

	if (release_dram_ctrl() != 0)
		pr_warn("[DRAMC] release SPM HW SEMAPHORE fail!\n");
	mod_timer(&zqcs_timer, jiffies + msecs_to_jiffies(280));
	local_irq_restore(save_flags);

#ifdef SW_TX_TRACKING
	if (res[0] != TX_DONE)
		dump_tx_log(res[0]);
	if (res[1] != TX_DONE)
		dump_tx_log(res[1]);
#endif
}

void del_zqcs_timer(void)
{
	del_timer_sync(&zqcs_timer);
}

void add_zqcs_timer(void)
{
	mod_timer(&zqcs_timer, jiffies + msecs_to_jiffies(280)); /* add_timer(&zqcs_timer); */
}

#ifdef DRAM_HQA
static unsigned int hqa_vcore;

int calculate_voltage(unsigned int x)
{
	return (600+((625*x)/100));
}

static void set_vdram(unsigned int vdram)
{
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	regulator_set_voltage(_reg_VDRAM1, vdram, MAX_VDRAM1);
#else
#if defined(HQA_LPDDR3)
	pmic_config_interface(MT6351_VDRAM_ANA_CON0, vdram, 0x71f, 0);
#elif defined(HQA_LPDDR4) || defined(HQA_LPDDR4X)
	fan53526_set_voltage((unsigned long)vdram);
#endif
#endif
}

static void set_vddq(unsigned int vddq)
{
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	regulator_set_voltage(_reg_VDRAM2, vddq, MAX_VDRAM2);
#else
#ifdef HQA_LPDDR4X
	fan53528buc08x_set_voltage((unsigned long)vddq);
#endif
#endif
}

static unsigned int get_vdram(void)
{
	unsigned int vdram;

#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	vdram = regulator_get_voltage(_reg_VDRAM1);
#else
#if defined(HQA_LPDDR3)
	pmic_read_interface(MT6351_VDRAM_ANA_CON0, &vdram, 0x71f, 0);
#elif defined(HQA_LPDDR4) || defined(HQA_LPDDR4X)
	vdram = (unsigned int)fan53526_get_voltage();
#endif
#endif
	return vdram;
}

static unsigned int get_vddq(void)
{
	unsigned int vddq = 0;

#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	vddq = regulator_get_voltage(_reg_VDRAM2);
#else
#ifdef HQA_LPDDR4X
	vddq = (unsigned int)fan53528buc08x_get_voltage();
#endif
#endif

	return vddq;
}

static void print_HQA_voltage(void)
{
#if defined(HVCORE_HVDRAM)
	pr_err("[HQA] Vcore HV, Vdram HV\n");
#elif defined(NVCORE_NVDRAM)
	pr_err("[HQA] Vcore NV, Vdram NV\n");
#elif defined(LVCORE_LVDRAM)
	pr_err("[HQA] Vcore LV, Vdram LV\n");
#elif defined(HVCORE_LVDRAM)
	pr_err("[HQA] Vcore HV, Vdram LV\n");
#elif defined(LVCORE_HVDRAM)
	pr_err("[HQA] Vcore LV, Vdram HV\n");
#else
	pr_err("[HQA] Undefined HQA condition\n");
#endif

#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	pr_err("[HQA] Vcore = %d uV (should be %d uV)\n",
		regulator_get_voltage(_reg_VCORE),
		hqa_vcore);
	pr_err("[HQA] Vdram = %d uV (should be %d uV)\n",
		get_vdram(), HQA_VDRAM);
	pr_err("[HQA] vddq = %d uV (should be %d uV)\n",
		get_vddq(), HQA_VDDQ);
#else
	pr_err("[HQA] Vcore = %d mV(should be %d mV)\n",
		calculate_voltage(upmu_get_reg_value(MT6351_BUCK_VCORE_CON4)),
		calculate_voltage(hqa_vcore));
	pr_err("[HQA] Vdram = 0x%x (should be 0x%x)\n",
		get_vdram(), HQA_VDRAM);
#ifdef HQA_LPDDR4X
	pr_err("[HQA] vddq = 0x%x (should be 0x%x)\n",
		get_vddq(), HQA_VDDQ);
#endif
#endif
}

void dram_HQA_adjust_voltage(void)
{
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	regulator_set_voltage(_reg_VCORE, hqa_vcore, MAX_VCORE);
#else
	pmic_config_interface(MT6351_BUCK_VCORE_CON4, hqa_vcore, 0x7F, 0);
	pmic_config_interface(MT6351_BUCK_VCORE_CON5, hqa_vcore, 0x7F, 0);
#endif
	set_vdram(HQA_VDRAM);
	set_vddq(HQA_VDDQ);

	print_HQA_voltage();
}

static int __init dram_hqa_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&dram_test_drv);
	if (ret) {
		pr_warn("fail to create dram_test platform driver\n");
		return ret;
	}

	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_emi_clk_mem_test);
	if (ret) {
		pr_warn("fail to create the emi_clk_mem_test sysfs files\n");
		return ret;
	}

	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_read_dram_data_rate);
	if (ret) {
		pr_warn("fail to create the read dram data rate sysfs files\n");
		return ret;
	}

	if (mt_get_chip_hw_ver() == 0xCA00) {
		pr_err("[HQA] set Vcore to HPM\n");
		hqa_vcore = HQA_VCORE_HPM;
	} else if (mt_get_chip_hw_ver() == 0xCA01) {
		pr_err("[HQA] set Vcore to LPM\n");
		hqa_vcore = HQA_VCORE_LPM;
	} else if (mt_get_chip_hw_ver() == 0xCB01) {
		pr_err("[HQA] set Vcore to HPM\n");
		hqa_vcore = HQA_VCORE_HPM;
	} else {
		pr_err("[HQA] chip ID error!\n");
		hqa_vcore = HQA_VCORE_HPM;
		/* return 0; */
		/* TODO: BUG(); */
	}
	dram_HQA_adjust_voltage();
	return 0;
}

late_initcall(dram_hqa_init);
#endif /*DRAM_HQA*/

/* int __init dram_test_init(void) */
static int __init dram_test_init(void)
{
	int ret = 0;

	DRAM_TYPE = 0;

	ret = platform_driver_register(&dram_test_drv);
	if (ret) {
		pr_warn("[DRAMC] init fail, ret 0x%x\n", ret);
		return ret;
	}

	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_emi_clk_mem_test);
	if (ret) {
		pr_warn("fail to create the emi_clk_mem_test sysfs files\n");
		return ret;
	}

	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_read_dram_data_rate);
	if (ret) {
		pr_warn("fail to create the read dram data rate sysfs files\n");
		return ret;
	}

	if (of_scan_flat_dt(dt_scan_dram_info, NULL) > 0) {
		pr_err("[DRAMC]find dt_scan_dram_info\n");
	} else {
		pr_err("[DRAMC]can't find dt_scan_dram_info\n");
		return -1;
	}

	return ret;
}

static void __exit dram_test_exit(void)
{
	platform_driver_unregister(&dram_test_drv);
}

postcore_initcall(dram_test_init);
module_exit(dram_test_exit);

#ifdef DRAMC_GET_EMI_WORKAROUND
static int __init dram_emi_init(void)
{
	struct device_node *node;

	/* DTS version */
	node = of_find_compatible_node(NULL, NULL, "mediatek,EMI");
	if (node) {
		EMI_BASE_ADDR = of_iomap(node, 0);
		pr_err("dramc get EMI_BASE_ADDR @ %p\n", EMI_BASE_ADDR);
	} else {
		pr_err("can't find compatible node\n");
		return -1;
	}

	return 0;
}

postcore_initcall(dram_emi_init);
#endif

MODULE_DESCRIPTION("MediaTek DRAMC Driver v0.1");
