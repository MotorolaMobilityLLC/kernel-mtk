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

#ifndef _XHCI_MTK_H
#define _XHCI_MTK_H

#include <linux/version.h>
/*#include <linux/usb.h>*/
#include "mtk_test_usb.h"

#include "xhci.h"
#include "xhci-mtk-scheduler.h"
#include "ssusb_sifslv_ippc.h"
#include "ssusb_usb3_mac_csr.h"
#include "ssusb_usb3_sys_csr.h"
#include "ssusb_usb2_csr.h"
#include "ssusb_xHCI_exclude_port_csr.h"

/* U3H IP CONFIG:
 * enable this according to the U3H IP num of the project
 */
#define  CFG_DEV_U3H0	1  /* if the project has one or more U3H IP, enable this */
#define  CFG_DEV_U3H1	0  /* if the project has two or more U3H IP, enable this */


#define FPGA_MODE		1   /* if run in FPGA,enable this */
#define OTG_SUPPORT		0   /* if OTG support,enable this */


/* U3H irq number*/
#if CFG_DEV_U3H0
	#define U3H_IRQ0 15
#endif
#if CFG_DEV_U3H1
	#define U3H_IRQ1 15
#endif


/*U3H register bank*/
#if CFG_DEV_U3H0
	/* physical base address for U3H IP0 */
	#define U3H_BASE0	0xf0040000
	#define IPPC_BASE0	0xf0044700
#endif
#if CFG_DEV_U3H0
	/* physical base address for U3H IP1 */
	#define U3H_BASE1	0xf0040000
	#define IPPC_BASE1	0xf0044700
#endif


/* Clock source */
/* Clock source may differ from project to project. Please check integrator */
#define U3_REF_CK_VAL 25      /* MHz = value */
#define U3_SYS_CK_VAL 125     /* MHz = value */
/*
 * HW uses a flexible clock to calculate SOF.
 * i.e., SW shall help HW to count the exact value to get 125us by set correct counter value
 */
#define FRAME_CK_60M    0
#define FRAME_CK_20M    1
#define FRAME_CK_24M    2
#define FRAME_CK_32M    3
#define FRAME_CK_48M    4

#define FRAME_CNT_CK_VAL  FRAME_CK_32M
#define FRAME_LEVEL2_CNT  20


#if FPGA_MODE
	/* Defined for PHY init in FPGA MODE */
	/* change this value according to U3 PHY calibration */
	#define U3_PHY_PIPE_PHASE_TIME_DELAY  0x16
#endif


/* offset may differ from project to project. Please check integrator */
#define SSUSB_USB3_CSR_OFFSET 0x00002400
#define SSUSB_USB2_CSR_OFFSET 0x00003400


#define MTK_U3H_SIZE  0x4000
#define MTK_IPPC_SIZE 0x100



/*=========================================================================================*/


#define u3h_writelmsk(addr, data, msk) \
	{ writel(((readl(addr) & ~(msk)) | ((data) & (msk))), addr); \
	}


struct mtk_u3h_hw {
	unsigned char  *u3h_virtual_base;
	unsigned char  *ippc_virtual_base;
	struct sch_port u3h_sch_port[MAX_PORT_NUM];
};

extern struct mtk_u3h_hw u3h_hw;

void reinitIP(struct device *dev);
void setInitialReg(struct device *dev);
void dbg_prb_out(void);
int u3h_phy_init(void);
int get_xhci_u3_port_num(struct device *dev);
int get_xhci_u2_port_num(struct device *dev);
int chk_frmcnt_clk(struct usb_hcd *hcd);

#endif
