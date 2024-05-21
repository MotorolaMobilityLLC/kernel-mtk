/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
*/
#ifndef __TMP_BTS_CAM_PCB_H__
#define __TMP_BTS_CAM_PCB_H__

/* chip dependent */

#define APPLY_PRECISE_NTC_TABLE
#define APPLY_AUXADC_CALI_DATA
#define APPLY_PRECISE_BTS_TEMP

#define AUX_IN2_NTC (2)
#define AUX_IN3_NTC (3)

#define PCB_RAP_PULL_UP_R		100000 /* 390K, pull up resister */

#define PCB_TAP_OVER_CRITICAL_LOW	4397119 /* base on 100K NTC temp
						 * default value -40 deg
						 */

#define PCB_RAP_PULL_UP_VOLTAGE		1800 /* 1.8V ,pull up voltage */

#define PCB_RAP_NTC_TABLE		7 /* default is NCP15WF104F03RC(100K) */

#define PCB_RAP_ADC_CHANNEL		AUX_IN2_NTC /* default is 0 */

#define CAM_RAP_PULL_UP_R		100000 /* 390K, pull up resister */

#define CAM_TAP_OVER_CRITICAL_LOW	4397119 /* base on 100K NTC temp
						 * default value -40 deg
						 */

#define CAM_RAP_PULL_UP_VOLTAGE	1800 /* 1.8V ,pull up voltage */

#define CAM_RAP_NTC_TABLE		7 /* default is NCP15WF104F03RC(100K) */

#define CAM_RAP_ADC_CHANNEL		AUX_IN3_NTC /* default is 1 */

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);
extern int IMM_IsAdcInitReady(void);

#endif	/* __TMP_BTS_H__ */
