/*****************************************************************************
* Copyright (C) 2015 ARM Limited or its affiliates.	                     *
* This program is free software; you can redistribute it and/or modify it    *
* under the terms of the GNU General Public License as published by the Free *
* Software Foundation; either version 2 of the License, or (at your option)  * 
* any later version.							     *
* This program is distributed in the hope that it will be useful, but 	     *
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
* for more details.							     *	
* You should have received a copy of the GNU General Public License along    *
* with this program; if not, write to the Free Software Foundation, 	     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.        *
******************************************************************************/


#ifndef _SSI_LLI_DEFS_H_
#define _SSI_LLI_DEFS_H_
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif
#include "ssi_bitops.h"


#define SASI_MAX_MLLI_ENTRY_SIZE 0x10000

#define LLI_SET_ADDR(lli_p, addr) \
		BITFIELD_SET(((uint32_t *)(lli_p))[LLI_WORD0_OFFSET], LLI_LADDR_BIT_OFFSET, LLI_LADDR_BIT_SIZE, (addr & UINT32_MAX)); \
		BITFIELD_SET(((uint32_t *)(lli_p))[LLI_WORD1_OFFSET], LLI_HADDR_BIT_OFFSET, LLI_HADDR_BIT_SIZE, ((addr >> 32) & UINT16_MAX));

#define LLI_SET_SIZE(lli_p, size) \
		BITFIELD_SET(((uint32_t *)(lli_p))[LLI_WORD1_OFFSET], LLI_SIZE_BIT_OFFSET, LLI_SIZE_BIT_SIZE, size)


/* Size of entry */
#define LLI_ENTRY_WORD_SIZE 2
#define LLI_ENTRY_BYTE_SIZE (LLI_ENTRY_WORD_SIZE * sizeof(uint32_t))

/* Word0[31:0] = ADDR[31:0] */
#define LLI_WORD0_OFFSET 0
#define LLI_LADDR_BIT_OFFSET 0
#define LLI_LADDR_BIT_SIZE 32
/* Word1[31:16] = ADDR[47:32]; Word1[15:0] = SIZE */
#define LLI_WORD1_OFFSET 1
#define LLI_SIZE_BIT_OFFSET 0
#define LLI_SIZE_BIT_SIZE 16
#define LLI_HADDR_BIT_OFFSET 16
#define LLI_HADDR_BIT_SIZE 16

#endif /*_SSI_LLI_DEFS_H_*/
