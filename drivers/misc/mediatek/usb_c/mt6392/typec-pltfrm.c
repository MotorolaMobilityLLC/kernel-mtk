/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include "mt_typec.h"

#ifdef CONFIG_PM
/**
 * typec_pltfrm_suspend - suspend power management function
 * @dev: pointer to device handle
 *
 * Returns 0
 */
static int typec_pltfrm_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct typec_hba *hba = platform_get_drvdata(pdev);

	disable_irq(hba->irq);

	return 0;
}

/**
 * typec_pltfrm_resume - resume power management function
 * @dev: pointer to device handle
 *
 * Returns 0
 */
static int typec_pltfrm_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct typec_hba *hba = platform_get_drvdata(pdev);

	enable_irq(hba->irq);

	return 0;
}
#else
#define typec_pltfrm_suspend NULL
#define typec_pltfrm_resume	NULL
#endif

#ifdef CONFIG_PM_RUNTIME
static int typec_pltfrm_runtime_suspend(struct device *dev)
{
	struct typec_hba *hba = dev_get_drvdata(dev);

	if (!hba)
		return 0;

	return typec_runtime_suspend(hba);
}

static int typec_pltfrm_runtime_resume(struct device *dev)
{
	struct typec_hba *hba = dev_get_drvdata(dev);

	if (!hba)
		return 0;

	return typec_runtime_resume(hba);
}

static int typec_pltfrm_runtime_idle(struct device *dev)
{
	struct typec_hba *hba = dev_get_drvdata(dev);

	if (!hba)
		return 0;

	return typec_runtime_idle(hba);
}
#else				/* !CONFIG_PM_RUNTIME */
#define typec_pltfrm_runtime_suspend	NULL
#define typec_pltfrm_runtime_resume	NULL
#define typec_pltfrm_runtime_idle	NULL
#endif				/* CONFIG_PM_RUNTIME */

/**
 * typec_pltfrm_probe - probe routine of the driver
 * @pdev: pointer to Platform device handle
 *
 * Returns 0 on success, non-zero value on failure
 */
static int typec_pltfrm_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct typec_hba *hba;
	struct mt6397_chip *mt6392;
	int irq = 0;
	int err;
	struct device *dev = &pdev->dev;

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	hba = kzalloc(sizeof(struct typec_hba), GFP_KERNEL);
	if (hba == NULL)
		return -EINVAL;

	/* Use regmap to read/write MT6392 TypeC register */
	mt6392 = dev_get_drvdata(pdev->dev.parent);
	hba->regmap = mt6392->regmap;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL)
		return -EINVAL;
	hba->addr_base = res->start;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL)
		return -EINVAL;
	irq = irq_create_mapping(mt6392->irq_domain, res->start);
	if (irq <= 0)
		return -EINVAL;

	pdev->id = 0;
	err = typec_init(dev, hba, irq, pdev->id);
	if (err) {
		dev_err(dev, "Intialization failed\n");
		goto out_disable_rpm;
	}

	platform_set_drvdata(pdev, hba);

	return 0;

out_disable_rpm:
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);

	return err;
}

/**
 * typec_pltfrm_remove - remove platform driver routine
 * @pdev: pointer to platform device handle
 *
 * Returns 0 on success, non-zero value on failure
 */
static int typec_pltfrm_remove(struct platform_device *pdev)
{
	struct typec_hba *hba = platform_get_drvdata(pdev);

	pm_runtime_get_sync(&(pdev)->dev);
	typec_remove(hba);

	return 0;
}

static const struct of_device_id typec_of_match[] = {
	{.compatible = "mediatek,mt6392-typec"},
	{},
};

static const struct dev_pm_ops typec_dev_pm_ops = {
	.suspend = typec_pltfrm_suspend,
	.resume = typec_pltfrm_resume,
	.runtime_suspend = typec_pltfrm_runtime_suspend,
	.runtime_resume = typec_pltfrm_runtime_resume,
	.runtime_idle = typec_pltfrm_runtime_idle,
};

static struct platform_driver typec_pltfrm_driver = {
	.probe = typec_pltfrm_probe,
	.remove = typec_pltfrm_remove,
	.driver = {
		   .name = "mt6392-typec",
		   .owner = THIS_MODULE,
		   .pm = &typec_dev_pm_ops,
		   .of_match_table = typec_of_match,
		   },
};

int typec_pltfrm_init(void)
{
	return platform_driver_register(&typec_pltfrm_driver);
}

void typec_pltfrm_exit(void)
{
	platform_driver_unregister(&typec_pltfrm_driver);
}
