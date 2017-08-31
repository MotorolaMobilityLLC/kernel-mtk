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
#define DEVAPC_TURN_ON         1
#define DEVAPC_USE_CCF         0

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
static struct clk *dapc_peri_clk;
#endif

static struct cdev *g_devapc_ctrl;
static unsigned int devapc_infra_irq;
static unsigned int devapc_peri_irq;
static void __iomem *devapc_pd_infra_base;
static void __iomem *devapc_pd_peri_base;

static unsigned int enable_dynamic_one_core_violation_debug;

#if DEVAPC_TURN_ON
static struct DEVICE_INFO devapc_infra_devices[] = {
	/* device name                          enable_vio_irq */
	{"INFRA_AO_INFRASYS_CONFIG_REGS",          true}, /* 0 */
	{"INFRA_AO_PMIC_WRAP",                     true},
	{"RESERVED",                               true},
	{"INFRA_AO_KEYPAD",                        true},
	{"INFRA_AO_APXGPT",                        true},
	{"INFRA_AO_AP_CIRQ_EINT",                  true},
	{"INFRA_AO_DEVICE_APC_MPU",                true},
	{"INFRA_AO_DEVICE_APC_AO",                 true},
	{"INFRA_AO_SEJ",                           true},
	{"RESERVED",                               true},

	{"INFRA_AO_TOP_MISC",                      true}, /* 10 */
	{"INFRA_AO_AES_TOP_0",                     true},
	{"INFRA_AO_AES_TOP_1",                     true},
	{"INFRA_AO_MDEM_TEMP_SHARE",               true},
	{"INFRA_SYSTEM_Timer",                     true},
	{"INFRA_AO_CLDMA_AO_TOP_AP",               true},
	{"INFRA_AO_CLDMA_AO_TOP_MD",               true},
	{"INFRA_AO_DVFS_CTRL_PROC",                true},
	{"INFRA_AO_SSPM_0",                        true},
	{"INFRA_AO_SSPM_1",                        true},

	{"INFRA_AO_SSPM_2",                        true}, /* 20 */
	{"INFRA_AO_SSPM_3",                        true},
	{"INFRA_AO_SSPM_4",                        true},
	{"INFRA_AO_SSPM_5",                        true},
	{"INFRA_AO_SSPM_6",                        true},
	{"INFRA_AO_SSPM_7",                        true},
	{"INFRA_AO_SSPM_8",                        true},
	{"INFRA_AO_SLEEP_CONTROL_PROCESSOR_0",     true},
	{"INFRA_AO_SLEEP_CONTROL_PROCESSOR_1",     true},
	{"INFRA_AO_SLEEP_CONTROL_PROCESSOR_2",     true},

	{"INFRA_AO_AUDIO_ANC_MD32",                true}, /* 30 */
	{"INFRA_AO_SSHUB",                         true},
	{"INFRA_PDN_MCSIB",                        true},
	{"Reserved",                               false},
	{"INFRASYS_TOP_MODULE",                    true},
	{"INFRASYS_SECURITY_GROUP",                true},
	{"INFRASYS_EMI_GROUP",                     true},
	{"INFRASYS_CCIF0_1_CLDMA_AP",              true},
	{"INFRASYS_CCIF2_3_AP",                    true},
	{"INFRASYS_CFG_GROUP",                     true},

	{"INFRASYS_SYS_CIRQ",                      true}, /* 40 */
	{"INFRASYS_GCE_GROUP",                     true},
	{"INFRASYS_SMI_GROUP",                     true},
	{"INFRASYS_PTP_Thermal_GROUP",             true},
	{"INFRASYS_MM_IOMMU_GROUP",                true},
	{"INFRASYS_PERISYS_IOMMU_GROUP",           true},
	{"INFRASYS_GPIO_GROUP",                    true},
	{"INFRASYS_BUS_TRACE_GROUP",               true},
	{"INFRASYS_EMI_MPU_REG",                   true},
	{"INFRASYS_LOW_POWER_GROUP",               true},

	{"CCIF4_5_GROUP",                          true}, /* 50 */
	{"RESERVED",                               true},
	{"INFRASYS_DRAMC_1_GROUP",                 true},
	{"INFRASYS_DRAMC_2_GROUP",                 true},
	{"INFRASYS_BSI_BPI_GROUP",                 true},
	{"RESERVED",                               true},
	{"INFRASYS_AUXADC",                        true},
	{"RESERVED",                               true},
	{"RESERVED",                               true},
	{"INFRASYS_CCIF0_MD1",                     true},

	{"INFRASYS_CCIF1_C2K",                     true}, /* 60 */
	{"INFRASYS_CCIF2_MD1",                     true},
	{"INFRASYS_CCIF3_C2K",                     true},
	{"INFRASYS_CLDMA_MD",                      true},
	{"INFRASYS_MD1_C2K_CCIF",                  true},
	{"INFRASYS_C2K_MD1_CCIF",                  true},
	{"INFRASYS_CCIF4",                         true},
	{"EAST_RESERVE_0",                         true},
	{"EAST_IO_RB",                             true},
	{"EAST_RESERVE_2",                         true},

	{"EAST_SSUSB_DIG_PHY_TOP",                 true}, /* 70 */
	{"EAST_USBSIF_TOP",                        true},
	{"EAST_RESERVE_5",                         true},
	{"EAST_RESERVE_6",                         true},
	{"EAST_RESERVE_7",                         true},
	{"EAST_RESERVE_8",                         true},
	{"EAST_RESERVE_9",                         true},
	{"EAST_RESERVE_A",                         true},
	{"EAST_RESERVE_B",                         true},
	{"EAST_MIPI_TX0_BASE",                     true},

	{"EAST_MIPI_TX1_BASE",                     true}, /* 80 */
	{"EAST_RESERVE_E",                         true},
	{"EAST_RESERVE_F",                         true},
	{"SOUTH_RESERVE_0",                        true},
	{"SOUTH_CARD_MPHY",                        true},
	{"SOUTH_UFS_MPHY",                         true},
	{"SOUTH_IO_BR",                            true},
	{"SOUTH_IO_BM2",                           true},
	{"SOUTH_IO_BM",                            true},
	{"RG_PERI_MSDC_PAD_MACRO",                 true},

	{"SOUTH_IO_BL",                            true}, /* 90 */
	{"SOUTH_IO_BL2",                           true},
	{"SOUTH_RESERVE_9",                        true},
	{"SOUTH_RESERVE_A",                        true},
	{"SOUTH_RESERVE_B",                        true},
	{"SOUTH_RESERVE_C",                        true},
	{"SOUTH_RESERVE_D",                        true},
	{"SOUTH_RESERVE_E",                        true},
	{"SOUTH_RESERVE_F",                        true},
	{"WEST_RESERVE_0",                         true},

	{"WEST_IO_LT",                             true}, /* 100 */
	{"WEST_PCIE_DIG_PHY_TOP",                  true},
	{"WEST_HDMI_TX_CONFIG",                    true},
	{"WEST_CSI",                               true},
	{"WEST_PCIE_DIG_PHY_TOP_P1",               true},
	{"WEST_RESERVE_6",                         true},
	{"WEST_RESERVE_7",                         true},
	{"WEST_RESERVE_8",                         true},
	{"WEST_RESERVE_9",                         true},
	{"WEST_RESERVE_A",                         true},

	{"WEST_RESERVE_B",                         true}, /* 110 */
	{"WEST_RESERVE_C",                         true},
	{"WEST_RESERVE_D",                         true},
	{"WEST_RESERVE_E",                         true},
	{"WEST_RESERVE_F",                         true},
	{"NORTH_IO_TR",                            true},
	{"NORTH_EFUSE_TOP",                        true},
	{"NORTH_RESERVE_2",                        true},
	{"NORTH_RESERVE_3",                        true},
	{"NORTH_RESERVE_4",                        true},

	{"NORTH_RESERVE_5",                        true}, /* 120 */
	{"NORTH_RESERVE_6",                        true},
	{"NORTH_RESERVE_7",                        true},
	{"NORTH_RESERVE_8",                        true},
	{"NORTH_RESERVE_9",                        true},
	{"NORTH_RESERVE_A",                        true},
	{"NORTH_RESERVE_B",                        true},
	{"NORTH_RESERVE_C",                        true},
	{"NORTH_RESERVE_D",                        true},
	{"NORTH_RESERVE_E",                        true},

	{"NORTH_RESERVE_F",                        true}, /* 130 */
	{"INFRASYS_AUDIO",                         true},
	{"MD1",                                    true},
	{"MD3(C2K)",                               true},
	{"MFG_rgx_jones_0",                        true},
	{"MFG_rgx_jones_1",                        true},
	{"MFG_rgx_jones_2",                        true},
	{"MFG_rgx_jones_3",                        true},
	{"MFG_rgx_jones_4",                        true},
	{"MFG_rgx_jones_5",                        true},

	{"MFG_rgx_jones_6",                        true}, /* 140 */
	{"MFG_rgx_jones_7",                        true},
	{"MFG_DVFSINFO",                           true},
	{"MFG_VAD",                                true},
	{"MFG_TOP_CONFIG",                         true},
	{"MFG_DFP",                                true},
	{"MJC_CONFIG",                             true},
	{"MJC_TOP",                                true},
	{"SMI_LARB8",                              true},
	{"MJC_IP_DFP",                             true},

	{"MJC_IP_VAD",                             true}, /* 150 */
	{"IPUSYS_TOP",                             true},
	{"IPU DMEM",                               true},
	{"IPU DMEM",                               true},
	{"IPU DMEM",                               true},
	{"IPU Core",                               true},
	{"IPUSYS  Power Meter",                    true},
	{"MMSYS_CONFIG",                           true},
	{"MDP_RDMA0",                              true},
	{"MDP_RDMA1",                              true},

	{"MDP_RSZ0",                               true}, /* 160 */
	{"MDP_RSZ1",                               true},
	{"MDP_RSZ2",                               true},
	{"MDP_WDMA",                               true},
	{"MDP_WROT0",                              true},
	{"MDP_WROT1",                              true},
	{"MDP_TDSHP",                              true},
	{"MDP_COLOR",                              true},
	{"DISP_OVL0",                              true},
	{"DISP_OVL1",                              true},

	{"DISP_OVL0_2L",                           true}, /* 170 */
	{"DISP_OVL1_2L",                           true},
	{"DISP_RDMA0",                             true},
	{"DISP_RDMA1",                             true},
	{"DISP_RDMA2",                             true},
	{"DISP_WDMA0",                             true},
	{"DISP_WDMA1",                             true},
	{"DISP_COLOR0",                            true},
	{"DISP_COLOR1",                            true},
	{"DISP_CCORR0",                            true},

	{"DISP_CCORR1",                            true}, /* 180 */
	{"DISP_AAL0",                              true},
	{"DISP_AAL1",                              true},
	{"DISP_GAMMA0",                            true},
	{"DISP_GAMMA1",                            true},
	{"DISP_OD",                                true},
	{"DISP_DITHER0",                           true},
	{"DISP_DITHER1",                           true},
	{"DISP_UFOE",                              true},
	{"DISP_DSC",                               true},

	{"DISP_SPLIT",                             true}, /* 190 */
	{"DSI0",                                   true},
	{"DSI1",                                   true},
	{"DPI0",                                   true},
	{"MM_MUTEX",                               true},
	{"SMI_LARB0",                              true},
	{"SMI_LARB1",                              true},
	{"SMI_COMMON",                             true},
	{"MMSYS_VAD",                              true},
	{"MMSYS_DFP",                              true},

	{"DISP_RSZ0",                              true}, /* 200 */
	{"DISP_RSZ1",                              true},
	{"MM_Reserved1",                           true},
	{"MM_Reserved2",                           true},
	{"MM_Reserved3",                           true},
	{"imgsys_top",                             true},
	{"smi_larb5",                              true},
	{"dip_a0",                                 true},
	{"dip_a1",                                 true},
	{"dip_a_nbc",                              true},

	{"VAD",                                    true}, /* 210 */
	{"dpe",                                    true},
	{"rsc",                                    true},
	{"WPE_A",                                  true},
	{"fdvt",                                   true},
	{"GEPF",                                   true},
	{"EAF",                                    true},
	{"pwr meter",                              true},
	{"Smi_larb2",                              true},
	{"vdec_top_global_con",                    true},

	{"vdec_top_smi_larb4",                     true}, /* 220 */
	{"vdec_top_full_top",                      true},
	{"vdec_top_vad",                           true},
	{"vdec_top_dpm",                           true},
	{"venc_global_con",                        true},
	{"smi_larb3",                              true},
	{"venc",                                   true},
	{"jpgenc",                                 true},
	{"jpgdec",                                 true},
	{"Venc_dfp_top",                           true},

	{"Venc_dfp_top",                           true}, /* 230 */
	{"camsys top",                             true},
	{"smi_larb6",                              true},
	{"smi_larb3",                              true},
	{"cam_top",                                true},
	{"cam_a",                                  true},
	{"cam_b",                                  true},
	{"cam_c",                                  true},
	{"cam_d",                                  true},
	{"camsys_dfp",                             true},

	{"camsys_vad",                             true}, /* 240 */
	{"unused",                                 true},
	{"cam_top_set",                            true},
	{"cam_a_set",                              true},
	{"cam_b_set",                              true},
	{"cam_c_set",                              true},
	{"cam_d_set",                              true},
	{"cam_top_inner",                          true},
	{"cam_a_inner",                            true},
	{"cam_b_inner",                            true},

	{"cam_c_inner",                            true}, /* 250 */
	{"cam_d_inner",                            true},
	{"Unused",                                 true},
	{"Unused",                                 true},
	{"Unused",                                 true},
	{"cam_top_clr",                            true},
	{"cam_a_clr",                              true},
	{"cam_b_clr",                              true},
	{"cam_c_clr",                              true},
	{"cam_d_clr",                              true},

	{"seninf_a",                               true}, /* 260 */
	{"seninf_b",                               true},
	{"seninf_c",                               true},
	{"seninf_d",                               true},
	{"seninf_e",                               true},
	{"seninf_f",                               true},
	{"seninf_g",                               true},
	{"seninf_h",                               true},
	{"camsv_a",                                true},
	{"camsv_b",                                true},

	{"camsv_c",                                true}, /* 270 */
	{"camsv_d",                                true},
	{"camsv_e",                                true},
	{"camsv_f",                                true},
	{"MD32 DMEM (0~64KB)",                     true},
	{"MD32 DMEM (64~96KB)",                    true},
	{"MD32 PMEM (32KB)",                       true},
	{"MD32(ip, external) + TSF",               true},
	{"mm_devapc_mpu_si0_device_vio_in",        false},
	{"mm_devapc_mpu_si1_device_vio_in",        false},

	{"mfg_devapc_mpu_si0_device_vio_in",       false}, /* 280 */
	{"mfg_devapc_mpu_si1_device_vio_in",       false},
	{"peri_devapc_mpu_si0_device_vio_in",      false},
	{"INFRASYS_CCIF5",                         true},

};


static struct DEVICE_INFO devapc_peri_devices[] = {
	/* device name                          enable_vio_irq */
	{"APDMA",                                  true}, /* 0 */
	{"Pericfg_reg",                            true},
	{"UART0",                                  true},
	{"UART1",                                  true},
	{"UART2",                                  true},
	{"UART3",                                  true},
	{"UART4",                                  true},
	{"PWM",                                    true},
	{"I2C0",                                   true},
	{"I2C1",                                   true},

	{"Reserved",                               true}, /* 10 */
	{"SPI6",                                   true},
	{"I2C4",                                   true},
	{"I2C5",                                   true},
	{"I2C6 ( I2C treble)",                     true},
	{"SPI0",                                   true},
	{"I2C3",                                   true},
	{"SPI8",                                   true},
	{"IRTX",                                   true},
	{"SPI7",                                   true},

	{"DISP_PWM0",                              true}, /* 20 */
	{"DISP_PWM1",                              true},
	{"SPI1",                                   true},
	{"SPI2",                                   true},
	{"UART5",                                  true},
	{"I2C7",                                   true},
	{"Peri_devapc",                            true},
	{"SPI3",                                   true},
	{"SPI4",                                   true},
	{"SPI5",                                   true},

	{"I2C8",                                   true}, /* 30 */
	{"I2C9",                                   true},
	{"USB3.0 PORT0 CSR and USB_APC_CON",       true},
	{"USB2.0 PORT1",                           true},
	{"Reserved",                               true},
	{"MSDC0",                                  true},
	{"MSDC1",                                  true},
	{"RESERVE",                                true},
	{"MSDC3",                                  true},
	{"Reserved",                               true},

	{"UFS_DEV",                                true}, /* 40 */
	{"USB3.0 PORT0 SIFSLV",                    true},
	{"RESERVE",                                true},
	{"RESERVE",                                true},
	{"RESERVE",                                true},
	{"UFO_ZIP",                                true},
	{"DX_CC_Public",                           true},
	{"DX_CC_Private",                          true},
	{"CQDMA",                                  true},
	{"AESFDE",                                 true},

	{"Peri_sub_common",                        true}, /* 50 */
	{"SPI9",                                   true},
	{"VAD",                                    true},
	{"I2C2(I3C)",                              true},
	{"RESERVE",                                true},
	{"RESERVE",                                true},
	{"RESERVE",                                true},
	{"RESERVE",                                true},
	{"RESERVE",                                true},
	{"RESERVE",                                true},

	{"RESERVE",                                true}, /* 60 */
	{"NOR flash",                              true},
	{"PERI_MBIST",                             true},
	{"RESERVE",                                true},
	{"PERI_POWER_METER0",                      true},
	{"RESERVED",                               true},
	{"PCIE_MAC0",                              true},
	{"NOR flash (AXI slave)",                  true},
	{"DEBUG_TOP",                              true},

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

	if (devapc_peri_irq != 0) {
		mt_irq_set_sens(devapc_peri_irq, MT_LEVEL_SENSITIVE);
		mt_irq_set_polarity(devapc_peri_irq, MT_POLARITY_LOW);
	}

	return 0;
}
#endif

#if DEVAPC_TURN_ON
static void unmask_infra_module_irq(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (apc_index > PD_INFRA_VIO_MASK_MAX_REG_INDEX) {
		pr_err("[DEVAPC] unmask_infra_module_irq: module overflow!\n");
		return;
	}

	*DEVAPC0_PD_INFRA_VIO_MASK(apc_index) &= (0xFFFFFFFF ^ (1 << apc_bit_index));
}

static void unmask_peri_module_irq(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (apc_index > PD_PERI_VIO_MASK_MAX_REG_INDEX) {
		pr_err("[DEVAPC] unmask_peri_module_irq: module overflow!\n");
		return;
	}

	*DEVAPC0_PD_PERI_VIO_MASK(apc_index) &= (0xFFFFFFFF ^ (1 << apc_bit_index));
}

static int clear_infra_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (apc_index > PD_INFRA_VIO_STA_MAX_REG_INDEX) {
		pr_err("[DEVAPC] clear_infra_vio_status: module overflow!\n");
		return -1;
	}

	*DEVAPC0_PD_INFRA_VIO_STA(apc_index) = (0x1 << apc_bit_index);

	return 0;
}

static int clear_peri_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (apc_index > PD_PERI_VIO_STA_MAX_REG_INDEX) {
		pr_err("[DEVAPC] clear_peri_vio_status: module overflow!\n");
		return -1;
	}

	*DEVAPC0_PD_PERI_VIO_STA(apc_index) = (0x1 << apc_bit_index);

	return 0;
}

static int check_infra_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;
	unsigned int vio_status = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (apc_index > PD_INFRA_VIO_STA_MAX_REG_INDEX) {
		pr_err("[DEVAPC] check_infra_vio_status: module overflow!\n");
		return -1;
	}

	vio_status = (*DEVAPC0_PD_INFRA_VIO_STA(apc_index) & (0x1 << apc_bit_index));

	if (vio_status)
		return 1;

	return 0;
}

static int check_peri_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;
	unsigned int vio_status = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (apc_index > PD_PERI_VIO_STA_MAX_REG_INDEX) {
		pr_err("[DEVAPC] check_peri_vio_status: module overflow!\n");
		return -1;
	}

	vio_status = (*DEVAPC0_PD_PERI_VIO_STA(apc_index) & (0x1 << apc_bit_index));

	if (vio_status)
		return 1;

	return 0;
}


static void start_devapc(void)
{
	unsigned int i;

	mt_reg_sync_writel(readl(DEVAPC0_PD_INFRA_APC_CON) & (0xFFFFFFFF ^ (1 << 2)), DEVAPC0_PD_INFRA_APC_CON);
	mt_reg_sync_writel(readl(DEVAPC0_PD_PERI_APC_CON) & (0xFFFFFFFF ^ (1 << 2)), DEVAPC0_PD_PERI_APC_CON);

	/* SMC call is called to set Device APC in LK instead */

	pr_err("[DEVAPC] INFRA VIO_STA 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x, 10:0x%x\n",
				   readl(DEVAPC0_PD_INFRA_VIO_STA(0)), readl(DEVAPC0_PD_INFRA_VIO_STA(1)),
				   readl(DEVAPC0_PD_INFRA_VIO_STA(2)), readl(DEVAPC0_PD_INFRA_VIO_STA(3)),
				   readl(DEVAPC0_PD_INFRA_VIO_STA(4)), readl(DEVAPC0_PD_INFRA_VIO_STA(5)),
				   readl(DEVAPC0_PD_INFRA_VIO_STA(6)), readl(DEVAPC0_PD_INFRA_VIO_STA(7)),
				   readl(DEVAPC0_PD_INFRA_VIO_STA(8)), readl(DEVAPC0_PD_INFRA_VIO_STA(9)),
				   readl(DEVAPC0_PD_INFRA_VIO_STA(10)));

	pr_err("[DEVAPC] PERI VIO_STA 0:0x%x, 1:0x%x, 2:0x%x\n",
			   readl(DEVAPC0_PD_PERI_VIO_STA(0)), readl(DEVAPC0_PD_PERI_VIO_STA(1)),
			   readl(DEVAPC0_PD_PERI_VIO_STA(2)));

	pr_err("[DEVAPC] INFRA VIO_MASK(B) 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x, 10:0x%x\n",
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(0)), readl(DEVAPC0_PD_INFRA_VIO_MASK(1)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(2)), readl(DEVAPC0_PD_INFRA_VIO_MASK(3)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(4)), readl(DEVAPC0_PD_INFRA_VIO_MASK(5)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(6)), readl(DEVAPC0_PD_INFRA_VIO_MASK(7)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(8)), readl(DEVAPC0_PD_INFRA_VIO_MASK(9)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(10)));

	pr_err("[DEVAPC] PERI VIO_MASK(B) 0:0x%x, 1:0x%x, 2:0x%x\n",
				   readl(DEVAPC0_PD_PERI_VIO_MASK(0)), readl(DEVAPC0_PD_PERI_VIO_MASK(1)),
				   readl(DEVAPC0_PD_PERI_VIO_MASK(2)));

	for (i = 0; i < ARRAY_SIZE(devapc_infra_devices); i++) {
		clear_infra_vio_status(i);
		if (true == devapc_infra_devices[i].enable_vio_irq)
			unmask_infra_module_irq(i);
	}

	for (i = 0; i < ARRAY_SIZE(devapc_peri_devices); i++) {
		clear_peri_vio_status(i);
		if (true == devapc_peri_devices[i].enable_vio_irq)
			unmask_peri_module_irq(i);
	}

	pr_err("[DEVAPC] INFRA VIO_MASK(A) 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x, 10:0x%x\n",
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(0)), readl(DEVAPC0_PD_INFRA_VIO_MASK(1)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(2)), readl(DEVAPC0_PD_INFRA_VIO_MASK(3)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(4)), readl(DEVAPC0_PD_INFRA_VIO_MASK(5)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(6)), readl(DEVAPC0_PD_INFRA_VIO_MASK(7)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(8)), readl(DEVAPC0_PD_INFRA_VIO_MASK(9)),
				   readl(DEVAPC0_PD_INFRA_VIO_MASK(10)));

	pr_err("[DEVAPC] PERI VIO_MASK(A) 0:0x%x, 1:0x%x, 2:0x%x\n",
				   readl(DEVAPC0_PD_PERI_VIO_MASK(0)), readl(DEVAPC0_PD_PERI_VIO_MASK(1)),
				   readl(DEVAPC0_PD_PERI_VIO_MASK(2)));

}

#endif

#if DEVAPC_TURN_ON
static irqreturn_t devapc_violation_irq(int irq_number, void *dev_id)
{
	unsigned int dbg0 = 0, dbg1 = 0;
	unsigned int master_id;
	unsigned int domain_id;
	unsigned int vio_addr_high;
	unsigned int read_violation;
	unsigned int write_violation;
	unsigned int device_count;
	unsigned int i;
	struct pt_regs *regs;

	if (irq_number == devapc_infra_irq) {

		dbg0 = readl(DEVAPC0_PD_INFRA_VIO_DBG0);
		dbg1 = readl(DEVAPC0_PD_INFRA_VIO_DBG1);
		master_id = (dbg0 & INFRA_VIO_DBG_MSTID) >> INFRA_VIO_DBG_MSTID_START_BIT;
		domain_id = (dbg0 & INFRA_VIO_DBG_DMNID) >> INFRA_VIO_DBG_DMNID_START_BIT;
		vio_addr_high = (dbg0 & INFRA_VIO_ADDR_HIGH) >> INFRA_VIO_ADDR_HIGH_START_BIT;
		write_violation = (dbg0 & INFRA_VIO_DBG_W) >> INFRA_VIO_DBG_W_START_BIT;
		read_violation = (dbg0 & INFRA_VIO_DBG_R) >> INFRA_VIO_DBG_R_START_BIT;

		/* violation information improvement (code should be less than 120 characters per line) */
		DEVAPC_VIO_MSG
			("%s,%s%s%s:%s, PID:%i, Vio Addr:0x%x (High:0x%x), Bus ID:0x%x, Dom ID:0x%x, DBG0:0x%x\n",
				"[DEVAPC] Violation(Infra",
				((read_violation  == 1) ? "R" : ""),
				((write_violation == 1) ? "W" : ""),
				") - Process",
				current->comm, current->pid, dbg1, vio_addr_high, master_id, domain_id,
				(*DEVAPC0_PD_INFRA_VIO_DBG0));

		DEVAPC_VIO_MSG
			("%s 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x, 8:0x%x, 9:0x%x, 10:0x%x\n",
				"[DEVAPC] VIO_STA",
				readl(DEVAPC0_PD_INFRA_VIO_STA(0)), readl(DEVAPC0_PD_INFRA_VIO_STA(1)),
				readl(DEVAPC0_PD_INFRA_VIO_STA(2)), readl(DEVAPC0_PD_INFRA_VIO_STA(3)),
				readl(DEVAPC0_PD_INFRA_VIO_STA(4)), readl(DEVAPC0_PD_INFRA_VIO_STA(5)),
				readl(DEVAPC0_PD_INFRA_VIO_STA(6)), readl(DEVAPC0_PD_INFRA_VIO_STA(7)),
				readl(DEVAPC0_PD_INFRA_VIO_STA(8)), readl(DEVAPC0_PD_INFRA_VIO_STA(9)),
				readl(DEVAPC0_PD_INFRA_VIO_STA(10)));

		device_count = ARRAY_SIZE(devapc_infra_devices);

		/* No need to check violation of EMI & EMI MPU slaves for Infra because they will not be unmasked */

		/* checking and showing violation normal slaves */
		for (i = 0; i < device_count; i++) {
			/* violation information improvement */

			if ((check_infra_vio_status(i)) && (devapc_infra_devices[i].enable_vio_irq == true)) {
				DEVAPC_VIO_MSG("[DEVAPC] Access Violation Slave: %s (infra index=%d)\n",
									devapc_infra_devices[i].device, i);
			}

			clear_infra_vio_status(i);
		}

		mt_reg_sync_writel(INFRA_VIO_DBG_CLR, DEVAPC0_PD_INFRA_VIO_DBG0);

		dbg0 = readl(DEVAPC0_PD_INFRA_VIO_DBG0);
		dbg1 = readl(DEVAPC0_PD_INFRA_VIO_DBG1);

	} else if (irq_number == devapc_peri_irq) {

		dbg0 = readl(DEVAPC0_PD_PERI_VIO_DBG0);
		dbg1 = readl(DEVAPC0_PD_PERI_VIO_DBG1);
		master_id = (dbg0 & PERI_VIO_DBG_MSTID) >> PERI_VIO_DBG_MSTID_START_BIT;
		domain_id = (dbg0 & PERI_VIO_DBG_DMNID) >> PERI_VIO_DBG_DMNID_START_BIT;
		vio_addr_high = (dbg0 & PERI_VIO_ADDR_HIGH) >> PERI_VIO_ADDR_HIGH_START_BIT;
		write_violation = (dbg0 & PERI_VIO_DBG_W) >> PERI_VIO_DBG_W_START_BIT;
		read_violation = (dbg0 & PERI_VIO_DBG_R) >> PERI_VIO_DBG_R_START_BIT;

		/* violation information improvement (code should be less than 120 characters per line) */
		DEVAPC_VIO_MSG
			("%s,%s%s%s:%s, PID:%i, Vio Addr:0x%x (High:0x%x), Bus ID:0x%x, Dom ID:0x%x, DBG0:0x%x\n",
				"[DEVAPC] Violation(Peri",
				((read_violation  == 1) ? "R" : ""),
				((write_violation == 1) ? "W" : ""),
				") - Process",
				current->comm, current->pid, dbg1, vio_addr_high, master_id, domain_id,
				(*DEVAPC0_PD_PERI_VIO_DBG0));

		DEVAPC_VIO_MSG("[DEVAPC] PERI VIO_STA 0:0x%x, 1:0x%x, 2:0x%x\n",
						readl(DEVAPC0_PD_PERI_VIO_STA(0)), readl(DEVAPC0_PD_PERI_VIO_STA(1)),
						readl(DEVAPC0_PD_PERI_VIO_STA(2)));

		device_count = ARRAY_SIZE(devapc_peri_devices);

		/* No need to check violation of EMI & EMI MPU slaves for Peri because they will not be unmasked */

		/* checking and showing violation normal slaves */
		for (i = 0; i < device_count; i++) {
			/* violation information improvement */

			if ((check_peri_vio_status(i)) && (devapc_peri_devices[i].enable_vio_irq == true)) {
				DEVAPC_VIO_MSG("[DEVAPC] Access Violation Slave: %s (peri index=%d)\n",
									devapc_peri_devices[i].device, i);
			}

			clear_peri_vio_status(i);
		}

		mt_reg_sync_writel(PERI_VIO_DBG_CLR, DEVAPC0_PD_PERI_VIO_DBG0);

		dbg0 = readl(DEVAPC0_PD_PERI_VIO_DBG0);
		dbg1 = readl(DEVAPC0_PD_PERI_VIO_DBG1);

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

	if ((dbg0 != 0) || (dbg1 != 0)) {
		DEVAPC_VIO_MSG("[DEVAPC] Multi-violation!\n");
		DEVAPC_VIO_MSG("[DEVAPC] DBG0 = %x, DBG1 = %x\n", dbg0, dbg1);
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
			DEVAPC_MSG("[DEVAPC] PD_INFRA_ADDRESS: %p, IRD: %d\n", devapc_pd_infra_base, devapc_infra_irq);
		} else {
			pr_err("[DEVAPC] can't find DAPC_INFRA_PD compatible node\n");
			return -1;
		}
	}

	if (devapc_pd_peri_base == NULL) {
		if (node) {
			devapc_pd_peri_base = of_iomap(node, DAPC_DEVICE_TREE_NODE_PD_PERI_INDEX);
			devapc_peri_irq = irq_of_parse_and_map(node, DAPC_DEVICE_TREE_NODE_PD_PERI_INDEX);
			DEVAPC_MSG("[DEVAPC] PD_PERI_ADDRESS: %p, IRD: %d\n", devapc_pd_peri_base, devapc_peri_irq);
		} else {
			pr_err("[DEVAPC] can't find DAPC_PERI_PD compatible node\n");
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

	ret = request_irq(devapc_peri_irq, (irq_handler_t) devapc_violation_irq,
			  IRQF_TRIGGER_LOW | IRQF_SHARED, "devapc", &g_devapc_ctrl);
	if (ret) {
		pr_err("[DEVAPC] Failed to request peri irq! (%d)\n", ret);
		return ret;
	}
#endif

/* CCF */
#if DEVAPC_USE_CCF
	dapc_infra_clk = devm_clk_get(&dev->dev, "devapc-infra-clock");
	if (IS_ERR(dapc_infra_clk)) {
		pr_err("[DEVAPC] (Infra) Cannot get devapc clock from common clock framework.\n");
		return PTR_ERR(dapc_infra_clk);
	}
	clk_prepare_enable(dapc_infra_clk);

	dapc_peri_clk = devm_clk_get(&dev->dev, "devapc-peri-clock");
	if (IS_ERR(dapc_peri_clk)) {
		pr_err("[DEVAPC] (Peri) Cannot get devapc clock from common clock framework.\n");
		return PTR_ERR(dapc_peri_clk);
	}
	clk_prepare_enable(dapc_peri_clk);
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
