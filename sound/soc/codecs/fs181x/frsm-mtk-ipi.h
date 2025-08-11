/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2024-01-17 File created.
 */

#ifndef __FRSM_MTK_IPI_H__
#define __FRSM_MTK_IPI_H__

#include <linux/delay.h>
#include "frsm-dev.h"
#include "frsm-amp-drv.h"

#define CONFIG_FRSM_ADSP_SUPPORT

extern int mtk_spk_send_ipi_buf_to_dsp(
		void *data_buffer, uint32_t data_size);
extern int mtk_spk_recv_ipi_buf_from_dsp(
		int8_t *buffer, int16_t size, uint32_t *buf_len);

static int frsm_send_adsp_params(struct device *dev, struct frsm_adsp_pkg *pkg)
{
	int buf_size, retry = 0;
	int *buf;
	int ret;

	if (dev == NULL || pkg == NULL) {
		pr_err("%s: Bad parameters\n", __func__);
		return -EINVAL;
	}

	dev_dbg(dev, "send pid:0x%x, size:%d", pkg->param_id, pkg->size);
	buf_size = pkg->size + sizeof(pkg->param_id);
	buf = kzalloc(buf_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	*buf = pkg->param_id;
	memcpy(buf + 1, pkg->buf, pkg->size);
	do {
		ret = mtk_spk_send_ipi_buf_to_dsp((void *)buf, buf_size);
		if (!ret)
			break;
		FRSM_DELAY_MS(10);
	} while (retry++ < FRSM_WAIT_TIMES);

	kfree(buf);
	if (ret)
		dev_err(dev, "Failed to send params:%d\n", ret);

	return ret;
}

static int frsm_recv_adsp_params(struct device *dev, struct frsm_adsp_pkg *pkg)
{
	int buf_size, rcv_size, retry = 0;
	int *buf;
	int ret;

	if (dev == NULL || pkg == NULL) {
		pr_err("%s: Bad parameters\n", __func__);
		return -EINVAL;
	}

	dev_dbg(dev, "recv pid:0x%x, size:%d", pkg->param_id, pkg->size);
	buf_size = pkg->size + sizeof(pkg->param_id);
	buf = kzalloc(buf_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	*buf = pkg->param_id;
	do {
		ret = mtk_spk_send_ipi_buf_to_dsp((void *)buf, sizeof(*buf));
		if (ret) {
			FRSM_DELAY_MS(10);
			continue;
		}
		ret = mtk_spk_recv_ipi_buf_from_dsp((void *)buf,
				buf_size, &rcv_size);
		if (!ret)
			break;
		FRSM_DELAY_MS(10);
	} while (retry++ < FRSM_WAIT_TIMES);

	if (ret) {
		kfree(buf);
		dev_err(dev, "Failed to recv params:%d\n", ret);
		return ret;
	}

	if (pkg->size > rcv_size)
		pkg->size = rcv_size;

	memcpy(pkg->buf, buf, pkg->size);
	kfree(buf);

	return 0;
}

#endif // __FRSM_MTK_IPI_H__
