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
 * @file    ilitek_drv_update.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "ilitek_drv_common.h"


/*=============================================================*/
// VARIABLE DECLARATION
/*=============================================================*/

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
/*
 * Note.
 * Please modify the name of the below .h depends on the vendor TP that you are using.
 */
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
#include "msg22xx_xxxx_update_bin.h" // for MSG22xx
#include "msg22xx_yyyy_update_bin.h"

static SwIdData_t _gSwIdData[] = {
    {MSG22XX_SW_ID_XXXX, msg22xx_xxxx_update_bin},
    {MSG22XX_SW_ID_YYYY, msg22xx_yyyy_update_bin},
};
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#include "msg28xx_xxxx_update_bin.h" // for MSG28xx/MSG58xxA/ILI2117A/ILI2118A
//#include "msg28xx_yyyy_update_bin.h"

static SwIdData_t _gSwIdData[] = {
    {MSG28XX_SW_ID_XXXX, msg28xx_xxxx_update_bin},
    //{MSG28XX_SW_ID_YYYY, msg28xx_yyyy_update_bin},
};
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

static u32 _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
static u32 _gIsUpdateInfoBlockFirst = 0;
static struct work_struct _gUpdateFirmwareBySwIdWork;
static struct workqueue_struct *_gUpdateFirmwareBySwIdWorkQueue = NULL;
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

static u8 _gTpVendorCode[3] = {0}; // for MSG22xx/MSG28xx

static u8 _gOneDimenFwData[MSG28XX_FIRMWARE_WHOLE_SIZE*1024] = {0}; // for MSG22xx : array size = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG22XX_FIRMWARE_INFO_BLOCK_SIZE, for MSG28xx : array size = MSG28XX_FIRMWARE_WHOLE_SIZE*1024

static u8 _gFwDataBuf[MSG28XX_FIRMWARE_WHOLE_SIZE*1024] = {0}; // for update firmware(MSG22xx/MSG28xx/MSG58xxa) from SD card

//static u32 _gnIliTekApStartAddr = 0;
//static u32 _gnIliTekApEndAddr = 0;
//static u32 _gnIliTekApChecksum = 0;

static u16 _gSFR_ADDR3_BYTE0_1_VALUE = 0x0000; // for MSG28xx
static u16 _gSFR_ADDR3_BYTE2_3_VALUE = 0x0000;

//------------------------------------------------------------------------------//

extern struct i2c_client *g_I2cClient;

extern u32 SLAVE_I2C_ID_DBBUS; 
extern u32 SLAVE_I2C_ID_DWI2C;

extern u8 IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED;

extern struct mutex g_Mutex;

extern u8 g_FwData[MAX_UPDATE_FIRMWARE_BUFFER_SIZE][1024];
extern u32 g_FwDataCount;
extern u8 g_FwVersionFlag;
extern u16 g_ChipType;
extern u16 g_OriginalChipType;

extern u8 g_IsUpdateFirmware;
extern u8 g_Msg22xxChipRevision;

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern u8 g_GestureWakeupFlag;
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

/*=============================================================*/
// FUNCTION DECLARATION
/*=============================================================*/

static s32 _DrvMsg22xxUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType);
static void _DrvReadReadDQMemStart(void);
static void _DrvReadReadDQMemEnd(void);

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
static void _DrvUpdateFirmwareBySwIdDoWork(struct work_struct *pWork);

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
static void _DrvMsg22xxCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
static void _DrvMsg28xxCheckFirmwareUpdateBySwId(void);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

void DrvOptimizeCurrentConsumption(void)
{
    u32 i;
    u8 szDbBusTxData[35] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DBG(&g_I2cClient->dev, "g_ChipType = 0x%x\n", g_ChipType);
    
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    if (g_GestureWakeupFlag == 1)
    {
        return;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        mutex_lock(&g_Mutex);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

        DrvTouchDeviceHwReset(); 

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

        // Enable burst mode
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x11;
        szDbBusTxData[2] = 0xA0; //bank:0x11, addr:h0050
    
        for (i = 0; i < 24; i ++)
        {
            szDbBusTxData[i+3] = 0x11;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+24);  // Write 0x11 for reg 0x1150~0x115B

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x11;
        szDbBusTxData[2] = 0xB8; //bank:0x11, addr:h005C
    
        for (i = 0; i < 6; i ++)
        {
            szDbBusTxData[i+3] = 0xFF;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+6);   // Write 0xFF for reg 0x115C~0x115E 
    
        // Clear burst mode
        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01)); 
    
        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        mutex_unlock(&g_Mutex);
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A) // CHIP_TYPE_MSG58XXA not need to execute the following code
    {
        mutex_lock(&g_Mutex);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
        DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

        DrvTouchDeviceHwReset(); 

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        // Enable burst mode
        RegSetLByteValue(0x1608, 0x21);

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x15;
        szDbBusTxData[2] = 0x20; //bank:0x15, addr:h0010
    
        for (i = 0; i < 8; i ++)
        {
            szDbBusTxData[i+3] = 0xFF;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+8);  // Write 0xFF for reg 0x1510~0x1513

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x15;
        szDbBusTxData[2] = 0x28; //bank:0x15, addr:h0014
    
        for (i = 0; i < 16; i ++)
        {
            szDbBusTxData[i+3] = 0x00;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+16);   // Write 0x00 for reg 0x1514~0x151B
    
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x21;
        szDbBusTxData[2] = 0x40; //bank:0x21, addr:h0020
    
        for (i = 0; i < 8; i ++)
        {
            szDbBusTxData[i+3] = 0xFF;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+8);  // Write 0xFF for reg 0x2120~0x2123

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x21;
        szDbBusTxData[2] = 0x20; //bank:0x21, addr:h0010
    
        for (i = 0; i < 32; i ++)
        {
            szDbBusTxData[i+3] = 0xFF;
        }
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+32);  // Write 0xFF for reg 0x2110~0x211F

        // Clear burst mode
        RegSetLByteValue(0x1608, 0x20);
    
        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        mutex_unlock(&g_Mutex);
    }
}

u16 DrvGetChipType(void)
{
    s32 rc =0;
    //u32 nTemp = 0;
    u16 nChipType = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    // Check chip type by using DbBus for MSG22XX/MSG28XX/MSG58XX/MSG58XXA/ILI2117A/ILI2118A.
    // Erase TP Flash first
    rc = DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    DrvDBBusI2cResponseAck();
    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01);

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K // for MSG22xx only
    // Exit flash low power mode
    RegSetLByteValue(0x1619, BIT1); 

    // Change PIU clock to 48MHz
    RegSetLByteValue(0x1E23, BIT6); 

    // Change mcu clock deglitch mux source
    RegSetLByteValue(0x1E54, BIT0); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // Set Password
    RegSetLByteValue(0x1616,0xAA);
    RegSetLByteValue(0x1617,0x55);
    RegSetLByteValue(0x1618,0xA5);
    RegSetLByteValue(0x1619,0x5A);

    // disable cpu read, in,tial); read
    RegSetLByteValue(0x1608,0x20);
    RegSetLByteValue(0x1606,0x20);
    RegSetLByteValue(0x1607,0x00);

    // set info block
    RegSetLByteValue(0x1607,0x08);
    // set info double buffer
    RegSetLByteValue(0x1604,0x01);

    // set eflash mode to read mode
    RegSetLByteValue(0x1606,0x01);
    RegSetLByteValue(0x1610,0x01);
    RegSetLByteValue(0x1611,0x00);

    // set read address
    RegSetLByteValue(0x1600,0x05);
    RegSetLByteValue(0x1601,0x00);

    nChipType = RegGet16BitValue(0x160A) & 0xFFFF;
 // Start MCU
    RegSetLByteValue(0x0FE6, 0x00);    
    if (nChipType == CHIP_TYPE_ILI2117A || nChipType == CHIP_TYPE_ILI2118A || nChipType == CHIP_TYPE_MSG2836A ||
        nChipType == CHIP_TYPE_MSG2846A || nChipType == CHIP_TYPE_MSG5846A || nChipType == CHIP_TYPE_MSG5856A)
    {
        DBG(&g_I2cClient->dev, "----------------------ILI Chip Type=0x%x-------------------------\n" , nChipType);
        g_OriginalChipType = nChipType;
        nChipType = 0xBF;
    }
    else
    {
        nChipType = RegGet16BitValue(0x1ECC) & 0xFF;
        
        DBG(&g_I2cClient->dev, "----------------------MSG Chip Type=0x%x-------------------------\n" , nChipType);
        g_OriginalChipType = nChipType;

        if (nChipType != CHIP_TYPE_MSG21XX &&   // (0x01) 
            nChipType != CHIP_TYPE_MSG21XXA &&  // (0x02) 
            nChipType != CHIP_TYPE_MSG26XXM &&  // (0x03) 
            nChipType != CHIP_TYPE_MSG22XX &&   // (0x7A)
            nChipType != CHIP_TYPE_MSG28XX &&   // (0x85)
            nChipType != CHIP_TYPE_MSG58XXA)    // (0xBF)
        {
            if (nChipType != 0) 
            {
                nChipType = CHIP_TYPE_MSG58XXA;
            }
        }
		
        if (nChipType == CHIP_TYPE_MSG22XX)  // (0x7A)
        {
            g_Msg22xxChipRevision = RegGetHByteValue(0x1ECE);
				
            DBG(&g_I2cClient->dev, "*** g_Msg22xxChipRevision = 0x%x ***\n", g_Msg22xxChipRevision);
        }
    }
    DBG(&g_I2cClient->dev, "*** g_OriginalChipType = 0x%x ***\n", g_OriginalChipType);
    DBG(&g_I2cClient->dev, "*** Chip Type = 0x%x ***\n", nChipType);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nChipType;
}	

void DrvGetCustomerFirmwareVersionByDbBus(EmemType_e eEmemType, u16 *pMajor, u16 *pMinor, u8 **ppVersion) // support MSG28xx only
{
    u16 nReadAddr = 0;
    u8  szTmpData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A)   
    {
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); 

        if (eEmemType == EMEM_MAIN) // Read SW ID from main block
        {
            DrvMsg28xxReadEFlashStart(0x7FFD, EMEM_MAIN); 
            nReadAddr = 0x7FFD;
        }
        else if (eEmemType == EMEM_INFO) // Read SW ID from info block
        {
            DrvMsg28xxReadEFlashStart(0x81FB, EMEM_INFO);
            nReadAddr = 0x81FB;
        }

        DrvMsg28xxReadEFlashDoRead(nReadAddr, &szTmpData[0]);

        DrvMsg28xxReadEFlashEnd();

        /*
          Ex. Major in Main Block :
              Major low byte at address 0x7FFD
          
              Major in Info Block :
              Major low byte at address 0x81FB
        */

        *pMajor = (szTmpData[1] << 8);
        *pMajor |= szTmpData[0];
        *pMinor = (szTmpData[3] << 8);
        *pMinor |= szTmpData[2];

        DBG(&g_I2cClient->dev, "*** Major = %d ***\n", *pMajor);
        DBG(&g_I2cClient->dev, "*** Minor = %d ***\n", *pMinor);
    
        if (*ppVersion == NULL)
        {
            *ppVersion = kzalloc(sizeof(u8)*11, GFP_KERNEL);
        }

        sprintf(*ppVersion, "%05d.%05d", *pMajor, *pMinor);

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
}	

/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/

static void _DrvMsg22xxGetTpVendorCode(u8 *pTpVendorCode)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG22XX)
    {
        u16 nRegData1, nRegData2;

        DrvTouchDeviceHwReset();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); 

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        // Exit flash low power mode
        RegSetLByteValue(0x1619, BIT1); 

        // Change PIU clock to 48MHz
        RegSetLByteValue(0x1E23, BIT6); 

        // Change mcu clock deglitch mux source
        RegSetLByteValue(0x1E54, BIT0); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        RegSet16BitValue(0x1600, 0xC1E9); // Set start address for tp vendor code on info block(Actually, start reading from 0xC1E8)
    
        // Enable burst mode
//        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        RegSetLByteValue(0x160E, 0x01); 

        nRegData1 = RegGet16BitValue(0x1604);
        nRegData2 = RegGet16BitValue(0x1606);

        pTpVendorCode[0] = ((nRegData1 >> 8) & 0xFF);
        pTpVendorCode[1] = (nRegData2 & 0xFF);
        pTpVendorCode[2] = ((nRegData2 >> 8) & 0xFF);

        DBG(&g_I2cClient->dev, "pTpVendorCode[0] = 0x%x , %c \n", pTpVendorCode[0], pTpVendorCode[0]); 
        DBG(&g_I2cClient->dev, "pTpVendorCode[1] = 0x%x , %c \n", pTpVendorCode[1], pTpVendorCode[1]); 
        DBG(&g_I2cClient->dev, "pTpVendorCode[2] = 0x%x , %c \n", pTpVendorCode[2], pTpVendorCode[2]); 
        
        // Clear burst mode
//        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvTouchDeviceHwReset();
    }
}

static u32 _DrvMsg22xxGetFirmwareCrcByHardware(EmemType_e eEmemType) 
{
    u16 nCrcDown = 0;
    u32 nTimeOut = 0;
    u32 nRetVal = 0; 

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

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
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // RIU password
    RegSet16BitValue(0x161A, 0xABBA);      

    // Set PCE high
    RegSetLByteValue(0x1618, 0x40);      

    if (eEmemType == EMEM_MAIN)
    {
        // Set start address and end address for main block
        RegSet16BitValue(0x1600, 0x0000);      
        RegSet16BitValue(0x1640, 0xBFF8);      
    }
    else if (eEmemType == EMEM_INFO)
    {
        // Set start address and end address for info block
        RegSet16BitValue(0x1600, 0xC000);      
        RegSet16BitValue(0x1640, 0xC1F8);      
    }

    // CRC reset
    RegSet16BitValue(0x164E, 0x0001);      

    RegSet16BitValue(0x164E, 0x0000);   
    
    // Trigger CRC check
    RegSetLByteValue(0x160E, 0x20);   
    mdelay(10);
       
    while (1)
    {
        DBG(&g_I2cClient->dev, "Wait CRC down\n");

        nCrcDown = RegGet16BitValue(0x164E);
        if (nCrcDown == 2)
        {
            break;		
        }
        mdelay(10);

        if ((nTimeOut ++) > 30)
        {
            DBG(&g_I2cClient->dev, "Get CRC down failed. Timeout.\n");

            goto GetCRCEnd;
        }
    }
    
    nRetVal = RegGet16BitValue(0x1652);
    nRetVal = (nRetVal << 16) | RegGet16BitValue(0x1650);

    GetCRCEnd:
	
    DBG(&g_I2cClient->dev, "Hardware CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static void _DrvMsg22xxConvertFwDataTwoDimenToOneDimen(u8 szTwoDimenFwData[][1024], u8* pOneDimenFwData)
{
    u32 i, j;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
    {
        if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
        {
            for (j = 0; j < 1024; j ++)
            {
                pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
            }
        }
        else // i == 48
        {
            for (j = 0; j < 512; j ++)
            {
                pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
            }
        }
    }
}

static void _DrvStoreFirmwareData(u8 *pBuf, u32 nSize)
{
    u32 nCount = nSize / 1024;
    u32 nRemainder = nSize % 1024;
    u32 i;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (nCount > 0) // nSize >= 1024
   	{
        for (i = 0; i < nCount; i ++)
        {
            memcpy(g_FwData[g_FwDataCount], pBuf+(i*1024), 1024);

            g_FwDataCount ++;
        }

        if (nRemainder > 0) // Handle special firmware size like MSG22XX(48.5KB)
        {
            DBG(&g_I2cClient->dev, "nRemainder = %d\n", nRemainder);

            memcpy(g_FwData[g_FwDataCount], pBuf+(i*1024), nRemainder);

            g_FwDataCount ++;
        }
    }
    else // nSize < 1024
    {
        if (nSize > 0)
        {
            memcpy(g_FwData[g_FwDataCount], pBuf, nSize);

            g_FwDataCount ++;
        }
    }

    DBG(&g_I2cClient->dev, "*** g_FwDataCount = %d ***\n", g_FwDataCount);

    if (pBuf != NULL)
    {
        DBG(&g_I2cClient->dev, "*** buf[0] = %c ***\n", pBuf[0]);
    }
}

static u16 _DrvMsg22xxGetSwId(EmemType_e eEmemType) 
{
    u16 nRetVal = 0; 
    u16 nRegData1 = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

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

    if (eEmemType == EMEM_MAIN) // Read SW ID from main block
    {
        RegSet16BitValue(0x1600, 0xBFF4); // Set start address for main block SW ID
    }
    else if (eEmemType == EMEM_INFO) // Read SW ID from info block
    {
        RegSet16BitValue(0x1600, 0xC1EC); // Set start address for info block SW ID
    }

    /*
      Ex. SW ID in Main Block :
          Major low byte at address 0xBFF4
          Major high byte at address 0xBFF5
          
          SW ID in Info Block :
          Major low byte at address 0xC1EC
          Major high byte at address 0xC1ED
    */
    
    // Enable burst mode
//    RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

    RegSetLByteValue(0x160E, 0x01); 

    nRegData1 = RegGet16BitValue(0x1604);
//    nRegData2 = RegGet16BitValue(0x1606);

    nRetVal = ((nRegData1 >> 8) & 0xFF) << 8;
    nRetVal |= (nRegData1 & 0xFF);
    
    // Clear burst mode
//    RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

    RegSet16BitValue(0x1600, 0x0000); 

    // Clear RIU password
    RegSet16BitValue(0x161A, 0x0000); 
    
    DBG(&g_I2cClient->dev, "SW ID = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;		
}

static void _DrvMsg28xxConvertFwDataTwoDimenToOneDimen(u8 szTwoDimenFwData[][1024], u8* pOneDimenFwData)
{
    u32 i, j;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i ++)
    {
        for (j = 0; j < 1024; j ++)
        {
            pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
        }
    }
}

static u32 _DrvMsg28xxCalculateCrc(u8 *pFwData, u32 nOffset, u32 nSize)
{
    u32 i;
    u32 nData = 0, nCrc = 0;
    u32 nCrcRule = 0x0C470C06; // 0000 1100 0100 0111 0000 1100 0000 0110

    for (i = 0; i < nSize; i += 4)
    {
   	    nData = (pFwData[nOffset+i]) | (pFwData[nOffset+i+1] << 8) | (pFwData[nOffset+i+2] << 16) | (pFwData[nOffset+i+3] << 24);
   	    nCrc = (nCrc >> 1) ^ (nCrc << 1) ^ (nCrc & nCrcRule) ^ nData;
    }
    
    return nCrc;
}

static void _DrvMsg28xxAccessEFlashInit(void)
{
    // Disable cpu read flash
    RegSetLByteValue(0x1606, 0x20);
    RegSetLByteValue(0x1608, 0x20);

    // Clear PROGRAM erase password
    RegSet16BitValue(0x1618, 0xA55A);
}

static void _DrvMsg28xxIspBurstWriteEFlashStart(u16 nStartAddr, u8 *pFirstData, u32 nBlockSize, u16 nPageNum, EmemType_e eEmemType)
{
    u16 nWriteAddr = nStartAddr/4;
    u8  szDbBusTxData[3] = {0};
    
    DBG(&g_I2cClient->dev, "*** %s() nStartAddr = 0x%x, nBlockSize = %d, nPageNum = %d, eEmemType = %d ***\n", __func__, nStartAddr, nBlockSize, nPageNum, eEmemType);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);
    
    // Set e-flash mode to page write mode
    RegSet16BitValue(0x1606, 0x0080);

    // Set data align
    RegSetLByteValue(0x1640, 0x01);
    
    if (eEmemType == EMEM_INFO) 
    {
        RegSetLByteValue(0x1607, 0x08);
    }
    
    // Set double buffer
    RegSetLByteValue(0x1604, 0x01);
        
    // Set page write number
    RegSet16BitValue(0x161A, nPageNum);

    // Set e-flash mode trigger(Trigger write mode)
    RegSetLByteValue(0x1606, 0x81);

    // Set init data
    RegSetLByteValue(0x1602, pFirstData[0]);
    RegSetLByteValue(0x1602, pFirstData[1]);
    RegSetLByteValue(0x1602, pFirstData[2]);
    RegSetLByteValue(0x1602, pFirstData[3]);

    // Set initial address(for latch SA, CA)
    RegSet16BitValue(0x1600, nWriteAddr);

    // Set initial address(for latch PA)
    RegSet16BitValue(0x1600, nWriteAddr);
    
    // Enable burst mode
    RegSetLByteValue(0x1608, 0x21);
    
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x16;
    szDbBusTxData[2] = 0x02;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);

    szDbBusTxData[0] = 0x20;
//    szDbBusTxData[1] = 0x00;
//    szDbBusTxData[2] = 0x00;    
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);
}	

static void _DrvMsg28xxIspBurstWriteEFlashDoWrite(u8 *pBufferData, u32 nLength)
{
    u32 i;
    u8  szDbBusTxData[3+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE] = {0};

    DBG(&g_I2cClient->dev, "*** %s() nLength = %d ***\n", __func__, nLength);

    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x16;
    szDbBusTxData[2] = 0x02;
        
    for (i = 0; i < nLength; i ++)
    {
        szDbBusTxData[3+i] = pBufferData[i];
    }
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3+nLength);
}	

static void _DrvMsg28xxIspBurstWriteEFlashEnd(void)
{
    u8 szDbBusTxData[1] = {0};

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    szDbBusTxData[0] = 0x21;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

    szDbBusTxData[0] = 0x7E;
    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

    // Clear burst mode
    RegSetLByteValue(0x1608, 0x20);
}

static void _DrvMsg28xxWriteEFlashStart(u16 nStartAddr, u8 *pFirstData, EmemType_e eEmemType)
{
    u16 nWriteAddr = nStartAddr/4;
    
    DBG(&g_I2cClient->dev, "*** %s() nStartAddr = 0x%x, eEmemType = %d ***\n", __func__, nStartAddr, eEmemType);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);
    
    // Set e-flash mode to write mode
    RegSet16BitValue(0x1606, 0x0040);

    // Set data align
    RegSetLByteValue(0x1640, 0x01);
    
    if (eEmemType == EMEM_INFO) 
    {
        RegSetLByteValue(0x1607, 0x08);
    }

    // Set double buffer
    RegSetLByteValue(0x1604, 0x01);
        
    // Set e-flash mode trigger(Trigger write mode)
    RegSetLByteValue(0x1606, 0x81);

    // Set init data
    RegSetLByteValue(0x1602, pFirstData[0]);
    RegSetLByteValue(0x1602, pFirstData[1]);
    RegSetLByteValue(0x1602, pFirstData[2]);
    RegSetLByteValue(0x1602, pFirstData[3]);

    // Set initial address(for latch SA, CA)
    RegSet16BitValue(0x1600, nWriteAddr);

    // Set initial address(for latch PA)
    RegSet16BitValue(0x1600, nWriteAddr);
}	

static void _DrvMsg28xxWriteEFlashDoWrite(u16 nStartAddr, u8 *pBufferData)
{
    u16 nWriteAddr = nStartAddr/4;

    DBG(&g_I2cClient->dev, "*** %s() nWriteAddr = %d ***\n", __func__, nWriteAddr);

    // Write data
    RegSetLByteValue(0x1602, pBufferData[0]);
    RegSetLByteValue(0x1602, pBufferData[1]);
    RegSetLByteValue(0x1602, pBufferData[2]);
    RegSetLByteValue(0x1602, pBufferData[3]);

    // Set address
    RegSet16BitValue(0x1600, nWriteAddr);
}	

static void _DrvMsg28xxWriteEFlashEnd(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    // Do nothing
}

void DrvMsg28xxReadEFlashStart(u16 nStartAddr, EmemType_e eEmemType)
{
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);

    RegSetLByteValue(0x1606, 0x02);

    RegSet16BitValue(0x1600, nStartAddr);
    
    if (eEmemType == EMEM_MAIN)
    {
        // Set main block
        RegSetLByteValue(0x1607, 0x00);

        // Set main double buffer
        RegSetLByteValue(0x1604, 0x01);

        // Set e-flash mode to read mode for main
        RegSet16BitValue(0x1606, 0x0001);
    }
    else if (eEmemType == EMEM_INFO)
    {
        // Set info block 
        RegSetLByteValue(0x1607, 0x08);
        
        // Set info double buffer
        RegSetLByteValue(0x1604, 0x01);
        
        // Set e-flash mode to read mode for info
        RegSet16BitValue(0x1606, 0x0801);
    }
}

void DrvMsg28xxReadEFlashDoRead(u16 nReadAddr, u8 *pReadData)
{
    u16 nRegData1 = 0, nRegData2 = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() nReadAddr = 0x%x ***\n", __func__, nReadAddr);

    // Set read address
    RegSet16BitValue(0x1600, nReadAddr);

    // Read 16+16 bits
    nRegData1 = RegGet16BitValue(0x160A);
    nRegData2 = RegGet16BitValue(0x160C);

    pReadData[0] = nRegData1 & 0xFF;
    pReadData[1] = (nRegData1 >> 8) & 0xFF;
    pReadData[2] = nRegData2 & 0xFF;
    pReadData[3] = (nRegData2 >> 8) & 0xFF;
}	

void DrvMsg28xxReadEFlashEnd(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    // Set read done
    RegSetLByteValue(0x1606, 0x02);

    // Unset info flag
    RegSetLByteValue(0x1607, 0x00);

    // Clear address
    RegSet16BitValue(0x1600, 0x0000);
}

/*
static void _DrvMsg28xxGetTpVendorCode(u8 *pTpVendorCode)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A)
    {
        u16 nReadAddr = 0;
        u8  szTmpData[4] = {0};

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        // Stop MCU
        RegSetLByteValue(0x0FE6, 0x01); 

        DrvMsg28xxReadEFlashStart(0x81FA, EMEM_INFO);
        nReadAddr = 0x81FA;

        DrvMsg28xxReadEFlashDoRead(nReadAddr, &szTmpData[0]);

        DBG(&g_I2cClient->dev, "szTmpData[0] = 0x%x\n", szTmpData[0]); // add for debug
        DBG(&g_I2cClient->dev, "szTmpData[1] = 0x%x\n", szTmpData[1]); // add for debug
        DBG(&g_I2cClient->dev, "szTmpData[2] = 0x%x\n", szTmpData[2]); // add for debug
        DBG(&g_I2cClient->dev, "szTmpData[3] = 0x%x\n", szTmpData[3]); // add for debug
   
        DrvMsg28xxReadEFlashEnd();

        pTpVendorCode[0] = szTmpData[1];
        pTpVendorCode[1] = szTmpData[2];
        pTpVendorCode[2] = szTmpData[3];

        DBG(&g_I2cClient->dev, "pTpVendorCode[0] = 0x%x , %c \n", pTpVendorCode[0], pTpVendorCode[0]); 
        DBG(&g_I2cClient->dev, "pTpVendorCode[1] = 0x%x , %c \n", pTpVendorCode[1], pTpVendorCode[1]); 
        DBG(&g_I2cClient->dev, "pTpVendorCode[2] = 0x%x , %c \n", pTpVendorCode[2], pTpVendorCode[2]); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
}
*/

static void _DrvMsg28xxGetSfrAddr3Value(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);

    // Set e-flash mode to read mode
    RegSetLByteValue(0x1606, 0x01);
    RegSetLByteValue(0x1610, 0x01);
    RegSetLByteValue(0x1607, 0x20);

    // Set read address
    RegSetLByteValue(0x1600, 0x03);
    RegSetLByteValue(0x1601, 0x00);

    _gSFR_ADDR3_BYTE0_1_VALUE = RegGet16BitValue(0x160A);
    _gSFR_ADDR3_BYTE2_3_VALUE = RegGet16BitValue(0x160C);

    DBG(&g_I2cClient->dev, "_gSFR_ADDR3_BYTE0_1_VALUE = 0x%4X, _gSFR_ADDR3_BYTE2_3_VALUE = 0x%4X\n", _gSFR_ADDR3_BYTE0_1_VALUE, _gSFR_ADDR3_BYTE2_3_VALUE);
}

static void _DrvMsg28xxUnsetProtectBit(void)
{
    u8 nB0, nB1, nB2, nB3;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    _DrvMsg28xxGetSfrAddr3Value();
    
    nB0 = _gSFR_ADDR3_BYTE0_1_VALUE & 0xFF;
    nB1 = (_gSFR_ADDR3_BYTE0_1_VALUE & 0xFF00) >> 8;

    nB2 = _gSFR_ADDR3_BYTE2_3_VALUE & 0xFF;
    nB3 = (_gSFR_ADDR3_BYTE2_3_VALUE & 0xFF00) >> 8;

    DBG(&g_I2cClient->dev, "nB0 = 0x%2X, nB1 = 0x%2X, nB2 = 0x%2X, nB3 = 0x%2X\n", nB0, nB1, nB2, nB3);

    nB2 = nB2 & 0xBF; // 10111111
    nB3 = nB3 & 0xFC; // 11111100

    DBG(&g_I2cClient->dev, "nB0 = 0x%2X, nB1 = 0x%2X, nB2 = 0x%2X, nB3 = 0x%2X\n", nB0, nB1, nB2, nB3);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);
    RegSetLByteValue(0x1610, 0x80);
    RegSetLByteValue(0x1607, 0x10);

    // Trigger SFR write
    RegSetLByteValue(0x1606, 0x01);

    // Set write data
    RegSetLByteValue(0x1602, nB0);
    RegSetLByteValue(0x1602, nB1);
    RegSetLByteValue(0x1602, nB2);
    RegSetLByteValue(0x1602, nB3);

    // Set write address
    RegSetLByteValue(0x1600, 0x03);
    RegSetLByteValue(0x1601, 0x00);

    // Set TM mode = 0
    RegSetLByteValue(0x1607, 0x00);

#ifdef CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    RegSetLByteValue(0x1606, 0x01);
    RegSetLByteValue(0x1606, 0x20);
#endif //CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
}

void _DrvMsg28xxSetProtectBit(void)
{
    u8 nB0, nB1, nB2, nB3;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    nB0 = _gSFR_ADDR3_BYTE0_1_VALUE & 0xFF;
    nB1 = (_gSFR_ADDR3_BYTE0_1_VALUE & 0xFF00) >> 8;

    nB2 = _gSFR_ADDR3_BYTE2_3_VALUE & 0xFF;
    nB3 = (_gSFR_ADDR3_BYTE2_3_VALUE & 0xFF00) >> 8;

    DBG(&g_I2cClient->dev, "nB0 = 0x%2X, nB1 = 0x%2X, nB2 = 0x%2X, nB3 = 0x%2X\n", nB0, nB1, nB2, nB3);

    // Disable cpu read flash
    RegSetLByteValue(0x1608, 0x20);
    RegSetLByteValue(0x1606, 0x20);
    RegSetLByteValue(0x1610, 0x80);
    RegSetLByteValue(0x1607, 0x10);

    // Trigger SFR write
    RegSetLByteValue(0x1606, 0x01);

    // Set write data
    RegSetLByteValue(0x1602, nB0);
    RegSetLByteValue(0x1602, nB1);
    RegSetLByteValue(0x1602, nB2);
    RegSetLByteValue(0x1602, nB3);

    // Set write address
    RegSetLByteValue(0x1600, 0x03);
    RegSetLByteValue(0x1601, 0x00);
    RegSetLByteValue(0x1606, 0x02);
}

void _DrvMsg28xxEraseEmem(EmemType_e eEmemType)
{
    u32 nInfoAddr = 0x20;
    u32 nTimeOut = 0;
    u8 nRegData = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DrvTouchDeviceHwReset();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    DrvDBBusI2cResponseAck();
    DBG(&g_I2cClient->dev, "Erase start\n");

    _DrvMsg28xxAccessEFlashInit();

    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01);

    // Set PROGRAM erase password
    RegSet16BitValue(0x1618, 0x5AA5);
    
    _DrvMsg28xxUnsetProtectBit();

    if (eEmemType == EMEM_MAIN) // 128KB
    {
        DBG(&g_I2cClient->dev, "Erase main block\n");

        // Set main block
        RegSetLByteValue(0x1607, 0x00);

        // Set e-flash mode to erase mode
        RegSetLByteValue(0x1606, 0xC0);

        // Set page erase main
        RegSetLByteValue(0x1607, 0x03);

        // e-flash mode trigger
        RegSetLByteValue(0x1606, 0xC1);

        nTimeOut = 0;
        while (1) // Wait erase done
        {
            nRegData = RegGetLByteValue(0x160E);
            nRegData = (nRegData & BIT3);
            
            DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT3)
            {
                break;
            }

            mdelay(10);
            
            if ((nTimeOut ++) > 10)
            {
                DBG(&g_I2cClient->dev, "Erase main block failed. Timeout.\n");

                goto EraseEnd;
            }
        }
    }
    else if (eEmemType == EMEM_INFO) // 2KB
    {
        DBG(&g_I2cClient->dev, "Erase info block\n");
        
        // Set info block
        RegSetLByteValue(0x1607, 0x08);

        // Set info double buffer
        RegSetLByteValue(0x1604, 0x01);

        // Set e-flash mode to erase mode
        RegSetLByteValue(0x1606, 0xC0);

        // Set page erase info
        RegSetLByteValue(0x1607, 0x09);
        
        for (nInfoAddr = 0x20; nInfoAddr <= MSG28XX_EMEM_INFO_MAX_ADDR; nInfoAddr += 0x20)
        {
            DBG(&g_I2cClient->dev, "nInfoAddr = 0x%x\n", nInfoAddr); // add for debug

            // Set address
            RegSet16BitValue(0x1600, nInfoAddr);

            // e-flash mode trigger
            RegSetLByteValue(0x1606, 0xC1);

            nTimeOut = 0;
            while (1) // Wait erase done
            {
                nRegData = RegGetLByteValue(0x160E);
                nRegData = (nRegData & BIT3);

                DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);
            
                if (nRegData == BIT3)
                {
                    break;
                }

                mdelay(10);
            
                if ((nTimeOut ++) > 10)
                {
                    DBG(&g_I2cClient->dev, "Erase info block failed. Timeout.\n");

                    // Set main block
                    RegSetLByteValue(0x1607, 0x00);

                    goto EraseEnd;
                }
            }        	
        }

        // Set main block
        RegSetLByteValue(0x1607, 0x00);
    }
    
    EraseEnd:
    
    _DrvMsg28xxSetProtectBit();

    RegSetLByteValue(0x1606, 0x00);
    RegSetLByteValue(0x1607, 0x00);
		
    // Clear PROGRAM erase password
    RegSet16BitValue(0x1618, 0xA55A);

    DBG(&g_I2cClient->dev, "Erase end\n");
    
    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
}

static void _DrvMsg28xxProgramEmem(EmemType_e eEmemType)
{
    u32 i, j;
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME) || defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
    u32 k;
#endif
    u32 nPageNum = 0, nLength = 0, nIndex = 0, nWordNum = 0;
    u32 nRetryTime = 0;
    u8  nRegData = 0;
    u8  szFirstData[MSG28XX_EMEM_SIZE_BYTES_ONE_WORD] = {0};
    u8  szBufferData[MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE] = {0};
#ifdef CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    u8  szWriteData[3+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*2] = {0};
#endif //CONFIG_ENABLE_HIGH_SPEED_ISP_MODE

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DrvTouchDeviceHwReset();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    DrvDBBusI2cResponseAck();
    DBG(&g_I2cClient->dev, "Program start\n");

    _DrvMsg28xxAccessEFlashInit();
    
    // Stop mcu
    RegSetLByteValue(0x0FE6, 0x01); 
    
    // Set PROGRAM erase password
    RegSet16BitValue(0x1618, 0x5AA5);

    _DrvMsg28xxUnsetProtectBit();
    
    if (eEmemType == EMEM_MAIN) // Program main block
    {
        DBG(&g_I2cClient->dev, "Program main block\n");

#ifdef CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; // 128*1024/128=1024

        // Set ISP mode
        RegSet16BitValueOn(0x1EBE, BIT15);

        RegSetLByteValue(0x1604, 0x01);
        
        RegSet16BitValue(0x161A, nPageNum);
        RegSet16BitValue(0x1600, 0x0000); // Set initial address
        RegSet16BitValueOn(0x3C00, BIT0); // Disable INT GPIO mode
        RegSet16BitValueOn(0x1EA0, BIT1); // Set ISP INT enable
        RegSet16BitValue(0x1E34, 0x0000); // Set DQMem start address

        _DrvReadReadDQMemStart();
        
        szWriteData[0] = 0x10;
        szWriteData[1] = 0x00;
        szWriteData[2] = 0x00;
        
        nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*2; //128*2=256
        
        for (j = 0; j < nLength; j ++)
        {
            szWriteData[3+j] = g_FwData[0][j];
        }
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3+nLength); // Write the first two pages(page 0 & page 1)
        
        _DrvReadReadDQMemEnd();

        RegSet16BitValueOn(0x1EBE, BIT15); // Set ISP mode
        RegSet16BitValueOn(0x1608, BIT0); // Set burst mode
        RegSet16BitValueOn(0x161A, BIT13); // Set ISP trig

        udelay(2000); // delay about 2ms

        _DrvReadReadDQMemStart();

        nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; //128

        for (i = 2; i < nPageNum; i ++) 
        {
            if (i == 2)
            {
                szWriteData[0] = 0x10;
                szWriteData[1] = 0x00;
                szWriteData[2] = 0x00;

                for (j = 0; j < nLength; j ++)
                {
                    szWriteData[3+j] = g_FwData[i/8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-(8*(i/8)))+j];
                }

                IicWriteData(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3+nLength); 
            }
            else if (i == (nPageNum - 1))
            {
                szWriteData[0] = 0x10;
                szWriteData[1] = 0x00;
                szWriteData[2] = 0x80;

                for (j = 0; j < nLength; j ++)
                {
                    szWriteData[3+j] = g_FwData[i/8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-(8*(i/8)))+j];
                }

                szWriteData[3+128] = 0xFF;
                szWriteData[3+129] = 0xFF;
                szWriteData[3+130] = 0xFF;
                szWriteData[3+131] = 0xFF;

                IicWriteData(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3+nLength+4); 
            }           
            else
            {
//                szWriteData[0] = 0x10;
//                szWriteData[1] = 0x00;
                if (szWriteData[2] == 0x00)
                {
                    szWriteData[2] = 0x80;
                }
                else // szWriteData[2] == 0x80
                {
                		szWriteData[2] = 0x00;
                }

                for (j = 0; j < nLength; j ++)
                {
                    szWriteData[3+j] = g_FwData[i/8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-(8*(i/8)))+j];
                }

                IicWriteData(SLAVE_I2C_ID_DBBUS, &szWriteData[0], 3+nLength); 
            }
        }

        _DrvReadReadDQMemEnd();

#else

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / 8; // 128*1024/8=16384 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / 32; // 128*1024/32=4096 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
        nPageNum = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024) / MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; // 128*1024/128=1024
#endif 
     
        nIndex = 0;
        
        for (i = 0; i < nPageNum; i ++) 
        {
            if (i == 0)
            {
                // Read first data 4 bytes
                nLength = MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;

                szFirstData[0] = g_FwData[0][0];
                szFirstData[1] = g_FwData[0][1];
                szFirstData[2] = g_FwData[0][2];
                szFirstData[3] = g_FwData[0][3];
            
                _DrvMsg28xxIspBurstWriteEFlashStart(nIndex, &szFirstData[0], MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024, nPageNum, EMEM_MAIN);

                nIndex += nLength;
            
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                nLength = 8 - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 4 = 8 - 4 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                nLength = 32 - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 28 = 32 - 4 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 124 = 128 - 4
#endif

                for (j = 0; j < nLength; j ++)
                {
                    szBufferData[j] = g_FwData[0][4+j];
                }
            }
            else
            {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                nLength = 8; 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                nLength = 32; 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; // 128
#endif 

                for (j = 0; j < nLength; j ++)
                {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                    szBufferData[j] = g_FwData[i/128][8*(i-(128*(i/128)))+j]; 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                    szBufferData[j] = g_FwData[i/32][32*(i-(32*(i/32)))+j]; 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                    szBufferData[j] = g_FwData[i/8][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-(8*(i/8)))+j];
#endif
                }
            }

            _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], nLength);
            
            udelay(2000); // delay about 2ms
        
            nIndex += nLength;
        }

        _DrvMsg28xxIspBurstWriteEFlashEnd();

        // Set write done
        RegSet16BitValueOn(0x1606, BIT2);

        // Check RBB
        nRegData = RegGetLByteValue(0x160E);
        nRetryTime = 0;
    
        while ((nRegData & BIT3) != BIT3)
        {
            mdelay(10);

            nRegData = RegGetLByteValue(0x160E);
    
            if (nRetryTime ++ > 100)
            {
                DBG(&g_I2cClient->dev, "main block can't wait write to done.\n");

                goto ProgramEnd;
            }
        }
#endif //CONFIG_ENABLE_HIGH_SPEED_ISP_MODE
    }
    else if (eEmemType == EMEM_INFO) // Program info block
    {
        DBG(&g_I2cClient->dev, "Program info block\n");

        nPageNum = (MSG28XX_FIRMWARE_INFO_BLOCK_SIZE * 1024) / MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; // 2*1024/128=16

        nIndex = 0;
        nIndex += MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; //128

        // Skip firt page(page 0) & Update page 1~14 by isp burst write mode
        for (i = 1; i < (nPageNum - 1); i ++) // Skip the first 128 byte and the last 128 byte of info block
        {
            if (i == 1)
            {
                // Read first data 4 bytes
                nLength = MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;
            
                szFirstData[0] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE];
                szFirstData[1] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+1];
                szFirstData[2] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+2];
                szFirstData[3] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+3];
            
                _DrvMsg28xxIspBurstWriteEFlashStart(nIndex, &szFirstData[0], MSG28XX_FIRMWARE_INFO_BLOCK_SIZE*1024, nPageNum-1, EMEM_INFO);
            
                nIndex += nLength;
            
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE - MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 124 = 128 - 4

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                for (j = 0; j < (nLength/8); j ++) // 124/8 = 15
                {
                    for (k = 0; k < 8; k ++)
                    {
                        szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+(8*j)+k];
                    }
                    
                    _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 8);

                    udelay(2000); // delay about 2ms 
                }

                for (k = 0; k < (nLength%8); k ++) // 124%8 = 4
                {
                    szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+(8*j)+k];
                }

                _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], (nLength%8)); // 124%8 = 4

                udelay(2000); // delay about 2ms 
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                for (j = 0; j < (nLength/32); j ++) // 124/8 = 3
                {
                    for (k = 0; k < 32; k ++)
                    {
                        szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+(32*j)+k];
                    }
                    
                    _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 32);

                    udelay(2000); // delay about 2ms 
                }

                for (k = 0; k < (nLength%32); k ++) // 124%32 = 28
                {
                    szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+(32*j)+k];
                }

                _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], (nLength%32)); // 124%8 = 28

                udelay(2000); // delay about 2ms 
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                for (j = 0; j < nLength; j ++)
                {
                    szBufferData[j] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+4+j];
                }
#endif
            }
            else
            {
                nLength = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE; //128

#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME)
                if (i < 8) // 1 < i < 8
                {
                    for (j = 0; j < (nLength/8); j ++) // 128/8 = 16
                    {
                        for (k = 0; k < 8; k ++)
                        {
                            szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*i+(8*j)+k];
                        }
                    
                        _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 8);

                        udelay(2000); // delay about 2ms 
                    }
                }
                else // i >= 8
                {
                    for (j = 0; j < (nLength/8); j ++) // 128/8 = 16
                    {
                        for (k = 0; k < 8; k ++)
                        {
                            szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-8)+(8*j)+k];
                        }
                    
                        _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 8);

                        udelay(2000); // delay about 2ms 
                    }
                }
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
                if (i < 8) // 1 < i < 8
                {
                    for (j = 0; j < (nLength/32); j ++) // 128/32 = 4
                    {
                        for (k = 0; k < 32; k ++)
                        {
                            szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*i+(32*j)+k];
                        }
                    
                        _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 32);

                        udelay(2000); // delay about 2ms 
                    }
                }
                else // i >= 8
                {
                    for (j = 0; j < (nLength/32); j ++) // 128/32 = 4
                    {
                        for (k = 0; k < 32; k ++)
                        {
                            szBufferData[k] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-8)+(32*j)+k];
                        }
                    
                        _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], 32);

                        udelay(2000); // delay about 2ms 
                    }
                }
#else // UPDATE FIRMWARE WITH 128 BYTE EACH TIME
                if (i < 8) // 1 < i < 8
                {
                    for (j = 0; j < nLength; j ++)
                    {
                        szBufferData[j] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*i+j];
                    }
                }
                else // i >= 8
                {
                    for (j = 0; j < nLength; j ++)
                    {
                        szBufferData[j] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE*(i-8)+j];
                    }
                }
#endif
            }
        
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME) || defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME)
            // Do nothing here
#else
            _DrvMsg28xxIspBurstWriteEFlashDoWrite(&szBufferData[0], nLength);

            udelay(2000); // delay about 2ms 
#endif        
            nIndex += nLength;
        }

        _DrvMsg28xxIspBurstWriteEFlashEnd();
    
        // Set write done
        RegSet16BitValueOn(0x1606, BIT2);
            
        // Check RBB
        nRegData = RegGetLByteValue(0x160E);
        nRetryTime = 0;
    
        while ((nRegData & BIT3) != BIT3)
        {
            mdelay(10);

            nRegData = RegGetLByteValue(0x160E);
    
            if (nRetryTime ++ > 100)
            {
                DBG(&g_I2cClient->dev, "Info block page 1~14 can't wait write to done.\n");

                goto ProgramEnd;
            }
        }

        RegSet16BitValueOff(0x1EBE, BIT15); 

        // Update page 15 by write mode
        nIndex = 15 * MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE;
        nWordNum = MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE / MSG28XX_EMEM_SIZE_BYTES_ONE_WORD; // 128/4=32
        nLength = MSG28XX_EMEM_SIZE_BYTES_ONE_WORD;

        for (i = 0; i < nWordNum; i ++) 
        {
            if (i == 0)
            {
                szFirstData[0] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE];
                szFirstData[1] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+1];
                szFirstData[2] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+2];
                szFirstData[3] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+3];

                _DrvMsg28xxWriteEFlashStart(nIndex, &szFirstData[0], EMEM_INFO);
            }
            else
            {
                for (j = 0; j < nLength; j ++)
                {
                    szFirstData[j] = g_FwData[MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+1][7*MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE+(4*i)+j];
                }

                _DrvMsg28xxWriteEFlashDoWrite(nIndex, &szFirstData[0]);            			
            }

            udelay(2000); // delay about 2ms 

            nIndex += nLength;
        }
        
        _DrvMsg28xxWriteEFlashEnd();

        // Set write done
        RegSet16BitValueOn(0x1606, BIT2);
            
        // Check RBB
        nRegData = RegGetLByteValue(0x160E);
        nRetryTime = 0;
    
        while ((nRegData & BIT3) != BIT3)
        {
            mdelay(10);

            nRegData = RegGetLByteValue(0x160E);
    
            if (nRetryTime ++ > 100)
            {
                DBG(&g_I2cClient->dev, "Info block page 15 can't wait write to done.\n");
                
                goto ProgramEnd;
            }
        }
    }

    ProgramEnd:
    
    _DrvMsg28xxSetProtectBit();

    // Clear PROGRAM erase password
    RegSet16BitValue(0x1618, 0xA55A);

    DBG(&g_I2cClient->dev, "Program end\n");

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
}

static u16 _DrvMsg28xxGetSwId(EmemType_e eEmemType) 
{
    u16 nRetVal = 0; 
    u16 nReadAddr = 0;
    u8  szTmpData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 

    if (eEmemType == EMEM_MAIN) // Read SW ID from main block
    {
        DrvMsg28xxReadEFlashStart(0x7FFD, EMEM_MAIN); 
        nReadAddr = 0x7FFD;
    }
    else if (eEmemType == EMEM_INFO) // Read SW ID from info block
    {
        DrvMsg28xxReadEFlashStart(0x81FB, EMEM_INFO);
        nReadAddr = 0x81FB;
    }

    DrvMsg28xxReadEFlashDoRead(nReadAddr, &szTmpData[0]);

    DrvMsg28xxReadEFlashEnd();

    /*
      Ex. SW ID in Main Block :
          Major low byte at address 0x7FFD
          
          SW ID in Info Block :
          Major low byte at address 0x81FB
    */

    nRetVal = (szTmpData[1] << 8);
    nRetVal |= szTmpData[0];
    
    DBG(&g_I2cClient->dev, "SW ID = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;		
}

static u32 _DrvMsg28xxGetFirmwareCrcByHardware(EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    u32 nRetryTime = 0;
    u32 nCrcEndAddr = 0;
    u16 nCrcDown = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DrvTouchDeviceHwReset();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    _DrvMsg28xxAccessEFlashInit();

    if (eEmemType == EMEM_MAIN)
    {
        // Disable cpu read flash
        RegSetLByteValue(0x1608, 0x20);
        RegSetLByteValue(0x1606, 0x20);
        
        // Set read flag
        RegSet16BitValue(0x1610, 0x0001);
    		
        // Mode reset main block
        RegSet16BitValue(0x1606, 0x0000);

        // CRC reset
        RegSet16BitValue(0x1620, 0x0002);

        RegSet16BitValue(0x1620, 0x0000);
        
        // Set CRC e-flash block start address => Main Block : 0x0000 ~ 0x7FFE
        RegSet16BitValue(0x1600, 0x0000);
        
        nCrcEndAddr = (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)/4-2;

        RegSet16BitValue(0x1622, nCrcEndAddr);
        
        // Trigger CRC check
        RegSet16BitValue(0x1620, 0x0001);
        
        nCrcDown = RegGet16BitValue(0x1620);
        
        nRetryTime = 0;
        while ((nCrcDown >> 15) == 0)
        {
            mdelay(10);

            nCrcDown = RegGet16BitValue(0x1620);
            nRetryTime ++;
            
            if (nRetryTime > 30)
            {
                DBG(&g_I2cClient->dev, "Wait main block nCrcDown failed.\n");
                break;
            }
        }
        
        nRetVal = RegGet16BitValue(0x1626);
        nRetVal = (nRetVal << 16) | RegGet16BitValue(0x1624);
    }
    else if (eEmemType == EMEM_INFO)
    {
        // Disable cpu read flash
        RegSetLByteValue(0x1608, 0x20);
        RegSetLByteValue(0x1606, 0x20);
        
        // Set read flag
        RegSet16BitValue(0x1610, 0x0001);
    		
        // Mode reset info block
        RegSet16BitValue(0x1606, 0x0800);

        RegSetLByteValue(0x1604, 0x01);

        // CRC reset
        RegSet16BitValue(0x1620, 0x0002);

        RegSet16BitValue(0x1620, 0x0000);
        
        // Set CRC e-flash block start address => Info Block : 0x0020 ~ 0x01FE
        RegSet16BitValue(0x1600, 0x0020);
        RegSet16BitValue(0x1622, 0x01FE);

        // Trigger CRC check
        RegSet16BitValue(0x1620, 0x0001);
        
        nCrcDown = RegGet16BitValue(0x1620);
        
        nRetryTime = 0;
        while ((nCrcDown >> 15) == 0)
        {
            mdelay(10);

            nCrcDown = RegGet16BitValue(0x1620);
            nRetryTime ++;
            
            if (nRetryTime > 30)
            {
                DBG(&g_I2cClient->dev, "Wait info block nCrcDown failed.\n");
                break;
            }
        }
        
        nRetVal = RegGet16BitValue(0x1626);
        nRetVal = (nRetVal << 16) | RegGet16BitValue(0x1624);
    }

    DBG(&g_I2cClient->dev, "Hardware CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvMsg28xxRetrieveFirmwareCrcFromEFlash(EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    u16 nReadAddr = 0;
    u8  szTmpData[4] = {0};

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 

    if (eEmemType == EMEM_MAIN) // Read main block CRC(128KB-4) from main block
    {
        DrvMsg28xxReadEFlashStart(0x7FFF, EMEM_MAIN);
        nReadAddr = 0x7FFF;
    }
    else if (eEmemType == EMEM_INFO) // Read info block CRC(2KB-MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE-4) from info block
    {
        DrvMsg28xxReadEFlashStart(0x81FF, EMEM_INFO);
        nReadAddr = 0x81FF;
    }

    DrvMsg28xxReadEFlashDoRead(nReadAddr, &szTmpData[0]);

    DBG(&g_I2cClient->dev, "szTmpData[0] = 0x%x\n", szTmpData[0]); // add for debug
    DBG(&g_I2cClient->dev, "szTmpData[1] = 0x%x\n", szTmpData[1]); // add for debug
    DBG(&g_I2cClient->dev, "szTmpData[2] = 0x%x\n", szTmpData[2]); // add for debug
    DBG(&g_I2cClient->dev, "szTmpData[3] = 0x%x\n", szTmpData[3]); // add for debug
   
    DrvMsg28xxReadEFlashEnd();

    nRetVal = (szTmpData[3] << 24);
    nRetVal |= (szTmpData[2] << 16);
    nRetVal |= (szTmpData[1] << 8);
    nRetVal |= szTmpData[0];
    
    DBG(&g_I2cClient->dev, "CRC = 0x%x\n", nRetVal);

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvMsg28xxRetrieveFrimwareCrcFromBinFile(u8 szTmpBuf[][1024], EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);
    
    if (szTmpBuf != NULL)
    {
        if (eEmemType == EMEM_MAIN) 
        {
            nRetVal = szTmpBuf[127][1023];
            nRetVal = (nRetVal << 8) | szTmpBuf[127][1022];
            nRetVal = (nRetVal << 8) | szTmpBuf[127][1021];
            nRetVal = (nRetVal << 8) | szTmpBuf[127][1020];
        }
        else if (eEmemType == EMEM_INFO) 
        {
            nRetVal = szTmpBuf[129][1023];
            nRetVal = (nRetVal << 8) | szTmpBuf[129][1022];
            nRetVal = (nRetVal << 8) | szTmpBuf[129][1021];
            nRetVal = (nRetVal << 8) | szTmpBuf[129][1020];
        }
    }

    return nRetVal;
}

static s32 _DrvMsg28xxCheckFirmwareBinIntegrity(u8 szFwData[][1024])
{
    u32 nCrcMain = 0, nCrcMainBin = 0;
    u32 nCrcInfo = 0, nCrcInfoBin = 0;
    u32 nRetVal = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
	
    _DrvMsg28xxConvertFwDataTwoDimenToOneDimen(szFwData, _gOneDimenFwData);
    
    /* Calculate main block CRC & info block CRC by device driver itself */
    nCrcMain = _DrvMsg28xxCalculateCrc(_gOneDimenFwData, 0, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
    nCrcInfo = _DrvMsg28xxCalculateCrc(_gOneDimenFwData, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE, MSG28XX_FIRMWARE_INFO_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);

    /* Read main block CRC & info block CRC from firmware bin file */
    nCrcMainBin = _DrvMsg28xxRetrieveFrimwareCrcFromBinFile(szFwData, EMEM_MAIN);
    nCrcInfoBin = _DrvMsg28xxRetrieveFrimwareCrcFromBinFile(szFwData, EMEM_INFO);

    DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainBin=0x%x, nCrcInfoBin=0x%x\n",
               nCrcMain, nCrcInfo, nCrcMainBin, nCrcInfoBin);

    if ((nCrcMainBin != nCrcMain) || (nCrcInfoBin != nCrcInfo))
    {
        DBG(&g_I2cClient->dev, "CHECK FIRMWARE BIN FILE INTEGRITY FAILED. CANCEL UPDATE FIRMWARE.\n");
      
        nRetVal = -1;
    } 
    else
    {
        DBG(&g_I2cClient->dev, "CHECK FIRMWARE BIN FILE INTEGRITY SUCCESS. PROCEED UPDATE FIRMWARE.\n");

        nRetVal = 0;
    }

    return nRetVal;
}

static s32 _DrvMsg28xxUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u32 nCrcMain = 0, nCrcMainHardware = 0, nCrcMainEflash = 0;
    u32 nCrcInfo = 0, nCrcInfoHardware = 0, nCrcInfoEflash = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    if (_DrvMsg28xxCheckFirmwareBinIntegrity(szFwData) < 0)
    {
        DBG(&g_I2cClient->dev, "CHECK FIRMWARE BIN FILE INTEGRITY FAILED. CANCEL UPDATE FIRMWARE.\n");

        g_FwDataCount = 0; // Reset g_FwDataCount to 0 

        DrvTouchDeviceHwReset();

        return -1;	
    }

    g_IsUpdateFirmware = 0x01; // Set flag to 0x01 for indicating update firmware is processing
    
    /////////////////////////
    // Erase
    /////////////////////////

    if (eEmemType == EMEM_ALL)
    {
        _DrvMsg28xxEraseEmem(EMEM_MAIN);
        _DrvMsg28xxEraseEmem(EMEM_INFO);
    }
    else if (eEmemType == EMEM_MAIN)
    {
        _DrvMsg28xxEraseEmem(EMEM_MAIN);
    }
    else if (eEmemType == EMEM_INFO)
    {
        _DrvMsg28xxEraseEmem(EMEM_INFO);
    }

    DBG(&g_I2cClient->dev, "erase OK\n");

    /////////////////////////
    // Program
    /////////////////////////

    if (eEmemType == EMEM_ALL)
    {
        _DrvMsg28xxProgramEmem(EMEM_MAIN);
        _DrvMsg28xxProgramEmem(EMEM_INFO);
    }
    else if (eEmemType == EMEM_MAIN)
    {
        _DrvMsg28xxProgramEmem(EMEM_MAIN);
    }
    else if (eEmemType == EMEM_INFO)
    {
        _DrvMsg28xxProgramEmem(EMEM_INFO);
    }
    
    DBG(&g_I2cClient->dev, "program OK\n");

    /* Calculate main block CRC & info block CRC by device driver itself */
    _DrvMsg28xxConvertFwDataTwoDimenToOneDimen(szFwData, _gOneDimenFwData);
    
    /* Read main block CRC & info block CRC from TP */
    if (eEmemType == EMEM_ALL)
    {
        nCrcMain = _DrvMsg28xxCalculateCrc(_gOneDimenFwData, 0, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
        nCrcInfo = _DrvMsg28xxCalculateCrc(_gOneDimenFwData, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE, MSG28XX_FIRMWARE_INFO_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);

        nCrcMainHardware = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);
        nCrcInfoHardware = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);

        nCrcMainEflash = _DrvMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);
        nCrcInfoEflash = _DrvMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_INFO);
    }
    else if (eEmemType == EMEM_MAIN)
    {
        nCrcMain = _DrvMsg28xxCalculateCrc(_gOneDimenFwData, 0, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
        nCrcMainHardware = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);
        nCrcMainEflash = _DrvMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);
    }
    else if (eEmemType == EMEM_INFO)
    {
        nCrcInfo = _DrvMsg28xxCalculateCrc(_gOneDimenFwData, MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE, MSG28XX_FIRMWARE_INFO_BLOCK_SIZE*1024-MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE-MSG28XX_EMEM_SIZE_BYTES_ONE_WORD);
        nCrcInfoHardware = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);
        nCrcInfoEflash = _DrvMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_INFO);
    }

    DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcInfo=0x%x, nCrcMainHardware=0x%x, nCrcInfoHardware=0x%x, nCrcMainEflash=0x%x, nCrcInfoEflash=0x%x\n",
               nCrcMain, nCrcInfo, nCrcMainHardware, nCrcInfoHardware, nCrcMainEflash, nCrcInfoEflash);

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DrvTouchDeviceHwReset();
    mdelay(300);

    g_IsUpdateFirmware = 0x00; // Set flag to 0x00 for indicating update firmware is finished

    if (eEmemType == EMEM_ALL)
    {
        if ((nCrcMainHardware != nCrcMain) || (nCrcInfoHardware != nCrcInfo) || (nCrcMainEflash != nCrcMain) || (nCrcInfoEflash != nCrcInfo))
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_MAIN)
    {
        if ((nCrcMainHardware != nCrcMain) || (nCrcMainEflash != nCrcMain))
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_INFO)
    {
        if ((nCrcInfoHardware != nCrcInfo) || (nCrcInfoEflash != nCrcInfo))
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }

    DBG(&g_I2cClient->dev, "Update SUCCESS\n");

    return 0;
}

s32 DrvUpdateFirmwareCash(u8 szFwData[][1024], EmemType_e eEmemType)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DBG(&g_I2cClient->dev, "g_ChipType = 0x%x\n", g_ChipType);
    
    if (g_ChipType == CHIP_TYPE_MSG22XX) // (0x7A)
    {
        _DrvMsg22xxGetTpVendorCode(_gTpVendorCode);
        
        if (_gTpVendorCode[0] == 'C' && _gTpVendorCode[1] == 'N' && _gTpVendorCode[2] == 'T') // for specific TP vendor which store some important information in info block, only update firmware for main block, do not update firmware for info block.
        {
            return _DrvMsg22xxUpdateFirmware(szFwData, EMEM_MAIN);
        }
        else
        {
            return _DrvMsg22xxUpdateFirmware(szFwData, EMEM_ALL);
        }
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A) // (0x85) || (0xBF) || (0x2117) || (0x2118)
    {
        DBG(&g_I2cClient->dev, "IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = %d\n", IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED);

        if (IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED) // Force to update firmware, do not check whether the vendor id of the update firmware bin file is equal to the vendor id on e-flash.
        {
            return _DrvMsg28xxUpdateFirmware(szFwData, EMEM_MAIN);
        }
        else
        {
            u16 eSwId = 0x0000;
            u16 eVendorId = 0x0000;

            eVendorId = szFwData[129][1005] << 8 | szFwData[129][1004]; // Retrieve major from info block
            eSwId = _DrvMsg28xxGetSwId(EMEM_INFO);

            DBG(&g_I2cClient->dev, "eVendorId = 0x%x, eSwId = 0x%x\n", eVendorId, eSwId);
    		
            // Check if the vendor id of the update firmware bin file is equal to the vendor id on e-flash. YES => allow update, NO => not allow update
            if (eSwId != eVendorId)
            {
                DrvTouchDeviceHwReset(); // Reset HW here to avoid touch may be not worked after get sw id. 

                DBG(&g_I2cClient->dev, "The vendor id of the update firmware bin file is different from the vendor id on e-flash. Not allow to update.\n");
                g_FwDataCount = 0; // Reset g_FwDataCount to 0

                return -1;
            }
            else
            {
                return _DrvMsg28xxUpdateFirmware(szFwData, EMEM_MAIN);
            }
        }
    }
    else 
    {
        DBG(&g_I2cClient->dev, "Unsupported chip type = 0x%x\n", g_ChipType);
        g_FwDataCount = 0; // Reset g_FwDataCount to 0

        return -1;
    }	
}

s32 DrvUpdateFirmwareBySdCard(const char *pFilePath)
{
    s32 nRetVal = 0;
    struct file *pfile = NULL;
    struct inode *inode;
    s32 fsize = 0;
    mm_segment_t old_fs;
    loff_t pos;
    u16 eSwId = 0x0000;
    u16 eVendorId = 0x0000;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    pfile = filp_open(pFilePath, O_RDONLY, 0);
    if (IS_ERR(pfile))
    {
        DBG(&g_I2cClient->dev, "Error occurred while opening file %s.\n", pFilePath);
        return -1;
    }

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 18, 0)
    inode = pfile->f_dentry->d_inode;
#else
    inode = pfile->f_path.dentry->d_inode;
#endif

    fsize = inode->i_size;

    DBG(&g_I2cClient->dev, "fsize = %d\n", fsize);

    if (fsize <= 0)
    {
        filp_close(pfile, NULL);
        return -1;
    }

    // read firmware
    memset(_gFwDataBuf, 0, MSG28XX_FIRMWARE_WHOLE_SIZE*1024);

    old_fs = get_fs();
    set_fs(KERNEL_DS);
  
    pos = 0;
    vfs_read(pfile, _gFwDataBuf, fsize, &pos);
  
    filp_close(pfile, NULL);
    set_fs(old_fs);

    _DrvStoreFirmwareData(_gFwDataBuf, fsize);

    DrvDisableFingerTouchReport();
    
    if (g_ChipType == CHIP_TYPE_MSG22XX)    
    {
        eVendorId = g_FwData[47][1013] <<8 | g_FwData[47][1012];
        eSwId = _DrvMsg22xxGetSwId(EMEM_MAIN);
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A)    
    {
        eVendorId = g_FwData[129][1005] << 8 | g_FwData[129][1004]; // Retrieve major from info block
        eSwId = _DrvMsg28xxGetSwId(EMEM_INFO);
    }

    DBG(&g_I2cClient->dev, "eVendorId = 0x%x, eSwId = 0x%x\n", eVendorId, eSwId);
    DBG(&g_I2cClient->dev, "IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED = %d\n", IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED);
    		
    if ((eSwId == eVendorId) || (IS_FORCE_TO_UPDATE_FIRMWARE_ENABLED))
    {
        if ((g_ChipType == CHIP_TYPE_MSG22XX && fsize == 49664/* 48.5KB */))
        {
            nRetVal = DrvUpdateFirmwareCash(g_FwData, EMEM_ALL);
        }
        else if ((g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A) && fsize == 133120/* 130KB */)
        {
            nRetVal = DrvUpdateFirmwareCash(g_FwData, EMEM_MAIN); // For MSG28xx sine mode requirement, update main block only, do not update info block.
        }
        else
       	{
            DrvTouchDeviceHwReset();

            DBG(&g_I2cClient->dev, "The file size of the update firmware bin file is not supported, fsize = %d\n", fsize);
            nRetVal = -1;
        }
    }
    else 
    {
        DrvTouchDeviceHwReset();

        DBG(&g_I2cClient->dev, "The vendor id of the update firmware bin file is different from the vendor id on e-flash. Not allow to update.\n");
        nRetVal = -1;
    }
 
    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware
    
    DrvEnableFingerTouchReport();

    return nRetVal;
}

//------------------------------------------------------------------------------//

static void _DrvReadReadDQMemStart(void)
{
    u8 nParCmdSelUseCfg = 0x7F;
    u8 nParCmdAdByteEn0 = 0x50;
    u8 nParCmdAdByteEn1 = 0x51;
    u8 nParCmdDaByteEn0 = 0x54;
    u8 nParCmdUSetSelB0 = 0x80;
    u8 nParCmdUSetSelB1 = 0x82;
    u8 nParCmdSetSelB2  = 0x85;
    u8 nParCmdIicUse    = 0x35;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSelUseCfg, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdAdByteEn1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdDaByteEn0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB0, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdUSetSelB1, 1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdSetSelB2,  1);
    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdIicUse,    1);
}

static void _DrvReadReadDQMemEnd(void)
{
    u8 nParCmdNSelUseCfg = 0x7E;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    IicWriteData(SLAVE_I2C_ID_DBBUS, &nParCmdNSelUseCfg, 1);
}

u32 DrvReadDQMemValue(u16 nAddr)
{
    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A)
    {
        u8 tx_data[3] = {0x10, (nAddr >> 8) & 0xFF, nAddr & 0xFF};
        u8 rx_data[4] = {0};

        DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

        DBG(&g_I2cClient->dev, "DQMem Addr = 0x%x\n", nAddr);

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();

        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073
        mdelay(100);

        _DrvReadReadDQMemStart();

        IicWriteData(SLAVE_I2C_ID_DBBUS, &tx_data[0], 3);
        IicReadData(SLAVE_I2C_ID_DBBUS, &rx_data[0], 4);

        _DrvReadReadDQMemEnd();

        // Start mcu
        RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
    
        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        return (rx_data[3] <<24 | rx_data[2] <<16 | rx_data[1] << 8 | rx_data[0]);
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

        // TODO : not support yet
    
        return 0;	
    }
}

void DrvWriteDQMemValue(u16 nAddr, u32 nData)
{
    if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A)
    {
        u8 szDbBusTxData[7] = {0};

        DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

        DBG(&g_I2cClient->dev, "DQMem Addr = 0x%x\n", nAddr);

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
    
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); //bank:mheg5, addr:h0073
        mdelay(100);

        _DrvReadReadDQMemStart();
    
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = ((nAddr >> 8) & 0xff);
        szDbBusTxData[2] = (nAddr & 0xff);
        szDbBusTxData[3] = nData & 0x000000FF;
        szDbBusTxData[4] = ((nData & 0x0000FF00) >> 8);
        szDbBusTxData[5] = ((nData & 0x00FF0000) >> 16);
        szDbBusTxData[6] = ((nData & 0xFF000000) >> 24);
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 7);

        _DrvReadReadDQMemEnd();

    	  // Start mcu
        RegSetLByteValue(0x0FE6, 0x00); //bank:mheg5, addr:h0073
        mdelay(100);

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

        // TODO : not support yet
    }
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
void DrvCheckFirmwareUpdateBySwId(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    if (g_ChipType == CHIP_TYPE_MSG22XX)    
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        _DrvMsg22xxCheckFirmwareUpdateBySwId();
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX        
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A)    
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
        _DrvMsg28xxCheckFirmwareUpdateBySwId();
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    }
    else
    {
        DBG(&g_I2cClient->dev, "This chip type (0x%x) does not support update firmware by sw id\n", g_ChipType);
    }
}	

//-------------------------Start of SW ID for MSG22XX----------------------------//

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
static void _DrvMsg22xxEraseEmem(EmemType_e eEmemType)
{
    u32 i = 0;
    u32 nTimeOut = 0;
    u16 nRegData = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    DBG(&g_I2cClient->dev, "Erase start\n");

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 
    
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    // Exit flash low power mode
    RegSetLByteValue(0x1619, BIT1); 

    // Change PIU clock to 48MHz
    RegSetLByteValue(0x1E23, BIT6); 

    // Change mcu clock deglitch mux source
    RegSetLByteValue(0x1E54, BIT0); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // Set PROGRAM password
    RegSetLByteValue(0x161A, 0xBA);
    RegSetLByteValue(0x161B, 0xAB);

    if (eEmemType == EMEM_ALL) // 48KB + 512Byte
    {
        DBG(&g_I2cClient->dev, "Erase all block\n");

        // Clear pce
        RegSetLByteValue(0x1618, 0x80);
        mdelay(100);

        // Chip erase
        RegSet16BitValue(0x160E, BIT3);

        DBG(&g_I2cClient->dev, "Wait erase done flag\n");

        while (1) // Wait erase done flag
        {
            nRegData = RegGet16BitValue(0x1610); // Memory status
            nRegData = nRegData & BIT1;
        
            DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT1)
            {
                break;		
            }

            mdelay(50);

            if ((nTimeOut ++) > 30)
            {
                DBG(&g_I2cClient->dev, "Erase all block failed. Timeout.\n");

                goto EraseEnd;
            }
        }
    }
    else if (eEmemType == EMEM_MAIN) // 48KB (32+8+8)
    {
        DBG(&g_I2cClient->dev, "Erase main block\n");

        for (i = 0; i < 3; i ++)
        {
            // Clear pce
            RegSetLByteValue(0x1618, 0x80);
            mdelay(10);

            if (i == 0)
            {
                RegSet16BitValue(0x1600, 0x0000);
            }
            else if (i == 1)
            {
                RegSet16BitValue(0x1600, 0x8000);
            }
            else if (i == 2)
            {
                RegSet16BitValue(0x1600, 0xA000);
            }

            // Sector erase
            RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

            DBG(&g_I2cClient->dev, "Wait erase done flag\n");

            nRegData = 0;
            nTimeOut = 0;

            while (1) // Wait erase done flag
            {
                nRegData = RegGet16BitValue(0x1610); // Memory status
                nRegData = nRegData & BIT1;
        
                DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

                if (nRegData == BIT1)
                {
                    break;		
                }
                mdelay(50);

                if ((nTimeOut ++) > 30)
                {
                    DBG(&g_I2cClient->dev, "Erase main block failed. Timeout.\n");

                    goto EraseEnd;
                }
            }
        }   
    }
    else if (eEmemType == EMEM_INFO) // 512Byte
    {
        DBG(&g_I2cClient->dev, "Erase info block\n");

        // Clear pce
        RegSetLByteValue(0x1618, 0x80);
        mdelay(10);

        RegSet16BitValue(0x1600, 0xC000);
    
        // Sector erase
        RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

        DBG(&g_I2cClient->dev, "Wait erase done flag\n");

        while (1) // Wait erase done flag
        {
            nRegData = RegGet16BitValue(0x1610); // Memory status
            nRegData = nRegData & BIT1;
        
            DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT1)
            {
                break;		
            }
            mdelay(50);

            if ((nTimeOut ++) > 30)
            {
                DBG(&g_I2cClient->dev, "Erase info block failed. Timeout.\n");

                goto EraseEnd;
            }
        }
    }

    EraseEnd:
    
    DBG(&g_I2cClient->dev, "Erase end\n");

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
}

static void _DrvMsg22xxProgramEmem(EmemType_e eEmemType) 
{
    u32 i, j; 
    u32 nRemainSize = 0, nBlockSize = 0, nSize = 0, index = 0;
    u32 nTimeOut = 0;
    u16 nRegData = 0;
    s32 rc = 0;
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    u8 szDbBusTxData[128] = {0};
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    u32 nSizePerWrite = 1;
#else 
    u32 nSizePerWrite = 125;
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    u8 szDbBusTxData[1024] = {0};
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    u32 nSizePerWrite = 1;
#else
    u32 nSizePerWrite = 1021;
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
#endif

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
    {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        nSizePerWrite = 41; // 123/3=41
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        nSizePerWrite = 340; // 1020/3=340
#endif
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        nSizePerWrite = 31; // 124/4=31
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        nSizePerWrite = 255; // 1020/4=255
#endif
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
    }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();

    // Hold reset pin before program
    RegSetLByteValue(0x1E06, 0x00);

    DBG(&g_I2cClient->dev, "Program start\n");

    RegSet16BitValue(0x161A, 0xABBA);
    RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

    if (eEmemType == EMEM_MAIN)
    {
        DBG(&g_I2cClient->dev, "Program main block\n");

        RegSet16BitValue(0x1600, 0x0000); // Set start address of main block
        nRemainSize = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024; //48KB
        index = 0;
    }
    else if (eEmemType == EMEM_INFO)
    {
        DBG(&g_I2cClient->dev, "Program info block\n");

        RegSet16BitValue(0x1600, 0xC000); // Set start address of info block
        nRemainSize = MSG22XX_FIRMWARE_INFO_BLOCK_SIZE; //512Byte
        index = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024;
    }
    else
    {
        DBG(&g_I2cClient->dev, "eEmemType = %d is not supported for program e-memory.\n", eEmemType);
        return;
    }

    RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01)); // Enable burst mode

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
    {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
        RegSet16BitValueOn(0x160A, BIT0); // Set Bit0 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
        RegSet16BitValueOn(0x160A, BIT0);
        RegSet16BitValueOn(0x160A, BIT1); // Set Bit0 = 1, Bit1 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
    }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    // Program start
    szDbBusTxData[0] = 0x10;
    szDbBusTxData[1] = 0x16;
    szDbBusTxData[2] = 0x02;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);    

    szDbBusTxData[0] = 0x20;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

    i = 0;
    
    while (nRemainSize > 0)
    {
        if (nRemainSize > nSizePerWrite)
        {
            nBlockSize = nSizePerWrite;
        }
        else
        {
            nBlockSize = nRemainSize;
        }

        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x16;
        szDbBusTxData[2] = 0x02;

        nSize = 3;

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05)
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            u32 k = 0;

            for (j = 0; j < nBlockSize; j ++)
            {
                for (k = 0; k < 3; k ++)
                {
                    szDbBusTxData[3+(j*3)+k] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
                }
                nSize = nSize + 3;
            }
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            u32 k = 0;

            for (j = 0; j < nBlockSize; j ++)
            {
                for (k = 0; k < 4; k ++)
                {
                    szDbBusTxData[3+(j*4)+k] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
                }
                nSize = nSize + 4;
            }
#else //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_C
            for (j = 0; j < nBlockSize; j ++)
            {
                szDbBusTxData[3+j] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
                nSize ++; 
            }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
        else
        {
            for (j = 0; j < nBlockSize; j ++)
            {
                szDbBusTxData[3+j] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
                nSize ++; 
            }
        }
#else
        for (j = 0; j < nBlockSize; j ++)
        {
            szDbBusTxData[3+j] = _gOneDimenFwData[index+(i*nSizePerWrite)+j];
            nSize ++; 
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        i ++;

        rc = IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);
        if (rc < 0)
        {
            DBG(&g_I2cClient->dev, "Write firmware data failed, rc = %d. Exit program procedure.\n", rc);

            goto ProgramEnd;
        }

        nRemainSize = nRemainSize - nBlockSize;
    }

    // Program end
    szDbBusTxData[0] = 0x21;

    IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
    {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
        RegSet16BitValueOff(0x160A, BIT0); // Set Bit0 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
        RegSet16BitValueOff(0x160A, BIT0);
        RegSet16BitValueOff(0x160A, BIT1); // Set Bit0 = 0, Bit1 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
    }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01)); // Clear burst mode

    DBG(&g_I2cClient->dev, "Wait write done flag\n");

    while (1) // Wait write done flag
    {
        // Polling 0x1610 is 0x0002
        nRegData = RegGet16BitValue(0x1610); // Memory status
        nRegData = nRegData & BIT1;
    
        DBG(&g_I2cClient->dev, "Wait write done flag nRegData = 0x%x\n", nRegData);

        if (nRegData == BIT1)
        {
            break;		
        }
        mdelay(10);

        if ((nTimeOut ++) > 30)
        {
            DBG(&g_I2cClient->dev, "Write failed. Timeout.\n");

            goto ProgramEnd;
        }
    }

    ProgramEnd:

    DBG(&g_I2cClient->dev, "Program end\n");

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();
}

static u32 _DrvMsg22xxRetrieveFirmwareCrcFromEFlash(EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    u16 nRegData1 = 0, nRegData2 = 0;

    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);

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

    if (eEmemType == EMEM_MAIN) // Read main block CRC(48KB-4) from main block
    {
        RegSet16BitValue(0x1600, 0xBFFC); // Set start address for main block CRC
    }
    else if (eEmemType == EMEM_INFO) // Read info block CRC(512Byte-4) from info block
    {
        RegSet16BitValue(0x1600, 0xC1FC); // Set start address for info block CRC
    }
    
    // Enable burst mode
    RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

    RegSetLByteValue(0x160E, 0x01); 

    nRegData1 = RegGet16BitValue(0x1604);
    nRegData2 = RegGet16BitValue(0x1606);

    nRetVal  = ((nRegData2 >> 8) & 0xFF) << 24;
    nRetVal |= (nRegData2 & 0xFF) << 16;
    nRetVal |= ((nRegData1 >> 8) & 0xFF) << 8;
    nRetVal |= (nRegData1 & 0xFF);
    
    DBG(&g_I2cClient->dev, "CRC = 0x%x\n", nRetVal);

    // Clear burst mode
    RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

    RegSet16BitValue(0x1600, 0x0000); 

    // Clear RIU password
    RegSet16BitValue(0x161A, 0x0000); 

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    return nRetVal;	
}

static u32 _DrvMsg22xxRetrieveFrimwareCrcFromBinFile(u8 szTmpBuf[], EmemType_e eEmemType) 
{
    u32 nRetVal = 0; 
    
    DBG(&g_I2cClient->dev, "*** %s() eEmemType = %d ***\n", __func__, eEmemType);
    
    if (szTmpBuf != NULL)
    {
        if (eEmemType == EMEM_MAIN) // Read main block CRC(48KB-4) from bin file
        {
            nRetVal  = szTmpBuf[0xBFFF] << 24;
            nRetVal |= szTmpBuf[0xBFFE] << 16;
            nRetVal |= szTmpBuf[0xBFFD] << 8;
            nRetVal |= szTmpBuf[0xBFFC];
        }
        else if (eEmemType == EMEM_INFO) // Read info block CRC(512Byte-4) from bin file
        {
            nRetVal  = szTmpBuf[0xC1FF] << 24;
            nRetVal |= szTmpBuf[0xC1FE] << 16;
            nRetVal |= szTmpBuf[0xC1FD] << 8;
            nRetVal |= szTmpBuf[0xC1FC];
        }
    }

    return nRetVal;
}

static s32 _DrvMsg22xxUpdateFirmwareBySwId(void) 
{
    s32 nRetVal = -1;
    u32 nCrcInfoA = 0, nCrcInfoB = 0, nCrcMainA = 0, nCrcMainB = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    DBG(&g_I2cClient->dev, "_gIsUpdateInfoBlockFirst = %d, g_IsUpdateFirmware = 0x%x\n", _gIsUpdateInfoBlockFirst, g_IsUpdateFirmware);

    _DrvMsg22xxConvertFwDataTwoDimenToOneDimen(g_FwData, _gOneDimenFwData);
    
    if (_gIsUpdateInfoBlockFirst == 1)
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvMsg22xxEraseEmem(EMEM_INFO);
            _DrvMsg22xxProgramEmem(EMEM_INFO);
 
            nCrcInfoA = _DrvMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_INFO);
            nCrcInfoB = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG(&g_I2cClient->dev, "nCrcInfoA = 0x%x, nCrcInfoB = 0x%x\n", nCrcInfoA, nCrcInfoB);
        
            if (nCrcInfoA == nCrcInfoB)
            {
                _DrvMsg22xxEraseEmem(EMEM_MAIN);
                _DrvMsg22xxProgramEmem(EMEM_MAIN);

                nCrcMainA = _DrvMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_MAIN);
                nCrcMainB = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

                DBG(&g_I2cClient->dev, "nCrcMainA = 0x%x, nCrcMainB = 0x%x\n", nCrcMainA, nCrcMainB);
        		
                if (nCrcMainA == nCrcMainB)
                {
                    g_IsUpdateFirmware = 0x00;
                    nRetVal = 0;
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvMsg22xxEraseEmem(EMEM_MAIN);
            _DrvMsg22xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_MAIN);
            nCrcMainB = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);
    		
            if (nCrcMainA == nCrcMainB)
            {
                g_IsUpdateFirmware = 0x00;
                nRetVal = 0;
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }
    }
    else //_gIsUpdateInfoBlockFirst == 0
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvMsg22xxEraseEmem(EMEM_MAIN);
            _DrvMsg22xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_MAIN);
            nCrcMainB = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB)
            {
                _DrvMsg22xxEraseEmem(EMEM_INFO);
                _DrvMsg22xxProgramEmem(EMEM_INFO);

                nCrcInfoA = _DrvMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_INFO);
                nCrcInfoB = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);
                
                DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

                if (nCrcInfoA == nCrcInfoB)
                {
                    g_IsUpdateFirmware = 0x00;
                    nRetVal = 0;
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvMsg22xxEraseEmem(EMEM_INFO);
            _DrvMsg22xxProgramEmem(EMEM_INFO);

            nCrcInfoA = _DrvMsg22xxRetrieveFrimwareCrcFromBinFile(_gOneDimenFwData, EMEM_INFO);
            nCrcInfoB = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

            if (nCrcInfoA == nCrcInfoB)
            {
                g_IsUpdateFirmware = 0x00;
                nRetVal = 0;
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }    		
    }
    
    return nRetVal;	
}

static void _DrvMsg22xxCheckFirmwareUpdateBySwId(void)
{
    u32 nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB;
    u32 i, j;
    u32 nSwIdListNum = 0;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 nIsSwIdValid = 0;
    u8 nFinalIndex = 0;
    u8 *pVersion = NULL;
    Msg22xxSwId_e eSwId = MSG22XX_SW_ID_UNDEFINED;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    DBG(&g_I2cClient->dev, "*** g_Msg22xxChipRevision = 0x%x ***\n", g_Msg22xxChipRevision);

    DrvDisableFingerTouchReport();

    nSwIdListNum  = sizeof(_gSwIdData) / sizeof(SwIdData_t);
    DBG(&g_I2cClient->dev, "*** sizeof(_gSwIdData)=%d, sizeof(SwIdData_t)=%d, nSwIdListNum=%d ***\n", (int)sizeof(_gSwIdData), (int)sizeof(SwIdData_t), (int)nSwIdListNum);

    nCrcMainA = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);
    nCrcMainB = _DrvMsg22xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);

    nCrcInfoA = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);
    nCrcInfoB = _DrvMsg22xxRetrieveFirmwareCrcFromEFlash(EMEM_INFO);
    
    _gUpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
    INIT_WORK(&_gUpdateFirmwareBySwIdWork, _DrvUpdateFirmwareBySwIdDoWork);

    DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcInfoA=0x%x, nCrcMainB=0x%x, nCrcInfoB=0x%x\n", nCrcMainA, nCrcInfoA, nCrcMainB, nCrcInfoB);

    if (nCrcMainA == nCrcMainB && nCrcInfoA == nCrcInfoB) // Case 1. Main Block:OK, Info Block:OK
    {
        eSwId = _DrvMsg22xxGetSwId(EMEM_MAIN);
    		
        nIsSwIdValid = 0;
        
        for (j = 0; j < nSwIdListNum; j ++)
        {
            DBG(&g_I2cClient->dev, "_gSwIdData[%d].nSwId = 0x%x\n", j, _gSwIdData[j].nSwId); // TODO : add for debug

            if (eSwId == _gSwIdData[j].nSwId)
            {
                nUpdateBinMajor = (*(_gSwIdData[j].pUpdateBin+0xBFF5))<<8 | (*(_gSwIdData[j].pUpdateBin+0xBFF4));
                nUpdateBinMinor = (*(_gSwIdData[j].pUpdateBin+0xBFF7))<<8 | (*(_gSwIdData[j].pUpdateBin+0xBFF6));
                nIsSwIdValid = 1;
                nFinalIndex = j;
                
                break;
            }
        }

        if (0 == nIsSwIdValid)
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG22XX_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;    		        						
        }
    		
        DrvGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);

        DBG(&g_I2cClient->dev, "eSwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%d, nUpdateBinMinor=%d\n", eSwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);

        if (nUpdateBinMinor > nMinor)
        {
            if (1 == nIsSwIdValid)
            {
                DBG(&g_I2cClient->dev, "nFinalIndex = %d\n", nFinalIndex); // TODO : add for debug

                for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                {
                    if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                    {
                        _DrvStoreFirmwareData((_gSwIdData[nFinalIndex].pUpdateBin+i*1024), 1024);
                    }
                    else // i == 48
                    {
                        _DrvStoreFirmwareData((_gSwIdData[nFinalIndex].pUpdateBin+i*1024), 512);
                    }
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

                eSwId = MSG22XX_SW_ID_UNDEFINED;
            }

            if (eSwId < MSG22XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
            {
                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
                _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
                g_IsUpdateFirmware = 0x11;
                queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
                return;
            }
            else
            {
                DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
                DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "The update bin version is older than or equal to the current firmware version on e-flash.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else if (nCrcMainA == nCrcMainB && nCrcInfoA != nCrcInfoB) // Case 2. Main Block:OK, Info Block:FAIL
    {
        eSwId = _DrvMsg22xxGetSwId(EMEM_MAIN);
    		
        DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);

        nIsSwIdValid = 0;

        for (j = 0; j < nSwIdListNum; j ++)
        {
            DBG(&g_I2cClient->dev, "_gSwIdData[%d].nSwId = 0x%x\n", j, _gSwIdData[j].nSwId); // TODO : add for debug

            if (eSwId == _gSwIdData[j].nSwId)
            {
                for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                {
                    if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                    {
                        _DrvStoreFirmwareData((_gSwIdData[j].pUpdateBin+i*1024), 1024);
                    }
                    else // i == 48
                    {
                        _DrvStoreFirmwareData((_gSwIdData[j].pUpdateBin+i*1024), 512);
                    }
                }
                nIsSwIdValid = 1;
                
                break;
            }
        }

        if (0 == nIsSwIdValid)
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG22XX_SW_ID_UNDEFINED;
        }

        if (eSwId < MSG22XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
            g_IsUpdateFirmware = 0x11;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else if (nCrcMainA != nCrcMainB && nCrcInfoA == nCrcInfoB) // Case 3. Main Block:FAIL, Info Block:OK
    {
        eSwId = _DrvMsg22xxGetSwId(EMEM_INFO);
		
        DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);

        nIsSwIdValid = 0;

        for (j = 0; j < nSwIdListNum; j ++)
        {
            DBG(&g_I2cClient->dev, "_gSwIdData[%d].nSwId = 0x%x\n", j, _gSwIdData[j].nSwId); // TODO : add for debug

            if (eSwId == _gSwIdData[j].nSwId)
            {
                for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i ++)
                {
                    if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) // i < 48
                    {
                        _DrvStoreFirmwareData((_gSwIdData[j].pUpdateBin+i*1024), 1024);
                    }
                    else // i == 48
                    {
                        _DrvStoreFirmwareData((_gSwIdData[j].pUpdateBin+i*1024), 512);
                    }
                }
                nIsSwIdValid = 1;
                
                break;
            }
        }

        if (0 == nIsSwIdValid)
      	{
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG22XX_SW_ID_UNDEFINED;
        }

        if (eSwId < MSG22XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            _gIsUpdateInfoBlockFirst = 0; // Set 0 for indicating main block is broken 
            g_IsUpdateFirmware = 0x11;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else // Case 4. Main Block:FAIL, Info Block:FAIL
    {
        DBG(&g_I2cClient->dev, "Main block and Info block are broken.\n");
        DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
    }

    DrvTouchDeviceHwReset();

    DrvEnableFingerTouchReport();
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX

//-------------------------End of SW ID for MSG22XX----------------------------//

//-------------------------Start of SW ID for MSG28XX----------------------------//

#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
/*
static s32 _DrvMsg28xxUpdateFirmwareBySwId(void) 
{
    s32 nRetVal = -1;
    u32 nCrcInfoA = 0, nCrcInfoB = 0, nCrcMainA = 0, nCrcMainB = 0;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    DBG(&g_I2cClient->dev, "_gIsUpdateInfoBlockFirst = %d, g_IsUpdateFirmware = 0x%x\n", _gIsUpdateInfoBlockFirst, g_IsUpdateFirmware);

    if (_gIsUpdateInfoBlockFirst == 1)
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvMsg28xxEraseEmem(EMEM_INFO);
            _DrvMsg28xxProgramEmem(EMEM_INFO);
 
            nCrcInfoA = _DrvMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
            nCrcInfoB = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG(&g_I2cClient->dev, "nCrcInfoA = 0x%x, nCrcInfoB = 0x%x\n", nCrcInfoA, nCrcInfoB);
        
            if (nCrcInfoA == nCrcInfoB)
            {
                _DrvMsg28xxEraseEmem(EMEM_MAIN);
                _DrvMsg28xxProgramEmem(EMEM_MAIN);

                nCrcMainA = _DrvMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
                nCrcMainB = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);

                DBG(&g_I2cClient->dev, "nCrcMainA = 0x%x, nCrcMainB = 0x%x\n", nCrcMainA, nCrcMainB);
        		
                if (nCrcMainA == nCrcMainB)
                {
                    g_IsUpdateFirmware = 0x00;
                    nRetVal = 0;
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvMsg28xxEraseEmem(EMEM_MAIN);
            _DrvMsg28xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
            nCrcMainB = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);
    		
            if (nCrcMainA == nCrcMainB)
            {
                g_IsUpdateFirmware = 0x00;
                nRetVal = 0;
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }
    }
    else //_gIsUpdateInfoBlockFirst == 0
    {
        if ((g_IsUpdateFirmware & 0x10) == 0x10)
        {
            _DrvMsg28xxEraseEmem(EMEM_MAIN);
            _DrvMsg28xxProgramEmem(EMEM_MAIN);

            nCrcMainA = _DrvMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_MAIN);
            nCrcMainB = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);

            DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB)
            {
                _DrvMsg28xxEraseEmem(EMEM_INFO);
                _DrvMsg28xxProgramEmem(EMEM_INFO);

                nCrcInfoA = _DrvMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
                nCrcInfoB = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);
                
                DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

                if (nCrcInfoA == nCrcInfoB)
                {
                    g_IsUpdateFirmware = 0x00;
                    nRetVal = 0;
                }
                else
                {
                    g_IsUpdateFirmware = 0x01;
                }
            }
            else
            {
                g_IsUpdateFirmware = 0x11;
            }
        }
        else if ((g_IsUpdateFirmware & 0x01) == 0x01)
        {
            _DrvMsg28xxEraseEmem(EMEM_INFO);
            _DrvMsg28xxProgramEmem(EMEM_INFO);

            nCrcInfoA = _DrvMsg28xxRetrieveFrimwareCrcFromBinFile(g_FwData, EMEM_INFO);
            nCrcInfoB = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_INFO);

            DBG(&g_I2cClient->dev, "nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

            if (nCrcInfoA == nCrcInfoB)
            {
                g_IsUpdateFirmware = 0x00;
                nRetVal = 0;
            }
            else
            {
                g_IsUpdateFirmware = 0x01;
            }
        }    		
    }
    
    return nRetVal;	
}
*/


static void _DrvMsg28xxCheckFirmwareUpdateBySwId(void) 
{
    u32 nCrcMainA, nCrcMainB;
    u32 i, j;
    u32 nSwIdListNum = 0;
    u16 nUpdateBinMajor = 0, nUpdateBinMinor = 0;
    u16 nMajor = 0, nMinor = 0;
    u8 nIsSwIdValid = 0;
    u8 nFinalIndex = 0;
    u8 *pVersion = NULL;
    Msg28xxSwId_e eMainSwId = MSG28XX_SW_ID_UNDEFINED, eSwId = MSG28XX_SW_ID_UNDEFINED;
    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    DrvDisableFingerTouchReport();

    nSwIdListNum  = sizeof(_gSwIdData) / sizeof(SwIdData_t);
    DBG(&g_I2cClient->dev, "*** sizeof(_gSwIdData)=%d, sizeof(SwIdData_t)=%d, nSwIdListNum=%d ***\n", (int)sizeof(_gSwIdData), (int)sizeof(SwIdData_t), (int)nSwIdListNum);

    nCrcMainA = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);
    nCrcMainB = _DrvMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);

    DBG(&g_I2cClient->dev, "nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

#ifdef CONFIG_ENABLE_CODE_FOR_DEBUG  // TODO : add for debug 
    if (nCrcMainA != nCrcMainB) 
    {
        for (i = 0; i < 5; i ++)
        {
            nCrcMainA = _DrvMsg28xxGetFirmwareCrcByHardware(EMEM_MAIN);
            nCrcMainB = _DrvMsg28xxRetrieveFirmwareCrcFromEFlash(EMEM_MAIN);
    
            DBG(&g_I2cClient->dev, "*** Retry[%d] : nCrcMainA=0x%x, nCrcMainB=0x%x ***\n", i, nCrcMainA, nCrcMainB);

            if (nCrcMainA == nCrcMainB)
            {
                break;            
            }

            mdelay(50);
        }
    }
#endif //CONFIG_ENABLE_CODE_FOR_DEBUG

    _gUpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
    INIT_WORK(&_gUpdateFirmwareBySwIdWork, _DrvUpdateFirmwareBySwIdDoWork);

    if (nCrcMainA == nCrcMainB) 
    {
        eMainSwId = _DrvMsg28xxGetSwId(EMEM_MAIN);
    		
        DBG(&g_I2cClient->dev, "eMainSwId=0x%x\n", eMainSwId);

        eSwId = eMainSwId;

        nIsSwIdValid = 0;
        
        for (j = 0; j < nSwIdListNum; j ++)
        {
            DBG(&g_I2cClient->dev, "_gSwIdData[%d].nSwId = 0x%x\n", j, _gSwIdData[j].nSwId); // TODO : add for debug

            if (eSwId == _gSwIdData[j].nSwId)
            {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                nUpdateBinMajor = ((*(_gSwIdData[j].pUpdateBin+127))[1013])<<8 | ((*(_gSwIdData[j].pUpdateBin+127))[1012]);
				nUpdateBinMinor = ((*(_gSwIdData[j].pUpdateBin+127))[1015])<<8 | ((*(_gSwIdData[j].pUpdateBin+127))[1014]);
#else // By one dimensional array
                nUpdateBinMajor = (*(_gSwIdData[j].pUpdateBin+0x1FFF5))<<8 | (*(_gSwIdData[j].pUpdateBin+0x1FFF4));
				nUpdateBinMinor = (*(_gSwIdData[j].pUpdateBin+0x1FFF7))<<8 | (*(_gSwIdData[j].pUpdateBin+0x1FFF6));
#endif //CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
                nIsSwIdValid = 1;
                nFinalIndex = j;
                
                break;
            }
        }

        if (0 == nIsSwIdValid)
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG28XX_SW_ID_UNDEFINED;
            nUpdateBinMajor = 0;
            nUpdateBinMinor = 0;    		        						
        }

        DrvGetCustomerFirmwareVersionByDbBus(EMEM_MAIN, &nMajor, &nMinor, &pVersion);
        if ((nUpdateBinMinor & 0xFF) > (nMinor & 0xFF))
        {
			if(g_FwVersionFlag)
			{
				DBG(&g_I2cClient->dev, "eSwId=0x%x, nMajor=%u, nMinor=%u.%u, nUpdateBinMajor=%u, nUpdateBinMinor=%u.%u\n", eSwId, nMajor, nMinor & 0xFF, (nMinor & 0xFF00) >> 8, nUpdateBinMajor, nUpdateBinMinor & 0xFF, (nUpdateBinMinor & 0xFF00) >> 8);
			}
			else
			{
				DBG(&g_I2cClient->dev, "eSwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%u, nUpdateBinMinor=%u\n", eSwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);
			}
            if (1 == nIsSwIdValid)
            {
                DBG(&g_I2cClient->dev, "nFinalIndex = %d\n", nFinalIndex); // TODO : add for debug

                for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvStoreFirmwareData(*(_gSwIdData[nFinalIndex].pUpdateBin+i), 1024);
#else // By one dimensional array
                    _DrvStoreFirmwareData((_gSwIdData[nFinalIndex].pUpdateBin+i*1024), 1024);
#endif //CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
                }
            }
            else
            {
                DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

                eSwId = MSG28XX_SW_ID_UNDEFINED;
            }

            if (eSwId < MSG28XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
            {
                g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

                _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
                _gIsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete 
                g_IsUpdateFirmware = 0x11;
                queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
                return;
            }
            else
            {
                DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
                DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
            }
        }
        else
        {
            DBG(&g_I2cClient->dev, "The update bin version is older than or equal to the current firmware version on e-flash.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }
    else 
    {
        eSwId = _DrvMsg28xxGetSwId(EMEM_INFO);
		
        DBG(&g_I2cClient->dev, "eSwId=0x%x\n", eSwId);
        
        nIsSwIdValid = 0;
        
        for (j = 0; j < nSwIdListNum; j ++)
        {
            DBG(&g_I2cClient->dev, "_gSwIdData[%d].nSwId = 0x%x\n", j, _gSwIdData[j].nSwId); // TODO : add for debug

            if (eSwId == _gSwIdData[j].nSwId)
            {
                for (i = 0; i < MSG28XX_FIRMWARE_WHOLE_SIZE; i ++)
                {
#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY // By two dimensional array
                    _DrvStoreFirmwareData(*(_gSwIdData[j].pUpdateBin+i), 1024);
#else // By one dimensional array
                    _DrvStoreFirmwareData((_gSwIdData[j].pUpdateBin+i*1024), 1024);
#endif //CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
                }
                nIsSwIdValid = 1;
                
                break;
            }
        }

        if (0 == nIsSwIdValid)
        {
            DBG(&g_I2cClient->dev, "eSwId = 0x%x is an undefined SW ID.\n", eSwId);

            eSwId = MSG28XX_SW_ID_UNDEFINED;
        }

        if (eSwId < MSG28XX_SW_ID_UNDEFINED && eSwId != 0x0000 && eSwId != 0xFFFF)
        {
            g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

            _gUpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
            _gIsUpdateInfoBlockFirst = 0; // Set 0 for indicating main block is broken 
            g_IsUpdateFirmware = 0x11;
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
            return;
        }
        else
        {
            DBG(&g_I2cClient->dev, "The sw id is invalid.\n");
            DBG(&g_I2cClient->dev, "Go to normal boot up process.\n");
        }
    }

    DrvTouchDeviceHwReset();

    DrvEnableFingerTouchReport();
}
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX

//-------------------------End of SW ID for MSG28XX----------------------------//

static void _DrvUpdateFirmwareBySwIdDoWork(struct work_struct *pWork)
{
    s32 nRetVal = -1;
    
    DBG(&g_I2cClient->dev, "*** %s() _gUpdateRetryCount = %d ***\n", __func__, _gUpdateRetryCount);

    if (g_ChipType == CHIP_TYPE_MSG22XX)    
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG22XX
        _DrvMsg22xxGetTpVendorCode(_gTpVendorCode);
        
        if (_gTpVendorCode[0] == 'C' && _gTpVendorCode[1] == 'N' && _gTpVendorCode[2] == 'T') // for specific TP vendor which store some important information in info block, only update firmware for main block, do not update firmware for info block.
        {
            nRetVal = _DrvMsg22xxUpdateFirmware(g_FwData, EMEM_MAIN);
        }
        else
        {
            nRetVal = _DrvMsg22xxUpdateFirmwareBySwId();
        }
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG22XX
    }
    else if (g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A)    
    {
#ifdef CONFIG_ENABLE_CHIP_TYPE_MSG28XX
        nRetVal = _DrvMsg28xxUpdateFirmware(g_FwData, EMEM_MAIN);
#endif //CONFIG_ENABLE_CHIP_TYPE_MSG28XX
    }
    else
    {
        DBG(&g_I2cClient->dev, "This chip type (0x%x) does not support update firmware by sw id\n", g_ChipType);

        DrvTouchDeviceHwReset(); 

        DrvEnableFingerTouchReport();

        nRetVal = -1;
        return;
    }
    
    DBG(&g_I2cClient->dev, "*** Update firmware by sw id result = %d ***\n", nRetVal);
    
    if (nRetVal == 0)
    {
        DBG(&g_I2cClient->dev, "Update firmware by sw id success\n");

        DrvTouchDeviceHwReset();

        DrvEnableFingerTouchReport();

        if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A)    
        {
            _gIsUpdateInfoBlockFirst = 0;
            g_IsUpdateFirmware = 0x00;
        }
    }
    else // nRetVal == -1 for MSG22xx/MSG28xx/MSG58xxa or nRetVal == 2/3/4 for ILI21xx
    {
        _gUpdateRetryCount --;
        if (_gUpdateRetryCount > 0)
        {
            DBG(&g_I2cClient->dev, "_gUpdateRetryCount = %d\n", _gUpdateRetryCount);
            queue_work(_gUpdateFirmwareBySwIdWorkQueue, &_gUpdateFirmwareBySwIdWork);
        }
        else
        {
            DBG(&g_I2cClient->dev, "Update firmware by sw id failed\n");

            DrvTouchDeviceHwReset();

            DrvEnableFingerTouchReport();

            if (g_ChipType == CHIP_TYPE_MSG22XX || g_ChipType == CHIP_TYPE_MSG28XX || g_ChipType == CHIP_TYPE_MSG58XXA || g_ChipType == CHIP_TYPE_ILI2117A || g_ChipType == CHIP_TYPE_ILI2118A)    
            {
                _gIsUpdateInfoBlockFirst = 0;
                g_IsUpdateFirmware = 0x00;
            }           
        }
    }
}

#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

//------------------------------------------------------------------------------//

static s32 _DrvMsg22xxUpdateFirmware(u8 szFwData[][1024], EmemType_e eEmemType)
{
    u32 i = 0, index = 0;
    u32 nCrcMain = 0, nCrcMainTp = 0;
    u32 nCrcInfo = 0, nCrcInfoTp = 0;
    u32 nRemainSize, nBlockSize, nSize;
    u32 nTimeOut = 0;
    u16 nRegData = 0;
    s32 rc = 0;
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    u8 szDbBusTxData[128] = {0};
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    u32 nSizePerWrite = 1;
#else 
    u32 nSizePerWrite = 125;
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    u8 szDbBusTxData[1024] = {0};
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    u32 nSizePerWrite = 1;
#else
    u32 nSizePerWrite = 1021;
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
#endif

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaReset();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    DBG(&g_I2cClient->dev, "*** g_Msg22xxChipRevision = 0x%x ***\n", g_Msg22xxChipRevision);

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05)
    {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        nSizePerWrite = 41; // 123/3=41
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        nSizePerWrite = 340; // 1020/3=340
#endif
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
        nSizePerWrite = 31; //124/4=31
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
        nSizePerWrite = 255; // 1020/4=255
#endif
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
    }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

    g_IsUpdateFirmware = 0x01; // Set flag to 0x01 for indicating update firmware is processing

    _DrvMsg22xxConvertFwDataTwoDimenToOneDimen(szFwData, _gOneDimenFwData);

    DrvTouchDeviceHwReset();

    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    
    DBG(&g_I2cClient->dev, "Erase start\n");

    // Stop MCU
    RegSetLByteValue(0x0FE6, 0x01); 

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
    // Exit flash low power mode
    RegSetLByteValue(0x1619, BIT1); 

    // Change PIU clock to 48MHz
    RegSetLByteValue(0x1E23, BIT6); 

    // Change mcu clock deglitch mux source
    RegSetLByteValue(0x1E54, BIT0); 
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K 
    // Set PROGRAM password
    RegSetLByteValue(0x161A, 0xBA);
    RegSetLByteValue(0x161B, 0xAB);

    if (eEmemType == EMEM_ALL) // 48KB + 512Byte
    {
        DBG(&g_I2cClient->dev, "Erase all block\n");

        // Clear pce
        RegSetLByteValue(0x1618, 0x80);
        mdelay(100);

        // Chip erase
        RegSet16BitValue(0x160E, BIT3);

        DBG(&g_I2cClient->dev, "Wait erase done flag\n");

        while (1) // Wait erase done flag
        {
            nRegData = RegGet16BitValue(0x1610); // Memory status
            nRegData = nRegData & BIT1;
        
            DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT1)
            {
                break;		
            }

            mdelay(50);

            if ((nTimeOut ++) > 30)
            {
                DBG(&g_I2cClient->dev, "Erase all block failed. Timeout.\n");

                goto UpdateEnd;
            }
        }
    }
    else if (eEmemType == EMEM_MAIN) // 48KB (32+8+8)
    {
        DBG(&g_I2cClient->dev, "Erase main block\n");

        for (i = 0; i < 3; i ++)
        {
            // Clear pce
            RegSetLByteValue(0x1618, 0x80);
            mdelay(10);

            if (i == 0)
            {
                RegSet16BitValue(0x1600, 0x0000);
            }
            else if (i == 1)
            {
                RegSet16BitValue(0x1600, 0x8000);
            }
            else if (i == 2)
            {
                RegSet16BitValue(0x1600, 0xA000);
            }

            // Sector erase
            RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

            DBG(&g_I2cClient->dev, "Wait erase done flag\n");

            nRegData = 0;
            nTimeOut = 0;

            while (1) // Wait erase done flag
            {
                nRegData = RegGet16BitValue(0x1610); // Memory status
                nRegData = nRegData & BIT1;
        
                DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

                if (nRegData == BIT1)
                {
                    break;		
                }
                mdelay(50);

                if ((nTimeOut ++) > 30)
                {
                    DBG(&g_I2cClient->dev, "Erase main block failed. Timeout.\n");

                    goto UpdateEnd;
                }
            }
        }   
    }
    else if (eEmemType == EMEM_INFO) // 512Byte
    {
        DBG(&g_I2cClient->dev, "Erase info block\n");

        // Clear pce
        RegSetLByteValue(0x1618, 0x80);
        mdelay(10);

        RegSet16BitValue(0x1600, 0xC000);
    
        // Sector erase
        RegSet16BitValue(0x160E, (RegGet16BitValue(0x160E) | BIT2));

        DBG(&g_I2cClient->dev, "Wait erase done flag\n");

        while (1) // Wait erase done flag
        {
            nRegData = RegGet16BitValue(0x1610); // Memory status
            nRegData = nRegData & BIT1;
        
            DBG(&g_I2cClient->dev, "Wait erase done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT1)
            {
                break;		
            }
            mdelay(50);

            if ((nTimeOut ++) > 30)
            {
                DBG(&g_I2cClient->dev, "Erase info block failed. Timeout.\n");

                goto UpdateEnd;
            }
        }
    }
    
    DBG(&g_I2cClient->dev, "Erase end\n");
    
    // Hold reset pin before program
    RegSetLByteValue(0x1E06, 0x00); 

    /////////////////////////
    // Program
    /////////////////////////

    if (eEmemType == EMEM_ALL || eEmemType == EMEM_MAIN) // 48KB
    {
        DBG(&g_I2cClient->dev, "Program main block start\n");
		
        // Program main block
        RegSet16BitValue(0x161A, 0xABBA);
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));
		
        RegSet16BitValue(0x1600, 0x0000); // Set start address of main block
        
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01)); // Enable burst mode
		
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            RegSet16BitValueOn(0x160A, BIT0); // Set Bit0 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            RegSet16BitValueOn(0x160A, BIT0);
            RegSet16BitValueOn(0x160A, BIT1); // Set Bit0 = 1, Bit1 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // Program start
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x16;
        szDbBusTxData[2] = 0x02;
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);    
		
        szDbBusTxData[0] = 0x20;
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    
		
        nRemainSize = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024; //48KB
        index = 0;
		    
        while (nRemainSize > 0)
        {
            if (nRemainSize > nSizePerWrite)
            {
                nBlockSize = nSizePerWrite;
            }
            else
            {
                nBlockSize = nRemainSize;
            }
		
            szDbBusTxData[0] = 0x10;
            szDbBusTxData[1] = 0x16;
            szDbBusTxData[2] = 0x02;
		
            nSize = 3;
		
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
            if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05)
            {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
                u32 j = 0;

                for (i = 0; i < nBlockSize; i ++)
                {
                    for (j = 0; j < 3; j ++)
                    {
                        szDbBusTxData[3+(i*3)+j] = _gOneDimenFwData[index*nSizePerWrite+i];
                    }
                    nSize = nSize + 3;
                }
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
                u32 j = 0;

                for (i = 0; i < nBlockSize; i ++)
                {
                    for (j = 0; j < 4; j ++)
                    {
                        szDbBusTxData[3+(i*4)+j] = _gOneDimenFwData[index*nSizePerWrite+i];
                    }
                    nSize = nSize + 4;
                }
#else //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_C
                for (i = 0; i < nBlockSize; i ++)
                {
                    szDbBusTxData[3+i] = _gOneDimenFwData[index*nSizePerWrite+i];
                    nSize ++; 
                }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
            }
            else
            {
                for (i = 0; i < nBlockSize; i ++)
                {
                    szDbBusTxData[3+i] = _gOneDimenFwData[index*nSizePerWrite+i];
                    nSize ++; 
                }
            }
#else
            for (i = 0; i < nBlockSize; i ++)
            {
                szDbBusTxData[3+i] = _gOneDimenFwData[index*nSizePerWrite+i];
                nSize ++; 
            }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

            index ++;
		
            rc = IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);
            if (rc < 0)
            {
                DBG(&g_I2cClient->dev, "Write firmware data failed, rc = %d. Exit program procedure.\n", rc);

                goto UpdateEnd;
            }
		        
            nRemainSize = nRemainSize - nBlockSize;
        }
		
        // Program end
        szDbBusTxData[0] = 0x21;
		
        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    
		
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            RegSet16BitValueOff(0x160A, BIT0); // Set Bit0 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            RegSet16BitValueOff(0x160A, BIT0);
            RegSet16BitValueOff(0x160A, BIT1); // Set Bit0 = 0, Bit1 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01)); // Clear burst mode

        DBG(&g_I2cClient->dev, "Wait main block write done flag\n");
		
        nRegData = 0;
        nTimeOut = 0;

        while (1) // Wait write done flag
        {
            // Polling 0x1610 is 0x0002
            nRegData = RegGet16BitValue(0x1610); // Memory status
            nRegData = nRegData & BIT1;
    
            DBG(&g_I2cClient->dev, "Wait write done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT1)
            {
                break;		
            }
            mdelay(10);

            if ((nTimeOut ++) > 30)
            {
                DBG(&g_I2cClient->dev, "Write failed. Timeout.\n");

                goto UpdateEnd;
            }
        }
    
        DBG(&g_I2cClient->dev, "Program main block end\n");
    }
    
    if (eEmemType == EMEM_ALL || eEmemType == EMEM_INFO) // 512 Byte
    {
        DBG(&g_I2cClient->dev, "Program info block start\n");

        // Program info block
        RegSet16BitValue(0x161A, 0xABBA);
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

        RegSet16BitValue(0x1600, 0xC000); // Set start address of info block
        
        RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01)); // Enable burst mode

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            RegSet16BitValueOn(0x160A, BIT0); // Set Bit0 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            RegSet16BitValueOn(0x160A, BIT0);
            RegSet16BitValueOn(0x160A, BIT1); // Set Bit0 = 1, Bit1 = 1 for enable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        // Program start
        szDbBusTxData[0] = 0x10;
        szDbBusTxData[1] = 0x16;
        szDbBusTxData[2] = 0x02;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);    

        szDbBusTxData[0] = 0x20;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

        nRemainSize = MSG22XX_FIRMWARE_INFO_BLOCK_SIZE; //512Byte
        index = 0;
    
        while (nRemainSize > 0)
        {
            if (nRemainSize > nSizePerWrite)
            {
                nBlockSize = nSizePerWrite;
            }
            else
            {
                nBlockSize = nRemainSize;
            }

            szDbBusTxData[0] = 0x10;
            szDbBusTxData[1] = 0x16;
            szDbBusTxData[2] = 0x02;

            nSize = 3;

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
            if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05)
            {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
                u32 j = 0;

                for (i = 0; i < nBlockSize; i ++)
                {
                    for (j = 0; j < 3; j ++)
                    {
                        szDbBusTxData[3+(i*3)+j] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                    }
                    nSize = nSize + 3;
                }
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
                u32 j = 0;

                for (i = 0; i < nBlockSize; i ++)
                {
                    for (j = 0; j < 4; j ++)
                    {
                        szDbBusTxData[3+(i*4)+j] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                    }
                    nSize = nSize + 4;
                }
#else //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_C
                for (i = 0; i < nBlockSize; i ++)
                {
                    szDbBusTxData[3+i] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                    nSize ++; 
                }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
            }
            else
            {
                for (i = 0; i < nBlockSize; i ++)
                {
                    szDbBusTxData[3+i] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                    nSize ++; 
                }
            }
#else
            for (i = 0; i < nBlockSize; i ++)
            {
                szDbBusTxData[3+i] = _gOneDimenFwData[(MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024)+(index*nSizePerWrite)+i];
                nSize ++; 
            }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

            index ++;

            IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);
        
            nRemainSize = nRemainSize - nBlockSize;
        }

        // Program end
        szDbBusTxData[0] = 0x21;

        IicWriteData(SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);    

#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
        if (g_Msg22xxChipRevision >= CHIP_TYPE_MSG22XX_REVISION_U05) 
        {
#if defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A)
            RegSet16BitValueOff(0x160A, BIT0); // Set Bit0 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
#elif defined(CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B)
            RegSet16BitValueOff(0x160A, BIT0);
            RegSet16BitValueOff(0x160A, BIT1); // Set Bit0 = 0, Bit1 = 0 for disable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
        }
#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

        RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01)); // Clear burst mode

        DBG(&g_I2cClient->dev, "Wait info block write done flag\n");

        nRegData = 0;
        nTimeOut = 0;

        while (1) // Wait write done flag
        {
            // Polling 0x1610 is 0x0002
            nRegData = RegGet16BitValue(0x1610); // Memory status
            nRegData = nRegData & BIT1;
    
            DBG(&g_I2cClient->dev, "Wait write done flag nRegData = 0x%x\n", nRegData);

            if (nRegData == BIT1)
            {
                break;		
            }
            mdelay(10);

            if ((nTimeOut ++) > 30)
            {
                DBG(&g_I2cClient->dev, "Write failed. Timeout.\n");

                goto UpdateEnd;
            }
        }

        DBG(&g_I2cClient->dev, "Program info block end\n");
    }
    
    UpdateEnd:

    if (eEmemType == EMEM_ALL || eEmemType == EMEM_MAIN)
    {
        // Get CRC 32 from updated firmware bin file
        nCrcMain  = _gOneDimenFwData[0xBFFF] << 24;
        nCrcMain |= _gOneDimenFwData[0xBFFE] << 16;
        nCrcMain |= _gOneDimenFwData[0xBFFD] << 8;
        nCrcMain |= _gOneDimenFwData[0xBFFC];

        // CRC Main from TP
        DBG(&g_I2cClient->dev, "Get Main CRC from TP\n");

        nCrcMainTp = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);
    
        DBG(&g_I2cClient->dev, "nCrcMain=0x%x, nCrcMainTp=0x%x\n", nCrcMain, nCrcMainTp);
    }

    if (eEmemType == EMEM_ALL || eEmemType == EMEM_INFO)
    {
        nCrcInfo  = _gOneDimenFwData[0xC1FF] << 24;
        nCrcInfo |= _gOneDimenFwData[0xC1FE] << 16;
        nCrcInfo |= _gOneDimenFwData[0xC1FD] << 8;
        nCrcInfo |= _gOneDimenFwData[0xC1FC];

        // CRC Info from TP
        DBG(&g_I2cClient->dev, "Get Info CRC from TP\n");

        nCrcInfoTp = _DrvMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

        DBG(&g_I2cClient->dev, "nCrcInfo=0x%x, nCrcInfoTp=0x%x\n", nCrcInfo, nCrcInfoTp);
    }

    g_FwDataCount = 0; // Reset g_FwDataCount to 0 after update firmware

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    DrvTouchDeviceHwReset();

    g_IsUpdateFirmware = 0x00; // Set flag to 0x00 for indicating update firmware is finished

    if (eEmemType == EMEM_ALL)
    {
        if ((nCrcMainTp != nCrcMain) || (nCrcInfoTp != nCrcInfo))
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_MAIN)
    {
        if (nCrcMainTp != nCrcMain)
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    else if (eEmemType == EMEM_INFO)
    {
        if (nCrcInfoTp != nCrcInfo)
        {
            DBG(&g_I2cClient->dev, "Update FAILED\n");
          
            return -1;
        } 
    }
    
    DBG(&g_I2cClient->dev, "Update SUCCESS\n");

    return 0;
}

