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

#ifndef ECCCI_INTERNAL_OPTION
#define ECCCI_INTERNAL_OPTION
/*================================================ */
/*Bool option part*/
/*================================================*/
/*#define CCCI_STATISTIC*/
#define FEATURE_GET_MD_GPIO_NUM
#define FEATURE_GET_MD_GPIO_VAL
#define FEATURE_GET_MD_ADC_NUM
#define FEATURE_GET_MD_ADC_VAL
#define FEATURE_GET_MD_EINT_ATTR
#if defined(FEATURE_GET_MD_EINT_ATTR)
#define FEATURE_GET_MD_EINT_ATTR_DTS
#endif

#define FEATURE_GET_MD_BAT_VOL
#define FEATURE_PM_IPO_H
/*#define FEATURE_DFO_EN*/
#define FEATURE_SEQ_CHECK_EN
#define FEATURE_POLL_MD_EN
#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
#define FEATURE_SCP_CCCI_SUPPORT
#endif
#define FEATURE_DIRECT_TETHERING_LOGGING
#define FEATURE_DHL_CCB_RAW_SUPPORT
#define FEATURE_AUDIO_RAW_SUPPORT
#define FEATURE_SMART_LOGGING
#define FEATURE_MD1MD3_SHARE_MEM

#if 0 /*DEPRECATED */
#define FEATURE_GET_TD_EINT_NUM
#define FEATURE_GET_DRAM_TYPE_CLK
#endif

#define ENABLE_DRAM_API
#define ENABLE_MEM_REMAP_HW
/*#define ENABLE_CHIP_VER_CHECK*/
/*#define ENABLE_2G_3G_CHECK*/
/*#define ENABLE_MD_WDT_DBG*/
#define ENABLE_CLDMA_AP_SIDE
#define ENABLE_MD_POWER_OFF_CHECK

#ifdef CONFIG_MTK_CONN_MD
#define FEATURE_CONN_MD_EXP_EN
#endif
#define FEATURE_USING_4G_MEMORY_API
#define FEATURE_VLTE_SUPPORT
/*#define FEATURE_LOW_BATTERY_SUPPORT disable for customer complaint*/
#ifdef CONFIG_MTK_FPGA
#define FEATURE_FPGA_PORTING
#else
#define FEATURE_RF_CLK_BUF
#define ENABLE_32K_CLK_LESS
#define FEATURE_MD_GET_CLIB_TIME
#define FEATURE_C2K_ALWAYS_ON
#define FEATURE_DBM_SUPPORT

/*#define ENABLE_EMI_PROTECTION*/
#endif
/* #define DISABLE_MD_WDT_PROCESS */ /* enable wdt after bringup */
#define NO_POWER_OFF_ON_STARTMD
#define NO_START_ON_SUSPEND_RESUME
#define MD_UMOLY_EE_SUPPORT
#define TEST_MESSAGE_FOR_BRINGUP
#define MD_EE_V3_SUPPORT

/* #define FEATURE_MTK_SWITCH_TX_POWER */
#ifdef FEATURE_MTK_SWITCH_TX_POWER
#define SWTP_COMPATIBLE_DEVICE_ID "mediatek, swtp-eint"
#endif

#define FEATURE_FORCE_ASSERT_CHECK_EN
#define FEATURE_CLK_CG_CONTROL
#define ENABLE_FAST_HEADER

/*================================================ */
/* misc size description */
#define CCCI_SMEM_DUMP_SIZE      4096 /* smem size we dump when EE */
#define CCCC_SMEM_CCIF_SRAM_SIZE 36 /* SRAM size we dump into smem */
#define CCCI_SMEM_SLEEP_MODE_DBG_DUMP 512 /* only dump first 512bytes in sleep mode info */
#define CCCI_SMEM_DBM_GUARD_SIZE 8
#define CCCI_SMEM_DBM_SIZE 160
#define CCCI_SMEM_CCISM_DUMP_SIZE (20*1024)
#define CCCI_SMEM_SIZE_RUNTIME_AP 0x800 /* AP runtime data size */
#define CCCI_SMEM_SIZE_RUNTIME_MD 0x800 /* MD runtime data size */
/*================================================*/
/* share memory region description */
#define CCCI_SMEM_OFFSET_EXCEPTION 0 /* offset in whole share memory region */
#define CCCI_SMEM_SIZE_EXCEPTION (64*1024) /* exception smem total size */
/*== subset of exception region begains ==*/
/* CCCI debug 2k ++++ MD sys debug 8k for v2, 10k for v3, 2k for other */
#define CCCI_SMEM_OFFSET_CCCI_DEBUG CCCI_SMEM_OFFSET_EXCEPTION /* MD CCCI debug info */
#define CCCI_SMEM_SIZE_CCCI_DEBUG 2048
#define CCCI_SMEM_OFFSET_MDSS_DEBUG (CCCI_SMEM_OFFSET_EXCEPTION+CCCI_SMEM_SIZE_CCCI_DEBUG) /* MD SS debug info */
#define CCCI_SMEM_SIZE_MDSS_DEBUG_UMOLY  8192
#define CCCI_SMEM_SIZE_MDSS_DEBUG_V3  (10*1024)
#define CCCI_SMEM_SIZE_MDSS_DEBUG 2048

#define CCCI_SMEM_OFFSET_SEQERR (CCCI_SMEM_OFFSET_CCCI_DEBUG+0x34)
#ifdef MD_EE_V3_SUPPORT
#define CCCI_SMEM_OFFSET_CCIF_SRAM (CCCI_SMEM_OFFSET_MDSS_DEBUG+0xF24)
#else
#define CCCI_SMEM_OFFSET_CCIF_SRAM (CCCI_SMEM_OFFSET_MDSS_DEBUG+1024-CCCC_SMEM_CCIF_SRAM_SIZE)
#endif
#define CCCI_SMEM_OFFSET_EPON (CCCI_SMEM_OFFSET_EXCEPTION+0xC64)
#define CCCI_SMEM_OFFSET_EPON_UMOLY (CCCI_SMEM_OFFSET_EXCEPTION+0x1830)
#define CCCI_SMEM_OFFSET_EPON_V3  (0x1CF4)
#ifdef ENABLE_MD_POWER_OFF_CHECK
#ifdef MD_EE_V3_SUPPORT
#define CCCI_SMEM_OFFSET_EPOF  (CCCI_SMEM_OFFSET_MDSS_DEBUG+7*1024+0x234)
#else
#define CCCI_SMEM_OFFSET_EPOF  (CCCI_SMEM_OFFSET_EXCEPTION+8*1024+31*4)
#endif
#endif
/* sleep mode debug at tail of 64k */
#define CCCI_SMEM_SIZE_SLEEP_MODE_DBG 1024 /* MD sleep mode debug info section in exception region tail */
#define CCCI_SMEM_OFFSET_SLEEP_MODE_DBG (CCCI_SMEM_OFFSET_EXCEPTION+CCCI_SMEM_SIZE_EXCEPTION \
										-CCCI_SMEM_SIZE_SLEEP_MODE_DBG)

#ifdef FEATURE_FORCE_ASSERT_CHECK_EN
/* MD CCCI force assert debug info */
#define CCCI_SMEM_FORCE_ASSERT_SIZE 1024
#define CCCI_SMEM_OFFSET_FORCE_ASSERT (CCCI_SMEM_OFFSET_EXCEPTION + CCCI_SMEM_SIZE_EXCEPTION \
						- CCCI_SMEM_FORCE_ASSERT_SIZE - CCCI_SMEM_SIZE_SLEEP_MODE_DBG)
#endif
#define CCCI_SMEM_OFFSET_DBM_DEBUG (CCCI_SMEM_OFFSET_EXCEPTION + CCCI_SMEM_SIZE_EXCEPTION \
						- CCCI_SMEM_DBM_SIZE - CCCI_SMEM_DBM_GUARD_SIZE*2)

#define CCCI_SMEM_SIZE_RUNTIME	(CCCI_SMEM_SIZE_RUNTIME_AP + CCCI_SMEM_SIZE_RUNTIME_MD)
#define CCCI_SMEM_OFFSET_RUNTIME (CCCI_SMEM_OFFSET_EXCEPTION + CCCI_SMEM_SIZE_EXCEPTION \
		- CCCI_SMEM_SIZE_RUNTIME - CCCI_SMEM_FORCE_ASSERT_SIZE - CCCI_SMEM_SIZE_SLEEP_MODE_DBG)

/*== subset of exception region ends ==*/
#define CCCI_SMEM_OFFSET_CCISM (CCCI_SMEM_OFFSET_EXCEPTION + CCCI_SMEM_SIZE_EXCEPTION) /* SCP<->MD1/3 */
#define CCCI_SMEM_SIZE_CCISM (32*1024)
#define CCCI_SMEM_OFFSET_CCB_CTRL (CCCI_SMEM_OFFSET_CCISM+CCCI_SMEM_SIZE_CCISM)
#define CCCI_SMEM_SIZE_CCB_CTRL (4096*4) /* round up to 4K alignment, mmap requirments, 0x425000 */
#define CCCI_SMEM_OFFSET_DT_NETD (CCCI_SMEM_OFFSET_CCB_CTRL + CCCI_SMEM_SIZE_CCB_CTRL)
#define CCCI_SMEM_SIZE_DT_NETD 4096
#define CCCI_SMEM_OFFSET_DT_USB (CCCI_SMEM_OFFSET_DT_NETD + CCCI_SMEM_SIZE_DT_NETD)
#define CCCI_SMEM_SIZE_DT_USB 4096
#define CCCI_SMEM_OFFSET_RAW_AUDIO_MD1 (CCCI_SMEM_OFFSET_DT_USB+CCCI_SMEM_SIZE_DT_USB)
#define CCCI_SMEM_SIZE_RAW_AUDIO (20*1024)

#define CCCI_SMEM_OFFSET_SMART_LOGGING (CCCI_SMEM_OFFSET_RAW_AUDIO_MD1+CCCI_SMEM_SIZE_RAW_AUDIO) /*APM->MD1*/
#define CCCI_SMEM_SIZE_SMART_LOGGING 0 /* variable, so it should be the last region for MD1 */

#define CCCI_SMEM_OFFSET_RAW_AUDIO_MD3 (CCCI_SMEM_OFFSET_CCISM+CCCI_SMEM_SIZE_CCISM)
#define CCCI_SMEM_OFFSET_CCIF_SMEM (CCCI_SMEM_OFFSET_RAW_AUDIO_MD3+CCCI_SMEM_SIZE_RAW_AUDIO) /*AP<->MD3*/
#define CCCI_SMEM_SIZE_CCIF_SMEM 0 /* variable, so it should be the last region for MD3 */

#define CCCI_SMEM_SIZE_RAW_DHL (16*1024*1024)
/*================================================ */

/*================================================ */
/*Configure value option part*/
/*================================================*/
#define md_protol "6292"
#define AP_PLATFORM_INFO    "MT6799E1"
#define CCCI_MTU            (3584-128)
#define CCCI_NET_MTU        (1500)
#define SKB_POOL_SIZE_4K    (256)	/*2*MD */
#define SKB_POOL_SIZE_1_5K  (256)	/*2*MD */
#define SKB_POOL_SIZE_16    (64)	/*2*MD */
#define BM_POOL_SIZE        (SKB_POOL_SIZE_4K+SKB_POOL_SIZE_1_5K+SKB_POOL_SIZE_16)
#define RELOAD_TH            3	/*reload pool if pool size dropped below 1/RELOAD_TH */
#define MD_HEADER_VER_NO    (3)
#define MEM_LAY_OUT_VER     (1)
#define AP_MD_HS_V2          2	/*handshake version*/

#define CURR_SEC_CCCI_SYNC_VER (1)	/*Note: must sync with sec lib, if ccci and sec has dependency change */
#define CCCI_DRIVER_VER     0x20110118

#define IPC_L4C_MSG_ID_LEN   (0x40)

#define CCCI_EE_HS_POLLING_TIME (1000)

#define CCCI_LOG_LEVEL 4
#endif
