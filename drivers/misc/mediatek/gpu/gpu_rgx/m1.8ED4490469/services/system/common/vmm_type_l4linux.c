/*************************************************************************/ /*!
@File			vmm_type_l4linux.c
@Title          Fiasco.OC L4LINUX VM manager type
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Fiasco.OC L4LINUX VM manager implementation
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
#include "img_types.h"
#include "pvrsrv_error.h"
#include "rgxheapconfig.h"
#include "interrupt_support.h"

#include "pvrsrv_vz.h"
#include "vz_physheap.h"

#include "vmm_impl.h"
#if defined(PVRSRV_GPUVIRT_GUESTDRV)
#include "vmm_impl_client.h"
#else
#include "vmm_impl_server.h"
#include "vmm_pvz_server.h"
#include "vz_vm.h"
#endif

#if defined(CONFIG_L4)
#include <l4/sys/err.h>
#include <l4/sys/irq.h>
#include <l4/re/env.h>
#else
/* Suppress build failures for non-l4linux builds */
#define l4_cap_idx_t			int
#define l4re_env_get_cap(x)		0
#define l4_is_invalid_cap(x)	0
#define l4x_register_irq(x)		0
#define l4x_unregister_irq(x)	PVR_UNREFERENCED_PARAMETER(x)
#define l4_irq_trigger(x)		PVR_UNREFERENCED_PARAMETER(x)
#endif

/* Valid values for the TC_MEMORY_CONFIG configuration option */
#define TC_MEMORY_LOCAL			(1)
#define TC_MEMORY_HOST			(2)
#define TC_MEMORY_HYBRID		(3)

/*
	Fiasco.OC/L4x setup:
		- Static system setup, this is performed at build-time
		- Use identical GPU/FW heap sizes for HOST/GUEST
		- 0 <=(FW/HEAP)=> xMB <=(GPU/HEAP)=> yMB
			- xMB = All OSID firmware heaps
			- yMB = All OSID graphics heaps

	Supported platforms:
		- ARM32/x86 UMA
			- DMA memory map:	[0<=(CPU/RAM)=>512MB<=(GPU/RAM)=>1GB]
			- ARM32-VEXPRESS:	512MB offset @ 0x80000000
			- X86-PC:			512MB offset @ 0x20000000

		- x86 LMA
			- Local memory map:	[0<=(GPU/GRAM)=>1GB]
			- X86-PC:			0 offset @ 0x00000000
*/
#ifndef TC_MEMORY_CONFIG
	#if defined (CONFIG_ARM)
		#define FWHEAP_BASE		0x80000000
	#else
		#define FWHEAP_BASE		0xDEADBEEF
	#endif
#else
	#if (TC_MEMORY_CONFIG == TC_MEMORY_LOCAL)
		/* Start of LMA card address */
		#define FWHEAP_BASE		0x0
	#elif (TC_MEMORY_CONFIG == TC_MEMORY_HOST)
		/* Start of UMA/CO memory address */
		#define FWHEAP_BASE		0x20000000
	#elif (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID)
		#error "TC_MEMORY_HYBRID: Not supported"
	#endif
#endif

#define VZ_FWHEAP_SIZE		(RGX_FIRMWARE_HEAP_SIZE)
#define VZ_FWHEAP_BASE		(FWHEAP_BASE+(VZ_FWHEAP_SIZE*PVRSRV_GPUVIRT_OSID))

#define VZ_GPUHEAP_SIZE		(1<<26)
#define VZ_GPUHEAP_BASE		((FWHEAP_BASE+(VZ_FWHEAP_SIZE*PVRSRV_GPUVIRT_NUM_OSID))+(VZ_GPUHEAP_SIZE*PVRSRV_GPUVIRT_OSID))

static PVRSRV_ERROR
L4xVmmpGetDevPhysHeapOrigin(PVRSRV_DEVICE_CONFIG *psDevConfig,
							PVRSRV_DEVICE_PHYS_HEAP eHeap,
							PVRSRV_DEVICE_PHYS_HEAP_ORIGIN *peOrigin)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_UNREFERENCED_PARAMETER(eHeap);
	*peOrigin = PVRSRV_DEVICE_PHYS_HEAP_ORIGIN_HOST;
	return PVRSRV_OK; 
}

static PVRSRV_ERROR
L4xVmmGetDevPhysHeapAddrSize(PVRSRV_DEVICE_CONFIG *psDevConfig,
							 PVRSRV_DEVICE_PHYS_HEAP eHeap,
							 IMG_UINT64 *pui64Size,
							 IMG_UINT64 *pui64Addr)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVR_UNREFERENCED_PARAMETER(eHeap);
	PVR_UNREFERENCED_PARAMETER(psDevConfig);

	switch (eHeap)
	{
		case PVRSRV_DEVICE_PHYS_HEAP_FW_LOCAL:
			*pui64Size = VZ_FWHEAP_SIZE;
			*pui64Addr = VZ_FWHEAP_BASE;
			break;

		case PVRSRV_DEVICE_PHYS_HEAP_GPU_LOCAL:
			*pui64Size = VZ_GPUHEAP_SIZE;
			*pui64Addr = VZ_GPUHEAP_BASE;
			break;

		default:
			*pui64Size = 0;
			*pui64Addr = 0;
			eError = PVRSRV_ERROR_NOT_IMPLEMENTED;
			PVR_ASSERT(0);
			break;
	}

	return eError;
}

#if defined(PVRSRV_GPUVIRT_GUESTDRV)
static PVRSRV_ERROR
L4xVmmCreateDevConfig(IMG_UINT32 ui32FuncID,
					  IMG_UINT32 ui32DevID,
					  IMG_UINT32 *pui32IRQ,
					  IMG_UINT32 *pui32RegsSize,
					  IMG_UINT64 *pui64RegsCpuPBase)
{
	PVR_UNREFERENCED_PARAMETER(ui32FuncID);
	PVR_UNREFERENCED_PARAMETER(ui32DevID);
	PVR_UNREFERENCED_PARAMETER(pui32IRQ);
	PVR_UNREFERENCED_PARAMETER(pui32RegsSize);
	PVR_UNREFERENCED_PARAMETER(pui64RegsCpuPBase);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

static PVRSRV_ERROR
L4xVmmDestroyDevConfig(IMG_UINT32 ui32FuncID,
					   IMG_UINT32 ui32DevID)
{
	PVR_UNREFERENCED_PARAMETER(ui32FuncID);
	PVR_UNREFERENCED_PARAMETER(ui32DevID);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

static PVRSRV_ERROR
L4xVmmCreateDevPhysHeaps(IMG_UINT32 ui32FuncID,
						 IMG_UINT32 ui32DevID,
						 IMG_UINT32 *peType,
						 IMG_UINT64 *pui64FwSize,
						 IMG_UINT64 *pui64FwAddr,
						 IMG_UINT64 *pui64GpuSize,
						 IMG_UINT64 *pui64GpuAddr)
{
	PVR_UNREFERENCED_PARAMETER(ui32FuncID);
	PVR_UNREFERENCED_PARAMETER(ui32DevID);
	PVR_UNREFERENCED_PARAMETER(peType);
	PVR_UNREFERENCED_PARAMETER(pui64FwSize);
	PVR_UNREFERENCED_PARAMETER(pui64FwAddr);
	PVR_UNREFERENCED_PARAMETER(pui64GpuSize);
	PVR_UNREFERENCED_PARAMETER(pui64GpuAddr);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

static PVRSRV_ERROR
L4xVmmDestroyDevPhysHeaps(IMG_UINT32 ui32FuncID,
						  IMG_UINT32 ui32DevID)
{
	PVR_UNREFERENCED_PARAMETER(ui32FuncID);
	PVR_UNREFERENCED_PARAMETER(ui32DevID);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

static PVRSRV_ERROR
L4xVmmMapDevPhysHeap(IMG_UINT32 ui32FuncID,
					 IMG_UINT32 ui32DevID,
					 IMG_UINT64 ui64Size,
					 IMG_UINT64 ui64Addr)
{
	PVR_UNREFERENCED_PARAMETER(ui32FuncID);
	PVR_UNREFERENCED_PARAMETER(ui32DevID);
	PVR_UNREFERENCED_PARAMETER(ui64Size);
	PVR_UNREFERENCED_PARAMETER(ui64Addr);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

static PVRSRV_ERROR
L4xVmmUnmapDevPhysHeap(IMG_UINT32 ui32FuncID,
					   IMG_UINT32 ui32DevID)
{
	PVR_UNREFERENCED_PARAMETER(ui32FuncID);
	PVR_UNREFERENCED_PARAMETER(ui32DevID);
	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

static VMM_PVZ_CONNECTION gsL4xVmmPvz =
{
	.sHostFuncTab = {
		/* pfnCreateDevConfig */
		&L4xVmmCreateDevConfig,

		/* pfnDestroyDevConfig */
		&L4xVmmDestroyDevConfig,

		/* pfnCreateDevPhysHeaps */
		&L4xVmmCreateDevPhysHeaps,

		/* pfnDestroyDevPhysHeaps */
		&L4xVmmDestroyDevPhysHeaps,

		/* pfnMapDevPhysHeap */
		&L4xVmmMapDevPhysHeap,

		/* pfnUnmapDevPhysHeap */
		&L4xVmmUnmapDevPhysHeap
	},

	.sConfigFuncTab = {
		/* pfnGetDevPhysHeapOrigin */
		&L4xVmmpGetDevPhysHeapOrigin,

		/* pfnGetGPUDevPhysHeap */
		&L4xVmmGetDevPhysHeapAddrSize
	}
};

static void _TriggerPvzConnection(l4_cap_idx_t irqcap)
{
	unsigned long flags;
	static DEFINE_SPINLOCK(lock);

	/* Notify that this OSID has transitioned on/offline */
	spin_lock_irqsave(&lock, flags);
	l4_irq_trigger(irqcap);
	spin_unlock_irqrestore(&lock, flags);
}

static PVRSRV_ERROR L4xCreatePvzConnection(void)
{
	l4_cap_idx_t irqcap;

	/* Get environment capability for PVZ/IRQ */
	irqcap = l4re_env_get_cap("pvzirq");
	if (l4_is_invalid_cap(irqcap))
	{
		PVR_DPF((PVR_DBG_ERROR,"Could not find pvz connection\n"));
		PVR_DPF((PVR_DBG_ERROR,"Unable to notify host of guest online event\n"));
		goto e0;
	}

	_TriggerPvzConnection(irqcap);
	return PVRSRV_OK;
e0:
	return PVRSRV_ERROR_INIT_FAILURE;
}

static void L4xDestroyPvzConnection(void)
{
	l4_cap_idx_t irqcap;

	/* Get environment capability for PVZ/IRQ */
	irqcap = l4re_env_get_cap("pvzirq");
	if (l4_is_invalid_cap(irqcap))
	{
		PVR_DPF((PVR_DBG_ERROR,"Could not find pvz connection\n"));
		PVR_DPF((PVR_DBG_ERROR,"Unable to notify host of guest offline event\n"));
		return;
	}

	_TriggerPvzConnection(irqcap);
}
#else /* defined(PVRSRV_GPUVIRT_GUESTDRV) */
static VMM_PVZ_CONNECTION gsL4xVmmPvz =
{
	.sGuestFuncTab = {
		/* pfnCreateDevConfig */
		&PvzServerCreateDevConfig,

		/* pfnDestroyDevConfig */
		&PvzServerDestroyDevConfig,

		/* pfnCreateDevPhysHeaps */
		&PvzServerCreateDevPhysHeaps,

		/* pfnDestroyDevPhysHeaps */
		PvzServerDestroyDevPhysHeaps,

		/* pfnMapDevPhysHeap */
		&PvzServerMapDevPhysHeap,

		/* pfnUnmapDevPhysHeap */
		&PvzServerUnmapDevPhysHeap
	},

	.sConfigFuncTab = {
		/* pfnGetDevPhysHeapOrigin */
		&L4xVmmpGetDevPhysHeapOrigin,

		/* pfnGetDevPhysHeapAddrSize */
		&L4xVmmGetDevPhysHeapAddrSize
	},

	.sVmmFuncTab = {
		/* pfnOnVmOnline */
		&PvzServerOnVmOnline,

		/* pfnOnVmOffline */
		&PvzServerOnVmOffline,

		/* pfnVMMConfigure */
		&PvzServerVMMConfigure
	}
};

static struct PvzIrqData {
	IMG_HANDLE hLISR;
	IMG_UINT32 irqnr;
	l4_cap_idx_t irqcap;
	IMG_CHAR pszIrqName[16];
	IMG_CHAR pszIsrName[16];
} gsPvzIrqData[RGXFW_NUM_OS - 1] = {{0}};

static struct PvzMisrData {
	IMG_HANDLE hMISR;
	IMG_UINT32 uiOSID[RGXFW_NUM_OS - 1];
} gsPvzMisrData = {NULL, {~0}};

static void L4xPvzConnectionMISR(void *pvData)
{
	struct PvzMisrData *psMisrData = pvData;
	IMG_UINT32 uiOSID;
	IMG_UINT32 uiIdx;

	for (uiOSID = 1; uiOSID < RGXFW_NUM_OS; uiOSID ++)
	{
		uiIdx = uiOSID - 1;
		if (psMisrData->uiOSID[uiIdx] == uiOSID)
		{
			if (PVRSRV_OK != gsL4xVmmPvz.sVmmFuncTab.pfnOnVmOnline(uiOSID, 0))
			{
				PVR_DPF((PVR_DBG_ERROR, "OnVmOnline failed for OSID%d", uiOSID));
			}
			else
			{
				PVR_LOG(("Guest OSID%d driver is online", uiOSID));
			}
		}
		else if (psMisrData->uiOSID[uiIdx] == 0)
		{
			if (PVRSRV_OK != gsL4xVmmPvz.sVmmFuncTab.pfnOnVmOffline(uiOSID))
			{
				PVR_DPF((PVR_DBG_ERROR, "OnVmOffline failed for OSID%d", uiOSID));
			}
			else
			{
				PVR_LOG(("Guest OSID%d driver is offline", uiOSID));
			}
		}
	}
}

static IMG_BOOL L4xPvzConnectionLISR(void *pvData)
{
	struct PvzMisrData *psMisrData = &gsPvzMisrData;
	struct PvzIrqData *psIrqData = &gsPvzIrqData[0];
	IMG_UINT32 uiOSID;
	IMG_UINT32 uiIdx;

	for (uiOSID = 1; uiOSID < RGXFW_NUM_OS; uiOSID ++)
	{
		uiIdx = uiOSID - 1;
		if (&psIrqData[uiIdx] == pvData)
		{
			/* For now, use simple state machine protocol */
			if (psMisrData->uiOSID[uiIdx] == uiOSID)
			{
				/* Guest driver in VM is offline */
				psMisrData->uiOSID[uiIdx] = 0;
			}
			else
			{
				/* Guest driver in VM is online */
				psMisrData->uiOSID[uiIdx] = uiOSID;
			}
			OSScheduleMISR(psMisrData->hMISR);
		}
	}

	return IMG_TRUE;
}

static PVRSRV_ERROR L4xCreatePvzConnection(void)
{
	IMG_UINT32 uiOSID;

	/* This pvz misr get notified whenever guest comes online */
	if (PVRSRV_OK != OSInstallMISR(&gsPvzMisrData.hMISR,
								   L4xPvzConnectionMISR,
								   &gsPvzMisrData))
	{
		PVR_DPF((PVR_DBG_ERROR, "Could not create pvz thread object"));
		goto e0;
	}

	/* Create abstract virtual IRQ for each guest OSID */
	for (uiOSID = 1; uiOSID < RGXFW_NUM_OS; uiOSID ++)
	{
		IMG_UINT32 uiIdx = uiOSID - 1;

		/* Each OSID has its own corresponding IRQ capability */
		OSSNPrintf(gsPvzIrqData[uiIdx].pszIrqName, 
				   sizeof(gsPvzIrqData[uiIdx].pszIrqName),
				   "pvzirq%d",
				   uiOSID);
		OSSNPrintf(gsPvzIrqData[uiIdx].pszIsrName,
				   sizeof(gsPvzIrqData[uiIdx].pszIsrName),
				   "l4pvz%d",
				   uiOSID);

		/* Get a free capability slot for the PVZ/IRQ capability for this OSID */
		gsPvzIrqData[uiIdx].irqcap = l4re_env_get_cap(gsPvzIrqData[uiIdx].pszIrqName);
		if (l4_is_invalid_cap(gsPvzIrqData[uiIdx].irqcap))
		{
			PVR_DPF((PVR_DBG_ERROR,
					"Could not find a PVZ/IRQ capability [%s] for OSID%d\n",
					gsPvzIrqData[uiIdx].pszIrqName, uiOSID));
			goto e1;
		} 

		/* Register IRQ capability with l4linux, obtain corresponding IRQ number */
		gsPvzIrqData[uiIdx].irqnr = l4x_register_irq(gsPvzIrqData[uiIdx].irqcap);
		if (gsPvzIrqData[uiIdx].irqnr < 0)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"Could not register OSID%d IRQ%d with kernel\n",
					gsPvzIrqData[uiIdx].irqnr,
					uiOSID));
			goto e1;
		}

		/* pvz IRQ number now known to l4linux kernel, install ISR for it */
		if (PVRSRV_OK != OSInstallSystemLISR(&gsPvzIrqData[uiIdx].hLISR,
											 gsPvzIrqData[uiIdx].irqnr,
											 gsPvzIrqData[uiIdx].pszIsrName,
											 L4xPvzConnectionLISR,
											 &gsPvzIrqData[uiIdx],
											 0))
		{
			PVR_DPF((PVR_DBG_ERROR,
					"Could not install system LISR for OSID%d PVZ/IRQ\n",
					uiOSID));
			goto e1;
		}
	}

	return PVRSRV_OK;

e1:
	for ( ; uiOSID >= 1; uiOSID --)
	{
		IMG_UINT32 uiIdx = uiOSID - 1;

		if (gsPvzIrqData[uiIdx].hLISR)
		{
			OSUninstallSystemLISR(gsPvzIrqData[uiIdx].hLISR);
		}

		if (gsPvzIrqData[uiIdx].irqnr)
		{
			l4x_unregister_irq(gsPvzIrqData[uiIdx].irqnr);
		}
	}
	OSUninstallMISR(gsPvzMisrData.hMISR);
e0:
	return PVRSRV_ERROR_INIT_FAILURE;
}

static void L4xDestroyPvzConnection(void)
{
	IMG_UINT32 uiOSID;
	IMG_UINT32 uiIdx;

	for (uiOSID = 1; uiOSID < RGXFW_NUM_OS; uiOSID ++)
	{
		uiIdx = uiOSID - 1;
		OSUninstallSystemLISR(gsPvzIrqData[uiIdx].hLISR);
		l4x_unregister_irq(gsPvzIrqData[uiIdx].irqnr);
	}

	OSUninstallMISR(gsPvzMisrData.hMISR);
}
#endif

PVRSRV_ERROR VMMCreatePvzConnection(VMM_PVZ_CONNECTION **psPvzConnection)
{
	PVR_ASSERT(psPvzConnection);
	*psPvzConnection = &gsL4xVmmPvz;
	return L4xCreatePvzConnection();
}

void VMMDestroyPvzConnection(VMM_PVZ_CONNECTION *psPvzConnection)
{
	PVR_ASSERT(psPvzConnection == &gsL4xVmmPvz);
	L4xDestroyPvzConnection();
}

/******************************************************************************
 End of file (vmm_type_l4linux.c)
******************************************************************************/
