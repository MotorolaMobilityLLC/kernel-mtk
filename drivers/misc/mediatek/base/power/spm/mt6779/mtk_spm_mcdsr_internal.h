/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __MTK_SPM_MCDSR_INTERNAL_H__
#define __MTK_SPM_MCDSR_INTERNAL_H__

#define MCDSR_TAG       "[MCDSR] "
#define mcdsr_dbg(fmt, args...)        pr_info(MCDSR_TAG fmt, ##args)

#define MCDSR_ENABLE      0
#define EXPECTED_SLP_US   0

struct mcdsr_status {
	bool enable;
	bool ddren_req;
	unsigned int block;
	bool block_profile;
	unsigned int expected_us;
	unsigned int expected_slp_us;
	unsigned int last_core_counter;
	unsigned int mcdsr_tag_sample;
	unsigned int mcdsr_tag_cnt;
};

enum {
	MCDSR_BLOCK_BY_EXPECTED_SLE = 0,
	MCDSR_BLOCK_BY_SPM_DRAM_RES,
	MCDSR_BLOCK_BY_SODI_CG,
	MCDSR_BLOCK_NUM,
};

enum {
	MCDSR_TAG_CAN_ENTER_START = 0,
	MCDSR_TAG_CAN_ENTER_END,
	MCDSR_TAG_DDREN_CHECK_START,
	MCDSR_TAG_DDREN_CHECK_END,
	MCDSR_TAG_DDREN_REQUEST_START,
	MCDSR_TAG_DDREN_REQUEST_END,
	MCDSR_TAG_NUM,
};

extern bool __mcdsr_get_enable(void);
extern void __mcdsr_can_enter(void);

extern ssize_t get_spm_mcdsr_state(char *ToUserBuf, size_t sz, void *priv);
extern ssize_t set_spm_mcdsr_state(char *FromUserBuf, size_t sz, void *priv);

#endif /* __MTK_SPM_MCDSR_INTERNAL_H__ */
