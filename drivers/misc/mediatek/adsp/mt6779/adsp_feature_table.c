/*
 * Copyright (C) 2011-2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>       /* needed by all modules */
#include "adsp_feature_define.h"
#include "adsp_dvfs.h"

#define ADSP_SYSTEM_UNIT(fname) {.name = fname, .freq = 0, .counter = 1}
#define ADSP_FEATURE_UNIT(fname) {.name = fname, .freq = 0, .counter = 0}

static DEFINE_MUTEX(adsp_feature_mutex);

/*adsp feature list*/
struct adsp_feature_tb adsp_feature_table[ADSP_NUM_FEATURE_ID] = {
	[SYSTEM_FEATURE_ID]           = ADSP_SYSTEM_UNIT("system"),
	[ADSP_LOGGER_FEATURE_ID]      = ADSP_FEATURE_UNIT("logger"),
	[AURISYS_FEATURE_ID]          = ADSP_FEATURE_UNIT("aurisys"),
	[AUDIO_CONTROLLER_FEATURE_ID] = ADSP_FEATURE_UNIT("audio_controller"),
	[AUDIO_DUMP_FEATURE_ID]       = ADSP_FEATURE_UNIT("audio_dump"),
	[PRIMARY_FEATURE_ID]          = ADSP_FEATURE_UNIT("primary"),
	[DEEPBUF_FEATURE_ID]          = ADSP_FEATURE_UNIT("deepbuf"),
	[OFFLOAD_FEATURE_ID]          = ADSP_FEATURE_UNIT("offload"),
	[AUDIO_PLAYBACK_FEATURE_ID]   = ADSP_FEATURE_UNIT("audplayback"),
	[EFFECT_HIGH_FEATURE_ID]      = ADSP_FEATURE_UNIT("effect_high"),
	[EFFECT_MEDIUM_FEATURE_ID]    = ADSP_FEATURE_UNIT("effect_medium"),
	[EFFECT_LOW_FEATURE_ID]       = ADSP_FEATURE_UNIT("effect_low"),
	[A2DP_PLAYBACK_FEATURE_ID]    = ADSP_FEATURE_UNIT("a2dp_playback"),
	[SPK_PROTECT_FEATURE_ID]      = ADSP_FEATURE_UNIT("spk_protect"),
	[VOICE_CALL_FEATURE_ID]       = ADSP_FEATURE_UNIT("voice_call"),
	[VOIP_FEATURE_ID]             = ADSP_FEATURE_UNIT("voip"),
	[CAPTURE_UL1_FEATURE_ID]      = ADSP_FEATURE_UNIT("capture_ul1"),
};

ssize_t adsp_dump_feature_state(char *buffer, int size)
{
	int n = 0, i = 0;
	struct adsp_feature_tb *unit;

	n += scnprintf(buffer + n, size - n, "%-20s %-8s %-8s\n",
		       "Feature_name", "Freq", "Counter");
	for (i = 0; i < ADSP_NUM_FEATURE_ID; i++) {
		unit = &adsp_feature_table[i];
		if (!unit->name)
			continue;
		n += scnprintf(buffer + n, size - n, "%-20s %-8d %-3d\n",
			unit->name, unit->freq, unit->counter);
	}
	return n;
}

int adsp_get_feature_index(char *str)
{
	int i = 0;
	struct adsp_feature_tb *unit;

	if (!str)
		return -EINVAL;

	for (i = 0; i < ADSP_NUM_FEATURE_ID; i++) {
		unit = &adsp_feature_table[i];
		if (!unit->name)
			continue;
		if (strncmp(unit->name, str, strlen(unit->name)) == 0)
			break;
	}

	return i == ADSP_NUM_FEATURE_ID ? -EINVAL : i;
}

bool adsp_feature_is_active(void)
{
	uint32_t fid;

	/* not include system feature */
	for (fid = 0; fid < ADSP_NUM_FEATURE_ID ; fid++) {
		if (adsp_feature_table[fid].counter > 0
			&& fid != SYSTEM_FEATURE_ID)
			break;
	}
	if (fid != ADSP_NUM_FEATURE_ID)
		return true;
	else
		return false;
}

int adsp_register_feature(enum adsp_feature_id id)
{
	int ret = 0;
#ifdef ADSP_DVFS_PROFILE
	ktime_t begin, end;

	begin = ktime_get();
#endif
	if (id >= ADSP_NUM_FEATURE_ID)
		return -EINVAL;

	if (!adsp_feature_table[id].name)
		return -EINVAL;

	pr_debug("[%s]%s, adsp_active=%d, adsp_ready=%x\n", __func__,
		 adsp_feature_table[id].name, adsp_feature_is_active(),
		 is_adsp_ready(ADSP_A_ID));

	ret = adsp_resume();
	if (ret < 0)
		return ret;

	mutex_lock(&adsp_feature_mutex);
	if (!adsp_feature_is_active())
		adsp_stop_suspend_timer();

	adsp_feature_table[id].counter += 1;
	mutex_unlock(&adsp_feature_mutex);
#ifdef ADSP_DVFS_PROFILE
	end = ktime_get();
	pr_debug("[%s]latency = %lld us\n",
		 __func__, ktime_us_delta(end, begin));
#endif
	return 0;
}

int adsp_deregister_feature(enum adsp_feature_id id)
{
	bool adsp_A_ready = is_adsp_ready(ADSP_A_ID);

	if (id >= ADSP_NUM_FEATURE_ID)
		return -EINVAL;

	if (!adsp_feature_table[id].name)
		return -EINVAL;

	pr_debug("[%s]%s, adsp_active=%d, adsp_ready=%x\n", __func__,
		 adsp_feature_table[id].name, adsp_feature_is_active(),
		 adsp_A_ready);

	if (!adsp_A_ready) {
		pr_warn("adsp deregister feature failed. Adsp is not ready\n");
		return -EALREADY;
	}

	mutex_lock(&adsp_feature_mutex);
	if (adsp_feature_table[id].counter == 0) {
		pr_err("[%s] error to deregister id=%d\n", __func__, id);
		WARN_ON(1);
		mutex_unlock(&adsp_feature_mutex);
		return -EINVAL;
	}

	adsp_feature_table[id].counter -= 1;

	/* no feature registered, delay 1s and then suspend adsp. */
	if (!adsp_feature_is_active())
		adsp_start_suspend_timer();
	mutex_unlock(&adsp_feature_mutex);

	return 0;
}

