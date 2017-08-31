/* drivers/misc/mediatek/include/mt-plat/charger_class.h
 * Header of Switching Charger Class Device Driver
 *
 * Copyright (C) 2016 Richtek Technology Corp.
 * Patrick Chang <patrick_chang@richtek.com>
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

#ifndef LINUX_POWER_CHARGER_CLASS_H
#define LINUX_POWER_CHARGER_CLASS_H

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mutex.h>


struct charger_properties {
	const char *alias_name;
};

struct charger_device {
	struct charger_properties props;
	const struct charger_ops *ops;
	struct mutex ops_lock;
	struct device dev;
	struct srcu_notifier_head evt_nh;
	void	*driver_data;
};

struct charger_ops {
	int (*suspend)(struct charger_device *, pm_message_t);
	int (*resume)(struct charger_device *);

	/* cable plug in/out */
	int (*plug_in)(struct charger_device *);
	int (*plug_out)(struct charger_device *);

	/* enable/disable charger */
	int (*enable)(struct charger_device *);
	int (*disable)(struct charger_device *);

	/* enable/disable chip */
	int (*enable_chip)(struct charger_device *, bool en);

	/* get/set charging current*/
	int (*get_charging_current)(struct charger_device *);
	int (*set_charging_current)(struct charger_device *, int uA);

	/* set cv */
	int (*set_constant_voltage)(struct charger_device *, int uv);
	int (*get_constant_voltage)(struct charger_device *);

	/* set input_current */
	int (*get_input_current)(struct charger_device *);
	int (*set_input_current)(struct charger_device *, int uA);

	/* kick wdt */
	int (*kick_wdt)(struct charger_device *);

	/* PE/PE+ */
	int (*send_ta_current_pattern)(struct charger_device *, bool is_increase);

	/* PE+ 20*/
	int (*send_ta20_current_pattern)(struct charger_device *, int uv);
	int (*set_ta20_reset)(struct charger_device *);

	int (*set_mivr)(struct charger_device *, int uv);

	/* enable/disable powerpath */
	int (*enable_powerpath)(struct charger_device *);
	int (*disable_powerpath)(struct charger_device *);

	/* enable/disable vbus ovp */
	int (*enable_vbus_ovp)(struct charger_device *);
	int (*disable_vbus_ovp)(struct charger_device *);

	/* enable/disable charging safety timer */
	int (*enable_safety_timer)(struct charger_device *);
	int (*disable_safety_timer)(struct charger_device *);

	/* direct charging */
	int (*enable_direct_charging)(struct charger_device *);
	int (*disable_direct_charging)(struct charger_device *);
	int (*kick_direct_charging_wdt)(struct charger_device *);

	/* run AICL */
	int (*run_aicl)(struct charger_device *);

	int (*get_charging_status)(struct charger_device *);
	int (*set_pe20_efficiency_table)(struct charger_device *);
	int (*dump_registers)(struct charger_device *);

	int (*get_ibus_adc)(struct charger_device *, u32 *ibus);
	int (*get_tchg_adc)(struct charger_device *, int *tchg_min,
		int *tchg_max);
};

static inline void *charger_dev_get_drvdata(const struct charger_device *charger_dev)
{
	return charger_dev->driver_data;
}

static inline void charger_dev_set_drvdata(struct charger_device *charger_dev, void *data)
{
	charger_dev->driver_data = data;
}

extern struct charger_device *charger_device_register(const char *name,
	struct device *parent, void *devdata, const struct charger_ops *ops,
	const struct charger_properties *props);
extern void charger_device_unregister(struct charger_device *charger_dev);
extern struct charger_device *get_charger_by_name(const char *name);

#define to_charger_device(obj) container_of(obj, struct charger_device, dev)

static inline void *charger_get_data(
	struct charger_device *charger_dev)
{
	return dev_get_drvdata(&charger_dev->dev);
}

extern int charger_dev_enable(struct charger_device *charger_dev);
extern int charger_dev_disable(struct charger_device *charger_dev);
extern int charger_dev_plug_in(struct charger_device *charger_dev);
extern int charger_dev_plug_out(struct charger_device *charger_dev);
extern int charger_dev_set_charging_current(struct charger_device *charger_dev, int uA);
extern int charger_dev_get_charging_current(struct charger_device *charger_dev);
extern int charger_dev_set_input_current(struct charger_device *charger_dev, int uA);
extern int charger_dev_get_input_current(struct charger_device *charger_dev);
extern int charger_dev_set_constant_voltage(struct charger_device *charger_dev, int uV);
extern int charger_dev_get_constant_voltage(struct charger_device *charger_dev);
extern int charger_dev_dump_registers(struct charger_device *charger_dev);
extern int charger_dev_enable_vbus_ovp(struct charger_device *charger_dev);
extern int charger_dev_disable_vbus_ovp(struct charger_device *charger_dev);
extern int charger_dev_set_mivr(struct charger_device *charger_dev, int uv);
extern int charger_dev_send_ta20_current_pattern(struct charger_device *charger_dev, int uv);
extern int charger_dev_set_ta20_reset(struct charger_device *charger_dev);
extern int charger_dev_set_pe20_efficiency_table(struct charger_device *charger_dev);

extern int register_charger_device_notifier(struct charger_device *charger_dev,
			      struct notifier_block *nb);
extern int unregister_charger_device_notifier(struct charger_device *charger_dev,
				struct notifier_block *nb);
extern int charger_dev_notify(struct charger_device *charger_dev, int event);
extern int charger_dev_get_charging_status(struct charger_device *charger_dev);
extern int charger_dev_enable_powerpath(struct charger_device *charger_dev);
extern int charger_dev_disable_powerpath(struct charger_device *charger_dev);


#endif /*LINUX_POWER_CHARGER_CLASS_H*/
