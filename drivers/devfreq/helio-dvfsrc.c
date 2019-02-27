/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/devfreq.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>

#include "governor.h"

#include <mt-plat/aee.h>
#include <spm/mtk_spm.h>

#include "helio-dvfsrc.h"
#include "mtk_dvfsrc_reg.h"

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#include <sspm_ipi.h>
#include <sspm_ipi_pin.h>
#include "helio-dvfsrc-ipi.h"
#endif

static struct helio_dvfsrc *dvfsrc;

#define DVFSRC_REG(offset) (dvfsrc->regs + offset)
#define DVFSRC_SRAM_REG(offset) (dvfsrc->sram_regs + offset)

void dvfsrc_write(u32 offset, u32 val)
{
	writel(val, DVFSRC_REG(offset));
}

u32 dvfsrc_read(u32 offset)
{
	return readl(DVFSRC_REG(offset));
}

void dvfsrc_rmw(u32 offset, u32 val, u32 mask, u32 shift)
{
	dvfsrc_write(offset, (dvfsrc_read(offset) & ~(mask << shift)) \
			| (val << shift));
}

#define dvfsrc_sram_write(offset, val) \
	writel(val, DVFSRC_SRAM_REG(offset))

#define dvfsrc_sram_read(offset) \
	readl(DVFSRC_SRAM_REG(offset))

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
void dvfsrc_update_sspm_vcore_opp_table(int opp, unsigned int vcore_uv)
{
	struct qos_data qos_d;

	qos_d.cmd = QOS_IPI_VCORE_OPP;
	qos_d.u.vcore_opp.opp = opp;
	qos_d.u.vcore_opp.vcore_uv = vcore_uv;

	qos_ipi_to_sspm_command(&qos_d, 3);
}

void dvfsrc_update_sspm_ddr_opp_table(int opp, unsigned int ddr_khz)
{
	struct qos_data qos_d;

	qos_d.cmd = QOS_IPI_DDR_OPP;
	qos_d.u.ddr_opp.opp = opp;
	qos_d.u.ddr_opp.ddr_khz = ddr_khz;

	qos_ipi_to_sspm_command(&qos_d, 3);
}

#endif

void helio_dvfsrc_enable(int dvfsrc_en)
{
	mutex_lock(&dvfsrc->devfreq->lock);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, dvfsrc_en,
			DVFSRC_EN_MASK, DVFSRC_EN_SHIFT);
	dvfsrc->enabled = dvfsrc_en;

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	helio_dvfsrc_sspm_ipi_init(dvfsrc_en);
#endif
	mutex_unlock(&dvfsrc->devfreq->lock);
}

static void helio_dvfsrc_pwrap_cmd(void)
{
}

static void dvfsrc_opp_table_init(void)
{
	int i;

	mutex_lock(&dvfsrc->devfreq->lock);

	for (i = 0; i < VCORE_DVFS_OPP_NUM; i++) {
		dvfsrc->opp_table[i].vcore_uv = get_vcore_uv(i);
		dvfsrc->opp_table[i].ddr_khz = get_ddr_khz(i);

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
		dvfsrc_update_sspm_vcore_opp_table(i,
				dvfsrc->opp_table[i].vcore_uv);
		dvfsrc_update_sspm_ddr_opp_table(i,
				dvfsrc->opp_table[i].ddr_khz);
#endif

		pr_info("opp %u: vcore_uv: %u, ddr_khz: %u\n", i,
				dvfsrc->opp_table[i].vcore_uv,
				dvfsrc->opp_table[i].ddr_khz);
	}

	helio_dvfsrc_pwrap_cmd();
	mutex_unlock(&dvfsrc->devfreq->lock);
}

static int helio_dvfsrc_platform_init(void)
{
	vcore_volt_init();
	dvfsrc_opp_table_init();

/* ToDo: framebuffer notifier for md table */

	if (!spm_load_firmware_status()) {
		pr_err("SPM FIRMWARE IS NOT READY\n");
		return -ENODEV;
	}

	mtk_spmfw_init();

	dvfsrc->init_config = dvfsrc_get_init_conf();

	return 0;
}

int dvfsrc_get_bw(int type)
{
	int ret = 0;
	int i;

	mutex_lock(&dvfsrc->devfreq->lock);
	if (type == QOS_TOTAL_AVE) {
		for (i = 0; i < QOS_TOTAL_BW_BUF_SIZE; i++)
			ret += dvfsrc_sram_read(QOS_TOTAL_BW_BUF(i));
		ret /= QOS_TOTAL_BW_BUF_SIZE;
	} else
		ret = dvfsrc_sram_read(QOS_TOTAL_BW + 4 * type);
	mutex_unlock(&dvfsrc->devfreq->lock);

	return ret;
}

int get_vcore_dvfs_level(void)
{
	int ret = 0;

	mutex_lock(&dvfsrc->devfreq->lock);

	ret = dvfsrc_read(DVFSRC_LEVEL) >> CURRENT_LEVEL_SHIFT;

	mutex_unlock(&dvfsrc->devfreq->lock);
	return ret;
}

int is_dvfsrc_enabled(void)
{
	if (dvfsrc)
		return dvfsrc->enabled;

	return 0;
}

static void dvfsrc_set_sw_bw(int type, int data)
{
	mutex_lock(&dvfsrc->devfreq->lock);
	switch (type) {
	case PM_QOS_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_0, data / 100);
		break;
	case PM_QOS_CPU_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_1, data / 100);
		break;
	case PM_QOS_GPU_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_2, data / 100);
		break;
	case PM_QOS_MM_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_3, data / 100);
		break;
	case PM_QOS_OTHER_MEMORY_BANDWIDTH:
		dvfsrc_write(DVFSRC_SW_BW_4, data / 100);
		break;
	default:
		break;
	}
	mutex_unlock(&dvfsrc->devfreq->lock);
}

static void dvfsrc_set_sw_req(int data, int mask, int shift)
{
	mutex_lock(&dvfsrc->devfreq->lock);
	dvfsrc_rmw(DVFSRC_SW_REQ, data, mask, shift);
	mutex_unlock(&dvfsrc->devfreq->lock);
}

static void dvfsrc_set_vcore_request2(int data, int mask, int shift)
{
	mutex_lock(&dvfsrc->devfreq->lock);
	dvfsrc_rmw(DVFSRC_VCORE_REQUEST2, data, mask, shift);
	mutex_unlock(&dvfsrc->devfreq->lock);
}

static void dvfsrc_release_force(void)
{
	mutex_lock(&dvfsrc->devfreq->lock);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 0,
			FORCE_EN_TAR_MASK, FORCE_EN_TAR_SHIFT);
	dvfsrc_write(DVFSRC_FORCE, 0);
	mutex_unlock(&dvfsrc->devfreq->lock);
}

static void dvfsrc_set_force_start(int data)
{
	mutex_lock(&dvfsrc->devfreq->lock);
	dvfsrc_write(DVFSRC_FORCE, data);
	dvfsrc_rmw(DVFSRC_BASIC_CONTROL, 1,
			FORCE_EN_TAR_MASK, FORCE_EN_TAR_SHIFT);
	mutex_unlock(&dvfsrc->devfreq->lock);
}

static void dvfsrc_set_force_end(void)
{
	mutex_lock(&dvfsrc->devfreq->lock);
	/* dvfsrc_write(DVFSRC_FORCE, 0); */
	mutex_unlock(&dvfsrc->devfreq->lock);
}

char *dvfsrc_dump_reg(char *ptr)
{
	char buf[1024];

	memset(buf, '\0', sizeof(buf));
	get_opp_info(buf);
	if (ptr)
		ptr += sprintf(ptr, "%s\n", buf);
	else
		pr_info("%s\n", buf);

	memset(buf, '\0', sizeof(buf));
	get_dvfsrc_reg(buf);
	if (ptr)
		ptr += sprintf(ptr, "%s\n", buf);
	else
		pr_info("%s\n", buf);

	memset(buf, '\0', sizeof(buf));
	get_spm_reg(buf);
	if (ptr)
		ptr += sprintf(ptr, "%s\n", buf);
	else
		pr_info("%s\n", buf);

	return ptr;
}

static struct devfreq_dev_profile helio_devfreq_profile = {
	.polling_ms	= 0,
};

static void helio_dvfsrc_reg_config(struct reg_config *config)
{
	int idx = 0;

	mutex_lock(&dvfsrc->devfreq->lock);
	while (config[idx].offset != -1) {
		dvfsrc_write(config[idx].offset, config[idx].val);
		idx++;
	}
	mutex_unlock(&dvfsrc->devfreq->lock);
}

static int helio_governor_event_handler(struct devfreq *devfreq,
					unsigned int event, void *data)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = dev_get_drvdata(devfreq->dev.parent);

	switch (event) {
	case DEVFREQ_GOV_SUSPEND:
		break;

	case DEVFREQ_GOV_RESUME:
		break;

	default:
		break;
	}
	return 0;
}

static struct devfreq_governor helio_dvfsrc_governor = {
	.name = "helio_dvfsrc",
	.event_handler = helio_governor_event_handler,
};

static int commit_data(struct helio_dvfsrc *dvfsrc, int type, int data)
{
	int ret = 0;
	int level = 0;

	if (!dvfsrc->enabled)
		return 0;

	mtk_spmfw_init();

	switch (type) {
	case PM_QOS_MEMORY_BANDWIDTH:
	case PM_QOS_CPU_MEMORY_BANDWIDTH:
	case PM_QOS_GPU_MEMORY_BANDWIDTH:
	case PM_QOS_MM_MEMORY_BANDWIDTH:
	case PM_QOS_OTHER_MEMORY_BANDWIDTH:
		dvfsrc_set_sw_bw(type, data);
		break;
	case PM_QOS_DDR_OPP:
		if (data >= DDR_OPP_NUM || data < 0)
			data = DDR_OPP_NUM - 1;

		level = DDR_OPP_NUM - data - 1;
		dvfsrc_set_sw_req(level, EMI_SW_AP_MASK, EMI_SW_AP_SHIFT);

		ret = wait_for_completion(get_cur_ddr_opp() <= data,
				DVFSRC_TIMEOUT);
		if (ret < 0) {
			pr_err("DVFSRC ddr: 0x%x, data: 0x%x\n", type, data);
			dvfsrc_dump_reg(NULL);
			aee_kernel_warning("DVFSRC", "ddr_opp failed.");
		}

		break;
	case PM_QOS_VCORE_OPP:
		if (data >= VCORE_OPP_NUM || data < 0)
			data = VCORE_OPP_NUM - 1;

		level = VCORE_OPP_NUM - data - 1;
		dvfsrc_set_vcore_request2(level,
				VCORE_QOS_GEAR0_MASK, VCORE_QOS_GEAR0_SHIFT);

		ret = wait_for_completion(get_cur_vcore_opp() <= data,
				DVFSRC_TIMEOUT);

		if (ret < 0) {
			pr_err("DVFSRC vcore: 0x%x, data: 0x%x\n", type, data);
			dvfsrc_dump_reg(NULL);
			aee_kernel_warning("DVFSRC", "vcore_opp failed.");
		}
		break;
	case PM_QOS_VCORE_DVFS_FORCE_OPP:
		if (data >= VCORE_DVFS_OPP_NUM || data < 0)
			data = VCORE_DVFS_OPP_NUM;

		if (data == VCORE_DVFS_OPP_NUM) { /* relase opp */
			dvfsrc_release_force();
			break;
		}

		/* force opp */
		level = 1 << (VCORE_DVFS_OPP_NUM - data - 1);
		dvfsrc_set_force_start(level);

		ret = wait_for_completion(get_cur_vcore_dvfs_opp() == data,
				DVFSRC_TIMEOUT);
		if (ret < 0) {
			pr_err("DVFSRC force: 0x%x, data: 0x%x\n", type, data);
			dvfsrc_dump_reg(NULL);
			aee_kernel_exception("DVFSRC", "force failed.");
		}

		dvfsrc_set_force_end();
		break;
	default:
		break;
	}

	return ret;
}

void dvfsrc_set_vcore_request(unsigned int vcore_level,
		unsigned int mask, unsigned int shift)
{
	int ret = 0;
	unsigned int val = 0;

	mutex_lock(&dvfsrc->devfreq->lock);

	dvfsrc_rmw(DVFSRC_VCORE_REQUEST, vcore_level, mask, shift);

	val = VCORE_OPP_NUM - vcore_level - 1;

	ret = wait_for_completion(get_cur_vcore_opp() <= val, DVFSRC_TIMEOUT);
	if (ret < 0) {
		dvfsrc_dump_reg(NULL);
		aee_kernel_exception("DVFSRC", "%s failed.", __func__);
	}

	mutex_unlock(&dvfsrc->devfreq->lock);
}

static int pm_qos_memory_bw_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b, struct helio_dvfsrc, pm_qos_memory_bw_nb);

	commit_data(dvfsrc, PM_QOS_MEMORY_BANDWIDTH, l);

	return NOTIFY_OK;
}

static int pm_qos_cpu_memory_bw_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b, struct helio_dvfsrc, pm_qos_cpu_memory_bw_nb);

	commit_data(dvfsrc, PM_QOS_CPU_MEMORY_BANDWIDTH, l);

	return NOTIFY_OK;
}

static int pm_qos_gpu_memory_bw_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b, struct helio_dvfsrc, pm_qos_gpu_memory_bw_nb);

	commit_data(dvfsrc, PM_QOS_GPU_MEMORY_BANDWIDTH, l);

	return NOTIFY_OK;
}

static int pm_qos_mm_memory_bw_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b, struct helio_dvfsrc, pm_qos_mm_memory_bw_nb);

	commit_data(dvfsrc, PM_QOS_MM_MEMORY_BANDWIDTH, l);

	return NOTIFY_OK;
}

static int pm_qos_other_memory_bw_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b,
			struct helio_dvfsrc, pm_qos_other_memory_bw_nb);

	commit_data(dvfsrc, PM_QOS_OTHER_MEMORY_BANDWIDTH, l);

	return NOTIFY_OK;
}

static int pm_qos_ddr_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b, struct helio_dvfsrc, pm_qos_ddr_opp_nb);

	commit_data(dvfsrc, PM_QOS_DDR_OPP, l);

	return NOTIFY_OK;
}

static int pm_qos_vcore_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b, struct helio_dvfsrc, pm_qos_vcore_opp_nb);

	commit_data(dvfsrc, PM_QOS_VCORE_OPP, l);

	return NOTIFY_OK;
}

static int pm_qos_vcore_dvfs_force_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b,
			struct helio_dvfsrc, pm_qos_vcore_dvfs_force_opp_nb);

	commit_data(dvfsrc, PM_QOS_VCORE_DVFS_FORCE_OPP, l);

	return NOTIFY_OK;
}

static void pm_qos_notifier_register(void)
{
	dvfsrc->pm_qos_memory_bw_nb.notifier_call =
		pm_qos_memory_bw_notify;
	dvfsrc->pm_qos_cpu_memory_bw_nb.notifier_call =
		pm_qos_cpu_memory_bw_notify;
	dvfsrc->pm_qos_gpu_memory_bw_nb.notifier_call =
		pm_qos_gpu_memory_bw_notify;
	dvfsrc->pm_qos_mm_memory_bw_nb.notifier_call =
		pm_qos_mm_memory_bw_notify;
	dvfsrc->pm_qos_other_memory_bw_nb.notifier_call =
		pm_qos_other_memory_bw_notify;
	dvfsrc->pm_qos_ddr_opp_nb.notifier_call =
		pm_qos_ddr_opp_notify;
	dvfsrc->pm_qos_vcore_opp_nb.notifier_call =
		pm_qos_vcore_opp_notify;
	dvfsrc->pm_qos_vcore_dvfs_force_opp_nb.notifier_call =
		pm_qos_vcore_dvfs_force_opp_notify;

	pm_qos_add_notifier(PM_QOS_MEMORY_BANDWIDTH,
			&dvfsrc->pm_qos_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_CPU_MEMORY_BANDWIDTH,
			&dvfsrc->pm_qos_cpu_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_GPU_MEMORY_BANDWIDTH,
			&dvfsrc->pm_qos_gpu_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_MM_MEMORY_BANDWIDTH,
			&dvfsrc->pm_qos_mm_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_OTHER_MEMORY_BANDWIDTH,
			&dvfsrc->pm_qos_other_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_DDR_OPP,
			&dvfsrc->pm_qos_ddr_opp_nb);
	pm_qos_add_notifier(PM_QOS_VCORE_OPP,
			&dvfsrc->pm_qos_vcore_opp_nb);
	pm_qos_add_notifier(PM_QOS_VCORE_DVFS_FORCE_OPP,
			&dvfsrc->pm_qos_vcore_dvfs_force_opp_nb);
}

static int helio_dvfsrc_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	dvfsrc = devm_kzalloc(&pdev->dev, sizeof(*dvfsrc), GFP_KERNEL);
	if (!dvfsrc)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	dvfsrc->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dvfsrc->regs))
		return PTR_ERR(dvfsrc->regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);

	dvfsrc->sram_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dvfsrc->sram_regs))
		return PTR_ERR(dvfsrc->sram_regs);

	platform_set_drvdata(pdev, dvfsrc);

	dvfsrc->devfreq = devm_devfreq_add_device(&pdev->dev,
						 &helio_devfreq_profile,
						 "helio_dvfsrc",
						 NULL);

	ret = helio_dvfsrc_add_interface(&pdev->dev);
	if (ret)
		return ret;

	pm_qos_notifier_register();

	ret = helio_dvfsrc_platform_init();
	if (ret)
		return ret;

	helio_dvfsrc_reg_config(dvfsrc->init_config);

	helio_dvfsrc_enable(0);

	pr_info("%s: init done\n", __func__);

	return 0;
}

static int helio_dvfsrc_remove(struct platform_device *pdev)
{
	helio_dvfsrc_remove_interface(&pdev->dev);
	return 0;
}

static const struct of_device_id helio_dvfsrc_of_match[] = {
	{ .compatible = "mediatek,dvfsrc" },
	{ .compatible = "mediatek,dvfsrc-v2" },
	{ },
};

MODULE_DEVICE_TABLE(of, helio_dvfsrc_of_match);

static __maybe_unused int helio_dvfsrc_suspend(struct device *dev)
{
	struct helio_dvfsrc *dvfsrc = dev_get_drvdata(dev);
	int ret = 0;

	ret = devfreq_suspend_device(dvfsrc->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to suspend the devfreq devices\n");
		return ret;
	}

	return 0;
}

static __maybe_unused int helio_dvfsrc_resume(struct device *dev)
{
	struct helio_dvfsrc *dvfsrc = dev_get_drvdata(dev);
	int ret = 0;

	ret = devfreq_resume_device(dvfsrc->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to resume the devfreq devices\n");
		return ret;
	}
	return ret;
}

static SIMPLE_DEV_PM_OPS(helio_dvfsrc_pm, helio_dvfsrc_suspend,
			 helio_dvfsrc_resume);

static struct platform_driver helio_dvfsrc_driver = {
	.probe	= helio_dvfsrc_probe,
	.remove	= helio_dvfsrc_remove,
	.driver = {
		.name = "helio-dvfsrc",
		.pm	= &helio_dvfsrc_pm,
		.of_match_table = helio_dvfsrc_of_match,
	},
};

static int __init helio_dvfsrc_init(void)
{
	int ret = 0;

	ret = devfreq_add_governor(&helio_dvfsrc_governor);
	if (ret) {
		pr_err("%s: failed to add governor: %d\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&helio_dvfsrc_driver);
	if (ret)
		devfreq_remove_governor(&helio_dvfsrc_governor);

	return ret;
}
late_initcall_sync(helio_dvfsrc_init)

static void __exit helio_dvfsrc_exit(void)
{
	int ret = 0;

	platform_driver_unregister(&helio_dvfsrc_driver);

	ret = devfreq_remove_governor(&helio_dvfsrc_governor);
	if (ret)
		pr_err("%s: failed to remove governor: %d\n", __func__, ret);
}
module_exit(helio_dvfsrc_exit)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Helio dvfsrc driver");
