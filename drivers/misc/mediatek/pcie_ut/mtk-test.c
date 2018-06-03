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

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <linux/device.h>
#include <asm/unaligned.h>
#include <linux/uaccess.h>
#include "mtk-test.h"
#include "mtk-test-lib.h"

const struct file_operations mtk_pcie_test_fops;

#define PCIE_PADDR 0x193f0000
void __iomem *MTK_REG_BASE;


#define CLI_MAGIC "CLI"
#define IOCTL_READ 0
#define IOCTL_WRITE 1

#define BUF_SIZE 200
#define MAX_ARG_SIZE 10

char w_buf[BUF_SIZE];
char r_buf[BUF_SIZE] = "this is a test";


struct CMD_TBL_T {
	char name[256];
	int (*cb_func)(int argc, char **argv);
};

struct CMD_TBL_T _arPCmdTbl[] = {
	{"pcie.register", &t_pcie_register_pci},
	{"pcie.unregister", &t_pcie_unregister_pci},
	{"pcie.mmio", &t_pcie_mmio},
	{"pcie.speed", &t_pcie_change_speed},
	{"pcie.speed_test", &t_pcie_speed_test},
	{"pcie.interrupt", &t_pcie_setup_int},
	{"pcie.aspm", &t_pcie_aspm},
	{"pcie.retrain", &t_pcie_retrain},
	{"pcie.aspm_retrain", &t_pcie_aspm_retrain},
	{"pcie.pcipm", &t_pcie_pcipm},
	{"pcie.l1pm", &t_pcie_l1pm},
	{"pcie.l1ss", &t_pcie_l1ss},
	{"pcie.reset", &t_pcie_reset},
	{"pcie.configuration", &t_pcie_configuration},
	{"pcie.linkwidth", &t_pcie_link_width},
	{"pcie.reset_config", &t_pcie_reset_config},
	{"pcie.memory", &t_pcie_memory},
	{"pcie.loopback", &t_pcie_loopback},
	{"pcie.loopback_cr4", &t_pcie_loopback_cr4},
#if 0
	{"pcie.phy_read", &t_pcie_phy_read},
	{"pcie.phy_write", &t_pcie_phy_write},
	{"pcie.phy_scan", &t_pcie_phy_scan},
#endif
	/* 6632 */
	{"pcie.set_own", &t_pcie_set_own},
	{"pcie.clear_own", &t_pcie_clear_own},
	{"pcie.test", &t_pcie_test},
	/* 6632 */

	{"mpcie.register", &t_pcie_register_pci},
	{"mpcie.unregister", &t_pcie_unregister_pci},

};

int call_function(char *buf)
{
	int i, ret, match;
	int argc;
	char *argv[MAX_ARG_SIZE];

	argc = 0;
	do {
		argv[argc] = strsep(&buf, " ");
		pr_debug("[%d] %s\r\n", argc, argv[argc]);
		argc++;
	} while (buf);

	match = 0;
	for (i = 0; i < ARRAY_SIZE(_arPCmdTbl); i++) {

		if ((!strcmp(_arPCmdTbl[i].name, argv[0])) &&
			(_arPCmdTbl[i].cb_func != NULL)) {

			ret = _arPCmdTbl[i].cb_func(argc, argv);
			for (i = 0; i < argc; i++)
				pr_debug("[%d] %s\r\n", i, argv[i]);

			pr_debug("RESULT: %s\r\n", (!ret ? "PASS" : "FAIL"));

			return ret;
		}
	}

	return -1;
}

static int mtk_pcie_test_open(struct inode *inode, struct file *file)
{
	MTK_REG_BASE = ioremap(PCIE_PADDR, 0x2000);
	pr_debug("mtk_pcie_test open: successful\n");
	return 0;
}
EXPORT_SYMBOL(MTK_REG_BASE);

static int mtk_pcie_test_release(struct inode *inode, struct file *file)
{
	pr_debug("mtk_pcie_test release: successful\n");
	return 0;
}

static ssize_t mtk_pcie_test_read(struct file *file,
	char *buf,
	size_t count,
	loff_t *ptr)
{
	pr_debug("mtk_pcie_test read: returning zero bytes\n");
	return 0;
}

static ssize_t mtk_pcie_test_write(struct file *file,
	const char *buf,
	size_t count,
	loff_t *ppos)
{
	pr_debug("mtk_pcie_test write: accepting zero bytes\n");
	return 0;
}

static long mtk_pcie_test_ioctl(struct file *file,
	unsigned int cmd,
	unsigned long arg)
{

	int len = BUF_SIZE;
	int err;

	switch (cmd) {
	case IOCTL_READ:
		err = copy_to_user((char *) arg, r_buf, len);
		if (err < 0)
			pr_err("mtk_pcie_test_ioctl copy_to_user error!\n");
		break;

	case IOCTL_WRITE:
		err = copy_from_user(w_buf, (char *) arg, len);
		if (err < 0) {
			pr_err("mtk_pcie_test_ioctl copy_from_user error!\n");
			break;
		}

		/* invoke function */
		err =  call_function(w_buf);
		break;

	default:
		return -ENOTTY;
	}

	return err;
}

const struct file_operations mtk_pcie_test_fops = {
	.owner =	THIS_MODULE,
	.read =		mtk_pcie_test_read,
	.write =	mtk_pcie_test_write,
	.unlocked_ioctl = mtk_pcie_test_ioctl,
	.open =		mtk_pcie_test_open,
	.release =	mtk_pcie_test_release,
};

static int __init mtk_test_init(void)
{

	int retval = 0;
	struct class *f_class;

	pr_debug("mtk_pcie_test Init\n");

	retval = register_chrdev(MTK_PCIE_TEST_MAJOR, DEVICE_NAME,
							 &mtk_pcie_test_fops);
	if (retval < 0)	{
		pr_debug("mtk_pcie_test Init failed, %d\n", retval);
		goto fail;
	}

	f_class = class_create(THIS_MODULE, "cli");
	device_create(f_class, NULL, MKDEV(MTK_PCIE_TEST_MAJOR, 0), NULL,
				  "cli");

	return 0;
fail:
	return retval;

}

late_initcall(mtk_test_init);
MODULE_LICENSE("GPL");


