// SPDX-License-Identifier: GPL-2.0+
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2024. All rights reserved.
 * 2024-06-03 File created.
 */

#include <linux/module.h>
#include <linux/list.h>
#include "spkr-amp-mngr.h"

#define SPKR_AMP_VERSION "v1.0.1"
#define SPKR_AMP_W_NAME  "Ext AMP"
#define SPKR_AMP_PREFIX  "SPK"

static LIST_HEAD(spkr_amp_list);
static DEFINE_MUTEX(spkr_amp_mutex);
static struct spkr_amp_mngr spkr_amp_mngr;

static int spkr_amp_get_devid(const char *name)
{
	int ndev = spkr_amp_mngr.ndev_dts;
	int id;

	if (ndev < 1 || ndev > SPKR_AMP_MAX)
		return -EINVAL;

	if (strlen(name) <= 3)
		return 0;

	if (strncmp(name, spkr_amp_mngr.spk_prefix, 3))
		return 0;

	id = name[3] - '0';
	if (id >= 0 && id <= ndev)
		return id;

	pr_err("%s: Invalid id-ndev: %d - %d\n", __func__, id, ndev);

	return -EINVAL;
}

static int spkr_amp_append_spk_prefix(struct device *dev,
		int id, const char **name)
{
	char str[SPKR_STRING_MAX];

	if (dev == NULL)
		return -EINVAL;

	if (name == NULL || *name == NULL)
		return 0;

	snprintf(str, sizeof(str), "%s%d %s",
			spkr_amp_mngr.spk_prefix, id, *name);

	*name = devm_kstrdup(dev, str, GFP_KERNEL);
	if (*name == NULL)
		return -ENOMEM;

	pr_debug("%s: name:%s\n", __func__, *name);

	return 0;
}

static struct spkr_amp *spkr_amp_get_pdev(int id)
{
	struct spkr_amp *spkr_amp;

	if (list_empty(&spkr_amp_list)) {
		pr_err("%s: spkr_amp_list empty\n", __func__);
		return NULL;
	}
	list_for_each_entry(spkr_amp, &spkr_amp_list, list) {
		pr_info("%s: id(%d) - spkr_amp->id(%d)\n", __func__, id, spkr_amp->id);
		if (id != spkr_amp->id)
			continue;
		return spkr_amp;
	}

	return NULL;
}

static int spkr_amp_init_dev(int id, bool force)
{
	struct spkr_amp *spkr_amp;
	int ret;

	spkr_amp = spkr_amp_get_pdev(id);
	if (spkr_amp == NULL)
		return -ENODEV;

	if (!spkr_amp->amp_ops.dev_init)
		return -ENOTSUPP;

	pr_info("%s: init dev: %d - %d\n", __func__, id, force);
	ret = spkr_amp->amp_ops.dev_init(id, force);
	if (ret)
		pr_err("%s: Failed to init dev:%d\n", __func__, ret);

	return ret;
}

static int spkr_amp_set_mode(int id, int mode)
{
	struct spkr_amp *spkr_amp;
	int ret;

	spkr_amp = spkr_amp_get_pdev(id);
	if (spkr_amp == NULL)
		return -ENODEV;

	if (!spkr_amp->amp_ops.set_mode)
		return -ENOTSUPP;

	pr_info("%s: set mode: %d - %d\n", __func__, id, mode);
	ret = spkr_amp->amp_ops.set_mode(id, mode);
	if (ret)
		pr_err("%s: Failed to set mode:%d\n", __func__, ret);

	return ret;
}

static int spkr_amp_switch(int id, bool on)
{
	struct spkr_amp *spkr_amp;
	int ret;

	spkr_amp = spkr_amp_get_pdev(id);
	if (spkr_amp == NULL)
		return -ENODEV;

	if (!spkr_amp->amp_ops.amp_switch)
		return -ENOTSUPP;

	pr_info("%s: amp switch: %d - %d\n", __func__, id, on);
	ret = spkr_amp->amp_ops.amp_switch(id, on);
	if (ret)
		pr_err("%s: Failed to set switch:%d\n", __func__, ret);

	return ret;
}

static int spkr_amp_switch_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	char spkr_switch;
	int id;

	id = spkr_amp_get_devid(kc->id.name);
	if (id < 0) {
		pr_err("%s: Invalid id:%d\n", __func__, id);
		return -EINVAL;
	}

	mutex_lock(&spkr_amp_mutex);
	spkr_switch = spkr_amp_mngr.spkr_switch[id];
	uc->value.integer.value[0] = spkr_switch;
	mutex_unlock(&spkr_amp_mutex);

	return 0;
}

static int spkr_amp_switch_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	char spkr_switch;
	int id, ret;

	id = spkr_amp_get_devid(kc->id.name);
	if (id < 0) {
		pr_err("%s: Invalid id:%d\n", __func__, id);
		return -EINVAL;
	}

	mutex_lock(&spkr_amp_mutex);
	spkr_switch = (char)uc->value.integer.value[0];
	ret = spkr_amp_switch(id, spkr_switch);
	mutex_unlock(&spkr_amp_mutex);
	if (ret) {
		pr_err("%s: Failed to set switch:%d\n", __func__, ret);
		return ret;
	}

	spkr_amp_mngr.spkr_switch[id] = spkr_switch;

	return 0;
}

static int spkr_amp_mode_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	char mode;
	int id;

	id = spkr_amp_get_devid(kc->id.name);
	if (id < 0) {
		pr_err("%s: Invalid id:%d\n", __func__, id);
		return -EINVAL;
	}

	mutex_lock(&spkr_amp_mutex);
	mode = spkr_amp_mngr.amp_mode[id];
	uc->value.integer.value[0] = mode;
	mutex_unlock(&spkr_amp_mutex);

	return 0;
}

static int spkr_amp_mode_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	char mode;
	int id, ret;

	id = spkr_amp_get_devid(kc->id.name);
	if (id < 0) {
		pr_err("%s: Invalid id:%d\n", __func__, id);
		return -EINVAL;
	}

	mutex_lock(&spkr_amp_mutex);
	mode = (char)uc->value.integer.value[0];
	ret = spkr_amp_set_mode(id, mode);
	mutex_unlock(&spkr_amp_mutex);
	if (ret) {
		pr_err("%s: Failed to set mode:%d\n", __func__, ret);
		return ret;
	}

	spkr_amp_mngr.amp_mode[id] = mode;

	return 0;
}

static int spkr_amp_dapm_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kc, int event)
{
	int id;
	int ret;

	id = spkr_amp_get_devid(w->name);
	if (id < 0) {
		pr_err("%s: Invalid id:%d\n", __func__, id);
		return -EINVAL;
	}

	pr_info("%s: id:%d event:%d\n", __func__, id, event);
	mutex_lock(&spkr_amp_mutex);
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* spk amp on control */
		ret = spkr_amp_switch(id, true);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* spk amp off control */
		ret = spkr_amp_switch(id, false);
		break;
	default:
		break;
	}
	mutex_unlock(&spkr_amp_mutex);

	return 0;
};

static const struct snd_kcontrol_new spkr_amp_kcontrols[] = {
	SOC_SINGLE_EXT(SPKR_AMP_W_NAME " Switch", SND_SOC_NOPM, 0, 1, 0,
			spkr_amp_switch_get, spkr_amp_switch_put),
	SOC_SINGLE_EXT(SPKR_AMP_W_NAME " Mode", SND_SOC_NOPM, 0, 15, 0,
			spkr_amp_mode_get, spkr_amp_mode_put),
};

static const struct snd_kcontrol_new spkr_amp_controls[] = {
	SOC_DAPM_PIN_SWITCH(SPKR_AMP_W_NAME),
	SOC_SINGLE_EXT(SPKR_AMP_W_NAME " Mode", SND_SOC_NOPM, 0, 15, 0,
			spkr_amp_mode_get, spkr_amp_mode_put),
};

static const struct snd_soc_dapm_widget spkr_amp_widgets[] = {
	SND_SOC_DAPM_SPK(SPKR_AMP_W_NAME, spkr_amp_dapm_event),
};

static const struct snd_soc_dapm_route spkr_amp_routes[] = {
	/* TODO */
//	{SPKR_AMP_W_NAME, NULL, "HPHL"}, /* qcom kona */
//	{SPKR_AMP_W_NAME, NULL, "HPHR"},
//	{SPKR_AMP_W_NAME, NULL, "AUX"},
//	{SPKR_AMP_W_NAME, NULL, "EAR"},
	{SPKR_AMP_W_NAME, NULL, "Receiver"}, /* mtk mt6539 */
	{SPKR_AMP_W_NAME, NULL, "LINEOUT L"},
	{SPKR_AMP_W_NAME, NULL, "Headphone L Ext Spk Amp"},
	{SPKR_AMP_W_NAME, NULL, "Headphone R Ext Spk Amp"},
};

static int spkr_amp_init_controls(struct snd_soc_card *card, int id)
{
	struct snd_kcontrol_new *new_kctrl, *kc;
	const char *name;
	int i, count;

	if (card == NULL)
		return -EINVAL;

	if (spkr_amp_mngr.dapm_register) {
		new_kctrl = devm_kzalloc(card->dev,
				sizeof(spkr_amp_controls), GFP_KERNEL);
		if (new_kctrl == NULL)
			return -ENOMEM;

		memcpy(new_kctrl, spkr_amp_controls,
				sizeof(spkr_amp_controls));
		count = ARRAY_SIZE(spkr_amp_controls);
	} else {
		new_kctrl = devm_kzalloc(card->dev,
				sizeof(spkr_amp_kcontrols), GFP_KERNEL);
		if (new_kctrl == NULL)
			return -ENOMEM;

		memcpy(new_kctrl, spkr_amp_kcontrols,
				sizeof(spkr_amp_kcontrols));
		count = ARRAY_SIZE(spkr_amp_kcontrols);
	}

	for (i = 0, kc = new_kctrl; i < count; i++, kc++) {
		spkr_amp_append_spk_prefix(card->dev, id,
				(const char **)&kc->name);
		if (!spkr_amp_mngr.dapm_register)
			continue;
		if (!strnstr(kc->name, SPKR_AMP_W_NAME " Switch",
				strlen(kc->name)))
			continue;
		name = (const char *)kc->private_value;
		spkr_amp_append_spk_prefix(card->dev, id, &name);
		kc->private_value = (unsigned long)name;
	}

	return snd_soc_add_card_controls(card, new_kctrl, count);
}

static int spkr_amp_init_widgets(struct device *dev, int id,
		struct snd_soc_dapm_context *dapm)
{
	struct snd_soc_dapm_widget *new_widget;
	int i, count;

	if (dev == NULL || dapm == NULL)
		return -EINVAL;

	new_widget = devm_kzalloc(dev, sizeof(spkr_amp_widgets), GFP_KERNEL);
	if (new_widget == NULL)
		return -ENOMEM;

	memcpy(new_widget, spkr_amp_widgets, sizeof(spkr_amp_widgets));
	count = ARRAY_SIZE(spkr_amp_widgets);

	for (i = 0; i < count; i++)
		spkr_amp_append_spk_prefix(dev, id, &new_widget[i].name);

	return snd_soc_dapm_new_controls(dapm, new_widget, count);
}

static int spkr_amp_init_route(struct device *dev, int id,
		struct snd_soc_dapm_context *dapm)
{
	struct snd_soc_dapm_route *new_route;
	int i, count;

	if (dev == NULL || dapm == NULL)
		return -EINVAL;

	new_route = devm_kzalloc(dev, sizeof(spkr_amp_routes), GFP_KERNEL);
	if (new_route == NULL)
		return -ENOMEM;

	memcpy(new_route, spkr_amp_routes, sizeof(spkr_amp_routes));
	count = ARRAY_SIZE(spkr_amp_routes);

	for (i = 0; i < count; i++)
		spkr_amp_append_spk_prefix(dev, id, &new_route[i].sink);

	return snd_soc_dapm_add_routes(dapm, new_route, count);
}

static int spkr_amp_add_widgets_no_prefix(struct snd_soc_card *card,
		struct snd_soc_dapm_context *dapm)
{
	int ret;

	if (card == NULL || dapm == NULL)
		return -EINVAL;

	ret = snd_soc_dapm_new_controls(dapm,
			spkr_amp_widgets, ARRAY_SIZE(spkr_amp_widgets));
	ret |= snd_soc_add_card_controls(card,
			spkr_amp_controls, ARRAY_SIZE(spkr_amp_controls));
	ret |= snd_soc_dapm_add_routes(dapm,
			spkr_amp_routes, ARRAY_SIZE(spkr_amp_routes));
	if (ret) {
		dev_err(card->dev, "Failed to add amp widgets:%d\n", ret);
		return ret;
	}

	snd_soc_dapm_disable_pin(dapm, SPKR_AMP_W_NAME);
	snd_soc_dapm_ignore_suspend(dapm, SPKR_AMP_W_NAME);

	return 0;
}

static int spkr_amp_add_widgets(struct snd_soc_card *card, int id,
		struct snd_soc_dapm_context *dapm)
{
	char *name;
	int ret;

	if (card == NULL || dapm == NULL)
		return -EINVAL;

	if (id < 1)
		return spkr_amp_add_widgets_no_prefix(card, dapm);

	ret  = spkr_amp_init_widgets(card->dev, id, dapm);
	ret |= spkr_amp_init_controls(card, id);
	ret |= spkr_amp_init_route(card->dev, id, dapm);
	if (ret) {
		dev_err(card->dev, "Failed to add amp widgets:%d\n", ret);
		return ret;
	}

	name = kzalloc(SPKR_STRING_MAX, GFP_KERNEL);
	if (name == NULL)
		return -ENOMEM;

	snprintf(name, SPKR_STRING_MAX, "%s%d %s",
			spkr_amp_mngr.spk_prefix, id, SPKR_AMP_W_NAME);
	snd_soc_dapm_disable_pin(dapm, name);
	snd_soc_dapm_ignore_suspend(dapm, name);
	kfree(name);

	return 0;
}

static int spkr_amp_add_kcontrols(struct snd_soc_card *card, int id)
{
	int ret;

	if (card == NULL)
		return -EINVAL;

	if (id < 1)
		return snd_soc_add_card_controls(card,
			spkr_amp_kcontrols, ARRAY_SIZE(spkr_amp_kcontrols));

	ret = spkr_amp_init_controls(card, id);
	if (ret) {
		dev_err(card->dev, "Failed to add amp kcontrols:%d\n", ret);
		return ret;
	}

	return 0;
}

int spkr_amp_dev_register(struct spkr_amp *spkr_amp)
{
	mutex_lock(&spkr_amp_mutex);
	INIT_LIST_HEAD(&spkr_amp->list);
	list_add_tail(&spkr_amp->list, &spkr_amp_list);
	spkr_amp_mngr.ndev_list++;
	mutex_unlock(&spkr_amp_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(spkr_amp_dev_register);

int spkr_amp_dapm_init(struct snd_soc_card *card)
{
	struct snd_soc_dapm_context *dapm;
	bool succ = false;
	int ndev, ndev2;
	int i, id;
	int ret;

	if (card == NULL || card->dev == NULL)
		return -EINVAL;

	ndev = spkr_amp_mngr.ndev_dts;
	ndev2 = spkr_amp_mngr.ndev_list;
	pr_info("%s: ndev: %d - %d\n", __func__, ndev, ndev2);
	if (ndev == 0 || ndev2 == 0) {
		dev_info(card->dev, "Skip init spkr_amp widgets\n");
		return 0;
	}

	dapm = &card->dapm;
	if (dapm == NULL) {
		dev_err(card->dev, "Failed to get dapm\n");
		return -EINVAL;
	}

	mutex_lock(&spkr_amp_mutex);
	for (i = 0; i < ndev; i++) {
		id = (ndev > 1) ? i + 1 : i;
		dev_info(card->dev, "init spkr_amp%d dapm\n", id);
		ret = spkr_amp_init_dev(id, true);
		if (ret)
			continue;
		if (spkr_amp_mngr.dapm_register) {
			ret = spkr_amp_add_widgets(card, id, dapm);
			if (!ret)
				succ = true;
		} else {
			ret = spkr_amp_add_kcontrols(card, id);
		}
	}

	if (succ)
		snd_soc_dapm_sync(dapm);

	mutex_unlock(&spkr_amp_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(spkr_amp_dapm_init);

static int spkr_amp_parse_dts(struct spkr_amp_mngr *amp_mngr)
{
	struct device_node *np;
	int ret;

	if (amp_mngr == NULL)
		return -EINVAL;

	np = amp_mngr->dev->of_node;
	ret = of_property_read_s32(np, "audio,amp-ndev",
			&amp_mngr->ndev_dts);
	if (ret)
		amp_mngr->ndev_dts = 0;

	ret = of_property_read_string(np, "audio,spk-prefix",
			&amp_mngr->spk_prefix);
	if (ret)
		amp_mngr->spk_prefix = SPKR_AMP_PREFIX;

	ret = of_property_read_u32(np, "audio,register-dapm",
			&amp_mngr->dapm_register);
	if (ret)
		amp_mngr->dapm_register = 1;

	dev_info(amp_mngr->dev, "amp-ndev:%d spk-prefix:%s register-dapm:%d\n",
			amp_mngr->ndev_dts, amp_mngr->spk_prefix, amp_mngr->dapm_register);

	return 0;
}

static int spkr_amp_probe(struct platform_device *pdev)
{
	struct spkr_amp_mngr *amp_mngr;
	int ret;

	dev_info(&pdev->dev, "Version: %s\n", SPKR_AMP_VERSION);

	amp_mngr = &spkr_amp_mngr;
	amp_mngr->dev = &pdev->dev;
	platform_set_drvdata(pdev, amp_mngr);

	ret = spkr_amp_parse_dts(amp_mngr);
	if (ret)
		return ret;

	return 0;
}

static int spkr_amp_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id spkr_amp_of_match[] = {
	{ .compatible = "audio,spkr-amp-mngr", },
	{},
};
MODULE_DEVICE_TABLE(of, spkr_amp_of_match);

static struct platform_driver spkr_amp_driver = {
	.driver = {
		.name = "spkr-amp-mngr",
		.of_match_table = spkr_amp_of_match,
	},
	.probe = spkr_amp_probe,
	.remove = spkr_amp_remove,
};

module_platform_driver(spkr_amp_driver);

MODULE_AUTHOR("FourSemi SW <support@foursemi.com>");
MODULE_DESCRIPTION("Speaker Amplifier Manager Driver");
MODULE_VERSION(SPKR_AMP_VERSION);
MODULE_LICENSE("GPL");
