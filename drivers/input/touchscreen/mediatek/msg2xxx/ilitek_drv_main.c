/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Johnson Yeh
 * Maintain: Luca Hsu, Tigers Huang, Dicky Chiang
 */

/**
 *
 * @file    ilitek_drv_main.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "ilitek_drv_common.h"

#ifdef CONFIG_ENABLE_ITO_MP_TEST
#include "ilitek_drv_mp_test.h"
#endif //CONFIG_ENABLE_ITO_MP_TEST

#include "new_mp_test/mp_common.h"

#include <ontim/ontim_dev_dgb.h>
static char version[64]="1.0";
static char vendor_name[64]="msg2xxx";
static char lcdname[64]="Unknown";
DEV_ATTR_DECLARE(touch_screen)
DEV_ATTR_DEFINE("version",version)
DEV_ATTR_DEFINE("vendor",vendor_name)
DEV_ATTR_DEFINE("lcdvendor",lcdname)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(touch_screen,touch_screen,8);
extern char *mtkfb_find_lcm_driver(void);
/*=============================================================*/
// VARIABLE DECLARATION
/*=============================================================*/

extern struct i2c_client *g_I2cClient;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
extern struct regulator *g_ReguVdd;
extern struct regulator *g_ReguVcc_i2c;
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

static struct work_struct _gFingerTouchWork;
int _gIrq = -1;

#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
static int _gGpioRst = 0;
static int _gGpioIrq = 0;
int MS_TS_MSG_IC_GPIO_RST = 0;
int MS_TS_MSG_IC_GPIO_INT = 0;

static struct pinctrl *_gTsPinCtrl = NULL;
static struct pinctrl_state *_gPinCtrlStateActive = NULL;
static struct pinctrl_state *_gPinCtrlStateSuspend = NULL;
static struct pinctrl_state *_gPinCtrlStateRelease = NULL;
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL

#ifdef CONFIG_ENABLE_NOTIFIER_FB
static struct notifier_block _gFbNotifier;
#else
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend _gEarlySuspend;
#endif
#endif //CONFIG_ENABLE_NOTIFIER_FB

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
struct input_dev *g_ProximityInputDevice = NULL;

u8 g_EnableTpProximity = 0;
u8 g_FaceClosingTp = 0; // for QCOM platform -> 1 : close to, 0 : far away 
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
extern struct tpd_device *tpd;

#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
static struct work_struct _gFingerTouchWork;
#else 
static DECLARE_WAIT_QUEUE_HEAD(_gWaiter);
static struct task_struct *_gThread = NULL;
static int _gTpdFlag = 0;
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
u8 g_EnableTpProximity = 0;
u8 g_FaceClosingTp = 1; // for MTK platform -> 0 : close to, 1 : far away 
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
extern struct regulator *g_ReguVdd;
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

extern struct of_device_id touch_dt_match_table[];

int _gIrq = -1;
int MS_TS_MSG_IC_GPIO_RST = 0; // Must set a value other than 1
int MS_TS_MSG_IC_GPIO_INT = 1; // Must set value as 1
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif 

#ifdef CONFIG_ENABLE_JNI_INTERFACE
static MsgToolDrvCmd_t *_gMsgToolCmdIn = NULL;
static u8 *_gSndCmdData = NULL;
static u8 *_gRtnCmdData = NULL;
#endif //CONFIG_ENABLE_JNI_INTERFACE

#ifdef CONFIG_ENABLE_ITO_MP_TEST
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
extern TestScopeInfo_t g_TestScopeInfo;
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
extern u32 g_IsInMpTest;
static ItoTestMode_e _gItoTestMode = 0;
#endif //CONFIG_ENABLE_ITO_MP_TEST

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#if defined(CONFIG_ENABLE_GESTURE_DEBUG_MODE)
u8 _gGestureWakeupPacket[GESTURE_DEBUG_MODE_PACKET_LENGTH] = {0};
#elif defined(CONFIG_ENABLE_GESTURE_INFORMATION_MODE)
u8 _gGestureWakeupPacket[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH] = {0};
#else
u8 _gGestureWakeupPacket[GESTURE_WAKEUP_PACKET_LENGTH] = {0}; // for MSG22xx/MSG28xx : packet length = GESTURE_WAKEUP_PACKET_LENGTH
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
u8 g_GestureDebugFlag = 0x00;
u8 g_GestureDebugMode = 0x00;
u8 g_LogGestureDebug[GESTURE_DEBUG_MODE_PACKET_LENGTH] = {0};
struct kset *g_GestureKSet = NULL;
struct kobject *g_GestureKObj = NULL;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static u32 _gLogGestureCount = 0;
static u8 _gLogGestureInforType = 0;
u32 g_LogGestureInfor[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH] = {0};
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

static u32 _gGestureWakeupValue[2] = {0};
u8 g_GestureWakeupFlag = 0;

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE // support at most 64 types of gesture wakeup mode
u32 g_GestureWakeupMode[2] = {0xFFFFFFFF, 0xFFFFFFFF};
#else                                              // support at most 16 types of gesture wakeup mode
u32 g_GestureWakeupMode[2] = {0x0000FFFF, 0x00000000};
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA // for MSG28xx/MSG58xxA/ILI21xx
static u8 _gTouchPacketFlag[2] = {0};
u16 g_FwPacketDataAddress = 0;
u16 g_FwPacketFlagAddress = 0;
u8 g_FwSupportSegment = 0;
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
static u8 _gChargerPlugIn = 0;
u8 g_ForceUpdate = 0;
int g_IsEnableChargerPlugInOutCheck = 1;
struct delayed_work g_ChargerPlugInOutCheckWork;
struct workqueue_struct *g_ChargerPlugInOutCheckWorkqueue = NULL;
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
static u8 _gCurrPress[SELF_MAX_TOUCH_NUM] = {0}; // for MSG22xx
static u8 _gPriorPress[SELF_MAX_TOUCH_NUM] = {0}; // for MSG22xx
static u8 _gPrevTouchStatus = 0;

static u8 _gPreviousTouch[MUTUAL_MAX_TOUCH_NUM] = {0}; // for MSG28xx/MSG58xxA/ILI21xx
static u8 _gCurrentTouch[MUTUAL_MAX_TOUCH_NUM] = {0};
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

static spinlock_t _gIrqLock;

static int _gInterruptFlag = 0;

static u16 _gDebugReg[MAX_DEBUG_REGISTER_NUM] = {0};
static u16 _gDebugRegValue[MAX_DEBUG_REGISTER_NUM] = {0};
static u32 _gDebugRegCount = 0;

static u8 _gDebugCmdArgu[MAX_DEBUG_COMMAND_ARGUMENT_NUM] = {0};
static u16 _gDebugCmdArguCount = 0;
static u32 _gDebugReadDataSize = 0;

static char _gDebugBuf[1024] = {0};

u8 *_gPlatformFwVersion = NULL; // internal use firmware version for MStar
u32 g_PlatformFwVersion[3] = {0};
u8 g_FwVersionFlag = 0; // 0: old 1:new ,platform FW version V01.010.03
u8 *_gFwVersion = NULL; // customer firmware version
u8 g_DebugFwModeBuffer[8] = {0};

static u32 _gIsUpdateComplete = 0;
static u32 _gFeatureSupportStatus = 0;

//static u8 _gIsFirstTouchKeyPressed[MUTUAL_MAX_TOUCH_NUM] = {0};

static u8 _gnDebugLogTimesStamp = 0;
static u8 _gReadTrimData[3] = {0};
s32 g_MpTestMode = 2;
//------------------------------------------------------------------------------//

static struct proc_dir_entry *_gProcClassEntry = NULL;
static struct proc_dir_entry *_gProcMsTouchScreenMsg20xxEntry = NULL;
static struct proc_dir_entry *_gProcDeviceEntry = NULL;
static struct proc_dir_entry *_gProcChipTypeEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareDataEntry = NULL;
static struct proc_dir_entry *_gProcApkFirmwareUpdateEntry = NULL;
static struct proc_dir_entry *_gProcCustomerFirmwareVersionEntry = NULL;
static struct proc_dir_entry *_gProcPlatformFirmwareVersionEntry = NULL;
static struct proc_dir_entry *_gProcDeviceDriverVersionEntry = NULL;
static struct proc_dir_entry *_gProcSdCardFirmwareUpdateEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareDebugEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareSetDebugValueEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareSmBusDebugEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareSetDQMemValueEntry = NULL;
#ifdef CONFIG_ENABLE_ITO_MP_TEST
static struct proc_dir_entry *_gProcMpTestEntry = NULL;
static struct proc_dir_entry *_gProcMpTestLogEntry = NULL;
static struct proc_dir_entry *_gProcMpTestFailChannelEntry = NULL;
static struct proc_dir_entry *_gProcMpTestScopeEntry = NULL;
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
static struct proc_dir_entry *_gProcMpTestLogALLEntry = NULL;
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#endif //CONFIG_ENABLE_ITO_MP_TEST
static struct proc_dir_entry *_gProcFirmwareModeEntry = NULL;
static struct proc_dir_entry *_gProcFirmwareSensorEntry = NULL;
static struct proc_dir_entry *_gProcFirmwarePacketHeaderEntry = NULL;
static struct proc_dir_entry *_gProcQueryFeatureSupportStatusEntry = NULL;
static struct proc_dir_entry *_gProcChangeFeatureSupportStatusEntry = NULL;
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
static struct proc_dir_entry *_gProcGestureWakeupModeEntry = NULL;
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
static struct proc_dir_entry *_gProcGestureDebugModeEntry = NULL;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static struct proc_dir_entry *_gProcGestureInforModeEntry = NULL;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
static struct proc_dir_entry *_gProcReportRateEntry = NULL;
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE
static struct proc_dir_entry *_gProcGloveModeEntry = NULL;
static struct proc_dir_entry *_gProcOpenGloveModeEntry = NULL;
static struct proc_dir_entry *_gProcCloseGloveModeEntry = NULL;
static struct proc_dir_entry *_gProcLeatherSheathModeEntry = NULL;
static struct proc_dir_entry *_gProcFilmModeEntry = NULL;
#ifdef CONFIG_ENABLE_JNI_INTERFACE
static struct proc_dir_entry *_gProcJniMethodEntry = NULL;
#endif //CONFIG_ENABLE_JNI_INTERFACE
static struct proc_dir_entry *_gProcSeLinuxLimitFirmwareUpdateEntry = NULL;
static struct proc_dir_entry *_gProcForceFirmwareUpdateEntry = NULL;
#ifdef CONFIG_ENABLE_ITO_MP_TEST
static struct proc_dir_entry *_gProcMpTestCustomisedEntry = NULL;
#endif
static struct proc_dir_entry *_gProcTrimCodeEntry = NULL;

static ssize_t _DrvProcfsChipTypeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsChipTypeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareDataRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareDataWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareUpdateWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsCustomerFirmwareVersionRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsCustomerFirmwareVersionWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsPlatformFirmwareVersionRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsPlatformFirmwareVersionWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsDeviceDriverVersionRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsDeviceDriverVersionWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsSdCardFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsSdCardFirmwareUpdateWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareDebugRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareDebugWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareSetDebugValueRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareSetDebugValueWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareSmBusDebugRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareSmBusDebugWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);

static ssize_t _DrvProcfsFirmwareSetDQMemValueRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareSetDQMemValueWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);

#ifdef CONFIG_ENABLE_ITO_MP_TEST
static ssize_t _DrvProcfsMpTestRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsMpTestWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsMpTestLogRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsMpTestLogWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsMpTestFailChannelRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsMpTestFailChannelWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsMpTestScopeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsMpTestScopeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
static ssize_t _DrvProcfsMpTestLogAllRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsMpTestLogAllWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#endif //CONFIG_ENABLE_ITO_MP_TEST

static ssize_t _DrvProcfsFirmwareModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareSensorRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwareSensorWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwarePacketHeaderRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsFirmwarePacketHeaderWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvKObjectPacketShow(struct kobject *pKObj, struct kobj_attribute *pAttr, char *pBuf);
static ssize_t _DrvKObjectPacketStore(struct kobject *pKObj, struct kobj_attribute *pAttr, const char *pBuf, size_t nCount);

static ssize_t _DrvProcfsQueryFeatureSupportStatusRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsQueryFeatureSupportStatusWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsChangeFeatureSupportStatusRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsChangeFeatureSupportStatusWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
static ssize_t _DrvProcfsGestureWakeupModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsGestureWakeupModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
static ssize_t _DrvProcfsGestureDebugModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsGestureDebugModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvKObjectGestureDebugShow(struct kobject *pKObj, struct kobj_attribute *pAttr, char *pBuf);
static ssize_t _DrvKObjectGestureDebugStore(struct kobject *pKObj, struct kobj_attribute *pAttr, const char *pBuf, size_t nCount);
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static ssize_t _DrvProcfsGestureInforModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsGestureInforModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
static ssize_t _DrvProcfsReportRateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsReportRateWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

static ssize_t _DrvProcfsGloveModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsGloveModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsOpenGloveModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsCloseGloveModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);

static ssize_t _DrvProcfsLeatherSheathModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsLeatherSheathModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);

#ifdef CONFIG_ENABLE_JNI_INTERFACE
static ssize_t _DrvJniMsgToolRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvJniMsgToolWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static long _DrvJniMsgToolIoctl(struct file *pFile, unsigned int nCmd, unsigned long nArg);
#endif //CONFIG_ENABLE_JNI_INTERFACE

static ssize_t _DrvProcfsSeLinuxLimitFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsForceFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);

#ifdef CONFIG_ENABLE_ITO_MP_TEST
static ssize_t DrvMainProcfsMpTestCustomisedWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t DrvMainProcfsMpTestCustomisedRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static const struct file_operations _gProcMpTestCustomised = { 
    .write = DrvMainProcfsMpTestCustomisedWrite,
    .read = DrvMainProcfsMpTestCustomisedRead,
};
#endif
static ssize_t _DrvProcfsTrimCodeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsTrimCodeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);

static ssize_t _DrvProcfsGetFilmModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos);
static ssize_t _DrvProcfsSetFilmModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos);
static void _DrvSelfGetFirmwareInfo(SelfFirmwareInfo_t *pInfo);
void DrvDBbusI2cResponseAck(void);
extern int ms_atoi(char *nptr);
static const struct file_operations _gProcChipType = { 
    .read = _DrvProcfsChipTypeRead,
    .write = _DrvProcfsChipTypeWrite,
};

static const struct file_operations _gProcFirmwareData = { 
    .read = _DrvProcfsFirmwareDataRead,
    .write = _DrvProcfsFirmwareDataWrite,
};

static const struct file_operations _gProcApkFirmwareUpdate = { 
    .read = _DrvProcfsFirmwareUpdateRead,
    .write = _DrvProcfsFirmwareUpdateWrite,
};

static const struct file_operations _gProcCustomerFirmwareVersion = { 
    .read = _DrvProcfsCustomerFirmwareVersionRead,
    .write = _DrvProcfsCustomerFirmwareVersionWrite,
};

static const struct file_operations _gProcPlatformFirmwareVersion = { 
    .read = _DrvProcfsPlatformFirmwareVersionRead,
    .write = _DrvProcfsPlatformFirmwareVersionWrite,
};

static const struct file_operations _gProcDeviceDriverVersion = { 
    .read = _DrvProcfsDeviceDriverVersionRead,
    .write = _DrvProcfsDeviceDriverVersionWrite,
};

static const struct file_operations _gProcSdCardFirmwareUpdate = { 
    .read = _DrvProcfsSdCardFirmwareUpdateRead,
    .write = _DrvProcfsSdCardFirmwareUpdateWrite,
};

static const struct file_operations _gProcFirmwareDebug = { 
    .read = _DrvProcfsFirmwareDebugRead,
    .write = _DrvProcfsFirmwareDebugWrite,
};

static const struct file_operations _gProcFirmwareSetDebugValue = { 
    .read = _DrvProcfsFirmwareSetDebugValueRead,
    .write = _DrvProcfsFirmwareSetDebugValueWrite,
};

static const struct file_operations _gProcFirmwareSmBusDebug = { 
    .read = _DrvProcfsFirmwareSmBusDebugRead,
    .write = _DrvProcfsFirmwareSmBusDebugWrite,
};

static const struct file_operations _gProcFirmwareSetDQMemValue = {
    .read = _DrvProcfsFirmwareSetDQMemValueRead,
    .write = _DrvProcfsFirmwareSetDQMemValueWrite,
};

#ifdef CONFIG_ENABLE_ITO_MP_TEST
static const struct file_operations _gProcMpTest = { 
    .read = _DrvProcfsMpTestRead,
    .write = _DrvProcfsMpTestWrite,
};

static const struct file_operations _gProcMpTestLog = { 
    .read = _DrvProcfsMpTestLogRead,
    .write = _DrvProcfsMpTestLogWrite,
};

static const struct file_operations _gProcMpTestFailChannel = { 
    .read = _DrvProcfsMpTestFailChannelRead,
    .write = _DrvProcfsMpTestFailChannelWrite,
};

static const struct file_operations _gProcMpTestScope = { 
    .read = _DrvProcfsMpTestScopeRead,
    .write = _DrvProcfsMpTestScopeWrite,
};

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
static const struct file_operations _gProcMpTestLogAll = {
    .read = _DrvProcfsMpTestLogAllRead,
    .write = _DrvProcfsMpTestLogAllWrite,
};
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#endif //CONFIG_ENABLE_ITO_MP_TEST

static const struct file_operations _gProcFirmwareMode = { 
    .read = _DrvProcfsFirmwareModeRead,
    .write = _DrvProcfsFirmwareModeWrite,
};

static const struct file_operations _gProcFirmwareSensor = { 
    .read = _DrvProcfsFirmwareSensorRead,
    .write = _DrvProcfsFirmwareSensorWrite,
};

static const struct file_operations _gProcFirmwarePacketHeader = { 
    .read = _DrvProcfsFirmwarePacketHeaderRead,
    .write = _DrvProcfsFirmwarePacketHeaderWrite,
};

static const struct file_operations _gProcQueryFeatureSupportStatus = { 
    .read = _DrvProcfsQueryFeatureSupportStatusRead,
    .write = _DrvProcfsQueryFeatureSupportStatusWrite,
};

static const struct file_operations _gProcChangeFeatureSupportStatus = { 
    .read = _DrvProcfsChangeFeatureSupportStatusRead,
    .write = _DrvProcfsChangeFeatureSupportStatusWrite,
};

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
static const struct file_operations _gProcGestureWakeupMode = { 
    .read = _DrvProcfsGestureWakeupModeRead,
    .write = _DrvProcfsGestureWakeupModeWrite,
};
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
static const struct file_operations _gProcGestureDebugMode = { 
    .read = _DrvProcfsGestureDebugModeRead,
    .write = _DrvProcfsGestureDebugModeWrite,
};
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static const struct file_operations _gProcGestureInforMode = { 
    .read = _DrvProcfsGestureInforModeRead,
    .write = _DrvProcfsGestureInforModeWrite,
};
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
static const struct file_operations _gProcReportRate = { 
    .read = _DrvProcfsReportRateRead,
    .write = _DrvProcfsReportRateWrite,
};
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

static const struct file_operations _gProcGloveMode= {
    .read = _DrvProcfsGloveModeRead,
    .write = _DrvProcfsGloveModeWrite,
};

static const struct file_operations _gProcOpenGloveMode= {
    .read = _DrvProcfsOpenGloveModeRead,
};

static const struct file_operations _gProcCloseGloveMode= {
    .read = _DrvProcfsCloseGloveModeRead,
};

static const struct file_operations _gProcLeatherSheathMode= {
    .read = _DrvProcfsLeatherSheathModeRead,
    .write = _DrvProcfsLeatherSheathModeWrite,
};
static const struct file_operations _gProcFilmMode= {
    .read = _DrvProcfsGetFilmModeRead,
    .write = _DrvProcfsSetFilmModeWrite,
};

#ifdef CONFIG_ENABLE_JNI_INTERFACE
static const struct file_operations _gProcJniMethod = {
    .read = _DrvJniMsgToolRead,
    .write = _DrvJniMsgToolWrite,
    .unlocked_ioctl = _DrvJniMsgToolIoctl,    
    .compat_ioctl = _DrvJniMsgToolIoctl,    
};
#endif //CONFIG_ENABLE_JNI_INTERFACE

static const struct file_operations _gProcSeLinuxLimitFirmwareUpdate = { 
    .read = _DrvProcfsSeLinuxLimitFirmwareUpdateRead,
};

static const struct file_operations _gProcForceFirmwareUpdate = { 
    .read = _DrvProcfsForceFirmwareUpdateRead,
};
static const struct file_operations _gProcTrimCode = { 
    .read = _DrvProcfsTrimCodeRead,
    .write = _DrvProcfsTrimCodeWrite,
};
//------------------------------------------------------------------------------//

u32 SLAVE_I2C_ID_DBBUS = (0xC4>>1); //0x62 // for MSG28xx/MSG58xxA/ILI2117A/ILI2118A
//u32 SLAVE_I2C_ID_DBBUS = (0xB2>>1); //0x59 // for MSG22xx
u32 SLAVE_I2C_ID_DWI2C = (0x4C>>1); //0x26 
//u32 SLAVE_I2C_ID_DWI2C = (0x82>>1); //0x41 // for ILI21xx

u16 FIRMWARE_MODE_UNKNOWN_MODE = 0xFFFF;
u16 FIRMWARE_MODE_DEMO_MODE = 0xFFFF;
u16 FIRMWARE_MODE_DEBUG_MODE = 0xFFFF;
u16 FIRMWARE_MODE_RAW_DATA_MODE = 0xFFFF;

u16 DEMO_MODE_PACKET_LENGTH = 0; // If project use MSG28xx, set MUTUAL_DEMO_MODE_PACKET_LENGTH as default. If project use MSG22xx, set SELF_DEMO_MODE_PACKET_LENGTH as default. 
u16 DEBUG_MODE_PACKET_LENGTH = 0; // If project use MSG28xx, set MUTUAL_DEBUG_MODE_PACKET_LENGTH as default. If project use MSG22xx, set SELF_DEBUG_MODE_PACKET_LENGTH as default. 
u16 MAX_TOUCH_NUM = 0; // If project use MSG28xx, set MUTUAL_MAX_TOUCH_NUM as default. If project use MSG22xx, set SELF_MAX_TOUCH_NUM as default. 

struct kset *g_TouchKSet = NULL;
struct kobject *g_TouchKObj = NULL;
u8 g_IsSwitchModeByAPK = 0;

u8 IS_GESTURE_WAKEUP_ENABLED = 0;
u8 IS_GESTURE_DEBUG_MODE_ENABLED = 0;
u8 IS_GESTURE_INFORMATION_MODE_ENABLED = 0;
u8 IS_GESTURE_WAKEUP_MODE_SUPPORT_64_TYPES_ENABLED = 0;

u8 TOUCH_DRIVER_DEBUG_LOG_LEVEL = CONFIG_TOUCH_DRIVER_DEBUG_LOG_LEVEL;
u8 IS_FIRMWARE_DATA_LOG_ENABLED = CONFIG_ENABLE_FIRMWARE_DATA_LOG;
u8 IS_APK_PRINT_FIRMWARE_SPECIFIC_LOG_ENABLED = CONFIG_ENABLE_APK_PRINT_FIRMWARE_SPECIFIC_LOG;
u8 IS_SELF_FREQ_SCAN_ENABLED = 0;
u8 IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = 0;
u8 IS_DISABLE_ESD_PROTECTION_CHECK = 0;


#ifdef CONFIG_TP_HAVE_KEY
int g_TpVirtualKey[] = {TOUCH_KEY_MENU, TOUCH_KEY_HOME, TOUCH_KEY_BACK, TOUCH_KEY_SEARCH};

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
struct kobject *g_PropertiesKObj = NULL;
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#define BUTTON_W (100)
#define BUTTON_H (100)

int g_TpVirtualKeyDimLocal[MAX_KEY_NUM][4] = {{(TOUCH_SCREEN_X_MAX/4)/2*1,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H},
                                                    {(TOUCH_SCREEN_X_MAX/4)/2*3,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H},
                                                    {(TOUCH_SCREEN_X_MAX/4)/2*5,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H},
                                                    {(TOUCH_SCREEN_X_MAX/4)/2*7,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H}};
#endif 
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

#ifdef CONFIG_ENABLE_ESD_PROTECTION
int g_IsEnableEsdCheck = 1;
struct delayed_work g_EsdCheckWork;
struct workqueue_struct *g_EsdCheckWorkqueue = NULL;
u8 g_IsHwResetByDriver = 0;
void DrvEsdCheck(struct work_struct *pWork);
#endif //CONFIG_ENABLE_ESD_PROTECTION

struct input_dev *g_InputDevice = NULL;
struct mutex g_Mutex;

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
u32 g_IsEnableReportRate = 0;
u32 g_InterruptCount = 0;
u32 g_ValidTouchCount = 0;
u32 g_InterruptReportRate = 0;
u32 g_ValidTouchReportRate = 0;
struct timeval g_StartTime;
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

u8 g_IsEnableGloveMode = 0;
u8 g_IsEnableLeatherSheathMode = 0;

u8 g_FwData[MAX_UPDATE_FIRMWARE_BUFFER_SIZE][1024];
u32 g_FwDataCount = 0;

u8 g_IsHotknotEnabled = 0;
u8 g_IsBypassHotknot = 0;

u16 g_ChipType = 0;
u16 g_OriginalChipType = 0;

u8 g_DemoModePacket[MUTUAL_DEMO_MODE_PACKET_LENGTH] = {0}; // for MSG22xx : DEMO_MODE_PACKET_LENGTH = SELF_DEMO_MODE_PACKET_LENGTH, for MSG28xx/MSG58xxA : DEMO_MODE_PACKET_LENGTH = MUTUAL_DEMO_MODE_PACKET_LENGTH, for ILI21xx : DEMO_MODE_PACKET_LENGTH = ILI21XX_DEMO_MODE_PACKET_LENGTH

MutualFirmwareInfo_t g_MutualFirmwareInfo;
SelfFirmwareInfo_t g_SelfFirmwareInfo;

u8 g_DebugModePacket[MUTUAL_DEBUG_MODE_PACKET_LENGTH] = {0}; // for MSG22xx : DEBUG_MODE_PACKET_LENGTH = SELF_DEBUG_MODE_PACKET_LENGTH, for MSG28xx/MSG58xxA/ILI21xx : DEBUG_MODE_PACKET_LENGTH = MUTUAL_DEBUG_MODE_PACKET_LENGTH
u8 g_LogModePacket[MUTUAL_DEBUG_MODE_PACKET_LENGTH] = {0}; // for MSG22xx : DEBUG_MODE_PACKET_LENGTH = SELF_DEBUG_MODE_PACKET_LENGTH, for MSG28xx/MSG58xxA/ILI21xx : DEBUG_MODE_PACKET_LENGTH = MUTUAL_DEBUG_MODE_PACKET_LENGTH
u16 g_FirmwareMode;

u8 g_IsUpdateFirmware = 0x00;
u8 g_Msg22xxChipRevision = 0x00;
u8 g_IsDisableFingerTouch = 0;
//u32 u32TouchCount = 0;

bool g_GestureState = false;

/*=============================================================*/
// FUNCTION DECLARATION
/*=============================================================*/

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern void DrvCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID
extern void _DrvMsg28xxSetProtectBit(void);
extern void _DrvMsg28xxEraseEmem(EmemType_e eEmemType);
static s32 _DrvCreateProcfsDirEntry(void);
static void _DrvRemoveProcfsDirEntry(void);
static s32 _DrvSelfParsePacket(u8 *pPacket, u16 nLength, SelfTouchInfo_t *pInfo);
static s32 _DrvMutualParsePacket(u8 *pPacket, u16 nLength, MutualTouchInfo_t *pInfo);

extern s32 DrvUpdateFirmwareCash(u8 szFwData[][1024], EmemType_e eEmemType);

extern u16 DrvGetChipType(void);
extern s32 DrvUpdateFirmwareBySdCard(const char *pFilePath);
extern s32 DrvIliTekUpdateFirmwareBySdCard(const char *pFilePath);
extern u32 DrvReadDQMemValue(u16 nAddr);
extern void DrvWriteDQMemValue(u16 nAddr, u32 nData);
extern void DrvOptimizeCurrentConsumption(void);

void DrvTouchDevicePowerOn(void);
void DrvTouchDevicePowerOff(void);
void DrvTouchDeviceHwReset(void);

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
void DrvOpenGestureDebugMode(u8 nGestureFlag);
void DrvCloseGestureDebugMode(void);
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static void _DrvConvertGestureInformationModeCoordinate(u8 *pRawData, u32 *pTranX, u32 *pTranY);
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
s32 DrvEnableProximity(void);
s32 DrvDisableProximity(void);
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
void DrvTpPsEnable(int nEnable);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
int DrvTpPsEnable(struct sensors_classdev* pProximityCdev, unsigned int nEnable);
#endif
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

#ifdef CONFIG_ENABLE_DMA_IIC
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>

static unsigned char *I2CDMABuf_va = NULL;
static dma_addr_t I2CDMABuf_pa = 0;

void DmaAlloc(void)
{
    if (NULL == I2CDMABuf_va)
    {
        if (NULL != g_InputDevice)
        {
            g_InputDevice->dev.coherent_dma_mask = DMA_BIT_MASK(32);
            I2CDMABuf_va = (u8 *)dma_alloc_coherent(&g_InputDevice->dev, MAX_I2C_TRANSACTION_LENGTH_LIMIT, &I2CDMABuf_pa, GFP_KERNEL);
        }
    }
    
    if (NULL == I2CDMABuf_va)
    {
        DBG(&g_I2cClient->dev, "DmaAlloc FAILED!\n");
    }
    else
    {
        DBG(&g_I2cClient->dev, "DmaAlloc SUCCESS!\n");
    }
}

void DmaReset(void)
{
    DBG(&g_I2cClient->dev, "Dma memory reset!\n");

    memset(I2CDMABuf_va, 0, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
}

void DmaFree(void)
{
    if (NULL != I2CDMABuf_va)
    {
	      if (NULL != g_InputDevice)
	      {
	          dma_free_coherent(&g_InputDevice->dev, MAX_I2C_TRANSACTION_LENGTH_LIMIT, I2CDMABuf_va, I2CDMABuf_pa);
	      }
	      I2CDMABuf_va = NULL;
	      I2CDMABuf_pa = 0;

        DBG(&g_I2cClient->dev, "DmaFree SUCCESS!\n");
    }
}
#endif //CONFIG_ENABLE_DMA_IIC

u16 RegGet16BitValue(u16 nAddr)
{
    u8 szTxData[3] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF};
    u8 szRxData[2] = {0};

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szRxData[0], 2);

    return (szRxData[1] << 8 | szRxData[0]);
}

u8 RegGetLByteValue(u16 nAddr)
{
    u8 szTxData[3] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF};
    u8 szRxData = {0};

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szRxData, 1);

    return (szRxData);
}

u8 RegGetHByteValue(u16 nAddr)
{
    u8 szTxData[3] = {0x10, (nAddr >> 8) & 0xFF, (nAddr & 0xFF) + 1};
    u8 szRxData = {0};

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DBBUS, &szRxData, 1);

    return (szRxData);
}

void RegGetXBitValue(u16 nAddr, u8 * pRxData, u16 nLength, u16 nMaxI2cLengthLimit)
{
    u16 nReadAddr = nAddr;    
    u16 nReadSize = 0;
    u16 nLeft = nLength;    
    u16 nOffset = 0; 
    u8 szTxData[3] = {0};

    szTxData[0] = 0x10;

    mutex_lock(&g_Mutex);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    while(nLeft > 0)
    {
        if(nLeft >= nMaxI2cLengthLimit)
        {
            nReadSize = nMaxI2cLengthLimit;
            DBG(&g_I2cClient->dev, "*** RegGetXBitValue# Length >= I2cMax   nReadAddr=%x, nReadSize=%d ***\n", nReadAddr, nReadSize);

            szTxData[1] = (nReadAddr >> 8) & 0xFF;
            szTxData[2] = nReadAddr & 0xFF;            
        
            IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
            IicReadData(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
        
            nReadAddr = nReadAddr + nReadSize;    //set next read address
            nLeft = nLeft - nReadSize;
            nOffset = nOffset + nReadSize;
            DBG(&g_I2cClient->dev, "*** RegGetXBitValue# Length >= I2cMax   nLeft=%d, nOffset=%d ***\n", nLeft, nOffset);
        }
        else
        {
            nReadSize = nLeft;
            DBG(&g_I2cClient->dev, "*** RegGetXBitValue# Length < I2cMax   nReadAddr=%x, nReadSize=%d ***\n", nReadAddr, nReadSize);

            szTxData[1] = (nReadAddr >> 8) & 0xFF;
            szTxData[2] = nReadAddr & 0xFF;            
        
            IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 3);
            IicReadData(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
            
            nLeft = 0;
            nOffset = nOffset + nReadSize;            
            DBG(&g_I2cClient->dev, "*** RegGetXBitValue# Length < I2cMax   nLeft=%d, nOffset=%d ***\n", nLeft, nOffset);
        }
    }

    mutex_unlock(&g_Mutex);    
}

void RegGetXBitWrite4ByteValue(u16 nAddr, u8 * pRxData, u16 nLength, u16 nMaxI2cLengthLimit)
{
    u16 nReadAddr = nAddr;    
    u16 nReadSize = 0;
    u16 nLeft = nLength;    
    u16 nOffset = 0; 
    u8 szTxData[4] = {0};

    szTxData[0] = 0x10;

    mutex_lock(&g_Mutex);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    while(nLeft > 0)
    {
        if(nLeft >= nMaxI2cLengthLimit)
        {
            nReadSize = nMaxI2cLengthLimit;
            DBG(&g_I2cClient->dev, "*** RegGetXBitValue# Length >= I2cMax   nReadAddr=%x, nReadSize=%d ***\n", nReadAddr, nReadSize);

            szTxData[2] = (nReadAddr >> 8) & 0xFF;
            szTxData[3] = nReadAddr & 0xFF;            
        
            IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 4);
            IicReadData(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
        
            nReadAddr = nReadAddr + nReadSize;    //set next read address
            nLeft = nLeft - nReadSize;
            nOffset = nOffset + nReadSize;
            DBG(&g_I2cClient->dev, "*** RegGetXBitValue# Length >= I2cMax   nLeft=%d, nOffset=%d ***\n", nLeft, nOffset);
        }
        else
        {
            nReadSize = nLeft;
            DBG(&g_I2cClient->dev, "*** RegGetXBitValue# Length < I2cMax   nReadAddr=%x, nReadSize=%d ***\n", nReadAddr, nReadSize);

            szTxData[2] = (nReadAddr >> 8) & 0xFF;
            szTxData[3] = nReadAddr & 0xFF;            
        
            IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 4);
            IicReadData(SLAVE_I2C_ID_DBBUS, &pRxData[nOffset], nReadSize);
            
            nLeft = 0;
            nOffset = nOffset + nReadSize;            
            DBG(&g_I2cClient->dev, "*** RegGetXBitValue# Length < I2cMax   nLeft=%d, nOffset=%d ***\n", nLeft, nOffset);
        }
    }

    mutex_unlock(&g_Mutex);    
}

s32 RegSet16BitValue(u16 nAddr, u16 nData)
{
    s32 rc = 0;
    u8 szTxData[5] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF, nData & 0xFF, nData >> 8};

    rc = IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 5);

    return rc;
}

s32 RetryRegSet16BitValue(u16 nAddr, u16 nData)
{
    s32 rc = 0;
    u32 nRetryCount = 0;
    
    while (nRetryCount < 5)
    {
        mdelay(5);
        rc = RegSet16BitValue(nAddr, nData);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "RegSet16BitValue(0x%x, 0x%x) success, rc = %d\n", nAddr, nData, rc);
            break;
        }

        nRetryCount ++;
    }
    if (nRetryCount == 5)
    {
        DBG(&g_I2cClient->dev, "RegSet16BitValue(0x%x, 0x%x) failed, rc = %d\n", nAddr, nData, rc);
    }
    
    return rc;
}

void RegSetLByteValue(u16 nAddr, u8 nData)
{
    u8 szTxData[4] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF, nData};
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 4);
}

void RegSetHByteValue(u16 nAddr, u8 nData)
{
    u8 szTxData[4] = {0x10, (nAddr >> 8) & 0xFF, (nAddr & 0xFF) + 1, nData};
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szTxData[0], 4);
}

void RegSet16BitValueOn(u16 nAddr, u16 nData) //Set bit on nData from 0 to 1
{
    u16 rData = RegGet16BitValue(nAddr);
    rData |= nData;
    RegSet16BitValue(nAddr, rData);
}

void RegSet16BitValueOff(u16 nAddr, u16 nData) //Set bit on nData from 1 to 0
{
    u16 rData = RegGet16BitValue(nAddr);
    rData &= (~nData);
    RegSet16BitValue(nAddr, rData);
}

u16 RegGet16BitValueByAddressMode(u16 nAddr, AddressMode_e eAddressMode)
{
    u16 nData = 0;
    
    if (eAddressMode == ADDRESS_MODE_16BIT)
    {
        nAddr = nAddr - (nAddr & 0xFF) + ((nAddr & 0xFF) << 1);
    }
    
    nData = RegGet16BitValue(nAddr);
    
    return nData;
}
	
void RegSet16BitValueByAddressMode(u16 nAddr, u16 nData, AddressMode_e eAddressMode)
{
    if (eAddressMode == ADDRESS_MODE_16BIT)
    {
        nAddr = nAddr - (nAddr & 0xFF) + ((nAddr & 0xFF) << 1);
    }
    
    RegSet16BitValue(nAddr, nData);
}

void RegMask16BitValue(u16 nAddr, u16 nMask, u16 nData, AddressMode_e eAddressMode) 
{
    u16 nTmpData = 0;
    
    if (nData > nMask)
    {
        return;
    }

    nTmpData = RegGet16BitValueByAddressMode(nAddr, eAddressMode);
    nTmpData = (nTmpData & (~nMask));
    nTmpData = (nTmpData | nData);
    RegSet16BitValueByAddressMode(nAddr, nTmpData, eAddressMode);
}

static void ReadFlashFinale28XX(void)
{
    DBG(&g_I2cClient->dev, "*** %s *** \n", __func__);

    // set read done
    RegSet16BitValue(0x1606, 0x02);

    // unset info flag
    RegSetLByteValue(0x1607, 0x00);

    // clear addr
    RegSet16BitValue(0x1600, 0x00);
}

static void ReadFlashInit28XX(u16 cayenne_address,int nBlockType)
{
    DBG(&g_I2cClient->dev, "*** %s *** \n", __func__);

    //wriu 0x1608 0x20
	RegSet16BitValue(0x1608, 0x20);
    //wriu 0x1606 0x20
    RegSet16BitValue(0x1606, 0x20);

    //wriu read done
    RegSet16BitValue(0x1606, 0x02);

    //set address
    RegSet16BitValue(0x1600, cayenne_address);

    if (nBlockType == EMEM_TYPE_INFO_BLOCK)
    {
    	//set Info Block
    	RegSetLByteValue((uint)0x1607, (uint)0x08);

        //set Info Double Buffer
    	RegSetLByteValue((uint)0x1604, (uint)0x01);
    }
    else
    {
        //set Main Block
    	RegSetLByteValue((uint)0x1607, (uint)0x00);

        //set Main Double Buffer
    	RegSetLByteValue((uint)0x1604, (uint)0x01);
    }
    // set FPGA flag
    RegSetLByteValue( 0x1610, 0x01);

    // set mode trigger
    if (nBlockType == EMEM_TYPE_INFO_BLOCK)
    {
        // set eflash mode to read mode
        // set info flag
        RegSet16BitValue(0x1606, 0x0801);
    }
    else
    {
        // set eflash mode to read mode
        RegSet16BitValue(0x1606, 0x0001);
    }
}

static int ReadFlashRIU28XX(u32 nAddr,int nBlockType,int nLength,u8 *pFlashData)
{
    uint read_16_addr_a = 0, read_16_addr_c = 0;

    DBG(&g_I2cClient->dev, "*** %s() nAddr:0x%x***\n", __func__, nAddr);

    //set read address
    RegSet16BitValue(0x1600, nAddr);

    //read 16+16 bits
    read_16_addr_a = RegGet16BitValue(0x160a);
    read_16_addr_c = RegGet16BitValue(0x160c);

    //DEBUG("*** %s() read_16_addr_a:%x read_16_addr_c:%x***\n",__func__,read_16_addr_a,read_16_addr_c);

    pFlashData[0] = (u8)(read_16_addr_a & 0xff);
    pFlashData[1] = (u8)((read_16_addr_a >> 8) & 0xff);
    pFlashData[2] = (u8)(read_16_addr_c & 0xff);
    pFlashData[3] = (u8)((read_16_addr_c >> 8) & 0xff);
    //DEBUG("*** %s() pFlashData[0]:0x%x pFlashData[1]:0x%x pFlashData[2]:0x%x pFlashData[3]:0x%x***\n",
    //		__func__,pFlashData[0],pFlashData[1],pFlashData[2],pFlashData[3]);
    return 0;
}

static int ReadFlash28XX(u32 nAddr,int nBlockType,int nLength,u8 *pFlashData)
{
	u16 _28xx_addr=nAddr/4;
	u32 addr_star,addr_end,addr_step;
	u32 read_byte=0;

	addr_star=nAddr;
	addr_end=nAddr+nLength;

	if ((addr_star>=EMEM_SIZE_MSG28XX) || (addr_end > EMEM_SIZE_MSG28XX))
	{
        DBG(&g_I2cClient->dev, "*** %s : addr_start = 0x%x , addr_end = 0x%x *** \n", __func__, addr_star,addr_end);
		return -1;
	}

	addr_step=4;

    ReadFlashInit28XX(_28xx_addr,nBlockType);

    for(addr_star=nAddr;addr_star<addr_end;addr_star+=addr_step)
    {
    	_28xx_addr=addr_star/4;

    	DBG(&g_I2cClient->dev, "*** %s() _28xx_addr:0x%x addr_star:0x%x addr_end:%x nLength:%d pFlashData:%p***\n", __func__,_28xx_addr,addr_star,addr_end,nLength,pFlashData);
    	ReadFlashRIU28XX(_28xx_addr,nBlockType,nLength,(pFlashData+read_byte));
    	DBG(&g_I2cClient->dev,"*** %s() pFlashData[%x]: %02x %02x %02x %02x read_byte:%d \n", __func__, addr_star, pFlashData[read_byte], pFlashData[read_byte+1], pFlashData[read_byte+2],pFlashData[read_byte+3],read_byte);
    	//pFlashData+=addr_step;
    	read_byte+=4;
    }

    ReadFlashFinale28XX();

	return 0;
}

int ReadFlash(u8 nChipType,u32 nAddr,int nBlockType,int nLength,u8 *pFlashData)
{
    int ret=0;

    DBG(&g_I2cClient->dev, "*** %s()  nChipType 0x%x***\n", __func__,nChipType);

	switch(nChipType) {

		case  CHIP_TYPE_MSG28XX:
		case  CHIP_TYPE_MSG58XXA:
			ret=ReadFlash28XX(nAddr,nBlockType,nLength,pFlashData);
		break;

		default:
		break;
	}

    return ret;
}

//void DrvGetProjectInfo(u8 project_id, u8 color_id, u8 golden_sample_version)
void DrvGetProjectInfo(void)
{
    u8 temp[12];
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    ReadFlash(g_ChipType,0x0780,EMEM_TYPE_INFO_BLOCK,12,temp);

    // printk("project ID: %s\n", project_id);
    // printk("Color ID: 0x%x,0x%x\n", color_id[0], color_id[1]);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

}

s32 DbBusEnterSerialDebugMode(void)
{
    s32 rc = 0;
    u8 data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;

    rc = IicWriteData(SLAVE_I2C_ID_DBBUS, data, 5);
    // DbBusIICUseBus();
    // DbBusIICReshape();

    return rc;
}

void DbBusExitSerialDebugMode(void)
{
    u8 data[1];
    // Exit the Serial Debug Mode
    data[0] = 0x45;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);

    // Delay some interval to guard the next transaction
//    udelay(200);        // delay about 0.2ms
}

 void DrvDBBusI2cResponseAck(void)
 {
    // i2c response ack
    if(g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA)
    {
        RegMask16BitValue(0x1E4F, BIT15, BIT15, ADDRESS_MODE_16BIT);
    }

 }
void DbBusIICUseBus(void)
{
    u8 data[1];

    // IIC Use Bus
    data[0] = 0x35;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusIICNotUseBus(void)
{
    u8 data[1];

    // IIC Not Use Bus
    data[0] = 0x34;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusIICReshape(void)
{
    u8 data[1];

    // IIC Re-shape
    data[0] = 0x71;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusStopMCU(void)
{
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusNotStopMCU(void)
{
    u8 data[1];

    // Not Stop the MCU
    data[0] = 0x36;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusResetSlave(void)
{
    u8 data[1];

    // IIC Reset Slave
    data[0] = 0x00;

    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);
}

void DbBusWaitMCU(void)
{
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;
    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);

    data[0] = 0x61;
    IicWriteData(SLAVE_I2C_ID_DBBUS, data, 1);    
}

void SetCfb(u8 Cfb)
{
	/// Setting Cfb
	switch (Cfb)
	{
		case _50p: /// Cfb = 50p
			RegMask16BitValue(0x1528, (u16)0x0070, (u16)0x0000, ADDRESS_MODE_16BIT);
			RegMask16BitValue(0x1523, (u16)0x0700, (u16)0x0000, ADDRESS_MODE_16BIT);	/// 0x1523[10:8] = 0x0, Rfb: 180kohm
	        break;

	    case _40p: /// Cfb = 40p
	    	RegMask16BitValue(0x1528, (u16)0x0070, (u16)0x0020, ADDRESS_MODE_16BIT);
	    	RegMask16BitValue(0x1523, (u16)0x0700, (u16)0x0100, ADDRESS_MODE_16BIT);	/// 0x1523[10:8] = 0x1, Rfb: 225kohm
	        break;

	    case _30p: /// Cfb = 30p
	    	RegMask16BitValue(0x1528, (u16)0x0070, (u16)0x0040, ADDRESS_MODE_16BIT);
	    	RegMask16BitValue(0x1523, (u16)0x0700, (u16)0x0200, ADDRESS_MODE_16BIT);	/// 0x1523[10:8] = 0x2, Rfb: 300kohm
	    	break;

	    case _20p: /// Cfb = 20p
	    	RegMask16BitValue(0x1528, (u16)0x0070, (u16)0x0060, ADDRESS_MODE_16BIT);
	      	RegMask16BitValue(0x1523, (u16)0x0700, (u16)0x0200, ADDRESS_MODE_16BIT);	/// 0x1523[10:8] = 0x2, Rfb: 300kohm
	        break;

	    case _10p: /// Cfb = 10p
	    	RegMask16BitValue(0x1528, (u16)0x0070, (u16)0x0070, ADDRESS_MODE_16BIT);
	    	RegMask16BitValue(0x1523, (u16)0x0700, (u16)0x0200, ADDRESS_MODE_16BIT);	/// 0x1523[10:8] = 0x2, Rfb: 300kohm
	        break;

	    default:
	        break;
	}
}

s32 IicWriteData(u8 nSlaveId, u8* pBuf, u16 nSize)
{
    s32 rc = 0;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[] =
    {
        {
            .addr = nSlaveId,
            .flags = 0, // if read flag is undefined, then it means write flag.
            .len = nSize,
            .buf = pBuf,
        },
    };
#ifdef ROCKCHIP_PLATFORM
	msgs[0].scl_rate = 400000;
#endif //ROCKCHIP_PLATFORM
    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
       transmitted, else error code. */
    // printk("ILITEK I2C W:");
    // for(i = 0; i < nSize; i++)
    //     printk("0x%X, ", pBuf[i]);
    // printk("\n");
    if (g_I2cClient != NULL)
    {
        if ((g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A) && nSlaveId == SLAVE_I2C_ID_DWI2C && g_IsUpdateFirmware != 0)
        {
            PRINTF_ERR("Not allow to execute SmBus command while update firmware.\n");
        }
        else
        {
            rc = i2c_transfer(g_I2cClient->adapter, msgs, 1);

            // no return error if command is for serialDebug mode
            if(nSize == 5)
            {
                if(pBuf[0] == 0x53 && pBuf[1] == 0x45 && pBuf[2] == 0x52
                 && pBuf[3] == 0x44 && pBuf[4] == 0x42)
                 {
                     rc = nSize;
                     goto out;
                 }
            }
            
            if(nSize == 1)
            {
                if(pBuf[0] == 0x45)
                {
                    rc = nSize;
                    goto out;
                }
            }
            if (rc == 1)
            {
                rc = nSize;
            }
            else // rc < 0
            {
                PRINTF_ERR("IicWriteData() error %d\n", rc);
            }
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
        if ((g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A) && nSlaveId == SLAVE_I2C_ID_DWI2C && g_IsUpdateFirmware != 0)
        {
            PRINTF_ERR("Not allow to execute SmBus command while update firmware.\n");
        }
        else
        {
            u8 nAddrBefore = g_I2cClient->addr;
            g_I2cClient->addr = nSlaveId;

#ifdef CONFIG_ENABLE_DMA_IIC
            if (nSize > 8 && NULL != I2CDMABuf_va)
            {
                s32 i = 0;
	          
                for (i = 0; i < nSize; i ++)
                {
                    I2CDMABuf_va[i] = pBuf[i];
                }
                g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
                rc = i2c_master_send(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nSize);
            }
            else
            {
                g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);	
                rc = i2c_master_send(g_I2cClient, pBuf, nSize);
            }
#else
            rc = i2c_master_send(g_I2cClient, pBuf, nSize);
#endif //CONFIG_ENABLE_DMA_IIC
            g_I2cClient->addr = nAddrBefore;

            // no return error if command is for serialDebug mode
            if(nSize == 5)
            {
                if(pBuf[0] == 0x53 && pBuf[1] == 0x45 && pBuf[2] == 0x52
                 && pBuf[3] == 0x44 && pBuf[4] == 0x42)
                 {
                     rc = nSize;
                     goto out;
                 }
            }
            
            if(nSize == 1)
            {
                if(pBuf[0] == 0x45)
                {
                    rc = nSize;
                    goto out;
                }
            }
            if (rc < 0)
            {
                PRINTF_ERR("IicWriteData() error %d, nSlaveId=%d, nSize=%d\n", rc, nSlaveId, nSize);
            }
        }            
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#endif
    
out:
    return rc;
}

s32 IicReadData(u8 nSlaveId, u8* pBuf, u16 nSize)
{
    s32 rc = 0;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[] =
    {
        {
            .addr = nSlaveId,
            .flags = I2C_M_RD, // read flag
            .len = nSize,
            .buf = pBuf,
        },
    };
#ifdef ROCKCHIP_PLATFORM
	msgs[0].scl_rate = 400000;
#endif //ROCKCHIP_PLATFORM
    /* If everything went ok (i.e. 1 msg transmitted), return #bytes
       transmitted, else error code. */
    if (g_I2cClient != NULL)
    {
        if ((g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A) && nSlaveId == SLAVE_I2C_ID_DWI2C && g_IsUpdateFirmware != 0)
        {
            PRINTF_ERR("Not allow to execute SmBus command while update firmware.\n");
        }
        else
        {
            rc = i2c_transfer(g_I2cClient->adapter, msgs, 1);
        
            if (rc == 1)
            {
                rc = nSize;
            }
            else // rc < 0
            {
                PRINTF_ERR("IicReadData() error %d\n", rc);
            }
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
        if ((g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A) && nSlaveId == SLAVE_I2C_ID_DWI2C && g_IsUpdateFirmware != 0)
        {
            PRINTF_ERR("Not allow to execute SmBus command while update firmware.\n");
        }
        else
        {
            u8 nAddrBefore = g_I2cClient->addr;
            g_I2cClient->addr = nSlaveId;

#ifdef CONFIG_ENABLE_DMA_IIC
            if (nSize > 8 && NULL != I2CDMABuf_va)
            {
                s32 i = 0;
        
                g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
                rc = i2c_master_recv(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nSize);
        
                for (i = 0; i < nSize; i ++)
                {
                    pBuf[i] = I2CDMABuf_va[i];
                }
            }
            else
            {
                g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);	
                rc = i2c_master_recv(g_I2cClient, pBuf, nSize);
            }
#else
            rc = i2c_master_recv(g_I2cClient, pBuf, nSize);
#endif //CONFIG_ENABLE_DMA_IIC
            // printk("ILITEK I2C R:");
            // for(i = 0; i < nSize; i++)
            //     printk("0x%X, ", pBuf[i]);
            // printk("\n");
            g_I2cClient->addr = nAddrBefore;

            if (rc < 0)
            {
                PRINTF_ERR("IicReadData() error %d, nSlaveId=%d, nSize=%d\n", rc, nSlaveId, nSize);
            }
        }            
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#endif
    
    return rc;
}

s32 IicSegmentReadDataByDbBus(u8 nRegBank, u8 nRegAddr, u8* pBuf, u16 nSize, u16 nMaxI2cLengthLimit)
{
    s32 rc = 0;
    u16 nLeft = nSize;
    u16 nOffset = 0;
    u16 nSegmentLength = 0;
    u16 nReadSize = 0;
    u16 nOver = 0;
    u8  szWriteBuf[3] = {0};
    u8  nNextRegBank = nRegBank;
    u8  nNextRegAddr = nRegAddr;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[2] =
    {
        {
            .addr = SLAVE_I2C_ID_DBBUS,
            .flags = 0, // write flag
            .len = 3,
            .buf = szWriteBuf,
        },
        {
            .addr = SLAVE_I2C_ID_DBBUS,
            .flags =  I2C_M_RD, // read flag
        },
    };
#ifdef ROCKCHIP_PLATFORM
    msgs[0].scl_rate = 400000;
    msgs[1].scl_rate = 400000;
#endif //ROCKCHIP_PLATFORM
    // If everything went ok (i.e. 1 msg transmitted), return #bytes transmitted, else error code. 
    if (g_I2cClient != NULL)
    {
        if (nMaxI2cLengthLimit >= 256)
        {
            nSegmentLength = 256;
        }
        else
        {
            nSegmentLength = 128;
        }
        
        PRINTF_ERR("nSegmentLength = %d\n", nSegmentLength);	// add for debug

        while (nLeft > 0)
        {
            szWriteBuf[0] = 0x10;
            nRegBank = nNextRegBank;
            szWriteBuf[1] = nRegBank;
            nRegAddr = nNextRegAddr;
            szWriteBuf[2] = nRegAddr;

            PRINTF_ERR("nRegBank = 0x%x\n", nRegBank);	// add for debug
            PRINTF_ERR("nRegAddr = 0x%x\n", nRegAddr);	// add for debug

            msgs[1].buf = &pBuf[nOffset];

            if (nLeft > nSegmentLength)
            {
                if ((nRegAddr + nSegmentLength) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = nRegAddr + nSegmentLength; 
                    
                    PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                    msgs[1].len = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += msgs[1].len;
                }
                else if ((nRegAddr + nSegmentLength) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00; 		
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                    msgs[1].len = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += msgs[1].len;
                }
                else // ((nRegAddr + nSegmentLength) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_INFO("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                    nOver = (nRegAddr + nSegmentLength) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                    msgs[1].len = nSegmentLength - nOver; 		
                    nLeft -= msgs[1].len;
                    nOffset += msgs[1].len;
                }
            }
            else
            {
                if ((nRegAddr + nLeft) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = nRegAddr + nLeft; 
                    
                    PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                    msgs[1].len = nLeft;
                    nLeft = 0;
//                    nOffset += msgs[1].len;
                }
                else if ((nRegAddr + nLeft) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00; 		
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                    msgs[1].len = nLeft;
                    nLeft = 0;
//                    nOffset += msgs[1].len;
                }
                else // ((nRegAddr + nLeft) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                    nOver = (nRegAddr + nLeft) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                    msgs[1].len = nLeft - nOver; 		
                    nLeft -= msgs[1].len;
                    nOffset += msgs[1].len;
                }
            }

            rc = i2c_transfer(g_I2cClient->adapter, &msgs[0], 2);
            if (rc == 2)
            {
                nReadSize = nReadSize + msgs[1].len;
            }
            else // rc < 0
            {
                PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_transfer() error %d\n", rc);
                
                return rc;
            }
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
#ifndef CONFIG_ENABLE_DMA_IIC
        u8 *pReadBuf = NULL;
#endif //CONFIG_ENABLE_DMA_IIC
        u16 nLength = 0;
        u8 nAddrBefore = g_I2cClient->addr;
        
        g_I2cClient->addr = SLAVE_I2C_ID_DBBUS;

        if (nMaxI2cLengthLimit >= 256)
        {
            nSegmentLength = 256;
        }
        else
        {
            nSegmentLength = 128;
        }

        PRINTF_ERR("nSegmentLength = %d\n", nSegmentLength);	// add for debug

#ifdef CONFIG_ENABLE_DMA_IIC
        if (NULL != I2CDMABuf_va)
        {
            s32 i = 0;

            while (nLeft > 0)
            {
                szWriteBuf[0] = 0x10;
                nRegBank = nNextRegBank;
                szWriteBuf[1] = nRegBank;
                nRegAddr = nNextRegAddr;
                szWriteBuf[2] = nRegAddr;

                PRINTF_ERR("nRegBank = 0x%x\n", nRegBank);	// add for debug
                PRINTF_ERR("nRegAddr = 0x%x\n", nRegAddr);	// add for debug

                if (nLeft > nSegmentLength)
                {
                    if ((nRegAddr + nSegmentLength) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = nRegAddr + nSegmentLength; 
                    
                        PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                        nLength = nSegmentLength;
                        nLeft -= nSegmentLength;
                    }
                    else if ((nRegAddr + nSegmentLength) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = 0x00; 		
                        nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                        PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                        nLength = nSegmentLength;
                        nLeft -= nSegmentLength;
                    }
                    else // ((nRegAddr + nSegmentLength) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = 0x00;
                        nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                        PRINTF_INFO("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                        nOver = (nRegAddr + nSegmentLength) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                        PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                        nLength = nSegmentLength - nOver; 		
                        nLeft -= nLength;
                    }
                }
                else
                {
                    if ((nRegAddr + nLeft) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = nRegAddr + nLeft; 
                    
                        PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                        nLength = nLeft;
                        nLeft = 0;
                    }
                    else if ((nRegAddr + nLeft) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = 0x00; 		
                        nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                        PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                        nLength = nLeft;
                        nLeft = 0;
                    }
                    else // ((nRegAddr + nLeft) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                    {
                        nNextRegAddr = 0x00;
                        nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                        PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                        nOver = (nRegAddr + nLeft) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                        PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                        nLength = nLeft - nOver; 		
                        nLeft -= nLength;
                    }
                }

                g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);
                rc = i2c_master_send(g_I2cClient, &szWriteBuf[0], 3);
                if (rc < 0)
                {
                    PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_master_send() error %d\n", rc);

                    return rc;
                }

                g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
                rc = i2c_master_recv(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nLength);
                if (rc < 0)
                {
                    PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_master_recv() error %d\n", rc);
                    
                    return rc;
                }
                else
                {
                    for (i = 0; i < nLength; i ++)
                    {
                        pBuf[i+nOffset] = I2CDMABuf_va[i];
                    }
                    nOffset += nLength;

                    nReadSize = nReadSize + nLength;
                }
            }
        }
        else
        {
            PRINTF_ERR("IicSegmentReadDataByDbBus() -> I2CDMABuf_va is NULL\n");
        }
#else
        while (nLeft > 0)
        {
            szWriteBuf[0] = 0x10;
            nRegBank = nNextRegBank;
            szWriteBuf[1] = nRegBank;
            nRegAddr = nNextRegAddr;
            szWriteBuf[2] = nRegAddr;

            PRINTF_ERR("nRegBank = 0x%x\n", nRegBank);	// add for debug
            PRINTF_ERR("nRegAddr = 0x%x\n", nRegAddr);	// add for debug

            pReadBuf = &pBuf[nOffset];

            if (nLeft > nSegmentLength)
            {
                if ((nRegAddr + nSegmentLength) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = nRegAddr + nSegmentLength; 
                    
                    PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                    nLength = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += nLength;
                }
                else if ((nRegAddr + nSegmentLength) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00; 		
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                    nLength = nSegmentLength;
                    nLeft -= nSegmentLength;
                    nOffset += nLength;
                }
                else // ((nRegAddr + nSegmentLength) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_INFO("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                    nOver = (nRegAddr + nSegmentLength) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                    nLength = nSegmentLength - nOver; 		
                    nLeft -= nLength;
                    nOffset += nLength;
                }
            }
            else
            {
                if ((nRegAddr + nLeft) < MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = nRegAddr + nLeft; 
                    
                    PRINTF_ERR("nNextRegAddr = 0x%x\n", nNextRegAddr);	// add for debug

                    nLength = nLeft;
                    nLeft = 0;
//                    nOffset += nLength;
                }
                else if ((nRegAddr + nLeft) == MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00; 		
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug

                    nLength = nLeft;
                    nLeft = 0;
//                    nOffset += nLength;
                }
                else // ((nRegAddr + nLeft) > MAX_TOUCH_IC_REGISTER_BANK_SIZE)
                {
                    nNextRegAddr = 0x00;
                    nNextRegBank = nRegBank + 1; // shift to read data from next register bank

                    PRINTF_ERR("nNextRegBank = 0x%x\n", nNextRegBank);	// add for debug
                    
                    nOver = (nRegAddr + nLeft) - MAX_TOUCH_IC_REGISTER_BANK_SIZE;

                    PRINTF_ERR("nOver = 0x%x\n", nOver);	// add for debug

                    nLength = nLeft - nOver; 		
                    nLeft -= nLength;
                    nOffset += nLength;
                }
            }

            rc = i2c_master_send(g_I2cClient, &szWriteBuf[0], 3);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_master_send() error %d\n", rc);

                return rc;
            }

            rc = i2c_master_recv(g_I2cClient, pReadBuf, nLength);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataByDbBus() -> i2c_master_recv() error %d\n", rc);

                return rc;
            }
            else
            {
                nReadSize = nReadSize + nLength;
            }
        }
#endif //CONFIG_ENABLE_DMA_IIC
        g_I2cClient->addr = nAddrBefore;
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#endif
    
    return nReadSize;
}

s32 IicSegmentReadDataBySmBus(u16 nAddr, u8* pBuf, u16 nSize, u16 nMaxI2cLengthLimit)
{
    s32 rc = 0;
    u16 nLeft = nSize;
    u16 nOffset = 0;
    u16 nReadSize = 0;
    u8  szWriteBuf[3] = {0};

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    struct i2c_msg msgs[2] =
    {
        {
            .addr = SLAVE_I2C_ID_DWI2C,
            .flags = 0, // write flag
            .len = 3,
            .buf = szWriteBuf,
        },
        {
            .addr = SLAVE_I2C_ID_DWI2C,
            .flags =  I2C_M_RD, // read flag
        },
    };
#ifdef ROCKCHIP_PLATFORM
    msgs[0].scl_rate = 400000;
    msgs[1].scl_rate = 400000;
#endif //ROCKCHIP_PLATFORM
    // If everything went ok (i.e. 1 msg transmitted), return #bytes transmitted, else error code. 
    if (g_I2cClient != NULL)
    {
        while (nLeft > 0)
        {
            szWriteBuf[0] = 0x53;
            szWriteBuf[1] = ((nAddr + nOffset) >> 8) & 0xFF;
            szWriteBuf[2] = (nAddr + nOffset) & 0xFF;

            msgs[1].buf = &pBuf[nOffset];

            if (nLeft > nMaxI2cLengthLimit)
            {
                msgs[1].len = nMaxI2cLengthLimit;
                nLeft -= nMaxI2cLengthLimit;
                nOffset += msgs[1].len;
            }
            else
            {
                msgs[1].len = nLeft;
                nLeft = 0;
//                nOffset += msgs[1].len;
            }

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM
            mdelay(I2C_SMBUS_WRITE_COMMAND_DELAY_FOR_PLATFORM);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM

            rc = i2c_transfer(g_I2cClient->adapter, &msgs[0], 2);
            if (rc == 2)
            {
                nReadSize = nReadSize + msgs[1].len;
            }
            else // rc < 0
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_transfer() error %d\n", rc);
                
                return rc;
            }
        }
    }
    else
    {
        PRINTF_ERR("i2c client is NULL\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (g_I2cClient != NULL)
    {
#ifndef CONFIG_ENABLE_DMA_IIC
        u8 *pReadBuf = NULL;
#endif //CONFIG_ENABLE_DMA_IIC
        u16 nLength = 0;
        u8 nAddrBefore = g_I2cClient->addr;
        
        g_I2cClient->addr = SLAVE_I2C_ID_DWI2C;

#ifdef CONFIG_ENABLE_DMA_IIC
        while (nLeft > 0)
        {
            s32 i = 0;

            szWriteBuf[0] = 0x53;
            szWriteBuf[1] = ((nAddr + nOffset) >> 8) & 0xFF;
            szWriteBuf[2] = (nAddr + nOffset) & 0xFF;

            if (nLeft > nMaxI2cLengthLimit)
            {
                nLength = nMaxI2cLengthLimit;
                nLeft -= nMaxI2cLengthLimit;
            }
            else
            {
                nLength = nLeft;
                nLeft = 0;
            }
            
            g_I2cClient->ext_flag = g_I2cClient->ext_flag & (~I2C_DMA_FLAG);
            rc = i2c_master_send(g_I2cClient, &szWriteBuf[0], 3);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_master_send() error %d\n", rc);

                return rc;
            }

            g_I2cClient->ext_flag = g_I2cClient->ext_flag | I2C_DMA_FLAG;
            rc = i2c_master_recv(g_I2cClient, (unsigned char *)I2CDMABuf_pa, nLength);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_master_recv() error %d\n", rc);

                return rc;
            }
            else
            {
                for (i = 0; i < nLength; i ++)
                {
                    pBuf[i+nOffset] = I2CDMABuf_va[i];
                }
                nOffset += nLength;

                nReadSize = nReadSize + nLength;
            }
        }
#else
        while (nLeft > 0)
        {
            szWriteBuf[0] = 0x53;
            szWriteBuf[1] = ((nAddr + nOffset) >> 8) & 0xFF;
            szWriteBuf[2] = (nAddr + nOffset) & 0xFF;

            pReadBuf = &pBuf[nOffset];

            if (nLeft > nMaxI2cLengthLimit)
            {
                nLength = nMaxI2cLengthLimit;
                nLeft -= nMaxI2cLengthLimit;
                nOffset += nLength;
            }
            else
            {
                nLength = nLeft;
                nLeft = 0;
//                nOffset += nLength;
            }
            
            rc = i2c_master_send(g_I2cClient, &szWriteBuf[0], 3);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_master_send() error %d\n", rc);

                return rc;
            }

            rc = i2c_master_recv(g_I2cClient, pReadBuf, nLength);
            if (rc < 0)
            {
                PRINTF_ERR("IicSegmentReadDataBySmBus() -> i2c_master_recv() error %d\n", rc);

                return rc;
            }
            else
            {
                nReadSize = nReadSize + nLength;
            }
        }
#endif //CONFIG_ENABLE_DMA_IIC
        g_I2cClient->addr = nAddrBefore;
   }
   else
   {
       PRINTF_ERR("i2c client is NULL\n");
   }
#endif
	
   return nReadSize;
}

//------------------------------------------------------------------------------//

static u8 _DrvCalculateCheckSum(u8 *pMsg, u32 nLength)
{
    s32 nCheckSum = 0;
    u32 i;

    for (i = 0; i < nLength; i ++)
    {
        nCheckSum += pMsg[i];
    }

    return (u8)((-nCheckSum) & 0xFF);
}

static u32 _DrvConvertCharToHexDigit(char *pCh, u32 nLength)
{
    u32 nRetVal = 0;
    u32 i;
    
    DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);

    for (i = 0; i < nLength; i ++)
    {
        char ch = *pCh++;
        u32 n = 0;
        u8  nIsValidDigit = 0;
        
        if ((i == 0 && ch == '0') || (i == 1 && ch == 'x'))
        {
            continue;		
        }
        
        if ('0' <= ch && ch <= '9')
        {
            n = ch-'0';
            nIsValidDigit = 1;
        }
        else if ('a' <= ch && ch <= 'f')
        {
            n = 10 + ch-'a';
            nIsValidDigit = 1;
        }
        else if ('A' <= ch && ch <= 'F')
        {
            n = 10 + ch-'A';
            nIsValidDigit = 1;
        }
        
        if (1 == nIsValidDigit)
        {
            nRetVal = n + nRetVal*16;
        }
    }
    
    return nRetVal;
}

void DrvReadFile(char *pFilePath, u8 *pBuf, u16 nLength)
{
    struct file *pFile = NULL;
    mm_segment_t old_fs;
    ssize_t nReadBytes = 0;    

    old_fs = get_fs();
    set_fs(get_ds());

    pFile = filp_open(pFilePath, O_RDONLY, 0);
    if (IS_ERR(pFile)) {
        DBG(&g_I2cClient->dev, "Open file failed: %s\n", pFilePath);
        return;
    }

    pFile->f_op->llseek(pFile, 0, SEEK_SET);
    nReadBytes = pFile->f_op->read(pFile, pBuf, nLength, &pFile->f_pos);
    DBG(&g_I2cClient->dev, "Read %d bytes!\n", (int)nReadBytes);

    set_fs(old_fs);        
    filp_close(pFile, NULL);    
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_JNI_INTERFACE
/*
static void _DebugJniShowArray(u8 *pBuf, u16 nLen)
{
    int i;

    for (i = 0; i < nLen; i ++)
    {
        DBG(&g_I2cClient->dev, "%02X ", pBuf[i]);       

        if (i%16==15)
        {  
            DBG(&g_I2cClient->dev, "\n");
        }
    }
    DBG(&g_I2cClient->dev, "\n");    
}
*/
u64 PtrToU64(u8 * pValue)
{
	uintptr_t nValue = (uintptr_t)pValue;
    return (u64)(0xFFFFFFFFFFFFFFFF&nValue);
}

u8 * U64ToPtr(u64 nValue)
{
	uintptr_t pValue = (uintptr_t)nValue;
	return (u8 *)pValue;
}

static ssize_t _DrvJniMsgToolRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    long nRet = 0;
    u8 nBusType = 0;
    u16 nReadLen = 0;
    u8 szCmdData[20] = {0};
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    //DBG(&g_I2cClient->dev, "*** nCount = %d ***\n", (int)nCount);       
    nBusType = nCount&0xFF;
    nReadLen = (nCount >> 8)&0xFFFF;
    if (nBusType == SLAVE_I2C_ID_DBBUS || nBusType == SLAVE_I2C_ID_DWI2C)
    {    
        IicReadData(nBusType, &szCmdData[0], nReadLen);
    }
    
	nRet = copy_to_user(pBuffer, &szCmdData[0], nReadLen); 
    return nRet;
}			  

static ssize_t _DrvJniMsgToolWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{
    long nRet = 0;	                       
    u8 nBusType = 0;  
    u16 nWriteLen = 0;    
    u8 szCmdData[20] = {0};    
	u8 *Tempbuffer;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DBG(&g_I2cClient->dev, "*** nCount = %d ***\n", (int)nCount);
	// copy data from user space
	nBusType = nCount&0xFF;
	Tempbuffer = (u8*)kmalloc(1024, GFP_KERNEL);	
	DBG(&g_I2cClient->dev, "*** nCount = 0x%x ***\n", nBusType);
	if (nBusType == SLAVE_I2C_ID_DBBUS || nBusType == SLAVE_I2C_ID_DWI2C)
	{
		nWriteLen = (nCount >> 8)&0xFFFF;    
		nRet = copy_from_user(szCmdData, &pBuffer[0], nWriteLen);         
		if(nRet < 0){
			printk("%s, copy data from user space, failed", __func__);
			return -1;
		}	      
		IicWriteData(nBusType, &szCmdData[0], nWriteLen);  	
	}
	else{
		nRet = copy_from_user(Tempbuffer, pBuffer, nCount-1);
		printk("Write driver command:%s\n", Tempbuffer);
		if(nRet < 0){
			printk("%s, copy data from user space, failed", __func__);
			return -1;
		}	
		if (strcmp(Tempbuffer, "erase_flash") == 0) {
			DBG(&g_I2cClient->dev, "start Erase Flash\n");
			_DrvMsg28xxEraseEmem(EMEM_MAIN);
			DBG(&g_I2cClient->dev, "end Erase Flash\n");
		}
		kfree(Tempbuffer);
	}
    return nCount;
}

static void _DrvJniRegGetXByteData(MsgToolDrvCmd_t *pCmd)
{    
    u16 nAddr = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);   
     
    nAddr = (_gSndCmdData[1]<<8)|_gSndCmdData[0];    
    RegGetXBitValue(nAddr, _gRtnCmdData, pCmd->nRtnCmdLen, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
    //_DebugJniShowArray(_gRtnCmdData, pCmd->nRtnCmdLen);
}

static void _DrvJniClearMsgToolMem(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
  
    memset(_gMsgToolCmdIn, 0, sizeof(MsgToolDrvCmd_t));
    memset(_gSndCmdData, 0, 1024);
    memset(_gRtnCmdData, 0, 1024);
}

static MsgToolDrvCmd_t* _DrvJniTransCmdFromUser(unsigned long nArg)
{
    long nRet; 
    MsgToolDrvCmd_t tCmdIn;    
    MsgToolDrvCmd_t *pTransCmd;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);  
    
    _DrvJniClearMsgToolMem();
    
    pTransCmd = (MsgToolDrvCmd_t *)_gMsgToolCmdIn;    
    nRet = copy_from_user(&tCmdIn, (void*)nArg, sizeof(MsgToolDrvCmd_t));
    pTransCmd->nCmdId = tCmdIn.nCmdId;

    //_DebugJniShowArray(&tCmdIn, sizeof( MsgToolDrvCmd_t));
    if (tCmdIn.nSndCmdLen > 0)
    {
        pTransCmd->nSndCmdLen = tCmdIn.nSndCmdLen;
        nRet = copy_from_user(_gSndCmdData, U64ToPtr(tCmdIn.nSndCmdDataPtr), pTransCmd->nSndCmdLen);    	
    }

    if (tCmdIn.nRtnCmdLen > 0)
    {
	    pTransCmd->nRtnCmdLen = tCmdIn.nRtnCmdLen;
        nRet = copy_from_user(_gRtnCmdData, U64ToPtr(tCmdIn.nRtnCmdDataPtr), pTransCmd->nRtnCmdLen);    	        
    }
  
    return pTransCmd;
}

static void _DrvJniTransCmdToUser(MsgToolDrvCmd_t *pTransCmd, unsigned long nArg)
{
    MsgToolDrvCmd_t tCmdOut;
    long nRet;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);      
    
    nRet = copy_from_user(&tCmdOut, (void*)nArg, sizeof(MsgToolDrvCmd_t));   

    //_DebugJniShowArray(&tCmdOut, sizeof( MsgToolDrvCmd_t));    
    nRet = copy_to_user( U64ToPtr(tCmdOut.nRtnCmdDataPtr), _gRtnCmdData, tCmdOut.nRtnCmdLen);
}

static long _DrvJniMsgToolIoctl(struct file *pFile, unsigned int nCmd, unsigned long nArg)
{
    long nRet = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);    
    
    switch (nCmd)
    {
        case MSGTOOL_IOCTL_RUN_CMD:
            {      
                MsgToolDrvCmd_t *pTransCmd;			
                pTransCmd = _DrvJniTransCmdFromUser(nArg);  
                
                switch (pTransCmd->nCmdId)
                {
                    case MSGTOOL_RESETHW:
                        DrvTouchDeviceHwReset();
                        break;
                    case MSGTOOL_REGGETXBYTEVALUE:
                        _DrvJniRegGetXByteData(pTransCmd);                       
	                    _DrvJniTransCmdToUser(pTransCmd, nArg);                                                 
                        break;
                    case MSGTOOL_HOTKNOTSTATUS:
                        _gRtnCmdData[0] = g_IsHotknotEnabled;                       
                        _DrvJniTransCmdToUser(pTransCmd, nArg);                                                 
                        break;
                    case MSGTOOL_FINGERTOUCH:
                        if (pTransCmd->nSndCmdLen == 1)
                        {
                            DBG(&g_I2cClient->dev, "*** JNI enable touch ***\n");                        
                            DrvEnableFingerTouchReport();
                            g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after MTPTool APK have sent i2c command to firmware. 
                        }
                        else if (pTransCmd->nSndCmdLen == 0)
                        {
                            DBG(&g_I2cClient->dev, "*** JNI disable touch ***\n");                                                
                            DrvDisableFingerTouchReport();
                            g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for MTPTool APK can send i2c command to firmware. 
                        }
                        break;
                    case MSGTOOL_BYPASSHOTKNOT:
                        if (pTransCmd->nSndCmdLen == 1)
                        {
                            DBG(&g_I2cClient->dev, "*** JNI enable bypass hotknot ***\n");                                                
                            g_IsBypassHotknot = 1;                                                      
                        }
                        else if (pTransCmd->nSndCmdLen == 0)
                        {
                            DBG(&g_I2cClient->dev, "*** JNI disable bypass hotknot ***\n");                                                
                            g_IsBypassHotknot = 0;
                        }
                        break;
                    case MSGTOOL_DEVICEPOWEROFF:
                        DrvTouchDevicePowerOff();
                        break;                        
                    case MSGTOOL_GETSMDBBUS:
                        DBG(&g_I2cClient->dev, "*** MSGTOOL_GETSMDBBUS ***\n");
                        _gRtnCmdData[0] = SLAVE_I2C_ID_DBBUS&0xFF;                       
                        _gRtnCmdData[1] = SLAVE_I2C_ID_DWI2C&0xFF;                                               
                        _DrvJniTransCmdToUser(pTransCmd, nArg);                                                 
                        break;
                    case MSGTOOL_SETIICDATARATE:
                        DBG(&g_I2cClient->dev, "*** MSGTOOL_SETIICDATARATE ***\n");                        
                        DrvSetIicDataRate(g_I2cClient, ((_gSndCmdData[1]<<8)|_gSndCmdData[0])*1000);
                        break; 
                    case MSGTOOL_ERASE_FLASH:
                        DBG(&g_I2cClient->dev, "*** MSGTOOL_ERASE_FLASH ***\n");    
						if(pTransCmd->nSndCmdDataPtr == 0)
						{
							DBG(&g_I2cClient->dev, "*** erase Main block ***\n");  
							_DrvMsg28xxEraseEmem(EMEM_MAIN);
						}
						else if(pTransCmd->nSndCmdDataPtr == 1)
						{
							DBG(&g_I2cClient->dev, "*** erase INFO block ***\n");  
							_DrvMsg28xxEraseEmem(EMEM_INFO);
						}
                        break;                                
                    default:  
                        break;
                }		            
            }   
		    break;
		
        default:
            nRet = -EINVAL;
            break;
    }

    return nRet;
}

static void _DrvJniCreateMsgToolMem(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gMsgToolCmdIn = (MsgToolDrvCmd_t*)kmalloc(sizeof(MsgToolDrvCmd_t), GFP_KERNEL);
    _gSndCmdData = (u8*)kmalloc(1024, GFP_KERNEL);	
    _gRtnCmdData = (u8*)kmalloc(1024, GFP_KERNEL);           
}

static void _DrvJniDeleteMsgToolMem(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
 
    if (_gMsgToolCmdIn)
    {
        kfree(_gMsgToolCmdIn);
        _gMsgToolCmdIn = NULL;
    }
    
    if (_gSndCmdData)
    {
        kfree(_gSndCmdData);
        _gSndCmdData = NULL;
    }
    
    if (_gRtnCmdData)
    {
        kfree(_gRtnCmdData);
        _gRtnCmdData = NULL;
    }
}
#endif //CONFIG_ENABLE_JNI_INTERFACE

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
void DrvTouchDeviceRegulatorPowerOn(bool nFlag)
{
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    s32 nRetVal = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    if (g_ReguVdd != NULL && g_ReguVcc_i2c != NULL)
    {
        if (nFlag == true)
        {
            nRetVal = regulator_enable(g_ReguVdd);
            if (nRetVal)
            {
                DBG(&g_I2cClient->dev, "regulator_enable(g_ReguVdd) failed. nRetVal=%d\n", nRetVal); 
            }
            mdelay(20);
    
            nRetVal = regulator_enable(g_ReguVcc_i2c);
            if (nRetVal)
            {
                DBG(&g_I2cClient->dev, "regulator_enable(g_ReguVcc_i2c) failed. nRetVal=%d\n", nRetVal); 
            }
            mdelay(20);
        }
        else
        {
            nRetVal = regulator_disable(g_ReguVdd);
            if (nRetVal)
            {
                DBG(&g_I2cClient->dev, "regulator_disable(g_ReguVdd) failed. nRetVal=%d\n", nRetVal); 
            }
            mdelay(20);
    
            nRetVal = regulator_disable(g_ReguVcc_i2c);
            if (nRetVal)
            {
                DBG(&g_I2cClient->dev, "regulator_disable(g_ReguVcc_i2c) failed. nRetVal=%d\n", nRetVal); 
            }
            mdelay(20);
        }
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    s32 nRetVal = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ReguVdd != NULL)
    {
        if (nFlag == true)
        {
            nRetVal = regulator_enable(g_ReguVdd);
            if (nRetVal)
            {
                DBG(&g_I2cClient->dev, "regulator_enable(g_ReguVdd) failed. nRetVal=%d\n", nRetVal);
            }
            mdelay(20);
        }
        else
        {
            nRetVal = regulator_disable(g_ReguVdd);
            if (nRetVal)
            {
                DBG(&g_I2cClient->dev, "regulator_disable(g_ReguVdd) failed. nRetVal=%d\n", nRetVal);
            }
            mdelay(20);
        }
    }
#else
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

//    hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP"); // For specific MTK BB chip(ex. MT6582), need to enable this function call for correctly power on Touch IC.
    hwPowerOn(PMIC_APP_CAP_TOUCH_VDD, VOL_2800, "TP"); // For specific MTK BB chip(ex. MT6735), need to enable this function call for correctly power on Touch IC.
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#endif
}
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

void DrvTouchDevicePowerOn(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef ALLWINNER_PLATFORM

	input_set_power_enable(&(config_info.input_type), 1);

	gpio_direction_output(config_info.wakeup_gpio.gpio, 1);
	mdelay(10); 
	__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
	mdelay(10);
	__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
	mdelay(25);

	
#else
    if (MS_TS_MSG_IC_GPIO_RST > 0)
    {
        gpio_direction_output(MS_TS_MSG_IC_GPIO_RST, 1);
//        gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 1); 
        mdelay(10); 
        gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 0);
        mdelay(10);
        gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 1);
        mdelay(25); 
    }
#endif //ALLWINNER_PLATFORM
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (MS_TS_MSG_IC_GPIO_RST >= 0 && MS_TS_MSG_IC_GPIO_RST != 1) // MS_TS_MSG_IC_GPIO_RST must be a value other than 1
    {
        tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 1);
        mdelay(10); 
        tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 0);
        mdelay(10);
        tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 1);
        mdelay(25); 
    }
#else
    if (MS_TS_MSG_IC_GPIO_RST > 0)
    {
        mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
        mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ONE);  
        mdelay(10); 

        mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
        mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ZERO);  
        mdelay(10);

#ifdef TPD_CLOSE_POWER_IN_SLEEP
        hwPowerDown(TPD_POWER_SOURCE, "TP"); 
        mdelay(10);
        hwPowerOn(TPD_POWER_SOURCE, VOL_2800, "TP"); 
        mdelay(10);  // reset pulse
#endif //TPD_CLOSE_POWER_IN_SLEEP

        mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
        mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ONE);
        mdelay(25); 
    }
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD    
#endif

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
    g_IsEnableChargerPlugInOutCheck = 1;
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 1;
    g_IsHwResetByDriver = 1; // Indicate HwReset is triggered by Device Driver instead of Firmware or Touch IC
#endif //CONFIG_ENABLE_ESD_PROTECTION
}

void DrvTouchDevicePowerOff(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    
    DrvOptimizeCurrentConsumption();

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef ALLWINNER_PLATFORM
	input_set_power_enable(&(config_info.input_type), 0);
	__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
#else
    if (MS_TS_MSG_IC_GPIO_RST > 0)
    {
//        gpio_direction_output(MS_TS_MSG_IC_GPIO_RST, 0);
        gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 0);
    }
#endif //ALLWINNER_PLATFORM
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (MS_TS_MSG_IC_GPIO_RST >= 0 && MS_TS_MSG_IC_GPIO_RST != 1) // MS_TS_MSG_IC_GPIO_RST must be a value other than 1
    {
        tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 0);
    }
#else
    if (MS_TS_MSG_IC_GPIO_RST > 0)
    {
        mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
        mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ZERO);  

#ifdef TPD_CLOSE_POWER_IN_SLEEP
        hwPowerDown(TPD_POWER_SOURCE, "TP");
#endif //TPD_CLOSE_POWER_IN_SLEEP
    }
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif    

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
    g_IsEnableChargerPlugInOutCheck = 0;
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 0;
#endif //CONFIG_ENABLE_ESD_PROTECTION
}

void DrvTouchDeviceHwReset(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef ALLWINNER_PLATFORM

	gpio_direction_output(config_info.wakeup_gpio.gpio, 1);
	mdelay(10); 
	__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
	mdelay(10);
	__gpio_set_value(config_info.wakeup_gpio.gpio, 1);

	mdelay(25);
#else
    if (MS_TS_MSG_IC_GPIO_RST > 0)
    {
        gpio_direction_output(MS_TS_MSG_IC_GPIO_RST, 1);
//        gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 1); 
        mdelay(10); 
        gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 0);
        mdelay(10);
        gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 1);
        mdelay(25); 
    }
#endif //ALLWINNER_PLATFORM
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (MS_TS_MSG_IC_GPIO_RST >= 0 && MS_TS_MSG_IC_GPIO_RST != 1) // MS_TS_MSG_IC_GPIO_RST must be a value other than 1
    {
        tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 1);
        mdelay(10); 
        tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 0);
        mdelay(10);
        tpd_gpio_output(MS_TS_MSG_IC_GPIO_RST, 1);
        mdelay(25); 
    }   
#else
    if (MS_TS_MSG_IC_GPIO_RST > 0)
    {
        mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
        mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ONE);
        mdelay(10); 
        mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
        mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ZERO);  
        mdelay(10);
        mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
        mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ONE);
        mdelay(25); 
    }
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
    g_IsEnableChargerPlugInOutCheck = 1;
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 1;
    g_IsHwResetByDriver = 1; // Indicate HwReset is triggered by Device Driver instead of Firmware or Touch IC
#endif //CONFIG_ENABLE_ESD_PROTECTION
}

void DrvDisableFingerTouchReport(void)
{
    unsigned long nIrqFlag;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag); 

    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)

    {
        if (_gInterruptFlag == 1)  
        {
            disable_irq_nosync(_gIrq);

            _gInterruptFlag = 0;
        }
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)         
    {
        if (_gInterruptFlag == 1) 
        {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
            disable_irq_nosync(_gIrq);
#else
            mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD            

            _gInterruptFlag = 0;
        }
    }
#endif

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
}

void DrvEnableFingerTouchReport(void)
{
    unsigned long nIrqFlag;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag); 

    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)

    if (_gInterruptFlag == 0) 
    {
        enable_irq(_gIrq);

        _gInterruptFlag = 1;        
    }

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

    if (_gInterruptFlag == 0) 
    {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
        enable_irq(_gIrq);
#else
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

        _gInterruptFlag = 1;        
    }

#endif

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
}

void DrvFingerTouchPressed(s32 nX, s32 nY, s32 nPressure, s32 nId)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    DBG(&g_I2cClient->dev, "point touch pressed\n"); 
 //   if (nId == 0)
 //   {
 //   	if (u32TouchCount < 360)
 //   	{
 //       	u32TouchCount++;
 //       	nPressure = 1;
 //      	}
 //   }
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
    input_mt_slot(g_InputDevice, nId);
    input_mt_report_slot_state(g_InputDevice, MT_TOOL_FINGER, true);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_X, nX);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_Y, nY);
    input_report_abs(g_InputDevice, ABS_MT_PRESSURE, nPressure);

    DBG(&g_I2cClient->dev, "nId=%d, nX=%d, nY=%d\n", nId, nX, nY); // TODO : add for debug
#else // TYPE A PROTOCOL
    input_report_key(g_InputDevice, BTN_TOUCH, 1);
    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        input_report_abs(g_InputDevice, ABS_MT_TRACKING_ID, nId); // ABS_MT_TRACKING_ID is used for MSG28xx/MSG58xxA/ILI21xx only
    }
    input_report_abs(g_InputDevice, ABS_MT_TOUCH_MAJOR, 1);
    input_report_abs(g_InputDevice, ABS_MT_WIDTH_MAJOR, 1);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_X, nX);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_Y, nY);
    input_report_abs(g_InputDevice, ABS_MT_PRESSURE, nPressure);

    input_mt_sync(g_InputDevice);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL


#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_MTK_BOOT
    if (tpd_dts_data.use_tpd_button)
    {
        if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
        {   
            tpd_button(nX, nY, 1);  
            DBG(&g_I2cClient->dev, "tpd_button(nX, nY, 1) = tpd_button(%d, %d, 1)\n", nX, nY); // TODO : add for debug
        }
    }
#endif //CONFIG_MTK_BOOT
#else
#ifdef CONFIG_TP_HAVE_KEY    
    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
    {   
        tpd_button(nX, nY, 1);  
        DBG(&g_I2cClient->dev, "tpd_button(nX, nY, 1) = tpd_button(%d, %d, 1)\n", nX, nY); // TODO : add for debug
    }
#endif //CONFIG_TP_HAVE_KEY
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

    TPD_EM_PRINT(nX, nY, nX, nY, nId, 1);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
}

void DrvFingerTouchReleased(s32 nX, s32 nY, s32 nId)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    DBG(&g_I2cClient->dev, "point touch released\n"); 

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
 //   if (nId == 0)
 //       u32TouchCount = 0;
    input_mt_slot(g_InputDevice, nId);
    input_mt_report_slot_state(g_InputDevice, MT_TOOL_FINGER, false);

    DBG(&g_I2cClient->dev, "nId=%d\n", nId); // TODO : add for debug
#else // TYPE A PROTOCOL
    input_report_key(g_InputDevice, BTN_TOUCH, 0);
    input_mt_sync(g_InputDevice);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL


#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_MTK_BOOT
    if (tpd_dts_data.use_tpd_button)
    {
        if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
        {   
            tpd_button(nX, nY, 0); 
            DBG(&g_I2cClient->dev, "tpd_button(nX, nY, 0) = tpd_button(%d, %d, 0)\n", nX, nY); // TODO : add for debug
        }            
    }
#endif //CONFIG_MTK_BOOT  
#else
#ifdef CONFIG_TP_HAVE_KEY 
    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
    {   
        tpd_button(nX, nY, 0); 
        DBG(&g_I2cClient->dev, "tpd_button(nX, nY, 0) = tpd_button(%d, %d, 0)\n", nX, nY); // TODO : add for debug
    }            
#endif //CONFIG_TP_HAVE_KEY    
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

    TPD_EM_PRINT(nX, nY, nX, nY, 0, 0);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
}

void DrvSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate)
{
    DBG(&g_I2cClient->dev, "*** %s() nIicDataRate = %d ***\n", __func__, nIicDataRate); 

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    // TODO : Please FAE colleague to confirm with customer device driver engineer for how to set i2c data rate on SPRD platform
    sprd_i2c_ctl_chg_clk(pClient->adapter->nr, nIicDataRate); 
    mdelay(100);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    // TODO : Please FAE colleague to confirm with customer device driver engineer for how to set i2c data rate on QCOM platform
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

#ifndef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    pClient->timing = nIicDataRate/1000;
#endif

#endif
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
s32 DrvEnableProximity(void)
{
    u8 szTxData[4] = {0};
    u32 i = 0;
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szTxData[0] = 0x52;
    szTxData[1] = 0x00;
    
    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        szTxData[2] = 0x4a; 
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        szTxData[2] = 0x47; 
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** Un-recognized chip type = 0x%x ***\n", g_ChipType);
        return -1;
    }
    
    szTxData[3] = 0xa0;

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
        if (rc > 0)
        {
            g_EnableTpProximity = 1;

            DBG(&g_I2cClient->dev, "Enable proximity detection success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Enable proximity detection failed, rc = %d\n", rc);
    }
    	
    return rc;
}

s32 DrvDisableProximity(void)
{
    u8 szTxData[4] = {0};
    u32 i = 0;
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szTxData[0] = 0x52;
    szTxData[1] = 0x00;

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        szTxData[2] = 0x4a; 
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        szTxData[2] = 0x47; 
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** Un-recognized chip type = 0x%x ***\n", g_ChipType);
        return -1;
    }

    szTxData[3] = 0xa1;
    
    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
        if (rc > 0)
        {
            g_EnableTpProximity = 0;

            DBG(&g_I2cClient->dev, "Disable proximity detection success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Disable proximity detection failed, rc = %d\n", rc);
    }

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    g_FaceClosingTp = 0;
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM

    return rc;
}

int DrvGetTpPsData(void)
{
    DBG(&g_I2cClient->dev, "*** %s() g_FaceClosingTp = %d ***\n", __func__, g_FaceClosingTp); 
	
    return g_FaceClosingTp;
}

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
void DrvTpPsEnable(int nEnable)
{
    DBG(&g_I2cClient->dev, "*** %s() nEnable = %d ***\n", __func__, nEnable); 

    if (nEnable)
    {
        DrvEnableProximity();
    }
    else
    {
        DrvDisableProximity();
    }
}

static int _DrvProximityOpen(struct inode *inode, struct file *file)
{
    int nRetVal = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    nRetVal = nonseekable_open(inode, file);
    if (nRetVal < 0)
    {
        return nRetVal;
    }

    file->private_data = i2c_get_clientdata(g_I2cClient);
    
    return 0;
}

static int _DrvProximityRelease(struct inode *inode, struct file *file)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    return 0;
}

static atomic_t _gPsFlag;

static long _DrvProximityIoctl(struct file *file, unsigned int cmd, unsigned long arg)
{
#if 0
    DBG(&g_I2cClient->dev, "*** %s() *** cmd = %d\n", __func__, _IOC_NR(cmd)); 

    switch (cmd)
    {
        case GTP_IOCTL_PROX_ON:
            DrvTpPsEnable(1);
            break;
        case GTP_IOCTL_PROX_OFF:
            DrvTpPsEnable(0);
            break;
        default:
            return -EINVAL;
    }
#else
    void __user *argp = (void __user *)arg;
    int flag;
    unsigned char data;
    
    DBG(&g_I2cClient->dev, "*** %s() *** cmd = %d\n", __func__, _IOC_NR(cmd)); 
    
    switch (cmd)
    {
        case LTR_IOCTL_SET_PFLAG:
            if (copy_from_user(&flag, argp, sizeof(flag)))
            {
                return -EFAULT;
            }
		
            if (flag < 0 || flag > 1)
            {
                return -EINVAL;
            }
            DBG(&g_I2cClient->dev, "flag = %d", flag); 
                
            atomic_set(&_gPsFlag, flag);	
            
            if (flag == 1)
            {
                DrvTpPsEnable(1);
            }
            else if (flag == 0)
            {
                DrvTpPsEnable(0);
            }		
            break;
		
        case LTR_IOCTL_GET_PFLAG:
            flag = atomic_read(&_gPsFlag);
            
            if (copy_to_user(argp, &flag, sizeof(flag))) 
            {
                return -EFAULT;
            }
            DBG(&g_I2cClient->dev, "flag = %d", flag); 
            break;

        case LTR_IOCTL_GET_DATA:
            if (copy_to_user(argp, &data, sizeof(data)))
            {
                return -EFAULT;
            }
            DBG(&g_I2cClient->dev, "flag = %d", flag); 
            break;

        case GTP_IOCTL_PROX_ON:
            DrvTpPsEnable(1);
            break;
        
        case GTP_IOCTL_PROX_OFF:
            DrvTpPsEnable(0);
            break;

        default:
            DBG(&g_I2cClient->dev, "*** %s() *** Invalid cmd = %d\n", __func__, _IOC_NR(cmd)); 
            return -EINVAL;
        } 
#endif

    return 0;
}

static const struct file_operations ProximityFileOps = {
    .owner = THIS_MODULE,
    .open = _DrvProximityOpen,
    .release = NULL, //_DrvProximityRelease,
    .unlocked_ioctl = _DrvProximityIoctl,
};

static struct miscdevice ProximityMisc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "ltr_558als", // Match the hal's name
    .fops = &ProximityFileOps,
};

static int _DrvProximityInputDeviceInit(struct i2c_client *pClient)
{
    int nRetVal = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    nRetVal = misc_register(&ProximityMisc);
    if (nRetVal)
    {
        DBG(&g_I2cClient->dev, "*** Failed to misc_register() for proximity *** nRetVal=%d\n", nRetVal); 
        goto ERROR_MISC_REGISTER_FAILED;
    }

    g_ProximityInputDevice = input_allocate_device();
    if (g_ProximityInputDevice == NULL)
    {
        DBG(&g_I2cClient->dev, "*** Failed to allocate proximity input device ***\n"); 
        nRetVal = -ENOMEM;
        goto ERROR_INPUT_DEVICE_ALLOCATE_FAILED;
    }

    g_ProximityInputDevice->name = "alps_pxy";
    g_ProximityInputDevice->phys  = "alps_pxy";
    g_ProximityInputDevice->id.bustype = BUS_I2C;
    g_ProximityInputDevice->dev.parent = &pClient->dev;
    g_ProximityInputDevice->id.vendor = 0x0001;
    g_ProximityInputDevice->id.product = 0x0001;
    g_ProximityInputDevice->id.version = 0x0010;

    set_bit(EV_ABS, g_ProximityInputDevice->evbit);
	
    input_set_abs_params(g_ProximityInputDevice, ABS_DISTANCE, 0, 1, 0, 0);

    nRetVal = input_register_device(g_ProximityInputDevice);
    if (nRetVal < 0) {
        DBG(&g_I2cClient->dev, "*** Unable to register proximity input device *** nRetVal=%d\n", nRetVal); 
        goto ERROR_INPUT_DEVICE_REGISTER_FAILED;
    }
    
    return 0;

ERROR_INPUT_DEVICE_REGISTER_FAILED:
    if (g_ProximityInputDevice)
    {
        input_free_device(g_ProximityInputDevice);
        g_ProximityInputDevice = NULL;
    }
ERROR_INPUT_DEVICE_ALLOCATE_FAILED:
    misc_deregister(&ProximityMisc);
ERROR_MISC_REGISTER_FAILED:

    return nRetVal;
}

static int _DrvProximityInputDeviceUnInit(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    misc_deregister(&ProximityMisc);

    if (g_ProximityInputDevice)
    {
        input_unregister_device(g_ProximityInputDevice);
        g_ProximityInputDevice = NULL;
    }
    
    return 0;
}
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
static struct sensors_classdev _gProximityCdev;

static struct sensors_classdev sensors_proximity_cdev = {
    .name = "msg2xxx-proximity",
    .vendor = "MStar",
    .version = 1,
    .handle = SENSORS_PROXIMITY_HANDLE,
    .type = SENSOR_TYPE_PROXIMITY,
    .max_range = "5.0",
    .resolution = "5.0",
    .sensor_power = "0.1",
    .min_delay = 0,
    .fifo_reserved_event_count = 0,
    .fifo_max_event_count = 0,
    .enabled = 0,
    .delay_msec = 200,
    .sensors_enable = NULL,
    .sensors_poll_delay = NULL,
};

int DrvTpPsEnable(struct sensors_classdev* pProximityCdev, unsigned int nEnable)
{
    DBG(&g_I2cClient->dev, "*** %s() nEnable = %d ***\n", __func__, nEnable); 

    if (nEnable)
    {
        DrvEnableProximity();
    }
    else
    {
        DrvDisableProximity();
    }
    
    return 0;
}

static ssize_t _DrvProximityDetectionShow(struct device *dev, struct device_attribute *attr, char *buf)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    DBG(&g_I2cClient->dev, "*** Tp Proximity State = %s ***\n", g_EnableTpProximity ? "open" : "close"); 
    
    return sprintf(buf, "%s\n", g_EnableTpProximity ? "open" : "close");
}

static ssize_t _DrvProximityDetectionStore(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    if (buf != NULL)
    {
        if (sysfs_streq(buf, "1"))
        {
            DrvTpPsEnable(&_gProximityCdev, 1);
        }
        else if (sysfs_streq(buf, "0"))
        {
            DrvTpPsEnable(&_gProximityCdev, 0);
        }
    }

    return size;
}

static struct device_attribute proximity_attribute = __ATTR(proximity, 0666/*0664*/, _DrvProximityDetectionShow, _DrvProximityDetectionStore);

static struct attribute *proximity_detection_attrs[] =
{
    &proximity_attribute.attr,
    NULL
};

static struct attribute_group proximity_detection_attribute_group = {
    .name = "Driver",
    .attrs = proximity_detection_attrs,
};

static int _DrvProximityInputDeviceInit(struct i2c_client *pClient)
{
    int nRetVal = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    g_ProximityInputDevice = input_allocate_device();
    if (g_ProximityInputDevice == NULL)
    {
        DBG(&g_I2cClient->dev, "*** Failed to allocate proximity input device ***\n"); 
        nRetVal = -ENOMEM;
        goto ERROR_INPUT_DEVICE_ALLOCATE_FAILED;
    }

    g_ProximityInputDevice->name = "msg2xxx-ps";
    g_ProximityInputDevice->phys = "I2C";
    g_ProximityInputDevice->dev.parent = &pClient->dev;
    g_ProximityInputDevice->id.bustype = BUS_I2C;

    set_bit(EV_ABS, g_ProximityInputDevice->evbit);

    input_set_abs_params(g_ProximityInputDevice, ABS_DISTANCE, 0, 1, 0, 0);
    
    nRetVal = input_register_device(g_ProximityInputDevice);
    if (nRetVal < 0) {
        DBG(&g_I2cClient->dev, "*** Unable to register proximity input device *** nRetVal=%d\n", nRetVal); 
        goto ERROR_INPUT_DEVICE_REGISTER_FAILED;
    }

    mdelay(10);

    nRetVal = sysfs_create_group(&g_ProximityInputDevice->dev.kobj, &proximity_detection_attribute_group);
    if (nRetVal < 0)
    {
        DBG(&g_I2cClient->dev, "*** Failed to sysfs_create_group() for proximity *** nRetVal=%d\n", nRetVal); 
        goto ERROR_SYSFS_CREATE_GROUP_FAILED;
    }

    input_set_drvdata(g_ProximityInputDevice, NULL);

    sensors_proximity_cdev.sensors_enable = DrvTpPsEnable;
    nRetVal = sensors_classdev_register(&pClient->dev, &sensors_proximity_cdev);
    if (nRetVal < 0) {
        DBG(&g_I2cClient->dev, "*** Failed to sensors_classdev_register() for proximity *** nRetVal=%d\n", nRetVal); 
        goto ERROR_SENSORS_CLASSDEV_REGISTER_FAILED;
    }

    return 0;

ERROR_SENSORS_CLASSDEV_REGISTER_FAILED:
ERROR_SYSFS_CREATE_GROUP_FAILED:
    if (g_ProximityInputDevice)
    {
        input_unregister_device(g_ProximityInputDevice);
        g_ProximityInputDevice = NULL;
    }
ERROR_INPUT_DEVICE_REGISTER_FAILED:
    if (g_ProximityInputDevice)
    {
        input_free_device(g_ProximityInputDevice);
        g_ProximityInputDevice = NULL;
    }
ERROR_INPUT_DEVICE_ALLOCATE_FAILED:

    return nRetVal;
}

static int _DrvProximityInputDeviceUnInit(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    if (g_ProximityInputDevice)
    {
        input_unregister_device(g_ProximityInputDevice);
        g_ProximityInputDevice = NULL;
    }

    sensors_classdev_unregister(&sensors_proximity_cdev);
    
    return 0;
}
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
int DrvTpPsOperate(void* pSelf, u32 nCommand, void* pBuffIn, int nSizeIn,
				   void* pBuffOut, int nSizeOut, int* pActualOut)
{
    int nErr = 0;
    int nValue;
    hwm_sensor_data *pSensorData;

    switch (nCommand)
    {
        case SENSOR_DELAY:
            if ((pBuffIn == NULL) || (nSizeIn < sizeof(int)))
            {
                DBG(&g_I2cClient->dev, "Set delay parameter error!\n"); 
                nErr = -EINVAL;
            }
            // Do nothing
            break;

        case SENSOR_ENABLE:
            if ((pBuffIn == NULL) || (nSizeIn < sizeof(int)))
            {
                DBG(&g_I2cClient->dev, "Enable sensor parameter error!\n"); 
                nErr = -EINVAL;
            }
            else
            {
                nValue = *(int *)pBuffIn;
                if (nValue)
                {
                    if (DrvEnableProximity() < 0)
                    {
                        DBG(&g_I2cClient->dev, "Enable ps fail: %d\n", nErr); 
                        return -1;
                    }
                }
                else
                {
                    if (DrvDisableProximity() < 0)
                    {
                        DBG(&g_I2cClient->dev, "Disable ps fail: %d\n", nErr); 
                        return -1;
                    }
                }
            }
            break;

        case SENSOR_GET_DATA:
            if ((pBuffOut == NULL) || (nSizeOut < sizeof(hwm_sensor_data)))
            {
                DBG(&g_I2cClient->dev, "Get sensor data parameter error!\n"); 
                nErr = -EINVAL;
            }
            else
            {
                pSensorData = (hwm_sensor_data *)pBuffOut;

                pSensorData->values[0] = DrvGetTpPsData();
                pSensorData->value_divide = 1;
                pSensorData->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }
            break;

       default:
           DBG(&g_I2cClient->dev, "Un-recognized parameter %d!\n", nCommand); 
           nErr = -1;
           break;
    }

    return nErr;
}
#endif
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION
void DrvSetFilmMode(u8 nFilmtype){
    u8 szTxData[2] = {0x13, nFilmtype};
    s32 rc;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

	mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
	rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 2);
	if (rc > 0)
	{
		DBG(&g_I2cClient->dev, "Set Film Mode success,\n");
	}
}

int DrvGetFilmMode(void){
    u8 szTxData[1] = {0x12};
	u8 szRxData[3] = {0};
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
	rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
	if (rc > 0)
	{
		DBG(&g_I2cClient->dev, "Get firmware info IicWriteData() success\n");
	}
	mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
	rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 3);
	if (rc > 0)
	{
		if(szRxData[1] == FEATURE_FILM_MODE_LOW_STRENGTH)
		{
			DBG(&g_I2cClient->dev, "Film mode: Low strength\n");
			return FEATURE_FILM_MODE_LOW_STRENGTH;
		}
		else if(szRxData[1] == FEATURE_FILM_MODE_HIGH_STRENGTH)
		{
			DBG(&g_I2cClient->dev, "Film mode: High strength\n");
			return	FEATURE_FILM_MODE_HIGH_STRENGTH;
		}
		else if(szRxData[1] == FEATURE_FILM_MODE_DEFAULT)
		{
			DBG(&g_I2cClient->dev, "Film mode: Default\n");	
			return FEATURE_FILM_MODE_DEFAULT;
		}
	}
	return -1;
}
//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_ESD_PROTECTION
void DrvEsdCheck(struct work_struct *pWork)
{
#ifdef CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE
    static u8 szEsdCheckValue[8];
    u8 szTxData[3] = {0};
    u8 szRxData[8] = {0};
    u32 i = 0;
    s32 retW = -1;
    s32 retR = -1;
#else
    u8 szData[1] = {0x00};
    u32 i = 0;
    s32 rc = 0;
#endif //CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE

    DBG(&g_I2cClient->dev, "*** %s() g_IsEnableEsdCheck = %d ***\n", __func__, g_IsEnableEsdCheck); 

    if (g_IsEnableEsdCheck == 0)
    {
        return;
    }

    if (_gInterruptFlag == 0) // Skip ESD check while finger touch
    {
        DBG(&g_I2cClient->dev, "Not allow to do ESD check while finger touch.\n");
        goto EsdCheckEnd;
    }
    	
    if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
    {
        DBG(&g_I2cClient->dev, "Not allow to do ESD check while update firmware is proceeding.\n");
        goto EsdCheckEnd;
    }

#ifdef CONFIG_ENABLE_ITO_MP_TEST
    if (g_IsInMpTest == 1) // Check whether mp test is proceeding
    {
        DBG(&g_I2cClient->dev, "Not allow to do ESD check while mp test is proceeding.\n");
        goto EsdCheckEnd;
    }
#endif //CONFIG_ENABLE_ITO_MP_TEST

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {
        if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE) // Skip ESD check while firmware mode is DEBUG MODE
        {
            DBG(&g_I2cClient->dev, "Not allow to do ESD check while firmware mode is DEBUG MODE.\n");
            goto EsdCheckEnd;
        }
    }

    if (IS_DISABLE_ESD_PROTECTION_CHECK) // Skip ESD check while mp test is triggered by MTPTool APK through JNI interface
    {
        DBG(&g_I2cClient->dev, "Not allow to do ESD check while mp test is triggered by MTPTool APK through JNI interface.\n");
        goto EsdCheckEnd;
    }

#ifdef CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE /* Method 1. Require the new ESD check command(CmdId:0x55) support from firmware which is currently implemented for MSG22XX only. So default is not supported. */
    szTxData[0] = 0x55;
    szTxData[1] = 0xAA;
    szTxData[2] = 0x55;

    mutex_lock(&g_Mutex);

    retW = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
    retR = IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 8);

    mutex_unlock(&g_Mutex);

    DBG(&g_I2cClient->dev, "szRxData[] : 0x%x  0x%x  0x%x  0x%x  0x%x  0x%x  0x%x  0x%x\n", \
            szRxData[0], szRxData[1], szRxData[2], szRxData[3],szRxData[4], szRxData[5], szRxData[6], szRxData[7]);

    DBG(&g_I2cClient->dev, "retW = %d, retR = %d\n", retW, retR);

    if (retW > 0 && retR > 0)
    {
        while (i < 8)
        {
            if (szEsdCheckValue[i] != szRxData[i])
            {
                break;
            }
            i ++;
        }
        
        if (i == 8)
        {
            if (szRxData[0] == 0 && szRxData[1] == 0 && szRxData[2] == 0 && szRxData[3] == 0 && szRxData[4] == 0 && szRxData[5] == 0 && szRxData[6] == 0 && szRxData[7] == 0)
            {
                DBG(&g_I2cClient->dev, "Firmware not support ESD check command.\n");
            }
            else
            {
                DBG(&g_I2cClient->dev, "ESD check failed case1.\n");
                
                DrvTouchDeviceHwReset();
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "ESD check success.\n");
        } 

        for (i = 0; i < 8; i ++)
        {
            szEsdCheckValue[i] = szRxData[i];
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "ESD check failed case2.\n");

        DrvTouchDeviceHwReset();
    }
#else /* Method 2. Use I2C write command for checking whether I2C connection is still available under ESD testing. */

    szData[0] = 0x00; // Dummy command for ESD check

    mutex_lock(&g_Mutex);
    
    while (i < 3)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
        {
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szData[0], 1);
        }
        else
        {
            DBG(&g_I2cClient->dev, "Un-recognized chip type = 0x%x\n", g_ChipType);
            break;
        }
        	
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "ESD check success\n");
            break;
        }
     
        i++;
    }
    if (i == 3)
    {
        DBG(&g_I2cClient->dev, "ESD check failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    if (i >= 3)
    {
        DrvTouchDeviceHwReset();
    }
#endif //CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE

EsdCheckEnd :

    if (g_IsEnableEsdCheck == 1)
    {
        queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
    }
}
#endif //CONFIG_ENABLE_ESD_PROTECTION

//------------------------------------------------------------------------------//

static void _DrvVariableInitialize(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
//        FIRMWARE_MODE_UNKNOWN_MODE = MSG22XX_FIRMWARE_MODE_UNKNOWN_MODE;
        FIRMWARE_MODE_DEMO_MODE = MSG22XX_FIRMWARE_MODE_DEMO_MODE;
        FIRMWARE_MODE_DEBUG_MODE = MSG22XX_FIRMWARE_MODE_DEBUG_MODE;
        FIRMWARE_MODE_RAW_DATA_MODE = MSG22XX_FIRMWARE_MODE_RAW_DATA_MODE;
        _DrvSelfGetFirmwareInfo(&g_SelfFirmwareInfo);
        g_FirmwareMode = FIRMWARE_MODE_DEMO_MODE;

        DEMO_MODE_PACKET_LENGTH = SELF_DEMO_MODE_PACKET_LENGTH;
        DEBUG_MODE_PACKET_LENGTH = SELF_DEBUG_MODE_PACKET_LENGTH;
        MAX_TOUCH_NUM = SELF_MAX_TOUCH_NUM;
    }	
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        FIRMWARE_MODE_UNKNOWN_MODE = MSG28XX_FIRMWARE_MODE_UNKNOWN_MODE;
        FIRMWARE_MODE_DEMO_MODE = MSG28XX_FIRMWARE_MODE_DEMO_MODE;
        FIRMWARE_MODE_DEBUG_MODE = MSG28XX_FIRMWARE_MODE_DEBUG_MODE;

        g_FirmwareMode = FIRMWARE_MODE_DEMO_MODE;

        DEMO_MODE_PACKET_LENGTH = MUTUAL_DEMO_MODE_PACKET_LENGTH;
        DEBUG_MODE_PACKET_LENGTH = MUTUAL_DEBUG_MODE_PACKET_LENGTH;
        MAX_TOUCH_NUM = MUTUAL_MAX_TOUCH_NUM;
    }	
}	

void DrvGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        u16 nRegData1, nRegData2;

        mutex_lock(&g_Mutex);

        //DrvTouchDeviceHwReset();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 
    
        // Exit flash low power mode
        RegSetLByteValue(0x1619, BIT1); 

        // Change PIU clock to 48MHz
        RegSetLByteValue(0x1E23, BIT6); 

        // Change mcu clock deglitch mux source
        RegSetLByteValue(0x1E54, BIT0); 
#else
        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        RegSet16BitValue(0x1600, 0xBFF4); // Set start address for customer firmware version on main block
    
        // Enable burst mode
//        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        RegSetLByteValue(0x160E, 0x01); 

        nRegData1 = RegGet16BitValue(0x1604);
        nRegData2 = RegGet16BitValue(0x1606);

        *pMajor = (((nRegData1 >> 8) & 0xFF) << 8) + (nRegData1 & 0xFF);
        *pMinor = (((nRegData2 >> 8) & 0xFF) << 8) + (nRegData2 & 0xFF);

        // Clear burst mode
//        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvTouchDeviceHwReset();

        mutex_unlock(&g_Mutex);
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)   
    {
        u8 szTxData[3] = {0};
        u8 szRxData[4] = {0};

        szTxData[0] = 0x03;

        mutex_lock(&g_Mutex);
    
        //DrvTouchDeviceHwReset();

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
        mdelay(I2C_SMBUS_READ_COMMAND_DELAY_FOR_PLATFORM);
        IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 4);

        mutex_unlock(&g_Mutex);

        DBG(&g_I2cClient->dev, "szRxData[0] = 0x%x\n", szRxData[0]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[1] = 0x%x\n", szRxData[1]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[2] = 0x%x\n", szRxData[2]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[3] = 0x%x\n", szRxData[3]); // add for debug

        *pMajor = (szRxData[1]<<8) + szRxData[0];
        *pMinor = (szRxData[3]<<8) + szRxData[2];
    }
    if (*ppVersion == NULL)
    {
        *ppVersion = kzalloc(sizeof(u8)*11, GFP_KERNEL);
    }
    DBG(&g_I2cClient->dev, "*** Major = %d ***\n", *pMajor);
	if(g_FwVersionFlag)
	{
		DBG(&g_I2cClient->dev, "*** Minor = %d.%d ***\n", (*pMinor & 0xFF), ((*pMinor >> 8) & 0xFF));
	}
	else
	{	
		DBG(&g_I2cClient->dev, "*** Minor = %d ***\n", *pMinor);
	}  
	sprintf(*ppVersion, "%05d.%05d", *pMajor, *pMinor);
}
void DrvCheckFWSupportDriver(void)
{
    u8 szTxData[1] = {0};
    u8 szRxData[4] = {0};
    u32 szDriverVer[4] = {0};
    szTxData[0] = 0x14;
    IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
    mdelay(I2C_SMBUS_READ_COMMAND_DELAY_FOR_PLATFORM);
    IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 4); 
    sscanf(DEVICE_DRIVER_RELEASE_VERSION,"%x.%x.%x.%x", &szDriverVer[0], &szDriverVer[1], &szDriverVer[2], &szDriverVer[3]);
    printk("Read FW support Driver Version: %d.%d.%d.%d\n", szRxData[0], szRxData[1], szRxData[2], szRxData[3]);
    if(((szDriverVer[0] + szDriverVer[1] +szDriverVer[2]  + szDriverVer[3]) & 0xff) >
    (szRxData[0] << 24) + (szRxData[1] << 16) + (szRxData[2] << 8) + szRxData[3]){
        if((szRxData[0] << 24) + (szRxData[1] << 16) + (szRxData[2] << 8) + szRxData[3] > 0)
        printk("Please upgreade your driver. The FW support driver %d.%d.%d.%d\n", szRxData[0], szRxData[1], szRxData[2], szRxData[3]);
    }
}
void DrvGetPlatformFirmwareVersion(u8 **ppVersion)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX) 
    {
        u32 i;
        u16 nRegData1, nRegData2;
        u8 szRxData[12] = {0};

        mutex_lock(&g_Mutex);

        DrvTouchDeviceHwReset();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 
    
        // Exit flash low power mode
        RegSetLByteValue(0x1619, BIT1); 

        // Change PIU clock to 48MHz
        RegSetLByteValue(0x1E23, BIT6); 

        // Change mcu clock deglitch mux source
        RegSetLByteValue(0x1E54, BIT0); 
#else
        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        RegSet16BitValue(0x1600, 0xC1F2); // Set start address for platform firmware version on info block(Actually, start reading from 0xC1F0)
    
        // Enable burst mode
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        for (i = 0; i < 3; i ++)
        {
            RegSetLByteValue(0x160E, 0x01); 

            nRegData1 = RegGet16BitValue(0x1604);
            nRegData2 = RegGet16BitValue(0x1606);

            szRxData[i*4+0] = (nRegData1 & 0xFF);
            szRxData[i*4+1] = ((nRegData1 >> 8 ) & 0xFF);
            
//            DBG(&g_I2cClient->dev, "szRxData[%d] = 0x%x , %c \n", i*4+0, szRxData[i*4+0], szRxData[i*4+0]); // add for debug
//            DBG(&g_I2cClient->dev, "szRxData[%d] = 0x%x , %c \n", i*4+1, szRxData[i*4+1], szRxData[i*4+1]); // add for debug
            
            szRxData[i*4+2] = (nRegData2 & 0xFF);
            szRxData[i*4+3] = ((nRegData2 >> 8 ) & 0xFF);

//            DBG(&g_I2cClient->dev, "szRxData[%d] = 0x%x , %c \n", i*4+2, szRxData[i*4+2], szRxData[i*4+2]); // add for debug
//            DBG(&g_I2cClient->dev, "szRxData[%d] = 0x%x , %c \n", i*4+3, szRxData[i*4+3], szRxData[i*4+3]); // add for debug
        }

        // Clear burst mode
        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*10, GFP_KERNEL);
        }
    
        sprintf(*ppVersion, "%c%c%c%c%c%c%c%c%c%c", szRxData[2], szRxData[3], szRxData[4],
            szRxData[5], szRxData[6], szRxData[7], szRxData[8], szRxData[9], szRxData[10], szRxData[11]);

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvTouchDeviceHwReset();

        mutex_unlock(&g_Mutex);
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)   
    {
        u8 szTxData[1] = {0};
        u8 szRxData[10] = {0};
    
        szTxData[0] = 0x0C;

        mutex_lock(&g_Mutex);
    
        //DrvTouchDeviceHwReset();

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

        IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
        mdelay(I2C_SMBUS_READ_COMMAND_DELAY_FOR_PLATFORM);
        IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 10);

        mutex_unlock(&g_Mutex);
           
        DBG(&g_I2cClient->dev, "szRxData[0] = 0x%x , %c \n", szRxData[0], szRxData[0]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[1] = 0x%x , %c \n", szRxData[1], szRxData[1]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[2] = 0x%x , %c \n", szRxData[2], szRxData[2]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[3] = 0x%x , %c \n", szRxData[3], szRxData[3]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[4] = 0x%x , %c \n", szRxData[4], szRxData[4]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[5] = 0x%x , %c \n", szRxData[5], szRxData[5]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[6] = 0x%x , %c \n", szRxData[6], szRxData[6]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[7] = 0x%x , %c \n", szRxData[7], szRxData[7]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[8] = 0x%x , %c \n", szRxData[8], szRxData[8]); // add for debug
        DBG(&g_I2cClient->dev, "szRxData[9] = 0x%x , %c \n", szRxData[9], szRxData[9]); // add for debug

        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*10, GFP_KERNEL);
        }

        sprintf(*ppVersion, "%.10s", szRxData);
		sscanf(*ppVersion, "V%02u.%03u.%02u\n", &g_PlatformFwVersion[0], &g_PlatformFwVersion[1], &g_PlatformFwVersion[2]);
		printk("*** %d, ILITEK %s() Platform FW version = %02u.%03u.%02u ***\n", __LINE__, __func__, g_PlatformFwVersion[0], g_PlatformFwVersion[1], g_PlatformFwVersion[2]);
		//platform FW version V01.010.03 is FW format check point
		if(g_PlatformFwVersion[0]*100000 + (g_PlatformFwVersion[1])*100 + g_PlatformFwVersion[2] >= 101003)
		{
			g_FwVersionFlag = 1;
		}
    }
    else
    {
        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*10, GFP_KERNEL);
        }
    
        sprintf(*ppVersion, "%s", "N/A");
    }

    DBG(&g_I2cClient->dev, "*** platform firmware version = %s ***\n", *ppVersion);
}

static s32 _DrvUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvUpdateFirmwareCash(szFwData, eEmemType);
}	

s32 DrvCheckUpdateFirmwareBySdCard(const char *pFilePath)
{
    s32 nRetVal = -1;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)    
    {
        nRetVal = DrvUpdateFirmwareBySdCard(pFilePath);
    }
    else
    {
        DBG(&g_I2cClient->dev, "This chip type (0x%x) does not support update firmware by sd card\n", g_ChipType);
    }
    
    return nRetVal;
}	

//------------------------------------------------------------------------------//

static u16 _DrvChangeFirmwareMode(u16 nMode)
{
    DBG(&g_I2cClient->dev, "*** %s() *** nMode = 0x%x\n", __func__, nMode);

    if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)  
    {
        u8 szTxData[2] = {0};
        u32 i = 0;
        s32 rc;

        g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for device driver can send change firmware mode i2c command to firmware. 
        szTxData[0] = 0x02;        
        szTxData[1] = (u8)nMode;

        mutex_lock(&g_Mutex);
        DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug

        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 2);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Change firmware mode success\n");
                break;
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Change firmware mode failed, rc = %d\n", rc);
        }

        DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
        mutex_unlock(&g_Mutex);

        g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after device driver have sent i2c command to firmware. 
    }

    return nMode;
}

static void _DrvSelfGetFirmwareInfo(SelfFirmwareInfo_t *pInfo) // for MSG22xx
{
    u8 szTxData[1] = {0};
    u8 szRxData[8] = {0};
    u32 i = 0;
    s32 rc;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for device driver can send get firmware info i2c command to firmware. 

    szTxData[0] = 0x01;

    mutex_lock(&g_Mutex);
    
    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get firmware info IicWriteData() success\n");
        }
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 8);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get firmware info IicReadData() success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Get firmware info failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);
    
    if (szRxData[0] == 0x81)
    {
        pInfo->nIsCanChangeFirmwareMode = 1;	
    }
    else
    {
        pInfo->nIsCanChangeFirmwareMode = 0;	
    }
    
    pInfo->nFirmwareMode = szRxData[1] & 0x7F;
    pInfo->nLogModePacketHeader = szRxData[2];
    pInfo->nLogModePacketLength = (szRxData[3]<<8) + szRxData[4];
    memcpy(g_DebugFwModeBuffer, szRxData, sizeof(szRxData));

    DBG(&g_I2cClient->dev, "pInfo->nFirmwareMode=0x%x, pInfo->nLogModePacketHeader=0x%x, pInfo->nLogModePacketLength=%d, pInfo->nIsCanChangeFirmwareMode=%d\n", pInfo->nFirmwareMode, pInfo->nLogModePacketHeader, pInfo->nLogModePacketLength, pInfo->nIsCanChangeFirmwareMode);

    g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after device driver have sent i2c command to firmware. 
}

static void _DrvMutualGetFirmwareInfo(MutualFirmwareInfo_t *pInfo) // for MSG28xx/MSG58xxA/ILI21xx
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)   
    {
        u8 szTxData[1] = {0};
        u8 szRxData[10] = {0};
        u32 i = 0;
        s32 rc;
    
        g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for device driver can send get firmware info i2c command to firmware.       
        szTxData[0] = 0x01;
        mutex_lock(&g_Mutex);
        DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug
    
#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Get firmware info IicWriteData() success\n");
            }
            
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 10);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Get firmware info IicReadData() success\n");

                if (szRxData[1] == FIRMWARE_MODE_DEMO_MODE || szRxData[1] == FIRMWARE_MODE_DEBUG_MODE)
                {
                    break;
                }
                else
                {
                    i = 0;
                }
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Get firmware info failed, rc = %d\n", rc);
        }

        DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
        mutex_unlock(&g_Mutex);
    
        // Add protection for incorrect firmware info check
        if ((szRxData[1] == FIRMWARE_MODE_DEBUG_MODE && szRxData[2] == 0xA7 && (szRxData[5] == PACKET_TYPE_TOOTH_PATTERN || szRxData[5] == PACKET_TYPE_CSUB_PATTERN || 
        szRxData[5] == PACKET_TYPE_FOUT_PATTERN || szRxData[5] == PACKET_TYPE_FREQ_PATTERN)) || (szRxData[1] == FIRMWARE_MODE_DEMO_MODE && szRxData[2] == 0x5A))
        {
            pInfo->nFirmwareMode = szRxData[1];
            DBG(&g_I2cClient->dev, "pInfo->nFirmwareMode = 0x%x\n", pInfo->nFirmwareMode);

            pInfo->nLogModePacketHeader = szRxData[2]; 
            pInfo->nLogModePacketLength = (szRxData[3]<<8) + szRxData[4]; 
            pInfo->nType = szRxData[5];
            pInfo->nMy = szRxData[6];
            pInfo->nMx = szRxData[7];
            pInfo->nSd = szRxData[8];
            pInfo->nSs = szRxData[9];

            DBG(&g_I2cClient->dev, "pInfo->nLogModePacketHeader = 0x%x\n", pInfo->nLogModePacketHeader);
            DBG(&g_I2cClient->dev, "pInfo->nLogModePacketLength = %d\n", pInfo->nLogModePacketLength);
            DBG(&g_I2cClient->dev, "pInfo->nType = 0x%x\n", pInfo->nType);
            DBG(&g_I2cClient->dev, "pInfo->nMy = %d\n", pInfo->nMy);
            DBG(&g_I2cClient->dev, "pInfo->nMx = %d\n", pInfo->nMx);
            DBG(&g_I2cClient->dev, "pInfo->nSd = %d\n", pInfo->nSd);
            DBG(&g_I2cClient->dev, "pInfo->nSs = %d\n", pInfo->nSs);
        }
        else
        {
            DBG(&g_I2cClient->dev, "Firmware info before correcting :\n");
            
            DBG(&g_I2cClient->dev, "FirmwareMode = 0x%x\n", szRxData[1]);
            DBG(&g_I2cClient->dev, "LogModePacketHeader = 0x%x\n", szRxData[2]);
            DBG(&g_I2cClient->dev, "LogModePacketLength = %d\n", (szRxData[3]<<8) + szRxData[4]);
            DBG(&g_I2cClient->dev, "Type = 0x%x\n", szRxData[5]);
            DBG(&g_I2cClient->dev, "My = %d\n", szRxData[6]);
            DBG(&g_I2cClient->dev, "Mx = %d\n", szRxData[7]);
            DBG(&g_I2cClient->dev, "Sd = %d\n", szRxData[8]);
            DBG(&g_I2cClient->dev, "Ss = %d\n", szRxData[9]);

            // Set firmware mode to demo mode(default)
            pInfo->nFirmwareMode = FIRMWARE_MODE_DEMO_MODE;
            pInfo->nLogModePacketHeader = 0x5A; 
            pInfo->nLogModePacketLength = DEMO_MODE_PACKET_LENGTH; 
            pInfo->nType = 0;
            pInfo->nMy = 0;
            pInfo->nMx = 0;
            pInfo->nSd = 0;
            pInfo->nSs = 0;

            DBG(&g_I2cClient->dev, "Firmware info after correcting :\n");

            DBG(&g_I2cClient->dev, "pInfo->nFirmwareMode = 0x%x\n", pInfo->nFirmwareMode);
            DBG(&g_I2cClient->dev, "pInfo->nLogModePacketHeader = 0x%x\n", pInfo->nLogModePacketHeader);
            DBG(&g_I2cClient->dev, "pInfo->nLogModePacketLength = %d\n", pInfo->nLogModePacketLength);
            DBG(&g_I2cClient->dev, "pInfo->nType = 0x%x\n", pInfo->nType);
            DBG(&g_I2cClient->dev, "pInfo->nMy = %d\n", pInfo->nMy);
            DBG(&g_I2cClient->dev, "pInfo->nMx = %d\n", pInfo->nMx);
            DBG(&g_I2cClient->dev, "pInfo->nSd = %d\n", pInfo->nSd);
            DBG(&g_I2cClient->dev, "pInfo->nSs = %d\n", pInfo->nSs);
        }
        
        g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after device driver have sent i2c command to firmware. 
    }
}

void DrvRestoreFirmwareModeToLogDataMode(void)
{
    DBG(&g_I2cClient->dev, "*** %s() g_IsSwitchModeByAPK = %d ***\n", __func__, g_IsSwitchModeByAPK);

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        if (g_IsSwitchModeByAPK == 1)
        {
            SelfFirmwareInfo_t tInfo;
    
            memset(&tInfo, 0x0, sizeof(SelfFirmwareInfo_t));

            _DrvSelfGetFirmwareInfo(&tInfo);

            DBG(&g_I2cClient->dev, "g_FirmwareMode = 0x%x, tInfo.nFirmwareMode = 0x%x\n", g_FirmwareMode, tInfo.nFirmwareMode);

            // Since reset_hw() will reset the firmware mode to demo mode, we must reset the firmware mode again after reset_hw().
            if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && FIRMWARE_MODE_DEBUG_MODE != tInfo.nFirmwareMode)
            {
                g_FirmwareMode = _DrvChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
            }
            else if (g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE && FIRMWARE_MODE_RAW_DATA_MODE != tInfo.nFirmwareMode)
            {
                g_FirmwareMode = _DrvChangeFirmwareMode(FIRMWARE_MODE_RAW_DATA_MODE);
            }
            else
            {
                DBG(&g_I2cClient->dev, "firmware mode is not restored\n");
            }
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        if (g_IsSwitchModeByAPK == 1)
        {
            MutualFirmwareInfo_t tInfo;
    
            memset(&tInfo, 0x0, sizeof(MutualFirmwareInfo_t));

            _DrvMutualGetFirmwareInfo(&tInfo);

            DBG(&g_I2cClient->dev, "g_FirmwareMode = 0x%x, tInfo.nFirmwareMode = 0x%x\n", g_FirmwareMode, tInfo.nFirmwareMode);

            // Since reset_hw() will reset the firmware mode to demo mode, we must reset the firmware mode again after reset_hw().
            if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && FIRMWARE_MODE_DEBUG_MODE != tInfo.nFirmwareMode)
            {
                g_FirmwareMode = _DrvChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
            }
            else
            {
                DBG(&g_I2cClient->dev, "firmware mode is not restored\n");
            }
        }
    }
}	

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
static void _DrvGetTouchPacketAddress(u16* pDataAddress, u16* pFlagAddress)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A || (g_ChipType == CHIP_TYPE_MSG22XX && IS_SELF_FREQ_SCAN_ENABLED))   
    {
        s32 rc = 0;
        u32 i = 0;
        u8 szTxData[1] = {0};
        u8 szRxData[4] = {0};
        szTxData[0] = 0x05;
        mutex_lock(&g_Mutex);
        DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug

        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 1);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Get touch packet address IicWriteData() success\n");
            }

            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 4);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Get touch packet address IicReadData() success\n");
                break;
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Get touch packet address failed, rc = %d\n", rc);
        }

        if (rc < 0)
        {
            g_FwSupportSegment = 0;
        }
        else
        {
			if(g_ChipType == CHIP_TYPE_MSG22XX)
			{
				*pDataAddress = 0;
				*pFlagAddress = (szRxData[0]<<8) + szRxData[1];
			}
			else
			{
				*pDataAddress = (szRxData[0]<<8) + szRxData[1];
				*pFlagAddress = (szRxData[2]<<8) + szRxData[3];
			}

            g_FwSupportSegment = 1;

            DBG(&g_I2cClient->dev, "*** *pDataAddress = 0x%2X ***\n", *pDataAddress); // add for debug
            DBG(&g_I2cClient->dev, "*** *pFlagAddress = 0x%2X ***\n", *pFlagAddress); // add for debug
        }

        DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
        mutex_unlock(&g_Mutex);
    }
}

static int _DrvCheckFingerTouchPacketFlagBit1(void)
{
    u8 szTxData[3] = {0};
    s32 nRetVal;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szTxData[0] = 0x53;
    szTxData[1] = (g_FwPacketFlagAddress >> 8) & 0xFF;
    szTxData[2] = g_FwPacketFlagAddress & 0xFF;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
    IicReadData(SLAVE_I2C_ID_DWI2C, &_gTouchPacketFlag[0], 2);

    if ((_gTouchPacketFlag[0] & BIT1) == 0x00)
    {
        nRetVal = 0; // Bit1 is 0
    }
    else
    {
        nRetVal = 1; // Bit1 is 1
    }
    DBG(&g_I2cClient->dev, "Bit1 = %d\n", nRetVal);

    return nRetVal;
}

static void _DrvResetFingerTouchPacketFlagBit1(void)
{
    u8 szTxData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szTxData[0] = 0x52;
    szTxData[1] = (g_FwPacketFlagAddress >> 8) & 0xFF;
    szTxData[2] = g_FwPacketFlagAddress & 0xFF;
    szTxData[3] = _gTouchPacketFlag[0] | BIT1;

    IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
}
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

//------------------------------------------------------------------------------//

static void _DrvOpenGloveMode(void) // used for MSG28xx only
{
    s32 rc = 0;
    u8 szTxData[3] = {0};
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x01;
    szTxData[2] = 0x01;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc > 0)
        {
            g_IsEnableGloveMode = 1;

            DBG(&g_I2cClient->dev, "Open glove mode success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
      	DBG(&g_I2cClient->dev, "Open glove mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after device driver have sent i2c command to firmware. 
}

static void _DrvCloseGloveMode(void) // used for MSG28xx only
{
    s32 rc = 0;
    u8 szTxData[3] = {0};
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x01;
    szTxData[2] = 0x00;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc > 0)
        {
            g_IsEnableGloveMode = 0;

            DBG(&g_I2cClient->dev, "Close glove mode success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
      	DBG(&g_I2cClient->dev, "Close glove mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after device driver have sent i2c command to firmware. 
}

static void _DrvGetGloveInfo(u8 *pGloveMode) // used for MSG28xx only
{
    u8 szTxData[3] = {0};
    u8 szRxData[2] = {0};
    u32 i = 0;
    s32 rc;

    g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x01;
    szTxData[2] = 0x02;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get glove info IicWriteData() success\n");
        }
        
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 1);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get glove info IicReadData() success\n");

            if (szRxData[0] == 0x00 || szRxData[0] == 0x01)
            {
                break;
            }
            else
            {
                i = 0;
            }
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Get glove info failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    *pGloveMode = szRxData[0];
    
    DBG(&g_I2cClient->dev, "*pGloveMode = 0x%x\n", *pGloveMode);

    g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after device driver have sent i2c command to firmware. 
}

//------------------------------------------------------------------------------//

static void _DrvOpenLeatherSheathMode(void) // used for MSG28xx only
{
    s32 rc = 0;
    u8 szTxData[3] = {0};
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x02;
    szTxData[2] = 0x01;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc > 0)
        {
            g_IsEnableLeatherSheathMode = 1;

            DBG(&g_I2cClient->dev, "Open leather sheath mode success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
      	DBG(&g_I2cClient->dev, "Open leather sheath mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after device driver have sent i2c command to firmware. 
}

static void _DrvCloseLeatherSheathMode(void) // used for MSG28xx only
{
    s32 rc = 0;
    u8 szTxData[3] = {0};
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x02;
    szTxData[2] = 0x00;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc > 0)
        {
            g_IsEnableLeatherSheathMode = 0;

            DBG(&g_I2cClient->dev, "Close leather sheath mode success\n");
            break;
        }

        i++;
    }
    if (i == 5)
    {
      	DBG(&g_I2cClient->dev, "Close leather sheath mode failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after device driver have sent i2c command to firmware. 
}

static void _DrvGetLeatherSheathInfo(u8 *pLeatherSheathMode) // used for MSG28xx only
{
    u8 szTxData[3] = {0};
    u8 szRxData[2] = {0};
    u32 i = 0;
    s32 rc;

    g_IsDisableFingerTouch = 1; // Skip finger touch ISR handling temporarily for device driver can send i2c command to firmware.

    szTxData[0] = 0x06;
    szTxData[1] = 0x02;
    szTxData[2] = 0x02;

    mutex_lock(&g_Mutex);

    while (i < 5)
    {
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get leather sheath info IicWriteData() success\n");
        }
        
        mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szRxData[0], 1);
        if (rc > 0)
        {
            DBG(&g_I2cClient->dev, "Get leather sheath info IicReadData() success\n");

            if (szRxData[0] == 0x00 || szRxData[0] == 0x01)
            {
                break;
            }
            else
            {
                i = 0;
            }
        }

        i++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "Get leather sheath info failed, rc = %d\n", rc);
    }

    mutex_unlock(&g_Mutex);

    *pLeatherSheathMode = szRxData[0];
    
    DBG(&g_I2cClient->dev, "*pLeatherSheathMode = 0x%x\n", *pLeatherSheathMode);

    g_IsDisableFingerTouch = 0; // Resume finger touch ISR handling after device driver have sent i2c command to firmware. 
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
void DrvChargerDetection(u8 nChargerStatus)
{
    u32 i = 0;
    u8 szTxData[2] = {0};
    s32 rc = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DBG(&g_I2cClient->dev, "_gChargerPlugIn = %d, nChargerStatus = %d, g_ForceUpdate = %d\n", _gChargerPlugIn, nChargerStatus, g_ForceUpdate);

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)   
    {
        szTxData[0] = 0x09;      
    }

    if (nChargerStatus) // charger plug in
    {
        if (_gChargerPlugIn == 0 || g_ForceUpdate == 1)
        {
          	szTxData[1] = 0xA5;
            
            while (i < 5)
            {
                mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
                mutex_lock(&g_Mutex);
                rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 2);
                mutex_unlock(&g_Mutex);
                if (rc > 0)
                {
                    _gChargerPlugIn = 1;

                    DBG(&g_I2cClient->dev, "Update status for charger plug in success.\n");
                    break;
                }

                i ++;
            }
            if (i == 5)
            {
                DBG(&g_I2cClient->dev, "Update status for charger plug in failed, rc = %d\n", rc);
            }

            g_ForceUpdate = 0; // Clear flag after force update charger status
        }
    }
    else  // charger plug out
    {
        if (_gChargerPlugIn == 1 || g_ForceUpdate == 1)
        {
          	szTxData[1] = 0x5A;
            
            while (i < 5)
            {
                mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
                mutex_lock(&g_Mutex);
                rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 2);
                mutex_unlock(&g_Mutex);
                if (rc > 0)
                {
                    _gChargerPlugIn = 0;

                    DBG(&g_I2cClient->dev, "Update status for charger plug out success.\n");
                    break;
                }
                
                i ++;
            }
            if (i == 5)
            {
                DBG(&g_I2cClient->dev, "Update status for charger plug out failed, rc = %d\n", rc);
            }

            g_ForceUpdate = 0; // Clear flag after force update charger status
        }
    }	
}	

static void _DrvChargerPlugInOutCheck(struct work_struct *pWork)
{
    u8 szChargerStatus[20] = {0};
    s32 i = 0;
    DBG(&g_I2cClient->dev, "*** %s() g_IsEnableChargerPlugInOutCheck = %d ***\n", __func__, g_IsEnableChargerPlugInOutCheck); 

    if (g_IsEnableChargerPlugInOutCheck == 0)
    {
        return;
    }
/*
    if (_gInterruptFlag == 0) // Skip charger plug in/out check while finger touch
    {
        DBG(&g_I2cClient->dev, "Not allow to do charger plug in/out check while finger touch.\n");
        goto ChargerPlugInOutCheckEnd;
    }
*/    	
    if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
    {
        DBG(&g_I2cClient->dev, "Not allow to do charger plug in/out check while update firmware is proceeding.\n");
        goto ChargerPlugInOutCheckEnd;
    }

#ifdef CONFIG_ENABLE_ITO_MP_TEST
    if (g_IsInMpTest == 1) // Check whether mp test is proceeding
    {
        DBG(&g_I2cClient->dev, "Not allow to do charger plug in/out check while mp test is proceeding.\n");
        goto ChargerPlugInOutCheckEnd;
    }
#endif //CONFIG_ENABLE_ITO_MP_TEST
     
    DrvReadFile(POWER_SUPPLY_BATTERY_STATUS_PATCH, szChargerStatus, 20);

    DBG(&g_I2cClient->dev, "*** Battery Status : %s ***\n", szChargerStatus);

    if (strstr(szChargerStatus, "Charging") != NULL || strstr(szChargerStatus, "Full") != NULL || strstr(szChargerStatus, "Fully charged") != NULL)
    {
        DrvChargerDetection(1); // charger plug-in
    }
    else // Not charging
    {
        DrvChargerDetection(0); // charger plug-out
    }

ChargerPlugInOutCheckEnd :

    if (g_IsEnableChargerPlugInOutCheck == 1)
    {
        queue_delayed_work(g_ChargerPlugInOutCheckWorkqueue, &g_ChargerPlugInOutCheckWork, CHARGER_DETECT_CHECK_PERIOD);
    }
}
#endif //CONFIG_ENABLE_CHARGER_DETECTION

//------------------------------------------------------------------------------//

static s32 _DrvMutualReadFingerTouchData(u8 *pPacket, u16 nReportPacketLength)
{
    s32 rc;

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {	
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            if (rc < 0)
            {
                DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
                return -1;
            }
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE)
        {
#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
            DBG(&g_I2cClient->dev, "*** g_FwPacketDataAddress = 0x%2X ***\n", g_FwPacketDataAddress); // add for debug
            DBG(&g_I2cClient->dev, "*** g_FwPacketFlagAddress = 0x%2X ***\n", g_FwPacketFlagAddress); // add for debug

            if (g_FwSupportSegment == 0)
            {
                DBG(&g_I2cClient->dev, "g_FwPacketDataAddress & g_FwPacketFlagAddress is un-initialized\n");
                return -1;
            }

            if (g_IsDisableFingerTouch == 1)
            {
                DBG(&g_I2cClient->dev, "Skip finger touch for handling get firmware info or change firmware mode\n");
                return -1;
            }

            if (0 != _DrvCheckFingerTouchPacketFlagBit1())
            {
                DBG(&g_I2cClient->dev, "Bit1 is not 0. FW is not ready for providing debug mode packet to Device Driver\n");
                return -1;
            }

            rc = IicSegmentReadDataBySmBus(g_FwPacketDataAddress, &pPacket[0], nReportPacketLength, MAX_I2C_TRANSACTION_LENGTH_LIMIT);

            _DrvResetFingerTouchPacketFlagBit1();

            if (rc < 0)
            {
                DBG(&g_I2cClient->dev, "I2C read debug mode packet data failed, rc = %d\n", rc);
                return -1;
            }
#else

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
            mdelay(I2C_SMBUS_READ_COMMAND_DELAY_FOR_PLATFORM);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM

            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            if (rc < 0)
            {
                DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
                return -1;
            }
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA    		
        }
        else
        {
            DBG(&g_I2cClient->dev, "WRONG FIRMWARE MODE : 0x%x\n", g_FirmwareMode);
            return -1;
        }
    }
    else
    {	
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
        if (rc < 0)
        {
            DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
            return -1;
        }
    } //IS_FIRMWARE_DATA_LOG_ENABLED	

    return 0;
}

static void _DrvSelfHandleFingerTouch(void) // for MSG22xx
{
    SelfTouchInfo_t tInfo;
    u32 i;
	u8 szTxData[4] = {0};
#ifdef CONFIG_TP_HAVE_KEY
    u8 nTouchKeyCode = 0;
#endif //CONFIG_TP_HAVE_KEY
    static u32 nLastKeyCode = 0;
    u8 *pPacket = NULL;
    u16 nReportPacketLength = 0;
    s32 rc;

//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);  // add for debug
    
    if (g_IsDisableFingerTouch == 1)
    {
        DBG(&g_I2cClient->dev, "Skip finger touch for handling get firmware info or change firmware mode\n");
        return;
    }
    
    mutex_lock(&g_Mutex);
//    DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug

    memset(&tInfo, 0x0, sizeof(SelfTouchInfo_t));

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    { 	
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEMO_MODE\n");

            nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
            pPacket = g_DemoModePacket;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE)
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEBUG_MODE\n");

            if (g_SelfFirmwareInfo.nLogModePacketHeader != 0x62 && g_SelfFirmwareInfo.nLogModePacketHeader != 0xA7)
            {
                DBG(&g_I2cClient->dev, "WRONG DEBUG MODE HEADER : 0x%x\n", g_SelfFirmwareInfo.nLogModePacketHeader);
                goto TouchHandleEnd;		
            }

            if (g_ChipType == CHIP_TYPE_MSG22XX && IS_SELF_FREQ_SCAN_ENABLED)
            {
                nReportPacketLength = SELF_FREQ_SCAN_PACKET_LENGTH;
				szTxData[0] = 0x53;
				szTxData[1] = g_FwPacketFlagAddress >> 8;
				szTxData[2] = g_FwPacketFlagAddress;
				IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
            }
            else
            {
                nReportPacketLength = g_SelfFirmwareInfo.nLogModePacketLength;
            }

            pPacket = g_DebugModePacket;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE)
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_RAW_DATA_MODE\n");

            if (g_SelfFirmwareInfo.nLogModePacketHeader != 0x62 && g_SelfFirmwareInfo.nLogModePacketHeader != 0xA7)
            {
                DBG(&g_I2cClient->dev, "WRONG RAW DATA MODE HEADER : 0x%x\n", g_SelfFirmwareInfo.nLogModePacketHeader);
                goto TouchHandleEnd;		
            }

            if (g_ChipType == CHIP_TYPE_MSG22XX && IS_SELF_FREQ_SCAN_ENABLED)
            {
                nReportPacketLength = SELF_FREQ_SCAN_PACKET_LENGTH;
				szTxData[0] = 0x53;
				szTxData[1] = g_FwPacketFlagAddress >> 8;
				szTxData[2] = g_FwPacketFlagAddress;
				IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
				DBG(&g_I2cClient->dev, "nReportPacketLength = %d\n", nReportPacketLength);
            }
            else
            {
                nReportPacketLength = g_SelfFirmwareInfo.nLogModePacketLength;
            }
            pPacket = g_DebugModePacket;
        }
        else
        {
            DBG(&g_I2cClient->dev, "WRONG FIRMWARE MODE : 0x%x\n", g_FirmwareMode);
            goto TouchHandleEnd;		
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEMO_MODE\n");

        nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
        pPacket = g_DemoModePacket;
    } //IS_FIRMWARE_DATA_LOG_ENABLED

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    if (g_GestureDebugMode == 1 && g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture debug mode packet length, g_ChipType=0x%x\n", g_ChipType);

        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
            nReportPacketLength = GESTURE_DEBUG_MODE_PACKET_LENGTH;
            pPacket = _gGestureWakeupPacket;
        }
        else
        {
            DBG(&g_I2cClient->dev, "This chip type does not support gesture debug mode.\n");
            goto TouchHandleEnd;		
        }
    }
    else if (g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture wakeup packet length, g_ChipType=0x%x\n", g_ChipType);
      
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
            nReportPacketLength = GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH;
#else
            nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
            pPacket = _gGestureWakeupPacket;
        }
        else
        {
            DBG(&g_I2cClient->dev, "This chip type does not support gesture wakeup.\n");
            goto TouchHandleEnd;
        }
    }

#else

    if (g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture wakeup packet length, g_ChipType=0x%x\n", g_ChipType);
        
        if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
            nReportPacketLength = GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH;
#else
            nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

            pPacket = _gGestureWakeupPacket;
        } 
        else
        {
            DBG(&g_I2cClient->dev, "This chip type does not support gesture wakeup.\n");
            goto TouchHandleEnd;		
        }
    }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        u32 i = 0;
        
        while (i < 5)
        {
            mdelay(50);

            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            
            if (rc > 0)
            {
                break;
            }
            
            i ++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
            goto TouchHandleEnd;		
        }
    }
    else
    {
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
        if (rc < 0)
        {
            DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
            goto TouchHandleEnd;		
        }
    }
#else
    rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
        goto TouchHandleEnd;		
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP   
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
        goto TouchHandleEnd;		
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    if (nReportPacketLength > 8)
    {
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
    }
    else
    {
        rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
    }

    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
        goto TouchHandleEnd;		
    }
#endif
    DBG(&g_I2cClient->dev, "nReportPacketLength = %d\n", nReportPacketLength);
    if (0 == _DrvSelfParsePacket(pPacket, nReportPacketLength, &tInfo))
    {
        //report...
        if ((tInfo.nFingerNum) == 0)   //touch end
        {
            if (nLastKeyCode != 0)
            {
                DBG(&g_I2cClient->dev, "key touch released\n");

                input_report_key(g_InputDevice, BTN_TOUCH, 0);
                input_report_key(g_InputDevice, nLastKeyCode, 0);
                input_sync(g_InputDevice);
                    
                nLastKeyCode = 0; //clear key status..
            }
            else
            {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
                input_report_key(g_InputDevice, BTN_TOUCH, 0);

                for (i = 0; i < MAX_TOUCH_NUM; i ++) 
                {
                    DrvFingerTouchReleased(0, 0, i);
                    _gPriorPress[i] = _gCurrPress[i];
                }

                input_report_key(g_InputDevice, BTN_TOOL_FINGER, 0);
#else // TYPE A PROTOCOL
                DrvFingerTouchReleased(0, 0, 0);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

                input_sync(g_InputDevice);
            }
        }
        else //touch on screen
        {
            if (tInfo.nTouchKeyCode != 0)
            {
#ifdef CONFIG_TP_HAVE_KEY
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                if (tInfo.nTouchKeyCode == 4) // TOUCH_KEY_HOME
                {
                    nTouchKeyCode = g_TpVirtualKey[1];           
                }
                else if (tInfo.nTouchKeyCode == 1) // TOUCH_KEY_MENU
                {
                    nTouchKeyCode = g_TpVirtualKey[0];
                }           
                else if (tInfo.nTouchKeyCode == 2) // TOUCH_KEY_BACK
                {
                    nTouchKeyCode = g_TpVirtualKey[2];
                }           
                else if (tInfo.nTouchKeyCode == 8) // TOUCH_KEY_SEARCH 
                {	
                    nTouchKeyCode = g_TpVirtualKey[3];           
                }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                if (tpd_dts_data.use_tpd_button)
                {
                    if (tInfo.nTouchKeyCode == 4) // TOUCH_KEY_HOME
                    {
                        nTouchKeyCode = tpd_dts_data.tpd_key_local[1];           
                    }
                    else if (tInfo.nTouchKeyCode == 1) // TOUCH_KEY_MENU
                    {
                        nTouchKeyCode = tpd_dts_data.tpd_key_local[0];
                    }           
                    else if (tInfo.nTouchKeyCode == 2) // TOUCH_KEY_BACK
                    {
                        nTouchKeyCode = tpd_dts_data.tpd_key_local[2];
                    }           
                    else if (tInfo.nTouchKeyCode == 8) // TOUCH_KEY_SEARCH 
                    {	
                        nTouchKeyCode = tpd_dts_data.tpd_key_local[3];           
                    }
                }
#else
                if (tInfo.nTouchKeyCode == 4) // TOUCH_KEY_HOME
                {
                    nTouchKeyCode = g_TpVirtualKey[1];           
                }
                else if (tInfo.nTouchKeyCode == 1) // TOUCH_KEY_MENU
                {
                    nTouchKeyCode = g_TpVirtualKey[0];
                }           
                else if (tInfo.nTouchKeyCode == 2) // TOUCH_KEY_BACK
                {
                    nTouchKeyCode = g_TpVirtualKey[2];
                }           
                else if (tInfo.nTouchKeyCode == 8) // TOUCH_KEY_SEARCH 
                {	
                    nTouchKeyCode = g_TpVirtualKey[3];           
                }
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif

                if (nLastKeyCode != nTouchKeyCode)
                {
                    DBG(&g_I2cClient->dev, "key touch pressed\n");
                    DBG(&g_I2cClient->dev, "nTouchKeyCode = %d, nLastKeyCode = %d\n", nTouchKeyCode, nLastKeyCode);
                    
                    nLastKeyCode = nTouchKeyCode;

                    input_report_key(g_InputDevice, BTN_TOUCH, 1);
                    input_report_key(g_InputDevice, nTouchKeyCode, 1);
                    input_sync(g_InputDevice);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL 
                    _gPrevTouchStatus = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                }
#endif //CONFIG_TP_HAVE_KEY
            }
            else
            {
                DBG(&g_I2cClient->dev, "tInfo->nFingerNum = %d...............\n", tInfo.nFingerNum);
                
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                for (i = 0; i < MAX_TOUCH_NUM; i ++) 
                {
                    if (tInfo.nFingerNum != 0)
                    {
                        if (_gCurrPress[i])
                        {
                            input_report_key(g_InputDevice, BTN_TOUCH, 1);
                            DrvFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, 1, i);
                            input_report_key(g_InputDevice, BTN_TOOL_FINGER, 1);
                        }
                        else if (0 == _gCurrPress[i] && 1 == _gPriorPress[i])
                        {
                            DrvFingerTouchReleased(0, 0, i);
                        }
                    }
                    _gPriorPress[i] = _gCurrPress[i];
                }
#else // TYPE A PROTOCOL
                for (i = 0; i < tInfo.nFingerNum; i ++) 
                {
                    DrvFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, 1, 0);
                }
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

                input_sync(g_InputDevice);
            }
        }

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
        if (g_IsEnableReportRate == 1)
        {
            if (g_ValidTouchCount == 4294967295UL)
            {
                g_ValidTouchCount = 0; // Reset count if overflow
                DBG(&g_I2cClient->dev, "g_ValidTouchCount reset to 0\n");
            } 	

            g_ValidTouchCount ++;

            DBG(&g_I2cClient->dev, "g_ValidTouchCount = %d\n", g_ValidTouchCount);
        }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE
    }

    TouchHandleEnd: 
    	
//    DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
    mutex_unlock(&g_Mutex);
}

static void _DrvMutualHandleFingerTouch(void) // for MSG28xx/MSG58xxA
{
    MutualTouchInfo_t tInfo;
    u32 i = 0;
#ifdef CONFIG_TP_HAVE_KEY
    static u32 nLastKeyCode = 0xFF;
#endif //CONFIG_TP_HAVE_KEY
    static u32 nLastCount = 0;
    u8 *pPacket = NULL;
    u16 nReportPacketLength = 0;

//    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);  // add for debug
    
    if (g_IsDisableFingerTouch == 1)
    {
        DBG(&g_I2cClient->dev, "Skip finger touch for handling get firmware info or change firmware mode\n");
        return;
    }

    mutex_lock(&g_Mutex); 
//    DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug

    memset(&tInfo, 0x0, sizeof(MutualTouchInfo_t));

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {	
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            {
                DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEMO_MODE\n");

                nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
                pPacket = g_DemoModePacket;
            }
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE)
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEBUG_MODE\n");

            if (g_MutualFirmwareInfo.nLogModePacketHeader != 0xA7)
            {
                DBG(&g_I2cClient->dev, "WRONG DEBUG MODE HEADER : 0x%x\n", g_MutualFirmwareInfo.nLogModePacketHeader);
                goto TouchHandleEnd;		
            }            
            {
                nReportPacketLength = g_MutualFirmwareInfo.nLogModePacketLength;
                pPacket = g_DebugModePacket;
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "WRONG FIRMWARE MODE : 0x%x\n", g_FirmwareMode);
            goto TouchHandleEnd;		
        }
    }
    else
    {    
        {
            DBG(&g_I2cClient->dev, "FIRMWARE_MODE_DEMO_MODE\n");

            nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
            pPacket = g_DemoModePacket;
        }
    } //IS_FIRMWARE_DATA_LOG_ENABLED

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    if (g_GestureDebugMode == 1 && g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture debug mode packet length, g_ChipType=0x%x\n", g_ChipType);

        if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A) 
        {
            nReportPacketLength = GESTURE_DEBUG_MODE_PACKET_LENGTH;
            pPacket = _gGestureWakeupPacket;
        }
        else
        {
            DBG(&g_I2cClient->dev, "This chip type does not support gesture debug mode.\n");
            goto TouchHandleEnd;		
        }
    }
    else if (g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture wakeup packet length\n");

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        nReportPacketLength = GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH;
#else
        nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

        pPacket = _gGestureWakeupPacket;
    }

#else

    if (g_GestureWakeupFlag == 1)
    {
        DBG(&g_I2cClient->dev, "Set gesture wakeup packet length\n");

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        nReportPacketLength = GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH;
#else
        nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

        pPacket = _gGestureWakeupPacket;
    }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        s32 rc;
        
        while (i < 5)
        {
            mdelay(50);

            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &pPacket[0], nReportPacketLength);
            if (rc > 0)
            {
                break;
            }
            
            i ++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "I2C read packet data failed, rc = %d\n", rc);
            goto TouchHandleEnd;		
        }
    }
    else
    {
        if (0 != _DrvMutualReadFingerTouchData(&pPacket[0], nReportPacketLength))
        {
            goto TouchHandleEnd;		
        }
    }
#else
    if (0 != _DrvMutualReadFingerTouchData(&pPacket[0], nReportPacketLength))
    {
         goto TouchHandleEnd;		
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP   
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    if (0 != _DrvMutualReadFingerTouchData(&pPacket[0], nReportPacketLength))
    {
        goto TouchHandleEnd;		
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    {
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
        
        if (0 != _DrvMutualReadFingerTouchData(&pPacket[0], nReportPacketLength))
        {
            goto TouchHandleEnd;		
        }
    }
#endif

    if (0 == _DrvMutualParsePacket(pPacket, nReportPacketLength, &tInfo))
    {
#ifdef CONFIG_TP_HAVE_KEY
        if (tInfo.nKeyCode != 0xFF)   //key touch pressed
        {
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
            DBG(&g_I2cClient->dev, "tInfo.nKeyCode=%x, nLastKeyCode=%x, g_TpVirtualKey[%d]=%d\n", tInfo.nKeyCode, nLastKeyCode, tInfo.nKeyCode, g_TpVirtualKey[tInfo.nKeyCode]);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
            if (tpd_dts_data.use_tpd_button)
            {
                DBG(&g_I2cClient->dev, "tInfo.nKeyCode=%x, nLastKeyCode=%x, tpd_dts_data.tpd_key_local[%d]=%d\n", tInfo.nKeyCode, nLastKeyCode, tInfo.nKeyCode, tpd_dts_data.tpd_key_local[tInfo.nKeyCode]);
            }
#else
            DBG(&g_I2cClient->dev, "tInfo.nKeyCode=%x, nLastKeyCode=%x, g_TpVirtualKey[%d]=%d\n", tInfo.nKeyCode, nLastKeyCode, tInfo.nKeyCode, g_TpVirtualKey[tInfo.nKeyCode]);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif   
         
            if (tInfo.nKeyCode < MAX_KEY_NUM)
            {
                if (tInfo.nKeyCode != nLastKeyCode)
                {
                    DBG(&g_I2cClient->dev, "key touch pressed\n");

                    input_report_key(g_InputDevice, BTN_TOUCH, 1);
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                    input_report_key(g_InputDevice, g_TpVirtualKey[tInfo.nKeyCode], 1);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                    if (tpd_dts_data.use_tpd_button)
                    {
                        input_report_key(g_InputDevice, tpd_dts_data.tpd_key_local[tInfo.nKeyCode], 1);
                    }
#else
                    input_report_key(g_InputDevice, g_TpVirtualKey[tInfo.nKeyCode], 1);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif   
                    input_sync(g_InputDevice);

                    nLastKeyCode = tInfo.nKeyCode;
                }
                else
                {
                    /// pass duplicate key-pressing
                    DBG(&g_I2cClient->dev, "REPEATED KEY\n");
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "WRONG KEY\n");
            }
        }
        else                        //key touch released
        {
            if (nLastKeyCode != 0xFF)
            {
                DBG(&g_I2cClient->dev, "key touch released\n");

                input_report_key(g_InputDevice, BTN_TOUCH, 0);
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                input_report_key(g_InputDevice, g_TpVirtualKey[nLastKeyCode], 0);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                if (tpd_dts_data.use_tpd_button)
                {
                    input_report_key(g_InputDevice, tpd_dts_data.tpd_key_local[nLastKeyCode], 0);
                }
#else
                input_report_key(g_InputDevice, g_TpVirtualKey[nLastKeyCode], 0);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif   
                input_sync(g_InputDevice);
                
                nLastKeyCode = 0xFF;
            }
        }
#endif //CONFIG_TP_HAVE_KEY

        DBG(&g_I2cClient->dev, "tInfo.nCount = %d, nLastCount = %d\n", tInfo.nCount, nLastCount);

        if (tInfo.nCount > 0)          //point touch pressed
        {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            for (i = 0; i < tInfo.nCount; i ++)
            {
                input_report_key(g_InputDevice, BTN_TOUCH, 1); 
                DrvFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, tInfo.tPoint[i].nP, tInfo.tPoint[i].nId);
                input_report_key(g_InputDevice, BTN_TOOL_FINGER, 1); 	
            }

            for (i = 0; i < MAX_TOUCH_NUM; i ++)
            {
                DBG(&g_I2cClient->dev, "_gPreviousTouch[%d]=%d, _gCurrentTouch[%d]=%d\n", i, _gPreviousTouch[i], i, _gCurrentTouch[i]); // TODO : add for debug

                if (_gCurrentTouch[i] == 0 && _gPreviousTouch[i] == 1)
                {
                    DrvFingerTouchReleased(0, 0, i);
                }
                _gPreviousTouch[i] = _gCurrentTouch[i];
            }
#else // TYPE A PROTOCOL
            for (i = 0; i < tInfo.nCount; i ++)
            {
                DrvFingerTouchPressed(tInfo.tPoint[i].nX, tInfo.tPoint[i].nY, tInfo.tPoint[i].nP, tInfo.tPoint[i].nId);
            }
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

            input_sync(g_InputDevice);

            nLastCount = tInfo.nCount;
        }
        else                        //point touch released
        {
            if (nLastCount > 0)
            {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
                input_report_key(g_InputDevice, BTN_TOUCH, 0);                      

                for (i = 0; i < MAX_TOUCH_NUM; i ++)
                {
                    DBG(&g_I2cClient->dev, "_gPreviousTouch[%d]=%d, _gCurrentTouch[%d]=%d\n", i, _gPreviousTouch[i], i, _gCurrentTouch[i]); // TODO : add for debug

                    if (_gCurrentTouch[i] == 0 && _gPreviousTouch[i] == 1)
                    {
                        DrvFingerTouchReleased(0, 0, i);
                    }
                    _gPreviousTouch[i] = _gCurrentTouch[i];
                }

                input_report_key(g_InputDevice, BTN_TOOL_FINGER, 0); 	
#else // TYPE A PROTOCOL
                DrvFingerTouchReleased(0, 0, 0);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
    
                input_sync(g_InputDevice);

                nLastCount = 0;
            }
        }

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
        if (g_IsEnableReportRate == 1)
        {
            if (g_ValidTouchCount == 4294967295UL)
            {
                g_ValidTouchCount = 0; // Reset count if overflow
                DBG(&g_I2cClient->dev, "g_ValidTouchCount reset to 0\n");
            } 	

            g_ValidTouchCount ++;

            DBG(&g_I2cClient->dev, "g_ValidTouchCount = %d\n", g_ValidTouchCount);
        }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE
    }

    TouchHandleEnd: 
    	
//    DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
    mutex_unlock(&g_Mutex);
}

#ifdef CONFIG_ENABLE_ITO_MP_TEST
static ssize_t DrvMainProcfsMpTestCustomisedWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{
    memset(_gDebugBuf, 0, 1);
    if (copy_from_user(_gDebugBuf, pBuffer, 1))
    {
        DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

        return -EFAULT;
    }
    printk("%s,_gDebugBuf:%s\n", __func__, _gDebugBuf);
    g_MpTestMode = ms_atoi(_gDebugBuf);  	
    printk("%s,g_MpTestMode:%d\n", __func__, g_MpTestMode);
    return nCount;
}

static ssize_t DrvMainProcfsMpTestCustomisedRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 result[16]; 
    int res = 0;
    char *ini_path = INI_PATH;

    if (*pPos != 0)
        return 0;

    res = startMPTest(g_ChipType, ini_path);
    pr_info("MP Test Result = %d \n", res);
 
    nLength = sprintf(result, "%d", res);

    *pPos += nLength;
    DBG(&g_I2cClient->dev,"end\n");
    return nLength;
}
#endif	
//------------------------------------------------------------------------------//

static ssize_t _DrvProcfsChipTypeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
	g_ChipType = DrvGetChipType(); 
    nLength = sprintf(nUserTempBuffer, "%d", g_OriginalChipType);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
        return -EFAULT;
	
    DBG(&g_I2cClient->dev, "g_ChipType = 0x%x, g_OriginalChipType = 0x%x\n", g_ChipType, g_OriginalChipType);

    if (g_ChipType == CHIP_TYPE_MSG22XX)  // (0x7A)
    {
        DBG(&g_I2cClient->dev, "g_Msg22xxChipRevision = 0x%x\n", g_Msg22xxChipRevision); 
    }
    *pPos += nLength;

    return nLength;
}			  
		  
static ssize_t _DrvProcfsChipTypeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    //g_ChipType = DrvGetChipType(); 

    return nCount;
}

static ssize_t _DrvProcfsFirmwareDataRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    DBG(&g_I2cClient->dev, "*** %s() g_FwDataCount = %d ***\n", __func__, g_FwDataCount);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    *pPos += g_FwDataCount;

    return g_FwDataCount;
}			  
			  
static ssize_t _DrvProcfsFirmwareDataWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{
    u32 nNum = nCount / 1024;
    u32 nRemainder = nCount % 1024;
    u32 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)    
    {
        if (nNum > 0) // nCount >= 1024
        {
            for (i = 0; i < nNum; i ++)
            {
				memset(_gDebugBuf, 0, 1024);
				if (copy_from_user(_gDebugBuf, pBuffer+(i*1024), 1024))
				{
					DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

					return -EFAULT;
				}  			
                memcpy(g_FwData[g_FwDataCount], _gDebugBuf, 1024);

                g_FwDataCount ++;
            }

            if (nRemainder > 0) // Handle special firmware size like MSG22XX(48.5KB)
            {
                DBG(&g_I2cClient->dev, "nRemainder = %d\n", nRemainder);
				memset(_gDebugBuf, 0, 1024);
				if (copy_from_user(_gDebugBuf, pBuffer+(i*1024), nRemainder))
				{
					DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

					return -EFAULT;
				}  
                memcpy(g_FwData[g_FwDataCount], _gDebugBuf, nRemainder);

                g_FwDataCount ++;
            }
        }
        else // nCount < 1024
        {
            if (nCount > 0)
            {
				memset(_gDebugBuf, 0, 1024);
				if (copy_from_user(_gDebugBuf, pBuffer, nCount))
				{
					DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

					return -EFAULT;
				}  			
                memcpy(g_FwData[g_FwDataCount], _gDebugBuf, nCount);

                g_FwDataCount ++;
            }
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "Unsupported chip type = 0x%x\n", g_ChipType);
    }

    DBG(&g_I2cClient->dev, "*** g_FwDataCount = %d ***\n", g_FwDataCount);

    if (_gDebugBuf != NULL)
    {
        DBG(&g_I2cClient->dev, "*** buf[0] = %c ***\n", _gDebugBuf[0]);
    }

    return nCount;
}

static ssize_t _DrvProcfsFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16];
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
    
    nLength = sprintf(nUserTempBuffer, "%d", _gIsUpdateComplete);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
        return -EFAULT;

    DBG(&g_I2cClient->dev, "*** _gIsUpdateComplete = %d ***\n", _gIsUpdateComplete);

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsFirmwareUpdateWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    s32 nRetVal = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() g_FwDataCount = %d ***\n", __func__, g_FwDataCount);

    DrvDisableFingerTouchReport();

    if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)    
    {
        nRetVal = _DrvUpdateFirmware(g_FwData, EMEM_ALL);
    }
    else
    {
        DBG(&g_I2cClient->dev, "This chip type (0x%x) does not support update firmware by MTPTool APK\n", g_ChipType);
        nRetVal = -1;
    }

    if (0 != nRetVal)
    {
        _gIsUpdateComplete = 0;
        DBG(&g_I2cClient->dev, "Update FAILED\n");
    }
    else
    {
        _gIsUpdateComplete = 1;
        DBG(&g_I2cClient->dev, "Update SUCCESS\n");
    }
    
    DrvEnableFingerTouchReport();

    return nCount;
}

static ssize_t _DrvProcfsCustomerFirmwareVersionRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
    nLength = sprintf(nUserTempBuffer, "%s", _gFwVersion);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}	
    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsCustomerFirmwareVersionWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u16 nMajor = 0, nMinor = 0;

    DrvGetCustomerFirmwareVersion(&nMajor, &nMinor, &_gFwVersion);

    DBG(&g_I2cClient->dev, "*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    return nCount;
}

static ssize_t _DrvProcfsPlatformFirmwareVersionRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() _gPlatformFwVersion = %s ***\n", __func__, _gPlatformFwVersion);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
    nLength = sprintf(nUserTempBuffer, "%s", _gPlatformFwVersion);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}	

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsPlatformFirmwareVersionWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DrvGetPlatformFirmwareVersion(&_gPlatformFwVersion);

    DBG(&g_I2cClient->dev, "*** %s() _gPlatformFwVersion = %s ***\n", __func__, _gPlatformFwVersion);

    return nCount;
}

static ssize_t _DrvProcfsDeviceDriverVersionRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
    nLength = sprintf(nUserTempBuffer, "%s", DEVICE_DRIVER_RELEASE_VERSION);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}	

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsDeviceDriverVersionWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

static ssize_t _DrvProcfsSdCardFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u16 nMajor = 0, nMinor = 0;
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DrvGetCustomerFirmwareVersion(&nMajor, &nMinor, &_gFwVersion);

    DBG(&g_I2cClient->dev, "*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    nLength = sprintf(nUserTempBuffer, "%s\n", _gFwVersion);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}	

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsSdCardFirmwareUpdateWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    char *pValid = NULL;
    char *pTmpFilePath = NULL;
    char szFilePath[100] = {0};
    char *pStr = NULL;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DBG(&g_I2cClient->dev, "pBuffer = %s\n", pBuffer);
	memset(_gDebugBuf, 0, 1024);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	} 
	pStr = _gDebugBuf;
    if (pStr != NULL)
    {
        pValid = strstr(pStr, ".bin");
        
        if (pValid)
        {
            pTmpFilePath = strsep((char **)&pStr, ".");
            
            DBG(&g_I2cClient->dev, "pTmpFilePath = %s\n", pTmpFilePath);

            strcat(szFilePath, pTmpFilePath);
            strcat(szFilePath, ".bin");

            DBG(&g_I2cClient->dev, "szFilePath = %s\n", szFilePath);
            
            if (0 != DrvCheckUpdateFirmwareBySdCard(szFilePath))
            {
                DBG(&g_I2cClient->dev, "Update FAILED\n");
            }
            else
            {
                DBG(&g_I2cClient->dev, "Update SUCCESS\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "The file type of the update firmware bin file is not a .bin file.\n");
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "The file path of the update firmware bin file is NULL.\n");
    }

    return nCount;
}

static ssize_t _DrvProcfsSeLinuxLimitFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    s32 nRetVal = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)    
    {
        DBG(&g_I2cClient->dev, "FIRMWARE_FILE_PATH_ON_SD_CARD = %s\n", FIRMWARE_FILE_PATH_ON_SD_CARD);
        nRetVal = DrvUpdateFirmwareBySdCard(FIRMWARE_FILE_PATH_ON_SD_CARD);
    }
    else
    {
        DBG(&g_I2cClient->dev, "This chip type (0x%x) does not support selinux limit firmware update\n", g_ChipType);
        nRetVal = -1;
    }

    if (0 != nRetVal)
    {
        _gIsUpdateComplete = 0;
        DBG(&g_I2cClient->dev, "Update FAILED\n");
    }
    else
    {
        _gIsUpdateComplete = 1;
        DBG(&g_I2cClient->dev, "Update SUCCESS\n");
    }

    nLength = sprintf(nUserTempBuffer, "%d", _gIsUpdateComplete);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    DBG(&g_I2cClient->dev, "*** _gIsUpdateComplete = %d ***\n", _gIsUpdateComplete);

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsForceFirmwareUpdateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DBG(&g_I2cClient->dev, "*** IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = %d ***\n", IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED);

    IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = 1; // Enable force firmware update
    _gFeatureSupportStatus = IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED;

    nLength = sprintf(nUserTempBuffer, "%d", _gFeatureSupportStatus);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsFirmwareDebugRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i, nLength = 0;
    u8 nBank, nAddr;
    u16 szRegData[MAX_DEBUG_REGISTER_NUM] = {0};
    u8 szOut[MAX_DEBUG_REGISTER_NUM*25] = {0}, szValue[10] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
                
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    
    for (i = 0; i < _gDebugRegCount; i ++)
    {
        szRegData[i] = RegGet16BitValue(_gDebugReg[i]);
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        nBank = (_gDebugReg[i] >> 8) & 0xFF;
        nAddr = _gDebugReg[i] & 0xFF;
    	  
        DBG(&g_I2cClient->dev, "reg(0x%02X,0x%02X)=0x%04X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%02X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%02X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }

    nLength = strlen(szOut);
    if(copy_to_user(pBuffer, szOut, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    *pPos += nLength;
    
    return nLength;
}

static ssize_t _DrvProcfsFirmwareDebugWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 i;
    char *pCh = NULL;
    char *pStr = NULL;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);
	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}  
    if (_gDebugBuf != NULL)
    {
        DBG(&g_I2cClient->dev, "*** pBuffer[0] = %c ***\n", _gDebugBuf[0]);
        DBG(&g_I2cClient->dev, "*** pBuffer[1] = %c ***\n", _gDebugBuf[1]);
        DBG(&g_I2cClient->dev, "*** pBuffer[2] = %c ***\n", _gDebugBuf[2]);
        DBG(&g_I2cClient->dev, "*** pBuffer[3] = %c ***\n", _gDebugBuf[3]);
        DBG(&g_I2cClient->dev, "*** pBuffer[4] = %c ***\n", _gDebugBuf[4]);
        DBG(&g_I2cClient->dev, "*** pBuffer[5] = %c ***\n", _gDebugBuf[5]);

        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);
       

        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;
        
        i = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (i < MAX_DEBUG_REGISTER_NUM))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);
            
            _gDebugReg[i] = _DrvConvertCharToHexDigit(pCh, strlen(pCh));

            DBG(&g_I2cClient->dev, "_gDebugReg[%d] = 0x%04X\n", i, _gDebugReg[i]);
            i ++;
        }
        _gDebugRegCount = i;
        
        DBG(&g_I2cClient->dev, "_gDebugRegCount = %d\n", _gDebugRegCount);
    }

    return nCount;
}

static ssize_t _DrvProcfsFirmwareSetDebugValueRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i, nLength = 0;
    u8 nBank, nAddr;
    u16 szRegData[MAX_DEBUG_REGISTER_NUM] = {0};
    u8 szOut[MAX_DEBUG_REGISTER_NUM*25] = {0}, szValue[10] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    
    for (i = 0; i < _gDebugRegCount; i ++)
    {
        szRegData[i] = RegGet16BitValue(_gDebugReg[i]);
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        nBank = (_gDebugReg[i] >> 8) & 0xFF;
        nAddr = _gDebugReg[i] & 0xFF;
    	  
        DBG(&g_I2cClient->dev, "reg(0x%02X,0x%02X)=0x%04X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%02X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%02X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }
	
    nLength = strlen(szOut);
    if(copy_to_user(pBuffer, szOut, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    *pPos += nLength;
    
    return nLength;
}

static ssize_t _DrvProcfsFirmwareSetDebugValueWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 i, j, k;
    char *pCh = NULL;
    char *pStr = NULL;  

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}
    if (_gDebugBuf != NULL)
    {
        DBG(&g_I2cClient->dev, "*** pBuffer[0] = %c ***\n", _gDebugBuf[0]);
        DBG(&g_I2cClient->dev, "*** pBuffer[1] = %c ***\n", _gDebugBuf[1]);

        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);      
        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;

        i = 0;
        j = 0;
        k = 0;
        
        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 2))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            if ((i%2) == 0)
            {
                _gDebugReg[j] = _DrvConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugReg[%d] = 0x%04X\n", j, _gDebugReg[j]);
                j ++;
            }
            else // (i%2) == 1
            {	
                _gDebugRegValue[k] = _DrvConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugRegValue[%d] = 0x%04X\n", k, _gDebugRegValue[k]);
                k ++;
            }

            i ++;
        }
        _gDebugRegCount = j;
        
        DBG(&g_I2cClient->dev, "_gDebugRegCount = %d\n", _gDebugRegCount);

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
    
        for (i = 0; i < _gDebugRegCount; i ++)
        {
            RegSet16BitValue(_gDebugReg[i], _gDebugRegValue[i]);
            DBG(&g_I2cClient->dev, "_gDebugReg[%d] = 0x%04X, _gDebugRegValue[%d] = 0x%04X\n", i, _gDebugReg[i], i , _gDebugRegValue[i]); // add for debug
        }

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
    
    return nCount;
}

static ssize_t _DrvProcfsFirmwareSmBusDebugRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i = 0, nLength = 0;
    u8 szSmBusRxData[MAX_I2C_TRANSACTION_LENGTH_LIMIT] = {0};
    u8 szOut[MAX_I2C_TRANSACTION_LENGTH_LIMIT*2] = {0};
    u8 szValue[10] = {0};
    s32 rc = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    mutex_lock(&g_Mutex);
    DBG(&g_I2cClient->dev, "*** %s() *** mutex_lock(&g_Mutex)\n", __func__);  // add for debug

    while (i < 5)
    {
        if (_gDebugCmdArguCount > 0) // Send write command
        {
            DBG(&g_I2cClient->dev, "Execute I2C SMBUS write command\n"); 

            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &_gDebugCmdArgu[0], _gDebugCmdArguCount);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "IicWriteData(0x%X, 0x%X, %d) success\n", SLAVE_I2C_ID_DWI2C, _gDebugCmdArgu[0], _gDebugCmdArguCount);
                
                if (_gDebugReadDataSize == 0)
                {
                    break; // No need to execute I2C SMBUS read command later. So, break here.
                }
            }
        }

        if (_gDebugReadDataSize > 0) // Send read command
        {
            DBG(&g_I2cClient->dev, "Execute I2C SMBUS read command\n"); 

            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
            rc = IicReadData(SLAVE_I2C_ID_DWI2C, &szSmBusRxData[0], _gDebugReadDataSize);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "IicReadData(0x%X, 0x%X, %d) success\n", SLAVE_I2C_ID_DWI2C, szSmBusRxData[0], _gDebugReadDataSize);
                break;
            }
        }

        i ++;
    }
    if (i == 5)
    {
        DBG(&g_I2cClient->dev, "IicWriteData() & IicReadData() failed, rc = %d\n", rc);
    }

    for (i = 0; i < _gDebugReadDataSize; i ++) // Output format 2.
    {
        DBG(&g_I2cClient->dev, "szSmBusRxData[%d] = 0x%x\n", i, szSmBusRxData[i]); 

        sprintf(szValue, "%02x", szSmBusRxData[i]);
        strcat(szOut, szValue);
        
        if (i < (_gDebugReadDataSize - 1))
        {
            strcat(szOut, ",");
        }
    }

    DBG(&g_I2cClient->dev, "*** %s() *** mutex_unlock(&g_Mutex)\n", __func__);  // add for debug
    mutex_unlock(&g_Mutex);
    nLength = strlen(szOut);
    if(copy_to_user(pBuffer, szOut, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    *pPos += nLength;
    
    return nLength;
}

static ssize_t _DrvProcfsFirmwareSmBusDebugWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 i, j;
    char szCmdType[5] = {0};
    char *pCh = NULL;
    char *pStr = NULL;  

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}
    if (_gDebugBuf != NULL)
    {
        DBG(&g_I2cClient->dev, "*** pBuffer[0] = %c ***\n", _gDebugBuf[0]);
        DBG(&g_I2cClient->dev, "*** pBuffer[1] = %c ***\n", _gDebugBuf[1]);
        DBG(&g_I2cClient->dev, "*** pBuffer[2] = %c ***\n", _gDebugBuf[2]);
        DBG(&g_I2cClient->dev, "*** pBuffer[3] = %c ***\n", _gDebugBuf[3]);
        DBG(&g_I2cClient->dev, "*** pBuffer[4] = %c ***\n", _gDebugBuf[4]);
        DBG(&g_I2cClient->dev, "*** pBuffer[5] = %c ***\n", _gDebugBuf[5]);

        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);

        // Reset to 0 before parsing the adb command
        _gDebugCmdArguCount = 0;
        _gDebugReadDataSize = 0;
        
        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;

        i = 0;
        j = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (j < MAX_DEBUG_COMMAND_ARGUMENT_NUM))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);
            
            if (strcmp(pCh, "w") == 0 || strcmp(pCh, "r") == 0)
            {
                memcpy(szCmdType, pCh, strlen(pCh));
            }
            else if (strcmp(szCmdType, "w") == 0)
            {
                _gDebugCmdArgu[j] = _DrvConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugCmdArgu[%d] = 0x%02X\n", j, _gDebugCmdArgu[j]);
                
                j ++;
       
                _gDebugCmdArguCount = j;
                DBG(&g_I2cClient->dev, "_gDebugCmdArguCount = %d\n", _gDebugCmdArguCount);
            }
            else if (strcmp(szCmdType, "r") == 0)
            {
                sscanf(pCh, "%d", &_gDebugReadDataSize);   
                DBG(&g_I2cClient->dev, "_gDebugReadDataSize = %d\n", _gDebugReadDataSize);
            }
            else
            {
                DBG(&g_I2cClient->dev, "Un-supported adb command format!\n");
            }

            i ++;
        }
    }

    return nCount;
}

static ssize_t _DrvProcfsFirmwareSetDQMemValueRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i, nLength = 0;
    u8 nBank, nAddr;
    u32 szRegData[MAX_DEBUG_REGISTER_NUM] = {0};
    u8 szOut[MAX_DEBUG_REGISTER_NUM*25] = {0}, szValue[10] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        szRegData[i] = DrvReadDQMemValue(_gDebugReg[i]);
    }

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        nBank = (_gDebugReg[i] >> 8) & 0xFF;
        nAddr = _gDebugReg[i] & 0xFF;

        DBG(&g_I2cClient->dev, "reg(0x%02X,0x%02X)=0x%08X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%02X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%02X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }
    nLength = strlen(szOut);
    if(copy_to_user(pBuffer, szOut, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsFirmwareSetDQMemValueWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 i, j, k;
    char *pCh = NULL;
    char *pStr = NULL;
    u16 nRealDQMemAddr = 0;
    u32 nRealDQMemValue = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}
    if (_gDebugBuf != NULL)
    {
        DBG(&g_I2cClient->dev, "*** pBuffer[0] = %c ***\n", _gDebugBuf[0]);
        DBG(&g_I2cClient->dev, "*** pBuffer[1] = %c ***\n", _gDebugBuf[1]);

        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);
        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;

        i = 0;
        j = 0;
        k = 0;

        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 2))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            if ((i%2) == 0)
            {
                _gDebugReg[j] = _DrvConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugReg[%d] = 0x%04X\n", j, _gDebugReg[j]);
                j ++;
            }
            else // (i%2) == 1
            {
                _gDebugRegValue[k] = _DrvConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "_gDebugRegValue[%d] = 0x%04X\n", k, _gDebugRegValue[k]);
                k ++;
            }

            i ++;
        }
        _gDebugRegCount = j;

        DBG(&g_I2cClient->dev, "_gDebugRegCount = %d\n", _gDebugRegCount);

        if ((_gDebugReg[0] % 4) == 0)
        {
            nRealDQMemAddr = _gDebugReg[0];
            nRealDQMemValue = DrvReadDQMemValue(nRealDQMemAddr);
            _gDebugReg[0] = nRealDQMemAddr;
            DBG(&g_I2cClient->dev, "nRealDQMemValue Raw = %X\n", nRealDQMemValue);
            nRealDQMemValue &= 0xFFFF0000;
            nRealDQMemValue |= _gDebugRegValue[0];
            DBG(&g_I2cClient->dev, "nRealDQMemValue Modify = %X\n", nRealDQMemValue);
            DrvWriteDQMemValue(nRealDQMemAddr, nRealDQMemValue);
        }
        else if ((_gDebugReg[0] % 4) == 2)
        {
            nRealDQMemAddr = _gDebugReg[0] - 2;
            nRealDQMemValue = DrvReadDQMemValue(nRealDQMemAddr);
            _gDebugReg[0] = nRealDQMemAddr;
            DBG(&g_I2cClient->dev, "nRealDQMemValue Raw = %X\n", nRealDQMemValue);

            nRealDQMemValue &= 0x0000FFFF;
            nRealDQMemValue |= (_gDebugRegValue[0] << 16);
            DBG(&g_I2cClient->dev, "nRealDQMemValue Modify = %X\n", nRealDQMemValue);
            DrvWriteDQMemValue(nRealDQMemAddr, nRealDQMemValue);
        }
    }

    return nCount;
}

static void _DrvHandleFingerTouch(void)
{
    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        _DrvSelfHandleFingerTouch();
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        _DrvMutualHandleFingerTouch();
    }
}

#ifdef CONFIG_ENABLE_ITO_MP_TEST
static ssize_t _DrvProcfsMpTestRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DBG(&g_I2cClient->dev, "*** ctp mp test status = %d ***\n", DrvMpTestGetTestResult());

    nLength = sprintf(nUserTempBuffer, "%d", DrvMpTestGetTestResult());
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsMpTestWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 nMode = 0;
    u32 i = 0;
    char *pCh = NULL;
	char *pStr = NULL;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}
	pStr = _gDebugBuf;
    if (pStr != NULL)
    {
        i = 0;
        while ((pCh = strsep((char **)&pStr, ",")) && (i < 1))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            nMode = _DrvConvertCharToHexDigit(pCh, strlen(pCh));

            i ++;
        }

        DBG(&g_I2cClient->dev, "Mp Test Mode = 0x%x\n", nMode);

        if (nMode == ITO_TEST_MODE_OPEN_TEST) //open test
        {
            _gItoTestMode = ITO_TEST_MODE_OPEN_TEST;
            DrvMpTestScheduleMpTestWork(ITO_TEST_MODE_OPEN_TEST);
        }
        else if (nMode == ITO_TEST_MODE_SHORT_TEST) //short test
        {
            _gItoTestMode = ITO_TEST_MODE_SHORT_TEST;
            DrvMpTestScheduleMpTestWork(ITO_TEST_MODE_SHORT_TEST);
        }
        else if (nMode == ITO_TEST_MODE_WATERPROOF_TEST) //waterproof test
        {
            _gItoTestMode = ITO_TEST_MODE_WATERPROOF_TEST;
            DrvMpTestScheduleMpTestWork(ITO_TEST_MODE_WATERPROOF_TEST);
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined MP Test Mode ***\n");
        }
    }

    return nCount;
/*
    u32 nMode = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    if (pBuffer != NULL)
    {
        sscanf(pBuffer, "%x", &nMode);   

        DBG(&g_I2cClient->dev, "Mp Test Mode = 0x%x\n", nMode);

        if (nMode == ITO_TEST_MODE_OPEN_TEST) //open test
        {
            _gItoTestMode = ITO_TEST_MODE_OPEN_TEST;
            DrvMpTestScheduleMpTestWork(ITO_TEST_MODE_OPEN_TEST);
        }
        else if (nMode == ITO_TEST_MODE_SHORT_TEST) //short test
        {
            _gItoTestMode = ITO_TEST_MODE_SHORT_TEST;
            DrvMpTestScheduleMpTestWork(ITO_TEST_MODE_SHORT_TEST);
        }
        else if (nMode == ITO_TEST_MODE_WATERPROOF_TEST) //waterproof test
        {
            _gItoTestMode = ITO_TEST_MODE_WATERPROOF_TEST;
            DrvMpTestScheduleMpTestWork(ITO_TEST_MODE_WATERPROOF_TEST);
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined MP Test Mode ***\n");
        }
    }
    
    return nCount;
*/
}

static ssize_t _DrvProcfsMpTestLogRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}    
    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DrvMpTestGetTestDataLog(_gItoTestMode, _gDebugBuf, &nLength);

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsMpTestLogWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

static ssize_t _DrvProcfsMpTestFailChannelRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    DrvMpTestGetTestFailChannel(_gItoTestMode, pBuffer, &nLength);

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsMpTestFailChannelWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

static ssize_t _DrvProcfsMpTestScopeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    //u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        DrvMpTestGetTestScope(&g_TestScopeInfo);

        nLength = sprintf(nUserTempBuffer, "%d,%d,%d", g_TestScopeInfo.nMx, g_TestScopeInfo.nMy, g_TestScopeInfo.nKeyNum);
		if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
		{
			DBG(&g_I2cClient->dev,"copy to user error\n");
			return -EFAULT;
		}

    }  
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    
    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsMpTestScopeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
static ssize_t _DrvProcfsMpTestLogAllRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength=0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}   

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        DrvMpTestGetTestLogAll(_gDebugBuf, &nLength);
    }

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsMpTestLogAllWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#endif //CONFIG_ENABLE_ITO_MP_TEST

/*--------------------------------------------------------------------------*/

static ssize_t _DrvProcfsFirmwareModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
	u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        _DrvMutualGetFirmwareInfo(&g_MutualFirmwareInfo);
        g_FirmwareMode = g_MutualFirmwareInfo.nFirmwareMode;

        DBG(&g_I2cClient->dev, "%s() firmware mode = 0x%x\n", __func__, g_MutualFirmwareInfo.nFirmwareMode);

        nLength = sprintf(nUserTempBuffer, "%x", g_MutualFirmwareInfo.nFirmwareMode);
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX && IS_SELF_FREQ_SCAN_ENABLED)
    {
        _DrvSelfGetFirmwareInfo(&g_SelfFirmwareInfo);
        g_FirmwareMode = g_SelfFirmwareInfo.nFirmwareMode;
            g_MutualFirmwareInfo.nFirmwareMode = g_FirmwareMode;
            g_MutualFirmwareInfo.nLogModePacketHeader = 0; 
            g_MutualFirmwareInfo.nLogModePacketLength = 0; 
            g_MutualFirmwareInfo.nType = 0;
            g_MutualFirmwareInfo.nMy = 0;
            g_MutualFirmwareInfo.nMx = 0;
            g_MutualFirmwareInfo.nSd = 0;
            g_MutualFirmwareInfo.nSs = 0;
		DBG(&g_I2cClient->dev,"%d,%d,%d,%d\n",g_MutualFirmwareInfo.nMx, g_MutualFirmwareInfo.nMy, g_MutualFirmwareInfo.nSs, g_MutualFirmwareInfo.nSd);
        DBG(&g_I2cClient->dev, "%s() firmware mode = 0x%x, can change firmware mode = %d\n", __func__, g_SelfFirmwareInfo.nFirmwareMode, g_SelfFirmwareInfo.nIsCanChangeFirmwareMode);
        nLength = sprintf(nUserTempBuffer, "%x", g_MutualFirmwareInfo.nFirmwareMode);
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        _DrvSelfGetFirmwareInfo(&g_SelfFirmwareInfo);
        g_FirmwareMode = g_SelfFirmwareInfo.nFirmwareMode;

        DBG(&g_I2cClient->dev, "%s() firmware mode = 0x%x, can change firmware mode = %d\n", __func__, g_SelfFirmwareInfo.nFirmwareMode, g_SelfFirmwareInfo.nIsCanChangeFirmwareMode);

        nLength = sprintf(nUserTempBuffer, "%x,%d", g_SelfFirmwareInfo.nFirmwareMode, g_SelfFirmwareInfo.nIsCanChangeFirmwareMode);
    }
	if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
		return -EFAULT;
	}	
    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsFirmwareModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 nMode;
	memset(_gDebugBuf, 0, 16);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    if (_gDebugBuf != NULL)
    {
        sscanf(_gDebugBuf, "%x", &nMode);   
        DBG(&g_I2cClient->dev, "firmware mode = 0x%x\n", nMode);

        g_IsSwitchModeByAPK = 0;

        if (nMode == FIRMWARE_MODE_DEMO_MODE) //demo mode
        {
            g_FirmwareMode = _DrvChangeFirmwareMode(FIRMWARE_MODE_DEMO_MODE);
        }
        else if (nMode == FIRMWARE_MODE_DEBUG_MODE) //debug mode
        {
            g_FirmwareMode = _DrvChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
            g_IsSwitchModeByAPK = 1;
            _gnDebugLogTimesStamp = 0; // Set _gnDebugLogTimesStamp for filter duplicate packet on MTPTool APK
        }
        else if (nMode == FIRMWARE_MODE_RAW_DATA_MODE) //raw data mode
        {
            if (g_ChipType == CHIP_TYPE_MSG22XX)
            {
                g_FirmwareMode = _DrvChangeFirmwareMode(FIRMWARE_MODE_RAW_DATA_MODE);
                g_IsSwitchModeByAPK = 1;
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined Firmware Mode ***\n");
        }
    }

    DBG(&g_I2cClient->dev, "*** g_FirmwareMode = 0x%x ***\n", g_FirmwareMode);
    
    return nCount;
}

static ssize_t _DrvProcfsFirmwareSensorRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        if (g_MutualFirmwareInfo.nLogModePacketHeader == 0xA7)
        {
            nLength = sprintf(nUserTempBuffer, "%d,%d,%d,%d", g_MutualFirmwareInfo.nMx, g_MutualFirmwareInfo.nMy, g_MutualFirmwareInfo.nSs, g_MutualFirmwareInfo.nSd);
        }
        else
        {
            DBG(&g_I2cClient->dev, "Undefined debug mode packet format : 0x%x\n", g_MutualFirmwareInfo.nLogModePacketHeader);
            nLength = 0;
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        nLength = sprintf(nUserTempBuffer, "%d", g_SelfFirmwareInfo.nLogModePacketLength);
    }
	if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
		return -EFAULT;
	}    
    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsFirmwareSensorWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

static ssize_t _DrvProcfsFirmwarePacketHeaderRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        nLength = sprintf(nUserTempBuffer, "%d", g_MutualFirmwareInfo.nLogModePacketHeader);

    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        nLength = sprintf(nUserTempBuffer, "%d", g_SelfFirmwareInfo.nLogModePacketHeader);
    }    
	if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
		return -EFAULT;
	}	
    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsFirmwarePacketHeaderWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return nCount;
}

static ssize_t _DrvKObjectPacketShow(struct kobject *pKObj, struct kobj_attribute *pAttr, char *pBuf)
{
    u32 nLength = 0;
	u32 i = 0;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (strcmp(pAttr->attr.name, "packet") == 0)
    {
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            if (g_DemoModePacket != NULL)
            {
                DBG(&g_I2cClient->dev, "g_FirmwareMode=%x, g_DemoModePacket[0]=%x, g_DemoModePacket[1]=%x\n", g_FirmwareMode, g_DemoModePacket[0], g_DemoModePacket[1]);
                DBG(&g_I2cClient->dev, "g_DemoModePacket[2]=%x, g_DemoModePacket[3]=%x\n", g_DemoModePacket[2], g_DemoModePacket[3]);
                DBG(&g_I2cClient->dev, "g_DemoModePacket[4]=%x, g_DemoModePacket[5]=%x\n", g_DemoModePacket[4], g_DemoModePacket[5]);
                
                memcpy(pBuf, g_DemoModePacket, DEMO_MODE_PACKET_LENGTH);

                nLength = DEMO_MODE_PACKET_LENGTH;
                DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);
            }
            else
            {
                DBG(&g_I2cClient->dev, "g_DemoModePacket is NULL\n");
            }
        }
        else //g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE || g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE
        {
            if (g_LogModePacket != NULL)
            {
                DBG(&g_I2cClient->dev, "g_FirmwareMode=%x, g_LogModePacket[0]=%x, g_LogModePacket[1]=%x\n", g_FirmwareMode, g_LogModePacket[0], g_LogModePacket[1]);
                DBG(&g_I2cClient->dev, "g_LogModePacket[2]=%x, g_LogModePacket[3]=%x\n", g_LogModePacket[2], g_LogModePacket[3]);
                DBG(&g_I2cClient->dev, "g_LogModePacket[4]=%x, g_LogModePacket[5]=%x\n", g_LogModePacket[4], g_LogModePacket[5]);

                if ((g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A) && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE) && (g_LogModePacket[0] == 0xA7))
                {
                    memcpy(pBuf, g_LogModePacket, g_MutualFirmwareInfo.nLogModePacketLength);

                    if (_gnDebugLogTimesStamp >= 255) 
                    {
                        _gnDebugLogTimesStamp = 0;
                    }
                    else
                    {
                        _gnDebugLogTimesStamp ++;    
                    }
                    
                    pBuf[g_MutualFirmwareInfo.nLogModePacketLength] = _gnDebugLogTimesStamp;
                    DBG(&g_I2cClient->dev, "_gnDebugLogTimesStamp=%d\n", pBuf[g_MutualFirmwareInfo.nLogModePacketLength]); // TODO : add for debug

                    nLength = g_MutualFirmwareInfo.nLogModePacketLength + 1;
                    DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);
                }
                else if ((g_ChipType == CHIP_TYPE_MSG22XX) && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE || g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE) && (g_LogModePacket[0] == 0x62))
                {
                    memcpy(pBuf, g_LogModePacket, g_SelfFirmwareInfo.nLogModePacketLength);

                    nLength = g_SelfFirmwareInfo.nLogModePacketLength;
                    DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);
                }
                else if ((g_ChipType == CHIP_TYPE_MSG22XX) && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE) && IS_SELF_FREQ_SCAN_ENABLED) //&& (g_LogModePacket[0] == 0xA7)
                {
					for(i = 0; i < SELF_FREQ_SCAN_PACKET_LENGTH+30; i++)
					{
						if(i < 5)
						{
							pBuf[i] = g_LogModePacket[i];
						}
						else if(i < (5 + 30))
						{
							pBuf[i] = 0;
						}
						else
						{
							pBuf[i] = g_LogModePacket[i-30];
						}
						printk("0x%x,", pBuf[i]);
					}
					printk("\n");
                    //memcpy(pBuf, g_LogModePacket, SELF_FREQ_SCAN_PACKET_LENGTH);

                    nLength = SELF_FREQ_SCAN_PACKET_LENGTH;
                    DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);
                }
                else
                {
                    DBG(&g_I2cClient->dev, "CURRENT MODE IS NOT DEBUG MODE/WRONG DEBUG MODE HEADER\n");
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "g_LogModePacket is NULL\n");
            }
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "pAttr->attr.name = %s \n", pAttr->attr.name);
    }

    return nLength;
}

static ssize_t _DrvKObjectPacketStore(struct kobject *pKObj, struct kobj_attribute *pAttr, const char *pBuf, size_t nCount)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
/*
    if (strcmp(pAttr->attr.name, "packet") == 0)
    {

    }
*/    	
    return nCount;
}

static struct kobj_attribute packet_attr = __ATTR(packet, 0664, _DrvKObjectPacketShow, _DrvKObjectPacketStore);

/* Create a group of attributes so that we can create and destroy them all at once. */
static struct attribute *attrs[] = {
    &packet_attr.attr,
    NULL,	/* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory. If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
static struct attribute_group attr_group = {
    .attrs = attrs,
};


//------------------------------------------------------------------------------//

static ssize_t _DrvProcfsQueryFeatureSupportStatusRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    nLength = sprintf(nUserTempBuffer, "%d", _gFeatureSupportStatus);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsQueryFeatureSupportStatusWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 nFeature;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);
	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}      
    if (_gDebugBuf != NULL)
    {
        sscanf(_gDebugBuf, "%x", &nFeature);   
        DBG(&g_I2cClient->dev, "nFeature = 0x%x\n", nFeature);

        if (nFeature == FEATURE_GESTURE_WAKEUP_MODE) 
        {
            _gFeatureSupportStatus = IS_GESTURE_WAKEUP_ENABLED;
        }
        else if (nFeature == FEATURE_GESTURE_DEBUG_MODE) 
        {
            _gFeatureSupportStatus = IS_GESTURE_DEBUG_MODE_ENABLED;
        }
        else if (nFeature == FEATURE_GESTURE_INFORMATION_MODE) 
        {
            _gFeatureSupportStatus = IS_GESTURE_INFORMATION_MODE_ENABLED;
        }
        else if (nFeature == FEATURE_TOUCH_DRIVER_DEBUG_LOG)
        {
            _gFeatureSupportStatus = TOUCH_DRIVER_DEBUG_LOG_LEVEL;
        }
        else if (nFeature == FEATURE_FIRMWARE_DATA_LOG)
        {
            _gFeatureSupportStatus = IS_FIRMWARE_DATA_LOG_ENABLED;
            
#ifdef CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
            if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A|| g_ChipType == CHIP_TYPE_MSG22XX)
            {
                if (_gFeatureSupportStatus == 1) // If the debug mode data log function is supported, then get packet address and flag address for segment read finger touch data.
                {
                    _DrvGetTouchPacketAddress(&g_FwPacketDataAddress, &g_FwPacketFlagAddress);
                }
            }
#endif //CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA
        }
        else if (nFeature == FEATURE_FORCE_TO_UPDATE_FIRMWARE)
        {
            _gFeatureSupportStatus = IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED;
        }
        else if (nFeature == FEATURE_DISABLE_ESD_PROTECTION_CHECK)
        {
            _gFeatureSupportStatus = IS_DISABLE_ESD_PROTECTION_CHECK;
        }
        else if (nFeature == FEATURE_APK_PRINT_FIRMWARE_SPECIFIC_LOG)
        {
            _gFeatureSupportStatus = IS_APK_PRINT_FIRMWARE_SPECIFIC_LOG_ENABLED;
        }
        else if (nFeature == FEATURE_SELF_FREQ_SCAN)
        {
			DBG(&g_I2cClient->dev, "*** change to  FEATURE_SELF_FREQ_SCAN ***\n");
            _gFeatureSupportStatus = IS_SELF_FREQ_SCAN_ENABLED;
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined Feature ***\n");
        }
    }

    DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);
    
    return nCount;
}

static ssize_t _DrvProcfsChangeFeatureSupportStatusRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    nLength = sprintf(nUserTempBuffer, "%d", _gFeatureSupportStatus);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}	

    DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsChangeFeatureSupportStatusWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 i;
    u32 nFeature = 0, nNewValue = 0;
    char *pCh = NULL;
    char *pStr = NULL;  

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}
    if (_gDebugBuf != NULL)
    {
        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);       
        _gDebugBuf[nCount] = '\0';
        pStr = _gDebugBuf;
        
        i = 0;
        
        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 3))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            if (i == 0)
            {
                nFeature = _DrvConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "nFeature = 0x%04X\n", nFeature);
            }
            else if (i == 1)
            {	
                nNewValue = _DrvConvertCharToHexDigit(pCh, strlen(pCh));
                DBG(&g_I2cClient->dev, "nNewValue = %d\n", nNewValue);
            }
            else
            {
                DBG(&g_I2cClient->dev, "End of parsing adb command.\n");
            }

            i ++;
        }
        if (nFeature == FEATURE_GESTURE_WAKEUP_MODE) 
        {
            IS_GESTURE_WAKEUP_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_GESTURE_WAKEUP_ENABLED;
        }
        else if (nFeature == FEATURE_GESTURE_DEBUG_MODE) 
        {
            IS_GESTURE_DEBUG_MODE_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_GESTURE_DEBUG_MODE_ENABLED;
        }
        else if (nFeature == FEATURE_GESTURE_INFORMATION_MODE) 
        {
            IS_GESTURE_INFORMATION_MODE_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_GESTURE_INFORMATION_MODE_ENABLED;
        }
        else if (nFeature == FEATURE_TOUCH_DRIVER_DEBUG_LOG)
        {
            TOUCH_DRIVER_DEBUG_LOG_LEVEL = nNewValue;
            _gFeatureSupportStatus = TOUCH_DRIVER_DEBUG_LOG_LEVEL;
        }
        else if (nFeature == FEATURE_FIRMWARE_DATA_LOG)
        {
            IS_FIRMWARE_DATA_LOG_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_FIRMWARE_DATA_LOG_ENABLED;
        }
        else if (nFeature == FEATURE_FORCE_TO_UPDATE_FIRMWARE)
        {
            IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED;
        }
        else if (nFeature == FEATURE_DISABLE_ESD_PROTECTION_CHECK)
        {
            IS_DISABLE_ESD_PROTECTION_CHECK = nNewValue;
            _gFeatureSupportStatus = IS_DISABLE_ESD_PROTECTION_CHECK;
        }
        else if (nFeature == FEATURE_APK_PRINT_FIRMWARE_SPECIFIC_LOG)
        {
            IS_APK_PRINT_FIRMWARE_SPECIFIC_LOG_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_APK_PRINT_FIRMWARE_SPECIFIC_LOG_ENABLED;
        }
        else if (nFeature == FEATURE_SELF_FREQ_SCAN)
        {
            IS_SELF_FREQ_SCAN_ENABLED = nNewValue;
            _gFeatureSupportStatus = IS_SELF_FREQ_SCAN_ENABLED;
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined Feature ***\n");
        }

        DBG(&g_I2cClient->dev, "*** _gFeatureSupportStatus = %d ***\n", _gFeatureSupportStatus);
    }
    
    return nCount;
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
static ssize_t _DrvProcfsGestureWakeupModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
    DBG(&g_I2cClient->dev, "g_GestureWakeupMode = 0x%x, 0x%x\n", g_GestureWakeupMode[0], g_GestureWakeupMode[1]);

    nLength = sprintf(nUserTempBuffer, "%x,%x", g_GestureWakeupMode[0], g_GestureWakeupMode[1]);
#else
    DBG(&g_I2cClient->dev, "g_GestureWakeupMode = 0x%x\n", g_GestureWakeupMode[0]);

    nLength = sprintf(nUserTempBuffer, "%x", g_GestureWakeupMode[0]);
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}	

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsGestureWakeupModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 nLength;
    u32 nWakeupMode[2] = {0};
	char *pStr = NULL;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);
	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}  
	pStr = _gDebugBuf;
    if (pStr != NULL)
    {
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
        u32 i;
        char *pCh;

        i = 0;
        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 2))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            nWakeupMode[i] = _DrvConvertCharToHexDigit(pCh, strlen(pCh));

            DBG(&g_I2cClient->dev, "nWakeupMode[%d] = 0x%04X\n", i, nWakeupMode[i]);
            i ++;
        }
#else
        sscanf(_gDebugBuf, "%x", &nWakeupMode[0]);
        DBG(&g_I2cClient->dev, "nWakeupMode = 0x%x\n", nWakeupMode[0]);
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

        nLength = nCount;
        DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG) == GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG) == GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG) == GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG) == GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG) == GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE1_FLAG) == GESTURE_WAKEUP_MODE_RESERVE1_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE1_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE1_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE2_FLAG) == GESTURE_WAKEUP_MODE_RESERVE2_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE2_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE2_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE3_FLAG) == GESTURE_WAKEUP_MODE_RESERVE3_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE3_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE3_FLAG);
        }

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE4_FLAG) == GESTURE_WAKEUP_MODE_RESERVE4_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE4_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE4_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE5_FLAG) == GESTURE_WAKEUP_MODE_RESERVE5_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE5_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE5_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE6_FLAG) == GESTURE_WAKEUP_MODE_RESERVE6_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE6_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE6_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE7_FLAG) == GESTURE_WAKEUP_MODE_RESERVE7_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE7_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE7_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE8_FLAG) == GESTURE_WAKEUP_MODE_RESERVE8_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE8_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE8_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE9_FLAG) == GESTURE_WAKEUP_MODE_RESERVE9_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE9_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE9_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE10_FLAG) == GESTURE_WAKEUP_MODE_RESERVE10_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE10_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE10_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE11_FLAG) == GESTURE_WAKEUP_MODE_RESERVE11_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE11_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE11_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE12_FLAG) == GESTURE_WAKEUP_MODE_RESERVE12_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE12_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE12_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE13_FLAG) == GESTURE_WAKEUP_MODE_RESERVE13_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE13_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE13_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE14_FLAG) == GESTURE_WAKEUP_MODE_RESERVE14_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE14_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE14_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE15_FLAG) == GESTURE_WAKEUP_MODE_RESERVE15_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE15_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE15_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE16_FLAG) == GESTURE_WAKEUP_MODE_RESERVE16_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE16_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE16_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE17_FLAG) == GESTURE_WAKEUP_MODE_RESERVE17_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE17_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE17_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE18_FLAG) == GESTURE_WAKEUP_MODE_RESERVE18_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE18_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE18_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE19_FLAG) == GESTURE_WAKEUP_MODE_RESERVE19_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE19_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE19_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE20_FLAG) == GESTURE_WAKEUP_MODE_RESERVE20_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE20_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE20_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE21_FLAG) == GESTURE_WAKEUP_MODE_RESERVE21_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE21_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE21_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE22_FLAG) == GESTURE_WAKEUP_MODE_RESERVE22_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE22_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE22_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE23_FLAG) == GESTURE_WAKEUP_MODE_RESERVE23_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE23_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE23_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE24_FLAG) == GESTURE_WAKEUP_MODE_RESERVE24_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE24_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE24_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE25_FLAG) == GESTURE_WAKEUP_MODE_RESERVE25_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE25_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE25_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE26_FLAG) == GESTURE_WAKEUP_MODE_RESERVE26_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE26_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE26_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE27_FLAG) == GESTURE_WAKEUP_MODE_RESERVE27_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE27_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE27_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE28_FLAG) == GESTURE_WAKEUP_MODE_RESERVE28_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE28_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE28_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE29_FLAG) == GESTURE_WAKEUP_MODE_RESERVE29_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE29_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE29_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE30_FLAG) == GESTURE_WAKEUP_MODE_RESERVE30_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE30_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE30_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE31_FLAG) == GESTURE_WAKEUP_MODE_RESERVE31_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE31_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE31_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE32_FLAG) == GESTURE_WAKEUP_MODE_RESERVE32_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE32_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE32_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE33_FLAG) == GESTURE_WAKEUP_MODE_RESERVE33_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE33_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE33_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE34_FLAG) == GESTURE_WAKEUP_MODE_RESERVE34_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE34_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE34_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE35_FLAG) == GESTURE_WAKEUP_MODE_RESERVE35_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE35_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE35_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE36_FLAG) == GESTURE_WAKEUP_MODE_RESERVE36_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE36_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE36_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE37_FLAG) == GESTURE_WAKEUP_MODE_RESERVE37_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE37_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE37_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE38_FLAG) == GESTURE_WAKEUP_MODE_RESERVE38_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE38_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE38_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE39_FLAG) == GESTURE_WAKEUP_MODE_RESERVE39_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE39_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE39_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE40_FLAG) == GESTURE_WAKEUP_MODE_RESERVE40_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE40_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE40_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE41_FLAG) == GESTURE_WAKEUP_MODE_RESERVE41_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE41_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE41_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE42_FLAG) == GESTURE_WAKEUP_MODE_RESERVE42_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE42_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE42_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE43_FLAG) == GESTURE_WAKEUP_MODE_RESERVE43_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE43_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE43_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE44_FLAG) == GESTURE_WAKEUP_MODE_RESERVE44_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE44_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE44_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE45_FLAG) == GESTURE_WAKEUP_MODE_RESERVE45_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE45_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE45_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE46_FLAG) == GESTURE_WAKEUP_MODE_RESERVE46_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE46_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE46_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE47_FLAG) == GESTURE_WAKEUP_MODE_RESERVE47_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE47_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE47_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE48_FLAG) == GESTURE_WAKEUP_MODE_RESERVE48_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE48_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE48_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE49_FLAG) == GESTURE_WAKEUP_MODE_RESERVE49_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE49_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE49_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE50_FLAG) == GESTURE_WAKEUP_MODE_RESERVE50_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE50_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE50_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE51_FLAG) == GESTURE_WAKEUP_MODE_RESERVE51_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE51_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE51_FLAG);
        }
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

        DBG(&g_I2cClient->dev, "g_GestureWakeupMode = 0x%x,  0x%x\n", g_GestureWakeupMode[0], g_GestureWakeupMode[1]);
    }
       
    return nCount;
}

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
static ssize_t _DrvProcfsGestureDebugModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }
    
    DBG(&g_I2cClient->dev, "g_GestureDebugMode = 0x%x\n", g_GestureDebugMode); // add for debug

    nLength = sprintf(nUserTempBuffer, "%d", g_GestureDebugMode);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsGestureDebugModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u8 ucGestureMode[2];
    u8 i;
    char *pCh;
	char *pStr = NULL;
	memset(_gDebugBuf, 0, 1024);
	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	} 
	pStr = _gDebugBuf;
    if (pStr != NULL)
    {
        i = 0;
        while ((pCh = strsep((char **)&pStr, " ,")) && (i < 2))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            ucGestureMode[i] = _DrvConvertCharToHexDigit(pCh, strlen(pCh));

            DBG(&g_I2cClient->dev, "ucGestureMode[%d] = 0x%04X\n", i, ucGestureMode[i]);
            i ++;
        }

        g_GestureDebugMode = ucGestureMode[0];
        g_GestureDebugFlag = ucGestureMode[1];

        DBG(&g_I2cClient->dev, "Gesture flag = 0x%x\n", g_GestureDebugFlag);

        if (g_GestureDebugMode == 0x01) //open gesture debug mode
        {
            DrvOpenGestureDebugMode(g_GestureDebugFlag);

//            input_report_key(g_InputDevice, RESERVER42, 1);
            input_report_key(g_InputDevice, KEY_POWER, 1);
            input_sync(g_InputDevice);
//            input_report_key(g_InputDevice, RESERVER42, 0);
            input_report_key(g_InputDevice, KEY_POWER, 0);
            input_sync(g_InputDevice);
        }
        else if (g_GestureDebugMode == 0x00) //close gesture debug mode
        {
            DrvCloseGestureDebugMode();
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined Gesture Debug Mode ***\n");
        }
    }

    return nCount;
}

static ssize_t _DrvKObjectGestureDebugShow(struct kobject *pKObj, struct kobj_attribute *pAttr, char *pBuf)
{
    u32 i = 0;
    u32 nLength = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (strcmp(pAttr->attr.name, "gesture_debug") == 0)
    {
        if (g_LogGestureDebug != NULL)
        {
            DBG(&g_I2cClient->dev, "g_LogGestureDebug[0]=%x, g_LogGestureDebug[1]=%x\n", g_LogGestureDebug[0], g_LogGestureDebug[1]);
            DBG(&g_I2cClient->dev, "g_LogGestureDebug[2]=%x, g_LogGestureDebug[3]=%x\n", g_LogGestureDebug[2], g_LogGestureDebug[3]);
            DBG(&g_I2cClient->dev, "g_LogGestureDebug[4]=%x, g_LogGestureDebug[5]=%x\n", g_LogGestureDebug[4], g_LogGestureDebug[5]);

            if (g_LogGestureDebug[0] == 0xA7 && g_LogGestureDebug[3] == 0x51)
            {
                for (i = 0; i < 0x80; i ++)
                {
                    pBuf[i] = g_LogGestureDebug[i];
                }

                nLength = 0x80;
                DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);
            }
            else
            {
                DBG(&g_I2cClient->dev, "CURRENT MODE IS NOT GESTURE DEBUG MODE/WRONG GESTURE DEBUG MODE HEADER\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "g_LogGestureDebug is NULL\n");
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "pAttr->attr.name = %s \n", pAttr->attr.name);
    }

    return nLength;
}

static ssize_t _DrvKObjectGestureDebugStore(struct kobject *pKObj, struct kobj_attribute *pAttr, const char *pBuf, size_t nCount)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
/*
    if (strcmp(pAttr->attr.name, "gesture_debug") == 0)
    {

    }
*/
    return nCount;
}

static struct kobj_attribute gesture_attr = __ATTR(gesture_debug, 0664, _DrvKObjectGestureDebugShow, _DrvKObjectGestureDebugStore);

/* Create a group of attributes so that we can create and destroy them all at once. */
static struct attribute *gestureattrs[] = {
    &gesture_attr.attr,
    NULL,	/* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory. If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
struct attribute_group gestureattr_group = {
    .attrs = gestureattrs,
};
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static ssize_t _DrvProcfsGestureInforModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u8 szOut[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH*5] = {0}, szValue[10] = {0};
    u32 szLogGestureInfo[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH] = {0};
    u32 i = 0;
    u32 nLength = 0;
	u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    _gLogGestureCount = 0;
    if (_gLogGestureInforType == FIRMWARE_GESTURE_INFORMATION_MODE_A) //FIRMWARE_GESTURE_INFORMATION_MODE_A
    {
        for (i = 0; i < 2; i ++)//0 EventFlag; 1 RecordNum
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[4 + i];
            _gLogGestureCount ++;
        }

        for (i = 2; i < 8; i ++)//2~3 Xst Yst; 4~5 Xend Yend; 6~7 char_width char_height
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[4 + i];
            _gLogGestureCount ++;
        }
    }
    else if (_gLogGestureInforType == FIRMWARE_GESTURE_INFORMATION_MODE_B) //FIRMWARE_GESTURE_INFORMATION_MODE_B
    {
        for (i = 0; i < 2; i ++)//0 EventFlag; 1 RecordNum
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[4 + i];
            _gLogGestureCount ++;
        }

        for (i = 0; i < g_LogGestureInfor[5]*2 ; i ++)//(X and Y)*RecordNum
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[12 + i];
            _gLogGestureCount ++;
        }
    }
    else if (_gLogGestureInforType == FIRMWARE_GESTURE_INFORMATION_MODE_C) //FIRMWARE_GESTURE_INFORMATION_MODE_C
    {
        for (i = 0; i < 6; i ++)//header
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[i];
            _gLogGestureCount ++;
        }

        for (i = 6; i < 86; i ++)
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[i];
            _gLogGestureCount ++;
        }

        szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[86];//dummy
        _gLogGestureCount ++;
        szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[87];//checksum
        _gLogGestureCount++;
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** Undefined GESTURE INFORMATION MODE ***\n");
    }

    for (i = 0; i < _gLogGestureCount; i ++)
    {
        sprintf(szValue, "%d", szLogGestureInfo[i]);
        strcat(szOut, szValue);
        strcat(szOut, ",");
    }

    nLength = strlen(szOut);
	
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsGestureInforModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    u32 nMode;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 16);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}
    if (_gDebugBuf != NULL)
    {
        sscanf(_gDebugBuf, "%x", &nMode);
        _gLogGestureInforType = nMode;
    }

    DBG(&g_I2cClient->dev, "*** _gLogGestureInforType type = 0x%x ***\n", _gLogGestureInforType);

    return nCount;
}
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
static ssize_t _DrvProcfsReportRateRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    struct timeval tEndTime;
    suseconds_t nStartTime, nEndTime, nElapsedTime;
    u32 nLength = 0;
	u8 nUserTempBuffer[16]; 
	
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DBG(&g_I2cClient->dev, "g_InterruptCount = %d, g_ValidTouchCount = %d\n", g_InterruptCount, g_ValidTouchCount);
    
    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    // Get end time
    do_gettimeofday(&tEndTime);
    
    nStartTime = g_StartTime.tv_sec + g_StartTime.tv_usec/1000000;
    nEndTime = tEndTime.tv_sec + tEndTime.tv_usec/1000000;

    nElapsedTime = nEndTime - nStartTime;

    DBG(&g_I2cClient->dev, "Start time : %lu sec, %lu msec\n", g_StartTime.tv_sec,  g_StartTime.tv_usec); 
    DBG(&g_I2cClient->dev, "End time : %lu sec, %lu msec\n", tEndTime.tv_sec, tEndTime.tv_usec); 

    DBG(&g_I2cClient->dev, "Elapsed time : %lu sec\n", nElapsedTime); 
    
    // Calculate report rate
    if (nElapsedTime != 0)
    {
        g_InterruptReportRate = g_InterruptCount / nElapsedTime;
        g_ValidTouchReportRate = g_ValidTouchCount / nElapsedTime;
    }
    else
    {
        g_InterruptReportRate = 0;		
        g_ValidTouchReportRate = 0;		
    }

    DBG(&g_I2cClient->dev, "g_InterruptReportRate = %d, g_ValidTouchReportRate = %d\n", g_InterruptReportRate, g_ValidTouchReportRate);

    g_InterruptCount = 0; // Reset count
    g_ValidTouchCount = 0;

    nLength = sprintf(nUserTempBuffer, "%d,%d", g_InterruptReportRate, g_ValidTouchReportRate);
    if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
	{
		DBG(&g_I2cClient->dev,"copy to user error\n");
        return -EFAULT;
	}	

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsReportRateWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 16);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}    
    if (_gDebugBuf != NULL)
    {
        sscanf(_gDebugBuf, "%d", &g_IsEnableReportRate);   

        DBG(&g_I2cClient->dev, "g_IsEnableReportRate = %d\n", g_IsEnableReportRate); // 1 : enable report rate calculation, 0 : disable report rate calculation, 2 : reset count

        g_InterruptCount = 0; // Reset count
        g_ValidTouchCount = 0;
    }
    
    return nCount;
}
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

/*--------------------------------------------------------------------------*/

static ssize_t _DrvProcfsGloveModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nGloveMode = 0;
	u8 nUserTempBuffer[16]; 
	
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        DrvDisableFingerTouchReport();

        _DrvGetGloveInfo(&nGloveMode);

        DrvEnableFingerTouchReport();

        DBG(&g_I2cClient->dev, "Glove Mode = 0x%x\n", nGloveMode);

        nLength = sprintf(nUserTempBuffer, "%x", nGloveMode);
		if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
		{
			DBG(&g_I2cClient->dev,"copy to user error\n");
			return -EFAULT;
		}	

    }

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsGloveModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nGloveMode = 0;
    u32 i = 0;
    char *pCh = NULL;
	char *pStr = NULL;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 16);
	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	} 
	pStr = _gDebugBuf;
    if (_gDebugBuf != NULL)
    {
        i = 0;
        while ((pCh = strsep((char **)&pStr, ",")) && (i < 1))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            nGloveMode = _DrvConvertCharToHexDigit(pCh, strlen(pCh));

            i ++;
        }

        DBG(&g_I2cClient->dev, "Glove Mode = 0x%x\n", nGloveMode);

        DrvDisableFingerTouchReport();

        if (nGloveMode == 0x01) //open glove mode
        {
            _DrvOpenGloveMode();
        }
        else if (nGloveMode == 0x00) //close glove mode
        {
            _DrvCloseGloveMode();
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined Glove Mode ***\n");
        }
        DBG(&g_I2cClient->dev, "g_IsEnableGloveMode = 0x%x\n", g_IsEnableGloveMode);

        DrvEnableFingerTouchReport();
    }

    return nCount;
}

static ssize_t _DrvProcfsOpenGloveModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        DrvDisableFingerTouchReport();

        _DrvOpenGloveMode();

        DrvEnableFingerTouchReport();
    }
    DBG(&g_I2cClient->dev, "g_IsEnableGloveMode = 0x%x\n", g_IsEnableGloveMode);

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsCloseGloveModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        DrvDisableFingerTouchReport();

        _DrvCloseGloveMode();

        DrvEnableFingerTouchReport();
    }
    DBG(&g_I2cClient->dev, "g_IsEnableGloveMode = 0x%x\n", g_IsEnableGloveMode);

    *pPos += nLength;

    return nLength;
}

/*--------------------------------------------------------------------------*/

static ssize_t _DrvProcfsLeatherSheathModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLength = 0;
    u8 nLeatherSheathMode = 0;
	u8 nUserTempBuffer[16]; 
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // If file position is non-zero, then assume the string has been read and indicate there is no more data to be read.
    if (*pPos != 0)
    {
        return 0;
    }

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        DrvDisableFingerTouchReport();

        _DrvGetLeatherSheathInfo(&nLeatherSheathMode);

        DrvEnableFingerTouchReport();

        DBG(&g_I2cClient->dev, "Leather Sheath Mode = 0x%x\n", nLeatherSheathMode);

        nLength = sprintf(nUserTempBuffer, "%x", nLeatherSheathMode);
		if(copy_to_user(pBuffer, nUserTempBuffer, nLength))
		{
			DBG(&g_I2cClient->dev,"copy to user error\n");
			return -EFAULT;
		}

    }

    *pPos += nLength;

    return nLength;
}

static ssize_t _DrvProcfsLeatherSheathModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)
{
    u32 nLeatherSheathMode = 0;
    u32 i = 0;
    char *pCh = NULL;
	char *pStr = NULL;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 16);
	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}    
	pStr = _gDebugBuf;
    if (pStr != NULL)
    {
        i = 0;
        while ((pCh = strsep((char **)&pStr, ",")) && (i < 1))
        {
            DBG(&g_I2cClient->dev, "pCh = %s\n", pCh);

            nLeatherSheathMode = _DrvConvertCharToHexDigit(pCh, strlen(pCh));

            i ++;
        }

        DBG(&g_I2cClient->dev, "Leather Sheath Mode = 0x%x\n", nLeatherSheathMode);

        DrvDisableFingerTouchReport();

        if (nLeatherSheathMode == 0x01) //open leather sheath mode
        {
            _DrvOpenLeatherSheathMode();
        }
        else if (nLeatherSheathMode == 0x00) //close leather sheath mode
        {
            _DrvCloseLeatherSheathMode();
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** Undefined Leather Sheath Mode ***\n");
        }
        DBG(&g_I2cClient->dev, "g_IsEnableLeatherSheathMode = 0x%x\n", g_IsEnableLeatherSheathMode);

        DrvEnableFingerTouchReport();
    }

    return nCount;
}
static ssize_t _DrvProcfsTrimCodeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)
{
	
    return nCount;
}
void WriteTrimcode(u8 *uArrayData, u32 nAddr)
{
	u8 * nBuf;
	nBuf = uArrayData;
    DrvDisableFingerTouchReport();
	DbBusEnterSerialDebugMode();
	DbBusStopMCU();
	DbBusIICUseBus();
	DbBusIICReshape();
	nBuf[0] = ((nBuf[0] & 0x01) << 7) + ((nBuf[0] & 0x7F) >> 1);  
	//DEBUG("*** set Write *** 0x%x 0x%x\n", nBuf[0],nBuf[1]);
	_DrvMsg28xxSetProtectBit();

// set write done
//RegSetLByteValue 0x1606 0x01	
    // Set Password
    RegSetLByteValue(0x1616,0xAA);
    RegSetLByteValue(0x1617,0x55);
    RegSetLByteValue(0x1618,0xA5);
    RegSetLByteValue(0x1619,0x5A);

    // disable cpu read, initial read
    RegSetLByteValue(0x1608,0x20);
    RegSetLByteValue(0x1606,0x20);

    // set info block
    RegSetLByteValue(0x1606,0x40);
	RegSetLByteValue(0x1607,0x00);

    // set info double buffer
    RegSetLByteValue(0x1610,0x00);

	// data align
	RegSetLByteValue(0x1640,0x01);
	
	//set info block
	RegSetLByteValue(0x1607,0x08);
	//set info double buffer
	RegSetLByteValue(0x1604,0x01);
	// eflash mode trigger
	RegSetLByteValue(0x1606,0x41);
	// set initial data
	RegSetLByteValue(0x1602,0xA5);
	RegSetLByteValue(0x1602,0x5A);
	RegSetLByteValue(0x1602,nBuf[1]);
	RegSetLByteValue(0x1602,nBuf[0]);
	// set initial address (for latch SA, CA)
	RegSetLByteValue(0x1600,0x00);
	RegSetLByteValue(0x1601,0x00);

	// set initial address (for latch PA)
	RegSetLByteValue(0x1600,0x00);
	RegSetLByteValue(0x1601,0x00);
	// set write done
	RegSetLByteValue(0x1606,0x84);
	DbBusIICNotUseBus();
	DbBusNotStopMCU();
	DbBusExitSerialDebugMode();
    mdelay(100);
    DrvEnableFingerTouchReport();
	DrvTouchDeviceHwReset();
    //DisableBypassHotknot();
	kfree(nBuf);
}
static int ReadTrimcode(u16 nAddr, u16 nLength)
{
	u8 tx_data[4] = {0};
	u8 rx_data[20] = {0};
	u8 * pBuf;
	u8 result;
    DBG(&g_I2cClient->dev,"*** %s() ***\n", __func__);
	pBuf = (u8*)kmalloc(nLength, GFP_KERNEL);  
    DrvDisableFingerTouchReport();
	DbBusEnterSerialDebugMode();
	DbBusStopMCU();
	DbBusIICUseBus();
	DbBusIICReshape();
	DBG(&g_I2cClient->dev,"*** set read ***0x%x 0x%x, 0x%x\n",  nAddr, nAddr >> 8, nAddr&(0x00FF));
    tx_data[0] = 0x10;
    tx_data[1] = nAddr >> 8;
	tx_data[2] = (nAddr&(0x00FF))*2;
    result = IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
	mdelay(50);
	result = IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data[0], 2);
	DBG(&g_I2cClient->dev,"0x%x, (rx_data[0]&0x3F) << 1 = 0x%x, (rx_data[0] >> 7 = 0x%x\n", rx_data[1], (rx_data[1]&0x7F) << 1,(rx_data[1] >> 7));
	pBuf[0] = rx_data[1];
	pBuf[1] = ((rx_data[1]&0x7F) << 1 )+ (rx_data[1] >> 7);
	pBuf[2] = rx_data[0];
	DBG(&g_I2cClient->dev,"0x%x,0x%x,0x%x\n", pBuf[0], pBuf[1],pBuf[2]);
	_gReadTrimData[0] = pBuf[0];
	_gReadTrimData[1] = pBuf[1];
	_gReadTrimData[2] = pBuf[2];
	DbBusIICNotUseBus();
	DbBusNotStopMCU();
	DbBusExitSerialDebugMode();
    mdelay(100);
    DrvEnableFingerTouchReport();
    //TouchDeviceResetHw();
	kfree(pBuf);
    return result;
}

static ssize_t _DrvProcfsTrimCodeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
	struct file *pfile = NULL, *ffile = NULL;
	mm_segment_t fs;
	s32 i32Temp = 0;
	u8 nTrimCodeMin = 0,nTrimCodeMax = 0,nInitTrimCode = 0, nTempBuf[3] = {0},nSetData = 0, i = 0;
	fs = get_fs();
	set_fs(KERNEL_DS);
	pfile = filp_open(ILITEK_TRIMCODE_INITIAL_PATH_ON_SD_CARD, O_RDONLY, 0);
	ReadTrimcode(0x143D, 3);
    if (IS_ERR(pfile))
    {

        DBG(&g_I2cClient->dev, "Error occurred while opening file %s.\n", ILITEK_TRIMCODE_INITIAL_PATH_ON_SD_CARD);
		mdelay(100);
		ffile = filp_open(ILITEK_TRIMCODE_INITIAL_PATH_ON_SD_CARD, O_CREAT | O_RDWR, 0);
		sprintf(_gDebugBuf,"trim code initial data:0x%x\n",_gReadTrimData[1]);
		nInitTrimCode = _gReadTrimData[1];
		ffile->f_op->write(ffile, _gDebugBuf, strlen(_gDebugBuf) * sizeof(char), &ffile->f_pos);
		set_fs(fs);
		filp_close(ffile, NULL);
    }
	else
	{
		pfile->f_op->read(pfile, _gDebugBuf, 1024, &pfile->f_pos);
		sscanf(_gDebugBuf, "trim code initial data:0x%x", &i32Temp);
		nInitTrimCode  = (u8)i32Temp;
		printk("%s\n,nInitTrimCode=0x%x\n", _gDebugBuf, nInitTrimCode);
	}
		nTrimCodeMax = nInitTrimCode + 2;
	if(nInitTrimCode - 2 < 0)
	{
		nTrimCodeMin = 0;
	}
	else
	{
		nTrimCodeMin = nInitTrimCode - 2;
	}
	DBG(&g_I2cClient->dev,"max:%d,min:%d,read trim:%d\n", nTrimCodeMax, nTrimCodeMin, _gReadTrimData[1]);
	{
		switch(_gReadTrimData[1]) 
		
		{
			case 0:
			case 64:
			case 128:
			case 192:
				nTempBuf[0] = _gReadTrimData[1] + 1;
				nTempBuf[1] = _gReadTrimData[2];
				DBG(&g_I2cClient->dev,"Read trim code: %d, modify level: 1\n", _gReadTrimData[1]);
				if(nTempBuf[0] < nTrimCodeMin && nTempBuf[0] < nTrimCodeMax)
				{
					DBG(&g_I2cClient->dev,"modify value overflow\n");
					return -1;
				}
				break;
			case 255:
			case 63:
			case 127:
			case 191:
				nTempBuf[0] = _gReadTrimData[1] - 1;
				nTempBuf[1] = _gReadTrimData[2];
				DBG(&g_I2cClient->dev,"Read trim code: %d, modify level: -1, -2\n", _gReadTrimData[1]);
				if(nTempBuf[0] < nTrimCodeMin && nTempBuf[0] < nTrimCodeMax)
				{
					DBG(&g_I2cClient->dev,"modify value overflow\n");
					return -1;
				}
				break;
			default:
				nTempBuf[0] = _gReadTrimData[1] - 1;
				nTempBuf[1] = _gReadTrimData[2];
				DBG(&g_I2cClient->dev,"Read trim code: %d, modify level: 1, -1, -2\n", _gReadTrimData[1]);
				if(nTempBuf[0] < nTrimCodeMin && nTempBuf[0] < nTrimCodeMax)
				{
					DBG(&g_I2cClient->dev,"modify value overflow\n");
					return -1;
				}
		}
	}
	nSetData = nTempBuf[0];
	for(i = 0; i < 6; i++)
	{
		WriteTrimcode(nTempBuf,0);
		ReadTrimcode(0x143D, 3);
		if(_gReadTrimData[1] == nSetData)
		{
			DBG(&g_I2cClient->dev,"Set Trim code: %d,status:Pass\n", _gReadTrimData[1]);
			return 0;
			break;
		}
		else
		{
			DBG(&g_I2cClient->dev,"Set Trim code error,Read Trim code: %d,retry count:%d\n", _gReadTrimData[1], i);
		}
	}
	DBG(&g_I2cClient->dev,"Read Trim code: %d,status:error\n", _gReadTrimData[1]);
	return -1;
}
static ssize_t _DrvProcfsSetFilmModeWrite(struct file *pFile, const char __user *pBuffer, size_t nCount, loff_t *pPos)  
{
	u8 nFilmType = 0;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	memset(_gDebugBuf, 0, 1024);

	if (copy_from_user(_gDebugBuf, pBuffer, nCount))
	{
		DBG(&g_I2cClient->dev, "copy_from_user() failed\n");

		return -EFAULT;
	}
    if (_gDebugBuf != NULL)
    {
        DBG(&g_I2cClient->dev, "nCount = %d\n", (int)nCount);       
        _gDebugBuf[nCount] = '\0';
		nFilmType = _DrvConvertCharToHexDigit(_gDebugBuf, strlen(_gDebugBuf));
		DBG(&g_I2cClient->dev, "nFeature = 0x%02X\n", nFilmType);
		DrvSetFilmMode(nFilmType);
	}
         
    return nCount;
}

static ssize_t _DrvProcfsGetFilmModeRead(struct file *pFile, char __user *pBuffer, size_t nCount, loff_t *pPos)
{
	u8 nFilmType = 0;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	nFilmType = DrvGetFilmMode();
	DBG(&g_I2cClient->dev, "*** %s() ***, nFilmType = %d\n", __func__, nFilmType);
	if (copy_to_user(pBuffer, &nFilmType, 1))
	{
		return -EFAULT;
	}	
    return 0;
}	

s32 DrvTouchDeviceInitialize(void)
{
    s32 nRetVal = 0;
#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
    int nErr;
    struct hwmsen_object tObjPs;
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _DrvCreateProcfsDirEntry(); // Create procfs directory entry

#ifdef CONFIG_ENABLE_JNI_INTERFACE
    _DrvJniCreateMsgToolMem();
#endif //CONFIG_ENABLE_JNI_INTERFACE

#ifdef CONFIG_ENABLE_ITO_MP_TEST
    DrvMpTestCreateMpTestWorkQueue();
#endif //CONFIG_ENABLE_ITO_MP_TEST
    
    g_ChipType = DrvGetChipType(); // Try to get chip type by SLAVE_I2C_ID_DBBUS(0x62) firstly.
    
    if (g_ChipType == 0) // If failed, try to get chip type by SLAVE_I2C_ID_DBBUS(0x59) again.
    {
        SLAVE_I2C_ID_DBBUS = (0xB2>>1); //0x59

        g_ChipType = DrvGetChipType(); 
    }

    DrvTouchDeviceHwReset();

    if (g_ChipType != 0) // To make sure TP is attached on cell phone.
    {
        if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
        {
            memset(&g_MutualFirmwareInfo, 0x0, sizeof(MutualFirmwareInfo_t));
        }
        else if (g_ChipType == CHIP_TYPE_MSG22XX)
        {
            memset(&g_SelfFirmwareInfo, 0x0, sizeof(SelfFirmwareInfo_t));
			memset(&g_MutualFirmwareInfo, 0x0, sizeof(MutualFirmwareInfo_t));
        }

        _DrvVariableInitialize();

#ifdef CONFIG_ENABLE_CHARGER_DETECTION 
        {
            u8 szChargerStatus[20] = {0};
     
            DrvReadFile(POWER_SUPPLY_BATTERY_STATUS_PATCH, szChargerStatus, 20);
            
            DBG(&g_I2cClient->dev, "*** Battery Status : %s ***\n", szChargerStatus);
            
            if (strstr(szChargerStatus, "Charging") != NULL || strstr(szChargerStatus, "Full") != NULL || strstr(szChargerStatus, "Fully charged") != NULL) // Charging
            {
                DrvChargerDetection(1); // charger plug-in
            }
            else // Not charging
            {
                DrvChargerDetection(0); // charger plug-out
            }
        }           
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
//        tsps_assist_register_callback("msg2xxx", &DrvTpPsEnable, &DrvGetTpPsData);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        tObjPs.polling = 0; // 0 : interrupt mode, 1 : polling mode
        tObjPs.sensor_operate = DrvTpPsOperate;
    
        if ((nErr = hwmsen_attach(ID_PROXIMITY, &tObjPs)))
        {
            DBG(&g_I2cClient->dev, "call hwmsen_attach() failed = %d\n", nErr);
        }
#endif
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION
    }
    else
    {
        nRetVal = -ENODEV;
    }

    return nRetVal;
}

static void _DrvRemoveProcfsDirEntry(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (_gProcGloveModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_GLOVE_MODE, _gProcDeviceEntry);  
        _gProcGloveModeEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_GLOVE_MODE);
    }

    if (_gProcOpenGloveModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_OPEN_GLOVE_MODE, _gProcDeviceEntry);   
        _gProcOpenGloveModeEntry = NULL; 		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_OPEN_GLOVE_MODE);
    }

    if (_gProcCloseGloveModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_CLOSE_GLOVE_MODE, _gProcDeviceEntry);   
        _gProcCloseGloveModeEntry = NULL; 		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_CLOSE_GLOVE_MODE);
    }

    if (_gProcLeatherSheathModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_LEATHER_SHEATH_MODE, _gProcDeviceEntry);  
        _gProcLeatherSheathModeEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_LEATHER_SHEATH_MODE);
    }
    if (_gProcFilmModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_LEATHER_SHEATH_MODE, _gProcDeviceEntry);  
        _gProcFilmModeEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_LEATHER_SHEATH_MODE);
    }

#ifdef CONFIG_ENABLE_JNI_INTERFACE
    if (_gProcJniMethodEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_JNI_NODE, _gProcDeviceEntry);    		
        _gProcJniMethodEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_JNI_NODE);
    }
#endif //CONFIG_ENABLE_JNI_INTERFACE	

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
    if (_gProcReportRateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_REPORT_RATE, _gProcDeviceEntry);    	
        _gProcReportRateEntry = NULL;	
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_REPORT_RATE);
    }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (_gProcGestureWakeupModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_GESTURE_WAKEUP_MODE, _gProcDeviceEntry);  
        _gProcGestureWakeupModeEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_GESTURE_WAKEUP_MODE);
    }
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    if (_gProcGestureDebugModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_GESTURE_DEBUG_MODE, _gProcDeviceEntry);    	
        _gProcGestureDebugModeEntry = NULL;	
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_GESTURE_DEBUG_MODE);
    }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
    if (_gProcGestureInforModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_GESTURE_INFORMATION_MODE, _gProcDeviceEntry);  
        _gProcGestureInforModeEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_GESTURE_INFORMATION_MODE);
    }
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    if (_gProcFirmwareModeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_MODE, _gProcDeviceEntry);    
        _gProcFirmwareModeEntry = NULL;		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_MODE);
    }

    if (_gProcFirmwareSensorEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_SENSOR, _gProcDeviceEntry);  
        _gProcFirmwareSensorEntry = NULL; 		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SENSOR);
    }

    if (_gProcFirmwarePacketHeaderEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_PACKET_HEADER, _gProcDeviceEntry);    		
        _gProcFirmwarePacketHeaderEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_PACKET_HEADER);
    }

    if (_gProcQueryFeatureSupportStatusEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS, _gProcDeviceEntry);   
        _gProcQueryFeatureSupportStatusEntry = NULL; 		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS);
    }

    if (_gProcChangeFeatureSupportStatusEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS, _gProcDeviceEntry);  
        _gProcChangeFeatureSupportStatusEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS);
    }

#ifdef CONFIG_ENABLE_ITO_MP_TEST
    if (_gProcMpTestEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MP_TEST, _gProcDeviceEntry);    		
        _gProcMpTestEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MP_TEST);
    }

    if (_gProcMpTestLogEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MP_TEST_LOG, _gProcDeviceEntry);  
        _gProcMpTestLogEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_LOG);
    }

    if (_gProcMpTestFailChannelEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MP_TEST_FAIL_CHANNEL, _gProcDeviceEntry);    		
        _gProcMpTestFailChannelEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_FAIL_CHANNEL);
    }

    if (_gProcMpTestScopeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MP_TEST_SCOPE, _gProcDeviceEntry); 
        _gProcMpTestScopeEntry = NULL;   		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_SCOPE);
    }

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    if (_gProcMpTestLogALLEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MP_TEST_LOG_ALL, _gProcDeviceEntry);
        _gProcMpTestLogALLEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_LOG_ALL);
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#endif //CONFIG_ENABLE_ITO_MP_TEST

    if (_gProcFirmwareSetDQMemValueEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_SET_DQMEM_VALUE, _gProcDeviceEntry);    		
        _gProcFirmwareSetDQMemValueEntry = NULL;
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SET_DQMEM_VALUE);
    }

    if (_gProcFirmwareSmBusDebugEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_SMBUS_DEBUG, _gProcDeviceEntry);    
        _gProcFirmwareSmBusDebugEntry = NULL;		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SMBUS_DEBUG);
    }

    if (_gProcFirmwareSetDebugValueEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_SET_DEBUG_VALUE, _gProcDeviceEntry);  
        _gProcFirmwareSetDebugValueEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SET_DEBUG_VALUE);
    }

    if (_gProcFirmwareDebugEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_DEBUG, _gProcDeviceEntry);    	
        _gProcFirmwareDebugEntry = NULL;	
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_DEBUG);
    }

    if (_gProcSdCardFirmwareUpdateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_SD_CARD_FIRMWARE_UPDATE, _gProcDeviceEntry);  
        _gProcSdCardFirmwareUpdateEntry = NULL;  		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_SD_CARD_FIRMWARE_UPDATE);
    }

    if (_gProcSeLinuxLimitFirmwareUpdateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE, _gProcDeviceEntry);
        _gProcSeLinuxLimitFirmwareUpdateEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE);
    }

    if (_gProcForceFirmwareUpdateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FORCE_FIRMWARE_UPDATE, _gProcDeviceEntry);
        _gProcForceFirmwareUpdateEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FORCE_FIRMWARE_UPDATE);
    }

    if (_gProcDeviceDriverVersionEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_DEVICE_DRIVER_VERSION, _gProcDeviceEntry);
        _gProcDeviceDriverVersionEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_DEVICE_DRIVER_VERSION);
    }

    if (_gProcPlatformFirmwareVersionEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_PLATFORM_FIRMWARE_VERSION, _gProcDeviceEntry);
        _gProcPlatformFirmwareVersionEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_PLATFORM_FIRMWARE_VERSION);
    }

    if (_gProcCustomerFirmwareVersionEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_CUSTOMER_FIRMWARE_VERSION, _gProcDeviceEntry);
        _gProcCustomerFirmwareVersionEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_CUSTOMER_FIRMWARE_VERSION);
    }

    if (_gProcApkFirmwareUpdateEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_UPDATE, _gProcDeviceEntry);
        _gProcApkFirmwareUpdateEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_UPDATE);
    }

    if (_gProcFirmwareDataEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_FIRMWARE_DATA, _gProcDeviceEntry);
        _gProcFirmwareDataEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_DATA);
    }

    if (_gProcChipTypeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_CHIP_TYPE, _gProcDeviceEntry);
        _gProcChipTypeEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_CHIP_TYPE);
    }

    if (_gProcTrimCodeEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_TRIM_CODE, _gProcDeviceEntry);
        _gProcTrimCodeEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_TRIM_CODE);
    }
	
    if (_gProcDeviceEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_DEVICE, _gProcMsTouchScreenMsg20xxEntry);
        _gProcDeviceEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_DEVICE);
    }

    if (_gProcMsTouchScreenMsg20xxEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_MS_TOUCHSCREEN_MSG20XX, _gProcClassEntry);
        _gProcMsTouchScreenMsg20xxEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_MS_TOUCHSCREEN_MSG20XX);
    }

    if (_gProcClassEntry != NULL)
    {
        remove_proc_entry(PROC_NODE_CLASS, NULL);
        _gProcClassEntry = NULL;    		
        DBG(&g_I2cClient->dev, "Remove procfs file node(%s) OK!\n", PROC_NODE_CLASS);
    }
}

/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)

#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
static ssize_t _DrvVirtualKeysShow(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    return sprintf(buf,
        __stringify(EV_KEY) ":" __stringify(KEY_HOMEPAGE) ":90:1330:100:100"
        ":" __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":270:1330:100:100"
        ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":450:1330:100:100"
        ":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":630:1330:100:100"
        "\n");
}

static struct kobj_attribute virtual_keys_attr = {
    .attr = {
        .name = "virtualkeys.msg2xxx_ts",
        .mode = S_IRUGO,
    },
    .show = &_DrvVirtualKeysShow,
};

static struct attribute *properties_attrs[] = {
    &virtual_keys_attr.attr,
    NULL
};

static struct attribute_group properties_attr_group = {
    .attrs = properties_attrs,
};

static void _DrvVirtualKeysInit(void)
{
    s32 nRetVal = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    g_PropertiesKObj = kobject_create_and_add("board_properties", NULL);
    if (g_PropertiesKObj == NULL)
    {
        DBG(&g_I2cClient->dev, "*** Failed to kobject_create_and_add() for virtual keys *** nRetVal=%d\n", nRetVal); 
        return;
    }
    
    nRetVal = sysfs_create_group(g_PropertiesKObj, &properties_attr_group);
    if (nRetVal < 0)
    {
        DBG(&g_I2cClient->dev, "*** Failed to sysfs_create_group() for virtual keys *** nRetVal=%d\n", nRetVal); 

        kobject_put(g_PropertiesKObj);
        g_PropertiesKObj = NULL;
    }
}

static void _DrvVirtualKeysUnInit(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    if (g_PropertiesKObj)
    {
        kobject_put(g_PropertiesKObj);
        g_PropertiesKObj = NULL;
    }
}
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
static s32 _DrvTouchPinCtrlInit(struct i2c_client *pClient)
{
    s32 nRetVal = 0;
    u32 nFlag = 0;
    struct device_node *pDeviceNode = pClient->dev.of_node;
	
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    
//    _gGpioRst = of_get_named_gpio_flags(pDeviceNode, "chipone,reset-gpio", 0, &nFlag);
    _gGpioRst = of_get_named_gpio_flags(pDeviceNode, "touch,reset-gpio", 0, &nFlag); // generally used for ILI21XX
//    _gGpioRst = of_get_named_gpio_flags(pDeviceNode, "mstar,rst-gpio", 0, &nFlag); // generally used for MSG22XX/MSG28XX/MSG58XXA
    
    MS_TS_MSG_IC_GPIO_RST = _gGpioRst;
    
    if (_gGpioRst < 0)
    {
        return _gGpioRst;
    }

//    _gGpioIrq = of_get_named_gpio_flags(pDeviceNode, "chipone,irq-gpio", 0, &nFlag);
	_gGpioIrq = of_get_named_gpio_flags(pDeviceNode, "touch,irq-gpio", 0, &nFlag); // generally used for ILI21XX
//    _gGpioIrq = of_get_named_gpio_flags(pDeviceNode, "mstar,irq-gpio", 0, &nFlag); // generally used for MSG22XX/MSG28XX/MSG58XXA
    
    MS_TS_MSG_IC_GPIO_INT = _gGpioIrq;
	
    DBG(&g_I2cClient->dev, "_gGpioRst = %d, _gGpioIrq = %d\n", _gGpioRst, _gGpioIrq); 
    
    if (_gGpioIrq < 0)
    {
        return _gGpioIrq;
    }
    /* Get pinctrl if target uses pinctrl */
    _gTsPinCtrl = devm_pinctrl_get(&(pClient->dev));
    if (IS_ERR_OR_NULL(_gTsPinCtrl)) 
    {
        nRetVal = PTR_ERR(_gTsPinCtrl);
        DBG(&g_I2cClient->dev, "Target does not use pinctrl nRetVal=%d\n", nRetVal); 
        goto ERROR_PINCTRL_GET;
    }

    _gPinCtrlStateActive = pinctrl_lookup_state(_gTsPinCtrl, PINCTRL_STATE_ACTIVE);
    if (IS_ERR_OR_NULL(_gPinCtrlStateActive)) 
    {
        nRetVal = PTR_ERR(_gPinCtrlStateActive);
        DBG(&g_I2cClient->dev, "Can not lookup %s pinstate nRetVal=%d\n", PINCTRL_STATE_ACTIVE, nRetVal); 
        goto ERROR_PINCTRL_LOOKUP;
    }

    _gPinCtrlStateSuspend = pinctrl_lookup_state(_gTsPinCtrl, PINCTRL_STATE_SUSPEND);
    if (IS_ERR_OR_NULL(_gPinCtrlStateSuspend)) 
    {
        nRetVal = PTR_ERR(_gPinCtrlStateSuspend);
        DBG(&g_I2cClient->dev, "Can not lookup %s pinstate nRetVal=%d\n", PINCTRL_STATE_SUSPEND, nRetVal); 
        goto ERROR_PINCTRL_LOOKUP;
    }

    _gPinCtrlStateRelease = pinctrl_lookup_state(_gTsPinCtrl, PINCTRL_STATE_RELEASE);
    if (IS_ERR_OR_NULL(_gPinCtrlStateRelease)) 
    {
        nRetVal = PTR_ERR(_gPinCtrlStateRelease);
        DBG(&g_I2cClient->dev, "Can not lookup %s pinstate nRetVal=%d\n", PINCTRL_STATE_RELEASE, nRetVal); 
    }
    
    pinctrl_select_state(_gTsPinCtrl, _gPinCtrlStateActive);
    
    return 0;

ERROR_PINCTRL_LOOKUP:
    devm_pinctrl_put(_gTsPinCtrl);
ERROR_PINCTRL_GET:
    _gTsPinCtrl = NULL;
	
    return nRetVal;
}

static void _DrvTouchPinCtrlUnInit(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    if (_gTsPinCtrl)
    {
        devm_pinctrl_put(_gTsPinCtrl);
        _gTsPinCtrl = NULL;
    }
}
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL

/* read data through I2C then report data to input sub-system when interrupt occurred */
static void _DrvFingerTouchDoWork(struct work_struct *pWork)
{
    unsigned long nIrqFlag;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _DrvHandleFingerTouch();

    DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag);  // add for debug

    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

    if (_gInterruptFlag == 0
#ifdef CONFIG_ENABLE_ITO_MP_TEST
        && g_IsInMpTest == 0
#endif //CONFIG_ENABLE_ITO_MP_TEST
    ) 
    {
        enable_irq(_gIrq);

        _gInterruptFlag = 1;
    } 
        
    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
}

/* The interrupt service routine will be triggered when interrupt occurred */
static irqreturn_t _DrvFingerTouchInterruptHandler(s32 nIrq, void *pDeviceId)
{
    unsigned long nIrqFlag;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag); 

    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

    if (_gInterruptFlag == 1
#ifdef CONFIG_ENABLE_ITO_MP_TEST
        && g_IsInMpTest == 0
#endif //CONFIG_ENABLE_ITO_MP_TEST
    ) 
    {
        disable_irq_nosync(_gIrq);

        _gInterruptFlag = 0;

        schedule_work(&_gFingerTouchWork);
    }

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
    
    return IRQ_HANDLED;
}
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
//static irqreturn_t _DrvFingerTouchInterruptHandler(s32 nIrq, struct irq_desc *desc)
static irqreturn_t _DrvFingerTouchInterruptHandler(s32 nIrq, void *pDeviceId)
#else
static void _DrvFingerTouchInterruptHandler(void)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    unsigned long nIrqFlag;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag); 

    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM

    if (_gInterruptFlag == 1
#ifdef CONFIG_ENABLE_ITO_MP_TEST
        && g_IsInMpTest == 0
#endif //CONFIG_ENABLE_ITO_MP_TEST
    ) 
    {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
        disable_irq_nosync(_gIrq);
#else
        mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

        _gInterruptFlag = 0;

        schedule_work(&_gFingerTouchWork);
    }

#else    

    if (_gInterruptFlag == 1
#ifdef CONFIG_ENABLE_ITO_MP_TEST
        && g_IsInMpTest == 0
#endif //CONFIG_ENABLE_ITO_MP_TEST
    )
    {    
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
        disable_irq_nosync(_gIrq);
#else
        mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

        _gInterruptFlag = 0;

        _gTpdFlag = 1;
        wake_up_interruptible(&_gWaiter);
    }        
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    return IRQ_HANDLED;
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
}

#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
/* read data through I2C then report data to input sub-system when interrupt occurred */
static void _DrvFingerTouchDoWork(struct work_struct *pWork)
{
    unsigned long nIrqFlag;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _DrvHandleFingerTouch();

    DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag);  // add for debug

    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

    if (_gInterruptFlag == 0
#ifdef CONFIG_ENABLE_ITO_MP_TEST
        && g_IsInMpTest == 0
#endif //CONFIG_ENABLE_ITO_MP_TEST
    ) 
    {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
        enable_irq(_gIrq);
#else
        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

        _gInterruptFlag = 1;
    }

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
}

#else

static int _DrvFingerTouchHandler(void *pUnUsed)
{
    unsigned long nIrqFlag;
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    sched_setscheduler(current, SCHED_RR, &param);

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
	
    do
    {
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(_gWaiter, _gTpdFlag != 0);
        _gTpdFlag = 0;
        
        set_current_state(TASK_RUNNING);

#ifdef CONFIG_ENABLE_ITO_MP_TEST
        if (g_IsInMpTest == 0)
        {
#endif //CONFIG_ENABLE_ITO_MP_TEST
            _DrvHandleFingerTouch();
#ifdef CONFIG_ENABLE_ITO_MP_TEST
        }
#endif //CONFIG_ENABLE_ITO_MP_TEST

        DBG(&g_I2cClient->dev, "*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag); 

        spin_lock_irqsave(&_gIrqLock, nIrqFlag);

        if (_gInterruptFlag == 0       
#ifdef CONFIG_ENABLE_ITO_MP_TEST
            && g_IsInMpTest == 0
#endif //CONFIG_ENABLE_ITO_MP_TEST
        )
        {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
            enable_irq(_gIrq);
#else
            mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

            _gInterruptFlag = 1;
        } 

        spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
		
    } while (!kthread_should_stop());
	
    return 0;
}
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
#endif

void DrvMutexVariableInitialize(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

    mutex_init(&g_Mutex);
    spin_lock_init(&_gIrqLock);
}

s32 DrvInputDeviceInitialize(struct i2c_client *pClient)
{
    s32 nRetVal = 0;
    u32 i = 0;

    DBG(&g_I2cClient->dev, "*** %s() *** %d \n", __func__,i);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    /* allocate an input device */
    g_InputDevice = input_allocate_device();
    if (g_InputDevice == NULL)
    {
        DBG(&g_I2cClient->dev, "*** Failed to allocate touch input device ***\n"); 
        return -ENOMEM;
    }

    g_InputDevice->name = pClient->name;
    g_InputDevice->phys = "I2C";
    g_InputDevice->dev.parent = &pClient->dev;
    g_InputDevice->id.bustype = BUS_I2C;
    
    /* set the supported event type for input device */
    set_bit(EV_ABS, g_InputDevice->evbit);
    set_bit(EV_SYN, g_InputDevice->evbit);
    set_bit(EV_KEY, g_InputDevice->evbit);
    set_bit(BTN_TOUCH, g_InputDevice->keybit);
    set_bit(INPUT_PROP_DIRECT, g_InputDevice->propbit);

#ifdef CONFIG_TP_HAVE_KEY
    // Method 1.
    { 
        for (i = 0; i < MAX_KEY_NUM; i ++)
        {
            input_set_capability(g_InputDevice, EV_KEY, g_TpVirtualKey[i]);
        }
    }

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
    _DrvVirtualKeysInit(); // Initialize virtual keys for specific SPRC/QCOM platform.
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

/*  
#ifdef CONFIG_TP_HAVE_KEY
    // Method 2.
    set_bit(TOUCH_KEY_MENU, g_InputDevice->keybit); //Menu
    set_bit(TOUCH_KEY_HOME, g_InputDevice->keybit); //Home
    set_bit(TOUCH_KEY_BACK, g_InputDevice->keybit); //Back
    set_bit(TOUCH_KEY_SEARCH, g_InputDevice->keybit); //Search
#endif //CONFIG_TP_HAVE_KEY
*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    input_set_capability(g_InputDevice, EV_KEY, KEY_POWER);
    input_set_capability(g_InputDevice, EV_KEY, KEY_UP);
    input_set_capability(g_InputDevice, EV_KEY, KEY_DOWN);
    input_set_capability(g_InputDevice, EV_KEY, KEY_LEFT);
    input_set_capability(g_InputDevice, EV_KEY, KEY_RIGHT);
    input_set_capability(g_InputDevice, EV_KEY, KEY_W);
    input_set_capability(g_InputDevice, EV_KEY, KEY_Z);
    input_set_capability(g_InputDevice, EV_KEY, KEY_V);
    input_set_capability(g_InputDevice, EV_KEY, KEY_O);
    input_set_capability(g_InputDevice, EV_KEY, KEY_M);
    input_set_capability(g_InputDevice, EV_KEY, KEY_C);
    input_set_capability(g_InputDevice, EV_KEY, KEY_E);
    input_set_capability(g_InputDevice, EV_KEY, KEY_S);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        input_set_abs_params(g_InputDevice, ABS_MT_TRACKING_ID, 0, (MUTUAL_MAX_TOUCH_NUM-1), 0, 0); // ABS_MT_TRACKING_ID is used for MSG28xx/MSG58xxA/ILI21xx only
    }

#ifndef CONFIG_ENABLE_TYPE_B_PROTOCOL
    input_set_abs_params(g_InputDevice, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_X, TOUCH_SCREEN_X_MIN, TOUCH_SCREEN_X_MAX, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_Y, TOUCH_SCREEN_Y_MIN, TOUCH_SCREEN_Y_MAX, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_PRESSURE, 0, 255, 0, 0);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        set_bit(BTN_TOOL_FINGER, g_InputDevice->keybit);
        input_mt_init_slots(g_InputDevice, SELF_MAX_TOUCH_NUM, 0); // for MSG22xx
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        set_bit(BTN_TOOL_FINGER, g_InputDevice->keybit);
        input_mt_init_slots(g_InputDevice, MUTUAL_MAX_TOUCH_NUM, 0);  // for MSG28xx/MSG58xxA/ILI21xx 
    }
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

    /* register the input device to input sub-system */
    nRetVal = input_register_device(g_InputDevice);
    if (nRetVal < 0)
    {
        DBG(&g_I2cClient->dev, "*** Unable to register touch input device *** nRetVal=%d\n", nRetVal); 
        return nRetVal;
    }

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
    nRetVal = _DrvProximityInputDeviceInit(pClient);
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    g_InputDevice = tpd->dev;
/*
    g_InputDevice->phys = "I2C";
    g_InputDevice->dev.parent = &pClient->dev;
    g_InputDevice->id.bustype = BUS_I2C;
    
    // set the supported event type for input device 
    set_bit(EV_ABS, g_InputDevice->evbit);
    set_bit(EV_SYN, g_InputDevice->evbit);
    set_bit(EV_KEY, g_InputDevice->evbit);
    set_bit(BTN_TOUCH, g_InputDevice->keybit);
    set_bit(INPUT_PROP_DIRECT, g_InputDevice->propbit);
*/

#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc(); // DmaAlloc() shall be called after g_InputDevice is available.
#endif //CONFIG_ENABLE_DMA_IIC


#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (tpd_dts_data.use_tpd_button)
    {
        for (i = 0; i < tpd_dts_data.tpd_key_num; i ++)
        {
            input_set_capability(g_InputDevice, EV_KEY, tpd_dts_data.tpd_key_local[i]);
        }
    }
#else
#ifdef CONFIG_TP_HAVE_KEY
    {
        for (i = 0; i < MAX_KEY_NUM; i ++)
        {
            input_set_capability(g_InputDevice, EV_KEY, g_TpVirtualKey[i]);
        }
    }
#endif //CONFIG_TP_HAVE_KEY
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    input_set_capability(g_InputDevice, EV_KEY, KEY_POWER);
    input_set_capability(g_InputDevice, EV_KEY, KEY_UP);
    input_set_capability(g_InputDevice, EV_KEY, KEY_DOWN);
    input_set_capability(g_InputDevice, EV_KEY, KEY_LEFT);
    input_set_capability(g_InputDevice, EV_KEY, KEY_RIGHT);
    input_set_capability(g_InputDevice, EV_KEY, KEY_W);
    input_set_capability(g_InputDevice, EV_KEY, KEY_Z);
    input_set_capability(g_InputDevice, EV_KEY, KEY_V);
    input_set_capability(g_InputDevice, EV_KEY, KEY_O);
    input_set_capability(g_InputDevice, EV_KEY, KEY_M);
    input_set_capability(g_InputDevice, EV_KEY, KEY_C);
    input_set_capability(g_InputDevice, EV_KEY, KEY_E);
    input_set_capability(g_InputDevice, EV_KEY, KEY_S);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        input_set_abs_params(g_InputDevice, ABS_MT_TRACKING_ID, 0, (MUTUAL_MAX_TOUCH_NUM-1), 0, 0); // ABS_MT_TRACKING_ID is used for MSG28xx/MSG58xxA/ILI21xx only
    }
    input_set_abs_params(g_InputDevice, ABS_MT_PRESSURE, 0, 255, 0, 0);

/*
#ifndef CONFIG_ENABLE_TYPE_B_PROTOCOL
    input_set_abs_params(g_InputDevice, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_X, TOUCH_SCREEN_X_MIN, TOUCH_SCREEN_X_MAX, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_Y, TOUCH_SCREEN_Y_MIN, TOUCH_SCREEN_Y_MAX, 0, 0);
*/

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        set_bit(BTN_TOOL_FINGER, g_InputDevice->keybit);
        input_mt_init_slots(g_InputDevice, SELF_MAX_TOUCH_NUM, 0); // for MSG22xx
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        set_bit(BTN_TOOL_FINGER, g_InputDevice->keybit);
        input_mt_init_slots(g_InputDevice, MUTUAL_MAX_TOUCH_NUM, 0); // for MSG28xx/MSG58xxA/ILI21xx
    }
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#endif

    return nRetVal;    
}

s32 DrvTouchDeviceRequestGPIO(struct i2c_client *pClient)
{
    s32 nRetVal = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 
    
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef ALLWINNER_PLATFORM

	//nRetVal = gpio_request(config_info.wakeup_number, "C_TP_RST");   
    //if (nRetVal < 0)
    //{
        //DBG(&g_I2cClient->dev, "*** Failed to request GPIO , error %d ***\n", nRetVal); 
    //}
	//gpio_free(CTP_IRQ_NUMBER);
    nRetVal = gpio_request(CTP_IRQ_NUMBER, "C_TP_INT");
	 
    if (nRetVal < 0)
    {
        DBG(&g_I2cClient->dev, "*** Failed to request GPIO , error %d ***\n", nRetVal); 
    }

	gpio_direction_input(CTP_IRQ_NUMBER);

#else
#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
    _DrvTouchPinCtrlInit(pClient);
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL

    nRetVal = gpio_request(MS_TS_MSG_IC_GPIO_RST, "C_TP_RST");     
    if (nRetVal < 0)
    {
        DBG(&g_I2cClient->dev, "*** Failed to request GPIO %d, error %d ***\n", MS_TS_MSG_IC_GPIO_RST, nRetVal); 
    }

    nRetVal = gpio_request(MS_TS_MSG_IC_GPIO_INT, "C_TP_INT");    
    if (nRetVal < 0)
    {
        DBG(&g_I2cClient->dev, "*** Failed to request GPIO %d, error %d ***\n", MS_TS_MSG_IC_GPIO_INT, nRetVal); 
    }

	gpio_direction_input(MS_TS_MSG_IC_GPIO_INT);
#endif //ALLWINNER_PLATFORM
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM

    return nRetVal;    
}

s32 DrvTouchDeviceRegisterFingerTouchInterruptHandler(void)
{
    s32 nRetVal = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    /* initialize the finger touch work queue */ 
    INIT_WORK(&_gFingerTouchWork, _DrvFingerTouchDoWork);
#ifdef ALLWINNER_PLATFORM
	int ret = 0;
	_gIrq = gpio_to_irq(CTP_IRQ_NUMBER);

	config_info.dev = &(g_InputDevice->dev);
	
	ret = input_request_int(&(config_info.input_type),_DrvFingerTouchInterruptHandler,CTP_IRQ_MODE,NULL);

	_gInterruptFlag = 1;

	if (ret) {		
		printk( "Ilitek: request irq failed\n");	
	}
#else 
    _gIrq = gpio_to_irq(MS_TS_MSG_IC_GPIO_INT);

    /* request an irq and register the isr */
    nRetVal = request_threaded_irq(_gIrq/*MS_TS_MSG_IC_GPIO_INT*/, NULL, _DrvFingerTouchInterruptHandler,
                  /*IRQF_TRIGGER_RISING*/ IRQF_TRIGGER_FALLING | IRQF_ONESHOT/* | IRQF_NO_SUSPEND */,
                  "msg2xxx", NULL); 

//        nRetVal = request_irq(_gIrq/*MS_TS_MSG_IC_GPIO_INT*/, _DrvFingerTouchInterruptHandler,
//                      IRQF_TRIGGER_RISING /* IRQF_TRIGGER_FALLING *//* | IRQF_NO_SUSPEND */,
//                      "msg2xxx", NULL); 

    _gInterruptFlag = 1;
    
    if (nRetVal != 0)
    {
        DBG(&g_I2cClient->dev, "*** Unable to claim irq %d; error %d ***\n", _gIrq, nRetVal); 
    }
#endif //ALLWINNER_PLATFORM 
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
    /* initialize the finger touch work queue */
    INIT_WORK(&_gFingerTouchWork, _DrvFingerTouchDoWork);
#else
    _gThread = kthread_run(_DrvFingerTouchHandler, 0, TPD_DEVICE);
    if (IS_ERR(_gThread))
    {
        nRetVal = PTR_ERR(_gThread);
        DBG(&g_I2cClient->dev, "Failed to create kernel thread: %d\n", nRetVal);
    }
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    {
        struct device_node *pDeviceNode = NULL;
        u32 ints[2] = {0,0};

        tpd_gpio_as_int(MS_TS_MSG_IC_GPIO_INT);

//            pDeviceNode = of_find_compatible_node(NULL, NULL, "mediatek, TOUCH_PANEL-eint"); 
        pDeviceNode = of_find_matching_node(pDeviceNode, touch_of_match);
        
        if (pDeviceNode)
        {
            of_property_read_u32_array(pDeviceNode, "debounce", ints, ARRAY_SIZE(ints));
            gpio_set_debounce(ints[0], ints[1]);
            
            _gIrq = irq_of_parse_and_map(pDeviceNode, 0);
            if (_gIrq == 0)
            {
                DBG(&g_I2cClient->dev, "*** Unable to irq_of_parse_and_map() ***\n");
            }

            /* request an irq and register the isr */
            nRetVal = request_threaded_irq(_gIrq/*MS_TS_MSG_IC_GPIO_INT*/, NULL, _DrvFingerTouchInterruptHandler,
                  /*IRQF_TRIGGER_RISING*/  IRQF_TRIGGER_FALLING /*IRQF_TRIGGER_NONE */| IRQF_ONESHOT/* | IRQF_NO_SUSPEND */,
                  "touch_panel-eint", NULL); 

//                nRetVal = request_irq(_gIrq/*MS_TS_MSG_IC_GPIO_INT*/, _DrvFingerTouchInterruptHandler,
//                      IRQF_TRIGGER_RISING /* IRQF_TRIGGER_FALLING *//*IRQF_TRIGGER_NONE *//* | IRQF_NO_SUSPEND */,
//                      "touch_panel-eint", NULL); 

            if (nRetVal != 0)
            {
                DBG(&g_I2cClient->dev, "*** Unable to claim irq %d; error %d ***\n", _gIrq, nRetVal);
                DBG(&g_I2cClient->dev, "*** gpio_pin=%d, debounce=%d ***\n", ints[0], ints[1]);
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "*** request_irq() can not find touch eint device node! ***\n");
        }

//            enable_irq(_gIrq);
    }
#else
    mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_INT, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_INT, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(MS_TS_MSG_IC_GPIO_INT, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(MS_TS_MSG_IC_GPIO_INT, GPIO_PULL_UP);

    mt_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
    mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE/* EINTF_TRIGGER_RISING */, _DrvFingerTouchInterruptHandler, 1);

    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

    _gInterruptFlag = 1;

#endif
    DrvDisableFingerTouchReport();
    mdelay(100);

    return nRetVal;
}

void DrvTouchDeviceRegisterEarlySuspend(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_NOTIFIER_FB
    _gFbNotifier.notifier_call = MsDrvInterfaceTouchDeviceFbNotifierCallback;
    fb_register_client(&_gFbNotifier);
#else
#ifdef CONFIG_HAS_EARLYSUSPEND
    _gEarlySuspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    _gEarlySuspend.suspend = MsDrvInterfaceTouchDeviceSuspend; //\C9?\A8?\C6\F0??\A3\AC\B9\D8\D6ж?\AC\B9??\E7
    _gEarlySuspend.resume = MsDrvInterfaceTouchDeviceResume;
    register_early_suspend(&_gEarlySuspend);
#endif
#endif //CONFIG_ENABLE_NOTIFIER_FB 

#ifdef ALLWINNER_PLATFORM
	device_enable_async_suspend(&g_I2cClient->dev);
	pm_runtime_set_active(&g_I2cClient->dev);
	pm_runtime_get(&g_I2cClient->dev);
	pm_runtime_enable(&g_I2cClient->dev);
#endif
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM
}

/* remove function is triggered when the input device is removed from input sub-system */
s32 DrvTouchDeviceRemove(struct i2c_client *pClient)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__); 

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef ALLWINNER_PLATFORM
	gpio_free(CTP_IRQ_NUMBER);
#else
    gpio_free(MS_TS_MSG_IC_GPIO_INT);
    gpio_free(MS_TS_MSG_IC_GPIO_RST);
#endif

    if (g_InputDevice)
    {
#ifdef ALLWINNER_PLATFORM  
		input_free_int(&(config_info.input_type), NULL);
#else
		//destroy_workqueue(irq_work_queue_allwiner);
        free_irq(_gIrq, g_InputDevice);
#endif
        input_unregister_device(g_InputDevice);
        g_InputDevice = NULL;
    }

#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
    _DrvTouchPinCtrlUnInit();
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL

#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
    _DrvVirtualKeysUnInit();
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
    _DrvProximityInputDeviceUnInit();
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION   

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    DrvTouchDeviceRegulatorPowerOn(false);

    if (g_ReguVdd != NULL)
    {
        regulator_put(g_ReguVdd);
        g_ReguVdd = NULL;
    }

    if (g_ReguVcc_i2c != NULL)
    {
        regulator_put(g_ReguVcc_i2c);
        g_ReguVcc_i2c = NULL;
    }
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    DrvTouchDeviceRegulatorPowerOn(false);

    if (g_ReguVdd != NULL)
    {
        regulator_put(g_ReguVdd);
        g_ReguVdd = NULL;
    }
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#endif    

    if (g_TouchKSet)
    {
        kset_unregister(g_TouchKSet);
        g_TouchKSet = NULL;
    }

    if (g_TouchKObj)
    {
        kobject_put(g_TouchKObj);
        g_TouchKObj = NULL;
    }

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    if (g_GestureKSet)
    {
        kset_unregister(g_GestureKSet);
        g_GestureKSet = NULL;
    }
    
    if (g_GestureKObj)
    {
        kobject_put(g_GestureKObj);
        g_GestureKObj = NULL;
    }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    _DrvRemoveProcfsDirEntry();

#ifdef CONFIG_ENABLE_JNI_INTERFACE
    _DrvJniDeleteMsgToolMem();
#endif //CONFIG_ENABLE_JNI_INTERFACE

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
    if (g_ChargerPlugInOutCheckWorkqueue)
    {
        destroy_workqueue(g_ChargerPlugInOutCheckWorkqueue);
        g_ChargerPlugInOutCheckWorkqueue = NULL;
    }
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    if (g_EsdCheckWorkqueue)
    {
        destroy_workqueue(g_EsdCheckWorkqueue);
        g_EsdCheckWorkqueue = NULL;
    }
#endif //CONFIG_ENABLE_ESD_PROTECTION

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    return 0;
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL 
static u32 _DrvPointDistance(u16 nX, u16 nY, u16 nPrevX, u16 nPrevY)
{ 
    u32 nRetVal = 0;
	
    nRetVal = (((nX-nPrevX)*(nX-nPrevX))+((nY-nPrevY)*(nY-nPrevY)));
    
    return nRetVal;
}
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

static s32 _DrvSelfParsePacket(u8 *pPacket, u16 nLength, SelfTouchInfo_t *pInfo) // for MSG22xx
{
    u8 nCheckSum = 0;
    u32 nDeltaX = 0, nDeltaY = 0;
    u32 nX = 0;
    u32 nY = 0;
#ifdef CONFIG_SWAP_X_Y
    u32 nTempX;
    u32 nTempY;
#endif
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
    static u8 nPrevTouchNum = 0; 
    static u16 szPrevX[SELF_MAX_TOUCH_NUM] = {0xFFFF, 0xFFFF};
    static u16 szPrevY[SELF_MAX_TOUCH_NUM] = {0xFFFF, 0xFFFF};
    static u8  szPrevPress[SELF_MAX_TOUCH_NUM] = {0};
    u32 i = 0;
    u16 szX[SELF_MAX_TOUCH_NUM] = {0};
    u16 szY[SELF_MAX_TOUCH_NUM] = {0};
    u16 nTemp = 0;
    u8  nChangePoints = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
    u16 nCheckSumIndex = nLength-1; //Set default checksum index for demo mode
	DBG(&g_I2cClient->dev, "check address : %d, nLength : %d\n", nCheckSumIndex, nLength);
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
    _gCurrPress[0] = 0;
    _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
    if (g_IsEnableReportRate == 1)
    {
        if (g_InterruptCount == 4294967295UL)
        {
            g_InterruptCount = 0; // Reset count if overflow
            DBG(&g_I2cClient->dev, "g_InterruptCount reset to 0\n");
        }	

        if (g_InterruptCount == 0)
        {
            // Get start time
            do_gettimeofday(&g_StartTime);
    
            DBG(&g_I2cClient->dev, "Start time : %lu sec, %lu msec\n", g_StartTime.tv_sec,  g_StartTime.tv_usec); 
        }
        
        g_InterruptCount ++;

        DBG(&g_I2cClient->dev, "g_InterruptCount = %d\n", g_InterruptCount);
    }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            nCheckSumIndex = 7;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE || g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE)
        {
            if (g_ChipType == CHIP_TYPE_MSG22XX && IS_SELF_FREQ_SCAN_ENABLED)
            {
                nCheckSumIndex = nLength-1;
            }
            else
            {
                nCheckSumIndex = 31;
            }
        }

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
        if (g_GestureWakeupFlag == 1)
        {
            nCheckSumIndex = nLength-1;
        }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
    } //IS_FIRMWARE_DATA_LOG_ENABLED
    DBG(&g_I2cClient->dev, "check address : %d, nLength : %d\n", nCheckSumIndex, nLength);
    nCheckSum = _DrvCalculateCheckSum(&pPacket[0], nCheckSumIndex);
    DBG(&g_I2cClient->dev, "check sum : [%x] == [%x]? \n", pPacket[nCheckSumIndex], nCheckSum);
	DBG(&g_I2cClient->dev, "check address : %d, nLength : %d\n", nCheckSumIndex, nLength);

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x08 && pPacket[3] == PACKET_TYPE_ESD_CHECK_HW_RESET && pPacket[4] == 0xFF && pPacket[5] == 0xFF && pPacket[6] == 0xFF)
        {
            DBG(&g_I2cClient->dev, "ESD HwReset check : g_IsUpdateFirmware=%d, g_IsHwResetByDriver=%d\n", g_IsUpdateFirmware, g_IsHwResetByDriver);

            if (g_IsUpdateFirmware == 0
                && g_IsHwResetByDriver == 0
#ifdef CONFIG_ENABLE_ITO_MP_TEST
                && g_IsInMpTest == 0
#endif //CONFIG_ENABLE_ITO_MP_TEST            	
            )
            {
#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
                if (g_EnableTpProximity == 1)
                {
                    DrvEnableProximity();
				
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                    g_FaceClosingTp = 0; // far away for SPRD/QCOM platform
				           
                    if (g_ProximityInputDevice != NULL)
                    {
                        input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 1);
                        input_sync(g_ProximityInputDevice);
                    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
                    {
                        int nErr;
                        hwm_sensor_data tSensorData;
				
                        g_FaceClosingTp = 1; // far away for MTK platform
				
                        // map and store data to hwm_sensor_data
                        tSensorData.values[0] = DrvGetTpPsData();
                        tSensorData.value_divide = 1;
                        tSensorData.status = SENSOR_STATUS_ACCURACY_MEDIUM;
                        // let up layer to know
                        if ((nErr = hwmsen_get_interrupt_data(ID_PROXIMITY, &tSensorData)))
                        {
                            DBG(&g_I2cClient->dev, "call hwmsen_get_interrupt_data() failed = %d\n", nErr);
                        }
                    }
#endif               
                }
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION
/*
                DrvFingerTouchReleased(0, 0, 0); // Send touch end for clearing point touch
                input_sync(g_InputDevice);
*/
            }
            
            g_IsHwResetByDriver = 0; //Reset check flag to 0 after HwReset check

            return -1;
        }
    }
#endif //CONFIG_ENABLE_ESD_PROTECTION

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        u8 nWakeupMode = 0;
        u8 bIsCorrectFormat = 0;

        DBG(&g_I2cClient->dev, "received raw data from touch panel as following:\n");
        DBG(&g_I2cClient->dev, "pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x pPacket[5]=%x \n", \
            pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5]);

        if (g_ChipType == CHIP_TYPE_MSG22XX && pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x06 && pPacket[3] == PACKET_TYPE_GESTURE_WAKEUP)
        {
            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;
        } 
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        else if (g_ChipType == CHIP_TYPE_MSG22XX && pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x80 && pPacket[3] == PACKET_TYPE_GESTURE_DEBUG)
        {
            u32 a = 0;

            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;
            
            for (a = 0; a < 0x80; a ++)
            {
                g_LogGestureDebug[a] = pPacket[a];
            }
            
            if (!(pPacket[5] >> 7))// LCM Light Flag = 0
            {
                nWakeupMode = 0xFE;
                DBG(&g_I2cClient->dev, "gesture debug mode LCM flag = 0\n");
            }
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        else if (g_ChipType == CHIP_TYPE_MSG22XX && pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x80 && pPacket[3] == PACKET_TYPE_GESTURE_INFORMATION)
        {
            u32 a = 0;
            u32 nTmpCount = 0;
            u32 nWidth = 0;
            u32 nHeight = 0;

            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;

            for (a = 0; a < 6; a ++)//header
            {
                g_LogGestureInfor[nTmpCount] = pPacket[a];
                nTmpCount++;
            }

            for (a = 6; a < 126; a = a+3)//parse packet to coordinate
            {
                u32 nTranX = 0;
                u32 nTranY = 0;
                
                _DrvConvertGestureInformationModeCoordinate(&pPacket[a], &nTranX, &nTranY);
                g_LogGestureInfor[nTmpCount] = nTranX;
                nTmpCount++;
                g_LogGestureInfor[nTmpCount] = nTranY;
                nTmpCount++;
            }
            
            nWidth = (((pPacket[12] & 0xF0) << 4) | pPacket[13]); //parse width & height
            nHeight = (((pPacket[12] & 0x0F) << 8) | pPacket[14]);

            DBG(&g_I2cClient->dev, "Before convert [width,height]=[%d,%d]\n", nWidth, nHeight);

            if ((pPacket[12] == 0xFF) && (pPacket[13] == 0xFF) && (pPacket[14] == 0xFF))
            {
                nWidth = 0; 
                nHeight = 0; 
            }
            else
            {
                nWidth = (nWidth * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                nHeight = (nHeight * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "After convert [width,height]=[%d,%d]\n", nWidth, nHeight);
            }

            g_LogGestureInfor[10] = nWidth;
            g_LogGestureInfor[11] = nHeight;
            
            g_LogGestureInfor[nTmpCount] = pPacket[126]; //Dummy
            nTmpCount++;
            g_LogGestureInfor[nTmpCount] = pPacket[127]; //checksum
            nTmpCount++;
            DBG(&g_I2cClient->dev, "gesture information mode Count = %d\n", nTmpCount);
        }
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        
        if (bIsCorrectFormat) 
        {
            DBG(&g_I2cClient->dev, "nWakeupMode = 0x%x\n", nWakeupMode);

            switch (nWakeupMode)
            {
                case 0x58:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by DOUBLE_CLICK gesture wakeup.\n");

                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x60:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG;
                    
                    DBG(&g_I2cClient->dev, "Light up screen by UP_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_UP, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_UP, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x61:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by DOWN_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_DOWN, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_DOWN, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x62:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by LEFT_DIRECT gesture wakeup.\n");

//                  input_report_key(g_InputDevice, KEY_LEFT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_LEFT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x63:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by RIGHT_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_RIGHT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_RIGHT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x64:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by m_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_M, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_M, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x65:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by W_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_W, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_W, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x66:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by C_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_C, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_C, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x67:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by e_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_E, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_E, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x68:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by V_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_V, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_V, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x69:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by O_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_O, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_O, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6A:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by S_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_S, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_S, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6B:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by Z_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_Z, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_Z, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6C:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE1_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE1_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER1, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER1, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6D:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE2_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE2_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER2, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER2, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6E:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE3_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE3_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER3, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER3, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
                case 0x6F:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE4_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE4_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER4, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER4, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x70:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE5_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE5_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER5, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER5, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x71:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE6_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE6_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER6, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER6, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x72:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE7_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE7_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER7, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER7, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x73:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE8_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE8_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER8, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER8, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x74:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE9_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE9_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER9, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER9, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x75:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE10_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE10_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER10, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER10, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x76:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE11_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE11_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER11, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER11, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x77:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE12_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE12_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER12, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER12, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x78:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE13_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE13_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER13, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER13, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x79:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE14_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE14_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER14, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER14, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7A:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE15_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE15_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER15, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER15, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7B:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE16_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE16_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER16, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER16, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7C:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE17_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE17_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER17, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER17, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7D:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE18_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE18_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER18, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER18, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7E:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE19_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE19_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER19, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER19, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7F:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE20_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE20_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER20, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER20, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x80:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE21_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE21_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER21, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER21, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x81:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE22_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE22_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER22, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER22, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x82:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE23_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE23_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER23, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER23, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x83:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE24_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE24_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER24, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER24, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x84:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE25_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE25_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER25, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER25, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x85:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE26_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE26_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER26, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER26, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x86:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE27_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE27_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER27, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER27, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x87:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE28_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE28_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER28, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER28, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x88:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE29_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE29_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER29, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER29, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x89:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE30_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE30_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER30, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER30, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8A:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE31_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE31_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER31, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER31, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8B:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE32_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE32_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER32, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER32, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8C:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE33_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE33_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER33, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER33, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8D:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE34_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE34_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER34, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER34, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8E:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE35_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE35_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER35, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER35, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8F:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE36_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE36_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER36, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER36, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x90:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE37_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE37_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER37, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER37, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x91:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE38_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE38_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER38, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER38, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x92:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE39_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE39_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER39, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER39, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x93:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE40_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE40_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER40, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER40, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x94:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE41_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE41_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER41, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER41, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x95:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE42_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE42_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER42, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER42, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x96:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE43_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE43_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER43, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER43, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x97:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE44_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE44_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER44, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER44, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x98:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE45_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE45_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER45, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER45, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x99:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE46_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE46_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER46, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER46, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9A:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE47_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE47_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER47, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER47, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9B:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE48_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE48_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER48, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER48, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9C:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE49_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE49_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER49, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER49, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9D:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE50_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE50_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER50, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER50, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9E:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE51_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE51_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER51, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER51, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
				case 0xFF://Gesture Fail
	            	_gGestureWakeupValue[1] = 0xFF;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_FAIL gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER51, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER51, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
                default:
                    _gGestureWakeupValue[0] = 0;
                    _gGestureWakeupValue[1] = 0;
                    DBG(&g_I2cClient->dev, "Un-supported gesture wakeup mode. Please check your device driver code.\n");
                    break;		
            }

            DBG(&g_I2cClient->dev, "_gGestureWakeupValue[0] = 0x%x\n", _gGestureWakeupValue[0]);
            DBG(&g_I2cClient->dev, "_gGestureWakeupValue[1] = 0x%x\n", _gGestureWakeupValue[1]);
        }
        else
        {
            DBG(&g_I2cClient->dev, "gesture wakeup packet format is incorrect.\n");
        }
        
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        // Notify android application to retrieve log data mode packet from device driver by sysfs.
        if (g_GestureKObj != NULL && pPacket[3] == PACKET_TYPE_GESTURE_DEBUG)
        {
            char *pEnvp[2];
            s32 nRetVal = 0;

            pEnvp[0] = "STATUS=GET_GESTURE_DEBUG";
            pEnvp[1] = NULL;

            nRetVal = kobject_uevent_env(g_GestureKObj, KOBJ_CHANGE, pEnvp);
            DBG(&g_I2cClient->dev, "kobject_uevent_env() nRetVal = %d\n", nRetVal);
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

        return -1;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    DBG(&g_I2cClient->dev, "received raw data from touch panel as following:\n");
    DBG(&g_I2cClient->dev, "pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x \n pPacket[5]=%x pPacket[6]=%x pPacket[7]=%x \n", \
                pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5], pPacket[6], pPacket[7]);

    if ((pPacket[nCheckSumIndex] == nCheckSum) && (pPacket[0] == 0x52))   // check the checksum of packet
    {
        nX = (((pPacket[1] & 0xF0) << 4) | pPacket[2]);         // parse the packet to coordinate
        nY = (((pPacket[1] & 0x0F) << 8) | pPacket[3]);

        nDeltaX = (((pPacket[4] & 0xF0) << 4) | pPacket[5]);
        nDeltaY = (((pPacket[4] & 0x0F) << 8) | pPacket[6]);

        DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);
        DBG(&g_I2cClient->dev, "[delta_x,delta_y]=[%d,%d]\n", nDeltaX, nDeltaY);

#ifdef CONFIG_SWAP_X_Y
        nTempY = nX;
        nTempX = nY;
        nX = nTempX;
        nY = nTempY;
        
        nTempY = nDeltaX;
        nTempX = nDeltaY;
        nDeltaX = nTempX;
        nDeltaY = nTempY;
#endif

#ifdef CONFIG_REVERSE_X
        nX = 2047 - nX;
        nDeltaX = 4095 - nDeltaX;
#endif

#ifdef CONFIG_REVERSE_Y
        nY = 2047 - nY;
        nDeltaY = 4095 - nDeltaY;
#endif

        /*
         * pPacket[0]:id, pPacket[1]~pPacket[3]:the first point abs, pPacket[4]~pPacket[6]:the relative distance between the first point abs and the second point abs
         * when pPacket[1]~pPacket[4], pPacket[6] is 0xFF, keyevent, pPacket[5] to judge which key press.
         * pPacket[1]~pPacket[6] all are 0xFF, release touch
        */
        if ((pPacket[1] == 0xFF) && (pPacket[2] == 0xFF) && (pPacket[3] == 0xFF) && (pPacket[4] == 0xFF) && (pPacket[6] == 0xFF))
        {
            pInfo->tPoint[0].nX = 0; // final X coordinate
            pInfo->tPoint[0].nY = 0; // final Y coordinate

            if ((pPacket[5] != 0x00) && (pPacket[5] != 0xFF)) /* pPacket[5] is key value */
            {   /* 0x00 is key up, 0xff is touch screen up */
#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d, pPacket[5] = 0x%x\n", g_EnableTpProximity, pPacket[5]);

                if (g_EnableTpProximity && ((pPacket[5] == 0x80) || (pPacket[5] == 0x40)))
                {
                    if (pPacket[5] == 0x80) // close to
                    {
                        g_FaceClosingTp = 1;

                        input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 0);
                        input_sync(g_ProximityInputDevice);
                    }
                    else if (pPacket[5] == 0x40) // far away
                    {
                        g_FaceClosingTp = 0;

                        input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 1);
                        input_sync(g_ProximityInputDevice);
                    }

                    DBG(&g_I2cClient->dev, "g_FaceClosingTp = %d\n", g_FaceClosingTp);
                   
                    return -1;
                }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
                if (g_EnableTpProximity && ((pPacket[5] == 0x80) || (pPacket[5] == 0x40)))
                {
                    int nErr;
                    hwm_sensor_data tSensorData;

                    if (pPacket[5] == 0x80) // close to
                    {
                        g_FaceClosingTp = 0;
                    }
                    else if (pPacket[5] == 0x40) // far away
                    {
                        g_FaceClosingTp = 1;
                    }
                    
                    DBG(&g_I2cClient->dev, "g_FaceClosingTp = %d\n", g_FaceClosingTp);

                    // map and store data to hwm_sensor_data
                    tSensorData.values[0] = DrvGetTpPsData();
                    tSensorData.value_divide = 1;
                    tSensorData.status = SENSOR_STATUS_ACCURACY_MEDIUM;
                    // let up layer to know
                    if ((nErr = hwmsen_get_interrupt_data(ID_PROXIMITY, &tSensorData)))
                    {
                        DBG(&g_I2cClient->dev, "call hwmsen_get_interrupt_data() failed = %d\n", nErr);
                    }
                    
                    return -1;
                }
#endif               
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

                /* 0x00 is key up, 0xff is touch screen up */
                DBG(&g_I2cClient->dev, "touch key down pPacket[5]=%d\n", pPacket[5]);

                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = pPacket[5];
                pInfo->nTouchKeyMode = 1;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                if (tpd_dts_data.use_tpd_button)
                {
                    if (pPacket[5] == 4) // TOUCH_KEY_HOME
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[1].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[1].key_y;
                    }
                    else if (pPacket[5] == 1) // TOUCH_KEY_MENU
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[0].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[0].key_y;
                    }           
                    else if (pPacket[5] == 2) // TOUCH_KEY_BACK
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[2].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[2].key_y;
                    }           
                    else if (pPacket[5] == 8) // TOUCH_KEY_SEARCH 
                    {	
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[3].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[3].key_y;
                    }
                    else
                    {
                        DBG(&g_I2cClient->dev, "multi-key is pressed.\n");

                        return -1;
                    }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                    _gCurrPress[0] = 1;
                    _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                }
#else
                if (pPacket[5] == 4) // TOUCH_KEY_HOME
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[1][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[1][1];
                }
                else if (pPacket[5] == 1) // TOUCH_KEY_MENU
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[0][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[0][1];
                }           
                else if (pPacket[5] == 2) // TOUCH_KEY_BACK
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[2][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[2][1];
                }           
                else if (pPacket[5] == 8) // TOUCH_KEY_SEARCH 
                {	
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[3][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[3][1];
                }
                else
                {
                    DBG(&g_I2cClient->dev, "multi-key is pressed.\n");

                    return -1;
                }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
            }
            else
            {   /* key up or touch up */
                DBG(&g_I2cClient->dev, "touch end\n");
                pInfo->nFingerNum = 0; //touch end
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;    
            }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            _gPrevTouchStatus = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL 
        }
        else
        {
            pInfo->nTouchKeyMode = 0; //Touch on screen...

//            if ((nDeltaX == 0) && (nDeltaY == 0))
            if (
#ifdef CONFIG_REVERSE_X
                (nDeltaX == 4095)
#else
                (nDeltaX == 0)
#endif
                &&
#ifdef CONFIG_REVERSE_Y
                (nDeltaY == 4095)
#else
                (nDeltaY == 0)
#endif
                )
            {   /* one touch point */
                pInfo->nFingerNum = 1; // one touch
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: [x,y]=[%d,%d]\n", __func__, nX, nY);
                DBG(&g_I2cClient->dev, "[%s]: point[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }
            else
            {   /* two touch points */
                u32 nX2, nY2;
                
                pInfo->nFingerNum = 2; // two touch
                /* Finger 1 */
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: point1[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);
                /* Finger 2 */
                if (nDeltaX > 2048)     // transform the unsigned value to signed value
                {
                    nDeltaX -= 4096;
                }
                
                if (nDeltaY > 2048)
                {
                    nDeltaY -= 4096;
                }

                nX2 = (u32)(nX + nDeltaX);
                nY2 = (u32)(nY + nDeltaY);

                pInfo->tPoint[1].nX = (nX2 * TOUCH_SCREEN_X_MAX) / TPD_WIDTH; 
                pInfo->tPoint[1].nY = (nY2 * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: point2[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[1].nX, pInfo->tPoint[1].nY);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            if (_gPrevTouchStatus == 1)
            {
                for (i = 0; i < MAX_TOUCH_NUM; i ++)
                {
                    szX[i] = pInfo->tPoint[i].nX;
                    szY[i] = pInfo->tPoint[i].nY;
                }
			
                if (/*(pInfo->nFingerNum == 1)&&*/nPrevTouchNum == 2)
                {
                    if (_DrvPointDistance(szX[0], szY[0], szPrevX[0], szPrevY[0]) > _DrvPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]))
                    {
                        nChangePoints = 1;
                    }
                }
                else if ((pInfo->nFingerNum == 2) && (nPrevTouchNum == 1))
                {
                    if (szPrevPress[0] == 1)
                    {
                        if(_DrvPointDistance(szX[0], szY[0], szPrevX[0] ,szPrevY[0]) > _DrvPointDistance(szX[1], szY[1], szPrevX[0], szPrevY[0]))
                        {
                            nChangePoints = 1;
                        }
                    }
                    else
                    {
                        if (_DrvPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]) < _DrvPointDistance(szX[1], szY[1], szPrevX[1], szPrevY[1]))
                        {
                            nChangePoints = 1;
                        }
                    }
                }
                else if ((pInfo->nFingerNum == 1) && (nPrevTouchNum == 1))
                {
                    if (_gCurrPress[0] != szPrevPress[0])
                    {
                        nChangePoints = 1;
                    }
                }
//                else if ((pInfo->nFingerNum == 2) && (nPrevTouchNum == 2))
//                {
//                }

                if (nChangePoints == 1)
                {
                    nTemp = _gCurrPress[0];
                    _gCurrPress[0] = _gCurrPress[1];
                    _gCurrPress[1] = nTemp;

                    nTemp = pInfo->tPoint[0].nX;
                    pInfo->tPoint[0].nX = pInfo->tPoint[1].nX;
                    pInfo->tPoint[1].nX = nTemp;

                    nTemp = pInfo->tPoint[0].nY;
                    pInfo->tPoint[0].nY = pInfo->tPoint[1].nY;
                    pInfo->tPoint[1].nY = nTemp;
                }
            }

            // Save current status
            for (i = 0; i < MAX_TOUCH_NUM; i ++)
            {
                szPrevPress[i] = _gCurrPress[i];
                szPrevX[i] = pInfo->tPoint[i].nX;
                szPrevY[i] = pInfo->tPoint[i].nY;
            }
            nPrevTouchNum = pInfo->nFingerNum;

            _gPrevTouchStatus = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
        }
        if (g_TouchKObj != NULL)
        {
            char szRspDemoModePacket[30] = {0};
            char szValue[3] = {0};
            char *pEnvp[3];
            s32 nRetVal = 0;  

            strcat(szRspDemoModePacket, "VALUE=");  

            for (i = 0; i < nLength; i ++)
            {
                sprintf(szValue, "%02x", g_DemoModePacket[i]);
                strcat(szRspDemoModePacket, szValue);
            }

            pEnvp[0] = "STATUS=GET_DEMO_MODE_PACKET";  
            pEnvp[1] = szRspDemoModePacket;  
            pEnvp[2] = NULL;
            DBG(&g_I2cClient->dev, "pEnvp[1] = %s\n", pEnvp[1]); // TODO : add for debug
    
            nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
            DBG(&g_I2cClient->dev, "kobject_uevent_env() STATUS=GET_DEMO_MODE_PACKET, nRetVal = %d\n", nRetVal);
        }
    }
    else if (pPacket[nCheckSumIndex] == nCheckSum && pPacket[0] == 0x62)
    {
        nX = ((pPacket[1] << 8) | pPacket[2]);  // Position_X
        nY = ((pPacket[3] << 8) | pPacket[4]);  // Position_Y

        nDeltaX = ((pPacket[13] << 8) | pPacket[14]); // Distance_X
        nDeltaY = ((pPacket[15] << 8) | pPacket[16]); // Distance_Y

        DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);
        DBG(&g_I2cClient->dev, "[delta_x,delta_y]=[%d,%d]\n", nDeltaX, nDeltaY);

#ifdef CONFIG_SWAP_X_Y
        nTempY = nX;
        nTempX = nY;
        nX = nTempX;
        nY = nTempY;
        
        nTempY = nDeltaX;
        nTempX = nDeltaY;
        nDeltaX = nTempX;
        nDeltaY = nTempY;
#endif

#ifdef CONFIG_REVERSE_X
        nX = 2047 - nX;
        nDeltaX = 4095 - nDeltaX;
#endif

#ifdef CONFIG_REVERSE_Y
        nY = 2047 - nY;
        nDeltaY = 4095 - nDeltaY;
#endif

        /*
         * pPacket[0]:id, pPacket[1]~pPacket[4]:the first point abs, pPacket[13]~pPacket[16]:the relative distance between the first point abs and the second point abs
         * when pPacket[1]~pPacket[7] is 0xFF, keyevent, pPacket[8] to judge which key press.
         * pPacket[1]~pPacket[8] all are 0xFF, release touch
         */
        if ((pPacket[1] == 0xFF) && (pPacket[2] == 0xFF) && (pPacket[3] == 0xFF) && (pPacket[4] == 0xFF) && (pPacket[5] == 0xFF) && (pPacket[6] == 0xFF) && (pPacket[7] == 0xFF))
        {
            pInfo->tPoint[0].nX = 0; // final X coordinate
            pInfo->tPoint[0].nY = 0; // final Y coordinate

            if ((pPacket[8] != 0x00) && (pPacket[8] != 0xFF)) /* pPacket[8] is key value */
            {   /* 0x00 is key up, 0xff is touch screen up */
                DBG(&g_I2cClient->dev, "touch key down pPacket[8]=%d\n", pPacket[8]);
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = pPacket[8];
                pInfo->nTouchKeyMode = 1;

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                pInfo->nFingerNum = 1;
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                if (tpd_dts_data.use_tpd_button)
                {
                    if (pPacket[8] == 4) // TOUCH_KEY_HOME
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[1].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[1].key_y;
                    }
                    else if (pPacket[8] == 1) // TOUCH_KEY_MENU
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[0].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[0].key_y;
                    }           
                    else if (pPacket[8] == 2) // TOUCH_KEY_BACK
                    {
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[2].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[2].key_y;
                    }           
                    else if (pPacket[8] == 8) // TOUCH_KEY_SEARCH 
                    {	
                        pInfo->tPoint[0].nX = tpd_dts_data.tpd_key_dim_local[3].key_x;
                        pInfo->tPoint[0].nY = tpd_dts_data.tpd_key_dim_local[3].key_y;
                    }
                    else
                    {
                        DBG(&g_I2cClient->dev, "multi-key is pressed.\n");

                        return -1;
                    }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                    _gCurrPress[0] = 1;
                    _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                }
#else
                if (pPacket[8] == 4) // TOUCH_KEY_HOME
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[1][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[1][1];
                }
                else if (pPacket[8] == 1) // TOUCH_KEY_MENU
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[0][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[0][1];
                }           
                else if (pPacket[8] == 2) // TOUCH_KEY_BACK
                {
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[2][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[2][1];
                }           
                else if (pPacket[8] == 8) // TOUCH_KEY_SEARCH 
                {	
                    pInfo->tPoint[0].nX = g_TpVirtualKeyDimLocal[3][0];
                    pInfo->tPoint[0].nY = g_TpVirtualKeyDimLocal[3][1];
                }
                else
                {
                    DBG(&g_I2cClient->dev, "multi-key is pressed.\n");

                    return -1;
                }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
            }
            else
            {   /* key up or touch up */
                DBG(&g_I2cClient->dev, "touch end\n");
                pInfo->nFingerNum = 0; //touch end
                pInfo->nTouchKeyCode = 0;
                pInfo->nTouchKeyMode = 0;    
            }

            if (g_TouchKObj != NULL)
            {
                char *pEnvp[2];
                s32 nRetVal = 0; 

                memcpy(g_LogModePacket, g_DebugModePacket, nLength); // Copy g_DebugModePacket to g_LogModePacket for avoiding the debug mode data which is received by MTPTool APK may be modified. 

                pEnvp[0] = "STATUS=GET_PACKET";  
                pEnvp[1] = NULL;  
    
                nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
                DBG(&g_I2cClient->dev, "kobject_uevent_env() nRetVal = %d\n", nRetVal);
            }
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            _gPrevTouchStatus = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL 
        }
        else
        {
            pInfo->nTouchKeyMode = 0; //Touch on screen...

//            if ((nDeltaX == 0) && (nDeltaY == 0))
            if (
#ifdef CONFIG_REVERSE_X
                (nDeltaX == 4095)
#else
                (nDeltaX == 0)
#endif
                &&
#ifdef CONFIG_REVERSE_Y
                (nDeltaY == 4095)
#else
                (nDeltaY == 0)
#endif
                )
            {   /* one touch point */
                pInfo->nFingerNum = 1; // one touch
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: [x,y]=[%d,%d]\n", __func__, nX, nY);
                DBG(&g_I2cClient->dev, "[%s]: point[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }
            else
            {   /* two touch points */
                u32 nX2, nY2;
                
                pInfo->nFingerNum = 2; // two touch
                /* Finger 1 */
                pInfo->tPoint[0].nX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                pInfo->tPoint[0].nY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: point1[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[0].nX, pInfo->tPoint[0].nY);
                /* Finger 2 */
                if (nDeltaX > 2048)     // transform the unsigned value to signed value
                {
                    nDeltaX -= 4096;
                }
                
                if (nDeltaY > 2048)
                {
                    nDeltaY -= 4096;
                }

                nX2 = (u32)(nX + nDeltaX);
                nY2 = (u32)(nY + nDeltaY);

                pInfo->tPoint[1].nX = (nX2 * TOUCH_SCREEN_X_MAX) / TPD_WIDTH; 
                pInfo->tPoint[1].nY = (nY2 * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "[%s]: point2[x,y]=[%d,%d]\n", __func__, pInfo->tPoint[1].nX, pInfo->tPoint[1].nY);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            if (_gPrevTouchStatus == 1)
            {
                for (i = 0; i < MAX_TOUCH_NUM; i ++)
                {
                    szX[i] = pInfo->tPoint[i].nX;
                    szY[i] = pInfo->tPoint[i].nY;
                }
			
                if (/*(pInfo->nFingerNum == 1)&&*/nPrevTouchNum == 2)
                {
                    if (_DrvPointDistance(szX[0], szY[0], szPrevX[0], szPrevY[0]) > _DrvPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]))
                    {
                        nChangePoints = 1;
                    }
                }
                else if ((pInfo->nFingerNum == 2) && (nPrevTouchNum == 1))
                {
                    if (szPrevPress[0] == 1)
                    {
                        if(_DrvPointDistance(szX[0], szY[0], szPrevX[0] ,szPrevY[0]) > _DrvPointDistance(szX[1], szY[1], szPrevX[0], szPrevY[0]))
                        {
                            nChangePoints = 1;
                        }
                    }
                    else
                    {
                        if (_DrvPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]) < _DrvPointDistance(szX[1], szY[1], szPrevX[1], szPrevY[1]))
                        {
                            nChangePoints = 1;
                        }
                    }
                }
                else if ((pInfo->nFingerNum == 1) && (nPrevTouchNum == 1))
                {
                    if (_gCurrPress[0] != szPrevPress[0])
                    {
                        nChangePoints = 1;
                    }
                }
//                else if ((pInfo->nFingerNum == 2) && (nPrevTouchNum == 2))
//                {
//                }

                if (nChangePoints == 1)
                {
                    nTemp = _gCurrPress[0];
                    _gCurrPress[0] = _gCurrPress[1];
                    _gCurrPress[1] = nTemp;

                    nTemp = pInfo->tPoint[0].nX;
                    pInfo->tPoint[0].nX = pInfo->tPoint[1].nX;
                    pInfo->tPoint[1].nX = nTemp;

                    nTemp = pInfo->tPoint[0].nY;
                    pInfo->tPoint[0].nY = pInfo->tPoint[1].nY;
                    pInfo->tPoint[1].nY = nTemp;
                }
            }

            // Save current status
            for (i = 0; i < MAX_TOUCH_NUM; i ++)
            {
                szPrevPress[i] = _gCurrPress[i];
                szPrevX[i] = pInfo->tPoint[i].nX;
                szPrevY[i] = pInfo->tPoint[i].nY;
            }
            nPrevTouchNum = pInfo->nFingerNum;

            _gPrevTouchStatus = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

            // Notify android application to retrieve log data mode packet from device driver by sysfs.   
            if (g_TouchKObj != NULL)
            {
                char *pEnvp[2];
                s32 nRetVal = 0; 

                memcpy(g_LogModePacket, g_DebugModePacket, nLength); // Copy g_DebugModePacket to g_LogModePacket for avoiding the debug mode data which is received by MTPTool APK may be modified. 

                pEnvp[0] = "STATUS=GET_PACKET";  
                pEnvp[1] = NULL;  
    
                nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
                DBG(&g_I2cClient->dev, "kobject_uevent_env() nRetVal = %d\n", nRetVal);
            }
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG22XX && IS_SELF_FREQ_SCAN_ENABLED)// && pPacket[nCheckSumIndex] == nCheckSum)// && pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_SELF_FREQ_SCAN)
    {
        // TODO :

        // Notify android application to retrieve log data mode packet from device driver by sysfs.   
        if (g_TouchKObj != NULL)
        {
            char *pEnvp[2];
            s32 nRetVal = 0; 
            memcpy(g_LogModePacket, g_DebugModePacket, nLength); // Copy g_DebugModePacket to g_LogModePacket for avoiding the self freq data which is received by MTPTool APK may be modified. 
			pEnvp[0] = "STATUS=GET_DEBUG_MODE_PACKET";   
            pEnvp[1] = NULL;  

            nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
            DBG(&g_I2cClient->dev, "kobject_uevent_env() nRetVal = %d\n", nRetVal);
        }
    }
    else
    {
        DBG(&g_I2cClient->dev, "pPacket[0]=0x%x, pPacket[7]=0x%x, nCheckSum=0x%x\n", pPacket[0], pPacket[7], nCheckSum);

        if (pPacket[nCheckSumIndex] != nCheckSum)
        {
            DBG(&g_I2cClient->dev, "WRONG CHECKSUM\n");
            return -1;
        }

        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE && pPacket[0] != 0x52)
        {
            DBG(&g_I2cClient->dev, "WRONG DEMO MODE HEADER\n");
            return -1;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && pPacket[0] != 0x62)
        {
            DBG(&g_I2cClient->dev, "WRONG DEBUG MODE HEADER\n");
            return -1;
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE && pPacket[0] != 0x62)
        {
            DBG(&g_I2cClient->dev, "WRONG RAW DATA MODE HEADER\n");
            return -1;
        }
    }

    return 0;
}

static s32 _DrvMutualParsePacket(u8 *pPacket, u16 nLength, MutualTouchInfo_t *pInfo) // for MSG28xx/MSG58xxA
{
    u32 i;
    u8 nCheckSum = 0;
    u32 nX = 0, nY = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
    if (g_IsEnableReportRate == 1)
    {
        if (g_InterruptCount == 4294967295UL)
        {
            g_InterruptCount = 0; // Reset count if overflow
            DBG(&g_I2cClient->dev, "g_InterruptCount reset to 0\n");
        }	

        if (g_InterruptCount == 0)
        {
            // Get start time
            do_gettimeofday(&g_StartTime);
    
            DBG(&g_I2cClient->dev, "Start time : %lu sec, %lu msec\n", g_StartTime.tv_sec,  g_StartTime.tv_usec); 
        }
        
        g_InterruptCount ++;

        DBG(&g_I2cClient->dev, "g_InterruptCount = %d\n", g_InterruptCount);
    }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

    DBG(&g_I2cClient->dev, "received raw data from touch panel as following:\n");
    DBG(&g_I2cClient->dev, "pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x \n pPacket[5]=%x pPacket[6]=%x pPacket[7]=%x pPacket[8]=%x \n", \
                pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5], pPacket[6], pPacket[7], pPacket[8]);

    if (IS_APK_PRINT_FIRMWARE_SPECIFIC_LOG_ENABLED)
    {
        if (pPacket[0] == 0x2C)
        {
            // Notify android application to retrieve firmware specific debug value packet from device driver by sysfs.   
            if (g_TouchKObj != NULL)
            {
                char szRspFwSpecificLogPacket[100] = {0};
                char szValue[3] = {0};
                char *pEnvp[3];
                s32 nRetVal = 0;  

                strcat(szRspFwSpecificLogPacket, "VALUE=");  

                for (i = 0; i < nLength; i ++)
                {
                    sprintf(szValue, "%02x", g_DemoModePacket[i]);
                    strcat(szRspFwSpecificLogPacket, szValue);
                }

                pEnvp[0] = "STATUS=GET_FW_LOG";  
                pEnvp[1] = szRspFwSpecificLogPacket;  
                pEnvp[2] = NULL;
                DBG(&g_I2cClient->dev, "pEnvp[1] = %s\n", pEnvp[1]); // TODO : add for debug
                DBG(&g_I2cClient->dev, "g_DemoModePacket[] = %s\n", g_DemoModePacket); // TODO : add for debug

                nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
                DBG(&g_I2cClient->dev, "kobject_uevent_env() STATUS=GET_FW_LOG, nRetVal = %d\n", nRetVal);
            }

            return -1;
        }
    }

    nCheckSum = _DrvCalculateCheckSum(&pPacket[0], (nLength-1));
    DBG(&g_I2cClient->dev, "checksum : [%x] == [%x]? \n", pPacket[nLength-1], nCheckSum);

    if (pPacket[nLength-1] != nCheckSum)
    {
        DBG(&g_I2cClient->dev, "WRONG CHECKSUM\n");
        return -1;
    }

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        u8 nWakeupMode = 0;
        u8 bIsCorrectFormat = 0;

        DBG(&g_I2cClient->dev, "received raw data from touch panel as following:\n");
        DBG(&g_I2cClient->dev, "pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x pPacket[5]=%x \n", \
                pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5]);

        if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x06 && pPacket[3] == PACKET_TYPE_GESTURE_WAKEUP) 
        {
            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;
        }
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        else if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x80 && pPacket[3] == PACKET_TYPE_GESTURE_DEBUG)
        {
            u32 a = 0;

            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;
            
            for (a = 0; a < 0x80; a ++)
            {
                g_LogGestureDebug[a] = pPacket[a];
            }

            if (!(pPacket[5] >> 7))// LCM Light Flag = 0
            {
                nWakeupMode = 0xFE;
                DBG(&g_I2cClient->dev, "gesture debug mode LCM flag = 0\n");
            }
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
        else if (pPacket[0] == 0xA7 && pPacket[1] == 0x00 && pPacket[2] == 0x80 && pPacket[3] == PACKET_TYPE_GESTURE_INFORMATION)
        {
            u32 a = 0;
            u32 nTmpCount = 0;
            u32 nWidth = 0;
            u32 nHeight = 0;

            nWakeupMode = pPacket[4];
            bIsCorrectFormat = 1;

            for (a = 0; a < 6; a ++)//header
            {
                g_LogGestureInfor[nTmpCount] = pPacket[a];
                nTmpCount++;
            }

            for (a = 6; a < 126; a = a+3)//parse packet to coordinate
            {
                u32 nTranX = 0;
                u32 nTranY = 0;
				
                _DrvConvertGestureInformationModeCoordinate(&pPacket[a], &nTranX, &nTranY);
                g_LogGestureInfor[nTmpCount] = nTranX;
                nTmpCount++;
                g_LogGestureInfor[nTmpCount] = nTranY;
                nTmpCount++;
            }
						
            nWidth = (((pPacket[12] & 0xF0) << 4) | pPacket[13]); //parse width & height
            nHeight = (((pPacket[12] & 0x0F) << 8) | pPacket[14]);

            DBG(&g_I2cClient->dev, "Before convert [width,height]=[%d,%d]\n", nWidth, nHeight);

            if ((pPacket[12] == 0xFF) && (pPacket[13] == 0xFF) && (pPacket[14] == 0xFF))
            {
                nWidth = 0; 
                nHeight = 0; 
            }
            else
            {
                nWidth = (nWidth * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
                nHeight = (nHeight * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
                DBG(&g_I2cClient->dev, "After convert [width,height]=[%d,%d]\n", nWidth, nHeight);
            }

            g_LogGestureInfor[10] = nWidth;
            g_LogGestureInfor[11] = nHeight;
    
            g_LogGestureInfor[nTmpCount] = pPacket[126]; //Dummy
            nTmpCount++;
            g_LogGestureInfor[nTmpCount] = pPacket[127]; //checksum
            nTmpCount++;
            DBG(&g_I2cClient->dev, "gesture information mode Count = %d\n", nTmpCount);
        }
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

        if (bIsCorrectFormat)
        {
            DBG(&g_I2cClient->dev, "nWakeupMode = 0x%x\n", nWakeupMode);

            switch (nWakeupMode)
            {
                case 0x58:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by DOUBLE_CLICK gesture wakeup.\n");

                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x60:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG;
                    
                    DBG(&g_I2cClient->dev, "Light up screen by UP_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_UP, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_UP, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x61:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by DOWN_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_DOWN, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_DOWN, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x62:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by LEFT_DIRECT gesture wakeup.\n");

//                  input_report_key(g_InputDevice, KEY_LEFT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_LEFT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x63:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by RIGHT_DIRECT gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_RIGHT, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_RIGHT, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x64:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by m_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_M, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_M, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x65:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by W_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_W, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_W, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;		
                case 0x66:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by C_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_C, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_C, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x67:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by e_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_E, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_E, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x68:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by V_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_V, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_V, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x69:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by O_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_O, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_O, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6A:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by S_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_S, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_S, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6B:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by Z_CHARACTER gesture wakeup.\n");

//                    input_report_key(g_InputDevice, KEY_Z, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, KEY_Z, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6C:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE1_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE1_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER1, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER1, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6D:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE2_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE2_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER2, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER2, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x6E:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE3_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE3_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER3, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER3, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
                case 0x6F:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE4_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE4_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER4, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER4, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x70:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE5_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE5_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER5, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER5, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x71:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE6_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE6_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER6, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER6, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x72:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE7_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE7_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER7, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER7, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x73:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE8_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE8_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER8, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER8, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x74:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE9_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE9_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER9, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER9, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x75:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE10_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE10_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER10, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER10, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x76:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE11_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE11_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER11, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER11, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x77:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE12_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE12_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER12, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER12, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x78:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE13_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE13_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER13, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER13, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x79:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE14_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE14_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER14, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER14, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7A:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE15_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE15_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER15, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER15, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7B:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE16_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE16_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER16, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER16, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7C:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE17_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE17_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER17, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER17, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7D:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE18_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE18_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER18, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER18, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7E:
                    _gGestureWakeupValue[0] = GESTURE_WAKEUP_MODE_RESERVE19_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE19_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER19, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER19, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x7F:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE20_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE20_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER20, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER20, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x80:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE21_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE21_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER21, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER21, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x81:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE22_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE22_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER22, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER22, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x82:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE23_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE23_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER23, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER23, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x83:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE24_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE24_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER24, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER24, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x84:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE25_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE25_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER25, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER25, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x85:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE26_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE26_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER26, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER26, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x86:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE27_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE27_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER27, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER27, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x87:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE28_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE28_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER28, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER28, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x88:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE29_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE29_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER29, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER29, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x89:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE30_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE30_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER30, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER30, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8A:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE31_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE31_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER31, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER31, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8B:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE32_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE32_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER32, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER32, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8C:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE33_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE33_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER33, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER33, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8D:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE34_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE34_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER34, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER34, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8E:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE35_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE35_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER35, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER35, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x8F:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE36_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE36_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER36, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER36, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x90:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE37_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE37_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER37, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER37, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x91:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE38_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE38_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER38, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER38, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x92:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE39_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE39_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER39, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER39, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x93:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE40_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE40_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER40, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER40, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x94:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE41_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE41_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER41, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER41, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x95:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE42_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE42_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER42, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER42, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x96:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE43_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE43_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER43, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER43, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x97:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE44_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE44_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER44, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER44, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x98:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE45_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE45_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER45, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER45, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x99:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE46_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE46_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER46, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER46, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9A:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE47_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE47_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER47, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER47, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9B:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE48_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE48_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER48, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER48, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9C:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE49_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE49_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER49, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER49, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9D:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE50_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE50_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER50, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER50, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
                case 0x9E:
                    _gGestureWakeupValue[1] = GESTURE_WAKEUP_MODE_RESERVE51_FLAG;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_RESERVE51_FLAG gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER51, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER51, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
                case 0xFF://Gesture Fail
                    _gGestureWakeupValue[1] = 0xFF;

                    DBG(&g_I2cClient->dev, "Light up screen by GESTURE_WAKEUP_MODE_FAIL gesture wakeup.\n");

//                    input_report_key(g_InputDevice, RESERVER51, 1);
                    input_report_key(g_InputDevice, KEY_POWER, 1);
                    input_sync(g_InputDevice);
//                    input_report_key(g_InputDevice, RESERVER51, 0);
                    input_report_key(g_InputDevice, KEY_POWER, 0);
                    input_sync(g_InputDevice);
                    break;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

                default:
                    _gGestureWakeupValue[0] = 0;
                    _gGestureWakeupValue[1] = 0;
                    DBG(&g_I2cClient->dev, "Un-supported gesture wakeup mode. Please check your device driver code.\n");
                    break;		
            }

            DBG(&g_I2cClient->dev, "_gGestureWakeupValue = 0x%x, 0x%x\n", _gGestureWakeupValue[0], _gGestureWakeupValue[1]);
        }
        else
        {
            DBG(&g_I2cClient->dev, "gesture wakeup packet format is incorrect.\n");
        }

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        // Notify android application to retrieve log data mode packet from device driver by sysfs.
        if (g_GestureKObj != NULL && pPacket[3] == PACKET_TYPE_GESTURE_DEBUG)
        {
            char *pEnvp[2];
            s32 nRetVal = 0;

            pEnvp[0] = "STATUS=GET_GESTURE_DEBUG";
            pEnvp[1] = NULL;

            nRetVal = kobject_uevent_env(g_GestureKObj, KOBJ_CHANGE, pEnvp);
            DBG(&g_I2cClient->dev, "kobject_uevent_env() STATUS=GET_GESTURE_DEBUG, nRetVal = %d\n", nRetVal);
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

        return -1;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP


    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {    	
        if (g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE && pPacket[0] != 0x5A)
        {
            {
                DBG(&g_I2cClient->dev, "WRONG DEMO MODE HEADER\n");
                return -1;
            }
        }
        else if (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE && (pPacket[0] != 0xA7 && pPacket[3] != PACKET_TYPE_TOOTH_PATTERN && 
        pPacket[3] != PACKET_TYPE_CSUB_PATTERN && pPacket[3] != PACKET_TYPE_FOUT_PATTERN  && pPacket[3] != PACKET_TYPE_FREQ_PATTERN))
        {
            DBG(&g_I2cClient->dev, "WRONG DEBUG MODE HEADER\n");
            return -1;
        }
    }
    else
    {
        if (pPacket[0] != 0x5A)
        {           
            {
                DBG(&g_I2cClient->dev, "WRONG DEMO MODE HEADER\n");
                return -1;        
            }
        }
    } //IS_FIRMWARE_DATA_LOG_ENABLED

    // Process raw data...
    if (pPacket[0] == 0x5A)
    {
        {
            for (i = 0; i < MAX_TOUCH_NUM; i ++)
            {
                if ((pPacket[(4*i)+1] == 0xFF) && (pPacket[(4*i)+2] == 0xFF) && (pPacket[(4*i)+3] == 0xFF))
                {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                    _gCurrentTouch[i] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                    
                    continue;
                }
		
                nX = (((pPacket[(4*i)+1] & 0xF0) << 4) | (pPacket[(4*i)+2]));
                nY = (((pPacket[(4*i)+1] & 0x0F) << 8) | (pPacket[(4*i)+3]));
                
                pInfo->tPoint[pInfo->nCount].nX = nX * TOUCH_SCREEN_X_MAX / TPD_WIDTH;
                pInfo->tPoint[pInfo->nCount].nY = nY * TOUCH_SCREEN_Y_MAX / TPD_HEIGHT;
                pInfo->tPoint[pInfo->nCount].nP = pPacket[4*(i+1)];
                pInfo->tPoint[pInfo->nCount].nId = i;
		
                DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);
                DBG(&g_I2cClient->dev, "point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
		
                pInfo->nCount ++;

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrentTouch[i] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }
        }

        if (IS_FIRMWARE_DATA_LOG_ENABLED)
        {    
            // Notify android application to retrieve demo mode packet from device driver by sysfs.   
            if (g_TouchKObj != NULL)
            {
                char szRspDemoModePacket[100] = {0};
                char szValue[3] = {0};
                char *pEnvp[3];
                s32 nRetVal = 0;  

                strcat(szRspDemoModePacket, "VALUE=");  

                for (i = 0; i < nLength; i ++)
                {
                    sprintf(szValue, "%02x", g_DemoModePacket[i]);
                    strcat(szRspDemoModePacket, szValue);
                }

                pEnvp[0] = "STATUS=GET_DEMO_MODE_PACKET";  
                pEnvp[1] = szRspDemoModePacket;  
                pEnvp[2] = NULL;
                DBG(&g_I2cClient->dev, "pEnvp[1] = %s\n", pEnvp[1]); // TODO : add for debug
        
                nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
                DBG(&g_I2cClient->dev, "kobject_uevent_env() STATUS=GET_DEMO_MODE_PACKET, nRetVal = %d\n", nRetVal);
            }
        }
    }
    else if (pPacket[0] == 0xA7 && (pPacket[3] == PACKET_TYPE_TOOTH_PATTERN || pPacket[3] == PACKET_TYPE_CSUB_PATTERN || pPacket[3] == PACKET_TYPE_FOUT_PATTERN || pPacket[3] == PACKET_TYPE_FREQ_PATTERN))
    {
        for (i = 0; i < MAX_TOUCH_NUM; i ++)
        {
            if ((pPacket[(3*i)+5] == 0xFF) && (pPacket[(3*i)+6] == 0xFF) && (pPacket[(3*i)+7] == 0xFF))
            {
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrentTouch[i] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

                continue;
            }
		
            nX = (((pPacket[(3*i)+5] & 0xF0) << 4) | (pPacket[(3*i)+6]));
            nY = (((pPacket[(3*i)+5] & 0x0F) << 8) | (pPacket[(3*i)+7]));

            pInfo->tPoint[pInfo->nCount].nX = nX * TOUCH_SCREEN_X_MAX / TPD_WIDTH;
            pInfo->tPoint[pInfo->nCount].nY = nY * TOUCH_SCREEN_Y_MAX / TPD_HEIGHT;
            pInfo->tPoint[pInfo->nCount].nP = 1;
            pInfo->tPoint[pInfo->nCount].nId = i;
		
            DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);
            DBG(&g_I2cClient->dev, "point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
		
            pInfo->nCount ++;

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            _gCurrentTouch[i] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
        }

        // Notify android application to retrieve debug mode packet from device driver by sysfs.   
        if (g_TouchKObj != NULL)
        {
            char *pEnvp[2];
            s32 nRetVal = 0;  

            memcpy(g_LogModePacket, g_DebugModePacket, nLength); // Copy g_DebugModePacket to g_LogModePacket for avoiding the debug mode data which is received by MTPTool APK may be modified. 

            pEnvp[0] = "STATUS=GET_DEBUG_MODE_PACKET";  
            pEnvp[1] = NULL;  
            
            nRetVal = kobject_uevent_env(g_TouchKObj, KOBJ_CHANGE, pEnvp); 
            DBG(&g_I2cClient->dev, "kobject_uevent_env() STATUS=GET_DEBUG_MODE_PACKET, nRetVal = %d\n", nRetVal);
        }
    }

#ifdef CONFIG_TP_HAVE_KEY
    if (pPacket[0] == 0x5A)
    {
        u8 nButton = pPacket[nLength-2]; //Since the key value is stored in 0th~3th bit of variable "button", we can only retrieve 0th~3th bit of it. 

//        if (nButton)
        if (nButton != 0xFF)
        {
            DBG(&g_I2cClient->dev, "button = %x\n", nButton);

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
            DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d, pPacket[nLength-2] = 0x%x\n", g_EnableTpProximity, pPacket[nLength-2]);

            if (g_EnableTpProximity && ((pPacket[nLength-2] == 0x80) || (pPacket[nLength-2] == 0x40)))
            {
                if (pPacket[nLength-2] == 0x80) // close to
                {
                    g_FaceClosingTp = 1;

                    input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 0);
                    input_sync(g_ProximityInputDevice);
                }
                else if (pPacket[nLength-2] == 0x40) // far away
                {
                    g_FaceClosingTp = 0;

                    input_report_abs(g_ProximityInputDevice, ABS_DISTANCE, 1);
                    input_sync(g_ProximityInputDevice);
                }

                DBG(&g_I2cClient->dev, "g_FaceClosingTp = %d\n", g_FaceClosingTp);
               
                return -1;
            }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
            if (g_EnableTpProximity && ((pPacket[nLength-2] == 0x80) || (pPacket[nLength-2] == 0x40)))
            {
                int nErr;
                hwm_sensor_data tSensorData;

                if (pPacket[nLength-2] == 0x80) // close to
                {
                    g_FaceClosingTp = 0;
                }
                else if (pPacket[nLength-2] == 0x40) // far away
                {
                    g_FaceClosingTp = 1;
                }
                
                DBG(&g_I2cClient->dev, "g_FaceClosingTp = %d\n", g_FaceClosingTp);

                // map and store data to hwm_sensor_data
                tSensorData.values[0] = DrvGetTpPsData();
                tSensorData.value_divide = 1;
                tSensorData.status = SENSOR_STATUS_ACCURACY_MEDIUM;
                // let up layer to know
                if ((nErr = hwmsen_get_interrupt_data(ID_PROXIMITY, &tSensorData)))
                {
                    DBG(&g_I2cClient->dev, "call hwmsen_get_interrupt_data() failed = %d\n", nErr);
                }
                
                return -1;
            }
#endif               
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

            for (i = 0; i < MAX_KEY_NUM; i ++)
            {
                if ((nButton & (1<<i)) == (1<<i))
                {
                    if (pInfo->nKeyCode == 0)
                    {
                        pInfo->nKeyCode = i;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                        DBG(&g_I2cClient->dev, "key[%d]=%d\n", i, g_TpVirtualKey[i]);

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                        if (tpd_dts_data.use_tpd_button)
                        {
                            DBG(&g_I2cClient->dev, "key[%d]=%d\n", i, tpd_dts_data.tpd_key_local[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                            pInfo->nKeyCode = 0xFF;
                            pInfo->tPoint[pInfo->nCount].nX = tpd_dts_data.tpd_key_dim_local[i].key_x;
                            pInfo->tPoint[pInfo->nCount].nY = tpd_dts_data.tpd_key_dim_local[i].key_y;
                            pInfo->tPoint[pInfo->nCount].nP = 1;
                            pInfo->tPoint[pInfo->nCount].nId = pInfo->nCount;
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                            _gCurrentTouch[pInfo->nCount] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                            DBG(&g_I2cClient->dev, "virtual key point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
                            pInfo->nCount ++;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                        }
#else
                        DBG(&g_I2cClient->dev, "key[%d]=%d\n", i, g_TpVirtualKey[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                        pInfo->nKeyCode = 0xFF;
                        pInfo->tPoint[pInfo->nCount].nX = g_TpVirtualKeyDimLocal[i][0];
                        pInfo->tPoint[pInfo->nCount].nY = g_TpVirtualKeyDimLocal[i][1];
                        pInfo->tPoint[pInfo->nCount].nP = 1;
                        pInfo->tPoint[pInfo->nCount].nId = pInfo->nCount;
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                        _gCurrentTouch[pInfo->nCount] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                        DBG(&g_I2cClient->dev, "virtual key point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
                        pInfo->nCount ++;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif 
                    }
                    else
                    {
                        /// if pressing multi-key => no report
                        pInfo->nKeyCode = 0xFF;
                    }
                }
            }
        }
        else
        {
            pInfo->nKeyCode = 0xFF;
        }
    }
    else if ((pPacket[0] == 0xA7 && pPacket[3] == PACKET_TYPE_TOOTH_PATTERN))
    {
        u8 nButton = 0xFF;
        nButton = pPacket[4]; // The pressed virtual key is stored in 5th byte for debug mode packet 0xA7.

            if (nButton != 0xFF)
            {
                DBG(&g_I2cClient->dev, "button = %x\n", nButton);

                for (i = 0; i < MAX_KEY_NUM; i ++)
                {
                    if ((nButton & (1<<i)) == (1<<i))
                    {
                        if (pInfo->nKeyCode == 0)
                        {
                            pInfo->nKeyCode = i;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
                            DBG(&g_I2cClient->dev, "key[%d]=%d\n", i, g_TpVirtualKey[i]);

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
                            if (tpd_dts_data.use_tpd_button)
                            {
                                DBG(&g_I2cClient->dev, "key[%d]=%d\n", i, tpd_dts_data.tpd_key_local[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                                pInfo->nKeyCode = 0xFF;
                                pInfo->tPoint[pInfo->nCount].nX = tpd_dts_data.tpd_key_dim_local[i].key_x;
                                pInfo->tPoint[pInfo->nCount].nY = tpd_dts_data.tpd_key_dim_local[i].key_y;
                                pInfo->tPoint[pInfo->nCount].nP = 1;
                                pInfo->tPoint[pInfo->nCount].nId = pInfo->nCount;
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                                _gCurrentTouch[pInfo->nCount] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                                DBG(&g_I2cClient->dev, "virtual key point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
                                pInfo->nCount ++;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                            }
#else
                            DBG(&g_I2cClient->dev, "key[%d]=%d\n", i, g_TpVirtualKey[i]);

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
                            pInfo->nKeyCode = 0xFF;
                            pInfo->tPoint[pInfo->nCount].nX = g_TpVirtualKeyDimLocal[i][0];
                            pInfo->tPoint[pInfo->nCount].nY = g_TpVirtualKeyDimLocal[i][1];
                            pInfo->tPoint[pInfo->nCount].nP = 1;
                            pInfo->tPoint[pInfo->nCount].nId = pInfo->nCount;
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                            _gCurrentTouch[pInfo->nCount] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
                            DBG(&g_I2cClient->dev, "virtual key point[%d] : (%d,%d) = %d\n", pInfo->tPoint[pInfo->nCount].nId, pInfo->tPoint[pInfo->nCount].nX, pInfo->tPoint[pInfo->nCount].nY, pInfo->tPoint[pInfo->nCount].nP);
                            pInfo->nCount ++;
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif 
                        }
                        else
                        {
                            /// if pressing multi-key => no report
                            pInfo->nKeyCode = 0xFF;
                        }
                    }
                }
            }
            else
            {
                pInfo->nKeyCode = 0xFF;
        }
    }
#endif //CONFIG_TP_HAVE_KEY

    return 0;
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
void DrvOpenGestureWakeup(u32 *pMode)
{
    u8 szTxData[4] = {0};
    u32 i = 0;
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2118A || g_ChipType == CHIP_TYPE_ILI2117A)
    {
        DBG(&g_I2cClient->dev, "wakeup mode 0 = 0x%x\n", pMode[0]);
        DBG(&g_I2cClient->dev, "wakeup mode 1 = 0x%x\n", pMode[1]);

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
        szTxData[0] = 0x59;
        szTxData[1] = 0x00;
        szTxData[2] = ((pMode[1] & 0xFF000000) >> 24);
        szTxData[3] = ((pMode[1] & 0x00FF0000) >> 16);

        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Enable gesture wakeup index 0 success\n");
                break;
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup index 0 failed\n");
        }

        szTxData[0] = 0x59;
        szTxData[1] = 0x01;
        szTxData[2] = ((pMode[1] & 0x0000FF00) >> 8);
        szTxData[3] = ((pMode[1] & 0x000000FF) >> 0);
	
        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Enable gesture wakeup index 1 success\n");
                break;
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup index 1 failed\n");
        }

        szTxData[0] = 0x59;
        szTxData[1] = 0x02;
        szTxData[2] = ((pMode[0] & 0xFF000000) >> 24);
        szTxData[3] = ((pMode[0] & 0x00FF0000) >> 16);
    
        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Enable gesture wakeup index 2 success\n");
                break;
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup index 2 failed\n");
        }

        szTxData[0] = 0x59;
        szTxData[1] = 0x03;
        szTxData[2] = ((pMode[0] & 0x0000FF00) >> 8);
        szTxData[3] = ((pMode[0] & 0x000000FF) >> 0);
    
        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Enable gesture wakeup index 3 success\n");
                break;
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup index 3 failed\n");
        }

        g_GestureWakeupFlag = 1; // gesture wakeup is enabled

#else
	
        szTxData[0] = 0x58;
        szTxData[1] = ((pMode[0] & 0x0000FF00) >> 8);
        szTxData[2] = ((pMode[0] & 0x000000FF) >> 0);

        while (i < 5)
        {
            mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); // delay 20ms
            rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
            if (rc > 0)
            {
                DBG(&g_I2cClient->dev, "Enable gesture wakeup success\n");
                break;
            }

            i++;
        }
        if (i == 5)
        {
            DBG(&g_I2cClient->dev, "Enable gesture wakeup failed\n");
        }

        g_GestureWakeupFlag = 1; // gesture wakeup is enabled
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
    }
}

void DrvCloseGestureWakeup(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    g_GestureWakeupFlag = 0; // gesture wakeup is disabled
}

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
void DrvOpenGestureDebugMode(u8 nGestureFlag)
{
    u8 szTxData[3] = {0};
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DBG(&g_I2cClient->dev, "Gesture Flag = 0x%x\n", nGestureFlag);

    szTxData[0] = 0x30;
    szTxData[1] = 0x01;
    szTxData[2] = nGestureFlag;

    mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
    rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "Enable gesture debug mode failed\n");
    }
    else
    {
        g_GestureDebugMode = 1; // gesture debug mode is enabled

        DBG(&g_I2cClient->dev, "Enable gesture debug mode success\n");
    }
}

void DrvCloseGestureDebugMode(void)
{
    u8 szTxData[3] = {0};
    s32 rc;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    szTxData[0] = 0x30;
    szTxData[1] = 0x00;
    szTxData[2] = 0x00;

    mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE);
    rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 3);
    if (rc < 0)
    {
        DBG(&g_I2cClient->dev, "Disable gesture debug mode failed\n");
    }
    else
    {
        g_GestureDebugMode = 0; // gesture debug mode is disabled

        DBG(&g_I2cClient->dev, "Disable gesture debug mode success\n");
    }
}
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static void _DrvConvertGestureInformationModeCoordinate(u8 *pRawData, u32 *pTranX, u32 *pTranY)
{
   	u32 nX;
   	u32 nY;
#ifdef CONFIG_SWAP_X_Y
   	u32 nTempX;
   	u32 nTempY;
#endif

   	nX = (((pRawData[0] & 0xF0) << 4) | pRawData[1]);         // parse the packet to coordinate
    nY = (((pRawData[0] & 0x0F) << 8) | pRawData[2]);

    DBG(&g_I2cClient->dev, "[x,y]=[%d,%d]\n", nX, nY);

#ifdef CONFIG_SWAP_X_Y
    nTempY = nX;
   	nTempX = nY;
    nX = nTempX;
    nY = nTempY;
#endif

#ifdef CONFIG_REVERSE_X
    nX = 2047 - nX;
#endif

#ifdef CONFIG_REVERSE_Y
    nY = 2047 - nY;
#endif

   	/*
   	 * pRawData[0]~pRawData[2] : the point abs,
   	 * pRawData[0]~pRawData[2] all are 0xFF, release touch
   	 */
    if ((pRawData[0] == 0xFF) && (pRawData[1] == 0xFF) && (pRawData[2] == 0xFF))
    {
   	    *pTranX = 0; // final X coordinate
        *pTranY = 0; // final Y coordinate
    }
    else
    {
     	  /* one touch point */
        *pTranX = (nX * TOUCH_SCREEN_X_MAX) / TPD_WIDTH;
        *pTranY = (nY * TOUCH_SCREEN_Y_MAX) / TPD_HEIGHT;
        DBG(&g_I2cClient->dev, "[%s]: [x,y]=[%d,%d]\n", __func__, nX, nY);
        DBG(&g_I2cClient->dev, "[%s]: point[x,y]=[%d,%d]\n", __func__, *pTranX, *pTranY);
    }
}
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static struct class *touchscreen_class;

static ssize_t gesture_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	if (g_GestureState)
		return sprintf(buf, "gesture: on\n");
	else
		return sprintf(buf, "gesture: off\n");
}
static ssize_t gesture_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	if(!strncmp(buf, "on", 2))
		g_GestureState = true;
	else if(!strncmp(buf, "off", 3))
		g_GestureState = false;
	pr_debug("buf = %s, g_GestureState = %d, count = %zu\n", buf, g_GestureState, count);
	return count;
}
static CLASS_ATTR(gesture, S_IRUSR|S_IWUSR, gesture_show, gesture_store);

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

//------------------------------------------------------------------------------//

static s32 _DrvCreateProcfsDirEntry(void)
{
    s32 nRetVal = 0;
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    u8 *pGesturePath = NULL;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
    u8 *pDevicePath = NULL;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gProcClassEntry = proc_mkdir(PROC_NODE_CLASS, NULL);

    _gProcMsTouchScreenMsg20xxEntry = proc_mkdir(PROC_NODE_MS_TOUCHSCREEN_MSG20XX, _gProcClassEntry);

    _gProcDeviceEntry = proc_mkdir(PROC_NODE_DEVICE, _gProcMsTouchScreenMsg20xxEntry);
#ifdef CONFIG_ENABLE_ITO_MP_TEST
    _gProcMpTestCustomisedEntry = proc_create(PROC_NODE_MP_TEST_CUSTOMISED, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTestCustomised);
    if (NULL == _gProcMpTestCustomisedEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST_CUSTOMISED);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_CUSTOMISED);
    }
#endif
    _gProcChipTypeEntry = proc_create(PROC_NODE_CHIP_TYPE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcChipType);
    if (NULL == _gProcChipTypeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_CHIP_TYPE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_CHIP_TYPE);
    }

    _gProcFirmwareDataEntry = proc_create(PROC_NODE_FIRMWARE_DATA, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareData);
    if (NULL == _gProcFirmwareDataEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_DATA);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_DATA);
    }

    _gProcApkFirmwareUpdateEntry = proc_create(PROC_NODE_FIRMWARE_UPDATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcApkFirmwareUpdate);
    if (NULL == _gProcApkFirmwareUpdateEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_UPDATE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_UPDATE);
    }

    _gProcCustomerFirmwareVersionEntry = proc_create(PROC_NODE_CUSTOMER_FIRMWARE_VERSION, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcCustomerFirmwareVersion);
    if (NULL == _gProcCustomerFirmwareVersionEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_CUSTOMER_FIRMWARE_VERSION);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_CUSTOMER_FIRMWARE_VERSION);
    }

    _gProcPlatformFirmwareVersionEntry = proc_create(PROC_NODE_PLATFORM_FIRMWARE_VERSION, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcPlatformFirmwareVersion);
    if (NULL == _gProcPlatformFirmwareVersionEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_PLATFORM_FIRMWARE_VERSION);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_PLATFORM_FIRMWARE_VERSION);
    }

    _gProcDeviceDriverVersionEntry = proc_create(PROC_NODE_DEVICE_DRIVER_VERSION, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcDeviceDriverVersion);
    if (NULL == _gProcDeviceDriverVersionEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_DEVICE_DRIVER_VERSION);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_DEVICE_DRIVER_VERSION);
    }

    _gProcSdCardFirmwareUpdateEntry = proc_create(PROC_NODE_SD_CARD_FIRMWARE_UPDATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcSdCardFirmwareUpdate);
    if (NULL == _gProcSdCardFirmwareUpdateEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_SD_CARD_FIRMWARE_UPDATE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_SD_CARD_FIRMWARE_UPDATE);
    }

    _gProcFirmwareDebugEntry = proc_create(PROC_NODE_FIRMWARE_DEBUG, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareDebug);
    if (NULL == _gProcFirmwareDebugEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_DEBUG);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_DEBUG);
    }

    _gProcFirmwareSetDebugValueEntry = proc_create(PROC_NODE_FIRMWARE_SET_DEBUG_VALUE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareSetDebugValue);
    if (NULL == _gProcFirmwareSetDebugValueEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_SET_DEBUG_VALUE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SET_DEBUG_VALUE);
    }

    _gProcFirmwareSmBusDebugEntry = proc_create(PROC_NODE_FIRMWARE_SMBUS_DEBUG, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareSmBusDebug);
    if (NULL == _gProcFirmwareSmBusDebugEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_SMBUS_DEBUG);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SMBUS_DEBUG);
    }

    _gProcFirmwareSetDQMemValueEntry = proc_create(PROC_NODE_FIRMWARE_SET_DQMEM_VALUE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareSetDQMemValue);
    if (NULL == _gProcFirmwareSetDQMemValueEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_SET_DQMEM_VALUE);
    }
    else
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SET_DQMEM_VALUE);
    }

#ifdef CONFIG_ENABLE_ITO_MP_TEST
    _gProcMpTestEntry = proc_create(PROC_NODE_MP_TEST, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTest);
    if (NULL == _gProcMpTestEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_MP_TEST);
    }

    _gProcMpTestLogEntry = proc_create(PROC_NODE_MP_TEST_LOG, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTestLog);
    if (NULL == _gProcMpTestLogEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST_LOG);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_LOG);
    }

    _gProcMpTestFailChannelEntry = proc_create(PROC_NODE_MP_TEST_FAIL_CHANNEL, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTestFailChannel);
    if (NULL == _gProcMpTestFailChannelEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST_FAIL_CHANNEL);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_FAIL_CHANNEL);
    }

    _gProcMpTestScopeEntry = proc_create(PROC_NODE_MP_TEST_SCOPE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTestScope);
    if (NULL == _gProcMpTestScopeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST_SCOPE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_SCOPE);
    }

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    _gProcMpTestLogALLEntry = proc_create(PROC_NODE_MP_TEST_LOG_ALL, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcMpTestLogAll);
    if (NULL == _gProcMpTestLogALLEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_MP_TEST_LOG_ALL);
    }
    else
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_MP_TEST_LOG_ALL);
    }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#endif //CONFIG_ENABLE_ITO_MP_TEST

    _gProcFirmwareModeEntry = proc_create(PROC_NODE_FIRMWARE_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareMode);
    if (NULL == _gProcFirmwareModeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_MODE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_MODE);
    }

    _gProcFirmwareSensorEntry = proc_create(PROC_NODE_FIRMWARE_SENSOR, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwareSensor);
    if (NULL == _gProcFirmwareSensorEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_SENSOR);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_SENSOR);
    }

    _gProcFirmwarePacketHeaderEntry = proc_create(PROC_NODE_FIRMWARE_PACKET_HEADER, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFirmwarePacketHeader);
    if (NULL == _gProcFirmwarePacketHeaderEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FIRMWARE_PACKET_HEADER);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FIRMWARE_PACKET_HEADER);
    }

    /* create a kset with the name of "kset_example" which is located under /sys/kernel/ */
    g_TouchKSet = kset_create_and_add("kset_example", NULL, kernel_kobj);
    if (!g_TouchKSet)
    {
        DBG(&g_I2cClient->dev, "*** kset_create_and_add() failed, nRetVal = %d ***\n", nRetVal);

        nRetVal = -ENOMEM;
    }

    g_TouchKObj = kobject_create();
    if (!g_TouchKObj)
    {
        DBG(&g_I2cClient->dev, "*** kobject_create() failed, nRetVal = %d ***\n", nRetVal);

        nRetVal = -ENOMEM;
        kset_unregister(g_TouchKSet);
        g_TouchKSet = NULL;
    }

    g_TouchKObj->kset = g_TouchKSet;

    nRetVal = kobject_add(g_TouchKObj, NULL, "%s", "kobject_example");
    if (nRetVal != 0)
    {
        DBG(&g_I2cClient->dev, "*** kobject_add() failed, nRetVal = %d ***\n", nRetVal);

        kobject_put(g_TouchKObj);
        g_TouchKObj = NULL;
        kset_unregister(g_TouchKSet);
        g_TouchKSet = NULL;
    }
    
    /* create the files associated with this kobject */
    nRetVal = sysfs_create_group(g_TouchKObj, &attr_group);
    if (nRetVal != 0)
    {
        DBG(&g_I2cClient->dev, "*** sysfs_create_file() failed, nRetVal = %d ***\n", nRetVal);

        kobject_put(g_TouchKObj);
        g_TouchKObj = NULL;
        kset_unregister(g_TouchKSet);
        g_TouchKSet = NULL;
    }
    
    pDevicePath = kobject_get_path(g_TouchKObj, GFP_KERNEL);
    DBG(&g_I2cClient->dev, "DEVPATH = %s\n", pDevicePath);


    _gProcQueryFeatureSupportStatusEntry = proc_create(PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcQueryFeatureSupportStatus);
    if (NULL == _gProcQueryFeatureSupportStatusEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS);
    }
    
    _gProcChangeFeatureSupportStatusEntry = proc_create(PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcChangeFeatureSupportStatus);
    if (NULL == _gProcChangeFeatureSupportStatusEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS);
    }

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    _gProcGestureWakeupModeEntry = proc_create(PROC_NODE_GESTURE_WAKEUP_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcGestureWakeupMode);
    if (NULL == _gProcGestureWakeupModeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_GESTURE_WAKEUP_MODE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_GESTURE_WAKEUP_MODE);
    }

	touchscreen_class = class_create(THIS_MODULE, "touchscreen");
	if (IS_ERR_OR_NULL(touchscreen_class)) {
		DBG(&g_I2cClient->dev, "%s: create class error!\n", __func__);
		return -ENOMEM;
	}

	nRetVal = class_create_file(touchscreen_class, &class_attr_gesture);
	if (nRetVal < 0) {
		DBG(&g_I2cClient->dev, "%s create gesture file failed!\n", __func__);
		class_destroy(touchscreen_class);
		return -ENOMEM;
	}

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    _gProcGestureDebugModeEntry = proc_create(PROC_NODE_GESTURE_DEBUG_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcGestureDebugMode);
    if (NULL == _gProcGestureDebugModeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_GESTURE_DEBUG_MODE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_GESTURE_DEBUG_MODE);
    }

    /* create a kset with the name of "kset_gesture" which is located under /sys/kernel/ */
    g_GestureKSet = kset_create_and_add("kset_gesture", NULL, kernel_kobj);
    if (!g_GestureKSet)
    {
        DBG(&g_I2cClient->dev, "*** kset_create_and_add() failed, nRetVal = %d ***\n", nRetVal);
        nRetVal = -ENOMEM;
    }

    g_GestureKObj = kobject_create();
    if (!g_GestureKObj)
    {
        DBG(&g_I2cClient->dev, "*** kobject_create() failed, nRetVal = %d ***\n", nRetVal);

        nRetVal = -ENOMEM;
        kset_unregister(g_GestureKSet);
        g_GestureKSet = NULL;
    }

    g_GestureKObj->kset = g_GestureKSet;

    nRetVal = kobject_add(g_GestureKObj, NULL, "%s", "kobject_gesture");
    if (nRetVal != 0)
    {
        DBG(&g_I2cClient->dev, "*** kobject_add() failed, nRetVal = %d ***\n", nRetVal);

        kobject_put(g_GestureKObj);
        g_GestureKObj = NULL;
        kset_unregister(g_GestureKSet);
        g_GestureKSet = NULL;
    }

    /* create the files associated with this g_GestureKObj */
    nRetVal = sysfs_create_group(g_GestureKObj, &gestureattr_group);
    if (nRetVal != 0)
    {
        DBG(&g_I2cClient->dev, "*** sysfs_create_file() failed, nRetVal = %d ***\n", nRetVal);

        kobject_put(g_GestureKObj);
        g_GestureKObj = NULL;
        kset_unregister(g_GestureKSet);
        g_GestureKSet = NULL;
    }

    pGesturePath = kobject_get_path(g_GestureKObj, GFP_KERNEL);
    DBG(&g_I2cClient->dev, "DEVPATH = %s\n", pGesturePath);
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
    _gProcGestureInforModeEntry = proc_create(PROC_NODE_GESTURE_INFORMATION_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcGestureInforMode);
    if (NULL == _gProcGestureInforModeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_GESTURE_INFORMATION_MODE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_GESTURE_INFORMATION_MODE);
    }
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
    _gProcReportRateEntry = proc_create(PROC_NODE_REPORT_RATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcReportRate);
    if (NULL == _gProcReportRateEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_REPORT_RATE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_REPORT_RATE);
    }
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

    _gProcGloveModeEntry = proc_create(PROC_NODE_GLOVE_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcGloveMode);
    if (NULL == _gProcGloveModeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_GLOVE_MODE);
    }
    else
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_GLOVE_MODE);
    }

    _gProcOpenGloveModeEntry = proc_create(PROC_NODE_OPEN_GLOVE_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcOpenGloveMode);
    if (NULL == _gProcOpenGloveModeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_OPEN_GLOVE_MODE);
    }
    else
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_OPEN_GLOVE_MODE);
    }

    _gProcCloseGloveModeEntry = proc_create(PROC_NODE_CLOSE_GLOVE_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcCloseGloveMode);
    if (NULL == _gProcCloseGloveModeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_CLOSE_GLOVE_MODE);
    }
    else
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_CLOSE_GLOVE_MODE);
    }

    _gProcLeatherSheathModeEntry = proc_create(PROC_NODE_LEATHER_SHEATH_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcLeatherSheathMode);
    if (NULL == _gProcLeatherSheathModeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_LEATHER_SHEATH_MODE);
    }
    else
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_LEATHER_SHEATH_MODE);
    }
    _gProcFilmModeEntry = proc_create(PROC_NODE_CONTROL_FILM_MODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcFilmMode);
    if (NULL == _gProcFilmModeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_CONTROL_FILM_MODE);
    }
    else
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_CONTROL_FILM_MODE);
    }

#ifdef CONFIG_ENABLE_JNI_INTERFACE
    _gProcJniMethodEntry = proc_create(PROC_NODE_JNI_NODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcJniMethod);
    if (NULL == _gProcJniMethodEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_JNI_NODE);
    }
    else
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_JNI_NODE);
    }
#endif //CONFIG_ENABLE_JNI_INTERFACE

    _gProcSeLinuxLimitFirmwareUpdateEntry = proc_create(PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcSeLinuxLimitFirmwareUpdate);
    if (NULL == _gProcSeLinuxLimitFirmwareUpdateEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE);
    }

    _gProcForceFirmwareUpdateEntry = proc_create(PROC_NODE_FORCE_FIRMWARE_UPDATE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcForceFirmwareUpdate);
    if (NULL == _gProcForceFirmwareUpdateEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_FORCE_FIRMWARE_UPDATE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_FORCE_FIRMWARE_UPDATE);
    }

    _gProcTrimCodeEntry = proc_create(PROC_NODE_TRIM_CODE, PROCFS_AUTHORITY, _gProcDeviceEntry, &_gProcTrimCode);
    if (NULL == _gProcTrimCodeEntry)
    {
        DBG(&g_I2cClient->dev, "Failed to create procfs file node(%s)!\n", PROC_NODE_TRIM_CODE);
    }   
    else 
    {
        DBG(&g_I2cClient->dev, "Create procfs file node(%s) OK!\n", PROC_NODE_TRIM_CODE);
    }
    return nRetVal;
}

#ifdef CONFIG_ENABLE_NOTIFIER_FB
int MsDrvInterfaceTouchDeviceFbNotifierCallback(struct notifier_block *pSelf, unsigned long nEvent, void *pData)
{
    struct fb_event *pEventData = pData;
    int *pBlank;

    if (pEventData && pEventData->data && (nEvent == FB_EVENT_BLANK || nEvent == FB_EARLY_EVENT_BLANK))
    {
        pBlank = pEventData->data;

		DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

        if ((*pBlank == FB_BLANK_UNBLANK || *pBlank == FB_BLANK_NORMAL) && nEvent == FB_EARLY_EVENT_BLANK)
        {
            DrvDisableFingerTouchReport();    
            DBG(&g_I2cClient->dev, "*** %s() TP Resume ***\n", __func__);

            if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
            {
                DBG(&g_I2cClient->dev, "Not allow to power on/off touch ic while update firmware.\n");
                return 0;
            }

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
            if (g_EnableTpProximity == 1)
            {
                DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d\n", g_EnableTpProximity);
                return 0;
            }
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION
            
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
            {
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
                if (g_GestureDebugMode == 1)
                {
                    DrvCloseGestureDebugMode();
                }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

                if (g_GestureWakeupFlag == 1)
                {
                    DBG(&g_I2cClient->dev, "g_GestureState = %d, g_ChipType = 0x%X\n", g_GestureState, g_ChipType);
                    DrvCloseGestureWakeup();
                    disable_irq_wake(_gIrq);
                }
                else
                {
                    DrvEnableFingerTouchReport(); 
                }
            }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP    
            {
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
                DrvTouchDeviceRegulatorPowerOn(true);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON               
                DrvTouchDevicePowerOn(); 
            }   
			_DrvMutualGetFirmwareInfo(&g_MutualFirmwareInfo);
#ifdef CONFIG_ENABLE_CHARGER_DETECTION 
            {
                u8 szChargerStatus[20] = {0};
     
                DrvReadFile(POWER_SUPPLY_BATTERY_STATUS_PATCH, szChargerStatus, 20);
            
                DBG(&g_I2cClient->dev, "*** Battery Status : %s ***\n", szChargerStatus);
            
                g_ForceUpdate = 1; // Set flag to force update charger status
                
                if (strstr(szChargerStatus, "Charging") != NULL || strstr(szChargerStatus, "Full") != NULL || strstr(szChargerStatus, "Fully charged") != NULL) // Charging
                {
                    DrvChargerDetection(1); // charger plug-in
                }
                else // Not charging
                {
                    DrvChargerDetection(0); // charger plug-out
                }

                g_ForceUpdate = 0; // Clear flag after force update charger status
            }           
#endif //CONFIG_ENABLE_CHARGER_DETECTION

            if (g_IsEnableGloveMode == 1)
            {
                _DrvOpenGloveMode();
            }

            if (g_IsEnableLeatherSheathMode == 1)
            {
                _DrvOpenLeatherSheathMode();
            }

            if (IS_FIRMWARE_DATA_LOG_ENABLED)    
            {
                DrvRestoreFirmwareModeToLogDataMode(); // Mark this function call for avoiding device driver may spend longer time to resume from suspend state.
            } //IS_FIRMWARE_DATA_LOG_ENABLED

#ifndef CONFIG_ENABLE_GESTURE_WAKEUP
            DrvEnableFingerTouchReport(); 
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
            g_IsEnableChargerPlugInOutCheck = 1;
            queue_delayed_work(g_ChargerPlugInOutCheckWorkqueue, &g_ChargerPlugInOutCheckWork, CHARGER_DETECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
            g_IsEnableEsdCheck = 1;
            queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_ESD_PROTECTION
        }
        else if (*pBlank == FB_BLANK_POWERDOWN && nEvent == FB_EVENT_BLANK)
        {
            DBG(&g_I2cClient->dev, "*** %s() TP Suspend ***\n", __func__);
            
#ifdef CONFIG_ENABLE_CHARGER_DETECTION
            g_IsEnableChargerPlugInOutCheck = 0;
            cancel_delayed_work_sync(&g_ChargerPlugInOutCheckWork);
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
            g_IsEnableEsdCheck = 0;
            cancel_delayed_work_sync(&g_EsdCheckWork);
#endif //CONFIG_ENABLE_ESD_PROTECTION

            if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
            {
                DBG(&g_I2cClient->dev, "Not allow to power on/off touch ic while update firmware.\n");
                return 0;
            }

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
            if (g_EnableTpProximity == 1)
            {
                DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d\n", g_EnableTpProximity);
                return 0;
            }
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
            {
                DBG(&g_I2cClient->dev, " g_GestureState = %d, g_ChipType = 0x%X\n", g_GestureState, g_ChipType);
                if (g_GestureWakeupMode[0] != 0x00000000 || g_GestureWakeupMode[1] != 0x00000000)
                {
                    DrvOpenGestureWakeup(&g_GestureWakeupMode[0]);
                    enable_irq_wake(_gIrq);
                    return 0;
                }
               
            }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

            DrvFingerTouchReleased(0, 0, 0); // Send touch end for clearing point touch
            input_sync(g_InputDevice);

            DrvDisableFingerTouchReport();    
            {
                DrvTouchDevicePowerOff(); 
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
                DrvTouchDeviceRegulatorPowerOn(false);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
            }    
        }
    }

    return 0;
}

#else

#ifndef CONFIG_HAS_EARLYSUSPEND
void MsDrvInterfaceTouchDeviceSuspend(struct device *pDevice)
#else
void MsDrvInterfaceTouchDeviceSuspend(struct early_suspend *pSuspend)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    u32 i = 0;
    DBG(&g_I2cClient->dev, "*** %s() *** %d \n", __func__,i);

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
    g_IsEnableChargerPlugInOutCheck = 0;
    cancel_delayed_work_sync(&g_ChargerPlugInOutCheckWork);
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 0;
    cancel_delayed_work_sync(&g_EsdCheckWork);
#endif //CONFIG_ENABLE_ESD_PROTECTION

    if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
    {
        DBG(&g_I2cClient->dev, "Not allow to power on/off touch ic while update firmware.\n");
        return;
    }

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
    if (g_EnableTpProximity == 1)
    {
        DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d\n", g_EnableTpProximity);
        return;
    }
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    {
        DBG(&g_I2cClient->dev, " g_GestureState = %d, g_ChipType = 0x%X\n", g_GestureState, g_ChipType);
        if (g_GestureWakeupMode[0] != 0x00000000 || g_GestureWakeupMode[1] != 0x00000000)
        {
            DrvOpenGestureWakeup(&g_GestureWakeupMode[0]);
            enable_irq_wake(_gIrq);
            return;
        }
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
    DrvFingerTouchReleased(0, 0, 0); // Send touch end for clearing point touch
    input_sync(g_InputDevice);

    DrvDisableFingerTouchReport();      
    {
        DrvTouchDevicePowerOff(); 
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
        DrvTouchDeviceRegulatorPowerOn(false);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON               
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
    input_report_key(g_InputDevice, BTN_TOUCH, 0);

    for (i = 0; i < MAX_TOUCH_NUM; i ++) 
    {
        DrvFingerTouchReleased(0, 0, i);
    }

    input_report_key(g_InputDevice, BTN_TOOL_FINGER, 0);
#else // TYPE A PROTOCOL
    DrvFingerTouchReleased(0, 0, 0);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
    input_sync(g_InputDevice);
    }    
}

#ifndef CONFIG_HAS_EARLYSUSPEND
void MsDrvInterfaceTouchDeviceResume(struct device *pDevice)
#else
void MsDrvInterfaceTouchDeviceResume(struct early_suspend *pSuspend)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
    {
        DBG(&g_I2cClient->dev, "Not allow to power on/off touch ic while update firmware.\n");
        return;
    }

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
    if (g_EnableTpProximity == 1)
    {
        DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d\n", g_EnableTpProximity);
        return;
    }
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    {
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        if (g_GestureDebugMode == 1)
        {
            DrvCloseGestureDebugMode();
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

        if (g_GestureWakeupFlag == 1)
        {
            DBG(&g_I2cClient->dev, "g_GestureState = %d, g_ChipType = 0x%X\n", g_GestureState, g_ChipType);
            DrvCloseGestureWakeup();
            disable_irq_wake(_gIrq);
        }
        else
        {
            DrvEnableFingerTouchReport(); 
        }
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP   
    {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
        DrvTouchDeviceRegulatorPowerOn(true);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON               
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
        DrvTouchDevicePowerOn(); 
    }   
    
#ifdef CONFIG_ENABLE_CHARGER_DETECTION 
    {
        u8 szChargerStatus[20] = {0};
 
        DrvReadFile(POWER_SUPPLY_BATTERY_STATUS_PATCH, szChargerStatus, 20);
        
        DBG(&g_I2cClient->dev, "*** Battery Status : %s ***\n", szChargerStatus);
        
        g_ForceUpdate = 1; // Set flag to force update charger status

        if (strstr(szChargerStatus, "Charging") != NULL || strstr(szChargerStatus, "Full") != NULL || strstr(szChargerStatus, "Fully charged") != NULL) // Charging
        {
            DrvChargerDetection(1); // charger plug-in
        }
        else // Not charging
        {
            DrvChargerDetection(0); // charger plug-out
        }

        g_ForceUpdate = 0; // Clear flag after force update charger status
    }           
#endif //CONFIG_ENABLE_CHARGER_DETECTION

    if (g_IsEnableGloveMode == 1)
    {
        _DrvOpenGloveMode();
    }

    if (g_IsEnableLeatherSheathMode == 1)
    {
        _DrvOpenLeatherSheathMode();
    }

    if (IS_FIRMWARE_DATA_LOG_ENABLED)    
    {
        DrvRestoreFirmwareModeToLogDataMode(); // Mark this function call for avoiding device driver may spend longer time to resume from suspend state.
    } //IS_FIRMWARE_DATA_LOG_ENABLED

#ifndef CONFIG_ENABLE_GESTURE_WAKEUP
    DrvEnableFingerTouchReport(); 
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
    g_IsEnableChargerPlugInOutCheck = 1;
    queue_delayed_work(g_ChargerPlugInOutCheckWorkqueue, &g_ChargerPlugInOutCheckWork, CHARGER_DETECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 1;
    queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_ESD_PROTECTION
}
#endif //CONFIG_ENABLE_NOTIFIER_FB

/* probe function is used for matching and initializing input device */
s32 /*__devinit*/ MsDrvInterfaceTouchDeviceProbe(struct i2c_client *pClient, const struct i2c_device_id *pDeviceId)
{
    s32 nRetVal = 0;
    u16 nMajor = 0, nMinor = 0;
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
    {
            return -EIO;
    }

    DrvMutexVariableInitialize(); 

    DrvTouchDeviceRequestGPIO(pClient);

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    DrvTouchDeviceRegulatorPowerOn(true);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

    DrvTouchDevicePowerOn();
    DBG(&g_I2cClient->dev, "***  int_status = %d ***\n", gpio_get_value(MS_TS_MSG_IC_GPIO_INT));
    nRetVal = DrvTouchDeviceInitialize();
    if (nRetVal == -ENODEV)
    {
        DrvTouchDeviceRemove(pClient);
        return nRetVal;
    }

    DrvInputDeviceInitialize(pClient); 

    DBG(&g_I2cClient->dev, "***  int_status = %d ***\n", gpio_get_value(MS_TS_MSG_IC_GPIO_INT));
    DrvTouchDeviceRegisterEarlySuspend();
    //platform FW version V01.010.03 is FW get driver version check point
    if(g_ChipType == CHIP_TYPE_MSG58XXA)
    {
        DrvCheckFWSupportDriver();
    }
	DrvGetPlatformFirmwareVersion(&_gPlatformFwVersion);
	DBG(&g_I2cClient->dev, "*** %s() _gPlatformFwVersion = %s ***\n", __func__, _gPlatformFwVersion);
	DrvGetCustomerFirmwareVersion(&nMajor, &nMinor, &_gFwVersion);
	if(g_FwVersionFlag)
	{
		printk("*** ILITEK %s() Major = %d, Minor = %d.%d, _gFwVersion = %s ***\n", __func__, nMajor, (nMinor & 0xFF), ((nMinor >> 8) & 0xFF), _gFwVersion);
	}
	else
	{
		printk("*** ILITEK %s() Major = %d, Minor = %d, _gFwVersion = %s ***\n", __func__, nMajor, nMinor, _gFwVersion);
	}
    DrvTouchDeviceRegisterFingerTouchInterruptHandler();
#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
	DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DrvCheckFirmwareUpdateBySwId();
#else
    DrvEnableFingerTouchReport();
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
    INIT_DELAYED_WORK(&g_ChargerPlugInOutCheckWork, _DrvChargerPlugInOutCheck);
    g_ChargerPlugInOutCheckWorkqueue = create_workqueue("charger_plug_in_out_check");
    queue_delayed_work(g_ChargerPlugInOutCheckWorkqueue, &g_ChargerPlugInOutCheckWork, CHARGER_DETECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    INIT_DELAYED_WORK(&g_EsdCheckWork, DrvEsdCheck);
    g_EsdCheckWorkqueue = create_workqueue("esd_check");
    queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_ESD_PROTECTION

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
	device_init_wakeup(&g_I2cClient->dev, 1);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
    memcpy(version,  _gFwVersion,  63);
    snprintf(lcdname, 63, "%s", mtkfb_find_lcm_driver());
    printk("firefly LCD name:%s\n", lcdname);

    REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();

    DBG(&g_I2cClient->dev, "*** ILITEK/MStar touch driver registered ***\n");
    return nRetVal;
}

/* remove function is triggered when the input device is removed from input sub-system */
s32 /*__devexit*/ MsDrvInterfaceTouchDeviceRemove(struct i2c_client *pClient)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvTouchDeviceRemove(pClient);
}

void MsDrvInterfaceTouchDeviceSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvSetIicDataRate(pClient, nIicDataRate);
}    
