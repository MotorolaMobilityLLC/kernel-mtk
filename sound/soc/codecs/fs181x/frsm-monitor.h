/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-08-14 File created.
 */

#ifndef __FRSM_MONITOR_H__
#define __FRSM_MONITOR_H__

#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include "internal.h"

static int frsm_mntr_irq_switch(struct frsm_dev *frsm_dev, bool enable)
{
	if (frsm_dev == NULL)
		return -EINVAL;

	if (frsm_dev->irq_id < 0)
		return 0;

	if (enable) {
		enable_irq(frsm_dev->irq_id);
		dev_dbg(frsm_dev->dev, "IRQ enabled\n");
	} else {
		disable_irq(frsm_dev->irq_id);
		dev_dbg(frsm_dev->dev, "IRQ disabled\n");
		cancel_delayed_work_sync(&frsm_dev->irq_work);
	}

	return 0;
}

static int frsm_mntr_switch(struct frsm_dev *frsm_dev, bool enable)
{
	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	if (enable) {
		queue_delayed_work(frsm_dev->thread_wq,
				&frsm_dev->delay_work,
				msecs_to_jiffies(10)); // 10ms
		frsm_mntr_irq_switch(frsm_dev, true);
	} else {
		frsm_mntr_irq_switch(frsm_dev, false);
		cancel_delayed_work_sync(&frsm_dev->delay_work);
	}

	return 0;
}

static void frsm_work_monitor(struct work_struct *work)
{
	struct frsm_dev *frsm_dev;
	int ret;

	if (work == NULL)
		return;

	frsm_dev = container_of(work, struct frsm_dev, delay_work.work);
	ret = frsm_send_event(frsm_dev, EVENT_STAT_MNTR);
	if (ret)
		goto func_exit;

	if (frsm_dev->irq_id > 0) /* use irq instead of delay work */
		goto func_exit;

	if (frsm_dev->pdata == NULL || !frsm_dev->pdata->mntr_enable)
		goto func_exit;

	queue_delayed_work(frsm_dev->thread_wq,
			&frsm_dev->delay_work,
			msecs_to_jiffies(frsm_dev->pdata->mntr_period));
	return;

func_exit:
	dev_dbg(frsm_dev->dev, "Pending monitor\n");
}

static void frsm_work_interrupt(struct work_struct *work)
{
	struct frsm_dev *frsm_dev;
	int ret;

	if (work == NULL)
		return;

	frsm_dev = container_of(work, struct frsm_dev, irq_work.work);
	ret = frsm_send_event(frsm_dev, EVENT_STAT_MNTR);
	if (ret)
		dev_err(frsm_dev->dev, "Failed to handle irq:%d\n", ret);
}

static irqreturn_t frsm_mntr_irq_handler(int irq, void *data)
{
	struct frsm_dev *frsm_dev = data;

	if (frsm_dev == NULL)
		return IRQ_HANDLED;

	dev_info(frsm_dev->dev, "irq handling...\n");
	queue_delayed_work(frsm_dev->thread_wq, &frsm_dev->irq_work, 0);

	return IRQ_HANDLED;
}

static int frsm_mntr_irq_init(struct frsm_dev *frsm_dev)
{
	struct frsm_platform_data *pdata;
	struct frsm_gpio *intz;
	unsigned long irq_flags;
	char *name, str[FRSM_STRING_MAX];
	int ret;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	pdata = frsm_dev->pdata;
	intz = pdata->gpio + FRSM_PIN_INTZ;
	if (IS_ERR_OR_NULL(intz->gpiod) || pdata->mntr_enable) {
		dev_info(frsm_dev->dev, "Skip to request IRQ\n");
		frsm_dev->irq_id = -1;
		return 0;
	}

	frsm_dev->irq_id = gpiod_to_irq(intz->gpiod);
	if (frsm_dev->irq_id < 0) {
		dev_err(frsm_dev->dev, "Invalid irq id:%d\n",
				frsm_dev->irq_id);
		return frsm_dev->irq_id;
	}

	INIT_DELAYED_WORK(&frsm_dev->irq_work, frsm_work_interrupt);
	if (pdata->irq_polarity)
		irq_flags = IRQF_TRIGGER_RISING | IRQF_ONESHOT;
	else
		irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;

	snprintf(str, sizeof(str), "frsm-irq.%d", pdata->spkr_id);
	name = devm_kstrdup(frsm_dev->dev, str, GFP_KERNEL);
	ret = devm_request_threaded_irq(frsm_dev->dev,
			frsm_dev->irq_id, NULL, frsm_mntr_irq_handler,
			irq_flags, name, frsm_dev);
	if (ret) {
		dev_err(frsm_dev->dev, "Failed to request IRQ-%d: %d\n",
				frsm_dev->irq_id, ret);
		return ret;
	}

	disable_irq(frsm_dev->irq_id);

	return 0;
}
static int frsm_mntr_parse_dts(struct frsm_dev *frsm_dev)
{
	struct frsm_platform_data *pdata;
	struct device_node *np;
	int ret;

	np = frsm_dev->dev->of_node;
	pdata = frsm_dev->pdata;

	/* Monitor switch */
	pdata->mntr_enable = of_property_read_bool(np, "mntr-enable");

	/* interrupt active level */
	ret = of_property_read_s32(np, "irq-polarity", &pdata->irq_polarity);
	if (ret)
		pdata->irq_polarity = 0; /* default low */

	/* Delay work period: ms */
	ret = of_property_read_u32(np, "mntr-period", &pdata->mntr_period);
	if (ret)
		pdata->mntr_period = 2000; /* 2s */

	dev_info(frsm_dev->dev,
			"mntr_enable:%d period:%d(ms)\n",
			pdata->mntr_enable, pdata->mntr_period);

	return 0;
}

static int frsm_mntr_init(struct frsm_dev *frsm_dev)
{
	int ret;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	ret = frsm_mntr_parse_dts(frsm_dev);
	if (ret)
		return ret;

	frsm_dev->thread_wq = create_singlethread_workqueue(
			dev_name(frsm_dev->dev));
	INIT_DELAYED_WORK(&frsm_dev->delay_work, frsm_work_monitor);

	ret = frsm_mntr_irq_init(frsm_dev);
	if (ret)
		dev_err(frsm_dev->dev, "Failed to request irq:%d\n", ret);

	return ret;
}

static void frsm_mntr_deinit(struct frsm_dev *frsm_dev)
{
	frsm_stub_mntr_switch(frsm_dev, false);
	if (frsm_dev->irq_id > 0)
		devm_free_irq(frsm_dev->dev, frsm_dev->irq_id, frsm_dev);
}

#endif // __FRSM_MONITOR_H__
