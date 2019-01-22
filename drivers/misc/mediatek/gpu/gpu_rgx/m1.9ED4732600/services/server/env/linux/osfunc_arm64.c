/*************************************************************************/ /*!
@File
@Title          arm specific OS functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    OS functions who's implementation are processor specific
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
#include <linux/version.h>
#include <linux/cpumask.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>

#include "pvrsrv_error.h"
#include "img_types.h"
#include "osfunc.h"
#include "pvr_debug.h"

#if defined(CONFIG_OUTER_CACHE)
  /* If you encounter a 64-bit ARM system with an outer cache, you'll need
   * to add the necessary code to manage that cache.  See osfunc_arm.c	
   * for an example of how to do so.
   */
	#error "CONFIG_OUTER_CACHE not supported on arm64."
#endif

static void per_cpu_cache_flush(void *arg)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0))
	static IMG_BOOL bLog = IMG_TRUE;
	/*
	  Global d-cache flush support is now deprecated from Linux v4.2 onwards.
	  If you find that more time is spent maintaining the d-cache, implement
	  a global flush (i.e. set/way operation) assembly sequence here. The
	  code should iteratively maintain all cache levels from the L1 cache
	  to the last level cache for a single core, also the code should use
	  exclusively arm64 <DC CISW> machine instruction _only_ which cleans
	  and invalidates the data cache.
	*/
	if (bLog)
	{
		PVR_LOG(("No d-cache global flush implementation, using range flush for large buffers"));
		bLog = IMG_FALSE;
	}
#else
	flush_cache_all();
#endif
	PVR_UNREFERENCED_PARAMETER(arg);
}

static inline void FlushRange(void *pvRangeAddrStart,
							  void *pvRangeAddrEnd,
							  PVRSRV_CACHE_OP eCacheOp)
{
	IMG_UINT32 ui32CacheLineSize = OSCPUCacheAttributeSize(PVR_DCACHE_LINE_SIZE);
	IMG_BYTE *pbStart = pvRangeAddrStart;
	IMG_BYTE *pbEnd = pvRangeAddrEnd;
	IMG_BYTE *pbBase;

	/*
	  On arm64, the TRM states in D5.8.1 (data and unified caches) that if cache 
	  maintenance is performed on a memory location using a VA, the effect of
	  that cache maintenance is visible to all VA aliases of the physical memory
	  location. So here it's quicker to issue the machine cache maintenance
	  instruction directly without going via the Linux kernel DMA framework as
	  this is sufficient to maintain the CPU d-caches on arm64.
	 */
	pbEnd = (IMG_BYTE *) PVR_ALIGN((uintptr_t)pbEnd, (uintptr_t)ui32CacheLineSize);
	for (pbBase = pbStart; pbBase < pbEnd; pbBase += ui32CacheLineSize)
	{
		switch (eCacheOp)
		{
			case PVRSRV_CACHE_OP_CLEAN:
				asm volatile ("dc cvac, %0" :: "r" (pbBase));
				break;

			case PVRSRV_CACHE_OP_INVALIDATE:
				asm volatile ("dc ivac, %0" :: "r" (pbBase));
				break;

			case PVRSRV_CACHE_OP_FLUSH:
				asm volatile ("dc civac, %0" :: "r" (pbBase));
				break;

			default:
				PVR_DPF((PVR_DBG_ERROR,
						"%s: Cache maintenance operation type %d is invalid",
						__FUNCTION__, eCacheOp));
				break;
		}
	}
}

PVRSRV_ERROR OSCPUOperation(PVRSRV_CACHE_OP uiCacheOp)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	switch(uiCacheOp)
	{
		case PVRSRV_CACHE_OP_CLEAN:
		case PVRSRV_CACHE_OP_FLUSH:
		case PVRSRV_CACHE_OP_INVALIDATE:
			on_each_cpu(per_cpu_cache_flush, NULL, 1);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0))
			eError = PVRSRV_ERROR_NOT_IMPLEMENTED;
#endif
			break;

		case PVRSRV_CACHE_OP_NONE:
			break;

		default:
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Global cache operation type %d is invalid",
					__FUNCTION__, uiCacheOp));
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			PVR_ASSERT(0);
			break;
	}

	return eError;
}

void OSFlushCPUCacheRangeKM(PVRSRV_DEVICE_NODE *psDevNode,
							void *pvVirtStart,
							void *pvVirtEnd,
							IMG_CPU_PHYADDR sCPUPhysStart,
							IMG_CPU_PHYADDR sCPUPhysEnd)
{
	struct dma_map_ops *dma_ops;

	if (pvVirtStart)
	{
		FlushRange(pvVirtStart, pvVirtEnd, PVRSRV_CACHE_OP_FLUSH);
		return;
	}

	dma_ops = get_dma_ops(psDevNode->psDevConfig->pvOSDevice);
	dma_ops->sync_single_for_device(NULL, sCPUPhysStart.uiAddr, sCPUPhysEnd.uiAddr - sCPUPhysStart.uiAddr, DMA_TO_DEVICE);
	dma_ops->sync_single_for_cpu(NULL, sCPUPhysStart.uiAddr, sCPUPhysEnd.uiAddr - sCPUPhysStart.uiAddr, DMA_FROM_DEVICE);
}

void OSCleanCPUCacheRangeKM(PVRSRV_DEVICE_NODE *psDevNode,
							void *pvVirtStart,
							void *pvVirtEnd,
							IMG_CPU_PHYADDR sCPUPhysStart,
							IMG_CPU_PHYADDR sCPUPhysEnd)
{
	struct dma_map_ops *dma_ops;

	if (pvVirtStart)
	{
		FlushRange(pvVirtStart, pvVirtEnd, PVRSRV_CACHE_OP_FLUSH);
		return;
	}

	dma_ops = get_dma_ops(psDevNode->psDevConfig->pvOSDevice);
	dma_ops->sync_single_for_device(NULL, sCPUPhysStart.uiAddr, sCPUPhysEnd.uiAddr - sCPUPhysStart.uiAddr, DMA_TO_DEVICE);
}

void OSInvalidateCPUCacheRangeKM(PVRSRV_DEVICE_NODE *psDevNode,
								 void *pvVirtStart,
								 void *pvVirtEnd,
								 IMG_CPU_PHYADDR sCPUPhysStart,
								 IMG_CPU_PHYADDR sCPUPhysEnd)
{
	struct dma_map_ops *dma_ops;

	if (pvVirtStart)
	{
		FlushRange(pvVirtStart, pvVirtEnd, PVRSRV_CACHE_OP_FLUSH);
		return;
	}

	dma_ops = get_dma_ops(psDevNode->psDevConfig->pvOSDevice);
	dma_ops->sync_single_for_cpu(NULL, sCPUPhysStart.uiAddr, sCPUPhysEnd.uiAddr - sCPUPhysStart.uiAddr, DMA_FROM_DEVICE);
}

PVRSRV_CACHE_OP_ADDR_TYPE OSCPUCacheOpAddressType(void)
{
	return PVRSRV_CACHE_OP_ADDR_TYPE_PHYSICAL;
}

void OSUserModeAccessToPerfCountersEn(void)
{
	/* FIXME: implement similarly to __arm__ */
}
