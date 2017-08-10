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

#ifndef __MTK_KBASE_SPM_H__
#define __MTK_KBASE_SPM_H__

#include <linux/seq_file.h>
#include <linux/device.h>
#include <linux/debugfs.h>

struct pcm_desc {
    const char *version;    /* PCM code version */
    const u32 *base;    /* binary array base */
    const u32 size;     /* binary array size */
    const u8 sess;      /* session number */
    const u8 replace;   /* replace mode */

#if 0
    u32 vec0;       /* event vector 0 config */
    u32 vec1;       /* event vector 1 config */
    u32 vec2;       /* event vector 2 config */
    u32 vec3;       /* event vector 3 config */
    u32 vec4;       /* event vector 4 config */
    u32 vec5;       /* event vector 5 config */
    u32 vec6;       /* event vector 6 config */
    u32 vec7;       /* event vector 7 config */
#endif
};

struct mtk_config {
	//struct clk *clk_smi_common;
	//struct clk *clk_display;

	struct clk *clk_mfg_async;
	struct clk *clk_mfg;
#ifndef MTK_MALI_APM
	struct clk *clk_mfg_core0;
	struct clk *clk_mfg_core1;
	struct clk *clk_mfg_core2;
	struct clk *clk_mfg_core3;
#endif
	struct clk *clk_mfg_main;
	struct clk *clk_mfg52m_vcg;
#ifdef MTK_GPU_SPM
	struct clk *clk_dvfs_gpu;
	struct clk *clk_gpupm;
	struct clk *clk_ap_dma;
#endif

	struct clk *mux_mfg52m;
	struct clk *mux_mfg52m_52m;

	unsigned int max_volt;
	unsigned int max_freq;
	unsigned int min_volt;
	unsigned int min_freq;

	int32_t async_value;
};

#define MFG_DEBUG_SEL   (0x180)
#define MFG_DEBUG_A     (0x184)
#define MFG_DEBUG_IDEL  (1<<2)

#define MFG_OCP_DCM_CON             0x460

#define DFP_ID                      0x0
#define DFP_CTRL                    0x4
#define DFP_EVENT_ACCUM_CTRL        0x8
#define DFP_SCALING_FACTOR          0x50
#define DFP_VOLT_FACTOR             0x54
#define DFP_PM_PERIOD               0x64
#define DFP_COUNTER_EN_0            0x6C
#define DFP_COUNTER_EN_1            0x70
#define DFP_WEIGHT_(x)              (0x7C+4*(x))

#define DVFS_GPU_POWERON_CONFIG_EN  0x0
#define DVFS_GPU_SPM_POWER_ON_VAL0  0x4
#define DVFS_GPU_SPM_POWER_ON_VAL1  0x8
#define DVFS_GPU_PCM_CON0           0x18
#define DVFS_GPU_PCM_CON1           0x1c
#define DVFS_GPU_PCM_IM_PTR         0x20
#define DVFS_GPU_PCM_IM_LEN         0x24
#define DVFS_GPU_PCM_REG_DATA_INI   0x28
#define DVFS_GPU_PCM_PWR_IO_EN      0x2c

#define DVFS_GPU_PCM_FSM_STA        0x178
#define FSM_STA_IM_STATE_IM_READY   (0x4 << 7)
#define FSM_STA_IM_STATE_MASK       (0x7 << 7)

#define M0_REC0          0x300
#define M1_REC0          0x350
#define M2_REC0          0x3A0

#define SPM_GPU_POWER    (0x3A0 + 4 * 9)
#define SPM_BYPASS_DFP   (0x3A0 + 4 * 10)

#define SPM_SW_FLAG      0x600
#define SPM_SW_DEBUG     0x604
#define SPM_SW_RSV_0     0x608
#define SPM_SW_RSV_1     0x60C
#define SPM_SW_RSV_2     0x610
#define SPM_SW_RSV_3     0x614
#define SPM_SW_RSV_4     0x618
#define SPM_SW_RSV_5     0x61C
#define SPM_SW_RSV_6     0x620
#define SPM_SW_RSV_7     0x624
#define SPM_SW_RSV_8     0x628
#define SPM_SW_RSV_9     0x62C
#define SPM_SW_RSV_10    0x630
#define SPM_SW_RSV_11    0x634
#define SPM_SW_RSV_12    0x638
#define SPM_SW_RSV_13    0x63C
#define SPM_SW_RSV_14    0x640
#define SPM_SW_RSV_15    0x644

#define SPM_SW_CUR_V     0x608 // CURRENT V_REG
#define SPM_SW_CUR_F     0x60C // CURRENT F_REG
#define SPM_SW_TMP_V     0x610 // TARGET V_REG
#define SPM_SW_TMP_F     0x614 // TARGET F_REG
#define SPM_SW_CEIL_V    0x618 // CEILING V_VALUE -> Auto to V_REG @ EN=1
#define SPM_SW_CEIL_F    0x61C // CEILING F_VALUE -> Auto to F_REG @ EN=1
#define SPM_SW_FLOOR_V   0x620 // FLOOR V_VALUE -> Auto to V_REG @ EN=1
#define SPM_SW_FLOOR_F   0x624 // FLOOR F_VALUE -> Auto to F_REG @ EN=1
#define SPM_SW_BOOST_IDX 0x628 //
#define SPM_SW_BOOST_CNT 0x62C // ms
#define SPM_SW_RECOVER_CNT 0x630 //

#define SPM_RSV_CON      0x648
//#define SPM_RSV_STA      0x64C // read-only ..
#define SPM_RSV_STA      0x644 // use RSV_15 instead
#define SPM_RSV_BIT_EN           (1<<0)
#define SPM_RSV_BIT_DVFS_EN      (1<<1)

#define M3_REC0          0x800
#define SPM_TAB_S(size)	        (0x800)
#define SPM_TAB_F(size, idx)    (0x800+4*((i)+1))
#define SPM_TAB_V(size, idx)    (0x800+4*((i)+(size)+1))

#define SPM_PROJECT_CODE            (0xB16<<16)
#define CON0_PCM_KICK_L             (1<<0)
#define CON0_IM_KICK_L              (1<<1)
#define CON0_PCM_CK_EN              (1<<2)
#define CON0_EN_SLEEP_DVS           (1<<3)
#define CON0_IM_AUTO_PDN_EN         (1<<4)
#define CON0_PCM_SW_RESET           (1<<15)
#define CON1_IM_SLAVE               (1<<0)
#define CON1_IM_SLEEP               (1<<1)
#define CON1_RF_SYNC                (1<<2)
#define CON1_MIF_APBEN              (1<<3)
#define CON1_IM_PDN                 (1<<4)
#define CON1_PCM_TIMER_EN           (1<<5)
#define CON1_IM_NONRP_EN            (1<<6)
#define CON1_DIS_MIF_PROT           (1<<7)
#define CON1_PCM_WDT_EN             (1<<8)
#define CON1_PCM_WDT_WAKE_MODE      (1<<9)
#define CON1_SPM_SRAM_SLEEP_B       (1<<10)
#define CON1_FIX_SC_CK_DIS          (CON1_PCM_WDT_EN | CON1_PCM_WDT_WAKE_MODE | CON1_SPM_SRAM_SLEEP_B)
#define CON1_SPM_SRAM_ISOINT_B      (1<<11)
#define CON1_EVENT_LOCK_EN          (1<<12)
#define CON1_SRCCLKEN_FAST_RESP     (1<<13)
#define CON1_SCP_APB_INTERNAL_EN    (1<<1$)

#define CLK_MISC_CFG_0              0x104

extern volatile void *g_MFG_base;
extern volatile void *g_DVFS_CPU_base;
extern volatile void *g_DVFS_GPU_base;
extern volatile void *g_DFP_base;
extern volatile void *g_TOPCK_base;
extern struct pcm_desc dvfs_gpu_pcm;
extern struct mtk_config *g_config;

#define base_write32(addr, value)     *(volatile uint32_t*)(addr) = (uint32_t)(value)
#define base_read32(addr)             (*(volatile uint32_t*)(addr))
#define MFG_write32(addr, value)      base_write32(g_MFG_base+addr, value)
#define MFG_read32(addr)              base_read32(g_MFG_base+addr)
#define DVFS_CPU_write32(addr, value) base_write32(g_DVFS_CPU_base+addr, value)
#define DVFS_CPU_read32(addr)         base_read32(g_DVFS_CPU_base+addr)
#define DVFS_GPU_write32(addr, value) base_write32(g_DVFS_GPU_base+addr, value)
#define DVFS_GPU_read32(addr)         base_read32(g_DVFS_GPU_base+addr)
#define DFP_write32(addr, value)      base_write32(g_DFP_base+addr, value)
#define DFP_read32(addr)              base_read32(g_DFP_base+addr)
#define TOPCK_write32(addr, value)    base_write32(g_TOPCK_base+addr, value)
#define TOPCK_read32(addr)            base_read32(g_TOPCK_base+addr)

void mtk_kbase_dpm_setup(int *dfp_weights);

void mtk_kbase_spm_kick(struct pcm_desc *pd);

int mtk_kbase_spm_isonline(void);

void mtk_kbase_spm_acquire(void);
void mtk_kbase_spm_release(void);

void mtk_kbase_spm_con(unsigned int val, unsigned int mask);
void mtk_kbase_spm_wait(void);

unsigned int mtk_kbase_spm_get_vol(unsigned int addr); /* unit uV, 1e6 = 1 V */
unsigned int mtk_kbase_spm_get_freq(unsigned int addr); /* unit MHz, 600 = 600 MHz */

void mtk_kbase_spm_set_dvfs_en(unsigned int en);
unsigned int mtk_kbase_spm_get_dvfs_en(void);

void mtk_kbase_spm_set_vol_freq_ceiling(unsigned int v, unsigned int f);
void mtk_kbase_spm_set_vol_freq_floor(unsigned int v, unsigned int f);
void mtk_kbase_spm_set_vol_freq_cf(unsigned int cv, unsigned int cf, unsigned int fv, unsigned int ff);

void mtk_kbase_spm_boost(unsigned int idx, unsigned int cnt);

void mtk_kbase_spm_update_table(void);

void mtk_kbase_spm_hal_init(void);

void mtk_gpu_spm_resume_hal(void);
void mtk_gpu_spm_fix_by_idx(unsigned int idx);
void mtk_gpu_spm_reset_fix(void);

void mtk_gpu_spm_pause(void);
void mtk_gpu_spm_resume(void);

#endif
