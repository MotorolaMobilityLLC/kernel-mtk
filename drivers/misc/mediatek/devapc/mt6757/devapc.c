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

#ifdef CONFIG_MTK_CLKMGR
/* mt_clkmgr */
#include <mach/mt_clkmgr.h>
#else
/* CCF */
#include <linux/clk.h>
#endif

#include "mt_device_apc.h"
#include "mt_io.h"
#include "sync_write.h"
#include "mach/irqs.h"
#include "devapc.h"

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
#include <linux/aee.h>
#endif

/* 0 for early porting */
#define DEVAPC_TURN_ON         0

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



#ifndef CONFIG_MTK_CLKMGR
/* bypass clock! */
static struct clk *dapc_clk;
#endif

static struct cdev *g_devapc_ctrl;
static unsigned int devapc_irq;
static void __iomem *devapc_pd_base;

static unsigned int enable_dynamic_one_core_violation_debug;

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
static unsigned long long devapc_vio_first_trigger_time[DEVAPC_TOTAL_SLAVES];

/* violation times */
static unsigned int devapc_vio_count[DEVAPC_TOTAL_SLAVES];

/* show the status of whether AEE warning has been populated */
static unsigned int devapc_vio_aee_shown[DEVAPC_TOTAL_SLAVES];
static unsigned int devapc_vio_current_aee_trigger_times;
#endif

#if DEVAPC_TURN_ON
static struct DEVICE_INFO devapc_devices[] = {
	/*device name                          enable_vio_irq */
    /* 0 */
    {"INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR",      true},
    {"INFRA_AO_INFRASYS_CONFIG_REGS",           true},
    {"INFRA_AO_IO_CFG",                         true},
    {"INFRA_AO_PERICFG",                        true},
    {"RESERVE",                                 true},
    {"INFRA_AO_GPIO",                           true},
    {"INFRA_AO_TOP_LEVEL_SLP_MANAGER",          true},
    {"INFRA_AO_TOP_RGU",                        true},
    {"INFRA_AO_APXGPT",                         true},
    {"INFRA_AO_RESERVE_REGION",                 true},

    /* 10 */
    {"INFRA_AO_SEJ",                            true},
    {"INFRA_AO_AP_CIRQ_EINT",                   true},
    {"INFRA_AO_APMIXEDSYS_FHCTL",               true},
    {"INFRA_AO_PMIC_WRAP",                      true},
    {"INFRA_AO_DEVICE_APC_AO",                  true},
    {"RESERVE",                                 true},
    {"INFRA_AO_KEYPAD",                         true},
    {"INFRA_AO_TOP_MISC",                       true},
    {"INFRA_AO_RESERVE_REGION",                 true},
    {"INFRA_AO_RESERVE_REGION",                 true},

    /* 20 */
    {"INFRA_AO_CLDMA_AO_TOP_AP",                true},
    {"INFRA_AO_CLDMA_AO_TOP_MD",                true},
    {"INFRA_AO_AES_TOP0",                       true},
    {"INFRA_AO_AES_TOP1",                       true},
    {"INFRA_AO_MDEM_TEMP_SHARE",                true},
    {"RESERVE",                                 true},
    {"INFRASYS_MCUSYS_CONFIG_REG",              true},
    {"INFRASYS_MCUSYS_CONFIG_REG1",             true},
    {"INFRASYS_MCUSYS_CONFIG_REG2",             true},
    {"INFRASYS_MCUSYS_CONFIG_REG3",             true},

    /* 30 */
    {"INFRASYS_SYSTEM_CIRQ",                    true},
    {"INFRASYS_MM_IOMMU_CONFIGURATION",         true},
    {"INFRASYS_EFUSEC",                         true},
    {"INFRASYS_DEVICE_APC_MONITOR",             true},
    {"INFRASYS_DEBUG_TRACKER",                  true},
    {"INFRASYS_CCI0_AP",                        true},
    {"INFRASYS_CCI0_MD",                        true},
    {"INFRASYS_CCI1_AP",                        true},
    {"INFRASYS_CCI1_MD",                        true},
    {"INFRASYS_MBIST_CONTROL_REG",              true},

    /* 40 */
    {"INFRASYS_CONTROL_REG",                    true},
    {"INFRASYS_TRNG",                           true},
    {"INFRASYS_GCPU",                           true},
    {"INFRASYS_MD_CCIF_MD1",                    true},
    {"INFRASYS_GCE",                            true},
    {"INFRASYS_MD_CCIF_MD2",                    true},
    {"INFRASYS_BOOTROM/SRAM",                   true},
    {"INFRASYS_ANA_MIPI_DSI0",                  true},
    {"INFRASYS_ANA_MIPI_DSI1",                  true},
    {"INFRASYS_ANA_MIPI_CSI0",                  true},

    /* 50 */
    {"INFRASYS_ANA_MIPI_CSI1",                  true},
    {"INFRASYS_EMI_BUS_INTERFACE",              true},
    {"INFRASYS_GPU_RSA",                        true},
    {"INFRASYS_CLDMA_PDN_AP",                   true},
    {"INFRASYS_CLDMA_PDN_MD",                   true},
    {"INFRASYS_MDSYS_INTF",                     true},
    {"INFRASYS_BPI_BIS_SLV0",                   true},
    {"INFRASYS_BPI_BIS_SLV1",                   true},
    {"INFRASYS_BPI_BIS_SLV2",                   true},
    {"INFRAYS_EMI_MPU_REG",                     true},

    /* 60 */
    {"INFRAYS_DVFS_PROC",                       true},
    {"DRAMC_CH0_TOP0_CONFIGURATION",            true},
    {"DRAMC_CH0_TOP1_CONFIGURATION",            true},
    {"DRAMC_CH0_TOP2_CONFIGURATION",            true},
    {"DRAMC_CH0_TOP3_CONFIGURATION",            true},
    {"RESERVE",                                 true},
    {"DRAMC_CH1_TOP0_CONFIGURATION",            true},
    {"DRAMC_CH1_TOP1_CONFIGURATION",            true},
    {"DRAMC_CH1_TOP2_CONFIGURATION",            true},
    {"DRAMC_CH1_TOP3_CONFIGURATION",            true},

    /* 70 */
    {"RESERVE",                                 true},
    {"PERI_GPU_SMI_COMMON",                     true},
    {"RESERVE",                                 true},
    {"INFRASYS_DEBUGTOP",                       true},
    {"DMA",                                     true},
    {"AUXADC",                                  true},
    {"UART0",                                   true},
    {"UART1",                                   true},
    {"UART2",                                   true},
    {"RESERVE",                                 true},

    /* 80 */
    {"PWM",                                     true},
    {"I2C0",                                    true},
    {"I2C1",                                    true},
    {"I2C2",                                    true},
    {"SPI0",                                    true},
    {"PTP_THERMAL_CTL",                         true},
    {"BTIF",                                    true},
    {"IRTX",                                    true},
    {"DISP_PWM",                                true},
    {"I2C3",                                    true},

    /* 90 */
    {"SPI1",                                    true},
    {"I2C4",                                    true},
    {"SPI2",                                    true},
    {"SPI3",                                    true},
    {"I2C1_IMM",                                true},
    {"I2C2_IMM",                                true},
    {"I2C5",                                    true},
    {"I2C5_IMM",                                true},
    {"SPI4",                                    true},
    {"SPI5",                                    true},

    /* 100 */
    {"USB2.0",                                  true},
    {"USB2.0_SIF",                              true},
    {"MSDC0",                                   true},
    {"MSDC1",                                   true},
    {"MSDC2",                                   true},
    {"RESERVE",                                 true},
    {"USB3.0",                                  true},
    {"USB3.0_SIF",                              true},
    {"USB3.0_SIF2",                             true},
    {"AUDIO",                                   true},

    /* 110 */
    {"WCN_AHB_SLAVE",                           true},
    {"MD_PERIPHERALS",                          true},
    {"MD2_PERIPHERALS",                         true},
    {"RESERVE",                                 true},
    {"RESERVE",                                 true},
    {"RESERVE",                                 true},
    {"G3D_CONFIG_0",                            true},
    {"G3D_CONFIG_1",                            true},
    {"G3D_CONFIG_2",                            true},
    {"G3D_CONFIG_3",                            true},

    /* 120 */
    {"RESERVE",                                 true},
    {"G3D_POWER_CONTROL",                       true},
    {"MALI_CONFIG",                             true},
    {"MMSYS_CONFIG",                            true},
    {"MDP_RDMA0",                               true},
    {"MDP_RDMA1",                               true},
    {"RESERVE",                                 true},
    {"MDP_RSZ1",                                true},
    {"MDP_RSZ2",                                true},
    {"RESERVE",                                 true},

    /* 130 */
    {"MDP_WROT0",                               true},
    {"MDP_WROT1",                               true},
    {"MDP_TDSHP",                               true},
    {"MDP_COLOR",                               true},
    {"DISP_OVL0",                               true},
    {"DISP_OVL1",                               true},
    {"DISP_OVL0_2L",                            true},
    {"DISP_OVL1_2L",                            true},
    {"DISP_RDMA0",                              true},
    {"DISP_RDMA1",                              true},

    /* 140 */
    {"RESERVE",                                 true},
    {"DISP_WDMA0",                              true},
    {"DISP_WDMA1",                              true},
    {"DISP_COLOR0",                             true},
    {"RESERVE",                                 true},
    {"DISP_CCORR0",                             true},
    {"RESERVE",                                 true},
    {"DISP_AAL0",                               true},
    {"RESERVE",                                 true},
    {"DISP_GAMMA0",                             true},

    /* 150 */
    {"RESERVE",                                 true},
    {"RESERVE",                                 true},
    {"DISP_DITHER0",                            true},
    {"RESERVE",                                 true},
    {"DSI_UFOE",                                true},
    {"RESERVE",                                 true},
    {"DSI_SPLIT",                               true},
    {"DSI0",                                    true},
    {"DSI1",                                    true},
    {"DPI0",                                    true},

    /* 160 */
    {"MM_MUTEX",                                true},
    {"SMI_LARB0",                               true},
    {"SMI_LARB4",                               true},
    {"SMI_COMMON",                              true},
    {"RESERVE",                                 true},
    {"RESERVE",                                 true},
    {"IMGSYS_CONFIG",                           true},
    {"SMI_LARB5",                               true},
    {"DIP_A0",                                  true},
    {"DIP_A1",                                  true},

    /* 170 */
    {"DIP_A2",                                  true},
    {"RESERVE",                                 true},
    {"RESERVE",                                 true},
    {"VAD",                                     true},
    {"DPE",                                     true},
    {"RSC",                                     true},
    {"RESERVE",                                 true},
    {"FDVT",                                    true},
    {"GEPF",                                    true},
    {"RESERVE",                                 true},

    /* 180 */
    {"DFP",                                     true},
    {"RESERVE",                                 true},
    {"VDEC_GLOBAL_CON",                         true},
    {"SMI_LARB1",                               true},
    {"VDEC_FULL_TOP",                           true},
    {"VENC_GLOBAL_CON",                         true},
    {"SMI_LARB3",                               true},
    {"VENC",                                    true},
    {"JPEGENC",                                 true},
    {"JPEGDEC",                                 true},

    /* 190 */
    {"CAMSYS_TOP",                              true},
    {"LARB2",                                   true},
    {"CAM_TOP",                                 true},
    {"CAM_A",                                   true},
    {"CAM_B",                                   true},
    {"CAM_TOP_SET",                             true},
    {"CAM_A_SET",                               true},
    {"CAM_B_SET",                               true},
    {"CAM_TOP_INNER",                           true},
    {"CAM_A_INNER",                             true},

    /* 200 */
    {"CAM_B_INNER",                             true},
    {"CAM_TOP_CLR",                             true},
    {"CAM_A_CLR",                               true},
    {"CAM_B_CLR",                               true},
    {"SENINF_A",                                true},
    {"SENINF_B",                                true},
    {"RESERVE",                                 true},
    {"RESERVE",                                 true},
    {"RESERVE",                                 true},
    {"RESERVE",                                 true},

    /* 210 */
    {"RESERVE",                                 true},
    {"RESERVE",                                 true},
    {"CAMSV_A",                                 true},
    {"CAMSV_B",                                 true},
    {"CAMSV_C",                                 true},
    {"CAMSV_D",                                 true},
    {"CAMSV_E",                                 true},
    {"CAMSV_F",                                 true},
    {"TSF",                                     true},
    {"CAMSYS_OTHERS",                           true},

    /* 220 */
    {"RESERVE",                                 true},
};
#endif

/*****************************************************************************
*FUNCTION DEFINITION
*****************************************************************************/
#if DEVAPC_TURN_ON
static int clear_vio_status(unsigned int module);
#endif
static int devapc_ioremap(void);

/**************************************************************************
*EXTERN FUNCTION
**************************************************************************/
int mt_devapc_emi_initial(void)
{
	devapc_ioremap();

	if (NULL != devapc_pd_base) {
		mt_reg_sync_writel(readl(IOMEM(DEVAPC0_PD_APC_CON)) & (0xFFFFFFFF ^ (1 << 2)),
				   DEVAPC0_PD_APC_CON);
		mt_reg_sync_writel(ABORT_EMI, DEVAPC0_D0_VIO_STA_7);
		mt_reg_sync_writel(ABORT_EMI_MPU, DEVAPC0_D0_VIO_STA_7);

		/* Notice: You cannot unmask Type B slave (e.g. EMI, EMI_MPU) unless unregistering IRQ */
		/* EMI and EMI_MPU are moved from Type B to Type 2 slave */

		pr_err("[DEVAPC] EMI_MPU Init done\n");
		return 0;
	}

	return -1;
}

int mt_devapc_check_emi_violation(void)
{
	if ((readl(IOMEM(DEVAPC0_D0_VIO_STA_7)) & ABORT_EMI) == 0)
		return -1;

	DEVAPC_VIO_MSG("EMI violation...\n");
	return 0;
}

int mt_devapc_check_emi_mpu_violation(void)
{
	if ((readl(IOMEM(DEVAPC0_D0_VIO_STA_7)) & ABORT_EMI_MPU) == 0)
		return -1;

	DEVAPC_VIO_MSG("EMI_MPU violation...\n");
	return 0;
}

int mt_devapc_clear_emi_violation(void)
{
	if ((readl(IOMEM(DEVAPC0_D0_VIO_STA_7)) & ABORT_EMI) != 0)
		mt_reg_sync_writel(ABORT_EMI, DEVAPC0_D0_VIO_STA_7);

	return 0;
}

int mt_devapc_clear_emi_mpu_violation(void)
{
	if ((readl(IOMEM(DEVAPC0_D0_VIO_STA_7)) & ABORT_EMI_MPU) != 0)
		mt_reg_sync_writel(ABORT_EMI_MPU, DEVAPC0_D0_VIO_STA_7);

	return 0;
}

/**************************************************************************
*STATIC FUNCTION
**************************************************************************/

static int devapc_ioremap(void)
{
	if (NULL == devapc_pd_base) {
		struct device_node *node = NULL;
		/*IO remap */

		node = of_find_compatible_node(NULL, NULL, "mediatek,devapc");
		if (node) {
			devapc_pd_base = of_iomap(node, 0);
			devapc_irq = irq_of_parse_and_map(node, 0);
			DEVAPC_MSG("[DEVAPC] PD_ADDRESS %p, IRD: %d\n", devapc_pd_base, devapc_irq);
		} else {
			pr_err("[DEVAPC] can't find DAPC_PD compatible node\n");
			return -1;
		}
	}

	return 0;
}

#ifdef CONFIG_MTK_HIBERNATION
static int devapc_pm_restore_noirq(struct device *device)
{
	if (devapc_irq != 0) {
		mt_irq_set_sens(devapc_irq, MT_LEVEL_SENSITIVE);
		mt_irq_set_polarity(devapc_irq, MT_POLARITY_LOW);
	}

	return 0;
}
#endif

#if DEVAPC_TURN_ON
static void unmask_module_irq(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC*2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC*2);

	switch (apc_index) {
	case 0:
		*DEVAPC0_D0_VIO_MASK_0 &= (0xFFFFFFFF ^ (1 << apc_bit_index));
		break;
	case 1:
		*DEVAPC0_D0_VIO_MASK_1 &= (0xFFFFFFFF ^ (1 << apc_bit_index));
		break;
	case 2:
		*DEVAPC0_D0_VIO_MASK_2 &= (0xFFFFFFFF ^ (1 << apc_bit_index));
		break;
	case 3:
		*DEVAPC0_D0_VIO_MASK_3 &= (0xFFFFFFFF ^ (1 << apc_bit_index));
		break;
	case 4:
		*DEVAPC0_D0_VIO_MASK_4 &= (0xFFFFFFFF ^ (1 << apc_bit_index));
		break;
	case 5:
		*DEVAPC0_D0_VIO_MASK_5 &= (0xFFFFFFFF ^ (1 << apc_bit_index));
		break;
	case 6:
		*DEVAPC0_D0_VIO_MASK_6 &= (0xFFFFFFFF ^ (1 << apc_bit_index));
		break;
	case 7:
		*DEVAPC0_D0_VIO_MASK_7 &= (0xFFFFFFFF ^ (1 << apc_bit_index));
		break;
	default:
		pr_err("[DEVAPC] unmask_module_irq: module overflow!\n");
		break;
	}
}

static void start_devapc(void)
{
	unsigned int i;

	mt_reg_sync_writel(readl(DEVAPC0_PD_APC_CON) & (0xFFFFFFFF ^ (1 << 2)), DEVAPC0_PD_APC_CON);

	/* SMC call is called in LK instead */

	for (i = 0; i < (ARRAY_SIZE(devapc_devices)); i++) {
	    clear_vio_status(i);
		if (true == devapc_devices[i].enable_vio_irq)
			unmask_module_irq(i);
	}

	pr_err("[DEVAPC] start_devapc: Current VIO_STA 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x\n",
		       readl(DEVAPC0_D0_VIO_STA_0), readl(DEVAPC0_D0_VIO_STA_1),
		       readl(DEVAPC0_D0_VIO_STA_2), readl(DEVAPC0_D0_VIO_STA_3),
		       readl(DEVAPC0_D0_VIO_STA_4), readl(DEVAPC0_D0_VIO_STA_5),
			   readl(DEVAPC0_D0_VIO_STA_6), readl(DEVAPC0_D0_VIO_STA_7));

	pr_err("[DEVAPC] start_devapc: Current VIO_MASK 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x\n",
		       readl(DEVAPC0_D0_VIO_MASK_0), readl(DEVAPC0_D0_VIO_MASK_1),
		       readl(DEVAPC0_D0_VIO_MASK_2), readl(DEVAPC0_D0_VIO_MASK_3),
		       readl(DEVAPC0_D0_VIO_MASK_4), readl(DEVAPC0_D0_VIO_MASK_5),
			   readl(DEVAPC0_D0_VIO_MASK_6), readl(DEVAPC0_D0_VIO_MASK_7));
}

static int clear_vio_status(unsigned int module)
{

	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	switch (apc_index) {
	case 0:
		*DEVAPC0_D0_VIO_STA_0 = (0x1 << apc_bit_index);
		break;
	case 1:
		*DEVAPC0_D0_VIO_STA_1 = (0x1 << apc_bit_index);
		break;
	case 2:
		*DEVAPC0_D0_VIO_STA_2 = (0x1 << apc_bit_index);
		break;
	case 3:
		*DEVAPC0_D0_VIO_STA_3 = (0x1 << apc_bit_index);
		break;
	case 4:
		*DEVAPC0_D0_VIO_STA_4 = (0x1 << apc_bit_index);
		break;
	case 5:
		*DEVAPC0_D0_VIO_STA_5 = (0x1 << apc_bit_index);
		break;
	case 6:
		*DEVAPC0_D0_VIO_STA_6 = (0x1 << apc_bit_index);
		break;
	case 7:
		*DEVAPC0_D0_VIO_STA_7 = (0x1 << apc_bit_index);
		break;
	default:
	    pr_err("[DEVAPC] clear_vio_status: module overflow!\n");
		break;
	}

	return 0;
}

static int check_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;
	unsigned int vio_status = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	switch (apc_index) {
	case 0:
		vio_status = (*DEVAPC0_D0_VIO_STA_0 & (0x1 << apc_bit_index));
		break;
	case 1:
		vio_status = (*DEVAPC0_D0_VIO_STA_1 & (0x1 << apc_bit_index));
		break;
	case 2:
		vio_status = (*DEVAPC0_D0_VIO_STA_2 & (0x1 << apc_bit_index));
		break;
	case 3:
		vio_status = (*DEVAPC0_D0_VIO_STA_3 & (0x1 << apc_bit_index));
		break;
	case 4:
		vio_status = (*DEVAPC0_D0_VIO_STA_4 & (0x1 << apc_bit_index));
		break;
	case 5:
		vio_status = (*DEVAPC0_D0_VIO_STA_5 & (0x1 << apc_bit_index));
		break;
	case 6:
		vio_status = (*DEVAPC0_D0_VIO_STA_6 & (0x1 << apc_bit_index));
		break;
	case 7:
		vio_status = (*DEVAPC0_D0_VIO_STA_7 & (0x1 << apc_bit_index));
		break;
	default:
	    pr_err("[DEVAPC] check_vio_status: module overflow!\n");
		break;
	}

	if (vio_status)
		return 1;

	return 0;
}
#endif

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
static void evaluate_aee_warning(unsigned int i, unsigned int dbg1)
{
	char aee_str[256];
	unsigned long long current_time;

	if (devapc_vio_aee_shown[i] == 0) {
		if (devapc_vio_count[i] < DEVAPC_VIO_AEE_TRIGGER_TIMES) {
			devapc_vio_count[i]++;

			if (devapc_vio_count[i] == 1) {
				/* violation for this slave is triggered the first time */

				/* get current time from start-up in ns */
				devapc_vio_first_trigger_time[i] = sched_clock();
#if 0
				DEVAPC_VIO_MSG("[DEVAPC] devapc_vio_first_trigger_time: %llu\n",
					devapc_vio_first_trigger_time[i] / 1000000); /* ms */
#endif
			}
		}

		if (devapc_vio_count[i] >= DEVAPC_VIO_AEE_TRIGGER_TIMES) {
			current_time = sched_clock(); /* get current time from start-up in ns */
#if 0
			DEVAPC_VIO_MSG("[DEVAPC] current_time: %llu\n",
				current_time / 1000000); /* ms */
			DEVAPC_VIO_MSG("[DEVAPC] devapc_vio_count[%d]: %d\n",
				i, devapc_vio_count[i]);
#endif
			if (((current_time - devapc_vio_first_trigger_time[i]) / 1000000) <=
					(unsigned long long)DEVAPC_VIO_AEE_TRIGGER_FREQUENCY) {  /* diff time by ms */
				/* Mark the flag for showing AEE (AEE should be shown only once) */
				devapc_vio_aee_shown[i] = 1;

				DEVAPC_VIO_MSG("[DEVAPC] Populating AEE Warning...\n");

				if (devapc_vio_current_aee_trigger_times <
						DEVAPC_VIO_MAX_TOTAL_MODULE_AEE_TRIGGER_TIMES) {
					devapc_vio_current_aee_trigger_times++;

					sprintf(aee_str, "[DEVAPC] Access Violation Slave: %s (index=%d)\n",
						devapc_devices[i].device, i);

					aee_kernel_warning(aee_str,
						"%s\nAccess Violation Slave: %s\nVio Addr: 0x%x\n%s%s\n",
						"Device APC Violation",
						devapc_devices[i].device,
						dbg1,
						"CRDISPATCH_KEY:Device APC Violation Issue/",
						devapc_devices[i].device
					);
				}
			}

			devapc_vio_count[i] = 0;
		}
	}
}
#endif

#if DEVAPC_TURN_ON
static irqreturn_t devapc_violation_irq(int irq, void *dev_id)
{
	unsigned int dbg0 = 0, dbg1 = 0;
	unsigned int master_id;
	unsigned int domain_id;
	unsigned int r_w_violation;
	unsigned int device_count;
	unsigned int i;
	struct pt_regs *regs;

	dbg0 = readl(DEVAPC0_VIO_DBG0);
	dbg1 = readl(DEVAPC0_VIO_DBG1);
	master_id = (dbg0 & VIO_DBG_MSTID) >> 0;
	domain_id = (dbg0 & VIO_DBG_DMNID) >> 16;
	r_w_violation = (dbg0 & VIO_DBG_W) >> 28;

	/* violation information improvement */
	if (1 == r_w_violation) {
		DEVAPC_VIO_MSG
		    ("[DEVAPC] Violation(W) - Process:%s, PID:%i, Vio Addr:0x%x, Bus ID:0x%x, Dom ID:0x%x, DBG0:0x%x\n",
		     current->comm, current->pid, dbg1, master_id, domain_id, (*DEVAPC0_VIO_DBG0));
	} else {
		DEVAPC_VIO_MSG
		    ("[DEVAPC] Violation(R) - Process:%s, PID:%i, Vio Addr:0x%x, Bus ID:0x%x, Dom ID:0x%x, DBG0:0x%x\n",
		     current->comm, current->pid, dbg1, master_id, domain_id, (*DEVAPC0_VIO_DBG0));
	}

	DEVAPC_VIO_MSG("[DEVAPC] VIO_STA 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x, 5:0x%x, 6:0x%x, 7:0x%x\n",
		       readl(DEVAPC0_D0_VIO_STA_0), readl(DEVAPC0_D0_VIO_STA_1),
		       readl(DEVAPC0_D0_VIO_STA_2), readl(DEVAPC0_D0_VIO_STA_3),
		       readl(DEVAPC0_D0_VIO_STA_4), readl(DEVAPC0_D0_VIO_STA_5),
			   readl(DEVAPC0_D0_VIO_STA_6), readl(DEVAPC0_D0_VIO_STA_7));

	device_count = (sizeof(devapc_devices) / sizeof(devapc_devices[0]));

	/* checking and showing violation EMI & EMI MPU slaves */
	if (check_vio_status(INDEX_EMI))
		DEVAPC_VIO_MSG("[DEVAPC] Access Violation Slave: EMI (index=%d)\n", INDEX_EMI);

	if (check_vio_status(INDEX_EMI_MPU))
		DEVAPC_VIO_MSG("[DEVAPC] Access Violation Slave: EMI_MPU (index=%d)\n", INDEX_EMI_MPU);

	/* checking and showing violation normal slaves */
	for (i = 0; i < device_count; i++) {
		/* violation information improvement */

		if ((check_vio_status(i)) && (devapc_devices[i].enable_vio_irq == true)) {
			DEVAPC_VIO_MSG("[DEVAPC] Access Violation Slave: %s (index=%d)\n",
								devapc_devices[i].device, i);

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)
			/* Frequency-based Violation AEE Warning (Under the condition that the violation
			 * for the module is not shown, it will trigger the AEE if "x" violations in "y" ms) */
			evaluate_aee_warning(i, dbg1);
#endif
		}

		clear_vio_status(i);
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

	mt_reg_sync_writel(VIO_DBG_CLR, DEVAPC0_VIO_DBG0);
	dbg0 = readl(DEVAPC0_VIO_DBG0);
	dbg1 = readl(DEVAPC0_VIO_DBG1);

	if ((dbg0 != 0) || (dbg1 != 0)) {
		DEVAPC_VIO_MSG("[DEVAPC] Multi-violation!\n");
		DEVAPC_VIO_MSG("[DEVAPC] DBG0 = %x, DBG1 = %x\n", dbg0, dbg1);
	}

	return IRQ_HANDLED;
}
#endif

static int devapc_probe(struct platform_device *dev)
{
#if DEVAPC_TURN_ON
	int ret;
#endif

	DEVAPC_MSG("[DEVAPC] module probe.\n");

	/*IO remap */
	devapc_ioremap();

	/*Temporarily disable Device APC kernel driver for bring-up */
	/*
	 * Interrupts of vilation (including SPC in SMI, or EMI MPU) are triggered by the device APC.
	 * need to share the interrupt with the SPC driver.
	 */
#if DEVAPC_TURN_ON
	ret = request_irq(devapc_irq, (irq_handler_t) devapc_violation_irq,
			  IRQF_TRIGGER_LOW | IRQF_SHARED, "devapc", &g_devapc_ctrl);
	if (ret) {
		pr_err("[DEVAPC] Failed to request irq! (%d)\n", ret);
		return ret;
	}
#endif

#ifdef CONFIG_MTK_CLKMGR
/* mt_clkmgr */
	enable_clock(MT_CG_INFRA_DEVAPC, "DEVAPC");
#else
/* CCF */
	dapc_clk = devm_clk_get(&dev->dev, "devapc-main");
	if (IS_ERR(dapc_clk)) {
		pr_err("[DEVAPC] cannot get devapc clock from common clock framework.\n");
		return PTR_ERR(dapc_clk);
	}
	clk_prepare_enable(dapc_clk);
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

struct platform_device devapc_device = {
	.name = "devapc",
	.id = -1,
};

static const struct file_operations devapc_dbg_fops = {
	.owner = THIS_MODULE,
	.open  = devapc_dbg_open,
	.write = devapc_dbg_write,
	.read = NULL,
};

static struct platform_driver devapc_driver = {
	.probe = devapc_probe,
	.remove = devapc_remove,
	.suspend = devapc_suspend,
	.resume = devapc_resume,
	.driver = {
		   .name = "devapc",
		   .owner = THIS_MODULE,
		   },
};

/*
 * devapc_init: module init function.
 */
static int __init devapc_init(void)
{
	int ret;

	DEVAPC_MSG("[DEVAPC] module init.\n");

	ret = platform_device_register(&devapc_device);
	if (ret) {
		pr_err("[DEVAPC] Unable to do device register(%d)\n", ret);
		return ret;
	}

	ret = platform_driver_register(&devapc_driver);
	if (ret) {
		pr_err("[DEVAPC] Unable to register driver (%d)\n", ret);
		platform_device_unregister(&devapc_device);
		return ret;
	}

	g_devapc_ctrl = cdev_alloc();
	if (!g_devapc_ctrl) {
		pr_err("[DEVAPC] Failed to add devapc device! (%d)\n", ret);
		platform_driver_unregister(&devapc_driver);
		platform_device_unregister(&devapc_device);
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

late_initcall(devapc_init);
module_exit(devapc_exit);
MODULE_LICENSE("GPL");
