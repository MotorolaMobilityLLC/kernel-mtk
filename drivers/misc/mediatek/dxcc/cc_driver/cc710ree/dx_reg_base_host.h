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
#ifndef __DX_REG_BASE_HOST_H__
#define __DX_REG_BASE_HOST_H__

/* Identify platform: Xilinx Zynq7000 ZC706 */
#define DX_PLAT_ZYNQ7000 1
#define DX_PLAT_ZYNQ7000_ZC706 1

/* SEP core clock frequency in MHz */
#define DX_SEP_FREQ_MHZ 50
#define DX_BASE_CC 0x80000000

#define DX_BASE_ENV_REGS 0x40008000
#define DX_BASE_ENV_CC_MEMORIES 0x40008000
#define DX_BASE_ENV_PERF_RAM 0x40009000

#define DX_BASE_HOST_RGF 0x0UL
#define DX_BASE_CRY_KERNEL     0x0UL
#define DX_BASE_ROM     0x40000000

#endif /*__DX_REG_BASE_HOST_H__*/
