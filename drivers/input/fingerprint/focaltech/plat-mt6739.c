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

//#include <mt_clkmgr.h>

#include "ff_log.h"

# undef LOG_TAG
#define LOG_TAG "mt6735"

/* TODO: */
#define FF_COMPATIBLE_NODE_1 "mediatek,fingerprint" //"mediatek,focal-fp" 
#define FF_COMPATIBLE_NODE_2 "mediatek,fpc1145"

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
    FF_LOGD("irq number is %d.", *irq_num);

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
    /*
     * FIXME: It seems that before 'disable_clock', the spi clock
     * must has been 'enable_clock'.
     */
    static bool b_spiclk_enabled = false;
    int err = 0;

	(void)b_spiclk_enabled;
    FF_LOGV("'%s' enter.", __func__);
    FF_LOGD("not use by song.li, clock: '%s'.", on ? "enable" : "disabled");
#if 0
    if (on && !b_spiclk_enabled) {
        enable_clock(MT_CG_PERI_SPI0, "spi");
        b_spiclk_enabled = true;
    } else if (!on && b_spiclk_enabled) {
        disable_clock(MT_CG_PERI_SPI0, "spi");
        b_spiclk_enabled = false;
    }
#endif
    FF_LOGV("'%s' leave.", __func__);
    return err;
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

