/*************************************************************************/ /*!
@File           vz_physheap_generic.c
@Title          System virtualization physheap configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System virtualization physical heap configuration
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

#if defined(CONFIG_L4)
static IMG_HANDLE gahPhysHeapIoRemap[PVRSRV_DEVICE_PHYS_HEAP_LAST];
#endif

static PVRSRV_ERROR
SysVzCreateDmaPhysHeap(PHYS_HEAP_CONFIG *psPhysHeapConfig)
{
	PVRSRV_ERROR eError;
	DMA_ALLOC *psDmaAlloc;
	PHYS_HEAP_REGION *psPhysHeapRegion;

	psPhysHeapRegion = &psPhysHeapConfig->pasRegions[0];
	PVR_ASSERT(psPhysHeapRegion->hPrivData != NULL);

	psDmaAlloc = (DMA_ALLOC*)psPhysHeapRegion->hPrivData;
	psDmaAlloc->ui64Size = psPhysHeapRegion->uiSize;

	eError = SysDmaAllocMem(psDmaAlloc);
	if (eError == PVRSRV_OK)
	{
		psPhysHeapRegion->sStartAddr.uiAddr = psDmaAlloc->sBusAddr.uiAddr;
		psPhysHeapRegion->sCardBase.uiAddr = psDmaAlloc->sBusAddr.uiAddr;
		psPhysHeapConfig->eType = PHYS_HEAP_TYPE_DMA;
	}
	else
	{
		psPhysHeapConfig->eType = PHYS_HEAP_TYPE_UMA;
	}

	return eError;
}

static void
SysVzDestroyDmaPhysHeap(PHYS_HEAP_CONFIG *psPhysHeapConfig)
{
	DMA_ALLOC *psDmaAlloc;
	PHYS_HEAP_REGION *psPhysHeapRegion;

	psPhysHeapRegion = &psPhysHeapConfig->pasRegions[0];
	psDmaAlloc = (DMA_ALLOC*)psPhysHeapRegion->hPrivData;

	if (psDmaAlloc != NULL)
	{
		PVR_ASSERT(psPhysHeapRegion->sStartAddr.uiAddr);
		PVR_ASSERT(psPhysHeapRegion->sCardBase.uiAddr);	
		PVR_ASSERT(psPhysHeapRegion->uiSize);

		SysDmaFreeMem(psDmaAlloc);

		psPhysHeapRegion->sCardBase.uiAddr = 0;	
		psPhysHeapRegion->sStartAddr.uiAddr = 0;
		psPhysHeapConfig->eType = PHYS_HEAP_TYPE_UMA;
	}
}

static PVRSRV_ERROR
SysVzCreatePhysHeap(PVRSRV_DEVICE_CONFIG *psDevConfig,
					PVRSRV_DEVICE_PHYS_HEAP ePhysHeap)
{
	IMG_DEV_PHYADDR sHeapAddr;
	IMG_UINT64 ui64HeapSize = 0;
	PVRSRV_ERROR eError = PVRSRV_OK;
	PHYS_HEAP_REGION *psPhysHeapRegion;
	PHYS_HEAP_CONFIG *psPhysHeapConfig;
	PVRSRV_DEVICE_PHYS_HEAP_ORIGIN eHeapOrigin;

	/* Lookup GPU/FW physical heap config, allocate primary region */
	psPhysHeapConfig = SysVzGetPhysHeapConfig(psDevConfig, ePhysHeap);
	PVR_ASSERT (psPhysHeapConfig != NULL);

	if (psPhysHeapConfig->pasRegions == NULL)
	{
		psPhysHeapConfig->pasRegions = OSAllocZMem(sizeof(PHYS_HEAP_REGION));
		if (psPhysHeapConfig->pasRegions == NULL)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			PVR_ASSERT(0);
			goto e0;
		}

		psPhysHeapConfig->pasRegions[0].bDynAlloc = IMG_TRUE;
		psPhysHeapConfig->ui32NumOfRegions++;
	}
	else
	{
		psPhysHeapConfig->pasRegions[0].bDynAlloc = IMG_FALSE;
	}

	if (psPhysHeapConfig->pasRegions[0].hPrivData == NULL)
	{
		DMA_ALLOC *psDmaAlloc = OSAllocZMem(sizeof(DMA_ALLOC));
		if (psDmaAlloc == NULL)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			PVR_ASSERT(0);
			goto e0;
		}

		psDmaAlloc->pvOSDevice = psDevConfig->pvOSDevice;
		psPhysHeapConfig->pasRegions[0].hPrivData = psDmaAlloc;
	}

	/* Lookup physheap addr/size from VM manager type */
	eError = SysVzGetPhysHeapAddrSize(psDevConfig,
									  ePhysHeap,
									  PHYS_HEAP_TYPE_UMA,
								 	  &sHeapAddr,
								 	  &ui64HeapSize);
	PVR_ASSERT(eError == PVRSRV_OK);

	/* Initialise physical heap and region state */
	psPhysHeapRegion = &psPhysHeapConfig->pasRegions[0];
	psPhysHeapRegion->sStartAddr.uiAddr = sHeapAddr.uiAddr;
	psPhysHeapRegion->sCardBase.uiAddr = sHeapAddr.uiAddr;
	psPhysHeapRegion->uiSize = ui64HeapSize;

	if (ePhysHeap == PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL)
	{
		/* Firmware physheaps require additional init */
		psPhysHeapConfig->pszPDumpMemspaceName = "SYSMEM";
		psPhysHeapConfig->psMemFuncs =
				psDevConfig->pasPhysHeaps[0].psMemFuncs;
	}

	/* Which driver is responsible for allocating the
	   physical memory backing the device physheap */
	eError = SysVzGetPhysHeapOrigin(psDevConfig,
									ePhysHeap,
									&eHeapOrigin);
	PVR_ASSERT(eError == PVRSRV_OK);

	if (psPhysHeapRegion->sStartAddr.uiAddr == 0)
	{
		if (psPhysHeapRegion->uiSize)
		{
			if (eHeapOrigin == PVRSRV_DEVICE_PHYS_HEAP_ORIGIN_HOST)
			{
				/* Scale DMA size by the number of OSIDs */
				psPhysHeapRegion->uiSize *= RGXFW_NUM_OS;
			}

			eError = SysVzCreateDmaPhysHeap(psPhysHeapConfig);
			if (eError != PVRSRV_OK)
			{
				eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				PVR_ASSERT(0);
				goto e0;
			}

			/* Verify the validity of DMA physheap region */
			PVR_ASSERT(psPhysHeapRegion->sStartAddr.uiAddr);
			PVR_ASSERT(psPhysHeapRegion->sCardBase.uiAddr);
			PVR_ASSERT(psPhysHeapRegion->uiSize);

			/* Services managed DMA physheap setup complete */
			psPhysHeapConfig->eType = PHYS_HEAP_TYPE_DMA;

			/* Only the PHYS_HEAP_TYPE_DMA should be registered */
			eError = SysVzRegisterPhysHeap(psDevConfig, ePhysHeap);
			if (eError != PVRSRV_OK)
			{
				PVR_ASSERT(0);
				goto e0;
			}

			if (eHeapOrigin == PVRSRV_DEVICE_PHYS_HEAP_ORIGIN_HOST)
			{
				/* Restore original physheap size */
				psPhysHeapRegion->uiSize /= RGXFW_NUM_OS;
			}
		}
		else
		{
			if (psPhysHeapConfig->pasRegions[0].hPrivData)
			{
				OSFreeMem(psPhysHeapConfig->pasRegions[0].hPrivData);
				psPhysHeapConfig->pasRegions[0].hPrivData = NULL;
			}

			if (psPhysHeapConfig->pasRegions[0].bDynAlloc)
			{
				OSFreeMem(psPhysHeapConfig->pasRegions);
				psPhysHeapConfig->pasRegions = NULL;
				psPhysHeapConfig->ui32NumOfRegions--;
				PVR_ASSERT(psPhysHeapConfig->ui32NumOfRegions == 0);
			}

			/* Kernel managed UMA physheap setup complete */
			psPhysHeapConfig->eType = PHYS_HEAP_TYPE_UMA;
		}
	}
	else
	{
		/* Verify the validity of carve-out physical heap region */
		PVR_ASSERT(psPhysHeapConfig->pasRegions[0].hPrivData != NULL);
		PVR_ASSERT(psPhysHeapConfig->pasRegions != NULL);
		PVR_ASSERT(psPhysHeapRegion->uiSize);

#if defined(CONFIG_L4)
		{
			IMG_UINT64 ui64Offset;
			IMG_UINT64 ui64BaseAddr;
			IMG_CPU_VIRTADDR pvCpuVAddr;
			PVR_ASSERT(psPhysHeapRegion->uiSize);

			/* On Fiasco.OC/l4linux, ioremap physheap now (might fail) */
			gahPhysHeapIoRemap[ePhysHeap] = 
							OSMapPhysToLin(psPhysHeapRegion->sStartAddr,
										   psPhysHeapRegion->uiSize,
										   PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
			PVR_ASSERT(gahPhysHeapIoRemap[ePhysHeap] != NULL);

			for (ui64Offset = 0;
				 ui64Offset < psPhysHeapRegion->uiSize;
				 ui64Offset += (IMG_UINT64)OSGetPageSize())
			{
				/* Pre-fault-in all physheap pages into l4linux address space,
				   this avoids having to pre-fault these before mapping into
				   an application address space during OSMMapPMRGeneric() call */
				ui64BaseAddr = psPhysHeapRegion->sStartAddr.uiAddr + ui64Offset;
				pvCpuVAddr = l4x_phys_to_virt(ui64BaseAddr);

				/* We need to ensure the compiler does not optimise this out */
				*((volatile int*)pvCpuVAddr) = *((volatile int*)pvCpuVAddr);
			}
		}
#endif

		/* Services managed UMA carve-out physheap 
		   setup complete */
		psPhysHeapConfig->eType = PHYS_HEAP_TYPE_UMA;
	}

	return eError;

e0:
	if (psPhysHeapConfig->pasRegions)
	{
		SysVzDeregisterPhysHeap(psDevConfig, ePhysHeap);

		if (psPhysHeapConfig->pasRegions[0].hPrivData)
		{
			OSFreeMem(psPhysHeapConfig->pasRegions[0].hPrivData);
			psPhysHeapConfig->pasRegions[0].hPrivData = NULL;
		}

		if (psPhysHeapConfig->pasRegions[0].bDynAlloc)
		{
			OSFreeMem(psPhysHeapConfig->pasRegions);
			psPhysHeapConfig->pasRegions = NULL;
			psPhysHeapConfig->ui32NumOfRegions--;
			PVR_ASSERT(psPhysHeapConfig->ui32NumOfRegions == 0);
		}
	}

	PVR_ASSERT(0);
	return  eError;
}

static void
SysVzDestroyPhysHeap(PVRSRV_DEVICE_CONFIG *psDevConfig,
					 PVRSRV_DEVICE_PHYS_HEAP ePhysHeap)
{
	PHYS_HEAP_CONFIG *psPhysHeapConfig;

	SysVzDeregisterPhysHeap(psDevConfig, ePhysHeap);

	psPhysHeapConfig = SysVzGetPhysHeapConfig(psDevConfig, ePhysHeap);
	if (psPhysHeapConfig == NULL || 
		psPhysHeapConfig->pasRegions == NULL)
	{
		return;
	}

#if defined(CONFIG_L4)
	if (gahPhysHeapIoRemap[ePhysHeap] != NULL)
	{
		OSUnMapPhysToLin(gahPhysHeapIoRemap[ePhysHeap],
						psPhysHeapConfig->pasRegions[0].uiSize,
						PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
	}

	gahPhysHeapIoRemap[ePhysHeap] = NULL;
#endif

	if (psPhysHeapConfig->pasRegions[0].hPrivData)
	{
		SysVzDestroyDmaPhysHeap(psPhysHeapConfig);
		OSFreeMem(psPhysHeapConfig->pasRegions[0].hPrivData);
		psPhysHeapConfig->pasRegions[0].hPrivData = NULL;
	}

	if (psPhysHeapConfig->pasRegions[0].bDynAlloc)
	{
		OSFreeMem(psPhysHeapConfig->pasRegions);
		psPhysHeapConfig->pasRegions = NULL;
		psPhysHeapConfig->ui32NumOfRegions--;
		PVR_ASSERT(psPhysHeapConfig->ui32NumOfRegions == 0);
	}
}

static PVRSRV_ERROR
SysVzCreateGpuPhysHeap(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_DEVICE_PHYS_HEAP eHeap = PVRSRV_DEVICE_PHYS_HEAP_GPU_LOCAL;
	return SysVzCreatePhysHeap(psDevConfig, eHeap);
}

static void
SysVzDestroyGpuPhysHeap(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_DEVICE_PHYS_HEAP eHeap = PVRSRV_DEVICE_PHYS_HEAP_GPU_LOCAL;
	SysVzDestroyPhysHeap(psDevConfig, eHeap);
}

static PVRSRV_ERROR
SysVzCreateFwPhysHeap(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_DEVICE_PHYS_HEAP eHeap = PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL;
	return SysVzCreatePhysHeap(psDevConfig, eHeap);
}

static void
SysVzDestroyFwPhysHeap(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_DEVICE_PHYS_HEAP eHeap = PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL;
	SysVzDestroyPhysHeap(psDevConfig, eHeap);
}

PHYS_HEAP_TYPE SysVzGetMemoryConfigPhysHeapType(void)
{
	return PHYS_HEAP_TYPE_UMA;
}

PVRSRV_ERROR SysVzInitDevPhysHeaps(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PVRSRV_ERROR eError;

	eError = SysVzCreateFwPhysHeap(psDevConfig);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	eError = SysVzCreateGpuPhysHeap(psDevConfig);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	return eError;
}

void SysVzDeInitDevPhysHeaps(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	SysVzDestroyGpuPhysHeap(psDevConfig);
	SysVzDestroyFwPhysHeap(psDevConfig);
}

/******************************************************************************
 End of file (vz_physheap_generic.c)
******************************************************************************/
