/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2016
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#ifndef CCU_TYPES_H
#define CCU_TYPES_H

/*******************************************************************************/
/* typedef unsigned char   BOOLEAN;    // uint8_t*/
typedef unsigned char MUINT8;
typedef unsigned short MUINT16;
typedef unsigned int MUINT32;
/*typedef unsigned long  MUINT64;*/
/**/
typedef signed char MINT8;
typedef signed short MINT16;
typedef signed int MINT32;
/*typedef signed long    MINT64;*/
/**/
typedef float MFLOAT;
typedef double MDOUBLE;
/**/
typedef void MVOID;
typedef int MBOOL;

#ifndef MTRUE
#define MTRUE 1
#endif

#ifndef MFALSE
#define MFALSE 0
#endif

#ifndef MNULL
#define MNULL 0
#endif

/******************************************************************************
*Sensor Types
******************************************************************************/
/* #define CCU_CODE_SLIM*/
/* typedef unsigned char   BOOLEAN;    // uint8_t*/
typedef unsigned char U8;	/* uint8_t*/
typedef unsigned short U16;	/* uint16_t*/
typedef unsigned int U32;	/* uint32_t*/
/*typedef unsigned long long  U64;    // uint64_t*/
typedef char I8;		/* int8_t*/
typedef short I16;		/* int16_t*/
typedef int I32;		/* int32_t*/
/*typedef long long           I64;    // int64_t*/

#ifndef NULL
#define NULL                0
#endif				/* NULL*/

/******************************************************************************
* Error code
******************************************************************************/
#define ERR_NONE                    (0)
#define ERR_INVALID                 (-1)
#define ERR_TIMEOUT                 (-2)


#endif				/* CCU_TYPES_H*/

