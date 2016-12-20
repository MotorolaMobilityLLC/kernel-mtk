/*
 *
 * FocalTech fts TouchScreen driver.
 *
 * Copyright (c) 2010-2016, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*****************************************************************************
*
* File Name: focaltech_upgrade_app_ecc.c
*
* Author:    fupeipei
*
* Created:    2016-08-22
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

/*****************************************************************************
* 1.Included header files
*****************************************************************************/
#include "../focaltech_core.h"
#include "../focaltech_flash.h"
#include <linux/wakelock.h>
#include <linux/timer.h>

/*****************************************************************************
* Static variables
*****************************************************************************/

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/

/*****************************************************************************
* Static function prototypes
*****************************************************************************/

#if (FTS_CHIP_IDC)
typedef enum
{
    APP_LEN_H    = 0,
    APP_LEN_L    = 1,
    APP_NE_LEN_H = 2,
    APP_NE_LEN_L = 3,
    P1_ECC_H     = 4,
    P1_ECC_L     = 5,
    P1_ECC_NE_H  = 6,
    P1_ECC_NE_L  = 7,
    P2_ECC_H     = 8,
    P2_ECC_L     = 9,
    P2_ECC_NE_H  = 0x0A,
    P2_ECC_NE_L  = 0x0B,
    VENDER_ID    = 0x0C,
    VENDER_ID_NE = 0x0D,
    FW_VERSION   = 0x0E,
    FW_VERSION_NE= 0x0F,
    UP_FLAG      = 0x10,
    UP_FLAG_NE   = 0x11,

    PBOOT_ID_H   = 0x1E,
    PBOOT_ID_L   = 0x1F
} ENUM_APP_INFO;

#define AL2_FCS_COEF          ((1 << 15) + (1 << 10) + (1 << 3))
#define APP_VERIF_ADDR        0x100   // Verification Info
#define APP_VERIF_LEN         0x20
#define APP_VERIF_ADDR        0x100   // Verification Info
#define APP_ADDR_PART1_START  0x0000
#define APP_ADDR_PART1_LEN    ((APP_VERIF_ADDR)-(APP_ADDR_PART1_START))
#define APP_ADDR_PART2_START  ((APP_VERIF_ADDR)+(APP_VERIF_LEN))
#define APP_DATA_MAX          (0xFFFE)
#define ADDR_PART1_ECC        (APP_VERIF_ADDR+P1_ECC_H)
#define ADDR_PART1_ECC_NE     (APP_VERIF_ADDR+P1_ECC_NE_H)
#define ADDR_PART2_ECC        (APP_VERIF_ADDR+P2_ECC_H)
#define ADDR_PART2_ECC_NE     (APP_VERIF_ADDR+P2_ECC_NE_H)
#define ADDR_APP_LEN_H        (APP_VERIF_ADDR+APP_LEN_H)

/*****************************************************************************
* Name: DrvReadPram16
* Brief: 读PRAM数据 16位
* Input:
* Output:
* Return:
*****************************************************************************/
u16 DrvReadPram16(u8* pbt_buf, u16 addr)
{
    return ((pbt_buf[addr]<<8) + (pbt_buf[addr+1]));
}

/*****************************************************************************
*   Name: DrvPramIfValid
*  Brief: 判断数据是否有效
*  Input:
* Output:
* Return:
*****************************************************************************/
bool DrvPramIfValid(u8* pbt_buf, u16 addr)
{
    u16 tmp1;
    u16 tmp2;
    tmp1 = DrvReadPram16(pbt_buf, (addr));
    tmp2 = DrvReadPram16(pbt_buf, (addr+2));

    if ((tmp1 + tmp2) == 0xFFFF)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************
* Name: GetCrc16
* Brief:
* Input:
* Output:
* Return:
*****************************************************************************/
u16 DrvCRC(u8* pbt_buf, u16 addr,u16 length)
{
    u16 cFcs = 0;
    u16 i, j;
    u16 debug;
    debug = DrvReadPram16(pbt_buf, (addr));
    FTS_DEBUG("[UPGRADE][ECC] : %04x  data:%04x, len:%04x!!",(addr),debug,length);
    for ( i = 0; i < length; i++ )
    {
        cFcs ^= DrvReadPram16(pbt_buf, (addr+i*2));
        for (j = 0; j < 16; j ++)
        {
            if (cFcs & 1)
            {
                cFcs = (u16)((cFcs >> 1) ^ AL2_FCS_COEF);
            }
            else
            {
                cFcs >>= 1;
            }
        }
    }
    return cFcs;
}

/*****************************************************************************
*   Name: task_check_mem
*  Brief:
*  Input:
* Output:
* Return:
*****************************************************************************/
bool task_check_mem_ecc(u8* pbt_buf, u16 StartAddr, u16 Len, u16 EccAddr)
{
    u16 ecc1;
    u16 ecc2;
    u16 cal_ecc;

    ecc1 = DrvReadPram16(pbt_buf, EccAddr);
    ecc2 = DrvReadPram16(pbt_buf, EccAddr+2);

    if ((ecc1 + ecc2) != 0xFFFF)
    {
        return false;
    }

    cal_ecc = DrvCRC(pbt_buf, StartAddr, (Len/2));

    FTS_DEBUG("[UPGRADE][ECC] : ecc1 = %x, cal_ecc = %x", ecc1, cal_ecc);

    if (ecc1 != cal_ecc)
    {
        FTS_DEBUG("[UPGRADE][ECC] : ecc error!!");
        return false;
    }
    return true;
}

/*****************************************************************************
* Name: task_check_mem
* Brief: 检验pram里的代码是否 valid
* Input:
* Output:
* Return:
*****************************************************************************/
bool fts_check_app_bin(u8* pbt_buf, u32 dw_lenth)
{
    u8 ch;
    u16 len1;
    ch = pbt_buf[0];

    if (dw_lenth <= 0)
    {
        return false;
    }
    // 1.首地址
    if (ch != 0x02)
    {
        FTS_DEBUG("[UPGRADE][ECC] : first byte is not 0x02!!");
        return false;
    }

    // 2 PART1 ECC
    if (!task_check_mem_ecc(pbt_buf, APP_ADDR_PART1_START, APP_ADDR_PART1_LEN, ADDR_PART1_ECC))
    {
        FTS_DEBUG("[UPGRADE][ECC] : ecc part 1 error!!");
        return false;
    }

    // 3. PART2 ECC
    len1 = DrvReadPram16(pbt_buf, ADDR_APP_LEN_H);
    if (!DrvPramIfValid(pbt_buf, ADDR_APP_LEN_H))
    {
        FTS_DEBUG("[UPGRADE][ECC] : ecc part 2 error!!");
        return false;
    }

    //长度溢出
    if (len1 > APP_DATA_MAX)
    {
        FTS_DEBUG("[UPGRADE][ECC] : length error!!");
        return false;
    }

    len1 -= ((APP_ADDR_PART1_LEN) + (APP_VERIF_LEN));

    return task_check_mem_ecc(pbt_buf, APP_ADDR_PART2_START, len1, ADDR_PART2_ECC);
}

#elif (FTS_CHIP_TYPE == _FT8006)
#define AL2_FCS_COEF                     ((1 << 15) + (1 << 10) + (1 << 3))
typedef enum
{
    APP_LEN_L      = 0x00,
    APP_LEN_L_NE   = 0x02,
    APP_P1_ECC     = 0x04,
    APP_P1_ECC_NE  = 0x06,
    APP_P2_ECC     = 0x08,
    APP_P2_ECC_NE  = 0x0A,
    APP_LEN_H      = 0x12,
    APP_LEN_H_NE   = 0x14,
    APP_BLR_ID     = 0x1C,  // bootloader id 0x55
    APP_BLR_ID_NE  = 0x1D,  // bootloader id ne 0xBB
} ENUM_APP_INFO;

#define APP_ADDR_START        0x0000 //0x5000// // app start addr
#define FW_CFG_TOTAL_SIZE     0x80
#define APP_VERIF_ADDR        0x100   // Verification Info
#define APP_VERIF_LEN         0x20
#define ADDR_APP_LEN_L        (APP_ADDR_START+APP_VERIF_ADDR+APP_LEN_L)
#define ADDR_APP_LEN_L_NE     (APP_ADDR_START+APP_VERIF_ADDR+APP_LEN_L_NE)
#define ADDR_PART1_ECC        (APP_ADDR_START+APP_VERIF_ADDR+APP_P1_ECC)
#define ADDR_PART1_ECC_NE     (APP_ADDR_START+APP_VERIF_ADDR+APP_P1_ECC_NE)
#define ADDR_PART2_ECC        (APP_ADDR_START+APP_VERIF_ADDR+APP_P2_ECC)
#define ADDR_PART2_ECC_NE     (APP_ADDR_START+APP_VERIF_ADDR+APP_P2_ECC_NE)
#define ADDR_APP_LEN_H        (APP_ADDR_START+APP_VERIF_ADDR+APP_LEN_H)
#define ADDR_APP_LEN_H_NE     (APP_ADDR_START+APP_VERIF_ADDR+APP_LEN_H_NE)
#define ADDR_BLR_ID           (APP_ADDR_START+APP_VERIF_ADDR+APP_BLR_ID)
#define APP_ADDR_PART1_START  APP_ADDR_START
#define APP_ADDR_PART1_LEN    APP_VERIF_ADDR
#define APP_ADDR_PART2_START  ((APP_ADDR_START)+(APP_VERIF_ADDR)+(APP_VERIF_LEN)+(FW_CFG_TOTAL_SIZE))
#define APP_DATA_MAX          (0x17FF0-0X1000)

/*****************************************************************************
* Name: DrvReadPram16
* Brief: 读PRAM数据 16位
* Input:
* Output:
* Return:
*****************************************************************************/
u16 DrvReadPram16(u8* pbt_buf, u32 addr)
{
    return ((pbt_buf[addr]<<8) + (pbt_buf[addr+1]));
}

/*******************************************************************************
* Name: GetCrc16
* Brief:
* Input:
* Output:
* Return:
*******************************************************************************/
u16 DrvCRC(u8* pbt_buf, u32 addr, u16 length)
{
    u16 cFcs = 0;
    u16 i, j;
    u16 debug;
    debug = DrvReadPram16(pbt_buf, (addr));
    FTS_DEBUG("[UPGRADE][ECC] : %04x  data:%04x, len:%04x!!",(addr),debug,length);
    for ( i = 0; i < length; i++ )
    {
        cFcs ^= DrvReadPram16(pbt_buf, addr+i*2);

        for (j = 0; j < 16; j ++)
        {
            if (cFcs & 0x01)
            {
                cFcs = (u16)((cFcs >> 1) ^ AL2_FCS_COEF);
            }
            else
            {
                cFcs >>= 1;
            }
        }
    }
    return cFcs;
}

/*******************************************************************************
*   Name: task_check_mem
*  Brief:
*  Input:
* Output:
* Return:
*******************************************************************************/
bool task_check_mem_ecc(u8* pbt_buf, u32 StartAddr, u32 Len, u16 EccAddr)
{
    u16 ecc1;
    u16 ecc2;
    u16 cal_ecc;

    ecc1 = DrvReadPram16(pbt_buf, EccAddr);
    ecc2 = DrvReadPram16(pbt_buf, EccAddr+2);

    if ((ecc1 + ecc2) != 0xFFFF)
    {
        return false;
    }

    cal_ecc = DrvCRC(pbt_buf, StartAddr, Len/2);

    FTS_DEBUG("[UPGRADE][ECC] : ecc1 = %x, cal_ecc = %x", ecc1, cal_ecc);

    if (ecc1 != cal_ecc)
    {
        FTS_DEBUG("[UPGRADE][ECC] : ecc error!!");
        return false;
    }
    return true;
}

/*******************************************************************************
*   Name: task_check_mem
*  Brief: 检验pram里的代码是否 valid
*  Input:
* Output:
* Return:
*******************************************************************************/
bool fts_check_app_bin(u8* pbt_buf, u32 dw_lenth)
{
    u16 lenH,lenH_ne,lenL,lenL_ne;
    u32 len;

    if (dw_lenth <= 0)
    {
        return false;
    }
    // PART1 ECC
    if (!task_check_mem_ecc(pbt_buf, APP_ADDR_PART1_START, APP_ADDR_PART1_LEN, ADDR_PART1_ECC))
    {
        FTS_DEBUG("[UPGRADE][ECC] : ecc part 1 error!!");
        return false;
    }

    // PART2 ECC
    lenH = DrvReadPram16(pbt_buf, ADDR_APP_LEN_H);
    lenH_ne = DrvReadPram16(pbt_buf, ADDR_APP_LEN_H_NE);
    lenL = DrvReadPram16(pbt_buf, ADDR_APP_LEN_L);
    lenL_ne = DrvReadPram16(pbt_buf, ADDR_APP_LEN_L_NE);

    len = (lenH<<16)|lenL;
    if (((lenH + lenH_ne) != 0xFFFF)||((lenL + lenL_ne) != 0xFFFF))
    {
        FTS_DEBUG("[UPGRADE][ECC] : ecc part 2 error!!");
        return false;
    }
    FTS_DEBUG("[UPGRADE][ECC] : len = 0x%x, dw_lenth = 0x%x!!", len, dw_lenth);

    //长度溢出
    if (len > APP_DATA_MAX)
    {
        FTS_DEBUG("[UPGRADE][ECC] : length error!!");
        return false;
    }

    len -= ((APP_ADDR_PART1_LEN) + (APP_VERIF_LEN)+(FW_CFG_TOTAL_SIZE));
    return task_check_mem_ecc(pbt_buf, APP_ADDR_PART2_START, len, ADDR_PART2_ECC);
}
#else
#define APP_ADDR_START    0x0000
#define APP_DATA_MAX      (0xD780)
#define AL2_FCS_COEF                     ((1 << 7) + (1 << 6) + (1 << 5))

/*****************************************************************************
*   Name: DrvCRC
*  Brief:
*  Input:
* Output:
* Return:
*****************************************************************************/
u8 DrvCRC(u8* pbt_buf, u16 start,u16 length)
{
    u8 cFcs = 0;
    u16 i, j;

    for ( i = 0; i < length; i++ )
    {
        cFcs ^= pbt_buf[start++];
        for (j = 0; j < 8; j ++)
        {
            if (cFcs & 1)
            {
                cFcs = (u8)((cFcs >> 1) ^ AL2_FCS_COEF);
            }
            else
            {
                cFcs >>= 1;
            }
        }
    }
    return cFcs;
}

/*****************************************************************************
* Name: task_check_mem
* Brief: 检验pram里的代码是否 valid
* Input:
* Output:
* Return:
*****************************************************************************/
bool fts_check_app_bin(u8* pbt_buf, u32 dw_lenth)
{
    u8 ch;
    u8 ecc1;
    u8 ecc2;
    u16 len1;
    u16 len2;
    u8 cal_ecc;
    u16 usAddrInfo;

    ch = pbt_buf[APP_ADDR_START];

    //1.首地址
    if (ch != 0x02)
    {
        FTS_DEBUG("[UPGRADE]: the first byte is not 0x02!!");
        return false;
    }

    usAddrInfo = (dw_lenth-8);

    //2.len
    len1  = pbt_buf[usAddrInfo++]<<8;
    len1 += pbt_buf[usAddrInfo++];

    len2  = pbt_buf[usAddrInfo++]<<8;
    len2 += pbt_buf[usAddrInfo++];

    //len无效
    if ( (len1 + len2) != 0xFFFF )
    {
        FTS_DEBUG("[UPGRADE]: the length of app.bin is error!!");
        return false;
    }

    //长度溢出
    if (len1 > APP_DATA_MAX)
    {
        FTS_DEBUG("[UPGRADE]: app.bin is too long!!");
        return false;
    }

    //3.ecc
    ecc1 = pbt_buf[usAddrInfo++];
    ecc2 = pbt_buf[usAddrInfo++];

    if ((ecc1 + ecc2) != 0xFF)
    {
        FTS_DEBUG("[UPGRADE]: ecc error!!");
        return false;
    }

    cal_ecc = DrvCRC(pbt_buf, APP_ADDR_START, len1);

    if (ecc1 != cal_ecc)
    {
        FTS_DEBUG("[UPGRADE]: ecc error!!");
        return false;
    }
    return true;
}

#endif

