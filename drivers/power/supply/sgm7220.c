#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/bits.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/usb/typec.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>

#undef dev_fmt
#define dev_fmt(fmt) "202202241930: " fmt

int current_max_sgm7220 = 0;
static int cc_attached = 0;
static int cc_orientation = 0;
static struct class *typec_class;
static struct device *port_device;
static struct i2c_client *sgm7220_client;

static ssize_t sgm7220_orientation_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%s\n",
                    !cc_orientation ? "none" : (
                    cc_orientation == 1 ? "cc_1" : "cc_2"));
}

static struct device_attribute dev_attr_orientation =
    __ATTR(typec_cc_polarity_role, S_IRUSR,
    sgm7220_orientation_show, NULL);

static void sgm7220_get_isr_status(void)
{
    int ret;
    u8 value;
    u8 attach;

    ret = i2c_smbus_read_byte_data(sgm7220_client, 0x09);
    if (ret < 0) {
        dev_err(&sgm7220_client->dev,
            "isr read 0x09 register failed");
        goto out;
    }

    value = ret;
    if (value & GENMASK(7, 6)) {
        attach = 1;
        dev_info(&sgm7220_client->dev, "attached");
    } else {
        attach = 0;
        dev_info(&sgm7220_client->dev, "detached");
    }
    
    if (attach == cc_attached) {
        dev_info(&sgm7220_client->dev, "same state [%s]",
            cc_attached ? "attached" : "detached");
        goto out;
    }

    cc_attached = attach;
    if (value & GENMASK(7, 6))
        cc_orientation = !(value & BIT(5)) ? 1 : 2;
    else {
        cc_orientation = 0;
        current_max_sgm7220 = 3000;
    }
    dev_info(&sgm7220_client->dev, "new state [%s]",
        cc_attached ? "attached" : "detached");
    dev_info(&sgm7220_client->dev,
            "cc_orientation [%d]", cc_orientation);

    ret = i2c_smbus_read_byte_data(sgm7220_client, 0x08);
    if (ret < 0) {
        dev_err(&sgm7220_client->dev,
            "isr read 0x08 register failed");
        goto out;
    }

    current_max_sgm7220 = ((ret & GENMASK(5, 4)) == BIT(4)) ? 1500 : 3000;
    dev_info(&sgm7220_client->dev,
        "cc_current_max [%d]", current_max_sgm7220);

out:
    ret = i2c_smbus_write_byte_data(sgm7220_client,
            0x09, value | (1 << 4));
    if (ret < 0)
        dev_err(&sgm7220_client->dev, "isr clear 0x09 bit4 failed");
}

static int sgm7220_set_drp_mode(void)
{
    int ret;

    ret = i2c_smbus_read_byte_data(sgm7220_client, 0x0a);
    if (ret < 0) {
       dev_err(&sgm7220_client->dev, "read 0x0a failed");
       return -ENODEV;
    }
    dev_info(&sgm7220_client->dev, "0x0a [%#x]", ret);
    if ((ret & GENMASK(5, 4)) != GENMASK(5, 4)) {
        dev_info(&sgm7220_client->dev, "set to drp mode");
        ret = i2c_smbus_write_byte_data(sgm7220_client, 0x0a, ret | GENMASK(5, 4));
        if (ret < 0) {
           dev_err(&sgm7220_client->dev, "write 0x0a failed");
           return -ENODEV;
        }
    }

    return 0;
}

static int sgm7220_get_initial_status(void)
{
    int ret;

    ret = i2c_smbus_read_byte_data(sgm7220_client, 0x08);
    if (ret < 0) {
        dev_err(&sgm7220_client->dev, "read 0x08 failed");
        return -ENODEV;
    }
    current_max_sgm7220 = ((ret & GENMASK(5, 4)) == BIT(4)) ? 1500 : 3000;

    ret = i2c_smbus_read_byte_data(sgm7220_client, 0x09);
    if (ret < 0) {
       dev_err(&sgm7220_client->dev, "read 0x09 failed");
       return -ENODEV;
    }
    if (ret & GENMASK(7, 6)) {
        cc_attached = 1;
        cc_orientation = !(ret & BIT(5)) ? 1 : 2;
    } else {
        cc_attached = 0;
        cc_orientation = 0;
    }

    dev_info(&sgm7220_client->dev,
            "initial cc_orientation: %d, cc_current_max: %d",
            cc_orientation, current_max_sgm7220);

    ret = i2c_smbus_write_byte_data(sgm7220_client, 0x09, ret | BIT(4));
    if (ret < 0) {
       dev_err(&sgm7220_client->dev, "write 0x09 failed");
       return -EPERM;
    }

    return 0;
}

static irqreturn_t sgm7220_irq_handler_thread(
    int irq, void *private)
{
    dev_info(&sgm7220_client->dev, "sgm7220 irq");
    sgm7220_get_isr_status();
    return IRQ_HANDLED;
}

static int sgm7220_driver_probe(
    struct i2c_client *client,
    const struct i2c_device_id *id)
{
    int ret;

    sgm7220_client = client;

    ret = sgm7220_set_drp_mode();
    if (ret) {
        dev_err(&sgm7220_client->dev, "set cc to drp mode failed");
        return -EPERM;
    }

    ret = sgm7220_get_initial_status();
    if (ret) {
        dev_err(&sgm7220_client->dev, "get cc initial state failed");
        return -EPERM;
    }

    typec_class = class_create(THIS_MODULE, "cclogic");
    if (IS_ERR(typec_class)) {
        dev_err(&client->dev, "create typec class failed");
        return -EPERM;
    }
    port_device = device_create(typec_class, &client->dev, 0, NULL, "port0");
    if (IS_ERR(port_device)) {
        dev_err(&client->dev, "create port0 device failed");
        goto err_create_device;
    }
    ret = device_create_file(port_device, &dev_attr_orientation);
    if (ret) {
        dev_err(&client->dev, "create orientation file failed");
        goto err_create_file;
    }

    ret = devm_request_threaded_irq(&client->dev, client->irq,
                    NULL, sgm7220_irq_handler_thread,
                    IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                    dev_name(&client->dev), NULL);
    if (ret) {
        dev_err(&client->dev, "request interrupt failed");
        goto err_request_irq;
    }
    ret = enable_irq_wake(client->irq);
    if (ret) {
        dev_err(&client->dev, "enable irq wake failed");
        goto err_enable_irq_wake;
    }

    dev_info(&client->dev, "sgm7220 probe finished");

    return 0;

err_enable_irq_wake:
    free_irq(client->irq, NULL);
err_request_irq:
    device_remove_file(&client->dev, &dev_attr_orientation);
err_create_file:
    device_destroy(typec_class, 0);
err_create_device:
    class_destroy(typec_class);

    return -EBUSY;
}

static int sgm7220_charger_remove(struct i2c_client *client)
{
    disable_irq_wake(client->irq);
    free_irq(client->irq, NULL);
    device_remove_file(&client->dev, &dev_attr_orientation);
    device_destroy(typec_class, 0);
    class_destroy(typec_class);

    return 0;
}

static const struct i2c_device_id sgm7220_i2c_ids[] = {
    { "sgm7220", 0 }, { },
};

static const struct of_device_id sgm7220_of_match[] = {
    { .compatible = "sgm,sgm7220", }, { },
};

static struct i2c_driver sgm7220_driver = {
    .driver = {
        .name = "sgm7220-cclogic",
        .of_match_table = sgm7220_of_match,
    },
    .probe = sgm7220_driver_probe,
    .remove = sgm7220_charger_remove,
    .id_table = sgm7220_i2c_ids,
};

module_i2c_driver(sgm7220_driver);
MODULE_LICENSE("GPL v2");

