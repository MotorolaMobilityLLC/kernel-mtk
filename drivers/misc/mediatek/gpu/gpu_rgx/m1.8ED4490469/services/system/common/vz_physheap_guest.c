/*************************************************************************/ /*!
@File           vz_physheap_guest.c
@Title          System virtualization guest physheap configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System virtualization guest physical heap configuration
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
#include "allocmem.h"
#include "physheap.h"
#include "rgxdevice.h"
#include "pvrsrv_device.h"
#include "rgxfwutils_vz.h"

#include "dma_support.h"
#include "vz_support.h"
#include "vz_vmm_pvz.h"
#include "vz_physheap.h"
#include "vmm_impl_client.h"
#include "vmm_pvz_client.h"
#if defined(PVRSRV_GPUVIRT_MULTIDRV_MODEL)
#include "vmm_pvz_mdm.h"
#endif

PVRSRV_ERROR
SysVzCreateDevPhysHeaps(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_ERROR eError;

	eError = PvzClientCreateDevPhysHeaps(psDevConfig, 0);
	PVR_ASSERT(eError == PVRSRV_OK);

#if defined(PVRSRV_GPUVIRT_MULTIDRV_MODEL)
	eError = PvzClientCreateDevPhysHeaps2(psDevConfig, 0);
	PVR_ASSERT(eError == PVRSRV_OK);
#endif

	return eError;
}

void
SysVzDestroyDevPhysHeaps(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
#if defined(PVRSRV_GPUVIRT_MULTIDRV_MODEL)
	PvzClientDestroyDevPhysHeaps2(psDevConfig, 0);
#endif

	PvzClientDestroyDevPhysHeaps(psDevConfig, 0);
}

PVRSRV_ERROR
SysVzRegisterFwPhysHeap(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_ERROR eError;
	PVRSRV_DEVICE_PHYS_HEAP_ORIGIN eHeapOrigin;
	PVRSRV_DEVICE_PHYS_HEAP eHeap = PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL;

	eError = SysVzGetPhysHeapOrigin(psDevConfig,
									eHeap,
									&eHeapOrigin);
	PVR_ASSERT(eError == PVRSRV_OK);

	if (eHeapOrigin == PVRSRV_DEVICE_PHYS_HEAP_ORIGIN_HOST)
	{
		eError = PVRSRV_OK;
	}
	else
	{
		PHYS_HEAP_CONFIG *psPhysHeapConfig;
		IMG_DEV_PHYADDR sDevPAddr;
		IMG_UINT64 ui64DevPSize;

		psPhysHeapConfig = SysVzGetPhysHeapConfig(psDevConfig, eHeap);
		PVR_ASSERT(psPhysHeapConfig && psPhysHeapConfig->pasRegions);

		sDevPAddr.uiAddr = psPhysHeapConfig->pasRegions[0].sStartAddr.uiAddr;
		ui64DevPSize = psPhysHeapConfig->pasRegions[0].uiSize;
		PVR_ASSERT(sDevPAddr.uiAddr && ui64DevPSize);

		eError = PvzClientMapDevPhysHeap(psDevConfig,
										 0,
										 sDevPAddr,
										 ui64DevPSize);
		PVR_ASSERT(eError == PVRSRV_OK);
	}

	return eError;
}

PVRSRV_ERROR
SysVzUnregisterFwPhysHeap(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_ERROR eError;
	PVRSRV_DEVICE_PHYS_HEAP_ORIGIN eHeapOrigin;
	PVRSRV_DEVICE_PHYS_HEAP eHeapType = PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL;

	eError = SysVzGetPhysHeapOrigin(psDevConfig,
									eHeapType,
									&eHeapOrigin);
	PVR_ASSERT(eError == PVRSRV_OK);

	if (eHeapOrigin == PVRSRV_DEVICE_PHYS_HEAP_ORIGIN_HOST)
	{
		eError = PVRSRV_OK;
	}
	else
	{
		eError = PvzClientUnmapDevPhysHeap(psDevConfig, 0);
		PVR_ASSERT(eError == PVRSRV_OK);
	}

	return eError;
}

PVRSRV_ERROR SysVzGetPhysHeapAddrSize(PVRSRV_DEVICE_CONFIG *psDevConfig,
									  PVRSRV_DEVICE_PHYS_HEAP ePhysHeap,
									  PHYS_HEAP_TYPE eHeapType,
									  IMG_DEV_PHYADDR *psAddr,
									  IMG_UINT64 *pui64Size)
{
	IMG_UINT64 uiAddr;
	PVRSRV_ERROR eError;
	VMM_PVZ_CONNECTION *psVmmPvz;

	PVR_UNREFERENCED_PARAMETER(eHeapType);

	psVmmPvz = SysVzPvzConnectionAcquire();
	PVR_ASSERT(psVmmPvz);

	PVR_ASSERT(psVmmPvz->sConfigFuncTab.pfnGetDevPhysHeapAddrSize);

	eError = psVmmPvz->sConfigFuncTab.pfnGetDevPhysHeapAddrSize(psDevConfig,
																ePhysHeap,
																pui64Size,
																&uiAddr);
	if (eError != PVRSRV_OK)
	{
		if (eError == PVRSRV_ERROR_NOT_IMPLEMENTED)
		{
			PVR_ASSERT(0);
		}

		goto e0;
	}

	psAddr->uiAddr = uiAddr;
e0:
	SysVzPvzConnectionRelease(psVmmPvz);
	return eError;
}

PVRSRV_ERROR SysVzGetPhysHeapOrigin(PVRSRV_DEVICE_CONFIG *psDevConfig,
									PVRSRV_DEVICE_PHYS_HEAP eHeap,
									PVRSRV_DEVICE_PHYS_HEAP_ORIGIN *peOrigin)
{
	PVRSRV_ERROR eError;
	VMM_PVZ_CONNECTION *psVmmPvz;

	psVmmPvz = SysVzPvzConnectionAcquire();
	PVR_ASSERT(psVmmPvz);

	PVR_ASSERT(psVmmPvz->sConfigFuncTab.pfnGetDevPhysHeapOrigin);

	eError = psVmmPvz->sConfigFuncTab.pfnGetDevPhysHeapOrigin(psDevConfig,
															  eHeap,
															  peOrigin);
	if (eError != PVRSRV_OK)
	{
		if (eError == PVRSRV_ERROR_NOT_IMPLEMENTED)
		{
			PVR_ASSERT(0);
		}

		goto e0;
	}

e0:
	SysVzPvzConnectionRelease(psVmmPvz);
	return eError;
}

/******************************************************************************
 End of file (vz_physheap_guest.c)
******************************************************************************/
