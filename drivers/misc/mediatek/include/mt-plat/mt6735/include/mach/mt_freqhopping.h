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

#ifndef __MT_FREQHOPPING_H__
#define __MT_FREQHOPPING_H__

/* #include <linux/xlog.h> */

#define PLATFORM_DEP_DEBUG_PROC_READ

#define FHTAG "[FH]"
#define VERBOSE_DEBUG 0
#define DEBUG_MSG_ENABLE 0

#if VERBOSE_DEBUG
#define FH_MSG(fmt, args...) \
	printk(FHTAG""fmt" <- %s(): L<%d>  PID<%s><%d>\n", \
	##args , __func__, __LINE__, current->comm, current->pid)

#define FH_MSG_DEBUG   FH_MSG
#else
#if 1				/* log level is 6 xlog */
#define FH_MSG(fmt, args...) pr_debug(fmt, ##args)

#else				/* log level is 4 (printk) */
#define FH_MSG(fmt, args...) printk(FHTAG""fmt"\n", ##args)
#endif

#define FH_MSG_DEBUG(fmt, args...)\
	do {    \
		if (DEBUG_MSG_ENABLE)           \
			printk(FHTAG""fmt"\n", ##args); \
	} while (0)
#endif



enum FH_FH_STATUS {
	FH_FH_DISABLE = 0,
	FH_FH_ENABLE_SSC,
	FH_FH_ENABLE_DFH,
	FH_FH_ENABLE_DVFS,
};

enum FH_PLL_STATUS {
	FH_PLL_DISABLE = 0,
	FH_PLL_ENABLE = 1
};


enum FH_CMD {
	FH_CMD_ENABLE = 1,
	FH_CMD_DISABLE,
	FH_CMD_ENABLE_USR_DEFINED,
	FH_CMD_DISABLE_USR_DEFINED,
	FH_CMD_INTERNAL_MAX_CMD
};


enum FH_PLL_ID {
	FH_MIN_PLLID = 0,
	FH_ARM_PLLID = FH_MIN_PLLID,
	FH_MAIN_PLLID = 1,
	FH_MEM_PLLID = 2,
	FH_MM_PLLID = 3,
	FH_VENC_PLLID = 4,
	FH_MSDC_PLLID = 5,
	FH_TVD_PLLID = 6,
	FH_MAX_PLLID = FH_TVD_PLLID,
	FH_PLL_NUM
};

/* keep track the status of each PLL */
/* TODO: do we need another "uint mode" for Dynamic FH */
typedef struct {
	unsigned int fh_status;
	unsigned int pll_status;
	unsigned int setting_id;
	unsigned int curr_freq;
	unsigned int user_defined;
} fh_pll_t;



struct freqhopping_ssc {
	unsigned int freq;
	unsigned int dt;
	unsigned int df;
	unsigned int upbnd;
	unsigned int lowbnd;
	unsigned int dds;
};

struct freqhopping_ioctl {
	unsigned int pll_id;
	struct freqhopping_ssc ssc_setting;	/* used only when user-define */
	int result;
};

int freqhopping_config(unsigned int pll_id, unsigned long vco_freq, unsigned int enable);
void mt_freqhopping_init(void);
void mt_freqhopping_pll_init(void);
int mt_h2l_mempll(void);
int mt_l2h_mempll(void);
int mt_h2l_dvfs_mempll(void);
int mt_l2h_dvfs_mempll(void);
int mt_is_support_DFS_mode(void);
void mt_fh_popod_save(void);
void mt_fh_popod_restore(void);
int mt_fh_dram_overclock(int clk);
int mt_fh_get_dramc(void);
int mt_freqhopping_devctl(unsigned int cmd, void *args);

int mt_dfs_mmpll(unsigned int target_freq);
int mt_dfs_armpll(unsigned int pll, unsigned int dds);
int mt_dfs_vencpll(unsigned int target_dds);

int mt_dfs_mempll(unsigned int target_dds);

extern int DFS_APDMA_END(void);
extern int DFS_APDMA_Enable(void);
extern void DFS_APDMA_dummy_read_preinit(void);
extern void DFS_APDMA_dummy_read_deinit(void);

#endif				/* !__MT_FREQHOPPING_H__ */
