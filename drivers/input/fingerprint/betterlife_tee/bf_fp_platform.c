#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/irqreturn.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/signal.h>
#include <linux/ctype.h>
#include <linux/kobject.h>
#include <linux/poll.h>
#include <net/sock.h>
#include <linux/delay.h>

#include <linux/kernel.h>

#ifndef MTK_ANDROID_L
#include <linux/interrupt.h>
#include <linux/gpio.h>
#else
#include <mach/eint.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#endif

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#include "bf_fp_platform.h"
static int bf_create_inputdev(void);

#define BF_IOCTL_MAGIC_NO                   0xFC
#define BF_IOCTL_INIT_ARGS                  _IOWR(BF_IOCTL_MAGIC_NO, 0,uint32_t)
#define BF_IOCTL_REGISTER_READ_WRITE        _IOWR(BF_IOCTL_MAGIC_NO,  1, uint32_t)
#define BF_IOCTL_RESET                      _IO(BF_IOCTL_MAGIC_NO,  2)
#define BF_IOCTL_DISABLE_INTERRUPT          _IO(BF_IOCTL_MAGIC_NO,  3)
#define BF_IOCTL_GAIN_ADJUST                _IOWR(BF_IOCTL_MAGIC_NO, 4,uint32_t)
#define BF_IOCTL_ENABLE_POWER               _IO(BF_IOCTL_MAGIC_NO, 5)
#define BF_IOCTL_DISABLE_POWER              _IO(BF_IOCTL_MAGIC_NO, 6)
#define BF_IOCTL_ENABLE_SPI_CLOCK           _IOW(BF_IOCTL_MAGIC_NO,  7,uint32_t)
#define BF_IOCTL_DISABLE_SPI_CLOCK          _IOW(BF_IOCTL_MAGIC_NO,  8,uint32_t)
#define BF_IOCTL_GET_ID                     _IOWR(BF_IOCTL_MAGIC_NO, 9, uint32_t)
#define BF_IOCTL_INIT_DEVICE                _IOW(BF_IOCTL_MAGIC_NO,  10,uint32_t)
#define BF_IOCTL_REMOVE_DEVICE              _IOW(BF_IOCTL_MAGIC_NO,  11,uint32_t)
#define BF_IOCTL_INPUT_KEY                  _IOW(BF_IOCTL_MAGIC_NO,  12,uint32_t)
#define BF_IOCTL_ENBACKLIGHT                _IOW(BF_IOCTL_MAGIC_NO,  13,uint32_t)
#define BF_IOCTL_ISBACKLIGHT                _IOWR(BF_IOCTL_MAGIC_NO, 14,uint32_t)
#define BF_IOCTL_DISPALY_STATUS             _IOW(BF_IOCTL_MAGIC_NO,  15,uint32_t)
#define BF_IOCTL_SET_PID                    _IOW(BF_IOCTL_MAGIC_NO,  16,uint32_t)
#define BF_IOCTL_INPUT_KEY_DOWN             _IOW(BF_IOCTL_MAGIC_NO,  17,uint32_t)
#define BF_IOCTL_INPUT_KEY_UP               _IOW(BF_IOCTL_MAGIC_NO,  18,uint32_t)
#define BF_IOCTL_LOW_RESET                  _IO(BF_IOCTL_MAGIC_NO,  19)
#define BF_IOCTL_HIGH_RESET                 _IO(BF_IOCTL_MAGIC_NO,  20)
#define BF_IOCTL_NETLINK_INIT               _IOW(BF_IOCTL_MAGIC_NO,  21,uint32_t)
#define BF_IOCTL_TRANS_IC_INFO              _IOW(BF_IOCTL_MAGIC_NO,  22,uint32_t)
#define BF_IOCTL_ENABLE_INTERRUPT           _IO(BF_IOCTL_MAGIC_NO,  23)
#define BF_IOCTL_RESET_FLAG                 _IOW(BF_IOCTL_MAGIC_NO,  24,uint32_t)
#define BF_IOCTL_IS_OPT_POWER_ON2V8       	_IOWR(BF_IOCTL_MAGIC_NO,  25, uint32_t)
#define BF_IOCTL_CREATE_INPUT               _IO(BF_IOCTL_MAGIC_NO,  26)
#define BF_IOCTL_COMPATIBLE_IN_HAL          _IOWR(BF_IOCTL_MAGIC_NO,  27, uint32_t)

//begin, add fpvendor for betterlife by song.li
#include "ontim/ontim_dev_dgb.h"
#define FPC_HW_INFO "BF3690"
DEV_ATTR_DECLARE(fingersensor)
DEV_ATTR_DEFINE("vendor", FPC_HW_INFO)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(fingersensor, fingersensor, 8);
//end

typedef enum bf_key {
    BF_KEY_NONE = 0,
    BF_KEY_POWER,
    BF_KEY_CAMERA,
    BF_KEY_UP,
    BF_KEY_DOWN,
    BF_KEY_RIGHT,
    BF_KEY_LEFT,
    BF_KEY_HOME,
    BF_KEY_F10,
    BF_KEY_F11
} bf_key_t;

/* for netlink use */
static int g_pid;
static int g_netlink_port = NETLINK_BF;
struct bf_device *g_bf_dev = NULL;
static struct input_dev *bf_inputdev = NULL;
bl_ic_info_t ic_info;
static uint32_t bf_key_need_report = 0;
static void bf_unregister(void);

#ifdef KERNEL_4_9
static struct wakeup_source fp_suspend_lock;
static struct wakeup_source hw_reset_lock;
#else
static struct wake_lock fp_suspend_lock;
static struct wake_lock hw_reset_lock;
#endif
static struct kobject *bf_kobj = NULL;
static DEFINE_MUTEX(irq_count_lock);
static DEFINE_MUTEX(g_dev_lock);
static irqreturn_t bf_eint_handler (int irq, void *data);
int g_bl229x_enbacklight = 1;

extern u32 g_chip_type;

#ifdef MTK_ANDROID_L
#include <cust_eint.h>
#include <cust_gpio_usage.h>
#include <mach/mt_gpio.h>
/*
#define GPIO_FP_INT_PIN             (GPIO2 | 0x80000000)//GPIO_FINGER_PRINT_EINT//(GPIO60 | 0x80000000)
#define CUST_EINT_FP_INT_NUM        2//CUST_EINT_FINGER_NUM
#define GPIO_FP_INT_PIN_M_EINT      GPIO_MODE_00
#define GPIO_FINGER_PRINT_RESET     (GPIO3 | 0x80000000)
*/
#define GPIO_FP_INT_PIN             (GPIO18 )//GPIO_FINGER_PRINT_EINT//(GPIO60 | 0x80000000)
#define CUST_EINT_FP_INT_NUM        2//CUST_EINT_FINGER_NUM
#define GPIO_FP_INT_PIN_M_EINT      GPIO_MODE_06
#define GPIO_FINGER_PRINT_RESET     (GPIO19)

extern void mt_eint_ack(unsigned int eint_num);
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

static void disable_irq_nosync(unsigned int irq)
{
    mt_eint_mask(irq);
}
static void enable_irq(unsigned int irq)
{
    mt_eint_unmask(irq);
}
static void enable_irq_wake(unsigned int irq)
{
}
static inline bool gpio_is_valid(int number)
{
    return number >= 0;
}

static inline int gpio_get_value(unsigned int gpio)
{
    return mt_get_gpio_in(gpio | 0x80000000);
}

static inline int gpio_direction_input(unsigned gpio)
{
    return mt_set_gpio_dir((gpio | 0x80000000), GPIO_DIR_IN);
}

//mt_set_gpio_out(GPIO_FINGER_POWER_3V3_EN_PIN, GPIO_OUT_ONE);
static inline int gpio_direction_output(unsigned gpio, int value)
{
    mt_set_gpio_dir((gpio | 0x80000000), GPIO_DIR_OUT);
    return mt_set_gpio_out((gpio | 0x80000000), value);
}

static inline int gpio_request(unsigned gpio, const char *label)
{
    if (gpio == g_bf_dev->irq_gpio) //(strcmp(label, "bf irq_gpio")==0)
        return mt_set_gpio_mode((gpio | 0x80000000), GPIO_FP_INT_PIN_M_EINT);
    else
        return mt_set_gpio_mode((gpio | 0x80000000), GPIO_MODE_00);
}

static inline int gpio_to_irq(unsigned int gpio)
{
    return gpio;
}

//中断处理函数
static void bf_eint_handler_l(void)
{
    disable_irq_nosync(g_bf_dev->irq_num);
    BF_LOG("\n");
    queue_work(g_bf_dev->fingerprint_workqueue, &(g_bf_dev->fingerprint_work));
}

void work_func(struct work_struct *p_work)
{
    BF_LOG("  ++\n");
    bf_eint_handler(g_bf_dev->irq_num, g_bf_dev);
    enable_irq(g_bf_dev->irq_num);
    BF_LOG("  --\n");
}


#endif
DECLARE_WAIT_QUEUE_HEAD (waiting_spi_prepare);
atomic_t suspended;

#if defined(CONFIG_FB)
/*----------------------------------------------------------------------------*/
static int fb_notifier_callback(struct notifier_block *self,
                                unsigned long event, void *data)
{
    struct fb_event *evdata = data;
    int *blank =  evdata->data;
    mutex_lock(&g_dev_lock);
    if (g_bf_dev && evdata && evdata->data) {
        if (event == FB_EVENT_BLANK ) {
            if (*blank == FB_BLANK_UNBLANK) {
                BF_LOG("fb_notifier_callback FB_BLANK_UNBLANK:%d", g_bf_dev->need_report);
                g_bf_dev->need_report = 0;
                bf_send_netlink_msg(g_bf_dev, BF_NETLINK_CMD_SCREEN_ON);
            } else if (*blank == FB_BLANK_POWERDOWN) {
                BF_LOG("fb_notifier_callback FB_BLANK_POWERDOWN:%d", g_bf_dev->need_report);
                g_bf_dev->need_report = 1;
                bf_send_netlink_msg(g_bf_dev, BF_NETLINK_CMD_SCREEN_OFF);
            }
        }
    }
    mutex_unlock(&g_dev_lock);
    return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void early_suspend(struct bf_device *bf_dev)
{
    BF_LOG("++++++++++\n");
    mutex_lock(&g_dev_lock);
    if (g_bf_dev) {
        g_bf_dev->need_report = 1;
        bf_send_netlink_msg(g_bf_dev, BF_NETLINK_CMD_SCREEN_OFF);
    }
    mutex_unlock(&g_dev_lock);
    BF_LOG("----------\n");
}
static void early_resume(struct bf_device *bf_dev)
{
    BF_LOG("+++++++++++\n");
    mutex_lock(&g_dev_lock);
    if (g_bf_dev) {
        g_bf_dev->need_report = 0;
        bf_send_netlink_msg(g_bf_dev, BF_NETLINK_CMD_SCREEN_ON);
    }
    mutex_unlock(&g_dev_lock);
    BF_LOG("----------\n");
}
#endif

static int bf_hw_power(struct bf_device *bf_dev, bool enable)
{
#if defined BF_PINCTL //BF_PINCTL
    if(!bf_dev->pinctrl_gpios) {
        BF_LOG("bf_hw_power BF_PINCTL failed:%d", enable);
        return -1;
    }

    if (enable) {
#ifdef NEED_OPT_POWER_ON2V8
        pinctrl_select_state (bf_dev->pinctrl_gpios, bf_dev->pins_power_2v8_high);
#endif
#ifdef NEED_OPT_POWER_ON1V8
        pinctrl_select_state (bf_dev->pinctrl_gpios, bf_dev->pins_power_1v8_high);
#endif
    } else {
#ifdef NEED_OPT_POWER_ON2V8
        pinctrl_select_state (bf_dev->pinctrl_gpios, bf_dev->pins_power_2v8_low);
#endif
#ifdef NEED_OPT_POWER_ON1V8
        pinctrl_select_state (bf_dev->pinctrl_gpios, bf_dev->pins_power_1v8_low);
#endif
    }
#else    //BF_GPIO
#ifdef NEED_OPT_POWER_ON2V8
    if(bf_dev->power_2v8_gpio < 0) {
        BF_LOG("bf_hw_power 2.8v BF_GPIO failed:%d", enable);
        return -1;
    }
#endif
#ifdef NEED_OPT_POWER_ON1V8
    if(bf_dev->power_1v8_gpio < 0) {
        BF_LOG("bf_hw_power 1.8v BF_GPIO failed:%d", enable);
        return -1;
    }
#endif
    if (enable) {
#ifdef NEED_OPT_POWER_ON2V8
        gpio_direction_output (bf_dev->power_2v8_gpio, 1);
#endif
#ifdef NEED_OPT_POWER_ON1V8
        gpio_direction_output (bf_dev->power_1v8_gpio, 1);
#endif
    } else {
#ifdef NEED_OPT_POWER_ON2V8
        gpio_direction_output (bf_dev->power_2v8_gpio, 0);
#endif
#ifdef NEED_OPT_POWER_ON1V8
        gpio_direction_output (bf_dev->power_1v8_gpio, 0);
#endif
    }
#endif    //BF_PINCTL
    return 0;
}

static int bf_hw_reset(struct bf_device *bf_dev)
{
#if defined BF_PINCTL
    pinctrl_select_state (bf_dev->pinctrl_gpios, bf_dev->pins_reset_low);
    mdelay(5);
    pinctrl_select_state (bf_dev->pinctrl_gpios, bf_dev->pins_reset_high);
#else
    gpio_direction_output (bf_dev->reset_gpio, 0);
    mdelay(5);
    gpio_direction_output (bf_dev->reset_gpio, 1);
#endif
    return 0;
}

static int bf_hw_reset_level (struct bf_device *bf_dev, bool enable)
{
    BF_LOG("bf_hw_reset_level %d", enable);
#if defined BF_PINCTL
    if (enable) {
        pinctrl_select_state (bf_dev->pinctrl_gpios, bf_dev->pins_reset_high);
    } else {
        pinctrl_select_state (bf_dev->pinctrl_gpios, bf_dev->pins_reset_low);
    }
#else
    if (enable) {
        gpio_direction_output(bf_dev->reset_gpio, 1);
    } else {
        gpio_direction_output(bf_dev->reset_gpio, 0);
    }
#endif
    return 0;
}

static ssize_t bf_show_hwreset(struct device *ddri, struct device_attribute *attr, char *buf)
{
    u32 pin_val = -1;
    pin_val = gpio_get_value(g_bf_dev->reset_gpio);
    BF_LOG("reset pin_val=%d\n", pin_val);

    return sprintf(buf, "reset pin_val=%d\n", pin_val);
}
static ssize_t bf_store_hwreset(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    char *next;
    u8 hwreset_flag  = simple_strtoul(buf, &next, 10);
    BF_LOG("hwreset_flag %d\n", hwreset_flag);
    if(hwreset_flag) {
        bf_hw_reset_level(g_bf_dev, 1);
    } else {
        bf_hw_reset_level(g_bf_dev, 0);
    }

    return size;
}
static DEVICE_ATTR(reset, 0664, bf_show_hwreset, bf_store_hwreset);


#if defined(NEED_OPT_POWER_ON2V8) || defined(NEED_OPT_POWER_ON1V8)

static ssize_t bf_show_powermode(struct device *ddri, struct device_attribute *attr, char *buf)
{
    ssize_t length = 0;

#ifdef NEED_OPT_POWER_ON2V8
    u32 pin_val_2v8 = -1;

    pin_val_2v8 = gpio_get_value(g_bf_dev->power_2v8_gpio);
    BF_LOG("power pin_val_2v8=%d\n", pin_val_2v8);
    length = sprintf(buf, "power pin_val_2v8=%d\n", pin_val_2v8);
#endif
#ifdef NEED_OPT_POWER_ON1V8
    u32 pin_val_1v8 = -1;

    pin_val_1v8 = gpio_get_value(g_bf_dev->power_1v8_gpio);
    BF_LOG("power pin_val_1v8=%d\n", pin_val_1v8);
    length = length + sprintf(buf + length, "power pin_val_1v8=%d\n", pin_val_1v8);
#endif

    return length;
}
static ssize_t bf_store_powermode(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    char *next;
    u8 power_flag  = simple_strtoul(buf, &next, 10);
    BF_LOG("power_flag %d\n", power_flag);
    if(power_flag) {
        bf_hw_power(g_bf_dev, 1);
    } else {
        bf_hw_power(g_bf_dev, 0);
    }

    return size;
}
static DEVICE_ATTR(power, 0664, bf_show_powermode, bf_store_powermode);
#endif

static ssize_t bf_show_chipid(struct device *ddri, struct device_attribute *attr, char *buf)
{
#ifdef BF_REE
    bf_chip_info();
    ic_info.ic_chipid = g_chip_type;
#endif

    return sprintf(buf, "%x", ic_info.ic_chipid);
}
static ssize_t bf_store_chipid(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    char *next;
    BF_LOG("bf_store_senorchipid ");

    return 0;
}
static DEVICE_ATTR(chipid, 0664, bf_show_chipid, bf_store_chipid);

static ssize_t bf_show_version(struct device *ddri, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s", ic_info.ca_ver);
}
static ssize_t bf_store_version(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    char *next;
    BF_LOG("bf_store_senorversion");

    return 0;
}
static DEVICE_ATTR(version, 0664, bf_show_version, bf_store_version);

/*----------------------------------------------------------------------------*/
static struct device_attribute *bf_attr_list[] = {
    &dev_attr_reset,
    &dev_attr_chipid,
    &dev_attr_version,
#if defined(NEED_OPT_POWER_ON2V8) || defined(NEED_OPT_POWER_ON1V8)
    &dev_attr_power
#endif
};
/*----------------------------------------------------------------------------*/
/*
static void bf_create_attributes(struct device *dev)
{
    int num = (int)(sizeof(bf_attr_list) / sizeof(bf_attr_list[0]));
    for (; num > 0;)
        device_create_file(dev, bf_attr_list[--num]);
}
*/
static int bf_sysfs_init(void)
{
    int ret = 0;
    bf_kobj = kobject_create_and_add("bf_sysfs", NULL);
    if(bf_kobj == NULL) {
        BF_LOG("subsystem_register failed\n");
        ret = -ENOMEM;
        return ret;
    }

    ret = sysfs_create_file(bf_kobj, &dev_attr_reset.attr);
    ret = sysfs_create_file(bf_kobj, &dev_attr_chipid.attr);
    ret = sysfs_create_file(bf_kobj, &dev_attr_version.attr);
#if defined(NEED_OPT_POWER_ON2V8) || defined(NEED_OPT_POWER_ON1V8)
    ret = sysfs_create_file(bf_kobj, &dev_attr_power.attr);
#endif
    if(ret) {
        BF_LOG("sysfs_create_file failed\n");
    }

    kobject_uevent(bf_kobj, KOBJ_ADD);
    return ret;
}

static int bf_sysfs_uninit(void)
{
    int ret;

    if(bf_kobj == NULL) {
        BF_LOG("bf_kobj don't exist \n");
        ret = -EEXIST;
        return ret;
    }

    sysfs_remove_file(bf_kobj, &dev_attr_reset.attr);
    sysfs_remove_file(bf_kobj, &dev_attr_chipid.attr);
    sysfs_remove_file(bf_kobj, &dev_attr_version.attr);
#if defined(NEED_OPT_POWER_ON2V8) || defined(NEED_OPT_POWER_ON1V8)
    sysfs_remove_file(bf_kobj, &dev_attr_power.attr);
#endif
    kobject_del(bf_kobj);
    return ret;
}
/*
static void bf_remove_attributes(struct device *dev)
{
    int num = (int)(sizeof(bf_attr_list) / sizeof(bf_attr_list[0]));
    for (; num > 0;)
        device_remove_file(dev, bf_attr_list[--num]);
}
*/
/**
 * get gpio information from device tree
 */
static int bf_main_get_gpio_info (struct bf_device *bf_dev)
{
    struct device_node *node = NULL;
#ifdef BF_PINCTL
    int32_t ret = 0;
#endif

#ifndef MTK_ANDROID_L

    node = bf_dev->pdev->dev.of_node;
    if (node) {
        bf_dev->reset_gpio = of_get_named_gpio(node, "fpreset-gpio", 0);
        if(bf_dev->reset_gpio < 0) {
            BF_LOG("get fpreset-gpio fail:%d", bf_dev->reset_gpio);
            return bf_dev->reset_gpio;
        }

        bf_dev->irq_gpio =  of_get_named_gpio(node, "fpint-gpio", 0);
        if(bf_dev->irq_gpio < 0) {
            BF_LOG("get fpint-gpio fail:%d", bf_dev->irq_gpio);
            return bf_dev->irq_gpio;
        }

        //if gpio_to_irq cause irq has problem then chang to use irq_of_parse_and_map func and dts needto modify
        //bf_dev->irq_num = irq_of_parse_and_map(bf_dev->pdev->dev.of_node, 0);
        bf_dev->irq_num = gpio_to_irq(bf_dev->irq_gpio);
        if(!bf_dev->irq_num) {
            BF_LOG("get irq number fail:%d", bf_dev->irq_num);
            return -ENXIO;
        }
        BF_LOG("fpreset-gpio:%d, fpint-gpio:%d, irq_num:%d", bf_dev->reset_gpio, bf_dev->irq_gpio, bf_dev->irq_num);
#ifdef NEED_OPT_POWER_ON2V8
        bf_dev->power_2v8_gpio =  of_get_named_gpio(node, "fppower-gpio", 0);
        if(bf_dev->power_2v8_gpio < 0) {
            BF_LOG("get fppower-gpio fail:%d", bf_dev->power_2v8_gpio);
            return bf_dev->power_2v8_gpio;
        }
#endif    //NEED_OPT_POWER_ON2V8

#ifdef NEED_OPT_POWER_ON1V8
        bf_dev->power_1v8_gpio =  of_get_named_gpio(node, "fppower-gpio1v8", 0);
        if(bf_dev->power_1v8_gpio < 0) {
            BF_LOG("get fppower-gpio fail:%d", bf_dev->power_1v8_gpio);
            return bf_dev->power_1v8_gpio;
        }
#endif    //NEED_OPT_POWER_ON1V8
    } else {
        BF_LOG( "device of_node is null");
        return -EINVAL;
    }
#else

    bf_dev->irq_gpio = GPIO_FP_INT_PIN ;
    bf_dev->reset_gpio = GPIO_FINGER_PRINT_RESET ;
    bf_dev->irq_num = CUST_EINT_FP_INT_NUM ;

#endif


#ifdef BF_PINCTL
    bf_dev->pinctrl_gpios = devm_pinctrl_get (&bf_dev->pdev->dev);
    if (!bf_dev->pinctrl_gpios) {
        ret = PTR_ERR (bf_dev->pinctrl_gpios);
        BF_LOG( "can't find fingerprint pinctrl");
        return ret;
    }

    bf_dev->pins_spi_default = pinctrl_lookup_state (bf_dev->pinctrl_gpios, "spi_default");
    if (!bf_dev->pins_spi_default) {
        ret = PTR_ERR (bf_dev->pins_spi_default);
        BF_LOG( "can't find fingerprint pinctrl spi_default");
        //return ret;
    }
    bf_dev->pins_reset_high = pinctrl_lookup_state (bf_dev->pinctrl_gpios, "reset-output-high");
    if (!bf_dev->pins_reset_high) {
        ret = PTR_ERR (bf_dev->pins_reset_high);
        BF_LOG( "can't find fingerprint pinctrl reset-output-high");
        return ret;
    }

    bf_dev->pins_reset_low = pinctrl_lookup_state (bf_dev->pinctrl_gpios, "reset-output-low");
    if (!bf_dev->pins_reset_low) {
        ret = PTR_ERR (bf_dev->pins_reset_low);
        BF_LOG( "can't find fingerprint pinctrl reset-output-low");
        return ret;
    }

    bf_dev->pins_fp_interrupt = pinctrl_lookup_state (bf_dev->pinctrl_gpios, "int_default");
    if (!bf_dev->pins_fp_interrupt) {
        ret = PTR_ERR (bf_dev->pins_fp_interrupt);
        BF_LOG( "can't find fingerprint pinctrl fp_interrupt");
        return ret;
    }

#ifdef NEED_OPT_POWER_ON2V8
    bf_dev->pins_power_2v8_high = pinctrl_lookup_state (bf_dev->pinctrl_gpios, "power_2v8_high");
    if (!bf_dev->pins_power_2v8_high) {
        ret = PTR_ERR (bf_dev->pins_power_2v8_high);
        BF_LOG ("can't find fingerprint pinctrl power_2v8_high");
        return ret;
    }

    bf_dev->pins_power_2v8_low = pinctrl_lookup_state (bf_dev->pinctrl_gpios, "power_2v8_low");
    if (!bf_dev->pins_power_2v8_low) {
        ret = PTR_ERR (bf_dev->pins_power_2v8_low);
        BF_LOG ("can't find fingerprint pinctrl power_2v8_low");
        return ret;
    }
#endif    //NEED_OPT_POWER_ON2V8

#ifdef NEED_OPT_POWER_ON1V8
    bf_dev->pins_power_1v8_high = pinctrl_lookup_state (bf_dev->pinctrl_gpios, "power_1v8_high");
    if (!bf_dev->pins_power_1v8_high) {
        ret = PTR_ERR (bf_dev->pins_power_1v8_high);
        BF_LOG ("can't find fingerprint pinctrl power_1v8_high");
        return ret;
    }

    bf_dev->pins_power_1v8_low = pinctrl_lookup_state (bf_dev->pinctrl_gpios, "power_1v8_low");
    if (!bf_dev->pins_power_1v8_low) {
        ret = PTR_ERR (bf_dev->pins_power_1v8_low);
        BF_LOG ("can't find fingerprint pinctrl power_1v8_low");
        return ret;
    }
#endif    //NEED_OPT_POWER_ON1V8

    BF_LOG( "get pinctrl info success!");

#endif    //BF_PINCTL

    return 0;
}


/*
 *pinctrl方式初始化reset int poer管脚，不包含spi pin初始化
 */
#ifdef BF_PINCTL
static int32_t bf_main_pinctrl_init(struct bf_device *bf_dev)
{
    int32_t error = 0;


    error = pinctrl_select_state(bf_dev->pinctrl_gpios, bf_dev->pins_reset_high);
    if (error) {
        dev_err(&bf_dev->pdev->dev, "failed to activate pins_reset_high state\n");
    }

    error = pinctrl_select_state(bf_dev->pinctrl_gpios, bf_dev->pins_fp_interrupt);
    if (error) {
        dev_err(&bf_dev->pdev->dev, "failed to activate pins_fp_interrupt state\n");
    }
#ifdef NEED_OPT_POWER_ON2V8
    error = pinctrl_select_state(bf_dev->pinctrl_gpios, bf_dev->pins_power_2v8_high);
    if (error) {
        dev_err(&bf_dev->pdev->dev, "failed to activate pins_power_2v8_high state\n");
    }

#endif
#ifdef NEED_OPT_POWER_ON_1V8
    error = pinctrl_select_state(bf_dev->pinctrl_gpios, bf_dev->pins_power_1v8_high);
    if (error) {
        dev_err(&bf_dev->pdev->dev, "failed to activate pins_power_1v8_high state\n");
    }

#endif

    return error;
}
#endif    //BF_PINCTL

/*
 *gpio方式初始化
 */
static int32_t bf_main_gpio_init(struct bf_device *bf_dev)
{
    int error = 0;
    /*reset pin*/
    if (gpio_is_valid(bf_dev->reset_gpio)) {
        error = gpio_request(bf_dev->reset_gpio, "bf reset");
        if (error) {
            dev_err(&bf_dev->pdev->dev, "unable to request reset GPIO %d\n", bf_dev->reset_gpio);
            goto out;
        } else {
            gpio_direction_output (bf_dev->reset_gpio, 1);
        }
    } else {
        dev_err(&bf_dev->pdev->dev, "invalid reset GPIO %d\n", bf_dev->reset_gpio);
        error = -1;
        goto out;
    }

    /*irq pin*/
    if (gpio_is_valid(bf_dev->irq_gpio)) {
        error = gpio_request(bf_dev->irq_gpio, "bf irq_gpio");
        if (error) {
            dev_err(&bf_dev->pdev->dev, "unable to request irq_gpio GPIO %d\n", bf_dev->irq_gpio);
            goto out1;
        } else {
            gpio_direction_input(bf_dev->irq_gpio);
        }
    } else {
        dev_err(&bf_dev->pdev->dev, "invalid irq_gpio GPIO %d\n", bf_dev->irq_gpio);
        error = -1;
        goto out1;
    }

#ifdef NEED_OPT_POWER_ON2V8
    if (gpio_is_valid(bf_dev->power_2v8_gpio)) {
        error = gpio_request(bf_dev->power_2v8_gpio, "bf power_2v8_gpio");
        if (error) {
            dev_err(&bf_dev->pdev->dev, "unable to request power_2v8_gpio GPIO %d\n", bf_dev->power_2v8_gpio);
            goto out2;
        } else {
            gpio_direction_output (bf_dev->power_2v8_gpio, 1);
        }
    } else {
        dev_err(&bf_dev->pdev->dev, "invalid power_2v8_gpio GPIO %d\n", bf_dev->power_2v8_gpio);
        error = -1;
        goto out2;
    }
#endif    //NEED_OPT_POWER_ON_2V8

#ifdef NEED_OPT_POWER_ON1V8
    if (gpio_is_valid(bf_dev->power_1v8_gpio)) {
        error = gpio_request(bf_dev->power_1v8_gpio, "bf power_1v8_gpio");
        if (error) {
            dev_err(&bf_dev->pdev->dev, "unable to request power_1v8_gpio GPIO %d\n", bf_dev->power_1v8_gpio);
            goto out3;
        } else {
            gpio_direction_output (bf_dev->power_1v8_gpio, 1);
        }

    } else {
        dev_err(&bf_dev->pdev->dev, "invalid power_1v8_gpio GPIO %d\n", bf_dev->power_1v8_gpio);
        error = -1;
        goto out3;
    }
#endif    //NEED_OPT_POWER_ON1V8
    return 0;


out3:
#ifdef NEED_OPT_POWER_ON2V8
#ifndef MTK_ANDROID_L
    gpio_free(bf_dev->power_2v8_gpio);
#endif
#endif //NEED_OPT_POWER_ON2V8

out2:
#ifndef MTK_ANDROID_L
    gpio_free(bf_dev->irq_gpio);
#endif
out1:
#ifndef MTK_ANDROID_L
    gpio_free(bf_dev->reset_gpio);
#endif
out:
    return error;
}

static void bf_main_gpio_uninit(struct bf_device *bf_dev)
{
#ifndef MTK_ANDROID_L
    if (gpio_is_valid(bf_dev->irq_gpio))
        gpio_free(bf_dev->irq_gpio);
    if (gpio_is_valid(bf_dev->reset_gpio))
        gpio_free(bf_dev->reset_gpio);
#ifdef NEED_OPT_POWER_ON2V8
    if (gpio_is_valid(bf_dev->power_2v8_gpio))
        gpio_free(bf_dev->power_2v8_gpio);
#endif
#ifdef NEED_OPT_POWER_ON1V8
    if (gpio_is_valid(bf_dev->power_1v8_gpio))
        gpio_free(bf_dev->power_1v8_gpio);
#endif
#endif
}

static void bf_main_pin_uninit(struct bf_device *bf_dev)
{
#ifdef BF_PINCTL
    if(bf_dev->pinctrl_gpios)
        devm_pinctrl_put(bf_dev->pinctrl_gpios);
#endif
}

static int32_t bf_main_pin_init(struct bf_device *bf_dev)
{
    int32_t error = 0;

#ifdef BF_PINCTL
    error = bf_main_pinctrl_init(bf_dev);
    if(error) {
        BF_LOG("bf_main_pinctrl_init fail!");
    }

#else /*else BF_GPIOCTRL*/
    error = bf_main_gpio_init(bf_dev);
    if(error) {
        BF_LOG("bf_main_gpio_init fail!");
    }

#endif    //BF_PINCTL
    return error;
}

/* -------------------------------------------------------------------- */
/* netlink functions                 */
/* -------------------------------------------------------------------- */
void bf_send_netlink_msg(struct bf_device *bf_dev, const int command)
{
    struct nlmsghdr *nlh = NULL;
    struct sk_buff *skb = NULL;
    int ret;
    char data_buffer[2];

    BF_LOG("enter, send command %d", command);
    memset(data_buffer, 0, 2);
    data_buffer[0] = (char)command;
    if (NULL == bf_dev->netlink_socket) {
        BF_LOG("invalid socket");
        return;
    }

    if (0 == g_pid) {
        BF_LOG("invalid native process pid");
        return;
    }

    /*alloc data buffer for sending to native*/
    skb = alloc_skb(MAX_NL_MSG_LEN, GFP_ATOMIC);
    if (skb == NULL) {
        return;
    }

    nlh = nlmsg_put(skb, 0, 0, 0, MAX_NL_MSG_LEN, 0);
    if (!nlh) {
        BF_LOG("nlmsg_put failed");
        kfree_skb(skb);
        return;
    }

    NETLINK_CB(skb).portid = 0;
    NETLINK_CB(skb).dst_group = 0;

    *(char *)NLMSG_DATA(nlh) = command;
    *((char *)NLMSG_DATA(nlh) + 1) = 0;
    ret = netlink_unicast(bf_dev->netlink_socket, skb, g_pid, MSG_DONTWAIT);
    if (ret < 0) {
        BF_LOG("send failed");
        return;
    }

    BF_LOG("send done, data length is %d", ret);
    return ;
}

static void bf_recv_netlink_msg(struct sk_buff *__skb)
{
    struct sk_buff *skb = NULL;
    struct nlmsghdr *nlh = NULL;
    char str[128];

    skb = skb_get(__skb);
    if (skb == NULL) {
        BF_LOG("skb_get return NULL");
        return;
    }

    if (skb->len >= NLMSG_SPACE(0)) {
        nlh = nlmsg_hdr(skb);
        //add by wangdongbo
        //memcpy(str, NLMSG_DATA(nlh), sizeof(str));
        g_pid = nlh->nlmsg_pid;
        BF_LOG("pid: %d, msg: %s", g_pid, str);
        mutex_lock(&irq_count_lock);
        g_bf_dev->irq_count = 1;
        mutex_unlock(&irq_count_lock);
    } else {
        BF_LOG("not enough data length");
    }

    kfree_skb(__skb);
}

/*
#ifndef MTK_ANDROID_L
static int bf_destroy_inputdev(void)
{
    if (bf_inputdev) {
        input_unregister_device(bf_inputdev);
        //input_free_device(bf_inputdev);
        bf_inputdev = NULL;
    }
    return 0;
}
#endif
*/
static int bf_close_netlink(struct bf_device *bf_dev)
{
    if (bf_dev->netlink_socket != NULL) {
        netlink_kernel_release(bf_dev->netlink_socket);
        bf_dev->netlink_socket = NULL;
        return 0;
    }

    BF_LOG("no netlink socket yet");
    return -1;
}

static int bf_init_netlink(struct bf_device *bf_dev)
{
    struct netlink_kernel_cfg cfg;

    BF_LOG("bf_init_netlink g_netlink_port: %d", g_netlink_port);

    memset(&cfg, 0, sizeof(struct netlink_kernel_cfg));
    cfg.input = bf_recv_netlink_msg;

    bf_dev->netlink_socket = netlink_kernel_create(&init_net, g_netlink_port, &cfg);
    if (bf_dev->netlink_socket == NULL) {
        BF_LOG("netlink create failed");
        return -1;
    }
    BF_LOG("netlink create success");
    return 0;
}

static irqreturn_t bf_eint_handler (int irq, void *data)
{
    struct bf_device *bf_dev = (struct bf_device *)data;

    wait_event_interruptible_timeout(waiting_spi_prepare, !atomic_read(&suspended), msecs_to_jiffies (100));
#ifdef KERNEL_4_9
    __pm_wakeup_event(&fp_suspend_lock, 2 * HZ);
#else
    wake_lock_timeout(&fp_suspend_lock, 2 * HZ);
#endif
    BF_LOG("++++irq_handler netlink send+++++,%d,%d", g_bf_dev->irq_count, bf_dev->doing_reset);
    if(g_bf_dev->irq_count) {
        if(!bf_dev->doing_reset) {
            mutex_lock(&irq_count_lock);
            g_bf_dev->irq_count = 0;
            mutex_unlock(&irq_count_lock);
        }
        bf_send_netlink_msg(bf_dev, BF_NETLINK_CMD_IRQ);
    }
    BF_LOG("-----irq_handler netlink -----");
    return IRQ_HANDLED;
}

/* -------------------------------------------------------------------- */
/* file operation function                                                                                */
/* -------------------------------------------------------------------- */
static long bf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int error = 0;
    u32 bl229x_enbacklight = 0;
#ifdef BF_REE
    bl_read_write_reg_command_t read_write_cmd;
    int dma_size = 0;
#endif
    struct bf_device *bf_dev = NULL;
    unsigned int key_event = 0;
    unsigned int value = 0;
    BF_LOG("bf_ioctl.");

    bf_dev = (struct bf_device *)filp->private_data;
    if (_IOC_TYPE(cmd) != BF_IOCTL_MAGIC_NO) {
        BF_LOG("Not blestech fingerprint cmd.");
        return -EINVAL;
    }

    if (_IOC_DIR(cmd) & _IOC_READ)
        error = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));

    if (error == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
        error = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

    if (error) {
        BF_LOG("Not blestech fingerprint cmd direction.");
        return -EINVAL;
    }

    switch (cmd) {
    case BF_IOCTL_RESET:
        BF_LOG("BF_IOCTL_RESET: chip reset command\n");
        bf_hw_reset(bf_dev);
        break;
    case BF_IOCTL_ENABLE_INTERRUPT:
        BF_LOG("BF_IOCTL_ENABLE_INTERRUPT:  command,%d\n", bf_dev->irq_count);
        mutex_lock(&irq_count_lock);
        bf_dev->irq_count = 1;
        mutex_unlock(&irq_count_lock);
        break;
    case BF_IOCTL_DISABLE_INTERRUPT:
        BF_LOG("BF_IOCTL_DISABLE_INTERRUPT:  command,%d\n", bf_dev->irq_count);
        mutex_lock(&irq_count_lock);
        bf_dev->irq_count = 0;
        mutex_unlock(&irq_count_lock);
        break;
    case BF_IOCTL_ENABLE_POWER:
        BF_LOG("BF_IOCTL_ENABLE_POWER:  command\n");
        bf_hw_power(bf_dev, 1);
        break;
    case BF_IOCTL_DISABLE_POWER:
        BF_LOG("BF_IOCTL_DISABLE_POWER:  command\n");
        bf_hw_power(bf_dev, 0);
        break;
    case BF_IOCTL_INPUT_KEY:
        key_event = (unsigned int)arg;
        BF_LOG("key:%d\n", key_event);

        input_report_key(bf_inputdev, key_event, 1);
        input_sync(bf_inputdev);
        input_report_key(bf_inputdev, key_event, 0);
        input_sync(bf_inputdev);
        break;
    case BF_IOCTL_ENBACKLIGHT:
        BF_LOG("BF_IOCTL_ENBACKLIGHT arg:%d\n", (int)arg);
        g_bl229x_enbacklight = (int)arg;
        break;
    case BF_IOCTL_ISBACKLIGHT:
        BF_LOG("BF_IOCTL_ISBACKLIGHT\n");
        bl229x_enbacklight = g_bl229x_enbacklight;
        if (copy_to_user((void __user*)arg, &bl229x_enbacklight, sizeof(u32) * 1) != 0 ) {
            error = -EFAULT;
        }
        break;
    case BF_IOCTL_GET_ID:
        if (copy_to_user((void __user*)arg, &g_chip_type, sizeof(u32) * 1) != 0 ) {
            error = -EFAULT;
        }
        break;
#ifdef BF_REE
    case BF_IOCTL_REGISTER_READ_WRITE:
        BF_LOG("BTL:BF_IOCTL_REGISTER_READ_WRITE\n");

        if(copy_from_user(&read_write_cmd, (bl_read_write_reg_command_t *)arg, sizeof(read_write_cmd))) {
            error = -EFAULT;
            BF_LOG("BF_IOCTL_REGISTER_READ_WRITE copy_to_user faile!");
            break;
        }

        if(read_write_cmd.len < 32) {
            spi_send_cmd(g_bf_dev, read_write_cmd.data_tx, read_write_cmd.data_rx, read_write_cmd.len);
            if (copy_to_user((void __user*)arg, &read_write_cmd, sizeof(bl_read_write_reg_command_t)) != 0 ) {
                BF_LOG("BF_IOCTL_REGISTER_READ_WRITE copy_to_user faile!");
                error = -EFAULT;
                break;
            }
        } else {
            dma_size = ((read_write_cmd.len / 1024) + 1) * 1024;
            memset(g_bf_dev->image_buf, 0xff, get_order(dma_size));
            spi_set_dma_en(1);
#ifdef USE_SPI1_4GB_TEST
            spi_dma_exchange(g_bf_dev, read_write_cmd, dma_size);
#else
            spi_send_cmd(g_bf_dev, read_write_cmd.data_tx, g_bf_dev->image_buf, dma_size);
#endif
            spi_set_dma_en(0);
        }
        break;
#endif
#ifdef CONFIG_MTK_CLK
    case BF_IOCTL_ENABLE_SPI_CLOCK:
        BF_LOG("BF_IOCTL_ENABLE_SPI_CLK:  command\n");
        bf_spi_clk_enable(bf_dev, 1);
        break;
    case BF_IOCTL_DISABLE_SPI_CLOCK:
        BF_LOG("BF_IOCTL_DISABLE_SPI_CLK:  command\n");
        bf_spi_clk_enable(bf_dev, 0);
        break;
#endif
    case BF_IOCTL_INPUT_KEY_DOWN:
        if(g_bl229x_enbacklight && g_bf_dev->need_report == 0 && bf_key_need_report == 0) {
            bf_key_need_report = 1;
            key_event = (int)arg;
            input_report_key(bf_inputdev, key_event, 1);
            input_sync(bf_inputdev);
        }
        break;
    case BF_IOCTL_INPUT_KEY_UP:
        if(bf_key_need_report == 1) {
            bf_key_need_report = 0;
            key_event = (int)arg;
            input_report_key(bf_inputdev, key_event, 0);
            input_sync(bf_inputdev);
        }
        break;
    case BF_IOCTL_LOW_RESET:
        BF_LOG("BF_IOCTL_LOW_RESET:  command\n");
#ifdef KERNEL_4_9
        __pm_wakeup_event(&hw_reset_lock, 2 * HZ);
#else
        wake_lock_timeout(&hw_reset_lock, 2 * HZ);
#endif
        bf_hw_reset_level(g_bf_dev, 0);
        break;

    case BF_IOCTL_HIGH_RESET:
        BF_LOG("BF_IOCTL_HIGH_RESET:  command\n");
        bf_hw_reset_level(g_bf_dev, 1);
        break;

    case BF_IOCTL_NETLINK_INIT:
        BF_LOG("BF_IOCTL_NETLINK_INIT:  command\n");
        g_netlink_port = (int)arg;
        bf_close_netlink(g_bf_dev);
        error = bf_init_netlink(g_bf_dev);
        if (error < 0) {
            BF_LOG("BF_IOCTL_NETLINK_INIT:  error\n");
        }
        break;

    case BF_IOCTL_TRANS_IC_INFO:
		BF_LOG("BTL:BF_IOCTL_TRANS_IC_INFO\n");
		//add fpvendor for betterlife by song.li
		CHECK_THIS_DEV_DEBUG_AREADY_EXIT();
		REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();

        BF_LOG("BTL:BF_IOCTL_TRANS_IC_INFO\n");
        if (!copy_from_user(&ic_info, (bl_ic_info_t *)arg, sizeof(bl_ic_info_t))) {
            BF_LOG("ic_info:  name = %s, chipid = %x, ta: %s, ca: %s \n",
                   ic_info.ic_name, ic_info.ic_chipid, ic_info.ta_ver, ic_info.ca_ver);
        }
        break;

    case BF_IOCTL_INIT_DEVICE:
        BF_LOG("BF_IOCTL_INIT_DEVICE:  command\n");
        error = bf_init_dts_and_irq(g_bf_dev);
        break;

    case BF_IOCTL_REMOVE_DEVICE:
        BF_LOG("BF_IOCTL_REMOVE_DEVICE:  command\n");
        //bf_remove(g_bf_dev->pdev);
        bf_unregister();
        break;

    case BF_IOCTL_RESET_FLAG:
        bf_dev->doing_reset = (u8)arg;
        BF_LOG("BF_IOCTL_RESET_FLAG:  command,%d\n", bf_dev->doing_reset);
        break;
    case BF_IOCTL_IS_OPT_POWER_ON2V8:
        BF_LOG("BF_IOCTL_IS_OPT_POWER_ON2V8:  command\n");
#ifdef NEED_OPT_POWER_ON2V8
        key_event = 1;
#else
        key_event = 0;
#endif
        if (copy_to_user((void __user*)arg, &key_event, sizeof(u32) * 1) != 0 ) {
            error = -EFAULT;
        }
        break;
    case BF_IOCTL_CREATE_INPUT:
        BF_LOG("BF_IOCTL_CREATE_INPUT:  command\n");
        error = bf_create_inputdev();
        if (error) {
            BF_LOG("BF_IOCTL_CREATE_INPUT:  error\n");
        }
        break;
    case BF_IOCTL_COMPATIBLE_IN_HAL:
        BF_LOG("BF_IOCTL_COMPATIBLE_IN_HAL:  command\n");
#ifdef COMPATIBLE_IN_HAL
        value = 1;
#else
        value = 0;
#endif
        if (copy_to_user((void __user*)arg, &value, sizeof(u32) * 1) != 0 ) {
            error = -EFAULT;
        }
        break;
    default:
        BF_LOG("Supportn't this command(%x)\n", cmd);
        break;
    }

    return error;
}


#ifdef CONFIG_COMPAT
static long bf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int retval = 0;

    retval = bf_ioctl(filp, cmd, arg);

    return retval;
}
#endif

/*----------------------------------------------------------------------------*/
static int bf_open (struct inode *inode, struct file *filp)
{
    struct bf_device *bf_dev = g_bf_dev;
    int status = 0;

    filp->private_data = bf_dev;
    BF_LOG( " Success to open device.");

    return status;
}


/* -------------------------------------------------------------------- */
static ssize_t bf_write (struct file *file, const char *buff, size_t count, loff_t *ppos)
{
    return -ENOMEM;
}

/* -------------------------------------------------------------------- */
static ssize_t bf_read (struct file *filp, char  *buff, size_t count, loff_t *ppos)
{

    ssize_t status = 0;
#ifdef BF_REE
    int ret = 0;
    //    spi_read_frame(struct  bf_device *bf_dev);

    ret = copy_to_user(buff, g_bf_dev->image_buf , count); //skip
    if (ret) {
        BF_LOG("copy_to_user failed\n");
        status = -EFAULT;
    }
#endif
    BF_LOG("status: %d \n", (int)status);
    BF_LOG("  --\n");
    return status;

}

/* -------------------------------------------------------------------- */
static int bf_release (struct inode *inode, struct file *file)
{
    int status = 0 ;
    return status;
}
static int bf_suspend (struct platform_device *pdev, pm_message_t state)
{
    BF_LOG("  ++\n");
    mutex_lock(&g_dev_lock);
    if(g_bf_dev != NULL)
        g_bf_dev->need_report = 1;
    mutex_unlock(&g_dev_lock);
    BF_LOG("\n");
    return 0;
}
static int bf_resume (struct platform_device *pdev)
{
    BF_LOG("  ++\n");
    mutex_lock(&g_dev_lock);
    if(g_bf_dev != NULL)
        g_bf_dev->need_report = 0;
    mutex_unlock(&g_dev_lock);
    BF_LOG("\n");
    return 0;
}

/*----------------------------------------------------------------------------*/
static const struct file_operations bf_fops = {
    .owner = THIS_MODULE,
    .open  = bf_open,
    .write = bf_write,
    .read  = bf_read,
    .release =  bf_release,
    .unlocked_ioctl = bf_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = bf_compat_ioctl,
#endif
};

static struct miscdevice bf_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = BF_DEV_NAME,
    .fops = &bf_fops,
};

int bf_remove(struct platform_device *pdev)
{
    struct bf_device *bf_dev = g_bf_dev;
    BF_LOG("bf_remove++++");
    mutex_lock(&g_dev_lock);
    //bf_remove_attributes(&bf_dev->pdev->dev);
    bf_sysfs_uninit();

    bf_hw_power(bf_dev, 0);

    if (bf_dev->irq_num) {
#ifndef MTK_ANDROID_L
        free_irq(bf_dev->irq_num, bf_dev);
#else
        disable_irq_nosync(bf_dev->irq_num);
#endif
        bf_dev->irq_count = 0;
        bf_dev->irq_num = 0;
    }

    bf_main_gpio_uninit(bf_dev);
    bf_main_pin_uninit(bf_dev);

#if defined(CONFIG_FB)
    fb_unregister_client(&bf_dev->fb_notify);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    unregister_early_suspend(&bf_dev->early_suspend);
#endif

    misc_deregister(&bf_misc_device);
    bf_close_netlink(bf_dev);

#if defined(BF_REE)
    free_pages((unsigned long)bf_dev->image_buf, get_order(BUF_SIZE));
#endif

    platform_set_drvdata(bf_dev->pdev, NULL);
    if(NULL != bf_dev)
        kfree(bf_dev);
    bf_dev      = NULL;
    g_bf_dev    = NULL;
    mutex_unlock(&g_dev_lock);
    BF_LOG("bf_remove----");
    return 0;
}

static int bf_create_inputdev(void)
{
    if (bf_inputdev) {
        BF_LOG("bf_inputdev in not null, already created!\n");
        return 0;
    }
    bf_inputdev = input_allocate_device();
    if (!bf_inputdev) {
        BF_LOG("bf_inputdev create faile!\n");
        return -ENOMEM;
    }
    __set_bit(EV_KEY, bf_inputdev->evbit);
    __set_bit(KEY_F10, bf_inputdev->keybit);    //68
    __set_bit(KEY_F11, bf_inputdev->keybit);    //88
    __set_bit(KEY_F12, bf_inputdev->keybit);    //88
    __set_bit(KEY_CAMERA, bf_inputdev->keybit); //212
    __set_bit(KEY_POWER, bf_inputdev->keybit);  //116
    __set_bit(KEY_PHONE, bf_inputdev->keybit);  //call 169
    __set_bit(KEY_BACK, bf_inputdev->keybit);   //call 158
    __set_bit(KEY_HOMEPAGE, bf_inputdev->keybit);   //call 172
    __set_bit(KEY_MENU, bf_inputdev->keybit);   //call 158

    __set_bit(KEY_F1, bf_inputdev->keybit);     //69
    __set_bit(KEY_F2, bf_inputdev->keybit);     //60
    __set_bit(KEY_F3, bf_inputdev->keybit);     //61
    __set_bit(KEY_F4, bf_inputdev->keybit);     //62
    __set_bit(KEY_F5, bf_inputdev->keybit);     //63
    __set_bit(KEY_F6, bf_inputdev->keybit);     //64
    __set_bit(KEY_F7, bf_inputdev->keybit);     //65
    __set_bit(KEY_F8, bf_inputdev->keybit);     //66
    __set_bit(KEY_F9, bf_inputdev->keybit);     //67

    __set_bit(KEY_UP, bf_inputdev->keybit);     //103
    __set_bit(KEY_DOWN, bf_inputdev->keybit);   //108
    __set_bit(KEY_LEFT, bf_inputdev->keybit);   //105
    __set_bit(KEY_RIGHT, bf_inputdev->keybit);  //106
    __set_bit(0x2D3, bf_inputdev->keybit);

    bf_inputdev->id.bustype = BUS_HOST;
    bf_inputdev->name = "betterlife_inputdev";
    if (input_register_device(bf_inputdev)) {
        BF_LOG("register inputdev failed");
        input_free_device(bf_inputdev);
        return -ENOMEM;
    }
    return 0;
}

int bf_init_dts_and_irq(struct bf_device *bf_dev)
{
    static int initialized = 0;
    int32_t status = -EINVAL;
    BF_LOG( "    ++++");
    if(initialized != 1) {
        status = bf_main_get_gpio_info(bf_dev);
        if(status) {
            BF_LOG("bf_main_get_gpio_info fail:%d", status);
            return -1;
        }

        status = bf_main_pin_init(bf_dev);
        if(status) {
            BF_LOG("bf_main_init fail:%d", status);
            bf_main_gpio_uninit(bf_dev);
            bf_main_pin_uninit(bf_dev);
            return -2;
        }

#ifdef MTK_ANDROID_L
        INIT_WORK(&(bf_dev->fingerprint_work), work_func);
        bf_dev->fingerprint_workqueue = create_singlethread_workqueue("bf_fingerpirnt_thread");
        mt_eint_registration(bf_dev->irq_num, EINTF_TRIGGER_RISING/*CUST_EINTF_TRIGGER_RISING*/, bf_eint_handler_l, 1);
#else
        status = request_threaded_irq (bf_dev->irq_num, NULL, bf_eint_handler,  IRQ_TYPE_EDGE_RISING /*IRQF_TRIGGER_RISING*/ | IRQF_ONESHOT, BF_DEV_NAME, bf_dev);
#endif

        if (status) {
            BF_LOG("irq thread request failed, retval=%d\n", status);
            bf_main_gpio_uninit(bf_dev);
            bf_main_pin_uninit(bf_dev);
            return -3;
        }

        bf_hw_power(bf_dev, 1);
        bf_hw_reset(bf_dev);

        enable_irq_wake(bf_dev->irq_num);
        initialized = 1;
    } else {
        BF_LOG( " has initilized, do nothing !");
    }

    BF_LOG( "    ----");
    return 0;
}

static void bf_platform_info(void)
{
    char bl_platform[128];
    char tee_platform[128];
#if defined (PLATFORM_SPRD)
    memcpy(bl_platform, "PLATFORM_SPRD", sizeof(bl_platform));
#elif defined(PLATFORM_QCOM)
    memcpy(bl_platform, "PLATFORM_QCOM", sizeof(bl_platform));
#elif defined(PLATFORM_RK)
    memcpy(bl_platform, "PLATFORM_RK", sizeof(bl_platform));
#elif defined(MTK_ANDROID_L)
    memcpy(bl_platform, "MTK_ANDROID_L", sizeof(bl_platform));
#elif defined(PLATFORM_MTK)
    memcpy(bl_platform, "PLATFORM_MTK", sizeof(bl_platform));
#elif defined(PLATFORM_MT6739)
    memcpy(bl_platform, "PLATFORM_MT6739", sizeof(bl_platform));
#elif defined(PLATFORM_MT676x)
    memcpy(bl_platform, "PLATFORM_MT676x", sizeof(bl_platform));
#elif defined(USE_SPI1_4GB_TEST)
    memcpy(bl_platform, "USE_SPI1_4GB_TEST", sizeof(bl_platform));
#else
    memcpy(bl_platform, "undefined", sizeof(bl_platform));
#endif

#if defined (TEE_TK)
    memcpy(tee_platform, "TK", sizeof(tee_platform));
#elif defined(TEE_BEANPOD)
    memcpy(tee_platform, "BEANPOD", sizeof(tee_platform));
#elif defined(BF_REE)
    memcpy(tee_platform, "REE", sizeof(tee_platform));
#elif defined(other)
    memcpy(tee_platform, "other", sizeof(tee_platform));
#else
    memcpy(tee_platform, "undefined", sizeof(tee_platform));
#endif
    BF_LOG("bf_platform_info: CPU_PLATFORM:%s TEE_PLATFORM:%s \r\n", bl_platform , tee_platform);
}

static int bf_probe(struct platform_device *pdev)
{
    struct bf_device *bf_dev = NULL;
    int32_t status = -EINVAL;

    BF_LOG( "++++++++++++");
    bf_platform_info();

    bf_dev = kzalloc(sizeof (struct bf_device), GFP_KERNEL);
    if (NULL == bf_dev) {
        BF_LOG( "kzalloc bf_dev failed.");
        status = -ENOMEM;
        goto err0;
    }

    bf_dev->pdev = pdev;
    bf_dev->irq_count = 0;
    bf_dev->doing_reset = 0;
    bf_dev->report_key = KEY_F10;
    bf_dev->reset_gpio = -1;
#ifdef NEED_OPT_POWER_ON2V8
    bf_dev->power_2v8_gpio = -1;
#endif
#ifdef NEED_OPT_POWER_ON1V8
    bf_dev->power_1v8_gpio = -1;
#endif
    bf_dev->irq_gpio = -1;
#ifdef KERNEL_4_9
    wakeup_source_init(&fp_suspend_lock, "fp_wakelock");
    wakeup_source_init(&hw_reset_lock, "fp_reset_wakelock");
#else
    wake_lock_init(&fp_suspend_lock, WAKE_LOCK_SUSPEND, "fp_wakelock");
    wake_lock_init(&hw_reset_lock, WAKE_LOCK_SUSPEND, "fp_reset_wakelock");
#endif
    atomic_set(&suspended, 0);
    g_bf_dev = bf_dev;
    platform_set_drvdata(pdev, bf_dev);

#if defined(BF_REE)
    g_bf_dev->image_buf = (u8*)__get_free_pages(GFP_KERNEL, get_order(BUF_SIZE));
    if (!bf_dev->image_buf) {
        BF_LOG("\ng_bf_dev->image_buf __get_free_pages failed\n");
        status = -ENOMEM;
        goto err1;
    }
#endif

#if defined(COMPATIBLE_IN_HAL) || defined(COMPATIBLE)
    BF_LOG("compatible in hal or COMPATIBLE, do not init gpio pin hw reset and init_irq.");
#else
    status = bf_init_dts_and_irq(bf_dev);
    if (status) {
        goto err2;
    }
#endif

    /* netlink interface init */
    /*BF_LOG ("bf netlink config");
    if (bf_init_netlink(bf_dev) < 0) {
        BF_LOG ("bf_netlink create failed");
        status = -EINVAL;
        goto err5;
    }*/

    status = misc_register(&bf_misc_device);
    if(status) {
        BF_LOG("bf_misc_device register failed\n");
        goto err6;
    }

#if defined(CONFIG_FB)
    bf_dev->fb_notify.notifier_call = fb_notifier_callback;
    status = fb_register_client(&bf_dev->fb_notify);
    if(status) {
        BF_LOG("fb_register_client failed\n");
        goto err7;
    }
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    bf_dev->early_suspend.suspend = early_suspend;
    bf_dev->early_suspend.resume = early_resume;
    bf_dev->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    register_early_suspend(&bf_dev->early_suspend);
#endif

    status = bf_sysfs_init();
    if(status) {
        BF_LOG("bf_sysfs_init failed\n");
        goto err8;
    }

    //bf_create_attributes(&bf_dev->pdev->dev);

#if !defined(COMPATIBLE_IN_HAL) && !defined(COMPATIBLE)
    bf_hw_power(bf_dev, 1);
    bf_hw_reset(bf_dev);
#endif

#if defined(BF_REE) || defined(COMPATIBLE) || defined(CONFIG_MTK_CLK)
    status = bf_spi_init(bf_dev);
    if(status) {
        BF_LOG("bf_spi_init fail:%d", status);
        goto err9;
    }
#endif

    BF_LOG ("bf_probe success!");
    return 0;

err9:
    //bf_remove_attributes(&bf_dev->pdev->dev);
    bf_sysfs_uninit();
err8:
#if defined(CONFIG_FB)
    fb_unregister_client(&bf_dev->fb_notify);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    unregister_early_suspend(&bf_dev->early_suspend);
#endif

err7:
    misc_deregister(&bf_misc_device);
err6:
    bf_close_netlink(bf_dev);
err5:
#ifndef MTK_ANDROID_L
    free_irq(bf_dev->irq_num, bf_dev);
#else
    disable_irq_nosync(bf_dev->irq_num);
#endif
err2:
#if defined(BF_REE)
    free_pages((unsigned long)bf_dev->image_buf, get_order(BUF_SIZE));
#endif
err1:
    platform_set_drvdata(bf_dev->pdev, NULL);
    kfree(bf_dev);
err0:
    BF_LOG("bf_probe occured error \n");
    return status;
}

#ifdef CONFIG_OF
static struct of_device_id bf_of_table[] = {
    {.compatible = "betterlife,platform",},
    {},
};
//MODULE_DEVICE_TABLE(of, bf_of_table);
#endif

#ifdef MTK_ANDROID_L
struct platform_device bf_plt_device = {
    .name = BF_DEV_NAME,
    .id = -1,
};
#endif

static struct platform_driver bf_plt_driver = {
    .driver = {
        .name = BF_DEV_NAME,
        .owner = THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = bf_of_table,
#endif
    },

    .probe = bf_probe,
    .remove = bf_remove,
    .resume = bf_resume,
    .suspend = bf_suspend,
};

module_platform_driver(bf_plt_driver);

void bf_unregister()
{
    platform_driver_unregister(&bf_plt_driver);

#if defined(BF_REE) || defined(COMPATIBLE) || defined(CONFIG_MTK_CLK)
    bf_spi_unregister();
#endif
}

#ifdef MTK_ANDROID_L    /*if active this, MUST no use platform dts, otherwise kernel crashes*/
static int  bf_plt_init(void)
{
    int ret = 0;

    ret = platform_device_register(&bf_plt_device);
    if(ret) {
        BF_LOG ("platform_device_register for blfp failed!");
        return -1;
    }
    /*
    ret = platform_driver_register(&bf_plt_driver);
    if(ret)
    {
        BF_LOG ("platform_driver_register for blfp failed!");
        return -1;
    }
    */
    return ret;
}


static void  bf_plt_exit(void)
{
    BF_LOG ("exit !");
    bf_unregister();
}

late_initcall(bf_plt_init);
module_exit(bf_plt_exit);
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR ("betterlife@blestech.com");
MODULE_DESCRIPTION ("Betterlife fingerprint sensor driver.");
