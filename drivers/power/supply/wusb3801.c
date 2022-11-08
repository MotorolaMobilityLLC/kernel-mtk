#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/bits.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#undef dev_fmt
#define dev_fmt(fmt) "202203091630: " fmt

#define NON_INTERRUPT     0x00
#define CC_ATTACHED       0x01
#define CC_DETACHED       0x02
#define CC_KNOWN_STATE    0x03

#define CC_STANDBY        0x00
#define CC1_ATTACHED      0x01
#define CC2_ATTACHED      0x02
#define CC12_ATTACHED     0x03

#define DEVICE_ID_REG     0x01
#define DEVICE_CTRL_REG   0x02
#define DEVICE_ISR_REG    0x03
#define DEVICE_STATE_REG  0x04
#define DEVICE_CTRL1_REG  0x05
#define DEVICE_TEST0_REG  0x06
#define DEVICE_TEST02_REG 0x08
#define DEVICE_TEST09_REG 0x0f

#define WUSB3801_ID      0x16
#define VBUS_DETECT_MASK BIT(7)
#define CC_DETECT_MASK   GENMASK(1, 0)

int current_max_wusb3801 = 0;
static int cc_attached = 0;
static int cc_orientation = 0;
static struct class *typec_class;
static struct device *port_device;
static struct delayed_work cc_current_work;
static struct i2c_client *wusb3801_client;

static char *current_status[4] = {
    "standby", "default", "1.5A", "3A",
};

static int wusb3801_check_value(
    int reg, int value, int timeout_ms)
{
    int ret, cnt = 0;

    while (cnt < timeout_ms) {
        cnt++;
        msleep(1);
        ret = i2c_smbus_read_byte_data(wusb3801_client, reg);
        if (ret < 0) {
            dev_err(&wusb3801_client->dev,
                "check %#04x[%#04x] failed", reg, value);
            return -EINVAL;
        }
        if (ret == value) {
            dev_err(&wusb3801_client->dev,
                "check %#04x[%#04x] finish", reg, value);
            break;
        }
    }
    if (cnt == timeout_ms) {
        dev_err(&wusb3801_client->dev,
                "check %#04x[%#04x] timeout", reg, value);
        return -EBUSY;
    }

    return 0;
}

static int wusb3801_cc_patch(void)
{
    int ret, cc_state;

    dev_info(&wusb3801_client->dev, "%s", __func__);
    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_TEST02_REG, 0x82);
    if (ret < 0) {
        dev_info(&wusb3801_client->dev, "write 0x08[0x82] failed");
        return -EINVAL;
    }
    msleep(100);
    ret = wusb3801_check_value(DEVICE_TEST02_REG, 0x82, 100);
    if (ret)
        return ret;

    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_TEST09_REG, 0xc0);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "write 0x0f[0xc0] failed");
        return -EINVAL;
    }
    msleep(100);
    ret = wusb3801_check_value(DEVICE_TEST09_REG, 0xc0, 100);
    if (ret)
        return ret;

    ret = i2c_smbus_read_byte_data(wusb3801_client, DEVICE_TEST0_REG);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "read cc status failed");
        return -EINVAL;
    }
    cc_state = !!(ret & BIT(6));
    msleep(10);

    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_TEST09_REG, 0x00);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "write 0x0f[0x00] failed");
        return -EINVAL;
    }
    msleep(10);
    ret = wusb3801_check_value(DEVICE_TEST09_REG, 0x00, 100);
    if (ret)
        return ret;

    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_TEST02_REG, 0x80);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "write 0x08[0x80] failed");
        return -EINVAL;
    }
    msleep(100);
    ret = wusb3801_check_value(DEVICE_TEST02_REG, 0x80, 100);
    if (ret)
        return ret;

    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_TEST02_REG, 0x00);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "write 0x08[0x00] failed");
        return -EINVAL;
    }
    ret = wusb3801_check_value(DEVICE_TEST02_REG, 0x00, 100);
    if (ret)
        return ret;

    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_TEST09_REG, 0x00);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "rewrite 0x0f[0x00] failed");
        return -EINVAL;
    }
    msleep(100);
    ret = wusb3801_check_value(DEVICE_TEST09_REG, 0x00, 100);
    if (ret)
        return ret;

    return cc_state;
}

static void wusb3801_current_work(
    struct work_struct *work)
{
    int cc_current;

    // check cc current
    cc_current = i2c_smbus_read_byte_data(wusb3801_client, DEVICE_STATE_REG);
    if (cc_current < 0) {
       dev_err(&wusb3801_client->dev, "get cc status reg failed");
       return;
    }
    cc_current &= GENMASK(6, 5);
    cc_current >>= 5;
    if (cc_current == 0x2)
        current_max_wusb3801 = 1500;
    else
        current_max_wusb3801 = 3000;
    dev_info(&wusb3801_client->dev,
        "cc current: %s, value: %d",
        current_status[cc_current], current_max_wusb3801);
}

static irqreturn_t wusb3801_irq_handler_thread(
    int irq, void *private)
{
    int ret, attch_state, cc_state;

    dev_info(&wusb3801_client->dev, "wusb3801 irq");
    ret = i2c_smbus_read_byte_data(wusb3801_client, DEVICE_ISR_REG);
    if (ret < 0) {
       dev_err(&wusb3801_client->dev, "read isr reg failed");
       goto out;
    }
    dev_info(&wusb3801_client->dev, "isr status: %#04x", ret);
    ret &= GENMASK(1, 0);
    if (ret == CC_ATTACHED) {
        attch_state = 1;
        dev_info(&wusb3801_client->dev, "attached");
    } else {
        attch_state = 0;
        dev_info(&wusb3801_client->dev, "detached");
    }

    // check cc orientation
    cc_state = i2c_smbus_read_byte_data(wusb3801_client, DEVICE_STATE_REG);
    if (cc_state < 0) {
       dev_err(&wusb3801_client->dev, "read cc status reg failed");
       goto out;
    }
    dev_info(&wusb3801_client->dev, "isr cc status: %#04x", cc_state);
    cc_state &= GENMASK(1, 0);

    if (attch_state == cc_attached) {
        dev_info(&wusb3801_client->dev, "same state [%s]",
            cc_attached ? "attached" : "detached");
        goto out;
    }
    cc_attached = attch_state;
    dev_info(&wusb3801_client->dev, "new state [%s]",
        cc_attached ? "attached" : "detached");

    if (!cc_attached) {
        cc_orientation = 0;
        current_max_wusb3801 = 3000;
        cancel_delayed_work(&cc_current_work);
        goto out;
    }

    if (cc_state != CC_STANDBY) {
        cc_orientation = cc_state;
        dev_info(&wusb3801_client->dev, "cc detected");
    } else {
        dev_info(&wusb3801_client->dev, "cc standby");
        cc_state = wusb3801_cc_patch();
        cc_orientation = cc_state ? 1 : 2;
    }
    schedule_delayed_work(&cc_current_work, 10 * HZ);
    dev_info(&wusb3801_client->dev, "cc_orientation [%d]", cc_orientation);

out:
    return IRQ_HANDLED;
}

static int wusb3801_init_device(void)
{
    int ret, init_attach, init_cc_state, cc_state;

    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_TEST02_REG, 0x00);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "write 0x08 reg failed");
        return -EINVAL;
    }

    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_TEST09_REG, 0x00);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "write 0x0f reg failed");
        return -EINVAL;
    }

    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_CTRL1_REG, 0x01);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "write 0x05 reg failed");
        return -EINVAL;
    }

    // Disable Acc, Try snk, Default current mode, DRP mode, Enable interrupt
    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_CTRL_REG, 0x34);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "init mode failed");
        return -EINVAL;
    }

    ret = i2c_smbus_write_byte_data(wusb3801_client, DEVICE_CTRL1_REG, 0x00);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev, "write 0x05 reg failed");
        return -EINVAL;
    }

    init_attach = i2c_smbus_read_byte_data(wusb3801_client, DEVICE_ISR_REG);
    if (init_attach < 0) {
        dev_err(&wusb3801_client->dev, "get isr reg failed");
        return -EINVAL;
    }
    dev_info(&wusb3801_client->dev, "initial isr status: %#04x", init_attach);
    init_attach &= GENMASK(1, 0);
    if (init_attach == CC_ATTACHED) {
        cc_attached = 1;
        dev_info(&wusb3801_client->dev, "initial attached");
    } else if (init_attach == CC_DETACHED)
        dev_info(&wusb3801_client->dev, "initial detached");
    else if (init_attach == NON_INTERRUPT)
        dev_info(&wusb3801_client->dev, "none interruption");
    else
        dev_info(&wusb3801_client->dev, "unknown status");

    ret = i2c_smbus_read_byte_data(wusb3801_client, DEVICE_STATE_REG);
    if (ret < 0) {
        dev_err(&wusb3801_client->dev,
            "%s: get wusb3801 0x04 reg failed", __func__);
        return -EINVAL;
    }
    dev_info(&wusb3801_client->dev, "initial cc status: %#04x", ret);
    if (!cc_attached) {
        if (ret & VBUS_DETECT_MASK) {
            cc_attached = 1;
            dev_info(&wusb3801_client->dev, "vbus detected");
        }
        if (ret & CC_DETECT_MASK) {
            cc_attached = 1;
            dev_info(&wusb3801_client->dev, "cc detected");
        }  
    }

    if (!cc_attached)
        return 0;

    init_cc_state &= GENMASK(1, 0);
    if (init_cc_state == CC1_ATTACHED) {
        cc_orientation = 1;
        dev_info(&wusb3801_client->dev, "initial cc1 attached");
    } else if (init_cc_state == CC2_ATTACHED) {
        cc_orientation = 2;
        dev_info(&wusb3801_client->dev, "initial cc2 attached");
    } else if (init_cc_state == CC12_ATTACHED) {
        cc_orientation = 3;
        dev_info(&wusb3801_client->dev, "initial cc1&cc2 attached");
    } else {
        dev_info(&wusb3801_client->dev, "initial cc standby");
        cc_state = wusb3801_cc_patch();
        cc_orientation = cc_state ? 1 : 2;
    }
    dev_info(&wusb3801_client->dev,
             "initial cc_orientation: %d, cc_current_max: %d",
             cc_orientation, current_max_wusb3801);

    return 0;
}

static ssize_t wusb3801_orientation_show(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%s\n",
                    !cc_orientation ? "none" : (
                    cc_orientation == 1 ? "cc_1" : "cc_2"));
}

static struct device_attribute dev_attr_orientation =
    __ATTR(typec_cc_polarity_role, S_IRUSR,
    wusb3801_orientation_show, NULL);

static int wusb3801_driver_probe(
    struct i2c_client *client,
    const struct i2c_device_id *id)
{
    int ret;

    wusb3801_client = client;
    ret = i2c_smbus_read_byte_data(wusb3801_client, DEVICE_ID_REG);
    if (ret != WUSB3801_ID) {
       dev_err(&wusb3801_client->dev, "detect wusb3801 failed");
       return -ENODEV;
    }

    INIT_DELAYED_WORK(&cc_current_work, wusb3801_current_work);
    ret = wusb3801_init_device();
    if (ret) {
        dev_err(&client->dev, "init wusb3801 failed");
        return -EPERM;
    }

    ret = devm_request_threaded_irq(&client->dev, client->irq,
                NULL, wusb3801_irq_handler_thread,
                IRQF_TRIGGER_LOW | IRQF_ONESHOT,
                dev_name(&client->dev), NULL);
    if (ret) {
        dev_err(&client->dev, "request interrupt failed");
        return -EBUSY;
    }
    ret = enable_irq_wake(client->irq);
    if (ret) {
        dev_err(&client->dev, "enable irq wake failed");
        goto err_irq_wake;
    }
    
    typec_class = class_create(THIS_MODULE, "cclogic");
    if (IS_ERR(typec_class)) {
        dev_err(&client->dev, "create typec class failed");
        goto err_class_create;
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

    dev_info(&client->dev, "wusb3801 probe finished");

    return 0;

err_create_file:
    device_destroy(typec_class, 0);
err_create_device:
    class_destroy(typec_class);
err_class_create:
    disable_irq_wake(client->irq);
err_irq_wake:
    free_irq(client->irq, NULL);
    return -EBUSY;
}

static int wusb3801_charger_remove(struct i2c_client *client)
{
    device_destroy(typec_class, 0);
    class_destroy(typec_class);
    disable_irq_wake(client->irq);
    free_irq(client->irq, NULL);

    return 0;
}

static const struct i2c_device_id wusb3801_i2c_ids[] = {
    { "wusb3801", 0 }, { },
};

static const struct of_device_id wusb3801_of_match[] = {
    { .compatible = "semi,wusb3801", }, { },
};

static struct i2c_driver wusb3801_driver = {
    .driver = {
        .name = "wusb3801-cclogic",
        .of_match_table = wusb3801_of_match,
    },
    .probe = wusb3801_driver_probe,
    .remove = wusb3801_charger_remove,
    .id_table = wusb3801_i2c_ids,
};

module_i2c_driver(wusb3801_driver);
MODULE_LICENSE("GPL v2");

