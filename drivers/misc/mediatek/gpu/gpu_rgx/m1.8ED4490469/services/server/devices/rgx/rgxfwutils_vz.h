/*************************************************************************/ /*!
@File
@Title          RGX virtualization firmware utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX virtualization firmware utility routines
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

#if !defined(__RGXFWUTILS_VIRT_H__)
#define __RGXFWUTILS_VIRT_H__

/*!
******************************************************************************

 @Function			RGXVzSetupFirmware

 @Description 		Performs additional firmware setup

 @Return			PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
 ******************************************************************************/
PVRSRV_ERROR RGXVzSetupFirmware(PVRSRV_DEVICE_NODE *psDeviceNode);

/*!
******************************************************************************

 @Function			RGXVzCreateFWKernelMemoryContext

 @Description 		Performs additional firmware memory context creation

 @Return			PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
 ******************************************************************************/
PVRSRV_ERROR RGXVzCreateFWKernelMemoryContext(PVRSRV_DEVICE_NODE *psDeviceNode);

/*!
******************************************************************************

 @Function			RGXVzDestroyFWKernelMemoryContext

 @Description 		Performs additional firmware memory context destruction

 @Return			PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
 ******************************************************************************/
PVRSRV_ERROR RGXVzDestroyFWKernelMemoryContext(PVRSRV_DEVICE_NODE *psDeviceNode);

#if !defined(PVRSRV_GPUVIRT_GUESTDRV)
/*!
******************************************************************************

 @Function			RGXVzRegisterFirmwarePhysHeap

 @Description 		Register and maps to device, a guest firmware physheap
 
 @Return			PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
 *****************************************************************************/
PVRSRV_ERROR RGXVzRegisterFirmwarePhysHeap(PVRSRV_DEVICE_NODE *psDeviceNode,
										   IMG_UINT32 ui32OSID,
										   IMG_DEV_PHYADDR sDevPAddr,
										   IMG_UINT64 ui64DevPSize);

/*!
******************************************************************************

 @Function			RGXVzDeregisterFirmwarePhysHeap

 @Description 		Unregister and unmap from device, a guest firmware physheap
 
 @Return			PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
 *****************************************************************************/
PVRSRV_ERROR RGXVzUnregisterFirmwarePhysHeap(PVRSRV_DEVICE_NODE *psDeviceNode,
											 IMG_UINT32 ui32OSID);
#endif

#endif /* __RGXFWUTILS_VIRT_H__ */
/******************************************************************************
 End of file (rgxfwutils_virt.h)
******************************************************************************/
