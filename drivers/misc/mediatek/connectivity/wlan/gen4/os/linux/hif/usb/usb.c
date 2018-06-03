/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/******************************************************************************
*[File]             sdio.c
*[Version]          v1.0
*[Revision Date]    2010-03-01
*[Author]
*[Description]
*    The program provides SDIO HIF driver
*[Copyright]
*    Copyright (C) 2010 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include "precomp.h"

#include <linux/usb.h>
#include <linux/mutex.h>

#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include "mt66xx_reg.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

#define HIF_USB_ERR_TITLE_STR           "["CHIP_NAME"] USB Access Error!"
#define HIF_USB_ERR_DESC_STR            "**USB Access Error**\n"

#define HIF_USB_ACCESS_RETRY_LIMIT      1

#define MT_MAC_BASE                     0x2

#define MTK_USB_PORT_MASK               0x0F
#define MTK_USB_BULK_IN_MIN_EP          4
#define MTK_USB_BULK_IN_MAX_EP          5
#define MTK_USB_BULK_OUT_MIN_EP         4
#define MTK_USB_BULK_OUT_MAX_EP         9

static const struct usb_device_id mtk_usb_ids[] = {
	/* {USB_DEVICE(0x0E8D,0x6632), .driver_info = MT_MAC_BASE}, */
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x6632, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt6632},
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x7666, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt6632},
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x7668, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt7668},
	{ /* end: all zeroes */ },
};

MODULE_DEVICE_TABLE(usb, mtk_usb_ids);

#if CFG_USB_TX_AGG
/* TODO */
const UINT_32 USB_TX_DATA_BUF_SIZE[TC_NUM] = { NIC_TX_PAGE_COUNT_TC0 / (USB_REQ_TX_DATA_CNT - 1),
	NIC_TX_PAGE_COUNT_TC1 / (USB_REQ_TX_DATA_CNT - 1),
	NIC_TX_PAGE_COUNT_TC2 / (USB_REQ_TX_DATA_CNT - 1),
	NIC_TX_PAGE_COUNT_TC3 / (USB_REQ_TX_DATA_CNT - 1),
	NIC_TX_PAGE_COUNT_TC4 / (USB_REQ_TX_DATA_CNT - 1),
	NIC_TX_PAGE_COUNT_TC5 / (USB_REQ_TX_DATA_CNT - 1),
};
#endif

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static probe_card pfWlanProbe;
static remove_card pfWlanRemove;

static BOOLEAN g_fgDriverProbed = FALSE;

static struct usb_driver mtk_usb_driver = {
	.name = "wlan",		/* "MTK USB WLAN Driver" */
	.id_table = mtk_usb_ids,
	.probe = NULL,
	.disconnect = NULL,
	.suspend = NULL,
	.resume = NULL,
	.reset_resume = NULL,
	.supports_autosuspend = 0,
};

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is a USB probe function
*
* \param[in] intf   USB interface
* \param[in] id     USB device id
*
* \return void
*/
/*----------------------------------------------------------------------------*/
static int mtk_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	int ret = 0;
	struct usb_device *dev;

	DBGLOG(HAL, EVENT, "mtk_usb_probe()\n");

	ASSERT(intf);
	ASSERT(id);

	dev = interface_to_usbdev(intf);
	dev = usb_get_dev(dev);

	/* printk(KERN_INFO DRV_NAME "Basic struct size checking...\n"); */
	/* printk(KERN_INFO DRV_NAME "sizeof(struct device) = %d\n", sizeof(struct device)); */
	/* printk(KERN_INFO DRV_NAME "sizeof(struct mmc_host) = %d\n", sizeof(struct mmc_host)); */
	/* printk(KERN_INFO DRV_NAME "sizeof(struct mmc_card) = %d\n", sizeof(struct mmc_card)); */
	/* printk(KERN_INFO DRV_NAME "sizeof(struct mmc_driver) = %d\n", sizeof(struct mmc_driver)); */
	/* printk(KERN_INFO DRV_NAME "sizeof(struct mmc_data) = %d\n", sizeof(struct mmc_data)); */
	/* printk(KERN_INFO DRV_NAME "sizeof(struct mmc_command) = %d\n", sizeof(struct mmc_command)); */
	/* printk(KERN_INFO DRV_NAME "sizeof(struct mmc_request) = %d\n", sizeof(struct mmc_request)); */
	/* printk(KERN_INFO DRV_NAME "sizeof(struct sdio_func) = %d\n", sizeof(struct sdio_func)); */

	/* printk(KERN_INFO DRV_NAME "Card information checking...\n"); */
	/* printk(KERN_INFO DRV_NAME "intf = 0x%p\n", intf); */
	/* printk(KERN_INFO DRV_NAME "Number of info = %d:\n", intf->card->num_info); */

#if 0
	for (i = 0; i < intf->card->num_info; i++)
		/* printk(KERN_INFO DRV_NAME "info[%d]: %s\n", i, intf->card->info[i]); */

	sdio_claim_host(intf);
	ret = sdio_enable_func(intf);
	sdio_release_host(intf);

	if (ret) {
		/* printk(KERN_INFO DRV_NAME"sdio_enable_func failed!\n"); */
		goto out;
	}
	/* printk(KERN_INFO DRV_NAME"sdio_enable_func done!\n"); */
#endif

#if 1
	DBGLOG(HAL, EVENT, "wlan_probe()\n");
	if (pfWlanProbe((PVOID) intf, (PVOID) id->driver_info) != WLAN_STATUS_SUCCESS) {
		/* printk(KERN_WARNING DRV_NAME"pfWlanProbe fail!call pfWlanRemove()\n"); */
		pfWlanRemove();
		DBGLOG(HAL, ERROR, "wlan_probe() failed\n");
		ret = -1;
	} else {
		g_fgDriverProbed = TRUE;
	}
#endif

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is a USB remove function
*
* \param[in] intf   USB interface
*
* \return void
*/
/*----------------------------------------------------------------------------*/
static void mtk_usb_disconnect(struct usb_interface *intf)
{
	P_GLUE_INFO_T prGlueInfo;

	DBGLOG(HAL, STATE, "mtk_usb_disconnect()\n");

	ASSERT(intf);
	prGlueInfo  = (P_GLUE_INFO_T)usb_get_intfdata(intf);

	prGlueInfo->rHifInfo.state = USB_STATE_LINK_DOWN;

	if (g_fgDriverProbed)
		pfWlanRemove();

	usb_set_intfdata(intf, NULL);
	usb_put_dev(interface_to_usbdev(intf));

	g_fgDriverProbed = FALSE;

	DBGLOG(HAL, STATE, "mtk_usb_disconnect() done\n");
}

int mtk_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
	P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)usb_get_intfdata(intf);
	UINT_8 count = 0;

	DBGLOG(HAL, STATE, "mtk_usb_suspend()\n");

	if (prGlueInfo->prAdapter->rWifiVar.ucWow) {
		DBGLOG(HAL, EVENT, "enter WOW flow\n");
		kalWowProcess(prGlueInfo, TRUE);
	}

	halUSBPreSuspendCmd(prGlueInfo->prAdapter);

	while (prGlueInfo->rHifInfo.state != USB_STATE_PRE_SUSPEND_DONE) {
		if (count > 25) {
			DBGLOG(HAL, ERROR, "pre_suspend timeout\n");
			break;
		}
		msleep(20);
		count++;
	}

	prGlueInfo->rHifInfo.state = USB_STATE_SUSPEND;
	halDisableInterrupt(prGlueInfo->prAdapter);
	halTxCancelAllSending(prGlueInfo->prAdapter);

	DBGLOG(HAL, STATE, "mtk_usb_suspend() done!\n");

	/* TODO */
	return 0;
}

int mtk_usb_resume(struct usb_interface *intf)
{
	int ret = 0;
	P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T)usb_get_intfdata(intf);

	DBGLOG(HAL, STATE, "mtk_usb_resume()\n");

	/* NOTE: USB bus may not really do suspend and resume*/
	ret = usb_control_msg(prGlueInfo->rHifInfo.udev,
			      usb_sndctrlpipe(prGlueInfo->rHifInfo.udev, 0), VND_REQ_FEATURE_SET,
			      DEVICE_VENDOR_REQUEST_OUT, FEATURE_SET_WVALUE_RESUME, 0, NULL, 0,
			      VENDOR_TIMEOUT_MS);
	if (ret)
		DBGLOG(HAL, ERROR, "VendorRequest FeatureSetResume ERROR: %x\n", (unsigned int)ret);

	prGlueInfo->rHifInfo.state = USB_STATE_PRE_RESUME;
	/* To trigger CR4 path */
	wlanSendDummyCmd(prGlueInfo->prAdapter, FALSE);

	prGlueInfo->rHifInfo.state = USB_STATE_LINK_UP;
	halEnableInterrupt(prGlueInfo->prAdapter);

	if (prGlueInfo->prAdapter->rWifiVar.ucWow) {
		DBGLOG(HAL, EVENT, "leave WOW flow\n");
		kalWowProcess(prGlueInfo, FALSE);
	}

	DBGLOG(HAL, STATE, "mtk_usb_resume() done!\n");

	/* TODO */
	return 0;
}

int mtk_usb_reset_resume(struct usb_interface *intf)
{
	DBGLOG(HAL, STATE, "mtk_usb_reset_resume()\n");

	mtk_usb_resume(intf);

	DBGLOG(HAL, STATE, "mtk_usb_reset_resume done!()\n");

	/* TODO */
	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief USB EP0 vendor request
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] uEndpointAddress
* \param[in] RequestType
* \param[in] Request
* \param[in] Value
* \param[in] Index
* \param[in] TransferBuffer
* \param[in] TransferBufferLength
*
* \retval 0          if success
*         non-zero   if fail, the return value of usb_control_msg()
*/
/*----------------------------------------------------------------------------*/
BOOL mtk_usb_vendor_request(IN P_GLUE_INFO_T prGlueInfo, IN UCHAR uEndpointAddress, IN UCHAR RequestType,
			    IN UCHAR Request, IN UINT_16 Value, IN UINT_16 Index, IN PVOID TransferBuffer,
			    IN UINT_32 TransferBufferLength)
{
	/* refer to RTUSB_VendorRequest */
	int ret = 0;

	/* TODO: semaphore */

	if (in_interrupt()) {
		DBGLOG(REQ, ERROR, "BUG: mtk_usb_vendor_request is called from invalid context\n");
		return FALSE;
	}

	if (prGlueInfo->rHifInfo.state != USB_STATE_LINK_UP)
		return FALSE;


#if 0
	if (prGlueInfo->ulFlag & GLUE_FLAG_HALT)
		return FALSE;
#endif

	mutex_lock(&prGlueInfo->rHifInfo.vendor_req_sem);

	if (RequestType == DEVICE_VENDOR_REQUEST_OUT)
		ret =
		    usb_control_msg(prGlueInfo->rHifInfo.udev,
				    usb_sndctrlpipe(prGlueInfo->rHifInfo.udev, uEndpointAddress), Request,
				    RequestType, Value, Index, TransferBuffer, TransferBufferLength,
				    VENDOR_TIMEOUT_MS);
	else if (RequestType == DEVICE_VENDOR_REQUEST_IN)
		ret =
		    usb_control_msg(prGlueInfo->rHifInfo.udev,
				    usb_rcvctrlpipe(prGlueInfo->rHifInfo.udev, uEndpointAddress), Request,
				    RequestType, Value, Index, TransferBuffer, TransferBufferLength,
				    VENDOR_TIMEOUT_MS);

	mutex_unlock(&prGlueInfo->rHifInfo.vendor_req_sem);

	return (ret == TransferBufferLength) ? 0 : ret;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief USB Bulk IN msg
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] len
* \param[in] buffer
* \param[in] InEp
*
* \retval
*/
/*----------------------------------------------------------------------------*/
int mtk_usb_bulk_in_msg(IN P_GLUE_INFO_T prGlueInfo, IN UINT_32 len, OUT UCHAR *buffer, int InEp)
{
	int ret = 0;
	UINT_32 count;

	if (in_interrupt()) {
		DBGLOG(REQ, ERROR, "BUG: mtk_usb_bulk_in_msg is called from invalid context\n");
		return FALSE;
	}

	mutex_lock(&prGlueInfo->rHifInfo.vendor_req_sem);

	/* do a blocking bulk read to get data from the device */
	ret = usb_bulk_msg(prGlueInfo->rHifInfo.udev,
			   usb_rcvbulkpipe(prGlueInfo->rHifInfo.udev, InEp), buffer, len, &count, BULK_TIMEOUT_MS);

	mutex_unlock(&prGlueInfo->rHifInfo.vendor_req_sem);

	if (!ret) {
#if 0 /* maximize buff len for usb in */
		if (count != len) {
			DBGLOG(HAL, WARN, "usb_bulk_msg(IN=%d) Warning. Data is not completed. (receive %u/%u)\n",
			       InEp, count, len);
		}
#endif
		return count;
	}

	DBGLOG(HAL, ERROR, "usb_bulk_msg(IN=%d) Fail. Error code = %d.\n", InEp, ret);
	return ret;
}

int mtk_usb_intr_in_msg(IN P_GLUE_INFO_T prGlueInfo, IN UINT_32 len, OUT UCHAR * buffer, int InEp)
{
	int ret = 0;
	UINT_32 count;

	if (in_interrupt()) {
		DBGLOG(REQ, ERROR, "BUG: mtk_usb_intr_in_msg is called from invalid context\n");
		return FALSE;
	}

	mutex_lock(&prGlueInfo->rHifInfo.vendor_req_sem);

	/* do a blocking interrupt read to get data from the device */
	ret = usb_interrupt_msg(prGlueInfo->rHifInfo.udev,
			   usb_rcvintpipe(prGlueInfo->rHifInfo.udev, InEp), buffer, len, &count, INTERRUPT_TIMEOUT_MS);

	mutex_unlock(&prGlueInfo->rHifInfo.vendor_req_sem);

	if (!ret) {
#if 0 /* maximize buff len for usb in */
		if (count != len) {
			DBGLOG(HAL, WARN, "usb_interrupt_msg(IN=%d) Warning. Data is not completed. (receive %u/%u)\n",
			       InEp, count, len);
		}
#endif
		return count;
	}

	DBGLOG(HAL, ERROR, "usb_interrupt_msg(IN=%d) Fail. Error code = %d.\n", InEp, ret);
	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief USB Bulk OUT msg
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] len
* \param[in] buffer
* \param[in] OutEp
*
* \retval
*/
/*----------------------------------------------------------------------------*/
int mtk_usb_bulk_out_msg(IN P_GLUE_INFO_T prGlueInfo, IN UINT_32 len, IN UCHAR *buffer, int OutEp)
{
	int ret = 0;
	UINT_32 count;

	if (in_interrupt()) {
		DBGLOG(REQ, ERROR, "BUG: mtk_usb_bulk_out_msg is called from invalid context\n");
		return FALSE;
	}

	mutex_lock(&prGlueInfo->rHifInfo.vendor_req_sem);

	/* do a blocking bulk read to get data from the device */
	ret = usb_bulk_msg(prGlueInfo->rHifInfo.udev,
			   usb_sndbulkpipe(prGlueInfo->rHifInfo.udev, OutEp), buffer, len, &count, BULK_TIMEOUT_MS);

	mutex_unlock(&prGlueInfo->rHifInfo.vendor_req_sem);

	if (!ret) {
		if (count != len) {
			DBGLOG(HAL, ERROR, "usb_bulk_msg(OUT=%d) Warning. Data is not completed. (send %u/%u)\n", OutEp,
			       count, len);
		}
		return ret;
	}

	DBGLOG(HAL, ERROR, "usb_bulk_msg(OUT=%d) Fail. Error code = %d.\n", OutEp, ret);
	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will register sdio bus to the os
*
* \param[in] pfProbe    Function pointer to detect card
* \param[in] pfRemove   Function pointer to remove card
*
* \return The result of registering sdio bus
*/
/*----------------------------------------------------------------------------*/
WLAN_STATUS glRegisterBus(probe_card pfProbe, remove_card pfRemove)
{
	int ret = 0;

	ASSERT(pfProbe);
	ASSERT(pfRemove);

	pfWlanProbe = pfProbe;
	pfWlanRemove = pfRemove;

	mtk_usb_driver.probe = mtk_usb_probe;
	mtk_usb_driver.disconnect = mtk_usb_disconnect;
	mtk_usb_driver.suspend = mtk_usb_suspend;
	mtk_usb_driver.resume = mtk_usb_resume;
	mtk_usb_driver.reset_resume = mtk_usb_reset_resume;
	mtk_usb_driver.supports_autosuspend = 1;

	ret = (usb_register(&mtk_usb_driver) == 0) ? WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;

	return ret;
}				/* end of glRegisterBus() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will unregister sdio bus to the os
*
* \param[in] pfRemove   Function pointer to remove card
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID glUnregisterBus(remove_card pfRemove)
{
	if (g_fgDriverProbed) {
		pfRemove();
		g_fgDriverProbed = FALSE;
	}
	usb_deregister(&mtk_usb_driver);
}				/* end of glUnregisterBus() */

PVOID glUsbInitQ(P_GL_HIF_INFO_T prHifInfo, struct list_head *prHead, UINT_32 u4Cnt)
{
	UINT_32 i;
	P_USB_REQ_T prUsbReqs, prUsbReq;

	INIT_LIST_HEAD(prHead);

	prUsbReqs = kcalloc(u4Cnt, sizeof(struct _USB_REQ_T), GFP_ATOMIC);
	prUsbReq = prUsbReqs;

	for (i = 0; i < u4Cnt; ++i) {
		prUsbReq->prHifInfo = prHifInfo;
		prUsbReq->prUrb = usb_alloc_urb(0, GFP_ATOMIC);

		if (prUsbReq->prUrb == NULL)
			DBGLOG(HAL, ERROR, "usb_alloc_urb() reports error\n");

		prUsbReq->prBufCtrl = NULL;

		INIT_LIST_HEAD(&prUsbReq->list);
		list_add_tail(&prUsbReq->list, prHead);

		prUsbReq++;
	}

	return (PVOID) prUsbReqs;
}

void glUsbUnInitQ(struct list_head *prHead)
{
	P_USB_REQ_T prUsbReq, prUsbReqNext;

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, prHead, list) {
		usb_free_urb(prUsbReq->prUrb);
		list_del_init(&prUsbReq->list);
	}
}

VOID glUsbEnqueueReq(P_GL_HIF_INFO_T prHifInfo, struct list_head *prHead, P_USB_REQ_T prUsbReq, BOOLEAN fgHead)
{
	unsigned long flags;

	spin_lock_irqsave(&prHifInfo->rQLock, flags);
	if (fgHead)
		list_add(&prUsbReq->list, prHead);
	else
		list_add_tail(&prUsbReq->list, prHead);
	spin_unlock_irqrestore(&prHifInfo->rQLock, flags);
}

P_USB_REQ_T glUsbDequeueReq(P_GL_HIF_INFO_T prHifInfo, struct list_head *prHead)
{
	P_USB_REQ_T prUsbReq;
	unsigned long flags;

	spin_lock_irqsave(&prHifInfo->rQLock, flags);
	if (list_empty(prHead)) {
		spin_unlock_irqrestore(&prHifInfo->rQLock, flags);
		return NULL;
	}
	prUsbReq = list_entry(prHead->next, struct _USB_REQ_T, list);
	list_del_init(prHead->next);
	spin_unlock_irqrestore(&prHifInfo->rQLock, flags);

	return prUsbReq;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function stores hif related info, which is initialized before.
*
* \param[in] prGlueInfo Pointer to glue info structure
* \param[in] u4Cookie   Pointer to UINT_32 memory base variable for _HIF_HPI
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID glSetHifInfo(P_GLUE_INFO_T prGlueInfo, ULONG ulCookie)
{
	struct usb_host_interface *alts;
	struct usb_host_endpoint *ep;
	struct usb_endpoint_descriptor *ep_desc;
	P_GL_HIF_INFO_T prHifInfo = &prGlueInfo->rHifInfo;
	P_USB_REQ_T prUsbReq, prUsbReqNext;
	UINT_32 i;
#if CFG_USB_TX_AGG
	UINT_8 ucTc;
#endif

	prHifInfo->eEventEpType = EVENT_EP_TYPE_UNKONW;

	prHifInfo->intf = (struct usb_interface *)ulCookie;
	prHifInfo->udev = interface_to_usbdev(prHifInfo->intf);

	alts = prHifInfo->intf->cur_altsetting;
	DBGLOG(HAL, STATE, "USB Device speed: %x [%u]\n",
		prHifInfo->udev->speed, alts->endpoint[0].desc.wMaxPacketSize);

	if (prHifInfo->eEventEpType == EVENT_EP_TYPE_UNKONW) {
		for (i = 0; i < alts->desc.bNumEndpoints; ++i) {
			ep = &alts->endpoint[i];
			if (ep->desc.bEndpointAddress == USB_EVENT_EP_IN) {
				ep_desc = &alts->endpoint[i].desc;
				switch (ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
				case USB_ENDPOINT_XFER_INT:
					prHifInfo->eEventEpType = EVENT_EP_TYPE_INTR;
					break;
				case USB_ENDPOINT_XFER_BULK:
				default:
					prHifInfo->eEventEpType = EVENT_EP_TYPE_BULK;
					break;
				}
			}
		}
	}
	ASSERT(prHifInfo->eEventEpType != EVENT_EP_TYPE_UNKONW);
	DBGLOG(HAL, INFO, "Event EP Type: %x\n", prHifInfo->eEventEpType);

	prHifInfo->prGlueInfo = prGlueInfo;
	usb_set_intfdata(prHifInfo->intf, prGlueInfo);

	SET_NETDEV_DEV(prGlueInfo->prDevHandler, &prHifInfo->udev->dev);

	mutex_init(&prHifInfo->vendor_req_sem);

	/* TX CMD */
	prHifInfo->prTxCmdReqHead = glUsbInitQ(prHifInfo, &prHifInfo->rTxCmdFreeQ, USB_REQ_TX_CMD_CNT);
	prUsbReq = list_entry(prHifInfo->rTxCmdFreeQ.next, struct _USB_REQ_T, list);
	i = 0;
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxCmdFreeQ, list) {
#if CFG_USB_CONSISTENT_DMA
		prHifInfo->rTxCmdBufCtrl[i].pucBuf =
		    usb_alloc_coherent(prHifInfo->udev, USB_TX_CMD_BUF_SIZE, GFP_ATOMIC,
				       &prUsbReq->prUrb->transfer_dma);
#else
		prHifInfo->rTxCmdBufCtrl[i].pucBuf = kmalloc(USB_TX_CMD_BUF_SIZE, GFP_ATOMIC);
#endif
		prHifInfo->rTxCmdBufCtrl[i].u4BufSize = USB_TX_CMD_BUF_SIZE;
		if (prHifInfo->rTxCmdBufCtrl[i].pucBuf == NULL) {
			DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
			goto error;
		}
		prUsbReq->prBufCtrl = &prHifInfo->rTxCmdBufCtrl[i];
		++i;
	}

	glUsbInitQ(prHifInfo, &prHifInfo->rTxCmdSendingQ, 0);

	/* TX Data */
#if CFG_USB_TX_AGG
	for (ucTc = 0; ucTc < USB_TC_NUM; ++ucTc) {
		prHifInfo->arTxDataReqHead[ucTc] = glUsbInitQ(prHifInfo,
								&prHifInfo->rTxDataFreeQ[ucTc], USB_REQ_TX_DATA_CNT);
		i = 0;
		list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataFreeQ[ucTc], list) {
			QUEUE_INITIALIZE(&prUsbReq->rSendingDataMsduInfoList);
			/* TODO: every endpoint should has an unique and only TC */
			*((PUINT_8)&prUsbReq->prPriv) = ucTc;
#if CFG_USB_CONSISTENT_DMA
			prHifInfo->rTxDataBufCtrl[ucTc][i].pucBuf =
			    usb_alloc_coherent(prHifInfo->udev, USB_TX_DATA_BUFF_SIZE, GFP_ATOMIC,
					       &prUsbReq->prUrb->transfer_dma);
#else
			prHifInfo->rTxDataBufCtrl[ucTc][i].pucBuf = kmalloc(USB_TX_DATA_BUFF_SIZE, GFP_ATOMIC);
#endif
			prHifInfo->rTxDataBufCtrl[ucTc][i].u4BufSize = USB_TX_DATA_BUFF_SIZE;
			prHifInfo->rTxDataBufCtrl[ucTc][i].u4WrIdx = 0;
			prUsbReq->prBufCtrl = &prHifInfo->rTxDataBufCtrl[ucTc][i];
			++i;
		}

		DBGLOG(INIT, INFO, "USB Tx URB INIT Tc[%u] cnt[%u] len[%u]\n", ucTc, i,
		       prHifInfo->rTxDataBufCtrl[ucTc][0].u4BufSize);

		glUsbInitQ(prHifInfo, &prHifInfo->rTxDataSendingQ[ucTc], 0);
	}
#else
	glUsbInitQ(prHifInfo, &prHifInfo->rTxDataFreeQ, USB_REQ_TX_DATA_CNT);
	prUsbReq = list_entry(prHifInfo->rTxDataFreeQ.next, struct _USB_REQ_T, list);
	i = 0;
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataFreeQ, list) {
		QUEUE_INITIALIZE(&prUsbReq->rSendingDataMsduInfoList);
#if CFG_USB_CONSISTENT_DMA
		prHifInfo->rTxDataBufCtrl[i].pucBuf =
		    usb_alloc_coherent(prHifInfo->udev, USB_TX_DATA_BUF_SIZE, GFP_ATOMIC,
				       &prUsbReq->prUrb->transfer_dma);
#else
		prHifInfo->rTxDataBufCtrl[i].pucBuf = kmalloc(USB_TX_DATA_BUF_SIZE, GFP_ATOMIC);
#endif
		prHifInfo->rTxDataBufCtrl[i].u4BufSize = USB_TX_DATA_BUF_SIZE;
		if (prHifInfo->rTxDataBufCtrl[i].pucBuf == NULL) {
			DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
			goto error;
		}
		prUsbReq->prBufCtrl = &prHifInfo->rTxDataBufCtrl[i];
		++i;
	}
#endif

#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
	glUsbInitQ(prHifInfo, &prHifInfo->rTxCmdCompleteQ, 0);
	glUsbInitQ(prHifInfo, &prHifInfo->rTxDataCompleteQ, 0);
#endif

	/* RX EVENT */
	prHifInfo->prRxEventReqHead = glUsbInitQ(prHifInfo, &prHifInfo->rRxEventFreeQ, USB_REQ_RX_EVENT_CNT);
	i = 0;
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxEventFreeQ, list) {
		prHifInfo->rRxEventBufCtrl[i].pucBuf = kmalloc(USB_RX_EVENT_BUF_SIZE, GFP_ATOMIC);
		prHifInfo->rRxEventBufCtrl[i].u4BufSize = USB_RX_EVENT_BUF_SIZE;
		if (prHifInfo->rRxEventBufCtrl[i].pucBuf == NULL) {
			DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
			goto error;
		}
		prUsbReq->prBufCtrl = &prHifInfo->rRxEventBufCtrl[i];
		++i;
	}

	glUsbInitQ(prHifInfo, &prHifInfo->rRxEventRunningQ, 0);

	/* RX Data */
	prHifInfo->prRxDataReqHead = glUsbInitQ(prHifInfo, &prHifInfo->rRxDataFreeQ, USB_REQ_RX_DATA_CNT);
	i = 0;
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxDataFreeQ, list) {
		prHifInfo->rRxDataBufCtrl[i].pucBuf = kmalloc(USB_RX_DATA_BUF_SIZE, GFP_ATOMIC);
		prHifInfo->rRxDataBufCtrl[i].u4BufSize = USB_RX_DATA_BUF_SIZE;
		if (prHifInfo->rRxDataBufCtrl[i].pucBuf == NULL) {
			DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
			goto error;
		}
		prUsbReq->prBufCtrl = &prHifInfo->rRxDataBufCtrl[i];
		++i;
	}

	glUsbInitQ(prHifInfo, &prHifInfo->rRxDataRunningQ, 0);

#if CFG_USB_RX_HANDLE_IN_HIF_THREAD
	glUsbInitQ(prHifInfo, &prHifInfo->rRxEventCompleteQ, 0);
	glUsbInitQ(prHifInfo, &prHifInfo->rRxDataCompleteQ, 0);
#endif

	prHifInfo->state = USB_STATE_LINK_UP;

	return;

error:
	/* TODO */
	;
}				/* end of glSetHifInfo() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function clears hif related info.
*
* \param[in] prGlueInfo Pointer to glue info structure
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID glClearHifInfo(P_GLUE_INFO_T prGlueInfo)
{
	/* P_GL_HIF_INFO_T prHifInfo = NULL; */
	/* ASSERT(prGlueInfo); */
	/* prHifInfo = &prGlueInfo->rHifInfo; */
#if CFG_USB_TX_AGG
	UINT_8 ucTc;
#endif
	P_USB_REQ_T prUsbReq, prUsbReqNext;
	P_GL_HIF_INFO_T prHifInfo = &prGlueInfo->rHifInfo;

#if CFG_USB_TX_AGG
	for (ucTc = 0; ucTc < USB_TC_NUM; ++ucTc) {
		list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataFreeQ[ucTc], list) {
#if CFG_USB_CONSISTENT_DMA
			usb_free_coherent(prHifInfo->udev, USB_TX_DATA_BUFF_SIZE,
				prUsbReq->prBufCtrl->pucBuf, prUsbReq->prUrb->transfer_dma);
#else
			kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
			usb_free_urb(prUsbReq->prUrb);
		}
		kfree(prHifInfo->arTxDataReqHead[ucTc]);
	}
#else
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataFreeQ, list) {
#if CFG_USB_CONSISTENT_DMA
		usb_free_coherent(prHifInfo->udev, USB_TX_DATA_BUFF_SIZE,
			prUsbReq->prBufCtrl->pucBuf, prUsbReq->prUrb->transfer_dma);
#else
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}
#endif

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxCmdFreeQ, list) {
#if CFG_USB_CONSISTENT_DMA
		usb_free_coherent(prHifInfo->udev, USB_TX_CMD_BUF_SIZE,
			prUsbReq->prBufCtrl->pucBuf, prUsbReq->prUrb->transfer_dma);
#else
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxCmdCompleteQ, list) {
#if CFG_USB_CONSISTENT_DMA
		usb_free_coherent(prHifInfo->udev, USB_TX_CMD_BUF_SIZE,
			prUsbReq->prBufCtrl->pucBuf, prUsbReq->prUrb->transfer_dma);
#else
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataCompleteQ, list) {
#if CFG_USB_CONSISTENT_DMA
		usb_free_coherent(prHifInfo->udev, USB_TX_CMD_BUF_SIZE,
			prUsbReq->prBufCtrl->pucBuf, prUsbReq->prUrb->transfer_dma);
#else
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}
#endif

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxDataFreeQ, list) {
		kfree(prUsbReq->prBufCtrl->pucBuf);
		usb_free_urb(prUsbReq->prUrb);
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxEventFreeQ, list) {
		kfree(prUsbReq->prBufCtrl->pucBuf);
		usb_free_urb(prUsbReq->prUrb);
	}

#if CFG_USB_RX_HANDLE_IN_HIF_THREAD
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxDataCompleteQ, list) {
		kfree(prUsbReq->prBufCtrl->pucBuf);
		usb_free_urb(prUsbReq->prUrb);
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxEventCompleteQ, list) {
		kfree(prUsbReq->prBufCtrl->pucBuf);
		usb_free_urb(prUsbReq->prUrb);
	}
#endif

	kfree(prHifInfo->prTxCmdReqHead);
	kfree(prHifInfo->prRxEventReqHead);
	kfree(prHifInfo->prRxDataReqHead);
}				/* end of glClearHifInfo() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Initialize bus operation and hif related information, request resources.
*
* \param[out] pvData    A pointer to HIF-specific data type buffer.
*                       For eHPI, pvData is a pointer to UINT_32 type and stores a
*                       mapped base address.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
BOOL glBusInit(PVOID pvData)
{
	return TRUE;
}				/* end of glBusInit() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Stop bus operation and release resources.
*
* \param[in] pvData A pointer to struct net_device.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID glBusRelease(PVOID pvData)
{
}				/* end of glBusRelease() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Setup bus interrupt operation and interrupt handler for os.
*
* \param[in] pvData     A pointer to struct net_device.
* \param[in] pfnIsr     A pointer to interrupt handler function.
* \param[in] pvCookie   Private data for pfnIsr function.
*
* \retval WLAN_STATUS_SUCCESS   if success
*         NEGATIVE_VALUE   if fail
*/
/*----------------------------------------------------------------------------*/
INT_32 glBusSetIrq(PVOID pvData, PVOID pfnIsr, PVOID pvCookie)
{
	int ret = 0;

	struct net_device *prNetDevice = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_GL_HIF_INFO_T prHifInfo = NULL;

	ASSERT(pvData);
	if (!pvData)
		return -1;

	prNetDevice = (struct net_device *)pvData;
	prGlueInfo = (P_GLUE_INFO_T) pvCookie;
	ASSERT(prGlueInfo);
	if (!prGlueInfo)
		return -1;

	prHifInfo = &prGlueInfo->rHifInfo;

	return ret;
}				/* end of glBusSetIrq() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Stop bus interrupt operation and disable interrupt handling for os.
*
* \param[in] pvData     A pointer to struct net_device.
* \param[in] pvCookie   Private data for pfnIsr function.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID glBusFreeIrq(PVOID pvData, PVOID pvCookie)
{
	struct net_device *prNetDevice = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_GL_HIF_INFO_T prHifInfo = NULL;

	ASSERT(pvData);
	if (!pvData) {
		/* printk(KERN_INFO DRV_NAME"%s null pvData\n", __FUNCTION__); */
		return;
	}
	prNetDevice = (struct net_device *)pvData;
	prGlueInfo = (P_GLUE_INFO_T) pvCookie;
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		/* printk(KERN_INFO DRV_NAME"%s no glue info\n", __FUNCTION__); */
		return;
	}

	prHifInfo = &prGlueInfo->rHifInfo;
}				/* end of glBusreeIrq() */

BOOLEAN glIsReadClearReg(UINT_32 u4Address)
{
	switch (u4Address) {
	case MCR_WHISR:
	case MCR_WASR:
	case MCR_D2HRM0R:
	case MCR_D2HRM1R:
	case MCR_WTQCR0:
	case MCR_WTQCR1:
	case MCR_WTQCR2:
	case MCR_WTQCR3:
	case MCR_WTQCR4:
	case MCR_WTQCR5:
	case MCR_WTQCR6:
	case MCR_WTQCR7:
		return TRUE;

	default:
		return FALSE;
	}
}

UINT_16 glGetUsbDeviceVendorId(struct usb_device *dev)
{
	return dev->descriptor.idVendor;
}				/* end of glGetUsbDeviceVendorId() */

UINT_16 glGetUsbDeviceProductId(struct usb_device *dev)
{
	return dev->descriptor.idProduct;
}				/* end of glGetUsbDeviceProductId() */

INT_32 glGetUsbDeviceManufacturerName(struct usb_device *dev, UCHAR *buffer, UINT_32 bufLen)
{
	return usb_string(dev, dev->descriptor.iManufacturer, buffer, bufLen);
}				/* end of glGetUsbDeviceManufacturerName() */

INT_32 glGetUsbDeviceProductName(struct usb_device *dev, UCHAR *buffer, UINT_32 bufLen)
{
	return usb_string(dev, dev->descriptor.iProduct, buffer, bufLen);
}				/* end of glGetUsbDeviceManufacturerName() */

INT_32 glGetUsbDeviceSerialNumber(struct usb_device *dev, UCHAR *buffer, UINT_32 bufLen)
{
	return usb_string(dev, dev->descriptor.iSerialNumber, buffer, bufLen);
}				/* end of glGetUsbDeviceSerialNumber() */


/*----------------------------------------------------------------------------*/
/*!
* \brief Read a 32-bit device register
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] u4Register Register offset
* \param[in] pu4Value   Pointer to variable used to store read value
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
BOOL kalDevRegRead(IN P_GLUE_INFO_T prGlueInfo, IN UINT_32 u4Register, OUT PUINT_32 pu4Value)
{
	int ret = 0;
	UINT_8 ucRetryCount = 0;

	ASSERT(prGlueInfo);
	ASSERT(pu4Value);

	*pu4Value = 0xFFFFFFFF;

	do {
		ret = mtk_usb_vendor_request(prGlueInfo, 0, DEVICE_VENDOR_REQUEST_IN, VND_REQ_REG_READ,
				       (u4Register & 0xffff0000) >> 16, (u4Register & 0x0000ffff), pu4Value,
				       sizeof(*pu4Value));

		if (ret || ucRetryCount)
			DBGLOG(HAL, ERROR, "usb_control_msg() status: %x retry: %u\n", (unsigned int)ret, ucRetryCount);


		ucRetryCount++;
		if (ucRetryCount > HIF_USB_ACCESS_RETRY_LIMIT)
			break;
	} while (ret);

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR,
				  HIF_USB_ERR_DESC_STR "USB() reports error: %x retry: %u", ret, ucRetryCount);
		DBGLOG(HAL, ERROR, "usb_readl() reports error: %x retry: %u\n", ret, ucRetryCount);
	} else {
		DBGLOG(HAL, INFO, "Get CR[0x%08x] value[0x%08x]\n", u4Register, *pu4Value);
	}

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevRegRead() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Write a 32-bit device register
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] u4Register Register offset
* \param[in] u4Value    Value to be written
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
BOOL kalDevRegWrite(IN P_GLUE_INFO_T prGlueInfo, IN UINT_32 u4Register, IN UINT_32 u4Value)
{
	int ret = 0;
	UINT_8 ucRetryCount = 0;

	ASSERT(prGlueInfo);

	do {
		ret = mtk_usb_vendor_request(prGlueInfo, 0, DEVICE_VENDOR_REQUEST_OUT, VND_REQ_REG_WRITE,
				       (u4Register & 0xffff0000) >> 16, (u4Register & 0x0000ffff), &u4Value,
				       sizeof(u4Value));

		if (ret || ucRetryCount)
			DBGLOG(HAL, ERROR, "usb_control_msg() status: %x retry: %u\n", (unsigned int)ret, ucRetryCount);

		ucRetryCount++;
		if (ucRetryCount > HIF_USB_ACCESS_RETRY_LIMIT)
			break;

	} while (ret);

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR,
				  HIF_USB_ERR_DESC_STR "usb_writel() reports error: %x retry: %u", ret, ucRetryCount);
		DBGLOG(HAL, ERROR, "usb_writel() reports error: %x retry: %u\n", ret, ucRetryCount);
	} else {
		DBGLOG(HAL, INFO, "Set CR[0x%08x] value[0x%08x]\n", u4Register, u4Value);
	}

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevRegWrite() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Read device I/O port
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] u2Port             I/O port offset
* \param[in] u2Len              Length to be read
* \param[out] pucBuf            Pointer to read buffer
* \param[in] u2ValidOutBufSize  Length of the buffer valid to be accessed
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
BOOL
kalDevPortRead(IN P_GLUE_INFO_T prGlueInfo,
	       IN UINT_16 u2Port, IN UINT_32 u4Len, OUT PUINT_8 pucBuf, IN UINT_32 u4ValidOutBufSize)
{
	P_GL_HIF_INFO_T prHifInfo = NULL;
	PUINT_8 pucDst = NULL;
	/* int count = u4Len; */
	int ret = 0;
	/* int bNum = 0; */

#if DBG
	DBGLOG(HAL, INFO, "++kalDevPortRead++ buf:0x%p, port:0x%x, length:%d\n", pucBuf, u2Port, u4Len);
#endif

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;

	ASSERT(pucBuf);
	pucDst = pucBuf;

	ASSERT(u4Len <= u4ValidOutBufSize);

	u2Port &= MTK_USB_PORT_MASK;
	if (prGlueInfo->rHifInfo.eEventEpType == EVENT_EP_TYPE_INTR &&
		u2Port == (USB_EVENT_EP_IN & USB_ENDPOINT_NUMBER_MASK)) {
		/* maximize buff len for usb in */
		ret = mtk_usb_intr_in_msg(prGlueInfo, u4ValidOutBufSize, pucDst, u2Port);
		if (ret != u4Len) {
			DBGLOG(HAL, WARN, "usb_interrupt_msg(IN=%d) Warning. Data is not completed. (receive %u/%u)\n",
			       u2Port, ret, u4Len);
		}
		ret = ret >= 0 ? 0 : ret;
	} else if (u2Port >= MTK_USB_BULK_IN_MIN_EP && u2Port <= MTK_USB_BULK_IN_MAX_EP) {
		/* maximize buff len for usb in */
		ret = mtk_usb_bulk_in_msg(prGlueInfo, u4ValidOutBufSize, pucDst, u2Port);
		if (ret != u4Len) {
			DBGLOG(HAL, WARN, "usb_bulk_msg(IN=%d) Warning. Data is not completed. (receive %u/%u)\n",
			       u2Port, ret, u4Len);
		}
		ret = ret >= 0 ? 0 : ret;
	} else {
		DBGLOG(HAL, ERROR, "kalDevPortRead reports error: invalid port %x\n", u2Port);
		ret = FALSE;
	}

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR, HIF_USB_ERR_DESC_STR "usb_readsb() reports error: %x", ret);
		DBGLOG(HAL, ERROR, "usb_readsb() reports error: %x\n", ret);
	}

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevPortRead() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Write device I/O port
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] u2Port             I/O port offset
* \param[in] u2Len              Length to be write
* \param[in] pucBuf             Pointer to write buffer
* \param[in] u2ValidInBufSize   Length of the buffer valid to be accessed
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
BOOL
kalDevPortWrite(IN P_GLUE_INFO_T prGlueInfo,
		IN UINT_16 u2Port, IN UINT_32 u4Len, IN PUINT_8 pucBuf, IN UINT_32 u4ValidInBufSize)
{
	P_GL_HIF_INFO_T prHifInfo = NULL;
	PUINT_8 pucSrc = NULL;
	/* int count = u4Len; */
	int ret = 0;
	/* int bNum = 0; */

#if DBG
	DBGLOG(HAL, INFO, "++kalDevPortWrite++ buf:0x%p, port:0x%x, length:%d\n", pucBuf, u2Port, u4Len);
#endif

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;

	ASSERT(pucBuf);
	pucSrc = pucBuf;

	ASSERT((u4Len + LEN_USB_UDMA_TX_TERMINATOR) <= u4ValidInBufSize);

	kalMemZero(pucSrc + u4Len, LEN_USB_UDMA_TX_TERMINATOR);
	u4Len += LEN_USB_UDMA_TX_TERMINATOR;

	u2Port &= MTK_USB_PORT_MASK;
	if (u2Port >= MTK_USB_BULK_OUT_MIN_EP && u2Port <= MTK_USB_BULK_OUT_MAX_EP) {
		ret = mtk_usb_bulk_out_msg(prGlueInfo, u4Len, pucSrc, 8);
	} else {
		DBGLOG(HAL, ERROR, "kalDevPortWrite reports error: invalid port %x\n", u2Port);
		ret = FALSE;
	}

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR, HIF_USB_ERR_DESC_STR "usb_writesb() reports error: %x", ret);
		DBGLOG(HAL, ERROR, "usb_writesb() reports error: %x\n", ret);
	}

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevPortWrite() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Set power state
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] ePowerMode
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
VOID glSetPowerState(IN P_GLUE_INFO_T prGlueInfo, IN UINT_32 ePowerMode)
{
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Write data to device
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] prMsduInfo         msdu info
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
BOOL kalDevWriteData(IN P_GLUE_INFO_T prGlueInfo, IN P_MSDU_INFO_T prMsduInfo)
{
	halTxUSBSendData(prGlueInfo, prMsduInfo);
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Kick Tx data to device
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
BOOL kalDevKickData(IN P_GLUE_INFO_T prGlueInfo)
{
	halTxUSBKickData(prGlueInfo);
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Write command to device
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] u4Addr             I/O port offset
* \param[in] ucData             Single byte of data to be written
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
BOOL kalDevWriteCmd(IN P_GLUE_INFO_T prGlueInfo, IN P_CMD_INFO_T prCmdInfo, IN UINT_8 ucTC)
{
	halTxUSBSendCmd(prGlueInfo, ucTC, prCmdInfo);
	return TRUE;
}

void glGetDev(PVOID ctx, struct device **dev)
{
	*dev = &(interface_to_usbdev((struct usb_interface *)ctx)->dev);
}

void glGetHifDev(P_GL_HIF_INFO_T prHif, struct device **dev)
{
	*dev = &(prHif->udev->dev);
}
