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

#ifndef __ADSP_FEATURE_DEFINE_H__
#define __ADSP_FEATURE_DEFINE_H__

/* reset recovery feature kernel option*/
//#define CFG_RECOVERY_SUPPORT

/* adsp platform configs*/
#define ADSP_BOOT_TIME_OUT_MONITOR       (1)
#define ADSP_LOGGER_ENABLE               (1)
#define ADSP_VCORE_TEST_ENABLE           (0)
#define ADSP_DVFS_ENABLE                 (0)
#define ADSP_DVFS_INIT_ENABLE            (0)
#define ADSP_TRAX                        (0)

/* adsp aed definition*/
#define ADSP_AED_STR_LEN                (1024)

/* emi mpu define*/
#ifdef CONFIG_MTK_EMI
#define ENABLE_ADSP_EMI_PROTECTION       (1)
#else
#define ENABLE_ADSP_EMI_PROTECTION       (0)
#endif
#define MPU_REGION_ID_ADSP_SMEM          (28)

/* adsp feature ID list */
enum adsp_feature_id {
	SYSTEM_FEATURE_ID             = 0,
	ADSP_LOGGER_FEATURE_ID        = 1,
	AURISYS_FEATURE_ID            = 10,
	AUDIO_CONTROLLER_FEATURE_ID,
	AUDIO_DUMP_FEATURE_ID         = 20,
	PRIMARY_FEATURE_ID,
	DEEPBUF_FEATURE_ID,
	OFFLOAD_FEATURE_ID,
	AUDIO_PLAYBACK_FEATURE_ID,
	EFFECT_HIGH_FEATURE_ID,
	EFFECT_MEDIUM_FEATURE_ID,
	EFFECT_LOW_FEATURE_ID,
	A2DP_PLAYBACK_FEATURE_ID,
	SPK_PROTECT_FEATURE_ID,
	VOICE_CALL_FEATURE_ID,
	VOIP_FEATURE_ID,
	CAPTURE_UL1_FEATURE_ID,
	ADSP_NUM_FEATURE_ID,
};

struct adsp_feature_tb {
	const char *name;
	uint32_t freq;
	int32_t counter;
};

ssize_t adsp_dump_feature_state(char *buffer, int size);
bool adsp_feature_is_active(void);
int adsp_register_feature(enum adsp_feature_id id);
int adsp_deregister_feature(enum adsp_feature_id id);
int adsp_get_feature_index(char *str);

#endif
