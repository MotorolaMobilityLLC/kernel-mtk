/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2022-12-04 File created.
 */

#ifndef __FRSM_I2C_AMP_H__
#define __FRSM_I2C_AMP_H__

#include <linux/list.h>
#include "internal.h"
#include "frsm-amp-drv.h"

static LIST_HEAD(g_frsm_list);

static int frsm_for_each(int (*func)(struct frsm_dev *frsm_dev,
		struct frsm_argv *argv), struct frsm_argv *argv)
{
	struct frsm_dev *frsm_dev;
	int ret = 0;

	if (func == NULL || argv == NULL)
		return -EINVAL;

	if (list_empty(&g_frsm_list))
		return -ENODEV;

	frsm_mutex_lock();
	list_for_each_entry(frsm_dev, &g_frsm_list, list)
		ret |= func(frsm_dev, argv);
	frsm_mutex_unlock();

	return ret;
}

static struct frsm_dev *frsm_get_pdev(int addr_spkid)
{
	struct frsm_dev *frsm_dev;
	int data;

	if (list_empty(&g_frsm_list))
		return NULL;

	list_for_each_entry(frsm_dev, &g_frsm_list, list) {
		if (frsm_dev->pdata == NULL)
			continue;
		if (addr_spkid > FRSM_DEV_MAX)
			data = frsm_dev->pdata->vrtl_addr;
		else
			data = frsm_dev->pdata->spkr_id;
		if (addr_spkid != data)
			continue;
		return frsm_dev;
	}

	return NULL;
}

static int frsm_send_amp_event(int event, void *buf, int buf_size)
{
	return frsm_amp_send_event(event, buf, buf_size);
}

static int frsm_get_batt_volume(struct frsm_dev *frsm_dev, int prot_type,
		int level, int *pvol)
{
	struct frsm_prot_params *params;
	struct frsm_prot_tbl *tbl;
	int idx, index;
	int threshold;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	params = frsm_dev->pdata->prot_params + prot_type;
	if (!params->tbl)
		return 0;

	tbl = params->tbl;
	for (index = -1, idx = 0; idx < params->tbl_count; idx++) {
		threshold = (tbl + idx)->threshold;
		if (threshold < level)
			continue;
		if (index < 0)
			index = idx;
		if (threshold < (tbl + index)->threshold)
			index = idx;
	}

	if (index < 0 || pvol == NULL)
		return 0;

	if (*pvol > (tbl + index)->volume)
		*pvol = (tbl + index)->volume;

	return 0;
}

static int frsm_i2c_init_dev(struct frsm_argv *argv)
{
	struct frsm_mode_params *params;
	struct frsm_dev *frsm_dev;
	int ret;

	if (argv == NULL || argv->size < sizeof(struct frsm_mode_params))
		return -EINVAL;

	params = (struct frsm_mode_params *)argv->buf;
	frsm_dev = frsm_get_pdev(params->spkid);
	if (frsm_dev == NULL || frsm_dev->pdata == NULL) {
		pr_err("init_fwm: frsm_dev is null\n");
		return -EINVAL;
	}

	if (frsm_dev->fwm_data && !params->mode) /* unforce */
		return 0;

	ret = frsm_firmware_init_async(frsm_dev,
			frsm_dev->pdata->fwm_name);
	if (ret)
		dev_err(frsm_dev->dev, "Failed to init firmware: %d\n", ret);

	return ret;
}

static int frsm_i2c_set_scene(struct frsm_argv *argv)
{
	struct frsm_mode_params *params;
	struct frsm_dev *frsm_dev;
	int nscene;
	int ret;

	if (argv == NULL || argv->size < sizeof(struct frsm_mode_params))
		return -EINVAL;

	params = (struct frsm_mode_params *)argv->buf;
	frsm_dev = frsm_get_pdev(params->spkid);
	if (frsm_dev == NULL || frsm_dev->dev == NULL) {
		pr_err("set_scene: frsm_dev is null\n");
		return -EINVAL;
	}

	if (params->mode == frsm_dev->next_scene)
		return 0;

	if (!frsm_dev->fwm_data) {
		ret = frsm_firmware_init_sync(frsm_dev,
				frsm_dev->pdata->fwm_name);
		if (ret)
			return ret;
	}

	ret = frsm_send_event(frsm_dev, EVENT_DEV_INIT);
	if (ret)
		return ret;

	nscene = frsm_get_scene_count(frsm_dev);
	if (params->mode < 0 || params->mode >= nscene) {
		dev_err(frsm_dev->dev, "Out of range scene:%d\n", params->mode);
		return -ENOTSUPP;
	}

	dev_info(frsm_dev->dev, "Switch scene: %d->%d\n",
			frsm_dev->next_scene, params->mode);

	frsm_dev->next_scene = params->mode;
	ret = frsm_send_event(frsm_dev, EVENT_SET_SCENE);
	if (ret) {
		dev_err(frsm_dev->dev, "Failed to set scene:%d\n", ret);
		frsm_dev->next_scene = -1;
	}

	return ret;
}

static int frsm_i2c_amp_switch(struct frsm_argv *argv)
{
	struct frsm_mode_params *params;
	struct frsm_dev *frsm_dev;
	bool state;
	int ret;

	if (argv == NULL || argv->size < sizeof(struct frsm_mode_params))
		return -EINVAL;

	params = (struct frsm_mode_params *)argv->buf;
	frsm_dev = frsm_get_pdev(params->spkid);
	if (frsm_dev == NULL)
		return -ENODEV;

	if (!frsm_dev->fwm_data) {
		ret = frsm_firmware_init_sync(frsm_dev,
				frsm_dev->pdata->fwm_name);
		if (ret)
			return ret;
	}

	state = test_bit(EVENT_STREAM_ON, &frsm_dev->state);
	if ((state ^ params->mode) == 0)
		return 0;

	if (params->mode) {
		ret = frsm_send_event(frsm_dev, EVENT_DEV_INIT);
		if (ret)
			return ret;
		ret  = frsm_send_event(frsm_dev, EVENT_HW_PARAMS);
		ret |= frsm_send_event(frsm_dev, EVENT_START_UP);
		ret |= frsm_send_event(frsm_dev, EVENT_STREAM_ON);
	} else {
		ret  = frsm_send_event(frsm_dev, EVENT_STREAM_OFF);
		ret |= frsm_send_event(frsm_dev, EVENT_SHUT_DOWN);
		ret |= frsm_send_event(frsm_dev, EVENT_SET_IDLE);
		ret |= frsm_send_event(frsm_dev, EVENT_DEV_INIT);
	}

	return ret;
}

static int frsm_i2c_amp_read(struct frsm_argv *argv)
{
	struct frsm_amp_reg *amp_reg;
	struct frsm_dev *frsm_dev;
	int ret;

	if (argv == NULL || argv->size < sizeof(struct frsm_amp_reg))
		return -EINVAL;

	amp_reg = (struct frsm_amp_reg *)argv->buf;
	frsm_dev = frsm_get_pdev(amp_reg->addr);
	if (frsm_dev == NULL)
		return -ENODEV;

	frsm_mutex_lock();
	mutex_lock(&frsm_dev->io_lock);
	ret = i2c_master_recv(frsm_dev->i2c,
			amp_reg->buf, amp_reg->size);
	mutex_unlock(&frsm_dev->io_lock);
	frsm_mutex_unlock();

	return ret;
}

static int frsm_i2c_amp_write(struct frsm_argv *argv)
{
	struct frsm_amp_reg *amp_reg;
	struct frsm_dev *frsm_dev;
	int ret;

	if (argv == NULL || argv->size < sizeof(struct frsm_amp_reg))
		return -EINVAL;

	amp_reg = (struct frsm_amp_reg *)argv->buf;
	frsm_dev = frsm_get_pdev(amp_reg->addr);
	if (frsm_dev == NULL)
		return -ENODEV;

	frsm_mutex_lock();
	mutex_lock(&frsm_dev->io_lock);
	ret = i2c_master_send(frsm_dev->i2c,
			amp_reg->buf, amp_reg->size);
	mutex_unlock(&frsm_dev->io_lock);
	frsm_mutex_unlock();

	return ret;
}

static int frsm_i2c_set_batt(struct frsm_dev *frsm_dev, struct frsm_argv *argv)
{
	struct frsm_batt *batt;
	int new_volume;
	int smask;
	int ret;

	if (frsm_dev == NULL || argv == NULL)
		return -EINVAL;

	if (argv->buf == NULL || argv->size != sizeof(struct frsm_batt))
		return -EINVAL;

	if (!frsm_dev->prot_batt || frsm_dev->pdata == NULL)
		return 0;

	smask = frsm_dev->pdata->mntr_scenes & BIT(frsm_dev->next_scene);
	if (!smask && frsm_dev->batt_vol == FRSM_VOLUME_MAX)
		return 0;

	batt = (struct frsm_batt *)argv->buf;
	new_volume = FRSM_VOLUME_MAX;
	ret = frsm_get_batt_volume(frsm_dev, FRSM_PROT_BSG,
			batt->cap, &new_volume);
	ret = frsm_get_batt_volume(frsm_dev, FRSM_PROT_CSG,
			batt->tempr, &new_volume);

	frsm_dev->batt_vol = new_volume;
	dev_dbg(frsm_dev->dev, "batt_vol:%d\n", new_volume);
	ret = frsm_stub_set_volume(frsm_dev);

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static int frsm_i2c_set_spkre(struct frsm_dev *frsm_dev, struct frsm_argv *argv)
{
	struct spkr_info *info;
	struct live_data *data;
	int ret;

	if (frsm_dev == NULL || argv == NULL)
		return -EINVAL;

	if (argv->buf == NULL || argv->size < sizeof(struct spkr_info))
		return -EINVAL;

	info = (struct spkr_info *)argv->buf;
	if (frsm_dev->pdata->spkr_id > 0)
		data = info->data + frsm_dev->pdata->spkr_id - 1;
	else if (info->ndev == 1)
		data = info->data;
	else
		return -EFAULT;

	dev_info(frsm_dev->dev, "Set spkr.%d re25:%d\n",
			frsm_dev->pdata->spkr_id, data->spkre);

	frsm_dev->save_otp_spkre = false;
	if (data->spkre > 0 && data->spkr0 == 1) {
		dev_info(frsm_dev->dev, "save_otp_spkre on!\n");
		frsm_dev->save_otp_spkre = true;
	}

	frsm_dev->spkre = data->spkre;
	if (frsm_dev->spkre == 0x3FFFF) { // 64 << 12 - 1
		frsm_dev->calib_mode = true;
		frsm_reg_dump(frsm_dev, 0xCF); // Debugging
		return 0;
	}

	ret = frsm_stub_set_spkr_prot(frsm_dev);
	frsm_dev->save_otp_spkre = false;
	frsm_dev->calib_mode = false;

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static int frsm_i2c_get_livedata(struct frsm_dev *frsm_dev,
		struct frsm_argv *argv)
{
	struct spkr_info *info;
	struct live_data *data;
	int ret;

	if (frsm_dev == NULL || argv == NULL)
		return -EINVAL;

	if (argv->buf == NULL || argv->size < sizeof(struct spkr_info))
		return -EINVAL;

	if (!frsm_dev->ops.get_livedata)
		return -ENOTSUPP;

	info = (struct spkr_info *)argv->buf;
	if (frsm_dev->pdata->spkr_id > 0)
		data = info->data + frsm_dev->pdata->spkr_id - 1;
	else if (info->ndev == 1)
		data = info->data;
	else
		return -EFAULT;

	data->spkre = frsm_dev->spkre;
	ret = frsm_dev->ops.get_livedata(frsm_dev, data);

	return ret;
}

static int frsm_i2c_set_tuning(struct frsm_dev *frsm_dev,
		struct frsm_argv *argv)
{
	char *state;

	if (frsm_dev == NULL || argv == NULL)
		return -EINVAL;

	if (argv->buf == NULL || argv->size < sizeof(char))
		return -EINVAL;

	state = (char *)argv->buf;
	if (*state)
		set_bit(STATE_TUNING, &frsm_dev->state);
	else
		clear_bit(STATE_TUNING, &frsm_dev->state);

	return 0;
}

static int frsm_i2c_get_mntren(struct frsm_dev *frsm_dev,
		struct frsm_argv *argv)
{
	int *mntr_en;
	int mask;

	if (frsm_dev == NULL || argv == NULL)
		return -EINVAL;

	if (argv->buf == NULL || argv->size < sizeof(int))
		return -EINVAL;

	mntr_en = (int *)argv->buf;
	mask = test_bit(EVENT_STREAM_ON, &frsm_dev->state);
	if (!frsm_dev->pdata || !mask)
		return 0;

	mask = frsm_dev->pdata->mntr_scenes & BIT(frsm_dev->next_scene);
	*mntr_en |= !!mask;

	return 0;
}

static int frsm_i2c_set_mute(struct frsm_dev *frsm_dev, struct frsm_argv *argv)
{
	int reg, value;
	int ret;

	if (!frsm_dev->reg_amp_mute)
		return 0;

	if (!test_bit(EVENT_STREAM_ON, &frsm_dev->state))
		return 0;

	ret = frsm_stub_tsmute(frsm_dev, 1);

	reg = HIGH16(frsm_dev->reg_amp_mute);
	value = LOW16(frsm_dev->reg_amp_mute);
	ret |= frsm_reg_write(frsm_dev, reg & 0xFF, value);
	if (ret)
		dev_err(frsm_dev->dev, "Failed to mute sync\n");

	return ret;
}

static int frsm_i2c_get_dtype(struct frsm_dev *frsm_dev, struct frsm_argv *argv)
{
	uint16_t *data;
	int spkid;

	if (HIGH8(frsm_dev->dev_id) != FRSM_DEVID_FS19AM)
		return 0;

	spkid = frsm_dev->pdata->spkr_id;
	if (spkid < 0 || spkid > FRSM_DEV_MAX)
		return -EINVAL;

	data = (uint16_t *)argv->buf;
	if (spkid <= 0)
		data[1] = frsm_dev->dtype;
	else
		data[spkid] = frsm_dev->dtype;

	return 0;
}

static int frsm_notify_callback(int event, void *buf, int size)
{
	struct frsm_argv argv;
	int ret;

	argv.size = size;
	argv.buf = buf;
	switch (event) {
	case EVENT_AMP_INIT_DEV:
		ret = frsm_i2c_init_dev(&argv);
		break;
	case EVENT_AMP_SET_SCENE:
		ret = frsm_i2c_set_scene(&argv);
		break;
	case EVENT_AMP_SPK_SWITCH:
		ret = frsm_i2c_amp_switch(&argv);
		break;
	case EVENT_AMP_REG_READ:
		ret = frsm_i2c_amp_read(&argv);
		break;
	case EVENT_AMP_REG_WRITE:
		ret = frsm_i2c_amp_write(&argv);
		break;
	case EVENT_AMP_SET_BATT:
		ret = frsm_for_each(frsm_i2c_set_batt, &argv);
		break;
	case EVENT_AMP_SET_CALRE:
		ret = frsm_for_each(frsm_i2c_set_spkre, &argv);
		break;
	case EVENT_AMP_GET_LIVEDATA:
		ret = frsm_for_each(frsm_i2c_get_livedata, &argv);
		break;
	case EVENT_AMP_SET_TUNING:
		ret = frsm_for_each(frsm_i2c_set_tuning, &argv);
		break;
	case EVENT_AMP_GET_MNTREN:
		ret = frsm_for_each(frsm_i2c_get_mntren, &argv);
		break;
	case EVENT_AMP_MUTE_SYNC:
		ret = frsm_for_each(frsm_i2c_set_mute, &argv);
		break;
	case EVENT_AMP_GET_DTYPE:
		ret = frsm_for_each(frsm_i2c_get_dtype, &argv);
		break;
	default:
		pr_err("%s: Unkown event:%d\n", __func__, event);
		ret = -ENOTSUPP;
		break;
	}

	return ret;
}

static int frsm_i2c_parse_prot_tbl(struct frsm_dev *frsm_dev, int prot_type)
{
	struct frsm_prot_tbl *prot_tbl;
	struct property *prop;
	const __be32 *buf32;
	const char *prop_name;
	int i, len, count;

	if (prot_type == FRSM_PROT_BSG)
		prop_name = "prot-bsg-table";
	else if (prot_type == FRSM_PROT_CSG)
		prop_name = "prot-csg-table";
	else
		return -EINVAL;

	prop = of_find_property(frsm_dev->dev->of_node, prop_name, &len);
	if (prop == NULL || prop->value == NULL)
		return 0;

	if ((len & 0x7) != 0) { // len must be 2*sizeof(int)*N bytes
		dev_err(frsm_dev->dev, "Invalid table size:%d\n", len);
		return -EINVAL;
	}

	prot_tbl = devm_kzalloc(frsm_dev->dev, len, GFP_KERNEL);
	if (prot_tbl == NULL)
		return -ENOMEM;

	buf32 = prop->value;
	count = len / sizeof(struct frsm_prot_tbl);
	frsm_dev->prot_batt = true;
	frsm_dev->pdata->prot_params[prot_type].tbl = prot_tbl;
	frsm_dev->pdata->prot_params[prot_type].tbl_count = count;
	dev_dbg(frsm_dev->dev, "table.%d count:%d\n", prot_type, count);

	for (i = 0; i < count; i++, prot_tbl++) {
		prot_tbl->threshold = be32_to_cpup(buf32++);
		prot_tbl->volume = be32_to_cpup(buf32++);
	}

	return 0;
}

static int frsm_i2c_amp_parse_dts(struct frsm_dev *frsm_dev)
{
	int ret;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	frsm_dev->pdata->bsg_volume_v2 = of_property_read_bool(
			frsm_dev->dev->of_node, "bsg-volume-v2");

	ret = of_property_read_u32(frsm_dev->dev->of_node, "mntr-scenes",
			&frsm_dev->pdata->mntr_scenes);
	if (ret)
		frsm_dev->pdata->mntr_scenes = 0;

	ret = frsm_i2c_parse_prot_tbl(frsm_dev, FRSM_PROT_BSG);
	if (ret)
		dev_err(frsm_dev->dev, "Failed to parse bsg tbl:%d\n", ret);

	ret = frsm_i2c_parse_prot_tbl(frsm_dev, FRSM_PROT_CSG);
	if (ret)
		dev_err(frsm_dev->dev, "Failed to parse csg tbl:%d\n", ret);

	return 0;
}

static int frsm_i2c_amp_init(struct frsm_dev *frsm_dev)
{
	int ret;

	INIT_LIST_HEAD(&frsm_dev->list);
	list_add_tail(&frsm_dev->list, &g_frsm_list);

	ret = frsm_i2c_amp_parse_dts(frsm_dev);

	return ret;
}

static inline void frsm_i2c_unused_func(void)
{
	frsm_send_amp_event(0, NULL, 0);
}

#endif // __FRSM_I2C_AMP_H__
