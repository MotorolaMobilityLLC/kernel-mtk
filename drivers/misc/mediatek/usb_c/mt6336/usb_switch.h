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

extern int usb3_switch_en(struct usbtypc *typec, int on);
extern int usb3_switch_sel(struct usbtypc *typec, int sel);
extern int usb_redriver_config(struct usbtypc *typec, int ctrl_pin, int stat);
extern int usbc_pinctrl_init(void);
extern struct usbtypc *get_usbtypec(void);

