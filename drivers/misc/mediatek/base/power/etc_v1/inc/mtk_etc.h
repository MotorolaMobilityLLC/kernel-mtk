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
#ifndef _MTK_ETC_
#define _MTK_ETC_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <mt-plat/sync_write.h>
#endif

#if defined(__MTK_ETC_C__)
#include <mt-plat/mtk_secure_api.h>
#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define mt_secure_call_etc	mt_secure_call
#else
/* This is workaround for ocp use */
static noinline int mt_secure_call_etc(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
	register u64 reg0 __asm__("x0") = function_id;
	register u64 reg1 __asm__("x1") = arg0;
	register u64 reg2 __asm__("x2") = arg1;
	register u64 reg3 __asm__("x3") = arg2;
	int ret = 0;

	asm volatile(
	"smc    #0\n"
		: "+r" (reg0)
		: "r" (reg1), "r" (reg2), "r" (reg3));

	ret = (int)reg0;
	return ret;
}
#endif
#endif


#ifdef CONFIG_ARM64
#define MTK_SIP_KERNEL_ETC_INIT		0xC2000300
#define MTK_SIP_KERNEL_ETC_VOLT_CHG     0xC2000301
#define MTK_SIP_KERNEL_ETC_PWR_OFF      0xC2000302
#define MTK_SIP_KERNEL_ETC_DMT_CTL      0xC2000303
#define MTK_SIP_KERNEL_ETC_RED		0xC2000304
#define MTK_SIP_KERNEL_ETC_ANK		0xC2000305
#else
#define MTK_SIP_KERNEL_ETC_INIT		0x82000300
#define MTK_SIP_KERNEL_ETC_VOLT_CHG     0x82000301
#define MTK_SIP_KERNEL_ETC_PWR_OFF      0x82000302
#define MTK_SIP_KERNEL_ETC_DMT_CTL      0x82000303
#define MTK_SIP_KERNEL_ETC_RED		0x82000304
#define MTK_SIP_KERNEL_ETC_ANK		0x82000305
#endif

enum mtk_etc_cpu_id {
	MT_ETC_CPU_LL,
	MT_ETC_CPU_L,
	MT_ETC_CPU_B,
	MT_ETC_CPU_CCI,

	NR_MT_ETC_CPU,
};

extern void mtk_etc_init(void);
extern void mtk_etc_voltage_change(unsigned int new_vout);
extern void mtk_etc_power_off(void);
extern void mtk_dormant_ctrl(unsigned int onOff);

/* SRAM debugging */
#ifdef CONFIG_MTK_RAM_CONSOLE
extern void aee_rr_rec_etc_status(u8 val);
extern void aee_rr_rec_etc_mode(u8 val);

extern u8 aee_rr_curr_etc_status(void);
extern u8 aee_rr_curr_etc_mode(void);
#endif

#endif
