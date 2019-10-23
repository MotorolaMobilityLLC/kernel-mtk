/**
 * plat-mt6735m.c
 *
 **/

#include <linux/stddef.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>

#include<linux/spi/spi.h>
#include <linux/platform_data/spi-mt65xx.h>
#include <linux/regulator/consumer.h>


#if !defined(CONFIG_MTK_CLKMGR)
# include <linux/clk.h>
#else
# include <mach/mt_clkmgr.h>
#endif

#include "ff_log.h"

# undef LOG_TAG
#define LOG_TAG "mt6739"

/* TODO: */
#define FF_COMPATIBLE_NODE_1 "mediatek,fpsensor_pinctrl" //"mediatek,focal-fp"
#define FF_COMPATIBLE_NODE_3 "mediatek,mt6765-spi-fp"
extern struct spi_device *g_spidev;
extern void mt_spi_disable_master_clk(struct spi_device *ms);
extern void mt_spi_enable_master_clk(struct spi_device *spidev);
extern void fpsensor_spi_clk_enable(u8 bonoff);

#if !defined(CONFIG_MTK_CLKMGR)
//struct clk *g_context_spiclk = NULL;
bool g_context_enabled = false;
#endif

/* Define pinctrl state types. */
typedef enum {
	FF_PINCTRL_STATE_PWR_ACT,
	FF_PINCTRL_STATE_PWR_CLR,
	FF_PINCTRL_STATE_RST_ACT,
	FF_PINCTRL_STATE_RST_CLR,
	FF_PINCTRL_STATE_INT_ACT,
	FF_PINCTRL_STATE_MAXIMUM /* Array size */
} ff_pinctrl_state_t;

static const char *g_pinctrl_state_names[FF_PINCTRL_STATE_MAXIMUM] = {
	"fpsensor_finger_power_high", "fpsensor_finger_power_low","fpsensor_finger_rst_high", "fpsensor_finger_rst_low", "fpsensor_eint_as_int",
};
static struct pinctrl *g_pinctrl = NULL;
static struct pinctrl_state *g_pin_states[FF_PINCTRL_STATE_MAXIMUM] = {
	NULL,
};

int ff_ctl_init_pins(int *irq_num)
{
	int err = 0, i;
	struct device_node *dev_node = NULL;
	struct platform_device *pdev = NULL;
	FF_LOGV("'%s' enter.", __func__);

	/* Find device tree node. */
	dev_node = of_find_compatible_node(NULL, NULL, FF_COMPATIBLE_NODE_1);
	if (!dev_node) {
		FF_LOGE("of_find_compatible_node(.., '%s') failed.", FF_COMPATIBLE_NODE_1);
		return (-ENODEV);
	}

	/* Convert to platform device. */
	pdev = of_find_device_by_node(dev_node);
	if (!pdev) {
		FF_LOGE("of_find_device_by_node(..) failed.");
		return (-ENODEV);
	}

	/* Retrieve the pinctrl handler. */
	g_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (!g_pinctrl) {
		FF_LOGE("devm_pinctrl_get(..) failed.");
		return (-ENODEV);
	}

	/* Register all pins. */
	for (i = 0; i < FF_PINCTRL_STATE_MAXIMUM; ++i) {
		g_pin_states[i] = pinctrl_lookup_state(g_pinctrl, g_pinctrl_state_names[i]);
		if (!g_pin_states[i]) {
			FF_LOGE("can't find pinctrl state for '%s'.", g_pinctrl_state_names[i]);
			err = (-ENODEV);
			break;
		}
	}
	if (i < FF_PINCTRL_STATE_MAXIMUM) {
		return (-ENODEV);
	}

	/* Initialize the INT pin. */
	err = pinctrl_select_state(g_pinctrl, g_pin_states[FF_PINCTRL_STATE_INT_ACT]);

	/* Retrieve the irq number. */
	*irq_num = irq_of_parse_and_map(dev_node, 0);
	printk("%s +%d, errno=%d, devnode=%s, irq=%d\n", __func__, __LINE__, err, dev_node->name, *irq_num);

#if !defined(CONFIG_MTK_CLKMGR)
	//
	// Retrieve the clock source of the SPI controller.
	//

	/* 3-1: Find device tree node. */
#if 0
	dev_node = of_find_compatible_node(NULL, NULL, FF_COMPATIBLE_NODE_3);
	if (!dev_node) {
		FF_LOGE("of_find_compatible_node(.., '%s') failed.", FF_COMPATIBLE_NODE_3);
		return (-ENODEV);
	}

	/* 3-2: Convert to platform device. */
	pdev = of_find_device_by_node(dev_node);
	if (!pdev) {
		FF_LOGE("of_find_device_by_node(..) failed.");
		return (-ENODEV);
	}
	FF_LOGD("spi controller name: %s.", pdev->name);

	/* 3-3: Retrieve the SPI clk handler. */
	g_context_spiclk = devm_clk_get(&pdev->dev, "spi-clk");
	if (!g_context_spiclk) {
		FF_LOGE("devm_clk_get(..) failed.");
		return (-ENODEV);
	}
#endif
#endif
	/* Initialize the RESET pin. */
	pinctrl_select_state(g_pinctrl, g_pin_states[FF_PINCTRL_STATE_RST_ACT]);

	FF_LOGV("'%s' leave.", __func__);
	return err;
}

int ff_ctl_free_pins(void)
{
	int err = 0;
	FF_LOGV("'%s' enter.", __func__);

	// TODO:
	devm_pinctrl_put(g_pinctrl);

	FF_LOGV("'%s' leave.", __func__);
	return err;
}

int ff_ctl_enable_spiclk(bool on)
{
	FF_LOGD("clock: '%s'.", on ? "enable" : "disabled");
#if 1
	/* Control the clock source. */
	if (on && !g_context_enabled) {
//		mt_spi_enable_master_clk(g_spidev);
		fpsensor_spi_clk_enable(1);
		g_context_enabled = true;
	} else if (!on && g_context_enabled) {
//		mt_spi_disable_master_clk(g_spidev);
		fpsensor_spi_clk_enable(0);
		g_context_enabled = false;
	}
#endif
	return 0;
}

int ff_ctl_enable_power(bool on)
{
	int err = 0;
	FF_LOGV("'%s' enter.", __func__);
	FF_LOGD("power: '%s'.", on ? "on" : "off");

	if (unlikely(!g_pinctrl)) {
		FF_LOGE("g_pinctrl is NULL.");
		return (-ENOSYS);
	}

	if (on) {
		//        err = pinctrl_select_state(g_pinctrl, g_pin_states[FF_PINCTRL_STATE_PWR_ACT]);
	} else {
		//        err = pinctrl_select_state(g_pinctrl, g_pin_states[FF_PINCTRL_STATE_PWR_CLR]);
	}

	FF_LOGV("'%s' leave.", __func__);
	return err;
}

int ff_ctl_reset_device(void)
{
	int err = 0;
	FF_LOGV("'%s' enter.", __func__);

	if (unlikely(!g_pinctrl)) {
		return (-ENOSYS);
	}

	/* 3-1: Pull down RST pin. */
	err = pinctrl_select_state(g_pinctrl, g_pin_states[FF_PINCTRL_STATE_RST_CLR]);

	/* 3-2: Delay for 10ms. */
	mdelay(10);

	/* Pull up RST pin. */
	err = pinctrl_select_state(g_pinctrl, g_pin_states[FF_PINCTRL_STATE_RST_ACT]);

	FF_LOGV("'%s' leave.", __func__);
	return err;
}

const char *ff_ctl_arch_str(void)
{
	return CONFIG_MTK_PLATFORM;
}

