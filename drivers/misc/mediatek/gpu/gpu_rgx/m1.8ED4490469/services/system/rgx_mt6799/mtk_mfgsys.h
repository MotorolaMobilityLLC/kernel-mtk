#ifndef MTK_MFGSYS_H
#define MTK_MFGSYS_H

#include "servicesext.h"
#include "rgxdevice.h"

/* control APM is enabled or not  */
#define MTK_PM_SUPPORT 1
#define MTCMOS_CONTROL 1

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

void MTKRGXDeviceInit(PVRSRV_DEVICE_CONFIG *psDevConfig);
void MTKQueryPowerState(int caller);

extern int spm_mtcmos_ctrl_mfg0(int state);
extern int spm_mtcmos_ctrl_mfg1(int state);
extern int spm_mtcmos_ctrl_mfg2(int state);

extern void switch_mfg_clk(int src);
extern int mtcmos_mfg_series_on(void);
extern void mfgsys_mtcmos_check(void);
extern unsigned int mfgsys_cg_check(void);

#endif
