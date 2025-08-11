/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-07-20 File created.
 */

#ifndef __FRSM_PINCTRL_H__
#define __FRSM_PINCTRL_H__

#include <linux/pinctrl/consumer.h>
#include "internal.h"

static int frsm_pinctrl_lookup(struct frsm_dev *frsm_dev,
		struct pinctrl_state **state, const char *name)
{
	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	*state = pinctrl_lookup_state(frsm_dev->pinc, name);
	if (IS_ERR_OR_NULL(*state)) {
		dev_err(frsm_dev->dev, "Failed to lookup state: %s\n", name);
		return PTR_ERR(*state);
	}

	return 0;
}

static int frsm_pinctrl_select(struct frsm_dev *frsm_dev,
		struct pinctrl_state *state)
{
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	if (IS_ERR_OR_NULL(state))
		return -EINVAL;

	ret = pinctrl_select_state(frsm_dev->pinc, state);
	if (ret) {
		//dev_err(frsm_dev->dev, "Failed to select state[%s]: %d\n",
		//		state->name, ret);
		return ret;
	}

	return 0;
}

static int frsm_pinctrl_parse_dts(struct frsm_dev *frsm_dev)
{
	struct frsm_platform_data *pdata;
	struct frsm_gpio *gpio;
	char *name;
	int ret;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	pdata = frsm_dev->pdata;
	if (pdata->spkr_id <= 0) {
		gpio = pdata->gpio + FRSM_PIN_SDZ;
		ret = frsm_pinctrl_lookup(frsm_dev,
				&gpio->state_sleep, "frsm_sdz_sleep");
		ret |= frsm_pinctrl_select(frsm_dev, gpio->state_sleep);
		ret |= frsm_pinctrl_lookup(frsm_dev,
				&gpio->state_active, "frsm_sdz_active");
		gpio = pdata->gpio + FRSM_PIN_INTZ;
		ret = frsm_pinctrl_lookup(frsm_dev,
				&gpio->state_sleep, "frsm_intz_sleep");
		ret |= frsm_pinctrl_select(frsm_dev, gpio->state_sleep);
		ret |= frsm_pinctrl_lookup(frsm_dev,
				&gpio->state_active, "frsm_intz_active");
		return 0;
	}

	name = kzalloc(FRSM_STRING_MAX, GFP_KERNEL);
	if (name == NULL)
		return -ENOMEM;

	gpio = pdata->gpio + FRSM_PIN_SDZ;
	snprintf(name, FRSM_STRING_MAX, "frsm_sdz%d_sleep", pdata->spkr_id);
	ret = frsm_pinctrl_lookup(frsm_dev, &gpio->state_sleep, name);
	ret |= frsm_pinctrl_select(frsm_dev, gpio->state_sleep);
	snprintf(name, FRSM_STRING_MAX, "frsm_sdz%d_active", pdata->spkr_id);
	ret |= frsm_pinctrl_lookup(frsm_dev, &gpio->state_active, name);
	gpio = pdata->gpio + FRSM_PIN_INTZ;
	snprintf(name, FRSM_STRING_MAX, "frsm_intz%d_sleep", pdata->spkr_id);
	ret = frsm_pinctrl_lookup(frsm_dev, &gpio->state_sleep, name);
	ret |= frsm_pinctrl_select(frsm_dev, gpio->state_sleep);
	snprintf(name, FRSM_STRING_MAX, "frsm_intz%d_active", pdata->spkr_id);
	ret |= frsm_pinctrl_lookup(frsm_dev, &gpio->state_active, name);
	kfree(name);

	return 0;
}

static int frsm_pinctrl_init(struct frsm_dev *frsm_dev)
{
	struct pinctrl *pinc;
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	pinc = devm_pinctrl_get(frsm_dev->dev);
	if (IS_ERR_OR_NULL(pinc)) {
		ret = PTR_ERR(pinc);
		dev_dbg(frsm_dev->dev, "Failed to get pinctrl:%d\n", ret);
		return ret;
	}

	frsm_dev->pinc = pinc;
	ret = frsm_pinctrl_parse_dts(frsm_dev);

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static void frsm_pinctrl_deinit(struct frsm_dev *frsm_dev)
{
	if (frsm_dev->pinc == NULL)
		return;

	devm_pinctrl_put(frsm_dev->pinc);
	frsm_dev->pinc = NULL;
}

#endif // __FRSM_PINCTRL_H__
