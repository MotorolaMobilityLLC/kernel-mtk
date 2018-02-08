//qiumeng@wind-mobi.com add at 20161109 begin
/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2016, FocalTech Systems, Ltd., all rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

 /************************************************************************
*
* File Name: focaltech_esd_protection.c
*
* Author:	  Software Department, FocalTech
*
* Created: 2016-03-18
*
* Modify:
*

************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "focaltech_core.h"

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define FOCALTECH_ESD_PROTECTION_INFO  "File Version of  focaltech_esd_protection.c:  V1.1.0 2016-03-24"
#define FTS_ESD_PROTECTION_EN			1	//0:disable, 1:enable
#define ESD_PROTECTION_WAIT_TIME 		2000	//ms

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/



/*******************************************************************************
* Static variables
*******************************************************************************/
static struct timeval g_last_comm_time;//the Communication time of i2c RW
static struct task_struct *thread_esd_protection = NULL;

static DECLARE_WAIT_QUEUE_HEAD(esd_protection_waiter);

static int g_start_esd_protection = 0;//
static int g_esd_protection_use_i2c = 0;//
static int g_esd_protection_checking = 0;//
/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
extern void ctp_esd_check_func(void);
/*******************************************************************************
* Static function prototypes
*******************************************************************************/
static int fts_esd_protection_timeout(void *unused);
static int fts_esd_protection_check(void);

int  fts_esd_protection_init(void);
int  fts_esd_protection_exit(void);
int  fts_esd_protection_notice(void);
int  fts_esd_protection_suspend(void);
int  fts_esd_protection_resume(void);

/*******************************************************************************
* functions body
*******************************************************************************/
int fts_esd_protection_init(void)
{
	int err = 0;

	if(0 == FTS_ESD_PROTECTION_EN)
		return 0;

	g_start_esd_protection = 1;
	g_esd_protection_use_i2c = 0;
	g_esd_protection_checking = 0;

	do_gettimeofday(&g_last_comm_time);

	thread_esd_protection = kthread_run(fts_esd_protection_timeout, 0, "focal_esd_protection");
	if (IS_ERR(thread_esd_protection))
	{
		err = PTR_ERR(thread_esd_protection);
		TPD_DEBUG("failed to create kernel thread: %d\n", err);
	}

	return 0;
}
int fts_esd_protection_exit(void)
{
	if(0 == FTS_ESD_PROTECTION_EN)
		return 0;

	kthread_stop(thread_esd_protection);

	msleep(ESD_PROTECTION_WAIT_TIME);
	return 0;
}
static int fts_esd_protection_timeout(void *unused)
{
	unsigned int iDeltaTime=0;
	unsigned long uljiffies=0;
	struct timeval tv;

	struct sched_param param = { .sched_priority = 5 };
	sched_setscheduler(current, SCHED_RR, &param);
	uljiffies=msecs_to_jiffies(ESD_PROTECTION_WAIT_TIME + 20);
	do
	{
		wait_event_interruptible_timeout(esd_protection_waiter, 0, uljiffies);
		if(0 == g_start_esd_protection)
			continue;

		do_gettimeofday(&tv);
		iDeltaTime = (tv.tv_sec - g_last_comm_time.tv_sec)*MSEC_PER_SEC + (tv.tv_usec - g_last_comm_time.tv_usec)/1000;
		if(ESD_PROTECTION_WAIT_TIME < iDeltaTime)
		{
			fts_esd_protection_check();
		}
	}while(!kthread_should_stop());

	return 0;
}

int  fts_esd_protection_suspend(void)
{
	g_start_esd_protection = 0;
	TPD_DEBUG("%s enter\n",__func__);
	return 0;
}
int  fts_esd_protection_resume(void)
{
	g_start_esd_protection = 1;
	TPD_DEBUG("%s enter\n",__func__);
	return 0;
}

int fts_esd_protection_notice(void)
{
	int i = 0;

	if(0 == FTS_ESD_PROTECTION_EN)
		return 0;

	if(1 == g_esd_protection_use_i2c)
		return -3;

	for(i = 0; i < 10; i++)
	{
		if(0 == g_esd_protection_checking)
			break;
		msleep(2);
	}
	if(i == 10)
	{
		//FTS_COMMON_DBG("Failed to read/write i2c,  the i2c communication has been accounted for ESD PROTECTION.\n");
		return -1;
	}

	/*
	if(0 == g_start_esd_protection)
		g_start_esd_protection = 1;
	*/
	do_gettimeofday(&g_last_comm_time);

	return 0;
}

static int fts_esd_protection_check(void)
{
	g_esd_protection_checking = 1;
	g_esd_protection_use_i2c = 1;
	ctp_esd_check_func();
	g_esd_protection_use_i2c = 0;
	g_esd_protection_checking = 0;
	return 0;
}
//qiumeng@wind-mobi.com add at 20161109 end