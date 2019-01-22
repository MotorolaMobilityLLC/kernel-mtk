/*************************************************************************/ /*!
@File           pvrsrv_virt_common.c
@Title          Core services virtualization functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Main GPU virtualization APIs for core services functions
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

#include "ra.h"
#include "pmr.h"
#include "log2.h"
#include "lists.h"
#include "pvrsrv.h"
#include "dllist.h"
#include "pdump_km.h"
#include "allocmem.h"
#include "rgxdebug.h"
#include "pvr_debug.h"
#include "devicemem.h"
#include "syscommon.h"
#include "pvrversion.h"
#include "pvrsrv_vz.h"
#include "physmem_lma.h"
#include "dma_support.h"
#include "vz_support.h"
#include "pvrsrv_device.h"
#include "physmem_osmem.h"
#include "connection_server.h"
#include "vz_physheap.h"


PVRSRV_ERROR IMG_CALLCONV PVRSRVVzDeviceCreate(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	IMG_UINT uiIdx;
	RA_BASE_T uBase;
	RA_LENGTH_T uSize;
	IMG_UINT64 ui64Size;
	PVRSRV_ERROR eError;
	PHYS_HEAP *psPhysHeap;
	IMG_CPU_PHYADDR sCpuPAddr;
	IMG_DEV_PHYADDR sDevPAddr;
	PHYS_HEAP_TYPE eHeapType;
	IMG_UINT32 ui32NumOfHeapRegions;

	/* First, register device GPU physical heap based on physheap config */
	psPhysHeap = psDeviceNode->apsPhysHeap[PVRSRV_DEVICE_PHYS_HEAP_GPU_LOCAL];
	ui32NumOfHeapRegions = PhysHeapNumberOfRegions(psPhysHeap);
	eHeapType = PhysHeapGetType(psPhysHeap);

	/* Normally, for GPU UMA physheap, use OS services but here we override this
	   if said physheap is DMA/UMA carve-out; for this create an RA to manage it */
	if (eHeapType == PHYS_HEAP_TYPE_UMA || eHeapType == PHYS_HEAP_TYPE_DMA)
	{
		if (ui32NumOfHeapRegions)
		{
			eError = PhysHeapRegionGetCpuPAddr(psPhysHeap, 0, &sCpuPAddr);
			if (eError != PVRSRV_OK)
			{
				PVR_ASSERT(IMG_FALSE);
				goto e0;
			}
	
			eError = PhysHeapRegionGetSize(psPhysHeap, 0, &ui64Size);
			if (eError != PVRSRV_OK)
			{
				PVR_ASSERT(IMG_FALSE);
				goto e0;
			}
	
			eError = PhysHeapRegionGetDevPAddr(psPhysHeap, 0, &sDevPAddr);
			if (eError != PVRSRV_OK)
			{
				PVR_ASSERT(IMG_FALSE);
				goto e0;
			}
		}
		else
		{
			sDevPAddr.uiAddr = (IMG_UINT64)0;
			sCpuPAddr.uiAddr = (IMG_UINT64)0;
			ui64Size = (IMG_UINT64)0;
		}

		if (sCpuPAddr.uiAddr && sDevPAddr.uiAddr && ui64Size)
		{
			psDeviceNode->ui32NumOfLocalMemArenas = ui32NumOfHeapRegions;
			PVR_ASSERT(ui32NumOfHeapRegions == 1);

			PVR_DPF((PVR_DBG_MESSAGE, "===== UMA (carve-out) memory, 1st phys heap (gpu)"));

			PVR_DPF((PVR_DBG_MESSAGE, "Creating RA for gpu memory 0x%016llx-0x%016llx",
					(IMG_UINT64) sCpuPAddr.uiAddr, sCpuPAddr.uiAddr + ui64Size - 1));

			uBase = sDevPAddr.uiAddr;
			uSize = (RA_LENGTH_T) ui64Size;
			PVR_ASSERT(uSize == ui64Size);

			psDeviceNode->apsLocalDevMemArenas = OSAllocMem(sizeof(RA_ARENA*));
			PVR_ASSERT(psDeviceNode->apsLocalDevMemArenas);
			psDeviceNode->apszRANames = OSAllocMem(sizeof(IMG_PCHAR));
			PVR_ASSERT(psDeviceNode->apszRANames);
			psDeviceNode->apszRANames[0] = OSAllocMem(PVRSRV_MAX_RA_NAME_LENGTH);
			PVR_ASSERT(psDeviceNode->apszRANames[0]);

			OSSNPrintf(psDeviceNode->apszRANames[0], PVRSRV_MAX_RA_NAME_LENGTH,
						"%s gpu mem", psDeviceNode->psDevConfig->pszName);
	
			psDeviceNode->apsLocalDevMemArenas[0] =
				RA_Create(psDeviceNode->apszRANames[0],
							OSGetPageShift(),	/* Use OS page size, keeps things simple */
							RA_LOCKCLASS_0,		/* This arena doesn't use any other arenas. */
							NULL,				/* No Import */
							NULL,				/* No free import */
							NULL,				/* No import handle */
							IMG_FALSE);
			if (psDeviceNode->apsLocalDevMemArenas[0] == NULL)
			{
				eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto e0;
			}
	
			if (!RA_Add(psDeviceNode->apsLocalDevMemArenas[0], uBase, uSize, 0 , NULL))
			{
				RA_Delete(psDeviceNode->apsLocalDevMemArenas[0]);
				eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto e0;
			}

			/* Replace the UMA allocator with LMA allocator */
			psDeviceNode->pfnDevPxAlloc = LMA_PhyContigPagesAlloc;
			psDeviceNode->pfnDevPxFree = LMA_PhyContigPagesFree;
			psDeviceNode->pfnDevPxMap = LMA_PhyContigPagesMap;
			psDeviceNode->pfnDevPxUnMap = LMA_PhyContigPagesUnmap;
			psDeviceNode->pfnDevPxClean = LMA_PhyContigPagesClean;
			psDeviceNode->uiMMUPxLog2AllocGran = OSGetPageShift();
			psDeviceNode->pfnCreateRamBackedPMR[PVRSRV_DEVICE_PHYS_HEAP_GPU_LOCAL] = PhysmemNewLocalRamBackedPMR;
		}
	}
	else
	{
		/* LMA heap sanity check */
		PVR_ASSERT(ui32NumOfHeapRegions);
	}

	/* Next, register device firmware physical heap based on heap config */
	psPhysHeap = psDeviceNode->apsPhysHeap[PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL];
	ui32NumOfHeapRegions = PhysHeapNumberOfRegions(psPhysHeap);
	eHeapType = PhysHeapGetType(psPhysHeap);

	PVR_ASSERT(eHeapType != PHYS_HEAP_TYPE_UNKNOWN);

	PVR_DPF((PVR_DBG_MESSAGE, "===== LMA/DMA/UMA (carve-out) memory, 2nd phys heap (fw)"));

	if (ui32NumOfHeapRegions)
	{
		eError = PhysHeapRegionGetCpuPAddr(psPhysHeap, 0, &sCpuPAddr);
		if (eError != PVRSRV_OK)
		{
			PVR_ASSERT(IMG_FALSE);
			goto e0;
		}
	
		eError = PhysHeapRegionGetSize(psPhysHeap, 0, &ui64Size);
		if (eError != PVRSRV_OK)
		{
			PVR_ASSERT(IMG_FALSE);
			goto e0;
		}
	
		eError = PhysHeapRegionGetDevPAddr(psPhysHeap, 0, &sDevPAddr);
		if (eError != PVRSRV_OK)
		{
			PVR_ASSERT(IMG_FALSE);
			goto e0;
		}
	}
	else
	{
		sDevPAddr.uiAddr = (IMG_UINT64)0;
		sCpuPAddr.uiAddr = (IMG_UINT64)0;
		ui64Size = (IMG_UINT64)0;
	}

	if (ui32NumOfHeapRegions)
	{
		PVRSRV_DEVICE_PHYS_HEAP_ORIGIN eHeapOrigin;

		SysVzGetPhysHeapOrigin(psDeviceNode->psDevConfig,
							   PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL,
							   &eHeapOrigin);

		PVR_ASSERT(sCpuPAddr.uiAddr && ui64Size);
		if (eHeapType != PHYS_HEAP_TYPE_LMA)
		{
			/* On some LMA config, fw base
			   starts at zero */
			PVR_ASSERT(sDevPAddr.uiAddr);
		}

		PVR_DPF((PVR_DBG_MESSAGE, "Creating RA for  fw memory 0x%016llx-0x%016llx",
				(IMG_UINT64) sCpuPAddr.uiAddr, sCpuPAddr.uiAddr + ui64Size - 1));

		/* Now we construct RA to manage FW heap */
		uBase = sDevPAddr.uiAddr;
		uSize = (RA_LENGTH_T) ui64Size;
		PVR_ASSERT(uSize == ui64Size);

		for (uiIdx = 0; uiIdx < RGXFW_NUM_OS; uiIdx++)
		{
			RA_BASE_T uOSIDBase = uBase + (uiIdx * ui64Size);

			OSSNPrintf(psDeviceNode->szKernelFwRAName[uiIdx], sizeof(psDeviceNode->szKernelFwRAName[uiIdx]),
						"%s fw mem", psDeviceNode->psDevConfig->pszName);

			psDeviceNode->psKernelFwMemArena[uiIdx] =
				RA_Create(psDeviceNode->szKernelFwRAName[uiIdx],
							OSGetPageShift(),	/* Use OS page size, keeps things simple */
							RA_LOCKCLASS_0,		/* This arena doesn't use any other arenas. */
							NULL,				/* No Import */
							NULL,				/* No free import */
							NULL,				/* No import handle */
							IMG_FALSE);
			if (psDeviceNode->psKernelFwMemArena[uiIdx] == NULL)
			{
				eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto e1;
			}

			if (!RA_Add(psDeviceNode->psKernelFwMemArena[uiIdx], uOSIDBase, uSize, 0 , NULL))
			{
				RA_Delete(psDeviceNode->psKernelFwMemArena[uiIdx]);
				eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto e1;
			}

			if (eHeapOrigin != PVRSRV_DEVICE_PHYS_HEAP_ORIGIN_HOST)
			{
				break;
			}
		}

		/* Fw physheap is always managed by LMA PMR factory */
		psDeviceNode->pfnCreateRamBackedPMR[PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL] = PhysmemNewLocalRamBackedPMR;
	}

	return PVRSRV_OK;
e1:
	PVRSRVVzDeviceDestroy(psDeviceNode);
e0:
	return eError;
}

PVRSRV_ERROR IMG_CALLCONV PVRSRVVzDeviceDestroy(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	IMG_UINT uiIdx;
	IMG_UINT64 ui64Size;
	PHYS_HEAP *psPhysHeap;
	IMG_CPU_PHYADDR sCpuPAddr;
	IMG_DEV_PHYADDR sDevPAddr;
	PHYS_HEAP_TYPE eHeapType;
	IMG_UINT32 ui32NumOfHeapRegions;
	PVRSRV_ERROR eError = PVRSRV_OK;

	/* First, unregister device firmware physical heap based on heap config */
	psPhysHeap = psDeviceNode->apsPhysHeap[PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL];
	ui32NumOfHeapRegions = PhysHeapNumberOfRegions(psPhysHeap);

	if (ui32NumOfHeapRegions)
	{
		for (uiIdx = 0; uiIdx < RGXFW_NUM_OS; uiIdx++)
		{
			if (psDeviceNode->psKernelFwMemArena[uiIdx])
			{
				RA_Delete(psDeviceNode->psKernelFwMemArena[uiIdx]);
				psDeviceNode->psKernelFwMemArena[uiIdx] = NULL;
			}
		}
	}

	/* Next, unregister device GPU physical heap based on heap config */
	psPhysHeap = psDeviceNode->apsPhysHeap[PVRSRV_DEVICE_PHYS_HEAP_GPU_LOCAL];
	ui32NumOfHeapRegions = PhysHeapNumberOfRegions(psPhysHeap);
	eHeapType = PhysHeapGetType(psPhysHeap);

	if (eHeapType == PHYS_HEAP_TYPE_UMA || eHeapType == PHYS_HEAP_TYPE_DMA)
	{
		if (ui32NumOfHeapRegions)
		{
			eError = PhysHeapRegionGetCpuPAddr(psPhysHeap, 0, &sCpuPAddr);
			if (eError != PVRSRV_OK)
			{
				PVR_ASSERT(IMG_FALSE);
				return eError;
			}
	
			eError = PhysHeapRegionGetSize(psPhysHeap, 0, &ui64Size);
			if (eError != PVRSRV_OK)
			{
				PVR_ASSERT(IMG_FALSE);
				return eError;
			}
	
			eError = PhysHeapRegionGetDevPAddr(psPhysHeap, 0, &sDevPAddr);
			if (eError != PVRSRV_OK)
			{
				PVR_ASSERT(IMG_FALSE);
				return eError;
			}
		}
		else
		{
			sDevPAddr.uiAddr = (IMG_UINT64)0;
			sCpuPAddr.uiAddr = (IMG_UINT64)0;
			ui64Size = (IMG_UINT64)0;
		}

		if (sCpuPAddr.uiAddr && sDevPAddr.uiAddr && ui64Size)
		{
			if (psDeviceNode->apszRANames && psDeviceNode->apsLocalDevMemArenas[0])
			{
				RA_Delete(psDeviceNode->apsLocalDevMemArenas[0]);
				OSFreeMem(psDeviceNode->apsLocalDevMemArenas);
				OSFreeMem(psDeviceNode->apszRANames[0]);
				OSFreeMem(psDeviceNode->apszRANames);
			}
		}
	}

	return eError;
}

/*****************************************************************************
 End of file (pvrsrv_virt_common.c)
*****************************************************************************/
