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

#ifndef _DISP_RECOVERY_H_
#define _DISP_RECOVERY_H_

#define GPIO_EINT_MODE	0
#define GPIO_DSI_MODE	1



/* defined in mtkfb.c should move to mtkfb.h*/
extern unsigned int islcmconnected;


void primary_display_check_recovery_init(void);
void primary_display_esd_check_enable(int enable);
unsigned int need_wait_esd_eof(void);

#endif
