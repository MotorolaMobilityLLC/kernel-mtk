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

#define XHCI_MTK_TEST_MAJOR 222
#define DEVICE_NAME "clih"

/* for auto test struct defs */

#define USBIF_OTG_EVENT_DEV_CONN_TMOUT "DEV_CONN_TMOUT"
#define USBIF_OTG_EVENT_NO_RESP_FOR_HNP_ENABLE "NO_RESP_FOR_HNP_ENABLE"
#define USBIF_OTG_EVENT_HUB_NOT_SUPPORTED "HUB_NOT_SUPPORTED"
#define USBIF_OTG_EVENT_DEV_NOT_SUPPORTED "DEV_NOT_SUPPORTED"

enum USB_DIR {
	USB_TX = 0,
	USB_RX
};

enum USB_TEST_CASE {
	Ctrol_Transfer = 0,
	Bulk_Random,
	Test_Loopback,
	Test_End
};

/* CTRL, BULK, INTR, ISO endpoint */
enum USB_TRANSFER_TYPE {
	USB_CTRL = 0,
	USB_BULK,
	USB_INTR,
	USB_ISO
};

enum USB_SPEED {
	SPEED_HIGH = 0,
	SPEED_FULL
};

enum state {
	BUSY = 0,
	READY,
	END
};

enum status {
	TRANSFER_SUCCESS = 0,
	TRANSFER_FAIL
};

struct usb_transfer {
	unsigned char    type;
	unsigned char    speed;
	unsigned int     length;
	unsigned short   maxp;
	unsigned char    state;
	unsigned char    status;
};


struct USB_MSG {
	unsigned short header;
	unsigned char  testcase;
	struct usb_transfer  transfer;
	unsigned short end;
};

extern int xhci_usbif_resource_get(void);
/* USBIF , skip slew rate calibration ?*/
extern void phy_probe_out_x(void);
extern void phy_dump_regs(void);
extern void phy_probe_reg(void);
extern void phy_reset(void);
extern void usb_phy_swpllmode_lpm(void);

