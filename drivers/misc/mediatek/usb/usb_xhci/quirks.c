// SPDX-License-Identifier: GPL-2.0
/*
 * MTK xhci quirk driver
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Denis Hsu <denis.hsu@mediatek.com>
 */

#include <linux/usb/quirks.h>
#include "quirks.h"
#include "xhci-mtk.h"
#include "xhci-trace.h"

struct usb_audio_quirk_flags_table {
	u32 id;
	u32 flags;
};

#define DEVICE_FLG(vid, pid, _flags) \
	{ .id = USB_ID(vid, pid), .flags = (_flags) }
#define VENDOR_FLG(vid, _flags) DEVICE_FLG(vid, 0, _flags)

/* quirk list in usbcore */
static const struct usb_device_id mtk_usb_quirk_list[] = {
	{USB_DEVICE(0x12d1, 0x3a07), .driver_info = USB_QUIRK_IGNORE_REMOTE_WAKEUP|USB_QUIRK_RESET},

	{ }  /* terminating entry must be last */
};

/* quirk list in /sound/usb */
static const struct usb_audio_quirk_flags_table mtk_snd_quirk_flags_table[] = {
		DEVICE_FLG(0x2d99, 0xa026,
		   QUIRK_FLAG_CTL_MSG_DELAY),
		DEVICE_FLG(0x12d1, 0x3a07,
		   QUIRK_FLAG_CTL_MSG_DELAY),

		{} /* terminator */
};

static int usb_match_device(struct usb_device *dev, const struct usb_device_id *id)
{
	if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
		id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
		return 0;

	/* No need to test id->bcdDevice_lo != 0, since 0 is never
	   greater than any unsigned number. */
	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
		(id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
		(id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
		(id->bDeviceClass != dev->descriptor.bDeviceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
		(id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
		(id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
		return 0;

	return 1;
}

static u32 usb_detect_static_quirks(struct usb_device *udev,
					const struct usb_device_id *id)
{
	u32 quirks = 0;

	for (; id->match_flags; id++) {
		if (!usb_match_device(udev, id))
			continue;

		quirks |= (u32)(id->driver_info);
		dev_info(&udev->dev,
			  "Set usbcore quirk_flags 0x%x for device %04x:%04x\n",
			  (u32)id->driver_info, id->idVendor,
			  id->idProduct);
	}

	return quirks;
}

static void snd_usb_init_quirk_flags(struct snd_usb_audio *chip)
{
	const struct usb_audio_quirk_flags_table *p;

	for (p = mtk_snd_quirk_flags_table; p->id; p++) {
		if (chip->usb_id == p->id ||
			(!USB_ID_PRODUCT(p->id) &&
			 USB_ID_VENDOR(chip->usb_id) == USB_ID_VENDOR(p->id))) {
			dev_info(&chip->dev->dev,
					  "Set audio quirk_flags 0x%x for device %04x:%04x\n",
					  p->flags, USB_ID_VENDOR(chip->usb_id),
					  USB_ID_PRODUCT(chip->usb_id));
			chip->quirk_flags |= p->flags;
			return;
		}
	}
}

/* update mtk usbcore quirk */
void xhci_mtk_apply_quirk(struct usb_device *udev)
{
	if (!udev)
		return;

	udev->quirks = usb_detect_static_quirks(udev, mtk_usb_quirk_list);
}

/* update mtk usb audio quirk */
void xhci_mtk_sound_usb_connect(void *unused, struct usb_interface *intf, struct snd_usb_audio *chip)
{
	if (!chip)
		return;

	snd_usb_init_quirk_flags(chip);
}

static void xhci_trace_ep0_urb(void *data, struct urb *urb)
{
	struct device *hcd_dev = (struct device *)data;
	struct usb_ctrlrequest *ctrl = NULL;
	struct usb_host_config *config = NULL;
	struct usb_interface_descriptor *intf_desc = NULL;
	int config_num, i;

	if (!urb || !urb->setup_packet || !urb->dev) {
		dev_dbg(hcd_dev, "%s urb/setup pkt/device can't be NULL\n", __func__);
		return;
	}

	ctrl = (struct usb_ctrlrequest *)urb->setup_packet;
	if (ctrl->bRequest != USB_REQ_SET_INTERFACE || ctrl->wValue == 0) {
		dev_dbg(hcd_dev, "%s it's not ep0 transfer request\n", __func__);
		return;
	}

	config = urb->dev->config;
	if (!config)
		return;
	config_num = urb->dev->descriptor.bNumConfigurations;

	for (i = 0; i < config_num; i++, config++) {
		if (config && config->desc.bNumInterfaces > 0)
			intf_desc = &config->intf_cache[0]->altsetting->desc;
		if (intf_desc && intf_desc->bInterfaceClass == USB_CLASS_AUDIO) {
			dev_dbg(hcd_dev, "delay 5ms for UAC device\n");
			mdelay(5);
			return;
		}
	}
}

void xhci_mtk_trace_init(struct device *dev)
{
	WARN_ON(register_trace_xhci_urb_enqueue_(xhci_trace_ep0_urb, dev));
}
