/*
* Copyright (C) 2011-2014 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

/* ----------------------------------------------------------------------------- */
#ifndef CAMERA_PIPE_MGR_IMP_H
#define CAMERA_PIPE_MGR_IMP_H
/* ----------------------------------------------------------------------------- */
typedef unsigned long long MUINT64;
typedef long long MINT64;
typedef unsigned long MUINT32;
typedef long MINT32;
typedef unsigned char MUINT8;
typedef char MINT8;
typedef bool MBOOL;
#define MTRUE               true
#define MFALSE              false
/* ----------------------------------------------------------------------------- */
#define LOG_TAG "CamPipeMgr"
#define LOG_MSG(fmt, arg...)    pr_debug(LOG_TAG "[%s]"          fmt "\r\n", __func__,           ##arg)
#define LOG_WRN(fmt, arg...)    pr_warn(LOG_TAG "[%s]WRN(%5d):" fmt "\r\n", __func__, __LINE__, ##arg)
#define LOG_ERR(fmt, arg...)    pr_err(LOG_TAG "[%s]ERR(%5d):" fmt "\r\n", __func__, __LINE__, ##arg)
#define LOG_DMP(fmt, arg...)    pr_err(LOG_TAG ""              fmt,                                ##arg)
/* ----------------------------------------------------------------------------- */
#define CAM_PIPE_MGR_DEV_NUM        (1)
#define CAM_PIPE_MGR_DEV_MINOR_NUM  (1)
#define CAM_PIPE_MGR_DEV_NO_MINOR   (0)
#define CAM_PIPE_MGR_JIFFIES_MAX    (0xFFFFFFFF)
#define CAM_PIPE_MGR_PROC_NAME      "Default"
#define CAM_PIPE_MGR_SCEN_HW_AMOUNT (7)
#define CAM_PIPE_MGR_TIMEOUT_MAX    (10*1000)
/* ----------------------------------------------------------------------------- */
#define CAM_PIPE_MGR_PIPE_NAME_LEN          (10)
#define CAM_PIPE_MGR_PIPE_NAME_CAM_IO       "CamIO"
#define CAM_PIPE_MGR_PIPE_NAME_POST_PROC    "PostProc"
#define CAM_PIPE_MGR_PIPE_NAME_XDP_CAM      "CamXDP"
/* ----------------------------------------------------------------------------- */
typedef enum {
	CAM_PIPE_MGR_PIPE_CAM_IO,
	CAM_PIPE_MGR_PIPE_POST_PROC,
	CAM_PIPE_MGR_PIPE_XDP_CAM,
	CAM_PIPE_MGR_PIPE_AMOUNT
} CAM_PIPE_MGR_PIPE_ENUM;
/*  */
typedef enum {
	CAM_PIPE_MGR_STATUS_OK,
	CAM_PIPE_MGR_STATUS_FAIL,
	CAM_PIPE_MGR_STATUS_TIMEOUT,
	CAM_PIPE_MGR_STATUS_UNKNOW
} CAM_PIPE_MGR_STATUS_ENUM;
/*  */
#define CAM_PIPE_MGR_LOCK_TABLE_NONE    (0)
#define CAM_PIPE_MGR_LOCK_TABLE_IC      ((1<<CAM_PIPE_MGR_PIPE_CAM_IO)| \
					    (1<<CAM_PIPE_MGR_PIPE_POST_PROC))
#define CAM_PIPE_MGR_LOCK_TABLE_VR      ((1<<CAM_PIPE_MGR_PIPE_CAM_IO)| \
					    (1<<CAM_PIPE_MGR_PIPE_XDP_CAM))
#define CAM_PIPE_MGR_LOCK_TABLE_ZSD     ((1<<CAM_PIPE_MGR_PIPE_CAM_IO)| \
					    (1<<CAM_PIPE_MGR_PIPE_XDP_CAM))
#define CAM_PIPE_MGR_LOCK_TABLE_IP      ((1<<CAM_PIPE_MGR_PIPE_POST_PROC))
#define CAM_PIPE_MGR_LOCK_TABLE_N3D     ((1<<CAM_PIPE_MGR_PIPE_CAM_IO)| \
					    (1<<CAM_PIPE_MGR_PIPE_POST_PROC))
#define CAM_PIPE_MGR_LOCK_TABLE_VSS     ((1<<CAM_PIPE_MGR_PIPE_CAM_IO)| \
					    (1<<CAM_PIPE_MGR_PIPE_POST_PROC))
/* ----------------------------------------------------------------------------- */
typedef struct {
	pid_t Pid;
	pid_t Tgid;
	char ProcName[TASK_COMM_LEN];
	MUINT32 PipeMask;
	MUINT32 TimeS;
	MUINT32 TimeUS;
} CAM_PIPE_MGR_PROC_STRUCT;
/*  */
typedef struct {
	pid_t Pid;
	pid_t Tgid;
	char ProcName[TASK_COMM_LEN];
	MUINT32 TimeS;
	MUINT32 TimeUS;
} CAM_PIPE_MGR_PIPE_STRUCT;
/*  */
typedef struct {
	MUINT32 PipeMask;
	spinlock_t SpinLock;
	dev_t DevNo;
	struct cdev *pCharDrv;
	struct class *pClass;
	wait_queue_head_t WaitQueueHead;
	CAM_PIPE_MGR_MODE_STRUCT Mode;
	CAM_PIPE_MGR_PIPE_STRUCT PipeInfo[CAM_PIPE_MGR_PIPE_AMOUNT];
	char PipeName[CAM_PIPE_MGR_PIPE_AMOUNT][CAM_PIPE_MGR_PIPE_NAME_LEN];
	MUINT32 PipeLockTable[CAM_PIPE_MGR_SCEN_HW_AMOUNT];
	MUINT32 LogMask;
} CAM_PIPE_MGR_STRUCT;
/* ----------------------------------------------------------------------------- */
#endif
/* ----------------------------------------------------------------------------- */
