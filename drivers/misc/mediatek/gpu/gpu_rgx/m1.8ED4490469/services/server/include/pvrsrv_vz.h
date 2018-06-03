/**************************************************************************/ /*!
@File           pvrsrv_virt.h
@Title          PowerVR services server virtualization header file
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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
*/ /***************************************************************************/

#ifndef PVRSRV_VIRT_H
#define PVRSRV_VIRT_H

typedef struct PVRSRV_VIRTZ_DATA_TAG
{
	IMG_HANDLE	hPvzConnection;
	POS_LOCK	hPvzLock;
#if !defined(PVRSRV_GPUVIRT_GUESTDRV)
	IMG_BOOL	abVmOnline[PVRSRV_GPUVIRT_NUM_OSID];
#endif
} PVRSRV_VIRTZ_DATA;

typedef enum _VMM_CONF_PARAM_
{
	VMM_CONF_PRIO_OSID0 = 0,
	VMM_CONF_PRIO_OSID1 = 1,
	VMM_CONF_PRIO_OSID2 = 2,
	VMM_CONF_PRIO_OSID3 = 3,
	VMM_CONF_PRIO_OSID4 = 4,
	VMM_CONF_PRIO_OSID5 = 5,
	VMM_CONF_PRIO_OSID6 = 6,
	VMM_CONF_PRIO_OSID7 = 7,
	VMM_CONF_ISOL_THRES = 8,
	VMM_CONF_HCS_DEADLINE = 9

} VMM_CONF_PARAM;

/*!
******************************************************************************

 @Function			PVRSRVVzDeviceCreate

 @Description 		Performs additional device creation

 @Return			PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
 ******************************************************************************/
PVRSRV_ERROR PVRSRVVzDeviceCreate(PVRSRV_DEVICE_NODE *psDeviceNode);

/*!
******************************************************************************

 @Function			PVRSRVVzDeviceDestroy

 @Description 		Performs additional device destruction

 @Return			PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
 ******************************************************************************/
PVRSRV_ERROR PVRSRVVzDeviceDestroy(PVRSRV_DEVICE_NODE *psDeviceNode);

#if !defined(PVRSRV_GPUVIRT_GUESTDRV)
/*!
******************************************************************************

 @Function			PVRSRVVzRegisterFirmwarePhysHeap

 @Description 		Request to map a physical heap to kernel FW memory context

 @Return			PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
 ******************************************************************************/
PVRSRV_ERROR PVRSRVVzRegisterFirmwarePhysHeap(PVRSRV_DEVICE_NODE *psDeviceNode,
											  IMG_DEV_PHYADDR sDevPAddr,
											  IMG_UINT64 ui64DevPSize,
											  IMG_UINT32 uiOSID);

/*!
******************************************************************************

 @Function			RGXVzSetupFirmware

 @Description 		Request to unmap a physical heap from kernel FW memory context

 @Return			PVRSRV_ERROR	PVRSRV_OK on success. Otherwise, a PVRSRV_
									error code
 ******************************************************************************/
PVRSRV_ERROR PVRSRVVzUnregisterFirmwarePhysHeap(PVRSRV_DEVICE_NODE *psDeviceNode,
												IMG_UINT32 uiOSID);
#endif
#endif /* PVRSRV_VIRT_H */
