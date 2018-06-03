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

#include "audio_dma_buf_control.h"

#include <scp_helper.h>
#include <scp_ipi.h>

#include "audio_log.h"
#include "audio_assert.h"


static audio_resv_dram_t resv_dram;
static audio_resv_dram_t resv_dram_spkprotect;


void init_reserved_dram(void)
{
	resv_dram.phy_addr = (char *)scp_get_reserve_mem_phys(OPENDSP_MEM_ID);
	resv_dram.vir_addr = (char *)scp_get_reserve_mem_virt(OPENDSP_MEM_ID);
	resv_dram.size     = (uint32_t)scp_get_reserve_mem_size(OPENDSP_MEM_ID);

	AUD_LOG_D("resv_dram: pa %p, va %p, sz 0x%x\n",
		  resv_dram.phy_addr, resv_dram.vir_addr, resv_dram.size);

	if (is_scp_ready(SCP_B_ID)) {
		AUD_ASSERT(resv_dram.phy_addr != NULL);
		AUD_ASSERT(resv_dram.vir_addr != NULL);
		AUD_ASSERT(resv_dram.size > 0);
	}

	/*speaker protection*/
	resv_dram_spkprotect.phy_addr = (char *)scp_get_reserve_mem_phys(MP3_MEM_ID);
	resv_dram_spkprotect.vir_addr = (char *)scp_get_reserve_mem_virt(MP3_MEM_ID);
	resv_dram_spkprotect.size     = (uint32_t)scp_get_reserve_mem_size(MP3_MEM_ID);

	AUD_LOG_D("resv_dram: pa %p, va %p, sz 0x%x\n",
		resv_dram_spkprotect.phy_addr, resv_dram_spkprotect.vir_addr, resv_dram_spkprotect.size);

	if (is_scp_ready(SCP_B_ID)) {
		AUD_ASSERT(resv_dram_spkprotect.phy_addr != NULL);
		AUD_ASSERT(resv_dram_spkprotect.vir_addr != NULL);
		AUD_ASSERT(resv_dram_spkprotect.size > 0);
	}
}


audio_resv_dram_t *get_reserved_dram(void)
{
	return &resv_dram;
}

char *get_resv_dram_vir_addr(char *resv_dram_phy_addr)
{
	uint32_t offset = 0;

	offset = resv_dram_phy_addr - resv_dram.phy_addr;
	return resv_dram.vir_addr + offset;
}


audio_resv_dram_t *get_reserved_dram_spkprotect(void)
{
	return &resv_dram_spkprotect;
}

char *get_resv_dram_spkprotect_vir_addr(char *resv_dram_phy_addr)
{
	uint32_t offset = 0;

	offset = resv_dram_phy_addr - resv_dram_spkprotect.phy_addr;
	return resv_dram_spkprotect.vir_addr + offset;
}

