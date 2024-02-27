/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MTK xhci quirk driver
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Denis Hsu <denis.hsu@mediatek.com>
 */

#ifndef __XHCI_MTK_QUIRKS_H
#define __XHCI_MTK_QUIRKS_H

#include <linux/usb.h>
#include "usbaudio.h"

void xhci_mtk_apply_quirk(struct usb_device *udev);
void xhci_mtk_sound_usb_connect(void *unused, struct usb_interface *intf, struct snd_usb_audio *chip);
void xhci_mtk_trace_init(struct device *dev);
void xhci_mtk_trace_deinit(struct device *dev);

#endif
