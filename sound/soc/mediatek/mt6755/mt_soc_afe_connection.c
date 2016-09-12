/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program
 * If not, see <http://www.gnu.org/licenses/>.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *  mt_sco_afe_connection.c
 *
 * Project:
 * --------
 *   MT6797  Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include "AudDrv_Common.h"
#include "AudDrv_Def.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#include "mt_soc_afe_connection.h"
#include "mt_soc_afe_control.h"
#include "mt_soc_digital_type.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <asm/irq.h>
#include <asm/div64.h>

/* mutex lock */
static DEFINE_MUTEX(afe_connection_mutex);

#define Soc_Aud_InterConnectionOutput_Num_Output_MT6755 (28)
#define Soc_Aud_InterConnectionInput_Num_Input_MT6755 (23)

/**
* here define conenction table for input and output
*/
static const char mConnectionTable[Soc_Aud_InterConnectionInput_Num_Input_MT6755]
	[Soc_Aud_InterConnectionOutput_Num_Output_MT6755] = {
	/*  0   1   2   3   4   5   6   7   8   9  10  11  12
	   13  14  15  16  17  18  19  20  21  22  23  24  25  26  27 */
	{3, 3, 3, 3, 3, 3, -1, 1, 1, 1, 1, -1, -1,
	 1, 1, 3, 3, 1, 1, 3, 25, 1, -1, -1, -1, -1, 1, -1},	/* I00 */
	{3, 3, 3, 3, 3, -1, 3, -1, -1, -1, -1, -1, -1,
	 1, 1, 1, 3, -1, -1, 3, 3, -1, 1, -1, 1, -1, -1, 1},	/* I01 */
	{1, 1, 1, 1, 1, 1, -1, 1, 1, -1, -1, 1, -1,
	 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I02 */
	{1, 1, 3, 1, 1, 1, 1, 1, -1, 1, -1, -1, -1,
	 -1, -1, 1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, 1, -1},	/* I03 */
	{1, 1, 3, 1, 1, -1, 1, -1, 1, -1, 1, -1, -1,
	 -1, -1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, -1, -1, 1},	/* I04 */
	{3, 3, 3, 3, 3, 1, -1, 0, -1, 1, -1, -1, -1,
	 1, 1, -1, -1, -1, -1, 3, 3, 1, -1, -1, -1, -1, -1, -1},	/* I05 */
	{3, 3, 3, 3, 3, -1, 1, -1, 0, -1, 1, -1, 0,
	 1, 1, -1, -1, -1, -1, 3, 3, -1, 1, -1, -1, -1, -1, -1},	/* I06 */
	{3, 3, 3, 3, 3, 1, -1, 0, -1, 0, -1, -1, -1,
	 1, 1, -1, -1, -1, -1, 3, 3, 1, -1, -1, -1, -1, -1, -1},	/* I07 */
	{3, 3, 3, 3, 3, -1, 1, -1, 0, -1, 0, -1, 0,
	 1, 1, -1, -1, -1, -1, 3, 3, -1, 1, -1, -1, -1, -1, -1},	/* I08 */
	{1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, 1,
	 -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, 1, -1, -1},	/* I09 */
	{3, -1, 3, 3, -1, 1, -1, 1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, 1, -1, 1, -1, 1, 1, 1, -1, 1, 1, -1},	/* I10 */
	{-1, 3, 3, -1, 3, -1, 1, -1, 1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, 1, -1, 1, 1, 1, -1, 1, -1, -1, 1},	/* I11 */
	{3, -1, 3, 3, -1, 0, -1, 1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, 1, -1, 1, -1, 1, 1, -1, -1, -1, -1, -1},	/* I12 */
	{-1, 3, 3, -1, 3, -1, 0, -1, 1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, 1, -1, 1, 1, 1, -1, -1, -1, -1, -1},	/* I13 */
	{1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, 1,
	 -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, -1, -1, -1, 1, 1},	/* I14 */
	{3, 3, 3, 3, 3, 3, -1, -1, -1, 1, -1, -1, -1,
	 1, 1, 3, 1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1},	/* I15 */
	{3, 3, 3, 3, 3, -1, 3, -1, -1, -1, 1, -1, -1,
	 1, 1, 1, 3, -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1},	/* I16 */
	{1, -1, 3, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1,
	 1, 1, 1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, -1, -1},	/* I17 */
	{-1, 1, 3, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1,
	 1, 1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, -1, -1, -1},	/* I18 */
	{3, -1, 3, 3, 3, 1, -1, 1, -1, 1, -1, 1, -1,
	 1, 1, 1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, -1, -1},	/* I19 */
	{-1, 3, 3, 3, 3, -1, 1, -1, 1, -1, 1, -1, 1,
	 1, 1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, -1, -1, -1},	/* I20 */
	{1, 1, 1, 1, 1, 1, -1, 1, 1, 1, -1, -1, 1,
	 1, 1, 1, 1, -1, -1, 1, 1, -1, -1, -1, 1, -1, 1, 1},	/* I21 */
	{1, 1, 1, 1, 1, -1, 1, -1, -1, -1, 1, -1, 1,
	 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, -1, -1},	/* I22 */
};


/* remind for AFE_CONN4 bit31 */
/**
* connection bits of certain bits
*/
static const char mConnectionbits[Soc_Aud_InterConnectionInput_Num_Input_MT6755]
	[Soc_Aud_InterConnectionOutput_Num_Output_MT6755] = {
	/* 0   1   2   3   4   5   6   7   8   9  10  11  12
	   13  14  15  16  17  18  19  20  21  22  23  24  25  26  27 */
	{0, 16, 0, 16, 0, 16, -1, 2, 5, 8, 12, -1, -1,
	 2, 15, 16, 22, 14, 18, 8, 24, 30, -1, -1, -1, -1, -1, -1},	/* I00 */
	{1, 17, 1, 17, 1, -1, 22, 3, 6, 9, 13, -1, -1,
	 3, 16, 17, 23, 15, 19, 10, 26, -1, 2, -1, 15, -1, -1, 5},	/* I01 */
	{2, 18, 2, 18, -1, 17, -1, 21, 22, -1, -1, 6, -1,
	 4, 17, -1, 24, 23, 24, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I02 */
	{3, 19, 3, 19, 3, 18, -1, 26, -1, 0, -1, -1, -1,
	 5, 18, 19, 25, 13, -1, 12, 28, 31, -1, 6, -1, 24, -1, -1},	/* I03 */
	{4, 20, 4, 20, 4, -1, 23, -1, 29, -1, 3, -1, -1,
	 6, 19, 20, 26, -1, 16, 13, 29, -1, 3, -1, 9, -1, -1, -1},	/* I04 */
	{5, 21, 5, 21, 5, 19, -1, 27, -1, 1, -1, 7, -1,
	 16, 20, 17, 26, 14, -1, 14, 31, 8, -1, -1, -1, -1, -1, -1},	/* I05 */
	{6, 22, 6, 22, 6, -1, 24, -1, 30, -1, 4, -1, 9,
	 17, 21, 18, 27, -1, 17, 16, 0, -1, 11, -1, -1, -1, -1, -1},	/* I06 */
	{7, 23, 7, 23, 7, 20, -1, 28, -1, 2, -1, 8, -1,
	 18, 22, 19, 28, 15, -1, 18, 2, 9, -1, -1, -1, -1, -1, -1},	/* I07 */
	{8, 24, 8, 24, 8, -1, 25, -1, 31, -1, 5, -1, 10,
	 19, 23, 20, 29, -1, 18, 20, 4, -1, 12, -1, -1, -1, -1, -1},	/* I08 */
	{9, 25, 9, 25, 9, 21, 27, -1, -1, 10, -1, -1, 11,
	 7, 20, 21, 27, 16, 20, 22, 26, -1, -1, 28, 29, 25, -1, -1},	/* I09 */
	{0, -1, 4, 8, -1, 12, -1, 14, -1, 26, 28, 30, 0,
	 -1, -1, -1, -1, 24, -1, 28, -1, 0, -1, 2, -1, 6, -1, -1},	/* I10 */
	{-1, 2, 6, -1, 10, -1, 13, -1, 15, 27, 29, 31, 1,
	 -1, -1, -1, -1, -1, 25, -1, 29, 31, 1, -1, 3, -1, -1, -1},	/* I11 */
	{0, -1, 4, 8, -1, 12, -1, 14, -1, 8, 10, 12, 14,
	 -1, -1, -1, -1, 6, -1, 2, -1, -1, 6, -1, -1, -1, -1, -1},	/* I12 */
	{-1, 2, 6, -1, 10, -1, 13, -1, 15, 9, 11, 13, 15,
	 -1, -1, -1, -1, -1, 7, -1, 3, 4, 7, -1, -1, -1, -1, -1},	/* I13 */
	{12, 17, 22, 27, 0, 5, -1, 4, 7, 11, -1, -1, 12,
	 8, 21, 28, 1, -1, -1, -1, 27, -1, -1, -1, -1, -1, -1, -1},	/* I14 */
	{13, 18, 23, 28, 1, 6, -1, -1, -1, 10, -1, -1, -1,
	 9, 22, 29, 2, -1, -1, 23, -1, 10, -1, -1, -1, -1, -1, -1},	/* I15 */
	{14, 19, 24, 29, 2, -1, 8, -1, -1, -1, 11, -1, -1,
	 10, 23, 30, 3, -1, -1, -1, -1, -1, 13, -1, -1, -1, -1, -1},	/* I16 */
	{0, 19, 27, 0, 2, 22, 8, 26, -1, 30, 11, 2, -1,
	 11, 24, 21, 30, 17, -1, 22, 6, 0, -1, 7, -1, -1, -1, -1},	/* I17 */
	{14, 3, 28, 29, 1, -1, 24, -1, 28, -1, 0, -1, 4,
	 12, 25, 22, 31, -1, 21, 23, 7, -1, 4, -1, 10, -1, -1, -1},	/* I18 */
	{1, 19, 10, 14, 18, 23, 8, 27, -1, 31, -1, 3, -1,
	 13, 26, 23, 0, 6, -1, 24, 28, 1, -1, 8, -1, -1, -1, -1},	/* I19 */
	{14, 4, 12, 16, 20, -1, 25, -1, 29, -1, 1, -1, 5,
	 14, 27, 24, 1, -1, 7, 25, 29, -1, 5, -1, 11, -1, -1, -1},	/* I20 */
	{12, 13, 14, 15, 16, 17, 8, 18, 19, 20, -1, -1, 21,
	 4, 5, 6, 7, -1, -1, 22, 23, -1, -1, -1, -1, -1, -1, -1},	/* I21 */
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I22 */
};

/**
* connection shift bits of certain bits
*/
static const char mShiftConnectionbits[Soc_Aud_InterConnectionInput_Num_Input_MT6755]
	[Soc_Aud_InterConnectionOutput_Num_Output_MT6755] = {
	/* 0   1   2   3   4   5   6   7   8   9  10  11  12
	   13  14  15  16  17  18  19  20  21  22  23  24  25 */
	{10, 26, 10, 26, 10, 19, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, 31, 25, -1, -1, 9, 25, -1, -1, -1, -1, -1},	/* I00 */
	{11, 27, 11, 27, 11, -1, 20, -1, -1, -1, -1, -1, -1,
	 -1, -1, 16, 4, -1, -1, 11, 27, -1, -1, -1, -1, -1},	/* I01 */
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I02 */
	{-1, -1, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I03 */
	{-1, -1, 26, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I04 */
	{12, 28, 12, 28, 12, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, 15, 30, -1, -1, -1, -1, -1},	/* I05 */
	{13, 29, 13, 29, 13, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, 17, 1, -1, -1, -1, -1, -1},	/* I06 */
	{14, 30, 14, 30, 14, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, 19, 3, -1, -1, -1, -1, -1},	/* I07 */
	{15, 31, 15, 31, 15, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, 21, 5, -1, -1, -1, -1, -1},	/* I08 */
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I09 */
	{1, -1, 5, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I10 */
	{-1, 3, 7, -1, 11, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I11 */
	{1, -1, 5, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I12 */
	{-1, 3, 7, -1, 11, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I13 */
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I14 */
	{15, 20, 25, 30, 3, 7, -1, -1, -1, 10, -1, -1, -1,
	 -1, -1, 0, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I15 */
	{16, 21, 26, 31, 4, -1, 9, -1, -1, -1, 11, -1, -1,
	 -1, -1, 30, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I16 */
	{14, 19, 30, 29, 2, -1, 8, -1, -1, -1, 11, -1, -1,
	 -1, -1, 30, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I17 */
	{14, 19, 31, 29, 2, -1, 8, -1, -1, -1, 11, -1, -1,
	 -1, -1, 30, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I18 */
	{2, 19, 11, 16, 19, -1, 8, -1, -1, -1, 11, -1, -1,
	 -1, -1, 30, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I19 */
	{14, 5, 13, 17, 21, -1, 8, -1, -1, -1, 11, -1, -1,
	 -1, -1, 30, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I20 */
	{14, 19, 24, 29, 2, -1, 8, -1, -1, -1, 11, -1, -1,
	 -1, -1, 30, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I21 */
};

/**
* connection of register
*/
static const short mConnectionReg[Soc_Aud_InterConnectionInput_Num_Input_MT6755]
	[Soc_Aud_InterConnectionOutput_Num_Output_MT6755] = {
	/*   0     1     2     3     4     5     6     7     8     9    10    11    12
	   13    14    15    16    17    18    19    20    21    22    23    24    25    26    27 */
	{0x20, 0x20, 0x24, 0x24, 0x28, 0x28, -1, 0x5c, -1, 0x5c, 0x5c, -1, -1,
	 0x448, 0x448, 0x438, 0x438, 0x5c, 0x5c, 0x464, 0x464, -1, -1, -1, -1, -1, -1, -1},	/* I00 */
	{0x20, 0x20, 0x24, 0x24, 0x28, -1, 0x28, 0x5c, 0x5c, 0x5c, 0x5c, -1, -1,
	 0x448, 0x448, 0x438, 0x438, 0x5c, 0x5c, 0x464, 0x464, -1, 0xbc, -1, 0x468, -1, -1, 0x46c},	/* I01 */
	{-1, 0x20, 0x24, 0x24, -1, -1, 0x30, 0x30, -1, -1, -1, 0x2c, -1,
	 0x448, 0x448, -1, -1, 0x30, 0x30, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I02 */
	{0x20, 0x20, 0x24, 0x24, 0x28, 0x28, -1, 0x28, -1, 0x2C, -1, -1, -1,
	 0x448, 0x448, 0x438, 0x438, 0x30, -1, 0x464, 0x464, 0x5c, -1, 0xbc, -1, 0xbc, -1, -1},	/* I03 */
	{0x20, 0x20, 0x24, 0x24, 0x28, -1, 0x28, -1, 0x28, -1, 0x2C, -1, -1,
	 0x448, 0x448, 0x438, 0x438, -1, 0x30, 0x464, 0x464, -1, 0xbc, -1, 0xbc, -1, -1, -1},	/* I04 */
	{0x20, 0x20, 0x24, 0x24, 0x28, 0x28, -1, 0x28, -1, 0x2C, -1, -1, -1,
	 0x420, 0x420, -1, -1, 0x30, -1, 0x464, 0x464, -1, -1, -1, -1, -1, -1, -1},	/* I05 */
	{0x20, 0x20, 0x24, 0x24, 0x28, -1, 0x28, -1, 0x28, -1, 0x2C, -1, 0x2C,
	 0x420, 0x420, -1, -1, -1, 0x30, 0x464, -1, -1, -1, -1, -1, -1, -1, -1},	/* I06 */
	{0x20, 0x20, 0x24, 0x24, 0x28, 0x28, -1, 0x28, -1, 0x2C, -1, -1, -1,
	 0x420, 0x420, -1, -1, 0x30, -1, 0x464, -1, -1, -1, -1, -1, -1, -1, -1},	/* I07 */
	{0x20, 0x20, 0x24, 0x24, 0x28, -1, 0x28, -1, 0x28, -1, 0x2C, -1, 0x2C,
	 0x420, 0x420, -1, -1, -1, 0x30, 0x464, -1, -1, -1, -1, -1, -1, -1, -1},	/* I08 */
	{0x20, 0x20, 0x24, 0x24, 0x28, 0x28, -1, -1, -1, 0x5c, -1, -1, 0x2C,
	 0x448, 0x448, 0x438, 0x438, 0x5c, 0x5c, 0x5c, 0x5c, -1, -1, 0xbc, 0xbc, 0xbc, -1, -1},	/* I09 */
	{0x420, -1, -1, 0x420, -1, 0x420, -1, 0x420, -1, -1, -1, -1, 0x448,
	 -1, -1, -1, -1, 0x420, -1, 0x448, -1, 0x448, 0x44c, 0x44c, -1, 0x44c, -1, -1},	/* I10 */
	{-1, 0x420, -1, -1, 0x420, -1, 0x420, -1, 0x420, -1, -1, -1, 0x448,
	 -1, -1, -1, -1, -1, 0x420, -1, 0x448, 0x448, 0x44c, -1, 0x44c, -1, -1, -1},	/* I11 */
	{0x438, 0x438, -1, 0x438, -1, 0x438, -1, 0x438, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, 0x440, -1, 0x444, -1, 0x444, 0x444, -1, -1, -1, -1, -1},	/* I12 */
	{-1, 0x438, -1, -1, 0x438, -1, 0x438, -1, 0x438, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, 0x440, -1, 0x444, 0x444, 0x444, -1, -1, -1, -1, -1},	/* I13 */
	{0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x30, -1, 0x5c, 0x5c, 0x5c, -1, -1, 0x30,
	 0x448, 0x448, 0x438, 0x440, -1, -1, 0x5c, 0x5c, -1, -1, -1, 0x44c, -1, -1, -1},	/* I14 */
	{0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x30, -1, -1, -1, 0x30, -1, -1, -1,
	 0x448, 0x448, 0x438, 0x440, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I15 */
	{0x2C, 0x2C, 0x2C, 0x2C, 0x30, -1, 0x30, -1, -1, -1, 0x30, -1, -1,
	 0x448, 0x448, 0x438, 0x440, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I16 */
	{0x460, 0x2C, 0x2C, 0x5c, 0x30, 0x460, 0x30, 0x460, -1, 0x460, 0x30, 0x464, -1,
	 0x448, 0x448, 0x438, 0x440, 0x5c, -1, 0x464, -1, 0xbc, -1, 0xbc, -1, 0xbc, -1, -1},	/* I17 */
	{0x2C, 0x460, 0x2C, 0x2C, 0x5c, -1, 0x460, -1, 0x460, -1, 0x464, -1, 0x464,
	 0x448, 0x448, 0x438, 0x440, -1, 0x5c, 0x464, -1, -1, 0xbc, -1, 0xbc, -1, -1, -1},	/* I18 */
	{0x460, 0x2C, 0x460, 0x460, 0x460, 0x460, 0x30, 0x460, -1, 0x460, 0x30, 0x464, -1,
	 0x448, 0x448, 0x438, 0x444, 0x464, -1, 0x5c, 0x5c, 0xbc, -1, 0xbc, -1, -1, -1, -1},	/* I19 */
	{0x2C, 0x460, 0x460, 0x460, 0x460, -1, 0x460, -1, 0x460, -1, 0x464, -1, 0x464,
	 0x448, 0x448, 0x438, 0x444, -1, 0x464, 0x5c, 0x5c, -1, 0xbc, -1, 0xbc, -1, -1, -1},	/* I20 */
	{0xbc, 0xbc, 0xbc, 0xbc, 0x30, -1, 0x30, 0xbc, 0xbc, 0xbc, 0x30, -1, 0xbc,
	 0x44c, 0x44c, 0x438, 0x440, -1, -1, 0xbc, 0xbc, -1, -1, -1, -1, -1, -1, -1},	/* I21 */
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I22 */
};


/**
* shift connection of register
*/
static const short mShiftConnectionReg[Soc_Aud_InterConnectionInput_Num_Input_MT6755]
	[Soc_Aud_InterConnectionOutput_Num_Output_MT6755] = {
	/*   0     1     2     3     4     5     6     7     8     9    10    11    12
	   13    14    15    16    17    18    19    20    21    22    23    24    25 */
	{0x20, 0x20, 0x24, 0x24, 0x28, 0x30, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, 0x438, -1, -1, -1, 0x464, 0x464, -1, -1, -1, -1, -1},	/* I00 */
	{0x20, 0x20, 0x24, 0x24, 0x28, -1, 0x30, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, 0x440, -1, -1, 0x464, 0x464, -1, -1, -1, -1, -1},	/* I01 */
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I02 */
	{-1, -1, 0x30, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I03 */
	{-1, -1, 0x30, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I04 */
	{0x20, 0x20, 0x24, 0x24, 0x28, -1, -1, -1, -1, -1, -1, 0x2C, -1,
	 -1, -1, -1, -1, -1, -1, 0x464, 0x464, -1, -1, -1, -1, -1},	/* I05 */
	{0x20, 0x20, 0x24, 0x24, 0x28, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, 0x464, -1, -1, -1, -1, -1, -1},	/* I06 */
	{0x20, 0x20, 0x24, 0x24, 0x28, -1, -1, -1, -1, -1, -1, 0x2C, -1,
	 -1, -1, -1, -1, -1, -1, 0x464, -1, -1, -1, -1, -1, -1},	/* I07 */
	{0x20, 0x20, 0x24, 0x24, 0x28, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, 0x464, -1, -1, -1, -1, -1, -1},	/* I08 */
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I09 */
	{0x420, -1, -1, 0x420, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I10 */
	{0x420, 0x420, -1, -1, 0x420, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I11 */
	{0x438, 0x438, -1, 0x438, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I12 */
	{-1, 0x438, -1, -1, 0x438, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I13 */
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I14 */
	{0x2C, 0x2C, -1, 0x2C, 0x30, 0x30, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, 0x440, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I15 */
	{0x2C, 0x2C, -1, 0x2C, 0x30, -1, 0x30, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, 0x440, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I16 */
	{0x2C, 0x2C, 0xbc, 0x2C, 0x30, -1, 0x30, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, 0x440, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I17 */
	{0x2C, 0x2C, 0xbc, 0x2C, 0x30, -1, 0x30, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, 0x440, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I18 */
	{0x460, 0x2C, 0x460, 0x460, 0x460, -1, 0x30, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, 0x440, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I19 */
	{0x2C, 0x460, 0x460, 0x460, 0x460, -1, 0x30, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, 0x440, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I20 */
	{0x2C, 0x2C, -1, 0x2C, 0x30, -1, 0x30, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, 0x440, -1, -1, -1, -1, -1, -1, -1, -1, -1},	/* I21 */
};

/**
* connection state of register
*/
static char mConnectionState[Soc_Aud_InterConnectionInput_Num_Input_MT6755]
	[Soc_Aud_InterConnectionOutput_Num_Output_MT6755] = {
	/* 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I00 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I01 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I02 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I03 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I04 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I05 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I06 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I07 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I08 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I09 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I10 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I11 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I12 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I13 */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* I14 */
	/* {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // I15 */
	/* {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // I16 */
};

typedef bool (*connection_function)(uint32);

bool SetDl1ToI2s0(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I05,
			Soc_Aud_InterConnectionOutput_O00);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I06,
			Soc_Aud_InterConnectionOutput_O01);
	return true;
}

bool SetDl1ToI2s1Dac(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I05,
			Soc_Aud_InterConnectionOutput_O03);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I06,
			Soc_Aud_InterConnectionOutput_O04);
	return true;
}

bool SetI2s2Adc2ToVulData2(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I23,
			Soc_Aud_InterConnectionOutput_O21);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I24,
			Soc_Aud_InterConnectionOutput_O22);
	return true;
}

bool SetI2s2AdcToVul(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O09);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I04,
			Soc_Aud_InterConnectionOutput_O10);
	return true;
}

bool SetDl1ToAwb(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I05,
			Soc_Aud_InterConnectionOutput_O05);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I06,
			Soc_Aud_InterConnectionOutput_O06);
	return true;
}

bool SetDl2ToAwb(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I07,
			Soc_Aud_InterConnectionOutput_O05);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I08,
			Soc_Aud_InterConnectionOutput_O06);
	return true;
}

bool SetDl1ToDaiBtOut(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I05,
			Soc_Aud_InterConnectionOutput_O02);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I06,
			Soc_Aud_InterConnectionOutput_O02);
	return true;
}

bool SetModem1InCh1ToI2s3(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I09,
			Soc_Aud_InterConnectionOutput_O00);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I09,
			Soc_Aud_InterConnectionOutput_O01);
	return true;
}

bool SetModem2InCh1ToI2s3(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I14,
			Soc_Aud_InterConnectionOutput_O00);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I14,
			Soc_Aud_InterConnectionOutput_O01);
	return true;
}

bool SetI2s0Ch2ToModem1OutCh4(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I01,
			Soc_Aud_InterConnectionOutput_O27);
	return true;
}

bool SetI2s0Ch2ToModem2OutCh4(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I01,
			Soc_Aud_InterConnectionOutput_O24);
	return true;
}

bool SetDl2ToI2s1Dac(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I07,
			Soc_Aud_InterConnectionOutput_O03);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I08,
			Soc_Aud_InterConnectionOutput_O04);
	return true;
}

bool SetDl2ToI2s1Dac2(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I07,
			Soc_Aud_InterConnectionOutput_O28);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I08,
			Soc_Aud_InterConnectionOutput_O29);
	return true;
}

bool SetDl2ToVul(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I07,
			Soc_Aud_InterConnectionOutput_O09);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I08,
			Soc_Aud_InterConnectionOutput_O10);
	return true;
}

bool SetI2s0ToHwGain1Out(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I00,
			Soc_Aud_InterConnectionOutput_O13);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I01,
			Soc_Aud_InterConnectionOutput_O14);
	return true;
}

bool SetHwGain1InToI2s1Dac(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I10,
			Soc_Aud_InterConnectionOutput_O03);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I11,
			Soc_Aud_InterConnectionOutput_O04);
	return true;
}

bool SetHwGain1InToI2s1Dac2(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I10,
			Soc_Aud_InterConnectionOutput_O28);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I11,
			Soc_Aud_InterConnectionOutput_O29);
	return true;
}

bool SetI2s0ToAwb(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I00,
			Soc_Aud_InterConnectionOutput_O05);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I01,
			Soc_Aud_InterConnectionOutput_O06);
	return true;
}

bool SetModem2InCh1ToModemDai(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I14,
			Soc_Aud_InterConnectionOutput_O12);
	return true;
}

bool SetModem1InCh1ToModemDai(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I09,
			Soc_Aud_InterConnectionOutput_O12);
	return true;
}

bool SetModem2InCh1ToI2s1Dac2(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I14,
			Soc_Aud_InterConnectionOutput_O28);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I14,
			Soc_Aud_InterConnectionOutput_O29);
	return true;
}

bool SetModem2InCh2ToI2s1Dac2(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I21,
			Soc_Aud_InterConnectionOutput_O28);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I21,
			Soc_Aud_InterConnectionOutput_O29);
	return true;
}

bool SetModem1InCh1ToI2s1Dac2(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I09,
			Soc_Aud_InterConnectionOutput_O28);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I09,
			Soc_Aud_InterConnectionOutput_O29);
	return true;
}

bool SetModem1InCh2ToI2s1Dac2(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I22,
			Soc_Aud_InterConnectionOutput_O28);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I22,
			Soc_Aud_InterConnectionOutput_O29);
	return true;
}

bool SetDl1Ch1ToModem2OutCh4(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I05,
			Soc_Aud_InterConnectionOutput_O24);
	return true;
}

bool SetDl1ToHwGain1Out(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I15,
			Soc_Aud_InterConnectionOutput_O13);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I16,
			Soc_Aud_InterConnectionOutput_O14);
	return true;
}

bool SetMrgI2sInToHwGain1Out(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I15,
			Soc_Aud_InterConnectionOutput_O13);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I16,
			Soc_Aud_InterConnectionOutput_O14);
	return true;
}

bool SetMrgI2sInToAwb(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I15,
			Soc_Aud_InterConnectionOutput_O05);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I16,
			Soc_Aud_InterConnectionOutput_O06);
	return true;
}

bool SetI2s2AdcToI2s3(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O00);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I04,
			Soc_Aud_InterConnectionOutput_O01);
	return true;
}

bool SetI2s2AdcToI2s1Dac(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O03);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I04,
			Soc_Aud_InterConnectionOutput_O04);
	return true;
}

bool SetI2s2AdcToI2s1Dac2(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O28);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I04,
			Soc_Aud_InterConnectionOutput_O29);
	return true;
}

bool SetI2s2AdcCh1ToI2s3(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O00);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O01);
	return true;
}

bool SetI2s2AdcCh1ToI2s1Dac(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O03);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O04);
	return true;
}

bool SetI2s2AdcCh1ToI2s1Dac2(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O28);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O29);
	return true;
}

bool SetI2s2AdcToModem2Out(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O17);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I04,
			Soc_Aud_InterConnectionOutput_O18);
	return true;
}

bool SetModem2InCh1ToI2s1Dac(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I14,
			Soc_Aud_InterConnectionOutput_O03);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I14,
			Soc_Aud_InterConnectionOutput_O04);
	return true;
}

bool SetDaiBtInToModem2Out(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I02,
			Soc_Aud_InterConnectionOutput_O17);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I02,
			Soc_Aud_InterConnectionOutput_O18);
	return true;
}

bool SetModem2InCh1ToDaiBtOut(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I14,
			Soc_Aud_InterConnectionOutput_O02);
	return true;
}

bool SetI2s2AdcToModem1Out(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I03,
			Soc_Aud_InterConnectionOutput_O07);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I04,
			Soc_Aud_InterConnectionOutput_O08);
	return true;
}

bool SetModem1InCh1ToI2s1Dac(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I09,
			Soc_Aud_InterConnectionOutput_O03);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I09,
			Soc_Aud_InterConnectionOutput_O04);
	return true;
}

bool SetDaiBtInToModem1Out(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I02,
			Soc_Aud_InterConnectionOutput_O07);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I02,
			Soc_Aud_InterConnectionOutput_O08);
	return true;
}

bool SetModem1InCh1ToDaiBtOut(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I09,
			Soc_Aud_InterConnectionOutput_O02);
	return true;
}

bool SetModem2InCh1ToAwbCh1(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I14,
			Soc_Aud_InterConnectionOutput_O05);
	return true;
}

bool SetModem1InCh1ToAwbCh1(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I09,
			Soc_Aud_InterConnectionOutput_O05);
	return true;
}

bool SetI2s0ToVul(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I00,
			Soc_Aud_InterConnectionOutput_O09);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I01,
			Soc_Aud_InterConnectionOutput_O10);
	return true;
}

bool SetDl1ToMrgI2sOut(uint32 ConnectionState)
{
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I05,
			Soc_Aud_InterConnectionOutput_O00);
	SetConnectionState(ConnectionState, Soc_Aud_InterConnectionInput_I06,
			Soc_Aud_InterConnectionOutput_O01);
	return true;
}

typedef struct connection_link_t {
	uint32 input;
	uint32 output;
	connection_function connectionFunction;
} connection_link_t;

static const connection_link_t mConnectionLink[] = {
	{Soc_Aud_AFE_IO_Block_MEM_DL1, Soc_Aud_AFE_IO_Block_I2S3, SetDl1ToI2s0},
	{Soc_Aud_AFE_IO_Block_MEM_DL1, Soc_Aud_AFE_IO_Block_I2S1_DAC, SetDl1ToI2s1Dac},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC_2, Soc_Aud_AFE_IO_Block_MEM_VUL_DATA2, SetI2s2Adc2ToVulData2},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC, Soc_Aud_AFE_IO_Block_MEM_VUL, SetI2s2AdcToVul},
	{Soc_Aud_AFE_IO_Block_MEM_DL1, Soc_Aud_AFE_IO_Block_MEM_AWB, SetDl1ToAwb},
	{Soc_Aud_AFE_IO_Block_MEM_DL2, Soc_Aud_AFE_IO_Block_MEM_AWB, SetDl2ToAwb},
	{Soc_Aud_AFE_IO_Block_MEM_DL1, Soc_Aud_AFE_IO_Block_DAI_BT_OUT, SetDl1ToDaiBtOut},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_1_I_CH1, Soc_Aud_AFE_IO_Block_I2S3, SetModem1InCh1ToI2s3},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_2_I_CH1, Soc_Aud_AFE_IO_Block_I2S3, SetModem2InCh1ToI2s3},
	{Soc_Aud_AFE_IO_Block_I2S0_CH2, Soc_Aud_AFE_IO_Block_MODEM_PCM_1_O_CH4, SetI2s0Ch2ToModem1OutCh4},
	{Soc_Aud_AFE_IO_Block_I2S0_CH2, Soc_Aud_AFE_IO_Block_MODEM_PCM_2_O_CH4, SetI2s0Ch2ToModem2OutCh4},
	{Soc_Aud_AFE_IO_Block_MEM_DL2, Soc_Aud_AFE_IO_Block_I2S1_DAC, SetDl2ToI2s1Dac},
	{Soc_Aud_AFE_IO_Block_MEM_DL2, Soc_Aud_AFE_IO_Block_I2S1_DAC_2, SetDl2ToI2s1Dac2},
	{Soc_Aud_AFE_IO_Block_MEM_DL2, Soc_Aud_AFE_IO_Block_MEM_VUL, SetDl2ToVul},
	{Soc_Aud_AFE_IO_Block_I2S0, Soc_Aud_AFE_IO_Block_HW_GAIN1_OUT, SetI2s0ToHwGain1Out},
	{Soc_Aud_AFE_IO_Block_HW_GAIN1_IN, Soc_Aud_AFE_IO_Block_I2S1_DAC, SetHwGain1InToI2s1Dac},
	{Soc_Aud_AFE_IO_Block_HW_GAIN1_IN, Soc_Aud_AFE_IO_Block_I2S1_DAC_2, SetHwGain1InToI2s1Dac2},
	{Soc_Aud_AFE_IO_Block_I2S0, Soc_Aud_AFE_IO_Block_MEM_AWB, SetI2s0ToAwb},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_2_I_CH1, Soc_Aud_AFE_IO_Block_MEM_MOD_DAI, SetModem2InCh1ToModemDai},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_1_I_CH1, Soc_Aud_AFE_IO_Block_MEM_MOD_DAI, SetModem1InCh1ToModemDai},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_2_I_CH1, Soc_Aud_AFE_IO_Block_I2S1_DAC_2, SetModem2InCh1ToI2s1Dac2},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_2_I_CH2, Soc_Aud_AFE_IO_Block_I2S1_DAC_2, SetModem2InCh2ToI2s1Dac2},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_1_I_CH1, Soc_Aud_AFE_IO_Block_I2S1_DAC_2, SetModem1InCh1ToI2s1Dac2},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_1_I_CH2, Soc_Aud_AFE_IO_Block_I2S1_DAC_2, SetModem1InCh2ToI2s1Dac2},
	{Soc_Aud_AFE_IO_Block_MEM_DL1_CH1, Soc_Aud_AFE_IO_Block_MODEM_PCM_2_O_CH4, SetDl1Ch1ToModem2OutCh4},
	{Soc_Aud_AFE_IO_Block_MEM_DL1, Soc_Aud_AFE_IO_Block_HW_GAIN1_OUT, SetDl1ToHwGain1Out},
	{Soc_Aud_AFE_IO_Block_MRG_I2S_IN, Soc_Aud_AFE_IO_Block_HW_GAIN1_OUT, SetMrgI2sInToHwGain1Out},
	{Soc_Aud_AFE_IO_Block_MRG_I2S_IN, Soc_Aud_AFE_IO_Block_MEM_AWB, SetMrgI2sInToAwb},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC, Soc_Aud_AFE_IO_Block_I2S3, SetI2s2AdcToI2s3},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC, Soc_Aud_AFE_IO_Block_I2S1_DAC, SetI2s2AdcToI2s1Dac},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC, Soc_Aud_AFE_IO_Block_I2S1_DAC_2, SetI2s2AdcToI2s1Dac2},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC_CH1, Soc_Aud_AFE_IO_Block_I2S3, SetI2s2AdcCh1ToI2s3},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC_CH1, Soc_Aud_AFE_IO_Block_I2S1_DAC, SetI2s2AdcCh1ToI2s1Dac},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC_CH1, Soc_Aud_AFE_IO_Block_I2S1_DAC_2, SetI2s2AdcCh1ToI2s1Dac2},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC, Soc_Aud_AFE_IO_Block_MODEM_PCM_2_O, SetI2s2AdcToModem2Out},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_2_I_CH1, Soc_Aud_AFE_IO_Block_I2S1_DAC, SetModem2InCh1ToI2s1Dac},
	{Soc_Aud_AFE_IO_Block_DAI_BT_IN, Soc_Aud_AFE_IO_Block_MODEM_PCM_2_O, SetDaiBtInToModem2Out},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_2_I_CH1, Soc_Aud_AFE_IO_Block_DAI_BT_OUT, SetModem2InCh1ToDaiBtOut},
	{Soc_Aud_AFE_IO_Block_I2S2_ADC, Soc_Aud_AFE_IO_Block_MODEM_PCM_1_O, SetI2s2AdcToModem1Out},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_1_I_CH1, Soc_Aud_AFE_IO_Block_I2S1_DAC, SetModem1InCh1ToI2s1Dac},
	{Soc_Aud_AFE_IO_Block_DAI_BT_IN, Soc_Aud_AFE_IO_Block_MODEM_PCM_1_O, SetDaiBtInToModem1Out},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_1_I_CH1, Soc_Aud_AFE_IO_Block_DAI_BT_OUT, SetModem1InCh1ToDaiBtOut},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_2_I_CH1, Soc_Aud_AFE_IO_Block_MEM_AWB_CH1, SetModem2InCh1ToAwbCh1},
	{Soc_Aud_AFE_IO_Block_MODEM_PCM_1_I_CH1, Soc_Aud_AFE_IO_Block_MEM_AWB_CH1, SetModem1InCh1ToAwbCh1},
	{Soc_Aud_AFE_IO_Block_I2S0, Soc_Aud_AFE_IO_Block_MEM_VUL, SetI2s0ToVul},
	{Soc_Aud_AFE_IO_Block_MEM_DL1, Soc_Aud_AFE_IO_Block_MRG_I2S_OUT, SetDl1ToMrgI2sOut}
};
static const int CONNECTION_LINK_NUM = sizeof(mConnectionLink) / sizeof(mConnectionLink[0]);

static bool CheckBitsandReg(short regaddr, char bits)
{
	if (regaddr <= 0 || bits < 0) {
		pr_debug("regaddr = %x bits = %d\n", regaddr, bits);
		return false;
	}
	return true;
}

bool SetConnectionState(uint32 ConnectionState, uint32 Input, uint32 Output)
{
	/* pr_debug("SetinputConnection ConnectionState = %d
	Input = %d Output = %d\n", ConnectionState, Input, Output); */
	if ((mConnectionTable[Input][Output]) < 0) {
		pr_warn("no connection mpConnectionTable[%d][%d] = %d\n", Input, Output,
		       mConnectionTable[Input][Output]);
	} else if ((mConnectionTable[Input][Output]) == 0) {
		pr_warn("test only !! mpConnectionTable[%d][%d] = %d\n", Input, Output,
		       mConnectionTable[Input][Output]);
	} else {
		if (mConnectionTable[Input][Output]) {
			int connectionBits = 0;
			int connectReg = 0;

			switch (ConnectionState) {
			case Soc_Aud_InterCon_DisConnect:
			{
				/* pr_debug("nConnectionState = %d\n", ConnectionState); */
				if ((mConnectionState[Input][Output] &
					Soc_Aud_InterCon_Connection) ==
					Soc_Aud_InterCon_Connection) {
					/* here to disconnect connect bits */
					connectionBits = mConnectionbits[Input][Output];
					connectReg = mConnectionReg[Input][Output];
					if (CheckBitsandReg(connectReg, connectionBits)) {
						Afe_Set_Reg(connectReg, 0 << connectionBits,
							    1 << connectionBits);
						mConnectionState[Input][Output] &=
						    ~(Soc_Aud_InterCon_Connection);
					}
				}
				if ((mConnectionState[Input][Output] &
					Soc_Aud_InterCon_ConnectionShift) ==
					Soc_Aud_InterCon_ConnectionShift) {
					/* here to disconnect connect shift bits */
					connectionBits =
					    mShiftConnectionbits[Input][Output];
					connectReg = mShiftConnectionReg[Input][Output];
					if (CheckBitsandReg(connectReg, connectionBits)) {
						Afe_Set_Reg(connectReg, 0 << connectionBits,
							    1 << connectionBits);
						mConnectionState[Input][Output] &=
						    ~(Soc_Aud_InterCon_ConnectionShift);
					}
				}
				break;
			}
			case Soc_Aud_InterCon_Connection:
				{
					/* pr_debug("nConnectionState = %d\n", ConnectionState); */
					/* here to disconnect connect shift bits */
					connectionBits = mConnectionbits[Input][Output];
					connectReg = mConnectionReg[Input][Output];
					if (CheckBitsandReg(connectReg, connectionBits)) {
						Afe_Set_Reg(connectReg, 1 << connectionBits,
							    1 << connectionBits);
						mConnectionState[Input][Output] |=
						    Soc_Aud_InterCon_Connection;
					}
					break;
				}
			case Soc_Aud_InterCon_ConnectionShift:
				{
					/* pr_debug("nConnectionState = %d\n", ConnectionState); */
					if ((mConnectionTable[Input][Output] &
					     Soc_Aud_InterCon_ConnectionShift) !=
					    Soc_Aud_InterCon_ConnectionShift) {
						pr_err("donn't support shift opeartion");
						break;
					}
					connectionBits = mShiftConnectionbits[Input][Output];
					connectReg = mShiftConnectionReg[Input][Output];
					if (CheckBitsandReg(connectReg, connectionBits)) {
						Afe_Set_Reg(connectReg, 1 << connectionBits,
							    1 << connectionBits);
						mConnectionState[Input][Output] |=
						    Soc_Aud_InterCon_ConnectionShift;
					}
					break;
				}
			default:
				pr_err("no this state ConnectionState = %d\n", ConnectionState);
				break;
			}
		}
	}
	return true;
}
EXPORT_SYMBOL(SetConnectionState);

connection_function GetConnectionFunction(uint32 Aud_block_In, uint32 Aud_block_Out)
{
	connection_function connectionFunction = 0;
	int i = 0;

	for (i = 0; i < CONNECTION_LINK_NUM; i++) {
		if ((mConnectionLink[i].input == Aud_block_In) &&
				(mConnectionLink[i].output == Aud_block_Out)) {
			return mConnectionLink[i].connectionFunction;
		}
	}
	return connectionFunction;
}

bool SetIntfConnectionState(uint32 ConnectionState, uint32 Aud_block_In, uint32 Aud_block_Out)
{
	bool ret = false;
	connection_function connectionFunction = GetConnectionFunction(Aud_block_In, Aud_block_Out);

	if (0 == connectionFunction) {
		pr_warn("no this connection function\n");
		return ret;
	}
	return connectionFunction(ConnectionState);
}
EXPORT_SYMBOL(SetIntfConnectionState);

bool SetIntfConnectionFormat(uint32 ConnectionFormat, uint32 Aud_block)
{
	switch (Aud_block) {
	case Soc_Aud_AFE_IO_Block_I2S3:
	{
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O00);
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O01);
		break;
	}
	case Soc_Aud_AFE_IO_Block_I2S1_DAC:
	{
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O03);
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O04);
		break;
	}
	case Soc_Aud_AFE_IO_Block_MEM_VUL:
	{
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O09);
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O10);
		break;
	}
	case Soc_Aud_AFE_IO_Block_MEM_VUL_DATA2:
	{
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O21);
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O22);
		break;
	}
	case Soc_Aud_AFE_IO_Block_DAI_BT_OUT:
	{
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O02);
		break;
	}
	case Soc_Aud_AFE_IO_Block_I2S1_DAC_2:
	{
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O28);
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O29);
		break;
	}
	case Soc_Aud_AFE_IO_Block_MEM_MOD_DAI:
	{
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O12);
		break;
	}
	case Soc_Aud_AFE_IO_Block_MEM_AWB:
	{
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O05);
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O06);
		break;
	}
	case Soc_Aud_AFE_IO_Block_MRG_I2S_OUT:
	{
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O00);
		SetoutputConnectionFormat(ConnectionFormat,
				Soc_Aud_InterConnectionOutput_O01);
		break;
	}
	default:
		pr_warn("no this Aud_block = %d\n", Aud_block);
		break;
	}
	return true;
}
EXPORT_SYMBOL(SetIntfConnectionFormat);

