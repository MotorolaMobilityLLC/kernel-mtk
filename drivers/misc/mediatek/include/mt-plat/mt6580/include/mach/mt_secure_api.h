/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */


#ifndef _MT_SECURE_API_H_
#define _MT_SECURE_API_H_

#include <mt-plat/sync_write.h>

/* Use the arch_extension sec pseudo op before switching to secure world */
#if defined(__GNUC__) && \
	defined(__GNUC_MINOR__) && \
	defined(__GNUC_PATCHLEVEL__) && \
	((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)) \
	>= 40502
#define MC_ARCH_EXTENSION_SEC
#endif

/*
 * return code for fastcalls
 */
#define MC_FC_RET_OK					0
#define MC_FC_RET_ERR_INVALID			1



/* t-base fastcall  */
/*
 *          ***   obsolete  ***
 * Command to inform SW that we will enter in Power Sleep (Dormant).
 * Parameters:
 *   - param0, the NW wakeup addr (phys).
 *   - param1, the cpuID who will sleep
 *   - param2, unused.
 * Return:
 *   - Cannot fail, always MC_FC_RET_OK.
 */
/*#define MC_FC_SLEEP                     -3 */

/*
 * Command to write NW reset addr for Slave cpus.
 * Parameters:
 *   - param0, the NW reset addr (phys).
 *   - param1, the cpuID.
 *   - param2, in case of erratum 802022, 1.
 *             otherwise, 0
 * Return:
 *   - Cannot fail, always MC_FC_RET_OK.
 */
#define MC_FC_SET_RESET_VECTOR		-301

/*
 *          ***   obsolete  ***
 * Command to switch off BootRom.
 * Parameters:
 *   - param0, unused.
 *   - param1, unsued.
 *   - param2, unused.
 * Return:
 *   - Cannot fail, always MC_FC_RET_OK.
 */
/* #define MC_FC_TURN_OFF_BOOTROM		-302 */

/*
 * Command to cancel a sleep command in case of Dormant abort.
 * Parameters:
 *   - param0, unused.
 *   - param1, unsued.
 *   - param2, unused.
 * Return:
 *   - Cannot fail, always MC_FC_RET_OK.
 */
#define MC_FC_SLEEP_CANCELLED		-303


/*
 * Command to inform SW that we will enter in Power Sleep (Dormant).
 * Parameters:
 *   - param0, the NW wakeup addr (phys).
 *   - param1, the cpuID who will sleep
 *   - param2, unused.
 * Return:
 *   - Cannot fail, always MC_FC_RET_OK.
 */
#define MC_FC_MTK_SLEEP             -304

#define MC_FC_ERRATA_808022		-305

#define MC_FC_MTK_AEEDUMP		-306


#if defined(CONFIG_ARM_PSCI)
/* Error Code */
#define SIP_SVC_E_SUCCESS               0
#define SIP_SVC_E_NOT_SUPPORTED         -1
#define SIP_SVC_E_INVALID_PARAMS        -2
#define SIP_SVC_E_INVALID_Range         -3
#define SIP_SVC_E_PERMISSION_DENY       -4

#ifdef CONFIG_ARM64
/* SIP SMC Call 64 */
#define MTK_SIP_KERNEL_MCUSYS_WRITE         0xC2000201
#define MTK_SIP_KERNEL_MCUSYS_ACCESS_COUNT  0xC2000202
#else
#include <asm/opcodes-sec.h>
#include <asm/opcodes-virt.h>
/* SIP SMC Call 32 */
#define MTK_SIP_KERNEL_MCUSYS_WRITE         0x82000201
#define MTK_SIP_KERNEL_MCUSYS_ACCESS_COUNT  0x82000202
#endif
#endif

/*
static noinline int mt_secure_call(u32 function_id, u32 arg0, u32 arg1, u32 arg2)
{
	register u32 reg0 __asm__("r0") = function_id;
	register u32 reg1 __asm__("r1") = arg0;
	register u32 reg2 __asm__("r2") = arg1;
	register u32 reg3 __asm__("r3") = arg2;
	int ret = 0;

	asm volatile(
			__SMC(0)
			: "+r" (reg0), "+r" (reg1), "+r" (reg2), "+r" (reg3));

	ret = reg0;
	return ret;
}
*/
/*
 * mt_secure_call
 * Parameters:
 *   - cmd, the command we want to execute.
 *   - param0, param1, param2: the parameters used for this command.
 * Return:
 *   - Error code of the command (MC_FC_RET_OK in case of success).
 */
static inline int mt_secure_call(uint32_t cmd, uint32_t param0, uint32_t param1, uint32_t param2)
{
	/* SMC expect values in r0-r3 */
	register u32 reg0 __asm__("r0") = cmd;
	register u32 reg1 __asm__("r1") = param0;
	register u32 reg2 __asm__("r2") = param1;
	register u32 reg3 __asm__("r3") = param2;
	int ret = 0;

	__asm__ volatile (
#ifdef MC_ARCH_EXTENSION_SEC
		/* This pseudo op is supported and required from
		 * binutils 2.21 on */
		".arch_extension sec\n"
#endif
		"smc 0\n"
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
	);

	/* set response */
	ret = reg0;
	return ret;
}


#define CONFIG_MCUSYS_WRITE_PROTECT

#if defined(CONFIG_MCUSYS_WRITE_PROTECT) && defined(CONFIG_ARM_PSCI)
#define SMC_IO_VIRT_TO_PHY(addr) (addr-0xF0000000+0x10000000)
#define mcusys_smc_write(addr, val) \
	mt_secure_call(MTK_SIP_KERNEL_MCUSYS_WRITE, SMC_IO_VIRT_TO_PHY(addr), val, 0)

#define mcusys_access_count() \
	mt_secure_call(MTK_SIP_KERNEL_MCUSYS_ACCESS_COUNT, 0, 0, 0)
#else
#define mcusys_smc_write(addr, val)      mt_reg_sync_writel(val, addr)
#define mcusys_access_count()            (0)
#endif

#endif /* _MT_SECURE_API_H_ */

