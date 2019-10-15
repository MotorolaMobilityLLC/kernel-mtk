/**
 * plat-mt6755.c
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
#define FF_COMPATIBLE_NODE_1 "mediatek,focal_fp"
#define FF_COMPATIBLE_NODE_2 "mediatek,fingerprint"
#define FF_COMPATIBLE_NODE_3 "mediatek,mt6765-spi"

#define GF_VDD_MIN_UV      2800000
#define GF_VDD_MAX_UV	   2800000

#if 0
/* Define pinctrl state types. */
typedef enum {
    FF_PINCTRL_STATE_CS_CLR,
    FF_PINCTRL_STATE_CS_ACT,
    FF_PINCTRL_STATE_MI_CLR,
    FF_PINCTRL_STATE_MI_ACT,
    FF_PINCTRL_STATE_MO_CLR,
    FF_PINCTRL_STATE_MO_ACT,
    FF_PINCTRL_STATE_CLK_CLR,
    FF_PINCTRL_STATE_CLK_ACT,
    FF_PINCTRL_STATE_RST_CLR,
    FF_PINCTRL_STATE_RST_ACT,
    FF_PINCTRL_STATE_INT,
    FF_PINCTRL_STATE_INT_CLR,
    FF_PINCTRL_STATE_INT_SET,
    FF_PINCTRL_STATE_POWER_ON,
    FF_PINCTRL_STATE_POWER_OFF,
    FF_PINCTRL_STATE_MAXIMUM /* Array size */
} ff_pinctrl_state_t;

static const char *g_pinctrl_state_names[FF_PINCTRL_STATE_MAXIMUM] = {
    "spi_cs_clr",
    "spi_cs_set",
    "spi_mi_clr",
    "spi_mi_set",
    "spi_mo_clr",
    "spi_mo_set",
    "spi_mclk_clr",
    "spi_mclk_set",
    "finger_rst_clr",
    "finger_rst_set",
    "eint",
    "eint_clr",
    "eint_set",
    "power_on",
    "power_off",
};
#else
/* Define pinctrl state types. */
typedef enum {
    FF_PINCTRL_STATE_RST_ACT,
    FF_PINCTRL_STATE_RST_CLR,
    FF_PINCTRL_STATE_INT,
    FF_PINCTRL_STATE_INT_CLR,
    FF_PINCTRL_STATE_INT_SET,
    FF_PINCTRL_STATE_MAXIMUM /* Array size */
} ff_pinctrl_state_t;

static const char *g_pinctrl_state_names[FF_PINCTRL_STATE_MAXIMUM] = {
    "fp_rst_high",
    "fp_rst_low",
    "eint_as_int",
    "eint_in_low",
    "eint_in_high",
    //"eint_in_float",
   // "miso_pull_up",
   // "miso_pull_disable",
};
#endif


extern struct spi_master *spi_ctl;
extern struct spi_device *g_spidev;
extern int ff_power_enable(bool on);

/* Native context and its singleton instance. */
typedef struct {
    struct pinctrl *pinctrl;
    struct pinctrl_state *pin_states[FF_PINCTRL_STATE_MAXIMUM];
#if !defined(CONFIG_MTK_CLKMGR)
    struct clk *spiclk;
#endif
    struct device *dev;
    bool b_spiclk_enabled;
} ff_mt6755_context_t;
static ff_mt6755_context_t ff_mt6755_context, *g_context = &ff_mt6755_context;

int ff_ctl_init_pins(int *irq_num)
{
    int err = 0, i;
    struct device_node *dev_node = NULL;
    struct platform_device *pdev = NULL;
    FF_LOGI("'%s' enter.", __func__);

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
//return -1;
    /* Retrieve the pinctrl handler. */
    g_context->pinctrl = devm_pinctrl_get(&pdev->dev);
    if (!g_context->pinctrl) {
        FF_LOGE("devm_pinctrl_get(..) failed.");
        return (-ENODEV);
    }
//return -2;
    /* Register all pins. */
    for (i = 0; i < FF_PINCTRL_STATE_MAXIMUM; ++i) {
        g_context->pin_states[i] = pinctrl_lookup_state(g_context->pinctrl, g_pinctrl_state_names[i]);
        if (!g_context->pin_states[i]) {
            FF_LOGE("can't find pinctrl state for '%s'.", g_pinctrl_state_names[i]);
            err = (-ENODEV);
            break;
        }
    }
    if (i < FF_PINCTRL_STATE_MAXIMUM) {
        return (-ENODEV);
    }
//return -3;
    /* Initialize the SPI pins. */
    //err = pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_CS_ACT]);
   // err = pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_CLK_ACT]);
   // err = pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_MO_ACT]);
    //err = pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_MI_ACT]);

    /* Initialize the INT pin. */
    err = pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_INT]);

    /* Retrieve the irq number. */
    dev_node = of_find_compatible_node(NULL, NULL, FF_COMPATIBLE_NODE_2);
    if (!dev_node) {
        FF_LOGE("of_find_compatible_node(.., '%s') failed.", FF_COMPATIBLE_NODE_2);
        return (-ENODEV);
    }
    *irq_num = irq_of_parse_and_map(dev_node, 0);
    FF_LOGD("irq number is %d.", *irq_num);

#if !defined(CONFIG_MTK_CLKMGR)
    //
    // Retrieve the clock source of the SPI controller.
    //

    /* 3-1: Find device tree node. */
#if 1
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
    g_context->spiclk = devm_clk_get(&pdev->dev, "spi-clk");
    if (!g_context->spiclk) {
        FF_LOGE("devm_clk_get(..) failed.");
        return (-ENODEV);
    }
#endif
#endif
    /* Initialize the RESET pin. */
    pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_RST_ACT]);

    FF_LOGI("'%s' leave.", __func__);
    return err;
}

int ff_ctl_free_pins(void)
{
    int err = 0;
    FF_LOGV("'%s' enter.", __func__);
// TASK6361889
    if(g_context->pinctrl)
    	devm_pinctrl_put(g_context->pinctrl);


    FF_LOGV("'%s' leave.", __func__);
    return err;
}

int ff_ctl_enable_spiclk(bool on)
{
#if 1
    int err = 0;

    //FF_LOGI("'%s' enter.", __func__);
    FF_LOGD("clock: '%s'.", on ? "enable" : "disabled");

    if (unlikely(!g_context->spiclk)) {
        return (-ENOSYS);
    }
    //FF_LOGI("b_spiclk_enabled = %d.", g_context->b_spiclk_enabled);
    /* Control the clock source. */
    if (on && !g_context->b_spiclk_enabled) {
        err = clk_prepare_enable(g_context->spiclk);
        if (err) {
            FF_LOGE("clk_prepare_enable(..) = %d.", err);
        }
        g_context->b_spiclk_enabled = true;
    } else if (!on && g_context->b_spiclk_enabled) {
        clk_disable_unprepare(g_context->spiclk);
        g_context->b_spiclk_enabled = false;
    }

    //FF_LOGI("'%s' leave.", __func__);
    return err;
#else
    return 0;
#endif

}
#if 0
static int fp_power_enable(bool on)
{
#if 1
	int ret = 0;
	struct regulator *reg;
	
		
	FF_LOGI("'%s' enter.", __func__);

	if(!g_spidev){
		FF_LOGI("g_spidev is null");
		return -1;
	}
	//&ff_ctl_context.miscdev
	reg = regulator_get(g_context->dev, "irtx_ldo");
	//reg = regulator_get(&g_spidev->dev, "irtx_ldo");
	if (IS_ERR(reg)) {
		ret = PTR_ERR(reg);
		FF_LOGI("Regulator get failed vdd ret=%d", ret);
		return ret;
	}
	FF_LOGI("Regulator get success vdd ret=%d", ret);
	ret = regulator_count_voltages(reg);
	FF_LOGI("regulator_count_voltages=%d", ret);
	if(ret){
	//if (regulator_count_voltages(reg) > 0) {
		ret = regulator_set_voltage(reg, GF_VDD_MIN_UV,
					   GF_VDD_MAX_UV);
		if (ret) {
			FF_LOGI("Regulator set_vtg failed vdd ret=%d", ret);
			//goto reg_vdd_put;
		}
	}
	if(on)
		ret = regulator_enable(reg);	/*enable regulator*/
	else
		ret = regulator_disable(reg);	/*enable regulator*/
	if (ret)
		FF_LOGI("regulator_enable() failed!");
	
	FF_LOGI("Regulator set_vtg OK vdd ret=%d", ret);
	return 0;
	#if 0
reg_vdd_put:
	regulator_put(reg);
	
	return ret;
	#endif
	#else
	return 0;
	#endif
}
#endif
int ff_ctl_enable_power(bool on)
{
    int err = 0;
    FF_LOGI("'%s' enter.", __func__);
    FF_LOGD("power: '%s'.", on ? "on" : "off");

    if (unlikely(!g_context->pinctrl)) {
        return (-ENOSYS);
    }

    if (on) {
		err = ff_power_enable(1);
        //err = pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_POWER_ON]);
    } else {
    		err = ff_power_enable(0);
        //err = pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_POWER_OFF]);
    }

    FF_LOGI("'%s' leave.", __func__);
    return err;
}

int ff_ctl_reset_device(void)
{
    int err = 0;
    FF_LOGV("'%s' enter.", __func__);

    if (unlikely(!g_context->pinctrl)) {
        return (-ENOSYS);
    }

    /* 3-1: Pull down RST pin. */
    err = pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_RST_CLR]);

    /* 3-2: Delay for 10ms. */
    mdelay(10);

    /* Pull up RST pin. */
    err = pinctrl_select_state(g_context->pinctrl, g_context->pin_states[FF_PINCTRL_STATE_RST_ACT]);

    FF_LOGV("'%s' leave.", __func__);
    return err;
}

const char *ff_ctl_arch_str(void)
{
    return CONFIG_MTK_PLATFORM;
}

