/*************************************************************************/ /*!
@File           rgxhwperf_vz_guest.c
@Title          Guest RGX HW Performance implementation
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX Virtualization HW Performance stub (Guest drivers only)
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include "pvrsrv.h"
#include "rgxhwperf.h"
#include "rgxapi_km.h"
#include "rgx_hwperf_km.h"
#if defined(SUPPORT_GPUTRACE_EVENTS)
#include "pvr_gputrace.h"
#endif

/******************************************************************************
 *
 *****************************************************************************/
PVRSRV_ERROR RGXHWPerfDataStoreCB(PVRSRV_DEVICE_NODE *psDevInfo)
{
	/* Technically, there is no need for the watchdog thread
	   in guest drivers....., at least that is the theory 
	   for now so we ok calls into this function */
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	return PVRSRV_OK;
}

PVRSRV_ERROR RGXHWPerfInit(PVRSRV_DEVICE_NODE *psRgxDevNode)
{
	PVR_UNREFERENCED_PARAMETER(psRgxDevNode);
	return PVRSRV_OK;
}

PVRSRV_ERROR RGXHWPerfInitOnDemandResources(void)
{
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfHostInit(IMG_UINT32 ui32BufSizeKB)
{
	/* Though we ok this call, use case is
	   predicated on device flags which
	   guest drivers do not support */
	return PVRSRV_OK;
}

PVRSRV_ERROR RGXHWPerfHostInitOnDemandResources(void)
{
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

void RGXHWPerfHostDeInit(void)
{
	
}

void RGXHWPerfHostSetEventFilter(IMG_UINT32 ui32Filter)
{
	
}

void RGXHWPerfDeinit(void)
{

}

/******************************************************************************
 * RGX HW Performance Profiling Server API(s)
 *****************************************************************************/
PVRSRV_ERROR PVRSRVRGXCtrlHWPerfKM(
	CONNECTION_DATA         *psConnection,
	PVRSRV_DEVICE_NODE      *psDeviceNode,
	 RGX_HWPERF_STREAM_ID    eStreamId,
	IMG_BOOL                 bToggle,
	IMG_UINT64               ui64Mask)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(eStreamId);
	PVR_UNREFERENCED_PARAMETER(bToggle);
	PVR_UNREFERENCED_PARAMETER(ui64Mask);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR PVRSRVRGXConfigEnableHWPerfCountersKM(
	CONNECTION_DATA          * psConnection,
	PVRSRV_DEVICE_NODE       * psDeviceNode,
	IMG_UINT32                 ui32ArrayLen,
	RGX_HWPERF_CONFIG_CNTBLK * psBlockConfigs)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(ui32ArrayLen);
	PVR_UNREFERENCED_PARAMETER(psBlockConfigs);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR PVRSRVRGXConfigCustomCountersKM(
	CONNECTION_DATA             * psConnection,
	PVRSRV_DEVICE_NODE          * psDeviceNode,
	IMG_UINT16                    ui16CustomBlockID,
	IMG_UINT16                    ui16NumCustomCounters,
	IMG_UINT32                  * pui32CustomCounterIDs)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(ui16CustomBlockID);
	PVR_UNREFERENCED_PARAMETER(ui16NumCustomCounters);
	PVR_UNREFERENCED_PARAMETER(pui32CustomCounterIDs);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR PVRSRVRGXCtrlHWPerfCountersKM(
	CONNECTION_DATA             * psConnection,
	PVRSRV_DEVICE_NODE          * psDeviceNode,
	IMG_BOOL                      bEnable,
	IMG_UINT32                    ui32ArrayLen,
	IMG_UINT16                  * psBlockIDs)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(bEnable);
	PVR_UNREFERENCED_PARAMETER(ui32ArrayLen);
	PVR_UNREFERENCED_PARAMETER(psBlockIDs);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}


#if defined(SUPPORT_GPUTRACE_EVENTS)
PVRSRV_ERROR RGXHWPerfFTraceGPUEventsEnabledSet(IMG_BOOL bNewValue)
{
	PVR_UNREFERENCED_PARAMETER(bNewValue);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

void RGXHWPerfFTraceGPUEnqueueEvent(
	PVRSRV_RGXDEV_INFO *psDevInfo,
	IMG_UINT32 ui32ExternalJobRef, 
	IMG_UINT32 ui32InternalJobRef,
	RGX_HWPERF_KICK_TYPE eKickType)
{
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32ExternalJobRef);
	PVR_UNREFERENCED_PARAMETER(ui32InternalJobRef);
	PVR_UNREFERENCED_PARAMETER(eKickType);
}

PVRSRV_ERROR RGXHWPerfFTraceGPUInit(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

void RGXHWPerfFTraceGPUDeInit(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}
#endif /* SUPPORT_GPUTRACE_EVENTS */


PVRSRV_ERROR RGXHWPerfLazyConnect(IMG_HANDLE* phDevData)
{
	PVR_UNREFERENCED_PARAMETER(phDevData);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfGetFilter(IMG_HANDLE  hDevData,
								RGX_HWPERF_STREAM_ID eStreamId,
								IMG_UINT64 *ui64Filter)
{
	PVR_UNREFERENCED_PARAMETER(hDevData);
	PVR_UNREFERENCED_PARAMETER(eStreamId);
	PVR_UNREFERENCED_PARAMETER(ui64Filter);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfFreeConnection(IMG_HANDLE hDevData)
{
	PVR_UNREFERENCED_PARAMETER(hDevData);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfConnect(IMG_HANDLE* phDevData)
{
	PVR_UNREFERENCED_PARAMETER(phDevData);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfControl(
		IMG_HANDLE           hDevData,
		RGX_HWPERF_STREAM_ID eStreamId,
		IMG_BOOL             bToggle,
		IMG_UINT64           ui64Mask)
{
	PVR_UNREFERENCED_PARAMETER(hDevData);
	PVR_UNREFERENCED_PARAMETER(eStreamId);
	PVR_UNREFERENCED_PARAMETER(bToggle);
	PVR_UNREFERENCED_PARAMETER(ui64Mask);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfConfigureAndEnableCounters(
		IMG_HANDLE					hDevData,
		IMG_UINT32					ui32NumBlocks,
		RGX_HWPERF_CONFIG_CNTBLK*	asBlockConfigs)
{
	PVR_UNREFERENCED_PARAMETER(hDevData);
	PVR_UNREFERENCED_PARAMETER(ui32NumBlocks);
	PVR_UNREFERENCED_PARAMETER(asBlockConfigs);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfDisableCounters(
		IMG_HANDLE   hDevData,
		IMG_UINT32   ui32NumBlocks,
		IMG_UINT16*   aeBlockIDs)
{
	PVR_UNREFERENCED_PARAMETER(hDevData);
	PVR_UNREFERENCED_PARAMETER(ui32NumBlocks);
	PVR_UNREFERENCED_PARAMETER(aeBlockIDs);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfAcquireData(
		IMG_HANDLE  hDevData,
		RGX_HWPERF_STREAM_ID eStreamId,
		IMG_PBYTE*  ppBuf,
		IMG_UINT32* pui32BufLen)
{
	PVR_UNREFERENCED_PARAMETER(hDevData);
	PVR_UNREFERENCED_PARAMETER(eStreamId);
	PVR_UNREFERENCED_PARAMETER(ppBuf);
	PVR_UNREFERENCED_PARAMETER(pui32BufLen);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfReleaseData(IMG_HANDLE hDevData, RGX_HWPERF_STREAM_ID eStreamId)
{
	PVR_UNREFERENCED_PARAMETER(hDevData);
	PVR_UNREFERENCED_PARAMETER(eStreamId);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

PVRSRV_ERROR RGXHWPerfDisconnect(IMG_HANDLE hDevData)
{
	PVR_UNREFERENCED_PARAMETER(hDevData);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

/******************************************************************************
 End of file (rgxhwperf_vz_guest.c)
******************************************************************************/
