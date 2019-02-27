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

#ifndef _DDP_MISC_H_
#define _DDP_MISC_H_

#define SMI_LARB_ULTRA_DIS 0x70
#define SMI_LARB_PREULTRA_DIS 0x74
#define SMI_LARB_FORCE_ULTRA 0x78
#define SMI_LARB_FORCE_PREULTRA 0x7C

void enable_smi_ultra(unsigned int larb, unsigned int value);
void enable_smi_preultra(unsigned int larb, unsigned int value);
void disable_smi_ultra(unsigned int larb, unsigned int value);
void disable_smi_preultra(unsigned int larb, unsigned int value);

#endif
