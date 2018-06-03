#ifndef MTK_MFGSYS_H
#define MTK_MFGSYS_H

#include "servicesext.h"
#include "rgxdevice.h"
#include "ged_dvfs.h"
#include "ged_fdvfs.h"

/* control APM is enabled or not  */
#ifndef MTK_BRINGUP
#define MTK_PM_SUPPORT 1
#else
#define MTK_PM_SUPPORT 0
#endif
#define MTCMOS_CONTROL 1

//extern to be used by PVRCore_Init in RGX DDK module.c 
PVRSRV_ERROR MTKMFGSystemInit(void);

void MTKMFGSystemDeInit(void);

void MTKDisablePowerDomain(void);

void MTKSetICVerion(void);

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

extern int spm_mtcmos_ctrl_mfg0(int state);
extern int spm_mtcmos_ctrl_mfg1(int state);
extern int spm_mtcmos_ctrl_mfg2(int state);
extern int spm_mtcmos_ctrl_mfg3(int state);


extern void switch_mfg_clk(int src);
extern int mtcmos_mfg_series_on(void);

extern unsigned int mtk_notify_sspm_fdvfs_gpu_pow_status(unsigned int enable);
extern void mfgsys_mtcmos_check(void);
extern unsigned int mfgsys_cg_check(void);

extern void ged_log_trace_counter(char *name, int count);

extern void (*ged_dvfs_cal_gpu_utilization_fp)(unsigned int *pui32Loading,
			unsigned int *pui32Block,
			unsigned int *pui32Idle);
extern void (*ged_dvfs_gpu_freq_commit_fp)(unsigned long ui32NewFreqID,
			GED_DVFS_COMMIT_TYPE eCommitType,
			int *pbCommited);

#endif
