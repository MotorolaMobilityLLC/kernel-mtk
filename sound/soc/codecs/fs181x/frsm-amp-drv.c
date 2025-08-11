// SPDX-License-Identifier: GPL-2.0+
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-12-23 File created.
 */

#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
//#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include "frsm-amp-drv.h"
#include "frsm-amp-class.h"
#if IS_ENABLED(CONFIG_SND_SOC_MSM_QDSP6V2_INTF)
#include "frsm-qcom-afe.h"
#endif
#if IS_ENABLED(CONFIG_SND_SOC_MTK_AUDIO_DSP)
#include "frsm-mtk-ipi.h"
#endif

#define FRSM_AMP_VERSION      "v1.0.1"
#define FRSM_PSPY_NAME        "battery"

static DEFINE_MUTEX(g_frsmamp_mutex);
static struct frsm_amp *g_frsm_amp;
static int (*g_frsm_notify_cb)(int cmd, void *buf, int size);

struct frsm_amp *frsm_amp_get_pdev(void)
{
	return g_frsm_amp;
}

#ifndef CONFIG_FRSM_ADSP_SUPPORT
static inline int frsm_send_adsp_params(struct device *dev,
		struct frsm_adsp_pkg *pkg)
{
	return -ENOTSUPP;
}

static inline int frsm_recv_adsp_params(struct device *dev,
		struct frsm_adsp_pkg *pkg)
{
	return -ENOTSUPP;
}
#endif

static int frsm_amp_get_dtype(struct frsm_amp *frsm_amp, struct frsm_argv *argv)
{
	uint16_t *data, val;
	int idx, ndev;
	int ret;

	ndev = frsm_amp->spkinfo.ndev;
	if (ndev < 2 || ndev > FRSM_DEV_MAX)
		return -ENOTSUPP;

	data = (uint16_t *)argv->buf;
	/* data[]:ndev,dtype1,dtype2,dtype3,... */
	*data = ndev;
	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_GET_DTYPE, argv);
	if (ret)
		return ret;

	for (idx = 1, val = 0; idx <= ndev; idx++) {
		dev_dbg(frsm_amp->dev, "dtype.%d:0x%04X\n", idx, data[idx]);
		if (data[idx] == 0)
			continue;
		val |= data[idx];
		if (val == data[idx])
			continue;
		return 0;
	}

	return -ENOTSUPP;
}

static int frsm_amp_send_dtype_params(struct frsm_amp *frsm_amp)
{
	int payload[FRSM_PAYLOAD_MAX] = { 0 };
	struct frsm_adsp_pkg *pkg;
	struct frsm_argv argv;
	int ndev;
	int ret;

	pkg = (struct frsm_adsp_pkg *)payload;
	pkg->module_id = AFE_MODULE_ID_FSADSP_RX;
	pkg->param_id = CAPI_V2_PARAM_FSADSP_DTYPE;
	argv.buf = pkg->buf;
	ndev = frsm_amp->spkinfo.ndev;
	ret = frsm_amp_get_dtype(frsm_amp, &argv);
	if (ret) {
		dev_info(frsm_amp->dev, "Skip to send dtype:%d\n", ret);
		return 0;
	}

	pkg->size = (ndev + 1) * sizeof(uint16_t);
	ret = frsm_send_adsp_params(frsm_amp->dev, pkg);
	if (ret)
		dev_err(frsm_amp->dev, "Failed to send dtype:%d\n", ret);

	return ret;
}

static int frsm_amp_send_calib_params(struct frsm_amp *frsm_amp)
{
	struct frsm_calib_params *params;
	int payload[FRSM_PAYLOAD_MAX];
	struct frsm_adsp_pkg *pkg;
	int i, dev_sum;
	int ret;

	if (!test_bit(FRSM_HAS_TX_VI, &frsm_amp->func))
		return -ENOTSUPP;

	if (!frsm_amp->stream_on)
		return 0;

	memset(payload, 0, sizeof(payload));
	pkg = (struct frsm_adsp_pkg *)payload;
	pkg->module_id = AFE_MODULE_ID_FSADSP_RX;
	pkg->param_id = CAPI_V2_PARAM_FSADSP_RE25;
	params = (struct frsm_calib_params *)pkg->buf;
	pkg->size = sizeof(*params);
	params->version = FRSM_CALIB_PARAMS_V1;
	params->ndev = frsm_amp->spkinfo.ndev;
	for (i = 0, dev_sum = 0; i < params->ndev; i++) {
		params->info[i].channel = i;
		params->info[i].re25 = frsm_amp->spkinfo.data[i].spkre;
		if (params->info[i].re25 == 0x3FFFF) // 64 << 12 - 1
			dev_sum++;
		dev_info(frsm_amp->dev, "send calre.%d:%d\n",
				i, params->info[i].re25);
	}

	if (dev_sum == params->ndev) {
		frsm_amp->calib_mode = true;
		return 0;
	}

	ret = frsm_send_adsp_params(frsm_amp->dev, pkg);
	frsm_amp->calib_mode = false;
	if (!ret)
		frsm_amp_send_dtype_params(frsm_amp);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

static int frsm_amp_send_spc_params(struct frsm_amp *frsm_amp)
{
	int payload[FRSM_PAYLOAD_MAX];
	struct frsm_adsp_pkg *pkg;
	struct frsm_batt *batt;
	int ret;

	if (!test_bit(FRSM_HAS_RX_SPC, &frsm_amp->func))
		return -ENOTSUPP;

	/* send batt prot info */
	pkg = (struct frsm_adsp_pkg *)payload;
	pkg->module_id = AFE_MODULE_ID_FSADSP_RX;
	pkg->param_id = CAPI_V2_PARAM_FSADSP_SYSTEM_INFO;
	pkg->size = sizeof(struct frsm_batt);

	batt = (struct frsm_batt *)pkg->buf;
	batt->batv = frsm_amp->batt.batv * 1000;
	batt->cap = frsm_amp->batt.cap;
	batt->tempr = frsm_amp->batt.tempr * 10;

	ret = frsm_send_adsp_params(frsm_amp->dev, pkg);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

static int frsm_amp_recv_live_params(struct frsm_amp *frsm_amp,
		struct spkr_info *info)
{
	int payload[FRSM_PAYLOAD_MAX];
	struct frsm_adsp_pkg *pkg;
	int *data;
	int i, ret;

	if (frsm_amp == NULL || info == NULL)
		return -EINVAL;

	if (!test_bit(FRSM_HAS_TX_VI, &frsm_amp->func))
		return -ENOTSUPP;

	if (info->ndev == 0)
		return -ENODEV;

	pkg = (struct frsm_adsp_pkg *)payload;
	pkg->module_id = AFE_MODULE_ID_FSADSP_RX;
	pkg->param_id  = CAPI_V2_PARAM_FSADSP_CALIB;
	pkg->size = 6 * sizeof(int) * info->ndev;
	ret = frsm_recv_adsp_params(frsm_amp->dev, pkg);
	if (ret)
		return ret;

	for (i = 0; i < info->ndev; i++) {
		data = pkg->buf + 6 * i;
		info->data[i].spkre = frsm_amp->spkinfo.data[i].spkre;
		if (frsm_amp->calib_mode)
			info->data[i].spkre = data[0];
		info->data[i].spkr0 = data[0];
		info->data[i].spkt0 = data[1];
		info->data[i].spkf0 = data[3];
		info->data[i].spkQ  = data[4];
		dev_info(frsm_amp->dev, "spk%d: %d,%d,%d,%d\n", i + 1,
				data[0], data[1], data[3], data[4]);
	}

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

int frsm_amp_set_fsalgo(struct frsm_amp *frsm_amp,
		struct frsm_adsp_pkg *pkg)
{
	int ret;

	if (frsm_amp == NULL || pkg == NULL)
		return -EINVAL;

	if (pkg->size <= 0)
		return -EINVAL;

	ret = frsm_send_adsp_params(frsm_amp->dev, pkg);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

int frsm_amp_notify(struct frsm_amp *frsm_amp, int event,
		struct frsm_argv *argv)
{
	if (frsm_amp == NULL || argv == NULL)
		return -EINVAL;

	if (g_frsm_notify_cb == NULL)
		return -ENOTSUPP;

	return g_frsm_notify_cb(event, argv->buf, argv->size);
}

int frsm_amp_set_tuning(struct frsm_amp *frsm_amp,
		struct frsm_argv *argv)
{
	int ret;

	if (frsm_amp == NULL || argv == NULL)
		return -EINVAL;

	if (argv->buf == NULL || argv->size < sizeof(char))
		return -EINVAL;

	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_SET_TUNING, argv);
	if (!ret)
		frsm_amp->is_tuning = !!*((char *)argv->buf);

	return ret;
}

int frsm_amp_set_calre(struct frsm_amp *frsm_amp, struct frsm_argv *argv)
{
	struct spkr_info *info;
	int ret;

	if (frsm_amp == NULL || argv == NULL)
		return -EINVAL;

	if (argv->buf == NULL || argv->size < sizeof(struct spkr_info))
		return -EINVAL;

	info = (struct spkr_info *)argv->buf;
	if (info->ndev != frsm_amp->spkinfo.ndev) {
		dev_err(frsm_amp->dev, "Invalid calre count:%d\n",
				info->ndev);
		return -EINVAL;
	}

	memcpy(&frsm_amp->spkinfo, argv->buf, sizeof(struct spkr_info));

	if (test_bit(FRSM_HAS_TX_VI, &frsm_amp->func))
		return frsm_amp_send_calib_params(frsm_amp);

	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_SET_CALRE, argv);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

int frsm_amp_get_livedata(struct frsm_amp *frsm_amp,
		struct frsm_argv *argv)
{
	int ret;

	if (frsm_amp == NULL || argv == NULL)
		return -EINVAL;

	if (argv->buf == NULL || argv->size < sizeof(struct spkr_info))
		return -EINVAL;

	if (test_bit(FRSM_HAS_RX_SPC, &frsm_amp->func))
		return -ENOTSUPP;

	if (test_bit(FRSM_HAS_TX_VI, &frsm_amp->func)) {
		ret = frsm_amp_recv_live_params(frsm_amp, &frsm_amp->spkinfo);
		memcpy(argv->buf, &frsm_amp->spkinfo, sizeof(struct spkr_info));
		return ret;
	}

	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_GET_LIVEDATA, argv);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

static int frsm_amp_set_batt(struct frsm_amp *frsm_amp)
{
	struct frsm_argv argv;
	int ret;

	if (frsm_amp == NULL || frsm_amp->dev == NULL)
		return -EINVAL;

	argv.buf = &frsm_amp->batt;
	argv.size = sizeof(frsm_amp->batt);
	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_SET_BATT, &argv);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

static int frsm_amp_get_mntren(struct frsm_amp *frsm_amp)
{
	struct frsm_argv argv;
	int mntr_en = 0;
	int ret;

	if (frsm_amp == NULL || frsm_amp->dev == NULL)
		return -EINVAL;

	if (!test_bit(FRSM_HAS_MNTR, &frsm_amp->func)) {
		frsm_amp->mntr_en = false;
		return -ENOTSUPP;
	}

	argv.buf = &mntr_en;
	argv.size = sizeof(mntr_en);
	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_GET_MNTREN, &argv);
	if (!ret && mntr_en)
		return 0;

	return -ENOTSUPP;
}

static int frsm_amp_get_ambient(struct frsm_amp *frsm_amp)
{
	union power_supply_propval prop;
	struct power_supply *psy;
	int ret;

	if (frsm_amp == NULL)
		return -EINVAL;

	psy = power_supply_get_by_name(FRSM_PSPY_NAME);
	if (psy == NULL) {
		dev_err(frsm_amp->dev, "Failed to get power supply!\n");
		return -EINVAL;
	}

	ret = power_supply_get_property(psy,
			POWER_SUPPLY_PROP_VOLTAGE_NOW, &prop);
	frsm_amp->batt.batv = DIV_ROUND_CLOSEST(prop.intval, 1000);

	ret |= power_supply_get_property(psy,
			POWER_SUPPLY_PROP_CAPACITY, &prop);
	frsm_amp->batt.cap = prop.intval;

	ret |= power_supply_get_property(psy,
			POWER_SUPPLY_PROP_TEMP, &prop);
	frsm_amp->batt.tempr = DIV_ROUND_CLOSEST(prop.intval, 10);

	power_supply_put(psy);
	dev_dbg(frsm_amp->dev, "batv:%d cap:%d tempr:%d\n",
			frsm_amp->batt.batv,
			frsm_amp->batt.cap,
			frsm_amp->batt.tempr);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

static int frsm_amp_smooth_batt(struct frsm_amp *frsm_amp)
{
	static struct frsm_batt batt;
	static int sum_count;

	if (frsm_amp == NULL)
		return -EINVAL;

	if (frsm_amp->batt.batv <= 0 || frsm_amp->batt.cap < 0)
		return -EINVAL;

	batt.batv += frsm_amp->batt.batv;
	batt.cap += frsm_amp->batt.cap;
	batt.tempr += frsm_amp->batt.tempr;

	if (++sum_count < frsm_amp->mntr_avg_count)
		return -EAGAIN;

	frsm_amp->batt.batv = batt.batv / sum_count;
	frsm_amp->batt.cap = batt.cap / sum_count;
	frsm_amp->batt.tempr = batt.tempr / sum_count;

	batt.batv = 0;
	batt.cap = 0;
	batt.tempr = 0;
	sum_count = 0;

	dev_dbg(frsm_amp->dev, "smooth batv:%d cap:%d tempr:%d\n",
			frsm_amp->batt.batv,
			frsm_amp->batt.cap,
			frsm_amp->batt.tempr);

	return 0;
}

int frsm_amp_prot_battery(struct frsm_amp *frsm_amp, bool restore)
{
	int ret;

	if (!frsm_amp->mntr_en && restore) { // disable monitor
		frsm_amp->batt.batv = 0xFFFF;
		frsm_amp->batt.cap = 0xFF;
		frsm_amp->batt.tempr = 0xFF;
	}

	if (test_bit(FRSM_HAS_RX_SPC, &frsm_amp->func))
		ret = frsm_amp_send_spc_params(frsm_amp);
	else
		ret = frsm_amp_set_batt(frsm_amp);

	return ret;
}

static void frsm_amp_prot_prepare(struct frsm_amp *frsm_amp, bool enable)
{
	dev_dbg(frsm_amp->dev, "prot prepare %s\n",
			enable ? "ON" : "OFF");

	if (enable && !frsm_amp->prot_prepared) {
		queue_delayed_work(frsm_amp->thread_wq,
				&frsm_amp->prepare_work, 0);
	} else {
		cancel_delayed_work_sync(&frsm_amp->prepare_work);
		frsm_amp->prot_prepared = false;
	}
}

static void frsm_amp_work_prot_prepare(struct work_struct *work)
{
	struct frsm_amp *frsm_amp;
	int ret;

	if (work == NULL)
		return;

	frsm_amp = container_of(work, struct frsm_amp, prepare_work.work);
	if (frsm_amp->prot_prepared)
		return;

	dev_dbg(frsm_amp->dev, "Do prot prepare work\n");

	ret  = frsm_amp_get_ambient(frsm_amp);
	ret |= frsm_amp_prot_battery(frsm_amp, true);

	if (test_bit(FRSM_HAS_TX_VI, &frsm_amp->func))
		ret |= frsm_amp_send_calib_params(frsm_amp);

	if (ret)
		dev_err(frsm_amp->dev, "Failed to set prot prepare:%d\n", ret);
	else
		frsm_amp->prot_prepared = true;
}

static void frsm_amp_work_monitor(struct work_struct *work)
{
	struct frsm_amp *frsm_amp;
	int ret;

	if (work == NULL)
		return;

	frsm_amp = container_of(work, struct frsm_amp, delay_work.work);
	ret = frsm_amp_get_mntren(frsm_amp);
	if (ret)
		goto mntr_restart;

	if (!frsm_amp->mntr_en && frsm_amp->batt.batv == 0xFFFF)
		goto mntr_restart;

	ret = frsm_amp_get_ambient(frsm_amp);
	if (ret || frsm_amp_smooth_batt(frsm_amp))
		goto mntr_restart;

	ret = frsm_amp_prot_battery(frsm_amp, true);

mntr_restart:
	queue_delayed_work(frsm_amp->thread_wq,
			&frsm_amp->delay_work,
			msecs_to_jiffies(frsm_amp->mntr_period));
}

int frsm_amp_mntr_switch(struct frsm_amp *frsm_amp, bool enable)
{
	bool state;

	if (frsm_amp == NULL)
		return -EINVAL;

	if (!test_bit(FRSM_HAS_MNTR, &frsm_amp->func))
		return 0;

	state = test_bit(EVENT_STAT_MNTR, &frsm_amp->state);
	if ((state ^ enable) == 0)
		return 0;

	if (enable) {
		if (!frsm_amp->stream_on)
			return 0;
		set_bit(EVENT_STAT_MNTR, &frsm_amp->state);
		queue_delayed_work(frsm_amp->thread_wq,
				&frsm_amp->delay_work,
				msecs_to_jiffies(frsm_amp->mntr_period));
	} else {
		clear_bit(EVENT_STAT_MNTR, &frsm_amp->state);
		cancel_delayed_work_sync(&frsm_amp->delay_work);
	}

	return 0;
}

void frsm_amp_register_notify_callback(int (*func)(
		int event, void *buf, int size))
{
	if (func == NULL) {
		pr_err("%s: Failed to set notify_cb\n", __func__);
		return;
	}

	mutex_lock(&g_frsmamp_mutex);
	g_frsm_notify_cb = func;
	mutex_unlock(&g_frsmamp_mutex);
}

int frsm_amp_send_event(int event, void *buf, int size)
{
	struct frsm_amp *frsm_amp = frsm_amp_get_pdev();
	struct frsm_argv argv;
	int ret;

	if (frsm_amp == NULL)
		return -EINVAL;

	if (test_bit(event, &frsm_amp->state))
		return 0;

	mutex_lock(&g_frsmamp_mutex);
	set_bit(event, &frsm_amp->state);
	dev_info(frsm_amp->dev, "event:%x state:%lx\n",
			event, frsm_amp->state);

	switch (event) {
	case EVENT_STREAM_ON:
		clear_bit(EVENT_STREAM_OFF, &frsm_amp->state);
		frsm_amp->stream_on = true;
		frsm_amp_prot_prepare(frsm_amp, true);
		ret = frsm_amp_mntr_switch(frsm_amp, true);
		break;
	case EVENT_STREAM_OFF:
		frsm_amp->stream_on = false;
		frsm_amp->calib_mode = false;
		frsm_amp_prot_prepare(frsm_amp, false);
		ret = frsm_amp_mntr_switch(frsm_amp, false);
		clear_bit(EVENT_STREAM_ON, &frsm_amp->state);
		argv.buf = NULL;
		argv.size = 0;
		ret = frsm_amp_notify(frsm_amp, EVENT_AMP_MUTE_SYNC, &argv);
		break;
	case EVENT_GET_DTYPE:
		argv.buf = buf;
		argv.size = size;
		ret = frsm_amp_get_dtype(frsm_amp, &argv);
		clear_bit(event, &frsm_amp->state);
		break;
	default:
		ret = -ENOTSUPP;
		break;
	}
	mutex_unlock(&g_frsmamp_mutex);

	return ret;
}

int frsm_amp_init_dev(int spkid, bool force)
{
	struct frsm_mode_params params;
	struct frsm_amp *frsm_amp;
	struct frsm_argv argv;
	int ret;

	frsm_amp = frsm_amp_get_pdev();
	if (frsm_amp == NULL)
		return -EINVAL;

	if (spkid < 0 || spkid > FRSM_DEV_MAX) {
		dev_err(frsm_amp->dev, "init: Invalid spkid:%d\n", spkid);
		return -EINVAL;
	}

	mutex_lock(&g_frsmamp_mutex);
	dev_info(frsm_amp->dev, "spkid:%d init:%d\n", spkid, force);
	params.spkid = spkid;
	params.mode = force;
	argv.buf = &params;
	argv.size = sizeof(params);
	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_INIT_DEV, &argv);
	mutex_unlock(&g_frsmamp_mutex);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

int frsm_amp_set_scene(int spkid, int scene_id)
{
	struct frsm_mode_params params;
	struct frsm_amp *frsm_amp = frsm_amp_get_pdev();
	struct frsm_argv argv;
	int ret;

	if (frsm_amp == NULL)
		return -EINVAL;

	if (spkid < 0 || spkid > FRSM_DEV_MAX) {
		dev_err(frsm_amp->dev, "scene: Invalid spkid:%d\n", spkid);
		return -EINVAL;
	}

	mutex_lock(&g_frsmamp_mutex);
	dev_info(frsm_amp->dev, "id:%d scene:%d\n", spkid, scene_id);
	params.spkid = spkid;
	params.mode = scene_id;
	argv.buf = &params;
	argv.size = sizeof(params);
	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_SET_SCENE, &argv);
	mutex_unlock(&g_frsmamp_mutex);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

int frsm_amp_spk_switch(int spkid, bool on)
{
	struct frsm_mode_params params;
	struct frsm_amp *frsm_amp = frsm_amp_get_pdev();
	struct frsm_argv argv;
	int ret;

	if (frsm_amp == NULL)
		return -EINVAL;

	if (spkid < 0 || spkid > FRSM_DEV_MAX) {
		dev_err(frsm_amp->dev, "switch: Invalid spkid:%d\n", spkid);
		return -EINVAL;
	}

	mutex_lock(&g_frsmamp_mutex);
	dev_info(frsm_amp->dev, "id:%d on:%d\n", spkid, on);
	params.spkid = spkid;
	params.mode = on;
	argv.buf = &params;
	argv.size = sizeof(params);
	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_SPK_SWITCH, &argv);
	mutex_unlock(&g_frsmamp_mutex);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

static int frsm_amp_parse_dts(struct frsm_amp *frsm_amp)
{
	struct device_node *np;
	int ret;

	if (frsm_amp == NULL)
		return -EINVAL;

	np = frsm_amp->dev->of_node;

	ret = of_property_read_s32(np, "frsm,amp-ndev",
			&frsm_amp->spkinfo.ndev);
	if (ret) {
		dev_err(frsm_amp->dev, "Failed to lookup amp-ndev\n");
		return ret;
	}

	if (of_property_read_bool(np, "frsm,has-tx-vi"))
		set_bit(FRSM_HAS_TX_VI, &frsm_amp->func);

	if (of_property_read_bool(np, "frsm,has-rx-spc"))
		set_bit(FRSM_HAS_RX_SPC, &frsm_amp->func);

	if (of_property_read_bool(np, "frsm,mntr-enable")) {
		set_bit(FRSM_HAS_MNTR, &frsm_amp->func);
		frsm_amp->mntr_en = true;
	}

	ret = of_property_read_s32(np, "frsm,mntr-period",
			&frsm_amp->mntr_period);
	if (ret)
		frsm_amp->mntr_period = 2000; /* 2s */

	ret = of_property_read_s32(np, "frsm,mntr-avg-count",
			&frsm_amp->mntr_avg_count);
	if (ret)
		frsm_amp->mntr_avg_count = 0;

	return 0;
}

static int frsm_amp_probe(struct platform_device *pdev)
{
	struct frsm_amp *frsm_amp;
	int ret = 0;

	dev_info(&pdev->dev, "Version: %s\n", FRSM_AMP_VERSION);

	frsm_amp = devm_kzalloc(&pdev->dev,
			sizeof(struct frsm_amp), GFP_KERNEL);
	if (frsm_amp == NULL)
		return -ENOMEM;

	frsm_amp->dev = &pdev->dev;
	platform_set_drvdata(pdev, frsm_amp);

	ret = frsm_amp_parse_dts(frsm_amp);
	if (ret)
		return ret;

	frsm_amp->thread_wq = create_singlethread_workqueue(
			dev_name(frsm_amp->dev));
	INIT_DELAYED_WORK(&frsm_amp->prepare_work, frsm_amp_work_prot_prepare);
	INIT_DELAYED_WORK(&frsm_amp->delay_work, frsm_amp_work_monitor);

	g_frsm_amp = frsm_amp;
	frsm_amp_class_init(frsm_amp);

	FRSM_FUNC_EXIT(frsm_amp->dev, ret);
	return ret;
}

static int frsm_amp_remove(struct platform_device *pdev)
{
	struct frsm_amp *frsm_amp = platform_get_drvdata(pdev);

	if (frsm_amp == NULL)
		return 0;

	//frsm_amp_misc_deinit(frsm_amp);
	frsm_amp_class_deinit(frsm_amp);

	return 0;
}

static const struct of_device_id frsm_amp_of_match[] = {
	{ .compatible = "foursemi,frsm-amp", },
	{},
};
MODULE_DEVICE_TABLE(of, frsm_amp_of_match);

struct platform_driver frsm_amp_driver = {
	.driver = {
		.name = "frsm-amp",
		.of_match_table = frsm_amp_of_match,
	},
	.probe = frsm_amp_probe,
	.remove = frsm_amp_remove,
};

#ifndef FRSM_DRV_2IN1_SUPPORT
module_platform_driver(frsm_amp_driver);

MODULE_AUTHOR("FourSemi SW <support@foursemi.com>");
MODULE_DESCRIPTION("FourSemi Amp platform driver");
MODULE_VERSION(FRSM_AMP_VERSION);
MODULE_LICENSE("GPL");
#endif
