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

extern volatile void * g_MFG_base;
extern volatile void * g_DVFS_GPU_base;
extern volatile void * g_DFP_base;
extern volatile void * g_TOPCK_base;

#define base_write32(addr, value)     *(volatile uint32_t*)(addr) = (uint32_t)(value)
#define base_read32(addr)             (*(volatile uint32_t*)(addr))
#define MFG_write32(addr, value)      base_write32(g_MFG_base+addr, value)
#define MFG_read32(addr)              base_read32(g_MFG_base+addr)
#define DVFS_GPU_write32(addr, value) base_write32(g_DVFS_GPU_base+addr, value)
#define DVFS_GPU_read32(addr)         base_read32(g_DVFS_GPU_base+addr)
#define DFP_write32(addr, value)      base_write32(g_DFP_base+addr, value)
#define DFP_read32(addr)              base_read32(g_DFP_base+addr)
#define TOPCK_write32(addr, value)    base_write32(g_TOPCK_base+addr, value)
#define TOPCK_read32(addr)            base_read32(g_TOPCK_base+addr)

void mtk_kbase_dpm_setup(int *dfp_weights);

void mtk_kbase_spm_kick(struct pcm_desc *pd);

#endif
