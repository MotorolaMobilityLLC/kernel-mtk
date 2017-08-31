/****************************************************************************
* This confidential and proprietary software may be used only as authorized *
* by a licensing agreement from ARM Israel.                                 *
* Copyright (C) 2015 ARM Limited or its affiliates. All rights reserved.    *
* The entire notice above must be reproduced on all authorized copies and   *
* copies may only be made to the extent permitted by a licensing agreement  *
* from ARM Israel.                                                          *
*****************************************************************************/

#ifndef __CC_LL_TEST_H__
#define __CC_LL_TEST_H__

#include "dx_reg_common.h"

/* Uncomment the following line when running Linux as secure OS (i.e., ARM::SCR.NS == 0) */
/* This option enables a few test steps which cannot be executed in insecure world */
//#define SECURE_LINUX

/* Supported CryptoCell products signature values */
/* Used to detect product ID at compile time */
#define DX_CC641P_SIG    0xDCC63200

/* CC device resources (which are missing from dx_reg_*.h files */
#if (DX_DEV_SIGNATURE == DX_CC641P_SIG)
#define DX_HW_VERSION  0xCF7A0030U
#define DX_ROM_VERSION 0x01000003U
#else
#error Unsupported DX_DEV_SIGNATURE value
#endif

#define DX_CC_SRAM_SIZE 4096

/* Default test parameters */
#define DX_CACHE_PARAMS_DEFAULT 0x000 /* Alternative: 0x333 for ACE coherency */
#define DMA_COHERENT_OPS_DEFAULT 0 /* Alternative: 1 to use arm_coherent_dma_ops */
#define RESULTS_VERIF_MSLEEP 10 /* [msec] delay before re-verifying results */

#endif /*__CC_LL_TEST_H__*/
