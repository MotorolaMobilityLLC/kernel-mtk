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

#ifndef _XHCI_MTK_POWER_H
#define _XHCI_MTK_POWER_H

/*#include <linux/usb.h>*/
#include "mtk_test_usb.h"

#include "xhci.h"
#include "mtk-test-lib.h"

extern int g_num_u3_port;
extern int g_num_u2_port;

extern void mtktest_enableXhciAllPortPower(struct xhci_hcd *xhci);
extern void mtktest_disableXhciAllPortPower(struct xhci_hcd *xhci);
extern void mtktest_enableAllClockPower(void);
extern void mtktest_disablePortClockPower(int port_index, int port_rev);
extern void mtktest_enablePortClockPower(int port_index, int port_rev);
extern void mtktest_disableAllClockPower(void);
extern void mtktest_resetIP(void);
extern unsigned int pmic_read_interface(unsigned int RegNum, unsigned int *val, unsigned int MASK, unsigned int SHIFT);
extern unsigned int pmic_config_interface(unsigned int RegNum, unsigned int val, unsigned int MASK, unsigned int SHIFT);
#ifdef CONFIG_MTK_BQ25896_SUPPORT
extern void bq25890_set_boost_ilim(unsigned int val);
extern void bq25890_otg_en(unsigned int val);
extern void bq25890_set_en_hiz(unsigned int val);
#endif
extern void usb_phy_recover(unsigned int clk_on);

#endif
