/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef MTK_MFGSYS_H
#define MTK_MFGSYS_H

#include "servicesext.h"
#include "rgxdevice.h"

/* control APM is enabled or not  */
#define MTK_PM_SUPPORT 1

//extern to be used by PVRCore_Init in RGX DDK module.c 
PVRSRV_ERROR MTKMFGSystemInit(void);

void MTKMFGSystemDeInit(void);

void MTKMFGEnableDVFSTimer(bool bEnable);

/* below register interface in RGX sysconfig.c */
PVRSRV_ERROR MTKDevPrePowerState(IMG_HANDLE hSysData, PVRSRV_DEV_POWER_STATE eNewPowerState,
                                         PVRSRV_DEV_POWER_STATE eCurrentPowerState,
									     IMG_BOOL bForced);

PVRSRV_ERROR MTKDevPostPowerState(IMG_HANDLE hSysData, PVRSRV_DEV_POWER_STATE eNewPowerState,
                                          PVRSRV_DEV_POWER_STATE eCurrentPowerState,
									      IMG_BOOL bForced);

PVRSRV_ERROR MTKSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState);

PVRSRV_ERROR MTKSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState);

#endif

