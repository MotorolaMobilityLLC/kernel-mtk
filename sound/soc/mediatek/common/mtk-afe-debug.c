// SPDX-License-Identifier: GPL-2.0
//
// mtk-afe-debug.c  --  Mediatek AFE Debug
//
// Copyright (c) 2018 MediaTek Inc.
// Author: Kai Chieh Chuang <kaichieh.chuang@mediatek.com>

#include <linux/device.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/uaccess.h>

#include <sound/soc.h>

#include "mtk-afe-debug.h"

#include "mtk-base-afe.h"

#define MAX_DEBUG_WRITE_INPUT 256

/* debugfs ops */
int mtk_afe_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

ssize_t mtk_afe_debugfs_write(struct file *f, const char __user *buf,
			      size_t count, loff_t *offset)
{
	struct mtk_base_afe *afe = f->private_data;
	char input[MAX_DEBUG_WRITE_INPUT];
	char *temp = input;
	char *command = NULL;
	char delim[] = " ,\n";
	const struct mtk_afe_debug_cmd *cf;

	memset((void *)input, 0, MAX_DEBUG_WRITE_INPUT);

	if (count > MAX_DEBUG_WRITE_INPUT)
		count = MAX_DEBUG_WRITE_INPUT;

	if (copy_from_user(input, buf, count))
		dev_warn(afe->dev, "%s(), copy_from_user fail, count = %zu\n",
			 __func__, count);

	command = strsep(&temp, delim);

	for (cf = afe->debug_cmds; cf->cmd; cf++) {
		if (strcmp(cf->cmd, command) == 0) {
			cf->fn(f, temp);
			break;
		}
	}

	return count;
}

/* debug function */
void mtk_afe_debug_write_reg(struct file *file, void *arg)
{
	struct mtk_base_afe *afe = file->private_data;
	char *token1 = NULL;
	char *token2 = NULL;
	char *temp = arg;
	char delim[] = " ,";
	unsigned long reg_addr = 0;
	unsigned long reg_value = 0;
	unsigned int reg_value_after;
	int ret = 0;

	token1 = strsep(&temp, delim);
	token2 = strsep(&temp, delim);
	dev_info(afe->dev, "%s(), token1 %s, token2 %s, temp %s\n",
		 __func__, token1, token2, temp);

	if ((token1 != NULL) && (token2 != NULL)) {
		ret = kstrtoul(token1, 16, &reg_addr);
		ret = kstrtoul(token2, 16, &reg_value);
		dev_info(afe->dev, "%s(), reg_addr 0x%lx, reg_value 0x%lx\n",
			 __func__, reg_addr, reg_value);

		regmap_write(afe->regmap, reg_addr, reg_value);
		regmap_read(afe->regmap, reg_addr, &reg_value_after);

		dev_info(afe->dev, "%s(), reg_addr 0x%lx, reg_value_after 0x%x\n",
			 __func__, reg_addr, reg_value_after);
	} else {
		dev_warn(afe->dev, "token1 or token2 is NULL!\n");
	}
}

MODULE_DESCRIPTION("Mediatek AFE Debug");
MODULE_AUTHOR("Kai Chieh Chuang <kaichieh.chuang@mediatek.com>");
MODULE_LICENSE("GPL v2");

