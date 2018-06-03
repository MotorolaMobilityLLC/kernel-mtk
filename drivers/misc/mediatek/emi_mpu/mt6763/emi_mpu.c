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
#include <linux/list.h>
#include <mt-plat/mtk_secure_api.h>
#ifdef CONFIG_MTK_AEE_FEATURE
#include <mt-plat/aee.h>
#endif
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <mt-plat/mtk_device_apc.h>
#include <mt-plat/sync_write.h>
/* #include "mach/irqs.h" */
#include <mt-plat/dma.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <mt-plat/mtk_io.h>
#include "emi_mpu.h"
#include "emi_bwl.h"
#include <mt-plat/mtk_ccci_common.h>
#include <linux/delay.h>

static int Violation_PortID = MASTER_ALL;

#define EMI_MPU_TEST	0
#if EMI_MPU_TEST
char mpu_test_buffer[0x20000] __aligned(PAGE_SIZE);
#endif

#define ENABLE_EMI_CHKER
#define ENABLE_EMI_WATCH_POINT

#define MAX_EMI_MPU_STORE_CMD_LEN 128
#define AXI_VIO_MONITOR_TIME	(1 * HZ)


static void __iomem *CEN_EMI_BASE;
static void __iomem *CHA_EMI_BASE;
static void __iomem *CHB_EMI_BASE;
static void __iomem *EMI_MPU_BASE;
static unsigned long long vio_addr;
static unsigned int emi_physical_offset;

struct mst_tbl_entry {
	u32 master;
	u32 port;
	u32 id_mask;
	u32 id_val;
	char *note;
	char *name;
};

static const struct mst_tbl_entry mst_tbl[] = {
	/* APMCU 1 */
	{.master = M0_AXI_MST_MCUSYS_MP0, .port = 0x0, .id_mask = 0x0001,
		.id_val = 0x0000,
		.note = "MT63_M0_MCUSYS_MP0",
		.name = "MT63_M0_MCUSYS_MP0"},
	{.master = M0_AXI_MST_MCUSYS_MP1, .port = 0x0, .id_mask = 0x0001,
		.id_val = 0x0001,
		.note = "MT63_M0_MCUSYS_MP1",
		.name = "MT63_M0_MCUSYS_MP1"},

	/* APMCU 2 */
	{.master = M1_AXI_MST_MCUSYS_MP0, .port = 0x1, .id_mask = 0x0001,
		.id_val = 0x0000,
		.note = "MT63_M1_MCUSYS_MP0",
		.name = "MT63_M1_MCUSYS_MP0"},
	{.master = M1_AXI_MST_MCUSYS_MP1, .port = 0x1, .id_mask = 0x0001,
		.id_val = 0x0001,
		.note = "MT63_M1_MCUSYS_MP1",
		.name = "MT63_M1_MCUSYS_MP1"},

	/* MM 1 */
	{.master = M2_AXI_MST_MMSYS_SMI_LARB0, .port = 0x2, .id_mask = 0x0380,
		.id_val = 0x0000,
		.note = "MT63_M2_LARB0",
		.name = "MT63_M2_LARB0"},
	{.master = M2_AXI_MST_IMGSYS_SMI_LARB1, .port = 0x2, .id_mask = 0x0380,
		.id_val = 0x0080,
		.note = "MT63_M2_LARB1",
		.name = "MT63_M2_LARB1"},
	{.master = M2_AXI_MST_CAMSYS_SMI_LARB2, .port = 0x2, .id_mask = 0x0380,
		.id_val = 0x0100,
		.note = "MT63_M2_LARB2",
		.name = "MT63_M2_LARB2"},
	{.master = M2_AXI_MST_VDEC_SMI_LARB3, .port = 0x2, .id_mask = 0x0380,
		.id_val = 0x0180,
		.note = "MT63_M2_LARB3",
		.name = "MT63_M2_LARB3"},

	/* MDMCU */
	{.master = M3_MD_MM, .port = 0x3, .id_mask = 0x0003,
		.id_val = 0x0000,
		.note = "MT63_M3_MD_MM",
		.name = "MT63_M3_MD_MM"},
	{.master = M3_MD_MMU, .port = 0x3, .id_mask = 0x0003,
		.id_val = 0x0001,
		.note = "MT63_M3_MD_MMU",
		.name = "Mt63_M3_MD_MMU"},
	{.master = M3_USIP, .port = 0x3, .id_mask = 0x0003,
		.id_val = 0x0002,
		.note = "MT63_M3_USIP",
		.name = "MT63_M3_USIP"},

	/* MDDMA */
	{.master = M4_MCUSYS_DFD, .port = 0x4, .id_mask = 0x0001,
		.id_val = 0x0001,
		.note = "MT63_M4_MCUSYS_DFD",
		.name = "MT63_M4_MCUSUS_DFD"},
	{.master = M4_HRQ_RD, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0000,
		.note = "MT63_M4_HRQ_RD",
		.name = "MT63_M4_HRQ_RD"},
	{.master = M4_HRQ_RD1_WR, .port = 0x4, .id_mask = 0x1FF5,
		.id_val = 0x1000,
		.note = "MT63_M4_HRQ_RD1_WR",
		.name = "MT63_M4_HRQ_RD1_WR"},
	{.master = M4_VTB, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x100C,
		.note = "MT63_M4_VTB",
		.name = "MT63_M4_VTB"},
	{.master = M4_TBO, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x1014,
		.note = "MT63_M4_TBO",
		.name = "MT63_M4_TBO"},
	{.master = M4_DFE_DUMP, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0808,
		.note = "MT63_M4_DFE_DUMP",
		.name = "MT63_M4_DFE_DUMP"},
	{.master = M4_BR_DMA, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x080A,
		.note = "MT63_M4_BR_DMA",
		.name = "MT63_M4_BR_DMA"},
	{.master = M4_MD2G, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0010,
		.note = "MT63_M4_MD2G",
		.name = "MT63_M4_MD2G"},
	{.master = M4_TXBRP0, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0050,
		.note = "MT63_M4_TXBRP0",
		.name = "MT63_M4_TXBRP0"},
	{.master = M4_TXBRP1, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0150,
		.note = "MT63_M4_TXBRP1",
		.name = "MT63_M4_TXBRP1"},
	{.master = M4_TPC, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x01D0,
		.note = "MT63_M4_TPC",
		.name = "MT63_M4_TPC"},
	{.master = M4_RXDFE_DMA, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0070,
		.note = "MT63_M4_RXDFE_DMA",
		.name = "MT63_M4_RXDFE_DMA"},
	{.master = M4_MRSG0, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x00F0,
		.note = "MT63_M4_MRSG0",
		.name = "MT63_M4_MRSG0"},
	{.master = M4_MRSG1, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0170,
		.note = "MT63_M4_MRSG1",
		.name = "MT63_M4_MRSG1"},
	{.master = M4_CNWDMA, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0130,
		.note = "MT63_M4_CNWDMA",
		.name = "MT63_M4_CNWDMA"},
	{.master = M4_CSH_DCXO, .port = 0x4, .id_mask = 0x1F7F,
		.id_val = 0x0030,
		.note = "MT63_M4_CSH_DCXO",
		.name = "MT63_M4_CSH_DCXO"},
	{.master = M4_MML2, .port = 0x4, .id_mask = 0x1800,
		.id_val = 0x1800,
		.note = "MT63_M4_MML2",
		.name = "MT63_M4_MML2"},
	{.master = M4_LOG_TOP, .port = 0x4, .id_mask = 0x1FDD,
		.id_val = 0x0004,
		.note = "MT63_M4_LOG_TOP",
		.name = "MT63_M4_LOG_TOP"},
	{.master = M4_TRACE_TOP, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0008,
		.note = "MT63_M4_TRACE_TOP",
		.name = "MT63_M4_TRACE_TOP"},
	{.master = M4_PPPHA, .port = 0x4, .id_mask = 0x1FBF,
		.id_val = 0x0002,
		.note = "MT63_M4_PPPHA",
		.name = "MT63_M4_PPPHA"},
	{.master = M4_IPSEC, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x000A,
		.note = "MT63_M4_IPSEC",
		.name = "MT63_M4_IPSEC"},
	{.master = M4_DBGSYS, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x000E,
		.note = "MT63_M4_DBGSYS",
		.name = "MT63_M4_DBGSYS"},
	{.master = M4_GDMA, .port = 0x4, .id_mask = 0x1E3F,
		.id_val = 0x000E,
		.note = "MT63_M4_GDMA",
		.name = "MT63_M4_GDMA"},

	/* MM 2 */
	{.master = M5_AXI_MST_MMSYS_SMI_LARB0, .port = 0x5, .id_mask = 0x0380,
		.id_val = 0x0000,
		.note = "MT63_M5_LARB0",
		.name = "MT63_M5_LARB0"},
	{.master = M5_AXI_MST_IMGSYS_SMI_LARB1, .port = 0x5, .id_mask = 0x0380,
		.id_val = 0x0080,
		.note = "MT63_M5_LARB1",
		.name = "MT63_M5_LARB1"},
	{.master = M5_AXI_MST_CAMSYS_SMI_LARB2, .port = 0x5, .id_mask = 0x0380,
		.id_val = 0x0100,
		.note = "MT63_M5_LARB2",
		.name = "MT63_M5_LARB2"},
	{.master = M5_AXI_MST_VDEC_SMI_LARB3, .port = 0x5, .id_mask = 0x0380,
		.id_val = 0x0180,
		.note = "MT63_M5_LARB3",
		.name = "MT63_M5_LARB3"},

	/* MFG */
	{.master = M6_AXI_MST_MFG_MALI, .port = 0x6, .id_mask = 0x0000,
		.id_val = 0x0000,
		.note = "MT63_M6_AXI_MST_MFG_MALI",
		.name = "MT63_M6_AXI_MST_MFG_MALI"},

	/* PERI */
	{.master = M7_AHB_MST_AUDIO, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x0106,
		.note = "MT63_M7_AHB_MST_AUDIO",
		.name = "MT63_M7_AHB_MST_AUDIO"},
	{.master = M7_AHB_MST_CONN_N9, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x000a,
		.note = "MT63_M7_AHB_MST_CONN_N9",
		.name = "MT63_M7_AHB_MST_CONN_N9"},
	{.master = M7_AHB_MST_MSDC1, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x0070,
		.note = "MT63_M7_AHB_MST_MSDC1",
		.name = "MT63_M7_AHB_MST_MSDC1"},
	{.master = M7_AHB_MST_MSDC2, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x00B0,
		.note = "MT63_M7_AHB_MST_MSDC2",
		.name = "MT63_M7_AHB_MST_MSDC2"},
	{.master = M7_AHB_MST_PWM, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x0030,
		.note = "MT63_M7_AHB_MST_PWM",
		.name = "MT63_M7_AHB_MST_PWM"},
	{.master = M7_AHB_MST_SSPM, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x0126,
		.note = "MT63_M7_AHB_MST_SSPM",
		.name = "MT63_M7_AHB_MST_SSPM"},
	{.master = M7_AHB_MST_SPI0, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x00F0,
		.note = "MT63_M7_AHB_MST_SPI0",
		.name = "MT63_M7_AHB_MST_SPI0"},
	{.master = M7_AHB_MST_SPI1, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x0306,
		.note = "MT63_M7_AHB_MST_SPI1",
		.name = "MT63_M7_AHB_MST_SPI1"},
	{.master = M7_AHB_MST_SPI2, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x00A6,
		.note = "MT63_M7_AHB_MST_SPI2",
		.name = "MT63_M7_AHB_MST_SPI2"},
	{.master = M7_AHB_MST_SPI3, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x01A6,
		.note = "MT63_M7_AHB_MST_SPI3",
		.name = "MT63_M7_AHB_MST_SPI3"},
	{.master = M7_AHB_MST_SPI4, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x02A6,
		.note = "MT63_M7_AHB_MST_SPI4",
		.name = "MT63_M7_AHB_MST_SPI4"},
	{.master = M7_AHB_MST_SPI5, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x03A6,
		.note = "MT63_M7_AHB_MST_SPI5",
		.name = "MT63_M7_AHB_MST_SPI5"},
	{.master = M7_AHB_MST_SPM, .port = 0x7, .id_mask = 0x1EFF,
		.id_val = 0x0126,
		.note = "MT63_M7_AHB_MST_SPM",
		.name = "MT63_M7_AHB_MST_SPM"},
	{.master = M7_AHB_MST_THERM, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x0326,
		.note = "MT63_M7_AHB_MST_THERM",
		.name = "MT63_M7_AHB_MST_THERM"},
	{.master = M7_AHB_MST_USB20_TOP_DMA, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x0006,
		.note = "MT63_M7_AHB_MST_USB20_TOP_DMA",
		.name = "MT63_M7_AHB_MST_USB20_TOP_DMA"},
	{.master = M7_AXI_MST_APDMA_E, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x0066,
		.note = "MT63_M7_AXI_MST_APDMA_E",
		.name = "MT63_M7_AXI_MST_APDMA_E"},
	{.master = M7_AXI_MST_CLDMA_CABGEN, .port = 0x7, .id_mask = 0x1F1F,
		.id_val = 0x0016,
		.note = "MT63_M7_AXI_MST_CLDMA_CABGEN",
		.name = "MT63_M7_AXI_MST_CLDMA_CABGEN"},
	{.master = M7_AXI_MST_CQ_DMA, .port = 0x7, .id_mask = 0x1F1F,
		.id_val = 0x0012,
		.note = "MT63_M7_AXI_MST_CQ_DMA",
		.name = "MT63_M7_AXI_MST_CQ_DMA"},
	{.master = M7_AXI_MST_DBG, .port = 0x7, .id_mask = 0x1FDF,
		.id_val = 0x0002,
		.note = "MT63_M7_AXI_MST_DBG",
		.name = "MT63_M7_AXI_MST_DBG"},
	{.master = M7_AXI_MST_DXCC, .port = 0x7, .id_mask = 0x1E0F,
		.id_val = 0x000E,
		.note = "MT63_M7_AXI_MST_DXCC",
		.name = "MT63_M7_AXI_MST_DXCC"},
	{.master = M7_AXI_MST_GCE, .port = 0x7, .id_mask = 0x1E1F,
		.id_val = 0x001A,
		.note = "MT63_M7_AXI_MST_GCE",
		.name = "MT63_M7_AXI_MST_GCE"},
	{.master = M7_AXI_MST_MFG_MALI, .port = 0x7, .id_mask = 0x1F81,
		.id_val = 0x0001,
		.note = "MT63_M7_AXI_MST_MFG_MALI",
		.name = "MT63_M7_AXI_MST_MFG_MALI"},
	{.master = M7_AXI_MST_MSDC0, .port = 0x7, .id_mask = 0x1FFF,
		.id_val = 0x0010,
		.note = "MT63_M7_AXI_MST_MSDC0",
		.name = "MT63_M7_AXI_MST_MSDC0"},
	{.master = M7_AXI_MST_UFSHCI_TOP_UFS, .port = 0x7, .id_mask = 0x1CFF,
		.id_val = 0x0046,
		.note = "MT63_M7_AXI_MST_UFSHCI_TOP_UFS",
		.name = "MT63_M7_AXI_MST_UFSHCI_TOP_UFS" },
};

static const char *UNKNOWN_MASTER = "unknown";
static DEFINE_SPINLOCK(emi_mpu_lock);

#ifdef ENABLE_EMI_CHKER
struct timer_list emi_axi_vio_timer;
#endif

char *smi_larb0_port[10] = {
	"MT63_LARB0_DISP_OVL0", "MT63_LARB0_DISP_OVL0_2L", "MT63_LARB0_DISP_OVL1_2L",
	"MT63_LARB0_DISP_RDMA0", "MT63_LARB0_DISP_RDMA1", "MT63_LARB0_DISP_WDMA0",
	"MT63_LARB0_MDP_RDMA0", "MT63_LARB0_MDP_WROT0", "MT63_LARB0_MDP_WDMA0",
	"MT63_LARB0_DISP_FAKE"};
char *smi_larb1_port[10] =  {
	"MT63_LARB1_IMGI", "MT63_LARB1_IMG2O", "MT63_LARB1_IMG3O",
	"MT63_LARB1_VIPI", "MT63_LARB1_ICEI",
	"MT63_LARB1_RP", "MT63_LARB1_WR", "MT63_LARB1_RB",
	"MT63_LARB1_DPE_RDMA", "MT63_LARB1_DPE_WDMA"};
char *smi_larb2_port[18] = {
	"MT63_LARB2_IMGO", "MT63_LARB2_RRZO", "MT63_LARB2_AAO", "MT63_LARB2_AFO",
	"MT63_LARB2_LSCI_0", "MT63_LARB2_LSC3I", "MT63_LARB2_RSSO",
	"MT63_LARB2_CAM_SV0", "MT63_LARB2_CAM_SV1", "MT63_LARB2_CAM_SV2",
	"MT63_LARB2_LCSO", "MT63_LARB2_UFEO", "MT63_LARB2_BPCI",
	"MT63_LARB2_PDO", "MT63_LARB2_RAWI", "MT63_LARB2_CCUI",
	"MT63_LARB2_CCUO", "MT63_LARB2_CCUG"};
char *smi_larb3_port[11] = {
	"MT63_LARB3_VENC_RCPU", "MT63_LARB3_VENC_REC", "MT63_LARB3_VENC_BSDMA",
	"MT63_LARB3_VENC_SV_COMV", "MT63_LARB3_VENC_RD_COMV",
	"MT63_LARB3_JPGENC_RDMA", "MT63_LARB3_JPGENC_BSDMA",
	"MT63_LARB3_VENC_CUR_LUMA", "MT63_LARB3_VENC_CUR_CHROMA",
	"MT63_LARB3_VENC_REF_LUMA", "MT63_LARB3_VENC_REF_CHROMA"};

static int __match_id(u32 axi_id, int tbl_idx, u32 port_ID)
{
	u32 mm_larb;
	u32 smi_port;

	if (((axi_id & mst_tbl[tbl_idx].id_mask) == mst_tbl[tbl_idx].id_val)
		&& (port_ID == mst_tbl[tbl_idx].port)) {
		switch (port_ID) {
		case MASTER_APMCU_CH1:
		case MASTER_APMCU_CH2:
		case MASTER_MDMCU:
		case MASTER_MDDMA:
		case MASTER_MFG:
		case MASTER_PERI:
			pr_err("Violation master name is %s.\n",
			mst_tbl[tbl_idx].name);
			break;
		case MASTER_MM_CH1:
		case MASTER_MM_CH2:
			mm_larb = axi_id >> 7;
			smi_port = (axi_id & 0x7F) >> 2;
			/* smi_larb0 */
			if (mm_larb == 0x0) {
				if (smi_port >= ARRAY_SIZE(smi_larb0_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb0_port[smi_port]);
			/* smi_larb1 */
			} else if (mm_larb == 0x1) {
				if (smi_port >= ARRAY_SIZE(smi_larb1_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb1_port[smi_port]);
			/* smi_larb2 */
			} else if (mm_larb == 0x2) {
				if (smi_port >= ARRAY_SIZE(smi_larb2_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb2_port[smi_port]);
			/* smi_larb3 */
			} else if (mm_larb == 0x3) {
				if (smi_port >= ARRAY_SIZE(smi_larb3_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb3_port[smi_port]);
			} else {	/*MM IOMMU Internal Used */
				pr_err("Violation master name is %s.\n",
				mst_tbl[tbl_idx].name);
			}
			break;
		default:
			pr_err("[EMI MPU ERROR] Invalidate port ID! lookup bus ID table failed!\n");
			break;
		}
		return 1;
	} else {
		return 0;
	}
}

static int isetocunt;
static void emi_mpu_set_violation_port(int portID)
{
	if (isetocunt == 0) {
		Violation_PortID = portID;
		isetocunt = 1;
	} else if (isetocunt == 2) {
		isetocunt = 0;
	}
}

int emi_mpu_get_violation_port(void)
{
	int ret;

	pr_err("[EMI MPU] emi_mpu_get_violation_port\n");

	if (isetocunt == 0) {
		pr_err("[EMI MPU] EMI did not set Port ID\n");
		isetocunt = 2;
		return MASTER_ALL;
	}

	ret = Violation_PortID;
	isetocunt = 0;

	if (ret >= MASTER_ALL)
		pr_err("[EMI MPU] Port ID: %d is an invalid Port ID\n", ret);
	else
		Violation_PortID = MASTER_ALL;

	return ret;
}

static char *__id2name(u32 id)
{
	int i;
	u32 axi_ID;
	u32 port_ID;
	u32 mm_larbID;
	u32 smi_portID;

	axi_ID = (id >> 3) & 0x00001FFF;
	port_ID = id & 0x00000007;
	emi_mpu_set_violation_port(port_ID);
	pr_err("[EMI MPU] axi_id = %x, port_id = %x\n", axi_ID, port_ID);

	for (i = 0; i < ARRAY_SIZE(mst_tbl); i++) {
		if (__match_id(axi_ID, i, port_ID))	{
			if ((port_ID == MASTER_MM_CH1) || (port_ID == MASTER_MM_CH2)) {
				mm_larbID = axi_ID >> 7;
				smi_portID = (axi_ID & 0x7F) >> 2;
				if (mm_larbID == 0x0)
					return smi_larb0_port[smi_portID];
				else if (mm_larbID == 0x1)
					return smi_larb1_port[smi_portID];
				else if (mm_larbID == 0x2)
					return smi_larb2_port[smi_portID];
				else if (mm_larbID == 0x3)
					return smi_larb3_port[smi_portID];
				else
					return mst_tbl[i].name;
			} else
				return mst_tbl[i].name;
		}
	}

	return (char *)UNKNOWN_MASTER;
}

static void __clear_emi_mpu_vio(unsigned int first)
{
	u32 dbg_s, dbg_t, i;

	/* clear violation status */
	for (i = 0; i < EMI_MPU_DOMAIN_NUMBER; i++) {
		mt_reg_sync_writel(0xFFFFFFFF, EMI_MPUD_ST(i));  /* Region abort violation */
		mt_reg_sync_writel(0x3, EMI_MPUD_ST2(i));  /* Out of region abort violation */
	}

	/* clear debug info */
	mt_reg_sync_writel(0x80000000, EMI_MPUS);
	dbg_s = readl(IOMEM(EMI_MPUS));
	dbg_t = readl(IOMEM(EMI_MPUT));

	if (first) {
		/* clear the permission setting of MPU*/
		/* do not need to clear here, we already do it at preloader */
	}

	if (dbg_s) {
		pr_err("Fail to clear EMI MPU violation\n");
		pr_err("EMI_MPUS = %x, EMI_MPUT = %x", dbg_s, dbg_t);
	}
}

static int mpu_check_violation(void)
{
	u32 dbg_s, dbg_t, dbg_t_2nd;
	u32 master_ID, domain_ID, wr_vio, wr_oo_vio;
	s32 region;
	char *master_name;

	dbg_s = readl(IOMEM(EMI_MPUS));
	dbg_t = readl(IOMEM(EMI_MPUT));
	dbg_t_2nd = readl(IOMEM(EMI_MPUT_2ND));
	vio_addr = (dbg_t + (((unsigned long long)(dbg_t_2nd & 0xF)) << 32) + emi_physical_offset);

	pr_alert("Clear status.\n");

	master_ID = (dbg_s & 0x0000FFFF);
	domain_ID = (dbg_s >> 21) & 0x0000000F;
	wr_vio = (dbg_s >> 29) & 0x00000003;
	wr_oo_vio = (dbg_s >> 27) & 0x00000003;
	region = (dbg_s >> 16) & 0x1F;

	/*TBD: print the abort region*/

	pr_err("EMI MPU violation.\n");
	pr_err("[EMI MPU] Debug info start ----------------------------------------\n");

	pr_err("EMI_MPUS = %x, EMI_MPUT = %x, EMI_MPUT_2ND = %x.\n", dbg_s, dbg_t, dbg_t_2nd);
	pr_err("Current process is \"%s \" (pid: %i).\n", current->comm, current->pid);
	pr_err("Violation address is 0x%llx.\n", vio_addr);
	pr_err("Violation master ID is 0x%x.\n", master_ID);
	/*print out the murderer name*/
	master_name = __id2name(master_ID);
	pr_err("Violation domain ID is 0x%x.\n", domain_ID);
	if (wr_vio == 1)
		pr_err("Write violation.\n");
	else if (wr_vio == 2)
		pr_err("Read violation.\n");
	else
		pr_err("Strange write / read violation value = %d.\n", wr_vio);
	pr_err("Corrupted region is %d\n\r", region);
	if (wr_oo_vio == 1)
		pr_err("Write out of range violation.\n");
	else if (wr_oo_vio == 2)
		pr_err("Read out of range violation.\n");

	pr_err("[EMI MPU] Debug info end------------------------------------------\n");

#ifdef CONFIG_MTK_AEE_FEATURE
	if (wr_vio != 0) {
		if (((master_ID & 0x7) == MASTER_MDMCU) || ((master_ID & 0x7) == MASTER_MDDMA)) {
			char str[60] = "0";
			char *pstr = str;

			sprintf(pstr, "EMI_MPUS = 0x%x, ADDR = 0x%llx", dbg_s, vio_addr);

			exec_ccci_kern_func_by_md_id(0, ID_MD_MPU_ASSERT, str, strlen(str));
			pr_err("[EMI MPU] MPU violation trigger MD str=%s strlen(str)=%d\n",
				str, (int)strlen(str));
		} else {
			exec_ccci_kern_func_by_md_id(0, ID_MD_MPU_ASSERT, NULL, 0);
			pr_err("[EMI MPU] MPU violation ack to MD\n");
		}

		aee_kernel_exception("EMI MPU",
"%sEMI_MPUS = 0x%x,EMI_MPUT = 0x%x,EMI_MPUT_2ND = 0x%x,vio_addr = 0x%llx\n CHKER = 0x%x,CHKER_TYPE = 0x%x,CHKER_ADR = 0x%x\n%s%s\n",
		     "EMI MPU violation.\n",
		     dbg_s, dbg_t, dbg_t_2nd, vio_addr,
		     readl(IOMEM(EMI_CHKER)),
		     readl(IOMEM(EMI_CHKER_TYPE)),
		     readl(IOMEM(EMI_CHKER_ADR)),
		     "CRDISPATCH_KEY:EMI MPU Violation Issue/",
		     master_name);
	}
#endif

	__clear_emi_mpu_vio(0);
	/* mt_devapc_clear_emi_violation(); */
	return 0;

}

/*EMI MPU violation handler*/
static irqreturn_t mpu_violation_irq(int irq, void *dev_id)
{
	pr_info("It's a MPU violation.\n");
	mpu_check_violation();
	return IRQ_HANDLED;
}

#if defined(CONFIG_MTK_PASR)
/* Acquire DRAM Setting for PASR/DPD */
static unsigned int get_segment_nr(unsigned int rk_index)
{
	unsigned int rank_size;
	unsigned int num_of_1;
	unsigned int i;

	rank_size = get_rank_size(rk_index) / get_ch_num();
	num_of_1 = 0;

	for (i = 0; i < 32; i++) {
		if (rank_size & 0x1)
			num_of_1++;
		rank_size = rank_size >> 1;
	}

	if (num_of_1 > 1)
		return 6;
	else
		return 8;
}

void acquire_dram_setting(struct basic_dram_setting *pasrdpd)
{
	unsigned int ch, rk;

	pasrdpd->channel_nr = get_ch_num();

	for (ch = 0; ch < MAX_CHANNELS; ch++) {
		for (rk = 0; rk < MAX_RANKS; rk++) {
			if ((ch >= pasrdpd->channel_nr) || (rk >= get_rk_num())) {
				pasrdpd->channel[ch].rank[rk].valid_rank = false;
				pasrdpd->channel[ch].rank[rk].rank_size = 0;
				pasrdpd->channel[0].rank[0].segment_nr = 0;
				continue;
			}
			pasrdpd->channel[ch].rank[rk].valid_rank = true;
			pasrdpd->channel[ch].rank[rk].rank_size = get_rank_size(rk) / (pasrdpd->channel_nr);
			pasrdpd->channel[ch].rank[rk].segment_nr = get_segment_nr(rk);

		}
	}
}
#endif

#ifndef CONFIG_ARM64
static noinline int mt_mpu_secure_call(u32 function_id,
	u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
	register u32 reg0 __asm__("r0") = function_id;
	register u32 reg1 __asm__("r1") = arg0;
	register u32 reg2 __asm__("r2") = arg1;
	register u32 reg3 __asm__("r3") = arg2;
	register u32 reg4 __asm__("r4") = arg3;
	int ret = 0;

	asm volatile (__SMC(0) : "+r"(reg0),
		"+r"(reg1), "+r"(reg2), "+r"(reg3), "+r"(reg4));

	ret = reg0;
	return ret;
}
#endif

/*
 * emi_mpu_set_region_protection: protect a region.
 * @start: start address of the region
 * @end: end address of the region
 * @region: EMI MPU region id
 * @access_permission: EMI MPU access permission
 * Return 0 for success, otherwise negative status code.
 */
int emi_mpu_set_region_protection(unsigned long long start,
unsigned long long end, int region, unsigned long long access_permission)
{
	int ret = 0;
	unsigned long flags;
#ifndef CONFIG_ARM64
	unsigned int start_a32;
	unsigned int end_a32;
	unsigned int apc8, apc0;

	start_a32 = start & 0xFFFFFFFF;
	end_a32 = end & 0xFFFFFFFF;
	apc8 = ((start >> 4) & 0xF0000000) |
		((end >> 8) & 0x0F000000) |
		((access_permission >> 32) & 0x00FFFFFF);
	apc0 = (access_permission & 0x04FFFFFF) |
		(((unsigned int) region & 0x1F) << 27);
	emi_mpu_smc_read(EMI_MPU_DBG); /* dummy read for SMC call workaround */
	spin_lock_irqsave(&emi_mpu_lock, flags);
	emi_mpu_smc_set(start_a32, end_a32, apc8, apc0);
	spin_unlock_irqrestore(&emi_mpu_lock, flags);
#else
	access_permission = access_permission & 0x00FFFFFF04FFFFFF;
	access_permission = access_permission | (((unsigned long long)region & 0x1F) << 27);
	spin_lock_irqsave(&emi_mpu_lock, flags);
	emi_mpu_smc_set(start, end, access_permission);
	spin_unlock_irqrestore(&emi_mpu_lock, flags);
#endif

	return ret;
}
EXPORT_SYMBOL(emi_mpu_set_region_protection);

static ssize_t emi_mpu_show(struct device_driver *driver, char *buf)
{
	char *ptr = buf;
	unsigned long long start, end;
	unsigned int reg_value;
	int i, j, region;
	static const char *permission[7] = {
		"No",
		"S_RW",
		"S_RW_NS_R",
		"S_RW_NS_W",
		"S_R_NS_R",
		"FORBIDDEN",
		"S_R_NS_RW"
	};

	for (region = 0; region < EMI_MPU_REGION_NUMBER; region++) {
		start = ((unsigned long long)mt_emi_reg_read(EMI_MPU_SA(region)) << 16) + emi_physical_offset;
		end = ((unsigned long long)mt_emi_reg_read(EMI_MPU_EA(region)) << 16) + emi_physical_offset;
		ptr += sprintf(ptr, "R%d-> 0x%llx to 0x%llx\n", region, start, end+0xFFFF);

		/* for(i=0; i<(EMI_MPU_DOMAIN_NUMBER/8); i++) { */
		/* only print first 8 domain */
		for (i = 0; i < 2; i++) {
			reg_value = mt_emi_reg_read(EMI_MPU_APC(region, i*8));
			for (j = 0; j < 8; j++) {
				ptr += snprintf(ptr, 16, "%s, ", permission[((reg_value >> (j*3)) & 0x7)]);
				if ((j == 3) || (j == 7))
					ptr += sprintf(ptr, "\n");
			}
		}
		ptr += sprintf(ptr, "\n");
	}


#if EMI_MPU_TEST
	{
		int temp;

		temp = (*((volatile unsigned int *)(mpu_test_buffer+0x10000)));
		pr_err("mpu_test_buffer+10000 = 0x%x\n", temp);
	}
#endif

	return strlen(buf);
}

static ssize_t emi_mpu_store(struct device_driver *driver,
const char *buf, size_t count)
{
	int i;
	unsigned long long start_addr;
	unsigned long long end_addr;
	unsigned long region;
	unsigned long long access_permission;
	char *command;
	char *ptr;
	char *token[6];
	ssize_t ret;

	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("emi_mpu_store command overflow.");
		return count;
	}
	pr_err("emi_mpu_store: %s\n", buf);

	command = kmalloc((size_t) MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strncpy(command, buf, (size_t) MAX_EMI_MPU_STORE_CMD_LEN);
	ptr = (char *)buf;

	if (!strncmp(buf, EN_MPU_STR, strlen(EN_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_devel("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 5; i++)
			pr_devel("token[%d] = %s\n", i, token[i]);

		/* kernel standardization
		 * start_addr = simple_strtoul(token[1], &token[1], 16);
		 * end_addr = simple_strtoul(token[2], &token[2], 16);
		 * region = simple_strtoul(token[3], &token[3], 16);
		 * access_permission = simple_strtoul(token[4], &token[4], 16);
		 */
		ret = kstrtoull(token[1], 16, &start_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse start_addr");
		ret = kstrtoull(token[2], 16, &end_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse end_addr");
		ret = kstrtoul(token[3], 10, (unsigned long *)&region);
		if (ret != 0)
			pr_err("kstrtoul fails to parse region");
		ret = kstrtoull(token[4], 16, &access_permission);
		if (ret != 0)
			pr_err("kstrtoull fails to parse access_permission");
		emi_mpu_set_region_protection(start_addr, end_addr, region, access_permission);
		pr_err("Set EMI_MPU: start: 0x%llx, end: 0x%llx, region: %lx, permission: 0x%llx.\n",
		       start_addr, end_addr, region, access_permission);
	} else if (!strncmp(buf, DIS_MPU_STR, strlen(DIS_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_devel("token[%d] = %s\n", i, token[i]);
			i++;
		}
		/* kernel standardization
		 * start_addr = simple_strtoul(token[1], &token[1], 16);
		 * end_addr = simple_strtoul(token[2], &token[2], 16);
		 * region = simple_strtoul(token[3], &token[3], 16);
		 */
		ret = kstrtoull(token[1], 16, &start_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse start_addr");
		ret = kstrtoull(token[2], 16, &end_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse end_addr");
		ret = kstrtoul(token[3], 10, (unsigned long *)&region);
		if (ret != 0)
			pr_err("kstrtoul fails to parse region");

		emi_mpu_set_region_protection(0x0, 0x0, region,
		SET_ACCESS_PERMISSON(UNLOCK,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION));

		pr_err("set EMI MPU: start: 0x%x, end: 0x%x, region: %lx, permission: 0x%llx\n",
		0, 0, region,
		SET_ACCESS_PERMISSON(UNLOCK,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION));
	} else {
		pr_err("Unknown emi_mpu command.\n");
	}

	kfree(command);

	return count;
}

DRIVER_ATTR(mpu_config, 0644, emi_mpu_show, emi_mpu_store);

#ifdef ENABLE_EMI_CHKER
static void emi_axi_set_chker(const unsigned int setting)
{
	int value;

	value = readl(IOMEM(EMI_CHKER));
	value &= ~(0x7 << 16);
	value |= (setting);

	mt_reg_sync_writel(value, EMI_CHKER);
}

static void emi_axi_set_master(const unsigned int setting)
{
	int value;

	value = readl(IOMEM(EMI_CHKER));
	value &= ~(0x0F << AXI_NON_ALIGN_CHK_MST);
	value |= (setting & 0xF) << AXI_NON_ALIGN_CHK_MST;

	mt_reg_sync_writel(value, EMI_CHKER);
}

static void emi_axi_dump_info(int aee_ke_en)
{
	int value, master_ID;
	char *master_name;

	value = readl(IOMEM(EMI_CHKER));
	master_ID = (value & 0x0000FFFF);

	if (value & 0x0000FFFF) {
		pr_err("AXI violation.\n");
		pr_err("[EMI MPU AXI] Debug info start ----------------------------------------\n");

		pr_err("EMI_CHKER = %x.\n", value);
		pr_err("Violation address is 0x%x.\n",
		readl(IOMEM(EMI_CHKER_ADR)));
		pr_err("Violation master ID is 0x%x.\n", master_ID);
		pr_err("Violation type is: AXI_ADR_CHK_EN(%d), AXI_LOCK_CHK_EN(%d), AXI_NON_ALIGN_CHK_EN(%d).\n",
			   (value & (1 << AXI_ADR_VIO)) ? 1 : 0,
			   (value & (1 << AXI_LOCK_ISSUE)) ? 1 : 0,
			   (value & (1 << AXI_NON_ALIGN_ISSUE)) ? 1 : 0);
		pr_err("%s violation.\n",
		(value & (1 << AXI_VIO_WR)) ? "Write" : "Read");
		master_name = __id2name(master_ID);
		pr_err("violation master_name is %s.\n", master_name);

		pr_err("[EMI MPU AXI] Debug info end ----------------------------------------\n");

#ifdef CONFIG_MTK_AEE_FEATURE
		if (aee_ke_en)
			aee_kernel_exception("EMI MPU AXI",
			"AXI violation.\nEMI_CHKER = 0x%x\nCRDISPATCH_KEY:EMI MPU Violation Issue/%s\n",
			value, master_name);
#endif
		/* clear AXI checker status */
		mt_reg_sync_writel((1 << AXI_VIO_CLR) | readl(IOMEM(EMI_CHKER)),
		EMI_CHKER);
	}
}

static void emi_axi_vio_timer_func(unsigned long a)
{
	emi_axi_dump_info(1);

	mod_timer(&emi_axi_vio_timer, jiffies + AXI_VIO_MONITOR_TIME);
}

static ssize_t emi_axi_vio_show(struct device_driver *driver, char *buf)
{
	int value;

	value = readl(IOMEM(EMI_CHKER));

	emi_axi_dump_info(0);

	return snprintf(buf,
			PAGE_SIZE,
			"AXI vio setting is: ADR_CHK_EN %s, LOCK_CHK_EN %s, NON_ALIGN_CHK_EN %s\n",
			(value & (1 << AXI_ADR_CHK_EN)) ? "ON" : "OFF",
			(value & (1 << AXI_LOCK_CHK_EN)) ? "ON" : "OFF",
			(value & (1 << AXI_NON_ALIGN_CHK_EN)) ? "ON" : "OFF");

}

ssize_t emi_axi_vio_store(struct device_driver *driver,
const char *buf, size_t count)
{
	int value;
	int cpu = 0;
	/* assign timer to CPU0 to avoid CPU plug-out
	 * and timer will be unavailable
	 */

	value = readl(IOMEM(EMI_CHKER));

	if (!strncmp(buf, "ADR_CHK_ON", strlen("ADR_CHK_ON"))) {
		emi_axi_set_chker(1 << AXI_ADR_CHK_EN);
		add_timer_on(&emi_axi_vio_timer, cpu);
	} else if (!strncmp(buf, "LOCK_CHK_ON", strlen("LOCK_CHK_ON"))) {
		emi_axi_set_chker(1 << AXI_LOCK_CHK_EN);
		add_timer_on(&emi_axi_vio_timer, cpu);
	} else if (!strncmp(buf, "NON_ALIGN_CHK_ON",
		strlen("NON_ALIGN_CHK_ON"))) {
		emi_axi_set_chker(1 << AXI_NON_ALIGN_CHK_EN);
		add_timer_on(&emi_axi_vio_timer, cpu);
	} else if (!strncmp(buf, "OFF", strlen("OFF"))) {
		emi_axi_set_chker(0);
		del_timer(&emi_axi_vio_timer);
	} else {
		pr_err("invalid setting\n");
	}

	return count;
}

DRIVER_ATTR(emi_axi_vio, 0644, emi_axi_vio_show, emi_axi_vio_store);

#endif /* #ifdef ENABLE_EMI_CHKER */

#ifdef ENABLE_EMI_WATCH_POINT
static void emi_wp_set_address(unsigned int address)
{
	mt_reg_sync_writel(address - emi_physical_offset, EMI_WP_ADR);
}

static void emi_wp_set_range(unsigned int range)
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value = (value & (~EMI_WP_RANGE)) | range;
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}

static void emi_wp_set_monitor_type(unsigned int type)
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value =
	(value & (~EMI_WP_RW_MONITOR)) | (type << EMI_WP_RW_MONITOR_SHIFT);
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}

#if 0
static void emi_wp_set_rw_disable(unsigned int type)
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value =
	(value & (~EMI_WP_RW_DISABLE)) | (type << EMI_WP_RW_DISABLE_SHIFT);
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}
#endif

static void emi_wp_enable(int enable)
{
	unsigned int value;

	/* Enable WP */
	value = readl(IOMEM(EMI_CHKER));
	value =
	(value & ~(1 << EMI_WP_ENABLE_SHIFT)) | (enable << EMI_WP_ENABLE_SHIFT);
	mt_reg_sync_writel(value, EMI_CHKER);
}

static void emi_wp_slave_error_enable(unsigned int enable)
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value =
	(value & ~(1 << EMI_WP_SLVERR_SHIFT)) | (enable << EMI_WP_SLVERR_SHIFT);
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}

static void emi_wp_int_enable(unsigned int enable)
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value =
	(value & ~(1 << EMI_WP_INT_SHIFT)) | (enable << EMI_WP_INT_SHIFT);
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}

static void emi_wp_clr_status(void)
{
	unsigned int value;
	int result;

	value = readl(IOMEM(EMI_CHKER));
	value |= 1 << EMI_WP_VIO_CLR_SHIFT;
	mt_reg_sync_writel(value, EMI_CHKER);

	result = readl(IOMEM(EMI_CHKER)) & EMI_WP_AXI_ID;
	result |= readl(IOMEM(EMI_CHKER_TYPE));
	result |= readl(IOMEM(EMI_CHKER_ADR));
	result |= readl(IOMEM(EMI_CHKER_ADR_2ND));

	if (result)
		pr_err("[EMI_WP] Clear WP status fail!!!!!!!!!!!!!!\n");
}

void emi_wp_get_status(void)
{
	unsigned int value, master_ID;
	char *master_name;

	value = readl(IOMEM(EMI_CHKER));

	if ((value & 0x80000000) == 0) {
		pr_err("[EMI_WP] No watch point hit\n");
		return;
	}

	master_ID = (value & EMI_WP_AXI_ID);
	pr_err("[EMI_WP] Violation master ID is 0x%x.\n", master_ID);
	pr_err("[EMI_WP] Violation Address is : 0x%llx\n",
	(readl(IOMEM(EMI_CHKER_ADR)) + (((unsigned long long)(readl(IOMEM(EMI_CHKER_ADR_2ND)) & 0xF)) << 32)
	+ emi_physical_offset));

	master_name = __id2name(master_ID);
	pr_err("[EMI_WP] EMI_CHKER = 0x%x, module is %s.\n",
	value, master_name);

	value = readl(IOMEM(EMI_CHKER_TYPE));
	pr_err("[EMI_WP] Transaction Type is : %d beat, %d byte, %s burst type (0x%X)\n",
	       (value & 0xF) + 1,
	       1 << ((value >> 4) & 0x7),
	       (value >> 7 & 1) ? "INCR" : "WRAP",
	       value);

	emi_wp_clr_status();
}

static int emi_wp_set(unsigned int enable,
unsigned int address, unsigned int range, unsigned int rw)
{
	if (address < emi_physical_offset) {
		pr_err("[EMI_WP] Address error, you can't set address less than 0x%X\n",
		emi_physical_offset);
		return -1;
	}
	if (range < 4 || range > 32) {
		pr_err("[EMI_WP] Range error, you can't set range less than 16 bytes and more than 4G bytes\n");
		return -1;
	}

	emi_wp_set_monitor_type(rw);
	emi_wp_set_address(address);
	emi_wp_set_range(range);
	emi_wp_slave_error_enable(1);
	emi_wp_int_enable(0);
	emi_wp_enable(enable);

	return 0;
}

static ssize_t emi_wp_vio_show(struct device_driver *driver, char *buf)
{
	unsigned int value, master_ID, type;
	unsigned long long vio_addr;
	char *master_name;
	char *ptr = buf;

	value = readl(IOMEM(EMI_CHKER));

	if ((value & 0x80000000) == 0) {
		return snprintf(buf, PAGE_SIZE,
		"[EMI_WP] No watch point hit\n");
	}

	master_ID = (value & EMI_WP_AXI_ID);
	master_name = __id2name(master_ID);

	type = readl(IOMEM(EMI_CHKER_TYPE));
	vio_addr = (readl(IOMEM(EMI_CHKER_ADR)) + (((unsigned long long)(readl(IOMEM(EMI_CHKER_ADR_2ND)) & 0xF)) << 32)
	+ emi_physical_offset);
	emi_wp_clr_status();
	ptr += snprintf(ptr, PAGE_SIZE, "[EMI WP] vio setting is: CHKER 0x%X, ",
			value);

	ptr += snprintf(ptr, PAGE_SIZE, "module is %s, Address is : 0x%llX,",
			master_name,
			vio_addr);

	ptr += snprintf(ptr, PAGE_SIZE, "Transaction Type is : %d beat, ",
			(type & 0xF) + 1);

	ptr += snprintf(ptr, PAGE_SIZE, "%d byte, %s burst type (0x%X)\n",
			1 << ((type >> 4) & 0x7),
			(type >> 7 & 1) ? "INCR" : "WRAP",
			type);
	return ptr - buf;
}

ssize_t emi_wp_vio_store(struct device_driver *driver,
const char *buf, size_t count)
{
	int i;
	unsigned int wp_addr;
	unsigned int range, start_addr, end_addr;
	unsigned int rw;
	char *command;
	char *ptr;
	char *token[5];
	int err = 0;

	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("emi_wp_store command overflow.");
		return count;
	}
	pr_err("emi_wp_store: %s\n", buf);

	command = kmalloc((size_t)MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strncpy(command, buf, (size_t)MAX_EMI_MPU_STORE_CMD_LEN);
	ptr = (char *)buf;

	if (!strncmp(buf, EN_WP_STR, strlen(EN_WP_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_debug("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 4; i++)
			pr_debug("token[%d] = %s\n", i, token[i]);

		err += kstrtoul(token[1], 16, (unsigned long *)&wp_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&range);
		err += kstrtoul(token[3], 16, (unsigned long *)&rw);
		if (err)
			goto out;

		emi_wp_set(1, wp_addr, range, rw);

		start_addr = (wp_addr >> range) << range;
		end_addr = start_addr + (1 << range)     - 1;
		pr_err("Set EMI_WP: address: 0x%x, range:%d, start addr: 0x%x, end addr: 0x%x,  rw: %d .\n",
		       wp_addr, range, start_addr, end_addr, rw);
	} else if (!strncmp(buf, DIS_WP_STR, strlen(DIS_WP_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_debug("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 4; i++)
			pr_debug("token[%d] = %s\n", i, token[i]);


		err += kstrtoul(token[1], 16, (unsigned long *)&wp_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&range);
		err += kstrtoul(token[3], 16, (unsigned long *)&rw);
		if (err)
			goto out;

		emi_wp_set(0, 0x40000000, 4, 2);
		pr_err("disable EMI WP\n");
	} else {
		pr_err("Unknown emi_wp command.\n");
	}

out:
	kfree(command);

	return count;
}


DRIVER_ATTR(emi_wp_vio, 0644, emi_wp_vio_show, emi_wp_vio_store);
#endif /* #ifdef ENABLE_EMI_WATCH_POINT */

#if ENABLE_AP_REGION
#define AP_REGION_ID   31
static void protect_ap_region(void)
{
	unsigned int ap_mem_mpu_id;
	unsigned long long ap_mem_mpu_attr;
	unsigned long long kernel_base;
	phys_addr_t dram_size;

	kernel_base = emi_physical_offset;
	dram_size = get_max_DRAM_size();

	ap_mem_mpu_id = AP_REGION_ID;
	ap_mem_mpu_attr = SET_ACCESS_PERMISSON(LOCK,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
		FORBIDDEN, FORBIDDEN, NO_PROTECTION, NO_PROTECTION,
		FORBIDDEN, SEC_R_NSEC_RW, FORBIDDEN, NO_PROTECTION,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION);

	emi_mpu_set_region_protection(kernel_base,
		(kernel_base+dram_size-1), ap_mem_mpu_id, ap_mem_mpu_attr);

}
#endif

static struct platform_driver emi_mpu_ctrl = {
	.driver = {
		.name = "emi_mpu_ctrl",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
	.id_table = NULL,
};

static int __init emi_mpu_mod_init(void)
{
	int ret;
	unsigned int mpu_irq = 0;

#if EMI_MPU_TEST
	unsigned int *ptr0_phyical;

	ptr0_phyical = (unsigned int *)__pa(mpu_test_buffer);
	pr_err("[EMI_MPU]   ptr0_phyical : %p\n", ptr0_phyical);
	*((volatile unsigned int *)(mpu_test_buffer+0x10000)) = 0xdeaddead;
#endif

	pr_info("Initialize EMI MPU.\n");

	isetocunt = 0;

	/* DTS version */
	CEN_EMI_BASE = mt_cen_emi_base_get();
	CHA_EMI_BASE = mt_chn_emi_base_get(0);
	CHB_EMI_BASE = mt_chn_emi_base_get(1);
	EMI_MPU_BASE = mt_emi_mpu_base_get();

	if (CEN_EMI_BASE == NULL) {
		pr_err("[EMI MPU] can't get CEN_EMI_BASE\n");
		return -1;
	}

	if (CHA_EMI_BASE == NULL) {
		pr_err("[EMI MPU] can't get CHA_EMI_BASE\n");
		return -1;
	}

	if (CHB_EMI_BASE == NULL) {
		pr_err("[EMI MPU] can't get CHB_EMI_BASE\n");
		return -1;
	}

	if (EMI_MPU_BASE == NULL) {
		pr_err("[EMI MPU] can't get EMI_MPU_BASE\n");
		return -1;
	}

	mt_emi_reg_base_set(CEN_EMI_BASE);

	pr_err("[EMI MPU] EMI_MPUS = 0x%x\n", readl(IOMEM(EMI_MPUS)));
	pr_err("[EMI MPU] EMI_MPUT = 0x%x\n", readl(IOMEM(EMI_MPUT)));
	pr_err("[EMI MPU] EMI_MPUT_2ND = 0x%x\n", readl(IOMEM(EMI_MPUT_2ND)));

	pr_err("[EMI MPU] EMI_WP_ADR = 0x%x\n", readl(IOMEM(EMI_WP_ADR)));
	pr_err("[EMI MPU] EMI_WP_ADR_2ND = 0x%x\n", readl(IOMEM(EMI_WP_ADR_2ND)));
	pr_err("[EMI MPU] EMI_WP_CTRL = 0x%x\n", readl(IOMEM(EMI_WP_CTRL)));
	pr_err("[EMI MPU] EMI_CHKER = 0x%x\n", readl(IOMEM(EMI_CHKER)));
	pr_err("[EMI MPU] EMI_CHKER_TYPE = 0x%x\n", readl(IOMEM(EMI_CHKER_TYPE)));
	pr_err("[EMI MPU] EMI_CHKER_ADR = 0x%x\n", readl(IOMEM(EMI_CHKER_ADR)));
	pr_err("[EMI MPU] EMI_CHKER_ADR_2ND = 0x%x\n", readl(IOMEM(EMI_CHKER_ADR_2ND)));

	if (readl(IOMEM(EMI_MPUS))) {
		pr_err("[EMI MPU] get MPU violation in driver init\n");
		mpu_check_violation();
	} else {
		__clear_emi_mpu_vio(0);
	}

	emi_physical_offset = 0x40000000;

	/*
	 * NoteXXX: Interrupts of violation (including SPC in SMI, or EMI MPU)
	 *		  are triggered by the device APC.
	 *		  Need to share the interrupt with the SPC driver.
	 */
	mpu_irq = mt_emi_mpu_irq_get();
	if (mpu_irq != 0) {
		ret = request_irq(mpu_irq,
			(irq_handler_t)mpu_violation_irq, IRQF_TRIGGER_LOW | IRQF_SHARED,
			"mt_emi_mpu", &emi_mpu_ctrl);
		if (ret != 0) {
			pr_err("Fail to request EMI_MPU interrupt. Error = %d.\n", ret);
			return ret;
		}
	}

#if ENABLE_AP_REGION
	protect_ap_region();
#endif

#ifdef ENABLE_EMI_CHKER
	/* AXI violation monitor setting and timer function create */
	mt_reg_sync_writel((1 << AXI_VIO_CLR) | readl(IOMEM(EMI_CHKER)), EMI_CHKER);
	emi_axi_set_master(MASTER_ALL);
	init_timer(&emi_axi_vio_timer);
	emi_axi_vio_timer.expires = jiffies + AXI_VIO_MONITOR_TIME;
	emi_axi_vio_timer.function = &emi_axi_vio_timer_func;
	emi_axi_vio_timer.data = ((unsigned long) 0);
#endif /* #ifdef ENABLE_EMI_CHKER */

#if !defined(USER_BUILD_KERNEL)
	/* register driver and create sysfs files */
	ret = platform_driver_register(&emi_mpu_ctrl);
	if (ret)
		pr_err("Fail to register EMI_MPU driver.\n");

	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_mpu_config);
	if (ret)
		pr_err("Fail to create MPU config sysfs file.\n");

#ifdef ENABLE_EMI_CHKER
ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_emi_axi_vio);

	if (ret)
		pr_err("Fail to create AXI violation monitor sysfs file.\n");
#endif

#ifdef ENABLE_EMI_WATCH_POINT
	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_emi_wp_vio);
	if (ret)
		pr_err("Fail to create WP violation monitor sysfs file.\n");

#endif
#endif

	return 0;
}

static void __exit emi_mpu_mod_exit(void)
{
}

module_init(emi_mpu_mod_init);
module_exit(emi_mpu_mod_exit);

