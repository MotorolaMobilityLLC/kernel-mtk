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
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/types.h>

#ifdef CONFIG_MTK_HIBERNATION
#include <mtk_hibernate_dpm.h>
#endif

/* CCF */
#include <linux/clk.h>

#include "mtk_io.h"
#include "sync_write.h"
#include "devapc.h"


/* 0 for early porting */
#define DEVAPC_TURN_ON         0
#define DEVAPC_USE_CCF         1

/* Debug message event */
#define DEVAPC_LOG_NONE        0x00000000
#define DEVAPC_LOG_ERR         0x00000001
#define DEVAPC_LOG_WARN        0x00000002
#define DEVAPC_LOG_INFO        0x00000004
#define DEVAPC_LOG_DBG         0x00000008

#define DEVAPC_LOG_LEVEL      (DEVAPC_LOG_DBG)

#define DEVAPC_MSG(fmt, args...) \
	do {    \
		if (DEVAPC_LOG_LEVEL & DEVAPC_LOG_DBG) { \
			pr_debug(fmt, ##args); \
		} else if (DEVAPC_LOG_LEVEL & DEVAPC_LOG_INFO) { \
			pr_info(fmt, ##args); \
		} else if (DEVAPC_LOG_LEVEL & DEVAPC_LOG_WARN) { \
			pr_warn(fmt, ##args); \
		} else if (DEVAPC_LOG_LEVEL & DEVAPC_LOG_ERR) { \
			pr_err(fmt, ##args); \
		} \
	} while (0)


#define DEVAPC_VIO_LEVEL      (DEVAPC_LOG_ERR)

#define DEVAPC_VIO_MSG(fmt, args...) \
	do {    \
		if (DEVAPC_VIO_LEVEL & DEVAPC_LOG_DBG) { \
			pr_debug(fmt, ##args); \
		} else if (DEVAPC_VIO_LEVEL & DEVAPC_LOG_INFO) { \
			pr_info(fmt, ##args); \
		} else if (DEVAPC_VIO_LEVEL & DEVAPC_LOG_WARN) { \
			pr_warn(fmt, ##args); \
		} else if (DEVAPC_VIO_LEVEL & DEVAPC_LOG_ERR) { \
			pr_err(fmt, ##args); \
		} \
	} while (0)



/* bypass clock! */
#if DEVAPC_USE_CCF
static struct clk *dapc_infra_clk;
#endif

static struct cdev *g_devapc_ctrl;
static unsigned int devapc_infra_irq;
static void __iomem *devapc_pd_infra_base;

static unsigned int enable_dynamic_one_core_violation_debug;

#if DEVAPC_TURN_ON
static struct DEVICE_INFO devapc_infra_devices[] = {
	/* device name                          enable_vio_irq */

	/* 0 */
	{"INFRA_AO_TOPCKGEN",                     true    },
	{"INFRA_AO_INFRASYS_CONFIG_REGS",         true    },
	{"INFRA_AO_RESERVE",                      true    },
	{"INFRA_AO_PERICFG",                      true    },
	{"INFRA_AO_EFUSE_AO_DEBUG",               true    },
	{"INFRA_AO_GPIO",                         true    },
	{"INFRA_AO_SLEEP_CONTROLLER",             true    },
	{"INFRA_AO_TOPRGU",                       true    },
	{"INFRA_AO_APXGPT",                       true    },
	{"INFRA_AO_RESERVE",                      true    },

	/* 10 */
	{"INFRA_AO_SEJ",                          true    },
	{"INFRA_AO_AP_CIRQ_EINT",                 true    },
	{"INFRA_AO_APMIXEDSYS",                   true    },
	{"INFRA_AO_PMIC_WRAP",                    true    },
	{"INFRA_AO_DEVICE_APC_AO_INFRA_PERI",     true    },
	{"INFRA_AO_SLEEP_CONTROLLER_MD",          true    },
	{"INFRA_AO_KEYPAD",                       true    },
	{"INFRA_AO_TOP_MISC",                     true    },
	{"INFRA_AO_DVFS_CTRL_PROC",               true    },
	{"INFRA_AO_MBIST_AO_REG",                 true    },

	/* 20 */
	{"INFRA_AO_CLDMA_AO_AP",                  true    },
	{"INFRA_AO_RESERVE",                      true    },
	{"INFRA_AO_AES_TOP_0",                    true    },
	{"INFRA_AO_SYS_TIMER",                    true    },
	{"INFRA_AO_MDEM_TEMP_SHARE",              true    },
	{"INFRA_AO_DEVICE_APC_AO_MD",             true    },
	{"INFRA_AO_SECURITY_AO",                  true    },
	{"INFRA_AO_TOPCKGEN_REG",                 true    },
	{"INFRA_AO_DEVICE_APC_AO_MM",             true    },
	{"INFRASYS_RESERVE",                      true    },

	/* 30 */
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_SYS_CIRQ",                     true    },
	{"INFRASYS_MM_IOMMU",                     true    },
	{"INFRASYS_EFUSE_PDN_DEBUG",              true    },
	{"INFRASYS_DEVICE_APC",                   true    },
	{"INFRASYS_DBG_TRACKER",                  true    },
	{"INFRASYS_CCIF0_AP",                     true    },
	{"INFRASYS_CCIF0_MD",                     true    },

	/* 40 */
	{"INFRASYS_CCIF1_MD",                     true    },
	{"INFRASYS_CLDMA_MD",                     true    },
	{"INFRASYS_MBIST",                        true    },
	{"INFRASYS_INFRA_PDN_REGISTER",           true    },
	{"INFRASYS_TRNG",                         true    },
	{"INFRASYS_DX_CC",                        true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_CQ_DMA",                       true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_SRAMROM",                      true    },

	/* 50 */
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_EMI",                          true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_CLDMA_PDN",                    true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },

	/* 60 */
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_EMI_MPU",                      true    },
	{"INFRASYS_DVFS_PROC",                    true    },
	{"INFRASYS_DRAMC_CH0_TOP0_CONFIGURATION", true    },
	{"INFRASYS_DRAMC_CH0_TOP1_CONFIGURATION", true    },
	{"INFRASYS_DRAMC_CH0_TOP2_CONFIGURATION", true    },
	{"INFRASYS_DRAMC_CH0_TOP3_CONFIGURATION", true    },
	{"INFRASYS_DRAMC_CH0_TOP4_CONFIGURATION", true    },
	{"INFRASYS_DRAMC_CH1_TOP0_CONFIGURATION", true    },

	/* 70 */
	{"INFRASYS_DRAMC_CH1_TOP1_CONFIGURATION", true    },
	{"INFRASYS_DRAMC_CH1_TOP2_CONFIGURATION", true    },
	{"INFRASYS_DRAMC_CH1_TOP3_CONFIGURATION", true    },
	{"INFRASYS_DRAMC_CH1_TOP4_CONFIGURATION", true    },
	{"INFRASYS_GCE",                          true    },
	{"INFRA_AO_PWRMCU_PARTITION_1",           true    },
	{"INFRA_AO_PWRMCU_PARTITION_2",           true    },
	{"INFRA_AO_PWRMCU_PARTITION_3",           true    },
	{"INFRA_AO_PWRMCU_PARTITION_4",           true    },
	{"INFRA_AO_PWRMCU_PARTITION_5",           true    },

	/* 80 */
	{"INFRA_AO_PWRMCU_PARTITION_6",           true    },
	{"INFRA_AO_PWRMCU_PARTITION_7",           true    },
	{"INFRA_AO_PWRMCU_PARTITION_8",           true    },
	{"INFRA_AO_MCUCFG",                       true    },
	{"INFRASYS_DBUGSYS",                      true    },
	{"PERISYS_APDMA",                         true    },
	{"PERISYS_AUXADC",                        true    },
	{"PERISYS_UART0",                         true    },
	{"PERISYS_UART1",                         true    },
	{"PERISYS_UART2",                         true    },

	/* 90 */
	{"PERISYS_UART3",                         true    },
	{"PERISYS_PWM",                           true    },
	{"PERISYS_I2C0",                          true    },
	{"PERISYS_I2C1",                          true    },
	{"PERISYS_I2C2",                          true    },
	{"PERISYS_SPI0",                          true    },
	{"PERISYS_PTP",                           true    },
	{"PERISYS_BTIF",                          true    },
	{"PERISYS_IRTX",                          true    },
	{"PERISYS_DISP_PWM",                      true    },

	/* 100 */
	{"PERISYS_I2C3",                          true    },
	{"PERISYS_SPI1",                          true    },
	{"PERISYS_I2C4",                          true    },
	{"PERISYS_SPI2",                          true    },
	{"PERISYS_SPI3",                          true    },
	{"PERISYS_I2C1_IMM",                      true    },
	{"PERISYS_I2C2_IMM",                      true    },
	{"PERISYS_I2C5",                          true    },
	{"PERISYS_I2C5_IMM",                      true    },
	{"PERISYS_SPI4",                          true    },

	/* 110 */
	{"PERISYS_SPI5",                          true    },
	{"PERISYS_USB",                           true    },
	{"PERISYS_USB_2.0_SUB",                   true    },
	{"PERISYS_MSDC0",                         true    },
	{"PERISYS_MSDC1",                         true    },
	{"PERISYS_MSDC2",                         true    },
	{"PERISYS_MSDC3",                         true    },
	{"PERISYS_UFS",                           true    },
	{"PERISUS_USB3.0_SIF",                    true    },
	{"PERISUS_USB3.0_SIF2",                   true    },

	/* 120 */
	{"PERISYS_RESERVE",                       true    },
	{"PERISYS_AUDIO",                         true    },
	{"EAST_RESERVE_0",                        true    },
	{"EAST_RESERVE_1",                        true    },
	{"EAST_RESERVE_2",                        true    },
	{"EAST_RESERVE_3",                        true    },
	{"EAST_RESERVE_4",                        true    },
	{"EAST_IO_CFG_RT",                        true    },
	{"EAST_IO_CFG_RM",                        true    },
	{"EAST_RESERVE_7",                        true    },

	/* 130 */
	{"EAST_CSI0_TOP_AO",                      true    },
	{"EAST_CSI1_TOP_AO",                      true    },
	{"EAST_RESERVE_A",                        true    },
	{"EAST_RESERVE_B",                        true    },
	{"EAST_RESERVE_C",                        true    },
	{"EAST_RESERVE_D",                        true    },
	{"EAST_RESERVE_E",                        true    },
	{"EAST_RESERVE_F",                        true    },
	{"SOUTH_RESERVE_0",                       true    },
	{"SOUTH_RESERVE_1",                       true    },

	/* 140 */
	{"SOUTH_RESERVE_2",                       true    },
	{"SOUTH_IO_CFG_RB",                       true    },
	{"SOUTH_RESERVE_4",                       true    },
	{"SOUTH_RESERVE_5",                       true    },
	{"SOUTH_RESERVE_6",                       true    },
	{"SOUTH_RESERVE_7",                       true    },
	{"SOUTH_RESERVE_8",                       true    },
	{"SOUTH_RESERVE_9",                       true    },
	{"SOUTH_RESERVE_A",                       true    },
	{"SOUTH_RESERVE_B",                       true    },

	/* 150 */
	{"SOUTH_RESERVE_C",                       true    },
	{"SOUTH_RESERVE_D",                       true    },
	{"SOUTH_RESERVE_E",                       true    },
	{"SOUTH_RESERVE_F",                       true    },
	{"WEST_RESERVE_0",                        true    },
	{"WEST_MSDC1_PAD_MACRO",                  true    },
	{"WEST_PCIE_PHYD",                        true    },
	{"WEST_ANA_HDMI",                         true    },
	{"WEST_RESERVE_4",                        true    },
	{"WEST_MIPI_TX_CONFIG",                   true    },

	/* 160 */
	{"WEST_MIPI_TX_CONFIG",                   true    },
	{"WEST_IO_CFG_LB",                        true    },
	{"WEST_IO_CFG_LM",                        true    },
	{"WEST_RESERVE_9",                        true    },
	{"WEST_RESERVE_A",                        true    },
	{"WEST_RESERVE_B",                        true    },
	{"WEST_RESERVE_C",                        true    },
	{"WEST_RESERVE_D",                        true    },
	{"WEST_RESERVE_E",                        true    },
	{"WEST_RESERVE_F",                        true    },

	/* 170 */
	{"NORTH_RESERVE_0",                       true    },
	{"NORTH_EFUSE",                           true    },
	{"NORTH_IO_CFG_LT",                       true    },
	{"NORTH_IO_CFG_TL",                       true    },
	{"NORTH_USB20_PHY",                       true    },
	{"NORTH_MSDC0_PAD_MACRO",                 true    },
	{"NORTH_RESERVE_6",                       true    },
	{"NORTH_RESERVE_7",                       true    },
	{"NORTH_RESERVE_8",                       true    },
	{"NORTH_RESERVE_9",                       true    },

	/* 180 */
	{"NORTH_UFS_MPHY",                        true    },
	{"NORTH_RESERVE_B",                       true    },
	{"NORTH_RESERVE_C",                       true    },
	{"NORTH_RESERVE_D",                       true    },
	{"NORTH_RESERVE_E",                       true    },
	{"NORTH_RESERVE_F",                       true    },
	{"PERISYS_CONN",                          true    },
	{"PERISYS_RESERVE",                       true    },
	{"PERISYS_RESERVE",                       true    },
	{"G3D_CONFIG",                            true    },

	/* 190 */
	{"MFG_VAD",                               true    },
	{"SC0_VAD",                               true    },
	{"MFG_OTHERS",                            true    },
	{"MMSYS_CONFIG",                          true    },
	{"MDP_RDMA0",                             true    },
	{"MDP_RDMA1",                             true    },
	{"MDP_RSZ0",                              true    },
	{"MDP_RSZ1",                              true    },
	{"MDP_WROT0",                             true    },
	{"MDP_WDMA",                              true    },

	/* 200 */
	{"MDP_TDSHP",                             true    },
	{"DISP_OVL0",                             true    },
	{"DISP_OVL0_2L",                          true    },
	{"DISP_OVL1_2L",                          true    },
	{"DISP_RDMA0",                            true    },
	{"DISP_RDMA1",                            true    },
	{"DISP_WDMA0",                            true    },
	{"DISP_COLOR0",                           true    },
	{"DISP_CCORR0",                           true    },
	{"DISP_AAL0",                             true    },

	/* 210 */
	{"DISP_GAMMA0",                           true    },
	{"DISP_DITHER0",                          true    },
	{"DSI_SPLIT",                             true    },
	{"DSI0",                                  true    },
	{"DPI",                                   true    },
	{"MM_MUTEX",                              true    },
	{"SMI_LARB0",                             true    },
	{"SMI_LARB1",                             true    },
	{"SMI_COMMON",                            true    },
	{"IMGSYS_CONFIG",                         true    },

	/* 220 */
	{"IMGSYS_SMI_LARB1",                      true    },
	{"IMGSYS_DISP_A0",                        true    },
	{"IMGSYS_DISP_A1",                        true    },
	{"IMGSYS_DISP_A2",                        true    },
	{"IMGSYS_RESERVE",                        true    },
	{"IMGSYS_RESERVE",                        true    },
	{"IMGSYS_VAD",                            true    },
	{"IMGSYS_DPE",                            true    },
	{"IMGSYS_RSC",                            true    },
	{"IMGSYS_RESERVE",                        true    },

	/* 230 */
	{"IMGSYS_FDVT",                           true    },
	{"IMGSYS_GEPF",                           true    },
	{"IMGSYS_RESERVE",                        true    },
	{"IMGSYS_DFP",                            true    },
	{"IMGSYS_RESERVE",                        true    },
	{"VCODESYS_VENC_GLOBAL_CON",              true    },
	{"VCODESYS_SMI_LARB3",                    true    },
	{"VCODESYS_VENC",                         true    },
	{"VCODESYS_JPGENC",                       true    },
	{"VCODESYS_VDEC_FULL_TOP",                true    },

	/* 240 */
	{"VCODESYS_MBIST_CTRL",                   true    },
	{"CAMSYS_CAMSYS_TOP",                     true    },
	{"CAMSYS_LARB2",                          true    },
	{"CAMSYS_CAM_TOP",                        true    },
	{"CAMSYS_CAM_A",                          true    },
	{"CAMSYS_CAM_B",                          true    },
	{"CAMSYS_CAM_TOP_SET",                    true    },
	{"CAMSYS_CAM_A_SET",                      true    },
	{"CAMSYS_CAM_B_SET",                      true    },
	{"CAMSYS_CAM_TOP_INNER",                  true    },

	/* 250 */
	{"CAMSYS_CAM_A_INNER",                    true    },
	{"CAMSYS_CAM_B_INNER",                    true    },
	{"CAMSYS_CAM_TOP_CLR",                    true    },
	{"CAMSYS_CAM_A_CLR",                      true    },
	{"CAMSYS_CAM_B_CLR",                      true    },
	{"CAMSYS_SENINF_A",                       true    },
	{"CAMSYS_SENINF_B",                       true    },
	{"CAMSYS_SENINF_C",                       true    },
	{"CAMSYS_SENINF_D",                       true    },
	{"CAMSYS_SENINF_E",                       true    },

	/* 260 */
	{"CAMSYS_SENINF_F",                       true    },
	{"CAMSYS_SENINF_G",                       true    },
	{"CAMSYS_SENINF_H",                       true    },
	{"CAMSYS_CAMSV_A",                        true    },
	{"CAMSYS_CAMSV_B",                        true    },
	{"CAMSYS_CAMSV_C",                        true    },
	{"CAMSYS_CAMSV_D",                        true    },
	{"CAMSYS_CAMSV_E",                        true    },
	{"CAMSYS_CAMSV_F",                        true    },
	{"CAMSYS_OTHERS_0",                       true    },

	/* 270 */
	{"CAMSYS_MD32_DMEM",                      true    },
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_MD32_PMEM",                      true    },
	{"CAMSYS_MD32_IP",                        true    },
	{"CAMSYS_CCU_DMA_TSF",                    true    },
	{"PWR_MCU_UNALIGN",                       true    },
	{"PWR_MCU_OUT_OF_BOUND",                  true    },
	{"PWR_MCU_ERR_WAY_EN",                    true    },
	{"EAST_PERIAPB_UNALIGN",                  true    },
	{"EAST_PERIAPB_OUT_OF_BOUND",             true    },

	/* 280 */
	{"EAST_PERIAPB_ERR_WAY_EN",               true    },
	{"SOUTH_PERIAPB_UNALIGN",                 true    },
	{"SOUTH_PERIAPB_OUT_OF_BOUND",            true    },
	{"SOUTH_PERIAPB_ERR_WAY_EN",              true    },
	{"WEST_PERIAPB_UNALIGN",                  true    },
	{"WEST_PERIAPB_OUT_OF_BOUND",             true    },
	{"WEST_PERIAPB_ERR_WAY_EN",               true    },
	{"NORTH_PERIAPB_UNALIGN",                 true    },
	{"NORTH_PERIAPB_OUT_OF_BOUND",            true    },
	{"NORTH_PERIAPB_ERR_WAY_EN",              true    },

	/* 290 */
	{"INFRA_PDN_DECODE_ERROR",                true    },
	{"TOPAXI_SI2_DECERR",                     true    },
	{"TOPAXI_SI1_DECERR",                     true    },
	{"TOPAXI_SI0_DECERR",                     true    },
	{"PERIAXI_SI0_DECERR",                    true    },
	{"PERIAXI_SI1_DECERR",                    true    },
	{"SRAMROM",                               false   },
	{"AP_DMA",                                false   },
	{"DEVICE_APC_AO_INFRA_PERI",              false   },
	{"DEVICE_APC_AO_MD",                      false   },

	/* 300 */
	{"DEVICE_APC_AO_MM",                      false   },
	{"CM_DQ_SECURE",                          false   },
	{"MM_IOMMU_DOMAIN",                       false   },
	{"DISP_GCE",                              false   },
	{"DEVICE_APC",                            false   },
	{"EMI",                                   false   },
	{"EMI_MPU",                               false   },
	{"PMIC_WRAP",                             false   },
};
#endif

/*
 * The extern functions for EMI MPU are removed because EMI MPU and Device APC
 * do not share the same IRQ now.
 */

/**************************************************************************
*STATIC FUNCTION
**************************************************************************/

#ifdef CONFIG_MTK_HIBERNATION
static int devapc_pm_restore_noirq(struct device *device)
{
	if (devapc_infra_irq != 0) {
		mt_irq_set_sens(devapc_infra_irq, MT_LEVEL_SENSITIVE);
		mt_irq_set_polarity(devapc_infra_irq, MT_POLARITY_LOW);
	}

	return 0;
}
#endif

#if DEVAPC_TURN_ON
static void unmask_infra_module_irq(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	if (module > PD_INFRA_VIO_MASK_MAX_INDEX) {
		pr_err("[DEVAPC] unmask_infra_module_irq: module overflow!\n");
		return;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	*DEVAPC_PD_INFRA_VIO_MASK(apc_index) &= (0xFFFFFFFF ^ (1 << apc_bit_index));
}

static int clear_infra_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	if (module > PD_INFRA_VIO_STA_MAX_INDEX) {
		pr_err("[DEVAPC] clear_infra_vio_status: module overflow!\n");
		return -1;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	*DEVAPC_PD_INFRA_VIO_STA(apc_index) = (0x1 << apc_bit_index);

	return 0;
}

static int check_infra_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	if (module > PD_INFRA_VIO_STA_MAX_INDEX) {
		pr_err("[DEVAPC] check_infra_vio_status: module overflow!\n");
		return -1;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (*DEVAPC_PD_INFRA_VIO_STA(apc_index) & (0x1 << apc_bit_index))
		return 1;

	return 0;
}

static void start_devapc(void)
{
	unsigned int i;

	mt_reg_sync_writel(0x80000000, DEVAPC_PD_INFRA_APC_CON);

	/* SMC call is called to set Device APC in LK instead */

	pr_err("[DEVAPC] INFRA VIO_MASK 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x\n",
		readl(DEVAPC_PD_INFRA_VIO_MASK(0)), readl(DEVAPC_PD_INFRA_VIO_MASK(1)),
		readl(DEVAPC_PD_INFRA_VIO_MASK(2)), readl(DEVAPC_PD_INFRA_VIO_MASK(3)),
		readl(DEVAPC_PD_INFRA_VIO_MASK(4)), readl(DEVAPC_PD_INFRA_VIO_MASK(5)),
		readl(DEVAPC_PD_INFRA_VIO_MASK(6)), readl(DEVAPC_PD_INFRA_VIO_MASK(7)),
		readl(DEVAPC_PD_INFRA_VIO_MASK(8)), readl(DEVAPC_PD_INFRA_VIO_MASK(9)));

	pr_err("[DEVAPC] INFRA VIO_STA 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x\n",
		readl(DEVAPC_PD_INFRA_VIO_STA(0)), readl(DEVAPC_PD_INFRA_VIO_STA(1)),
		readl(DEVAPC_PD_INFRA_VIO_STA(2)), readl(DEVAPC_PD_INFRA_VIO_STA(3)),
		readl(DEVAPC_PD_INFRA_VIO_STA(4)), readl(DEVAPC_PD_INFRA_VIO_STA(5)),
		readl(DEVAPC_PD_INFRA_VIO_STA(6)), readl(DEVAPC_PD_INFRA_VIO_STA(7)),
		readl(DEVAPC_PD_INFRA_VIO_STA(8)), readl(DEVAPC_PD_INFRA_VIO_STA(9)));

	pr_err("[DEVAPC] Clear INFRA VIO_STA and unmask INFRA VIO_MASK...\n");

	for (i = 0; i < ARRAY_SIZE(devapc_infra_devices); i++)
		if (true == devapc_infra_devices[i].enable_vio_irq) {
			clear_infra_vio_status(i);
			unmask_infra_module_irq(i);
		}

	pr_err("[DEVAPC] INFRA VIO_MASK 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x\n",
		readl(DEVAPC_PD_INFRA_VIO_MASK(0)), readl(DEVAPC_PD_INFRA_VIO_MASK(1)),
		readl(DEVAPC_PD_INFRA_VIO_MASK(2)), readl(DEVAPC_PD_INFRA_VIO_MASK(3)),
		readl(DEVAPC_PD_INFRA_VIO_MASK(4)), readl(DEVAPC_PD_INFRA_VIO_MASK(5)),
		readl(DEVAPC_PD_INFRA_VIO_MASK(6)), readl(DEVAPC_PD_INFRA_VIO_MASK(7)),
		readl(DEVAPC_PD_INFRA_VIO_MASK(8)), readl(DEVAPC_PD_INFRA_VIO_MASK(9)));

	pr_err("[DEVAPC] INFRA VIO_STA 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x\n",
		readl(DEVAPC_PD_INFRA_VIO_STA(0)), readl(DEVAPC_PD_INFRA_VIO_STA(1)),
		readl(DEVAPC_PD_INFRA_VIO_STA(2)), readl(DEVAPC_PD_INFRA_VIO_STA(3)),
		readl(DEVAPC_PD_INFRA_VIO_STA(4)), readl(DEVAPC_PD_INFRA_VIO_STA(5)),
		readl(DEVAPC_PD_INFRA_VIO_STA(6)), readl(DEVAPC_PD_INFRA_VIO_STA(7)),
		readl(DEVAPC_PD_INFRA_VIO_STA(8)), readl(DEVAPC_PD_INFRA_VIO_STA(9)));

}


static irqreturn_t devapc_violation_irq(int irq_number, void *dev_id)
{
	unsigned int dbg0 = 0, dbg1 = 0;
	unsigned int master_id;
	unsigned int domain_id;
	unsigned int vio_addr_high;
	unsigned int read_violation;
	unsigned int write_violation;
	unsigned int device_count;
	unsigned int shift_done;
	unsigned int i;
	struct pt_regs *regs;

	if (irq_number == devapc_infra_irq) {
		DEVAPC_VIO_MSG("[DEVAPC] INFRA VIO_MASK 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_MASK(0)), readl(DEVAPC_PD_INFRA_VIO_MASK(1)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(2)), readl(DEVAPC_PD_INFRA_VIO_MASK(3)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(4)));
		DEVAPC_VIO_MSG("[DEVAPC] INFRA VIO_MASK 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_MASK(5)), readl(DEVAPC_PD_INFRA_VIO_MASK(6)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(7)), readl(DEVAPC_PD_INFRA_VIO_MASK(8)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(9)));

		DEVAPC_VIO_MSG("[DEVAPC] INFRA VIO_STA 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_STA(0)), readl(DEVAPC_PD_INFRA_VIO_STA(1)),
			readl(DEVAPC_PD_INFRA_VIO_STA(2)), readl(DEVAPC_PD_INFRA_VIO_STA(3)),
			readl(DEVAPC_PD_INFRA_VIO_STA(4)));
		DEVAPC_VIO_MSG("[DEVAPC] INFRA VIO_STA 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_STA(5)), readl(DEVAPC_PD_INFRA_VIO_STA(6)),
			readl(DEVAPC_PD_INFRA_VIO_STA(7)), readl(DEVAPC_PD_INFRA_VIO_STA(8)),
			readl(DEVAPC_PD_INFRA_VIO_STA(9)));

		DEVAPC_VIO_MSG("[DEVAPC] VIO_SHIFT_STA: 0x%x\n", readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA));

		DEVAPC_VIO_MSG("[DEVAPC] Dump INFRA DBG0 & DBG1...\n");
		for (i = 0; i <= PD_INFRA_VIO_SHIFT_MAX_BIT; ++i)
			if (readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA) & (0x1 << i)) {
				mt_reg_sync_writel(0x1 << i, DEVAPC_PD_INFRA_VIO_SHIFT_SEL);
				mt_reg_sync_writel(0x1, DEVAPC_PD_INFRA_VIO_SHIFT_CON);
				for (shift_done = 0; (shift_done < 0x100)
					&& ((readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON) & 0x3) != 0x3); ++shift_done)
					DEVAPC_VIO_MSG("[DEVAPC] Syncing INFRA DBG0 & DBG1 (%d, %d)\n", i, shift_done);

				DEVAPC_VIO_MSG("[DEVAPC] VIO_SHIFT_SEL=0x%X, VIO_SHIFT_CON=0x%X\n",
					readl(DEVAPC_PD_INFRA_VIO_SHIFT_SEL), readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON));
				if ((readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON) & 0x3) == 0x3) {
					DEVAPC_VIO_MSG("[DEVAPC] Sync INFRA DBG0 & DBG1 (%d) SUCCESS\n", i);
					shift_done = 1;
				} else {
					DEVAPC_VIO_MSG("[DEVAPC] Sync INFRA DBG0 & DBG1 (%d) FAIL\n", i);
					shift_done = 0;
				}

				mt_reg_sync_writel(0x0, DEVAPC_PD_INFRA_VIO_SHIFT_CON);
				if (shift_done) {
					dbg0 = readl(DEVAPC_PD_INFRA_VIO_DBG0);
					dbg1 = readl(DEVAPC_PD_INFRA_VIO_DBG1);
					master_id = (dbg0 & INFRA_VIO_DBG_MSTID) >> INFRA_VIO_DBG_MSTID_START_BIT;
					domain_id = (dbg0 & INFRA_VIO_DBG_DMNID) >> INFRA_VIO_DBG_DMNID_START_BIT;
					write_violation = (dbg0 & INFRA_VIO_DBG_W_VIO) >> INFRA_VIO_DBG_W_VIO_START_BIT;
					read_violation = (dbg0 & INFRA_VIO_DBG_R_VIO) >> INFRA_VIO_DBG_R_VIO_START_BIT;
					vio_addr_high = (dbg0 & INFRA_VIO_ADDR_HIGH) >> INFRA_VIO_ADDR_HIGH_START_BIT;
				}
				mt_reg_sync_writel(0x0, DEVAPC_PD_INFRA_VIO_SHIFT_SEL);
				mt_reg_sync_writel(0x1 << i, DEVAPC_PD_INFRA_VIO_SHIFT_STA);
				DEVAPC_VIO_MSG("[DEVAPC] VIO_SHIFT_STA=0x%X, VIO_SHIFT_SEL=0x%X, VIO_SHIFT_CON=0x%X\n",
					readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA), readl(DEVAPC_PD_INFRA_VIO_SHIFT_SEL),
					readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON));

				/* violation information improvement */
				if (shift_done) {
					DEVAPC_VIO_MSG("[DEVAPC] Violation(Infra,%s%s) - Process:%s, PID:%i\n",
						read_violation == 1 ? "R" : " ", write_violation == 1 ? "W" : " ",
						current->comm, current->pid);
					DEVAPC_VIO_MSG("[DEVAPC] Vio Addr:0x%x (High:0x%x), Bus ID:0x%x, Dom ID:0x%x\n",
						dbg1, vio_addr_high, master_id, domain_id);
				}
			}

		device_count = ARRAY_SIZE(devapc_infra_devices);

		/* No need to check violation of EMI & EMI MPU slaves for Infra because they will not be unmasked */

		/* checking and showing violation normal slaves */
		for (i = 0; i < device_count; i++)
			if (devapc_infra_devices[i].enable_vio_irq == true && check_infra_vio_status(i) == 1) {
				clear_infra_vio_status(i);
				DEVAPC_VIO_MSG("[DEVAPC] Access Violation Slave: %s (infra index=%d)\n",
					devapc_infra_devices[i].device, i);
			}

		DEVAPC_VIO_MSG("[DEVAPC] INFRA VIO_STA 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_STA(0)), readl(DEVAPC_PD_INFRA_VIO_STA(1)),
			readl(DEVAPC_PD_INFRA_VIO_STA(2)), readl(DEVAPC_PD_INFRA_VIO_STA(3)),
			readl(DEVAPC_PD_INFRA_VIO_STA(4)));
		DEVAPC_VIO_MSG("[DEVAPC] INFRA VIO_STA 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_STA(5)), readl(DEVAPC_PD_INFRA_VIO_STA(6)),
			readl(DEVAPC_PD_INFRA_VIO_STA(7)), readl(DEVAPC_PD_INFRA_VIO_STA(8)),
			readl(DEVAPC_PD_INFRA_VIO_STA(9)));

		DEVAPC_VIO_MSG("[DEVAPC] INFRA VIO_SHIFT_STA: 0x%x\n", readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA));

	} else {
		DEVAPC_VIO_MSG("[DEVAPC] (ERROR) irq_number %d is not registered!\n", irq_number);
	}

	if ((DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG) || (enable_dynamic_one_core_violation_debug)) {
		DEVAPC_VIO_MSG("[DEVAPC] ====== Start dumping Device APC violation tracing ======\n");

		DEVAPC_VIO_MSG("[DEVAPC] **************** [All IRQ Registers] ****************\n");
		regs = get_irq_regs();
		show_regs(regs);

		DEVAPC_VIO_MSG("[DEVAPC] **************** [All Current Task Stack] ****************\n");
		show_stack(current, NULL);

		DEVAPC_VIO_MSG("[DEVAPC] ====== End of dumping Device APC violation tracing ======\n");
	}

	return IRQ_HANDLED;
}
#endif

static int devapc_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
#if DEVAPC_TURN_ON
	int ret;
#endif

	DEVAPC_MSG("[DEVAPC] module probe.\n");

	if (devapc_pd_infra_base == NULL) {
		if (node) {
			devapc_pd_infra_base = of_iomap(node, DAPC_DEVICE_TREE_NODE_PD_INFRA_INDEX);
			devapc_infra_irq = irq_of_parse_and_map(node, DAPC_DEVICE_TREE_NODE_PD_INFRA_INDEX);
			DEVAPC_MSG("[DEVAPC] PD_INFRA_ADDRESS: %p, IRQ: %d\n", devapc_pd_infra_base, devapc_infra_irq);
		} else {
			pr_err("[DEVAPC] can't find DAPC_INFRA_PD compatible node\n");
			return -1;
		}
	}

#if DEVAPC_TURN_ON
	ret = request_irq(devapc_infra_irq, (irq_handler_t) devapc_violation_irq,
			  IRQF_TRIGGER_LOW | IRQF_SHARED, "devapc", &g_devapc_ctrl);
	if (ret) {
		pr_err("[DEVAPC] Failed to request infra irq! (%d)\n", ret);
		return ret;
	}
#endif

/* CCF */
#if DEVAPC_USE_CCF
	dapc_infra_clk = devm_clk_get(&pdev->dev, "devapc-infra-clock");
	if (IS_ERR(dapc_infra_clk)) {
		pr_err("[DEVAPC] (Infra) Cannot get devapc clock from common clock framework.\n");
		return PTR_ERR(dapc_infra_clk);
	}
	clk_prepare_enable(dapc_infra_clk);
#endif

#ifdef CONFIG_MTK_HIBERNATION
	register_swsusp_restore_noirq_func(ID_M_DEVAPC, devapc_pm_restore_noirq, NULL);
#endif

#if DEVAPC_TURN_ON
	start_devapc();
#endif

	return 0;
}

static int devapc_remove(struct platform_device *dev)
{
	clk_disable_unprepare(dapc_infra_clk);
	return 0;
}

static int devapc_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int devapc_resume(struct platform_device *dev)
{
	DEVAPC_MSG("[DEVAPC] module resume.\n");
	return 0;
}

static int check_debug_input_type(const char *str)
{
	if (sysfs_streq(str, "1"))
		return DAPC_INPUT_TYPE_DEBUG_ON;
	else if (sysfs_streq(str, "0"))
		return DAPC_INPUT_TYPE_DEBUG_OFF;
	else
		return 0;
}

static ssize_t devapc_dbg_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	int input_type;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return -EFAULT;

	desc[len] = '\0';

	input_type = check_debug_input_type(desc);
	if (!input_type)
		return -EFAULT;

	if (input_type == DAPC_INPUT_TYPE_DEBUG_ON) {
		enable_dynamic_one_core_violation_debug = 1;
		pr_err("[DEVAPC] One-Core Debugging: Enabled\n");
	} else if (input_type == DAPC_INPUT_TYPE_DEBUG_OFF) {
		enable_dynamic_one_core_violation_debug = 0;
		pr_err("[DEVAPC] One-Core Debugging: Disabled\n");
	}

	return count;
}

static int devapc_dbg_open(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations devapc_dbg_fops = {
	.owner = THIS_MODULE,
	.open  = devapc_dbg_open,
	.write = devapc_dbg_write,
	.read = NULL,
};

static const struct of_device_id plat_devapc_dt_match[] = {
	{ .compatible = "mediatek,devapc" },
	{},
};

static struct platform_driver devapc_driver = {
	.probe = devapc_probe,
	.remove = devapc_remove,
	.suspend = devapc_suspend,
	.resume = devapc_resume,
	.driver = {
			.name = "devapc",
			.owner = THIS_MODULE,
			.of_match_table	= plat_devapc_dt_match,
	},
};

/*
 * devapc_init: module init function.
 */
static int __init devapc_init(void)
{
	int ret;

	DEVAPC_MSG("[DEVAPC] kernel module init.\n");

	ret = platform_driver_register(&devapc_driver);
	if (ret) {
		pr_err("[DEVAPC] Unable to register driver (%d)\n", ret);
		return ret;
	}

	g_devapc_ctrl = cdev_alloc();
	if (!g_devapc_ctrl) {
		pr_err("[DEVAPC] Failed to add devapc device! (%d)\n", ret);
		platform_driver_unregister(&devapc_driver);
		return ret;
	}
	g_devapc_ctrl->owner = THIS_MODULE;

	proc_create("devapc_dbg", (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH), NULL,
		&devapc_dbg_fops);

	return 0;
}

/*
 * devapc_exit: module exit function.
 */
static void __exit devapc_exit(void)
{
	DEVAPC_MSG("[DEVAPC] DEVAPC module exit\n");
#ifdef CONFIG_MTK_HIBERNATION
	unregister_swsusp_restore_noirq_func(ID_M_DEVAPC);
#endif
}

/* Device APC no longer shares IRQ with EMI and can be changed to use the earlier "arch_initcall" */
arch_initcall(devapc_init);
module_exit(devapc_exit);
MODULE_LICENSE("GPL");
