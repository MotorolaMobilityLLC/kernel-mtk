/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtk-sp-common.h  --  Mediatek Smart Phone Common
 *
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Kai Chieh Chuang <kaichieh.chuang@mediatek.com>
 */

#ifndef _MTK_SP_COMMON_H_
#define _MTK_SP_COMMON_H_

#if defined(CONFIG_MTK_AEE_FEATURE)
#include <mt-plat/aee.h>

#define AUDIO_AEE(message) \
	(aee_kernel_exception_api(__FILE__, \
				  __LINE__, \
				  DB_OPT_FTRACE, message, \
				  "audio assert"))
#else
#define AUDIO_AEE(message) WARN_ON(true)
#endif

/* Declare weak function due to not all platform has definition */
extern int scp_get_semaphore_3way(int flag) __attribute__((weak));
extern int scp_release_semaphore_3way(int flag) __attribute__((weak));

#if defined(CONFIG_MTK_AUDIODSP_SUPPORT) && \
	defined(CONFIG_MTK_TINYSYS_SCP_SUPPORT)
#define AUDREG_SEMA_3WAY_GET(is_scp_sema_support) \
	({ \
		int __ret = -1; \
		if (is_scp_sema_support) \
			__ret = scp_get_semaphore_3way(SCP_SEMA_AUDIOREG); \
		else \
			__ret = get_adsp_semaphore(SEMA_AUDIOREG); \
		__ret; \
	})

#define AUDREG_SEMA_3WAY_RELEASE(is_scp_sema_support) \
	({ \
		if (is_scp_sema_support) \
			scp_release_semaphore_3way(SCP_SEMA_AUDIOREG); \
		else \
			release_adsp_semaphore(SEMA_AUDIOREG); \
	})
#elif defined(CONFIG_MTK_TINYSYS_SCP_SUPPORT)
#define AUDREG_SEMA_3WAY_GET(is_scp_sema_support) \
	({ \
		int __ret = -1; \
		if (is_scp_sema_support) \
			__ret = scp_get_semaphore_3way(SCP_SEMA_AUDIOREG); \
		__ret; \
	})

#define AUDREG_SEMA_3WAY_RELEASE(is_scp_sema_support) \
	({ \
		if (is_scp_sema_support) \
			scp_release_semaphore_3way(SCP_SEMA_AUDIOREG); \
	})
#elif defined(CONFIG_MTK_AUDIODSP_SUPPORT)
#define AUDREG_SEMA_3WAY_GET(is_scp_sema_support) \
	({ \
		get_adsp_semaphore(SEMA_AUDIOREG); \
	})

#define AUDREG_SEMA_3WAY_RELEASE(is_scp_sema_support) \
	({ \
		release_adsp_semaphore(SEMA_AUDIOREG); \
	})
#else
#define AUDREG_SEMA_3WAY_GET(is_scp_sema_support) (-1)
#define AUDREG_SEMA_3WAY_RELEASE(is_scp_sema_support) (-1)
#endif

bool mtk_get_speech_status(void);

#endif
