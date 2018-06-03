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

#include <linux/module.h>
#include <linux/sched.h>
#include "mtk_mfgsys.h"
#include <mach/mt_clkmgr.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_gpufreq.h>
#include "pvr_gputrace.h"
#include "rgxdevice.h"
#include "osfunc.h"
#include "pvrsrv.h"
#include "rgxhwperf.h"
#include "device.h"
#include "rgxinit.h"

#include "ged_dvfs.h"
typedef void IMG_VOID;
/* MTK: disable 6795 DVFS temporarily, fix and remove me */

#ifdef CONFIG_MTK_HIBERNATION
#include "sysconfig.h"
#include <mach/mtk_hibernate_dpm.h>
#include <mach/mt_irq.h>
#include <mach/irqs.h>
#endif

#include <trace/events/mtk_events.h>
#include <linux/mtk_gpu_utility.h>

#define MTK_DEFER_DVFS_WORK_MS          10000
#define MTK_DVFS_SWITCH_INTERVAL_MS     50//16//100
#define MTK_SYS_BOOST_DURATION_MS       50
#define MTK_WAIT_FW_RESPONSE_TIMEOUT_US 5000
#define MTK_GPIO_REG_OFFSET             0x30
#define MTK_GPU_FREQ_ID_INVALID         0xFFFFFFFF
#define MTK_RGX_DEVICE_INDEX_INVALID    -1

static IMG_HANDLE g_RGXutilUser = NULL;
static IMG_HANDLE g_hDVFSTimer = NULL;
static POS_LOCK ghDVFSTimerLock = NULL;
static POS_LOCK ghDVFSLock = NULL;

typedef void* IMG_PVOID;

#if defined(MTK_USE_HW_APM) && defined(CONFIG_ARCH_MT6795)
static IMG_PVOID g_pvRegsKM = NULL;
#endif
#ifdef MTK_CAL_POWER_INDEX
static IMG_PVOID g_pvRegsBaseKM = NULL;
#endif

static IMG_BOOL g_bTimerEnable = IMG_FALSE;
static IMG_BOOL g_bExit = IMG_TRUE;
static IMG_INT32 g_iSkipCount;
static IMG_UINT32 g_sys_dvfs_time_ms;

static IMG_UINT32 g_bottom_freq_id;
static IMG_UINT32 gpu_bottom_freq;
static IMG_UINT32 g_cust_boost_freq_id;
static IMG_UINT32 gpu_cust_boost_freq;
static IMG_UINT32 g_cust_upbound_freq_id;
static IMG_UINT32 gpu_cust_upbound_freq;

static IMG_UINT32 gpu_power = 0;
static IMG_UINT32 gpu_dvfs_enable;
static IMG_UINT32 boost_gpu_enable;
static IMG_UINT32 gpu_debug_enable;
static IMG_UINT32 gpu_dvfs_force_idle = 0;
static IMG_UINT32 gpu_dvfs_cb_force_idle = 0;

static IMG_UINT32 gpu_pre_loading = 0;
static IMG_UINT32 gpu_loading = 0;
static IMG_UINT32 gpu_block = 0;
static IMG_UINT32 gpu_idle = 0;
static IMG_UINT32 gpu_freq = 0;

static IMG_BOOL g_bDeviceInit = IMG_FALSE;

static IMG_BOOL g_bUnsync =IMG_FALSE;
static IMG_UINT32 g_ui32_unsync_freq_id = 0;

#ifdef CONFIG_MTK_SEGMENT_TEST
static IMG_UINT32 efuse_mfg_enable =0;
#endif

#ifdef CONFIG_MTK_HIBERNATION
int gpu_pm_restore_noirq(struct device *device);
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
int gpu_pm_restore_noirq(struct device *device)
{
#if defined(MTK_CONFIG_OF) && defined(CONFIG_OF)
    int irq = MTKSysGetIRQ();
#else
    int irq = SYS_MTK_RGX_IRQ;
#endif
    mt_irq_set_sens(irq, MT_LEVEL_SENSITIVE);
    mt_irq_set_polarity(irq, MT_POLARITY_LOW);
    return 0;
}
#endif

static PVRSRV_DEVICE_NODE* MTKGetRGXDevNode(IMG_VOID)
{
    PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
    IMG_UINT32 i;
    for (i = 0; i < psPVRSRVData->ui32RegisteredDevices; i++)
    {
        PVRSRV_DEVICE_NODE* psDeviceNode = &psPVRSRVData->psDeviceNodeList[i];
        if (psDeviceNode && psDeviceNode->psDevConfig )
            /*&&
            psDeviceNode->psDevConfig->eDeviceType == PVRSRV_DEVICE_TYPE_RGX)*/
        {
            return psDeviceNode;
        }
    }
    return NULL;
}

static IMG_UINT32 MTKGetRGXDevIdx(IMG_VOID)
{
    static IMG_UINT32 ms_ui32RGXDevIdx = MTK_RGX_DEVICE_INDEX_INVALID;
    if (MTK_RGX_DEVICE_INDEX_INVALID == ms_ui32RGXDevIdx)
    {
        PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
        IMG_UINT32 i;
        for (i = 0; i < psPVRSRVData->ui32RegisteredDevices; i++)
        {
            PVRSRV_DEVICE_NODE* psDeviceNode = &psPVRSRVData->psDeviceNodeList[i];
            if (psDeviceNode && psDeviceNode->psDevConfig )
                /* &&
                psDeviceNode->psDevConfig->eDeviceType == PVRSRV_DEVICE_TYPE_RGX) */
            {
                ms_ui32RGXDevIdx = i;
                break;
            }
        }
    }
    return ms_ui32RGXDevIdx;
}

static IMG_VOID MTKWriteBackFreqToRGX(PVRSRV_DEVICE_NODE* psDevNode, IMG_UINT32 ui32NewFreq)
{
    PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
    // PVRSRV_DEVICE_NODE* psDeviceNode = &psPVRSRVData->psDeviceNodeList[ui32DeviceIndex];
    
    RGX_DATA* psRGXData = (RGX_DATA*)psDevNode->psDevConfig->hDevData;
    psRGXData->psRGXTimingInfo->ui32CoreClockSpeed = ui32NewFreq * 1000; /* kHz to Hz write to RGX as the same unit */
}

static IMG_VOID MTKEnableMfgClock(void)
{
    mt_gpufreq_voltage_enable_set(1);
    ged_dvfs_gpu_clock_switch_notify(1);
	enable_clock(MT_CG_MFG_AXI, "MFG");
	enable_clock(MT_CG_MFG_MEM, "MFG");
	enable_clock(MT_CG_MFG_G3D, "MFG");
	enable_clock(MT_CG_MFG_26M, "MFG");

#if defined(CONFIG_ARCH_MT6795)
#else
#ifdef CONFIG_MTK_SEGMENT_TEST
	//check mfg
	if (DRV_Reg32(0xf0006610) & 0x10)
	{
		efuse_mfg_enable =1;
	}
#endif
#endif

    if (gpu_debug_enable)
    {
        PVR_DPF((PVR_DBG_ERROR, "MTKEnableMfgClock"));
    }
}

#define MFG_BUS_IDLE_BIT ( 1 << 16 )

static IMG_VOID MTKDisableMfgClock(IMG_BOOL bForce)
{
#if defined(MTK_USE_HW_APM) && defined(CONFIG_ARCH_MT6795)    
    volatile int polling_count = 200000;
    volatile int i;

    if( IMG_FALSE == bForce)
{
        do {
            /// 0x13FFF000[16]
            /// 1'b1: bus idle
            /// 1'b0: bus busy  
            if (  DRV_Reg32(g_pvRegsKM) & MFG_BUS_IDLE_BIT )
            {
                break;
            }
            
        } while (polling_count--);
    }
#endif

    disable_clock(MT_CG_MFG_26M, "MFG");
    disable_clock(MT_CG_MFG_G3D, "MFG");
    disable_clock(MT_CG_MFG_MEM, "MFG");
    disable_clock(MT_CG_MFG_AXI, "MFG");
    ged_dvfs_gpu_clock_switch_notify(0);
    mt_gpufreq_voltage_enable_set(0);

    if (gpu_debug_enable)
    {
        PVR_DPF((PVR_DBG_ERROR, "MTKDisableMfgClock"));
    }
}

#if defined(MTK_USE_HW_APM) && defined(CONFIG_ARCH_MT6795)
static int MTKInitHWAPM(void)
{
    if (!g_pvRegsKM)
    {
        PVRSRV_DEVICE_NODE* psDevNode = MTKGetRGXDevNode();
        if (psDevNode)
        {
            IMG_CPU_PHYADDR sRegsPBase;
            PVRSRV_RGXDEV_INFO* psDevInfo = psDevNode->pvDevice;
            PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;
            if (psDevInfo)
            {
                PVR_DPF((PVR_DBG_ERROR, "psDevInfo->pvRegsBaseKM: %p", psDevInfo->pvRegsBaseKM));
            }
            if (psDevConfig)
            {
                sRegsPBase = psDevConfig->sRegsCpuPBase;
                sRegsPBase.uiAddr += 0xfff000;
                PVR_DPF((PVR_DBG_ERROR, "sRegsCpuPBase.uiAddr: 0x%08lx", (unsigned long)psDevConfig->sRegsCpuPBase.uiAddr));
                PVR_DPF((PVR_DBG_ERROR, "sRegsPBase.uiAddr: 0x%08lx", (unsigned long)sRegsPBase.uiAddr));
                g_pvRegsKM = OSMapPhysToLin(sRegsPBase, 0xFF, 0);
            }
        }
    }

    if (g_pvRegsKM)
    {
#if 0
    	DRV_WriteReg32(g_pvRegsKM + 0x24, 0x80076674);
    	DRV_WriteReg32(g_pvRegsKM + 0x28, 0x0e6d0a09);
#else
        //DRV_WriteReg32(g_pvRegsKM + 0xa0, 0x00bd0140);
        DRV_WriteReg32(g_pvRegsKM + 0x24, 0x002f313f);
        DRV_WriteReg32(g_pvRegsKM + 0x28, 0x3f383609);
        DRV_WriteReg32(g_pvRegsKM + 0xe0, 0x6c630176);
        DRV_WriteReg32(g_pvRegsKM + 0xe4, 0x75515a48);
        DRV_WriteReg32(g_pvRegsKM + 0xe8, 0x00210228);
        DRV_WriteReg32(g_pvRegsKM + 0xec, 0x80000000);
#endif
    }
	return PVRSRV_OK;
}

static int MTKDeInitHWAPM(void)
{
#if 0
    if (g_pvRegsKM)
    {
	    DRV_WriteReg32(g_pvRegsKM + 0x24, 0x0);
    	DRV_WriteReg32(g_pvRegsKM + 0x28, 0x0);
    }
#endif
	return PVRSRV_OK;
}
#endif

static IMG_BOOL MTKDoGpuDVFS(IMG_UINT32 ui32NewFreqID, IMG_BOOL bIdleDevice)
{
    PVRSRV_ERROR eResult;
    IMG_UINT32 ui32RGXDevIdx;
    IMG_BOOL bet = IMG_FALSE;

    // bottom bound
    if (ui32NewFreqID > g_bottom_freq_id)
    {
        ui32NewFreqID = g_bottom_freq_id;
    }
    if (ui32NewFreqID > g_cust_boost_freq_id)
    {
        ui32NewFreqID = g_cust_boost_freq_id;
    }

    // up bound
    if (ui32NewFreqID < g_cust_upbound_freq_id)
    {
        ui32NewFreqID = g_cust_upbound_freq_id;
    }

    // thermal power limit
    if (ui32NewFreqID < mt_gpufreq_get_thermal_limit_index())
    {
        ui32NewFreqID = mt_gpufreq_get_thermal_limit_index();
    }

    // no change
    if (ui32NewFreqID == mt_gpufreq_get_cur_freq_index())
    {
        return IMG_FALSE;
    }

    ui32RGXDevIdx = MTKGetRGXDevIdx();
    if (MTK_RGX_DEVICE_INDEX_INVALID == ui32RGXDevIdx)
    {
        return IMG_FALSE;
    }

    eResult = PVRSRVDevicePreClockSpeedChange(ui32RGXDevIdx, bIdleDevice, (IMG_VOID*)NULL);
    if ((PVRSRV_OK == eResult) || (PVRSRV_ERROR_RETRY == eResult))
    {
        unsigned int ui32GPUFreq;
        unsigned int ui32CurFreqID;
        PVRSRV_DEV_POWER_STATE ePowerState;

        PVRSRVGetDevicePowerState(ui32RGXDevIdx, &ePowerState);
        if (ePowerState != PVRSRV_DEV_POWER_STATE_ON)
        {
            MTKEnableMfgClock();
        }

        mt_gpufreq_target(ui32NewFreqID);
        ui32CurFreqID = mt_gpufreq_get_cur_freq_index();
        ui32GPUFreq = mt_gpufreq_get_frequency_by_level(ui32CurFreqID);
        gpu_freq = ui32GPUFreq;
#if defined(CONFIG_TRACING) && defined(CONFIG_MTK_SCHED_TRACERS)
        
        if (PVRGpuTraceEnabled())
        {
            trace_gpu_freq(ui32GPUFreq);
        }
        
#endif
        MTKWriteBackFreqToRGX(ui32RGXDevIdx, ui32GPUFreq);

#ifdef MTK_DEBUG
        pr_debug("PVR_K: %s 3DFreq=%d, Volt=%d\n", __func__, ui32GPUFreq, mt_gpufreq_get_cur_volt());
#endif

        if (ePowerState != PVRSRV_DEV_POWER_STATE_ON)
        {
            MTKDisableMfgClock(IMG_TRUE);
        }

        if (PVRSRV_OK == eResult)
        {
            PVRSRVDevicePostClockSpeedChange(ui32RGXDevIdx, bIdleDevice, (IMG_VOID*)NULL);
        }

        return IMG_TRUE;
    }

    return IMG_FALSE;
}


/* For ged_dvfs idx commit */
static void MTKCommitFreqIdx(unsigned long ui32NewFreqID, GED_DVFS_COMMIT_TYPE eCommitType, int* pbCommited)
{
    PVRSRV_DEV_POWER_STATE ePowerState;
    PVRSRV_DEVICE_NODE* psDevNode = MTKGetRGXDevNode();
    PVRSRV_ERROR eResult;
    
    
    if (psDevNode)
    {
        eResult = PVRSRVDevicePreClockSpeedChange(psDevNode, IMG_FALSE, (IMG_VOID*)NULL);
        if ((PVRSRV_OK == eResult) || (PVRSRV_ERROR_RETRY == eResult))
        {
            unsigned int ui32GPUFreq;
            unsigned int ui32CurFreqID;
            PVRSRV_DEV_POWER_STATE ePowerState;

            PVRSRVGetDevicePowerState(psDevNode, &ePowerState);
			
            if (ePowerState == PVRSRV_DEV_POWER_STATE_ON)
            {
                mt_gpufreq_target(ui32NewFreqID);
					g_bUnsync = IMG_FALSE;
            }
            else
				{
					g_ui32_unsync_freq_id = ui32NewFreqID;
					g_bUnsync = IMG_TRUE;
				}
				
            ui32CurFreqID = mt_gpufreq_get_cur_freq_index();
            ui32GPUFreq = mt_gpufreq_get_frequency_by_level(ui32CurFreqID);
            gpu_freq = ui32GPUFreq;
    #if defined(CONFIG_TRACING) && defined(CONFIG_MTK_SCHED_TRACERS)
    
            
            if (PVRGpuTraceEnabled())
            {
                trace_gpu_freq(ui32GPUFreq);
            }
            
    #endif
            MTKWriteBackFreqToRGX(psDevNode, ui32GPUFreq);

    #ifdef MTK_DEBUG
            pr_debug("PVR_K: %s 3DFreq=%d, Volt=%d\n", __func__, ui32GPUFreq, mt_gpufreq_get_cur_volt());
    #endif

            if (PVRSRV_OK == eResult)
            {
                PVRSRVDevicePostClockSpeedChange(psDevNode, IMG_FALSE, (IMG_VOID*)NULL);
            }

		/*Always return true because the APM would almost letting GPU power down with high possibility while DVFS commiting*/
            if(pbCommited)
						*pbCommited = IMG_TRUE;
				return;
        }
    }
    
    if(pbCommited)
						*pbCommited = IMG_FALSE;
}

#define mt_gpufreq_get_freq_by_idx mt_gpufreq_get_frequency_by_level
unsigned int MTKCommitFreqForPVR(unsigned long ui32NewFreq)
{
    int i32MaxLevel = (int)(mt_gpufreq_get_dvfs_table_num()-1);
    unsigned int ui32NewFreqID = 0;
    unsigned long gpu_freq;
    int i;
    int promoted = 0;

    // promotion bit at LSB
    promoted = ui32NewFreq&0x1;
    
    ui32NewFreq /= 1000; // shrink to KHz
    ui32NewFreqID = i32MaxLevel;
    for (i = 0; i <= i32MaxLevel; i++)
    {
        gpu_freq = mt_gpufreq_get_freq_by_idx(i);

        if (ui32NewFreq > gpu_freq)
        {
            if(i==0)
                ui32NewFreqID = 0;
            else
                ui32NewFreqID = i-1;
            break;
        }
    }
    
    if(ui32NewFreqID!=0)
        ui32NewFreqID-=promoted; 
    
    PVR_DPF((PVR_DBG_ERROR, "MTKCommitFreqIdxForPVR: idx: %d | %d\n",ui32NewFreqID, promoted));
    
    MTKCommitFreqIdx(ui32NewFreqID, 0x7788, NULL);
    
    gpu_freq = mt_gpufreq_get_freq_by_idx(ui32NewFreqID);
    return gpu_freq * 1000; // scale up to Hz
}

static void MTKFreqInputBoostCB(unsigned int ui32BoostFreqID)
{
    if (0 < g_iSkipCount)
    {
        return;
    }

    if (boost_gpu_enable == 0)
    {
        return;
    }

    OSLockAcquire(ghDVFSLock);

    if (ui32BoostFreqID < mt_gpufreq_get_cur_freq_index())
    {
        if (MTKDoGpuDVFS(ui32BoostFreqID, gpu_dvfs_cb_force_idle == 0 ? IMG_FALSE : IMG_TRUE))
        {
            g_sys_dvfs_time_ms = OSClockms();
        }
    }

    OSLockRelease(ghDVFSLock);

}

static void MTKFreqPowerLimitCB(unsigned int ui32LimitFreqID)
{
    if (0 < g_iSkipCount)
    {
        return;
    }

    OSLockAcquire(ghDVFSLock);

    if (ui32LimitFreqID > mt_gpufreq_get_cur_freq_index())
    {
        if (MTKDoGpuDVFS(ui32LimitFreqID, gpu_dvfs_cb_force_idle == 0 ? IMG_FALSE : IMG_TRUE))
        {
            g_sys_dvfs_time_ms = OSClockms();
        }
    }

    OSLockRelease(ghDVFSLock);
}

#ifdef MTK_CAL_POWER_INDEX
static IMG_VOID MTKStartPowerIndex(IMG_VOID)
{
    if (!g_pvRegsBaseKM)
    {
        PVRSRV_DEVICE_NODE* psDevNode = MTKGetRGXDevNode();
        if (psDevNode)
        {
            PVRSRV_RGXDEV_INFO* psDevInfo = psDevNode->pvDevice;
            if (psDevInfo)
            {
                g_pvRegsBaseKM = psDevInfo->pvRegsBaseKM;
            }
        }
    }
    if (g_pvRegsBaseKM)
    {
        DRV_WriteReg32(g_pvRegsBaseKM + (uintptr_t)0x6320, 0x1);
    }
}

static IMG_VOID MTKReStartPowerIndex(IMG_VOID)
{
    if (g_pvRegsBaseKM)
    {
        DRV_WriteReg32(g_pvRegsBaseKM + (uintptr_t)0x6320, 0x1);
    }
}

static IMG_VOID MTKStopPowerIndex(IMG_VOID)
{
    if (g_pvRegsBaseKM)
    {
        DRV_WriteReg32(g_pvRegsBaseKM + (uintptr_t)0x6320, 0x0);
    }
}

static IMG_UINT32 MTKCalPowerIndex(IMG_VOID)
{
    IMG_UINT32 ui32State, ui32Result;
    PVRSRV_DEV_POWER_STATE  ePowerState;
    IMG_BOOL bTimeout;
    IMG_UINT32 u32Deadline;
    IMG_PVOID pvGPIO_REG = g_pvRegsKM + (uintptr_t)MTK_GPIO_REG_OFFSET;
    IMG_PVOID pvPOWER_ESTIMATE_RESULT = g_pvRegsBaseKM + (uintptr_t)6328;

    if ((!g_pvRegsKM) || (!g_pvRegsBaseKM))
    {
        return 0;
    }

    if (PVRSRVPowerLock() != PVRSRV_OK)
    {
        return 0;
    }

	PVRSRVGetDevicePowerState(MTKGetRGXDevIdx(), &ePowerState);
    if (ePowerState != PVRSRV_DEV_POWER_STATE_ON)
	{
		PVRSRVPowerUnlock();
		return 0;
	}

    //writes 1 to GPIO_INPUT_REQ, bit[0]
    DRV_WriteReg32(pvGPIO_REG, DRV_Reg32(pvGPIO_REG) | 0x1);

    //wait for 1 in GPIO_OUTPUT_REQ, bit[16]
    bTimeout = IMG_TRUE;
    u32Deadline = OSClockus() + MTK_WAIT_FW_RESPONSE_TIMEOUT_US;
    while(OSClockus() < u32Deadline)
    {
        if (0x10000 & DRV_Reg32(pvGPIO_REG))
        {
            bTimeout = IMG_FALSE;
            break;
        }
    }

    //writes 0 to GPIO_INPUT_REQ, bit[0]
    DRV_WriteReg32(pvGPIO_REG, DRV_Reg32(pvGPIO_REG) & (~0x1));
    if (bTimeout)
    {
        PVRSRVPowerUnlock();
        return 0;
    }

    //read GPIO_OUTPUT_DATA, bit[24]
    ui32State = DRV_Reg32(pvGPIO_REG) >> 24;

    //read POWER_ESTIMATE_RESULT
    ui32Result = DRV_Reg32(pvPOWER_ESTIMATE_RESULT);

    //writes 1 to GPIO_OUTPUT_ACK, bit[17]
    DRV_WriteReg32(pvGPIO_REG, DRV_Reg32(pvGPIO_REG)|0x20000);

    //wait for 0 in GPIO_OUTPUT_REG, bit[16]
    bTimeout = IMG_TRUE;
    u32Deadline = OSClockus() + MTK_WAIT_FW_RESPONSE_TIMEOUT_US;
    while(OSClockus() < u32Deadline)
    {
        if (!(0x10000 & DRV_Reg32(pvGPIO_REG)))
        {
            bTimeout = IMG_FALSE;
            break;
        }
    }

    //writes 0 to GPIO_OUTPUT_ACK, bit[17]
    DRV_WriteReg32(pvGPIO_REG, DRV_Reg32(pvGPIO_REG) & (~0x20000));
    if (bTimeout)
    {
        PVRSRVPowerUnlock();
        return 0;
    }

    MTKReStartPowerIndex();

    PVRSRVPowerUnlock();

    return (1 == ui32State) ? ui32Result : 0;
}
#endif

static IMG_VOID MTKCalGpuLoading(unsigned int* pui32Loading , unsigned int* pui32Block,unsigned int* pui32Idle)
{

    PVRSRV_DEVICE_NODE* psDevNode = MTKGetRGXDevNode();
    if (!psDevNode)
    {
        PVR_DPF((PVR_DBG_ERROR, "psDevNode not found"));
        return;
    }
    PVRSRV_RGXDEV_INFO* psDevInfo = psDevNode->pvDevice;
    if (psDevInfo && psDevInfo->pfnGetGpuUtilStats)
    {
        RGXFWIF_GPU_UTIL_STATS sGpuUtilStats = {0};
        
        /* 1.5DDK support multiple user to query GPU utilization, Need Init here*/    
        if(NULL==g_RGXutilUser)
        {
            if(psDevInfo)
            {
                RGXRegisterGpuUtilStats(&g_RGXutilUser);
            }
        }
        
        if(NULL==g_RGXutilUser)
            return ;
    
        psDevInfo->pfnGetGpuUtilStats(psDevInfo->psDeviceNode, g_RGXutilUser, &sGpuUtilStats);
        if (sGpuUtilStats.bValid)
        {
#if 0
            PVR_DPF((PVR_DBG_ERROR,"Loading: A(%d), I(%d), B(%d)",
                sGpuUtilStats.ui64GpuStatActiveHigh, sGpuUtilStats.ui64GpuStatIdle, sGpuUtilStats.ui64GpuStatBlocked));
#endif
#if defined(__arm64__) || defined(__aarch64__)
		*pui32Loading = (100*(sGpuUtilStats.ui64GpuStatActiveHigh + sGpuUtilStats.ui64GpuStatActiveLow)) / sGpuUtilStats.ui64GpuStatCumulative;
                *pui32Block = (100*(sGpuUtilStats.ui64GpuStatBlocked)) / sGpuUtilStats.ui64GpuStatCumulative;
                *pui32Idle = (100*(sGpuUtilStats.ui64GpuStatIdle)) / sGpuUtilStats.ui64GpuStatCumulative;
#else
		*pui32Loading = (unsigned long)(100*(sGpuUtilStats.ui64GpuStatActiveHigh + sGpuUtilStats.ui64GpuStatActiveLow)) / (unsigned long)sGpuUtilStats.ui64GpuStatCumulative;
		*pui32Block =  (unsigned long)(100*(sGpuUtilStats.ui64GpuStatBlocked)) / (unsigned long)sGpuUtilStats.ui64GpuStatCumulative;
		*pui32Idle = (unsigned long)(100*(sGpuUtilStats.ui64GpuStatIdle)) / (unsigned long)sGpuUtilStats.ui64GpuStatCumulative;
#endif
        }
    }
}

static IMG_BOOL MTKGpuDVFSPolicy(IMG_UINT32 ui32GPULoading, unsigned int* pui32NewFreqID)
{
    int i32MaxLevel = (int)(mt_gpufreq_get_dvfs_table_num() - 1);
    int i32CurFreqID = (int)mt_gpufreq_get_cur_freq_index();
    int i32NewFreqID = i32CurFreqID;

    if (ui32GPULoading >= 99)
    {
        i32NewFreqID = 0;
    }
    else if (ui32GPULoading <= 1)
    {
        i32NewFreqID = i32MaxLevel;
    }
    else if (ui32GPULoading >= 85)
    {
        i32NewFreqID -= 2;
    }
    else if (ui32GPULoading <= 30)
    {
        i32NewFreqID += 2;
    }
    else if (ui32GPULoading >= 70)
    {
        i32NewFreqID -= 1;
    }
    else if (ui32GPULoading <= 50)
    {
        i32NewFreqID += 1;
    }

    if (i32NewFreqID < i32CurFreqID)
    {
        if (gpu_pre_loading * 17 / 10 < ui32GPULoading)
        {
            i32NewFreqID -= 1;
        }
    }
    else if (i32NewFreqID > i32CurFreqID)
    {
        if (ui32GPULoading * 17 / 10 < gpu_pre_loading)
        {
            i32NewFreqID += 1;
        }
    }

    if (i32NewFreqID > i32MaxLevel)
    {
        i32NewFreqID = i32MaxLevel;
    }
    else if (i32NewFreqID < 0)
    {
        i32NewFreqID = 0;
    }

    if (i32NewFreqID != i32CurFreqID)
    {
        
        *pui32NewFreqID = (unsigned int)i32NewFreqID;
        return IMG_TRUE;
    }
    
    return IMG_FALSE;
}

static IMG_VOID MTKDVFSTimerFuncCB(IMG_PVOID pvData)
{
    int i32MaxLevel = (int)(mt_gpufreq_get_dvfs_table_num() - 1);
    int i32CurFreqID = (int)mt_gpufreq_get_cur_freq_index();

    if (gpu_debug_enable)
    {
        PVR_DPF((PVR_DBG_ERROR, "MTKDVFSTimerFuncCB"));
    }

    if (0 == gpu_dvfs_enable)
    {
        gpu_power = 0;
        gpu_loading = 0;
        gpu_block= 0;
        gpu_idle = 0;
        return;
    }

    if (g_iSkipCount > 0)
    {
        gpu_power = 0;
        gpu_loading = 0;
        gpu_block= 0;
        gpu_idle = 0;
        g_iSkipCount -= 1;
    }
    else if ((!g_bExit) || (i32CurFreqID < i32MaxLevel))
    {
        IMG_UINT32 ui32NewFreqID;

        // calculate power index
#ifdef MTK_CAL_POWER_INDEX
        gpu_power = MTKCalPowerIndex();
#else
        gpu_power = 0;
#endif

        MTKCalGpuLoading(&gpu_loading, &gpu_block, &gpu_idle);

        OSLockAcquire(ghDVFSLock);

        // check system boost duration
        if ((g_sys_dvfs_time_ms > 0) && (OSClockms() - g_sys_dvfs_time_ms < MTK_SYS_BOOST_DURATION_MS))
        {
            OSLockRelease(ghDVFSLock);
            return;
        }
        else
        {
            g_sys_dvfs_time_ms = 0;
        }

        // do gpu dvfs
        if (MTKGpuDVFSPolicy(gpu_loading, &ui32NewFreqID))
        {
            MTKDoGpuDVFS(ui32NewFreqID, gpu_dvfs_force_idle == 0 ? IMG_FALSE : IMG_TRUE);
        }

        gpu_pre_loading = gpu_loading;

        OSLockRelease(ghDVFSLock);
    }
}

void MTKMFGEnableDVFSTimer(bool bEnable)
{
    if (gpu_debug_enable)
    {
        PVR_DPF((PVR_DBG_ERROR, "MTKMFGEnableDVFSTimer: %s", bEnable ? "yes" : "no"));
    }

    if (NULL == g_hDVFSTimer)
    {
        PVR_DPF((PVR_DBG_ERROR, "MTKMFGEnableDVFSTimer: g_hDVFSTimer is NULL"));
        return;
    }

    OSLockAcquire(ghDVFSTimerLock);

    if (bEnable)
    {
        if (!g_bTimerEnable)
        {
            if (PVRSRV_OK == OSEnableTimer(g_hDVFSTimer))
            {
                g_bTimerEnable = IMG_TRUE;
            }
        }
    }
    else
    {
        if (g_bTimerEnable)
        {
            if (PVRSRV_OK == OSDisableTimer(g_hDVFSTimer))
            {
                g_bTimerEnable = IMG_FALSE;
            }
        }
    }

    OSLockRelease(ghDVFSTimerLock);
}

static bool MTKCheckDeviceInit(void)
{
    PVRSRV_DEVICE_NODE* psDevNode = MTKGetRGXDevNode();
    bool ret = false;
    
    if(psDevNode)
    {
        if(psDevNode->eDevState == PVRSRV_DEVICE_STATE_ACTIVE)
            ret = true;
    }
    return ret;
}


PVRSRV_ERROR MTKDevPrePowerState(IMG_HANDLE hSysData, PVRSRV_DEV_POWER_STATE eNewPowerState,
                                         PVRSRV_DEV_POWER_STATE eCurrentPowerState,
									     IMG_BOOL bForced)
{
    if( PVRSRV_DEV_POWER_STATE_OFF == eNewPowerState &&
        PVRSRV_DEV_POWER_STATE_ON == eCurrentPowerState )
    {
#ifndef ENABLE_COMMON_DVFS
        if (g_hDVFSTimer && g_bDeviceInit)
#else
        if (g_bDeviceInit)
#endif
        {
            g_bExit = IMG_TRUE;

#ifdef MTK_CAL_POWER_INDEX
            MTKStopPowerIndex();
#endif
        }
        else
        {
            g_bDeviceInit = MTKCheckDeviceInit();
        }

#if defined(MTK_USE_HW_APM) && defined(CONFIG_ARCH_MT6795)
        MTKDeInitHWAPM();
#endif
        MTKDisableMfgClock(IMG_FALSE);
    }
	return PVRSRV_OK;
}

PVRSRV_ERROR MTKDevPostPowerState(IMG_HANDLE hSysData, PVRSRV_DEV_POWER_STATE eNewPowerState,
                                          PVRSRV_DEV_POWER_STATE eCurrentPowerState,
									      IMG_BOOL bForced)
{
    if( PVRSRV_DEV_POWER_STATE_OFF == eCurrentPowerState &&
        PVRSRV_DEV_POWER_STATE_ON == eNewPowerState)
    {
        MTKEnableMfgClock();

#if defined(MTK_USE_HW_APM) && defined(CONFIG_ARCH_MT6795)
        MTKInitHWAPM();
#endif
#ifndef ENABLE_COMMON_DVFS        
        if (g_hDVFSTimer && g_bDeviceInit)
#else            
        if( g_bDeviceInit )
#endif            
        {
#ifdef MTK_CAL_POWER_INDEX
            MTKStartPowerIndex();
#endif
            g_bExit = IMG_FALSE;
        }
        else
        {
            g_bDeviceInit = MTKCheckDeviceInit();
        }
#if 0
        if (g_iSkipCount > 0)
        {
            // During boot up
            unsigned int ui32NewFreqID = mt_gpufreq_get_dvfs_table_num() - 1;
            unsigned int ui32CurFreqID = mt_gpufreq_get_cur_freq_index();
            if (ui32NewFreqID != ui32CurFreqID)
            {
                IMG_UINT32 ui32RGXDevIdx = MTKGetRGXDevIdx();
                unsigned int ui32GPUFreq = mt_gpufreq_get_frequency_by_level(ui32NewFreqID);
                mt_gpufreq_target(ui32NewFreqID);
                MTKWriteBackFreqToRGX(ui32RGXDevIdx, ui32GPUFreq);
            }
        }
#endif

		if(IMG_TRUE == g_bUnsync)
		{
			mt_gpufreq_target(g_ui32_unsync_freq_id);
			g_bUnsync = IMG_FALSE;
		}
      
    }
    return PVRSRV_OK;
}

PVRSRV_ERROR MTKSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	if(PVRSRV_SYS_POWER_STATE_OFF == eNewPowerState)
    {
		;
    }

	return PVRSRV_OK;
}

PVRSRV_ERROR MTKSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	if(PVRSRV_SYS_POWER_STATE_ON == eNewPowerState)
	{
    }

	return PVRSRV_OK;
}

static void MTKBoostGpuFreq(void)
{
    if (gpu_debug_enable)
    {
        PVR_DPF((PVR_DBG_ERROR, "MTKBoostGpuFreq"));
    }
    MTKFreqInputBoostCB(0);
}

static void MTKSetBottomGPUFreq(unsigned int ui32FreqLevel)
{
    unsigned int ui32MaxLevel;

    if (gpu_debug_enable)
    {
        PVR_DPF((PVR_DBG_ERROR, "MTKSetBottomGPUFreq: freq = %d", ui32FreqLevel));
    }

    ui32MaxLevel = mt_gpufreq_get_dvfs_table_num() - 1;
    if (ui32MaxLevel < ui32FreqLevel)
    {
        ui32FreqLevel = ui32MaxLevel;
    }

    OSLockAcquire(ghDVFSLock);

    // 0 => The highest frequency
    // table_num - 1 => The lowest frequency
    g_bottom_freq_id = ui32MaxLevel - ui32FreqLevel;
    gpu_bottom_freq = mt_gpufreq_get_frequency_by_level(g_bottom_freq_id);

    if (g_bottom_freq_id < mt_gpufreq_get_cur_freq_index())
    {
        MTKDoGpuDVFS(g_bottom_freq_id, gpu_dvfs_cb_force_idle == 0 ? IMG_FALSE : IMG_TRUE);
    }
     
    OSLockRelease(ghDVFSLock);

}

static unsigned int MTKCustomGetGpuFreqLevelCount(void)
{
    return mt_gpufreq_get_dvfs_table_num();
}

static void MTKCustomBoostGpuFreq(unsigned int ui32FreqLevel)
{
    unsigned int ui32MaxLevel;

    if (gpu_debug_enable)
    {
        PVR_DPF((PVR_DBG_ERROR, "MTKCustomBoostGpuFreq: freq = %d", ui32FreqLevel));
    }

    ui32MaxLevel = mt_gpufreq_get_dvfs_table_num() - 1;
    if (ui32MaxLevel < ui32FreqLevel)
    {
        ui32FreqLevel = ui32MaxLevel;
    }

    OSLockAcquire(ghDVFSLock);

    // 0 => The highest frequency
    // table_num - 1 => The lowest frequency
    g_cust_boost_freq_id = ui32MaxLevel - ui32FreqLevel;
    gpu_cust_boost_freq = mt_gpufreq_get_frequency_by_level(g_cust_boost_freq_id);

    if (g_cust_boost_freq_id < mt_gpufreq_get_cur_freq_index())
    {
        MTKDoGpuDVFS(g_cust_boost_freq_id, gpu_dvfs_cb_force_idle == 0 ? IMG_FALSE : IMG_TRUE);
    }

    OSLockRelease(ghDVFSLock);
}

static void MTKCustomUpBoundGpuFreq(unsigned int ui32FreqLevel)
{
    unsigned int ui32MaxLevel;

    if (gpu_debug_enable)
    {
        PVR_DPF((PVR_DBG_ERROR, "MTKCustomUpBoundGpuFreq: freq = %d", ui32FreqLevel));
    }

    ui32MaxLevel = mt_gpufreq_get_dvfs_table_num() - 1;
    if (ui32MaxLevel < ui32FreqLevel)
    {
        ui32FreqLevel = ui32MaxLevel;
    }

    OSLockAcquire(ghDVFSLock);

    // 0 => The highest frequency
    // table_num - 1 => The lowest frequency
    g_cust_upbound_freq_id = ui32MaxLevel - ui32FreqLevel;
    gpu_cust_upbound_freq = mt_gpufreq_get_frequency_by_level(g_cust_upbound_freq_id);

    if (g_cust_upbound_freq_id > mt_gpufreq_get_cur_freq_index())
    {
        MTKDoGpuDVFS(g_cust_upbound_freq_id, gpu_dvfs_cb_force_idle == 0 ? IMG_FALSE : IMG_TRUE);
    }
     
    OSLockRelease(ghDVFSLock);
}

unsigned int MTKGetCustomBoostGpuFreq(void)
{
    unsigned int ui32MaxLevel = mt_gpufreq_get_dvfs_table_num() - 1;
    return ui32MaxLevel - g_cust_boost_freq_id;
}

unsigned int MTKGetCustomUpBoundGpuFreq(void)
{
    unsigned int ui32MaxLevel = mt_gpufreq_get_dvfs_table_num() - 1;
    return ui32MaxLevel - g_cust_upbound_freq_id;
}

static IMG_UINT32 MTKGetGpuLoading(IMG_VOID)
{
    return gpu_loading;
}

static IMG_UINT32 MTKGetGpuBlock(IMG_VOID)
{
    return gpu_block;
}

static IMG_UINT32 MTKGetGpuIdle(IMG_VOID)
{
    return gpu_idle;
}

static IMG_UINT32 MTKGetPowerIndex(IMG_VOID)
{
    return gpu_power;
}

typedef void (*gpufreq_input_boost_notify)(unsigned int );
typedef void (*gpufreq_power_limit_notify)(unsigned int );
extern void mt_gpufreq_input_boost_notify_registerCB(gpufreq_input_boost_notify pCB);
extern void mt_gpufreq_power_limit_notify_registerCB(gpufreq_power_limit_notify pCB);

extern unsigned int (*mtk_get_gpu_loading_fp)(void);
extern unsigned int (*mtk_get_gpu_block_fp)(void);
extern unsigned int (*mtk_get_gpu_idle_fp)(void);
extern unsigned int (*mtk_get_gpu_power_loading_fp)(void);
extern void (*mtk_enable_gpu_dvfs_timer_fp)(bool bEnable);
extern void (*mtk_boost_gpu_freq_fp)(void);
extern void (*mtk_set_bottom_gpu_freq_fp)(unsigned int);

extern unsigned int (*mtk_custom_get_gpu_freq_level_count_fp)(void);
extern void (*mtk_custom_boost_gpu_freq_fp)(unsigned int ui32FreqLevel);
extern void (*mtk_custom_upbound_gpu_freq_fp)(unsigned int ui32FreqLevel);
extern unsigned int (*mtk_get_custom_boost_gpu_freq_fp)(void);
extern unsigned int (*mtk_get_custom_upbound_gpu_freq_fp)(void);

// extern int* (*mtk_get_gpu_cur_owner_fp)(void);

extern void (*ged_dvfs_cal_gpu_utilization_fp)(unsigned int* pui32Loading , unsigned int* pui32Block,unsigned int* pui32Idle);
extern void (*ged_dvfs_gpu_freq_commit_fp)(unsigned long ui32NewFreqID, GED_DVFS_COMMIT_TYPE eCommitType, int* pbCommited);

#ifdef SUPPORT_PDVFS
static IMG_VOID MTKFakeGpuLoading(unsigned int* pui32Loading , unsigned int* pui32Block,unsigned int* pui32Idle)
{
    *pui32Loading = 0;
    *pui32Block = 0;
    *pui32Idle = 0;
}
#endif

PVRSRV_ERROR MTKMFGSystemInit(void)
{
    PVRSRV_ERROR error;
    
#ifdef MTK_GPU_DVFS
    gpu_dvfs_enable = 1;
#else
    gpu_dvfs_enable = 0;
#endif
    
#ifndef ENABLE_COMMON_DVFS      
	error = OSLockCreate(&ghDVFSLock, LOCK_TYPE_PASSIVE);
	if (error != PVRSRV_OK)
    {
        PVR_DPF((PVR_DBG_ERROR, "Create DVFS Lock Failed"));
        goto ERROR;
    }

	error = OSLockCreate(&ghDVFSTimerLock, LOCK_TYPE_PASSIVE);
	if (error != PVRSRV_OK)
    {
        PVR_DPF((PVR_DBG_ERROR, "Create DVFS Timer Lock Failed"));
        goto ERROR;
    }

    g_iSkipCount = MTK_DEFER_DVFS_WORK_MS / MTK_DVFS_SWITCH_INTERVAL_MS;

    g_hDVFSTimer = OSAddTimer(MTKDVFSTimerFuncCB, (IMG_VOID *)NULL, MTK_DVFS_SWITCH_INTERVAL_MS);
    if(!g_hDVFSTimer)
    {
    	PVR_DPF((PVR_DBG_ERROR, "Create DVFS Timer Failed"));
        goto ERROR;
    }

    if (PVRSRV_OK == OSEnableTimer(g_hDVFSTimer))
    {
        g_bTimerEnable = IMG_TRUE;
    }
    boost_gpu_enable = 1;

    g_sys_dvfs_time_ms = 0;

    g_bottom_freq_id = mt_gpufreq_get_dvfs_table_num() - 1;
    gpu_bottom_freq = mt_gpufreq_get_frequency_by_level(g_bottom_freq_id);

    g_cust_boost_freq_id = mt_gpufreq_get_dvfs_table_num() - 1;
    gpu_cust_boost_freq = mt_gpufreq_get_frequency_by_level(g_cust_boost_freq_id);

    g_cust_upbound_freq_id = 0;
    gpu_cust_upbound_freq = mt_gpufreq_get_frequency_by_level(g_cust_upbound_freq_id);

    gpu_debug_enable = 0;

    mt_gpufreq_input_boost_notify_registerCB(MTKFreqInputBoostCB);
    mt_gpufreq_power_limit_notify_registerCB(MTKFreqPowerLimitCB);

    mtk_boost_gpu_freq_fp = MTKBoostGpuFreq;

    mtk_set_bottom_gpu_freq_fp = MTKSetBottomGPUFreq;

    mtk_custom_get_gpu_freq_level_count_fp = MTKCustomGetGpuFreqLevelCount;

    mtk_custom_boost_gpu_freq_fp = MTKCustomBoostGpuFreq;

    mtk_custom_upbound_gpu_freq_fp = MTKCustomUpBoundGpuFreq;

    mtk_get_custom_boost_gpu_freq_fp = MTKGetCustomBoostGpuFreq;

    mtk_get_custom_upbound_gpu_freq_fp = MTKGetCustomUpBoundGpuFreq;

    mtk_get_gpu_power_loading_fp = MTKGetPowerIndex;

    mtk_get_gpu_loading_fp = MTKGetGpuLoading;
    mtk_get_gpu_block_fp = MTKGetGpuBlock;
    mtk_get_gpu_idle_fp = MTKGetGpuIdle;
#else
#ifdef SUPPORT_PDVFS
    ged_dvfs_cal_gpu_utilization_fp = MTKFakeGpuLoading; /* trun-off GED loading based DVFS */
#else    
    ged_dvfs_cal_gpu_utilization_fp = MTKCalGpuLoading;
    ged_dvfs_gpu_freq_commit_fp = MTKCommitFreqIdx;    
#endif
#endif    
	//mtk_get_gpu_cur_owner_fp = OSGetBridgeLockOwnerID;


#ifdef CONFIG_MTK_HIBERNATION
    register_swsusp_restore_noirq_func(ID_M_GPU, gpu_pm_restore_noirq, NULL);
#endif

#if defined(MTK_USE_HW_APM) && defined(CONFIG_ARCH_MT6795)
    if (!g_pvRegsKM)
    {
        PVRSRV_DEVICE_NODE* psDevNode = MTKGetRGXDevNode();
        if (psDevNode)
        {
            IMG_CPU_PHYADDR sRegsPBase;
            PVRSRV_RGXDEV_INFO* psDevInfo = psDevNode->pvDevice;
            PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;
            if (psDevConfig && (!g_pvRegsKM))
            {
                sRegsPBase = psDevConfig->sRegsCpuPBase;
                sRegsPBase.uiAddr += 0xfff000;
        	    g_pvRegsKM = OSMapPhysToLin(sRegsPBase, 0xFF, 0);
            }
        }
    }
#endif
    return PVRSRV_OK;

ERROR:

    MTKMFGSystemDeInit();

    return PVRSRV_ERROR_INIT_FAILURE;
}

IMG_VOID MTKMFGSystemDeInit(void)
{
#ifdef CONFIG_MTK_HIBERNATION
    unregister_swsusp_restore_noirq_func(ID_M_GPU);
#endif

    g_bExit = IMG_TRUE;

#ifndef ENABLE_COMMON_DVFS      
	if(g_hDVFSTimer)
	{
        OSDisableTimer(g_hDVFSTimer);
		OSRemoveTimer(g_hDVFSTimer);
		g_hDVFSTimer = IMG_NULL;
    }

    if (ghDVFSLock)
    {
        OSLockDestroy(ghDVFSLock);
        ghDVFSLock = NULL;
    }

    if (ghDVFSTimerLock)
    {
        OSLockDestroy(ghDVFSTimerLock);
        ghDVFSTimerLock = NULL;
    }
#endif    

#ifdef MTK_CAL_POWER_INDEX
    g_pvRegsBaseKM = NULL;
#endif

#if defined(MTK_USE_HW_APM) && defined(CONFIG_ARCH_MT6795)
    if (g_pvRegsKM)
    {
        OSUnMapPhysToLin(g_pvRegsKM, 0xFF, 0);
        g_pvRegsKM = NULL;
    }
#endif
}

#ifndef ENABLE_COMMON_DVFS  
module_param(gpu_loading, uint, 0644);
module_param(gpu_block, uint, 0644);
module_param(gpu_idle, uint, 0644);
module_param(gpu_dvfs_enable, uint, 0644);
module_param(boost_gpu_enable, uint, 0644);
module_param(gpu_dvfs_force_idle, uint, 0644);
module_param(gpu_dvfs_cb_force_idle, uint, 0644);
module_param(gpu_bottom_freq, uint, 0644);
module_param(gpu_cust_boost_freq, uint, 0644);
module_param(gpu_cust_upbound_freq, uint, 0644);
module_param(gpu_freq, uint, 0644);
#endif

module_param(gpu_power, uint, 0644);
module_param(gpu_debug_enable, uint, 0644);


#ifdef CONFIG_MTK_SEGMENT_TEST
module_param(efuse_mfg_enable, uint, 0644);
#endif
