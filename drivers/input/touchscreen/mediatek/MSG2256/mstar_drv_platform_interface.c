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
 * @file    mstar_drv_platform_interface.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_platform_interface.h"
#include "mstar_drv_main.h"
#include "mstar_drv_ic_fw_porting_layer.h"
#include "mstar_drv_platform_porting_layer.h"
#include "mstar_drv_utility_adaption.h"

#ifdef CONFIG_ENABLE_HOTKNOT
#include "mstar_drv_hotknot.h"
#endif //CONFIG_ENABLE_HOTKNOT

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern u32 g_GestureWakeupMode[2];
extern u8 g_GestureWakeupFlag;

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
extern u8 g_GestureDebugFlag;
extern u8 g_GestureDebugMode;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
extern u8 g_EnableTpProximity;
static u8 g_ps_flag = 0;
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_ENABLE_GLOVE_MODE
extern u8 g_IsEnableGloveMode;
#endif //CONFIG_ENABLE_GLOVE_MODE

extern u8 g_IsUpdateFirmware;

extern struct input_dev *g_InputDevice;
extern struct i2c_client *g_I2cClient;
extern u32 SLAVE_I2C_ID_DWI2C;
#ifdef CONFIG_ENABLE_HOTKNOT
extern u8 g_HotKnotState;
//extern u32 SLAVE_I2C_ID_DWI2C;
#endif //CONFIG_ENABLE_HOTKNOT

#ifdef CONFIG_ENABLE_CHARGER_DETECTION
extern u8 g_ForceUpdate;
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_ESD_PROTECTION
extern int g_IsEnableEsdCheck;
extern struct delayed_work g_EsdCheckWork;
extern struct workqueue_struct *g_EsdCheckWorkqueue;
#endif //CONFIG_ENABLE_ESD_PROTECTION

extern u8 IS_FIRMWARE_DATA_LOG_ENABLED;


/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/


/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

#ifdef CONFIG_ENABLE_HOTKNOT
static u8 _gAMStartCmd[4] = {HOTKNOT_SEND, ADAPTIVEMOD_BEGIN, 0, 0};
#endif //CONFIG_ENABLE_HOTKNOT

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/
#define CONFIG_ENABLE_HEADPHONES_DETECTION
#ifdef CONFIG_ENABLE_HEADPHONES_DETECTION

static u8 bSuspendHeadphones = 0;
static struct hrtimer headphonesTimer;
static struct workqueue_struct *headphonesWq;
static struct work_struct headphonesWork;

static u8 DrvGetHeadphonesStateFromSys(void)
{
                u8 szHeadphonesState[5] = {0};
     
                DrvCommonReadFile("/sys/class/switch/h2w/state", szHeadphonesState, 5);
            
                //DBG(&g_I2cClient->dev, "*** headphones get state : %s ***\n", szHeadphonesState);
                //printk("*** headphones get state : %s ***\n", szHeadphonesState);
                            
                if (strstr(szHeadphonesState, "1") != NULL || strstr(szHeadphonesState, "2") != NULL) 
                {
                    return 1;
                }
                else 
                {
                    return 0;
                }
 }

static void DrvSetHeadphonesStateToFw(u8 bPlugIn)
{
	 s32 rc = -1;
	 u8 szTxData[4] = {0};

    	 szTxData[0] = 0x52;
   	 szTxData[1] = 0x14;
	 szTxData[2] = 0x01;
	 
	if(bPlugIn)
	{
		 szTxData[3] = 1;
	}
	else 
	{
		szTxData[3] = 0;
	}

	 mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); 
        rc = IicWriteData(SLAVE_I2C_ID_DWI2C, &szTxData[0], 4);
	 mdelay(I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE); 

       printk("***headphones set rc=%d;data=%d ***\n", rc,szTxData[3]);
 	
}

static enum hrtimer_restart headphones_timer_func(struct hrtimer *timer)
{   
	queue_work(headphonesWq, &headphonesWork);
	return HRTIMER_NORESTART;
}

 static void headphones_work_func(struct work_struct *work)
{
	static u8 lastHeadphonesState = 0;
	u8 currHeadphonesState = DrvGetHeadphonesStateFromSys();
	
	if(lastHeadphonesState!=currHeadphonesState)
	{
		DrvSetHeadphonesStateToFw(currHeadphonesState);
	}
	
 	lastHeadphonesState = currHeadphonesState;

        hrtimer_start(&headphonesTimer, ktime_set(0, 125000000), HRTIMER_MODE_REL);

}




static void headphones_init(void)
{
	headphonesWq = create_singlethread_workqueue("headphones detection");
	INIT_WORK(&headphonesWork, headphones_work_func);

	hrtimer_init(&headphonesTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	headphonesTimer.function = headphones_timer_func;
	hrtimer_start(&headphonesTimer, ktime_set(1, 0), HRTIMER_MODE_REL);
}
#endif
#ifdef CONFIG_ENABLE_NOTIFIER_FB
int MsDrvInterfaceTouchDeviceFbNotifierCallback(struct notifier_block *pSelf, unsigned long nEvent, void *pData)
{
    struct fb_event *pEventData = pData;
    int *pBlank;

    if (pEventData && pEventData->data && nEvent == FB_EVENT_BLANK)
    {
        pBlank = pEventData->data;

        if (*pBlank == FB_BLANK_UNBLANK)
        {
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
#ifdef CONFIG_ENABLE_HOTKNOT
            if (g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE && g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_AFTER_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT
            {
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
                if (g_GestureDebugMode == 1)
                {
                    DrvIcFwLyrCloseGestureDebugMode();
                }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

                if (g_GestureWakeupFlag == 1)
                {
                    DrvIcFwLyrCloseGestureWakeup();
                }
                else
                {
                    DrvPlatformLyrEnableFingerTouchReport(); 
                }
            }
#ifdef CONFIG_ENABLE_HOTKNOT
            else    // Enable touch in hotknot transfer mode
            {
                DrvPlatformLyrEnableFingerTouchReport();     
            }
#endif //CONFIG_ENABLE_HOTKNOT
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
    
#ifdef CONFIG_ENABLE_HOTKNOT
            if (g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE && g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_AFTER_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT        
            {
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
                DrvPlatformLyrTouchDeviceRegulatorPowerOn(true);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON               
                DrvPlatformLyrTouchDevicePowerOn(); 
            }   
  #ifdef CONFIG_ENABLE_HEADPHONES_DETECTION
     if(bSuspendHeadphones)
    {
    	  DrvSetHeadphonesStateToFw(DrvGetHeadphonesStateFromSys());
         bSuspendHeadphones = 0;
	  hrtimer_start(&headphonesTimer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }
#endif
    
#ifdef CONFIG_ENABLE_CHARGER_DETECTION 
            {
                u8 szChargerStatus[20] = {0};
     
                DrvCommonReadFile("/sys/class/power_supply/battery/status", szChargerStatus, 20);
            
                DBG(&g_I2cClient->dev, "*** Battery Status : %s ***\n", szChargerStatus);
            
                g_ForceUpdate = 1; // Set flag to force update charger status
                
                if (strstr(szChargerStatus, "Charging") != NULL || strstr(szChargerStatus, "Full") != NULL || strstr(szChargerStatus, "Fully charged") != NULL) // Charging
                {
                    DrvFwCtrlChargerDetection(1); // charger plug-in
                }
                else // Not charging
                {
                    DrvFwCtrlChargerDetection(0); // charger plug-out
                }

                g_ForceUpdate = 0; // Clear flag after force update charger status
            }           
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_GLOVE_MODE
            if (g_IsEnableGloveMode == 1)
            {
                DrvIcFwLyrOpenGloveMode();
            }
#endif //CONFIG_ENABLE_GLOVE_MODE

            if (IS_FIRMWARE_DATA_LOG_ENABLED)    
            {
                DrvIcFwLyrRestoreFirmwareModeToLogDataMode(); // Mark this function call for avoiding device driver may spend longer time to resume from suspend state.
            } //IS_FIRMWARE_DATA_LOG_ENABLED

#ifndef CONFIG_ENABLE_GESTURE_WAKEUP
            DrvPlatformLyrEnableFingerTouchReport(); 
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_ESD_PROTECTION
            g_IsEnableEsdCheck = 1;
            queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_ESD_PROTECTION
        }
        else if (*pBlank == FB_BLANK_POWERDOWN)
        {
            DBG(&g_I2cClient->dev, "*** %s() TP Suspend ***\n", __func__);
            
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
#ifdef CONFIG_ENABLE_HEADPHONES_DETECTION
       cancel_work_sync(&headphonesWork);
	hrtimer_cancel(&headphonesTimer);
       bSuspendHeadphones = 1;
#endif

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_HOTKNOT
            if (g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE && g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_AFTER_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT
            {
                if (g_GestureWakeupMode[0] != 0x00000000 || g_GestureWakeupMode[1] != 0x00000000)
                {
                    DrvIcFwLyrOpenGestureWakeup(&g_GestureWakeupMode[0]);
                    return 0;
                }
            }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_HOTKNOT
            if (g_HotKnotState == HOTKNOT_BEFORE_TRANS_STATE || g_HotKnotState == HOTKNOT_TRANS_STATE || g_HotKnotState == HOTKNOT_AFTER_TRANS_STATE)
            {
                IicWriteData(SLAVE_I2C_ID_DWI2C, &_gAMStartCmd[0], 4); 
            }
#endif //CONFIG_ENABLE_HOTKNOT 

            DrvPlatformLyrFingerTouchReleased(0, 0, 0); // Send touch end for clearing point touch
            input_sync(g_InputDevice);

            DrvPlatformLyrDisableFingerTouchReport();

#ifdef CONFIG_ENABLE_HOTKNOT
            if (g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE && g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_AFTER_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT        
            {
                DrvPlatformLyrTouchDevicePowerOff(); 
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
                DrvPlatformLyrTouchDeviceRegulatorPowerOn(false);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
            }    
        }
    }

    return 0;
}

#else

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
void MsDrvInterfaceTouchDeviceSuspend(struct device *pDevice)
#else
void MsDrvInterfaceTouchDeviceSuspend(struct early_suspend *pSuspend)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    g_IsEnableEsdCheck = 0;
    cancel_delayed_work_sync(&g_EsdCheckWork);
#endif //CONFIG_ENABLE_ESD_PROTECTION

    if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
    {
        //DBG(&g_I2cClient->dev, "Not allow to power on/off touch ic while update firmware.\n");
	TPD_DMESG("tp suspend firmware\n");
        return;
    }
	TPD_DMESG("tp suspend start\n");
#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
    if (g_EnableTpProximity == 1)
    {
	g_ps_flag = 1;
        //DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d\n", g_EnableTpProximity);
	TPD_DMESG("tp suspend ps return\n");
        return;
    }
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_ENABLE_HEADPHONES_DETECTION
       cancel_work_sync(&headphonesWork);
	hrtimer_cancel(&headphonesTimer);
       bSuspendHeadphones = 1;
#endif


#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_HOTKNOT
    if (g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE && g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_AFTER_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT
    {
        if (g_GestureWakeupMode[0] != 0x00000000 || g_GestureWakeupMode[1] != 0x00000000)
        {
            DrvIcFwLyrOpenGestureWakeup(&g_GestureWakeupMode[0]);
            return;
        }
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_HOTKNOT
    if (g_HotKnotState == HOTKNOT_BEFORE_TRANS_STATE || g_HotKnotState == HOTKNOT_TRANS_STATE || g_HotKnotState == HOTKNOT_AFTER_TRANS_STATE)
    {
        IicWriteData(SLAVE_I2C_ID_DWI2C, &_gAMStartCmd[0], 4); 
    }
#endif //CONFIG_ENABLE_HOTKNOT  

    DrvPlatformLyrFingerTouchReleased(0, 0, 0); // Send touch end for clearing point touch
    input_sync(g_InputDevice);

    DrvPlatformLyrDisableFingerTouchReport();
    tpd_gpio_as_int(2);
#ifdef CONFIG_ENABLE_HOTKNOT
    if (g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE && g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_AFTER_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT        
    {
        DrvPlatformLyrTouchDevicePowerOff(); 
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
        DrvPlatformLyrTouchDeviceRegulatorPowerOn(false);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON               
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    }    
}

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
void MsDrvInterfaceTouchDeviceResume(struct device *pDevice)
#else
void MsDrvInterfaceTouchDeviceResume(struct early_suspend *pSuspend)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_IsUpdateFirmware != 0) // Check whether update frimware is finished
    {
        //DBG(&g_I2cClient->dev, "Not allow to power on/off touch ic while update firmware.\n");
	TPD_DMESG("tp resume firmware\n");
        return;
    }
	TPD_DMESG("tp resume no ps return\n");
#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
    if (g_EnableTpProximity == 1)
    {
	if(g_ps_flag == 1)
	{
		g_ps_flag = 0;       	
		DBG(&g_I2cClient->dev, "g_EnableTpProximity = %d\n", g_EnableTpProximity);
	 	TPD_DMESG("tp resume  ps return\n");
        	return;
	}
    }
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION
	TPD_DMESG("tp resume no ps return\n");
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_HOTKNOT
    if (g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE && g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_AFTER_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT
    {
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
        if (g_GestureDebugMode == 1)
        {
            DrvIcFwLyrCloseGestureDebugMode();
        }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

        if (g_GestureWakeupFlag == 1)
        {
            DrvIcFwLyrCloseGestureWakeup();
        }
        else
        {
            DrvPlatformLyrEnableFingerTouchReport(); 
        }
    }
#ifdef CONFIG_ENABLE_HOTKNOT
    else    // Enable touch in hotknot transfer mode
    {
        DrvPlatformLyrEnableFingerTouchReport();     
    }
#endif //CONFIG_ENABLE_HOTKNOT
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
    
#ifdef CONFIG_ENABLE_HOTKNOT
    if (g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE && g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_AFTER_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT        
    {
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
	 tpd_gpio_as_int(1);
        DrvPlatformLyrTouchDeviceRegulatorPowerOn(true);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON               
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
        DrvPlatformLyrTouchDevicePowerOn(); 
    }   
     #ifdef CONFIG_ENABLE_HEADPHONES_DETECTION
     if(bSuspendHeadphones)
    {
    	  DrvSetHeadphonesStateToFw(DrvGetHeadphonesStateFromSys());
         bSuspendHeadphones = 0;
	  hrtimer_start(&headphonesTimer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }
#endif
#ifdef CONFIG_ENABLE_CHARGER_DETECTION 
    {
        u8 szChargerStatus[20] = {0};
 
        DrvCommonReadFile("/sys/class/power_supply/battery/status", szChargerStatus, 20);
        
        DBG(&g_I2cClient->dev, "*** Battery Status : %s ***\n", szChargerStatus);
        
        g_ForceUpdate = 1; // Set flag to force update charger status

        if (strstr(szChargerStatus, "Charging") != NULL || strstr(szChargerStatus, "Full") != NULL || strstr(szChargerStatus, "Fully charged") != NULL) // Charging
        {
            DrvFwCtrlChargerDetection(1); // charger plug-in
        }
        else // Not charging
        {
            DrvFwCtrlChargerDetection(0); // charger plug-out
        }

        g_ForceUpdate = 0; // Clear flag after force update charger status
    }           
#endif //CONFIG_ENABLE_CHARGER_DETECTION

#ifdef CONFIG_ENABLE_GLOVE_MODE
    if (g_IsEnableGloveMode == 1)
    {
        DrvIcFwLyrOpenGloveMode();
    }
#endif //CONFIG_ENABLE_GLOVE_MODE

    if (IS_FIRMWARE_DATA_LOG_ENABLED)    
    {
        DrvIcFwLyrRestoreFirmwareModeToLogDataMode(); // Mark this function call for avoiding device driver may spend longer time to resume from suspend state.
    } //IS_FIRMWARE_DATA_LOG_ENABLED

#ifndef CONFIG_ENABLE_GESTURE_WAKEUP
    DrvPlatformLyrEnableFingerTouchReport(); 
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

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

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
  
    DrvPlatformLyrVariableInitialize(); 

    DrvPlatformLyrTouchDeviceRequestGPIO(pClient);

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    DrvPlatformLyrTouchDeviceRegulatorPowerOn(true);
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

    DrvPlatformLyrTouchDevicePowerOn();

    nRetVal = DrvMainTouchDeviceInitialize();
    if (nRetVal == -ENODEV)
    {
        DrvPlatformLyrTouchDeviceRemove(pClient);
        return nRetVal;
    }
#ifdef CONFIG_ENABLE_HEADPHONES_DETECTION
	headphones_init();
#endif
    DrvPlatformLyrInputDeviceInitialize(pClient); 
    
#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaAlloc(); // DmaAlloc() shall be called after DrvPlatformLyrInputDeviceInitialize()
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvPlatformLyrTouchDeviceRegisterFingerTouchInterruptHandler();

    DrvPlatformLyrTouchDeviceRegisterEarlySuspend();

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
    DrvIcFwLyrCheckFirmwareUpdateBySwId();
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_ESD_PROTECTION
    INIT_DELAYED_WORK(&g_EsdCheckWork, DrvPlatformLyrEsdCheck);
    g_EsdCheckWorkqueue = create_workqueue("esd_check");
    queue_delayed_work(g_EsdCheckWorkqueue, &g_EsdCheckWork, ESD_PROTECT_CHECK_PERIOD);
#endif //CONFIG_ENABLE_ESD_PROTECTION

    DBG(&g_I2cClient->dev, "*** MStar touch driver registered ***\n");
    
    return nRetVal;
}

/* remove function is triggered when the input device is removed from input sub-system */
s32 /*__devexit*/ MsDrvInterfaceTouchDeviceRemove(struct i2c_client *pClient)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    return DrvPlatformLyrTouchDeviceRemove(pClient);
}

void MsDrvInterfaceTouchDeviceSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvPlatformLyrSetIicDataRate(pClient, nIicDataRate);
}    
