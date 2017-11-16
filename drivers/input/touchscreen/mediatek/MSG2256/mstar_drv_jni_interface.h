////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2012 MStar Semiconductor, Inc.
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
 * @file    mstar_drv_jni_interface.h
 *
 * @brief   This file defines the jni interface functions
 *
 *
 */

#ifndef __MSTAR_DRV_JNI_INTERFACE_H__
#define __MSTAR_DRV_JNI_INTERFACE_H__


////////////////////////////////////////////////////////////
/// Included Files
////////////////////////////////////////////////////////////
#include "mstar_drv_common.h"


#ifdef CONFIG_ENABLE_JNI_INTERFACE
////////////////////////////////////////////////////////////
/// Data Types
////////////////////////////////////////////////////////////
/*
typedef struct
{
    u64          nCmdId;
    u8         *pSndCmdData;      //send data to fw  
    u64         nSndCmdLen;
    u8         *pRtnCmdData;      //receive data from fw
    u64         nRtnCmdLen;
} MsgToolDrvCmd_t;
*/

typedef struct
{
    u64         nCmdId;
    u64         nSndCmdDataPtr;      //send data to fw  
    u64         nSndCmdLen;
    u64         nRtnCmdDataPtr;      //receive data from fw
    u64         nRtnCmdLen;
} MsgToolDrvCmd_t;



////////////////////////////////////////////////////////////
/// Macro
////////////////////////////////////////////////////////////
#define MSGTOOL_MAGIC_NUMBER               96
#define MSGTOOL_IOCTL_RUN_CMD              _IO(MSGTOOL_MAGIC_NUMBER, 1)


#define MSGTOOL_RESETHW           0x01 
#define MSGTOOL_REGGETXBYTEVALUE  0x02 
#define MSGTOOL_HOTKNOTSTATUS     0x03 
#define MSGTOOL_FINGERTOUCH       0x04
#define MSGTOOL_BYPASSHOTKNOT     0x05
#define MSGTOOL_DEVICEPOWEROFF    0x06
#define MSGTOOL_GETSMDBBUS        0x07
#define MSGTOOL_SETIICDATARATE    0x08



////////////////////////////////////////////////////////////
/// Function Prototypes
////////////////////////////////////////////////////////////

extern ssize_t MsgToolRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
extern ssize_t MsgToolWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);  
extern long MsgToolIoctl( struct file *pFile, unsigned int nCmd, unsigned long nArg );
extern void CreateMsgToolMem(void);
extern void DeleteMsgToolMem(void);


#endif //CONFIG_ENABLE_JNI_INTERFACE
#endif // __MSTAR_DRV_JNI_INTERFACE_H__
