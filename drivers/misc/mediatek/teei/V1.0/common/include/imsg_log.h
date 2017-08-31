/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All rights reserved
 *
 * This file and software is confidential and proprietary to MICROTRUST Inc.
 * Unauthorized copying of this file and software is strictly prohibited.
 * You MUST NOT disclose this file and software unless you get a license
 * agreement from MICROTRUST Incorporated.
 */
#ifndef _ISEE_IMSG_LOG_H_
#define _ISEE_IMSG_LOG_H_

#ifndef IMSG_TAG
#define IMSG_TAG "[ISEE DRV]"
#endif

#if defined(CONFIG_MICROTRUST_DEBUG)
#define IMSG_DEBUG_BUILD      1
#else
#define IMSG_DEBUG_BUILD      0
#endif

#include <linux/printk.h>
#include <linux/time.h>

static inline unsigned long now_ms(void)
{
	struct timeval now_time;

	do_gettimeofday(&now_time);
	return ((now_time.tv_sec * 1000000) + now_time.tv_usec)/1000;
}

#define IMSG_PRINTK(fmt, ...)           pr_info(fmt, ##__VA_ARGS__)

#define IMSG_PRINT_ERROR(fmt, ...)      pr_debug(fmt, ##__VA_ARGS__)
#define IMSG_PRINT_WARN(fmt, ...)       pr_debug(fmt, ##__VA_ARGS__)
#define IMSG_PRINT_INFO(fmt, ...)       pr_info(fmt, ##__VA_ARGS__)
#define IMSG_PRINT_DEBUG(fmt, ...)      pr_debug(fmt, ##__VA_ARGS__)
#define IMSG_PRINT_TRACE(fmt, ...)      pr_debug(fmt, ##__VA_ARGS__)
#define IMSG_PRINT_ENTER(fmt, ...)      pr_debug(fmt, ##__VA_ARGS__)
#define IMSG_PRINT_LEAVE(fmt, ...)      pr_debug(fmt, ##__VA_ARGS__)
#define IMSG_PRINT_PROFILE(fmt, ...)    pr_debug(fmt, ##__VA_ARGS__)
#define IMSG_PRINT(LEVEL, fmt, ...)     IMSG_PRINT_##LEVEL("%s[%s]: " fmt, IMSG_TAG, #LEVEL, ##__VA_ARGS__)

#define IMSG_PRINT_TIME_S(fmt, ...) \
	unsigned long start_ms; \
	do { \
		start_ms = now_ms(); \
		IMSG_PRINT(PROFILE, fmt " (start:%lu)\n", ##__VA_ARGS__, start_ms); \
	} while (0)

#define IMSG_PRINT_TIME_E(fmt, ...) \
	do { \
		unsigned long end_ms, delta_ms; \
		end_ms = now_ms(); \
		delta_ms = end_ms - start_ms; \
		IMSG_PRINT(PROFILE, fmt " (end:%lu, spend:%lu ms)\n", ##__VA_ARGS__, end_ms, delta_ms); \
	} while (0)

/*************************************************************************/
/* Declear macros ********************************************************/
/*************************************************************************/
#define IMSG_WARN(fmt, ...)         IMSG_PRINT(WARN, fmt, ##__VA_ARGS__)
#define IMSG_ERROR(fmt, ...)        IMSG_PRINT(ERROR, fmt, ##__VA_ARGS__)

#if IMSG_DEBUG_BUILD == 1
#define IMSG_INFO(fmt, ...)         IMSG_PRINT(INFO, fmt, ##__VA_ARGS__)
#define IMSG_DEBUG(fmt, ...)        IMSG_PRINT(DEBUG, fmt, ##__VA_ARGS__)
#define IMSG_TRACE(fmt, ...)        IMSG_PRINT(TRACE, fmt, ##__VA_ARGS__)
#define IMSG_ENTER()                IMSG_PRINT(ENTER, "%s\n", __func__)
#define IMSG_LEAVE()                IMSG_PRINT(LEAVE, "%s\n", __func__)
#define IMSG_PROFILE_S(fmt, ...)    IMSG_PRINT_TIME_S(fmt, ##__VA_ARGS__)
#define IMSG_PROFILE_E(fmt, ...)    IMSG_PRINT_TIME_E(fmt, ##__VA_ARGS__)
#else
#define IMSG_INFO(fmt, ...)         do { } while (0)
#define IMSG_DEBUG(fmt, ...)        do { } while (0)
#define IMSG_TRACE(fmt, ...)        do { } while (0)
#define IMSG_ENTER()                do { } while (0)
#define IMSG_LEAVE()                do { } while (0)
#define IMSG_PROFILE_S(fmt, ...)    do { } while (0)
#define IMSG_PROFILE_E(fmt, ...)    do { } while (0)
#endif

#endif /*end of _ISEE_IMSG_LOG_H_*/
