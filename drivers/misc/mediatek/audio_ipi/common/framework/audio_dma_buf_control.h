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

#ifndef AUDIO_DMA_BUF_CONTROL_H
#define AUDIO_DMA_BUF_CONTROL_H

#include <linux/types.h>

typedef struct {
	char    *phy_addr;
	char    *vir_addr;
	uint32_t size;
} audio_resv_dram_t;


void init_reserved_dram(void);
audio_resv_dram_t *get_reserved_dram(void);

char *get_resv_dram_vir_addr(char *resv_dram_phy_addr);


#endif /* end of AUDIO_DMA_BUF_CONTROL_H */

