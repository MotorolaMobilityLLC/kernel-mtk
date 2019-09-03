/*
 * Platform data for Cirrus Logic Madera codecs
 *
 * Copyright 2015-2017 Cirrus Logic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef MADERA_PDATA_H
#define MADERA_PDATA_H

#include <linux/kernel.h>
#include <linux/irqchip/irq-madera-pdata.h>
#include <linux/regulator/arizona-ldo1.h>
#include <linux/regulator/arizona-micsupp.h>
#include <linux/regulator/machine.h>
#include <sound/madera-pdata.h>
#include <linux/extcon/extcon-madera-pdata.h>

#define MADERA_MAX_MICBIAS		4
#define MADERA_MAX_CHILD_MICBIAS	4

#define MADERA_MAX_GPSW			2

struct pinctrl_map;
struct regulator_init_data;

/**
 * struct madera_micbias_pin_pdata - MICBIAS pin configuration
 *
 * @init_data: initialization data for the regulator
 */
struct madera_micbias_pin_pdata {
	struct regulator_init_data *init_data;
	u32 active_discharge;
};

/**
 * struct madera_micbias_pdata - Regulator configuration for an on-chip MICBIAS
 *
 * @init_data: initialization data for the regulator
 * @ext_cap:   set to true if an external capacitor is fitted
 * @pin:       Configuration for each output pin from this MICBIAS
 */
struct madera_micbias_pdata {
	struct regulator_init_data *init_data;
	u32 active_discharge;
	bool ext_cap;

	struct madera_micbias_pin_pdata pin[MADERA_MAX_CHILD_MICBIAS];
};

/**
 * struct madera_pdata - Configuration data for Madera devices
 *
 * @reset:	    GPIO controlling /RESET (0 = none)
 * @ldo1:	    Substruct of pdata for the LDO1 regulator
 * @micvdd:	    Substruct of pdata for the MICVDD regulator
 * @irqchip:	    Substruct of pdata for the irqchip driver
 * @gpio_base:	    Base GPIO number
 * @gpio_configs:   Array of GPIO configurations (See Documentation/pinctrl.txt)
 * @n_gpio_configs: Number of entries in gpio_configs
 * @codec:	    Substructure of pdata for the ASoC codec driver
 *		    See include/sound/madera-pdata.h
 * @gpsw:	    General purpose switch mode setting (See the SW1_MODE field
 *		    in the datasheet for the available values for your codec)
 */
struct madera_pdata {
	int reset;

	struct arizona_ldo1_pdata ldo1;
	struct arizona_micsupp_pdata micvdd;

	struct madera_irqchip_pdata irqchip;

	int gpio_base;

	const struct pinctrl_map *gpio_configs;
	int n_gpio_configs;

	/** MICBIAS configurations */
	struct madera_micbias_pdata micbias[MADERA_MAX_MICBIAS];

	struct madera_codec_pdata codec;

	u32 gpsw[MADERA_MAX_GPSW];

	/** Accessory detection configurations */
	struct madera_accdet_pdata accdet[MADERA_MAX_ACCESSORY];
};

#endif

