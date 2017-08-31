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

u8 fde_aes_get_hw(void)
{
	return fde_aes_context.hw_crypto;
}

u8 fde_aes_get_dev(u8 id)
{
	return fde_aes_context.dev_num[id];
}

void fde_aes_set_case(u32 test_case)
{
#ifdef USER_BUILD_KERNEL
	fde_aes_context.test_case = 0;
#else
	fde_aes_context.test_case = test_case;
#endif
}

u32 fde_aes_get_case(void)
{
	return fde_aes_context.test_case;
}

void fde_aes_set_log(u8 enable)
{
	fde_aes_context.log = enable;
}

u8 fde_aes_get_log(void)
{
	return fde_aes_context.log;
}

void fde_aes_set_msdc_id(u8 id)
{
	fde_aes_context.msdc_id = id;
}

u8 fde_aes_get_msdc_id(void)
{
	return fde_aes_context.msdc_id;
}

void fde_aes_set_fde(u8 enable)
{
	fde_aes_context.enable_fde = enable;
}

u8 fde_aes_get_fde(void)
{
	return fde_aes_context.enable_fde;
}

void fde_aes_set_raw(u8 enable)
{
	fde_aes_context.raw = enable;
}

u8 fde_aes_get_raw(void)
{
	return fde_aes_context.raw;
}

void fde_aes_set_range(u8 enable)
{
	fde_aes_context.chk_range = enable;
}

u8 fde_aes_get_range(void)
{
	return fde_aes_context.chk_range;
}

void fde_aes_set_range_start(u32 start)
{
	fde_aes_context.chk_start = start;
}

u32 fde_aes_get_range_start(void)
{
	return fde_aes_context.chk_start;
}

void fde_aes_set_range_end(u32 end)
{
	fde_aes_context.chk_end = end;
}

u32 fde_aes_get_range_end(void)
{
	return fde_aes_context.chk_end;
}

void fde_aes_set_sw(u8 enable)
{
	fde_aes_context.sw_crypto = enable;
}

u8 fde_aes_get_sw(void)
{
	return fde_aes_context.sw_crypto;
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

	fde_aes_context.dev_num[fde_aes_context.context_id] = 1;
	if (fde_aes_check_cmd(FDE_AES_EN_MSG, fde_aes_get_log(), fde_aes_context.context_id))
		FDEERR("%s MSDC%d Size %d BP %d H 0x%x L 0x%x\n",
			__func__,
			fde_aes_context.context_id,
			fde_aes_context.sector_size,
			fde_aes_context.context_bp,
			fde_aes_context.sector_offset_H,
			fde_aes_context.sector_offset_L);

	return FDE_OK;
}

/* Check if enable by eMMC and SD module */
s32 fde_aes_check_enable(s32 dev_num, u8 bEnable)
{
	#define MSDC_MAX_ID 2 /* 0:eMMC; 1:SD; 2:SDIO */
	static int iCnt[MSDC_MAX_ID] = {0};
	unsigned long flags;

	if (dev_num >= 2) /* SDIO etc */
		return 0;

	spin_lock_irqsave(&fde_aes_context.lock, flags);

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

	spin_unlock_irqrestore(&fde_aes_context.lock, flags);

	return 0;
}

s32 fde_aes_exec(s32 dev_num, u32 blkcnt, u32 opcode)
{
	s32 status;
	unsigned long flags;

	if (!fde_aes_context.status) { /* Need to enable FDE when MSDC send command */
		FDEERR("MSDC%d block 0x%x CMD%d exec\n", dev_num, blkcnt, opcode);
		fde_aes_check_enable(dev_num, 1);
	}

	spin_lock_irqsave(&fde_aes_context.lock, flags);

	fde_aes_context.context_id = dev_num;
	fde_aes_context.context_bp = 1;
	fde_aes_context.sector_size = 1;
	fde_aes_context.sector_offset_L = blkcnt;
	fde_aes_context.sector_offset_H = 0;

	if (fde_aes_check_cmd(FDE_AES_EN_MSG, fde_aes_get_log(), dev_num))
		FDEERR("%s MSDC%d CMD%d block 0x%x\n", __func__, dev_num, opcode, blkcnt);

	status = fde_aes_set_context();

	spin_unlock_irqrestore(&fde_aes_context.lock, flags);

	return status;
}

s32 fde_aes_done(s32 dev_num, u32 blkcnt, u32 opcode)
{
	FDELOG("MSDC%d block 0x%x CMD%d done\n", dev_num, blkcnt, opcode);
	return 0;
}

static int fde_aes_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;

	memset(&fde_aes_context, 0, sizeof(fde_aes_context));

	spin_lock_init(&fde_aes_context.lock);

	/* FDE_AES Base */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	fde_aes_context.base = devm_ioremap_resource(&pdev->dev, res);
	if (!fde_aes_context.base) {

		FDEERR("DT find compatible base failed\n");
		ret = -FDE_ENODEV;
	} else {

		FDELOG("register base %p\n", fde_aes_context.base);
		fde_aes_proc_init();
	}


	if (!ret)
		fde_aes_context.hw_crypto = 1;

	FDELOG("init status %s(%d)\n", ret?"fail":"pass", ret);
	return ret;
}

static int fde_aes_remove(struct platform_device *dev)
{
	mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0xb, 0, 0);
	FDELOG("exit\n");
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id fde_aes_of_ids[] = {
	{ .compatible = "mediatek,fde_aes_core", },
	{}
};
#endif

static struct platform_driver mtk_fde_aes_driver = {
	.probe = fde_aes_probe,
	.remove = fde_aes_remove,
	.driver = {
		.name = "fde_aes",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = fde_aes_of_ids,
#endif
		},
};

static int __init fde_aes_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_fde_aes_driver);
	if (ret)
		FDEERR("init FAIL, ret 0x%x!!!\n", ret);

	return ret;
}

module_init(fde_aes_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek Inc.");
MODULE_DESCRIPTION("MTK FDE AES driver");
