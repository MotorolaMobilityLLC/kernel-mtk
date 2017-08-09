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

#ifndef __MT_EEM2_H__
#define __MT_EEM2_H__



#include <linux/kernel.h>
#include <../../../include/mt-plat/sync_write.h>
/*#include <../../../include/mt-plat/mt_typedefs.h>*/
#include <../../../include/mt-plat/mt6797/include/mach/mt_secure_api.h>
/* #include <mach/sync_write.h> */
/* #include "mach/mt_reg_base.h" */
/* #include "mach/mt_typedefs.h"*/
/* #include "mach/mt_secure_api.h" */

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif



#define EEM2_REG_NUM		2
#define EEM2_BIG_REG_NUM	4


/*
EEM2 control register 0; [0x10220678], [0x10220738]
#define EEM2_DET_SWRST                     31:31
#define EEM2_DET_RAMPSTART                 13:12
#define EEM2_DET_RAMPSTEP                  11:8
#define EEM2_DET_DELAY                      7:4
#define EEM2_DET_AUTO_STOP_BYPASS_ENABLE    3:3
#define EEM2_DET_TRIGGER_PUL_DELAY          2:2
#define EEM2_CTRL_ENABLE                    1:1
#define EEM2_DET_ENABLE                     0:0
*/


/*
EEM2 control register 1; [0x1022067C], [0x01022073C]
#define EEM2_MP0_nCORERESET        31:28
#define EEM2_MP0_STANDBYWFE        27:24
#define EEM2_MP0_STANDBYWFI        23:20
#define EEM2_MP0_STANDBYWFIL2      19:19
*/

/* 0x1020073C */
/*
#define EEM2_MP1_nCORERESET        18:15
#define EEM2_MP1_STANDBYWFE        14:11
#define EEM2_MP1_STANDBYWFI        10:7
#define EEM2_MP1_STANDBYWFIL2       6:6
*/


/* 0x10222424 */
/*
#define EEM2_BIG_DET_TRIGGER_PUL_DELAY    16:16
#define EEM2_BIG_DET_RAMPSTART            15:14
#define EEM2_BIG_DET_DELAY                13:10
#define EEM2_BIG_DET_RAMPSTEP              9:6
#define EEM2_BIG_DET_AUTOSTOP_ENABLE       5:5
#define EEM2_BIG_DET_NOCPU_ENABLE          4:4
#define EEM2_BIG_DET_CPU_ENABLE            3:0
*/

/*
 * EEM2 rampstart rate
 */
enum {
	EEM2_RAMPSTART_0 = 0,
	EEM2_RAMPSTART_1 = 1,
	EEM2_RAMPSTART_2 = 2,
	EEM2_RAMPSTART_3 = 3
};



/**
* EEM2 register setting
*/
struct EEM2_data {
	unsigned int SWRST; /* 31:31 */
	unsigned int RAMPSTART; /* 13:12 */
	unsigned int RAMPSTEP; /* 11:8 */
	unsigned int DELAY; /* 7:4 */
	unsigned int AUTO_STOP_BYPASS_ENABLE; /* 3:3 */
	unsigned int TRIGGER_PUL_DELAY; /* 2:2 */
	unsigned int CTRL_ENABLE; /* 1:1 */
	unsigned int DET_ENABLE; /* 0:0 */
};

struct EEM2_big_data {
	/* 10222424 */
	unsigned int BIG_TRIGGER_PUL_DELAY; /* 16:16 */
	unsigned int BIG_RAMPSTART; /* 15:14 */
	unsigned int BIG_DELAY; /* 13:10 */
	unsigned int BIG_RAMPSTEP; /* 9:6 */
	unsigned int BIG_AUTOSTOPENABLE; /* 5:5 */
	unsigned int BIG_NOCPUENABLE; /* 4:4 */
	unsigned int BIG_CPUENABLE; /* 3:0 */
};

struct EEM2_trig {
	unsigned int mp0_nCORE_RESET; /* 31:28 */
	unsigned int mp0_STANDBY_WFE; /* 27:24 */
	unsigned int mp0_STANDBY_WFI; /* 23:20 */
	unsigned int mp0_STANDBY_WFIL2; /* 19:19 */
	unsigned int mp1_nCORE_RESET; /* 18:15 */
	unsigned int mp1_STANDBY_WFE; /* 14:11 */
	unsigned int mp1_STANDBY_WFI; /* 10:7 */
	unsigned int mp1_STANDBY_WFIL2; /* 6:6 */
};

struct EEM2_big_trig {
	/*
	31 : CPU PO RESET
	30 : CORE RESET
	29 : WFI
	28 : WFE
	0x10200278, 0x1020027C, 0x10200280
	*/
	unsigned int volatile ctrl_regs[3];
};


extern void turn_on_LO(void);
extern void turn_off_LO(void);
extern void turn_on_big_LO(void);
extern void turn_off_big_LO(void);
extern void eem2_pre_iomap(void);
extern int eem2_pre_init(void);

#endif
