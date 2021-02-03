/*
 * awinic_cali.c cali_module
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
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/of.h>
//#include <sound/q6adm-v2.h>
//#include <sound/apr_audio-v2.h>
#include "aw882xx.h"
#include "awinic_cali.h"
#include "awinic_monitor.h"
#include "awinic_dsp.h"

static DEFINE_MUTEX(g_dsp_lock);
static DEFINE_MUTEX(g_msg_dsp_lock);

static const uint32_t PARAM_ID_INDEX_TABLE[][INDEX_PARAMS_ID_MAX] = {
	{
		AFE_PARAM_ID_AWDSP_RX_PARAMS,
		AFE_PARAM_ID_AWDSP_RX_SET_ENABLE,
		AFE_PARAM_ID_AWDSP_TX_SET_ENABLE,
		AFE_PARAM_ID_AWDSP_RX_VMAX_L,
		AFE_PARAM_ID_AWDSP_RX_CALI_CFG_L,
		AFE_PARAM_ID_AWDSP_RX_RE_L,
		AFE_PARAM_ID_AWDSP_RX_NOISE_L,
		AFE_PARAM_ID_AWDSP_RX_F0_L,
		AFE_PARAM_ID_AWDSP_RX_REAL_DATA_L,
		AFE_PARAM_ID_AWDSP_RX_MSG,
	},
	{
		AFE_PARAM_ID_AWDSP_RX_PARAMS,
		AFE_PARAM_ID_AWDSP_RX_SET_ENABLE,
		AFE_PARAM_ID_AWDSP_TX_SET_ENABLE,
		AFE_PARAM_ID_AWDSP_RX_VMAX_R,
		AFE_PARAM_ID_AWDSP_RX_CALI_CFG_R,
		AFE_PARAM_ID_AWDSP_RX_RE_R,
		AFE_PARAM_ID_AWDSP_RX_NOISE_R,
		AFE_PARAM_ID_AWDSP_RX_F0_R,
		AFE_PARAM_ID_AWDSP_RX_REAL_DATA_R,
		AFE_PARAM_ID_AWDSP_RX_MSG,
	},
};

/***************dsp communicate**************/
#ifdef AW_MTK_PLATFORM
extern int mtk_spk_send_ipi_buf_to_dsp(void *data_buffer, uint32_t data_size);
extern int mtk_spk_recv_ipi_buf_from_dsp(int8_t *buffer, int16_t size, uint32_t *buf_len);
#elif defined AW_QCOM_PLATFORM
extern int afe_get_topology(int port_id);
extern int aw_send_afe_cal_apr(uint32_t param_id,
	void *buf, int cmd_size, bool write);
extern int aw_send_afe_rx_module_enable(void *buf, int size);
extern int aw_send_afe_tx_module_enable(void *buf, int size);

#else
static int afe_get_topology(int port_id)
{
	return -EPERM;
}
static int aw_send_afe_cal_apr(uint32_t param_id,
	void *buf, int cmd_size, bool write)
{
	return 0;
}
static int aw_send_afe_rx_module_enable(void *buf, int size)
{
	return 0;
}
static int aw_send_afe_tx_module_enable(void *buf, int size)
{
	return 0;
}
#endif

#ifdef AW_QCOM_PLATFORM
extern int aw_adm_param_enable(int port_id, int module_id,  int param_id, int enable);
#else
static int aw_adm_param_enable(int port_id, int module_id,  int param_id, int enable)
{
	return 0;
}
#endif

static int aw_get_params_id_by_index(int index, int32_t *params_id, int channel)
{
	if (index > INDEX_PARAMS_ID_MAX || channel > 1) {
		pr_err("%s: error: index is %d, channel %d\n",
			__func__, index, channel);
		return -EINVAL;
	}
	*params_id = PARAM_ID_INDEX_TABLE[channel][index];
	return 0;
}

#ifdef AW_MTK_PLATFORM
/*****************mtk dsp communication function start**********************/
static int aw_mtk_write_data_to_dsp(int index, void *data, int data_size, int channel)
{
	int32_t param_id;
	int32_t *dsp_data = NULL;
	struct aw_dsp_msg_hdr *hdr = NULL;
	int ret;

	ret = aw_get_params_id_by_index(index, &param_id, channel);
	if (ret < 0)
		return ret;

	pr_debug("%s: param id = 0x%x", __func__, param_id);

	dsp_data = kzalloc(sizeof(struct aw_dsp_msg_hdr) + data_size,
			GFP_KERNEL);
	if (!dsp_data) {
		pr_err("%s: kzalloc dsp_msg error\n", __func__);
		return -ENOMEM;
	}

	hdr = (struct aw_dsp_msg_hdr *)dsp_data;
	hdr->type = DSP_MSG_TYPE_DATA;
	hdr->opcode_id = param_id;
	hdr->version = AWINIC_DSP_MSG_HDR_VER;

	memcpy(((char *)dsp_data) + sizeof(struct aw_dsp_msg_hdr),
		data, data_size);

	ret = mtk_spk_send_ipi_buf_to_dsp(dsp_data,
				sizeof(struct aw_dsp_msg_hdr) + data_size);
	if (ret < 0) {
		pr_err("%s:write data failed\n", __func__);
		kfree(dsp_data);
		dsp_data = NULL;
		return ret;
	}

	kfree(dsp_data);
	dsp_data = NULL;
	return 0;
}

static int aw_mtk_read_data_from_dsp(int index, void *data, int data_size, int channel)
{
	int ret;
	int32_t param_id;
	struct aw_dsp_msg_hdr hdr;

	ret = aw_get_params_id_by_index(index, &param_id, channel);
	if (ret < 0)
		return ret;

	pr_debug("%s: param id = 0x%x", __func__, param_id);
	hdr.type = DSP_MSG_TYPE_CMD;
	hdr.opcode_id = param_id;
	hdr.version = AWINIC_DSP_MSG_HDR_VER;

	mutex_lock(&g_msg_dsp_lock);
	ret = mtk_spk_send_ipi_buf_to_dsp(&hdr, sizeof(struct aw_dsp_msg_hdr));
	if (ret < 0) {
		pr_err("%s:send cmd failed\n", __func__);
		goto dsp_msg_failed;
	}

	ret = mtk_spk_recv_ipi_buf_from_dsp(data, data_size, &data_size);
	if (ret < 0) {
		pr_err("%s:get data failed\n", __func__);
		goto dsp_msg_failed;
	}
	mutex_unlock(&g_msg_dsp_lock);
	return 0;

dsp_msg_failed:
	mutex_unlock(&g_msg_dsp_lock);
	return ret;
}

static int aw_mtk_write_msg_to_dsp(int inline_id,
		void *data, int data_size, int channel)
{
	int32_t *dsp_msg = NULL;
	struct aw_dsp_msg_hdr *hdr = NULL;
	int ret;

	dsp_msg = kzalloc(sizeof(struct aw_dsp_msg_hdr) + data_size,
			GFP_KERNEL);
	if (!dsp_msg) {
		pr_err("%s: inline_id:0x%x kzalloc dsp_msg error\n",
			__func__, inline_id);
		return -ENOMEM;
	}
	hdr = (struct aw_dsp_msg_hdr *)dsp_msg;
	hdr->type = DSP_MSG_TYPE_DATA;
	hdr->opcode_id = inline_id;
	hdr->version = AWINIC_DSP_MSG_HDR_VER;

	memcpy(((char *)dsp_msg) + sizeof(struct aw_dsp_msg_hdr),
		data, data_size);

	ret = aw_mtk_write_data_to_dsp(INDEX_PARAMS_ID_AWDSP_RX_MSG, (void *)dsp_msg,
				sizeof(struct aw_dsp_msg_hdr) + data_size, channel);
	if (ret < 0) {
		pr_err("%s:inline_id:0x%x, write data failed\n",
			__func__, inline_id);
		kfree(dsp_msg);
		dsp_msg = NULL;
		return ret;
	}

	kfree(dsp_msg);
	dsp_msg = NULL;
	return 0;
}

static int aw_mtk_read_msg_from_dsp(int inline_id,
			char *data, int data_size, int channel)
{
	struct aw_dsp_msg_hdr hdr[2];
	int ret;

	hdr[0].type = DSP_MSG_TYPE_DATA;
	hdr[0].opcode_id = AFE_PARAM_ID_AWDSP_RX_MSG;
	hdr[0].version = AWINIC_DSP_MSG_HDR_VER;
	hdr[1].type = DSP_MSG_TYPE_CMD;
	hdr[1].opcode_id = inline_id;
	hdr[1].version = AWINIC_DSP_MSG_HDR_VER;

	mutex_lock(&g_msg_dsp_lock);
	ret = mtk_spk_send_ipi_buf_to_dsp(&hdr, 2 * sizeof(struct aw_dsp_msg_hdr));
	if (ret < 0) {
		pr_err("%s:send cmd failed\n", __func__);
		goto dsp_msg_failed;
	}

	ret = mtk_spk_recv_ipi_buf_from_dsp(data, data_size, &data_size);
	if (ret < 0) {
		pr_err("%s:get data failed\n", __func__);
		goto dsp_msg_failed;
	}
	mutex_unlock(&g_msg_dsp_lock);
	return 0;

dsp_msg_failed:
	mutex_unlock(&g_msg_dsp_lock);
	return ret;
}

static int aw_mtk_send_module_enable(void *buf, uint8_t type)
{
	int ret;

	switch (type) {
	case AW_RX_MODULE:
		ret = aw_mtk_write_data_to_dsp(AFE_PARAM_ID_AWDSP_RX_SET_ENABLE,
					buf, sizeof(uint32_t), 0);
	case AW_TX_MODULE:
		ret = aw_mtk_write_data_to_dsp(AFE_PARAM_ID_AWDSP_TX_SET_ENABLE,
					buf, sizeof(uint32_t), 0);
		break;
	default:
		pr_err("%s: mtk unsupported type %d\n", __func__, type);
		return -EINVAL;
	}

	return ret;
}

static int aw_mtk_get_module_enable(void *buf, uint8_t type)
{
	int ret;

	switch (type) {
	case AW_RX_MODULE:
		ret = aw_mtk_read_data_from_dsp(INDEX_PARAMS_ID_RX_ENBALE,
					buf, sizeof(uint32_t), 0);
	case AW_TX_MODULE:
		ret = aw_mtk_read_data_from_dsp(INDEX_PARAMS_ID_TX_ENABLE,
					buf, sizeof(uint32_t), 0);
		break;
	default:
		pr_err("%s: mtk unsupported type %d\n", __func__, type);
		return -EINVAL;
	}

	return ret;
}
/********************mtk dsp communication function end***********************/
#else
/******************qcom dsp communication function start**********************/
static int aw_check_dsp_ready(void)
{
	int ret;

	ret = afe_get_topology(AFE_PORT_ID_AWDSP_RX);
	pr_debug("%s: topo_id 0x%x\n", __func__, ret);

	if (ret <= 0)
		return false;
	else
		return true;
}

static int aw_qcom_write_data_to_dsp(int index, void *data, int data_size, int channel)
{
	int ret;
	int32_t param_id;
	int try = 0;

	ret = aw_get_params_id_by_index(index, &param_id, channel);
	if (ret < 0)
		return ret;

	while (try < AW_DSP_TRY_TIME) {
		if (aw_check_dsp_ready()) {
			mutex_lock(&g_dsp_lock);
			ret = aw_send_afe_cal_apr(param_id, data,
					data_size, true);
			mutex_unlock(&g_dsp_lock);
			return ret;
		} else {
			try++;
			msleep(AW_DSP_SLEEP_TIME);
			pr_debug("%s: afe not ready try again\n", __func__);
		}
	}

	return -EINVAL;
}

static int aw_qcom_read_data_from_dsp(int index, void *data, int data_size, int channel)
{
	int ret;
	int32_t param_id;
	int try = 0;

	ret = aw_get_params_id_by_index(index, &param_id, channel);
	if (ret < 0)
		return ret;

	while (try < AW_DSP_TRY_TIME) {
		if (aw_check_dsp_ready()) {
			mutex_lock(&g_dsp_lock);
			ret = aw_send_afe_cal_apr(param_id, data,
						data_size, false);
			mutex_unlock(&g_dsp_lock);
			return ret;
		} else {
			try++;
			msleep(AW_DSP_SLEEP_TIME);
			pr_debug("[Awinic] %s: afe not ready try again\n",
				__func__);
		}
	}

	return -EINVAL;
}


static int aw_qcom_write_msg_to_dsp(int inline_id,
			char *data, int data_size, int channel)
{
	int32_t *dsp_msg = NULL;
	struct aw_dsp_msg_hdr *hdr = NULL;
	int ret;

	dsp_msg = kzalloc(sizeof(struct aw_dsp_msg_hdr) + data_size,
			GFP_KERNEL);
	if (!dsp_msg) {
		pr_err("%s: inline_id:0x%x kzalloc dsp_msg error\n",
			__func__, inline_id);
		return -ENOMEM;
	}
	hdr = (struct aw_dsp_msg_hdr *)dsp_msg;
	hdr->type = DSP_MSG_TYPE_DATA;
	hdr->opcode_id = inline_id;
	hdr->version = AWINIC_DSP_MSG_HDR_VER;

	memcpy(((char *)dsp_msg) + sizeof(struct aw_dsp_msg_hdr),
		data, data_size);

	ret = aw_qcom_write_data_to_dsp(INDEX_PARAMS_ID_AWDSP_RX_MSG,
			(void *)dsp_msg,
			sizeof(struct aw_dsp_msg_hdr) + data_size, channel);
	if (ret < 0) {
		pr_err("%s:inline_id:0x%x, write data failed\n",
			__func__, inline_id);
		kfree(dsp_msg);
		dsp_msg = NULL;
		return ret;
	}

	kfree(dsp_msg);
	dsp_msg = NULL;
	return 0;
}

static int aw_qcom_read_msg_from_dsp(int inline_id,
			char *data, int data_size, int channel)
{
	struct aw_dsp_msg_hdr dsp_msg;
	int ret;

	dsp_msg.type = DSP_MSG_TYPE_CMD;
	dsp_msg.opcode_id = inline_id;
	dsp_msg.version = AWINIC_DSP_MSG_HDR_VER;

	mutex_lock(&g_msg_dsp_lock);
	ret = aw_qcom_write_data_to_dsp(INDEX_PARAMS_ID_AWDSP_RX_MSG,
			&dsp_msg, sizeof(struct aw_dsp_msg_hdr), channel);
	if (ret < 0) {
		pr_err("%s:inline_id:0x%x, write cmd to dsp failed\n",
			__func__, inline_id);
		goto dsp_msg_failed;
	}

	ret = aw_qcom_read_data_from_dsp(INDEX_PARAMS_ID_AWDSP_RX_MSG,
			data, data_size, channel);
	if (ret < 0) {
		pr_err("%s:inline_id:0x%x, read data from dsp failed\n",
			__func__, inline_id);
		goto dsp_msg_failed;
	}

	mutex_unlock(&g_msg_dsp_lock);
	return 0;

dsp_msg_failed:
	mutex_unlock(&g_msg_dsp_lock);
	return ret;
}

static int aw_qcom_send_module_enable(void *buf, int type)
{

	if (type == AW_RX_MODULE)
		return aw_send_afe_rx_module_enable(buf, sizeof(int32_t));
	else
		return aw_send_afe_tx_module_enable(buf, sizeof(int32_t));
}

static int aw_qcom_get_module_enable(void *buf, int type)
{
	int ret;

	if (type == AW_RX_MODULE) {
		ret = aw_qcom_read_data_from_dsp(INDEX_PARAMS_ID_RX_ENBALE,
						buf, sizeof(int32_t), 0);
		if (ret) {
			pr_err("%s: read afe rx failed \n", __func__);
			return ret;
		}
	} else {
		ret = aw_qcom_read_data_from_dsp(INDEX_PARAMS_ID_TX_ENABLE,
						buf, sizeof(int32_t), 0);
		if (ret) {
			pr_err("%s: read afe tx failed \n", __func__);
			return ret;
		}
	}
	return 0;
}


#endif
/*****************qcom dsp communication function end*********************/
int aw_write_data_to_dsp(int index, void *data, int data_size, int channel)
{
#ifdef AW_MTK_PLATFORM
	return aw_mtk_write_data_to_dsp(index, data, data_size, channel);
#else
	return aw_qcom_write_data_to_dsp(index, data, data_size, channel);
#endif
}

int aw_read_data_from_dsp(int index, void *data, int data_size, int channel)
{
#ifdef AW_MTK_PLATFORM
	return aw_mtk_read_data_from_dsp(index, data, data_size, channel);
#else
	return aw_qcom_read_data_from_dsp(index, data, data_size, channel);
#endif
}

int aw_write_msg_to_dsp(int inline_id, void *data, int data_size, int channel)
{
#ifdef AW_MTK_PLATFORM
	return aw_mtk_write_msg_to_dsp(inline_id, data, data_size, channel);
#else
	return aw_qcom_write_msg_to_dsp(inline_id, data, data_size, channel);
#endif
}

int aw_read_msg_from_dsp(int inline_id, void *data, int data_size, int channel)
{
#ifdef AW_MTK_PLATFORM
	return aw_mtk_read_msg_from_dsp(inline_id, data, data_size, channel);
#else
	return aw_qcom_read_msg_from_dsp(inline_id, data, data_size, channel);
#endif
}

int aw_send_module_enable(void *buf, uint8_t type)
{
#ifdef AW_MTK_PLATFORM
	return aw_mtk_send_module_enable(buf, type);
#else
	return aw_qcom_send_module_enable(buf, type);
#endif
}

int aw_get_module_enable(void *buf, uint8_t type)
{
#ifdef AW_MTK_PLATFORM
	return aw_mtk_get_module_enable(buf, type);
#else
	return aw_qcom_get_module_enable(buf, type);
#endif
}

int aw_get_f0_q(struct f0_q_data *data, int data_size, int channel)
{
	int ret;

	ret = aw_read_msg_from_dsp(INLINE_PARAM_ID_F0_Q,
				(void *)data, data_size, channel);
	if (ret < 0) {
		pr_err("%s : get f0_q failed\n", __func__);
		return ret;
	}

	return 0;
}

int aw_get_algo_version(unsigned         int *data)
{
	int ret;

	ret = aw_read_msg_from_dsp(INLINE_PARAMS_ID_VERSION,
			(void *)data, sizeof(int), 0);
	if (ret < 0) {
		pr_err("%s : get version failed, ret is %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s : algo version is 0x%08x", __func__, *data);
	return 0;
}

int aw_dsp_copp_module_en(bool enable)
{
	int ret;
	int port_id = AFE_PORT_ID_AWDSP_RX;
	int module_id = AW_COPP_MODULE_ID;
	int param_id =  AW_COPP_MODULE_PARAMS_ID_EN;

	ret = aw_adm_param_enable(port_id, module_id, param_id, enable);
	if (ret) {
		return -EINVAL;
	}

	pr_info("%s: set skt %s", __func__, enable == 1 ? "enable" : "disable");
	return 0;
}


