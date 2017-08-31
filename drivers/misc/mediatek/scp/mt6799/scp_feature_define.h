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

#ifndef __SCP_FEATURE_DEFINE_H__
#define __SCP_FEATURE_DEFINE_H__

/* reset recovery feature kernel option
* #define CFG_RECOVERY_SUPPORT
*/

/* scp platform configs*/
#define SCP_BOOT_TIME_OUT_MONITOR       (0)
#define SCP_LOGGER_ENABLE               (1)
#define SCP_VCORE_TEST_ENABLE		(1)

/* scp aed definition*/
#define SCP_AED_STR_LEN		(512)

/* scp logger size definition*/
#define LOG_TO_AP_UART_LINE 64
#define SCP_FIFO_SIZE 2048

/* emi mpu define*/
#define ENABLE_SCP_EMI_PROTECTION       (0)
#define MPU_REGION_ID_SCP_SMEM       6

/* scp feature ID list */
typedef enum {
	VOW_FEATURE_ID		= 0,
	OPEN_DSP_FEATURE_ID	= 1,
	SENS_FEATURE_ID		= 2,
	MP3_FEATURE_ID		= 3,
	FLP_FEATURE_ID		= 4,
	RTOS_FEATURE_ID		= 5,
#if SCP_VCORE_TEST_ENABLE
	VCORE_TEST_FEATURE_ID   = 6,
	VCORE_TEST2_FEATURE_ID  = 7,
	VCORE_TEST3_FEATURE_ID  = 8,
	VCORE_TEST4_FEATURE_ID  = 9,
	VCORE_TEST5_FEATURE_ID  = 10,
	VCORE_TEST6_FEATURE_ID   = 11,
	VCORE_TEST7_FEATURE_ID  = 12,
	VCORE_TEST8_FEATURE_ID  = 13,
	VCORE_TEST9_FEATURE_ID  = 14,
	VCORE_TEST10_FEATURE_ID  = 15,
	NUM_FEATURE_ID		= 16,
#else
	NUM_FEATURE_ID		= 6,
#endif
} feature_id_t;

typedef struct scp_feature_tb {
	uint32_t feature;
	uint32_t freq;
	uint32_t core;
	uint32_t enable;
} scp_feature_tb_t;

extern scp_feature_tb_t feature_table[NUM_FEATURE_ID];
extern void scp_register_feature(feature_id_t id);
extern void scp_deregister_feature(feature_id_t id);

#endif
