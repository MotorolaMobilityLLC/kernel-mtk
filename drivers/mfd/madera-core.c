/*
 * Core MFD support for Cirrus Logic Madera codecs
 *
 * Copyright 2015-2017 Cirrus Logic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/mfd/core.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>

#include <linux/mfd/madera/core.h>
#include <linux/mfd/madera/registers.h>

#include "madera.h"

#define CS47L15_SILICON_ID	0x6370
#define CS47L35_SILICON_ID	0x6360
#define CS47L85_SILICON_ID	0x6338
#define CS47L90_SILICON_ID	0x6364
#define CS47L92_SILICON_ID	0x6371

#define MADERA_32KZ_MCLK2	1

static const char * const madera_core_supplies[] = {
	"AVDD",
	"DBVDD1",
};

static const struct mfd_cell madera_ldo1_devs[] = {
	{ .name = "madera-ldo1" },
};

static const struct mfd_cell madera_pinctrl_dev[] = {
	{
		.name = "madera-pinctrl",
		.of_compatible = "cirrus,madera-pinctrl",
	},
};

static const char * const cs47l15_supplies[] = {
	"MICVDD",
	"CPVDD1",
	"SPKVDD",
};

static const struct mfd_cell cs47l15_devs[] = {
	{ .name = "madera-irq" },
	{ .name = "madera-gpio" },
	{
		.name = "madera-extcon",
		.parent_supplies = cs47l15_supplies,
		.num_parent_supplies = 1, /* We only need MICVDD */
	},
	{
		.name = "cs47l15-codec",
		.parent_supplies = cs47l15_supplies,
		.num_parent_supplies = ARRAY_SIZE(cs47l15_supplies),
	},
};

static const char * const cs47l35_supplies[] = {
	"MICVDD",
	"DBVDD2",
	"CPVDD1",
	"CPVDD2",
	"SPKVDD",
};

static const struct mfd_cell cs47l35_devs[] = {
	{ .name = "madera-irq", },
	{ .name = "madera-micsupp" },
	{ .name = "madera-gpio", },
	{
		.name = "madera-extcon",
		.parent_supplies = cs47l35_supplies,
		.num_parent_supplies = 1, /* We only need MICVDD */
	},
	{
		.name = "cs47l35-codec",
		.parent_supplies = cs47l35_supplies,
		.num_parent_supplies = ARRAY_SIZE(cs47l35_supplies),
	},
};

static const char * const cs47l85_supplies[] = {
	"MICVDD",
	"DBVDD2",
	"DBVDD3",
	"DBVDD4",
	"CPVDD1",
	"CPVDD2",
	"SPKVDDL",
	"SPKVDDR",
};

static const struct mfd_cell cs47l85_devs[] = {
	{ .name = "madera-irq", },
	{ .name = "madera-micsupp", },
	{ .name = "madera-gpio", },
	{
		.name = "madera-extcon",
		.parent_supplies = cs47l85_supplies,
		.num_parent_supplies = 1, /* We only need MICVDD */
	},
	{
		.name = "cs47l85-codec",
		.parent_supplies = cs47l85_supplies,
		.num_parent_supplies = ARRAY_SIZE(cs47l85_supplies),
	},
};

static const char * const cs47l90_supplies[] = {
	"MICVDD",
	"DBVDD2",
	"DBVDD3",
	"DBVDD4",
	"CPVDD1",
	"CPVDD2",
};

static const struct mfd_cell cs47l90_devs[] = {
	{ .name = "madera-irq", },
	{ .name = "madera-micsupp", },
	{ .name = "madera-gpio", },
	{
		.name = "madera-extcon",
		.parent_supplies = cs47l90_supplies,
		.num_parent_supplies = 1, /* We only need MICVDD */
	},
	{
		.name = "cs47l90-codec",
		.parent_supplies = cs47l90_supplies,
		.num_parent_supplies = ARRAY_SIZE(cs47l90_supplies),
	},
};

static const char * const cs47l92_supplies[] = {
	"MICVDD",
	"CPVDD1",
	"CPVDD2",
};

static const struct mfd_cell cs47l92_devs[] = {
	{ .name = "madera-irq", },
	{ .name = "madera-micsupp", },
	{ .name = "madera-gpio" },
	{
		.name = "madera-extcon",
		.parent_supplies = cs47l92_supplies,
		.num_parent_supplies = 1, /* We only need MICVDD */
	},
	{
		.name = "cs47l92-codec",
		.parent_supplies = cs47l92_supplies,
		.num_parent_supplies = ARRAY_SIZE(cs47l92_supplies),
	},
};


const char *madera_name_from_type(enum madera_type type)
{
	switch (type) {
	case CS47L15:
		return "CS47L15";
	case CS47L35:
		return "CS47L35";
	case CS47L85:
		return "CS47L85";
	case CS47L90:
		return "CS47L90";
	case CS47L91:
		return "CS47L91";
	case CS47L92:
		return "CS47L92";
	case CS47L93:
		return "CS47L93";
	case WM1840:
		return "WM1840";
	default:
		return "Unknown";
	}
}
EXPORT_SYMBOL_GPL(madera_name_from_type);

#define MADERA_BOOT_POLL_MAX_INTERVAL_US  5000
#define MADERA_BOOT_POLL_TIMEOUT_US	 25000

static int madera_wait_for_boot(struct madera *madera)
{
	unsigned int val;
	int ret;

	/*
	 * We can't use an interrupt as we need to runtime resume to do so,
	 * so we poll the status bit. This won't race with the interrupt
	 * handler because it will be blocked on runtime resume.
	 */
	ret = regmap_read_poll_timeout(madera->regmap,
				       MADERA_IRQ1_RAW_STATUS_1,
				       val,
				       (val & MADERA_BOOT_DONE_STS1),
				       MADERA_BOOT_POLL_MAX_INTERVAL_US,
				       MADERA_BOOT_POLL_TIMEOUT_US);

	if (ret)
		dev_err(madera->dev, "Polling BOOT_DONE_STS failed: %d\n", ret);

	/*
	 * BOOT_DONE defaults to unmasked on boot so we must ack it.
	 * Do this unconditionally to avoid interrupt storms
	 */
	regmap_write(madera->regmap, MADERA_IRQ1_STATUS_1,
		     MADERA_BOOT_DONE_EINT1);

	if (ret)
		dev_err(madera->dev, "Polling BOOT_DONE_STS failed: %d\n", ret);

	pm_runtime_mark_last_busy(madera->dev);

	return ret;
}

static int madera_soft_reset(struct madera *madera)
{
	int ret;

	ret = regmap_write(madera->regmap, MADERA_SOFTWARE_RESET, 0);
	if (ret != 0) {
		dev_err(madera->dev, "Failed to soft reset device: %d\n", ret);
		return ret;
	}

	/* Allow time for internal clocks to startup after reset */
	usleep_range(1000, 2000);

	return 0;
}

static void madera_enable_hard_reset(struct madera *madera)
{
	if (madera->reset_gpio)
		gpiod_set_value_cansleep(madera->reset_gpio, 0);
}

static void madera_disable_hard_reset(struct madera *madera)
{
	if (madera->reset_gpio) {
		gpiod_set_value_cansleep(madera->reset_gpio, 1);
		usleep_range(1000, 2000);
	}
}

#ifdef CONFIG_PM
static int madera_runtime_resume(struct device *dev)
{
	struct madera *madera = dev_get_drvdata(dev);
	int ret;

	dev_dbg(dev, "Leaving sleep mode\n");

	madera_enable_hard_reset(madera);

	ret = regulator_enable(madera->dcvdd);
	if (ret) {
		dev_err(dev, "Failed to enable DCVDD: %d\n", ret);
		return ret;
	}

	regcache_cache_only(madera->regmap, false);
	regcache_cache_only(madera->regmap_32bit, false);

	madera_disable_hard_reset(madera);

	if (!madera->reset_gpio) {
		usleep_range(2000, 3000);

		ret = madera_wait_for_boot(madera);
		if (ret)
			goto err;

		ret = madera_soft_reset(madera);
		if (ret) {
			dev_err(dev, "Failed to reset: %d\n", ret);
			goto err;
		}
	}

	ret = madera_wait_for_boot(madera);
	if (ret)
		goto err;

	ret = regcache_sync(madera->regmap);
	if (ret) {
		dev_err(dev, "Failed to restore 16-bit register cache\n");
		goto err;
	}

	ret = regcache_sync(madera->regmap_32bit);
	if (ret) {
		dev_err(dev, "Failed to restore 32-bit register cache\n");
		goto err;
	}

	return 0;

err:
	regcache_cache_only(madera->regmap_32bit, true);
	regcache_cache_only(madera->regmap, true);
	regulator_disable(madera->dcvdd);

	return ret;
}

static int madera_runtime_suspend(struct device *dev)
{
	struct madera *madera = dev_get_drvdata(dev);

	dev_dbg(madera->dev, "Entering sleep mode\n");

	regcache_cache_only(madera->regmap, true);
	regcache_mark_dirty(madera->regmap);
	regcache_cache_only(madera->regmap_32bit, true);
	regcache_mark_dirty(madera->regmap_32bit);

	regulator_disable(madera->dcvdd);

	return 0;
}
#endif

const struct dev_pm_ops madera_pm_ops = {
	SET_RUNTIME_PM_OPS(madera_runtime_suspend,
			   madera_runtime_resume,
			   NULL)
};
EXPORT_SYMBOL_GPL(madera_pm_ops);

unsigned int madera_get_num_micbias(struct madera *madera)
{

	switch (madera->type) {
	case CS47L15:
		return 1;
	case CS47L35:
		return 2;
	case CS47L85:
	case WM1840:
		return 4;
	case CS47L90:
	case CS47L91:
	case CS47L92:
	case CS47L93:
		return 2;
	default:
		dev_warn(madera->dev, "No micbias known for codec %s\n",
			 madera_name_from_type(madera->type));
		return 0;
	}
}
EXPORT_SYMBOL_GPL(madera_get_num_micbias);

unsigned int madera_get_num_childbias(struct madera *madera,
				      unsigned int micbias)
{
	/*
	 * micbias argument reserved for future codecs that don't
	 * have the same number of children on each micbias
	 */

	switch (madera->type) {
	case CS47L15:
		return 3;
	case CS47L35:
		return 2;
	case CS47L85:
	case WM1840:
		return 0;
	case CS47L90:
	case CS47L91:
		return 4;
	case CS47L92:
	case CS47L93:
		switch (micbias) {
		case 1:
			return 4;
		case 2:
			return 2;
		default:
			dev_warn(madera->dev,
				 "No child micbias for codec %s, micbias %u\n",
				 madera_name_from_type(madera->type),
				 micbias);
			return 0;
		}
	default:
		dev_warn(madera->dev, "No child micbias known for codec %s\n",
			 madera_name_from_type(madera->type));
		return 0;
	}
}
EXPORT_SYMBOL_GPL(madera_get_num_childbias);

#ifdef CONFIG_OF
const struct of_device_id madera_of_match[] = {
	{ .compatible = "cirrus,cs47l15", .data = (void *)CS47L15 },
	{ .compatible = "cirrus,cs47l35", .data = (void *)CS47L35 },
	{ .compatible = "cirrus,cs47l85", .data = (void *)CS47L85 },
	{ .compatible = "cirrus,cs47l90", .data = (void *)CS47L90 },
	{ .compatible = "cirrus,cs47l91", .data = (void *)CS47L91 },
	{ .compatible = "cirrus,cs47l92", .data = (void *)CS47L92 },
	{ .compatible = "cirrus,cs47l93", .data = (void *)CS47L93 },
	{ .compatible = "cirrus,wm1840", .data = (void *)WM1840 },
	{},
};
EXPORT_SYMBOL_GPL(madera_of_match);

unsigned long madera_get_type_from_of(struct device *dev)
{
	const struct of_device_id *id = of_match_device(madera_of_match, dev);

	if (id)
		return (unsigned long)id->data;
	else
		return 0;
}
EXPORT_SYMBOL_GPL(madera_get_type_from_of);
#endif

static int madera_get_reset_gpio(struct madera *madera)
{
	int ret;

	/* We use 0 in pdata to indicate a GPIO has not been set */
	if (dev_get_platdata(madera->dev) && (madera->pdata.reset > 0)) {
		/* Start out with /RESET asserted */
		ret = devm_gpio_request_one(madera->dev,
					    madera->pdata.reset,
					    GPIOF_DIR_OUT | GPIOF_INIT_LOW,
					    "madera reset");
		if (!ret)
			madera->reset_gpio = gpio_to_desc(madera->pdata.reset);
	} else {
		madera->reset_gpio = devm_gpiod_get_optional(madera->dev,
							     "reset",
							     GPIOD_OUT_LOW);
		if (IS_ERR(madera->reset_gpio))
			ret = PTR_ERR(madera->reset_gpio);
		else
			ret = 0;
	}

	/* Ensure period of reset asserted before we apply the supplies */
	msleep(20);

	if (ret == -EPROBE_DEFER)
		return ret;

	if (ret) {
		dev_err(madera->dev, "Failed to request /RESET: %d\n", ret);
		return ret;
	}

	if (!madera->reset_gpio)
		dev_warn(madera->dev,
			 "Running without reset GPIO is not recommended\n");

	return 0;
}

static void madera_prop_get_micbias_child(struct madera *madera,
					 const char *name,
					 struct madera_micbias_pin_pdata *pdata)
{
	struct device_node *np;
	struct regulator_desc desc = { };

	np = of_get_child_by_name(madera->dev->of_node, name);
	if (!np)
		return;

	desc.name = name;
	pdata->init_data = of_get_regulator_init_data(madera->dev, np, &desc);
	of_property_read_u32(np, "regulator-active-discharge",
			     &pdata->active_discharge);
}

static void madera_prop_get_micbias_gen(struct madera *madera,
				       const char *name,
				       struct madera_micbias_pdata *pdata)
{
	struct device_node *np;
	struct regulator_desc desc = { };

	np = of_get_child_by_name(madera->dev->of_node, name);
	if (!np)
		return;

	desc.name = name;
	pdata->init_data = of_get_regulator_init_data(madera->dev, np, &desc);
	pdata->ext_cap = of_property_read_bool(np, "cirrus,ext-cap");
	of_property_read_u32(np, "regulator-active-discharge",
			     &pdata->active_discharge);
}

static void madera_prop_get_micbias(struct madera *madera)
{
	struct madera_micbias_pdata *pdata;
	char name[10];
	int i, child;

	for (i = madera_get_num_micbias(madera) - 1; i >= 0; --i) {
		pdata = &madera->pdata.micbias[i];

		snprintf(name, sizeof(name), "MICBIAS%d", i + 1);
		madera_prop_get_micbias_gen(madera, name, pdata);

		child = madera_get_num_childbias(madera, i + 1) - 1;
		for (; child >= 0; --child) {
			snprintf(name, sizeof(name), "MICBIAS%d%c",
				 i + 1, 'A' + child);
			madera_prop_get_micbias_child(madera, name,
						     &pdata->pin[child]);
		}
	}
}

static void madera_configure_micbias(struct madera *madera)
{
	unsigned int num_micbias = madera_get_num_micbias(madera);
	struct madera_micbias_pdata *pdata;
	struct regulator_init_data *init_data;
	unsigned int num_child_micbias;
	unsigned int val, mask, reg;
	int i, child, ret;

	for (i = 0; i < num_micbias; ++i) {
		pdata = &madera->pdata.micbias[i];

		/* Configure the child micbias pins */
		val = 0;
		mask = 0;
		num_child_micbias = madera_get_num_childbias(madera, i + 1);
		for (child = 0; child < num_child_micbias; ++child) {
			if (!pdata->pin[child].init_data)
				continue;

			mask |= MADERA_MICB1A_DISCH << (child * 4);
			if (pdata->pin[child].active_discharge)
				val |= MADERA_MICB1A_DISCH << (child * 4);
		}

		if (mask) {
			reg = MADERA_MIC_BIAS_CTRL_5 + (i * 2);
			ret = regmap_update_bits(madera->regmap, reg, mask, val);
			if (ret)
				dev_warn(madera->dev,
					 "Failed to write 0x%x (%d)\n",
					 reg, ret);

			dev_dbg(madera->dev,
				"Set MICBIAS_CTRL%d mask=0x%x val=0x%x\n",
				i + 5, mask, val);
		}

		/* configure the parent */
		init_data = pdata->init_data;
		if (!init_data)
			continue;

		mask = MADERA_MICB1_LVL_MASK | MADERA_MICB1_EXT_CAP |
			MADERA_MICB1_BYPASS | MADERA_MICB1_RATE;

		if (!init_data->constraints.max_uV)
			init_data->constraints.max_uV = 2800000;

		val = (init_data->constraints.max_uV - 1500000) / 100000;
		val <<= MADERA_MICB1_LVL_SHIFT;

		if (pdata->ext_cap)
			val |= MADERA_MICB1_EXT_CAP;

		/* if no child biases the discharge is set in the parent */
		if (num_child_micbias == 0) {
			mask |= MADERA_MICB1_DISCH;

			if (pdata->active_discharge)
				val |= MADERA_MICB1_DISCH;
		}

		if (init_data->constraints.soft_start)
			val |= MADERA_MICB1_RATE;

		if (init_data->constraints.valid_ops_mask &
		    REGULATOR_CHANGE_BYPASS)
			val |= MADERA_MICB1_BYPASS;

		reg = MADERA_MIC_BIAS_CTRL_1 + i;
		ret = regmap_update_bits(madera->regmap, reg, mask, val);
		if (ret)
			dev_warn(madera->dev, "Failed to write 0x%x (%d)\n",
				 reg, ret);

		dev_dbg(madera->dev, "Set MICBIAS_CTRL%d mask=0x%x val=0x%x\n",
			i + 1, mask, val);
	}
}

static int madera_dev_select_pinctrl(struct madera *madera,
				     struct pinctrl *pinctrl,
				     const char *name)
{
	struct pinctrl_state *pinctrl_state;
	int ret;

	pinctrl_state = pinctrl_lookup_state(pinctrl, name);

	/* it's ok if it doesn't exist */
	if (!IS_ERR(pinctrl_state)) {
		dev_dbg(madera->dev, "Applying pinctrl %s state\n", name);
		ret = pinctrl_select_state(pinctrl, pinctrl_state);
		if (ret) {
			dev_err(madera->dev,
				"Failed to select pinctrl %s state: %d\n",
				name, ret);
			return ret;
		}
	}

	return 0;
}

int madera_dev_init(struct madera *madera)
{
	struct device *dev = madera->dev;
	const char *name;
	unsigned int hwid;
	int (*patch_fn)(struct madera *) = NULL;
	const struct mfd_cell *mfd_devs;
	struct pinctrl *pinctrl;
	int n_devs = 0;
	int i, ret;

	dev_set_drvdata(madera->dev, madera);

	/*
	 * Pinctrl subsystem only configures pinctrls if all referenced pins
	 * are registered. Create our pinctrl child now so that its pins exist
	 * otherwise external pinctrl dependencies will fail
	 * Note: Can't devm_ because it is cleaned up after children are already
	 * destroyed
	 */
	ret = mfd_add_devices(madera->dev, PLATFORM_DEVID_NONE,
			      madera_pinctrl_dev, 1, NULL, 0, NULL);
	if (ret) {
		dev_err(madera->dev, "Failed to add pinctrl child: %d\n", ret);
		return ret;
	}

	pinctrl = pinctrl_get(dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		dev_err(madera->dev, "Failed to get pinctrl: %d\n", ret);
		goto err_devs;
	}

	/* Use (optional) minimal config with only external pin bindings */
	ret = madera_dev_select_pinctrl(madera, pinctrl, "probe");
	if (ret)
		goto err_pinctrl;

	mutex_init(&madera->reg_setting_lock);

	BLOCKING_INIT_NOTIFIER_HEAD(&madera->notifier);

	if (dev_get_platdata(madera->dev)) {
		memcpy(&madera->pdata, dev_get_platdata(madera->dev),
		       sizeof(madera->pdata));
	}

	ret = madera_get_reset_gpio(madera);
	if (ret)
		goto err_pinctrl;

	madera_prop_get_micbias(madera);

	regcache_cache_only(madera->regmap, true);
	regcache_cache_only(madera->regmap_32bit, true);

	for (i = 0; i < ARRAY_SIZE(madera_core_supplies); i++)
		madera->core_supplies[i].supply = madera_core_supplies[i];

	madera->num_core_supplies = ARRAY_SIZE(madera_core_supplies);

	switch (madera->type) {
	case CS47L15:
	case CS47L35:
	case CS47L90:
	case CS47L91:
	case CS47L92:
	case CS47L93:
		break;
	case CS47L85:
	case WM1840:
		/* On these DCVDD could be supplied from the onboard LDO1 */
		ret = mfd_add_devices(madera->dev, PLATFORM_DEVID_NONE,
				      madera_ldo1_devs,
				      ARRAY_SIZE(madera_ldo1_devs),
				      NULL, 0, NULL);
		if (ret) {
			dev_err(dev, "Failed to add LDO1 child: %d\n", ret);
			goto err_pinctrl;
		}
		break;
	default:
		dev_err(madera->dev, "Unknown device type %d\n", madera->type);
		ret = -ENODEV;
		goto err_pinctrl;
	}

	ret = devm_regulator_bulk_get(dev, madera->num_core_supplies,
				      madera->core_supplies);
	if (ret) {
		dev_err(dev, "Failed to request core supplies: %d\n", ret);
		goto err_pinctrl;
	}

	/*
	 * Don't use devres here because the only device we have to get
	 * against is the MFD device and DCVDD will likely be supplied by
	 * one of its children. Meaning that the regulator will be
	 * destroyed by the time devres calls regulator put.
	 */
	madera->dcvdd = regulator_get(madera->dev, "DCVDD");
	if (IS_ERR(madera->dcvdd)) {
		ret = PTR_ERR(madera->dcvdd);
		dev_err(dev, "Failed to request DCVDD: %d\n", ret);
		goto err_pinctrl;
	}

	ret = regulator_bulk_enable(madera->num_core_supplies,
				    madera->core_supplies);
	if (ret) {
		dev_err(dev, "Failed to enable core supplies: %d\n", ret);
		goto err_dcvdd;
	}

	ret = regulator_enable(madera->dcvdd);
	if (ret) {
		dev_err(dev, "Failed to enable DCVDD: %d\n", ret);
		goto err_enable;
	}

	madera_disable_hard_reset(madera);

	regcache_cache_only(madera->regmap, false);
	regcache_cache_only(madera->regmap_32bit, false);

	/* If we don't have a reset GPIO use a soft reset */
	if (!madera->reset_gpio) {
		ret = madera_soft_reset(madera);
		if (ret)
			goto err_reset;
	}

	ret = madera_wait_for_boot(madera);
	if (ret) {
		dev_err(madera->dev, "Device failed initial boot: %d\n", ret);
		goto err_reset;
	}

	/*
	 * Verify that this is a chip we know about before we
	 * starting doing any writes to its registers
	 */
	ret = regmap_read(madera->regmap, MADERA_SOFTWARE_RESET, &hwid);
	if (ret) {
		dev_err(dev, "Failed to read ID register: %d\n", ret);
		goto err_reset;
	}

	switch (hwid) {
	case CS47L15_SILICON_ID:
	case CS47L35_SILICON_ID:
	case CS47L85_SILICON_ID:
	case CS47L90_SILICON_ID:
	case CS47L92_SILICON_ID:
		break;
	default:
		dev_err(madera->dev, "Unknown device ID: %x\n", hwid);
		ret = -EINVAL;
		goto err_reset;
	}


	ret = regmap_read(madera->regmap, MADERA_HARDWARE_REVISION,
			  &madera->rev);
	if (ret) {
		dev_err(dev, "Failed to read revision register: %d\n", ret);
		goto err_reset;
	}
	madera->rev &= MADERA_HW_REVISION_MASK;

	name = madera_name_from_type(madera->type);

	switch (hwid) {
	case CS47L15_SILICON_ID:
		if (IS_ENABLED(CONFIG_MFD_CS47L15)) {
			switch (madera->type) {
			case CS47L15:
				patch_fn = cs47l15_patch;
				mfd_devs = cs47l15_devs;
				n_devs = ARRAY_SIZE(cs47l15_devs);
				break;
			default:
				break;
			}
		}
		break;
	case CS47L35_SILICON_ID:
		if (IS_ENABLED(CONFIG_MFD_CS47L35)) {
			switch (madera->type) {
			case CS47L35:
				patch_fn = cs47l35_patch;
				mfd_devs = cs47l35_devs;
				n_devs = ARRAY_SIZE(cs47l35_devs);
				break;
			default:
				break;
			}
		}
		break;
	case CS47L85_SILICON_ID:
		if (IS_ENABLED(CONFIG_MFD_CS47L85)) {
			switch (madera->type) {
			case CS47L85:
			case WM1840:
				patch_fn = cs47l85_patch;
				mfd_devs = cs47l85_devs;
				n_devs = ARRAY_SIZE(cs47l85_devs);
				break;
			default:
				break;
			}
		}
		break;
	case CS47L90_SILICON_ID:
		if (IS_ENABLED(CONFIG_MFD_CS47L90)) {
			switch (madera->type) {
			case CS47L90:
			case CS47L91:
				patch_fn = cs47l90_patch;
				mfd_devs = cs47l90_devs;
				n_devs = ARRAY_SIZE(cs47l90_devs);
				break;
			default:
				break;
			}
		}
		break;
	case CS47L92_SILICON_ID:
		if (IS_ENABLED(CONFIG_MFD_CS47L92)) {
			switch (madera->type) {
			case CS47L92:
			case CS47L93:
				patch_fn = cs47l92_patch;
				mfd_devs = cs47l92_devs;
				n_devs = ARRAY_SIZE(cs47l92_devs);
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	if (!n_devs) {
		dev_err(madera->dev, "Device ID 0x%x not a %s\n", hwid, name);
		ret = -ENODEV;
		goto err_reset;
	}

	dev_info(dev, "%s silicon revision %d\n", name, madera->rev);

	/* Apply hardware patch */
	if (patch_fn) {
		ret = patch_fn(madera);
		if (ret) {
			dev_err(madera->dev, "Failed to apply patch %d\n", ret);
			goto err_reset;
		}
	}

	/* Apply (optional) main pinctrl config, this will configure our pins */
	ret = madera_dev_select_pinctrl(madera, pinctrl, "active");
	if (ret)
		goto err_reset;

	/* Init 32k clock sourced from MCLK2 */
	ret = regmap_update_bits(madera->regmap,
			MADERA_CLOCK_32K_1,
			MADERA_CLK_32K_ENA_MASK | MADERA_CLK_32K_SRC_MASK,
			MADERA_CLK_32K_ENA | MADERA_32KZ_MCLK2);
	if (ret) {
		dev_err(madera->dev, "Failed to init 32k clock: %d\n", ret);
		goto err_reset;
	}

	madera_configure_micbias(madera);

	pm_runtime_set_active(madera->dev);
	pm_runtime_enable(madera->dev);
	pm_runtime_set_autosuspend_delay(madera->dev, 100);
	pm_runtime_use_autosuspend(madera->dev);

	ret = mfd_add_devices(madera->dev, PLATFORM_DEVID_NONE,
			      mfd_devs, n_devs,
			      NULL, 0, NULL);
	if (ret) {
		dev_err(madera->dev, "Failed to add subdevices: %d\n", ret);
		goto err_pm_runtime;
	}

	pinctrl_put(pinctrl);

	return 0;

err_pm_runtime:
	pm_runtime_disable(madera->dev);
err_reset:
	madera_enable_hard_reset(madera);
	regulator_disable(madera->dcvdd);
err_enable:
	regulator_bulk_disable(madera->num_core_supplies,
			       madera->core_supplies);
err_dcvdd:
	regulator_put(madera->dcvdd);
err_pinctrl:
	pinctrl_put(pinctrl);
err_devs:
	mfd_remove_devices(dev);

	return ret;
}
EXPORT_SYMBOL_GPL(madera_dev_init);

int madera_dev_exit(struct madera *madera)
{
	/* Prevent any IRQs being serviced while we clean up */
	disable_irq(madera->irq);

	/*
	 * DCVDD could be supplied by a child node, we must disable it before
	 * removing the children, and prevent PM runtime from turning it back on
	 */
	pm_runtime_disable(madera->dev);

	regulator_disable(madera->dcvdd);
	regulator_put(madera->dcvdd);

	mfd_remove_devices(madera->dev);
	madera_enable_hard_reset(madera);

	regulator_bulk_disable(madera->num_core_supplies,
			       madera->core_supplies);
	return 0;
}
EXPORT_SYMBOL_GPL(madera_dev_exit);
