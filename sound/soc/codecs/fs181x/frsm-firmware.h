/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-06-14 File created.
 */

#ifndef __FRSM_FIRMWARE_H__
#define __FRSM_FIRMWARE_H__

#include <linux/slab.h>
#include <linux/crc16.h>
#include <linux/firmware.h>
#include "internal.h"

#ifndef FW_ACTION_HOTPLUG
#define FW_ACTION_HOTPLUG 1
#endif

static struct fwm_table *frsm_get_fwm_table(struct frsm_dev *frsm_dev, int type)
{
	struct fwm_table *table;
	struct fwm_index *index;
	int i, count;

	if (frsm_dev == NULL || frsm_dev->fwm_data == NULL)
		return NULL;

	table = (struct fwm_table *)frsm_dev->fwm_data->params;
	index = (struct fwm_index *)table->buf;

	count = table->size / sizeof(struct fwm_index);
	for (i = 0; i < count; i++, index++) {
		if (index->type != type)
			continue;
		return (struct fwm_table *)((char *)table + index->offset);
	}

	return NULL;
}

static int frsm_get_fwm_string(struct frsm_dev *frsm_dev,
		int offset, const char **pstr)
{
	struct fwm_table *table;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	table = frsm_get_fwm_table(frsm_dev, INDEX_STRING);
	if (table == NULL) {
		dev_err(frsm_dev->dev, "Failed to get string table!\n");
		return -EINVAL;
	}

	if (pstr && offset >= 0)
		*pstr = (char *)table + offset;

	return 0;
}

static int frsm_get_scene_count(struct frsm_dev *frsm_dev)
{
	struct scene_table *scene;
	int count;

	if (frsm_dev == NULL || frsm_dev->tbl_scene == NULL)
		return -EINVAL;

	scene = (struct scene_table *)frsm_dev->tbl_scene->buf;
	count = frsm_dev->tbl_scene->size / sizeof(*scene) - 1;

	return count;
}

static int frsm_get_scene_name(struct frsm_dev *frsm_dev,
		int scene_id, const char *name)
{
	struct scene_table *scene;
	int count;

	if (frsm_dev == NULL || frsm_dev->tbl_scene == NULL)
		return -EINVAL;

	count = frsm_get_scene_count(frsm_dev);
	if (scene_id < 0 || scene_id >= count) {
		dev_err(frsm_dev->dev, "Invalid scene id:%d\n", scene_id);
		return -ENOTSUPP;
	}

	scene = (struct scene_table *)frsm_dev->tbl_scene->buf;
	scene += scene_id + 1;

	return frsm_get_fwm_string(frsm_dev, scene->name, &name);
}

static int frsm_write_reg_table(struct frsm_dev *frsm_dev, uint16_t offset)
{
	struct fwm_table *table;
	struct reg_table *reg;
	int ret;

	if (frsm_dev == NULL)
		return -EINVAL;

	if (offset == 0xFFFF) {
		dev_info(frsm_dev->dev, "Skip to dump reg table\n");
		return 0;
	}

	table = frsm_get_fwm_table(frsm_dev, INDEX_REG);
	if (table == NULL) {
		dev_err(frsm_dev->dev, "Failed to find reg table\n");
		return -EINVAL;
	}

	reg = (struct reg_table *)((char *)table + offset);
	dev_dbg(frsm_dev->dev, "reg table size:%d\n", reg->size);
	if (reg->size == 0) {
		dev_err(frsm_dev->dev, "Invalid reg size:%d\n", reg->size);
		return -EINVAL;
	}

	ret = frsm_reg_write_table(frsm_dev, reg);

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static int frsm_write_model_table(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t offset)
{
	struct file_table *model;
	struct fwm_table *table;
	int i, burst_len;
	int ret;

	if (frsm_dev == NULL)
		return -EINVAL;

	if (reg == 0xFF || offset == 0xFFFF) {
		dev_info(frsm_dev->dev, "Skip to write model table\n");
		return 0;
	}

	table = frsm_get_fwm_table(frsm_dev, INDEX_MODEL);
	if (table == NULL) {
		dev_err(frsm_dev->dev, "Failed to find model table\n");
		return -EINVAL;
	}

	model = (struct file_table *)((char *)table + offset);
	dev_dbg(frsm_dev->dev, "model table size:%d\n", model->size);
	if (model->size == 0 || (model->size & 0x3)) {
		dev_err(frsm_dev->dev, "Invalid model size:%d\n", model->size);
		return -EINVAL;
	}

	burst_len = frsm_dev->bst_wcam_len;
	if (burst_len > model->size || (burst_len & 0x3)) {
		/* burst_len is 0 or multiples of 4 */
		dev_err(frsm_dev->dev,
				"Invalid burst len:%d\n", burst_len);
		return -EINVAL;
	}
	if (burst_len == 0) { // write all one time
		ret = frsm_reg_bulk_write(frsm_dev, reg,
				model->buf, model->size);
	} else {
		for (i = 0; i < model->size; i += burst_len)
			ret = frsm_reg_bulk_write(frsm_dev,
					reg, model->buf + i, burst_len);
	}

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static int frsm_write_effect_table(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t offset)
{
	struct file_table *effect;
	struct fwm_table *table;
	int i, burst_len;
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	if (reg == 0xFF || offset == 0xFFFF) {
		dev_info(frsm_dev->dev, "Skip to write effect table\n");
		return 0;
	}

	table = frsm_get_fwm_table(frsm_dev, INDEX_EFFECT);
	if (table == NULL) {
		dev_err(frsm_dev->dev, "Failed to find effect table\n");
		return -EINVAL;
	}

	effect = (struct file_table *)((char *)table + offset);
	dev_dbg(frsm_dev->dev, "effect table size:%d\n", effect->size);
	if (effect->size == 0 || (effect->size & 0x3)) {
		dev_err(frsm_dev->dev,
				"Invalid effect size:%d\n", effect->size);
		return -EINVAL;
	}

	burst_len = frsm_dev->bst_wcam_len;
	if (burst_len > effect->size || (burst_len & 0x3)) {
		/* burst_len is 0 or multiples of 4 */
		dev_err(frsm_dev->dev,
				"Invalid burst len:%d\n", burst_len);
		return -EINVAL;
	}
	if (burst_len == 0) { // write all one time
		ret = frsm_reg_bulk_write(frsm_dev, reg,
				effect->buf, effect->size);
	} else {
		for (i = 0; i < effect->size; i += burst_len)
			ret = frsm_reg_bulk_write(frsm_dev,
					reg, effect->buf + i, burst_len);
	}

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static int frsm_firmware_parser(struct frsm_dev *frsm_dev,
		struct fwm_header *hdr)
{
	struct fwm_table *table;
	uint16_t crcsum;
	int spkr_id;

	if (frsm_dev == NULL || hdr == NULL)
		return -EINVAL;

	if (LOW8(hdr->chip_type) == FRSM_DEVID_FS18YN &&
			HIGH8(frsm_dev->dev_id) == FRSM_DEVID_FS18HS) {
		/* FS1816 compatible with FS1832 */
		dev_info(frsm_dev->dev, "DEVID match compatible! fm:%04X dev:%04X\n",
				hdr->chip_type, frsm_dev->dev_id);
	} else if (LOW8(hdr->chip_type) != HIGH8(frsm_dev->dev_id)) {
		dev_err(frsm_dev->dev, "DEVID dismatch! fm:%04X != dev:%04X\n",
				hdr->chip_type, frsm_dev->dev_id);
		return -EINVAL;
	}

	spkr_id = frsm_dev->pdata ? frsm_dev->pdata->spkr_id : 0;
	if (hdr->rsvd[0] && hdr->rsvd[0] != spkr_id) {
		dev_err(frsm_dev->dev, "FWM: spkid dismatch!\n");
		return -EFAULT;
	}

	crcsum = crc16(0x0000, (char *)&hdr->crc_size, hdr->crc_size);
	if (crcsum != hdr->crc16) {
		dev_err(frsm_dev->dev, "Failed to check crcsum: %04X != %04X\n",
				hdr->crc16, crcsum);
		return -EINVAL;
	}

	if (frsm_dev->fwm_data)
		devm_kfree(frsm_dev->dev, (void *)frsm_dev->fwm_data);

	frsm_dev->fwm_data = hdr;

	table = frsm_get_fwm_table(frsm_dev, INDEX_STRING);
	if (table == NULL) {
		dev_err(frsm_dev->dev, "Failed to get string table!\n");
		return -EINVAL;
	}

	dev_info(frsm_dev->dev, "Project: %s\n", (char *)table + hdr->project);
	dev_info(frsm_dev->dev, "Device : %s\n", (char *)table + hdr->device);
	dev_info(frsm_dev->dev, "Date   : %04d%02d%02d-%02d%02d\n",
			hdr->date.year, hdr->date.month, hdr->date.day,
			hdr->date.hour, hdr->date.minute);

	return 0;
}

static void frsm_firmware_inited(const struct firmware *cont, void *context)
{
	struct frsm_dev *frsm_dev;
	struct fwm_header *hdr;
	int ret;

	if (cont == NULL || context == NULL)
		return;

	frsm_dev = (struct frsm_dev *)context;
	dev_info(frsm_dev->dev, "firmware size: %zu\n", cont->size);

	hdr = devm_kzalloc(frsm_dev->dev,
			cont->size, GFP_KERNEL);
	if (hdr == NULL)
		return;

	memcpy((void *)hdr, cont->data, cont->size);
	release_firmware(cont);

	ret = frsm_firmware_parser(frsm_dev, hdr);
	if (ret) {
		dev_err(frsm_dev->dev, "Failed to parse firmware: %d\n", ret);
		devm_kfree(frsm_dev->dev, hdr);
		return;
	}

	dev_info(frsm_dev->dev, "Load firmware success!\n");

	frsm_dev->state &= ~(BIT(EVENT_STAT_MNTR) - 1);
	frsm_dev->tbl_scene = NULL;
	ret = frsm_send_event(frsm_dev, EVENT_DEV_INIT);
	if (ret) {
		dev_err(frsm_dev->dev, "Failed to init device: %d\n", ret);
		devm_kfree(frsm_dev->dev, hdr);
		return;
	}

	ret = frsm_send_event(frsm_dev, EVENT_SET_SCENE);

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
}

static int frsm_firmware_init_sync(struct frsm_dev *frsm_dev, const char *name)
{
	const struct firmware *cont;
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL || name == NULL)
		return -EINVAL;

	dev_info(frsm_dev->dev, "loading %s in sync mode", name);
	ret = request_firmware(&cont, name, frsm_dev->dev);
	if (ret) {
		dev_err(frsm_dev->dev, "Failed to request %s: %d\n", name, ret);
		return ret;
	}

	frsm_firmware_inited(cont, (void *)frsm_dev);
	if (frsm_dev->fwm_data == NULL) {
		dev_err(frsm_dev->dev, "Failed to parse %s: %d", name, ret);
		return -EINVAL;
	}

	return 0;
}

static int frsm_firmware_init_async(struct frsm_dev *frsm_dev,
		const char *name)
{
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL || name == NULL)
		return -EINVAL;

	dev_info(frsm_dev->dev, "loading %s in async mode", name);
	frsm_dev->force_init = true;
	ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
			name, frsm_dev->dev, GFP_KERNEL, frsm_dev,
			frsm_firmware_inited);

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static inline void frsm_firmware_unused_func(void)
{
	frsm_firmware_init_sync(NULL, NULL);
	frsm_firmware_init_async(NULL, NULL);
	frsm_write_model_table(NULL, 0, 0);
	frsm_write_effect_table(NULL, 0, 0);
	frsm_get_scene_name(NULL, 0, NULL);
}

#endif // __FRSM_FIRMWARE_H__
