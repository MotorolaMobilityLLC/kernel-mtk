/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include "xhci-mtk.h"
#include "xhci-mtk-power.h"
#include "xhci-mtk-scheduler.h"
#include "mtk-phy.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/jiffies.h>


static const char hcd_name[] = "xhci-hcd";

/*defined for FPGA PHY daughter board init*/
#if FPGA_MODE
int mtk_u3h_phy_init(void)
{
		int PhyDrv;
	int TimeDelay;

	PhyDrv = 2;
	TimeDelay = U3_PHY_PIPE_PHASE_TIME_DELAY;

	u3phy_init();
	pr_debug("phy registers and operations initial done\n");
	if (u3phy_ops->u2_slew_rate_calibration)
		u3phy_ops->u2_slew_rate_calibration(u3phy);
	else
		pr_err("WARN: PHY doesn't implement u2 slew rate calibration function\n");

	if (u3phy_ops->init(u3phy) != PHY_TRUE) {
		pr_err("WARN: PHY INIT FAIL\n");
		return PHY_FALSE;
	}
	if ((u3phy_ops->change_pipe_phase(u3phy, PhyDrv, TimeDelay)) != PHY_TRUE) {
		pr_err("WARN: PHY change_pipe_phase FAIL\n");
		return PHY_FALSE;
	}
	return PHY_TRUE;
}
#endif


int get_xhci_u3_port_num(struct device *dev)
{
	struct mtk_u3h_hw *u3h_hw;
	__u32 __iomem *addr;
	u32 data;
	int u3_port_num;

	u3h_hw = dev->platform_data;

	addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_XHCI_CAP);
	data = readl(addr);
	u3_port_num = data & SSUSB_IP_XHCI_U3_PORT_NO;

	return u3_port_num;
}

int get_xhci_u2_port_num(struct device *dev)
{
	struct mtk_u3h_hw *u3h_hw;
	__u32 __iomem *addr;
	u32 data;
	int u2_port_num;

	u3h_hw = dev->platform_data;

	addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_XHCI_CAP);
	data = readl(addr);

	u2_port_num = (data & SSUSB_IP_XHCI_U2_PORT_NO) >> 8;

	return u2_port_num;
}

#ifdef FPGA_MODE
/*
 * the arrays (eof_cnt_table, frame_ref_clk, itp_delta_ratio) must
 * correspond to sequence as below define
 * #define FRAME_CK_60M    0
 * #define FRAME_CK_20M    1
 * #define FRAME_CK_24M    2
 * #define FRAME_CK_32M    3
 * #define FRAME_CK_48M    4
*/
enum {
	LS_OFFSET = 0,
	LS_BANK,
	FS_OFFSET,
	FS_BANK,
	HS_SYNC_OFFSET,
	HS_SYNC_BANK,
	HS_ASYNC_OFFSET,
	HS_ASYNC_BANK,
	SS_OFFSET,
	SS_BANK,
};
static int eof_cnt_table[][10] = {
	/* LS offset, bank; FS offset, bank; HS_sync_offset, bank; HS_async_offset,bank; SS_offset,bank*/
	{171, 19, 58, 10, 0, 4, 0, 2, 150, 0}, /* 60 MHz */
	{57, 19, 19, 10, 0, 4, 0, 2, 50, 0}, /* 20MHz */
	{68, 19, 23, 10, 0, 4, 0, 2, 60, 0}, /* 24 MHz */
	{91, 19, 31, 10, 0, 4, 0, 2, 80, 0},  /* 32 MHz */
	{137, 19, 46, 10, 0, 4, 0, 2, 120, 0} /* 48 MHz */
	};

static int frame_ref_clk[] = {60, 20, 24, 32, 48};

/*
 * calculate ITP_delta_value
 * from register map
 * 60MHz: 60/60 = 5'b01000
 * 20MHz: 60/20 = 5'b11000
 * 24MHz: 60/24 = 5'b10100
 * 32MHz: 60/32 = 5'b01111
 * 48MHz: 60/48 = 5'b01010
 */
static int itp_delta_ratio[] = {0x8, 0x18, 0x14, 0xf, 0xa};

void set_frame_cnt_ck(struct device *dev)
{
	struct mtk_u3h_hw *u3h_hw;
	void __iomem *xhci_reg;
	u32 level1_cnt;

	dev_warn(dev, "%s, cur frame_ck = %dMHz\n", __func__, frame_ref_clk[FRAME_CNT_CK_VAL]);
		u3h_hw = dev->platform_data;
	xhci_reg = (void __iomem *) u3h_hw->u3h_virtual_base;

	/*
	 * set two-level count wrapper boundary
	 * e.g. frame_ck = 60MHz
	 * total count value = 125us * 60MHz = 7500
	 * the level-2 counter wrapper boundary is bounded to 20
	 * so level-1 counter wrapper boundary = 7500 / 20 - 1 = 374
	 */
	u3h_writelmsk(xhci_reg + HFCNTR_CFG, (FRAME_LEVEL2_CNT - 1) << 17,
		INIT_FRMCNT_LEV2_FULL_RANGE);
	level1_cnt = 125 * frame_ref_clk[FRAME_CNT_CK_VAL] / FRAME_LEVEL2_CNT;
	/* double confirm integer divider */
	WARN_ON(level1_cnt * FRAME_LEVEL2_CNT != 125 * frame_ref_clk[FRAME_CNT_CK_VAL]);
	u3h_writelmsk(xhci_reg + HFCNTR_CFG, (level1_cnt - 1) << 8,
		INIT_FRMCNT_LEV1_FULL_RANGE);

	/* set ITP delta ratio */
	u3h_writelmsk(xhci_reg + HFCNTR_CFG, (itp_delta_ratio[FRAME_CNT_CK_VAL] << 1),
		ITP_DELTA_CLK_RATIO);

	/*
	 * set EOF generated point
	 * e.g., LS EOF, frame_ref_ck = 60 MHz, LS_EOF_BANK = 19, LS_EOF_OFFSET = 171
	 * when level-2 counter == 19, level-1 count == 171, HW generates LS_EOF signal
	 * the value has no special concern, just suggested by IC designer
	 * xx_BANK: alias of level-2 count value
	 * xx_OFFSET: alias of level-1 count value
	 */
	u3h_writelmsk(xhci_reg + LS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][LS_OFFSET],
		LS_EOF_OFFSET);
	u3h_writelmsk(xhci_reg + LS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][LS_BANK] << 16,
		LS_EOF_BANK);

	u3h_writelmsk(xhci_reg + FS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][FS_OFFSET],
		FS_EOF_OFFSET);
	u3h_writelmsk(xhci_reg + FS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][FS_BANK] << 16,
		FS_EOF_BANK);

	u3h_writelmsk(xhci_reg + SYNC_HS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][HS_SYNC_OFFSET],
		SYNC_HS_EOF_OFFSET);
	u3h_writelmsk(xhci_reg + SYNC_HS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][HS_SYNC_BANK] << 16,
		SYNC_HS_EOF_BANK);

	u3h_writelmsk(xhci_reg + ASYNC_HS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][HS_ASYNC_OFFSET],
		ASYNC_HS_EOF_OFFSET);
	u3h_writelmsk(xhci_reg + ASYNC_HS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][HS_ASYNC_BANK] << 16,
		ASYNC_HS_EOF_BANK);

	u3h_writelmsk(xhci_reg + SS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][SS_OFFSET],
		SS_EOF_OFFSET);
	u3h_writelmsk(xhci_reg + SS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][SS_BANK] << 16,
		SS_EOF_BANK);

}

#else
/*
 * XXX have suggested that IC designers shall set the default value due to ASIC clock
 * i.e., ASIC can use the default value
 */
void set_frame_cnt_ck(struct device *dev) { }
#endif

void setInitialReg(struct device *dev)
{
	int i;
	struct mtk_u3h_hw *u3h_hw;
	__u32 __iomem *usb3_csr_base;
	__u32 __iomem *usb2_csr_base;
	__u32 __iomem *addr;
	u32 data;

	int u3_port_num, u2_port_num;

		u3h_hw = dev->platform_data;
		usb3_csr_base = (u32 __iomem *)(u3h_hw->u3h_virtual_base + SSUSB_USB3_CSR_OFFSET);
	usb2_csr_base = (u32 __iomem *)(u3h_hw->u3h_virtual_base + SSUSB_USB2_CSR_OFFSET);

	/* get u3 & u2 port num */
	u3_port_num = get_xhci_u3_port_num(dev);
	u2_port_num = get_xhci_u2_port_num(dev);

	for (i = 0; i < u3_port_num; i++) {
		/* set MAC reference clock speed */
		addr = usb3_csr_base + U3H_UX_EXIT_LFPS_TIMING_PARAMETER;
		data = ((300*U3_REF_CK_VAL + (1000-1)) / 1000);
		u3h_writelmsk(addr, data, RX_UX_EXIT_LFPS_REF);

		addr = usb3_csr_base + U3H_REF_CK_PARAMETER;
		data = U3_REF_CK_VAL;
		u3h_writelmsk(addr, data, REF_1000NS);

		/* set SYS_CK */
		addr = usb3_csr_base + U3H_TIMING_PULSE_CTRL;
		data = U3_SYS_CK_VAL;
		u3h_writelmsk(addr, data, CNT_1US_VALUE);
	}

	for (i = 0; i < u2_port_num; i++) {
		addr = usb2_csr_base + U3H_USB20_TIMING_PARAMETER;
		data = U3_SYS_CK_VAL;
		u3h_writelmsk(addr, data, TIME_VALUE_1US);
	}
#ifdef FPGA_MODE
	set_frame_cnt_ck(dev);
#endif
}

void reinitIP(struct device *dev)
{
	enableAllClockPower(dev);
	setInitialReg(dev);
	mtk_xhci_scheduler_init(dev);
}

/* return code */
#define RET_SUCCESS 0
#define RET_FAIL -1
static bool not_chk_frm;

/* automatilcally check frame cnt value in case that SA forgets to confirm */
int chk_frmcnt_clk(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci;
	int frame_id, frame_id_new, delta, ret;

	ret = RET_SUCCESS;
	if (not_chk_frm)
		return ret;
	not_chk_frm = true;
	xhci = hcd_to_xhci(hcd);
	frame_id = xhci_readl(xhci, &xhci->run_regs->microframe_index);
	msleep(1000);
	frame_id_new = xhci_readl(xhci, &xhci->run_regs->microframe_index);
	if (frame_id_new < frame_id)
		frame_id_new += 1<<14;
	delta = (frame_id_new - frame_id) >> 3;
	xhci_err(xhci, "frame timing delta = %d\n",
		delta);
	if (abs(delta-1000) > 10) {
		ret = RET_FAIL;
		xhci_err(xhci, "\n\n+++++++++\nERROR! maybe frame cnt clock is wrong!\n");
		xhci_err(xhci, "start=%d, end=%d\n+++++++++\n\n", frame_id, frame_id_new);
	}
	return ret;
}



#if CFG_DEV_U3H0
static struct resource mtk_resource_u3h0[] = {
	[0] = {
				.start = U3H_IRQ0,
				.end   = U3H_IRQ0,
				.flags = IORESOURCE_IRQ,
	},
	[1] = {
						.name = "u3h",
			 /*physical address*/
				.start = U3H_BASE0,
				.end   = U3H_BASE0 + MTK_U3H_SIZE - 1,
				.flags = IORESOURCE_MEM,
	},
	[2] = {
						.name = "ippc",
			 /*physical address*/
				.start = IPPC_BASE0,
				.end   = IPPC_BASE0 + MTK_IPPC_SIZE - 1,
				.flags = IORESOURCE_MEM,
	},

};
#endif

#if CFG_DEV_U3H1
static struct resource mtk_resource_u3h1[] = {
	[0] = {
			.start = U3H_IRQ1,
			.end   = U3H_IRQ1,
			.flags = IORESOURCE_IRQ,
	},
	[1] = {
			.name = "u3h",
			 /*physical address*/
			.start = U3H_BASE1,
			.end   = U3H_BASE1 + MTK_U3H_SIZE - 1,
			.flags = IORESOURCE_MEM,
	},
	[2] = {
			.name = "ippc",
			 /*physical address*/
			.start = IPPC_BASE1,
			.end   = IPPC_BASE1 + MTK_IPPC_SIZE - 1,
			.flags = IORESOURCE_MEM,
	},

};
#endif

#if CFG_DEV_U3H0
struct mtk_u3h_hw u3h_hw0;
#endif

#if CFG_DEV_U3H1
struct mtk_u3h_hw u3h_hw1;
#endif

static u64 mtk_u3h_dma_mask = 0xffffffffUL;

static struct platform_device mtk_device_u3h[] = {
#if CFG_DEV_U3H0
	{
		.name          = hcd_name,
		.id            = 0,
		.resource      = mtk_resource_u3h0,
		.num_resources = ARRAY_SIZE(mtk_resource_u3h0),
		.dev           = {
							.platform_data = &u3h_hw0,
							.dma_mask = &mtk_u3h_dma_mask,
							.coherent_dma_mask = 0xffffffffUL,
						 },
	},
#endif
#if CFG_DEV_U3H1
	{
		.name          = hcd_name,
		.id            = 0,
		.resource      = mtk_resource_u3h1,
		.num_resources = ARRAY_SIZE(mtk_resource_u3h1),
		.dev           = {
							.platform_data = &u3h_hw1,
							.dma_mask = &mtk_u3h_dma_mask,
							.coherent_dma_mask = 0xffffffffUL,
						 },
	},
#endif
};


static  int __init mtk_u3h_init(void)
{
	int ret;
	int i;
	int u3h_dev_num;

	u3h_dev_num = ARRAY_SIZE(mtk_device_u3h);
	for (i = 0; i < u3h_dev_num; i++) {
		ret = platform_device_register(&mtk_device_u3h[i]);
		if (ret != 0)
			return ret;
	}

#if FPGA_MODE
		mtk_u3h_phy_init();
#endif

	return ret;
}

static void __exit mtk_u3h_cleanup(void)
{
	int u3h_dev_num;
	int i;

	u3h_dev_num = ARRAY_SIZE(mtk_device_u3h);
	for (i = 0; i < u3h_dev_num; i++)
		platform_device_unregister(&mtk_device_u3h[i]);
}

module_init(mtk_u3h_init);
module_exit(mtk_u3h_cleanup);
MODULE_LICENSE("GPL");
