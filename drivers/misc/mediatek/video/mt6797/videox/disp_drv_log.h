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

#ifndef __DISP_DRV_LOG_H__
#define __DISP_DRV_LOG_H__
#include "display_recorder.h"
#include "ddp_debug.h"

#if 0 /*set 1 to output log to mobilelog */
#define DISP_LOG_PRINT(level, sub_module, fmt, arg...)      \
	    pr_debug(fmt, ##arg)

#define DISPMSG(string, args...)                   \
	do {                                       \
		if (ddp_debug_dbg_log_level())          \
			pr_debug("[DISP]"string, ##args);            \
	} while (0)

#define DISPDBG(string, args...)                   \
	do {                                       \
		if (ddp_debug_dbg_log_level())          \
			pr_debug("disp/"string, ##args);            \
	} while (0)

#define DISPCHECK(string, args...)                   \
	do {                                       \
		if (ddp_debug_dbg_log_level())          \
			pr_debug("[DISPCHECK]"string, ##args);            \
	} while (0)

#define DISPPR_FENCE(string, args...)                    \
	do { \
		if (ddp_debug_dbg_log_level()) {         \
			dprec_logger_pr(DPREC_LOGGER_FENCE, string, ##args);\
			pr_debug("fence/"string, ##args);\
		} \
	} while (0)
/*#define DISPMSG(string, args...) pr_debug("[DISP]"string, ##args)*/
/*#define DISPDBG(string, args...) pr_debug("disp/"string, ##args)*/
#define DISPERR	DISPPR_ERROR

#define DISPFUNC() DISPDBG("[DISP]func|%s\n", __func__)	/* default on, err msg */
#define DISPDBGFUNC() DISPDBG("[DISP]func|%s\n", __func__)	/* default on, err msg */
/*#define DISPCHECK(string, args...) pr_debug("[DISPCHECK]"string, ##args)*/

#define DISPPR_HWOP(string, args...)	/* dprec_logger_pr(DPREC_LOGGER_HWOP, string, ##args); */
#define DISPPR_ERROR(string, args...) \
	do {\
		dprec_logger_pr(DPREC_LOGGER_ERROR, string, ##args);\
		pr_err("[DISP][%s #%d]ERROR:"string, __func__, __LINE__, ##args);\
	} while (0)

#define DISPWARN(string, args...) \
	do { \
		dprec_logger_pr(DPREC_LOGGER_ERROR, string, ##args);\
		pr_warn("[DISP][%s #%d]warn:"string, __func__, __LINE__, ##args);\
	} while (0)

/*#define DISPPR_FENCE(string, args...)  do { \
		dprec_logger_pr(DPREC_LOGGER_FENCE, string, ##args);\
		pr_debug("fence/"string, ##args);\
	} while (0)*/
#define disp_aee_print(string, args...) do {\
	char disp_name[100];\
	snprintf(disp_name, 100, "[DISP]"string, ##args); \
	aee_kernel_warning_api(__FILE__, __LINE__, \
		DB_OPT_DEFAULT | DB_OPT_MMPROFILE_BUFFER | DB_OPT_DISPLAY_HANG_DUMP | DB_OPT_DUMP_DISPLAY, \
		disp_name, "[DISP] error"string, ##args);  \
	pr_err("DISP error: "string, ##args);  \
} while (0)
#else

#define DISP_LOG_PRINT(level, sub_module, fmt, args...)			\
	dprec_logger_pr(DPREC_LOGGER_DEBUG, fmt, ##args)

#define DISPMSG(string, args...)					\
	do {								\
		dprec_logger_pr(DPREC_LOGGER_DEBUG, string, ##args);	\
		if (g_mobilelog)					\
			pr_debug("[DISP]"string, ##args);		\
	} while (0)

#define DISPCHECK(string, args...)					\
	do {								\
		dprec_logger_pr(DPREC_LOGGER_DEBUG, string, ##args);	\
		if (g_mobilelog)					\
			pr_debug("[DISPCHECK]"string, ##args);		\
	} while (0)

#define DISPWARN(string, args...)					\
	do {								\
		dprec_logger_pr(DPREC_LOGGER_ERROR, string, ##args);	\
		pr_warn("[DISP][%s #%d]warn:"string,			\
				__func__, __LINE__, ##args); \
	} while (0)

#define DISPPR_ERROR(string, args...)					\
	do {								\
		dprec_logger_pr(DPREC_LOGGER_ERROR, string, ##args);	\
		pr_err("[DISP][%s #%d]ERROR:"string,			\
				__func__, __LINE__, ##args);		\
	} while (0)

#define DISPERR(string, args...) DISPPR_ERROR(string, ##args)

#define DISPPR_FENCE(string, args...)					\
	do {								\
		dprec_logger_pr(DPREC_LOGGER_FENCE, string, ##args);	\
		if (g_mobilelog)					\
			pr_debug("fence/"string, ##args);		\
	} while (0)

#define DISPDBG(string, args...)					\
	do {								\
		if (ddp_debug_dbg_log_level()) {			\
			DISPMSG(string, ##args);			\
		}							\
	} while (0)

#define DISPFUNC()							\
	do {								\
		dprec_logger_pr(DPREC_LOGGER_DEBUG, "func|%s\n", __func__); \
		if (g_mobilelog)					\
			pr_debug("[DISP]func|%s\n", __func__);		\
	} while (0)

#define DISPDBGFUNC() DISPFUNC()

#define DISPPR_HWOP(string, args...)

#define disp_aee_print(string, args...) do {				\
	char disp_name[100];						\
	snprintf(disp_name, 100, "[DISP]"string, ##args);		\
	aee_kernel_warning_api(__FILE__, __LINE__,			\
		DB_OPT_DEFAULT | DB_OPT_MMPROFILE_BUFFER | DB_OPT_DISPLAY_HANG_DUMP | DB_OPT_DUMP_DISPLAY, \
		disp_name, "[DISP] error"string, ##args);		\
	pr_err("DISP error: "string, ##args);				\
} while (0)
#endif
#endif				/* __DISP_DRV_LOG_H__ */
