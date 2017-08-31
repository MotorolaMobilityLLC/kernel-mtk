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
#include "fde_aes_dbg.h"

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

u32 fde_aes_check_cmd(u32 cmd, u32 p1, u32 p2)
{
	u32 ret = 0;

	if (fde_aes_get_case() == cmd) {

		switch (cmd) {

		case FDE_AES_NORMAL:
		case FDE_AES_WR_REG:
		case FDE_AES_DUMP:
		case FDE_AES_DUMP_ALL:
			ret = 0;
			break;

		case FDE_AES_EN_MSG:
		case FDE_AES_EN_FDE:
		case FDE_AES_EN_RAW:
		case FDE_AES_CK_RANGE:
			if (p1 == 1 && fde_aes_get_msdc_id() == p2)
				ret = 1;
			break;

		case FDE_AES_EN_SW_CRYPTO:
			if (p1 == 1)
				ret = 1;
			break;

		default:
			FDEERR("Err Command %d\n", cmd);
			break;
		}
	}

	return ret;
}

static void fde_aes_test_case(char *buf)
{
	void __iomem *FDE_AES_CORE_BASE;
	int sscanf_num;
	int cmd, p1, p2, p3, p4;
	int status;

	cmd = p1 = p2 = p3 = p4 = -1;
	status = 0;

	sscanf_num = sscanf(buf, "%x %x %x %x", &cmd, &p1, &p2, &p3);

	if (cmd == -1)
		return;

	FDE_AES_CORE_BASE = fde_aes_get_base();
	if (!FDE_AES_CORE_BASE)
		return;

	FDEERR("Command %d Para %d %d %d %d\n", cmd, p1, p2, p3, p4);
	fde_aes_set_case(cmd);
	fde_aes_set_msdc_id(FDE_MSDC_MAX);

	switch (cmd) {

	case FDE_AES_NORMAL:
		FDEERR("Normal Test\n");
		break;

	case FDE_AES_WR_REG:
		FDEERR("Normal write/read secure only register\n");
		FDE_WRITE32(REG_COM_A, 0xffffffff);
		if (FDE_READ32(REG_COM_A))
			status |= cmd;
		FDE_WRITE32(REG_COM_B, 0xffffffff);
		if (FDE_READ32(REG_COM_B))
			status |= cmd;
		FDE_WRITE32(REG_COM_C, 0xffffffff);
		if (FDE_READ32(REG_COM_C))
			status |= cmd;
		FDE_WRITE32(REG_COM_D, 0xffffffff);
		if (FDE_READ32(REG_COM_D))
			status |= cmd;
		FDE_WRITE32(REG_COM_E, 0xffffffff);
		if (FDE_READ32(REG_COM_E))
			status |= cmd;
		FDE_WRITE32(REG_COM_F, 0xffffffff);
		if (FDE_READ32(REG_COM_F))
			status |= cmd;
		FDE_WRITE32(REG_COM_G, 0xffffffff);
		if (FDE_READ32(REG_COM_G))
			status |= cmd;
		FDE_WRITE32(REG_COM_H, 0xffffffff);
		if (FDE_READ32(REG_COM_H))
			status |= cmd;
		FDE_WRITE32(REG_COM_I, 0xffffffff);
		if (FDE_READ32(REG_COM_I))
			status |= cmd;
		FDE_WRITE32(REG_COM_J, 0xffffffff);
		if (FDE_READ32(REG_COM_J))
			status |= cmd;
		FDE_WRITE32(REG_COM_K, 0xffffffff);
		if (FDE_READ32(REG_COM_K))
			status |= cmd;
		FDE_WRITE32(REG_COM_L, 0xffffffff);
		if (FDE_READ32(REG_COM_L))
			status |= cmd;
		FDE_WRITE32(REG_COM_M, 0xffffffff);
		if (FDE_READ32(REG_COM_M))
			status |= cmd;
		FDE_WRITE32(REG_COM_N, 0xffffffff);
		if (FDE_READ32(REG_COM_N))
			status |= cmd;
		FDE_WRITE32(REG_COM_O, 0xffffffff);
		if (FDE_READ32(REG_COM_O))
			status |= cmd;
		FDE_WRITE32(REG_COM_P, 0xffffffff);
		if (FDE_READ32(REG_COM_P))
			status |= cmd;
		FDE_WRITE32(REG_COM_Q, 0xffffffff);
		if (FDE_READ32(REG_COM_Q))
			status |= cmd;
		FDE_WRITE32(REG_COM_R, 0xffffffff);
		if (FDE_READ32(REG_COM_R))
			status |= cmd;
		FDE_WRITE32(REG_COM_S, 0xffffffff);
		if (FDE_READ32(REG_COM_S))
			status |= cmd;
		FDE_WRITE32(REG_COM_T, 0xffffffff);
		if (FDE_READ32(REG_COM_T))
			status |= cmd;
		FDE_WRITE32(REG_COM_U, 0xffffffff);
		if (FDE_READ32(REG_COM_U))
			status |= cmd;
		FDEERR("Test Case-%d %s\n", cmd, status == 0 ? "PASS" : "FAIL");
		break;

	case FDE_AES_DUMP:
		fde_aes_dump_reg();
		break;

	case FDE_AES_DUMP_ALL:
		mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0, 0xa, 0);
		break;

	case FDE_AES_EN_MSG:
		FDEERR("FDE_AES enable log\n");
		fde_aes_set_log(p1);
		fde_aes_set_msdc_id(p2);
		break;

	case FDE_AES_EN_FDE:
		FDEERR("FDE_AES enable FDE\n");
		fde_aes_set_fde(p1);
		fde_aes_set_msdc_id(p2);
		break;

	case FDE_AES_EN_RAW:
		FDEERR("FDE_AES enable raw\n");
		fde_aes_set_raw(p1);
		fde_aes_set_msdc_id(p2);
		break;

	case FDE_AES_CK_RANGE:
		FDEERR("FDE_AES enable raw\n");
		fde_aes_set_range(p1);
		fde_aes_set_msdc_id(p2);
		fde_aes_set_range_start(p3);
		fde_aes_set_range_end(p4);
		break;

	case FDE_AES_EN_SW_CRYPTO:
		FDEERR("FDE_AES enable sw crypto\n");
		fde_aes_set_sw(p1);
		fde_aes_set_msdc_id(p2);
		break;

	default:
		FDEERR("Err Command %d\n", cmd);
		break;
	}
}

static ssize_t fde_aes_proc_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char buf[16];
	size_t len = count;

	if (len >= sizeof(buf))
		len = sizeof(buf) - 1;

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';

	fde_aes_test_case(buf);

	return len;
}

static int fde_aes_read(struct seq_file *m, void *v)
{
	seq_puts(m, "\nMTK HW FDE_AES Command :\n");
	seq_puts(m, "\t echo cmd p1 p2 p3 p4 > /proc/fde_aes\n");

	seq_puts(m, "Normal case :\n");
	seq_printf(m, "\t echo %d > /proc/fde_aes\n", FDE_AES_NORMAL);

	seq_puts(m, "Write register test :\n");
	seq_printf(m, "\t echo %d > /proc/fde_aes\n", FDE_AES_WR_REG);

	seq_puts(m, "Dump registers :\n");
	seq_printf(m, "\t echo %d > /proc/fde_aes\n", FDE_AES_DUMP);

	seq_puts(m, "Dump all registers :\n");
	seq_printf(m, "\t echo %d > /proc/fde_aes\n", FDE_AES_DUMP_ALL);

	seq_puts(m, "Enable FDE message [enable] [msdc-id] :\n");
	seq_printf(m, "\t echo %d 1 0 > /proc/fde_aes\n", FDE_AES_EN_MSG);

	seq_puts(m, "Enable FDE [enable] [msdc-id] :\n");
	seq_printf(m, "\t echo %d 1 0 > /proc/fde_aes\n", FDE_AES_EN_FDE);

	seq_puts(m, "Read cipher data [enable] [msdc-id] :\n");
	seq_printf(m, "\t echo %d 1 0 > /proc/fde_aes\n", FDE_AES_EN_RAW);

	seq_puts(m, "Check read/write range [enable] [msdc-id] [start-block] [end-block] :\n");
	seq_printf(m, "\t echo %d 1 1 8800 3AAFDE > /proc/fde_aes\n", FDE_AES_CK_RANGE);

	seq_puts(m, "Enable sw crypto [enable] :\n");
	seq_printf(m, "\t echo %d 1 > /proc/fde_aes\n", FDE_AES_EN_SW_CRYPTO);

	seq_puts(m, "Please input in HEX\n");

	seq_printf(m, "\nMTK HW FDE_AES : %s\n", fde_aes_get_hw() ? "Enable" : "Disable");
	seq_printf(m, "\teMMC:%s SD:%s\n", fde_aes_get_dev(FDE_MSDC0) ? "Enable" : "Disable",
					fde_aes_get_dev(FDE_MSDC1) ? "Enable" : "Disable");
	seq_printf(m, "\tTest Case %d\n\n", fde_aes_get_case());

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
