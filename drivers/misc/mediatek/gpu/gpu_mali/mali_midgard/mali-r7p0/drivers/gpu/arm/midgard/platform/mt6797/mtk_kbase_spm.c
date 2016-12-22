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

#include "mtk_kbase_spm.h"

#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include <mt_gpufreq.h>
#include <mt-plat/mt_lpae.h>

#include <mtk_mali_config.h>

void mtk_kbase_dpm_setup(int *dfp_weights)
{
	int i;

	DFP_write32(DFP_CTRL, 0x2);

	DFP_write32(DFP_ID, 0x55aa3344);
	DFP_write32(DFP_VOLT_FACTOR, 1);
	DFP_write32(DFP_PM_PERIOD, 1);

	DFP_write32(DFP_COUNTER_EN_0, 0xffffffff);
	DFP_write32(DFP_COUNTER_EN_1, 0xffffffff);

	DFP_write32(DFP_SCALING_FACTOR, 16);

	for (i = 0; i < 110; ++i)
	{
		DFP_write32(DFP_WEIGHT_(i), dfp_weights[i]);
	}

	DFP_write32(DFP_CTRL, 0x3);
}

static DEFINE_MUTEX(spm_lock);

static void spm_acquire(void)
{
	mutex_lock(&spm_lock);
}
static void spm_release(void)
{
	mutex_unlock(&spm_lock);
}

void mtk_kbase_spm_acquire(void)
{
	spm_acquire();
}
void mtk_kbase_spm_release(void)
{
	spm_release();
}

static void _mtk_kbase_spm_kick_lock(void)
{
	int retry = 0;
	DVFS_CPU_write32(0x0, 0x0b160001);
	do {
		DVFS_CPU_write32(0x428, 0x1);
		if (((++retry) % 10000) == 0)
			pr_MTK_err("polling dvfs_cpu:0x428 , retry=%d\n", retry);
	} while(DVFS_CPU_read32(0x428) != 0x1);
}
static void _mtk_kbase_spm_kick_unlock(void)
{
	DVFS_CPU_write32(0x428, 0x1);
}

void mtk_kbase_spm_kick(struct pcm_desc *pd)
{
	dma_addr_t pa;
	uint32_t tmp;

	spm_acquire();

	/* setup dvfs_gpu */
	DVFS_GPU_write32(DVFS_GPU_POWERON_CONFIG_EN, SPM_PROJECT_CODE | 0x1);

	DVFS_GPU_write32(DVFS_GPU_PCM_REG_DATA_INI, 0x1);
	DVFS_GPU_write32(DVFS_GPU_PCM_PWR_IO_EN, 0x0);
	DVFS_GPU_write32(DVFS_GPU_PCM_CON0, SPM_PROJECT_CODE | CON0_PCM_CK_EN | CON0_EN_SLEEP_DVS | CON0_PCM_SW_RESET);
	DVFS_GPU_write32(DVFS_GPU_PCM_CON0, SPM_PROJECT_CODE | CON0_PCM_CK_EN | CON0_EN_SLEEP_DVS );

	/* init PCM r0 and r7 */
	tmp = DVFS_GPU_read32(DVFS_GPU_SPM_POWER_ON_VAL0);
	DVFS_GPU_write32(DVFS_GPU_PCM_REG_DATA_INI, tmp);
	DVFS_GPU_write32(DVFS_GPU_PCM_PWR_IO_EN, 0x010000);
	DVFS_GPU_write32(DVFS_GPU_PCM_PWR_IO_EN, 0x000000);

	tmp = DVFS_GPU_read32(DVFS_GPU_SPM_POWER_ON_VAL1);
	DVFS_GPU_write32(DVFS_GPU_PCM_REG_DATA_INI, tmp | 0x200);
	DVFS_GPU_write32(DVFS_GPU_PCM_PWR_IO_EN, 0x800000);
	DVFS_GPU_write32(DVFS_GPU_PCM_PWR_IO_EN, 0x000000);

	/* IM base */
	pa = __pa(pd->base);
	MAPPING_DRAM_ACCESS_ADDR(pa);
	DVFS_GPU_write32(DVFS_GPU_PCM_IM_PTR, (uint32_t)pa);
	DVFS_GPU_write32(DVFS_GPU_PCM_IM_LEN, pd->size);
	DVFS_GPU_write32(DVFS_GPU_PCM_CON1, SPM_PROJECT_CODE | CON1_PCM_TIMER_EN | CON1_FIX_SC_CK_DIS | CON1_RF_SYNC);
	DVFS_GPU_write32(DVFS_GPU_PCM_CON1, SPM_PROJECT_CODE | CON1_PCM_TIMER_EN | CON1_FIX_SC_CK_DIS | CON1_MIF_APBEN | CON1_IM_NONRP_EN);

	DVFS_GPU_write32(DVFS_GPU_PCM_PWR_IO_EN, 0x0081); /* sync register and enable IO output for r0 and r7 */

	/* Lock before fetch IM */
	_mtk_kbase_spm_kick_lock();

	/* Kick */
	DVFS_GPU_write32(DVFS_GPU_PCM_CON0, SPM_PROJECT_CODE | CON0_PCM_CK_EN | CON0_EN_SLEEP_DVS | CON0_IM_KICK_L | CON0_PCM_KICK_L);
	DVFS_GPU_write32(DVFS_GPU_PCM_CON0, SPM_PROJECT_CODE | CON0_PCM_CK_EN | CON0_EN_SLEEP_DVS);

	/* Wait IM ready */
	while ((DVFS_GPU_read32(DVFS_GPU_PCM_FSM_STA) & FSM_STA_IM_STATE_MASK) != FSM_STA_IM_STATE_IM_READY);

	/* Unlock after IM ready */
	_mtk_kbase_spm_kick_unlock();

	spm_release();

	MTK_err("spm-kick done");
}

int mtk_kbase_spm_isonline(void)
{
	return (DVFS_GPU_read32(0x640) != 0xbabebabe);
}

void mtk_kbase_spm_con(unsigned int val, unsigned int mask)
{
	unsigned int reg = DVFS_GPU_read32(SPM_RSV_CON);
	reg &= ~mask;
	reg |= (val & mask);
	DVFS_GPU_write32(SPM_RSV_CON, reg);
}

void mtk_kbase_spm_wait(void)
{
	int retry = 0xffff;

	while (DVFS_GPU_read32(SPM_RSV_STA) != DVFS_GPU_read32(SPM_RSV_CON) /*&& --retry*/)
	{
		udelay(1);

		if (!mtk_kbase_spm_isonline())
			break;
	}

	if (retry <= 0)
	{
		pr_MTK_err("dvfs_gpu spm wait timeout! STA:%u CON:%d\n", DVFS_GPU_read32(SPM_RSV_STA), DVFS_GPU_read32(SPM_RSV_CON));
	}
}

unsigned int mtk_kbase_spm_get_vol(unsigned int addr)
{
	unsigned int volreg = DVFS_GPU_read32(addr);

	if (volreg < 0xaa)
		return 603000 + (0x3f & volreg) * 12826;
	else
		return volreg * 1000;
}

unsigned int mtk_kbase_spm_get_freq(unsigned int addr)
{
	unsigned int freqreg = DVFS_GPU_read32(addr);

	if (freqreg & 0x80000000)
	{
		unsigned long mfgpll = freqreg & ~0xffe00000;
		unsigned int post_div = (freqreg >> 24) & 0x3;
		return (((mfgpll * 100 * 26  >> 14) / (1 << post_div) + 5) / 10) * 100;
	}
	else
	{
		return freqreg;
	}
}

void mtk_kbase_spm_set_dvfs_en(unsigned int en)
{
	spm_acquire();
	mtk_kbase_spm_con(en ? SPM_RSV_BIT_DVFS_EN : 0, SPM_RSV_BIT_DVFS_EN);
	spm_release();
}

static void spm_vf_adjust(unsigned int* pv, unsigned int* pf)
{
	if (pv && *pv > g_config->max_volt) *pv = g_config->max_volt;
	if (pv && *pv < g_config->min_volt) *pv = g_config->min_volt;
	if (pf && *pf > g_config->max_freq) *pf = g_config->max_freq;
	if (pf && *pf < g_config->min_freq) *pf = g_config->min_freq;
}

void mtk_kbase_spm_set_vol_freq_ceiling(unsigned int v, unsigned int f)
{
	int en;
	spm_vf_adjust(&v, &f);
	spm_acquire();
	en = DVFS_GPU_read32(SPM_RSV_CON);
	DVFS_GPU_write32(SPM_RSV_CON, 0);
	mtk_kbase_spm_wait();
	DVFS_GPU_write32(SPM_SW_CEIL_V, v);
	DVFS_GPU_write32(SPM_SW_CEIL_F, f);
	DVFS_GPU_write32(SPM_RSV_CON, en);
	mtk_kbase_spm_wait();
	spm_release();
}
void mtk_kbase_spm_set_vol_freq_floor(unsigned int v, unsigned int f)
{
	int en;
	spm_vf_adjust(&v, &f);
	spm_acquire();
	en = DVFS_GPU_read32(SPM_RSV_CON);
	DVFS_GPU_write32(SPM_RSV_CON, 0);
	mtk_kbase_spm_wait();
	DVFS_GPU_write32(SPM_SW_FLOOR_V, v);
	DVFS_GPU_write32(SPM_SW_FLOOR_F, f);
	DVFS_GPU_write32(SPM_RSV_CON, en);
	mtk_kbase_spm_wait();
	spm_release();
}
void mtk_kbase_spm_set_vol_freq_cf(unsigned int cv, unsigned int cf, unsigned int fv, unsigned int ff)
{
	int en;
	spm_vf_adjust(&cv, &cf);
	spm_vf_adjust(&fv, &ff);
	spm_acquire();
	en = DVFS_GPU_read32(SPM_RSV_CON);
	DVFS_GPU_write32(SPM_RSV_CON, 0);
	mtk_kbase_spm_wait();
	/* special case, ceiling = floor */
	DVFS_GPU_write32(SPM_SW_CEIL_V, cv);
	DVFS_GPU_write32(SPM_SW_CEIL_F, cf);
	DVFS_GPU_write32(SPM_SW_FLOOR_V, fv);
	DVFS_GPU_write32(SPM_SW_FLOOR_F, ff);
	DVFS_GPU_write32(SPM_RSV_CON, en);
	mtk_kbase_spm_wait();
	spm_release();
}

void mtk_kbase_spm_boost(unsigned int idx, unsigned int cnt)
{
	int en;
	spm_acquire();
	en = DVFS_GPU_read32(SPM_RSV_CON);
	DVFS_GPU_write32(SPM_RSV_CON, 0);
	mtk_kbase_spm_wait();
	DVFS_GPU_write32(SPM_SW_BOOST_IDX, idx);
	DVFS_GPU_write32(SPM_SW_BOOST_CNT, cnt);
	DVFS_GPU_write32(SPM_RSV_CON, en);
	mtk_kbase_spm_wait();
	spm_release();
}

void mtk_kbase_spm_update_table(void)
{
	int i, num;
	int en;
	spm_acquire();
	en = DVFS_GPU_read32(SPM_RSV_CON);
	DVFS_GPU_write32(SPM_RSV_CON, 0);
	mtk_kbase_spm_wait();

	num = mt_gpufreq_get_dvfs_table_num();
	mtk_kbase_spm_wait();
	for (i = 0; i < num; ++i)
	{
		unsigned int freq = mt_gpufreq_get_freq_by_idx(i);
		unsigned int volt = mt_gpufreq_get_volt_by_idx(i) / 100; // to mA
		DVFS_GPU_write32(SPM_TAB_F(num, i), freq);
		DVFS_GPU_write32(SPM_TAB_V(num, i), volt);
	}

	DVFS_GPU_write32(SPM_RSV_CON, en);
	mtk_kbase_spm_wait();
	spm_release();
}
