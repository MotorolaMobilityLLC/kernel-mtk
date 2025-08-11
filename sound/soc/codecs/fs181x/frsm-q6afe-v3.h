/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2024-01-24 File created.
 */

#ifndef __FRSM_Q6AFE_V3_H__
#define __FRSM_Q6AFE_V3_H__

#include <dsp/apr_audio-v2.h>
#include <dsp/q6audio-v2.h>
#include <dsp/q6afe-v2.h>

#define AFE_MODULE_ID_FSADSP_TX (0x10001110)
#define AFE_MODULE_ID_FSADSP_RX (0x10001111)
#define AFE_TOPO_ID_FSADSP_TX   (0x11111112)
#define AFE_TOPO_ID_FSADSP_RX   (0x11111111)

#define FRSM_APR_CHUNK_SIZE     (64*sizeof(int))
#define FRSM_ADSP_DELAY_US      (10000)
#define FRSM_ADSP_RETRY_MAX     (20)

struct frsm_resp_params {
	uint16_t size;
	void *params;
};

static struct frsm_resp_params g_frsm_resp;

int afe_get_topology(int port_id);
static int q6afe_get_params(u16 port_id, struct mem_mapping_hdr *mem_hdr,
			    struct param_hdr_v3 *param_hdr);
static int q6afe_set_params(u16 port_id, int index,
			    struct mem_mapping_hdr *mem_hdr,
			    u8 *packed_param_data, u32 packed_data_size);

static int afe_frsm_callback(int opcode, void *payload, uint16_t size)
{
	uint32_t *params = payload;
	int hdr_size;

	if ((params[1] != AFE_MODULE_ID_FSADSP_RX)
		&& (params[1] != AFE_MODULE_ID_FSADSP_TX))
		return -ENOTSUPP;

	if (params[0] != 0) {
		pr_err("%s Invalid status:%d\n", __func__, params[0]);
		return -EINVAL;
	}

	pr_debug("%s opcode:%x status:%d size:%d\n",
			__func__, opcode, params[0], size);
	if (g_frsm_resp.params == NULL || g_frsm_resp.size <= 0) {
		pr_err("%s Not have resp buffer\n", __func__);
		return -ENOMEM;
	}

	switch (opcode) {
	case AFE_PORT_CMDRSP_GET_PARAM_V2:
		hdr_size = sizeof(uint32_t) + sizeof(struct param_hdr_v1);
		break;
	case AFE_PORT_CMDRSP_GET_PARAM_V3:
		hdr_size = sizeof(uint32_t) + sizeof(struct param_hdr_v3);
		break;
	default:
		pr_err("%s invalid opcode:%x\n", __func__, opcode);
		return -EINVAL;
	}

	if (g_frsm_resp.size > size - hdr_size)
		g_frsm_resp.size = size - hdr_size;

	memcpy(g_frsm_resp.params, (uint8_t *)payload + hdr_size,
			g_frsm_resp.size);

	return 0;
}

static int afe_get_frsm_port_id(int mid)
{
	int tid, port;
	int i;

	if (mid == AFE_MODULE_ID_FSADSP_RX)
		port = this_afe.vi_rx_port;
	else
		port = this_afe.vi_tx_port;

	for (i = 0; i < FRSM_ADSP_RETRY_MAX; i++) {
		tid = afe_get_topology(port);
		if (tid == AFE_TOPO_ID_FSADSP_RX)
			return port;
		if (tid == AFE_TOPO_ID_FSADSP_TX)
			return port;
		if (tid > 0)
			break;
		usleep_range(FRSM_ADSP_DELAY_US, FRSM_ADSP_DELAY_US + 10);
	}

	return -ETIMEDOUT;
}

int afe_send_frsm_params(int mid, int pid, void *buf, size_t size)
{
	struct param_hdr_v3 param_hdr;
	uint8_t *packed_param_data;
	int packed_data_size;
	int index, port_id;
	int ret;

	if (size > FRSM_APR_CHUNK_SIZE) {
		pr_err("%s invalid size:%zu\n", __func__, size);
		return -EINVAL;
	}

	port_id = afe_get_frsm_port_id(mid);
	index = afe_get_port_index(port_id);
	if ((index < 0) || (index >= AFE_MAX_PORTS)) {
		pr_err("%s invalid port:%d index:%d\n",
				__func__, port_id, index);
		return -EINVAL;
	}

	packed_data_size = sizeof(union param_hdrs) + size;
	packed_param_data = kzalloc(packed_data_size, GFP_KERNEL);
	if (packed_param_data == NULL)
		return -ENOMEM;

	param_hdr.module_id = mid;
	param_hdr.instance_id = INSTANCE_ID_0;
	param_hdr.param_id = pid;
	param_hdr.param_size = size;

	ret = q6common_pack_pp_params(packed_param_data,
			&param_hdr, buf, &packed_data_size);
	if (ret) {
		pr_err("%s: Failed to pack data:%d\n", __func__, ret);
		kfree(packed_param_data);
		return ret;
	}

	ret = q6afe_set_params(port_id, index, NULL,
			packed_param_data, packed_data_size);
	kfree(packed_param_data);
	if (ret)
		pr_err("%s Failed to send mid:%x pid:%x\n", __func__, mid, pid);

	return ret;
}
EXPORT_SYMBOL(afe_send_frsm_params);

int afe_recv_frsm_params(int mid, int pid, void *buf, size_t size)
{
	struct param_hdr_v3 param_hdr;
	int index, port_id;
	int ret;

	if (size > FRSM_APR_CHUNK_SIZE) {
		pr_err("%s invalid size:%zu\n", __func__, size);
		return -EINVAL;
	}

	port_id = afe_get_frsm_port_id(mid);
	index = afe_get_port_index(port_id);
	if ((index < 0) || (index >= AFE_MAX_PORTS)) {
		pr_err("%s invalid port:%d index:%d\n",
				__func__, port_id, index);
		return -EINVAL;
	}

	param_hdr.module_id = mid;
	param_hdr.instance_id = INSTANCE_ID_0;
	param_hdr.param_id = pid;
	param_hdr.param_size = size;

	g_frsm_resp.params = kzalloc(size, GFP_KERNEL);
	if (g_frsm_resp.params == NULL)
		return -ENOMEM;

	g_frsm_resp.size = size;
	ret = q6afe_get_params(port_id, NULL, &param_hdr);
	if (!ret)
		memcpy(buf, g_frsm_resp.params, g_frsm_resp.size);
	else
		pr_err("%s Failed to recv mid:%x pid:%x\n", __func__, mid, pid);

	kfree(g_frsm_resp.params);
	g_frsm_resp.params = NULL;
	g_frsm_resp.size = 0;

	return ret;
}
EXPORT_SYMBOL(afe_recv_frsm_params);

#endif // __FRSM_Q6AFE_V3_H__
