
#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_config_platform.h"

#include <platform/mtk_platform_common.h>

#include <mt_gpufreq.h>
#include <fan53555.h>

struct mtk_config {
	struct clk *clk_mfg_async;
	struct clk *clk_mfg;
	struct clk *clk_mfg_core0;
	struct clk *clk_mfg_core1;
	struct clk *clk_mfg_core2;
	struct clk *clk_mfg_core3;
	struct clk *clk_mfg_main;
};

static volatile void * g_ldo_base = NULL;

static void mt6797_gpu_set_power(int on)
{
	if (on)
	{
		if (!IS_ERR_OR_NULL((void*)g_ldo_base))
		{
			*(volatile uint32_t*)(g_ldo_base+0xfc0) = (uint32_t)0x0f0f0f0f;
			*(volatile uint32_t*)(g_ldo_base+0xfc4) = (uint32_t)0x0f0f0f0f;
			*(volatile uint32_t*)(g_ldo_base+0xfbc) = (uint32_t)0xff;
		}

		fan53555_config_interface(0x1, 0xA7, 0xff, 0x0);
	}
	else
	{
		fan53555_config_interface(0x1, 0x47, 0xff, 0x0);

		if (!IS_ERR_OR_NULL((void*)g_ldo_base))
		{
			*(volatile uint32_t*)(g_ldo_base+0xfbc) = (uint32_t)0x0;
		}
	}
}


#define MTKCLK_prepare_enable(clk) \
    if (config->clk && clk_prepare_enable(config->clk)) \
        pr_alert("MALI: clk_prepare_enable failed when enabling " #clk );

#define MTKCLK_disable_unprepare(clk) \
    if (config->clk) clk_disable_unprepare(config->clk);

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	struct mtk_config* config = kbdev->mtk_config;

	mt6797_gpu_set_power(1);

	MTKCLK_prepare_enable(clk_mfg_async);
	MTKCLK_prepare_enable(clk_mfg);
	MTKCLK_prepare_enable(clk_mfg_core0);
	MTKCLK_prepare_enable(clk_mfg_core1);
	MTKCLK_prepare_enable(clk_mfg_core2);
	MTKCLK_prepare_enable(clk_mfg_core3);
	MTKCLK_prepare_enable(clk_mfg_main);

	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	struct mtk_config* config = kbdev->mtk_config;

	MTKCLK_disable_unprepare(clk_mfg_main);
	MTKCLK_disable_unprepare(clk_mfg_core3);
	MTKCLK_disable_unprepare(clk_mfg_core2);
	MTKCLK_disable_unprepare(clk_mfg_core1);
	MTKCLK_disable_unprepare(clk_mfg_core0);
	MTKCLK_disable_unprepare(clk_mfg);
	MTKCLK_disable_unprepare(clk_mfg_async);

	mt6797_gpu_set_power(0);
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = NULL,
	.power_resume_callback = NULL
};

static struct kbase_platform_config versatile_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}


int kbase_platform_early_init(void)
{
	g_ldo_base = ioremap_nocache(0x10001000, 0x1000);

	return 0;
}


int mtk_platform_init(struct platform_device *pdev, struct kbase_device *kbdev)
{
	struct mtk_config* config = kmalloc(sizeof(struct mtk_config), GFP_KERNEL);

	if (config == NULL)
	{
		return -1;
	}

	config->clk_mfg_async = devm_clk_get(&pdev->dev, "mtcmos-mfg-async");
	config->clk_mfg = devm_clk_get(&pdev->dev, "mtcmos-mfg");
	config->clk_mfg_core0 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core0");
	config->clk_mfg_core1 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core1");
	config->clk_mfg_core2 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core2");
	config->clk_mfg_core3 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core3");
	config->clk_mfg_main = devm_clk_get(&pdev->dev, "mfg-main");

	dev_err(kbdev->dev, "xxxx mfg_async:%p\n", config->clk_mfg_async);
	dev_err(kbdev->dev, "xxxx mfg:%p\n", config->clk_mfg);
	dev_err(kbdev->dev, "xxxx mfg_core0:%p\n", config->clk_mfg_core0);
	dev_err(kbdev->dev, "xxxx mfg_core1:%p\n", config->clk_mfg_core1);
	dev_err(kbdev->dev, "xxxx mfg_core2:%p\n", config->clk_mfg_core2);
	dev_err(kbdev->dev, "xxxx mfg_core3:%p\n", config->clk_mfg_core3);
	dev_err(kbdev->dev, "xxxx mfg_main:%p\n", config->clk_mfg_main);

	kbdev->mtk_config = config;

	return 0;
}
