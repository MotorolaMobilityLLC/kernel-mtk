
#include "mtk_kbase_spm.h"

#include <linux/seq_file.h>
#include <linux/debugfs.h>
#if 0
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/string.h>
#endif
#include <linux/io.h>

void mtk_kbase_spm_kick(struct pcm_desc *pd)
{
    uint32_t tmp;

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
    DVFS_GPU_write32(DVFS_GPU_PCM_IM_PTR, (uint32_t)virt_to_phys(pd->base));
    DVFS_GPU_write32(DVFS_GPU_PCM_IM_LEN, pd->size);
    printk(KERN_ERR "xxxx pcm.base = 0x%0llx\n", virt_to_phys(pd->base));
    printk(KERN_ERR "xxxx pcm.base (uint32_t) = 0x%08x\n", (uint32_t)virt_to_phys(pd->base));
    printk(KERN_ERR "xxxx pcm.size = %u\n", pd->size - 1);
    DVFS_GPU_write32(DVFS_GPU_PCM_CON1, SPM_PROJECT_CODE | CON1_PCM_TIMER_EN | CON1_FIX_SC_CK_DIS | CON1_RF_SYNC);
    DVFS_GPU_write32(DVFS_GPU_PCM_CON1, SPM_PROJECT_CODE | CON1_PCM_TIMER_EN | CON1_FIX_SC_CK_DIS | CON1_MIF_APBEN);

    DVFS_GPU_write32(DVFS_GPU_PCM_PWR_IO_EN, 0x0081); /* sync register and enable IO output for r0 and r7 */

    /* Kick */
    DVFS_GPU_write32(DVFS_GPU_PCM_CON0, SPM_PROJECT_CODE | CON0_PCM_CK_EN | CON0_EN_SLEEP_DVS | CON0_IM_KICK_L | CON0_PCM_KICK_L);
    DVFS_GPU_write32(DVFS_GPU_PCM_CON0, SPM_PROJECT_CODE | CON0_PCM_CK_EN | CON0_EN_SLEEP_DVS);
}

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

