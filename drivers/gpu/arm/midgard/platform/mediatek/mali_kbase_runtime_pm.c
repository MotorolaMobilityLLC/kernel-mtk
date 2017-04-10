/*
 * Copyright (c) 2017 MediaTek Inc.
 * Author: Chiawen Lee <chiawen.lee@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <linux/pm_runtime.h>
#include <linux/suspend.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>


struct mfg_base {
	void __iomem *reg_base;
	struct clk *mfg_pll;
	struct clk *mfg_sel;
};

#define REG_MFG_G3D BIT(0)

#define REG_MFG_CG_STA 0x00
#define REG_MFG_CG_SET 0x04
#define REG_MFG_CG_CLR 0x08

static void mtk_mfg_set_clock_gating(void __iomem *reg)
{
	writel(REG_MFG_G3D, reg + REG_MFG_CG_SET);
}

static void mtk_mfg_clr_clock_gating(void __iomem *reg)
{
	writel(REG_MFG_G3D, reg + REG_MFG_CG_CLR);
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	struct mfg_base *mfg = (struct mfg_base *)kbdev->platform_context;

	pm_runtime_get_sync(kbdev->dev);
	clk_prepare_enable(mfg->mfg_pll);
	clk_prepare_enable(mfg->mfg_sel);
	mtk_mfg_clr_clock_gating(mfg->reg_base);

	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	struct mfg_base *mfg = (struct mfg_base *)kbdev->platform_context;

	mtk_mfg_set_clock_gating(mfg->reg_base);
	clk_disable_unprepare(mfg->mfg_sel);
	clk_disable_unprepare(mfg->mfg_pll);
	pm_runtime_put_autosuspend(kbdev->dev);
}

int kbase_device_runtime_init(struct kbase_device *kbdev)
{
	return 0;
}

void kbase_device_runtime_disable(struct kbase_device *kbdev)
{
}

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	return 0;
}

static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
}

static void pm_callback_resume(struct kbase_device *kbdev)
{
}

static void pm_callback_suspend(struct kbase_device *kbdev)
{
	pm_callback_runtime_off(kbdev);
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback = pm_callback_suspend,
	.power_resume_callback = pm_callback_resume,
#ifdef KBASE_PM_RUNTIME
	.power_runtime_init_callback = kbase_device_runtime_init,
	.power_runtime_term_callback = kbase_device_runtime_disable,
	.power_runtime_on_callback = pm_callback_runtime_on,
	.power_runtime_off_callback = pm_callback_runtime_off,
#else				/* KBASE_PM_RUNTIME */
	.power_runtime_init_callback = NULL,
	.power_runtime_term_callback = NULL,
	.power_runtime_on_callback = NULL,
	.power_runtime_off_callback = NULL,
#endif				/* KBASE_PM_RUNTIME */
};

int mali_mfgsys_init(struct kbase_device *kbdev, struct mfg_base *mfg)
{
	int err = 0;

	mfg->reg_base = of_iomap(kbdev->dev->of_node, 1);
	if (!mfg->reg_base)
		return -ENOMEM;

	mfg->mfg_pll = devm_clk_get(kbdev->dev, "mfg_pll");
	if (IS_ERR(mfg->mfg_pll)) {
		err = PTR_ERR(mfg->mfg_pll);
		dev_err(kbdev->dev, "devm_clk_get mfg_pll failed\n");
		goto err_iounmap_reg_base;
	}

	mfg->mfg_sel = devm_clk_get(kbdev->dev, "mfg_sel");
	if (IS_ERR(mfg->mfg_sel)) {
		err = PTR_ERR(mfg->mfg_sel);
		dev_err(kbdev->dev, "devm_clk_get mfg_sel failed\n");
		goto err_iounmap_reg_base;
	}

	return 0;

err_iounmap_reg_base:
	iounmap(mfg->reg_base);
	return err;
}

void mali_mfgsys_deinit(struct kbase_device *kbdev)
{
	struct mfg_base *mfg = (struct mfg_base *)kbdev->platform_context;

	iounmap(mfg->reg_base);
}


static int platform_init(struct kbase_device *kbdev)
{
	int err;
	struct mfg_base *mfg;

	mfg = devm_kzalloc(kbdev->dev, sizeof(*mfg), GFP_KERNEL);
	if (!mfg)
		return -ENOMEM;

	err = mali_mfgsys_init(kbdev, mfg);
	if (err)
		return err;

	kbdev->platform_context = mfg;
	pm_runtime_enable(kbdev->dev);

	return err;
}

static void platform_term_func(struct kbase_device *kbdev)
{
	mali_mfgsys_deinit(kbdev);
}

struct kbase_platform_funcs_conf platform_funcs = {
	.platform_init_func = platform_init,
	.platform_term_func = platform_term_func
};

