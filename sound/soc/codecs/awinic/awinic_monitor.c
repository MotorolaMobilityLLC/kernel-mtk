/*
 * awinic_monitor.c monitor_module
 *
 *
 * Copyright (c) 2019 AWINIC Technology CO., LTD
 *
 *  Author: Nick Li <liweilei@awinic.com.cn>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
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
#include <linux/of.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include "aw882xx.h"
#include "aw882xx_reg.h"
#include "awinic_cali.h"
#include "awinic_monitor.h"
#include "awinic_dsp.h"


static DEFINE_MUTEX(g_aw_monitor_lock);
static LIST_HEAD(g_aw_monitor_list);
uint8_t g_monitor_dev_num;
struct aw_monitor_cfg g_monitor_cfg;
#define AW_MONITOR_FILE "aw882xx_monitor.bin"

extern unsigned char g_cali_status;

static int aw882xx_monitor_get_voltage(struct aw882xx *aw882xx,
						unsigned int *vol)
{
	int ret = -1;
	uint16_t local_vol = 0;

	ret = aw882xx_i2c_read(aw882xx, AW882XX_VBAT_REG, vol);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s: read voltage failed !\n",
			__func__);
		return ret;
	}
	local_vol = (*vol) * AW882XX_MONITOR_VBAT_RANGE / AW882XX_MONITOR_INT_10BIT;

	*vol = local_vol;
	aw_dev_dbg(aw882xx->dev, "%s: chip voltage is %d\n",
		__func__, local_vol);
	return 0;
}

static int aw882xx_monitor_get_temperature(struct aw882xx *aw882xx,  int *temp)
{
	int ret = -1;
	unsigned int reg_val = 0;
	uint16_t local_temp;

	ret = aw882xx_i2c_read(aw882xx, AW882XX_TEMP_REG, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s: get temperature failed!\n",
			__func__);
		return ret;
	}

	local_temp = reg_val;
	if (local_temp & AW882XX_MONITOR_TEMP_SIGN_MASK)
		local_temp = local_temp | AW882XX_MONITOR_TEMP_NEG_MASK;

	*temp = (int)local_temp;

	aw_dev_dbg(aw882xx->dev, "%s: chip temperature = %d\n",
		__func__, *temp);
	return 0;
}

static int aw882xx_monitor_get_temp_and_vol(struct aw882xx *aw882xx)
{
	struct aw882xx_monitor *monitor = &aw882xx->monitor;
	unsigned int voltage = 0;
	int current_temp = 0;
	int ret = -1;

#ifdef AW_DEBUG
	if (monitor->test_vol == 0) {
		ret = aw882xx_monitor_get_voltage(aw882xx, &voltage);
		if (ret < 0)
			return ret;
	} else {
		voltage = monitor->test_vol;
	}

	if (monitor->test_temp == 0) {
		ret = aw882xx_monitor_get_temperature(aw882xx, &current_temp);
		if (ret)
			return ret;
	} else {
		current_temp = monitor->test_temp;
	}
#else
	ret = aw882xx_monitor_get_voltage(aw882xx, &voltage);
	if (ret < 0)
		return ret;

	ret = aw882xx_monitor_get_temperature(aw882xx, &current_temp);
	if (ret < 0)
		return ret;
#endif

	monitor->vol_trace.sum_val += voltage;
	monitor->temp_trace.sum_val += current_temp;
	monitor->samp_count++;

	return 0;
}

static int aw882xx_monitor_trace_data_from_table(struct aw882xx *aw882xx,
			struct aw_table_info table_info,
			struct aw_monitor_trace *data_trace)
{
	int i;

	if (table_info.aw_table == NULL) {
		aw_dev_err(aw882xx->dev, "%s: table_info.aw_table is null\n",
			__func__);
		return -EINVAL;
	}

	for (i = 0; i < table_info.table_num; i++) {
		if (data_trace->sum_val >= table_info.aw_table[i].min_val &&
			data_trace->sum_val <= table_info.aw_table[i].max_val) {
			memcpy(&data_trace->aw_table, &table_info.aw_table[i],
				sizeof(struct aw_table));
			break;
		}
	}

	data_trace->pre_val = data_trace->sum_val;
	data_trace->sum_val = 0;
	return 0;
}

static int aw882xx_monitor_first_get_data_form_table(struct aw882xx *aw882xx,
				struct aw_table_info table_info,
			struct aw_monitor_trace *data_trace)
{
	int i;

	if (table_info.aw_table == NULL) {
		aw_dev_err(aw882xx->dev, "%s: table_info.aw_table is null\n",
			__func__);
		return -EINVAL;
	}

	for (i = 0; i < table_info.table_num; i++) {
		if (data_trace->sum_val >= table_info.aw_table[i].min_val) {
			memcpy(&data_trace->aw_table, &table_info.aw_table[i],
				sizeof(struct aw_table));
			break;
		}
	}
	data_trace->pre_val = data_trace->sum_val;
	data_trace->sum_val = 0;
	return 0;
}

static int aw882xx_monitor_get_data_from_table(struct aw882xx *aw882xx,
					struct aw_table_info table_info,
					struct aw_monitor_trace *data_trace,
					uint32_t aplha)
{
	struct aw882xx_monitor *monitor = &aw882xx->monitor;

	if (monitor->first_entry == AW_FIRST_ENTRY) {
		return aw882xx_monitor_first_get_data_form_table(aw882xx,
						table_info, data_trace);
	} else {
		data_trace->sum_val = data_trace->sum_val / monitor->samp_count;
		data_trace->sum_val = ((int32_t)aplha * data_trace->sum_val +
			(1000 - (int32_t)aplha) * data_trace->pre_val) / 1000;
		return aw882xx_monitor_trace_data_from_table(aw882xx,
						table_info, data_trace);
	}

	return 0;
}

static int aw882xx_monitor_get_data(struct aw882xx *aw882xx)
{
	struct aw882xx_monitor *monitor = &aw882xx->monitor;
	struct aw_monitor_cfg *monitor_cfg = monitor->monitor_cfg;
	struct aw_monitor_trace *vol_trace = &monitor->vol_trace;
	struct aw_monitor_trace *temp_trace = &monitor->temp_trace;
	int ret;

	if (monitor_cfg->vol_switch) {
		ret = aw882xx_monitor_get_data_from_table(aw882xx,
			monitor_cfg->vol_info, vol_trace,
			monitor_cfg->vol_aplha);
		if (ret < 0)
			return ret;
	} else {
		vol_trace->aw_table.ipeak = IPEAK_NONE;
		vol_trace->aw_table.gain = GAIN_NONE;
		vol_trace->aw_table.vmax = VMAX_NONE;
	}

	if (monitor_cfg->temp_switch) {
		ret = aw882xx_monitor_get_data_from_table(aw882xx,
			monitor_cfg->temp_info, temp_trace,
			monitor_cfg->temp_aplha);
		if (ret < 0)
			return ret;
	} else {
		temp_trace->aw_table.ipeak = IPEAK_NONE;
		temp_trace->aw_table.gain = GAIN_NONE;
		temp_trace->aw_table.vmax = VMAX_NONE;
	}

	aw_dev_info(aw882xx->dev, "%s: vol: ipeak = 0x%x, gain = 0x%x, vmax = 0x%x\n",
			__func__, vol_trace->aw_table.ipeak,
			vol_trace->aw_table.gain,
			vol_trace->aw_table.vmax);

	aw_dev_info(aw882xx->dev, "%s: temp: ipeak = 0x%x, gain = 0x%x, vmax = 0x%x\n",
			__func__, temp_trace->aw_table.ipeak,
			temp_trace->aw_table.gain,
			temp_trace->aw_table.vmax);

	return 0;
}

static void aw882xx_monitor_get_cfg(struct aw882xx *aw882xx,
					struct aw_table *set_table)
{
	struct aw882xx_monitor *monitor = &aw882xx->monitor;
	struct aw_table *temp_data = &monitor->temp_trace.aw_table;
	struct aw_table *vol_data = &monitor->vol_trace.aw_table;

	if (temp_data->ipeak == IPEAK_NONE && vol_data->ipeak == IPEAK_NONE) {
		memcpy(set_table, temp_data, sizeof(struct aw_table));
	} else if (temp_data->ipeak == IPEAK_NONE) {
		memcpy(set_table, vol_data, sizeof(struct aw_table));
	} else if (vol_data->ipeak == IPEAK_NONE) {
		memcpy(set_table, temp_data, sizeof(struct aw_table));
	} else {
		set_table->ipeak = (temp_data->ipeak < vol_data->ipeak ?
				temp_data->ipeak : vol_data->ipeak);
		set_table->gain = (temp_data->gain < vol_data->gain ?
				vol_data->gain : temp_data->gain);
		set_table->vmax = (temp_data->vmax < vol_data->vmax ?
				vol_data->vmax : temp_data->vmax);
	}
}

static void aw882xx_monitor_set_ipeak(struct aw882xx *aw882xx,
				uint8_t ipeak)
{
	struct aw_monitor_cfg *monitor_cfg = aw882xx->monitor.monitor_cfg;
	unsigned int reg_val = 0;
	unsigned int read_reg_val;
	int ret;

	if (ipeak == IPEAK_NONE || (!monitor_cfg->ipeak_switch))
		return;

	ret = aw882xx_i2c_read(aw882xx, AW882XX_SYSCTRL2_REG, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s: read ipeak failed\n", __func__);
		return;
	}

	read_reg_val = reg_val;
	read_reg_val &= AW882XX_BIT_SYSCTRL2_BST_IPEAK_MASK;

	if (read_reg_val == ipeak) {
		aw_dev_info(aw882xx->dev, "%s: ipeak = 0x%x, no change\n",
					__func__, read_reg_val);
		return;
	}
	reg_val &= (~AW882XX_BIT_SYSCTRL2_BST_IPEAK_MASK);
	read_reg_val = ipeak;
	reg_val |= read_reg_val;

	ret = aw882xx_i2c_write(aw882xx, AW882XX_SYSCTRL2_REG, reg_val);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s: write ipeak failed\n", __func__);
		return;
	}
	aw_dev_info(aw882xx->dev, "%s: set reg val = 0x%x, ipeak = 0x%x\n",
					__func__, reg_val, ipeak);
}

static void aw882xx_monitor_set_gain(struct aw882xx *aw882xx,
				uint16_t gain)
{
	struct aw_monitor_cfg *monitor_cfg = aw882xx->monitor.monitor_cfg;
	uint32_t read_volume;
	uint32_t set_volume;
	uint32_t gain_db;
	int ret;

	if (gain == GAIN_NONE || (!monitor_cfg->gain_switch))
		return;

	ret = aw882xx_get_volume(aw882xx, &read_volume);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s: read volume failed\n", __func__);
		return;
	}

	gain_db = aw882xx_reg_val_to_db(gain);

	/*add offset*/
	set_volume = gain_db + aw882xx->db_offset;

	if (read_volume == set_volume) {
		aw_dev_info(aw882xx->dev, "%s: set db = %d.%d dB, no change\n",
				__func__, GET_DB_INT(read_volume),
				GET_DB_DECIMAL(read_volume));
		return;
	}

	ret = aw882xx_set_volume(aw882xx, set_volume);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s: set volume failed\n", __func__);
		return;
	}

	aw_dev_info(aw882xx->dev, "%s: set_volume = %d.%d dB\n",
			__func__, GET_DB_INT(set_volume),
			GET_DB_DECIMAL(set_volume));
}


static void aw882xx_monitor_set_vmax(struct aw882xx *aw882xx,
						uint32_t vmax)
{
	struct aw882xx_chan_info *chan_info = &aw882xx->chan_info;
	struct aw_monitor_cfg *monitor_cfg = aw882xx->monitor.monitor_cfg;
	uint32_t local_vmax = vmax;
	int ret;

	if (vmax == VMAX_NONE || (!monitor_cfg->vmax_switch))
		return;

	if ((aw882xx->monitor.pre_vmax == vmax) &&
		(aw882xx->monitor.first_entry != AW_FIRST_ENTRY)) {
		aw_dev_info(aw882xx->dev, "%s: vmax no change\n",
			__func__);
		return;
	}

	ret = aw_write_data_to_dsp(INDEX_PARAMS_ID_RX_VMAX,
				&local_vmax, sizeof(uint32_t),
				chan_info->channel);
	if (ret)
		aw_dev_err(aw882xx->dev, "%s: dsp_msg_write error\n",
			__func__);

	aw882xx->monitor.pre_vmax = vmax;
	aw_dev_info(aw882xx->dev, "%s: set vmax = 0x%x\n", __func__, vmax);
}

static int aw882xx_monitor_work(struct aw882xx *aw882xx)
{
	struct aw882xx_monitor *monitor = &aw882xx->monitor;
	struct aw_monitor_cfg *monitor_cfg = monitor->monitor_cfg;
	struct aw_table set_table;
	int ret = -1;

	if (g_cali_status != 0) {
		aw_dev_info(aw882xx->dev, "%s: done nothing while start cali",
			__func__);
		return 0;
	}

	ret = aw882xx_monitor_get_temp_and_vol(aw882xx);
	if (ret < 0)
		return ret;

	if (monitor->samp_count < monitor_cfg->monitor_count &&
		(monitor->first_entry == AW_NOT_FIRST_ENTRY))
		return 0;

	ret = aw882xx_monitor_get_data(aw882xx);
	if (ret < 0)
		return ret;

	aw882xx_monitor_get_cfg(aw882xx, &set_table);

	aw_dev_dbg(aw882xx->dev, "%s: set_ipeak = 0x%x, set_gain = 0x%x, set_vmax = 0x%x\n",
		__func__, set_table.ipeak, set_table.gain, set_table.vmax);

	aw882xx_monitor_set_ipeak(aw882xx, set_table.ipeak);

	aw882xx_monitor_set_gain(aw882xx, set_table.gain);

	aw882xx_monitor_set_vmax(aw882xx, set_table.vmax);

	monitor->samp_count = 0;

	if (monitor->first_entry == AW_FIRST_ENTRY)
		monitor->first_entry = AW_NOT_FIRST_ENTRY;

	return 0;
}

static int aw882xx_get_hmute(struct aw882xx *aw882xx)
{
	unsigned int reg_val = 0;
	int ret;

	aw_dev_dbg(aw882xx->dev, "%s: enter\n", __func__);

	aw882xx_i2c_read(aw882xx, AW882XX_SYSCTRL2_REG, &reg_val);
	if ((~AW882XX_HMUTE_MASK) & reg_val)
		ret = 1;
	else
		ret = 0;

	return ret;
}

static void aw882xx_monitor_work_func(struct work_struct *work)
{
	struct aw882xx *aw882xx = container_of(work,
			struct aw882xx, monitor.delay_work.work);
	struct aw_monitor_cfg *monitor_cfg = aw882xx->monitor.monitor_cfg;

	aw_dev_dbg(aw882xx->dev, "%s: monitor is_enable %d,scene_mode %d,monitor_status:%d, monitor_switch:%d\n",
		__func__, aw882xx->monitor.is_enable, aw882xx->scene_mode,
		monitor_cfg->monitor_status, monitor_cfg->monitor_switch);

	if (aw882xx->monitor.is_enable &&
		(aw882xx->scene_mode == AW882XX_SPEAKER_MODE) &&
		(monitor_cfg->monitor_status == AW_MON_CFG_OK) &&
		monitor_cfg->monitor_switch) {
		if (!aw882xx_get_hmute(aw882xx)) {
			aw882xx_monitor_work(aw882xx);
			schedule_delayed_work(&aw882xx->monitor.delay_work,
				msecs_to_jiffies(monitor_cfg->monitor_time));
		}
	}
}

void aw882xx_monitor_start(struct aw882xx_monitor *monitor)
{
	struct aw882xx *aw882xx = container_of(monitor,
			struct aw882xx, monitor);

	aw_dev_info(aw882xx->dev, "%s: enter\n", __func__);
	monitor->first_entry = AW_FIRST_ENTRY;
	monitor->samp_count = 0;
	monitor->vol_trace.sum_val = 0;
	monitor->temp_trace.sum_val = 0;
	aw882xx->monitor.pre_vmax = 0;

	schedule_delayed_work(&aw882xx->monitor.delay_work,
				msecs_to_jiffies(0));
}

void aw882xx_monitor_stop(struct aw882xx_monitor *monitor)
{
	struct aw882xx *aw882xx = container_of(monitor,
				struct aw882xx, monitor);

	aw_dev_info(aw882xx->dev, "%s: enter\n", __func__);
	cancel_delayed_work_sync(&aw882xx->monitor.delay_work);
}


/*****************************************************
 * load monitor config
 *****************************************************/
static int aw_check_monitor_profile(struct aw882xx *aw882xx,
					struct aw882xx_container *cont)
{
	struct aw_monitor_hdr *monitor_hdr =
		(struct aw_monitor_hdr *)cont->data;
	int temp_size, vol_size;

	if (cont->len < sizeof(struct aw_monitor_hdr)) {
		aw_dev_err(aw882xx->dev,
			"%s:params size[%d] < struct aw_monitor_hdr size[%d]!\n",
			__func__, cont->len,
			(int)sizeof(struct aw_monitor_hdr));
		return -ENOMEM;
	}

	if (monitor_hdr->temp_offset > cont->len) {
		aw_dev_err(aw882xx->dev,
			"%s:temp_offset[%d] overflow file size[%d]!\n",
			__func__, monitor_hdr->temp_offset, cont->len);
		return -ENOMEM;
	}

	if (monitor_hdr->vol_offset > cont->len) {
		aw_dev_err(aw882xx->dev,
			"%s:vol_offset[%d] overflow file size[%d]!\n",
			__func__, monitor_hdr->vol_offset, cont->len);
		return -ENOMEM;
	}

	temp_size = monitor_hdr->temp_num * monitor_hdr->single_temp_size;
	if (temp_size > cont->len) {
		aw_dev_err(aw882xx->dev,
			"%s:temp_size:[%d] overflow file size[%d]!!\n",
			__func__, temp_size, cont->len);
		return -ENOMEM;
	}

	vol_size = monitor_hdr->vol_num * monitor_hdr->single_vol_size;
	if (vol_size > cont->len) {
		aw_dev_err(aw882xx->dev,
			"%s:vol_size:[%d] overflow file size[%d]!\n",
			__func__, vol_size, cont->len);
		return -ENOMEM;
	}

	return 0;
}

static void aw_parse_monitor_hdr(struct aw882xx *aw882xx,
					struct aw882xx_container *cont)
{
	struct aw_monitor_hdr *monitor_hdr =
			(struct aw_monitor_hdr *)cont->data;
	struct aw_monitor_cfg *monitor_cfg = aw882xx->monitor.monitor_cfg;

	monitor_cfg->monitor_switch = monitor_hdr->monitor_switch;
	monitor_cfg->monitor_time = monitor_hdr->monitor_time;
	monitor_cfg->monitor_count = monitor_hdr->monitor_count;
	monitor_cfg->ipeak_switch = monitor_hdr->ipeak_switch;
	monitor_cfg->gain_switch = monitor_hdr->gain_switch;
	monitor_cfg->vmax_switch = monitor_hdr->vmax_switch;
	monitor_cfg->temp_switch = monitor_hdr->temp_switch;
	monitor_cfg->temp_aplha = monitor_hdr->temp_aplha;
	monitor_cfg->vol_switch = monitor_hdr->vol_switch;
	monitor_cfg->vol_aplha = monitor_hdr->vol_aplha;

	aw_dev_info(aw882xx->dev, "%s: chip name:%s\n",
		__func__, monitor_hdr->chip_type);
	aw_dev_info(aw882xx->dev, "%s: ui ver:0x%x\n",
		__func__, monitor_hdr->ui_ver);

	aw_dev_info(aw882xx->dev,
		"%s:monitor_switch:%d, monitor_time:%d (ms), monitor_count:%d\n",
		__func__, monitor_cfg->monitor_switch,
		monitor_cfg->monitor_time, monitor_cfg->monitor_count);

	aw_dev_info(aw882xx->dev,
		"%s:ipeak_switch:%d, gain_switch:%d, vmax_switch:%d\n",
		__func__, monitor_cfg->ipeak_switch,
		monitor_cfg->gain_switch, monitor_cfg->vmax_switch);

	aw_dev_info(aw882xx->dev,
		"%s:temp_switch:%d, temp_aplha:%d, vol_switch:%d, vol_aplha:%d\n",
		__func__, monitor_cfg->temp_switch,
		monitor_cfg->temp_aplha, monitor_cfg->vol_switch,
		monitor_cfg->vol_aplha);
}

static void aw_populate_data_to_table(struct aw882xx *aw882xx,
		struct aw_table_info *table_info, const char *offset_ptr)
{
	int i;

	for (i = 0; i < table_info->table_num * AW_TABLE_SIZE; i += AW_TABLE_SIZE) {
		table_info->aw_table[i / AW_TABLE_SIZE].min_val =
			GET_16_DATA(offset_ptr[1 + i], offset_ptr[i]);
		table_info->aw_table[i / AW_TABLE_SIZE].max_val =
			GET_16_DATA(offset_ptr[3 + i], offset_ptr[2 + i]);
		table_info->aw_table[i / AW_TABLE_SIZE].ipeak =
			GET_16_DATA(offset_ptr[5 + i], offset_ptr[4 + i]);
		table_info->aw_table[i / AW_TABLE_SIZE].gain =
			GET_16_DATA(offset_ptr[7 + i], offset_ptr[6 + i]);
		table_info->aw_table[i / AW_TABLE_SIZE].vmax =
			GET_32_DATA(offset_ptr[11 + i], offset_ptr[10 + i],
				offset_ptr[9 + i], offset_ptr[8 + i]);
	}

	for (i = 0; i < table_info->table_num; i++)
		aw_dev_info(aw882xx->dev,
			"min_val:%d, max_val:%d, ipeak:0x%x, gain:0x%x, vmax:0x%x\n",
			table_info->aw_table[i].min_val,
			table_info->aw_table[i].max_val,
			table_info->aw_table[i].ipeak,
			table_info->aw_table[i].gain,
			table_info->aw_table[i].vmax);

}

static int aw_parse_temp_data(struct aw882xx *aw882xx,
			struct aw882xx_container *cont)
{
	struct aw_monitor_hdr *monitor_hdr =
			(struct aw_monitor_hdr *)cont->data;
	struct aw_table_info *temp_info =
		&aw882xx->monitor.monitor_cfg->temp_info;

	aw_dev_info(aw882xx->dev, "%s: ===parse temp start ===\n",
		__func__);

	if (temp_info->aw_table != NULL) {
		kfree(temp_info->aw_table);
		temp_info->aw_table = NULL;
	}

	temp_info->aw_table = kzalloc((monitor_hdr->temp_num * AW_TABLE_SIZE),
			GFP_KERNEL);
	if (!temp_info->aw_table)
		return -ENOMEM;

	temp_info->table_num = monitor_hdr->temp_num;
	aw_populate_data_to_table(aw882xx, temp_info,
		&cont->data[monitor_hdr->temp_offset]);
	aw_dev_info(aw882xx->dev, "%s: ===parse temp end ===\n",
		__func__);
	return 0;
}

static int aw_parse_vol_data(struct aw882xx *aw882xx,
			struct aw882xx_container *cont)
{
	struct aw_monitor_hdr *monitor_hdr =
			(struct aw_monitor_hdr *)cont->data;
	struct aw_table_info *vol_info =
		&aw882xx->monitor.monitor_cfg->vol_info;

	aw_dev_info(aw882xx->dev, "%s: ===parse vol start ===\n",
		__func__);

	if (vol_info->aw_table != NULL) {
		kfree(vol_info->aw_table);
		vol_info->aw_table = NULL;
	}

	vol_info->aw_table = kzalloc((monitor_hdr->vol_num * AW_TABLE_SIZE),
			GFP_KERNEL);
	if (!vol_info->aw_table)
		return -ENOMEM;

	vol_info->table_num = monitor_hdr->vol_num;
	aw_populate_data_to_table(aw882xx, vol_info,
		&cont->data[monitor_hdr->vol_offset]);
	aw_dev_info(aw882xx->dev, "%s: ===parse vol end ===\n",
		__func__);
	return 0;
}

static int aw_parse_monitor_data(struct aw882xx *aw882xx,
					struct aw882xx_container *cont)
{
	int ret;
	struct aw_monitor_cfg *monitor_cfg = aw882xx->monitor.monitor_cfg;

	ret = aw_check_monitor_profile(aw882xx, cont);
	if (ret < 0) {
		aw_dev_err(aw882xx->dev, "%s:check %s failed\n",
				__func__, AW_MONITOR_FILE);
		return ret;
	}

	aw_parse_monitor_hdr(aw882xx, cont);

	ret = aw_parse_temp_data(aw882xx, cont);
	if (ret < 0)
		return ret;

	ret = aw_parse_vol_data(aw882xx, cont);
	if (ret < 0) {
		if (monitor_cfg->temp_info.aw_table != NULL) {
			kfree(monitor_cfg->temp_info.aw_table);
			monitor_cfg->temp_info.aw_table = NULL;
			monitor_cfg->temp_info.table_num = 0;
		}
		return ret;
	}

	monitor_cfg->monitor_status = AW_MON_CFG_OK;
	return 0;
}

static int aw_monitor_param_check_sum(struct aw882xx *aw882xx,
					struct aw882xx_container *cont)
{
	int i, check_sum = 0;
	struct aw_monitor_hdr *monitor_hdr =
		(struct aw_monitor_hdr *)cont->data;

	for (i = 4; i < cont->len; i++)
		check_sum += (uint8_t)cont->data[i];

	if (monitor_hdr->check_sum != check_sum) {
		aw_dev_err(aw882xx->dev, "%s:check_sum[%d] is not equal to actual check_sum[%d]\n",
			__func__, monitor_hdr->check_sum, check_sum);
		return -ENOMEM;
	}

	return 0;
}

static int aw_parse_monitor_profile(struct aw882xx *aw882xx,
					struct aw882xx_container *cont)
{
	struct aw_monitor_hdr *monitor_hdr =
			(struct aw_monitor_hdr *)cont->data;
	int ret;

	ret = aw_monitor_param_check_sum(aw882xx, cont);
	if (ret < 0)
		return ret;

	switch (monitor_hdr->monitor_ver) {
	case AW_MONITOR_HDR_VER_0_1_0:
		return aw_parse_monitor_data(aw882xx, cont);
	default:
		aw_dev_err(aw882xx->dev, "%s:cfg version:0x%x unsupported\n",
				__func__, monitor_hdr->monitor_ver);
		return -EINVAL;
	}
}

static void aw_monitor_profile_loaded(const struct firmware *cont,
					void *context)
{
	struct aw882xx *aw882xx = context;
	struct aw882xx_container *monitor_cfg = NULL;
	int ret;

	mutex_lock(&g_aw_monitor_lock);
	if (aw882xx->monitor.monitor_cfg->monitor_status == AW_MON_CFG_ST) {
		if (!cont) {
			aw_dev_err(aw882xx->dev, "%s:failed to read %s\n",
					__func__, AW_MONITOR_FILE);
			goto exit;
		}

		aw_dev_info(aw882xx->dev, "%s: loaded %s - size: %zu\n",
			__func__, AW_MONITOR_FILE, cont ? cont->size : 0);

		monitor_cfg = kzalloc(cont->size + sizeof(int), GFP_KERNEL);
		if (!monitor_cfg) {
			aw_dev_err(aw882xx->dev, "%s: error allocating memory\n", __func__);
			goto exit;
		}
		monitor_cfg->len = cont->size;
		memcpy(monitor_cfg->data, cont->data, cont->size);
		ret = aw_parse_monitor_profile(aw882xx, monitor_cfg);
		if (ret < 0)
			aw_dev_err(aw882xx->dev, "%s:parse monitor cfg failed\n",
				__func__);
		kfree(monitor_cfg);
		monitor_cfg = NULL;
	}

exit:
	release_firmware(cont);
	mutex_unlock(&g_aw_monitor_lock);
}

int aw882xx_load_monitor_profile(struct aw882xx_monitor *monitor)
{
	int ret;
	struct aw882xx *aw882xx = container_of(monitor,
					struct aw882xx, monitor);

	if (!monitor->is_enable) {
		aw_dev_info(aw882xx->dev, "%s: monitor flag:%d, monitor bin noload\n",
			__func__, monitor->is_enable);
		ret = 0;
	} else {
		ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
				AW_MONITOR_FILE,
				aw882xx->dev, GFP_KERNEL, aw882xx,
				aw_monitor_profile_loaded);
	}

	return ret;
}

static void aw882xx_monitor_load_fw_work_func(struct work_struct *work)
{
	struct aw882xx_monitor *monitor = container_of(work,
				struct aw882xx_monitor, load_fw_work.work);

	aw882xx_load_monitor_profile(monitor);
}

void aw882xx_deinit_monitor_profile(struct aw882xx_monitor *monitor)
{
	struct aw_monitor_cfg *monitor_cfg = monitor->monitor_cfg;

	monitor_cfg->monitor_status = AW_MON_CFG_ST;

	if (monitor_cfg->temp_info.aw_table != NULL) {
		kfree(monitor_cfg->temp_info.aw_table);
		monitor_cfg->temp_info.aw_table = NULL;
	}

	if (monitor_cfg->vol_info.aw_table != NULL) {
		kfree(monitor_cfg->vol_info.aw_table);
		monitor_cfg->vol_info.aw_table = NULL;
	}
	memset(monitor_cfg, 0, sizeof(struct aw_monitor_cfg));
}

#ifdef AW_DEBUG
static ssize_t aw882xx_vol_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	uint32_t vol = 0;
	int ret = -1;

	if (count == 0)
		return 0;

	ret = kstrtouint(buf, 0, &vol);
	if (ret < 0)
		return ret;

	aw_dev_info(aw882xx->dev, "%s: vol set =%d\n", __func__, vol);
	aw882xx->monitor.test_vol = vol;

	return count;
}

static ssize_t aw882xx_vol_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint32_t local_vol = aw882xx->monitor.test_vol;

	len += snprintf(buf+len, PAGE_SIZE-len,
		"aw882xx vol: %d\n", local_vol);
	return len;
}

static ssize_t aw882xx_temp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	int32_t temp = 0;
	int ret = -1;

	if (count == 0)
		return 0;

	ret = kstrtoint(buf, 0, &temp);
	if (ret < 0)
		return ret;

	aw_dev_info(aw882xx->dev, "%s: temp set =%d\n", __func__, temp);
	aw882xx->monitor.test_temp = temp;

	return count;
}

static ssize_t aw882xx_temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	int32_t local_temp = aw882xx->monitor.test_temp;

	len += snprintf(buf+len, PAGE_SIZE-len,
		"aw882xx temp: %d\n", local_temp);
	return len;
}

static DEVICE_ATTR(vol, S_IWUSR | S_IRUGO,
	aw882xx_vol_show, aw882xx_vol_store);
static DEVICE_ATTR(temp, S_IWUSR | S_IRUGO,
	aw882xx_temp_show, aw882xx_temp_store);
#endif

static ssize_t aw882xx_monitor_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	uint32_t enable = 0;
	int ret = -1;

	if (count == 0)
		return 0;

	ret = kstrtouint(buf, 0, &enable);
	if (ret < 0)
		return ret;

	aw_dev_info(aw882xx->dev, "%s:monitor  enable set =%d\n",
		__func__, enable);
	aw882xx->monitor.is_enable = enable;
	if (enable)
		schedule_delayed_work(&aw882xx->monitor.delay_work,
					msecs_to_jiffies(0));

	return count;
}

static ssize_t aw882xx_monitor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	ssize_t len = 0;
	uint32_t local_enable;

	local_enable = aw882xx->monitor.is_enable;
	len += snprintf(buf+len, PAGE_SIZE-len,
		"aw882xx monitor enable: %d\n", local_enable);
	return len;
}

static ssize_t aw_monitor_update_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw882xx *aw882xx = dev_get_drvdata(dev);
	struct list_head *list;
	struct aw882xx_monitor *aw_monitor = NULL;
	uint32_t update = 0;
	int ret = -1;

	if (count == 0)
		return 0;

	ret = kstrtouint(buf, 0, &update);
	if (ret < 0)
		return ret;

	aw_dev_info(aw882xx->dev, "%s:monitor update = %d\n",
		__func__, update);

	if (update) {
		/*stop all monitor*/
		list_for_each(list, &g_aw_monitor_list) {
			aw_monitor = container_of(list,
				struct aw882xx_monitor, list);
			aw882xx_monitor_stop(aw_monitor);
		}

		aw882xx_deinit_monitor_profile(&aw882xx->monitor);
		aw882xx_load_monitor_profile(&aw882xx->monitor);
		msleep(50);
		/*start all monitor*/
		list_for_each(list, &g_aw_monitor_list) {
			aw_monitor = container_of(list,
				struct aw882xx_monitor, list);
			aw882xx_monitor_start(aw_monitor);
		}
	}

	return count;
}

static DEVICE_ATTR(monitor, S_IWUSR | S_IRUGO,
	aw882xx_monitor_show, aw882xx_monitor_store);
static DEVICE_ATTR(monitor_update, S_IWUSR,
	NULL, aw_monitor_update_store);


static struct attribute *aw882xx_monitor_attr[] = {
	&dev_attr_monitor.attr,
	&dev_attr_monitor_update.attr,
#ifdef AW_DEBUG
	&dev_attr_vol.attr,
	&dev_attr_temp.attr,
#endif
	NULL
};

static struct attribute_group aw882xx_monitor_attr_group = {
	.attrs = aw882xx_monitor_attr,
};

void aw882xx_monitor_init(struct aw882xx_monitor *monitor)
{
	int ret;
	struct aw882xx *aw882xx = container_of(monitor,
				struct aw882xx, monitor);

	aw_dev_info(aw882xx->dev, "%s: enter\n", __func__);

#ifdef AW_DEBUG
	monitor->test_vol = 0;
	monitor->test_temp = 0;
#endif

	mutex_lock(&g_aw_monitor_lock);
	if (g_monitor_dev_num == 0)
		memset(&g_monitor_cfg, 0, sizeof(struct aw_monitor_cfg));
	monitor->monitor_cfg = &g_monitor_cfg;
	g_monitor_dev_num++;
	mutex_unlock(&g_aw_monitor_lock);

	INIT_LIST_HEAD(&monitor->list);
	list_add(&monitor->list, &g_aw_monitor_list);

	INIT_DELAYED_WORK(&monitor->delay_work, aw882xx_monitor_work_func);
	INIT_DELAYED_WORK(&monitor->load_fw_work, aw882xx_monitor_load_fw_work_func);


	ret = sysfs_create_group(&aw882xx->dev->kobj,
				&aw882xx_monitor_attr_group);
	if (ret < 0)
		aw_dev_err(aw882xx->dev, "%s error creating sysfs attr files\n",
			__func__);
}

void aw882xx_monitor_deinit(struct aw882xx_monitor *monitor)
{
	struct aw882xx *aw882xx = container_of(monitor,
			struct aw882xx, monitor);

	mutex_lock(&g_aw_monitor_lock);
	aw882xx_monitor_stop(monitor);
	g_monitor_dev_num--;
	if (g_monitor_dev_num == 0)
		aw882xx_deinit_monitor_profile(monitor);
	mutex_unlock(&g_aw_monitor_lock);

	sysfs_remove_group(&aw882xx->dev->kobj, &aw882xx_monitor_attr_group);
}

/*****************************************************
 * device tree parse monitor param
 *****************************************************/
void aw882xx_parse_monitor_dt(struct aw882xx_monitor *monitor)
{
	int ret;
	struct aw882xx *aw882xx = container_of(monitor,
				struct aw882xx, monitor);
	struct device_node *np = aw882xx->dev->of_node;

	ret = of_property_read_u32(np, "monitor-flag", &monitor->is_enable);
	if (ret) {
		monitor->is_enable = AW882XX_MONITOR_DEFAULT_FLAG;
		aw_dev_info(aw882xx->dev,
			"%s: monitor-flag get failed ,user default value!\n",
			__func__);
	} else {
		aw_dev_info(aw882xx->dev, "%s: monitor-flag = %d\n",
			__func__, monitor->is_enable);
	}
}

