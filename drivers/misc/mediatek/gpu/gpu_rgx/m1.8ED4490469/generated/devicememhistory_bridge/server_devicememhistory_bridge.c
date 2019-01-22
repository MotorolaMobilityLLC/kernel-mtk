/*************************************************************************/ /*!
@File
@Title          Server bridge for devicememhistory
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implements the server side of the bridge for devicememhistory
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

#include <stddef.h>
#include <asm/uaccess.h>

#include "img_defs.h"

#include "devicemem_history_server.h"


#include "common_devicememhistory_bridge.h"

#include "allocmem.h"
#include "pvr_debug.h"
#include "connection_server.h"
#include "pvr_bridge.h"
#include "rgx_bridge.h"
#include "srvcore.h"
#include "handle.h"

#include <linux/slab.h>

#include "lock.h"


#if defined(SUPPORT_DEVICEMEMHISTORY_BRIDGE)



/* ***************************************************************************
 * Server-side bridge entry points
 */
 
static IMG_INT
PVRSRVBridgeDevicememHistoryMap(IMG_UINT32 ui32DispatchTableEntry,
					  PVRSRV_BRIDGE_IN_DEVICEMEMHISTORYMAP *psDevicememHistoryMapIN,
					  PVRSRV_BRIDGE_OUT_DEVICEMEMHISTORYMAP *psDevicememHistoryMapOUT,
					 CONNECTION_DATA *psConnection)
{
	IMG_CHAR *uiTextInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE   *pArrayArgsBuffer = NULL;

	IMG_UINT32 ui32BufferSize = 
			(DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR)) +
			0;


	PVR_UNREFERENCED_PARAMETER(psConnection);



	if (ui32BufferSize != 0)
	{
		pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

		if(!pArrayArgsBuffer)
		{
			psDevicememHistoryMapOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto DevicememHistoryMap_exit;
		}
	}

	
	{
		uiTextInt = (IMG_CHAR*)(((IMG_UINT8 *)pArrayArgsBuffer) + ui32NextOffset);
		ui32NextOffset += DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR);
	}

			/* Copy the data over */
			if (DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR) > 0)
			{
				if ( !OSAccessOK(PVR_VERIFY_READ, (void*) psDevicememHistoryMapIN->puiText, DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR))
					|| (OSCopyFromUser(NULL, uiTextInt, psDevicememHistoryMapIN->puiText,
					DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR)) != PVRSRV_OK) )
				{
					psDevicememHistoryMapOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

					goto DevicememHistoryMap_exit;
				}
			}


	psDevicememHistoryMapOUT->eError =
		DevicememHistoryMapKM(
					psDevicememHistoryMapIN->sDevVAddr,
					psDevicememHistoryMapIN->uiSize,
					uiTextInt);




DevicememHistoryMap_exit:


	/* Allocated space should be equal to the last updated offset */
	PVR_ASSERT(ui32BufferSize == ui32NextOffset);

	if(pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);


	return 0;
}


static IMG_INT
PVRSRVBridgeDevicememHistoryUnmap(IMG_UINT32 ui32DispatchTableEntry,
					  PVRSRV_BRIDGE_IN_DEVICEMEMHISTORYUNMAP *psDevicememHistoryUnmapIN,
					  PVRSRV_BRIDGE_OUT_DEVICEMEMHISTORYUNMAP *psDevicememHistoryUnmapOUT,
					 CONNECTION_DATA *psConnection)
{
	IMG_CHAR *uiTextInt = NULL;

	IMG_UINT32 ui32NextOffset = 0;
	IMG_BYTE   *pArrayArgsBuffer = NULL;

	IMG_UINT32 ui32BufferSize = 
			(DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR)) +
			0;


	PVR_UNREFERENCED_PARAMETER(psConnection);



	if (ui32BufferSize != 0)
	{
		pArrayArgsBuffer = OSAllocMemNoStats(ui32BufferSize);

		if(!pArrayArgsBuffer)
		{
			psDevicememHistoryUnmapOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto DevicememHistoryUnmap_exit;
		}
	}

	
	{
		uiTextInt = (IMG_CHAR*)(((IMG_UINT8 *)pArrayArgsBuffer) + ui32NextOffset);
		ui32NextOffset += DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR);
	}

			/* Copy the data over */
			if (DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR) > 0)
			{
				if ( !OSAccessOK(PVR_VERIFY_READ, (void*) psDevicememHistoryUnmapIN->puiText, DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR))
					|| (OSCopyFromUser(NULL, uiTextInt, psDevicememHistoryUnmapIN->puiText,
					DEVICEMEM_HISTORY_TEXT_BUFSZ * sizeof(IMG_CHAR)) != PVRSRV_OK) )
				{
					psDevicememHistoryUnmapOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;

					goto DevicememHistoryUnmap_exit;
				}
			}


	psDevicememHistoryUnmapOUT->eError =
		DevicememHistoryUnmapKM(
					psDevicememHistoryUnmapIN->sDevVAddr,
					psDevicememHistoryUnmapIN->uiSize,
					uiTextInt);




DevicememHistoryUnmap_exit:


	/* Allocated space should be equal to the last updated offset */
	PVR_ASSERT(ui32BufferSize == ui32NextOffset);

	if(pArrayArgsBuffer)
		OSFreeMemNoStats(pArrayArgsBuffer);


	return 0;
}




/* *************************************************************************** 
 * Server bridge dispatch related glue 
 */

static POS_LOCK pDEVICEMEMHISTORYBridgeLock;
static IMG_BOOL bUseLock = IMG_TRUE;
#endif /* SUPPORT_DEVICEMEMHISTORY_BRIDGE */

#if defined(SUPPORT_DEVICEMEMHISTORY_BRIDGE)
PVRSRV_ERROR InitDEVICEMEMHISTORYBridge(void);
PVRSRV_ERROR DeinitDEVICEMEMHISTORYBridge(void);

/*
 * Register all DEVICEMEMHISTORY functions with services
 */
PVRSRV_ERROR InitDEVICEMEMHISTORYBridge(void)
{
	PVR_LOGR_IF_ERROR(OSLockCreate(&pDEVICEMEMHISTORYBridgeLock, LOCK_TYPE_PASSIVE), "OSLockCreate");

	SetDispatchTableEntry(PVRSRV_BRIDGE_DEVICEMEMHISTORY, PVRSRV_BRIDGE_DEVICEMEMHISTORY_DEVICEMEMHISTORYMAP, PVRSRVBridgeDevicememHistoryMap,
					pDEVICEMEMHISTORYBridgeLock, bUseLock);

	SetDispatchTableEntry(PVRSRV_BRIDGE_DEVICEMEMHISTORY, PVRSRV_BRIDGE_DEVICEMEMHISTORY_DEVICEMEMHISTORYUNMAP, PVRSRVBridgeDevicememHistoryUnmap,
					pDEVICEMEMHISTORYBridgeLock, bUseLock);


	return PVRSRV_OK;
}

/*
 * Unregister all devicememhistory functions with services
 */
PVRSRV_ERROR DeinitDEVICEMEMHISTORYBridge(void)
{
	PVR_LOGR_IF_ERROR(OSLockDestroy(pDEVICEMEMHISTORYBridgeLock), "OSLockDestroy");
	return PVRSRV_OK;
}
#else /* SUPPORT_DEVICEMEMHISTORY_BRIDGE */
/* This bridge is conditional on SUPPORT_DEVICEMEMHISTORY_BRIDGE - when not defined,
 * do not populate the dispatch table with its functions
 */
#define InitDEVICEMEMHISTORYBridge() \
	PVRSRV_OK

#define DeinitDEVICEMEMHISTORYBridge() \
	PVRSRV_OK

#endif /* SUPPORT_DEVICEMEMHISTORY_BRIDGE */
