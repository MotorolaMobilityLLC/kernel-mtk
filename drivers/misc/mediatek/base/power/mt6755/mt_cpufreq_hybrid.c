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
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/wait.h>

#include <mt-plat/sync_write.h>
#include <mt-plat/mt_lpae.h>
#include "mt_cpufreq_hybrid.h"
#include "mt_cpufreq.h"		/* for ext-buck API */


/**************************************
 * [Hybrid DVFS] Config
 **************************************/


/**************************************
 * [CPU SPM] Parameter
 **************************************/
#define CSRAM_BASE		0x0011c000
#define CSRAM_SIZE		0xf80		/* 3968 bytes */

#define OFFS_DATA_S		0x260
#define OFFS_WDT_LATCH0		0x264
#define OFFS_WDT_LATCH1		0x268
#define OFFS_WDT_LATCH2		0x26c
#define OFFS_TIMER_OUT1		0x270
#define OFFS_TIMER_OUT2		0x274
#define OFFS_PCM_PC1		0x278
#define OFFS_PCM_PC2		0x27c
#define OFFS_FW_RSV5		0x280
#define OFFS_FW_RSV3		0x284
#define OFFS_FW_RSV4		0x288
#define OFFS_FW_RSV2		0x28c
#define OFFS_PAUSE_TIME		0x290
#define OFFS_UNPAUSE_TIME	0x294
#define OFFS_INIT_OPP		0x2b0
#define OFFS_INIT_FREQ		0x2c0
#define OFFS_INIT_VOLT		0x2d0
#define OFFS_SW_RSV4		0x2e0
#define OFFS_PAUSE_SRC		0x2e4
#define OFFS_DVFS_WAIT		0x2e8
#define OFFS_FUNC_ENTER		0x2ec
#define OFFS_LOG_S		0x340
#define OFFS_LOG_E		0xf70

#define DVFS_TIMEOUT		6000		/* us */
#define SEMA_GET_TIMEOUT	2000		/* us */
#define PAUSE_TIMEOUT		2000		/* us */


/**************************************
 * [Hybrid DVFS] Parameter
 **************************************/
#define ENTRY_EACH_LOG		3


/**************************************
 * [CPU SPM] Definition
 **************************************/
#define CSPM_BASE		cspm_base	/* map 0x10227000 */

#define CSPM_POWERON_CONFIG_EN		(CSPM_BASE + 0x000)
#define CSPM_POWER_ON_VAL0		(CSPM_BASE + 0x004)
#define CSPM_POWER_ON_VAL1		(CSPM_BASE + 0x008)
#define CSPM_CLK_CON			(CSPM_BASE + 0x00c)
#define CSPM_CLK_SETTLE			(CSPM_BASE + 0x010)
#define CSPM_AP_STANDBY_CON		(CSPM_BASE + 0x014)
#define CSPM_PCM_CON0			(CSPM_BASE + 0x018)
#define CSPM_PCM_CON1			(CSPM_BASE + 0x01c)
#define CSPM_PCM_IM_PTR			(CSPM_BASE + 0x020)
#define CSPM_PCM_IM_LEN			(CSPM_BASE + 0x024)
#define CSPM_PCM_REG_DATA_INI		(CSPM_BASE + 0x028)
#define CSPM_PCM_PWR_IO_EN		(CSPM_BASE + 0x02c)
#define CSPM_PCM_TIMER_VAL		(CSPM_BASE + 0x030)
#define CSPM_PCM_WDT_VAL		(CSPM_BASE + 0x034)
#define CSPM_PCM_IM_HOST_RW_PTR		(CSPM_BASE + 0x038)
#define CSPM_PCM_IM_HOST_RW_DAT		(CSPM_BASE + 0x03c)
#define CSPM_PCM_EVENT_VECTOR0		(CSPM_BASE + 0x040)
#define CSPM_PCM_EVENT_VECTOR1		(CSPM_BASE + 0x044)
#define CSPM_PCM_EVENT_VECTOR2		(CSPM_BASE + 0x048)
#define CSPM_PCM_EVENT_VECTOR3		(CSPM_BASE + 0x04c)
#define CSPM_PCM_EVENT_VECTOR4		(CSPM_BASE + 0x050)
#define CSPM_PCM_EVENT_VECTOR5		(CSPM_BASE + 0x054)
#define CSPM_PCM_EVENT_VECTOR6		(CSPM_BASE + 0x058)
#define CSPM_PCM_EVENT_VECTOR7		(CSPM_BASE + 0x05c)
#define CSPM_PCM_EVENT_VECTOR8		(CSPM_BASE + 0x060)
#define CSPM_PCM_EVENT_VECTOR9		(CSPM_BASE + 0x064)
#define CSPM_PCM_EVENT_VECTOR10		(CSPM_BASE + 0x068)
#define CSPM_PCM_EVENT_VECTOR11		(CSPM_BASE + 0x06c)
#define CSPM_PCM_EVENT_VECTOR12		(CSPM_BASE + 0x070)
#define CSPM_PCM_EVENT_VECTOR13		(CSPM_BASE + 0x074)
#define CSPM_PCM_EVENT_VECTOR14		(CSPM_BASE + 0x078)
#define CSPM_PCM_EVENT_VECTOR15		(CSPM_BASE + 0x07c)
#define CSPM_PCM_EVENT_VECTOR_EN	(CSPM_BASE + 0x080)
#define CSPM_SW_INT_SET			(CSPM_BASE + 0x090)
#define CSPM_SW_INT_CLEAR		(CSPM_BASE + 0x094)
#define CSPM_SCP_MAILBOX		(CSPM_BASE + 0x098)
#define CSPM_SCP_IRQ			(CSPM_BASE + 0x09c)
#define CSPM_TWAM_CON			(CSPM_BASE + 0x0a0)
#define CSPM_TWAM_WINDOW_LEN		(CSPM_BASE + 0x0a4)
#define CSPM_TWAM_IDLE_SEL		(CSPM_BASE + 0x0a8)
#define CSPM_TWAM_CON1			(CSPM_BASE + 0x0ac)
#define CSPM_CPU_WAKEUP_EVENT		(CSPM_BASE + 0x0b0)
#define CSPM_IRQ_MASK			(CSPM_BASE + 0x0b4)
#define CSPM_SRC_REQ			(CSPM_BASE + 0x0b8)
#define CSPM_SRC_MASK			(CSPM_BASE + 0x0bc)
#define CSPM_SRC2_MASK			(CSPM_BASE + 0x0c0)
#define CSPM_WAKEUP_EVENT_MASK		(CSPM_BASE + 0x0c4)
#define CSPM_WAKEUP_EVENT_EXT_MASK	(CSPM_BASE + 0x0c8)
#define CSPM_SCP_CLK_CON		(CSPM_BASE + 0x0d0)
#define CSPM_PCM_DEBUG_CON		(CSPM_BASE + 0x0d4)
#define CSPM_PCM_REG0_DATA		(CSPM_BASE + 0x100)
#define CSPM_PCM_REG1_DATA		(CSPM_BASE + 0x104)
#define CSPM_PCM_REG2_DATA		(CSPM_BASE + 0x108)
#define CSPM_PCM_REG3_DATA		(CSPM_BASE + 0x10c)
#define CSPM_PCM_REG4_DATA		(CSPM_BASE + 0x110)
#define CSPM_PCM_REG5_DATA		(CSPM_BASE + 0x114)
#define CSPM_PCM_REG6_DATA		(CSPM_BASE + 0x118)
#define CSPM_PCM_REG7_DATA		(CSPM_BASE + 0x11c)
#define CSPM_PCM_REG8_DATA		(CSPM_BASE + 0x120)
#define CSPM_PCM_REG9_DATA		(CSPM_BASE + 0x124)
#define CSPM_PCM_REG10_DATA		(CSPM_BASE + 0x128)
#define CSPM_PCM_REG11_DATA		(CSPM_BASE + 0x12c)
#define CSPM_PCM_REG12_DATA		(CSPM_BASE + 0x130)
#define CSPM_PCM_REG13_DATA		(CSPM_BASE + 0x134)
#define CSPM_PCM_REG14_DATA		(CSPM_BASE + 0x138)
#define CSPM_PCM_REG15_DATA		(CSPM_BASE + 0x13c)
#define CSPM_PCM_REG12_MASK_B_STA	(CSPM_BASE + 0x140)
#define CSPM_PCM_REG12_EXT_DATA		(CSPM_BASE + 0x144)
#define CSPM_PCM_REG12_EXT_MASK_B_STA	(CSPM_BASE + 0x148)
#define CSPM_PCM_EVENT_REG_STA		(CSPM_BASE + 0x14c)
#define CSPM_PCM_TIMER_OUT		(CSPM_BASE + 0x150)
#define CSPM_PCM_WDT_OUT		(CSPM_BASE + 0x154)
#define CSPM_IRQ_STA			(CSPM_BASE + 0x158)
#define CSPM_WAKEUP_STA			(CSPM_BASE + 0x15c)
#define CSPM_WAKEUP_EXT_STA		(CSPM_BASE + 0x160)
#define CSPM_WAKEUP_MISC		(CSPM_BASE + 0x164)
#define CSPM_BUS_PROTECT_RDY		(CSPM_BASE + 0x168)
#define CSPM_BUS_PROTECT2_RDY		(CSPM_BASE + 0x16c)
#define CSPM_SUBSYS_IDLE_STA		(CSPM_BASE + 0x170)
#define CSPM_CPU_IDLE_STA		(CSPM_BASE + 0x174)
#define CSPM_PCM_FSM_STA		(CSPM_BASE + 0x178)
#define CSPM_PCM_WDT_LATCH0		(CSPM_BASE + 0x190)
#define CSPM_PCM_WDT_LATCH1		(CSPM_BASE + 0x194)
#define CSPM_PCM_WDT_LATCH2		(CSPM_BASE + 0x198)
#define CSPM_DRAMC_DBG_LATCH		(CSPM_BASE + 0x19c)
#define CSPM_TWAM_LAST_STA0		(CSPM_BASE + 0x1a0)
#define CSPM_TWAM_LAST_STA1		(CSPM_BASE + 0x1a4)
#define CSPM_TWAM_LAST_STA2		(CSPM_BASE + 0x1a8)
#define CSPM_TWAM_LAST_STA3		(CSPM_BASE + 0x1ac)
#define CSPM_TWAM_LAST_STA4		(CSPM_BASE + 0x1b0)
#define CSPM_TWAM_LAST_STA5		(CSPM_BASE + 0x1b4)
#define CSPM_TWAM_LAST_STA6		(CSPM_BASE + 0x1b8)
#define CSPM_TWAM_LAST_STA7		(CSPM_BASE + 0x1bc)
#define CSPM_TWAM_CURR_STA0		(CSPM_BASE + 0x1c0)
#define CSPM_TWAM_CURR_STA1		(CSPM_BASE + 0x1c4)
#define CSPM_TWAM_CURR_STA2		(CSPM_BASE + 0x1c8)
#define CSPM_TWAM_CURR_STA3		(CSPM_BASE + 0x1cc)
#define CSPM_TWAM_CURR_STA4		(CSPM_BASE + 0x1d0)
#define CSPM_TWAM_CURR_STA5		(CSPM_BASE + 0x1d4)
#define CSPM_TWAM_CURR_STA6		(CSPM_BASE + 0x1d8)
#define CSPM_TWAM_CURR_STA7		(CSPM_BASE + 0x1dc)
#define CSPM_TWAM_TIMER_OUT		(CSPM_BASE + 0x1e0)
#define CSPM_DVFS_CON			(CSPM_BASE + 0x400)
#define CSPM_MDBSI_CON			(CSPM_BASE + 0x404)
#define CSPM_BSI_GEN			(CSPM_BASE + 0x410)
#define CSPM_BSI_EN_SR			(CSPM_BASE + 0x414)
#define CSPM_BSI_CLK_SR			(CSPM_BASE + 0x418)
#define CSPM_BSI_D0_SR			(CSPM_BASE + 0x41c)
#define CSPM_BSI_D1_SR			(CSPM_BASE + 0x420)
#define CSPM_BSI_D2_SR			(CSPM_BASE + 0x424)
#define CSPM_AP_SEMA			(CSPM_BASE + 0x428)
#define CSPM_SPM_SEMA			(CSPM_BASE + 0x42c)
#define CSPM_MP0_CPU0_WFI_EN		(CSPM_BASE + 0x530)
#define CSPM_MP0_CPU1_WFI_EN		(CSPM_BASE + 0x534)
#define CSPM_MP0_CPU2_WFI_EN		(CSPM_BASE + 0x538)
#define CSPM_MP0_CPU3_WFI_EN		(CSPM_BASE + 0x53c)
#define CSPM_MP1_CPU0_WFI_EN		(CSPM_BASE + 0x540)
#define CSPM_MP1_CPU1_WFI_EN		(CSPM_BASE + 0x544)
#define CSPM_MP1_CPU2_WFI_EN		(CSPM_BASE + 0x548)
#define CSPM_MP1_CPU3_WFI_EN		(CSPM_BASE + 0x54c)
#define CSPM_SW_FLAG			(CSPM_BASE + 0x600)
#define CSPM_SW_DEBUG			(CSPM_BASE + 0x604)
#define CSPM_SW_RSV0			(CSPM_BASE + 0x608)
#define CSPM_SW_RSV1			(CSPM_BASE + 0x60c)
#define CSPM_SW_RSV2			(CSPM_BASE + 0x610)
#define CSPM_SW_RSV3			(CSPM_BASE + 0x614)
#define CSPM_SW_RSV4			(CSPM_BASE + 0x618)
#define CSPM_SW_RSV5			(CSPM_BASE + 0x61c)
#define CSPM_RSV_CON			(CSPM_BASE + 0x620)
#define CSPM_RSV_STA			(CSPM_BASE + 0x624)

#define POWER_ON_VAL1_DEF	0x20		/* PCM clock is 26M */

#define PCM_FSM_STA_DEF		0x48490

#define PROJECT_CODE		0xb16

#define REGWR_EN		(1U << 0)
#define REGWR_CFG_KEY		(PROJECT_CODE << 16)

#define CON0_PCM_KICK		(1U << 0)
#define CON0_IM_KICK		(1U << 1)
#define CON0_PCM_CK_EN		(1U << 2)
#define CON0_PCM_SW_RESET	(1U << 15)
#define CON0_CFG_KEY		(PROJECT_CODE << 16)

#define CON1_IM_SLAVE		(1U << 0)
#define CON1_MIF_APBEN		(1U << 3)
#define CON1_PCM_TIMER_EN	(1U << 5)
#define CON1_IM_NONRP_EN	(1U << 6)
#define CON1_PCM_WDT_EN		(1U << 8)
#define CON1_PCM_WDT_WAKE_MODE	(1U << 9)
#define CON1_SPM_SRAM_SLP_B	(1U << 10)
#define CON1_SPM_SRAM_ISO_B	(1U << 11)
#define CON1_EVENT_LOCK_EN	(1U << 12)
#define CON1_CFG_KEY		(PROJECT_CODE << 16)

#define PCM_PWRIO_EN_R7		(1U << 7)
#define PCM_RF_SYNC_R0		(1U << 16)
#define PCM_RF_SYNC_R1		(1U << 17)
#define PCM_RF_SYNC_R7		(1U << 23)

#define PCM_SW_INT0		(1U << 0)
#define PCM_SW_INT1		(1U << 1)
#define PCM_SW_INT_ALL		(PCM_SW_INT1 | PCM_SW_INT0)

#define TMT_RISING		0
#define TMT_FALLING		1
#define TMT_HIGH_LEVEL		2
#define TMT_LOW_LEVEL		3

#define TWAM_ENABLE		(1U << 0)
#define TWAM_SPEED_MODE_EN	(1U << 1)
#define TWAM_SW_RST		(1U << 2)
#define TWAM_MON_TYPE0(val)	(((val) & 0x3) << 4)
#define TWAM_MON_TYPE1(val)	(((val) & 0x3) << 6)
#define TWAM_MON_TYPE2(val)	(((val) & 0x3) << 8)
#define TWAM_MON_TYPE3(val)	(((val) & 0x3) << 10)
#define TWAM_SIG_SEL0(val)	(((val) & 0x1f) << 12)
#define TWAM_SIG_SEL1(val)	(((val) & 0x1f) << 17)
#define TWAM_SIG_SEL2(val)	(((val) & 0x1f) << 22)
#define TWAM_SIG_SEL3(val)	(((val) & 0x1f) << 27)

#define IRQM_TWAM		(1U << 2)
#define IRQM_PCM_RETURN		(1U << 3)
#define IRQM_RET_IRQ0		(1U << 8)
#define IRQM_RET_IRQ1		(1U << 9)

#define IRQM_RET_IRQ_SEC	(IRQM_RET_IRQ1)
#define IRQM_ALL_EXC_RET0	(IRQM_RET_IRQ_SEC | IRQM_TWAM)
#define IRQM_ALL		(IRQM_ALL_EXC_RET0 | IRQM_RET_IRQ0 | IRQM_PCM_RETURN)

#define WAKE_SRC_CPU		(1U << 0)
#define WAKE_SRC_TWAM		(1U << 1)

#define R7_PCM_TIMER_SET	(1U << 9)

#define IRQS_TWAM		(1U << 2)
#define IRQS_PCM_RETURN		(1U << 3)
#define IRQS_SW_INT0		(1U << 4)
#define IRQS_SW_INT1		(1U << 5)

#define IRQC_TWAM		IRQS_TWAM
#define IRQC_PCM_RETURN		IRQS_PCM_RETURN
#define IRQC_ALL		(IRQC_PCM_RETURN | IRQC_TWAM)

#define FSM_PC_STA_IDLE		(1U << 4)
#define FSM_PC_STA_INC		(1U << 5)
#define FSM_PC_STA_STALL	(1U << 6)
#define FSM_PCM_KICK		(1U << 21)

#define SW_L_F_MIN(val)		(((val) & 0xf) << 0)
#define SW_L_F_MAX(val)		(((val) & 0xf) << 4)
#define SW_L_F_DES(val)		(((val) & 0xf) << 8)
#define SW_L_F_ASSIGN		(1U << 12)
#define SW_L_PAUSE		(1U << 13)
#define L_CLUSTER_EN		(1U << 14)
#define BYPASS_FIT		(1U << 15)
#define SW_B_F_MIN(val)		(((val) & 0xf) << 16)
#define SW_B_F_MAX(val)		(((val) & 0xf) << 20)
#define SW_B_F_DES(val)		(((val) & 0xf) << 24)
#define SW_B_F_ASSIGN		(1U << 28)
#define SW_B_PAUSE		(1U << 29)
#define B_CLUSTER_EN		(1U << 30)

#define SW_L_F_MIN_MASK		(0xf << 0)
#define SW_L_F_MAX_MASK		(0xf << 4)
#define SW_L_F_DES_MASK		(0xf << 8)
#define SW_B_F_MIN_MASK		(0xf << 16)
#define SW_B_F_MAX_MASK		(0xf << 20)
#define SW_B_F_DES_MASK		(0xf << 24)
#define SW_ALL_PAUSE		(SW_B_PAUSE | SW_L_PAUSE)

#define L_F_CURR(val)		(((val) & 0xf) << 0)
#define L_F_DES(val)		(((val) & 0xf) << 4)
#define L_V_CURR(val)		(((val) & 0x7f) << 8)
#define FW_L_DONE		(1U << 15)
#define B_F_CURR(val)		(((val) & 0xf) << 16)
#define B_F_DES(val)		(((val) & 0xf) << 20)
#define B_V_CURR(val)		(((val) & 0x7f) << 24)
#define FW_B_DONE		(1U << 31)

#define L_F_CURR_MASK		(0xf << 0)
#define L_F_DES_MASK		(0xf << 4)
#define L_V_CURR_MASK		(0x7f << 8)
#define B_F_CURR_MASK		(0xf << 16)
#define B_F_DES_MASK		(0xf << 20)
#define B_V_CURR_MASK		(0x7f << 24)
#define FW_ALL_DONE		(FW_B_DONE | FW_L_DONE)

/* pause source flag */
#define PSF_PAUSE_INIT		(1U << PAUSE_INIT)
#define PSF_PAUSE_I2CDRV	(1U << PAUSE_I2CDRV)
#define PSF_PAUSE_IDLE		(1U << PAUSE_IDLE)
#define PSF_PAUSE_SUSPEND	(1U << PAUSE_SUSPEND)

/* debug log flag */
#define DLF_KICK		(1U << 0)
#define DLF_CLUSTER		(1U << 1)
#define DLF_DVFS		(1U << 2)
#define DLF_SEMA		(1U << 3)
#define DLF_PAUSE		(1U << 4)
#define DLF_STOP		(1U << 5)

/* function enter flag */
#define FEF_DVFS		(1U << 0)
#define FEF_CLUSTER_OFF		(1U << 1)

struct pcm_desc;

struct pwr_ctrl {
	u8 twam_wfi_init;
	u8 r7_ctrl_en;

	u32 wake_src;
};


/**************************************
 * [Hybrid DVFS] Definition
 **************************************/
#define DBG_REPO_S		CSRAM_BASE
#define DBG_REPO_E		(DBG_REPO_S + CSRAM_SIZE)
#define DBG_REPO_DATA_S		(DBG_REPO_S + OFFS_DATA_S)
#define DBG_REPO_DATA_E		(DBG_REPO_S + OFFS_LOG_S)
#define DBG_REPO_LOG_S		(DBG_REPO_S + OFFS_LOG_S)
#define DBG_REPO_LOG_E		(DBG_REPO_S + OFFS_LOG_E)

#define DBG_REPO_SIZE		(DBG_REPO_E - DBG_REPO_S)
#define DBG_REPO_NUM		(DBG_REPO_SIZE / sizeof(u32))

#define REPO_I_DATA_S		(OFFS_DATA_S / sizeof(u32))
#define REPO_I_WDT_LATCH0	(OFFS_WDT_LATCH0 / sizeof(u32))
#define REPO_I_WDT_LATCH1	(OFFS_WDT_LATCH1 / sizeof(u32))
#define REPO_I_WDT_LATCH2	(OFFS_WDT_LATCH2 / sizeof(u32))
#define REPO_I_LOG_S		(OFFS_LOG_S / sizeof(u32))

#define REPO_GUARD0		0x55aa55aa
#define REPO_GUARD1		0xaa55aa55

struct cpuhvfs_dvfsp {
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl;

	void __iomem *log_repo;

	u32 init_done;		/* for dvfsp and log_repo */
	u32 use_i2c;

	int (*init_dvfsp)(struct cpuhvfs_dvfsp *dvfsp);
	int (*kick_dvfsp)(struct cpuhvfs_dvfsp *dvfsp, struct init_sta *sta);
	bool (*is_kicked)(struct cpuhvfs_dvfsp *dvfsp);

	void (*cluster_on)(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster);
	void (*cluster_off)(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster);

	int (*set_target)(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster, unsigned int index,
			  unsigned int *ret_volt);
	unsigned int (*get_volt)(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster);

	int (*get_sema)(struct cpuhvfs_dvfsp *dvfsp, enum sema_user user);
	void (*release_sema)(struct cpuhvfs_dvfsp *dvfsp, enum sema_user user);

	int (*pause_dvfsp)(struct cpuhvfs_dvfsp *dvfsp, enum pause_src src);
	void (*unpause_dvfsp)(struct cpuhvfs_dvfsp *dvfsp, enum pause_src src);

	int (*stop_dvfsp)(struct cpuhvfs_dvfsp *dvfsp);

	void (*dump_info)(struct cpuhvfs_dvfsp *dvfsp, const char *fmt, ...);
};

struct cpuhvfs_data {
	struct cpuhvfs_dvfsp *dvfsp;

	u32 *dbg_repo;		/* => dvfsp->log_repo */
	struct dentry *dbg_fs;
};


#ifdef CONFIG_HYBRID_CPU_DVFS

/**************************************
 * [CPU SPM] Declaration
 **************************************/
#include "mt_cpufreq_hybrid_fw.h"

static struct pwr_ctrl dvfs_ctrl = {
	.r7_ctrl_en	= 1,

	.wake_src	= WAKE_SRC_TWAM | WAKE_SRC_CPU,
};

static void __iomem *cspm_base;
static void __iomem *csram_base;
static void __iomem *sema_reg[NUM_SEMA_USER];

static struct clk *i2c_clk;

static u32 cspm_probe_done;

static struct init_sta suspend_sta = {
	.opp	= { [0 ... (NUM_CPU_CLUSTER - 1)] = UINT_MAX },
	.volt	= { [0 ... (NUM_CPU_CLUSTER - 1)] = UINT_MAX },
};

/**
 * 1. Will not be restored after IPO-H boot
 * 2. Keep PAUSE_INIT set until cluster-on notification
 */
static u32 pause_src_map __nosavedata = PSF_PAUSE_INIT;

static u32 pause_log_en = /*PSF_PAUSE_SUSPEND |*/
			  /*PSF_PAUSE_IDLE |*/
			  /*PSF_PAUSE_I2CDRV |*/
			  PSF_PAUSE_INIT;

/**
 * dbgx_log_en[15:0] : show on UART/MobileLog
 * dbgx_log_en[31:16]: show on MobileLog
 */
static u32 dbgx_log_en = /*(DLF_DVFS << 16) |*/
			 DLF_STOP |
			 DLF_PAUSE |
			 /*DLF_SEMA |*/
			 /*DLF_DVFS |*/
			 DLF_CLUSTER |
			 DLF_KICK;

static u32 dvfs_fail_ke;
static u32 sema_fail_ke;
static u32 pause_fail_ke;

static DEFINE_SPINLOCK(cspm_lock);
static DEFINE_SPINLOCK(dvfs_lock);

static DECLARE_WAIT_QUEUE_HEAD(dvfs_wait);

static int cspm_module_init(struct cpuhvfs_dvfsp *dvfsp);
static int cspm_go_to_dvfs(struct cpuhvfs_dvfsp *dvfsp, struct init_sta *sta);
static bool cspm_is_pcm_kicked(struct cpuhvfs_dvfsp *dvfsp);

static void cspm_cluster_notify_on(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster);
static void cspm_cluster_notify_off(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster);

static int cspm_set_target_opp(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster, unsigned int index,
			       unsigned int *ret_volt);
static unsigned int cspm_get_curr_volt(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster);

static int cspm_get_semaphore(struct cpuhvfs_dvfsp *dvfsp, enum sema_user user);
static void cspm_release_semaphore(struct cpuhvfs_dvfsp *dvfsp, enum sema_user user);

static int cspm_pause_pcm_running(struct cpuhvfs_dvfsp *dvfsp, enum pause_src src);
static void cspm_unpause_pcm_to_run(struct cpuhvfs_dvfsp *dvfsp, enum pause_src src);

static int cspm_stop_pcm_running(struct cpuhvfs_dvfsp *dvfsp);

static void cspm_dump_debug_info(struct cpuhvfs_dvfsp *dvfsp, const char *fmt, ...);


/**************************************
 * [Hybrid DVFS] Declaration
 **************************************/
static struct cpuhvfs_dvfsp g_dvfsp = {
	.pcmdesc	= &dvfs_pcm_6311,
	.pwrctrl	= &dvfs_ctrl,

	.use_i2c	= 1,

	.init_dvfsp	= cspm_module_init,
	.kick_dvfsp	= cspm_go_to_dvfs,
	.is_kicked	= cspm_is_pcm_kicked,

	.cluster_on	= cspm_cluster_notify_on,
	.cluster_off	= cspm_cluster_notify_off,

	.set_target	= cspm_set_target_opp,
	.get_volt	= cspm_get_curr_volt,

	.get_sema	= cspm_get_semaphore,
	.release_sema	= cspm_release_semaphore,

	.pause_dvfsp	= cspm_pause_pcm_running,
	.unpause_dvfsp	= cspm_unpause_pcm_to_run,

	.stop_dvfsp	= cspm_stop_pcm_running,

	.dump_info	= cspm_dump_debug_info,
};

static struct cpuhvfs_data g_cpuhvfs = {
	.dvfsp		= &g_dvfsp,
};

static u32 dbg_repo_bak[DBG_REPO_NUM];


/**************************************
 * [CPU SPM] Macro / Inline
 **************************************/
#define cspm_read(addr)			__raw_readl(addr)
#define cspm_write(addr, val)		mt_reg_sync_writel(val, addr)

#define csram_read(offs)		__raw_readl(csram_base + (offs))
#define csram_write(offs, val)		mt_reg_sync_writel(val, csram_base + (offs))

#define csram_write_fw_sta()					\
do {								\
	csram_write(OFFS_FW_RSV5, cspm_read(CSPM_SW_RSV5));	\
	csram_write(OFFS_FW_RSV3, cspm_read(CSPM_SW_RSV3));	\
	csram_write(OFFS_FW_RSV4, cspm_read(CSPM_SW_RSV4));	\
	csram_write(OFFS_FW_RSV2, cspm_read(CSPM_SW_RSV2));	\
} while (0)

#define cspm_get_timestamp()		cspm_read(CSPM_PCM_TIMER_OUT)
#define cspm_get_pcmpc()		cspm_read(CSPM_PCM_REG15_DATA)

#define cspm_get_min_freq_ll()		((cspm_read(CSPM_SW_RSV4) & SW_L_F_MIN_MASK) >> 0)
#define cspm_get_min_freq_l()		((cspm_read(CSPM_SW_RSV4) & SW_B_F_MIN_MASK) >> 16)

#define cspm_get_max_freq_ll()		((cspm_read(CSPM_SW_RSV4) & SW_L_F_MAX_MASK) >> 4)
#define cspm_get_max_freq_l()		((cspm_read(CSPM_SW_RSV4) & SW_B_F_MAX_MASK) >> 20)

#define cspm_is_ll_paused()		(!!(cspm_read(CSPM_SW_RSV4) & SW_L_PAUSE))
#define cspm_is_l_paused()		(!!(cspm_read(CSPM_SW_RSV4) & SW_B_PAUSE))
#define cspm_is_all_paused()		((cspm_read(CSPM_SW_RSV4) & SW_ALL_PAUSE) == SW_ALL_PAUSE)

#define cspm_get_curr_freq_ll()		((cspm_read(CSPM_SW_RSV5) & L_F_CURR_MASK) >> 0)
#define cspm_get_curr_freq_l()		((cspm_read(CSPM_SW_RSV5) & B_F_CURR_MASK) >> 16)

#define cspm_get_curr_volt_ll()		((cspm_read(CSPM_SW_RSV5) & L_V_CURR_MASK) >> 8)
#define cspm_get_curr_volt_l()		((cspm_read(CSPM_SW_RSV5) & B_V_CURR_MASK) >> 24)

#define cspm_is_fw_ll_done()		(!!(cspm_read(CSPM_SW_RSV5) & FW_L_DONE))
#define cspm_is_fw_l_done()		(!!(cspm_read(CSPM_SW_RSV5) & FW_B_DONE))
#define cspm_is_fw_all_done()		((cspm_read(CSPM_SW_RSV5) & FW_ALL_DONE) == FW_ALL_DONE)

#define cspm_err(fmt, args...)		pr_err("[CPUHVFS] " fmt, ##args)
#define cspm_warn(fmt, args...)		pr_warn("[CPUHVFS] " fmt, ##args)
#define cspm_debug(fmt, args...)	pr_debug("[CPUHVFS] " fmt, ##args)

#define cspm_dbgx(flag, fmt, args...)			\
do {							\
	if (dbgx_log_en & DLF_##flag)			\
		cspm_err(fmt, ##args);			\
	else if (dbgx_log_en & (DLF_##flag << 16))	\
		cspm_debug(fmt, ##args);		\
	else						\
		;					\
} while (0)

#define wait_complete_us(condition, delay, timeout)		\
({								\
	int _i = 0;						\
	int _n = DIV_ROUND_UP(timeout, delay);			\
	csram_write(OFFS_PCM_PC1, cspm_get_pcmpc());		\
	csram_write(OFFS_TIMER_OUT1, cspm_get_timestamp());	\
	while (!(condition)) {					\
		if (_i >= _n) {					\
			_i = -_i;				\
			break;					\
		}						\
		udelay(delay);					\
		_i++;						\
	}							\
	csram_write(OFFS_TIMER_OUT2, cspm_get_timestamp());	\
	csram_write(OFFS_PCM_PC2, cspm_get_pcmpc());		\
	_i;							\
})

static inline u32 base_va_to_pa(const u32 *base)
{
	phys_addr_t pa = virt_to_phys(base);

	MAPPING_DRAM_ACCESS_ADDR(pa);	/* for 4GB mode */

	return (u32)pa;
}

static inline u32 opp_sw_to_fw(unsigned int index)
{
	return index < NUM_CPU_OPP ? (NUM_CPU_OPP - 1) - index : 0 /* lowest frequency */;
}

static inline unsigned int opp_fw_to_sw(u32 freq)
{
	return freq < NUM_CPU_OPP ? (NUM_CPU_OPP - 1) - freq : 0 /* highest frequency */;
}


/**************************************
 * [Hybrid DVFS] Macro / Inline
 **************************************/
#define set_dvfsp_init_done(dvfsp)	((dvfsp)->init_done = 1)
#define is_dvfsp_uninit(dvfsp)		(!(dvfsp)->init_done)

#define cpuhvfs_crit(fmt, args...)	pr_err("[CPUHVFS] " fmt, ##args)
#define cpuhvfs_err(fmt, args...)	pr_err("[CPUHVFS] " fmt, ##args)
#define cpuhvfs_warn(fmt, args...)	pr_warn("[CPUHVFS] " fmt, ##args)
#define cpuhvfs_debug(fmt, args...)	pr_debug("[CPUHVFS] " fmt, ##args)

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

#define DEFINE_FOPS_RW(fname)						\
static int fname##_open(struct inode *inode, struct file *file)		\
{									\
	return single_open(file, fname##_show, inode->i_private);	\
}									\
static const struct file_operations fname##_fops = {			\
	.open		= fname##_open,					\
	.read		= seq_read,					\
	.llseek		= seq_lseek,					\
	.write		= fname##_write,				\
	.release	= single_release,				\
}


/**************************************
 * [CPU SPM] Function
 **************************************/
static void __cspm_pcm_sw_reset(void)
{
	u32 sta;

	cspm_write(CSPM_PCM_CON0, CON0_CFG_KEY | CON0_PCM_SW_RESET);
	cspm_write(CSPM_PCM_CON0, CON0_CFG_KEY);

	udelay(10);

	sta = cspm_read(CSPM_PCM_FSM_STA);
	if (sta != PCM_FSM_STA_DEF) {
		cspm_err("FAILED TO RESET PCM (0x%x)\n", sta);
		BUG();
	}
}

static void __cspm_register_init(void)
{
	/* enable register write */
	cspm_write(CSPM_POWERON_CONFIG_EN, REGWR_CFG_KEY | REGWR_EN);

	/* init power control register */
	cspm_write(CSPM_POWER_ON_VAL1, POWER_ON_VAL1_DEF | R7_PCM_TIMER_SET);
	cspm_write(CSPM_PCM_PWR_IO_EN, 0);

	/* reset PCM */
	__cspm_pcm_sw_reset();

	/* init PCM control register */
	cspm_write(CSPM_PCM_CON0, CON0_CFG_KEY | CON0_PCM_CK_EN);
	cspm_write(CSPM_PCM_CON1, CON1_CFG_KEY | CON1_EVENT_LOCK_EN |
				  CON1_SPM_SRAM_ISO_B | CON1_SPM_SRAM_SLP_B |
				  CON1_MIF_APBEN);
	cspm_write(CSPM_PCM_IM_PTR, 0);
	cspm_write(CSPM_PCM_IM_LEN, 0);

	/* clean wakeup event raw status */
	cspm_write(CSPM_WAKEUP_EVENT_MASK, ~0);

	/* clean IRQ status */
	cspm_write(CSPM_IRQ_MASK, IRQM_ALL);
	cspm_write(CSPM_IRQ_STA, IRQC_ALL);
	cspm_write(CSPM_SW_INT_CLEAR, PCM_SW_INT_ALL);
}

static void __cspm_reset_and_init_pcm(const struct pcm_desc *pcmdesc)
{
	/* do basic init because of Infra power-on reset */
#if 0
	/* others will set REGWR_EN for SEMA access */
	if (!(cspm_read(CSPM_POWERON_CONFIG_EN) & REGWR_EN))
#else
	if (!(cspm_read(CSPM_PCM_CON1) & CON1_MIF_APBEN))
#endif
		__cspm_register_init();

	/* reset PCM */
	__cspm_pcm_sw_reset();

	/* init PCM_CON0 */
	cspm_write(CSPM_PCM_CON0, CON0_CFG_KEY | CON0_PCM_CK_EN);

	/* init PCM_CON1 (disable PCM timer and PCM WDT) */
	cspm_write(CSPM_PCM_CON1, CON1_CFG_KEY | CON1_EVENT_LOCK_EN |
				  CON1_SPM_SRAM_ISO_B | CON1_SPM_SRAM_SLP_B |
				  (pcmdesc->replace ? 0 : CON1_IM_NONRP_EN) |
				  CON1_MIF_APBEN);
}

static void __cspm_kick_im_to_fetch(const struct pcm_desc *pcmdesc)
{
	u32 ptr, len, con0;

	/* tell IM where is PCM code (use slave mode if code existed) */
	ptr = base_va_to_pa(pcmdesc->base);
	len = pcmdesc->size - 1;
	if (cspm_read(CSPM_PCM_IM_PTR) != ptr || cspm_read(CSPM_PCM_IM_LEN) != len ||
	    pcmdesc->sess > 2) {
		cspm_write(CSPM_PCM_IM_PTR, ptr);
		cspm_write(CSPM_PCM_IM_LEN, len);
	} else {
		cspm_write(CSPM_PCM_CON1, cspm_read(CSPM_PCM_CON1) | CON1_CFG_KEY | CON1_IM_SLAVE);
	}

	/* kick IM to fetch (only toggle IM_KICK) */
	con0 = cspm_read(CSPM_PCM_CON0) & ~(CON0_IM_KICK | CON0_PCM_KICK);
	cspm_write(CSPM_PCM_CON0, con0 | CON0_CFG_KEY | CON0_IM_KICK);
	cspm_write(CSPM_PCM_CON0, con0 | CON0_CFG_KEY);
}

static void __cspm_init_pcm_register(void)
{
	/* init r7 with POWER_ON_VAL1 */
	cspm_write(CSPM_PCM_REG_DATA_INI, cspm_read(CSPM_POWER_ON_VAL1));
	cspm_write(CSPM_PCM_PWR_IO_EN, PCM_RF_SYNC_R7);
	cspm_write(CSPM_PCM_PWR_IO_EN, 0);
}

static void __cspm_init_event_vector(const struct pcm_desc *pcmdesc)
{
	/* init event vector register */
	cspm_write(CSPM_PCM_EVENT_VECTOR0, pcmdesc->vec0);
	cspm_write(CSPM_PCM_EVENT_VECTOR1, pcmdesc->vec1);

	/* disable event vector (enabled by FW) */
	cspm_write(CSPM_PCM_EVENT_VECTOR_EN, 0);
}

static void __cspm_set_wakeup_event(const struct pwr_ctrl *pwrctrl)
{
	/* start PCM timer (as free-run timer) */
	cspm_write(CSPM_PCM_TIMER_VAL, 0xffffffff);
	cspm_write(CSPM_PCM_CON1, cspm_read(CSPM_PCM_CON1) | CON1_CFG_KEY | CON1_PCM_TIMER_EN);

	/* unmask PCM wakeup source */
	cspm_write(CSPM_WAKEUP_EVENT_MASK, ~pwrctrl->wake_src);

	/* unmask CSPM IRQ */
	cspm_write(CSPM_IRQ_MASK, IRQM_ALL_EXC_RET0);
}

static void __cspm_kick_pcm_to_run(const struct pwr_ctrl *pwrctrl, const struct init_sta *sta)
{
	u32 con0;

	/* init register to match FW expectation */
	cspm_write(CSPM_SW_RSV4, SW_B_PAUSE |		/* must */
				 SW_B_F_MAX(NUM_CPU_OPP - 1) |
				 SW_B_F_MIN(0) |
				 SW_L_PAUSE |		/* must */
				 SW_L_F_MAX(NUM_CPU_OPP - 1) |
				 SW_L_F_MIN(0) |
				 BYPASS_FIT);		/* must */
	csram_write(OFFS_SW_RSV4, cspm_read(CSPM_SW_RSV4));

	cspm_write(CSPM_SW_RSV5, B_F_CURR(opp_sw_to_fw(sta->opp[CPU_CLUSTER_L])) |
				 B_F_DES(opp_sw_to_fw(sta->opp[CPU_CLUSTER_L])) |
				 B_V_CURR(sta->volt[CPU_CLUSTER_L]) |
				 L_F_CURR(opp_sw_to_fw(sta->opp[CPU_CLUSTER_LL])) |
				 L_F_DES(opp_sw_to_fw(sta->opp[CPU_CLUSTER_LL])) |
				 L_V_CURR(sta->volt[CPU_CLUSTER_LL]));
	csram_write(OFFS_FW_RSV5, cspm_read(CSPM_SW_RSV5));

	/* init variable to match FW state */
	pause_src_map |= PSF_PAUSE_INIT;
	csram_write(OFFS_PAUSE_SRC, pause_src_map);

	/* enable r7 to control power */
	cspm_write(CSPM_PCM_PWR_IO_EN, pwrctrl->r7_ctrl_en ? PCM_PWRIO_EN_R7 : 0);

	/*cspm_dbgx(KICK, "kick PCM to run, RSV4: 0x%x, RSV5: 0x%x\n",
			cspm_read(CSPM_SW_RSV4), cspm_read(CSPM_SW_RSV5));*/

	/* kick PCM to run (only toggle PCM_KICK) */
	con0 = cspm_read(CSPM_PCM_CON0) & ~(CON0_IM_KICK | CON0_PCM_KICK);
	cspm_write(CSPM_PCM_CON0, con0 | CON0_CFG_KEY | CON0_PCM_KICK);
	cspm_write(CSPM_PCM_CON0, con0 | CON0_CFG_KEY);
}

static void __cspm_save_curr_sta(struct init_sta *sta)
{
	csram_write_fw_sta();

	sta->opp[CPU_CLUSTER_LL] = opp_fw_to_sw(cspm_get_curr_freq_ll());
	sta->opp[CPU_CLUSTER_L] = opp_fw_to_sw(cspm_get_curr_freq_l());

	sta->volt[CPU_CLUSTER_LL] = cspm_get_curr_volt_ll();
	sta->volt[CPU_CLUSTER_L] = cspm_get_curr_volt_l();
}

static void __cspm_clean_after_pause(void)
{
	/* disable TWAM timer */
	cspm_write(CSPM_TWAM_CON, cspm_read(CSPM_TWAM_CON) & ~TWAM_ENABLE);

	/* disable r7 to control power */
	cspm_write(CSPM_PCM_PWR_IO_EN, 0);

	/* clean PCM timer event */
	cspm_write(CSPM_PCM_CON1, CON1_CFG_KEY | (cspm_read(CSPM_PCM_CON1) & ~CON1_PCM_TIMER_EN));

	/* clean wakeup event raw status */
	cspm_write(CSPM_WAKEUP_EVENT_MASK, ~0);

	/* clean IRQ status */
	cspm_write(CSPM_IRQ_MASK, IRQM_ALL);
	cspm_write(CSPM_IRQ_STA, IRQC_ALL);
	cspm_write(CSPM_SW_INT_CLEAR, PCM_SW_INT_ALL);
}

static void cspm_dump_debug_info(struct cpuhvfs_dvfsp *dvfsp, const char *fmt, ...)
{
	u32 timer, reg15, ap_sema, spm_sema;
	u32 rsv3, rsv4, rsv5;
	char msg[320];
	va_list args;

	timer = cspm_read(CSPM_PCM_TIMER_OUT);
	reg15 = cspm_read(CSPM_PCM_REG15_DATA);
	ap_sema = cspm_read(CSPM_AP_SEMA);
	spm_sema = cspm_read(CSPM_SPM_SEMA);

	rsv5 = cspm_read(CSPM_SW_RSV5);
	rsv3 = cspm_read(CSPM_SW_RSV3);
	rsv4 = cspm_read(CSPM_SW_RSV4);

	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	cspm_err("%s\n", msg);
	cspm_err("FW_VER: %s (%u)\n", dvfsp->pcmdesc->version, dvfsp->use_i2c);

	cspm_err("PCM_TIMER: %08x\n", timer);
	cspm_err("PCM_REG15: %u, SEMA: 0x(%x %x)\n", reg15, ap_sema, spm_sema);

	cspm_err("SW_RSV4: 0x%x\n", rsv4);
	cspm_err("SW_RSV5: 0x%x\n", rsv5);
	cspm_err("SW_RSV3: 0x%x\n", rsv3);

	cspm_err("SW_RSV2 : 0x%x\n", cspm_read(CSPM_SW_RSV2));
	cspm_err("SW_DEBUG: 0x%x\n", cspm_read(CSPM_SW_DEBUG));
	cspm_err("SW_FLAG : 0x%x\n", cspm_read(CSPM_SW_FLAG));

	cspm_err("PCM_FSM_STA: 0x%x\n", cspm_read(CSPM_PCM_FSM_STA));
}

static int __cspm_pause_pcm_running(struct cpuhvfs_dvfsp *dvfsp, u32 psf)
{
	int r = 0;

	/* @dvfsp may be uninitialized (PAUSE_INIT is set) */

	if (pause_src_map == 0) {
		cspm_write(CSPM_SW_RSV4, cspm_read(CSPM_SW_RSV4) | SW_ALL_PAUSE);
		csram_write(OFFS_SW_RSV4, cspm_read(CSPM_SW_RSV4));
		csram_write(OFFS_PAUSE_TIME, cspm_get_timestamp());

		udelay(10);	/* skip FW_DONE 1->0 transition */

		r = wait_complete_us(cspm_is_fw_all_done(), 10, PAUSE_TIMEOUT);
		csram_write_fw_sta();

		if (r >= 0) {	/* pause done */
			r = 0;
			if (dvfsp->use_i2c)
				clk_disable(i2c_clk);
		}
	}

	if (!r) {
		pause_src_map |= psf;
		if (csram_base)
			csram_write(OFFS_PAUSE_SRC, pause_src_map);

		if (cspm_base && psf == PSF_PAUSE_SUSPEND)
			__cspm_save_curr_sta(&suspend_sta);
	} else {
		cspm_dump_debug_info(dvfsp, "PAUSE TIMEOUT, psf = 0x%x", psf);
		BUG_ON(pause_fail_ke);
	}

	return r;
}

static void __cspm_unpause_pcm_to_run(struct cpuhvfs_dvfsp *dvfsp, u32 psf)
{
	int r;
	u32 rsv4;

	/* @dvfsp may be uninitialized (PAUSE_INIT is set) */

	pause_src_map &= ~psf;
	if (csram_base)
		csram_write(OFFS_PAUSE_SRC, pause_src_map);

	if (pause_src_map == 0 && csram_base /* avoid Coverity complaining */) {
		r = (dvfsp->use_i2c ? clk_enable(i2c_clk) : 0);
		if (!r) {
			rsv4 = cspm_read(CSPM_SW_RSV4);
			csram_write(OFFS_SW_RSV4, rsv4);

			if (rsv4 & L_CLUSTER_EN)
				rsv4 &= ~SW_L_PAUSE;
			if (rsv4 & B_CLUSTER_EN)
				rsv4 &= ~SW_B_PAUSE;
			BUG_ON((rsv4 & SW_ALL_PAUSE) == SW_ALL_PAUSE);	/* no cluster on!? */

			cspm_write(CSPM_SW_RSV4, rsv4);
			csram_write(OFFS_SW_RSV4, cspm_read(CSPM_SW_RSV4));
			csram_write(OFFS_UNPAUSE_TIME, cspm_get_timestamp());

			wake_up(&dvfs_wait);	/* for set_target_opp */
			csram_write(OFFS_DVFS_WAIT, 0);
		} else {
			cspm_err("FAILED TO ENABLE I2C CLOCK (%d)\n", r);
			BUG();
		}
	}
}

static int cspm_stop_pcm_running(struct cpuhvfs_dvfsp *dvfsp)
{
	int r;
	unsigned long flags;

	spin_lock(&dvfs_lock);
	cspm_dbgx(STOP, "[%08x] stop pcm\n", cspm_get_timestamp());

	r = __cspm_pause_pcm_running(dvfsp, PSF_PAUSE_INIT);
	if (!r) {
		spin_lock_irqsave(&cspm_lock, flags);

		__cspm_clean_after_pause();

		__cspm_pcm_sw_reset();

		spin_unlock_irqrestore(&cspm_lock, flags);
	}
	spin_unlock(&dvfs_lock);

	return r;
}

static int cspm_pause_pcm_running(struct cpuhvfs_dvfsp *dvfsp, enum pause_src src)
{
	int r;
	u32 psf = (1U << src);

	/* @dvfsp may be uninitialized (PAUSE_INIT is set) */

	spin_lock(&dvfs_lock);
	if (pause_log_en & psf)
		cspm_dbgx(PAUSE, "pause pcm, src = %u, map = 0x%x\n", src, pause_src_map);

	r = __cspm_pause_pcm_running(dvfsp, psf);
	spin_unlock(&dvfs_lock);

	return r;
}

static void cspm_unpause_pcm_to_run(struct cpuhvfs_dvfsp *dvfsp, enum pause_src src)
{
	u32 psf = (1U << src);

	/* @dvfsp may be uninitialized (PAUSE_INIT is set) */

	spin_lock(&dvfs_lock);
	if (pause_log_en & psf)
		cspm_dbgx(PAUSE, "unpause pcm, src = %u, map = 0x%x\n", src, pause_src_map);

	if (psf & pause_src_map)
		__cspm_unpause_pcm_to_run(dvfsp, psf);
	spin_unlock(&dvfs_lock);
}

static int cspm_get_semaphore(struct cpuhvfs_dvfsp *dvfsp, enum sema_user user)
{
	int i;
	int n = DIV_ROUND_UP(SEMA_GET_TIMEOUT, 10);

	if (user == SEMA_I2C_DRV)	/* workaround */
		return dvfsp->use_i2c ? cspm_pause_pcm_running(dvfsp, PAUSE_I2CDRV) : 0;

	if (is_dvfsp_uninit(dvfsp))
		return 0;

	cspm_dbgx(SEMA, "sema get, user = %u\n", user);

	cspm_write(CSPM_POWERON_CONFIG_EN, REGWR_CFG_KEY | REGWR_EN);	/* enable register write */

	for (i = 0; i < n; i++) {
		cspm_write(sema_reg[user], 0x1);
		if (cspm_read(sema_reg[user]) & 0x1)
			return 0;

		udelay(10);
	}

	cspm_dump_debug_info(dvfsp, "SEMA_USER%u GET TIMEOUT", user);
	BUG_ON(sema_fail_ke);

	return -EBUSY;
}

static void cspm_release_semaphore(struct cpuhvfs_dvfsp *dvfsp, enum sema_user user)
{
	if (user == SEMA_I2C_DRV) {	/* workaround */
		if (dvfsp->use_i2c)
			cspm_unpause_pcm_to_run(dvfsp, PAUSE_I2CDRV);
		return;
	}

	if (is_dvfsp_uninit(dvfsp))
		return;

	cspm_dbgx(SEMA, "sema release, user = %u\n", user);

	cspm_write(CSPM_POWERON_CONFIG_EN, REGWR_CFG_KEY | REGWR_EN);	/* enable register write */

	if (cspm_read(sema_reg[user]) & 0x1) {
		cspm_write(sema_reg[user], 0x1);
		BUG_ON(cspm_read(sema_reg[user]) & 0x1);	/* semaphore release failed */
	}
}

static int cspm_set_target_opp(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster, unsigned int index,
			       unsigned int *ret_volt)
{
	int r = 0;
	u32 f_des = opp_sw_to_fw(index);
	u32 rsv4, v;

	spin_lock(&dvfs_lock);
	csram_write(OFFS_FUNC_ENTER, (cluster << 24) | (index << 16) | FEF_DVFS);

	cspm_dbgx(DVFS, "cluster%u dvfs, opp = %u <%u>, pause = 0x%x\n",
			cluster, index, f_des, pause_src_map);

	while (pause_src_map != 0) {
		csram_write(OFFS_DVFS_WAIT, (cluster << 8) | index);
		spin_unlock(&dvfs_lock);

		wait_event(dvfs_wait, pause_src_map == 0);	/* wait for PCM to be unpaused */

		spin_lock(&dvfs_lock);
	}

	rsv4 = cspm_read(CSPM_SW_RSV4);
	csram_write(OFFS_SW_RSV4, rsv4);

	switch (cluster) {
	case CPU_CLUSTER_LL:
		if (!(rsv4 & SW_L_PAUSE)) {	/* cluster on */
			rsv4 &= ~SW_L_F_DES_MASK;
			cspm_write(CSPM_SW_RSV4, rsv4 | SW_L_F_ASSIGN | SW_L_F_DES(f_des));
			csram_write(OFFS_SW_RSV4, cspm_read(CSPM_SW_RSV4));

			r = wait_complete_us(cspm_get_curr_freq_ll() == f_des, 10, DVFS_TIMEOUT);
			csram_write_fw_sta();
		}

		v = cspm_get_curr_volt_ll();
		break;
	case CPU_CLUSTER_L:
	default:
		if (!(rsv4 & SW_B_PAUSE)) {	/* cluster on */
			rsv4 &= ~SW_B_F_DES_MASK;
			cspm_write(CSPM_SW_RSV4, rsv4 | SW_B_F_ASSIGN | SW_B_F_DES(f_des));
			csram_write(OFFS_SW_RSV4, cspm_read(CSPM_SW_RSV4));

			r = wait_complete_us(cspm_get_curr_freq_l() == f_des, 10, DVFS_TIMEOUT);
			csram_write_fw_sta();
		}

		v = cspm_get_curr_volt_l();
		break;
	}

	if (r >= 0) {	/* DVFS done */
		r = 0;
		if (ret_volt)
			*ret_volt = v;
	} else {
		cspm_dump_debug_info(dvfsp, "CLUSTER%u DVFS TIMEOUT, opp = %u <%u>",
					    cluster, index, f_des);
		BUG_ON(dvfs_fail_ke);
	}

	csram_write(OFFS_FUNC_ENTER, 0);
	spin_unlock(&dvfs_lock);

	return r;
}

static unsigned int cspm_get_curr_volt(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster)
{
	switch (cluster) {
	case CPU_CLUSTER_LL:
		return cspm_get_curr_volt_ll();
	case CPU_CLUSTER_L:
	default:
		return cspm_get_curr_volt_l();
	}

	return UINT_MAX;
}

static void cspm_cluster_notify_on(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster)
{
	u32 time, rsv4;

	spin_lock(&dvfs_lock);
	time = cspm_get_timestamp();
	rsv4 = cspm_read(CSPM_SW_RSV4);
	csram_write(OFFS_SW_RSV4, rsv4);

	cspm_dbgx(CLUSTER, "[%08x] cluster%u on, pause = 0x%x, RSV4: 0x%x\n",
			   time, cluster, pause_src_map, rsv4);

	switch (cluster) {
	case CPU_CLUSTER_LL:
		WARN_ON(!(rsv4 & SW_L_PAUSE));	/* not paused at cluster off */

		if (pause_src_map == 0)
			rsv4 &= ~SW_L_PAUSE;
		cspm_write(CSPM_SW_RSV4, rsv4 | L_CLUSTER_EN);
		csram_write(OFFS_SW_RSV4, cspm_read(CSPM_SW_RSV4));
		break;
	case CPU_CLUSTER_L:
	default:
		WARN_ON(!(rsv4 & SW_B_PAUSE));	/* not paused at cluster off */

		if (pause_src_map == 0)
			rsv4 &= ~SW_B_PAUSE;
		cspm_write(CSPM_SW_RSV4, rsv4 | B_CLUSTER_EN);
		csram_write(OFFS_SW_RSV4, cspm_read(CSPM_SW_RSV4));
		break;
	}

	if (PSF_PAUSE_INIT & pause_src_map) {
		BUG_ON(rsv4 & (B_CLUSTER_EN | L_CLUSTER_EN));	/* not right after kick */

		__cspm_unpause_pcm_to_run(dvfsp, PSF_PAUSE_INIT);
	}
	spin_unlock(&dvfs_lock);
}

static void cspm_cluster_notify_off(struct cpuhvfs_dvfsp *dvfsp, unsigned int cluster)
{
	int r;
	u32 time, rsv4;

	spin_lock(&dvfs_lock);
	csram_write(OFFS_FUNC_ENTER, (cluster << 24) | FEF_CLUSTER_OFF);

	time = cspm_get_timestamp();
	rsv4 = cspm_read(CSPM_SW_RSV4);
	csram_write(OFFS_SW_RSV4, rsv4);

	cspm_dbgx(CLUSTER, "[%08x] cluster%u off, pause = 0x%x, RSV4: 0x%x\n",
			   time, cluster, pause_src_map, rsv4);

	/* DFS only to the lowest frequency (no I2C control) */
	switch (cluster) {
	case CPU_CLUSTER_LL:
		if (WARN_ON(!(rsv4 & L_CLUSTER_EN)))	/* already off */
			goto out;

		cspm_write(CSPM_SW_RSV4, rsv4 & ~(L_CLUSTER_EN | SW_L_PAUSE | SW_L_F_ASSIGN));
		csram_write(OFFS_SW_RSV4, cspm_read(CSPM_SW_RSV4));

		/* FW will set SW_PAUSE when done */
		r = wait_complete_us(cspm_get_curr_freq_ll() == 0 && cspm_is_ll_paused(),
				     10, DVFS_TIMEOUT);
		csram_write_fw_sta();
		break;
	case CPU_CLUSTER_L:
	default:
		if (WARN_ON(!(rsv4 & B_CLUSTER_EN)))	/* already off */
			goto out;

		cspm_write(CSPM_SW_RSV4, rsv4 & ~(B_CLUSTER_EN | SW_B_PAUSE | SW_B_F_ASSIGN));
		csram_write(OFFS_SW_RSV4, cspm_read(CSPM_SW_RSV4));

		/* FW will set SW_PAUSE when done */
		r = wait_complete_us(cspm_get_curr_freq_l() == 0 && cspm_is_l_paused(),
				     10, DVFS_TIMEOUT);
		csram_write_fw_sta();
		break;
	}

	if (r < 0) {
		cspm_dump_debug_info(dvfsp, "CLUSTER%u OFF TIMEOUT", cluster);
		BUG();
	}

out:
	csram_write(OFFS_FUNC_ENTER, 0);
	spin_unlock(&dvfs_lock);
}

static bool cspm_is_pcm_kicked(struct cpuhvfs_dvfsp *dvfsp)
{
	return !!(cspm_read(CSPM_PCM_FSM_STA) & FSM_PCM_KICK);
}

static void __cspm_check_and_update_sta(struct init_sta *sta)
{
	int i;

	for (i = 0; i < NUM_CPU_CLUSTER; i++) {
		if (sta->opp[i] == OPP_AT_SUSPEND) {
			BUG_ON(suspend_sta.opp[i] == UINT_MAX);		/* without suspend */

			sta->opp[i] = suspend_sta.opp[i];
			suspend_sta.opp[i] = UINT_MAX;
		}
		if (sta->volt[i] == VOLT_AT_SUSPEND) {
			BUG_ON(suspend_sta.volt[i] == UINT_MAX);	/* without suspend */

			sta->volt[i] = suspend_sta.volt[i];
			suspend_sta.volt[i] = UINT_MAX;
		}

		csram_write(OFFS_INIT_OPP + i * sizeof(u32), sta->opp[i]);
		csram_write(OFFS_INIT_FREQ + i * sizeof(u32), sta->freq[i]);
		csram_write(OFFS_INIT_VOLT + i * sizeof(u32), sta->volt[i]);

		cspm_dbgx(KICK, "cluster%d: opp = %u, freq = %u, volt = 0x%x\n",
				i, sta->opp[i], sta->freq[i], sta->volt[i]);
	}
}

static int cspm_go_to_dvfs(struct cpuhvfs_dvfsp *dvfsp, struct init_sta *sta)
{
	unsigned long flags;
	struct pcm_desc *pcmdesc = dvfsp->pcmdesc;
	struct pwr_ctrl *pwrctrl = dvfsp->pwrctrl;

	spin_lock(&dvfs_lock);
	__cspm_check_and_update_sta(sta);

	spin_lock_irqsave(&cspm_lock, flags);

	__cspm_reset_and_init_pcm(pcmdesc);

	__cspm_kick_im_to_fetch(pcmdesc);

	__cspm_init_pcm_register();

	__cspm_init_event_vector(pcmdesc);

	__cspm_set_wakeup_event(pwrctrl);

	__cspm_kick_pcm_to_run(pwrctrl, sta);

	spin_unlock_irqrestore(&cspm_lock, flags);
	spin_unlock(&dvfs_lock);

	return 0;
}

static int cspm_probe(struct platform_device *pdev)
{
	int r;
	unsigned long flags;

	cspm_base = of_iomap(pdev->dev.of_node, 0);
	if (!cspm_base) {
		cspm_err("FAILED TO MAP CSPM REGISTER\n");
		return -ENOMEM;
	}

	csram_base = of_iomap(pdev->dev.of_node, 1);
	if (!csram_base) {
		cspm_err("FAILED TO MAP CSRAM MEMORY\n");
		return -ENOMEM;
	}

	i2c_clk = devm_clk_get(&pdev->dev, "i2c");
	if (IS_ERR(i2c_clk)) {
		cspm_err("FAILED TO GET I2C CLOCK (%ld)\n", PTR_ERR(i2c_clk));
		return PTR_ERR(i2c_clk);
	}
	r = clk_prepare(i2c_clk);
	if (r) {
		cspm_err("FAILED TO PREPARE I2C CLOCK (%d)\n", r);
		return r;
	}

	/* build register mapping for general access */
	sema_reg[SEMA_FHCTL_DRV] = CSPM_AP_SEMA;

	spin_lock_irqsave(&cspm_lock, flags);
	__cspm_register_init();
	spin_unlock_irqrestore(&cspm_lock, flags);

	cspm_probe_done = 1;

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cspm_of_match[] = {
	{ .compatible = "mediatek,mt6755-dvfsp", },
	{}
};
#endif

static struct platform_driver cspm_driver = {
	.probe	= cspm_probe,
	.driver	= {
		.name		= "cspm",
		.owner		= THIS_MODULE,
		.of_match_table	= of_match_ptr(cspm_of_match),
	},
};

static int cspm_module_init(struct cpuhvfs_dvfsp *dvfsp)
{
	int r;

	r = platform_driver_register(&cspm_driver);
	if (r) {
		cspm_err("FAILED TO REGISTER CSPM DRIVER (%d)\n", r);
		return r;
	}

	if (!cspm_probe_done) {
		cspm_err("FAILED TO PROBE CSPM DEVICE\n");
		return -ENODEV;
	}

	dvfsp->log_repo = csram_base;

	return 0;
}


/**************************************
 * [Hybrid DVFS] Function
 **************************************/
static int dvfsp_fw_show(struct seq_file *m, void *v)
{
	struct cpuhvfs_dvfsp *dvfsp = m->private;
	struct pcm_desc *pcmdesc = dvfsp->pcmdesc;

	seq_printf(m, "version = %s (%u)\n", pcmdesc->version, dvfsp->use_i2c);
	seq_printf(m, "base    = 0x%p -> 0x%x\n", pcmdesc->base, base_va_to_pa(pcmdesc->base));
	seq_printf(m, "size    = %u\n"  , pcmdesc->size);
	seq_printf(m, "sess    = %u\n"  , pcmdesc->sess);
	seq_printf(m, "replace = %u\n"  , pcmdesc->replace);
	seq_printf(m, "vec0    = 0x%x\n", pcmdesc->vec0);
	seq_printf(m, "vec1    = 0x%x\n", pcmdesc->vec1);

	return 0;
}

static int dvfsp_reg_show(struct seq_file *m, void *v)
{
	seq_printf(m, "PCM_TIMER  : %08x\n", cspm_read(CSPM_PCM_TIMER_OUT));
	seq_printf(m, "PCM_REG15  : %u\n"  , cspm_read(CSPM_PCM_REG15_DATA));
	seq_puts(m,   "============\n");
	seq_printf(m, "SW_RSV4    : 0x%x\n", cspm_read(CSPM_SW_RSV4));
	seq_printf(m, "SW_RSV5    : 0x%x\n", cspm_read(CSPM_SW_RSV5));
	seq_printf(m, "SW_RSV3    : 0x%x\n", cspm_read(CSPM_SW_RSV3));
	seq_puts(m,   "============\n");
	seq_printf(m, "SW_RSV2    : 0x%x\n", cspm_read(CSPM_SW_RSV2));
	seq_printf(m, "SW_DEBUG   : 0x%x\n", cspm_read(CSPM_SW_DEBUG));
	seq_printf(m, "SW_FLAG    : 0x%x\n", cspm_read(CSPM_SW_FLAG));
	seq_puts(m,   "============\n");
	seq_printf(m, "PCM_FSM_STA: 0x%x\n", cspm_read(CSPM_PCM_FSM_STA));

	return 0;
}

static int dbg_repo_show(struct seq_file *m, void *v)
{
	int i;
	u32 *repo = m->private;
	char ch;

	/* only save current status to debug repo */
	if (repo != dbg_repo_bak) {
		repo[REPO_I_WDT_LATCH0] = cspm_read(CSPM_PCM_FSM_STA);
		repo[REPO_I_WDT_LATCH1] = cspm_read(CSPM_PCM_REG6_DATA);
		repo[REPO_I_WDT_LATCH2] = cspm_read(CSPM_SW_RSV5);
	}

	for (i = 0; i < DBG_REPO_NUM; i++) {
		if (i >= REPO_I_LOG_S && (i - REPO_I_LOG_S) % ENTRY_EACH_LOG == 0)
			ch = ':';	/* timestamp */
		else
			ch = '.';

		seq_printf(m, "%4d%c%08x%c", i, ch, repo[i], i % 4 == 3 ? '\n' : ' ');
	}

	return 0;
}

DEFINE_FOPS_RO(dvfsp_fw);
DEFINE_FOPS_RO(dvfsp_reg);
DEFINE_FOPS_RO(dbg_repo);

static int create_cpuhvfs_debug_fs(struct cpuhvfs_data *cpuhvfs)
{
	struct dentry *root;

	/* create /sys/kernel/debug/cpuhvfs */
	root = debugfs_create_dir("cpuhvfs", NULL);
	if (!root)
		return -EPERM;

	debugfs_create_file("dvfsp_fw", 0444, root, cpuhvfs->dvfsp, &dvfsp_fw_fops);
	debugfs_create_file("dvfsp_reg", 0444, root, cpuhvfs->dvfsp, &dvfsp_reg_fops);

	debugfs_create_file("dbg_repo", 0444, root, cpuhvfs->dbg_repo, &dbg_repo_fops);
	debugfs_create_file("dbg_repo_bak", 0444, root, dbg_repo_bak, &dbg_repo_fops);

	debugfs_create_x32("pause_src_map", 0444, root, &pause_src_map);
	debugfs_create_x32("pause_log_en", 0644, root, &pause_log_en);

	debugfs_create_x32("dbgx_log_en", 0644, root, &dbgx_log_en);

	debugfs_create_bool("dvfs_fail_ke", 0644, root, &dvfs_fail_ke);
	debugfs_create_bool("sema_fail_ke", 0644, root, &sema_fail_ke);
	debugfs_create_bool("pause_fail_ke", 0644, root, &pause_fail_ke);

	cpuhvfs->dbg_fs = root;

	return 0;
}

static void init_cpuhvfs_debug_repo(struct cpuhvfs_data *cpuhvfs)
{
	u32 __iomem *dbg_repo = cpuhvfs->dvfsp->log_repo;

	/* backup debug repo for later analysis */
	memcpy_fromio(dbg_repo_bak, dbg_repo, DBG_REPO_SIZE);
	dbg_repo_bak[REPO_I_WDT_LATCH0] = cspm_read(CSPM_PCM_WDT_LATCH0);
	dbg_repo_bak[REPO_I_WDT_LATCH1] = cspm_read(CSPM_PCM_WDT_LATCH1);
	dbg_repo_bak[REPO_I_WDT_LATCH2] = cspm_read(CSPM_PCM_WDT_LATCH2);
	cpuhvfs_crit("WDT_LATCH0: 0x%x, LATCH1: 0x%x, LATCH2: 0x%x\n",
		     dbg_repo_bak[REPO_I_WDT_LATCH0],
		     dbg_repo_bak[REPO_I_WDT_LATCH1],
		     dbg_repo_bak[REPO_I_WDT_LATCH2]);

	dbg_repo[0] = REPO_GUARD0;
	dbg_repo[1] = REPO_GUARD1;
	dbg_repo[2] = REPO_GUARD0;
	dbg_repo[3] = REPO_GUARD1;

	memset_io((void __iomem *)dbg_repo + DBG_REPO_DATA_S - DBG_REPO_S,
		  0,
		  DBG_REPO_E - DBG_REPO_DATA_S);

	dbg_repo[REPO_I_DATA_S] = REPO_GUARD0;

	cpuhvfs->dbg_repo = dbg_repo;
}


/**************************************
 * [Hybrid DVFS] API
 **************************************/
void cpuhvfs_dump_dvfsp_info(void)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (is_dvfsp_uninit(dvfsp))
		return;

	dvfsp->dump_info(dvfsp, "DVFSP INFO DUMP");
}

/**
 * Should be called at syscore_suspend stage (1 CPU/1 cluster)
 */
int cpuhvfs_dvfsp_suspend(void)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (is_dvfsp_uninit(dvfsp))
		return 0;

	return dvfsp->pause_dvfsp(dvfsp, PAUSE_SUSPEND);
}

/**
 * Should be called at syscore_resume stage (1 CPU/1 cluster)
 */
void cpuhvfs_dvfsp_resume(unsigned int on_cluster, struct init_sta *sta)
{
	int r;
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (on_cluster >= NUM_CPU_CLUSTER || !sta)
		return;

	if (is_dvfsp_uninit(dvfsp))
		return;

	if (!dvfsp->is_kicked(dvfsp)) {
		r = dvfsp->kick_dvfsp(dvfsp, sta);
		BUG_ON(r);

		dvfsp->cluster_on(dvfsp, on_cluster);
	}

	dvfsp->unpause_dvfsp(dvfsp, PAUSE_SUSPEND);
}

int cpuhvfs_stop_dvfsp_running(void)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (is_dvfsp_uninit(dvfsp))
		return 0;

	return dvfsp->stop_dvfsp(dvfsp);
}

int cpuhvfs_restart_dvfsp_running(struct init_sta *sta)
{
	int r, i;
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (!sta)
		return -EINVAL;

	if (is_dvfsp_uninit(dvfsp))
		return -ENODEV;

	if (dvfsp->is_kicked(dvfsp))
		return -EPERM;

	r = dvfsp->kick_dvfsp(dvfsp, sta);
	if (r)
		return r;

	for (i = 0; i < NUM_CPU_CLUSTER; i++) {
		if (sta->is_on[i])
			dvfsp->cluster_on(dvfsp, i);
	}

	return 0;
}

int cpuhvfs_pause_dvfsp_running(enum pause_src src)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (src >= NUM_PAUSE_SRC || src == PAUSE_INIT || src == PAUSE_SUSPEND)
		return -EINVAL;

	return dvfsp->pause_dvfsp(dvfsp, src);
}

void cpuhvfs_unpause_dvfsp_to_run(enum pause_src src)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (src >= NUM_PAUSE_SRC || src == PAUSE_INIT || src == PAUSE_SUSPEND)
		return;

	dvfsp->unpause_dvfsp(dvfsp, src);
}

int cpuhvfs_get_dvfsp_semaphore(enum sema_user user)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (user >= NUM_SEMA_USER)
		return -EINVAL;

	return dvfsp->get_sema(dvfsp, user);
}

void cpuhvfs_release_dvfsp_semaphore(enum sema_user user)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (user >= NUM_SEMA_USER)
		return;

	dvfsp->release_sema(dvfsp, user);
}

/**
 * Current voltage (PMIC value) will be saved in @ret_volt if succeed
 */
int cpuhvfs_set_target_opp(unsigned int cluster, unsigned int index, unsigned int *ret_volt)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (cluster >= NUM_CPU_CLUSTER || index >= NUM_CPU_OPP)
		return -EINVAL;

	if (is_dvfsp_uninit(dvfsp))
		return -ENODEV;

	if (!dvfsp->is_kicked(dvfsp)) {
		cpuhvfs_err("CANNOT SET OPP, DVFSP IS NOT KICKED\n");
		return -EPERM;
	}

	return dvfsp->set_target(dvfsp, cluster, index, ret_volt);
}

/**
 * Get current voltage (PMIC value) from DVFSP. Return UINT_MAX if unavailable
 */
unsigned int cpuhvfs_get_curr_volt(unsigned int cluster)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (cluster >= NUM_CPU_CLUSTER)
		return UINT_MAX;

	if (is_dvfsp_uninit(dvfsp))
		return UINT_MAX;

	if (!dvfsp->is_kicked(dvfsp))
		return UINT_MAX;

	return dvfsp->get_volt(dvfsp, cluster);
}

void cpuhvfs_notify_cluster_on(unsigned int cluster)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (cluster >= NUM_CPU_CLUSTER)
		return;

	if (is_dvfsp_uninit(dvfsp))
		return;

	if (!dvfsp->is_kicked(dvfsp))
		return;

	dvfsp->cluster_on(dvfsp, cluster);
}

/**
 * Require scaling to the lowest frequency for cluster off
 */
void cpuhvfs_notify_cluster_off(unsigned int cluster)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	BUG_ON(cluster >= NUM_CPU_CLUSTER);
	BUG_ON(is_dvfsp_uninit(dvfsp));
	BUG_ON(!dvfsp->is_kicked(dvfsp));

	dvfsp->cluster_off(dvfsp, cluster);
}

int cpuhvfs_kick_dvfsp_to_run(struct init_sta *sta)
{
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (!sta)
		return -EINVAL;

	if (is_dvfsp_uninit(dvfsp))
		return -ENODEV;

	if (dvfsp->is_kicked(dvfsp))
		return 0;

	return dvfsp->kick_dvfsp(dvfsp, sta);
}

int cpuhvfs_module_init(void)
{
	int r;
	struct cpuhvfs_data *cpuhvfs = &g_cpuhvfs;
	struct cpuhvfs_dvfsp *dvfsp = g_cpuhvfs.dvfsp;

	if (!cpuhvfs->dbg_repo) {
		cpuhvfs_err("FAILED TO PRE-INIT CPUHVFS\n");
		return -ENODEV;
	}

#ifdef CONFIG_MTK_PMIC_CHIP_MT6353
	if (!is_ext_buck_exist() || !is_ext_buck_sw_ready()) {
		dvfsp->pcmdesc = &dvfs_pcm;
		dvfsp->use_i2c = 0;
	}
#endif

	cpuhvfs_crit("ext_buck = %d/%d, dvfsp_fw = %s, use_i2c = %u\n",
		     is_ext_buck_exist(), is_ext_buck_sw_ready(),
		     dvfsp->pcmdesc->version, dvfsp->use_i2c);

	set_dvfsp_init_done(dvfsp);

	r = create_cpuhvfs_debug_fs(cpuhvfs);
	if (r) {
		cpuhvfs_err("FAILED TO CREATE DEBUG FILESYSTEM (%d)\n", r);
		return r;
	}

	return 0;
}

static int cpuhvfs_pre_module_init(void)
{
	int r;
	struct cpuhvfs_data *cpuhvfs = &g_cpuhvfs;

	r = cpuhvfs->dvfsp->init_dvfsp(cpuhvfs->dvfsp);
	if (r) {
		cpuhvfs_err("FAILED TO INIT DVFS PROCESSOR (%d)\n", r);
		return r;
	}

	init_cpuhvfs_debug_repo(cpuhvfs);

	return 0;
}
fs_initcall(cpuhvfs_pre_module_init);

#endif	/* CONFIG_HYBRID_CPU_DVFS */

MODULE_DESCRIPTION("Hybrid CPU DVFS Driver v0.7.1");
