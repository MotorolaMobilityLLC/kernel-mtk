/*
 * FPC Capacitive Fingerprint sensor device driver
 *
 * This driver will control the platform resources that the FPC fingerprint
 * sensor needs to operate. The major things are probing the sensor to check
 * that it is actually connected and let the Kernel know this and with that also
 * enabling and disabling of regulators, enabling and disabling of platform
 * clocks.
 * *
 * The driver will expose most of its available functionality in sysfs which
 * enables dynamic control of these features from eg. a user space process.
 *
 * The sensor's IRQ events will be pushed to Kernel's event handling system and
 * are exposed in the drivers event fpc_node. This makes it possible for a user
 * space process to poll the input fpc_node and receive IRQ events easily. Usually
 * this fpc_node is available under /dev/input/eventX where 'X' is a number given by
 * the event system. A user space process will need to traverse all the event
 * nodes and ask for its parent's name (through EVIOCGNAME) which should match
 * the value in device tree named input-device-name.
 *
 *
 * Copyright (c) 2020-2021 Fingerprint Cards AB <tech@fingerprints.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 */
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/pm_wakeup.h>
#include <linux/regulator/consumer.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/ctype.h>

#define MTK
#define FPC_SPI_IOC_SUPPORT 0
#define FPC_MODULE_NAME     "fpc1020"
#define FPC_SPI_BUF_SIZE    (128*1024 - 16)
#define FPC_COMPATIBLE_ID   "fpc,fpc1020"
#define GENERIC_OK          0
#define FPC_ERR             -1
#define FPC_GPIO_COUNT      3
#define SPI_MODE_MASK       (SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
                | SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
                | SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
                | SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)

#define CHECK_STATUS(status) if(status) { pr_err("%s, err line:%d, status=%d\n", __func__, __LINE__, status); return FPC_ERR; }
#define CHECK_NULL(v) if(v == NULL) { pr_err("%s, err line:%d, %s is null\n", __func__, __LINE__, #v); return FPC_ERR; }

#define FPC_IOC_MAGIC       'p'
#define FPC_IOC_WAKEUP      _IO(FPC_IOC_MAGIC, 0)
#define FPC_IOC_CLK_ON      _IO(FPC_IOC_MAGIC, 1)
#define FPC_IOC_CLK_OFF     _IO(FPC_IOC_MAGIC, 2)
#define FPC_IOC_SPI_MESSAGE _IOW(FPC_IOC_MAGIC, 3, char[sizeof(struct spi_ioc_transfer)])
#define FPC_IOC_READ_ID     _IOW(FPC_IOC_MAGIC, 4, uint)
#define FPC_IOC_READ_SYS    _IOW(FPC_IOC_MAGIC, 6, char[128])

static dev_t fpc_devt;
static u8 *fpc_spi_buf;
static s32 fpc_irq_num;
static struct cdev *fpc_c_dev;
static struct class *fpc_class;
static s32 fpc_gpio[FPC_GPIO_COUNT];
static struct device_node *fpc_node;
static struct spi_device *fpc_spi_dev;
static struct regulator *fpc_regulator;
static struct wakeup_source *fpc_wakeup_s;
static struct device *fpc_dev;
static uint fpc_vdd = 30;
static uint fpc_hw_id;

#ifndef FPC_DRIVER_SPI
#define FPC_DRIVER_SPI 1
#endif

#ifdef CONFIG_MICROTRUST_TEE_SUPPORT
#include "teei_fp.h"
#include "tee_client_api.h"
struct TEEC_UUID fpc_uuid = { 0xa3808888, 0xc30c, 0x4dd0, {0xa3, 0x19, 0xea, 0x29, 0x64, 0x3d, 0x4d, 0x4b} };
#endif

void set_tee_worker_threads_on_big_core(bool big_core);

#ifdef MTK
#include <linux/platform_data/spi-mt65xx.h>
extern void mt_spi_disable_master_clk(struct spi_device *spidev);
extern void mt_spi_enable_master_clk(struct spi_device *spidev);
#endif

static irqreturn_t fpc_irq_handler(int irq, void *handle);
static int fpc_dev_destory(void);
static int fpc_dev_create(void);

static void fpc_free(void)
{
    int i = 0;
    int gpio_count = FPC_GPIO_COUNT;
    const char ptl_name[][16] = {"fpc_vdd_of", "fpc_rst_lo"};
    int len = sizeof(ptl_name)/sizeof(ptl_name[0]);
    if (of_property_read_bool(fpc_node, "pinctrl-names")) {
        struct pinctrl *ptl = devm_pinctrl_get(fpc_dev);
        for (i = 0; i < len; i++) {
            struct pinctrl_state *ptl_state = pinctrl_lookup_state(ptl, ptl_name[i]);
            if (!IS_ERR(ptl_state)) {
                pinctrl_select_state(ptl, ptl_state);
                pr_info("%s, by pinctrl:%s\n", __func__, ptl_name[i]);
            }
        }
        devm_pinctrl_put(ptl);
    } else {
        pr_info("%s, no pinctrl\n", __func__);
    }
    if(fpc_irq_num > 0) {
        disable_irq_nosync(fpc_irq_num);
        devm_free_irq(fpc_dev, fpc_irq_num, fpc_dev);
        pr_info("%s, free fpc_irq_num = %d\n", __func__, fpc_irq_num);
        fpc_irq_num = 0;
    }
    while(gpio_count) {
        if(gpio_is_valid(fpc_gpio[gpio_count - 1])) {
            gpio_set_value(fpc_gpio[gpio_count - 1], 0);
            devm_gpio_free(fpc_dev, fpc_gpio[gpio_count - 1]);
            pr_info("%s, free gpio = %d\n", __func__, fpc_gpio[gpio_count - 1]);
        }
        gpio_count--;
    }
    if (of_property_read_bool(fpc_node, "vfp-supply")) {
        regulator_disable(fpc_regulator);
        devm_regulator_put(fpc_regulator);
        fpc_regulator = NULL;
    }
    if (fpc_wakeup_s) wakeup_source_unregister(fpc_wakeup_s);
    fpc_wakeup_s = NULL;
    // wakeup_source_trash(fpc_wakeup_s);
}

static int fpc_spi_sync(u8 *tx, u8 *rx, u32 len, u32 speed)
{
    struct spi_transfer t = {
        .tx_buf = tx,
        .rx_buf = rx,
        .len = len,
        .speed_hz = speed,
    };
    int status = 0;
    struct spi_message m;
    CHECK_NULL(fpc_spi_dev)
    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    status = spi_sync(fpc_spi_dev, &m);
    if(status) pr_err("%s err status = %d", __func__, status);
    return status;
}

static void fpc_id_read(void)
{
    int status = 0;
    u8 buf[3] = {0xfc, 0, 0};
    status = fpc_spi_sync(buf, buf, 3, 1000000);
    pr_info("%s, id = %2x %2x %2x\n", __func__, buf[0], buf[1], buf[2]);
    if (buf[1] >= 0x10 && buf[1] != 0xff) {
        fpc_hw_id = (buf[1] << 8) + buf[2];
        #ifdef CONFIG_MICROTRUST_TEE_SUPPORT
        if(fpc_hw_id > 0x1000) memcpy(&uuid_fp, &fpc_uuid, sizeof(struct TEEC_UUID));
        #endif
    }
}

static void fpc_optical_id_read(void)
{
    #define READ_OP(x) (((x) << 1) | 0x01)
    u8 REG_HWID_H = 0x00;
    u8 REG_HWID_L = 0x01;
    u8 tx1[] = { READ_OP(REG_HWID_H), 0 };
    u8 tx2[] = { READ_OP(REG_HWID_L), 0 };
    fpc_spi_sync(tx1, tx1, 2, 1000000);
    fpc_spi_sync(tx2, tx2, 2, 1000000);
    pr_info("%s, id = %2x %2x\n", __func__, tx1[1], tx2[1]);
    if(tx1[1] > 0x10 && tx1[1] != 0xff) {
        fpc_hw_id = (tx1[1] << 8) | tx2[1];
    }
}

static void fpc_gpio_set(int vdd, int rst, int irq)
{
    int status = 0;
    int irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT;

    gpio_direction_output(vdd, 0);
    gpio_direction_output(rst, 0);
    msleep(3);
    pr_info("%s, begin vdd %d rst %d\n", __func__, vdd, rst);
    gpio_set_value(vdd, 1);
    gpio_set_value(rst, 1);
    pr_info("%s end vdd %d rst %d\n", __func__, vdd, rst);

    msleep(10);
    gpio_set_value(rst, 0);
    msleep(3);
    gpio_set_value(rst, 1);
    msleep(3);
    pr_info("%s, rst %d\n", __func__, rst);

    if (fpc_irq_num <= 0 && gpio_is_valid(irq)) {
        fpc_irq_num = gpio_to_irq(irq); //fpc_irq_num = irq_of_parse_and_map(fpc_node, 0);
        status = devm_request_threaded_irq(fpc_dev, fpc_irq_num, NULL, fpc_irq_handler, irqf, dev_name(fpc_dev), fpc_dev);
        // fpc_wakeup_s = wakeup_source_register(dev_name(fpc_dev));
        fpc_wakeup_s = wakeup_source_register(fpc_dev, dev_name(fpc_dev));
        enable_irq_wake(fpc_irq_num);
    }
    pr_info("%s, fpc_irq_num = %d, value = %d\n", __func__, fpc_irq_num, gpio_get_value(irq));
}

static void fpc_gpio_request(void)
{
    int i = 0;
    const char gpio_name[][16] = {"fpc_vdd", "fpc_rst", "fpc_irq"};
    int len = sizeof(gpio_name)/sizeof(gpio_name[0]);
    if (fpc_irq_num <= 0) {
        for (i = 0; i < len; i++) {
            fpc_gpio[i] = of_get_named_gpio(fpc_node, gpio_name[i], 0);
            if(gpio_is_valid(fpc_gpio[i]) && devm_gpio_request(fpc_dev, fpc_gpio[i], FPC_MODULE_NAME) == 0) {
                pr_info("%s, ok %s = %d\n", __func__, gpio_name[i], fpc_gpio[i]);
            } else {
                pr_info("%s, no %s %d\n", __func__, gpio_name[i], fpc_gpio[i]);
            }
        }
    }
    fpc_gpio_set(fpc_gpio[0], fpc_gpio[1], fpc_gpio[2]);
}

static void fpc_pinctrl(void)
{
    if (of_property_read_bool(fpc_node, "pinctrl-names")) {
        struct pinctrl *ptl = devm_pinctrl_get(fpc_dev);
        struct pinctrl_state *ptl_state = pinctrl_lookup_state(ptl, "fpc_irq_en");
        pr_info("fpc fpc_pinctrl_on begin\n");
        pinctrl_select_state(ptl, ptl_state);
        ptl_state = pinctrl_lookup_state(ptl, "fpc_vdd_of");
        pinctrl_select_state(ptl, ptl_state);
        ptl_state = pinctrl_lookup_state(ptl, "fpc_rst_lo");
        pinctrl_select_state(ptl, ptl_state);
        msleep(3);
        ptl_state = pinctrl_lookup_state(ptl, "fpc_vdd_on");
        pinctrl_select_state(ptl, ptl_state);
        ptl_state = pinctrl_lookup_state(ptl, "fpc_rst_hi");
        pinctrl_select_state(ptl, ptl_state);
        msleep(10);
        ptl_state = pinctrl_lookup_state(ptl, "fpc_rst_lo");
        pinctrl_select_state(ptl, ptl_state);
        msleep(3);
        ptl_state = pinctrl_lookup_state(ptl, "fpc_rst_hi");
        pinctrl_select_state(ptl, ptl_state);
        msleep(3);
        devm_pinctrl_put(ptl);
        pr_info("fpc fpc_pinctrl_on end\n");
    }
}

static void fpc_power_regulator(void)
{
    int status = 0;
    if (of_property_read_bool(fpc_node, "vfp-supply")) {
        fpc_regulator = devm_regulator_get_optional(fpc_dev, "vfp");
        if (IS_ERR(fpc_regulator)) {
            pr_err("err %s +%d\n", __func__, __LINE__);
            return;
        }
        if (regulator_is_enabled(fpc_regulator)) {
            regulator_disable(fpc_regulator);
            msleep(3);
        }
        status = regulator_set_voltage(fpc_regulator, fpc_vdd*100000, fpc_vdd*100000);
        if(status) {
            pr_err("err %s +%d, status = %d\n", __func__, __LINE__, status);
            return;           
        }
        regulator_enable(fpc_regulator);
        pr_info("%s, fpc_vdd = %d\n", __func__, fpc_vdd);
    } else {
        pr_info("%s, no\n", __func__);
    }
}

static ssize_t irq_get(struct device *dev, struct device_attribute *attr, char *buf)
{
    (void)dev;(void)attr;
    return scnprintf(buf, PAGE_SIZE, "%i\n", gpio_get_value(fpc_gpio[2]));
}

static ssize_t irq_ack(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    (void)dev;(void)attr;(void)buf;
    return count;
}
static DEVICE_ATTR(irq, S_IRUSR | S_IWUSR, irq_get, irq_ack);
static struct attribute *fpc_attributes[] = {
    &dev_attr_irq.attr,
    NULL
};

static struct attribute_group fpc_attribute_group = {
    .attrs = fpc_attributes,
};

static irqreturn_t fpc_irq_handler(int irq, void *handle)
{
    struct device *dev = (struct device *)handle;
    sysfs_notify(&dev->kobj, NULL, dev_attr_irq.attr.name);
    return IRQ_HANDLED;
}

static int fpc_spi_probe(struct spi_device *spi_dev)
{
    pr_info("%s\n", __func__);
    fpc_spi_dev = spi_dev;
    return 0;
}

static int fpc_plat_probe(struct platform_device *device)
{
    pr_info("%s\n", __func__);
    fpc_dev = &(device->dev);
    fpc_node = of_find_compatible_node(NULL, NULL, FPC_COMPATIBLE_ID);
    return fpc_dev_create();
}

static int fpc_spi_remove(struct spi_device *dev)
{
    (void)dev;
    return 0;
}

static int fpc_plat_remove(struct platform_device *dev)
{
    (void)dev;
    return fpc_dev_destory();
}

static struct of_device_id fpc_of_match[] = {
    { .compatible = FPC_COMPATIBLE_ID, },
    { .compatible = "fpc,fpc_spi", },
    { .compatible = "mediatek,fingerprint", },
    {}
};

MODULE_DEVICE_TABLE(of, fpc_of_match);

static struct spi_driver fpc_spi_driver = {
    .driver = {
        .name = FPC_MODULE_NAME,
        .owner = THIS_MODULE,
        .bus = &spi_bus_type,
        .of_match_table = fpc_of_match,
    },
    .probe = fpc_spi_probe,
    .remove = fpc_spi_remove,
};

static struct platform_driver fpc_plat_driver = {
    .driver = {
        .name = FPC_MODULE_NAME,
        .owner = THIS_MODULE,
        .of_match_table = fpc_of_match,
    },
    .probe = fpc_plat_probe,
    .remove = fpc_plat_remove,
};

static int __init fpc_init(void)
{
    return platform_driver_register(&fpc_plat_driver);
}

static void __exit fpc_exit(void)
{
    platform_driver_unregister(&fpc_plat_driver);
}

static int fpc_ioctl_spi(unsigned long arg)
{
    int status = 0;
    struct spi_ioc_transfer spi_ioc;
    if (FPC_SPI_IOC_SUPPORT == 0 || fpc_spi_dev == NULL) {
        pr_err("%s, err\n", __func__);
        return -1;
    }
    status = copy_from_user(&spi_ioc, (struct spi_ioc_transfer __user *)arg, sizeof(struct spi_ioc_transfer));
    CHECK_STATUS(status)

    if(fpc_spi_buf == NULL) {
        fpc_spi_buf = kmalloc(FPC_SPI_BUF_SIZE, GFP_KERNEL);
        CHECK_NULL(fpc_spi_buf)
    }
    if(spi_ioc.len > FPC_SPI_BUF_SIZE) {
        pr_info("%s, err spi_ioc.len = %d\n", __func__, spi_ioc.len);
        return -1;
    }
    status = copy_from_user(fpc_spi_buf, (const u8 __user *)(uintptr_t)spi_ioc.tx_buf, spi_ioc.len);
    CHECK_STATUS(status)

    status = fpc_spi_sync(fpc_spi_buf, fpc_spi_buf, spi_ioc.len, spi_ioc.speed_hz);
    CHECK_STATUS(status)

    if(spi_ioc.rx_buf) {
        status = copy_to_user((u8 __user *)(uintptr_t)spi_ioc.rx_buf, fpc_spi_buf, spi_ioc.len);
        CHECK_STATUS(status)
    }
    return GENERIC_OK;
}

static long fpc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    char *fpc_sys_path = NULL;
    if(_IOC_TYPE(cmd) != FPC_IOC_MAGIC) {
        pr_err("%s, err line:%d\n", __func__ ,__LINE__);
        return -ENOTTY;
    }
    switch(cmd) {
        case FPC_IOC_WAKEUP:
            __pm_wakeup_event(fpc_wakeup_s, 3000);
            set_tee_worker_threads_on_big_core(true);
            pr_info("set_tee_worker_threads_on_big_core.\n");
            break;
        case FPC_IOC_READ_SYS:
            fpc_sys_path = kobject_get_path(&fpc_dev->kobj, GFP_KERNEL);
            if(!fpc_sys_path) {
                pr_err("%s +%d", __func__, __LINE__);
                return -ENOTTY;
            }
            copy_to_user((char __user *)(uintptr_t)arg, fpc_sys_path, strlen(fpc_sys_path));
            kfree(fpc_sys_path);
            break;
        case FPC_IOC_SPI_MESSAGE:
            return fpc_ioctl_spi(arg);
    #ifdef MTK
        case FPC_IOC_CLK_ON:
            CHECK_NULL(fpc_spi_dev);
            mt_spi_enable_master_clk(fpc_spi_dev);
            break;
        case FPC_IOC_CLK_OFF:
            CHECK_NULL(fpc_spi_dev);
            mt_spi_disable_master_clk(fpc_spi_dev);
            break;
    #endif
        default:
            pr_err("%s, err line:%d\n", __func__ ,__LINE__);
            return -ENOTTY;
    }
    return GENERIC_OK;
}

static ssize_t fpc_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int status = 0;
    char cmd[8] = {0};
    if(count > 8) {
        pr_err("err %s +%d\n", __func__, __LINE__);
        return FPC_ERR;
    }
    status = copy_from_user(cmd, buf, count);
    CHECK_STATUS(status)

    if (strncmp(cmd, "id", 2) == 0) {
        fpc_id_read();
        pr_err("%s:fpc id read\n", __func__);
    } else if(strncmp(cmd, "oid", 3) == 0) {
        fpc_optical_id_read();
        pr_err("%s:fpc optical id read\n", __func__);
    } else if(strncmp(cmd, "init", 4) == 0) {
        fpc_power_regulator();
        pr_err("%s:fpc power regulator\n", __func__);
        fpc_pinctrl();
        pr_err("%s:fpc pinctrl\n", __func__);
        fpc_gpio_request();
        pr_err("%s:fpc gpio request\n", __func__);
    } else if(strncmp(cmd, "spi", 3) == 0) {
        spi_register_driver(&fpc_spi_driver);
        pr_err("%s:fpc spi register driver\n", __func__);
    } else if(strncmp(cmd, "uspi", 4) == 0) {
        spi_unregister_driver(&fpc_spi_driver);
        pr_err("%s:fpc uspi register driver\n", __func__);
    } else if(strncmp(cmd, "free", 4) == 0) {
        fpc_free();
        pr_err("%s:fpc free\n", __func__);
    } else if(isdigit(cmd[0]) && isdigit(cmd[1]) && (2 == strlen(cmd))){
        kstrtouint(cmd, 10, &fpc_vdd);
        fpc_power_regulator();
        fpc_pinctrl();
        fpc_gpio_request();
        pr_err("%s:fpc cmd 10\n", __func__);
    } else {
        pr_err("%s id oid init spi uspi free 33\n", __func__);
        return FPC_ERR;
    }
    return count;
}

static const struct file_operations fpc1020_ops = {
    .owner = THIS_MODULE,
    .write = fpc_write,
    .unlocked_ioctl = fpc_ioctl,
};

static int fpc_dev_create(void)
{
    struct device *c_device;
    int status = sysfs_create_group(&fpc_dev->kobj, &fpc_attribute_group);
    CHECK_STATUS(status)
    status = alloc_chrdev_region(&fpc_devt, 0, 1, FPC_MODULE_NAME);
    CHECK_STATUS(status)
    fpc_class = class_create(THIS_MODULE, FPC_MODULE_NAME);
    if (IS_ERR(fpc_class)) {
        pr_err("%s err class_create\n", __func__);
        return FPC_ERR;
    }
    c_device = device_create(fpc_class, fpc_dev, fpc_devt, NULL, FPC_MODULE_NAME);
    if (IS_ERR(c_device)) {
        pr_err("%s err device_create\n", __func__);
        return FPC_ERR;
    }
    fpc_c_dev = cdev_alloc();
    if (IS_ERR(fpc_c_dev)) {
        pr_err("%s err cdev_alloc\n", __func__);
        return FPC_ERR;
    }
    cdev_init(fpc_c_dev, &fpc1020_ops);
    status = cdev_add(fpc_c_dev, MKDEV(MAJOR(fpc_devt), 0), 1);
    CHECK_STATUS(status)
    return GENERIC_OK;
}

static int fpc_dev_destory(void)
{
    pr_info("%s\n", __func__);
    fpc_free();
    device_destroy(fpc_class, fpc_devt);
    class_destroy(fpc_class);
    cdev_del(fpc_c_dev);
    unregister_chrdev_region(fpc_devt, 1);
    if(fpc_spi_buf) kfree(fpc_spi_buf);
    sysfs_remove_group(&fpc_dev->kobj, &fpc_attribute_group);
    return GENERIC_OK;
}

late_initcall(fpc_init);
module_exit(fpc_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("sheldon <sheldon.xie@fingerprints.com>");
