/****************************************************************************
* This confidential and proprietary software may be used only as authorized *
* by a licensing agreement from ARM Israel.                                 *
* Copyright (C) 2015 ARM Limited or its affiliates. All rights reserved.    *
* The entire notice above must be reproduced on all authorized copies and   *
* copies may only be made to the extent permitted by a licensing agreement  *
* from ARM Israel.                                                          *
*****************************************************************************/

/* pseudo ssi_hal.h for cc_ll_test (to be able to include code from CC drivers) */

#ifndef __DX_HAL_H__
#define __DX_HAL_H__

#include <linux/io.h>

#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
#define READ_REGISTER(_addr) ioread32((_addr))
#define WRITE_REGISTER(_addr, _data)  iowrite32((_data), (_addr))
#else /* Default PPC */
#error Unsupported platform
#endif

#define SASI_HAL_WRITE_REGISTER(offset, val) WRITE_REGISTER(cc_base + offset, val)
#define SASI_HAL_READ_REGISTER(offset) READ_REGISTER(cc_base + offset)

#endif
