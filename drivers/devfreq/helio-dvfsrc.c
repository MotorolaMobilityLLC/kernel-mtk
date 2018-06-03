/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include <linux/notifier.h>
#include <linux/pm_qos.h>

#include <linux/slab.h>

#include "governor.h"

#include <helio-dvfsrc.h>
#include <helio-dvfsrc-opp.h>
#include <mtk_dvfsrc_reg.h>
#include <mtk_spm_vcore_dvfs.h>
#include <mtk_spm_vcore_dvfs_ipi.h>
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#include <sspm_ipi.h>
#include <sspm_ipi_pin.h>
#endif

#include <mt-plat/aee.h>

__weak void helio_dvfsrc_platform_init(struct helio_dvfsrc *dvfsrc) { }
__weak void spm_check_status_before_dvfs(void) { }
__weak int spm_dvfs_flag_init(void) { return 0; }
__weak int vcore_opp_init(void) { return 0; }

static struct opp_profile opp_table[VCORE_DVFS_OPP_NUM];
static struct helio_dvfsrc *dvfsrc;

void dvfsrc_update_opp_table(void)
{
	struct opp_profile *opp_ctrl_table = opp_table;
	int opp;

	mutex_lock(&dvfsrc->devfreq->lock);
	for (opp = 0; opp < VCORE_DVFS_OPP_NUM; opp++)
		opp_ctrl_table[opp].vcore_uv = dvfsrc_get_vcore_by_steps(opp);

	mutex_unlock(&dvfsrc->devfreq->lock);
}

char *dvfsrc_get_opp_table_info(char *p)
{
	struct opp_profile *opp_ctrl_table = opp_table;
	int i;
	char *buff_end = p + PAGE_SIZE;

	for (i = 0; i < VCORE_DVFS_OPP_NUM; i++) {
		p += snprintf(p, buff_end - p, "[OPP%d] vcore_uv: %d (0x%x)\n", i, opp_ctrl_table[i].vcore_uv,
			     vcore_uv_to_pmic(opp_ctrl_table[i].vcore_uv));
		p += snprintf(p, buff_end - p, "[OPP%d] ddr_khz : %d\n", i, opp_ctrl_table[i].ddr_khz);
		p += snprintf(p, buff_end - p, "\n");
	}

	for (i = 0; i < VCORE_DVFS_OPP_NUM; i++)
		p += snprintf(p, buff_end - p, "OPP%d  : %u\n", i, opp_ctrl_table[i].vcore_uv);

	return p;
}

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
void dvfsrc_update_sspm_vcore_opp_table(int opp, unsigned int vcore_uv)
{
	struct qos_data qos_d;

	qos_d.cmd = QOS_IPI_VCORE_OPP;
	qos_d.u.vcore_opp.opp = opp;
	qos_d.u.vcore_opp.vcore_uv = vcore_uv;

	sspm_ipi_send_async(IPI_ID_QOS, IPI_OPT_DEFAUT, &qos_d, 3);
}

void dvfsrc_update_sspm_ddr_opp_table(int opp, unsigned int ddr_khz)
{
	struct qos_data qos_d;

	qos_d.cmd = QOS_IPI_DDR_OPP;
	qos_d.u.ddr_opp.opp = opp;
	qos_d.u.ddr_opp.ddr_khz = ddr_khz;

	sspm_ipi_send_async(IPI_ID_QOS, IPI_OPT_DEFAUT, &qos_d, 3);
}
#endif

void dvfsrc_init_opp_table(void)
{
	struct opp_profile *opp_ctrl_table = opp_table;
	int opp;

	mutex_lock(&dvfsrc->devfreq->lock);
	dvfsrc->curr_vcore_uv = vcorefs_get_curr_vcore();
	dvfsrc->curr_ddr_khz = vcorefs_get_curr_ddr();

	pr_info("curr_vcore_uv: %u, curr_ddr_khz: %u\n",
			dvfsrc->curr_vcore_uv,
			dvfsrc->curr_ddr_khz);

	for (opp = 0; opp < VCORE_DVFS_OPP_NUM; opp++) {
		opp_ctrl_table[opp].vcore_uv = dvfsrc_get_vcore_by_steps(opp);
		opp_ctrl_table[opp].ddr_khz = dvfsrc_get_ddr_by_steps(opp);

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
		dvfsrc_update_sspm_vcore_opp_table(opp, opp_ctrl_table[opp].vcore_uv);
		dvfsrc_update_sspm_ddr_opp_table(opp, opp_ctrl_table[opp].ddr_khz);
#endif

		pr_info("opp %u: vcore_uv: %u, ddr_khz: %u\n", opp,
				opp_ctrl_table[opp].vcore_uv,
				opp_ctrl_table[opp].ddr_khz);
	}

	spm_vcorefs_pwarp_cmd();
	mutex_unlock(&dvfsrc->devfreq->lock);
}

int is_vcorefs_can_work(void)
{
	if (dvfsrc)
		return dvfsrc->enable;

	return 0;
}

static struct devfreq_dev_profile helio_devfreq_profile = {
	.polling_ms	= 0,
};

static int helio_dvfsrc_reg_config(struct helio_dvfsrc *dvfsrc, struct reg_config *config)
{
	int i;

	mutex_lock(&dvfsrc->devfreq->lock);
	for (i = 0; config[i].offset != -1; i++)
		dvfsrc_write(dvfsrc, config[i].offset, config[i].val);

	mutex_unlock(&dvfsrc->devfreq->lock);

	return 0;
}

static int helio_governor_event_handler(struct devfreq *devfreq,
					unsigned int event, void *data)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = dev_get_drvdata(devfreq->dev.parent);

	switch (event) {
	case DEVFREQ_GOV_SUSPEND:
		helio_dvfsrc_reg_config(dvfsrc, dvfsrc->suspend_config);
		break;

	case DEVFREQ_GOV_RESUME:
		helio_dvfsrc_reg_config(dvfsrc, dvfsrc->resume_config);
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

static void helio_dvfsrc_enable(struct helio_dvfsrc *dvfsrc)
{
	mutex_lock(&dvfsrc->devfreq->lock);

	dvfsrc_write(dvfsrc, DVFSRC_VCORE_REQUEST,
			(dvfsrc_read(dvfsrc, DVFSRC_VCORE_REQUEST) & ~(0x3 << 20)));
	dvfsrc_write(dvfsrc, DVFSRC_EMI_REQUEST,
			(dvfsrc_read(dvfsrc, DVFSRC_EMI_REQUEST) & ~(0x3 << 20)));

	dvfsrc->enable = 1;

	mutex_unlock(&dvfsrc->devfreq->lock);
}

static int commit_data(struct helio_dvfsrc *dvfsrc, int type, int data)
{
	int ret = 0;
	int level = 0;
	int force_en_tar = 0;

	mutex_lock(&dvfsrc->devfreq->lock);

	if (!dvfsrc->enable)
		goto out;

	spm_check_status_before_dvfs();

	ret = wait_for_completion(is_dvfsrc_in_progress(dvfsrc) == 0, DVFSRC_TIMEOUT);
	if (ret) {
		spm_vcorefs_dump_dvfs_regs(NULL);
		pr_err("Failed to get response from dvfsrc\n");
		goto out;
	}

	switch (type) {
	case PM_QOS_MEMORY_BANDWIDTH:
		dvfsrc_write(dvfsrc, DVFSRC_SW_BW_0, data / 100);
		break;
	case PM_QOS_CPU_MEMORY_BANDWIDTH:
		dvfsrc_write(dvfsrc, DVFSRC_SW_BW_1, data / 100);
		break;
	case PM_QOS_GPU_MEMORY_BANDWIDTH:
		dvfsrc_write(dvfsrc, DVFSRC_SW_BW_2, data / 100);
		break;
	case PM_QOS_MM_MEMORY_BANDWIDTH:
		dvfsrc_write(dvfsrc, DVFSRC_SW_BW_3, data / 100);
		break;
	case PM_QOS_MD_PERI_MEMORY_BANDWIDTH:
		dvfsrc_write(dvfsrc, DVFSRC_SW_BW_4, data / 100);
		break;
	case PM_QOS_EMI_OPP:
		if (data >= DDR_OPP_NUM)
			data = DDR_OPP_NUM - 1;

		level = DDR_OPP_NUM - data - 1;
		dvfsrc_write(dvfsrc, DVFSRC_SW_REQ,
				(dvfsrc_read(dvfsrc, DVFSRC_SW_REQ)
				& ~(0x3)) | level);

		ret = wait_for_completion(get_dvfsrc_level(dvfsrc) >= emi_to_vcore_dvfs_level[level],
				SPM_DVFS_TIMEOUT);
		if (ret < 0) {
			spm_vcorefs_dump_dvfs_regs(NULL);
			aee_kernel_exception("VCOREFS", "dvfsrc cannot be done.");
		}

		break;
	case PM_QOS_VCORE_OPP:
		if (data >= VCORE_OPP_NUM)
			data = VCORE_OPP_NUM - 1;

		level = ((VCORE_OPP_NUM - data - 1) << 2);
		dvfsrc_write(dvfsrc, DVFSRC_SW_REQ,
				(dvfsrc_read(dvfsrc, DVFSRC_SW_REQ)
				& ~(0xC)) | level);

		ret = wait_for_completion(get_dvfsrc_level(dvfsrc) >= vcore_to_vcore_dvfs_level[level],
				SPM_DVFS_TIMEOUT);
		if (ret < 0) {
			spm_vcorefs_dump_dvfs_regs(NULL);
			aee_kernel_exception("VCOREFS", "dvfsrc cannot be done.");
		}

		break;
	case PM_QOS_VCORE_DVFS_FIXED_OPP:
		if (data >= VCORE_DVFS_OPP_NUM)
			data = VCORE_DVFS_OPP_NUM;

		if (data == VCORE_DVFS_OPP_NUM) { /* no fix opp*/
			level = 0;
			force_en_tar = 0;
			dvfsrc_write(dvfsrc, DVFSRC_BASIC_CONTROL,
					(dvfsrc_read(dvfsrc, DVFSRC_BASIC_CONTROL)
					& ~(1 << 15)) | force_en_tar);
			dvfsrc_write(dvfsrc, DVFSRC_FORCE, level);
		} else { /* fix opp */
			level = 1 << (VCORE_DVFS_OPP_NUM - data - 1);
			force_en_tar = (1 << 15);
			dvfsrc_write(dvfsrc, DVFSRC_FORCE, level);
			dvfsrc_write(dvfsrc, DVFSRC_BASIC_CONTROL,
					(dvfsrc_read(dvfsrc, DVFSRC_BASIC_CONTROL)
					& ~(1 << 15)) | force_en_tar);

			ret = wait_for_completion(get_dvfsrc_level(dvfsrc) == vcore_dvfs_to_vcore_dvfs_level[level],
					SPM_DVFS_TIMEOUT);
			if (ret < 0) {
				spm_vcorefs_dump_dvfs_regs(NULL);
				aee_kernel_exception("VCOREFS", "dvfsrc cannot be done.");
			}

		}
		break;
	default:
		break;
	}

	if (ret)
		pr_err("Failed to adjust dvfsrc level\n");

out:
	mutex_unlock(&dvfsrc->devfreq->lock);

	return ret;
}

#if 0
void dvfsrc_set_vcore_request(unsigned int mask, unsigned int vcore_level)
{
	int r = 0;
	unsigned int val = 0;

	mutex_lock(&dvfsrc->devfreq->lock);

	/* check DVFS idle */
	r = wait_for_completion(is_dvfsrc_in_progress(dvfsrc) == 0, SPM_DVFS_TIMEOUT);
	if (r < 0) {
		spm_vcorefs_dump_dvfs_regs(NULL);
		aee_kernel_exception("VCOREFS", "dvfsrc cannot be idle.");
		goto out;
	}

	val = (spm_read(DVFSRC_VCORE_REQUEST) & ~mask) | vcore_level;
	dvfsrc_write(dvfsrc, DVFSRC_VCORE_REQUEST, val);

	r = wait_for_completion(get_dvfsrc_level(dvfsrc) >= vcore_to_vcore_dvfs_level[vcore_level], SPM_DVFS_TIMEOUT);
	if (r < 0) {
		spm_vcorefs_dump_dvfs_regs(NULL);
		aee_kernel_exception("VCOREFS", "dvfsrc cannot be done.");
	}

out:
	mutex_unlock(&dvfsrc->devfreq->lock);
}
#endif

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

static int pm_qos_md_peri_memory_bw_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b, struct helio_dvfsrc, pm_qos_md_peri_memory_bw_nb);

	commit_data(dvfsrc, PM_QOS_MD_PERI_MEMORY_BANDWIDTH, l);

	return NOTIFY_OK;
}

static int pm_qos_emi_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b, struct helio_dvfsrc, pm_qos_emi_opp_nb);

	commit_data(dvfsrc, PM_QOS_EMI_OPP, l);

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

static int pm_qos_vcore_dvfs_fixed_opp_notify(struct notifier_block *b,
		unsigned long l, void *v)
{
	struct helio_dvfsrc *dvfsrc;

	dvfsrc = container_of(b, struct helio_dvfsrc, pm_qos_vcore_dvfs_fixed_opp_nb);

	commit_data(dvfsrc, PM_QOS_VCORE_DVFS_FIXED_OPP, l);

	return NOTIFY_OK;
}

static void pm_qos_notifier_register(struct helio_dvfsrc *dvfsrc)
{
	dvfsrc->pm_qos_memory_bw_nb.notifier_call = pm_qos_memory_bw_notify;
	dvfsrc->pm_qos_cpu_memory_bw_nb.notifier_call = pm_qos_cpu_memory_bw_notify;
	dvfsrc->pm_qos_gpu_memory_bw_nb.notifier_call = pm_qos_gpu_memory_bw_notify;
	dvfsrc->pm_qos_mm_memory_bw_nb.notifier_call = pm_qos_mm_memory_bw_notify;
	dvfsrc->pm_qos_md_peri_memory_bw_nb.notifier_call = pm_qos_md_peri_memory_bw_notify;
	dvfsrc->pm_qos_emi_opp_nb.notifier_call = pm_qos_emi_opp_notify;
	dvfsrc->pm_qos_vcore_opp_nb.notifier_call = pm_qos_vcore_opp_notify;
	dvfsrc->pm_qos_vcore_dvfs_fixed_opp_nb.notifier_call = pm_qos_vcore_dvfs_fixed_opp_notify;

	pm_qos_add_notifier(PM_QOS_MEMORY_BANDWIDTH, &dvfsrc->pm_qos_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_CPU_MEMORY_BANDWIDTH, &dvfsrc->pm_qos_cpu_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_GPU_MEMORY_BANDWIDTH, &dvfsrc->pm_qos_gpu_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_MM_MEMORY_BANDWIDTH, &dvfsrc->pm_qos_mm_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_MD_PERI_MEMORY_BANDWIDTH, &dvfsrc->pm_qos_md_peri_memory_bw_nb);
	pm_qos_add_notifier(PM_QOS_EMI_OPP, &dvfsrc->pm_qos_emi_opp_nb);
	pm_qos_add_notifier(PM_QOS_VCORE_OPP, &dvfsrc->pm_qos_vcore_opp_nb);
	pm_qos_add_notifier(PM_QOS_VCORE_DVFS_FIXED_OPP, &dvfsrc->pm_qos_vcore_dvfs_fixed_opp_nb);
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

	ret = helio_dvfsrc_add_interface(&pdev->dev);
	if (ret)
		return ret;

	helio_dvfsrc_platform_init(dvfsrc);

	dvfsrc->devfreq = devm_devfreq_add_device(&pdev->dev,
						 &helio_devfreq_profile,
						 "helio_dvfsrc",
						 NULL);
	vcore_opp_init();
	dvfsrc_init_opp_table();
	spm_check_status_before_dvfs();

	ret = helio_dvfsrc_reg_config(dvfsrc, dvfsrc->init_config);
	if (ret)
		return ret;

	pm_qos_notifier_register(dvfsrc);
	helio_dvfsrc_enable(dvfsrc);

	platform_set_drvdata(pdev, dvfsrc);

	return 0;
}

static int helio_dvfsrc_remove(struct platform_device *pdev)
{
	helio_dvfsrc_remove_interface(&pdev->dev);
	return 0;
}

static const struct of_device_id helio_dvfsrc_of_match[] = {
	{ .compatible = "mediatek,dvfsrc_top" },
	{ .compatible = "mediatek,helio-dvfsrc-v2" },
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
