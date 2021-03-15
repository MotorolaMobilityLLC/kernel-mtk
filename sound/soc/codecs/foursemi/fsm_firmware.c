/**
 * Copyright (C) Fourier Semiconductor Inc. 2016-2020. All rights reserved.
 * 2019-02-22 File created.
 */

#if defined(CONFIG_FSM_FIRMWARE)
#include "fsm_public.h"
#include <linux/firmware.h>
#include <linux/slab.h>

#define FS_NONDSP_FW_NAME "fs_algo.preset"
static struct fsm_firmware *g_fs_algo_fw;
static int g_fsm_fw_init;

/*
#ifdef FSM_UNUSED_CODE
static void *fsm_devm_kzalloc(struct device *dev, void *buf, size_t size)
{
	char *devm_buf = devm_kzalloc(dev, size, GFP_KERNEL);

	if (!devm_buf) {
		return devm_buf;
	} else {
		memcpy(devm_buf, buf, size);
	}

	return devm_buf;
}

static void fsm_devm_kfree(struct device *dev, void *buf)
{
	if (buf) {
		devm_kfree(dev, buf);
		buf = NULL;
	}
}
#endif
*/

static void fsm_firmware_inited(const struct firmware *cont, void *context)
{
	struct device *dev = (struct device *)context;
	int ret;

	if (dev == NULL || cont == NULL) {
		pr_err("bad parameter");
		return;
	}

	pr_info("size: %zu", cont->size);
	fsm_mutex_lock();
	ret = fsm_parse_preset(cont->data, (uint32_t)cont->size);
	fsm_mutex_unlock();
	release_firmware(cont);
	if (ret) {
		pr_err("parse firmware fail: %d", ret);
		g_fsm_fw_init = 0;
	}
}

#ifdef CONFIG_FSM_NONDSP
struct fsm_firmware *fsm_algo_get_fw(void)
{
	return g_fs_algo_fw;
}

static void fsm_algo_fw_inited(const struct firmware *cont, void *context)
{
	struct device *dev = (struct device *)context;
	struct algo_preset_hdr *preset_hdr;
	struct fsm_firmware *fs_algo_fw;
	char string[32] = { 0 };

	if (dev == NULL || cont == NULL) {
		pr_err("bad parameter");
		return;
	}
	pr_info("size: %zu", cont->size);
	fs_algo_fw = fsm_alloc_mem(cont->size);
	if (fs_algo_fw) {
		fs_algo_fw->size = cont->size;
		memcpy(fs_algo_fw->data, cont->data, cont->size);
		pr_info("load firmware done");
	} else {
		pr_info("alloc memory fail");
	}
	release_firmware(cont);
	g_fs_algo_fw = fs_algo_fw;
	preset_hdr = (struct algo_preset_hdr *)fs_algo_fw->data;
	strncpy(string, preset_hdr->customer, 8);
	pr_info("customer: %s", string);
	strncpy(string, preset_hdr->project, 8);
	pr_info("project : %s", string);
}
#endif

int fsm_firmware_init(char *fw_name)
{
	struct device *dev;
	int ret;

	if (fw_name == NULL) {
		pr_err("invalid firmware name");
		return -EINVAL;
	}
	if (fsm_get_presets() || g_fsm_fw_init) {
		return MODULE_INITED;
	}
	dev = fsm_get_pdev();
	if (dev == NULL) {
		pr_err("invalid device");
		return -EINVAL;
	}

	pr_info("loading %s in nowait mode", fw_name);
	ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
			fw_name, dev, GFP_KERNEL,
			dev, fsm_firmware_inited);
	if (ret) {
		pr_err("request %s failed: %d", fw_name, ret);
		return ret;
	}
#ifdef CONFIG_FSM_NONDSP
	pr_info("loading %s in nowait mode", FS_NONDSP_FW_NAME);
	ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
			FS_NONDSP_FW_NAME, dev, GFP_KERNEL,
			dev, fsm_algo_fw_inited);
	if (ret) {
		pr_err("request %s failed: %d", FS_NONDSP_FW_NAME, ret);
		return ret;
	}
#endif
	g_fsm_fw_init = 1;

	return ret;
}

int fsm_firmware_init_sync(char *fw_name)
{
	fsm_config_t *cfg = fsm_get_config();
	const struct firmware *fw_cont;
	struct device *dev;
	int ret;

	if (fw_name == NULL) {
		pr_err("invalid firmware name");
		return -EINVAL;
	}
	if (cfg->force_fw) {
		pr_info("force loading");
		cfg->force_fw = false;
		fsm_set_presets(NULL);
		g_fsm_fw_init = 0;
	}
	if (fsm_get_presets() || g_fsm_fw_init) {
		return MODULE_INITED;
	}
	dev = fsm_get_pdev();
	if (dev == NULL) {
		pr_err("invalid device");
		return -EINVAL;
	}

	pr_info("loading %s in sync mode", fw_name);
	ret = request_firmware(&fw_cont, fw_name, dev);
	if (ret) {
		pr_err("request %s failed: %d", fw_name, ret);
		return ret;
	}
	ret = fsm_parse_preset(fw_cont->data, (uint32_t)fw_cont->size);
	release_firmware(fw_cont);
#ifdef CONFIG_FSM_NONDSP
	ret = request_firmware(&fw_cont, FS_NONDSP_FW_NAME, dev);
	if (ret) {
		pr_err("request %s failed: %d", FS_NONDSP_FW_NAME, ret);
	} else {
		fsm_algo_fw_inited(fw_cont, dev);
	}
#endif
	if (ret) {
		pr_err("parse firmware fail: %d", ret);
		return ret;
	}
	g_fsm_fw_init = 1;

	return ret;
}

void fsm_firmware_deinit(void)
{
	fsm_free_mem(g_fs_algo_fw);
	g_fsm_fw_init = 0;
}
#endif
