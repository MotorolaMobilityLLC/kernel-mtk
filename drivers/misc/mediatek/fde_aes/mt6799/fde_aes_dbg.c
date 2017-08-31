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

#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <mt-plat/mtk_secure_api.h>
#include "fde_aes.h"

static s32 fde_aes_dump_reg(void)
{
	void __iomem *FDE_AES_CORE_BASE;

	FDE_AES_CORE_BASE = fde_aes_get_base();
	if (!FDE_AES_CORE_BASE)
		return -FDE_ENODEV;

	FDEERR(" ################################ FDE_AES DUMP START (Kernel) ################################\n");

	FDEERR("CONTEXT_SEL   %p = %x\n", CONTEXT_SEL, FDE_READ32(CONTEXT_SEL));
	FDEERR("CONTEXT_WORD0 %p = %x\n", CONTEXT_WORD0, FDE_READ32(CONTEXT_WORD0));
	FDEERR("CONTEXT_WORD1 %p = %x\n", CONTEXT_WORD1, FDE_READ32(CONTEXT_WORD1));
	FDEERR("CONTEXT_WORD3 %p = %x\n", CONTEXT_WORD3, FDE_READ32(CONTEXT_WORD3));

	FDEERR("REG_COM_A  %p = %x\n", REG_COM_A, FDE_READ32(REG_COM_A));
	FDEERR("REG_COM_B  %p = %x\n", REG_COM_B, FDE_READ32(REG_COM_B));
	FDEERR("REG_COM_C  %p = %x\n", REG_COM_C, FDE_READ32(REG_COM_C));

	FDEERR("REG_COM_D %p = %x\n", REG_COM_D, FDE_READ32(REG_COM_D));
	FDEERR("REG_COM_E %p = %x\n", REG_COM_E, FDE_READ32(REG_COM_E));
	FDEERR("REG_COM_F %p = %x\n", REG_COM_F, FDE_READ32(REG_COM_F));
	FDEERR("REG_COM_G %p = %x\n", REG_COM_G, FDE_READ32(REG_COM_G));
	FDEERR("REG_COM_H %p = %x\n", REG_COM_H, FDE_READ32(REG_COM_H));
	FDEERR("REG_COM_I %p = %x\n", REG_COM_I, FDE_READ32(REG_COM_I));
	FDEERR("REG_COM_J %p = %x\n", REG_COM_J, FDE_READ32(REG_COM_J));
	FDEERR("REG_COM_K %p = %x\n", REG_COM_K, FDE_READ32(REG_COM_K));

	FDEERR("REG_COM_L %p = %x\n", REG_COM_L, FDE_READ32(REG_COM_L));
	FDEERR("REG_COM_M %p = %x\n", REG_COM_M, FDE_READ32(REG_COM_M));
	FDEERR("REG_COM_N %p = %x\n", REG_COM_N, FDE_READ32(REG_COM_N));
	FDEERR("REG_COM_O %p = %x\n", REG_COM_O, FDE_READ32(REG_COM_O));
	FDEERR("REG_COM_P %p = %x\n", REG_COM_P, FDE_READ32(REG_COM_P));
	FDEERR("REG_COM_Q %p = %x\n", REG_COM_Q, FDE_READ32(REG_COM_Q));
	FDEERR("REG_COM_R %p = %x\n", REG_COM_R, FDE_READ32(REG_COM_R));
	FDEERR("REG_COM_S %p = %x\n", REG_COM_S, FDE_READ32(REG_COM_S));

	FDEERR("REG_COM_T %p = %x\n", REG_COM_T, FDE_READ32(REG_COM_T));
	FDEERR("REG_COM_U %p = %x\n", REG_COM_U, FDE_READ32(REG_COM_U));

	FDEERR(" ################################ FDE_AES DUMP END ################################\n");

	return FDE_OK;
}

static void fde_aes_test_case(u32 testcase)
{
	void __iomem *FDE_AES_CORE_BASE;
	u32 status = 0;

	FDE_AES_CORE_BASE = fde_aes_get_base();
	if (!FDE_AES_CORE_BASE)
		return;

	FDEERR("Test Case %d\n", testcase);
	fde_aes_set_case(testcase);

	switch (testcase) {

	case 0: /* Normal */
		FDEERR("Normal Test\n");
		break;

	case 1: /* Normal write/read register */
		FDEERR("Normal write/read secure only register\n");
		FDE_WRITE32(REG_COM_A, 0xffffffff);
		if (FDE_READ32(REG_COM_A))
			status |= testcase;
		FDE_WRITE32(REG_COM_B, 0xffffffff);
		if (FDE_READ32(REG_COM_B))
			status |= testcase;
		FDE_WRITE32(REG_COM_C, 0xffffffff);
		if (FDE_READ32(REG_COM_C))
			status |= testcase;
		FDE_WRITE32(REG_COM_D, 0xffffffff);
		if (FDE_READ32(REG_COM_D))
			status |= testcase;
		FDE_WRITE32(REG_COM_E, 0xffffffff);
		if (FDE_READ32(REG_COM_E))
			status |= testcase;
		FDE_WRITE32(REG_COM_F, 0xffffffff);
		if (FDE_READ32(REG_COM_F))
			status |= testcase;
		FDE_WRITE32(REG_COM_G, 0xffffffff);
		if (FDE_READ32(REG_COM_G))
			status |= testcase;
		FDE_WRITE32(REG_COM_H, 0xffffffff);
		if (FDE_READ32(REG_COM_H))
			status |= testcase;
		FDE_WRITE32(REG_COM_I, 0xffffffff);
		if (FDE_READ32(REG_COM_I))
			status |= testcase;
		FDE_WRITE32(REG_COM_J, 0xffffffff);
		if (FDE_READ32(REG_COM_J))
			status |= testcase;
		FDE_WRITE32(REG_COM_K, 0xffffffff);
		if (FDE_READ32(REG_COM_K))
			status |= testcase;
		FDE_WRITE32(REG_COM_L, 0xffffffff);
		if (FDE_READ32(REG_COM_L))
			status |= testcase;
		FDE_WRITE32(REG_COM_M, 0xffffffff);
		if (FDE_READ32(REG_COM_M))
			status |= testcase;
		FDE_WRITE32(REG_COM_N, 0xffffffff);
		if (FDE_READ32(REG_COM_N))
			status |= testcase;
		FDE_WRITE32(REG_COM_O, 0xffffffff);
		if (FDE_READ32(REG_COM_O))
			status |= testcase;
		FDE_WRITE32(REG_COM_P, 0xffffffff);
		if (FDE_READ32(REG_COM_P))
			status |= testcase;
		FDE_WRITE32(REG_COM_Q, 0xffffffff);
		if (FDE_READ32(REG_COM_Q))
			status |= testcase;
		FDE_WRITE32(REG_COM_R, 0xffffffff);
		if (FDE_READ32(REG_COM_R))
			status |= testcase;
		FDE_WRITE32(REG_COM_S, 0xffffffff);
		if (FDE_READ32(REG_COM_S))
			status |= testcase;
		FDE_WRITE32(REG_COM_T, 0xffffffff);
		if (FDE_READ32(REG_COM_T))
			status |= testcase;
		FDE_WRITE32(REG_COM_U, 0xffffffff);
		if (FDE_READ32(REG_COM_U))
			status |= testcase;
		FDEERR("Test Case-%d %s\n", testcase, status == 0 ? "PASS" : "FAIL");
		break;

	case 2:
		fde_aes_dump_reg();
		break;

	case 10:
		mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0, 0xa, 0);
		break;

	case 11:
		FDEERR("FDE_AES check storage\n");
		break;

	default:
		FDEERR("Err Case %d\n", testcase);
		break;
	}
}

static ssize_t fde_aes_proc_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char buf[16];
	size_t len = count;
	int n;

	if (len >= sizeof(buf))
		len = sizeof(buf) - 1;

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';

	if (kstrtol(buf, 10, (long int *)&n))
		return -EINVAL;

	if (n < 0 || n >= 0xff)
		return -EFAULT;

	FDEERR("parameter %d\n", n);
	fde_aes_test_case(n);

	return len;
}

static int fde_aes_read(struct seq_file *m, void *v)
{
	seq_printf(m, "MTK HW FDE_AES : %s\n", fde_aes_get_hw() ? "Enable" : "Disable");
	seq_printf(m, "\teMMC:%s SD:%s\n", fde_aes_get_dev(FDE_MSDC0) ? "Enable" : "Disable",
					fde_aes_get_dev(FDE_MSDC1) ? "Enable" : "Disable");
	seq_printf(m, "\tTest Case %d\n", fde_aes_get_case());

	return 0;
}

static int fde_aes_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fde_aes_read, NULL);
}

static const struct file_operations fde_aes_proc_fops = {
	.open = fde_aes_proc_open,
	.write = fde_aes_proc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifndef USER_BUILD_KERNEL
#define PROC_PERM		0660
#else
#define PROC_PERM		0440
#endif

int fde_aes_proc_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("fde_aes", PROC_PERM, NULL, &fde_aes_proc_fops); /* Also for userload version. */

	if (!entry) {

		pr_err("[%s]: failed to create /proc/fde_aes\n", __func__);
		return -ENOMEM;
	}

	FDELOG("create /proc/fde_aes\n");

	return 0;
}
EXPORT_SYMBOL_GPL(fde_aes_proc_init);
