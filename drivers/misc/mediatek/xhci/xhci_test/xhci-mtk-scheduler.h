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

#ifndef _XHCI_MTK_SCHEDULER_H
#define _XHCI_MTK_SCHEDULER_H

#define MTK_SCH_NEW 1

#define SCH_SUCCESS 1
#define SCH_FAIL  0

#define MAX_EP_NUM      64
#define SS_BW_BOUND     51000
/* #define HS_BW_BOUND      6144*/
#define HS_BW_BOUND                   6145    /* for HS HB transfer test. (Test Plan SOP 3.4.1 and 3.4.2) */

#define USB_EP_CONTROL  0
#define USB_EP_ISOC   1
#define USB_EP_BULK   2
#define USB_EP_INT    3

#define USB_SPEED_LOW 1
#define USB_SPEED_FULL  2
#define USB_SPEED_HIGH  3
#define USB_SPEED_SUPER 5

/* mtk scheduler bitmasks */
#define BPKTS(p)  ((p) & 0x3f)
#define BCSCOUNT(p) (((p) & 0x7) << 8)
#define BBM(p)    ((p) << 11)
#define BOFFSET(p)  ((p) & 0x3fff)
#define BREPEAT(p)  (((p) & 0x7fff) << 16)


#define NULL ((void *)0)

struct mtk_xhci_ep_ctx {
	unsigned int ep_info;
	unsigned int ep_info2;
	unsigned long long deq;
	unsigned int tx_info;
	/* offset 0x14 - 0x1f reserved for HC internal use */
	unsigned int reserved[3];
};


struct sch_ep {
	/* device info */
	int dev_speed;
	int isTT;
	/* ep info */
	int is_in;
	int ep_type;
	int maxp;
	int interval;
	int burst;
	int mult;
	/* scheduling info */
	int offset;
	int repeat;
	int pkts;
	int cs_count;
	int burst_mode;
	/* other */
	int bw_cost;  /* bandwidth cost in each repeat; including overhead */
	unsigned int *ep;  /* address of usb_endpoint pointer */
};


int mtktest_mtk_xhci_scheduler_init(void);
int mtktest_mtk_xhci_scheduler_add_ep(int dev_speed, int is_in, int isTT, int ep_type,
	int maxp, int interval, int burst, int mult, unsigned int *ep, unsigned int *ep_ctx, struct sch_ep *sch_ep);

struct sch_ep *mtktest_mtk_xhci_scheduler_remove_ep(int dev_speed, int is_in, int isTT,
	int ep_type, unsigned int *ep);

#endif
