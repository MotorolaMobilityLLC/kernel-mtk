// SPDX-License-Identifier: GPL-2.0
//
// mtk-scp-vow-common.c  --
//
// Copyright (c) 2019 MediaTek Inc.
// Author: Poyen <Poyen.wu@mediatek.com>



#include <linux/io.h>
#include "mtk-sram-manager.h"
#include "mtk-base-afe.h"
#include "mtk-afe-fe-dai.h"
#if IS_ENABLED(CONFIG_MTK_TINYSYS_SCP_SUPPORT)
#include "scp.h"
#endif
#include "mtk-scp-vow-common.h"
#include "mtk-scp-vow-platform.h"


int allocate_vow_bargein_mem(struct snd_pcm_substream *substream,
			     dma_addr_t *phys_addr,
			     unsigned char **virt_addr,
			     unsigned int size,
			     snd_pcm_format_t format,
			     struct mtk_base_afe *afe)
{
	struct mtk_base_afe_memif *memif;
	int id;
	int ret = 0;
	unsigned char *area;	/* virtual pointer */
	dma_addr_t addr;	/* physical address */
	size_t bytes = size;

	id = get_scp_vow_memif_id();
	memif = &afe->memif[id];

	if (mtk_audio_sram_allocate(afe->sram,
				    &addr,
				    &area,
				    bytes,
				    substream,
				    format, false) == 0) {
		/* Using SRAM */
		dev_info(afe->dev, "%s(), use SRAM\n", __func__);
		memif->using_sram = 1;
	} else {
		/* Using DRAM */
		dev_info(afe->dev, "%s(), use DRAM\n", __func__);
#if IS_ENABLED(CONFIG_MTK_TINYSYS_SCP_SUPPORT)
		addr = scp_get_reserve_mem_phys(VOW_BARGEIN_MEM_ID);
		area =
			(uint8_t *)scp_get_reserve_mem_virt(VOW_BARGEIN_MEM_ID);
		memif->using_sram = 0;
#else
		dev_info(afe->dev, "%s(), scp not supported, scp_get_reserve_mem failed.\n",
			__func__);
		return -EINVAL;
#endif

	}

	memset_io(area, 0, bytes);
	ret = mtk_memif_set_addr(afe, id, area, addr, bytes);
	if (ret) {
		dev_info(afe->dev, "%s(), error, set addr, ret %d\n",
			 __func__, ret);
		return ret;
	}

	dev_info(afe->dev, "%s(), addr = %pad, area = %p, bytes = %zu\n",
		 __func__, &addr, area, bytes);

	if ((memif->using_sram == 0) && (afe->request_dram_resource))
		afe->request_dram_resource(afe->dev);

	*phys_addr = addr;
	*virt_addr = area;
	return ret;
}

int get_scp_vow_memif_id(void)
{
	return get_scp_vow_memif_platform_id();
}
