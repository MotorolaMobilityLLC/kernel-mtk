/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
extern int g_vpu_log_level;
extern unsigned int g_mdla_func_mask;
/* LOG & AEE */
#define MDLA_TAG "[mdla]"
/*#define VPU_DEBUG*/
#ifdef VPU_DEBUG
#define LOG_DBG(format, args...)    pr_debug(MDLA_TAG " " format, ##args)
#else
#define LOG_DBG(format, args...)
#endif
#define LOG_INF(format, args...)    pr_info(MDLA_TAG " " format, ##args)
#define LOG_WRN(format, args...)    pr_info(MDLA_TAG "[warn] " format, ##args)
#define LOG_ERR(format, args...)    pr_info(MDLA_TAG "[error] " format, ##args)

enum MdlaFuncMask {
	VFM_NEED_WAIT_VCORE		= 0x1,
	VFM_ROUTINE_PRT_SYSLOG = 0x2
};

enum MdlaLogThre {
	/* >1, performance break down check */
	MdlaLogThre_PERFORMANCE    = 1,

	/* >2, algo info, opp info check */
	Log_ALGO_OPP_INFO  = 2,

	/* >3, state machine check, while wait vcore/do running */
	Log_STATE_MACHINE  = 3,

	/* >4, dump buffer mva */
	MdlaLogThre_DUMP_BUF_MVA   = 4
};
enum MDLA_power_param {
	MDLA_POWER_PARAM_FIX_OPP,
	MDLA_POWER_PARAM_DVFS_DEBUG,
	MDLA_POWER_PARAM_JTAG,
	MDLA_POWER_PARAM_LOCK,
	MDLA_POWER_PARAM_VOLT_STEP,
};
#define mdla_print_seq(seq_file, format, args...) \
	{ \
		if (seq_file) \
			seq_printf(seq_file, format, ##args); \
		else \
			LOG_ERR(format, ##args); \
	}

/**
 * mdla_dump_register - dump the register table, and show the content
 *                     of all fields.
 * @s:		the pointer to seq_file.
 */
int mdla_dump_register(struct seq_file *s);

/**
 * mdla_dump_image_file - dump the binary information stored in flash storage.
 * @s:          the pointer to seq_file.
 */
int mdla_dump_image_file(struct seq_file *s);

/**
 * mdla_dump_mesg - dump the log buffer, which is wroted by VPU
 * @s:          the pointer to seq_file.
 */
int mdla_dump_mesg(struct seq_file *s);

/**
 * mdla_dump_mdla - dump the mdla status
 * @s:          the pointer to seq_file.
 */
int mdla_dump_mdla(struct seq_file *s);

/**
 * mdla_dump_device_dbg - dump the remaining user information to debug fd leak
 * @s:          the pointer to seq_file.
 */
int mdla_dump_device_dbg(struct seq_file *s);


#ifdef CONFIG_MTK_MDLA_DEBUG
#define mdla_debug pr_debug
void mdla_dump_reg(void);
void mdla_debugfs_init(void);
void mdla_debugfs_exit(void);
#else
#define mdla_debug(...)
static inline void mdla_dump_reg(void)
{
}
static inline void mdla_debugfs_init(void)
{
}
static inline void mdla_debugfs_exit(void)
{
}
#endif

