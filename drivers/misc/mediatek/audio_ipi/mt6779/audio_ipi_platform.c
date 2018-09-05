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

#include <audio_ipi_platform.h>

#include <linux/printk.h>
#include <linux/bug.h>


#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
#include <scp_ipi.h>
#endif

#ifdef CONFIG_MTK_AUDIODSP_SUPPORT
#include <adsp_ipi.h>
#endif

#include <audio_task.h>



bool audio_opendsp_ready(const uint8_t task)
{
	uint32_t opendsp_id = audio_get_opendsp_id(task);

	bool ret = false;

	switch (opendsp_id) {
	case AUDIO_OPENDSP_USE_CM4_A:
	case AUDIO_OPENDSP_USE_CM4_B:
#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
		/* avoid cm4_b not build... opendsp_id => scp_core_id!! */
		ret = is_scp_ready((enum scp_core_id)opendsp_id);
#else
		pr_notice("%s(), opendsp_id %u task %d not build!!\n",
			  __func__, opendsp_id, task);
		ret = false;
		WARN_ON(1);
#endif
		break;
	case AUDIO_OPENDSP_USE_HIFI3:
#ifdef CONFIG_MTK_AUDIODSP_SUPPORT
		ret = is_adsp_ready(ADSP_A_ID);
#else
		pr_notice("%s(), opendsp_id %u task %d not build!!\n",
			  __func__, opendsp_id, task);
		ret = false;
		WARN_ON(1);
#endif
		break;
	default:
		pr_notice("%s(), opendsp_id %u task %d not support!!\n",
			  __func__, opendsp_id, task);
		WARN_ON(1);
	}

	return ret;
}


uint32_t audio_get_opendsp_id(const uint8_t task)
{
	uint32_t opendsp_id = AUDIO_OPENDSP_ID_INVALID;

	switch (task) {
	case TASK_SCENE_VOICE_ULTRASOUND:
	case TASK_SCENE_SPEAKER_PROTECTION:
	case TASK_SCENE_VOW:
		opendsp_id = AUDIO_OPENDSP_USE_CM4_A;
		break;
	case TASK_SCENE_PHONE_CALL:
	case TASK_SCENE_PLAYBACK_MP3:
	case TASK_SCENE_RECORD:
	case TASK_SCENE_VOIP:
	case TASK_SCENE_PRIMARY:
	case TASK_SCENE_DEEPBUFFER:
	case TASK_SCENE_AUDPLAYBACK:
	case TASK_SCENE_AUD_SWMIXER1:
	case TASK_SCENE_AUD_SWMIXER2:
	case TASK_SCENE_AUD_DAEMON:
	case TASK_SCENE_AUDIO_CONTROLLER:
		opendsp_id = AUDIO_OPENDSP_USE_HIFI3;
		break;
	default:
		pr_notice("%s(), task %d not support!!\n",
			  __func__, task);
		opendsp_id = AUDIO_OPENDSP_ID_INVALID;
		WARN_ON(1);
	}

	return opendsp_id;
}


uint32_t audio_get_ipi_id(const uint8_t task)
{
	uint32_t opendsp_id = audio_get_opendsp_id(task);
	uint32_t ipi_id = 0xFFFFFFFF;

	switch (opendsp_id) {
	case AUDIO_OPENDSP_USE_CM4_A:
	case AUDIO_OPENDSP_USE_CM4_B:
#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
		ipi_id = IPI_AUDIO;
#else
		pr_notice("%s(), opendsp_id %u task %d not build!!\n",
			  __func__, opendsp_id, task);
		ipi_id = 0xFFFFFFFF;
		WARN_ON(1);
#endif
		break;
	case AUDIO_OPENDSP_USE_HIFI3:
#ifdef CONFIG_MTK_AUDIODSP_SUPPORT
		ipi_id = ADSP_IPI_AUDIO;
#else
		pr_notice("%s(), opendsp_id %u task %d not build!!\n",
			  __func__, opendsp_id, task);
		ipi_id = 0xFFFFFFFF;
		WARN_ON(1);
#endif
		break;
	default:
		pr_notice("%s(), opendsp_id %u task %d not support!!\n",
			  __func__, opendsp_id, task);
		WARN_ON(1);
		ipi_id = 0xFFFFFFFF;
	}

	return ipi_id;
}



