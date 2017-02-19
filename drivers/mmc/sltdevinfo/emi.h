/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef MT_EMI_H
#define MT_EMI_H

//#if (6797 == MACH_TYPE)
#if 1
#define DDR_RESERVE_MODE
#else
//#define DDR_RESERVE_MODE
#endif

//#define DUMP_VCORE_TABLE

typedef unsigned int U32;

typedef struct _AC_TIMING_EXTERNAL_T
{
    // U 00
    //U32 AC_TIME_EMI_DUMMY_01 :2;
    //U32 AC_TIME_EMI_DRAM_TYPE         :2;
    U32 AC_TIME_EMI_FREQUENCY         :16;
    U32 AC_TIME_EMI_TRAS         :8;
    U32 AC_TIME_EMI_TRP           :8;
    
    // U 01
    U32 AC_TIME_EMI_TRPAB           :8;
    U32 AC_TIME_EMI_TRC           :8;
    U32 AC_TIME_EMI_TRFC           :8;
    U32 AC_TIME_EMI_TRFCPB           :8;
    
    // U 02
    U32 AC_TIME_EMI_TXP           :8;
    U32 AC_TIME_EMI_TRTP           :8;
    U32 AC_TIME_EMI_TRCD           :8;
    U32 AC_TIME_EMI_TWR           :8;
    
    // U 03
    U32 AC_TIME_EMI_TWTR           :8;
    U32 AC_TIME_EMI_TRRD           :8;
    U32 AC_TIME_EMI_TFAW           :8;
    U32 AC_TIME_EMI_TRTW           :8;

    // U 04    
    U32 AC_TIME_EMI_REFCNT           :8; //(REFFRERUN = 0)
    U32 AC_TIME_EMI_REFCNT_FR_CLK           :8; //(REFFRERUN = 1)
    U32 AC_TIME_EMI_TXREFCNT           :8;
    U32 AC_TIME_EMI_TZQCS           :8;

    // U 05
    U32 AC_TIME_EMI_TRAS_05T           :2;
    U32 AC_TIME_EMI_TRP_05T           :2;
    U32 AC_TIME_EMI_TRPAB_05T           :2;
    U32 AC_TIME_EMI_TRC_05T           :2;
    U32 AC_TIME_EMI_TRFC_05T           :2;
    U32 AC_TIME_EMI_TRFCPB_05T           :2;
    U32 AC_TIME_EMI_TXP_05T           :2;
    U32 AC_TIME_EMI_TRTP_05T           :2;
    U32 AC_TIME_EMI_TRCD_05T           :2;
    U32 AC_TIME_EMI_TWR_05T           :2;
    U32 AC_TIME_EMI_TWTR_05T           :2;
    U32 AC_TIME_EMI_TRRD_05T           :2;
    U32 AC_TIME_EMI_TFAW_05T           :2;
    U32 AC_TIME_EMI_TRTW_05T           :2;
    U32 AC_TIME_EMI_REFCNT_05T           :2; //(REFFRERUN = 0)
    U32 AC_TIME_EMI_REFCNT_FR_CLK_05T           :2; //(REFFRERUN = 1)

    // U 06
    U32 AC_TIME_EMI_TXREFCNT_05T           :2;
    U32 AC_TIME_EMI_TZQCS_05T           :2;
#if 0      
    U32 AC_TIME_EMI_XRTW2W           :4;
    U32 AC_TIME_EMI_XRTW2R           :4;
    U32 AC_TIME_EMI_XRTR2W           :4;
    U32 AC_TIME_EMI_XRTR2R           :4;     
    U32 AC_TIME_EMI_DUMMY_02               :12;

    // U 07
    U32 AC_TIME_EMI_WRITE_LATENCY_0x41c           :32;
    // U 08
    U32 AC_TIME_EMI_WRITE_LATENCY_0x420           :32;
    // U 09
    U32 AC_TIME_EMI_WRITE_LATENCY_0x424           :32;
    // U 10
    U32 AC_TIME_EMI_WRITE_LATENCY_0x428           :32;
    // U 11
    U32 AC_TIME_EMI_WRITE_LATENCY_0x42c           :32;

    // U 12
    U32 AC_TIME_EMI_DQSINCTL_FOR_GATING           :8;
    U32 AC_TIME_EMI_DATLAT           :8;
    U32 AC_TIME_EMI_MODE_REG_WL           :8;
    U32 AC_TIME_EMI_MODE_REG_RL           :8;
#endif
   
}AC_TIMING_EXTERNAL_T;

typedef volatile unsigned int *V_UINT32P;
int get_dram_rank_nr (void);
void get_dram_rank_size(unsigned int dram_rank_size[]);
#define DDR1  1
#define LPDDR2  2
#define LPDDR3  3
#define PCDDR3  4
typedef struct
{
    char DEVINFO_MCP[32];
    int   sub_version;            // sub_version: 0x1 for new version
    int  type;                /* 0x0000 : Invalid
                                 0x0001 : Discrete DDR1
                                 0x0002 : Discrete LPDDR2
                                 0x0003 : Discrete LPDDR3
                                 0x0004 : Discrete PCDDR3
                                 0x0101 : MCP(NAND+DDR1)
                                 0x0102 : MCP(NAND+LPDDR2)
                                 0x0103 : MCP(NAND+LPDDR3)
                                 0x0104 : MCP(NAND+PCDDR3)
                                 0x0201 : MCP(eMMC+DDR1)
                                 0x0202 : MCP(eMMC+LPDDR2)
                                 0x0203 : MCP(eMMC+LPDDR3)
                                 0x0204 : MCP(eMMC+PCDDR3)
                              */
    int   id_length;              // EMMC and NAND ID checking length
    int   fw_id_length;              // FW ID checking length
    char  ID[16];
    char  fw_id[8];               // To save fw id
    int   EMI_CONA_VAL;           //@0x3000
    int   EMI_CONH_VAL;
    
    union   {        
    int DRAMC_ACTIME_UNION[7];
    AC_TIMING_EXTERNAL_T AcTimeEMI;
    };       

    int   DRAM_RANK_SIZE[4];
    int   EMI_CONF_VAL;
    int   reserved[9];

#if 0
    union
    {
        struct
        {
            int   iLPDDR2_MODE_REG_1;
            int   iLPDDR2_MODE_REG_2;
            int   iLPDDR2_MODE_REG_3;
            int   iLPDDR2_MODE_REG_5;
            int   iLPDDR2_MODE_REG_10;
            int   iLPDDR2_MODE_REG_63;
        };
        struct
        {
            int   iDDR1_MODE_REG;
            int   iDDR1_EXT_MODE_REG;
        };
        struct
        {
            int   iPCDDR3_MODE_REG0;
            int   iPCDDR3_MODE_REG1;
            int   iPCDDR3_MODE_REG2;
            int   iPCDDR3_MODE_REG3;
        };
        struct
        {
            int   iLPDDR3_MODE_REG_1;
            int   iLPDDR3_MODE_REG_2;
            int   iLPDDR3_MODE_REG_3;
            int   iLPDDR3_MODE_REG_5;
            int   iLPDDR3_MODE_REG_10;
            int   iLPDDR3_MODE_REG_63;
        };
    };
#else
    int   iLPDDR3_MODE_REG_5;
#endif    
    int   PIN_MUX_TYPE;   
} EMI_SETTINGS;

int mt_get_dram_type (void); 
/* 0: invalid */
/* 1: mDDR1 */
/* 2: LPDDR2 */
/* 3: LPDDR3 */
/* 4: PCDDR3 */

typedef enum 
{
  CHIP_6597 = 1,
  CHIP_6597M     
} CHIP_TYPE;

#endif
