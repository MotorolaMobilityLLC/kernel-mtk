////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2014 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (??MStar Confidential Information??) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_platform_interface.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __MSTAR_DRV_PLATFORM_INTERFACE_H__
#define __MSTAR_DRV_PLATFORM_INTERFACE_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include "mstar_drv_common.h"

/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

extern s32 /*__devinit*/ MsDrvInterfaceTouchDeviceProbe(struct i2c_client *pClient, const struct i2c_device_id *pDeviceId);
extern s32 /*__devexit*/ MsDrvInterfaceTouchDeviceRemove(struct i2c_client *pClient);
#ifdef CONFIG_ENABLE_NOTIFIER_FB
extern int MsDrvInterfaceTouchDeviceFbNotifierCallback(struct notifier_block *pSelf, unsigned long nEvent, void *pData);
#else
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
extern void MsDrvInterfaceTouchDeviceResume(struct device *pDevice);
extern void MsDrvInterfaceTouchDeviceSuspend(struct device *pDevice);
#else
extern void MsDrvInterfaceTouchDeviceResume(struct early_suspend *pSuspend);        
extern void MsDrvInterfaceTouchDeviceSuspend(struct early_suspend *pSuspend);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif //CONFIG_ENABLE_NOTIFIER_FB
extern void MsDrvInterfaceTouchDeviceSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate);       
#endif  /* __MSTAR_DRV_PLATFORM_INTERFACE_H__ */
