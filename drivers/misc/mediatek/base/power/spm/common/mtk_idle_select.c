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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#include <mt-plat/mtk_meminfo.h> /* dcs_get_dcs_status_trylock/unlock */

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#include <teei_client_main.h> /* is_teei_ready */
#endif

#include <mtk_spm_resource_req.h>
#include <mtk_idle.h>
#include <mtk_idle_internal.h>
#include <mtk_spm_internal.h> /* mtk_idle_cond_update_state */


/* [ByChip] Internal weak functions: implemented in mtk_spm.c */
int __attribute__((weak)) spm_load_firmware_status(void) { return -1; }
/* [ByChip] Internal weak functions: implemented in mtk_idle_cond_check.c */
void __attribute__((weak)) mtk_idle_cond_update_state(void) {}


static bool mtk_idle_can_enter(int idle_type, int reason)
{
	if (idle_type == IDLE_TYPE_DP)
		return dpidle_can_enter(reason);
	else if (idle_type == IDLE_TYPE_SO3)
		return sodi3_can_enter(reason);
	else if (idle_type == IDLE_TYPE_SO)
		return sodi_can_enter(reason);
	/* always can enter rgidle */
	return true;
}

int mtk_idle_select(int cpu)
{
	int idx;
	int reason = NR_REASONS;
	#if defined(CONFIG_MTK_UFS_BOOTING)
	unsigned long flags = 0;
	unsigned int ufs_locked;
	#endif
	#if defined(CONFIG_MTK_DCS)
	int ch = 0, ret = -1;
	enum dcs_status dcs_status;
	bool dcs_lock_get = false;
	#endif

	__profile_idle_start(IDLE_TYPE_DP, PIDX_SELECT_TO_ENTER);
	__profile_idle_start(IDLE_TYPE_SO3, PIDX_SELECT_TO_ENTER);
	__profile_idle_start(IDLE_TYPE_SO, PIDX_SELECT_TO_ENTER);

	/* 1. spmfw firmware is loaded ? */
	#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* return -1:not init, 0:not loaded, 1: loaded */
	if (spm_load_firmware_status() < 1) {
		reason = BY_FRM;
		goto get_idle_idx;
	}
	#endif

	/* 2. spm resources are all used currently ? */
	if (spm_get_resource_usage() == SPM_RESOURCE_ALL) {
		reason = BY_SRR;
		goto get_idle_idx;
	}

	/* 3. locked by ufs ? */
	#if defined(CONFIG_MTK_UFS_BOOTING)
	spin_lock_irqsave(&idle_ufs_spin_lock, flags);
	ufs_locked = idle_ufs_lock;
	spin_unlock_irqrestore(&idle_ufs_spin_lock, flags);

	if (ufs_locked) {
		reason = BY_UFS;
		goto get_idle_idx;
	}
	#endif

	/* 4. tee is ready ? */
	#if !defined(CONFIG_FPGA_EARLY_PORTING) && \
		defined(CONFIG_MICROTRUST_TEE_SUPPORT)
	if (!is_teei_ready()) {
		reason = BY_TEE;
		goto get_idle_idx;
	}
	#endif

	/* Update current idle condition state for later check */
	mtk_idle_cond_update_state();

	/* 5. is dcs channel switching ? */
	#if defined(CONFIG_MTK_DCS)
	/* check if DCS channel switching */
	ret = dcs_get_dcs_status_trylock(&ch, &dcs_status);
	if (ret) {
		reason = BY_DCS;
		goto get_idle_idx;
	}

	dcs_lock_get = true;
	#endif

get_idle_idx:
	for (idx = 0 ; idx < NR_IDLE_TYPES; idx++)
		if (mtk_idle_can_enter(idx, reason))
			break;
	idx = idx < NR_IDLE_TYPES ? idx : -1;

	#if defined(CONFIG_MTK_DCS)
	if (dcs_lock_get)
		dcs_get_dcs_status_unlock();
	#endif

	return idx;
}

