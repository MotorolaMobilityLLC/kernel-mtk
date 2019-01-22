/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/kernel.h>
#include <mt-plat/sync_write.h>

#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI) || defined(CONFIG_MTK_HW_FDE)
/* Error Code */
#define SIP_SVC_E_SUCCESS               0
#define SIP_SVC_E_NOT_SUPPORTED         -1
#define SIP_SVC_E_INVALID_PARAMS        -2
#define SIP_SVC_E_INVALID_Range         -3
#define SIP_SVC_E_PERMISSION_DENY       -4

#ifdef CONFIG_ARM64
#define MTK_SIP_SMC_AARCH_BIT			0x40000000
#else
#define MTK_SIP_SMC_AARCH_BIT			0x00000000
#endif

#define MTK_SIP_KERNEL_MCUSYS_WRITE         (0x82000201 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_MCUSYS_ACCESS_COUNT  (0x82000202 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_L2_SHARING           (0x82000203 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_WDT                  (0x82000204 | MTK_SIP_SMC_AARCH_BIT)

#define MTK_SIP_KERNEL_GIC_DUMP         (0x82000205 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_DAPC_INIT        (0x82000206 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_EMIMPU_WRITE         (0x82000207 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_EMIMPU_READ          (0x82000208 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_EMIMPU_SET           (0x82000209 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_SCP_RESET            (0x8200020B | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_DISABLE_DFD	    (0x8200020F | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_ICACHE_DUMP          (0x82000210 | MTK_SIP_SMC_AARCH_BIT)

#define MTK_SIP_KERNEL_MCSI_A_WRITE		(0x82000211 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_MCSI_A_READ		(0x82000212 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_TIME_SYNC		(0x82000213 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_DFD			(0x82000214 | MTK_SIP_SMC_AARCH_BIT)
/*CPU operations*/
#define MTK_SIP_POWER_DOWN_CLUSTER	(0x82000215 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_POWER_UP_CLUSTER	(0x82000216 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERENL_MCSI_NS_ACCESS		(0x82000217 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_POWER_DOWN_CORE		(0x82000218 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_POWER_UP_CORE		(0x82000219 | MTK_SIP_SMC_AARCH_BIT)
/* SPM related SMC call */
#define MTK_SIP_KERNEL_SPM_SUSPEND_ARGS		(0x8200021A | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_SPM_FIRMWARE_STATUS	(0x8200021B | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_SPM_IRQ0_HANDLER		(0x8200021C | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_SPM_AP_MDSRC_REQ		(0x8200021D | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS	(0x8200021E | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_SPM_LEGACY_SLEEP		(0x8200021F | MTK_SIP_SMC_AARCH_BIT)

/* SPM related SMC call */
#define MTK_SIP_KERNEL_SPM_VCOREFS_ARGS		(0x82000220 | MTK_SIP_SMC_AARCH_BIT)

/* SPM deepidle related SMC call */
#define MTK_SIP_KERNEL_SPM_DPIDLE_ARGS		(0x82000221 | MTK_SIP_SMC_AARCH_BIT)

/* SPM SODI related SMC call */
#define MTK_SIP_KERNEL_SPM_SODI_ARGS		(0x82000222 | MTK_SIP_SMC_AARCH_BIT)

/* DCM SMC call */
#define MTK_SIP_KERNEL_DCM			(0x82000223 | MTK_SIP_SMC_AARCH_BIT)

/* SPM DCS S1 control */
#define MTK_SIP_KERNEL_SPM_DCS_S1		(0x82000224 | MTK_SIP_SMC_AARCH_BIT)

/* SPM dummy read */
#define MTK_SIP_KERNEL_SPM_DUMMY_READ		(0x82000225 | MTK_SIP_SMC_AARCH_BIT)

/* SPM sleep deepidle related SMC call */
#define MTK_SIP_KERNEL_SPM_SLEEP_DPIDLE_ARGS	(0x82000226 | MTK_SIP_SMC_AARCH_BIT)

#define MTK_SIP_KERNEL_CHECK_SECURE_CG		(0x82000227 | MTK_SIP_SMC_AARCH_BIT)

/* HW FDE related SMC call */
#define MTK_SIP_KERNEL_HW_FDE_UFS_INIT      (0x82000230 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_HW_FDE_AES_INIT      (0x82000231 | MTK_SIP_SMC_AARCH_BIT)

/* LPDMA SMC calls */
#define MTK_SIP_KERNEL_LPDMA_WRITE		(0x82000231 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_LPDMA_READ		(0x82000232 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_LPDMA_GET		(0x82000233 | MTK_SIP_SMC_AARCH_BIT)

/* FIQ CACHE */
#define MTK_SIP_KERNEL_CACHE_FLUSH_FIQ		(0x82000234 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_CACHE_FLUSH_INIT		(0x82000235 | MTK_SIP_SMC_AARCH_BIT)

#define MTK_SIP_KERNEL_ICCS_STATE		(0x82000236 | MTK_SIP_SMC_AARCH_BIT)

#define MTK_SIP_KERNEL_DRAM_DCS_CHB		(0x82000289 | MTK_SIP_SMC_AARCH_BIT)
#define MTK_SIP_KERNEL_MSG                  (0x820002ff | MTK_SIP_SMC_AARCH_BIT)

/* SCP DVFS related SMC call */
#define MTK_SIP_KERNEL_SCP_DVFS_CTRL	(0x820003E2 | MTK_SIP_SMC_AARCH_BIT)

#define TBASE_SMC_AEE_DUMP                  (0xB200AEED)

#ifdef CONFIG_ARM64
/* SIP SMC Call 64 */
static noinline int mt_secure_call(u64 function_id,
	u64 arg0, u64 arg1, u64 arg2)
{
	register u64 reg0 __asm__("x0") = function_id;
	register u64 reg1 __asm__("x1") = arg0;
	register u64 reg2 __asm__("x2") = arg1;
	register u64 reg3 __asm__("x3") = arg2;
	int ret = 0;

	asm volatile ("smc    #0\n" : "+r" (reg0) :
		"r"(reg1), "r"(reg2), "r"(reg3));

	ret = (int)reg0;
	return ret;
}

#else
#include <asm/opcodes-sec.h>
#include <asm/opcodes-virt.h>
/* SIP SMC Call 32 */
static noinline int mt_secure_call(u32 function_id,
	u32 arg0, u32 arg1, u32 arg2)
{
	register u32 reg0 __asm__("r0") = function_id;
	register u32 reg1 __asm__("r1") = arg0;
	register u32 reg2 __asm__("r2") = arg1;
	register u32 reg3 __asm__("r3") = arg2;
	int ret = 0;

	asm volatile (__SMC(0) : "+r"(reg0),
		"+r"(reg1), "+r"(reg2), "+r"(reg3));

	ret = reg0;
	return ret;
}
#endif
#define tbase_trigger_aee_dump() \
	mt_secure_call(TBASE_SMC_AEE_DUMP, 0, 0, 0)
#endif

#define dram_smc_dcs(OnOff) \
mt_secure_call(MTK_SIP_KERNEL_DRAM_DCS_CHB, OnOff, 0, 0)

#define emi_mpu_smc_write(offset, val) \
mt_secure_call(MTK_SIP_KERNEL_EMIMPU_WRITE, offset, val, 0)

#define emi_mpu_smc_read(offset) \
mt_secure_call(MTK_SIP_KERNEL_EMIMPU_READ, offset, 0, 0)

#define emi_mpu_smc_set(start, end, region_permission) \
mt_secure_call(MTK_SIP_KERNEL_EMIMPU_SET, start, end, region_permission)

#define lpdma_smc_write(offset, val) \
mt_secure_call(MTK_SIP_KERNEL_LPDMA_WRITE, offset, val, 0)

#define lpdma_smc_read(offset) \
mt_secure_call(MTK_SIP_KERNEL_LPDMA_READ, offset, 0, 0)

#define lpdma_smc_get_mode() \
mt_secure_call(MTK_SIP_KERNEL_LPDMA_GET, 0, 0, 0)

#define mcsi_a_smc_read_phy(addr) \
mt_secure_call(MTK_SIP_KERNEL_MCSI_A_READ, addr, 0, 0)

#define mcsi_a_smc_write_phy(addr, val) \
mt_secure_call(MTK_SIP_KERNEL_MCSI_A_WRITE, addr, val, 0)

#define dcm_smc_msg(init_type) \
mt_secure_call(MTK_SIP_KERNEL_DCM, init_type, 0, 0)

#define dcm_smc_read_cnt(type) \
mt_secure_call(MTK_SIP_KERNEL_DCM, type, 1, 0)

#define CONFIG_MCUSYS_WRITE_PROTECT

#if defined(CONFIG_MCUSYS_WRITE_PROTECT) && \
	(defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI))

#define kernel_smc_msg(x1, x2, x3) \
	mt_secure_call(MTK_SIP_KERNEL_MSG, x1, x2, x3)

#define mcsi_reg_read(offset) \
	mt_secure_call(MTK_SIP_KERENL_MCSI_NS_ACCESS, 0, offset, 0)

#define mcsi_reg_write(val, offset) \
	mt_secure_call(MTK_SIP_KERENL_MCSI_NS_ACCESS, 1, offset, val)

#define mcsi_reg_set_bitmask(val, offset) \
	mt_secure_call(MTK_SIP_KERENL_MCSI_NS_ACCESS, 2, offset, val)

#define mcsi_reg_clear_bitmask(val, offset) \
	mt_secure_call(MTK_SIP_KERENL_MCSI_NS_ACCESS, 3, offset, val)

#ifdef CONFIG_ARM64		/* Kernel 64 */
#define mcusys_smc_write(virt_addr, val) \
	mt_reg_sync_writel(val, virt_addr)

#define mcusys_smc_write_phy(addr, val) \
mt_secure_call(MTK_SIP_KERNEL_MCUSYS_WRITE, addr, val, 0)

#define mcusys_access_count() \
mt_secure_call(MTK_SIP_KERNEL_MCUSYS_ACCESS_COUNT, 0, 0, 0)

#else				/* Kernel 32 */
#define SMC_IO_VIRT_TO_PHY(addr) (addr-0xF0000000+0x10000000)
#define mcusys_smc_write(virt_addr, val) \
	mt_secure_call(MTK_SIP_KERNEL_MCUSYS_WRITE, \
		SMC_IO_VIRT_TO_PHY(virt_addr), val, 0)

#define mcusys_smc_write_phy(addr, val) \
mt_secure_call(MTK_SIP_KERNEL_MCUSYS_WRITE, addr, val, 0)

#define mcusys_access_count() \
mt_secure_call(MTK_SIP_KERNEL_MCUSYS_ACCESS_COUNT, 0, 0, 0)
#endif

#else
#define mcusys_smc_write(virt_addr, val) \
	mt_reg_sync_writel(val, virt_addr)
#define mcusys_access_count()               (0)
#define kernel_smc_msg(x1, x2, x3)
#endif

#endif				/* _MT_SECURE_API_H_ */
