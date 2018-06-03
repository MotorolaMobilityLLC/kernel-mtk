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
* The Secure_env_init file is basically a wrapper to add any additinal functionality in order 
* to initialize the platform dependent secure environment before testing it. Iniatilization of the 
* environment is done in Secure_Env_Init().
* Secure_Env_Init() in this implementation will do 2 initializations:
    - Set/Initialize a Secure Session Key.
    - Enable the Secure Timer.
* this file also provides 2 addtional services:
* Secure_Env_Get_Session_Key() : This function returns a valid Secure Key Session, used when genrating dynamically 
* a blob by test application.
* Secure_Env_Get_Secure_Timer_Boundaries() : This function returns a pair of Secure timer boundary ( start and end)

* Note That In real life , there is no such interface need , and you can just remove this functionality. 
* However for testing purpose this functionality is required.
****************************************************************/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <crypto/cryptodev.h>
#include <sys/mman.h>
#include <errno.h>

#include "secure_config.h"
#include "secure_env_init.h"

#pragma GCC diagnostic ignored "-Wcast-align"



#define ENV_REG_BASE_ADR                DX_BASE_ENV_REGS
#define ENV_REG_AREA_LEN                0x100000


uint64_t gCcEnvRegBase;
const CC_AESCCM_Key_t g_intsessionKey = {0x78,0x56,0x34,0x12,0x78,0x56,0x34,0x12,0x78,0x56,0x34,0x12,0x78,0x56,0x34,0x12};


inline unsigned int read_env_reg(unsigned int reg)
{
    return *(volatile uint64_t *)(gCcEnvRegBase+reg);
}

inline void write_env_reg(unsigned int reg, unsigned int val)
{
    *(volatile unsigned int *)(gCcEnvRegBase+reg) = val;
}


int Secure_Env_Init(void* env_ctx __attribute__((unused)))
{

    uint32_t i;
    int32_t fd_mem;
    uint32_t lowVal[2], highVal[2];

    printf("Start Secure_Env_Init()");
    if ((fd_mem=open("/dev/mem", O_RDWR|O_SYNC))<0)
    {
        fprintf(stderr, "Error: Can not open /dev/mem (%s)\n", strerror(errno));
        abort();
    }

    /* mmap ENV register area */
    gCcEnvRegBase = (uint64_t)mmap(0, ENV_REG_AREA_LEN, PROT_READ|PROT_WRITE, MAP_SHARED, fd_mem, ENV_REG_BASE_ADR);
    if (gCcEnvRegBase == 0 || gCcEnvRegBase== 0xFFFFFFFF )
    {
        fprintf(stderr, "\t Secure_Env_Init: Error: env reg mmap failed (%s)\n", strerror(errno));
        return 1;
    }
    printf("\t Secure_Env_Init: ENV reg mapping; phys addr %x to addr %x\n\n",ENV_REG_BASE_ADR,gCcEnvRegBase);

    /* session key */
    printf("\t Secure_Env_Init: Set A Secure Chanel Session Key Start\n");
    /* reset the A0 always on area (session key, counter and HOST address) */
    /* This muts be done here to allow cold boot (depend on the counter value) */
    //WRITE_REGISTER( g_sep_rom_base_addr + ENV_RESET_SESSION_KEY, 0x1);
    write_env_reg(DX_ENV_RESET_SESSION_KEY_REG_OFFSET ,0x1);
    usleep(5000);
    for(i=0 ; i<4 ; i++) {
        write_env_reg( (DX_ENV_SESSION_KEY_0_REG_OFFSET + 4*i) ,((uint32_t*)g_intsessionKey)[i]);
        usleep(5000);
    }
    //WRITE_REGISTER( g_sep_rom_base_addr + ENV_SESSION_KEY_VALID, 0x1);
    write_env_reg(DX_ENV_SESSION_KEY_VALID_REG_OFFSET ,0x1);
    usleep(5000);

    printf("\t Secure_Env_Init: Set session key End\n\n");

    
    /* init Secure Timer */
    printf("\t Secure_Env_Init: Init And Start Secure Timer \n");
    write_env_reg(DX_ENV_SECURE_TIMER_EN_REG_OFFSET ,0x0);
    write_env_reg(DX_ENV_SECURE_TIMER_INIT_LOW_REG_OFFSET ,0x010CE28F);
    write_env_reg(DX_ENV_SECURE_TIMER_INIT_HI_REG_OFFSET ,0x0 );
    write_env_reg(DX_ENV_SECURE_TIMER_EN_REG_OFFSET ,0x1);

    //just do some check that timer is working
    for (i=0;i<2;i++) {
        write_env_reg(DX_ENV_SECURE_TIMER_EN_REG_OFFSET ,0x0);
        lowVal[i] = read_env_reg(DX_ENV_SECURE_TIMER_LOW_REG_OFFSET);
        highVal[i] = read_env_reg(DX_ENV_SECURE_TIMER_HI_REG_OFFSET);
        write_env_reg(DX_ENV_SECURE_TIMER_EN_REG_OFFSET ,0x1);
        printf ("\t Secure_Env_Init: Timer is  %08x %08x\n", highVal[i], lowVal[i]);
        usleep(100);
    }

    if ( (lowVal[0]!=lowVal[1]) || (highVal[0]!=highVal[1]) )
        printf("\t Secure_Env_Init: Secure timer was started and is beating \n\n");
    else {
        printf("\t Secure_Env_Init: Secure timer was started but no beating was detected. Please check this issue or disable this environment setting \n\n");
        return 1;
    }

    return 0;
}

int Secure_Env_Get_Session_Key(void* env_ctx __attribute__((unused)), CC_AESCCM_Key_t sessionKey)
{
    if (sessionKey == NULL) {
        return 1;
    }

    memcpy(sessionKey, g_intsessionKey, sizeof(CC_AESCCM_Key_t));
    return 0;
}


//This function returns timer time stamps to use for a test (test is detected using the test context param
int Secure_Env_Get_Secure_Timer_Boundaries(void* test_ctx __attribute__((unused)), uint64_t* startTimeStamp __attribute__((unused)), uint64_t* endTimeStamp __attribute__((unused)))
{
    return 1;
}
