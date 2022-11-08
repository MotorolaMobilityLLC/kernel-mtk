
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include "charger_class.h"
#include <linux/scc_drv.h>

static int charge_online = 0;
static int charge_type = 0;
static int irq_gpio = 0;
static struct charger_device *charge_dev;
static struct i2c_client *client_sc89890;
static struct delayed_work charge_work;



	static u8 reg_index[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14,
    };

static irqreturn_t sc89890_irq_handler_thread(
    int irq, void *private)
{
    struct i2c_client *client = private;
    dev_err(&client->dev, "danny handle irq\n");
    schedule_delayed_work(&charge_work, msecs_to_jiffies(100));
    return IRQ_HANDLED;
}

static enum power_supply_usb_type sc89890_usb_type[] = {
    POWER_SUPPLY_USB_TYPE_SDP,
    POWER_SUPPLY_USB_TYPE_CDP,
    POWER_SUPPLY_USB_TYPE_DCP,
    POWER_SUPPLY_USB_TYPE_UNKNOWN,
};

static enum power_supply_property sc89890_power_supply_props[] = {
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_USB_TYPE,
};

static int sc89890_charger_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:
        val->intval = charge_online;
        break;
    case POWER_SUPPLY_PROP_USB_TYPE:
     if(charge_online ==1)
     {
        if (charge_type == 1)
            val->intval = POWER_SUPPLY_USB_TYPE_SDP;
        else if (charge_type == 2)
           val->intval = POWER_SUPPLY_USB_TYPE_CDP;
        else 
             val->intval = POWER_SUPPLY_USB_TYPE_DCP;
     }else
            val->intval = POWER_SUPPLY_USB_TYPE_UNKNOWN;
         
	 pr_err("danny sc89890 get property charge_type=%d\n",charge_type);
     break;
    
    default:
        dev_err(&client_sc89890->dev, "danny get prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static struct power_supply *psy_sc89890;
static char *sc89890_charger_supplied_to[] = { "battery", "mtk-master-charger", };
static const struct charger_properties sc89890_chg_props = {
    .alias_name = "sc89890",
};


static struct power_supply_desc sc89890_power_supply_desc = {
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .usb_types = sc89890_usb_type,
    .num_usb_types = ARRAY_SIZE(sc89890_usb_type),
    .properties = sc89890_power_supply_props,
    .num_properties = ARRAY_SIZE(sc89890_power_supply_props),
    .get_property = sc89890_charger_get_property,
};

static struct power_supply *sc89890_register_usb_psy(
    struct i2c_client *client)
{
    struct power_supply_config psy_cfg = {
        .drv_data = client,
        .of_node = client->dev.of_node,
    };

    psy_cfg.supplied_to = sc89890_charger_supplied_to;
    psy_cfg.num_supplicants = ARRAY_SIZE(sc89890_charger_supplied_to);
    return power_supply_register(&client->dev, &sc89890_power_supply_desc, &psy_cfg);
}

static int sc89890_get_charge_type(void)
{
    int ret, value;

    ret = i2c_smbus_read_byte_data(client_sc89890, 0x0b);
    if (ret < 0) {
        dev_err(&client_sc89890->dev, "get iaicr register failed");
        return -EINVAL;
    } else {
        value = (ret & 0xE0) >> 5;
        return value;
    }
}

static void sc89890_charge_detect_work(
    struct work_struct *work)
{
    int ret, online;
    int i;
    for (i = 0; i < ARRAY_SIZE(reg_index); i++) {
        ret = i2c_smbus_read_byte_data(client_sc89890, reg_index[i]);
        if (ret < 0) {
             dev_err(&client_sc89890->dev, "danny read faid\n");
        }
       // pr_err("danny sc89890 %#04x: %#04x\n", reg_index[i], ret);
    }

    ret = i2c_smbus_read_byte_data(client_sc89890, 0x11);
    if (ret < 0)
        dev_err(&client_sc89890->dev, "read reg 0x11 failed");
    else{
          online = !!(ret & 0x80);
    }

    charge_type = sc89890_get_charge_type();
    if(charge_type !=0){
     pr_err("danny charge_type =%d",charge_type);
     if (charge_type == 4)  //HVDCP
     {  
                //  ret = i2c_smbus_write_byte_data(client_sc89890, 0x01, 0x43);
                //  msleep(2000);
                //  ret = i2c_smbus_write_byte_data(client_sc89890, 0x01, 0xcb); 
        pr_err("danny sc89890 HVDCP\n");
     } 
     else if (charge_type == 3)  // DCP
     { 
        pr_err("danny sc89890 dcp\n");
     }
     else if (charge_type == 2)
        pr_err("danny sc89890 cdp\n");
     else  // SDP 
       pr_err("danny sc89890 sdp\n");
            
    } 
    else 
    {  
       charge_type = 0;      
    }
    if (online != charge_online) {
        charge_online = online;
        pr_err("danny sc89890 power_supply_changed end\n");
    }
   power_supply_changed(psy_sc89890);
  
}

static int sc89890_charging_switch(
    struct charger_device *chg_dev, bool enable)
{
    return 0;
}

static int sc89890_set_ichrg_curr(
    struct charger_device *chg_dev, unsigned int chrg_curr)
{
    return 0;
}

static int sc89890_get_input_curr_lim(
    struct charger_device *chg_dev, unsigned int *ilim)
{
    return 0;
}

static int sc89890_set_input_curr_lim(
    struct charger_device *chg_dev, unsigned int iindpm)
{
    return 0;
}

static int sc89890_get_chrg_volt(
    struct charger_device *chg_dev, unsigned int *volt)
{
    return 4448000;
}

static int sc89890_set_chrg_volt(
    struct charger_device *chg_dev, unsigned int chrg_volt)
{
    return 0;
}

static int sc89890_reset_watch_dog_timer(
    struct charger_device *chg_dev)
{
    return 0;
}

static int sc89890_set_input_volt_lim(
    struct charger_device *chg_dev, unsigned int vindpm)
{
    return 0;
}

static int sc89890_get_charging_status(
    struct charger_device *chg_dev, bool *is_done)
{
    return 0;
}

static int sc89890_enable_safetytimer(
    struct charger_device *chg_dev, bool en)
{
    return 0;
}

static int sc89890_get_is_safetytimer_enable(
    struct charger_device *chg_dev, bool *en)
{
    return 0;
}

static int sc89890_enable_otg(
    struct charger_device *chg_dev, bool en)
{
    return 0;
}

static int sc89890_set_boost_current_limit(
    struct charger_device *chg_dev, u32 uA)
{
    return 0;
}

static int sc89890_do_event(
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

static int sc89890_en_pe_current_partern(
    struct charger_device *chg_dev, bool is_up)
{
    return 0;
}

static struct charger_ops sc89890_chg_ops = {
    .event = sc89890_do_event,
    .enable_otg = sc89890_enable_otg,
    .enable = sc89890_charging_switch,
    .set_charging_current = sc89890_set_ichrg_curr,
    .get_input_current = sc89890_get_input_curr_lim,
    .set_input_current = sc89890_set_input_curr_lim,
    .get_constant_voltage = sc89890_get_chrg_volt,
    .set_constant_voltage = sc89890_set_chrg_volt,
    .kick_wdt = sc89890_reset_watch_dog_timer,
    .set_mivr = sc89890_set_input_volt_lim,
    .is_charging_done = sc89890_get_charging_status,
    .enable_safety_timer = sc89890_enable_safetytimer,
    .is_safety_timer_enabled = sc89890_get_is_safetytimer_enable,
    .set_boost_current_limit = sc89890_set_boost_current_limit,
    .send_ta_current_pattern = sc89890_en_pe_current_partern,
};

static int sc89890_driver_probe(
    struct i2c_client *client, const struct i2c_device_id *id)
{
    
	int ret = 0;
    int irqn ;
    client_sc89890 = client;
    pr_err("danny sc89890-start-10\n");
	
    INIT_DELAYED_WORK(&charge_work, sc89890_charge_detect_work);

	 ret = i2c_smbus_write_byte_data(client, 0x14, 0x80);
    if (ret < 0) {
        dev_err(&client->dev, "write reg: 0x14 failed");
        return -EINVAL;
    }

    
    ret = i2c_smbus_write_byte_data(client, 0x07, 0x8d);
    if (ret < 0) {
        dev_err(&client->dev, "write reg: 0x07 failed");
        return -EINVAL;
    }

	ret = i2c_smbus_write_byte_data(client, 0x00, 0x08);
    if (ret < 0) {
        dev_err(&client->dev, "write reg: 0x00 failed");
        return -EINVAL;
    }

    ret = i2c_smbus_write_byte_data(client_sc89890, 0x02, 0x11);

    irq_gpio = of_get_named_gpio(client->dev.of_node, "sc,irq-gpio", 0);
    if (!gpio_is_valid(irq_gpio)) {
        pr_err("danny irq-gpio failed\n");
        return 0;
    }
    
    irqn = gpio_to_irq(irq_gpio);
    if (irqn > 0)
        client->irq = irqn;
    else {
       pr_err("danny map irq-gpio to irq failed\n");
       return 0;
    }
    
    ret = gpio_request_one(irq_gpio, GPIOF_DIR_IN, "sc89890_irq_pin");
    if (ret) {
       pr_err("danny request rt,irq-gpio failed\n");
       gpio_free(irq_gpio);
       return 0;
    }    
    
     ret = devm_request_threaded_irq(&client->dev, client->irq, NULL,
                sc89890_irq_handler_thread,
                IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                dev_name(&client->dev), client);
    if (ret) {
      pr_err("danny sc89890 request interrupt failed\n");
        return -EPERM;
    }
     
    ret = enable_irq_wake(client->irq);
    if (ret) {
       pr_err("danny irq_wake failed\n");
        return 0;
    }
  

    /* Register charger device */
    charge_dev = charger_device_register("primary_chg",
                        &client->dev, client_sc89890,
                        &sc89890_chg_ops,
                        &sc89890_chg_props);
    if (IS_ERR_OR_NULL(charge_dev)) {
        dev_err(&client->dev, "sc89890 register charger device failed\n");
        return -EPERM;
    }

    psy_sc89890 = sc89890_register_usb_psy(client);
    if (IS_ERR_OR_NULL(psy_sc89890)) {
        dev_err(&client->dev, "register usb psy failed");
        charger_device_unregister(charge_dev);
        return -EPERM;
    }

     pr_err("danny sc89890-10\n");
     sc89890_irq_handler_thread(client->irq,client);
    dev_info(&client->dev, "sc89890_driver_probe succeeded at 202112021300");
     pr_err("danny sc89890 end\n");
    return 0;
}

static int sc89890_charger_remove(struct i2c_client *client)
{
    cancel_delayed_work(&charge_work);
    power_supply_unregister(psy_sc89890);
    charger_device_unregister(charge_dev);
    gpio_free(irq_gpio);
    return 0;
}

static const struct i2c_device_id sc89890_i2c_ids[] = {
    { "sc89890", 0 }, { },
};

static const struct of_device_id sc89890_of_match[] = {
    { .compatible = "ti,sc89890-1", }, { },
};

static struct i2c_driver sc89890_driver = {
    .driver = {
        .name = "sc89890-charger",
        .of_match_table = sc89890_of_match,        
    },
    .probe = sc89890_driver_probe,
    .remove = sc89890_charger_remove,
    .id_table = sc89890_i2c_ids,
};

module_i2c_driver(sc89890_driver);
MODULE_LICENSE("GPL v2");
