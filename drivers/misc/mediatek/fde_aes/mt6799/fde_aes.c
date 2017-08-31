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

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/mmc/mmc.h>
#include <crypto/hash.h>
#include <mt-plat/mtk_secure_api.h>
#include "fde_aes.h"
#include "fde_aes_dbg.h"

static mt_fde_aes_context fde_aes_context;

/* Property of fde_aes_context */
void __iomem *fde_aes_get_base(void)
{
	return fde_aes_context.base;
}

u32 fde_aes_get_hw(void)
{
	return fde_aes_context.hw_crypto;
}

void fde_aes_set_case(u32 test_case)
{
	fde_aes_context.test_case = test_case;
}

u32 fde_aes_get_case(void)
{
	return fde_aes_context.test_case;
}

static s32 fde_aes_set_context(void)
{
	void __iomem *FDE_AES_CORE_BASE = fde_aes_context.base;

	if (!fde_aes_context.base)
		return -FDE_ENODEV;

	/* Check CONTEXT_WORD */
	if (fde_aes_context.context_id & 0xfe)
		return -FDE_EINVAL;

	/* Check CONTEXT_BP */
	if (fde_aes_context.context_bp & 0xfe)
		return -FDE_EINVAL;

	/* Check Data unit (sector) size */
	if (fde_aes_context.sector_size == 0 || fde_aes_context.sector_size > 0x4)
		return -FDE_EINVAL;

	/* Context */
	FDE_WRITE32(CONTEXT_SEL, fde_aes_context.context_id);
	FDE_WRITE32(CONTEXT_WORD0,
		((fde_aes_context.sector_size << 6) | (fde_aes_context.context_id << 1) | fde_aes_context.context_bp));
	FDE_WRITE32(CONTEXT_WORD1, fde_aes_context.sector_offset_L);
	FDE_WRITE32(CONTEXT_WORD3, fde_aes_context.sector_offset_H);

	fde_aes_context.hw_crypto = 1;

	return FDE_OK;
}

/* Check if enable by eMMC and SD module */
s32 fde_aes_check_enable(s32 dev_num, u8 bEnable)
{
	#define MSDC_MAX_ID 2 /* 0:eMMC; 1:SD; 2:SDIO */
	static int iCnt[MSDC_MAX_ID] = {0};

	if (dev_num >= 2) /* SDIO etc */
		return 0;

	spin_lock(&fde_aes_context.lock);

	FDELOG("Before check MSDC%d dev-%s use-%s status-%s\n",
		dev_num, iCnt[dev_num]?"enable":"disable",
		bEnable?"enable":"disable",
		fde_aes_context.status?"enable":"disable");

	/* Suspend */
	if (!bEnable && (iCnt[dev_num] == 1)) {
		if (iCnt[!dev_num] == 0) {
			FDELOG("SMC MSDC%d off\n", dev_num);
			mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0xb, 0, 0);
			fde_aes_context.status = 0;
		}
		iCnt[dev_num] -= 1;
	}

	/* Resume */
	if (bEnable && (iCnt[dev_num] == 0)) {
		if (iCnt[!dev_num] == 0) {
			FDELOG("SMC MSDC%d on\n", dev_num);
			mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0xa, 0, 0);
			fde_aes_context.status = 1;
		}
		iCnt[dev_num] += 1;
	}

	FDELOG("After check MSDC%d dev-%s use-%s status-%s\n",
		dev_num, iCnt[dev_num]?"enable":"disable",
		bEnable?"enable":"disable",
		fde_aes_context.status?"enable":"disable");

	if (iCnt[dev_num] < 0) /* WARN_ON(iCnt[dev_num] < 0); */
		iCnt[dev_num] = 0;

	spin_unlock(&fde_aes_context.lock);

	return 0;
}

s32 fde_aes_exec(s32 dev_num, u32 blkcnt, u32 opcode)
{
	s32 status;

	if (!fde_aes_context.status) /* Need to enable FDE when MSDC send command */
		FDEERR("MSDC%d block 0x%x CMD%d exec\n", dev_num, blkcnt, opcode);

	BUG_ON(!fde_aes_context.status);

	spin_lock(&fde_aes_context.lock);

	fde_aes_context.context_id = dev_num;
	fde_aes_context.context_bp = 1;
	fde_aes_context.sector_size = 1;
	fde_aes_context.sector_offset_L = blkcnt;
	fde_aes_context.sector_offset_H = 0;

	status = fde_aes_set_context();

	spin_unlock(&fde_aes_context.lock);

	FDELOG("MSDC%d block 0x%x CMD%d exec\n", dev_num, blkcnt, opcode);

	return status;
}

s32 fde_aes_done(s32 dev_num, u32 blkcnt, u32 opcode)
{
	FDELOG("MSDC%d block 0x%x CMD%d done\n", dev_num, blkcnt, opcode);
	return 0;
}

static int __init fde_aes_init(void)
{
	struct device_node *node = NULL;
	int ret = 0;

	memset(&fde_aes_context, 0, sizeof(fde_aes_context));

	spin_lock_init(&fde_aes_context.lock);

	mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0xa, 0, 0);
	fde_aes_context.status = 1;

	/* FDE_AES Base */
	node = of_find_compatible_node(NULL, NULL, "mediatek,fde_aes_core");

	if (node) {

		fde_aes_context.base = of_iomap(node, 0);
		if (!fde_aes_context.base) {

			FDEERR("DT find compatible base failed\n");
			ret = -FDE_ENODEV;
		} else {

			FDELOG("register base %p\n", fde_aes_context.base);
			fde_aes_proc_init();
		}
	} else {

		FDEERR("DT find compatible node failed\n");
		ret = -FDE_ENODEV;
	}

	FDELOG("init status %s(%d)\n", ret?"fail":"pass", ret);
	return ret;
}

static void __exit fde_aes_exit(void)
{
	FDELOG("exit\n");
}

module_init(fde_aes_init);
module_exit(fde_aes_exit);

MODULE_DESCRIPTION("MTK FDE AES driver");
MODULE_LICENSE("GPL");
