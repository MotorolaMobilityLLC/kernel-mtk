/*
 * Copyright (C) 2019 Richtek Technology Corp.
 *
 * Richtek RT1711 Type-C Port Control Driver
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/pm_runtime.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/cpu.h>
#include <linux/version.h>
#include <linux/pm_wakeup.h>

#include "inc/pd_dbg_info.h"
#include "inc/tcpci.h"
#include "inc/rt1711.h"

#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif /* CONFIG_RT_REGMAP */

#if 1 /*  #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))*/
#include <linux/sched/rt.h>
#endif /* #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)) */

#define RT1711_DRV_VERSION	"1.0.6_MTK"
#define RT1711_IRQ_WAKE_TIME	(500) /* ms */

struct rt1711_chip {
	struct i2c_client *client;
	struct device *dev;
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *m_dev;
#endif /* CONFIG_RT_REGMAP */
	struct semaphore io_lock;
	struct semaphore suspend_lock;
	struct tcpc_desc *tcpc_desc;
	struct tcpc_device *tcpc;
	struct kthread_worker irq_worker;
	struct kthread_work irq_work;
	struct task_struct *irq_worker_task;

	int irq_gpio;
	int irq;
};

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT1711_REG_VENDOR_ID, 2, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_ALERT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_ALERT_MASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_POWER_STATUS_MASK, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_CCSTATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_POWER_STATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_ROLECTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_POWERCTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_COMMAND, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_MSGHEADERINFO, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_RCVDETECT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_RCVBYTECNT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_RXBUFFRAMETYPE, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_RXBUFHEADER, 2, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_RXDATA, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_TRANSMITHEADERLOWCMD, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_TRANSMIT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_TRANSMITBYTECNT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_TXHDR, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_TXDATA, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_PHYCTRL2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_PHYCTRL3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_PHYCTRL6, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_RXTXDBG, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_LOW_POWER_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_BMCIO_RXDZSEL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_CC_EXT_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_SWRESET, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_TTCPCFILTER, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_DRPTOGGLECYCLE, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_DRPDUTYCTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_UNLOCKPW2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_UNLOCKPW1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_RDCAL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_OSC_FREQ, 1, RT_VOLATILE, {});
RT_REG_DECL(RT1711_REG_TESTMODE, 1, RT_VOLATILE, {});

static const rt_register_map_t rt1711_chip_regmap[] = {
	RT_REG(RT1711_REG_VENDOR_ID),
	RT_REG(RT1711_REG_ALERT),
	RT_REG(RT1711_REG_ALERT_MASK),
	RT_REG(RT1711_REG_POWER_STATUS_MASK),
	RT_REG(RT1711_REG_CCSTATUS),
	RT_REG(RT1711_REG_POWER_STATUS),
	RT_REG(RT1711_REG_ROLECTRL),
	RT_REG(RT1711_REG_POWERCTRL),
	RT_REG(RT1711_REG_COMMAND),
	RT_REG(RT1711_REG_MSGHEADERINFO),
	RT_REG(RT1711_REG_RCVDETECT),
	RT_REG(RT1711_REG_RCVBYTECNT),
	RT_REG(RT1711_REG_RXBUFFRAMETYPE),
	RT_REG(RT1711_REG_RXBUFHEADER),
	RT_REG(RT1711_REG_RXDATA),
	RT_REG(RT1711_REG_TRANSMITHEADERLOWCMD),
	RT_REG(RT1711_REG_TRANSMIT),
	RT_REG(RT1711_REG_TRANSMITBYTECNT),
	RT_REG(RT1711_REG_TXHDR),
	RT_REG(RT1711_REG_TXDATA),
	RT_REG(RT1711_REG_PHYCTRL2),
	RT_REG(RT1711_REG_PHYCTRL3),
	RT_REG(RT1711_REG_PHYCTRL6),
	RT_REG(RT1711_REG_RXTXDBG),
	RT_REG(RT1711_REG_LOW_POWER_CTRL),
	RT_REG(RT1711_REG_BMCIO_RXDZSEL),
	RT_REG(RT1711_REG_CC_EXT_CTRL),
	RT_REG(RT1711_REG_SWRESET),
	RT_REG(RT1711_REG_TTCPCFILTER),
	RT_REG(RT1711_REG_DRPTOGGLECYCLE),
	RT_REG(RT1711_REG_DRPDUTYCTRL),
	RT_REG(RT1711_REG_UNLOCKPW2),
	RT_REG(RT1711_REG_UNLOCKPW1),
	RT_REG(RT1711_REG_RDCAL),
	RT_REG(RT1711_REG_OSC_FREQ),
	RT_REG(RT1711_REG_TESTMODE),
};
#define RT1711_CHIP_REGMAP_SIZE ARRAY_SIZE(rt1711_chip_regmap)

#endif /* CONFIG_RT_REGMAP */

static int rt1711_read_device(void *client, u32 reg, int len, void *dst)
{
	struct i2c_client *i2c = (struct i2c_client *)client;
	int ret = 0, count = 5;

	while (count) {
		ret = i2c_smbus_read_i2c_block_data(i2c, reg, len, dst);
		if (ret < 0)
			count--;
		else
			return ret;
		udelay(100);
	}
	return ret;
}

static int rt1711_write_device(void *client, u32 reg, int len, const void *src)
{
	struct i2c_client *i2c = (struct i2c_client *)client;
	int ret = 0, count = 5;

	while (count) {
		ret = i2c_smbus_write_i2c_block_data(i2c, reg, len, src);
		if (ret < 0)
			count--;
		else
			return ret;
		udelay(100);
	}
	return ret;
}

static int rt1711_reg_read(struct i2c_client *i2c, u8 reg)
{
	struct rt1711_chip *chip = i2c_get_clientdata(i2c);
	u8 val = 0;
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_read(chip->m_dev, reg, 1, &val);
#else
	ret = rt1711_read_device(i2c, reg, 1, &val);
#endif /* CONFIG_RT_REGMAP */
	if (ret < 0) {
		dev_notice(chip->dev, "%s: reg read fail(%d)\n", __func__, ret);
		return ret;
	}
	return val;
}

static int rt1711_reg_write(struct i2c_client *i2c, u8 reg, const u8 data)
{
	struct rt1711_chip *chip = i2c_get_clientdata(i2c);
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_write(chip->m_dev, reg, 1, &data);
#else
	ret = rt1711_write_device(i2c, reg, 1, &data);
#endif /* CONFIG_RT_REGMAP */
	if (ret < 0)
		dev_notice(chip->dev,
				"%s: reg write fail(%d)\n", __func__, ret);
	return ret;
}

static int rt1711_block_write(struct i2c_client *i2c,
			u8 reg, int len, const void *src)
{
	struct rt1711_chip *chip = i2c_get_clientdata(i2c);
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_write(chip->m_dev, reg, len, src);
#else
	ret = rt1711_write_device(i2c, reg, len, src);
#endif /* #ifdef CONFIG_RT_REGMAP */
	if (ret < 0)
		dev_notice(chip->dev,
				"%s: block write fail(%d)\n", __func__, ret);
	return ret;
}

static int rt1711_update_bits(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct rt1711_chip *chip = i2c_get_clientdata(i2c);
	int ret = 0;
	u8 data;

	down(&chip->io_lock);
	ret = rt1711_reg_read(i2c, reg);
	if (ret < 0) {
		up(&chip->io_lock);
		return ret;
	}
	data = ret;

	data &= ~mask;
	data |= (val & mask);
	ret = rt1711_reg_write(i2c, reg, data);
	up(&chip->io_lock);
	return 0;
}

static inline int rt1711_set_bit(struct i2c_client *i2c, u8 reg, u8 mask)
{
	return rt1711_update_bits(i2c, reg, mask, mask);
}

static inline int rt1711_clr_bit(struct i2c_client *i2c, u8 reg, u8 mask)
{
	return rt1711_update_bits(i2c, reg, 0x00, mask);
}

static inline int32_t rt1711_write_word(struct i2c_client *client,
					uint8_t reg_addr, uint16_t data)
{
	data = cpu_to_le16(data);
	return rt1711_block_write(client, reg_addr, 2, (uint8_t *)&data);
}

static inline int rt1711_i2c_write8(
	struct tcpc_device *tcpc, u8 reg, const u8 data)
{
	struct rt1711_chip *chip = tcpc_get_dev_data(tcpc);
	int ret;

	down(&chip->io_lock);
	ret = rt1711_reg_write(chip->client, reg, data);
	up(&chip->io_lock);
	return ret;
}

static inline int rt1711_i2c_write16(
		struct tcpc_device *tcpc, u8 reg, const u16 data)
{
	struct rt1711_chip *chip = tcpc_get_dev_data(tcpc);
	int ret;

	down(&chip->io_lock);
	ret = rt1711_write_word(chip->client, reg, data);
	up(&chip->io_lock);
	return ret;
}

static inline int rt1711_i2c_read8(struct tcpc_device *tcpc, u8 reg)
{
	struct rt1711_chip *chip = tcpc_get_dev_data(tcpc);
	int ret;

	down(&chip->io_lock);
	ret = rt1711_reg_read(chip->client, reg);
	up(&chip->io_lock);
	return ret;
}

#ifdef CONFIG_RT_REGMAP
static struct rt_regmap_fops rt1711_regmap_fops = {
	.read_device = rt1711_read_device,
	.write_device = rt1711_write_device,
};
#endif /* CONFIG_RT_REGMAP */

static int rt1711_regmap_init(struct rt1711_chip *chip)
{
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_properties *props;
	char name[32];
	int len;

	props = devm_kzalloc(chip->dev, sizeof(*props), GFP_KERNEL);
	if (!props)
		return -ENOMEM;

	props->register_num = RT1711_CHIP_REGMAP_SIZE;
	props->rm = rt1711_chip_regmap;

	props->rt_regmap_mode = RT_MULTI_BYTE | RT_CACHE_DISABLE |
				RT_IO_PASS_THROUGH | RT_DBG_SPECIAL;
	snprintf(name, sizeof(name), "rt1711-%02x", chip->client->addr);

	len = strlen(name);
	props->name = devm_kzalloc(chip->dev, len + 1, GFP_KERNEL);
	props->aliases = devm_kzalloc(chip->dev, len + 1, GFP_KERNEL);

	if ((!props->name) || (!props->aliases))
		return -ENOMEM;

	strlcpy((char *)props->name, name, len + 1);
	strlcpy((char *)props->aliases, name, len + 1);
	props->io_log_en = 0;

	chip->m_dev = rt_regmap_device_register(props,
			&rt1711_regmap_fops, chip->dev, chip->client, chip);
	if (!chip->m_dev) {
		dev_notice(chip->dev, "%s: regmap register fail\n", __func__);
		return -EINVAL;
	}
#endif
	return 0;
}

static int rt1711_regmap_deinit(struct rt1711_chip *chip)
{
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(chip->m_dev);
#endif
	return 0;
}

static inline int rt1711_software_reset(struct tcpc_device *tcpc)
{
	int ret = rt1711_i2c_write8(tcpc, RT1711_REG_SWRESET, 1);

	if (ret < 0)
		return ret;

	udelay(1000);
	return ret;
}

static int rt1711_init_testmode(struct tcpc_device *tcpc)
{
	int ret = 0;

#if 0	/* Enable it unless you want to write locked register */
	ret = rt1711_reg_write(chip->client, RT1711_REG_UNLOCKPW1, 0x62);
	ret |= rt1711_reg_write(chip->client, RT1711_REG_UNLOCKPW2, 0x86);
#endif

	/* For MQP CC-Noise2 Case */
	ret |= rt1711_i2c_write8(tcpc, RT1711_REG_PHYCTRL2, 0x3E);
	ret |= rt1711_i2c_write8(tcpc, RT1711_REG_BMCIO_RXDZSEL, 0x81);

	return ret;
}

static int rt1711_init_alert_mask(struct tcpc_device *tcpc)
{
	uint16_t mask;

	mask = RT1711_REG_ALERT_CC_STATUS | RT1711_REG_ALERT_POWER_STATUS;

	return rt1711_i2c_write16(tcpc, RT1711_REG_ALERT_MASK, mask);
}

static int rt1711_init_power_status_mask(struct tcpc_device *tcpc)
{
	uint8_t mask = RT1711_VBUS_PRES_MASK;

#ifdef CONFIG_TCPC_VSAFE0V_DETECT_IC
	mask |= RT1711_VBUS_80_MASK;
#endif /* CONFIG_TCPC_VSAFE0V_DETECT_IC */

	return rt1711_i2c_write8(tcpc, RT1711_REG_POWER_STATUS_MASK, mask);
}

static void rt1711_irq_work_handler(struct kthread_work *work)
{
	struct rt1711_chip *chip =
			container_of(work, struct rt1711_chip, irq_work);
	int regval = 0;
	int gpio_val;

	/* make sure I2C bus had resumed */
	down(&chip->suspend_lock);
	tcpci_lock_typec(chip->tcpc);

	do {
		regval = tcpci_alert(chip->tcpc);
		if (regval)
			break;
		gpio_val = gpio_get_value(chip->irq_gpio);
	} while (gpio_val == 0);

	tcpci_unlock_typec(chip->tcpc);
	up(&chip->suspend_lock);

#ifdef DEBUG_GPIO
	gpio_set_value(DEBUG_GPIO, 1);
#endif
}

static irqreturn_t rt1711_intr_handler(int irq, void *data)
{
	struct rt1711_chip *chip = data;

	pm_wakeup_event(chip->dev, RT1711_IRQ_WAKE_TIME);
	queue_kthread_work(&chip->irq_worker, &chip->irq_work);
	return IRQ_HANDLED;
}

static int rt1711_init_alert(struct rt1711_chip *chip)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	int ret;
	char *name;
	int len;

	len = strlen(chip->tcpc_desc->name);
	name = devm_kzalloc(chip->dev, len + 5, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	snprintf(name, len + 5, "%s-IRQ", chip->tcpc_desc->name);

	dev_info(chip->dev, "%s: name = %s, gpio = %d\n", __func__,
				chip->tcpc_desc->name, chip->irq_gpio);

	ret = devm_gpio_request(chip->dev, chip->irq_gpio, name);
	if (ret < 0) {
		dev_notice(chip->dev, "%s: gpio request fail(%d)\n",
				__func__, ret);
		goto init_alert_err;
	}

	ret = gpio_direction_input(chip->irq_gpio);
	if (ret < 0) {
		dev_notice(chip->dev, "%s: gpio direction input fail(%d)\n",
				__func__, ret);
		goto init_alert_err;
	}

	chip->irq = gpio_to_irq(chip->irq_gpio);
	if (chip->irq <= 0) {
		dev_notice(chip->dev, "%s: gpio to irq fail(%d)\n",
				__func__, chip->irq);
		goto init_alert_err;
	}

	dev_info(chip->dev, "%s: IRQ number = %d\n", __func__, chip->irq);

	init_kthread_worker(&chip->irq_worker);
	chip->irq_worker_task = kthread_run(kthread_worker_fn,
			&chip->irq_worker, chip->tcpc_desc->name);
	if (IS_ERR(chip->irq_worker_task)) {
		dev_notice(chip->dev, "%s: create tcpc task fail\n", __func__);
		goto init_alert_err;
	}

	sched_setscheduler(chip->irq_worker_task, SCHED_FIFO, &param);
	init_kthread_work(&chip->irq_work, rt1711_irq_work_handler);

	ret = request_irq(chip->irq, rt1711_intr_handler,
		IRQF_TRIGGER_FALLING | IRQF_NO_THREAD |
		IRQF_NO_SUSPEND, name, chip);
	if (ret < 0) {
		dev_notice(chip->dev, "%s: request irq%d fail(%d)\n",
				__func__, chip->irq, ret);
		goto request_irq_err;
	}

	device_init_wakeup(chip->dev, true);
	return 0;
request_irq_err:
	kthread_stop(chip->irq_worker_task);
init_alert_err:
	return -EINVAL;
}

static int rt1711_alert_status_clear(struct tcpc_device *tcpc, uint32_t mask)
{
	uint8_t mask_t = (uint8_t) mask;

	/* Write 0 to clear */
	return rt1711_i2c_write8(tcpc, RT1711_REG_ALERT, ~mask_t);
}

static int rt1711_tcpc_init(struct tcpc_device *tcpc, bool sw_reset)
{
	int ret;
	int power_status;

	RT1711_INFO("\n");

	if (sw_reset) {
		ret = rt1711_software_reset(tcpc);
		if (ret < 0)
			return ret;
	}

	power_status = rt1711_i2c_read8(tcpc, RT1711_REG_POWER_STATUS);
	/* if EC Success, power_status should be 0 */
	if (power_status < 0)
		return power_status;

	ret = rt1711_init_testmode(tcpc);
	if (ret < 0)
		return ret;

	/* UFP Both RD setting */
	/* DRP = 0, RpVal = 0 (Default), Rd, Rd */
	rt1711_i2c_write8(tcpc, RT1711_REG_ROLECTRL,
		RT1711_REG_ROLE_CTRL_RES_SET(0, 0, TYPEC_CC_RD, TYPEC_CC_RD));

	/*
	 * CC Detect Debounce : 26.7*val us
	 * Transition window count : spec 12~20us, baseon 2.4MHz
	 * DRP Toggle Cycle : 50 + 10*val ms
	 * DRP Duty Ctrl : dcSRC: 30%
	 */

	rt1711_i2c_write8(tcpc, RT1711_REG_TTCPCFILTER, 0x05);
	/* For No-GoodRC Case */
	rt1711_i2c_write8(tcpc, RT1711_REG_PHYCTRL3, 0x70);
	rt1711_i2c_write8(tcpc, RT1711_REG_DRPTOGGLECYCLE, 0x02);
	rt1711_i2c_write8(tcpc, RT1711_REG_DRPDUTYCTRL, 10);

	/* Clear Rx Count */
	rt1711_i2c_write8(tcpc, RT1711_REG_RCVBYTECNT, 0);

	/* Clear Alert Mask & Status */
	rt1711_i2c_write16(tcpc, RT1711_REG_ALERT_MASK, 0);
	rt1711_i2c_write16(tcpc, RT1711_REG_ALERT, 0);

	rt1711_init_power_status_mask(tcpc);
	rt1711_init_alert_mask(tcpc);

	return 0;
}

static int rt1711_fault_status_clear(struct tcpc_device *tcpc, uint8_t status)
{
	return 0;
}

int rt1711_get_alert_mask(struct tcpc_device *tcpc, uint32_t *mask)
{
	int ret;

	ret = rt1711_i2c_read8(tcpc, RT1711_REG_ALERT_MASK);
	if (ret < 0)
		return ret;

	*mask = ret;
	return 0;
}

static int rt1711_get_alert_status(struct tcpc_device *tcpc, uint32_t *alert)

{
	int ret;

	ret = rt1711_i2c_read8(tcpc, RT1711_REG_ALERT);
	if (ret < 0)
		return ret;

	*alert = ret;
	return 0;
}

static int rt1711_get_power_status(
		struct tcpc_device *tcpc, uint16_t *pwr_status)
{
	int ret;

	ret = rt1711_i2c_read8(tcpc, RT1711_REG_POWER_STATUS);
	if (ret < 0)
		return ret;

	*pwr_status = 0;
	if (ret & RT1711_VBUS_PRES_MASK)
		*pwr_status |= TCPC_REG_POWER_STATUS_VBUS_PRES;

#ifdef CONFIG_TCPC_VSAFE0V_DETECT_IC
	if (ret & RT1711_VBUS_80_MASK)
		*pwr_status |= TCPC_REG_POWER_STATUS_EXT_VSAFE0V;
#endif
	return 0;
}

static int rt1711_get_fault_status(struct tcpc_device *tcpc, uint8_t *status)
{
	*status = 0;
	return 0;
}

static int rt1711_get_cc(struct tcpc_device *tcpc, int *cc1, int *cc2)
{
	int status, role_ctrl, cc_role;
	bool act_as_sink;

	status = rt1711_i2c_read8(tcpc, RT1711_REG_CCSTATUS);
	if (status < 0)
		return status;

	role_ctrl = rt1711_i2c_read8(tcpc, RT1711_REG_ROLECTRL);
	if (role_ctrl < 0)
		return role_ctrl;

	if (status & RT1711_DRP_TOGGLING_MASK) {
		*cc1 = TYPEC_CC_DRP_TOGGLING;
		*cc2 = TYPEC_CC_DRP_TOGGLING;
		return 0;
	}

	*cc1 = RT1711_REG_CC_STATUS_CC1(status);
	*cc2 = RT1711_REG_CC_STATUS_CC2(status);

	cc_role = RT1711_REG_CC_STATUS_CC1(role_ctrl);

	switch (cc_role) {
	case TYPEC_CC_RP:
		act_as_sink = false;
		break;
	case TYPEC_CC_RD:
		act_as_sink = true;
		break;
	default:
		act_as_sink = RT1711_REG_CC_STATUS_DRP_RESULT(status);
		break;
	}

	/*
	 * If status is not open, then OR in termination to convert to
	 * enum tcpc_cc_voltage_status.
	 */

	if (*cc1 != TYPEC_CC_VOLT_OPEN)
		*cc1 |= (act_as_sink << 2);

	if (*cc2 != TYPEC_CC_VOLT_OPEN)
		*cc2 |= (act_as_sink << 2);

	return 0;
}

static int rt1711_enable_vsafe0v_detect(
	struct tcpc_device *tcpc, bool enable)
{
	int ret = rt1711_i2c_read8(tcpc, RT1711_REG_POWER_STATUS_MASK);
	if (ret < 0)
		return ret;

	if (enable)
		ret |= RT1711_VBUS_80_MASK;
	else
		ret &= ~RT1711_VBUS_80_MASK;

	return rt1711_i2c_write8(tcpc,
			RT1711_REG_POWER_STATUS_MASK, (uint8_t) ret);
}

static int rt1711_set_cc(struct tcpc_device *tcpc, int pull)
{
	struct rt1711_chip *chip = tcpc_get_dev_data(tcpc);
	int ret;
	uint8_t data;
	int rp_lvl = TYPEC_CC_PULL_GET_RP_LVL(pull);

	RT1711_INFO("\n");
	pull = TYPEC_CC_PULL_GET_RES(pull);
	if (pull == TYPEC_CC_DRP) {
		/* Workaround: toggle reg0x1A[6] to re-enable DRP */
		data = RT1711_REG_ROLE_CTRL_RES_SET(
				0, rp_lvl, TYPEC_CC_OPEN, TYPEC_CC_OPEN);

		ret = rt1711_i2c_write8(tcpc, RT1711_REG_ROLECTRL, data);
		if (ret < 0)
			dev_notice(chip->dev, "%s: i2c write8 fail(%d)\n",
					__func__, ret);
		else
			rt1711_enable_vsafe0v_detect(tcpc, false);

		data = RT1711_REG_ROLE_CTRL_RES_SET(
				1, rp_lvl, TYPEC_CC_OPEN, TYPEC_CC_OPEN);

		ret = rt1711_i2c_write8(tcpc, RT1711_REG_ROLECTRL, data);
		if (ret < 0)
			dev_notice(chip->dev, "%s: i2c write8 fail(%d)\n",
					__func__, ret);
	} else {
		data = RT1711_REG_ROLE_CTRL_RES_SET(0, rp_lvl, pull, pull);
		ret = rt1711_i2c_write8(tcpc, RT1711_REG_ROLECTRL, data);
	}

	return ret;
}

static int rt1711_set_polarity(struct tcpc_device *tcpc, int polarity)
{
	int data;

	data = rt1711_i2c_read8(tcpc, RT1711_REG_POWERCTRL);
	if (data < 0)
		return data;

	data &= ~RT1711_PLUG_ORIENT_MASK;
	data |= polarity ? RT1711_PLUG_ORIENT_MASK : 0;

	return rt1711_i2c_write8(tcpc, RT1711_REG_POWERCTRL, data);
}

static int rt1711_set_low_rp_duty(struct tcpc_device *tcpc, bool low_rp)
{
	/* low: 10%, normal: 30% */
	uint8_t sel = low_rp ? 8 : 10;

	return rt1711_i2c_write8(tcpc, RT1711_REG_DRPDUTYCTRL, sel);
}

static int rt1711_set_vconn(struct tcpc_device *tcpc, int enable)
{
	int data;

	data = rt1711_i2c_read8(tcpc, RT1711_REG_POWERCTRL);
	if (data < 0)
		return data;

	data &= ~RT1711_VCONN_MASK;
	data |= enable ? RT1711_VCONN_MASK : 0;

	return rt1711_i2c_write8(tcpc, RT1711_REG_POWERCTRL, data);
}

#ifdef CONFIG_TCPC_LOW_POWER_MODE
static int rt1711_is_low_power_mode(struct tcpc_device *tcpc)
{
	int rv = rt1711_i2c_read8(tcpc, RT1711_REG_LOW_POWER_CTRL);

	if (rv < 0)
		return rv;

	return (rv & RT1711_BMCIO_LPEN) != 0;
}

static int rt1711_set_low_power_mode(
		struct tcpc_device *tcpc, bool en, int pull)
{
	int rv = 0;
	uint8_t data;

	if (en) {
		data = RT1711_BMCIO_LPEN;

		if (pull & TYPEC_CC_RP)
			data |= RT1711_BMCIO_LPRPRD;

#ifdef CONFIG_TYPEC_CAP_NORP_SRC
		data |= RT1711_BMCIO_BG_EN | RT1711_REG_VBUS_DETEN;
#endif
	} else {
		data = RT1711_BMCIO_BG_EN |
			RT1711_REG_VBUS_DETEN | RT1711_REG_BMCIO_OSC_EN;
		rt1711_enable_vsafe0v_detect(tcpc, true);
	}

	rv = rt1711_i2c_write8(tcpc, RT1711_REG_LOW_POWER_CTRL, data);
	return rv;
}
#endif	/* CONFIG_TCPC_LOW_POWER_MODE */

static int rt1711_tcpc_deinit(struct tcpc_device *tcpc)
{
	return rt1711_software_reset(tcpc);
}

#ifdef CONFIG_USB_POWER_DELIVERY
static int rt1711_set_msg_header(
	struct tcpc_device *tcpc, uint8_t power_role, uint8_t data_role)
{
	return 0;
}

static int rt1711_protocol_reset(struct tcpc_device *tcpc)
{
	return 0;
}

static int rt1711_set_rx_enable(struct tcpc_device *tcpc, uint8_t enable)
{
	return 0;
}

static int rt1711_get_message(struct tcpc_device *tcpc, uint32_t *payload,
			uint16_t *msg_head, enum tcpm_transmit_type *frame_type)
{
	return 0;
}

static int rt1711_set_bist_carrier_mode(
	struct tcpc_device *tcpc, uint8_t pattern)
{
	return 0;
}

#ifdef CONFIG_USB_PD_RETRY_CRC_DISCARD
static int rt1711_retransmit(struct tcpc_device *tcpc)
{
	return 0;
}
#endif

static int rt1711_transmit(struct tcpc_device *tcpc,
	enum tcpm_transmit_type type, uint16_t header, const uint32_t *data)
{
	return 0;
}

static int rt1711_set_bist_test_mode(struct tcpc_device *tcpc, bool en)
{
	return 0;
}
#endif /* CONFIG_USB_POWER_DELIVERY */

static struct tcpc_ops rt1711_tcpc_ops = {
	.init = rt1711_tcpc_init,
	.alert_status_clear = rt1711_alert_status_clear,
	.fault_status_clear = rt1711_fault_status_clear,
	.get_alert_mask = rt1711_get_alert_mask,
	.get_alert_status = rt1711_get_alert_status,
	.get_power_status = rt1711_get_power_status,
	.get_fault_status = rt1711_get_fault_status,
	.get_cc = rt1711_get_cc,
	.set_cc = rt1711_set_cc,
	.set_polarity = rt1711_set_polarity,
	.set_low_rp_duty = rt1711_set_low_rp_duty,
	.set_vconn = rt1711_set_vconn,
	.deinit = rt1711_tcpc_deinit,

#ifdef CONFIG_TCPC_LOW_POWER_MODE
	.is_low_power_mode = rt1711_is_low_power_mode,
	.set_low_power_mode = rt1711_set_low_power_mode,
#endif	/* CONFIG_TCPC_LOW_POWER_MODE */

#ifdef CONFIG_USB_POWER_DELIVERY
	.set_msg_header = rt1711_set_msg_header,
	.set_rx_enable = rt1711_set_rx_enable,
	.protocol_reset = rt1711_protocol_reset,
	.get_message = rt1711_get_message,
	.transmit = rt1711_transmit,
	.set_bist_test_mode = rt1711_set_bist_test_mode,
	.set_bist_carrier_mode = rt1711_set_bist_carrier_mode,
#ifdef CONFIG_USB_PD_RETRY_CRC_DISCARD
	.retransmit = rt1711_retransmit,
#endif	/* CONFIG_USB_PD_RETRY_CRC_DISCARD */
#endif	/* CONFIG_USB_POWER_DELIVERY */
};

static int rt_parse_dt(struct rt1711_chip *chip)
{
	struct device_node *np;
	int ret;

	pr_info("%s\n", __func__);

	np = of_find_node_by_name(NULL, "type_c_port0");
	if (!np) {
		pr_notice("%s: find node type_c_port0 fail\n", __func__);
		return -ENODEV;
	}

#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
	ret = of_get_named_gpio(np, "rt1711,intr_gpio", 0);
	if (ret < 0) {
		pr_notice("%s: find rt1711,intr_gpio fail(%d)\n",
				__func__, ret);
		return ret;
	}
	chip->irq_gpio = ret;
#else
	ret = of_property_read_u32(np,
		"rt1711,intr_gpio_num", &chip->irq_gpio);
	if (ret < 0)
		pr_notice("%s: find rt1711,intr_gpio_num fail(%d)\n",
				__func__, ret);
#endif
	return ret;
}

/*
 * In some platform pr_info may spend too much time on printing debug message.
 * So we use this function to test the printk performance.
 * If your platform cannot not pass this check function, please config
 * PD_DBG_INFO, this will provide the threaded debug message for you.
 */
#if TCPC_ENABLE_ANYMSG
static void check_printk_performance(void)
{
	int i;
	u64 t1, t2;
	u32 nsrem;

#ifdef CONFIG_PD_DBG_INFO
	for (i = 0; i < 10; i++) {
		t1 = local_clock();
		pd_dbg_info("%d\n", i);
		t2 = local_clock();
		t2 -= t1;
		nsrem = do_div(t2, 1000000000);
		pd_dbg_info("pd_dbg_info : t2-t1 = %lu\n",
				(unsigned long)nsrem / 1000);
	}
	for (i = 0; i < 10; i++) {
		t1 = local_clock();
		pr_info("%d\n", i);
		t2 = local_clock();
		t2 -= t1;
		nsrem = do_div(t2, 1000000000);
		pr_info("pr_info : t2-t1 = %lu\n",
				(unsigned long)nsrem / 1000);
	}
#else
	for (i = 0; i < 10; i++) {
		t1 = local_clock();
		pr_info("%d\n", i);
		t2 = local_clock();
		t2 -= t1;
		nsrem = do_div(t2, 1000000000);
		pr_info("t2-t1 = %lu\n",
				(unsigned long)nsrem /  1000);
		PD_BUG_ON(nsrem > 100*1000);
	}
#endif /* CONFIG_PD_DBG_INFO */
}
#endif /* TCPC_ENABLE_ANYMSG */

static int rt1711_tcpcdev_init(struct rt1711_chip *chip)
{
	struct tcpc_desc *desc;
	struct device_node *np;
	u32 val, len;
	const char *name = "default";

	np = of_find_node_by_name(NULL, "type_c_port0");
	if (!np) {
		dev_notice(chip->dev,
				"%s: find node type_c_port0 fail\n", __func__);
		return -ENODEV;
	}

	desc = devm_kzalloc(chip->dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	if (of_property_read_u32(np, "rt-tcpc,role_def", &val) >= 0) {
		if (val >= TYPEC_ROLE_NR)
			desc->role_def = TYPEC_ROLE_DRP;
		else
			desc->role_def = val;
	} else {
		dev_info(chip->dev, "%s: use default Role DRP\n", __func__);
		desc->role_def = TYPEC_ROLE_DRP;
	}

	if (of_property_read_u32(
		np, "rt-tcpc,notifier_supply_num", &val) >= 0) {
		if (val < 0)
			desc->notifier_supply_num = 0;
		else
			desc->notifier_supply_num = val;
	} else
		desc->notifier_supply_num = 0;

	if (of_property_read_u32(np, "rt-tcpc,rp_level", &val) >= 0) {
		switch (val) {
		case 0: /* RP Default */
			desc->rp_lvl = TYPEC_CC_RP_DFT;
			break;
		case 1: /* RP 1.5V */
			desc->rp_lvl = TYPEC_CC_RP_1_5;
			break;
		case 2: /* RP 3.0V */
			desc->rp_lvl = TYPEC_CC_RP_3_0;
			break;
		default:
			break;
		}
	}

#ifdef CONFIG_TCPC_VCONN_SUPPLY_MODE
	if (of_property_read_u32(np, "rt-tcpc,vconn_supply", &val) >= 0) {
		if (val >= TCPC_VCONN_SUPPLY_NR)
			desc->vconn_supply = TCPC_VCONN_SUPPLY_ALWAYS;
		else
			desc->vconn_supply = val;
	} else {
		dev_info(chip->dev, "%s: use default VconnSupply\n", __func__);
		desc->vconn_supply = TCPC_VCONN_SUPPLY_ALWAYS;
	}
#endif	/* CONFIG_TCPC_VCONN_SUPPLY_MODE */

	of_property_read_string(np, "rt-tcpc,name", (char const **)&name);

	len = strlen(name);
	desc->name = devm_kzalloc(chip->dev, len + 1, GFP_KERNEL);
	if (!desc->name)
		return -ENOMEM;

	strlcpy((char *)desc->name, name, len + 1);

	chip->tcpc_desc = desc;

	chip->tcpc = tcpc_device_register(chip->dev,
			desc, &rt1711_tcpc_ops, chip);
	if (IS_ERR(chip->tcpc))
		return -EINVAL;

	/* for RT1711H TCPC 0.6 */
#if 0
	chip->tcpc->tcpc_flags = TCPC_FLAGS_CHECK_CC_STABLE;
#else
	chip->tcpc->tcpc_flags = 0;
#endif

	return 0;
}

static int rt1711_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct rt1711_chip *chip;
	int ret = 0;
	bool use_dt = client->dev.of_node;
	u16 vendor;

	dev_info(&client->dev, "%s\n", __func__);
	if (i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_I2C_BLOCK | I2C_FUNC_SMBUS_BYTE_DATA))
		pr_info("I2C functionality : OK...\n");
	else
		pr_info("I2C functionality check : failuare...\n");

	ret = rt1711_read_device(client, RT1711_REG_VENDOR_ID, 2, &vendor);
	if (ret < 0) {
		dev_notice(&client->dev,
				"%s: read vendor ID fail(%d)\n", __func__, ret);
		return -EIO;
	}

	if (vendor != 0) {
		dev_notice(&client->dev, "%s: vendor ID is not 0 (0x%02X)\n",
				__func__, vendor);
		return -ENODEV;
	}

#if TCPC_ENABLE_ANYMSG
	check_printk_performance();
#endif /* TCPC_ENABLE_ANYMSG */

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	if (use_dt)
		rt_parse_dt(chip);
	else {
		dev_notice(&client->dev, "%s: no dts node\n", __func__);
		return -ENODEV;
	}
	chip->dev = &client->dev;
	chip->client = client;
	sema_init(&chip->io_lock, 1);
	sema_init(&chip->suspend_lock, 1);
	i2c_set_clientdata(client, chip);

	ret = rt1711_regmap_init(chip);
	if (ret < 0) {
		dev_notice(chip->dev,
				"%s: regmap init fail(%d)\n", __func__, ret);
		return -EINVAL;
	}

	ret = rt1711_tcpcdev_init(chip);
	if (ret < 0) {
		dev_notice(chip->dev,
				"%s: tcpc dev init fail(%d)\n", __func__, ret);
		goto err_tcpc_reg;
	}

	rt1711_software_reset(chip->tcpc);

	ret = rt1711_init_alert(chip);
	if (ret < 0) {
		dev_notice(chip->dev,
				"%s: init alert fail(%d)\n", __func__, ret);
		goto err_irq_init;
	}

	tcpc_schedule_init_work(chip->tcpc);
	dev_info(chip->dev, "%s probe OK!\n", __func__);
	return 0;

err_irq_init:
	tcpc_device_unregister(chip->dev, chip->tcpc);
err_tcpc_reg:
	rt1711_regmap_deinit(chip);
	return ret;
}

static int rt1711_i2c_remove(struct i2c_client *client)
{
	struct rt1711_chip *chip = i2c_get_clientdata(client);

	if (chip) {
		tcpc_device_unregister(chip->dev, chip->tcpc);
		rt1711_regmap_deinit(chip);
	}

	return 0;
}

#ifdef CONFIG_PM
static int rt1711_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt1711_chip *chip = i2c_get_clientdata(client);

	RT1711_INFO("\n");
	down(&chip->suspend_lock);
	if (device_may_wakeup(dev))
		enable_irq_wake(chip->irq);

	return 0;
}

static int rt1711_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt1711_chip *chip = i2c_get_clientdata(client);

	RT1711_INFO("\n");
	if (device_may_wakeup(dev))
		disable_irq_wake(chip->irq);
	up(&chip->suspend_lock);
	return 0;
}

static void rt1711_shutdown(struct i2c_client *client)
{
	struct rt1711_chip *chip = i2c_get_clientdata(client);

	/* Please reset IC here */
	if (chip != NULL && chip->irq)
		disable_irq(chip->irq);

	pr_info("%s++\n", __func__);
	i2c_smbus_write_byte_data(client, RT1711_REG_SWRESET, 1);
	udelay(1000);
	i2c_smbus_write_byte_data(client, RT1711_REG_ROLECTRL,
		RT1711_REG_ROLE_CTRL_RES_SET(0, 0, TYPEC_CC_RD, TYPEC_CC_RD));
	i2c_smbus_write_byte_data(client, RT1711_REG_LOW_POWER_CTRL,
			RT1711_BMCIO_LPEN);
	pr_info("%s--\n", __func__);
}

#ifdef CONFIG_PM_RUNTIME
static int rt1711_pm_suspend_runtime(struct device *device)
{
	dev_dbg(device, "pm_runtime: suspending...\n");
	return 0;
}

static int rt1711_pm_resume_runtime(struct device *device)
{
	dev_dbg(device, "pm_runtime: resuming...\n");
	return 0;
}
#endif /* CONFIG_PM_RUNTIMR */

static const struct dev_pm_ops rt1711_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(
		rt1711_i2c_suspend,
		rt1711_i2c_resume)
#ifdef CONFIG_PM_RUNTIME
	SET_RUNTIME_PM_OPS(
		rt1711_pm_suspend_runtime,
		rt1711_pm_resume_runtime,
		NULL
	)
#endif /* CONFIG_PM_RUNTIMR */
};
#define RT1711_PM_OPS	(&rt1711_pm_ops)
#else
#define RT1711_PM_OPS	(NULL)
#endif /* CONFIG_PM */

static const struct i2c_device_id rt1711_id_table[] = {
	{"rt1711", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, rt1711_id_table);

static const struct of_device_id rt_match_table[] = {
	{.compatible = "mediatek,usb_type_c",},
	{},
};

static struct i2c_driver rt1711_driver = {
	.driver = {
		.name = "usb_type_c",
		.owner = THIS_MODULE,
		.of_match_table = rt_match_table,
		.pm = RT1711_PM_OPS,
	},
	.probe = rt1711_i2c_probe,
	.remove = rt1711_i2c_remove,
	.shutdown = rt1711_shutdown,
	.id_table = rt1711_id_table,
};

static int __init rt1711_init(void)
{
	struct device_node *np;

	pr_info("rt1711_init (%s): initializing...\n", RT1711_DRV_VERSION);
	np = of_find_node_by_name(NULL, "usb_type_c");
	if (np != NULL)
		pr_info("usb_type_c node found...\n");
	else
		pr_info("usb_type_c node not found...\n");

	return i2c_add_driver(&rt1711_driver);
}
module_init(rt1711_init);

static void __exit rt1711_exit(void)
{
	i2c_del_driver(&rt1711_driver);
}
module_exit(rt1711_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff Chang <jeff_chang@richtek.com>");
MODULE_DESCRIPTION("RT1711 TCPC Driver");
MODULE_VERSION(RT1711_DRV_VERSION);

/**** Release Note ****
 * 1.0.6_MTK
 * (1) enable low power mode at shutdown
 * (2) Rename & modify I2C API
 *
 * 1.0.5_MTK
 * (1) Change compatible to usb_type_c
 * (2) Correct alloc size
 * (3) Complete tcpc_ops
 *
 * 1.0.4_MTK
 * (1) change RT1711 to MTK version
 * (2) ignore retransmit message when receive tx_discard
 *
 */
