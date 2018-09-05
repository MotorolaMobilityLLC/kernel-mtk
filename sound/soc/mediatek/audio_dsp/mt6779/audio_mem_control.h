/*
 * Copyright (C) 2018 MediaTek Inc.
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
/*
 * audio_mem_control.h --  Mediatek ADSP dmemory control
 *
 * Copyright (c) 2018 MediaTek Inc.
 * Author: Chipeng <Chipeng.chang@mediatek.com>
 */


#ifndef AUDIO_MEM_CONTROL_H
#define AUDIO_MEM_CONTROL_H

#include <linux/genalloc.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>

#include "../mtk-base-dsp.h"

/* clk relate */
#include "../../mt3967/mt3967-afe-clk.h"


struct audio_dsp_dram;
struct gen_pool;
struct mtk_base_dsp;
struct mtk_base_dsp_mem;

enum {
	ADSP_TASK_ATOD_MSG_MEM,
	ADSP_TASK_DTOA_MSG_MEM,
	ADSP_TASK_RINGBUF_MEM,
	ADSP_TASK_HWBUF_MEM,
	ADSP_TASK_SHAREMEM_NUM
};

/* first time to inint scp dram segment */
void init_mtk_adsp_dram_segment(void);

int get_mtk_adsp_dram(struct audio_dsp_dram *dsp_dram, int id);

/* dump dsp reserved dram stats */
void dump_mtk_adsp_dram(struct audio_dsp_dram buffer);
void dump_all_adsp_dram(void);

/* gen pool create */
int mtk_adsp_gen_pool_create(int min_alloc_order, int nid);
struct gen_pool *mtk_get_adsp_dram_gen_pool(int id);

void mtk_dump_sndbuffer(struct snd_dma_buffer *dma_audio_buffer);
int wrap_dspdram_sndbuffer(struct snd_dma_buffer *dma_audio_buffer,
			   struct audio_dsp_dram *dsp_dram_buffer);

int scp_reservedid_to_dsp_daiid(int id);
int dsp_daiid_to_scp_reservedid(int task_dai_id);
int get_taskid_by_afe_daiid(int afe_dai_id);
int get_afememdl_by_afe_taskid(int task_dai_id);
int get_afememul_by_afe_taskid(int task_dai_id);
int get_afememref_by_afe_taskid(int task_dai_id);
int get_featureid_by_dsp_daiid(int dspid);


int dump_mtk_adsp_gen_pool(void);

int mtk_adsp_genpool_allocate_sharemem_ring(struct mtk_base_dsp_mem *dsp_mem,
					    unsigned int size, int id);
int mtk_adsp_genpool_free_sharemem_ring(struct mtk_base_dsp_mem *dsp_mem,
					int id);

int mtk_adsp_genpool_allocate_sharemem_msg(struct mtk_base_dsp_mem *dsp_mem,
					  int size,
					  int id);
int mtk_init_adsp_msg_sharemem(struct audio_dsp_dram *msg_atod_share_buf,
				unsigned long vaddr, unsigned long long paddr,
				int size);

/* get struct of sharemem_block */
struct audio_dsp_dram *mtk_get_adsp_sharemem_block(int audio_task_id);
unsigned int mtk_get_adsp_sharemem_size(int audio_task_id,
					int task_sharemem_id);

/* init dsp share memory */
int mtk_init_adsp_audio_share_mem(struct mtk_base_dsp *dsp);

int dsp_dram_request(struct device *dev);
int dsp_dram_release(struct device *dev);

#endif /* end of AUDIO_MEM_CONTROL_H */
