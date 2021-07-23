#include <linux/module.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/syscalls.h>
#include <sound/control.h>
#include <linux/uaccess.h>

#include "aw_data_type.h"
#include "aw_log.h"
#include "aw_device.h"
#include "aw_bin_parse.h"
#include "aw_calib.h"

#define AW_DEV_SYSST_CHECK_MAX   (10)

enum {
	AW_EXT_DSP_WRITE_NONE = 0,
	AW_EXT_DSP_WRITE,
};

static char *profile_name[AW_PROFILE_MAX] = {"Music", "Voice", "Voip", "Ringtone", "Ringtone_hs", "Lowpower",
						"Bypass", "Mmi", "Fm", "Notification", "Receiver"};

static unsigned int g_fade_in_time = AW_1000_US / 10;
static unsigned int g_fade_out_time = AW_1000_US >> 1;
static LIST_HEAD(g_dev_list);
static DEFINE_MUTEX(g_dev_lock);

static int aw_dev_reg_dump(struct aw_device *aw_dev)
{
	int reg_num = aw_dev->ops.aw_get_reg_num();
	uint8_t i = 0;
	uint16_t reg_val = 0;

	for (i = 0; i < reg_num; i++) {
		if (aw_dev->ops.aw_check_rd_access(i)) {
			aw_dev->ops.aw_reg_read(aw_dev, i, &reg_val);
			aw_dev_info(aw_dev->dev, "read: reg = 0x%02x, val = 0x%04x",
				i, reg_val);
		}
	}

	return 0;
}

int aw_dev_get_prof_data(struct aw_device *aw_dev, int index,
			struct aw_prof_desc **prof_desc);


static uint8_t aw_dev_crc8_check(unsigned char *data , uint32_t data_size)
{
	uint8_t crc_value = 0x00;
	uint8_t pdatabuf = 0;
	int i;

	while (data_size--) {
		pdatabuf = *data++;
		for (i = 0; i < 8; i++) {
			/*if the lowest bit is 1*/
			if ((crc_value ^ (pdatabuf)) & 0x01) {
				/*Xor multinomial*/
				crc_value ^= 0x18;
				crc_value >>= 1;
				crc_value |= 0x80;
			} else {
				crc_value >>= 1;
			}
			pdatabuf >>= 1;
		}
	}
	return crc_value;
}

static int aw_dev_check_cfg_by_hdr(struct aw_container *aw_cfg)
{
	struct aw_cfg_hdr *cfg_hdr = NULL;
	struct aw_cfg_dde *cfg_dde = NULL;
	unsigned int end_data_offset = 0;
	unsigned int act_data = 0;
	unsigned int hdr_ddt_len = 0;
	uint8_t act_crc8 = 0;
	int i;

	cfg_hdr = (struct aw_cfg_hdr *)aw_cfg->data;

	/*check file type id is awinic acf file*/
	if (cfg_hdr->a_id != ACF_FILE_ID) {
		aw_pr_err("not acf type file");
		return -EINVAL;
	}

	hdr_ddt_len = cfg_hdr->a_hdr_offset + cfg_hdr->a_ddt_size;
	if (hdr_ddt_len > aw_cfg->len) {
		aw_pr_err("hdrlen with ddt_len [%d] overflow file size[%d]",
		cfg_hdr->a_hdr_offset, aw_cfg->len);
		return -EINVAL;
	}

	/*check data size*/
	cfg_dde = (struct aw_cfg_dde *)((char *)aw_cfg->data + cfg_hdr->a_hdr_offset);
	act_data += hdr_ddt_len;
	for (i = 0; i < cfg_hdr->a_ddt_num; i++)
		act_data += cfg_dde[i].data_size;

	if (act_data != aw_cfg->len) {
		aw_pr_err("act_data[%d] not equal to file size[%d]!",
			act_data, aw_cfg->len);
		return -EINVAL;
	}

	for (i = 0; i < cfg_hdr->a_ddt_num; i++) {
		/* data check */
		end_data_offset = cfg_dde[i].data_offset + cfg_dde[i].data_size;
		if (end_data_offset > aw_cfg->len) {
			aw_pr_err("a_ddt_num[%d] end_data_offset[%d] overflow file size[%d]",
				i, end_data_offset, aw_cfg->len);
			return -EINVAL;
		}

		/* crc check */
		act_crc8 = aw_dev_crc8_check(aw_cfg->data + cfg_dde[i].data_offset, cfg_dde[i].data_size);
		if (act_crc8 != cfg_dde[i].data_crc) {
			aw_pr_err("a_ddt_num[%d] crc8 check failed, act_crc8:0x%x != data_crc 0x%x",
				i, (uint32_t)act_crc8, cfg_dde[i].data_crc);
			return -EINVAL;
		}
	}

	aw_pr_info("project name [%s]", cfg_hdr->a_project);
	aw_pr_info("custom name [%s]", cfg_hdr->a_custom);
	aw_pr_info("version name [%d.%d.%d.%d]", cfg_hdr->a_version[3], cfg_hdr->a_version[2],
						cfg_hdr->a_version[1], cfg_hdr->a_version[0]);
	aw_pr_info("author id %d", cfg_hdr->a_author_id);

	return 0;
}

int aw_dev_load_acf_check(struct aw_container *aw_cfg)
{
	struct aw_cfg_hdr *cfg_hdr = NULL;

	if (aw_cfg == NULL) {
		aw_pr_err("aw_prof is NULL");
		return -ENOMEM;
	}

	if (aw_cfg->len < sizeof(struct aw_cfg_hdr)) {
		aw_pr_err("cfg hdr size[%d] overflow file size[%d]",
			aw_cfg->len, (int)sizeof(struct aw_cfg_hdr));
		return -EINVAL;
	}

	cfg_hdr = (struct aw_cfg_hdr *)aw_cfg->data;
	switch (cfg_hdr->a_hdr_version) {
	case AW_CFG_HDR_VER_0_0_0_1:
		return aw_dev_check_cfg_by_hdr(aw_cfg);
	default:
		aw_pr_err("unsupported hdr_version [0x%x]", cfg_hdr->a_hdr_version);
		return -EINVAL;
	}

	return 0;
}

static int aw_dev_dsp_data_order(struct aw_device *aw_dev,
				uint8_t *data, uint32_t data_len)
{
	int i = 0;
	uint8_t tmp_val = 0;

	aw_dev_dbg(aw_dev->dev, "enter");

	if (data_len % 2 != 0) {
		aw_dev_dbg(aw_dev->dev, "data_len:%d unsupported", data_len);
		return -EINVAL;
	}

	for (i = 0; i < data_len; i += 2) {
		tmp_val = data[i];
		data[i] = data[i + 1];
		data[i + 1] = tmp_val;
	}

	return 0;
}

static int aw_dev_parse_raw_reg(struct aw_device *aw_dev,
		uint8_t *data, uint32_t data_len, struct aw_prof_desc *prof_desc)
{
	aw_dev_info(aw_dev->dev, "data_size:%d enter", data_len);

	prof_desc->sec_desc[AW_DATA_TYPE_REG].data = data;
	prof_desc->sec_desc[AW_DATA_TYPE_REG].len = data_len;

	prof_desc->prof_st = AW_PROFILE_OK;

	return 0;
}

static int aw_dev_parse_raw_dsp_cfg(struct aw_device *aw_dev,
		uint8_t *data, uint32_t data_len, struct aw_prof_desc *prof_desc)
{
	int ret;

	aw_dev_info(aw_dev->dev, "data_size:%d enter", data_len);

	ret = aw_dev_dsp_data_order(aw_dev, data, data_len);
	if (ret < 0)
		return ret;

	prof_desc->sec_desc[AW_DATA_TYPE_DSP_CFG].data = data;
	prof_desc->sec_desc[AW_DATA_TYPE_DSP_CFG].len = data_len;

	prof_desc->prof_st = AW_PROFILE_OK;

	return 0;
}

static int aw_dev_parse_raw_dsp_fw(struct aw_device *aw_dev,
		uint8_t *data, uint32_t data_len, struct aw_prof_desc *prof_desc)
{
	int ret;

	aw_dev_info(aw_dev->dev, "data_size:%d enter", data_len);

	ret = aw_dev_dsp_data_order(aw_dev, data, data_len);
	if (ret < 0)
		return ret;

	prof_desc->sec_desc[AW_DATA_TYPE_DSP_FW].data = data;
	prof_desc->sec_desc[AW_DATA_TYPE_DSP_FW].len = data_len;

	prof_desc->prof_st = AW_PROFILE_OK;

	return 0;
}

static int aw_dev_prof_parse_multi_bin(struct aw_device *aw_dev,
		uint8_t *data, uint32_t data_len, struct aw_prof_desc *prof_desc)
{
	struct aw_bin *aw_bin = NULL;
	int i;
	int ret;

	aw_bin = devm_kzalloc(aw_dev->dev, data_len + sizeof(struct aw_bin), GFP_KERNEL);
	if (aw_bin == NULL) {
		aw_dev_err(aw_dev->dev, "kzalloc aw_bin failed");
		return -ENOMEM;
	}

	aw_bin->info.len = data_len;
	memcpy(aw_bin->info.data, data, data_len);

	ret = aw_parsing_bin_file(aw_bin);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "parse bin failed");
		goto parse_bin_failed;
	}

	for (i = 0; i < aw_bin->all_bin_parse_num; i++) {
		if (aw_bin->header_info[i].bin_data_type == DATA_TYPE_REGISTER) {
			prof_desc->sec_desc[AW_DATA_TYPE_REG].len = aw_bin->header_info[i].valid_data_len;
			prof_desc->sec_desc[AW_DATA_TYPE_REG].data = data + aw_bin->header_info[i].valid_data_addr;
		} else if (aw_bin->header_info[i].bin_data_type == DATA_TYPE_DSP_REG) {
			ret= aw_dev_dsp_data_order(aw_dev, data + aw_bin->header_info[i].valid_data_addr,
					aw_bin->header_info[i].valid_data_len);
			if (ret < 0)
				return ret;

			prof_desc->sec_desc[AW_DATA_TYPE_DSP_CFG].len = aw_bin->header_info[i].valid_data_len;
			prof_desc->sec_desc[AW_DATA_TYPE_DSP_CFG].data = data + aw_bin->header_info[i].valid_data_addr;
		} else if (aw_bin->header_info[i].bin_data_type == DATA_TYPE_DSP_FW) {
			ret = aw_dev_dsp_data_order(aw_dev, data + aw_bin->header_info[i].valid_data_addr,
					aw_bin->header_info[i].valid_data_len);
			if (ret < 0)
				return ret;
			prof_desc->sec_desc[AW_DATA_TYPE_DSP_FW].len = aw_bin->header_info[i].valid_data_len;
			prof_desc->sec_desc[AW_DATA_TYPE_DSP_FW].data = data + aw_bin->header_info[i].valid_data_addr;
		}
	}
	devm_kfree(aw_dev->dev, aw_bin);
	aw_bin = NULL;
	prof_desc->prof_st = AW_PROFILE_OK;
	return 0;

parse_bin_failed:
	devm_kfree(aw_dev->dev, aw_bin);
	aw_bin = NULL;
	return ret;
}

static int aw_dev_parse_data_by_sec_type(struct aw_device *aw_dev, struct aw_cfg_hdr *cfg_hdr,
			struct aw_cfg_dde *cfg_dde, struct aw_prof_desc *scene_prof_desc)
{

	switch (cfg_dde->data_type) {
	case ACF_SEC_TYPE_REG:
		return aw_dev_parse_raw_reg(aw_dev,
				(uint8_t *)cfg_hdr + cfg_dde->data_offset,
				cfg_dde->data_size, scene_prof_desc);
	case ACF_SEC_TYPE_DSP_CFG:
		return aw_dev_parse_raw_dsp_cfg(aw_dev,
				(uint8_t *)cfg_hdr + cfg_dde->data_offset,
				cfg_dde->data_size, scene_prof_desc);
	case ACF_SEC_TYPE_DSP_FW:
		return aw_dev_parse_raw_dsp_fw(aw_dev,
				(uint8_t *)cfg_hdr + cfg_dde->data_offset,
				cfg_dde->data_size, scene_prof_desc);
	case ACF_SEC_TYPE_MUTLBIN:
		return aw_dev_prof_parse_multi_bin(aw_dev,
				(uint8_t *)cfg_hdr + cfg_dde->data_offset,
				cfg_dde->data_size, scene_prof_desc);
	case ACF_SEC_TYPE_MONITOR:
		return aw_monitor_parse_fw(&aw_dev->monitor_desc,
				(uint8_t *)cfg_hdr + cfg_dde->data_offset,
				cfg_dde->data_size);
	}
	return 0;
}

static int aw_dev_parse_dev_type(struct aw_device *aw_dev,
		struct aw_cfg_hdr *prof_hdr, struct aw_all_prof_info *all_prof_info)
{
	int i = 0;
	int ret;
	int sec_num = 0;
	struct aw_cfg_dde *cfg_dde =
		(struct aw_cfg_dde *)((char *)prof_hdr + prof_hdr->a_hdr_offset);

	aw_dev_info(aw_dev->dev, "enter");

	for (i = 0; i < prof_hdr->a_ddt_num; i++) {
		if ((aw_dev->i2c->adapter->nr == cfg_dde[i].dev_bus) &&
			(aw_dev->i2c->addr == cfg_dde[i].dev_addr) &&
			(cfg_dde[i].type == AW_DEV_TYPE_ID)) {

			ret = aw_dev_parse_data_by_sec_type(aw_dev, prof_hdr, &cfg_dde[i],
					&all_prof_info->prof_desc[cfg_dde[i].dev_profile]);
			if (ret < 0) {
				aw_dev_err(aw_dev->dev, "parse failed");
				return ret;
			}
			sec_num++;
		}
	}

	if (sec_num == 0) {
		aw_dev_info(aw_dev->dev, "get dev type num is %d, please use default",
					sec_num);
		return AW_DEV_TYPE_NONE;
	}

	return AW_DEV_TYPE_OK;
}

static int aw_dev_parse_dev_default_type(struct aw_device *aw_dev,
		struct aw_cfg_hdr *prof_hdr, struct aw_all_prof_info *all_prof_info)
{
	int i = 0;
	int ret;
	int sec_num = 0;
	struct aw_cfg_dde *cfg_dde =
		(struct aw_cfg_dde *)((char *)prof_hdr + prof_hdr->a_hdr_offset);

	aw_dev_info(aw_dev->dev, "enter");

	for (i = 0; i < prof_hdr->a_ddt_num; i++) {
		if ((aw_dev->channel == cfg_dde[i].dev_index) &&
			(cfg_dde[i].type == AW_DEV_DEFAULT_TYPE_ID)) {

			ret = aw_dev_parse_data_by_sec_type(aw_dev, prof_hdr, &cfg_dde[i],
					&all_prof_info->prof_desc[cfg_dde[i].dev_profile]);
			if (ret < 0) {
				aw_dev_err(aw_dev->dev, "parse failed");
				return ret;
			}
			sec_num++;
		}
	}

	if (sec_num == 0) {
		aw_dev_err(aw_dev->dev, "get dev default type failed, get num[%d]", sec_num);
		return -EINVAL;
	}

	return 0;
}


static int aw_dev_load_cfg_by_hdr(struct aw_device *aw_dev,
		struct aw_cfg_hdr *prof_hdr, struct aw_all_prof_info *all_prof_info)
{
	int ret;

	ret = aw_dev_parse_dev_type(aw_dev, prof_hdr, all_prof_info);
	if (ret < 0) {
		return ret;
	} else if (ret == AW_DEV_TYPE_NONE) {
		aw_dev_info(aw_dev->dev, "get dev type num is 0, parse default dev");
		ret = aw_dev_parse_dev_default_type(aw_dev, prof_hdr, all_prof_info);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int aw_dev_cfg_get_vaild_prof(struct aw_device *aw_dev,
				struct aw_all_prof_info all_prof_info)
{
	int i;
	int num = 0;
	struct aw_sec_data_desc *sec_desc = NULL;
	struct aw_prof_desc *prof_desc = all_prof_info.prof_desc;
	struct aw_prof_info *prof_info = &aw_dev->prof_info;

	for (i = 0; i < AW_PROFILE_MAX; i++) {
		if (prof_desc[i].prof_st == AW_PROFILE_OK) {
			sec_desc = prof_desc[i].sec_desc;
			if ((sec_desc[AW_DATA_TYPE_REG].data != NULL) &&
				(sec_desc[AW_DATA_TYPE_REG].len != 0) &&
				(sec_desc[AW_DATA_TYPE_DSP_CFG].data != NULL) &&
				(sec_desc[AW_DATA_TYPE_DSP_CFG].len != 0) &&
				(sec_desc[AW_DATA_TYPE_DSP_FW].data != NULL) &&
				(sec_desc[AW_DATA_TYPE_DSP_FW].len != 0)) {
				prof_info->count++;
			}
		}
	}

	aw_dev_info(aw_dev->dev, "get vaild profile:%d", aw_dev->prof_info.count);

	if (!prof_info->count) {
		aw_dev_err(aw_dev->dev, "no profile data");
		return -EPERM;
	}

	prof_info->prof_desc = devm_kzalloc(aw_dev->dev,
					prof_info->count * sizeof(struct aw_prof_desc),
					GFP_KERNEL);
	if (prof_info->prof_desc == NULL) {
		aw_dev_err(aw_dev->dev, "prof_desc kzalloc failed");
		return -ENOMEM;
	}

	for (i = 0; i < AW_PROFILE_MAX; i++) {
		if (prof_desc[i].prof_st == AW_PROFILE_OK) {
			sec_desc = prof_desc[i].sec_desc;
			if ((sec_desc[AW_DATA_TYPE_REG].data != NULL) &&
				(sec_desc[AW_DATA_TYPE_REG].len != 0) &&
				(sec_desc[AW_DATA_TYPE_DSP_CFG].data != NULL) &&
				(sec_desc[AW_DATA_TYPE_DSP_CFG].len != 0) &&
				(sec_desc[AW_DATA_TYPE_DSP_FW].data != NULL) &&
				(sec_desc[AW_DATA_TYPE_DSP_FW].len != 0)) {
				if (num >= prof_info->count) {
					aw_dev_err(aw_dev->dev, "get scene num[%d] overflow count[%d]",
						num, prof_info->count);
					return -ENOMEM;
				}
				prof_info->prof_desc[num] = prof_desc[i];
				prof_info->prof_desc[num].id = i;
				num++;
			}
		}
	}

	return 0;
}

static int aw_dev_cfg_load(struct aw_device *aw_dev, struct aw_container *aw_cfg)
{
	struct aw_cfg_hdr *cfg_hdr = NULL;
	struct aw_all_prof_info all_prof_info;
	int ret;

	aw_dev_info(aw_dev->dev, "enter");
	memset(&all_prof_info, 0, sizeof(struct aw_all_prof_info));

	cfg_hdr = (struct aw_cfg_hdr *)aw_cfg->data;
	switch (cfg_hdr->a_hdr_version) {
	case AW_CFG_HDR_VER_0_0_0_1:
		ret = aw_dev_load_cfg_by_hdr(aw_dev, cfg_hdr, &all_prof_info);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "hdr_cersion[0x%x] parse failed",
						cfg_hdr->a_hdr_version);
			return ret;
		}
		break;
	default:
		aw_pr_err("unsupported hdr_version [0x%x]", cfg_hdr->a_hdr_version);
		return -EINVAL;
	}

	ret = aw_dev_cfg_get_vaild_prof(aw_dev, all_prof_info);
	if (ret < 0)
		return ret;

	aw_dev->fw_status = AW_DEV_FW_OK;
	aw_dev_info(aw_dev->dev, "parse cfg success");
	return 0;
}

static void aw_dev_fade_in(struct aw_device *aw_dev)
{
	int i = 0;
	struct aw_volume_desc *desc = &aw_dev->volume_desc;
	int fade_step = aw_dev->fade_step;

	if (!aw_dev->fade_en)
		return;

	if (fade_step == 0 || g_fade_in_time == 0) {
		aw_dev->ops.aw_set_volume(aw_dev, desc->init_volume);
		return;
	}
	/*volume up*/
	for (i = desc->mute_volume; i >= desc->init_volume; i -= fade_step) {
		aw_dev->ops.aw_set_volume(aw_dev, i);
		usleep_range(g_fade_in_time, g_fade_in_time + 10);
	}
	if (i != desc->init_volume)
		aw_dev->ops.aw_set_volume(aw_dev, desc->init_volume);


}

static void aw_dev_fade_out(struct aw_device *aw_dev)
{
	int i = 0;
	uint16_t start_volume = 0;
	struct aw_volume_desc *desc = &aw_dev->volume_desc;
	int fade_step = aw_dev->fade_step;

	if (!aw_dev->fade_en)
		return;

	if (fade_step == 0 || g_fade_out_time == 0) {
		aw_dev->ops.aw_set_volume(aw_dev, desc->mute_volume);
		return;
	}

	aw_dev->ops.aw_get_volume(aw_dev, &start_volume);
	i = start_volume;
	for (i = start_volume; i <= desc->mute_volume; i += fade_step) {
		aw_dev->ops.aw_set_volume(aw_dev, i);
		usleep_range(g_fade_out_time, g_fade_out_time + 10);
	}
	if (i != desc->mute_volume) {
		aw_dev->ops.aw_set_volume(aw_dev, desc->mute_volume);
		usleep_range(g_fade_out_time, g_fade_out_time + 10);
	}
}

int aw_dev_get_fade_vol_step(struct aw_device *aw_dev)
{
	return aw_dev->fade_step;
}

void aw_dev_set_fade_vol_step(struct aw_device *aw_dev, unsigned int step)
{
	aw_dev->fade_step = step;
}

void aw_dev_get_fade_time(unsigned int *time, bool fade_in)
{
	if (fade_in)
		*time = g_fade_in_time;
	else
		*time = g_fade_out_time;
}

void aw_dev_set_fade_time(unsigned int time, bool fade_in)
{
	if (fade_in)
		g_fade_in_time = time;
	else
		g_fade_out_time = time;
}

static uint64_t aw_dev_dsp_crc32_reflect(uint64_t ref, uint8_t ch)
{
	int i;
	uint64_t value = 0;

	for (i = 1; i < (ch + 1); i++) {
		if (ref & 1)
			value |= 1 << (ch - i);

		ref >>= 1;
	}

	return value;
}

static uint32_t aw_dev_calc_dsp_cfg_crc32(uint8_t *buf, uint32_t len)
{
	uint8_t i;
	uint32_t crc = 0xffffffff;

	while (len--) {
		for (i = 1; i != 0; i <<= 1) {
			if ((crc & 0x80000000) != 0) {
				crc <<= 1;
				crc ^= 0x1EDC6F41;
			} else {
				crc <<= 1;
			}

			if ((*buf & i) != 0)
				crc ^= 0x1EDC6F41;
		}
		buf++;
	}

	return (aw_dev_dsp_crc32_reflect(crc, 32)^0xffffffff);
}

static int aw_dev_set_dsp_crc32(struct aw_device *aw_dev)
{
	uint32_t crc_value;
	uint32_t crc_data_len = 0;
	int ret;
	struct aw_sec_data_desc *crc_dsp_cfg = &aw_dev->crc_dsp_cfg;
	struct aw_dsp_crc_desc *desc = &aw_dev->dsp_crc_desc;

	/*get crc data len*/
	crc_data_len = (desc->dsp_reg - aw_dev->dsp_mem_desc.dsp_cfg_base_addr) * 2;
	if (crc_data_len > crc_dsp_cfg->len) {
		aw_dev_err(aw_dev->dev, "crc data len :%d > cfg_data len:%d",
			crc_data_len, crc_dsp_cfg->len);
		return ret;
	}

	if (crc_data_len % 4 != 0) {
		aw_dev_err(aw_dev->dev, "The crc data len :%d unsupport", crc_data_len);
		return ret;
	}

	crc_value = aw_dev_calc_dsp_cfg_crc32(crc_dsp_cfg->data, crc_data_len);

	aw_dev_info(aw_dev->dev, "crc_value:0x%x", crc_value);
	ret = aw_dev->ops.aw_dsp_write(aw_dev, desc->dsp_reg, crc_value,
						desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "set dsp crc value failed");
		return ret;
	}

	return 0;
}

static int aw_dev_dsp_crc_check_enable(struct aw_device *aw_dev, bool flag)
{
	struct aw_dsp_crc_desc *dsp_crc_desc = &aw_dev->dsp_crc_desc;
	int ret;

	aw_dev_info(aw_dev->dev, "enter,flag:%d", flag);
	if (flag) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, dsp_crc_desc->ctl_reg,
				dsp_crc_desc->ctl_mask, dsp_crc_desc->ctl_enable);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "enable dsp crc failed");
			return ret;
		}
	} else {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, dsp_crc_desc->ctl_reg,
				dsp_crc_desc->ctl_mask, dsp_crc_desc->ctl_disable);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "close dsp crc failed");
			return ret;
		}
	}

	return 0;
}


static int aw_dev_dsp_st_check(struct aw_device *aw_dev)
{
	struct aw_sysst_desc *desc = &aw_dev->sysst_desc;
	int ret = -1;
	uint16_t reg_val = 0;
	int i;

	for (i = 0; i < AW_DSP_ST_CHECK_MAX; i++) {
		ret = aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "read reg0x%x failed", desc->reg);
			continue;
		}

		if ((reg_val & (~desc->dsp_mask)) != desc->dsp_check) {
			aw_dev_err(aw_dev->dev, "check dsp st fail,reg_val:0x%04x", reg_val);
			ret= -EINVAL;
			continue;
		} else {
			aw_dev_info(aw_dev->dev, "dsp st check ok, reg_val:0x%04x", reg_val);
			return 0;
		}
	}

	return ret;
}

static int aw_dev_dsp_crc32_check(struct aw_device *aw_dev)
{
	int ret;

	if (aw_dev->dsp_cfg == AW_DEV_DSP_BYPASS) {
		aw_dev_info(aw_dev->dev, "dsp bypass");
		return 0;
	}

	ret = aw_dev_set_dsp_crc32(aw_dev);
	if (ret < 0) {
		aw_dev_info(aw_dev->dev, "set dsp crc32 failed");
		return ret;
	}

	aw_dev_dsp_crc_check_enable(aw_dev, true);

	/*dsp enable*/
	aw_dev_dsp_enable(aw_dev, true);
	usleep_range(AW_5000_US, AW_5000_US + 100);

	ret = aw_dev_dsp_st_check(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "check crc32 fail");
		return ret;
	}

	aw_dev_dsp_crc_check_enable(aw_dev, false);
	aw_dev->dsp_crc_st = AW_DSP_CRC_OK;
	return 0;
}

static void aw_dev_pwd(struct aw_device *aw_dev, bool pwd)
{
	struct aw_pwd_desc *pwd_desc = &aw_dev->pwd_desc;

	aw_dev_dbg(aw_dev->dev, "enter");

	if (pwd) {
		aw_dev->ops.aw_reg_write_bits(aw_dev, pwd_desc->reg,
				pwd_desc->mask,
				pwd_desc->enable);
	} else {
		aw_dev->ops.aw_reg_write_bits(aw_dev, pwd_desc->reg,
				pwd_desc->mask,
				pwd_desc->disable);
	}
	aw_dev_info(aw_dev->dev, "done");
}

static void aw_dev_amppd(struct aw_device *aw_dev, bool amppd)
{
	struct aw_amppd_desc *amppd_desc = &aw_dev->amppd_desc;

	aw_dev_dbg(aw_dev->dev, "enter");
	if (amppd) {
		aw_dev->ops.aw_reg_write_bits(aw_dev, amppd_desc->reg,
				amppd_desc->mask,
				amppd_desc->enable);
	} else {
		aw_dev->ops.aw_reg_write_bits(aw_dev, amppd_desc->reg,
				amppd_desc->mask,
				amppd_desc->disable);
	}
	aw_dev_info(aw_dev->dev, "done");
}


void aw_dev_mute(struct aw_device *aw_dev, bool mute)
{
	struct aw_mute_desc *mute_desc = &aw_dev->mute_desc;

	aw_dev_dbg(aw_dev->dev, "enter");
	if (mute) {
		aw_dev_fade_out(aw_dev);
		aw_dev->ops.aw_reg_write_bits(aw_dev, mute_desc->reg,
				mute_desc->mask, mute_desc->enable);
	} else {
		aw_dev->ops.aw_reg_write_bits(aw_dev, mute_desc->reg,
				mute_desc->mask, mute_desc->disable);
		aw_dev_fade_in(aw_dev);
	}
	aw_dev_info(aw_dev->dev, "done");
}

int aw_dev_get_hmute(struct aw_device *aw_dev)
{
	uint16_t reg_val = 0;
	int ret;
	struct aw_mute_desc *desc = &aw_dev->mute_desc;

	aw_dev_dbg(aw_dev->dev, "enter");

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
	if (ret < 0)
		return ret;

	if (reg_val & (~desc->mask))
		ret = 1;
	else
		ret = 0;

	return ret;
}

static int aw_dev_get_icalk(struct aw_device *aw_dev, int16_t *icalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t reg_icalk = 0;
	struct aw_vcalb_desc *desc = &aw_dev->vcalb_desc;

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->icalk_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "reg read failed");
		return ret;
	}

	reg_icalk = reg_val & (~desc->icalk_reg_mask);

	if (reg_icalk & (~desc->icalk_sign_mask))
		reg_icalk = reg_icalk | desc->icalk_neg_mask;

	*icalk = (int16_t)reg_icalk;

	return 0;
}

static int aw_dev_get_vcalk(struct aw_device *aw_dev, int16_t *vcalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t reg_vcalk = 0;
	struct aw_vcalb_desc *desc = &aw_dev->vcalb_desc;

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->vcalk_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "reg read failed");
		return ret;
	}

	reg_val = reg_val >> desc->vcalk_shift;

	reg_vcalk = (uint16_t)reg_val & (~desc->vcalk_reg_mask);

	if (reg_vcalk & (~desc->vcalk_sign_mask))
		reg_vcalk = reg_vcalk | desc->vcalk_neg_mask;

	*vcalk = (int16_t)reg_vcalk;

	return 0;
}

static int aw_dev_get_vcalk_dac(struct aw_device *aw_dev, int16_t *vcalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t reg_vcalk = 0;
	struct aw_vcalb_desc *desc = &aw_dev->vcalb_desc;

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->icalk_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "reg read failed");
		return ret;
	}

	reg_vcalk = reg_val >> desc->vcalk_dac_shift;

	if (reg_vcalk & desc->vcalk_dac_mask)
		reg_vcalk = reg_vcalk | desc->vcalk_dac_neg_mask;

	*vcalk = (int16_t)reg_vcalk;

	return 0;
}

int aw_dev_modify_dsp_cfg(struct aw_device *aw_dev,
			unsigned int addr, uint32_t dsp_data, unsigned char data_type)
{
	uint32_t addr_offset = 0;
	int len = 0;
	uint8_t temp_data[4] = { 0 };
	struct aw_sec_data_desc *crc_dsp_cfg = &aw_dev->crc_dsp_cfg;

	aw_dev_dbg(aw_dev->dev, "addr:0x%x, dsp_data:0x%x", addr, dsp_data);
	if (data_type == AW_DSP_16_DATA) {
		temp_data[0] = (uint8_t)(dsp_data & 0x00ff);
		temp_data[1] = (uint8_t)((dsp_data & 0xff00) >> 8);
		len = 2;
	} else if (data_type == AW_DSP_32_DATA) {
		temp_data[0] = (uint8_t)(dsp_data & 0x000000ff);
		temp_data[1] = (uint8_t)((dsp_data & 0x0000ff00) >> 8);
		temp_data[2] = (uint8_t)((dsp_data & 0x00ff0000) >> 16);
		temp_data[3] = (uint8_t)((dsp_data & 0xff000000) >> 24);
		len = 4;
	} else {
		aw_dev_err(aw_dev->dev, "data type[%d] unsupported", data_type);
		return -EINVAL;
	}

	addr_offset = (addr - aw_dev->dsp_mem_desc.dsp_cfg_base_addr) * 2;
	if (addr_offset > crc_dsp_cfg->len) {
		aw_dev_err(aw_dev->dev, "addr_offset[%d] > crc_dsp_cfg->len[%d]",
				addr_offset, crc_dsp_cfg->len);
		return -EINVAL;
	}

	memcpy(crc_dsp_cfg->data + addr_offset, temp_data, len);
	return 0;
}

static int aw_dev_vsense_select(struct aw_device *aw_dev, int *vsense_select)
{
	int ret = -1;
	struct aw_vcalb_desc *desc = &aw_dev->vcalb_desc;
	uint16_t vsense_reg_val;

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->vcalb_vsense_reg, &vsense_reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read vsense_reg_val failed");
		return ret;
	}
	aw_dev_dbg(aw_dev->dev, "vsense_reg = 0x%x", vsense_reg_val);

	if (vsense_reg_val & (~desc->vcalk_vdsel_mask)) {
		*vsense_select = AW_DEV_VDSEL_VSENSE;
		aw_dev_info(aw_dev->dev, "vsense outside");
		return 0;
	}

	*vsense_select = AW_DEV_VDSEL_DAC;
	aw_dev_info(aw_dev->dev, "vsense inside");
	return 0;
}

static int aw_dev_set_vcalb(struct aw_device *aw_dev)
{
	int ret = -1;
	uint32_t reg_val = 0;
	int vcalb;
	int icalk;
	int vcalk;
	int16_t icalk_val = 0;
	int16_t vcalk_val = 0;
	struct aw_vcalb_desc *desc = &aw_dev->vcalb_desc;
	uint32_t vcalb_adj;
	int vsense_select = -1;

	ret = aw_dev->ops.aw_dsp_read(aw_dev, desc->vcalb_dsp_reg, &vcalb_adj, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read vcalb_adj failed");
		return ret;
	}

	ret = aw_dev_vsense_select(aw_dev, &vsense_select);
	aw_dev_dbg(aw_dev->dev, "vsense_select = %d", vsense_select);
	if (ret < 0) {
		return ret;
	}

	ret = aw_dev_get_icalk(aw_dev, &icalk_val);
	icalk = desc->cabl_base_value + desc->icalk_value_factor * icalk_val;

	if (vsense_select == AW_DEV_VDSEL_VSENSE) {
		ret = aw_dev_get_vcalk(aw_dev, &vcalk_val);
		vcalk = desc->cabl_base_value + desc->vcalk_value_factor * vcalk_val;
		vcalb = desc->vcal_factor * desc->vscal_factor /
			desc->iscal_factor * icalk / vcalk * vcalb_adj;

		aw_dev_dbg(aw_dev->dev, "vcalk_factor=%d, vscal_factor=%d, icalk=%d, vcalk=%d",
				desc->vcalk_value_factor, desc->vscal_factor, icalk, vcalk);
	} else if (vsense_select == AW_DEV_VDSEL_DAC) {
		ret = aw_dev_get_vcalk_dac(aw_dev, &vcalk_val);
		vcalk = desc->cabl_base_value + desc->vcalk_value_factor_vsense_in * vcalk_val;
		vcalb = desc->vcal_factor * desc->vscal_factor_vsense_in /
			desc->iscal_factor * icalk / vcalk * vcalb_adj;

		aw_dev_dbg(aw_dev->dev, "vcalk_dac_factor=%d, vscal_dac_factor=%d, icalk=%d, vcalk=%d",
				desc->vcalk_value_factor_vsense_in, desc->vscal_factor_vsense_in, icalk, vcalk);
	} else {
		aw_dev_err(aw_dev->dev, "unsupport vsense status");
		return -EINVAL;
	}

	if ((vcalk == 0) || (desc->iscal_factor == 0)) {
		aw_dev_err(aw_dev->dev, "vcalk:%d or desc->iscal_factor:%d unsupported",
			vcalk, desc->iscal_factor);
		return -EINVAL;
	}

	vcalb = vcalb >> aw_dev->vcalb_desc.vcalb_adj_shift;
	reg_val = (uint32_t)vcalb;

	aw_dev_dbg(aw_dev->dev, "vcalb=%d, reg_val=0x%x, vcalb_adj =0x%x", vcalb, reg_val, vcalb_adj);

	ret = aw_dev->ops.aw_dsp_write(aw_dev, desc->vcalb_dsp_reg, reg_val, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write vcalb failed");
		return ret;
	}

	ret = aw_dev_modify_dsp_cfg(aw_dev, desc->vcalb_dsp_reg,
					(uint32_t)reg_val, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "modify dsp cfg failed");
		return ret;
	}

	aw_dev_info(aw_dev->dev, "done");

	return 0;
}

int aw_dev_get_int_status(struct aw_device *aw_dev, uint16_t *int_status)
{
	int ret = -1;
	uint16_t reg_val = 0;

	ret = aw_dev->ops.aw_reg_read(aw_dev, aw_dev->int_desc.st_reg, &reg_val);
	if (ret < 0)
		aw_dev_err(aw_dev->dev, "read interrupt reg fail, ret=%d", ret);
	else
		*int_status = reg_val;

	aw_dev_dbg(aw_dev->dev, "read interrupt reg = 0x%04x", *int_status);
	return ret;
}

void aw_dev_clear_int_status(struct aw_device *aw_dev)
{
	uint16_t int_status = 0;

	/*read int status and clear*/
	aw_dev_get_int_status(aw_dev, &int_status);
	/*make sure int status is clear*/
	aw_dev_get_int_status(aw_dev, &int_status);
	aw_dev_info(aw_dev->dev, "done");
}


static int aw_dev_get_iis_status(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t reg_val = 0;
	struct aw_sysst_desc *desc = &aw_dev->sysst_desc;

	aw_dev_dbg(aw_dev->dev, "enter");

	aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
	if ((reg_val & desc->pll_check) == desc->pll_check)
		ret = 0;
	else
		aw_dev_err(aw_dev->dev, "check pll lock fail,reg_val:0x%04x", reg_val);

	return ret;
}


static int aw_dev_mode1_pll_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t i = 0;

	for (i = 0; i < AW_DEV_SYSST_CHECK_MAX; i++) {
		ret = aw_dev_get_iis_status(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "mode1 iis signal check error");
			usleep_range(AW_2000_US, AW_2000_US + 10);
		} else {
			return 0;
		}
	}

	return ret;
}

static int aw_dev_mode2_pll_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t i = 0;
	uint16_t reg_val = 0;
	struct aw_cco_mux_desc *cco_mux_desc = &aw_dev->cco_mux_desc;

	aw_dev->ops.aw_reg_read(aw_dev, cco_mux_desc->reg, &reg_val);
	reg_val &= (~cco_mux_desc->mask);
	if (reg_val == cco_mux_desc->divider) {
		aw_dev_dbg(aw_dev->dev, "CCO_MUX is already divider");
		return ret;
	}

	/* change mode2 */
	aw_dev->ops.aw_reg_write_bits(aw_dev, cco_mux_desc->reg,
		cco_mux_desc->mask, cco_mux_desc->divider);

	for (i = 0; i < AW_DEV_SYSST_CHECK_MAX; i++) {
		ret = aw_dev_get_iis_status(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "mode2 iis signal check error");
			usleep_range(AW_2000_US, AW_2000_US + 10);
		} else {
			break;
		}
	}

	/* change mode1*/
	aw_dev->ops.aw_reg_write_bits(aw_dev, cco_mux_desc->reg,
		cco_mux_desc->mask, cco_mux_desc->bypass);

	if (ret == 0) {
		usleep_range(AW_2000_US, AW_2000_US + 10);
		for (i = 0; i < AW_DEV_SYSST_CHECK_MAX; i++) {
			ret = aw_dev_mode1_pll_check(aw_dev);
			if (ret < 0) {
				aw_dev_err(aw_dev->dev, "mode2 switch to mode1, iis signal check error");
				usleep_range(AW_2000_US, AW_2000_US + 10);
			} else {
				break;
			}
		}
	}

	return ret;
}

int aw_dev_syspll_check(struct aw_device *aw_dev)
{
	int ret = -1;

	ret = aw_dev_mode1_pll_check(aw_dev);
	if (ret < 0) {
		aw_dev_info(aw_dev->dev, "mode1 check iis failed try switch to mode2 check");
		ret = aw_dev_mode2_pll_check(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "mode2 check iis failed");
			return ret;
		}
	}

	return ret;
}

int aw_dev_sysst_check(struct aw_device *aw_dev)
{
	int ret = -1;
	unsigned char i;
	uint16_t reg_val = 0;
	struct aw_sysst_desc *desc = &aw_dev->sysst_desc;

	for (i = 0; i < AW_DEV_SYSST_CHECK_MAX; i++) {
		aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
		if (((reg_val & (~desc->st_mask)) & desc->st_check) == desc->st_check) {
			ret = 0;
			break;
		} else {
			aw_dev_dbg(aw_dev->dev, "check fail, cnt=%d, reg_val=0x%04x",
				i, reg_val);
			usleep_range(AW_2000_US, AW_2000_US + 10);
		}
	}
	if (ret < 0)
		aw_dev_err(aw_dev->dev, "check fail");

	return ret;
}

static int aw_dev_get_monitor_sysint_st(struct aw_device *aw_dev)
{
	int ret = 0;
	struct aw_int_desc *desc = &aw_dev->int_desc;

	if ((desc->intst_mask) & (desc->sysint_st)) {
		aw_dev_err(aw_dev->dev,
			"monitor check fail:0x%04x", desc->sysint_st);
		ret = -EINVAL;
	}
	desc->sysint_st = 0;

	return ret;
}

static int aw_dev_sysint_check(struct aw_device *aw_dev)
{
	int ret = 0;
	uint16_t reg_val = 0;
	struct aw_int_desc *desc = &aw_dev->int_desc;

	aw_dev_get_int_status(aw_dev, &reg_val);

	if (reg_val & (desc->intst_mask)) {
		aw_dev_err(aw_dev->dev, "pa stop check fail:0x%04x", reg_val);
		ret = -EINVAL;
	}

	return ret;
}

int aw_dev_set_intmask(struct aw_device *aw_dev, bool flag)
{
	int ret = -1;
	struct aw_int_desc *desc = &aw_dev->int_desc;

	if (flag)
		ret = aw_dev->ops.aw_reg_write(aw_dev, desc->mask_reg,
					desc->int_mask);
	else
		ret = aw_dev->ops.aw_reg_write(aw_dev, desc->mask_reg,
					desc->mask_default);

	aw_dev_info(aw_dev->dev, "done");
	return ret;
}

void aw_dev_dsp_enable(struct aw_device *aw_dev, bool dsp)
{
	int ret = -1;
	struct aw_dsp_en_desc *desc = &aw_dev->dsp_en_desc;

	if (dsp) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, desc->reg,
					desc->mask, desc->enable);
		if (ret < 0)
			aw_dev_err(aw_dev->dev, "enable dsp failed");
	} else {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, desc->reg,
					desc->mask, desc->disable);
		if (ret < 0)
			aw_dev_err(aw_dev->dev, "disable dsp failed");
	}

	aw_dev_info(aw_dev->dev, "done");
}

static int aw_dev_get_dsp_config(struct aw_device *aw_dev, unsigned char *dsp_cfg)
{
	int ret = -1;
	uint16_t reg_val = 0;
	struct aw_dsp_en_desc *desc = &aw_dev->dsp_en_desc;

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "reg read failed");
		return ret;
	}

	if (reg_val & (~desc->mask))
		*dsp_cfg = AW_DEV_DSP_BYPASS;
	else
		*dsp_cfg = AW_DEV_DSP_WORK;

	aw_dev_info(aw_dev->dev, "done");

	return 0;
}

void aw_dev_memclk_select(struct aw_device *aw_dev, unsigned char flag)
{
	struct aw_memclk_desc *desc = &aw_dev->memclk_desc;
	int ret = -1;

	if (flag == AW_DEV_MEMCLK_PLL) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, desc->reg,
					desc->mask, desc->mcu_hclk);
		if (ret < 0)
			aw_dev_err(aw_dev->dev, "memclk select pll failed");

	} else if (flag == AW_DEV_MEMCLK_OSC) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, desc->reg,
					desc->mask, desc->osc_clk);
		if (ret < 0)
			aw_dev_err(aw_dev->dev, "memclk select OSC failed");
	} else {
		aw_dev_err(aw_dev->dev, "unknown memclk config, flag=0x%x", flag);
	}

	aw_dev_info(aw_dev->dev, "done");
}

int aw_dev_get_dsp_status(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t reg_val = 0;
	struct aw_watch_dog_desc *desc = &aw_dev->watch_dog_desc;

	aw_dev_info(aw_dev->dev, "enter");

	aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
	if (reg_val & (~desc->mask))
		ret = 0;

	return ret;
}

static int aw_dev_get_vmax(struct aw_device *aw_dev, unsigned int *vmax)
{
	int ret = -1;
	struct aw_vmax_desc *desc = &aw_dev->vmax_desc;

	ret = aw_dev->ops.aw_dsp_read(aw_dev, desc->dsp_reg, vmax, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "get vmax failed");
		return ret;
	}

	return 0;
}


/******************************************************
 *
 * aw_dev update cfg
 *
 ******************************************************/
int aw_dev_get_profile_count(struct aw_device *aw_dev)
{
	if (aw_dev == NULL) {
		aw_pr_err("aw_dev is NULL");
		return -ENOMEM;
	}

	return aw_dev->prof_info.count;
}

int aw_dev_check_profile_index(struct aw_device *aw_dev, int index)
{
	if ((index >= aw_dev->prof_info.count) || (index < 0))
		return -EINVAL;
	else
		return 0;
}

int aw_dev_get_profile_index(struct aw_device *aw_dev)
{
	return aw_dev->set_prof;
}

int aw_dev_set_profile_index(struct aw_device *aw_dev, int index)
{

	if ((index >= aw_dev->prof_info.count) || (index < 0)) {
		return -EINVAL;
	} else {
		aw_dev->set_prof = index;

		aw_dev_info(aw_dev->dev, "set prof[%s]",
			profile_name[aw_dev->prof_info.prof_desc[index].id]);
	}

	return 0;
}

char *aw_dev_get_prof_name(struct aw_device *aw_dev, int index)
{
	struct aw_prof_desc *prof_desc = NULL;

	if ((index >= aw_dev->prof_info.count) || (index < 0)) {
		aw_dev_err(aw_dev->dev, "index[%d] overflow count[%d]",
			index, aw_dev->prof_info.count);
		return NULL;
	}

	prof_desc = &aw_dev->prof_info.prof_desc[index];

	return profile_name[prof_desc->id];
}


int aw_dev_get_prof_data(struct aw_device *aw_dev, int index,
			struct aw_prof_desc **prof_desc)
{
	if ((index >= aw_dev->prof_info.count) || (index < 0)) {
		aw_dev_err(aw_dev->dev, "%s: index[%d] overflow count[%d]\n",
			__func__, index, aw_dev->prof_info.count);
		return -EINVAL;
	}

	*prof_desc = &aw_dev->prof_info.prof_desc[index];

	return 0;
}

static int aw_dev_reg_container_update(struct aw_device *aw_dev,
				uint8_t *data, uint32_t len)
{
	int i, ret;
	uint8_t reg_addr = 0;
	uint16_t reg_val = 0;
	uint16_t read_val;
	struct aw_int_desc *int_desc = &aw_dev->int_desc;
	int16_t *reg_data = NULL;
	int data_len;

	aw_dev_dbg(aw_dev->dev, "enter");
	reg_data = (int16_t *)data;
	data_len = len >> 1;

	if (data_len % 2 != 0) {
		aw_dev_err(aw_dev->dev, "data len:%d unsupported",
				data_len);
		return -EINVAL;
	}

	for (i = 0; i < data_len; i += 2) {
		reg_addr = reg_data[i];
		reg_val = reg_data[i + 1];
		aw_dev_dbg(aw_dev->dev, "reg = 0x%02x, val = 0x%04x",
				reg_addr, reg_val);
		if (reg_addr == int_desc->mask_reg) {
			int_desc->int_mask = reg_val;
			reg_val = int_desc->mask_default;
		}
		if (reg_addr == aw_dev->mute_desc.reg) {
			aw_dev->ops.aw_reg_read(aw_dev, reg_addr, &read_val);
			read_val &= (~aw_dev->mute_desc.mask);
			reg_val &= aw_dev->mute_desc.mask;
			reg_val |= read_val;
		}
		if (reg_addr == aw_dev->dsp_crc_desc.ctl_reg) {
			reg_val &= aw_dev->dsp_crc_desc.ctl_mask;
		}

		ret = aw_dev->ops.aw_reg_write(aw_dev, reg_addr, reg_val);
		if (ret < 0)
			break;
	}
	aw_dev->ops.aw_get_volume(aw_dev, (uint16_t *)&aw_dev->volume_desc.init_volume);

	/*keep min volume*/
	if (aw_dev->fade_en)
		aw_dev->ops.aw_set_volume(aw_dev, aw_dev->volume_desc.mute_volume);

	aw_dev_get_dsp_config(aw_dev, &aw_dev->dsp_cfg);

	aw_dev_dbg(aw_dev->dev, "exit");

	return ret;
}

static int aw_dev_reg_update(struct aw_device *aw_dev,
					uint8_t *data, uint32_t len)
{

	aw_dev_dbg(aw_dev->dev, "reg len:%d", len);

	if (len && (data != NULL)) {
		aw_dev_reg_container_update(aw_dev, data, len);
	} else {
		aw_dev_err(aw_dev->dev, "reg data is null or len is 0");
		return -EPERM;
	}

	return 0;
}

static int aw_dev_dsp_container_update(struct aw_device *aw_dev,
			uint8_t *data, uint32_t len, uint16_t base)
{
	int i;
	struct aw_dsp_mem_desc *dsp_mem_desc = &aw_dev->dsp_mem_desc;
#ifdef AW_DSP_I2C_WRITES
	uint32_t tmp_len = 0;
#else
	uint16_t reg_val = 0;
#endif

	aw_dev_dbg(aw_dev->dev, "enter");
	mutex_lock(aw_dev->i2c_lock);
#ifdef AW_DSP_I2C_WRITES
	/* i2c writes */
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg, base);

	for (i = 0; i < len; i += AW_MAX_RAM_WRITE_BYTE_SIZE) {
		if ((len - i) < AW_MAX_RAM_WRITE_BYTE_SIZE)
			tmp_len = len - i;
		else
			tmp_len = AW_MAX_RAM_WRITE_BYTE_SIZE;
		aw_dev->ops.aw_i2c_writes(aw_dev, dsp_mem_desc->dsp_mdat_reg,
					&data[i], tmp_len);
	}

#else
	/* i2c write */
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg, base);
	for (i = 0; i < len; i += 2) {
		reg_val = (data[i] << 8) + data[i + 1];
		aw_dev->ops.aw_i2c_writes(aw_dev, dsp_mem_desc->dsp_mdat_reg,
					reg_val);
	}
#endif
	mutex_unlock(aw_dev->i2c_lock);
	aw_dev_dbg(aw_dev->dev, "exit");

	return 0;
}

int aw_dev_dsp_fw_update(struct aw_device *aw_dev,
			uint8_t *data, uint32_t len)
{
	struct aw_dsp_mem_desc *dsp_mem_desc = &aw_dev->dsp_mem_desc;

	aw_dev_dbg(aw_dev->dev, "dsp firmware len:%d", len);

	if (len && (data != NULL)) {
		aw_dev_dsp_container_update(aw_dev,
			data, len, dsp_mem_desc->dsp_fw_base_addr);
		aw_dev->dsp_fw_len = len;
	} else {
		aw_dev_err(aw_dev->dev, "dsp firmware data is null or len is 0");
		return -EPERM;
	}

	return 0;
}

static int aw_dev_copy_to_crc_dsp_cfg(struct aw_device *aw_dev,
			uint8_t *data, uint32_t size)
{
	struct aw_sec_data_desc *crc_dsp_cfg = &aw_dev->crc_dsp_cfg;
	int ret;

	if (crc_dsp_cfg->data == NULL) {
		crc_dsp_cfg->data = devm_kzalloc(aw_dev->dev, size, GFP_KERNEL);
		if (!crc_dsp_cfg->data) {
			aw_dev_err(aw_dev->dev, "error allocating memory");
			return -ENOMEM;
		}
		crc_dsp_cfg->len = size;
	} else if (crc_dsp_cfg->len < size) {
		devm_kfree(aw_dev->dev, crc_dsp_cfg->data);
		crc_dsp_cfg->data = NULL;
		crc_dsp_cfg->data = devm_kzalloc(aw_dev->dev, size, GFP_KERNEL);
		if (!crc_dsp_cfg->data) {
			aw_dev_err(aw_dev->dev, "error allocating memory");
			return -ENOMEM;
		}
	}
	memcpy(crc_dsp_cfg->data, data ,size);
	ret = aw_dev_dsp_data_order(aw_dev, crc_dsp_cfg->data, size);
	if (ret < 0)
		return ret;

	return 0;
}


int aw_dev_dsp_cfg_update(struct aw_device *aw_dev,
			uint8_t *data, uint32_t len)
{
	struct aw_dsp_mem_desc *dsp_mem_desc = &aw_dev->dsp_mem_desc;
	int ret;

	aw_dev_dbg(aw_dev->dev, "dsp config len:%d", len);

	if (len && (data != NULL)) {
		aw_dev_dsp_container_update(aw_dev,
			data, len, dsp_mem_desc->dsp_cfg_base_addr);
		aw_dev->dsp_cfg_len = len;

		ret = aw_dev_copy_to_crc_dsp_cfg(aw_dev, data, len);
		if (ret < 0)
			return ret;

		aw_dev_set_vcalb(aw_dev);
		aw_cali_svc_get_ra(&aw_dev->cali_desc);

		if (aw_dev->ops.aw_get_hw_mon_st) {
			ret = aw_dev->ops.aw_get_hw_mon_st(aw_dev,
					&aw_dev->monitor_desc.hw_mon_en);
			if (ret < 0)
				return ret;
		}

		ret = aw_dev_get_vmax(aw_dev, &aw_dev->vmax_desc.init_vmax);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "get vmax failed");
			return ret;
		} else {
			aw_dev_info(aw_dev->dev, "get init vmax:0x%x",
						aw_dev->vmax_desc.init_vmax);
		}

		aw_dev->dsp_crc_st = AW_DSP_CRC_NA;
	} else {
		aw_dev_err(aw_dev->dev, "dsp config data is null or len is 0");
		return -EPERM;
	}

	return 0;
}

static int aw_dev_sram_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t reg_val = 0;
	struct aw_dsp_mem_desc *dsp_mem_desc = &aw_dev->dsp_mem_desc;

	mutex_lock(aw_dev->i2c_lock);
	/*check the odd bits of reg 0x40*/
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg,
					AW_DSP_ODD_NUM_BIT_TEST);
	aw_dev->ops.aw_i2c_read(aw_dev, dsp_mem_desc->dsp_madd_reg, &reg_val);
	if (AW_DSP_ODD_NUM_BIT_TEST != reg_val) {
		aw_dev_err(aw_dev->dev, "check reg 0x40 odd bit failed, read[0x%x] does not match write[0x%x]",
				reg_val, AW_DSP_ODD_NUM_BIT_TEST);
		goto error;
	}

	/*check the even bits of reg 0x40*/
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg,
					AW_DSP_EVEN_NUM_BIT_TEST);
	aw_dev->ops.aw_i2c_read(aw_dev, dsp_mem_desc->dsp_madd_reg, &reg_val);
	if (AW_DSP_EVEN_NUM_BIT_TEST != reg_val) {
		aw_dev_err(aw_dev->dev, "check reg 0x40 even bit failed, read[0x%x] does not match write[0x%x]",
				reg_val, AW_DSP_EVEN_NUM_BIT_TEST);
		goto error;
	}

	/*check dsp_fw_base_addr*/
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg,
					dsp_mem_desc->dsp_fw_base_addr);
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_mdat_reg,
					AW_DSP_EVEN_NUM_BIT_TEST);

	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg,
					dsp_mem_desc->dsp_fw_base_addr);
	aw_dev->ops.aw_i2c_read(aw_dev, dsp_mem_desc->dsp_mdat_reg, &reg_val);
	if (AW_DSP_EVEN_NUM_BIT_TEST != reg_val) {
		aw_dev_err(aw_dev->dev, "check dsp fw addr failed, read[0x%x] does not match write[0x%x]",
						reg_val, AW_DSP_EVEN_NUM_BIT_TEST);
		goto error;
	}

	/*check dsp_cfg_base_addr*/
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg,
					dsp_mem_desc->dsp_cfg_base_addr);
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_mdat_reg,
					AW_DSP_ODD_NUM_BIT_TEST);

	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg,
					dsp_mem_desc->dsp_cfg_base_addr);
	aw_dev->ops.aw_i2c_read(aw_dev, dsp_mem_desc->dsp_mdat_reg, &reg_val);
	if (AW_DSP_ODD_NUM_BIT_TEST != reg_val) {
		aw_dev_err(aw_dev->dev, "check dsp cfg failed, read[0x%x] does not match write[0x%x]",
						reg_val, AW_DSP_ODD_NUM_BIT_TEST);
		goto error;
	}

	mutex_unlock(aw_dev->i2c_lock);
	return 0;

error:
	mutex_unlock(aw_dev->i2c_lock);
	return ret;
}

int aw_dev_fw_update(struct aw_device *aw_dev, bool up_dsp_fw_en, bool force_up_en)
{
	int ret = -1;
	struct aw_prof_desc *set_prof_desc = NULL;
	struct aw_sec_data_desc *sec_desc = NULL;
	char *prof_name = NULL;

	if ((aw_dev->cur_prof == aw_dev->set_prof) &&
			(force_up_en == AW_FORCE_UPDATE_OFF)) {
		aw_dev_info(aw_dev->dev, "scene no change, not update");
		return 0;
	}

	if (aw_dev->fw_status == AW_DEV_FW_FAILED) {
		aw_dev_err(aw_dev->dev, "fw status[%d] error", aw_dev->fw_status);
		return -EPERM;
	}

	prof_name = aw_dev_get_prof_name(aw_dev, aw_dev->set_prof);
	if (prof_name == NULL)
		return -ENOMEM;

	aw_dev_info(aw_dev->dev, "start update %s", prof_name);

	ret = aw_dev_get_prof_data(aw_dev, aw_dev->set_prof, &set_prof_desc);
	if (ret < 0)
		return ret;

	/*update reg*/
	sec_desc = set_prof_desc->sec_desc;
	ret = aw_dev_reg_update(aw_dev, sec_desc[AW_DATA_TYPE_REG].data,
					sec_desc[AW_DATA_TYPE_REG].len);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "update reg failed");
		return ret;
	}

	aw_dev_mute(aw_dev, true);

	if (aw_dev->dsp_cfg == AW_DEV_DSP_WORK)
		aw_dev_dsp_enable(aw_dev, false);

	aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_OSC);

	if (up_dsp_fw_en) {
		ret = aw_dev_sram_check(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "check sram failed");
			goto error;
		}

		/*update dsp firmware*/
		ret = aw_dev_dsp_fw_update(aw_dev, sec_desc[AW_DATA_TYPE_DSP_FW].data,
					sec_desc[AW_DATA_TYPE_DSP_FW].len);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "update dsp fw failed");
			goto error;
		}
	}

	/*update dsp config*/
	ret = aw_dev_dsp_cfg_update(aw_dev, sec_desc[AW_DATA_TYPE_DSP_CFG].data,
					sec_desc[AW_DATA_TYPE_DSP_CFG].len);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "update dsp cfg failed");
		goto error;
	}

	aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_PLL);

	aw_dev->cur_prof = aw_dev->set_prof;

	aw_dev_info(aw_dev->dev, "load %s done", prof_name);
	return 0;

error:
	aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_PLL);

	return ret;
}

int aw_dev_dsp_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t i = 0;

	aw_dev_dbg(aw_dev->dev, "enter");

	if (aw_dev->dsp_cfg == AW_DEV_DSP_BYPASS) {
		aw_dev_dbg(aw_dev->dev, "dsp bypass");
		return 0;
	} else if (aw_dev->dsp_cfg == AW_DEV_DSP_WORK) {
		for (i = 0; i < AW_DEV_DSP_CHECK_MAX; i++) {
			aw_dev_dsp_enable(aw_dev, false);
			aw_dev_dsp_enable(aw_dev, true);
			usleep_range(AW_1000_US, AW_1000_US + 10);
			ret = aw_dev_get_dsp_status(aw_dev);
			if (ret < 0) {
				aw_dev_err(aw_dev->dev, "dsp wdt status error=%d", ret);
				usleep_range(AW_2000_US, AW_2000_US + 10);
			} else {
				return 0;
			}
		}
	} else {
		aw_dev_err(aw_dev->dev, "unknown dsp cfg=%d", aw_dev->dsp_cfg);
		return -EINVAL;
	}

	return -EINVAL;
}

static int aw_dev_set_cfg_f0_fs(struct aw_device *aw_dev)
{
	uint32_t f0_fs = 0;
	struct aw_cfgf0_fs_desc *cfgf0_fs_desc = &aw_dev->cfgf0_fs_desc;
	int ret;

	if (aw_dev->ops.aw_set_cfg_f0_fs) {
		aw_dev->ops.aw_set_cfg_f0_fs(aw_dev, &f0_fs);
		ret = aw_dev_modify_dsp_cfg(aw_dev, cfgf0_fs_desc->dsp_reg,
					f0_fs, cfgf0_fs_desc->data_type);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "modify dsp cfg failed");
			return ret;
		}
	}

	return 0;
}

static void aw_dev_cali_re_update(struct aw_cali_desc *cali_desc)
{
	struct aw_device *aw_dev =
		container_of(cali_desc, struct aw_device, cali_desc);

	if (AW_ERRO_CALI_RE_VALUE != aw_dev->cali_desc.cali_re) {
		aw_cali_svc_set_cali_re_to_dsp(&aw_dev->cali_desc);
	} else {
		aw_dev_err(aw_dev->dev, "cali_re:%d is error, no set",
				aw_dev->cali_desc.cali_re);
	}
}


int aw_device_start(struct aw_device *aw_dev)
{
	int ret = -1;

	aw_dev_info(aw_dev->dev, "enter");

	if (aw_dev->status == AW_DEV_PW_ON) {
		aw_dev_info(aw_dev->dev, "already power on");
		return 0;
	}

	/*power on*/
	aw_dev_pwd(aw_dev, false);
	usleep_range(AW_2000_US, AW_2000_US + 10);

	ret = aw_dev_syspll_check(aw_dev);
	if (ret < 0) {
		aw_dev_dbg(aw_dev->dev, "pll check failed cannot start");
		aw_dev_reg_dump(aw_dev);
		goto pll_check_fail;
	}

	/*amppd on*/
	aw_dev_amppd(aw_dev, false);
	usleep_range(AW_1000_US, AW_1000_US + 50);

	/*check i2s status*/
	ret = aw_dev_sysst_check(aw_dev);
	if (ret < 0) {
		/*check failed*/
		aw_dev_reg_dump(aw_dev);
		goto sysst_check_fail;
	}

	/*dsp bypass*/
	aw_dev_dsp_enable(aw_dev, false);
	if (aw_dev->ops.aw_dsp_fw_check) {
		ret = aw_dev->ops.aw_dsp_fw_check(aw_dev);
		if (ret < 0) {
			aw_dev_reg_dump(aw_dev);
			goto dsp_fw_check_fail;
		}
	}
	aw_dev_set_cfg_f0_fs(aw_dev);

	aw_dev_cali_re_update(&aw_dev->cali_desc);

	if (aw_dev->dsp_crc_st != AW_DSP_CRC_OK) {
		ret = aw_dev_dsp_crc32_check(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "dsp crc check failed");
			aw_dev_reg_dump(aw_dev);
			goto crc_check_fail;
		}
	}

	ret = aw_dev_dsp_check(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "check dsp status failed");
		aw_dev_reg_dump(aw_dev);
		goto dsp_check_fail;
	}

	/*enable tx feedback*/
	if (aw_dev->ops.aw_i2s_tx_enable)
		aw_dev->ops.aw_i2s_tx_enable(aw_dev, true);

	/*close mute*/
	aw_dev_mute(aw_dev, false);

	/*clear inturrupt*/
	aw_dev_clear_int_status(aw_dev);
	/*set inturrupt mask*/
	aw_dev_set_intmask(aw_dev, true);

	aw_monitor_start(&aw_dev->monitor_desc);

	aw_dev->status = AW_DEV_PW_ON;

	aw_dev_info(aw_dev->dev, "done");

	return 0;

dsp_check_fail:
crc_check_fail:
	aw_dev_dsp_enable(aw_dev, false);
dsp_fw_check_fail:
sysst_check_fail:
	/*clear interrupt*/
	aw_dev_clear_int_status(aw_dev);
	aw_dev_amppd(aw_dev, true);
pll_check_fail:
	aw_dev_pwd(aw_dev, true);
	aw_dev->status = AW_DEV_PW_OFF;
	return ret;
}

int aw_device_stop(struct aw_device *aw_dev)
{
	struct aw_sec_data_desc *dsp_cfg =
		&aw_dev->prof_info.prof_desc[aw_dev->cur_prof].sec_desc[AW_DATA_TYPE_DSP_CFG];
	struct aw_sec_data_desc *dsp_fw =
		&aw_dev->prof_info.prof_desc[aw_dev->cur_prof].sec_desc[AW_DATA_TYPE_DSP_FW];
	int int_st = 0;
	int monitor_int_st = 0;

	aw_dev_dbg(aw_dev->dev, "enter");

	if (aw_dev->status == AW_DEV_PW_OFF) {
		aw_dev_info(aw_dev->dev, "already power off");
		return 0;
	}

	aw_dev->status = AW_DEV_PW_OFF;

	aw_monitor_stop(&aw_dev->monitor_desc);

	/*set mute*/
	aw_dev_mute(aw_dev, true);
	usleep_range(AW_4000_US, AW_4000_US + 100);

	/*close tx feedback*/
	if (aw_dev->ops.aw_i2s_tx_enable)
		aw_dev->ops.aw_i2s_tx_enable(aw_dev, false);
	usleep_range(AW_1000_US, AW_1000_US + 100);

	/*set defaut int mask*/
	aw_dev_set_intmask(aw_dev, false);

	/*check sysint state*/
	int_st = aw_dev_sysint_check(aw_dev);

	/*close dsp*/
	aw_dev_dsp_enable(aw_dev, false);

	/*enable amppd*/
	aw_dev_amppd(aw_dev, true);

	/*check monitor process sysint state*/
	monitor_int_st = aw_dev_get_monitor_sysint_st(aw_dev);

	if (int_st < 0 || monitor_int_st < 0) {
		/*system status anomaly*/
		aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_OSC);
		aw_dev_dsp_cfg_update(aw_dev, dsp_cfg->data, dsp_cfg->len);
		aw_dev_dsp_fw_update(aw_dev, dsp_fw->data, dsp_fw->len);
		aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_PLL);
	}

	/*set power down*/
	aw_dev_pwd(aw_dev, true);

	aw_dev_info(aw_dev->dev, "done");
	return 0;
}

/*deinit aw_device*/
void aw_dev_deinit(struct aw_device *aw_dev)
{
	if (aw_dev == NULL)
		return;

	if (aw_dev->prof_info.prof_desc != NULL) {
		devm_kfree(aw_dev->dev, aw_dev->prof_info.prof_desc);
		aw_dev->prof_info.prof_desc = NULL;
	}
	aw_dev->prof_info.count = 0;

	if (aw_dev->crc_dsp_cfg.data != NULL) {
		aw_dev->crc_dsp_cfg.len = 0;
		devm_kfree(aw_dev->dev, aw_dev->crc_dsp_cfg.data);
		aw_dev->crc_dsp_cfg.data = NULL;
	}

}

/*init aw_device*/
int aw_device_init(struct aw_device *aw_dev, struct aw_container *aw_cfg)
{
	int ret;

	if (aw_dev == NULL || aw_cfg == NULL) {
		aw_pr_err("aw_dev is NULL or aw_cfg is NULL");
		return -ENOMEM;
	}

	ret = aw_dev_cfg_load(aw_dev, aw_cfg);
	if (ret < 0) {
		aw_dev_deinit(aw_dev);
		aw_dev_err(aw_dev->dev, "aw_dev acf parse failed");
		return -EINVAL;
	}

	aw_dev->cur_prof = aw_dev->prof_info.prof_desc[0].id;
	aw_dev->set_prof = aw_dev->prof_info.prof_desc[0].id;
	ret = aw_dev_fw_update(aw_dev, AW_FORCE_UPDATE_ON,
			AW_DSP_FW_UPDATE_ON);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "fw update failed");
		return ret;
	}

	aw_dev_set_intmask(aw_dev, false);

	/*set mute*/
	aw_dev_mute(aw_dev, true);

	/*close tx feedback*/
	if (aw_dev->ops.aw_i2s_tx_enable)
		aw_dev->ops.aw_i2s_tx_enable(aw_dev, false);
	usleep_range(AW_1000_US, AW_1000_US + 100);

	/*enable amppd*/
	aw_dev_amppd(aw_dev, true);
	/*close dsp*/
	aw_dev_dsp_enable(aw_dev, false);
	/*set power down*/
	aw_dev_pwd(aw_dev, true);

	mutex_lock(&g_dev_lock);
	list_add(&aw_dev->list_node, &g_dev_list);
	mutex_unlock(&g_dev_lock);

	aw_dev_info(aw_dev->dev, "init done");

	return 0;
}

static void aw883xx_parse_channel_dt(struct aw_device *aw_dev)
{
	int ret;
	uint32_t channel_value;
	struct device_node *np = aw_dev->dev->of_node;

	ret = of_property_read_u32(np, "sound-channel", &channel_value);
	if (ret < 0) {
		aw_dev_info(aw_dev->dev,
			"read sound-channel failed,use default 0");
		aw_dev->channel = AW_DEV_DEFAULT_CH;
		return;
	}

	aw_dev_dbg(aw_dev->dev, "read sound-channel value is: %d",
			channel_value);
	aw_dev->channel = channel_value;
}

static void aw883xx_parse_fade_enable_dt(struct aw_device *aw_dev)
{
	int ret = -1;
	struct device_node *np = aw_dev->dev->of_node;
	uint32_t fade_en;

	ret = of_property_read_u32(np, "fade-enable", &fade_en);
	if (ret < 0) {
		aw_dev_info(aw_dev->dev,
			"read fade-enable failed, close fade_in_out");
		fade_en = AW_FADE_IN_OUT_DEFAULT;
	} else {
		aw_dev_info(aw_dev->dev, "read fade-enable value is: %d", fade_en);
	}

	aw_dev->fade_en = fade_en;
}

static void aw883xx_parse_re_range_dt(struct aw_device *aw_dev)
{
	int ret;
	uint32_t re_max;
	uint32_t re_min;
	struct device_node *np = aw_dev->dev->of_node;

	ret = of_property_read_u32(np, "re-min", &re_min);
	if (ret < 0) {
		aw_dev_info(aw_dev->dev,
			"read re-min value failed, set deafult value:[%d]mohm",
			aw_dev->re_range.re_min);
	} else {
		aw_dev_info(aw_dev->dev,
			"parse re-min:[%d]", re_min);
		aw_dev->re_range.re_min = re_min;
	}

	ret = of_property_read_u32(np, "re-max", &re_max);
	if (ret < 0) {
		aw_dev_info(aw_dev->dev,
			"read re-max failed, set deafult value:[%d]mohm",
			aw_dev->re_range.re_max);
	} else {
		aw_dev_info(aw_dev->dev,
			"parse re-max:[%d]", re_max);
		aw_dev->re_range.re_max = re_max;
	}

}

static void aw_device_parse_dt(struct aw_device *aw_dev)
{
	aw883xx_parse_channel_dt(aw_dev);
	aw883xx_parse_fade_enable_dt(aw_dev);
	aw883xx_parse_re_range_dt(aw_dev);
}

int aw_dev_get_list_head(struct list_head **head)
{
	if (list_empty(&g_dev_list))
		return -EINVAL;

	*head = &g_dev_list;

	return 0;
}

int aw_device_probe(struct aw_device *aw_dev)
{
	INIT_LIST_HEAD(&aw_dev->list_node);

	aw_device_parse_dt(aw_dev);

	aw_cali_init(&aw_dev->cali_desc);

	aw_monitor_init(&aw_dev->monitor_desc);

	return 0;
}

int aw_device_remove(struct aw_device *aw_dev)
{
	aw_monitor_deinit(&aw_dev->monitor_desc);

	aw_cali_deinit(&aw_dev->cali_desc);

	return 0;
}

