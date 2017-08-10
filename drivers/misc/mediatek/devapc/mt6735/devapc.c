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
#include <mach/irqs.h>
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

#include <mach/mt_secure_api.h>

#include "devapc.h"

/* Debug message event */
#define DEVAPC_LOG_NONE		0x00000000
#define DEVAPC_LOG_ERR		0x00000001
#define DEVAPC_LOG_WARN		0x00000002
#define DEVAPC_LOG_INFO		0x00000004
#define DEVAPC_LOG_DBG		0x00000008

#define DEVAPC_LOG_LEVEL	  (DEVAPC_LOG_ERR)

#define DEVAPC_MSG(fmt, args...) \
	do {	\
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


#ifndef CONFIG_MTK_CLKMGR
/* CCF */
static struct clk *dapc_clk;
#endif

static struct cdev *g_devapc_ctrl;
static unsigned int devapc_irq;
static void __iomem *devapc_ao_base;
static void __iomem *devapc_pd_base;

static unsigned int enable_dynamic_one_core_violation_debug;

#if defined(CONFIG_ARCH_MT6735)

static struct DEVICE_INFO devapc_devices[] = {
	/*0*/
	{"INFRA_AO_INFRASYS_CONFIG_REGS"},
	{"INFRA_AO_PMIC_WRAP_CONTROL_REG"},
	{"INFRA_AO_PERISYS_CONFIG_REGS"},
	{"INFRA_AO_KPAD_CONTROL_REG"},
	{"INFRA_AO_GPT"},
	{"INFRA_AO_APMCU_EINT_CONTROLLER"},
	{"INFRA_AO_TOP_LEVEL_SLP_MANAGER"},
	{"INFRA_AO_DEVICE_APC_AO"},
	{"INFRA_AO_SEJ"},
	{"INFRA_AO_RSVD"},

	 /*10*/
	{"INFRA_AO_CLDMA_AO_TOP_AP"},
	{"INFRA_AO_CLDMA_AO_TOP_MD"},
	{"INFRASYS_MCUSYS_CONFIG_REG"},
	{"INFRASYS_CONTROL_REG"},
	{"INFRASYS_BOOTROM/SRAM"},
	{"INFRASYS_EMI_BUS_INTERFACE"},
	{"INFRASYS_SYSTEM_CIRQ"},
	{"INFRASYS_MM_IOMMU_CONFIGURATION"},
	{"INFRASYS_EFUSEC"},
	{"INFRASYS_DEVICE_APC_MONITOR"},

	 /*20*/
	{"INFRASYS_MCU_BIU_CONFIGURATION"},
	{"INFRASYS_AP_MIXED_CONTROL_REG"},
	{"INFRASYS_CA7_AP_CCIF"},
	{"INFRASYS_CA7_MD_CCIF"},
	{"RSVD"},
	{"INFRASYS_GPIO1_CONTROLLER"},
	{"INFRASYS_MBIST_CONTROL_REG"},
	{"INFRASYS_TRNG"},
	{"INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR"},
	{"INFRASYS_GPIO1_CONTROLLER"},

	 /*30*/
	{"INFRA_AO_TOP_LEVEL_REST_GENERATOR"},
	{"INFRASYS_DDRPHY"},
	{"INFRASYS_DRAM_CONTROLLER"},
	{"INFRASYS_MIPI_RX_ANA"},
	{"INFRASYS_GCPU"},
	{"INFRASYS_GCE"},
	{"INFRASYS_CCIF_AP_1"},
	{"INFRASYS_CCIF_MD_1"},
	{"INFRASYS_CLDMA_PDN_AP"},
	{"INFRASYS_CLDMA_PDN_MD"},

	 /*40*/
	{"INFRASYS_MD2MD_CCIF0"},
	{"INFRASYS_MD2MD_CCIF1"},
	{"INFRASYS_MDSYSINTF"},
	{"DEGBUGSYS"},
	{"DMA"},
	{"AUXADC"},
	{"UART0"},
	{"UART1"},
	{"UART2"},
	{"UART3"},

	 /*50*/
	{"PWM"},
	{"I2C0"},
	{"I2C1"},
	{"I2C2"},
	{"SPI0"},
	{"PTP_THERMAL_CTL"},
	{"BTIF"},
	{"UART4"},
	{"DISP_PWM"},
	{"I2C3"},

	 /*60*/
	{"IRDA"},
	{"IR_TX"},
	{"USB2.0"},
	{"USB2.0 SIF"},
	{"AUDIO"},
	{"MSDC0"},
	{"MSDC1"},
	{"MSDC2"},
	{"USB3.0"},
	{"WCN_AHB_SLAVE"},

	 /*70*/
	{"MD2_PERIPHERALS"},
	{"MD3_PERIPHERALS"},
	{"G3D_CONFIG"},
	{"MALI"},
	{"MMSYS_CONFIG"},
	{"MDP_RDMA"},
	{"MDP_RSZ0"},
	{"MDP_RSZ1"},
	{"MDP_WDMA"},
	{"MDP_WROT"},

	 /*80*/
	{"MDP_TDSHP"},
	{"DISP_OVL"},
	{"DISP_RDMA0"},
	{"DISP_RDMA1"},
	{"DISP_WDMA"},
	{"DISP_COLOR"},
	{"DISP_CCORR"},
	{"DISP_AAL"},
	{"DISP_GAMMA"},
	{"DISP_DITHER"},

	 /*90*/
	{"Reserved"},
	{"DSI"},
	{"DPI"},
	{"Reserved"},
	{"MM_MUTEX"},
	{"SMI_LARB0"},
	{"SMI_COMMON"},
	{"MIPI_TX_CONFIG"},
	{"IMGSYS_CONFIG"},
	{"IMGSYS_SMI_LARB2"},

	 /*100*/
	{"IMGSYS_CAM1"},
	{"IMGSYS_CAM2"},
	{"IMGSYS_CAM3"},
	{"IMGSYS_CAM4"},
	{"IMGSYS_SENINF"},
	{"IMGSYS_CAMSV"},
	{"IMGSYS_FDVT"},
	{"IMGSYS_CAM5"},
	{"IMGSYS_CAM6"},
	{"IMGSYS_CAM7"},

	 /*110*/
	{"VDECSYS_GLOBAL_CONFIGURATION"},
	{"SMI_LARB1"},
	{"VDEC_FULL_TOP"},
	{"VENC_GLOBAL_CON"},
	{"SMI_LARB3"},
	{"VENC"},
	{"JPEG_ENC"},
	{"JPEG_DEC"},

};

#elif defined(CONFIG_ARCH_MT6735M)

static struct DEVICE_INFO devapc_devices[] = {
	/* Slave */
	{"INFRA_AO_INFRASYS_CONFIG_REGS"},
	{"INFRA_AO_PMIC_WRAP_CONTROL_REG"},
	{"INFRA_AO_PERISYS_CONFIG_REGS"},
	{"INFRA_AO_KPAD_CONTROL_REG"},
	{"INFRA_AO_GPT"},
	{"INFRA_AO_APMCU_EINT_CONTROLLER"},
	{"INFRA_AO_TOP_LEVEL_SLP_MANAGER"},
	{"INFRA_AO_DEVICE_APC_AO"},
	{"INFRA_AO_SEJ"},
	{"INFRA_AO_RSVD"},

	 /*10*/
	{"INFRA_AO_CLDMA_AO_TOP_AP"},
	{"INFRA_AO_CLDMA_AO_TOP_MD"},
	{"Reserved"},
	{"INFRASYS_MCUSYS_CONFIG_REG"},
	{"INFRASYS_CONTROL_REG"},
	{"INFRASYS_BOOTROM/SRAM"},
	{"INFRASYS_EMI_BUS_INTERFACE"},
	{"INFRASYS_SYSTEM_CIRQ"},
	{"INFRASYS_MM_IOMMU_CONFIGURATION"},
	{"INFRASYS_EFUSEC"},

	 /*20*/
	{"INFRASYS_DEVICE_APC_MONITOR"},
	{"INFRASYS_MCU_BIU_CONFIGURATION"},
	{"INFRASYS_AP_MIXED_CONTROL_REG"},
	{"INFRASYS_CA7_AP_CCIF"},
	{"INFRASYS_CA7_MD_CCIF"},
	{"RSVD"},
	{"INFRASYS_GPIO1_CONTROLLER"},
	{"INFRASYS_MBIST_CONTROL_REG"},
	{"INFRASYS_TRNG"},
	{"INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR"},

	 /*30*/
	{"INFRASYS_GPIO1_CONTROLLER"},
	{"INFRA_AO_TOP_LEVEL_REST_GENERATOR"},
	{"INFRASYS_DDRPHY"},
	{"INFRASYS_DRAM_CONTROLLER"},
	{"INFRASYS_MIPI_RX_ANA"},
	{"INFRASYS_GCPU"},
	{"INFRASYS_GCE"},
	{"INFRASYS_CCIF_AP_1"},
	{"INFRASYS_CCIF_MD_1"},
	{"INFRASYS_CLDMA_PDN_AP"},

	 /*40*/
	{"INFRASYS_CLDMA_PDN_MD"},
	{"INFRASYS_MDSYSINTF"},
	{"DEGBUGSYS"},
	{"DMA"},
	{"AUXADC"},
	{"UART0"},
	{"UART1"},
	{"UART2"},
	{"UART3"},
	{"PWM"},

	 /*50*/
	{"I2C0"},
	{"I2C1"},
	{"I2C2"},
	{"SPI0"},
	{"PTP_THERMAL_CTL"},
	{"BTIF"},
	{"Reserved"},
	{"DISP_PWM"},
	{"I2C3"},
	{"IRDA"},

	 /*60*/
	{"IR_TX"},
	{"USB2.0"},
	{"USB2.0 SIF"},
	{"AUDIO"},
	{"MSDC0"},
	{"MSDC1"},
	{"RESERVE"},
	{"RESERVE"},
	{"WCN_AHB_SLAVE"},
	{"MD_PERIPHERALS"},

	 /*70*/
	{"RESERVE"},
	{"G3D_CONFIG"},
	{"MALI"},
	{"MMSYS_CONFIG"},
	{"MDP_RDMA"},
	{"MDP_RSZ0"},
	{"MDP_RSZ1"},
	{"MDP_WDMA"},
	{"MDP_WROT"},
	{"MDP_TDSHP"},

	 /*80*/
	{"DISP_OVL0"},
	{"DISP_OVL1"},
	{"DISP_RDMA0"},
	{"DISP_RDMA1"},
	{"DISP_WDMA"},
	{"DISP_COLOR"},
	{"DISP_CCORR"},
	{"DISP_AAL"},
	{"DISP_GAMMA"},
	{"DISP_DITHER"},

	 /*90*/
	{"Reserved"},
	{"DPI"},
	{"DSI"},
	{"Reserved"},
	{"MM_MUTEX"},
	{"SMI_LARB0"},
	{"SMI_COMMON"},
	{"MIPI_TX_CONFIG"},
	{"IMGSYS_CONFIG"},
	{"IMGSYS_SMI_LARB2"},

	 /*100*/
	{"IMGSYS_CAM1"},
	{"IMGSYS_CAM2"},
	{"IMGSYS_SENINF"},
	{"VENC"},
	{"JPGENC"},
	{"VDEC"},
	{"VDEC_GLOBAL_CON"},
	{"SMI_LARB1"},
	{"VDEC_FULL_TOP"},

};

#elif defined(CONFIG_ARCH_MT6753)

static struct DOMAIN_INFO domain_settings[] = {
	{"AP"},
	{"MD1"},
	{"CONN"},
	{"Reserved"},
	{"MM"},
	{"MD3"},
	{"MFG"},
	{"Reserved"},
};

static struct DEVICE_INFO devapc_devices[] = {
	{"INFRA_AO_INFRASYS_CONFIG_REGS"},
	{"INFRA_AO_PMIC_WRAP_CONTROL_REG"},
	{"INFRA_AO_PERISYS_CONFIG_REGS"},
	{"INFRA_AO_KPAD_CONTROL_REG"},
	{"INFRA_AO_GPT"},
	{"INFRA_AO_APMCU_EINT_CONTROLLER"},
	{"INFRA_AO_TOP_LEVEL_SLP_MANAGER"},
	{"INFRA_AO_DEVICE_APC_AO"},
	{"INFRA_AO_SEJ"},
	{"INFRA_AO_RSVD"},

	 /*10*/
	{"INFRA_AO_CLDMA_AO_TOP_AP"},
	{"INFRA_AO_CLDMA_AO_TOP_MD"},
	{"INFRASYS_MCUSYS_CONFIG_REG"},
	{"INFRASYS_CONTROL_REG"},
	{"INFRASYS_BOOTROM/SRAM"},
	{"INFRASYS_EMI_BUS_INTERFACE"},
	{"INFRASYS_SYSTEM_CIRQ"},
	{"INFRASYS_MM_IOMMU_CONFIGURATION"},
	{"INFRASYS_EFUSEC"},
	{"INFRASYS_DEVICE_APC_MONITOR"},

	 /*20*/
	{"INFRASYS_MCU_BIU_CONFIGURATION"},
	{"INFRASYS_AP_MIXED_CONTROL_REG"},
	{"INFRASYS_CA7_AP_CCIF"},
	{"INFRASYS_CA7_MD_CCIF"},
	{"RSVD"},
	{"INFRASYS_MBIST_CONTROL_REG"},
	{"INFRASYS_DRAM_CONTROLLER"},
	{"INFRASYS_TRNG"},
	{"INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR"},
	{"INFRASYS_GPIO1_CONTROLLER"},

	 /*30*/
	{"INFRA_AO_TOP_LEVEL_REST_GENERATOR"},
	{"INFRASYS_DDRPHY"},
	{"INFRASYS_DRAM_CONTROLLER"},
	{"INFRASYS_MIPI_RX_ANA"},
	{"INFRASYS_GCPU"},
	{"INFRASYS_GCE"},
	{"INFRASYS_CCIF_AP_1"},
	{"INFRASYS_CCIF_MD_1"},
	{"INFRASYS_CLDMA_PDN_AP"},
	{"INFRASYS_CLDMA_PDN_MD"},

	 /*40*/
	{"INFRASYS_MD2MD_CCIF0"},
	{"INFRASYS_MD2MD_CCIF1"},
	{"INFRASYS_MDSYSINTF"},
	{"DEGBUGSYS"},
	{"DMA"},
	{"AUXADC"},
	{"UART0"},
	{"UART1"},
	{"UART2"},
	{"UART3"},

	 /*50*/
	{"PWM"},
	{"I2C0"},
	{"I2C1"},
	{"I2C2"},
	{"SPI0"},
	{"PTP_THERMAL_CTL"},
	{"BTIF"},
	{"UART4"},
	{"DISP_PWM"},
	{"I2C3"},

	 /*60*/
	{"IRDA"},
	{"IR_TX"},
	{"I2C4"},
	{"USB2.0"},
	{"USB2.0 SIF"},
	{"AUDIO"},
	{"MSDC0"},
	{"MSDC1"},
	{"MSDC2"},
	{"MSDC3"},

	/*70*/
	{"WCN_AHB_SLAVE"},
	{"MD_PERIPHERALS"},
	{"MD2_PERIPHERALS"},
	{"G3D_CONFIG"},
	{"MALI"},
	{"MMSYS_CONFIG"},
	{"MDP_RDMA"},
	{"MDP_RSZ0"},
	{"MDP_RSZ1"},
	{"MDP_WDMA"},

	/*80*/
	{"MDP_WROT"},
	{"MDP_TDSHP"},
	{"DISP_OVL0"},
	{"DISP_OVL1"},
	{"DISP_RDMA0"},
	{"DISP_RDMA1"},
	{"DISP_WDMA"},
	{"DISP_COLOR"},
	{"DISP_CCORR"},
	{"DISP_AAL"},

	/*90*/
	{"DISP_GAMMA"},
	{"DISP_DITHER"},
	{"Reserved"},
	{"DISP_OD"},
	{"DSI"},
	{"DPI"},
	{"MM_MUTEX"},
	{"SMI_LARB0"},
	{"SMI_COMMON"},
	{"IMGSYS_CONFIG"},

	/*100*/
	{"IMGSYS_SMI_LARB2"},
	{"IMGSYS_CAM1"},
	{"IMGSYS_CAM2"},
	{"IMGSYS_CAM3"},
	{"IMGSYS_CAM4"},
	{"IMGSYS_SENINF"},
	{"IMGSYS_CAMSV"},
	{"IMGSYS_FDVT"},
	{"IMGSYS_CAM5"},
	{"IMGSYS_CAM6"},

	/*110*/
	{"IMGSYS_CAM7"},
	{"VDECSYS_GLOBAL_CONFIGURATION"},
	{"SMI_LARB1"},
	{"VDEC_FULL_TOP"},
	{"VENC_GLOBAL_CON"},
	{"SMI_LARB3"},
	{"VENC"},
	{"JPEG_ENC"},
	{"JPEG_DEC"},

};


#else

#error "Wrong Config type"

#endif

/*****************************************************************************
*FUNCTION DEFINITION
*****************************************************************************/
static int clear_vio_status(unsigned int module);

static int devapc_ioremap(void);
/**************************************************************************
*EXTERN FUNCTION
**************************************************************************/
int mt_devapc_check_emi_violation(void)
{
	if ((readl(IOMEM(DEVAPC0_D0_VIO_STA_3)) & ABORT_EMI) == 0)
		return -1;

	pr_debug("EMI violation! It should be cleared by EMI MPU driver later!\n");
	return 0;
}

int mt_devapc_emi_initial(void)
{
	DEVAPC_MSG("EMI_DAPC Init start\n");

	devapc_ioremap();

	if (NULL != devapc_ao_base) {
		mt_reg_sync_writel(readl(IOMEM(DEVAPC0_APC_CON)) & (0xFFFFFFFF ^ (1 << 2)), DEVAPC0_APC_CON);
		mt_reg_sync_writel(readl(IOMEM(DEVAPC0_PD_APC_CON)) & (0xFFFFFFFF ^ (1 << 2)), DEVAPC0_PD_APC_CON);
		mt_reg_sync_writel(ABORT_EMI, DEVAPC0_D0_VIO_STA_3);
		mt_reg_sync_writel(readl(IOMEM(DEVAPC0_D0_VIO_MASK_3)) & (0xFFFFFFFF ^ (ABORT_EMI)),
								DEVAPC0_D0_VIO_MASK_3);
		DEVAPC_MSG("EMI_DAPC Init done\n");
		return 0;
	}

	return -1;
}

int mt_devapc_clear_emi_violation(void)
{
	if ((readl(IOMEM(DEVAPC0_D0_VIO_STA_3)) & ABORT_EMI) != 0)
		mt_reg_sync_writel(ABORT_EMI, DEVAPC0_D0_VIO_STA_3);

	return 0;
}


 /*
 * mt_devapc_set_permission: set module permission on device apc.
 * @module: the moudle to specify permission
 * @domain_num: domain index number
 * @permission_control: specified permission
 * no return value.
 */
int mt_devapc_set_permission(unsigned int module, E_MASK_DOM domain_num, APC_ATTR permission)
{
	unsigned int *base;
	unsigned int clr_bit = 0x3 << ((module % 16) * 2);
	unsigned int set_bit = permission << ((module % 16) * 2);

	if (module >= DEVAPC_DEVICE_NUMBER) {
		DEVAPC_MSG("[DEVAPC] ERROR, device number %d exceeds the max number!\n", module);
		return -1;
	}

	if (DEVAPC_DOMAIN_AP == domain_num)
		base = DEVAPC0_D0_APC_0 + (module / 16) * 4;
	else if (DEVAPC_DOMAIN_MD1 == domain_num)
		base = DEVAPC0_D1_APC_0 + (module / 16) * 4;
	else if (DEVAPC_DOMAIN_CONN == domain_num)
		base = DEVAPC0_D2_APC_0 + (module / 16) * 4;
	else if (DEVAPC_DOMAIN_MD32 == domain_num)
		base = DEVAPC0_D3_APC_0 + (module / 16) * 4;
#if defined(CONFIG_ARCH_MT6735)
	else if (DEVAPC_DOMAIN_MM == domain_num)
		base = DEVAPC0_D4_APC_0 + (module / 16) * 4;
	else if (DEVAPC_DOMAIN_MD3 == domain_num)
		base = DEVAPC0_D5_APC_0 + (module / 16) * 4;
	else if (DEVAPC_DOMAIN_MFG == domain_num)
		base = DEVAPC0_D6_APC_0 + (module / 16) * 4;
#elif defined(CONFIG_ARCH_MT6735M)
	/* blank intentionally */
#elif defined(CONFIG_ARCH_MT6753)
	else if (DEVAPC_DOMAIN_MM == domain_num)
		base = DEVAPC0_D4_APC_0 + (module / 16) * 4;
	else if (DEVAPC_DOMAIN_MD3 == domain_num)
		base = DEVAPC0_D5_APC_0 + (module / 16) * 4;
	else if (DEVAPC_DOMAIN_MFG == domain_num)
		base = DEVAPC0_D6_APC_0 + (module / 16) * 4;
#else
#error "Wrong Config type"
#endif
	else {
		DEVAPC_MSG("[DEVAPC] ERROR, domain number %d exceeds the max number!\n", domain_num);
		return -2;
	}

	mt_reg_sync_writel(readl(base) & ~clr_bit, base);
	mt_reg_sync_writel(readl(base) | set_bit, base);
	return 0;
}
/**************************************************************************
*STATIC FUNCTION
**************************************************************************/

static int devapc_ioremap(void)
{
	struct device_node *node = NULL;
	/*IO remap*/
	node = of_find_compatible_node(NULL, NULL, "mediatek,DEVAPC_AO");
	if (node) {
		devapc_ao_base = of_iomap(node, 0);
		DEVAPC_MSG("[DEVAPC] AO_ADDRESS %p\n", devapc_ao_base);
	} else {
		pr_err("[DEVAPC] can't find DAPC_AO compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,DEVAPC");
	if (node) {
		devapc_pd_base = of_iomap(node, 0);
		devapc_irq = irq_of_parse_and_map(node, 0);
		DEVAPC_MSG("[DEVAPC] PD_ADDRESS %p, IRD: %d\n", devapc_pd_base, devapc_irq);
	} else {
		pr_err("[DEVAPC] can't find DAPC_PD compatible node\n");
		return -1;
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

#if defined(CONFIG_ARCH_MT6753)
static int check_vio_status(unsigned int module)
{
	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;
	unsigned int vio_status = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC*2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC*2);

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
	default:
		break;
	}

	if (vio_status)
		return 1;

	return 0;
}
#endif

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) || defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
/* ATF or TEE runs this path */

static void start_devapc(void)
{

	mt_reg_sync_writel(readl(DEVAPC0_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC0_PD_APC_CON);

	DEVAPC_MSG("[DEVAPC] Making SMC call to ATF.\n");

	/*Set DAPC in ATF*/
	mt_secure_call(MTK_SIP_KERNEL_DAPC_INIT, 0, 0, 0);

}

#else

#error "Wrong Arch type (Kernel only)"

#endif


/*
 * clear_vio_status: clear violation status for each module.
 * @module: the moudle to clear violation status
 * @devapc_num: device apc index number (device apc 0 or 1)
 * @domain_num: domain index number (AP or MD domain)
 * no return value.
 */
static int clear_vio_status(unsigned int module)
{

	unsigned int apc_index = 0;
	unsigned int apc_bit_index = 0;

	apc_index = module / (MOD_NO_IN_1_DEVAPC*2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC*2);

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
	default:
		break;
	}

	return 0;
}

static irqreturn_t devapc_violation_irq(int irq, void *dev_id)
{
	unsigned int dbg0 = 0, dbg1 = 0;
	unsigned int master_id;
	unsigned int domain_id;
	unsigned int r_w_violation;
	int i;
	struct pt_regs *regs;

	dbg0 = readl(DEVAPC0_VIO_DBG0);
	dbg1 = readl(DEVAPC0_VIO_DBG1);
	master_id = dbg0 & VIO_DBG_MSTID;
	domain_id = dbg0 & VIO_DBG_DMNID;
	r_w_violation = dbg0 & VIO_DBG_RW;

#if defined(CONFIG_ARCH_MT6753)
	/* violation information improvement for Denali-3 */
	if ((domain_id >= 0) && (domain_id < ARRAY_SIZE(domain_settings))) {
		if (1 == r_w_violation) {
			pr_err("[DEVAPC] Device Access Permission Write Violation - Process:%s PID:%i Vio Addr:0x%x , Bus ID:0x%x, Dom ID:0x%x (%s), VIO_DBG0:0x%x\n",
				current->comm, current->pid, dbg1, master_id, domain_id,
				domain_settings[domain_id].name, (*DEVAPC0_VIO_DBG0));
		} else {
			pr_err("[DEVAPC] Device Access Permission Read Violation - Process:%s PID:%i Vio Addr:0x%x , Bus ID:0x%x, Dom ID:0x%x (%s), VIO_DBG0:0x%x\n",
			current->comm, current->pid, dbg1, master_id, domain_id,
			domain_settings[domain_id].name, (*DEVAPC0_VIO_DBG0));
		}
	} else {
		if (1 == r_w_violation) {
			pr_err("[DEVAPC] Device Access Permission Write Violation - Process:%s PID:%i Vio Addr:0x%x , Bus ID:0x%x , Dom ID:0x%x, VIO_DBG0:0x%x\n",
			current->comm, current->pid, dbg1, master_id, domain_id, (*DEVAPC0_VIO_DBG0));
		} else {
			pr_err("[DEVAPC] Device Access Permission Read Violation - Process:%s PID:%i Vio Addr:0x%x , Bus ID:0x%x , Dom ID:0x%x, VIO_DBG0:0x%x\n",
			current->comm, current->pid, dbg1, master_id, domain_id, (*DEVAPC0_VIO_DBG0));
		}
	}
#else
	if (1 == r_w_violation) {
		pr_err("[DEVAPC] Device Access Permission Write Violation - Process:%s PID:%i Vio Addr:0x%x , Bus ID:0x%x , Dom ID:0x%x\n",
			current->comm, current->pid, dbg1, master_id, domain_id);
	} else {
		pr_err("[DEVAPC] Device Access Permission Read Violation - Process:%s PID:%i Vio Addr:0x%x , Bus ID:0x%x , Dom ID:0x%x\n",
			current->comm, current->pid, dbg1, master_id, domain_id);
	}
#endif

	pr_err("[DEVAPC] VIO_STA 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x, 4:0x%x\n",
		readl(DEVAPC0_D0_VIO_STA_0), readl(DEVAPC0_D0_VIO_STA_1), readl(DEVAPC0_D0_VIO_STA_2),
		readl(DEVAPC0_D0_VIO_STA_3), readl(DEVAPC0_D0_VIO_STA_4));

	for (i = 0; i < (ARRAY_SIZE(devapc_devices)); i++) {

#if defined(CONFIG_ARCH_MT6753)
		/* violation information improvement */

		if (check_vio_status(i))
			pr_err("[DEVAPC] Access Violation Slave: %s (index=%d)\n", devapc_devices[i].device, i);
#endif

		clear_vio_status(i);
	}

	if ((DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG) || (enable_dynamic_one_core_violation_debug)) {
		pr_err("[DEVAPC] ====== Start dumping Device APC violation tracing ======\n");

		pr_err("[DEVAPC] **************** [All IRQ Registers] ****************\n");
		regs = get_irq_regs();
		show_regs(regs);

		pr_err("[DEVAPC] **************** [All Current Task Stack] ****************\n");
		show_stack(current, NULL);

		pr_err("[DEVAPC] ====== End of dumping Device APC violation tracing ======\n");
	}

	mt_reg_sync_writel(VIO_DBG_CLR, DEVAPC0_VIO_DBG0);
	dbg0 = readl(DEVAPC0_VIO_DBG0);
	dbg1 = readl(DEVAPC0_VIO_DBG1);

	if ((dbg0 != 0) || (dbg1 != 0)) {
		pr_err("[DEVAPC] Multi-violation!\n");
		pr_err("[DEVAPC] DBG0 = %x, DBG1 = %x\n", dbg0, dbg1);
	}

	return IRQ_HANDLED;
}

static int devapc_probe(struct platform_device *dev)
{
	int ret;

	DEVAPC_MSG("[DEVAPC] module probe.\n");
	/*IO remap*/
	devapc_ioremap();

	/*
	* Interrupts of vilation (including SPC in SMI, or EMI MPU) are triggered by the device APC.
	* need to share the interrupt with the SPC driver.
	*/
	ret = request_irq(devapc_irq, (irq_handler_t)devapc_violation_irq,
	IRQF_TRIGGER_LOW | IRQF_SHARED, "devapc", &g_devapc_ctrl);
	if (ret) {
		pr_err("[DEVAPC] Failed to request irq! (%d)\n", ret);
		return ret;
	}

#ifdef CONFIG_MTK_CLKMGR
/* mt_clkmgr */
	enable_clock(MT_CG_INFRA_DEVAPC, "DEVAPC");
#else
/* CCF */
	dapc_clk = devm_clk_get(&dev->dev, "devapc-main");
	if (IS_ERR(dapc_clk)) {
		pr_err("[DEVAPC] cannot get dapc clock.\n");
		return PTR_ERR(dapc_clk);
	}
	clk_prepare_enable(dapc_clk);
#endif

#ifdef CONFIG_MTK_HIBERNATION
	register_swsusp_restore_noirq_func(ID_M_DEVAPC, devapc_pm_restore_noirq, NULL);
#endif

	start_devapc();

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

struct platform_device devapc_device = {
	.name = "devapc",
	.id = -1,
};

static const struct of_device_id mt_dapc_of_match[] = {
	{ .compatible = "mediatek,DEVAPC", },
	{/* sentinel */},
};

MODULE_DEVICE_TABLE(of, mt_dapc_of_match);

static struct platform_driver devapc_driver = {
	.probe = devapc_probe,
	.remove = devapc_remove,
	.suspend = devapc_suspend,
	.resume = devapc_resume,
	.driver = {
	.name = "dapc",
	.owner = THIS_MODULE,
#ifdef CONFIG_OF
	.of_match_table = mt_dapc_of_match,
#endif
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
