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

#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/tick.h>

#include <mtk_spm_internal.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>
#include <mtk_spm_mcdsr.h>
#include <mtk_spm_mcdsr_internal.h>

#include <mtk_idle.h>
#include <mtk_secure_api.h>

static DEFINE_SPINLOCK(mcdsr_spin_lock);

static unsigned int mcdsr_block_by[MCDSR_BLOCK_NUM] = { 0 };
static unsigned long long mcdsr_tag_cal[MCDSR_TAG_NUM] = { 0 };
static unsigned long long mcdsr_tag_result[MCDSR_TAG_NUM] = { 0 };

static struct mcdsr_status mcdsr = {
	.enable = MCDSR_ENABLE,
	.ddren_req = 0,
	.block = 0,
	.block_profile = 0,
	.expected_us = 0,
	.expected_slp_us = EXPECTED_SLP_US,
	.last_core_counter = 0,
	.mcdsr_tag_sample = 0,
	.mcdsr_tag_cnt = 0,
};

static char *mcdsr_block_name[MCDSR_BLOCK_NUM] = {
	"MCDSR_BLOCK_BY_EXPECTED_SLE",
	"MCDSR_BLOCK_BY_SPM_DRAM_RES",
	"MCDSR_BLOCK_BY_SODI_CG",
};

static char *mcdsr_profile_tags_name[MCDSR_TAG_NUM] = {
	"MCDSR_TAG_CAN_ENTER_START",
	"MCDSR_TAG_CAN_ENTER_END",
	"MCDSR_TAG_DDREN_CHECK_START",
	"MCDSR_TAG_DDREN_CHECK_END",
	"MCDSR_TAG_DDREN_REQUEST_START",
	"MCDSR_TAG_DDREN_REQUEST_END",
};

static inline unsigned long long mcdsr_get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec);
}

static void mcdsr_tag_init(unsigned int time)
{
	int tag = 0;

	for (tag = 0; tag < MCDSR_TAG_NUM; tag++)
		mcdsr_tag_result[tag] = 0;

	mcdsr.mcdsr_tag_sample = time;
	mcdsr.mcdsr_tag_cnt = time;
}

static void mcdsr_tag_profile(int tag)
{
	if (mcdsr.mcdsr_tag_cnt)
		mcdsr_tag_cal[tag] = mcdsr_get_current_time_us();
}

static void mcdsr_tag_time_cnt(void)
{
	unsigned int tag;

	if (mcdsr.mcdsr_tag_cnt > 0) {
		mcdsr.mcdsr_tag_cnt--;

		for (tag = 1; tag < MCDSR_TAG_NUM; tag += 2)
			mcdsr_tag_result[tag] +=
				mcdsr_tag_cal[tag] - mcdsr_tag_cal[tag-1];
	}
}

static void mcdsr_set_enable(bool enable)
{
	unsigned long flags;
	bool request;

	spin_lock_irqsave(&mcdsr_spin_lock, flags);

	/* request to high, then update setting */
	request = enable ? 1 : 1;

	mt_secure_call(MTK_SIP_KERNEL_SPM_ARGS, SPM_ARGS_MCDSR_SET_DDREN_REQ,
				request, 0, 0);

	mt_secure_call(MTK_SIP_KERNEL_SPM_ARGS, SPM_ARGS_MCDSR_SET_ENABLE,
				enable, 0, 0);

	mcdsr.ddren_req = request;
	mcdsr.enable = enable;

	spin_unlock_irqrestore(&mcdsr_spin_lock, flags);
}

bool __mcdsr_get_enable(void)
{
	unsigned long flags;
	bool enable;

	spin_lock_irqsave(&mcdsr_spin_lock, flags);

	enable = mcdsr.enable;

	spin_unlock_irqrestore(&mcdsr_spin_lock, flags);

	return enable;
}

static bool mcdsr_check_ddren_request(void)
{
	struct timespec t;
	bool request;
	int bit;

	t = ktime_to_timespec(tick_nohz_get_sleep_length());
	mcdsr.expected_us = t.tv_sec * USEC_PER_SEC + t.tv_nsec / NSEC_PER_USEC;

	mcdsr.block = 0;

	mcdsr_tag_profile(MCDSR_TAG_DDREN_CHECK_START);

	mtk_idle_cond_update_state();

	if (mcdsr.expected_us < mcdsr.expected_slp_us)
		mcdsr.block |= (1 << MCDSR_BLOCK_BY_EXPECTED_SLE);
	if (spm_get_resource_usage() & SPM_RESOURCE_DRAM)
		mcdsr.block |= (1 << MCDSR_BLOCK_BY_SPM_DRAM_RES);
	if (!mtk_idle_cond_check(IDLE_TYPE_SO))
		mcdsr.block |= (1 << MCDSR_BLOCK_BY_SODI_CG);

	mcdsr_tag_profile(MCDSR_TAG_DDREN_CHECK_END);

	/* counter block by */
	if (mcdsr.block_profile == 1) {
		mcdsr.last_core_counter++;
		for (bit = 0; bit < MCDSR_BLOCK_NUM; bit++) {
			if (mcdsr.block & (1 << bit))
				mcdsr_block_by[bit] += 1;
		}
	}

	request = mcdsr.block ? 1 : 0;

	return request;
}

void __mcdsr_can_enter(void)
{
	unsigned long flags;
	static unsigned int pre_ddren_req = -1;

	if (!mcdsr.enable)
		return;

	spin_lock_irqsave(&mcdsr_spin_lock, flags);

	mcdsr_tag_profile(MCDSR_TAG_CAN_ENTER_START);

	mcdsr.ddren_req = mcdsr_check_ddren_request();

	mcdsr_tag_profile(MCDSR_TAG_DDREN_REQUEST_START);

	if (mcdsr.ddren_req != pre_ddren_req || mcdsr.mcdsr_tag_cnt != 0) {

		pre_ddren_req = mcdsr.ddren_req;

		mt_secure_call(MTK_SIP_KERNEL_SPM_ARGS,
			       SPM_ARGS_MCDSR_SET_DDREN_REQ,
			       mcdsr.ddren_req, 0, 0);
	}

	mcdsr_tag_profile(MCDSR_TAG_DDREN_REQUEST_END);

	mcdsr_tag_profile(MCDSR_TAG_CAN_ENTER_END);

	mcdsr_tag_time_cnt();

	spin_unlock_irqrestore(&mcdsr_spin_lock, flags);
}

/*
 * debugfs (/d/spm/spm_mcdsr_state)
 */
#define NF_CMD_BUF	128
#define LOG_BUF_LEN	4096
static char dbg_buf[LOG_BUF_LEN] = { 0 };

#undef mcdsr_log
#define log2buf(p, s, fmt, args...) \
	(p += scnprintf(p, sizeof(s) - strlen(s), fmt, ##args))
#define mcdsr_log(fmt, args...)	log2buf(p, dbg_buf, fmt, ##args)

ssize_t get_spm_mcdsr_state(char *ToUserBuf, size_t sz, void *priv)
{
	int len = 0, bit = 0, tag = 0;
	char *p = ToUserBuf;
	unsigned int ratio_raw = 0;
	unsigned int ratio_int = 0;
	unsigned int ratio_fraction = 0;
	unsigned int ratio_dur = 0;

	mcdsr_log("MCDSR status:\n");
	mcdsr_log("\tmcdsr.enable          = %d\n", mcdsr.enable);
	mcdsr_log("\tmcdsr.block           = 0x%x\n", mcdsr.block);
	mcdsr_log("\tmcdsr.ddren_req       = %d\n", mcdsr.ddren_req);
	mcdsr_log("\tmcdsr.expected_us     = %d\n", mcdsr.expected_us);
	mcdsr_log("\tmcdsr.expected_slp_us = %d\n", mcdsr.expected_slp_us);
	mcdsr_log("\tSPM_SRC_REQ[7] = 0x%x\n",
		  spm_read(SPM_SRC_REQ) & REG_SPM_DDR_EN_REQ_LSB);
	mcdsr_log("\tSPM_SW_FLAG    = 0x%x\n", spm_read(SPM_SW_FLAG));
	mcdsr_log("\tSPM_SW_FLAG_2  = 0x%x\n",
		  spm_read(SPM_SW_FLAG_2)); /* bit[5]=1 MCDSR disable */
	mcdsr_log("\tSRC_REQ_STA_0  = 0x%x\n", spm_read(SRC_REQ_STA_0));
	mcdsr_log("\tSRC_REQ_STA_1  = 0x%x\n", spm_read(SRC_REQ_STA_1));
	mcdsr_log("\tSRC_REQ_STA_2  = 0x%x\n", spm_read(SRC_REQ_STA_2));
	mcdsr_log("\tSPM_SW_RSV_8   = 0x%x\n",
		  spm_read(SPM_SW_RSV_8)); /* bit[31]=1 LP DDREN cnt en */
	mcdsr_log("\tSPM_SW_RSV_9   = 0x%x\n",
		  spm_read(SPM_SW_RSV_9)); /* bit[31]=1 MCDSR DDREN cnt en */
	mcdsr_log("\n");

	mcdsr_log("MCDSR block profile:\n");
	mcdsr_log("\tmcdsr.block_profile = %d\n", mcdsr.block_profile);
	mcdsr_log("\tmcdsr.last_core_counter = %d\n", mcdsr.last_core_counter);
	for (bit = 0; bit < MCDSR_BLOCK_NUM; bit++) {
		ratio_raw = 100 * mcdsr_block_by[bit];
		ratio_dur = mcdsr.last_core_counter;
		ratio_int = ratio_raw / ratio_dur;
		ratio_fraction = (1000 * (ratio_raw % ratio_dur)) / ratio_dur;

		mcdsr_log("\t[%-27s]: %3u.%03u%% (%d)\n",
			  mcdsr_block_name[bit],
			  ratio_int, ratio_fraction, mcdsr_block_by[bit]);
	}
	mcdsr_log("\n");

	mcdsr_log("MCDSR tag profile:\n");
	mcdsr_log("\tmcdsr.mcdsr_tag_sample = %d\n", mcdsr.mcdsr_tag_sample);
	mcdsr_log("\tmcdsr.mcdsr_tag_cnt = %d\n", mcdsr.mcdsr_tag_cnt);
	for (tag = 1; tag < MCDSR_TAG_NUM; tag += 2) {
		mcdsr_log("\t[%-27s]: %3llu us\n", mcdsr_profile_tags_name[tag],
				(mcdsr_tag_result[tag])/mcdsr.mcdsr_tag_sample);
	}

	len = p - ToUserBuf;

	return len;
}

ssize_t set_spm_mcdsr_state(char *FromUserBuf, size_t sz, void *priv)
{
	char cmd[NF_CMD_BUF];
	unsigned long flags;
	int param;
	int bit = 0;

	if (sscanf(FromUserBuf, "%127s %d", cmd, &param) == 2) {
		if (!strcmp(cmd, "enable")) {
			mcdsr_set_enable(param);
		} else if (!strcmp(cmd, "expected_slp_us")) {
			mcdsr.expected_slp_us = param;
		} else if (!strcmp(cmd, "mcdsr_tag_sample")) {
			mcdsr_tag_init(param);
		} else if (!strcmp(cmd, "block_profile")) {
			spin_lock_irqsave(&mcdsr_spin_lock, flags);

			mcdsr.block_profile = param;

			if (mcdsr.block_profile == 1) {
				mcdsr.last_core_counter = 0;
				for (bit = 0; bit < MCDSR_BLOCK_NUM; bit++)
					mcdsr_block_by[bit] = 0;
			} else {


			}

			spin_unlock_irqrestore(&mcdsr_spin_lock, flags);
		}
	}

	return -EINVAL;
}

