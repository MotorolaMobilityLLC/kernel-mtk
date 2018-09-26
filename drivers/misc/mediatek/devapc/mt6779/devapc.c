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
#include <mt-plat/mtk_secure_api.h>

#ifdef CONFIG_MTK_HIBERNATION
#include <mtk_hibernate_dpm.h>
#endif

/* CCF */
#include <linux/clk.h>

#include "mtk_io.h"
#include "sync_write.h"
#include "devapc.h"

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
#include <mt-plat/aee.h>
#endif

/* 0 for early porting */
#define DEVAPC_TURN_ON         1
#define DEVAPC_USE_CCF         1

/* Debug message event */
#define DEVAPC_LOG_NONE        0x00000000
#define DEVAPC_LOG_INFO        0x00000001
#define DEVAPC_LOG_DBG         0x00000002

#define DEBUG
#ifdef DEBUG
#define DEVAPC_LOG_LEVEL	(DEVAPC_LOG_DBG)
#else
#define DEVAPC_LOG_LEVEL	(DEVAPC_LOG_NONE)
#endif

#define DEVAPC_DBG_MSG(fmt, args...) \
	do {    \
		if (DEVAPC_LOG_LEVEL & DEVAPC_LOG_DBG) { \
			pr_debug(fmt, ##args); \
		} else if (DEVAPC_LOG_LEVEL & DEVAPC_LOG_INFO) { \
			pr_info(fmt, ##args); \
		} \
	} while (0)


#define DEVAPC_VIO_LEVEL      (DEVAPC_LOG_INFO)

#define DEVAPC_MSG(fmt, args...) \
	do {    \
		if (DEVAPC_VIO_LEVEL & DEVAPC_LOG_DBG) { \
			pr_debug(fmt, ##args); \
		} else if (DEVAPC_VIO_LEVEL & DEVAPC_LOG_INFO) { \
			pr_info(fmt, ##args); \
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
unsigned int devapc_vio_current_aee_trigger_times;

#if DEVAPC_TURN_ON
static struct DEVICE_INFO devapc_infra_devices[] = {
	/* device name                          enable_vio_irq */

	/* 0 */
	{"INFRA_AO_TOPCKGEN",                     true    },
	{"INFRA_AO_INFRASYS_CONFIG_REGS",         true    },
	{"INFRA_AO_RESERVE",                      true    },
	{"INFRA_AO_ PERICFG",                     true    },
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
	{"INFRA_AO_ DVFS_CTRL_PROC",              true    },
	{"INFRA_AO_MBIST_AO_REG",                 true    },

	/* 20 */
	{"INFRA_AO_CLDMA_AO_AP",                  true    },
	{"INFRA_AO_DEVICE_MPU",                   true    },
	{"INFRA_AO_AES_TOP_0",                    true    },
	{"INFRA_AO_SYS_TIMER",                    true    },
	{"INFRA_AO_MDEM_TEMP_SHARE",              true    },
	{"INFRA_AO_DEVICE_APC_AO_MD",             true    },
	{"INFRA_AO_SECURITY_AO",                  true    },
	{"INFRA_AO_TOPCKGEN_REG",                 true    },
	{"INFRA_AO_DEVICE_APC_AO_MM",             true    },
	{"INFRA_AO_SLEEP_SRAM",                   true    },

	/* 30 */
	{"INFRA_AO_SLEEP_SRAM",                   true    },
	{"INFRA_AO_SLEEP_SRAM",                   true    },
	{"INFRA_AO_SLEEP_SRAM",                   true    },
	{"INFRASYS_DPMAIF",                       true    },
	{"INFRASYS_DPMAIF",                       true    },
	{"INFRASYS_DPMAIF",                       true    },
	{"INFRASYS_DPMAIF",                       true    },
	{"INFRASYS_SYS_CIRQ",                     true    },
	{"INFRASYS_MM_IOMMU",                     true    },
	{"INFRASYS_EFUSE_PDN_DEBUG",              true    },

	/* 40 */
	{"INFRASYS_DEVICE_APC",                   true    },
	{"INFRASYS_DBG_TRACKER",                  true    },
	{"INFRASYS_CCIF0_AP",                     true    },
	{"INFRASYS_CCIF0_MD",                     true    },
	{"INFRASYS_CCIF1_AP",                     true    },
	{"INFRASYS_CCIF1_MD",                     true    },
	{"INFRASYS_MBIST",                        true    },
	{"INFRASYS_INFRA_PDN_REGISTER",           true    },
	{"INFRASYS_TRNG",                         true    },
	{"INFRASYS_DX_CC",                        true    },

	/* 50 */
	{"INFRASYS_MD_CCIF_MD1",                  true    },
	{"INFRASYS_CQ_DMA",                       true    },
	{"INFRASYS_MD_CCIF_MD2",                  true    },
	{"INFRASYS_SRAMROM",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_EMI",                          true    },
	{"INFRASYS_DEVICE_MPU",                   true    },

	/* 60 */
	{"INFRASYS_CLDMA_PDN",                    true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_EMI_MPU",                      true    },
	{"INFRASYS_DVFS_PROC",                    true    },
	{"INFRASYS_GCE",                          true    },
	{"INFRASYS_RESERVE",                      true    },

	/* 70 */
	{"INFRASYS_DRAMC_CH1_TOP0",               true    },
	{"INFRASYS_DRAMC_CH1_TOP1",               true    },
	{"INFRASYS_DRAMC_CH1_TOP2",               true    },
	{"INFRASYS_DRAMC_CH1_TOP3",               true    },
	{"INFRASYS_DRAMC_CH1_TOP4",               true    },
	{"INFRASYS_DRAMC_CH1_TOP5",               true    },
	{"INFRASYS_DRAMC_CH1_TOP6",               true    },
	{"INFRASYS_CCIF2_AP",                     true    },
	{"INFRASYS_CCIF2_MD",                     true    },
	{"INFRASYS_CCIF3_AP",                     true    },

	/* 80 */
	{"INFRASYS_CCIF3_MD",                     true    },
	{"INFRASYS_DRAMC_CH0_TOP0",               true    },
	{"INFRASYS_DRAMC_CH0_TOP1",               true    },
	{"INFRASYS_DRAMC_CH0_TOP2",               true    },
	{"INFRASYS_DRAMC_CH0_TOP3",               true    },
	{"INFRASYS_DRAMC_CH0_TOP4",               true    },
	{"INFRASYS_DRAMC_CH0_TOP5",               true    },
	{"INFRASYS_DRAMC_CH0_TOP6",               true    },
	{"INFRASYS_CCIF4_AP",                     true    },
	{"INFRASYS_CCIF4_MD",                     true    },

	/* 90 */
	{"INFRA_TOPAXI_BUS_TRACE",                true    },
	{"vpu_iommu/mm_iommu",                    true    },
	{"vpu_iommu/mm_iommu",                    true    },
	{"vpu_iommu/mm_iommu",                    true    },
	{"vpu_iommu/mm_iommu",                    true    },
	{"vpu_iommu/mm_iommu",                    true    },
	{"sub_common2x1",                         true    },
	{"sub_common2x1",                         true    },
	{"sub_common2x1",                         true    },
	{"INFRASYS_RESERVE",                      true    },

	/* 100 */
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },
	{"INFRASYS_RESERVE",                      true    },

	/* 110 */
	{"INFRA_AO_SSPM Partition 1",             true    },
	{"INFRA_AO_SSPM Partition 2",             true    },
	{"INFRA_AO_SSPM Partition 3",             true    },
	{"INFRA_AO_SSPM Partition 4",             true    },
	{"INFRA_AO_SSPM Partition 5",             true    },
	{"INFRA_AO_SSPM Partition 6",             true    },
	{"INFRA_AO_SSPM Partition 7",             true    },
	{"INFRA_AO_SSPM Partition 8",             true    },
	{"INFRA_AO_SSPM Partition 9",             true    },
	{"INFRA_AO_SSPM Partition 10",            true    },

	/* 120 */
	{"INFRA_AO_SSPM Partition 11",            true    },
	{"INFRA_AO_SCP",                          true    },
	{"INFRA_AO_HIFI3",                        true    },
	{"INFRA_AO_MCUCFG",                       true    },
	{"INFRASYS_DBUGSYS",                      true    },
	{"PERISYS_APDMA",                         true    },
	{"PERISYS_AUXADC",                        true    },
	{"PERISYS_UART0",                         true    },
	{"PERISYS_UART1",                         true    },
	{"PERISYS_UART2",                         true    },

	/* 130 */
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

	/* 140 */
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

	/* 150 */
	{"PERISYS_SPI5",                          true    },
	{"PERISYS_I2C7",                          true    },
	{"PERISYS_I2C8",                          true    },
	{"PERISYS_BUS_TRACE",                     true    },
	{"PERISYS_SPI6",                          true    },
	{"PERISYS_SPI7",                          true    },
	{"PERISYS_USB",                           true    },
	{"PERISYS_USB_2.0_SUB",                   true    },
	{"PERISYS_AUDIO",                         true    },
	{"PERISYS_MSDC0",                         true    },

	/* 160 */
	{"PERISYS_MSDC1",                         true    },
	{"PERISYS_MSDC2",                         true    },
	{"PERISYS_MSDC3",                         true    },
	{"PERISYS_UFS",                           true    },
	{"PERISUS_USB3.0_SIF",                    true    },
	{"PERISUS_USB3.0_SIF2",                   true    },
	{"EAST_RESERVE_0",                        true    },
	{"EAST_ RESERVE_1",                       true    },
	{"EAST_ RESERVE_2",                       true    },
	{"EAST_ RESERVE_3",                       true    },

	/* 170 */
	{"EAST_ RESERVE_4",                       true    },
	{"EAST_IO_CFG_RT",                        true    },
	{"EAST_ RESERVE_6",                       true    },
	{"EAST_ RESERVE_7",                       true    },
	{"EAST_CSI0_TOP_AO",                      true    },
	{"EAST_CSI1_TOP_AO",                      true    },
	{"EAST_ RESERVE_A",                       true    },
	{"EAST_ RESERVE_B",                       true    },
	{"EAST_RESERVE_C",                        true    },
	{"EAST_RESERVE_D",                        true    },

	/* 180 */
	{"EAST_RESERVE_E",                        true    },
	{"EAST_RESERVE_F",                        true    },
	{"SOUTH_RESERVE_0",                       true    },
	{"SOUTH_RESERVE_1",                       true    },
	{"SOUTH_IO_CFG_RM",                       true    },
	{"SOUTH_IO_CFG_RB",                       true    },
	{"SOUTH_EFUSE",                           true    },
	{"SOUTH_ RESERVE_5",                      true    },
	{"SOUTH_ RESERVE_6",                      true    },
	{"SOUTH_ RESERVE_7",                      true    },

	/* 190 */
	{"SOUTH_ RESERVE_8",                      true    },
	{"SOUTH_ RESERVE_9",                      true    },
	{"SOUTH_ RESERVE_A",                      true    },
	{"SOUTH_ RESERVE_B",                      true    },
	{"SOUTH_RESERVE_C",                       true    },
	{"SOUTH_RESERVE_D",                       true    },
	{"SOUTH_RESERVE_E",                       true    },
	{"SOUTH_RESERVE_F",                       true    },
	{"WEST_ RESERVE_0",                       true    },
	{"WEST_ msdc1_pad_macro",                 true    },

	/* 200 */
	{"WEST_ RESERVE_2",                       true    },
	{"WEST_ RESERVE_3",                       true    },
	{"WEST_ RESERVE_4",                       true    },
	{"WEST_ MIPI_TX_CONFIG",                  true    },
	{"WEST_ RESERVE_6",                       true    },
	{"WEST_ IO_CFG_LB",                       true    },
	{"WEST_ IO_CFG_LM",                       true    },
	{"WEST_ IO_CFG_BL",                       true    },
	{"WEST_ RESERVE_A",                       true    },
	{"WEST_ RESERVE_B",                       true    },

	/* 210 */
	{"WEST_ RESERVE_C",                       true    },
	{"WEST_ RESERVE_D",                       true    },
	{"WEST_RESERVE_E",                        true    },
	{"WEST_RESERVE_F",                        true    },
	{"NORTH_RESERVE_0",                       true    },
	{"NORTH_RESERVE_1",                       true    },
	{"NORTH_IO_CFG_LT",                       true    },
	{"NORTH_IO_CFG_TL",                       true    },
	{"NORTH_USB20 PHY",                       true    },
	{"NORTH_msdc0 pad macro",                 true    },

	/* 220 */
	{"NORTH_ RESERVE_6",                      true    },
	{"NORTH_ RESERVE_7",                      true    },
	{"NORTH_ RESERVE_8",                      true    },
	{"NORTH_ RESERVE_9",                      true    },
	{"NORTH_ UFS_MPHY",                       true    },
	{"NORTH_ RESERVE_B",                      true    },
	{"NORTH_RESERVE_C",                       true    },
	{"NORTH_RESERVE_D",                       true    },
	{"NORTH_RESERVE_E",                       true    },
	{"NORTH_RESERVE_F",                       true    },

	/* 230 */
	{"PERISYS_CONN",                          true    },
	{"PERISYS_MD_VIOLATION",                  true    },
	{"Gpu",                                   true    },
	{"Gpu",                                   true    },
	{"Gpu",                                   true    },
	{"Gpu",                                   true    },
	{"Gpu",                                   true    },
	{"Gpu",                                   true    },
	{"Gpu",                                   true    },
	{"Gpu",                                   true    },

	/* 240 */
	{"Gpu",                                   true    },
	{"DFD",                                   true    },
	{"G3D_CONFIG",                            true    },
	{"G3D_DVFS",                              true    },
	{"MFG_OTHERS",                            true    },
	{"MMSYS_CONFIG",                          true    },
	{"MDP_RDMA0",                             true    },
	{"MDP_RDMA1",                             true    },
	{"MDP_RSZ0",                              true    },
	{"MDP_RSZ1",                              true    },

	/* 250 */
	{"MDP_WROT0",                             true    },
	{"MDP_WDMA",                              true    },
	{"MDP_TDSHP",                             true    },
	{"DISP_OVL0",                             true    },
	{"DISP_OVL0_2L",                          true    },
	{"DISP_OVL1_2L",                          true    },
	{"DISP_RDMA0",                            true    },
	{"DISP_RDMA1",                            true    },
	{"DISP_WDMA0",                            true    },
	{"DISP_COLOR0",                           true    },

	/* 260 */
	{"DISP_CCORR0",                           true    },
	{"DISP_AAL0",                             true    },
	{"DISP_GAMMA0",                           true    },
	{"DISP_DITHER0",                          true    },
	{"DSI_SPLIT",                             true    },
	{"DSI0",                                  true    },
	{"DPI",                                   true    },
	{"MM_MUTEX",                              true    },
	{"SMI_LARB0",                             true    },
	{"SMI_LARB1",                             true    },

	/* 270 */
	{"SMI_COMMON",                            true    },
	{"DISP_RSZ",                              true    },
	{"MDP_AAL",                               true    },
	{"MDP_CCORR",                             true    },
	{"DBI",                                   true    },
	{"MMSYS_RESERVE",                         true    },
	{"MMSYS_RESERVE",                         true    },
	{"MDP_WROT1",                             true    },
	{"DISP_POSTMASK0",                        true    },
	{"MMSYS_OTHERS",                          true    },

	/* 280 */
	{"imgsys1_top",                           true    },
	{"dip_a0",                                true    },
	{"dip_a1",                                true    },
	{"dip_a2",                                true    },
	{"dip_a3",                                true    },
	{"dip_a4",                                true    },
	{"dip_a5",                                true    },
	{"dip_a6",                                true    },
	{"dip_a7",                                true    },
	{"dip_a8",                                true    },

	/* 290 */
	{"dip_a9",                                true    },
	{"dip_a10",                               true    },
	{"dip_a11",                               true    },
	{"IMGSYS_RESERVE",                        true    },
	{"smi_larb5",                             true    },
	{"smi_larb6",                             true    },
	{"IMGSYS_OTHERS",                         true    },
	{"VENCSYS_GLOBAL_CON",                    true    },
	{"VENCSYSSYS_SMI_LARB3",                  true    },
	{"VENCSYS_VENC",                          true    },

	/* 300 */
	{"VENCSYS_JPGENC",                        true    },
	{"VENC_RESERVE",                          true    },
	{"VENC_RESERVE",                          true    },
	{"VENC_RESERVE",                          true    },
	{"VENCSYS_MBIST_CTRL",                    true    },
	{"VENCSYS_OTHERS",                        true    },
	{"VDECSYS_GLOBAL_CON",                    true    },
	{"vdec_smi_larbx",                        true    },
	{"VDECSYS_FULL_TOP",                      true    },
	{"vdec_full_top_lat",                     true    },

	/* 310 */
	{"VDEC_RESERVE",                          true    },
	{"VDEC_RESERVE",                          true    },
	{"VDECSYS_OTHERS",                        true    },
	{"CAMSYS_CAMSYS_TOP",                     true    },
	{"CAMSYS_LARB9",                          true    },
	{"CAMSYS_LARB10",                         true    },
	{"CAMSYS_LARB11",                         true    },
	{"CAMSYS_seninf_a",                       true    },
	{"CAMSYS_seninf_b",                       true    },
	{"CAMSYS_seninf_c",                       true    },

	/* 320 */
	{"CAMSYS_seninf_d",                       true    },
	{"CAMSYS_seninf_e",                       true    },
	{"CAMSYS_seninf_f",                       true    },
	{"CAMSYS_seninf_g",                       true    },
	{"CAMSYS_seninf_h",                       true    },
	{"CAMSYS_smi_sub_common",                 true    },
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_RESERVED",                       true    },
	{"cam_raw_a_ip_group_0",                  true    },

	/* 330 */
	{"cam_raw_a_ip_group_1",                  true    },
	{"cam_raw_a_ip_group_2",                  true    },
	{"cam_raw_a_ip_group_3",                  true    },
	{"cam_raw_a_dma_0",                       true    },
	{"cam_raw_a_dma_1",                       true    },
	{"ltm_curve_a_0",                         true    },
	{"ltm_curve_a_1",                         true    },
	{"cam_raw_a_ip_group_0_inner",            true    },
	{"cam_raw_a_ip_group_1_inner",            true    },
	{"cam_raw_a_ip_group_2_inner",            true    },

	/* 340 */
	{"cam_raw_a_ip_group_3_inner",            true    },
	{"cam_raw_a_dma_0_inner",                 true    },
	{"cam_raw_a_dma_1_inner",                 true    },
	{"ltm_curve_a_0_inner",                   true    },
	{"ltm_curve_a_1_inner",                   true    },
	{"cam_raw_b_ip_group_0",                  true    },
	{"cam_raw_b_ip_group_1",                  true    },
	{"cam_raw_b_ip_group_2",                  true    },
	{"cam_raw_b_ip_group_3",                  true    },
	{"cam_raw_b_dma_0",                       true    },

	/* 350 */
	{"cam_raw_b_dma_1",                       true    },
	{"ltm_curve_b_0",                         true    },
	{"ltm_curve_b_1",                         true    },
	{"cam_raw_b_ip_group_0_inner",            true    },
	{"cam_raw_b_ip_group_1_inner",            true    },
	{"cam_raw_b_ip_group_2_inner",            true    },
	{"cam_raw_b_ip_group_3_inner",            true    },
	{"cam_raw_b_dma_0_inner",                 true    },
	{"cam_raw_b_dma_1_inner",                 true    },
	{"ltm_curve_b_0_inner",                   true    },

	/* 360 */
	{"ltm_curve_b_1_inner",                   true    },
	{"cam_raw_c_ip_group_0",                  true    },
	{"cam_raw_c_ip_group_1",                  true    },
	{"cam_raw_c_ip_group_2",                  true    },
	{"cam_raw_c_ip_group_3",                  true    },
	{"cam_raw_c_dma_0",                       true    },
	{"cam_raw_c_dma_1",                       true    },
	{"ltm_curve_c_0",                         true    },
	{"ltm_curve_c_1",                         true    },
	{"cam_raw_c_ip_group_0_inner",            true    },

	/* 370 */
	{"cam_raw_c_ip_group_1_inner",            true    },
	{"cam_raw_c_ip_group_2_inner",            true    },
	{"cam_raw_c_ip_group_3_inner",            true    },
	{"cam_raw_c_dma_0_inner",                 true    },
	{"cam_raw_c_dma_1_inner",                 true    },
	{"ltm_curve_c_0_inner",                   true    },
	{"ltm_curve_c_1_inner",                   true    },
	{"cam_raw_a_AA_histogram_0",              true    },
	{"cam_raw_a_AA_histogram_1",              true    },
	{"cam_raw_a_AA_histogram_2",              true    },

	/* 380 */
	{"cam_raw_a_MAE_histogram",               true    },
	{"cam_raw_b_AA_histogram_0",              true    },
	{"cam_raw_b_AA_histogram_1",              true    },
	{"cam_raw_b_AA_histogram_2",              true    },
	{"cam_raw_b_MAE_histogram",               true    },
	{"cam_raw_c_AA_histogram_0",              true    },
	{"cam_raw_c_AA_histogram_1",              true    },
	{"cam_raw_c_AA_histogram_2",              true    },
	{"cam_raw_c_MAE_histogram",               true    },
	{"CAMSYS_RESERVED",                       true    },

	/* 390 */
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_adl",                            true    },
	{"camsv_0",                               true    },
	{"camsv_1",                               true    },
	{"camsv_2",                               true    },
	{"camsv_3",                               true    },
	{"camsv_4",                               true    },
	{"camsv_5",                               true    },

	/* 400 */
	{"camsv_6",                               true    },
	{"camsv_7",                               true    },
	{"CAMSYS_asg",                            true    },
	{"cam_raw_a_set",                         true    },
	{"cam_raw_a_clr",                         true    },
	{"cam_raw_b_set",                         true    },
	{"cam_raw_b_clr",                         true    },
	{"cam_raw_c_set",                         true    },
	{"cam_raw_c_clr",                         true    },
	{"CAMSYS_RESERVED",                       true    },

	/* 410 */
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_RESERVED",                       true    },
	{"CAMSYS_tsf",                            true    },
	{"ccu_ctl",                               true    },
	{"ccu_h2t_a",                             true    },
	{"ccu_t2h_a",                             true    },
	{"ccu_dma",                               true    },

	/* 420 */
	{"CAMSYS_RESERVED",                       true    },
	{"DCCM",                                  true    },
	{"CAMSYS_RESEVE",                         true    },
	{"CAMSYS_OTHERS",                         true    },
	{"APUSYS_apu_conn_config",                true    },
	{"APUSYS_ADL_CTRL",                       true    },
	{"APUSYS_vcore_config",                   true    },
	{"APUSYS_apu_edma",                       true    },
	{"APUSYS_apu1_edmb",                      true    },
	{"APUSYS_apu2_edmc",                      true    },

	/* 430 */
	{"APUSYS_COREA_DMEM_0",                   true    },
	{"APUSYS_COREA_DMEM_1",                   true    },
	{"APUSYS_COREA_IMEM",                     true    },
	{"APUSYS_COREA_CONTROL",                  true    },
	{"APUSYS_COREA_DEBUG",                    true    },
	{"APUSYS_COREB_DMEM_0",                   true    },
	{"APUSYS_COREB_DMEM_1",                   true    },
	{"APUSYS_COREB_IMEM",                     true    },
	{"APUSYS_COREB_CONTROL",                  true    },
	{"APUSYS_COREB_DEBUG",                    true    },

	/* 440 */
	{"APUSYS_ RESERVED",                      true    },
	{"APUSYS_ RESERVED",                      true    },
	{"APUSYS_ RESERVED",                      true    },
	{"APUSYS_COREC_CONTROL",                  true    },
	{"APUSYS_COREC_MDLA",                     true    },
	{"APUSYS_OTHERS",                         true    },
	{"IPESYS_ipesys_top",                     true    },
	{"IPESYS_fdvt",                           true    },
	{"IPESYS_fe",                             true    },
	{"IPESYS_rsc",                            true    },

	/* 450 */
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },

	/* 460 */
	{"IPESYS_smi_2x1_sub_common",             true    },
	{"IPESYS_smi_larb8",                      true    },
	{"IPESYS_depth",                          true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },

	/* 470 */
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_ RESERVED",                      true    },
	{"IPESYS_smi_larb7",                      true    },
	{"IPESYS_OTHERS",                         true    },

};
#endif


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
		pr_info("[DEVAPC] %s: module overflow!\n", __func__);
		return;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	*DEVAPC_PD_INFRA_VIO_MASK(apc_index) &=
		(0xFFFFFFFF ^ (1 << apc_bit_index));
}

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
static void mask_infra_module_irq(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	if (module > PD_INFRA_VIO_MASK_MAX_INDEX) {
		pr_info("[DEVAPC] %s: module overflow!\n", __func__);
		return;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	*DEVAPC_PD_INFRA_VIO_MASK(apc_index) |= (1 << apc_bit_index);
}
#endif

static int clear_infra_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	if (module > PD_INFRA_VIO_STA_MAX_INDEX) {
		pr_info("[DEVAPC] %s: module overflow!\n", __func__);
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
		pr_info("[DEVAPC] %s: module overflow!\n", __func__);
		return -1;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (*DEVAPC_PD_INFRA_VIO_STA(apc_index) & (0x1 << apc_bit_index))
		return 1;

	return 0;
}

static void print_vio_mask_sta(void)
{
	DEVAPC_DBG_MSG("[DEVAPC] %s 0:0x%x 1:0x%x 2:0x%x 3:0x%x ",
			"INFRA VIO_MASK",
			readl(DEVAPC_PD_INFRA_VIO_MASK(0)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(1)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(2)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(3)));
	DEVAPC_DBG_MSG("4:0x%x 5:0x%x 6:0x%x 7:0x%x 8:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_MASK(4)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(5)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(6)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(7)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(8)));
	DEVAPC_DBG_MSG("[DEVAPC] %s 9:0x%x 10:0x%x 11:0x%x 12:0x%x ",
			"INFRA VIO_MASK",
			readl(DEVAPC_PD_INFRA_VIO_MASK(9)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(10)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(11)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(12)));
	DEVAPC_DBG_MSG("13:0x%x 14:0x%x 15:0x%x 16:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_MASK(13)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(14)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(15)),
			readl(DEVAPC_PD_INFRA_VIO_MASK(16)));

	DEVAPC_DBG_MSG("[DEVAPC] %s 0:0x%x 1:0x%x 2:0x%x 3:0x%x ",
			"INFRA VIO_STA",
			readl(DEVAPC_PD_INFRA_VIO_STA(0)),
			readl(DEVAPC_PD_INFRA_VIO_STA(1)),
			readl(DEVAPC_PD_INFRA_VIO_STA(2)),
			readl(DEVAPC_PD_INFRA_VIO_STA(3)));
	DEVAPC_DBG_MSG("4:0x%x 5:0x%x 6:0x%x 7:0x%x 8:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_STA(4)),
			readl(DEVAPC_PD_INFRA_VIO_STA(5)),
			readl(DEVAPC_PD_INFRA_VIO_STA(6)),
			readl(DEVAPC_PD_INFRA_VIO_STA(7)),
			readl(DEVAPC_PD_INFRA_VIO_STA(8)));
	DEVAPC_DBG_MSG("[DEVAPC] %s 9:0x%x 10:0x%x 11:0x%x 12:0x%x ",
			"INFRA VIO_STA",
			readl(DEVAPC_PD_INFRA_VIO_STA(9)),
			readl(DEVAPC_PD_INFRA_VIO_STA(10)),
			readl(DEVAPC_PD_INFRA_VIO_STA(11)),
			readl(DEVAPC_PD_INFRA_VIO_STA(12)));
	DEVAPC_DBG_MSG("13:0x%x 14:0x%x 15:0x%x 16:0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_STA(13)),
			readl(DEVAPC_PD_INFRA_VIO_STA(14)),
			readl(DEVAPC_PD_INFRA_VIO_STA(15)),
			readl(DEVAPC_PD_INFRA_VIO_STA(16)));
}

static void start_devapc(void)
{
	unsigned int i;

	DEVAPC_MSG("[DEVAPC] %s\n", __func__);

	mt_reg_sync_writel(0x80000000, DEVAPC_PD_INFRA_APC_CON);
	print_vio_mask_sta();

	DEVAPC_DBG_MSG("[DEVAPC] %s\n",
		"Clear INFRA VIO_STA and unmask INFRA VIO_MASK...");

	for (i = 0; i < ARRAY_SIZE(devapc_infra_devices); i++)
		if (true == devapc_infra_devices[i].enable_vio_irq) {
			clear_infra_vio_status(i);
			unmask_infra_module_irq(i);
		}

	print_vio_mask_sta();

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
	devapc_vio_current_aee_trigger_times = 0;
#endif

}

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)

/* violation index corresponds to subsys */
static const char *index_to_subsys(unsigned int index)
{
	if (index >= 232 && index <= 244)
		return "MFGSYS";
	else if ((index >= 268 && index <= 270) || index == 294 || index == 295
		|| index == 298 || index == 307 || index == 325 || index == 460
		|| index == 461 || index == 477)
		return "SMI";
	else if ((index >= 253 && index <= 263) || index == 271 || index == 278)
		return "MMSYS_DISP";
	else if (index >= 244 && index <= 279)
		return "MMSYS";
	else if (index >= 280 && index <= 296)
		return "IMGSYS";
	else if (index >= 297 && index <= 305)
		return "VENCSYS";
	else if (index >= 306 && index <= 312)
		return "VDECSYS";
	else if (index >= 313 && index <= 423)
		return "CAMSYS";
	else if (index >= 424 && index <= 445)
		return "VPUSYS";
	else if (index >= 446 && index <= 478)
		return "IPESYS";
	else if (index < ARRAY_SIZE(devapc_infra_devices))
		return devapc_infra_devices[index].device;
	else
		return "OUT_OF_BOUND";
}

static void execute_aee(unsigned int i, unsigned int dbg0, unsigned int dbg1)
{
	char subsys_str[48] = {0};
	unsigned int domain_id;

	DEVAPC_MSG("[DEVAPC] Executing AEE Exception...\n");

	/* mask irq for module "i" */
	mask_infra_module_irq(i);

	/* Mark the flag for showing AEE (AEE should be shown only once) */
	if (devapc_vio_current_aee_trigger_times <
		DEVAPC_VIO_MAX_TOTAL_AEE_TRIGGER_TIMES) {

		devapc_vio_current_aee_trigger_times++;
		domain_id = (dbg0 & INFRA_VIO_DBG_DMNID)
			>> INFRA_VIO_DBG_DMNID_START_BIT;
		if (domain_id == 1) {
			strncpy(subsys_str, "MD_SI", sizeof(subsys_str));
		} else {
			strncpy(subsys_str, index_to_subsys(i),
				sizeof(subsys_str));
		}
		subsys_str[sizeof(subsys_str)-1] = '\0';

		aee_kernel_exception("DEVAPC",
			"%s %s, Vio Addr: 0x%x\n%s%s\n",
			"[DEVAPC] Violation Slave:",
			devapc_infra_devices[i].device,
			dbg1,
			"CRDISPATCH_KEY:Device APC Violation Issue/",
			subsys_str
			);
	}

	/* unmask irq for module "i" */
	unmask_infra_module_irq(i);

}
#endif // AEE_FEATURE

static unsigned int sync_vio_dbg(int shift_bit)
{
	unsigned int shift_count = 0;
	unsigned int sync_done;

	mt_reg_sync_writel(0x1 << shift_bit, DEVAPC_PD_INFRA_VIO_SHIFT_SEL);
	mt_reg_sync_writel(0x1,	DEVAPC_PD_INFRA_VIO_SHIFT_CON);

	for (shift_count = 0; (shift_count < 100) &&
		((readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON) & 0x3) != 0x3);
		++shift_count)
		DEVAPC_DBG_MSG("[DEVAPC] Syncing INFRA DBG0 & DBG1 (%d, %d)\n",
				shift_bit, shift_count);

	DEVAPC_DBG_MSG("[DEVAPC] VIO_SHIFT_SEL=0x%X, VIO_SHIFT_CON=0x%X\n",
			readl(DEVAPC_PD_INFRA_VIO_SHIFT_SEL),
			readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON));

	if ((readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON) & 0x3) == 0x3)
		sync_done = 1;
	else {
		sync_done = 0;
		DEVAPC_MSG("[DEVAPC] sync failed, shift_bit: %d\n",
				shift_bit);
	}

	/* disable shift mechanism */
	mt_reg_sync_writel(0x0, DEVAPC_PD_INFRA_VIO_SHIFT_CON);
	mt_reg_sync_writel(0x0, DEVAPC_PD_INFRA_VIO_SHIFT_SEL);
	mt_reg_sync_writel(0x1 << shift_bit, DEVAPC_PD_INFRA_VIO_SHIFT_STA);

	return sync_done;
}

#ifdef DBG_ENABLE
static void dump_backtrace(void)
{
	struct pt_regs *regs;

	if ((DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG)
		|| (enable_dynamic_one_core_violation_debug)) {
		DEVAPC_MSG("[DEVAPC] ====== %s ======\n",
			"Start dumping Device APC violation tracing");

		DEVAPC_MSG("[DEVAPC] ****** %s ******\n",
			"[All IRQ Registers]");
		regs = get_irq_regs();
		show_regs(regs);

		DEVAPC_MSG("[DEVAPC] ****** %s ******\n",
			"[All Current Task Stack]");
		show_stack(current, NULL);

		DEVAPC_MSG("[DEVAPC] ====== %s ======\n",
			"End of dumping Device APC violation tracing");
	}
}
#endif

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

	if (irq_number != devapc_infra_irq) {
		DEVAPC_MSG("[DEVAPC] (ERROR) irq_number %d %s\n",
				irq_number,
				"is not registered!");

		return IRQ_NONE;
	}
	print_vio_mask_sta();

	DEVAPC_DBG_MSG("[DEVAPC] VIO_SHIFT_STA: 0x%x\n",
			readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA));

	for (i = 0; i <= PD_INFRA_VIO_SHIFT_MAX_BIT; ++i) {
		if (readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA) & (0x1 << i)) {

			if (sync_vio_dbg(i) == 0)
				continue;

			DEVAPC_DBG_MSG("[DEVAPC] %s%X, %s%X, %s%X\n",
					"VIO_SHIFT_STA=0x",
					readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA),
					"VIO_SHIFT_SEL=0x",
					readl(DEVAPC_PD_INFRA_VIO_SHIFT_SEL),
					"VIO_SHIFT_CON=0x",
					readl(DEVAPC_PD_INFRA_VIO_SHIFT_CON));

			dbg0 = readl(DEVAPC_PD_INFRA_VIO_DBG0);
			dbg1 = readl(DEVAPC_PD_INFRA_VIO_DBG1);

			master_id = (dbg0 & INFRA_VIO_DBG_MSTID)
				>> INFRA_VIO_DBG_MSTID_START_BIT;
			domain_id = (dbg0 & INFRA_VIO_DBG_DMNID)
				>> INFRA_VIO_DBG_DMNID_START_BIT;
			write_violation = (dbg0 & INFRA_VIO_DBG_W_VIO)
				>> INFRA_VIO_DBG_W_VIO_START_BIT;
			read_violation = (dbg0 & INFRA_VIO_DBG_R_VIO)
				>> INFRA_VIO_DBG_R_VIO_START_BIT;
			vio_addr_high = (dbg0 & INFRA_VIO_ADDR_HIGH)
				>> INFRA_VIO_ADDR_HIGH_START_BIT;

			/* violation information */
			DEVAPC_MSG("%s%s%s%s%x %s%x, %s%x, %s%x\n",
					"[DEVAPC] Violation(",
					read_violation == 1?" R":"",
					write_violation == 1?" W ) - ":" ) - ",
					"Vio Addr:0x", dbg1,
					"High:0x", vio_addr_high,
					"Bus ID:0x", master_id,
					"Dom ID:0x", domain_id);

			DEVAPC_MSG("%s - %s%s, %s%i\n",
					"[DEVAPC] Violation",
					"Process:", current->comm,
					"PID:", current->pid);

			break;
		}
	}

	device_count = ARRAY_SIZE(devapc_infra_devices);

	/* checking and showing violation normal slaves */
	for (i = 0; i < device_count; i++) {
		if (devapc_infra_devices[i].enable_vio_irq == true
			&& check_infra_vio_status(i) == 1) {
			clear_infra_vio_status(i);
			DEVAPC_MSG("%s %s %s (%s=%d)\n",
					"[DEVAPC]",
					"Access Violation Slave:",
					devapc_infra_devices[i].device,
					"infra index",
					i);

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
			execute_aee(i, dbg0, dbg1);
#endif
		}
	}

#ifdef DBG_ENABLE
	dump_backtrace();
#endif
	return IRQ_HANDLED;
}
#endif //DEVAPC_TURN_ON

static int devapc_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
#if DEVAPC_TURN_ON
	int ret;
#endif

	DEVAPC_MSG("[DEVAPC] %s\n", __func__);

	if (devapc_pd_infra_base == NULL) {
		if (node) {
			devapc_pd_infra_base = of_iomap(node,
				DAPC_DEVICE_TREE_NODE_PD_INFRA_INDEX);
			devapc_infra_irq = irq_of_parse_and_map(node,
				DAPC_DEVICE_TREE_NODE_PD_INFRA_INDEX);
			DEVAPC_MSG("[DEVAPC] PD_INFRA_ADDRESS: %p, IRQ: %d\n",
				devapc_pd_infra_base, devapc_infra_irq);
		} else {
			pr_info("[DEVAPC] %s\n",
				"can't find DAPC_INFRA_PD compatible node");
			return -1;
		}
	}

#if DEVAPC_TURN_ON
	ret = request_irq(devapc_infra_irq, (irq_handler_t)devapc_violation_irq,
			IRQF_TRIGGER_LOW | IRQF_SHARED,
			"devapc", &g_devapc_ctrl);
	if (ret) {
		pr_info("[DEVAPC] Failed to request infra irq! (%d)\n", ret);
		return ret;
	}
#endif

	/* CCF */
#if DEVAPC_USE_CCF
	dapc_infra_clk = devm_clk_get(&pdev->dev, "devapc-infra-clock");
	if (IS_ERR(dapc_infra_clk)) {
		pr_info("[DEVAPC] (Infra) %s\n",
			"Cannot get devapc clock from common clock framework.");
		return PTR_ERR(dapc_infra_clk);
	}
	clk_prepare_enable(dapc_infra_clk);
#endif

#ifdef CONFIG_MTK_HIBERNATION
	register_swsusp_restore_noirq_func(ID_M_DEVAPC,
		devapc_pm_restore_noirq, NULL);
#endif

#if DEVAPC_TURN_ON
	start_devapc();
#endif

	return 0;
}

static int devapc_remove(struct platform_device *dev)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
	clk_disable_unprepare(dapc_infra_clk);
	return 0;
}

static int devapc_suspend(struct platform_device *dev, pm_message_t state)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
	return 0;
}

static int devapc_resume(struct platform_device *dev)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
	return 0;
}

#ifdef DBG_ENABLE
static ssize_t devapc_dbg_read(struct file *file, char __user *buffer,
	size_t count, loff_t *ppos)
{
	int ret;
	ssize_t retval = 0;
	char msg[256] = "DBG: dump devapc reg...\n";

	if (*ppos >= strlen(msg))
		return 0;

	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
	DEVAPC_MSG("[DEVAPC] start to dump devapc reg...\n");

	retval = simple_read_from_buffer(buffer, count, ppos, msg, strlen(msg));

	ret = mt_secure_call(MTK_SIP_LK_DAPC, 1, 0, 0, 0);
	if (ret == 0)
		DEVAPC_MSG("dump devapc reg success !\n");
	else
		DEVAPC_MSG("dump devapc reg failed !\n");

	return retval;
}

static ssize_t devapc_dbg_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	char input[32];
	char *pinput = NULL;
	char *tmp = NULL;
	long i;
	int len = 0, ret = 0;
	long slave_type = 0, domain = 0, index = 0;

	DEVAPC_MSG("[DEVAPC] debugging...\n");
	len = (count < (sizeof(input) - 1)) ? count : (sizeof(input) - 1);
	if (copy_from_user(input, buffer, len)) {
		pr_info("[DEVAPC] copy from user failed!\n");
		return -EFAULT;
	}

	input[len] = '\0';
	pinput = input;

	if (sysfs_streq(input, "0")) {
		enable_dynamic_one_core_violation_debug = 0;
		DEVAPC_MSG("[DEVAPC] One-Core Debugging: Disabled\n");
	} else if (sysfs_streq(input, "1")) {
		enable_dynamic_one_core_violation_debug = 1;
		DEVAPC_MSG("[DEVAPC] One-Core Debugging: Enabled\n");
	} else {
		tmp = strsep(&pinput, " ");
		if (tmp != NULL)
			i = kstrtol(tmp, 10, &slave_type);
		else
			slave_type = E_DAPC_OTHERS_SLAVE;

		if (slave_type >= E_DAPC_OTHERS_SLAVE) {
			pr_info("[DEVAPC] wrong input slave type\n");
			return -EFAULT;
		}
		DEVAPC_MSG("[DEVAPC] slave_type = %lu\n", slave_type);

		tmp = strsep(&pinput, " ");
		if (tmp != NULL)
			i = kstrtol(tmp, 10, &domain);
		else
			domain = E_DOMAIN_OTHERS;

		if (domain >= E_DOMAIN_OTHERS) {
			pr_info("[DEVAPC] wrong input domain type\n");
			return -EFAULT;
		}
		DEVAPC_MSG("[DEVAPC] domain id = %lu\n", domain);

		tmp = strsep(&pinput, " ");
		if (tmp != NULL)
			i = kstrtol(tmp, 10, &index);
		else
			index = 0xFFFFFFFF;

		if (index > DEVAPC_TOTAL_SLAVES) {
			pr_info("[DEVAPC] wrong input index type\n");
			return -EFAULT;
		}
		DEVAPC_MSG("[DEVAPC] slave index = %lu\n", index);

		ret = mt_secure_call(MTK_SIP_LK_DAPC, slave_type,
					domain, index, 0);

		DEVAPC_MSG("dump devapc_ao reg = 0x%x.\n", ret);
	}

	return count;
}
#endif

static int devapc_dbg_open(struct inode *inode, struct file *file)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
	return 0;
}

static const struct file_operations devapc_dbg_fops = {
	.owner = THIS_MODULE,
	.open  = devapc_dbg_open,
#ifdef DBG_ENABLE
	.write = devapc_dbg_write,
	.read = devapc_dbg_read,
#else
	.write = NULL,
	.read = NULL,
#endif
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
		pr_info("[DEVAPC] Unable to register driver (%d)\n", ret);
		return ret;
	}

	g_devapc_ctrl = cdev_alloc();
	if (!g_devapc_ctrl) {
		pr_info("[DEVAPC] Failed to add devapc device! (%d)\n", ret);
		platform_driver_unregister(&devapc_driver);
		return ret;
	}
	g_devapc_ctrl->owner = THIS_MODULE;

	proc_create("devapc_dbg", 0664, NULL, &devapc_dbg_fops);
	/* 0664: (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) */

	return 0;
}

/*
 * devapc_exit: module exit function.
 */
static void __exit devapc_exit(void)
{
	DEVAPC_MSG("[DEVAPC] %s.\n", __func__);
#ifdef CONFIG_MTK_HIBERNATION
	unregister_swsusp_restore_noirq_func(ID_M_DEVAPC);
#endif
}

arch_initcall(devapc_init);
module_exit(devapc_exit);
MODULE_LICENSE("GPL");
