/***************************************************************
*  Copyright 2014 (c) Discretix Technologies Ltd.              *
*  This software is protected by copyright, international      *
*  treaties and various patents. Any copy, reproduction or     *
*  otherwise use of this software must be authorized in a      *
*  license agreement and include this Copyright Notice and any *
*  other notices specified in the license agreement.           *
*  Any redistribution in binary form must be authorized in the *
*  license agreement and include this Copyright Notice and     *
*  any other notices specified in the license agreement and/or *
*  in materials provided with the binary distribution.         *
****************************************************************/

/* pseudo dx_hal.h for cc_ll_test (to be able to include code from CC drivers) */

#ifndef __DX_HAL_H__
#define __DX_HAL_H__

#include <linux/io.h>

#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
#define READ_REGISTER(_addr) ioread32((_addr))
#define WRITE_REGISTER(_addr, _data)  iowrite32((_data), (_addr))
#else /* Default PPC */
#error Unsupported platform
#endif

#define DX_HAL_WriteCcRegister(offset, val) WRITE_REGISTER(cc_base + offset, val)
#define DX_HAL_ReadCcRegister(offset) READ_REGISTER(cc_base + offset)

#endif
