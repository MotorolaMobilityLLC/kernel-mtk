#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#endif

#include "gf_spi.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

static struct pinctrl_state *ldo_1v8_on, *ldo_1v8_off, *ldo_3v3_on, *ldo_3v3_off;

/*GPIO pins reference.*/
int gf_parse_dts(struct gf_dev* gf_dev)
{
    struct device_node *node;
    struct platform_device *pdev = NULL;
    int ret = 0;

    node = of_find_compatible_node(NULL, NULL, "mediatek,goodix-fp");
    if (node) {
        pdev = of_find_device_by_node(node);
        if (pdev) {
            gf_dev->pinctrl_gpios = devm_pinctrl_get(&pdev->dev);
            if (IS_ERR(gf_dev->pinctrl_gpios)) {
                ret = PTR_ERR(gf_dev->pinctrl_gpios);
                pr_info("%s can't find fingerprint pinctrl\n", __func__);
                return ret;
            }
        } else {
            pr_info("%s platform device is null\n", __func__);
        }
    } else {
        pr_info("%s device node is null\n", __func__);
    }

    gf_dev->pins_irq = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fp_irq_mode");
    if (IS_ERR(gf_dev->pins_irq)) {
        ret = PTR_ERR(gf_dev->pins_irq);
        pr_info("%s can't find fingerprint pinctrl irq\n", __func__);
        return ret;
    }
    gf_dev->pins_reset_high = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fp_reset_high");
    if (IS_ERR(gf_dev->pins_reset_high)) {
        ret = PTR_ERR(gf_dev->pins_reset_high);
        pr_info("%s can't find fingerprint pinctrl reset_high\n", __func__);
        return ret;
    }
    gf_dev->pins_reset_low = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fp_reset_low");
    if (IS_ERR(gf_dev->pins_reset_low)) {
        ret = PTR_ERR(gf_dev->pins_reset_low);
        pr_info("%s can't find fingerprint pinctrl reset_low\n", __func__);
        return ret;
    }

    ldo_1v8_off = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fp_ldo1v8_off");
    if (IS_ERR(ldo_1v8_off)) {
        ret = PTR_ERR(ldo_1v8_off);
        pr_info("%s can't find fingerprint pinctrl ldo_1v8_off\n", __func__);
        return ret;
    }
    ldo_1v8_on = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fp_ldo1v8_on");
    if (IS_ERR(ldo_1v8_on)) {
        ret = PTR_ERR(ldo_1v8_on);
        pr_info("%s can't find fingerprint pinctrl ldo_1v8_on\n", __func__);
        return ret;
    }
    ldo_3v3_off = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fp_ldo3v3_off");
    if (IS_ERR(ldo_3v3_off)) {
        ret = PTR_ERR(ldo_3v3_off);
        pr_info("%s can't find fingerprint pinctrl ldo_3v3_off\n", __func__);
        return ret;
    }
    ldo_3v3_on = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fp_ldo3v3_on");
    if (IS_ERR(ldo_3v3_on)) {
        ret = PTR_ERR(ldo_3v3_on);
        pr_info("%s can't find fingerprint pinctrl ldo_3v3_on\n", __func__);
        return ret;
    }
    pr_info("%s, get pinctrl success!\n", __func__);
    return 0;
}

void gf_cleanup(struct gf_dev* gf_dev)
{
    pr_info("[info] %s\n",__func__);
    if (gpio_is_valid(gf_dev->irq_gpio))
    {
        gpio_free(gf_dev->irq_gpio);
        pr_info("remove irq_gpio success\n");
    }
    if (gpio_is_valid(gf_dev->reset_gpio))
    {
        gpio_free(gf_dev->reset_gpio);
        pr_info("remove reset_gpio success\n");
    }
}


/*power management*/
int gf_power_on(struct gf_dev* gf_dev)
{
    int rc = 0;
//	hwPowerOn(MT6331_POWER_LDO_VIBR, VOL_2800, "fingerprint");
    pinctrl_select_state(gf_dev->pinctrl_gpios, ldo_1v8_on);
    pinctrl_select_state(gf_dev->pinctrl_gpios, ldo_3v3_on);
    return rc;
}

int gf_power_off(struct gf_dev* gf_dev)
{
    int rc = 0;
//	hwPowerDown(MT6331_POWER_LDO_VIBR, "fingerprint");
    pinctrl_select_state(gf_dev->pinctrl_gpios, ldo_1v8_off);
    pinctrl_select_state(gf_dev->pinctrl_gpios, ldo_3v3_off);
    return rc;
}


/********************************************************************
*CPU output low level in RST pin to reset GF. This is the MUST action for GF.
*Take care of this function. IO Pin driver strength / glitch and so on.
********************************************************************/
int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_low);
    mdelay((delay_ms > 3)?delay_ms:3);
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_high);
	return 0;
}

int gf_irq_num(struct gf_dev *gf_dev)
{
    struct device_node *node;

    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_irq);

    node = of_find_compatible_node(NULL, NULL, "mediatek,goodix-fp");
    if (node) {
        gf_dev->irq_num = irq_of_parse_and_map(node, 0);
        pr_info("%s, gf_irq = %d\n", __func__, gf_dev->irq_num);
        gf_dev->irq = gf_dev->irq_num;
    } else
        pr_info("%s can't find compatible node\n", __func__);
    return gf_dev->irq;
}

