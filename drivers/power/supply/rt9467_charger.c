#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include "charger_class.h"

static int charge_online = 0;
static int charge_type = 0;
static struct charger_device *charge_dev;
static struct i2c_client *client_rt9467;
static struct delayed_work charge_work;

/*static irqreturn_t rt9467_irq_handler_thread(
    int irq, void *private)
{
    struct i2c_client *client = private;

    dev_info(&client->dev, "rt9467 isr");

    return IRQ_HANDLED;
}*/

static enum power_supply_usb_type rt9467_usb_type[] = {
    POWER_SUPPLY_USB_TYPE_SDP,
    POWER_SUPPLY_USB_TYPE_CDP,
    POWER_SUPPLY_USB_TYPE_DCP,
    POWER_SUPPLY_USB_TYPE_UNKNOWN,
};

static enum power_supply_property rt9467_power_supply_props[] = {
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_USB_TYPE,
};

static int rt9467_charger_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:
        val->intval = charge_online;
        break;
    case POWER_SUPPLY_PROP_USB_TYPE:
        if (charge_type == 2 || charge_type == 3)
            val->intval = POWER_SUPPLY_USB_TYPE_SDP;
        else if (charge_type == 4)
            val->intval = POWER_SUPPLY_USB_TYPE_DCP;
        else if (charge_type == 5)
            val->intval = POWER_SUPPLY_USB_TYPE_CDP;
        else
            val->intval = POWER_SUPPLY_USB_TYPE_UNKNOWN;
        break;
    default:
        dev_err(&client_rt9467->dev, "get prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static struct power_supply *psy_rt9467;
static char *rt9467_charger_supplied_to[] = { "battery", "mtk-master-charger_psy", };
static const struct charger_properties rt9467_chg_props = {
    .alias_name = "rt9467",
};


static struct power_supply_desc rt9467_power_supply_desc = {
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .usb_types = rt9467_usb_type,
    .num_usb_types = ARRAY_SIZE(rt9467_usb_type),
    .properties = rt9467_power_supply_props,
    .num_properties = ARRAY_SIZE(rt9467_power_supply_props),
    .get_property = rt9467_charger_get_property,
};

static struct power_supply *rt9467_register_usb_psy(
    struct i2c_client *client)
{
    struct power_supply_config psy_cfg = {
        .drv_data = client,
        .of_node = client->dev.of_node,
    };

    psy_cfg.supplied_to = rt9467_charger_supplied_to;
    psy_cfg.num_supplicants = ARRAY_SIZE(rt9467_charger_supplied_to);
    return power_supply_register(&client->dev, &rt9467_power_supply_desc, &psy_cfg);
}

static void rt9467_set_iaicr(int current_ma)
{
    int ret, value;

    if (current_ma < 100)
        value = 0;
    else if (current_ma > 3250)
        value = 3250;
    else
        value = (((current_ma - 100) / 50) << 2) | 0x2;

    ret = i2c_smbus_write_byte_data(client_rt9467, 0x03, value);
    if (ret < 0)
        dev_err(&client_rt9467->dev, "set iaicr register failed");
    else
        dev_info(&client_rt9467->dev, "set iaicr: %d", value);
}

static int rt9467_get_charge_type(void)
{
    int ret, value;

    ret = i2c_smbus_read_byte_data(client_rt9467, 0x13);
    if (ret < 0) {
        dev_err(&client_rt9467->dev, "get iaicr register failed");
        return -EINVAL;
    } else {
        value = ret & 0x07;
        return value;
    }
}

static void rt9467_charge_detect_work(
    struct work_struct *work)
{
    int ret, online;

    ret = i2c_smbus_read_byte_data(client_rt9467, 0x50);
    if (ret < 0)
        dev_err(&client_rt9467->dev, "read reg 0x50 failed");
    else
        online = !!(ret & 0x80);

    if (online != charge_online) {
        charge_online = online;
        if (online) {
            msleep(500); // wait bc1.2 complete
            charge_type = rt9467_get_charge_type();
            if (charge_type == 4)  // DCP
                rt9467_set_iaicr(2000);
            else if (charge_type == 5)  // DCP
                rt9467_set_iaicr(1500);
            else  // SDP
                rt9467_set_iaicr(500);
        } else {
            charge_type = 0;
            rt9467_set_iaicr(500);
        }
        dev_emerg(&client_rt9467->dev, "charge_online: %d", charge_online);
        power_supply_changed(psy_rt9467);
    }

    schedule_delayed_work(&charge_work, msecs_to_jiffies(100));
}

static int rt9467_charging_switch(
    struct charger_device *chg_dev, bool enable)
{
    return 0;
}

static int rt9467_set_ichrg_curr(
    struct charger_device *chg_dev, unsigned int chrg_curr)
{
    return 0;
}

static int rt9467_get_input_curr_lim(
    struct charger_device *chg_dev, unsigned int *ilim)
{
    return 0;
}

static int rt9467_set_input_curr_lim(
    struct charger_device *chg_dev, unsigned int iindpm)
{
    return 0;
}

static int rt9467_get_chrg_volt(
    struct charger_device *chg_dev, unsigned int *volt)
{
    return 4448000;
}

static int rt9467_set_chrg_volt(
    struct charger_device *chg_dev, unsigned int chrg_volt)
{
    return 0;
}

static int rt9467_reset_watch_dog_timer(
    struct charger_device *chg_dev)
{
    return 0;
}

static int rt9467_set_input_volt_lim(
    struct charger_device *chg_dev, unsigned int vindpm)
{
    return 0;
}

static int rt9467_get_charging_status(
    struct charger_device *chg_dev, bool *is_done)
{
    return 0;
}

static int rt9467_enable_safetytimer(
    struct charger_device *chg_dev, bool en)
{
    return 0;
}

static int rt9467_get_is_safetytimer_enable(
    struct charger_device *chg_dev, bool *en)
{
    return 0;
}

static int rt9467_enable_otg(
    struct charger_device *chg_dev, bool en)
{
    return 0;
}

static int rt9467_set_boost_current_limit(
    struct charger_device *chg_dev, u32 uA)
{
    return 0;
}

static int rt9467_do_event(
    struct charger_device *chg_dev, u32 event,
                u32 args)
{
    if (chg_dev == NULL)
        return -EINVAL;

    switch (event) {
    case CHARGER_DEV_NOTIFY_EOC:
        charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
        break;
    case CHARGER_DEV_NOTIFY_RECHG:
        charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
        break;
    default:
        break;
    }

    return 0;
}

static int rt9467_en_pe_current_partern(
    struct charger_device *chg_dev, bool is_up)
{
    return 0;
}

static struct charger_ops rt9467_chg_ops = {
    .event = rt9467_do_event,
    .enable_otg = rt9467_enable_otg,
    .enable = rt9467_charging_switch,
    .set_charging_current = rt9467_set_ichrg_curr,
    .get_input_current = rt9467_get_input_curr_lim,
    .set_input_current = rt9467_set_input_curr_lim,
    .get_constant_voltage = rt9467_get_chrg_volt,
    .set_constant_voltage = rt9467_set_chrg_volt,
    .kick_wdt = rt9467_reset_watch_dog_timer,
    .set_mivr = rt9467_set_input_volt_lim,
    .is_charging_done = rt9467_get_charging_status,
    .enable_safety_timer = rt9467_enable_safetytimer,
    .is_safety_timer_enabled = rt9467_get_is_safetytimer_enable,
    .set_boost_current_limit = rt9467_set_boost_current_limit,
    .send_ta_current_pattern = rt9467_en_pe_current_partern,
};

static int rt9467_driver_probe(
    struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;

    client_rt9467 = client;
    /*ret = devm_request_threaded_irq(&client->dev, client->irq, NULL,
                rt9467_irq_handler_thread,
                IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                dev_name(&client->dev), client);
    if (ret) {
        dev_err(&client->dev, "rt9467 request interrupt failed\n");
        return -EPERM;
    }*/

    ret = i2c_smbus_write_byte_data(client, 0x02, 0x0B);
    if (ret < 0) {
        dev_err(&client->dev, "write reg: 0x02 failed");
        return -EINVAL;
    }

    ret = i2c_smbus_write_byte_data(client, 0x03, 0x22);
    if (ret < 0) {
        dev_err(&client->dev, "write reg: 0x07 failed");
        return -EINVAL;
    }

    /* Register charger device */
    charge_dev = charger_device_register("primary_chg",
                        &client->dev, client_rt9467,
                        &rt9467_chg_ops,
                        &rt9467_chg_props);
    if (IS_ERR_OR_NULL(charge_dev)) {
        dev_err(&client->dev, "rt9467 register charger device failed\n");
        return -EPERM;
    }

    psy_rt9467 = rt9467_register_usb_psy(client);
    if (IS_ERR_OR_NULL(psy_rt9467)) {
        dev_err(&client->dev, "register usb psy failed");
        charger_device_unregister(charge_dev);
        return -EPERM;
    }

    INIT_DELAYED_WORK(&charge_work, rt9467_charge_detect_work);
    schedule_delayed_work(&charge_work, msecs_to_jiffies(100));
 
    dev_info(&client->dev, "rt9467_driver_probe succeeded at 202112021300");

    return 0;
}

static int rt9467_charger_remove(struct i2c_client *client)
{
    cancel_delayed_work(&charge_work);
    power_supply_unregister(psy_rt9467);
    charger_device_unregister(charge_dev);

    return 0;
}

static const struct i2c_device_id rt9467_i2c_ids[] = {
    { "rt9467", 0 }, { },
};

static const struct of_device_id rt9467_of_match[] = {
    { .compatible = "richtek,rt9467", }, { },
};

static struct i2c_driver rt9467_driver = {
    .driver = {
        .name = "rt9467-charger",
        .of_match_table = rt9467_of_match,        
    },
    .probe = rt9467_driver_probe,
    .remove = rt9467_charger_remove,
    .id_table = rt9467_i2c_ids,
};

module_i2c_driver(rt9467_driver);
MODULE_LICENSE("GPL v2");
