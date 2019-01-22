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

typedef enum {
	USB_TX = 0,
	USB_RX
} USB_DIR;

typedef enum {
	Ctrol_Transfer = 0,
	Bulk_Random,
	Test_Loopback,
	Test_End
} USB_TEST_CASE;

/* CTRL, BULK, INTR, ISO endpoint */
typedef enum {
	USB_CTRL = 0,
	USB_BULK,
	USB_INTR,
	USB_ISO
} USB_TRANSFER_TYPE;

typedef enum {
	SPEED_HIGH = 0,
	SPEED_FULL
} USB_SPEED;

typedef enum {
	BUSY = 0,
	READY,
	END
} state;

typedef enum {
	TRANSFER_SUCCESS = 0,
	TRANSFER_FAIL
} status;

typedef struct {
	unsigned char    type;
	unsigned char    speed;
	unsigned int     length;
	unsigned short   maxp;
	unsigned char    state;
	unsigned char    status;
} USB_TRANSFER;


typedef struct {
	unsigned short header;
	unsigned char  testcase;
	USB_TRANSFER   transfer;
	unsigned short end;
} USB_MSG;

extern int xhci_usbif_resource_get(void);
/* USBIF , skip slew rate calibration ?*/
extern void phy_probe_out_x(void);
extern void phy_dump_regs(void);
extern void phy_probe_reg(void);
extern void phy_reset(void);
extern void usb_phy_swpllmode_lpm(void);

