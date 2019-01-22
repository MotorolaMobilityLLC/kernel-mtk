/* drivers/power/mediatek/charger_class.c
 * Switching Charger Class Device Driver
 *
 * Copyright (C) 2013 Richtek Technology Corp.
 * Author: Patrick Chang <patrick_chang@richtek.com>
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

#include <linux/module.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <mt-plat/charger_class.h>

static struct class *charger_class;

static ssize_t charger_show_name(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct charger_device *charger_dev = to_charger_device(dev);

	return sprintf(buf, "%s\n",
		       charger_dev->props.alias_name ?
		       charger_dev->props.alias_name : "anonymous");
}

static int charger_suspend(struct device *dev, pm_message_t state)
{
	struct charger_device *charger_dev = to_charger_device(dev);

	if (charger_dev->ops->suspend)
		charger_dev->ops->suspend(charger_dev, state);
	return 0;
}

static int charger_resume(struct device *dev)
{
	struct charger_device *charger_dev = to_charger_device(dev);

	if (charger_dev->ops->resume)
		charger_dev->ops->resume(charger_dev);
	return 0;
}

static void charger_device_release(struct device *dev)
{
	struct charger_device *charger_dev = to_charger_device(dev);

	kfree(charger_dev);
}

int charger_dev_enable(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->enable)
		charger_dev->ops->enable(charger_dev);
	return 0;
}

int charger_dev_disable(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->disable)
		charger_dev->ops->disable(charger_dev);

	return 0;
}

int charger_dev_plug_in(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->plug_in)
		charger_dev->ops->plug_in(charger_dev);

	return 0;
}

int charger_dev_plug_out(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->plug_out)
		charger_dev->ops->plug_out(charger_dev);

	return 0;
}

int charger_dev_set_charging_current(struct charger_device *charger_dev, int uA)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->set_charging_current)
		charger_dev->ops->set_charging_current(charger_dev, uA);

	return 0;
}

int charger_dev_get_charging_current(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->get_charging_current)
		return charger_dev->ops->get_charging_current(charger_dev);

	return 0;
}

int charger_dev_set_input_current(struct charger_device *charger_dev, int uA)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->set_input_current)
		charger_dev->ops->set_input_current(charger_dev, uA);

	return 0;
}

int charger_dev_get_input_current(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->get_input_current)
		return charger_dev->ops->get_input_current(charger_dev);

	return 0;
}

int charger_dev_set_constant_voltage(struct charger_device *charger_dev, int uV)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->set_constant_voltage)
		charger_dev->ops->set_constant_voltage(charger_dev, uV);

	return 0;
}

int charger_dev_get_constant_voltage(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->get_constant_voltage)
		return charger_dev->ops->get_constant_voltage(charger_dev);

	return 0;
}

int charger_dev_dump_registers(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->dump_registers)
		return charger_dev->ops->dump_registers(charger_dev);

	return 0;
}

int charger_dev_get_charging_status(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->get_charging_status)
		return charger_dev->ops->get_charging_status(charger_dev);

	return 0;
}

int charger_dev_enable_vbus_ovp(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->enable_vbus_ovp)
		return charger_dev->ops->enable_vbus_ovp(charger_dev);

	return 0;
}

int charger_dev_disable_vbus_ovp(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->disable_vbus_ovp)
		return charger_dev->ops->disable_vbus_ovp(charger_dev);

	return 0;
}

int charger_dev_set_mivr(struct charger_device *charger_dev, int uv)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->set_mivr)
		return charger_dev->ops->set_mivr(charger_dev, uv);

	return 0;
}

int charger_dev_enable_powerpath(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->enable_powerpath)
		return charger_dev->ops->enable_powerpath(charger_dev);

	return 0;
}

int charger_dev_disable_powerpath(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->disable_powerpath)
		return charger_dev->ops->disable_powerpath(charger_dev);

	return 0;
}

int charger_dev_get_mivr_state(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->get_mivr_state)
		return charger_dev->ops->get_mivr_state(charger_dev);

	return 0;
}

int charger_dev_send_ta20_current_pattern(struct charger_device *charger_dev, int uv)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->send_ta20_current_pattern)
		return charger_dev->ops->send_ta20_current_pattern(charger_dev, uv);

	return 0;
}

int charger_dev_set_ta20_reset(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->set_ta20_reset)
		return charger_dev->ops->set_ta20_reset(charger_dev);

	return 0;
}

int charger_dev_set_pe20_efficiency_table(struct charger_device *charger_dev)
{
	if (charger_dev != NULL && charger_dev->ops != NULL && charger_dev->ops->set_pe20_efficiency_table)
		return charger_dev->ops->set_pe20_efficiency_table(charger_dev);

	return 0;
}

int charger_dev_notify(struct charger_device *charger_dev, int event)
{
	return srcu_notifier_call_chain(
		&charger_dev->evt_nh, event, charger_dev);
}

static DEVICE_ATTR(name, S_IRUGO, charger_show_name, NULL);

static struct attribute *charger_class_attrs[] = {
	&dev_attr_name.attr,
	NULL,
};

static const struct attribute_group charger_group = {
	.attrs = charger_class_attrs,
};

static const struct attribute_group *charger_groups[] = {
	&charger_group,
	NULL,
};

int register_charger_device_notifier(struct charger_device *charger_dev,
			      struct notifier_block *nb)
{
	int ret;

	ret = srcu_notifier_chain_register(&charger_dev->evt_nh, nb);
	return ret;
}
EXPORT_SYMBOL(register_charger_device_notifier);

int unregister_charger_device_notifier(struct charger_device *charger_dev,
				struct notifier_block *nb)
{
	return srcu_notifier_chain_unregister(&charger_dev->evt_nh, nb);
}
EXPORT_SYMBOL(unregister_charger_device_notifier);

/**
 * charger_device_register - create and register a new object of
 *   charger_device class.
 * @name: the name of the new object
 * @parent: a pointer to the parent device
 * @devdata: an optional pointer to be stored for private driver use.
 * The methods may retrieve it by using charger_get_data(charger_dev).
 * @ops: the charger operations structure.
 *
 * Creates and registers new charger device. Returns either an
 * ERR_PTR() or a pointer to the newly allocated device.
 */
struct charger_device *charger_device_register(const char *name,
		struct device *parent, void *devdata,
		const struct charger_ops *ops,
		const struct charger_properties *props)
{
	struct charger_device *charger_dev;
	int rc;

	pr_debug("charger_device_register: name=%s\n", name);
	charger_dev = kzalloc(sizeof(*charger_dev), GFP_KERNEL);
	if (!charger_dev)
		return ERR_PTR(-ENOMEM);

	mutex_init(&charger_dev->ops_lock);
	charger_dev->dev.class = charger_class;
	charger_dev->dev.parent = parent;
	charger_dev->dev.release = charger_device_release;
	dev_set_name(&charger_dev->dev, name);
	dev_set_drvdata(&charger_dev->dev, devdata);
	srcu_init_notifier_head(&charger_dev->evt_nh);

	/* Copy properties */
	if (props) {
		memcpy(&charger_dev->props, props,
		       sizeof(struct charger_properties));
	}
	rc = device_register(&charger_dev->dev);
	if (rc) {
		kfree(charger_dev);
		return ERR_PTR(rc);
	}
	charger_dev->ops = ops;
	return charger_dev;
}
EXPORT_SYMBOL(charger_device_register);

/**
 * charger_device_unregister - unregisters a switching charger device
 * object.
 * @charger_dev: the switching charger device object to be unregistered
 * and freed.
 *
 * Unregisters a previously registered via charger_device_register object.
 */
void charger_device_unregister(struct charger_device *charger_dev)
{
	if (!charger_dev)
		return;

	mutex_lock(&charger_dev->ops_lock);
	charger_dev->ops = NULL;
	mutex_unlock(&charger_dev->ops_lock);
	device_unregister(&charger_dev->dev);
}
EXPORT_SYMBOL(charger_device_unregister);


static int charger_match_device_by_name(struct device *dev,
	const void *data)
{
	const char *name = data;

	return strcmp(dev_name(dev), name) == 0;
}

struct charger_device *get_charger_by_name(const char *name)
{
	struct device *dev;

	if (!name)
		return (struct charger_device *)NULL;
	dev = class_find_device(charger_class, NULL, name,
				charger_match_device_by_name);

	return dev ? to_charger_device(dev) : NULL;

}
EXPORT_SYMBOL(get_charger_by_name);

static void __exit charger_class_exit(void)
{
	class_destroy(charger_class);
}

static int __init charger_class_init(void)
{
	charger_class = class_create(THIS_MODULE, "switching_charger");
	if (IS_ERR(charger_class)) {
		pr_err("Unable to create charger class; errno = %ld\n",
			PTR_ERR(charger_class));
		return PTR_ERR(charger_class);
	}
	charger_class->dev_groups = charger_groups;
	charger_class->suspend = charger_suspend;
	charger_class->resume = charger_resume;
	return 0;
}

subsys_initcall(charger_class_init);
module_exit(charger_class_exit);

MODULE_DESCRIPTION("Switching Charger Class Device");
MODULE_AUTHOR("Patrick Chang <patrick_chang@richtek.com>");
MODULE_VERSION("1.0.0_G");
MODULE_LICENSE("GPL");
