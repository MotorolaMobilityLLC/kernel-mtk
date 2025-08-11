/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2024-01-17 File created.
 */

#ifndef __FRSM_QCOM_AFE_H__
#define __FRSM_QCOM_AFE_H__

#define CONFIG_FRSM_ADSP_SUPPORT

extern int afe_send_frsm_params(int mid, int pid, void *buf, size_t size);
extern int afe_recv_frsm_params(int mid, int pid, void *buf, size_t size);

static int frsm_send_adsp_params(struct device *dev, struct frsm_adsp_pkg *pkg)
{
	int ret;

	if (dev == NULL || pkg == NULL) {
		pr_err("%s: Bad parameters\n", __func__);
		return -EINVAL;
	}

	dev_dbg(dev, "send pid:0x%x, size:%d", pkg->param_id, pkg->size);
	ret = afe_send_frsm_params(pkg->module_id, pkg->param_id,
			(void *)pkg->buf, pkg->size);
	if (ret)
		dev_err(dev, "Failed to send params:%d\n", ret);

	return ret;
}

static int frsm_recv_adsp_params(struct device *dev, struct frsm_adsp_pkg *pkg)
{
	int ret;

	if (dev == NULL || pkg == NULL) {
		pr_err("%s: Bad parameters\n", __func__);
		return -EINVAL;
	}

	dev_dbg(dev, "recv pid:0x%x, size:%d", pkg->param_id, pkg->size);
	ret = afe_recv_frsm_params(pkg->module_id, pkg->param_id,
			(void *)pkg->buf, pkg->size);
	if (ret)
		dev_err(dev, "Failed to recv params:%d\n", ret);

	return ret;
}

static inline void frsm_adsp_unused_func(void)
{
	frsm_recv_adsp_params(NULL, NULL);
}

#endif // __FRSM_QCOM_AFE_H__
