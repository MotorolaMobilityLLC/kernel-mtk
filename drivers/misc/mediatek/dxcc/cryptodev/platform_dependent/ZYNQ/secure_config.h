/*****************************************************************************
* Copyright (C) 2015 ARM Limited or its affiliates.	                         *
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
*****************************************************************************/


#ifndef _SECURE_CONFIG_H
#define _SECURE_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

//These registers values were taken from dx_env.h. Need to update/change these registers depending on the Platform!!!!
#define DX_ENV_RESET_SESSION_KEY_REG_OFFSET 	0x494UL 	 
#define DX_ENV_SESSION_KEY_0_REG_OFFSET 	0x4A0UL 	
#define DX_ENV_SESSION_KEY_VALID_REG_OFFSET 	0x4B0UL 	

#define DX_ENV_SECURE_TIMER_EN_REG_OFFSET 	0x618UL 	
#define DX_ENV_SECURE_TIMER_INIT_LOW_REG_OFFSET 	0x610UL 	
#define DX_ENV_SECURE_TIMER_INIT_HI_REG_OFFSET 	0x614UL 	
#define DX_ENV_SECURE_TIMER_LOW_REG_OFFSET 	0x61CUL 	
#define DX_ENV_SECURE_TIMER_HI_REG_OFFSET 	0x620UL 	

//These registers values were taken from dx_reg_base_host.h. Need to update/change these registers depending on the Platform!!!!
#define DX_BASE_ENV_REGS 0x40008000


//For the blob address range configuration.
#define SECURE_ADDRESS_0    0x0000000038000000ULL
#define SECURE_ADDRESS_1    0x0000000038020000ULL
#define SECURE_ADDRESS_2    0x0000000038040000ULL
#define SECURE_ADDRESS_3    0x0000000038100000ULL
#define SECURE_ADDRESS_4    0x0000000038200000ULL
#define SECURE_ADDRESS_5    0x0000000038010000ULL
#define SECURE_ADDRESS_6    0x0000000038030000ULL
#define SECURE_ADDRESS_7    0x0000000038050000ULL
#define SECURE_ADDRESS_8    0x0000000038060000ULL
#define SECURE_ADDRESS_9    0x0000000038070000ULL
#define SECURE_ADDRESS_10   0x0000000038002000ULL
#define SECURE_ADDRESS_11   0x0000000038080000ULL
#define SECURE_ADDRESS_12   0x00000000380a0000ULL
#define SECURE_ADDRESS_13   0x00000000380c0000ULL
#define SECURE_ADDRESS_14   0x00000000380e0000ULL

#define ADDR_RANGE_0        0x10000
#define ADDR_RANGE_00       0x1000
#define ADDR_RANGE_000      0x1000000
#define ADDR_RANGE_0000     0x2000
#define ADDR_RANGE_1        0x10000
#define ADDR_RANGE_2        0x10000
#define ADDR_RANGE_3        0x200000
#define ADDR_RANGE_33       0x1000
#define ADDR_RANGE_4        0x10000
#define ADDR_RANGE_44       0x8E00000
#define ADDR_RANGE_444      0x200000
#define ADDR_RANGE_5        0x10000
#define ADDR_RANGE_6        0x10000
#define ADDR_RANGE_7        0x10000
#define ADDR_RANGE_8        0x10000
#define ADDR_RANGE_9        0x10000
#define ADDR_RANGE_10       0x7E000
#define ADDR_RANGE_11       0x10000
#define ADDR_RANGE_12       0x10000
#define ADDR_RANGE_13       0x10000
#define ADDR_RANGE_14       0x10000

#define SECURE_TIMER_BOUNDARY_0  0x010CE28F ,0x00000000 ,0x36583916 ,0x00038672 
#define SECURE_TIMER_BOUNDARY_1  0x010CE28F ,0x00000000 ,0x36583916 ,0x00058672 


#ifdef __cplusplus
}
#endif

#endif

