/*
 * Hardware Compressed RAM block device
 * Copyright (C) 2015 Google Inc.
 *
 * Sonny Rao <sonnyrao@chromium.org>
 *
 */

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <asm/irqflags.h>
#include "hwzram_impl.h"

static unsigned int hwzram_default_fifo_size = 64;

#ifdef CONFIG_OF

static int hwzram_impl_of_probe(struct platform_device *pdev)
{
	struct hwzram_impl *hwz;
	struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	int irq = platform_get_irq(pdev, 0);

	pr_info("%s: got irq %d\n", __func__, irq);

	/* if (hwzram_force_timer) */
	/* irq = 0; */

	pr_devel("%s starting\n", __func__);

	hwz = hwzram_impl_init(&pdev->dev, hwzram_default_fifo_size,
			       mem->start, irq);

	platform_set_drvdata(pdev, hwz);

	if (IS_ERR(hwz))
		return -EINVAL;

	return 0;
}

static int hwzram_impl_of_remove(struct platform_device *pdev)
{
	struct hwzram_impl *hwz = platform_get_drvdata(pdev);

	hwzram_impl_destroy(hwz);

	return 0;
}

static const struct of_device_id hwzram_impl_of_match[] = {
	{ .compatible = "google,zram", },
	{ .compatible = "mediatek,zram", },
	{ .compatible = "mediatek,ufozip", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hwzram_impl_of_match);


static struct platform_driver hwzram_impl_of_driver = {
	.probe		= hwzram_impl_of_probe,
	.remove		= hwzram_impl_of_remove,
	.driver		= {
		.name	= "hwzram_impl",
		.of_match_table = of_match_ptr(hwzram_impl_of_match),
	},
};

module_platform_driver(hwzram_impl_of_driver);
#endif

module_param(hwzram_default_fifo_size, uint, 0644);
MODULE_AUTHOR("Sonny Rao");
MODULE_DESCRIPTION("Hardware Memory compression accelerator");

