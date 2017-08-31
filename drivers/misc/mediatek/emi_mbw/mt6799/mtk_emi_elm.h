/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __MT_EMI_BW_ELM_H__
#define __MT_EMI_BW_ELM_H__

#define EMI_LTCT0  (EMI_BASE_ADDR + 0x6B0)
#define EMI_LTCT1  (EMI_BASE_ADDR + 0x6B4)
#define EMI_LTCT2  (EMI_BASE_ADDR + 0x6B8)
#define EMI_LTCT3  (EMI_BASE_ADDR + 0x6BC)
#define EMI_LTST    (EMI_BASE_ADDR + 0x6C0)

#define LT_CMD_THR_VAL 100
extern void emi_md_elm_Init(void);

#endif  /* !__MT_EMI_BW_ELM_H__ */
