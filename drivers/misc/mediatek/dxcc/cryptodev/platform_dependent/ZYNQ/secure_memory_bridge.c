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


/***************************************************************
* The Secure_Memory_Bridge file is basically a wrapper to add any additinal functionality in order 
* to read/write data into secure address regions.
* Iniatilization of the Secure Memory Bridge is done in Secure_Memory_Bridge_Init().
* this file also provides 2 addtional services:
* Secure_Memory_Bridge_Set_Data() : This function updates the secure address with the content of a given buffer
* Secure_Memory_Bridge_Get_Data() : This function reads data from a secure address and updates a given buffer

* Note That In real life , there is no such interface requirement , and you can just remove this functionality. 
* However for testing purpose this functionality is required.
****************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <crypto/cryptodev.h>
#include <sys/mman.h>
#include <errno.h>

#include "secure_config.h"

#include "secure_memory_bridge.h"
#pragma GCC diagnostic ignored "-Werror"

#define FREE_MEM_BASE_ADR               0x30000000
#define SEP_MEM_BASE_ADR                FREE_MEM_BASE_ADR
#define SEP_MEM_AREA_LEN                0x10000000
uint64_t gMemBaseAddr;


int Secure_Memory_Bridge_Init(void* sec_mem_ctx __attribute__((unused)))
{
    int fd_mem;

    if ((fd_mem=open("/dev/mem", O_RDWR|O_SYNC))<0)
    {
        fprintf(stderr, "Error: Can not open /dev/mem (%s)\n", strerror(errno));
        return 1;
    }

    gMemBaseAddr = (uint64_t)mmap(0, SEP_MEM_AREA_LEN, PROT_READ|PROT_WRITE, MAP_SHARED, fd_mem, SEP_MEM_BASE_ADR);
    if (gMemBaseAddr == 0 || gMemBaseAddr == 0xFFFFFFFF )
    {
        fprintf(stderr, "Error: mem mmap failed (%s)\n", strerror(errno));
        return 1;
    }

    return 0;
}

int Secure_Memory_Bridge_Set_Data(void* sec_mem_test_ctx __attribute__((unused)), void* secure_address_handle, char* data, unsigned int data_len) 
{
    __u64 mapped_secure_address;
    
    if (!secure_address_handle) 
        return 1;

    mapped_secure_address = gMemBaseAddr + ((__u64)(intptr_t)secure_address_handle - SEP_MEM_BASE_ADR);
    memcpy((void*)(intptr_t)mapped_secure_address, (const void*)data, data_len);

    return 0;
}

int Secure_Memory_Bridge_Get_Data(void* sec_mem_test_ctx __attribute__((unused)), void* secure_address_handle, char* data, unsigned int data_len) 
{
    __u64 mapped_secure_address;
    
    if (!secure_address_handle) 
        return 1;

    mapped_secure_address = gMemBaseAddr + ((__u64)(intptr_t)secure_address_handle - SEP_MEM_BASE_ADR);
    memcpy(data, (const void*)(intptr_t)mapped_secure_address, data_len);

    return 0;
}


