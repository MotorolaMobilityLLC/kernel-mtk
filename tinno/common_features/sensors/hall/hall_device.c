/* ------------------------------------------------------------------------- */
/* hall_device.c - a device driver for the hall device interface             */
/* ------------------------------------------------------------------------- */
/*
    Copyright (C) 2013-2020 Leatek Co.,Ltd

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
/* ------------------------------------------------------------------------- */
/*#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>

#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>

#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <linux/switch.h>
#include <linux/delay.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#include <linux/input.h>
//#include <linux/wakelock.h>
#include <linux/time.h>

#include <linux/string.h>
#include <linux/of_irq.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_reg_base.h>
#include <mach/irqs.h>

#include <cust_eint.h>
#include <cust_gpio_usage.h>
#include <mach/mt_gpio.h>
#include <mach/eint.h>
#include <mach/mt_pm_ldo.h>

#include <linux/mtgpio.h>
#include <linux/gpio.h>
*/
#include <linux/kthread.h>

//#include <linux/wakelock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/ioctl.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mt-plat/aee.h>
#ifdef CONFIG_MTK_SMARTBOOK_SUPPORT
#include <linux/sbsuspend.h>    /* smartbook */
#endif
#include <linux/atomic.h>

#include <linux/kernel.h>
#include <linux/delay.h>

#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/sched.h>
//#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/types.h>

#ifdef CONFIG_PM_WAKELOCKS
#include <linux/pm_wakeup.h>
#else
#include <linux/wakelock.h>
#endif

#ifdef CONFIG_TINNO_PRODUCT_INFO
#include <dev_info.h>
#endif

/** guomingyi add. */
#define HALL_DEVICE_CONTROAL_IMPL
/*------
----------------------------------------------------------------
static variable defination
----------------------------------------------------------------------*/

//#define HALL_DEVNAME    "hall_dev"

#define EN_DEBUG

#if defined(EN_DEBUG)

#define TRACE_FUNC  printk("[hall_dev] function: %s, line: %d \n", __func__, __LINE__);

#define HALL_DEBUG  printk
#else

#define TRACE_FUNC(x,...)

#define HALL_DEBUG(x,...)
#endif

#define  HALL_CLOSE  0
#define  HALL_OPEN    1

//#define HALL_SWITCH_EINT        CUST_EINT_MHALL_NUM
//#define HALL_SWITCH_DEBOUNCE    CUST_EINT_MHALL_DEBOUNCE_CN       /* ms */
//#define HALL_SWITCH_TYPE        CUST_EINT_MHALL_POLARITY
//#define HALL_SWITCH_SENSITIVE   CUST_EINT_MHALL_SENSITIVE


/*<BUG><VGAAO-731>*/
#if defined(CONFIG_PROJECT_V600)
#define HALL_SW_DEBOUCE
#endif	

#ifdef HALL_SW_DEBOUCE
static struct timer_list hall_delay_report_timer;
static unsigned char in_timer_flag = 0;
static unsigned char key_hallopen_cnt = 0;
static unsigned char key_hallclose_cnt = 0;
#endif

/****************************************************************/
/*******static function defination                             **/
/****************************************************************/

//static struct device *hall_nor_device = NULL;
static struct input_dev *hall_input_dev;
static  int cur_hall_status = HALL_OPEN;

#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source hall_key_lock;
#else
struct wake_lock hall_key_lock;
#endif

static int hall_key_event = 0;
#ifndef HALL_DEVICE_CONTROAL_IMPL
static int g_hall_first = 1;
#endif
int hall_irq;
struct pinctrl *hallpinctrl = NULL;

#if defined(CONFIG_OF)
static irqreturn_t hall_eint_func(int irq, void *data);
#else
static void hall_eint_func(unsigned long data);
#endif
//#define KEY_HALLOPEN  2
//#define KEY_HALLCLOSE  3
static atomic_t send_event_flag = ATOMIC_INIT(0);
static DECLARE_WAIT_QUEUE_HEAD(send_event_wq);
static int hall_probe(struct platform_device *pdev);
static int hall_remove(struct platform_device *dev);
/****************************************************************/
/*******export function defination                             **/
/****************************************************************/
#if defined(CONFIG_OF)
struct platform_device hall_device =
{
    .name     = "hall_eint",
    .id       = -1,
    /* .dev    ={ */
    /* .release = accdet_dumy_release, */
    /* } */
};

#endif
struct of_device_id hall_of_match[] =
{
    { .compatible = "mediatek, hall", },
    {},
};

static struct platform_driver hall_driver =
{
    .probe  = hall_probe,
    .remove  = hall_remove,
    .driver    = {
        .name       = "hall",
        .of_match_table = hall_of_match,
    },
};

static ssize_t hall_status_info_show (struct device_driver *driver, char *buf)
{
    HALL_DEBUG("[hall_dev] cur_hall_status=%d\n", cur_hall_status);
    return sprintf(buf, "%d\n", cur_hall_status);
}

static ssize_t hall_status_info_store (struct device_driver *driver, const char *buf,size_t count)
{
    HALL_DEBUG("[hall_dev] %s ON/OFF value = %d:\n ", __func__, cur_hall_status);

    if(sscanf(buf, "%u", &cur_hall_status) != 1)
    {
        HALL_DEBUG("[hall_dev]: Invalid values\n");
        return -EINVAL;
    }
    return count;
}

// guomingyi add.
#ifdef HALL_DEVICE_CONTROAL_IMPL
static int sendKeyEvent(void *unuse);
static struct task_struct *kr_monior_thread = NULL;
static ssize_t hall_enable_read(struct device_driver *driver, char *buf);

int get_hall_enable_state(char *buf, void *args) {
    return hall_enable_read(args, buf);
}

/** request irq */
static int hall_dev_request_irq(void)
{
    int ret = 0;
    u32 ints[2] = {0, 0};
    struct device_node *node;

    /** find hall node from dts */
    node = of_find_compatible_node(NULL, NULL, "mediatek, hall");
    if (node) {
        /** map irq num from dts config */
        of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
        hall_irq = irq_of_parse_and_map(node, 0);
        HALL_DEBUG("[Hall] irq num = %d\n", hall_irq);
        /** linux api to request irq */
        if (hall_irq <= 0) {
            goto err;
        }
        ret = request_irq(hall_irq, hall_eint_func, IRQF_TRIGGER_NONE, "hall", NULL);
        if(ret > 0) {
            goto err;
        }
    }
    else {
        goto err;
    }

    return 0;

err:
    HALL_DEBUG("[Hall] request irq failed!:%d\n", ret);
    return -1;
}

/** init gpio pin ctrl */
void setIntPinConfig(void)
{
    struct pinctrl_state *pins_cfg = NULL;

    /** get pin config from dts */
    pins_cfg = pinctrl_lookup_state(hallpinctrl, "pin_cfg");
    if (IS_ERR(pins_cfg)) {
        BUG_ON(1);
        return;
    }

    /** set pin to input mode */
    pinctrl_select_state(hallpinctrl, pins_cfg);
    HALL_DEBUG("[hall_device]%s:%d success!\n",__func__,__LINE__);
}

/** hall irq init */
int hall_enable_set(int enable)
{
    if (kr_monior_thread == NULL) {
        /** init kernel wake lock */
#ifdef CONFIG_PM_WAKELOCKS
	wakeup_source_init(&hall_key_lock, "hall key wakelock");
#else
	wake_lock_init(&hall_key_lock, WAKE_LOCK_SUSPEND, "hall key wakelock");
#endif

        /** init sleep queue */
        init_waitqueue_head(&send_event_wq);

        /** create kernel thread loop */
        kr_monior_thread = kthread_run(sendKeyEvent, 0, "key_report_trhead");
        if (IS_ERR(kr_monior_thread)) {
            HALL_DEBUG("[hall_dev]:%s:%d kthread_run err!\n",__func__,__LINE__);
            goto err;
        }

        /** request irq */
        if (hall_dev_request_irq() < 0) {
            goto err;
        }
    }

    HALL_DEBUG("[hall_dev]:%s:%d init success!\n",__func__,__LINE__);
    return 0;

err:
    HALL_DEBUG("[hall_dev]:%s:%d init failed!\n",__func__,__LINE__);
    return -1;
}


/* This function called by init.rc:
   on property:ro.feature.leather=true
       write /sys/bus/platform/drivers/hall/hall_enable 1
*/
static ssize_t hall_enable_read(struct device_driver *driver, char *buf)
{
    if (kr_monior_thread != NULL) {
        return sprintf(buf, "%d\n", hall_key_event);
    }
    else {
        return sprintf(buf, "%s\n", "-1");
    }
}

static ssize_t hall_enable_store(struct device_driver * driver, const char * buf, size_t count)
{
    HALL_DEBUG("[hall_dev]:%s:%d,enable:%s\n",__func__,__LINE__,buf);
    if (buf[0] == '1') {
        hall_enable_set(1);
    }
    return count;
}
static DRIVER_ATTR(hall_enable, 0664, hall_enable_read,  hall_enable_store);
#endif /* HALL_DEVICE_CONTROAL_IMPL */

static DRIVER_ATTR(hall_state, 0664, hall_status_info_show,  hall_status_info_store);

#ifdef HALL_SW_DEBOUCE
static void hall_delay_report_fn(void)
{
HALL_DEBUG("[hall_dev]:-----hall_delay_report_fn----[%d][%d][%d]\n", in_timer_flag, key_hallopen_cnt, key_hallclose_cnt);
    if(0==in_timer_flag)
    {
        key_hallopen_cnt = 0;
        key_hallclose_cnt = 0;
        in_timer_flag = 1;
        hall_delay_report_timer.expires = jiffies + HZ / 5;     /* delay seconds */
        add_timer(&hall_delay_report_timer);
    }

    if(HALL_OPEN == hall_key_event)
    {
        if(key_hallopen_cnt<50)key_hallopen_cnt++;
    }
    else if(HALL_CLOSE == hall_key_event)
    {
        if(key_hallclose_cnt<50)key_hallclose_cnt++;
    }
		
}

static void hall_delay_timer_fn(unsigned long unused)
{
	HALL_DEBUG("[hall_dev]:-----[%d][%d][%d]\n", in_timer_flag, key_hallopen_cnt, key_hallclose_cnt);
		

    if(  ((key_hallopen_cnt>0)&&(0==key_hallclose_cnt)) ||((0==key_hallopen_cnt)&&(key_hallclose_cnt>0))  )	
    {
        //send key event
        if(HALL_OPEN == hall_key_event)
        {
            HALL_DEBUG("[hall_dev]:HALL_OPEN!\n");
            input_report_key(hall_input_dev, KEY_HALLOPEN, 1);
            input_report_key(hall_input_dev, KEY_HALLOPEN, 0);
            input_sync(hall_input_dev);
        }
        else if(HALL_CLOSE == hall_key_event)
        {
            HALL_DEBUG("[hall_dev]:HALL_CLOSE!\n");
            input_report_key(hall_input_dev, KEY_HALLCLOSE, 1);
            input_report_key(hall_input_dev, KEY_HALLCLOSE, 0);
            input_sync(hall_input_dev);
        }
    }

    key_hallopen_cnt = 0;
    key_hallclose_cnt = 0;
    in_timer_flag = 0;
}
#endif

static int sendKeyEvent(void *unuse)
{
    while(1)
    {

        HALL_DEBUG("[hall_dev]:sendKeyEvent wait\n");
#ifdef CONFIG_OF

        enable_irq(hall_irq);
		enable_irq_wake(hall_irq);
        HALL_DEBUG("[HALL]enable_irq  !!!!!!\n");
#else
        /* for detecting the return to old_hall_state */
        mt_eint_unmask(HALL_SWITCH_EINT);
#endif

        //wait for signal
        wait_event_interruptible(send_event_wq, (atomic_read(&send_event_flag) != 0));
        
#ifdef CONFIG_PM_WAKELOCKS
	__pm_wakeup_event(&hall_key_lock, 2 * HZ);
#else
	wake_lock_timeout(&hall_key_lock, 2 * HZ);  //set the wake lock.
#endif

        HALL_DEBUG("[hall_dev]:going to send event %d\n", hall_key_event);
#ifdef CONFIG_OF
        disable_irq(hall_irq);
#else
        mt_eint_mask(HALL_SWITCH_EINT);
#endif

#ifdef HALL_SW_DEBOUCE
        //send key event
        if(HALL_OPEN == hall_key_event)
        {
            HALL_DEBUG("[hall_dev]:HALL_OPEN!  not report now !!!  used HALL_SW_DEBOUCE...\n");
        }
        else if(HALL_CLOSE == hall_key_event)
        {
            HALL_DEBUG("[hall_dev]:HALL_CLOSE!  not report now !!!  used HALL_SW_DEBOUCE...\n");
        }		
        hall_delay_report_fn();
#else
        //send key event
        if(HALL_OPEN == hall_key_event)
        {
            HALL_DEBUG("[hall_dev]:HALL_OPEN!\n");
            input_report_key(hall_input_dev, KEY_HALLOPEN, 1);
            input_report_key(hall_input_dev, KEY_HALLOPEN, 0);
            input_sync(hall_input_dev);
        }
        else if(HALL_CLOSE == hall_key_event)
        {
            HALL_DEBUG("[hall_dev]:HALL_CLOSE!\n");
            input_report_key(hall_input_dev, KEY_HALLCLOSE, 1);
            input_report_key(hall_input_dev, KEY_HALLCLOSE, 0);
            input_sync(hall_input_dev);
        }
#endif

        atomic_set(&send_event_flag, 0);
    }
    return 0;
}

static ssize_t notify_sendKeyEvent(int event)
{
    hall_key_event = event;
    atomic_set(&send_event_flag, 1);
    wake_up(&send_event_wq);
    HALL_DEBUG("[hall_dev]:notify_sendKeyEvent !\n");
    return 0;
}

#ifdef CONFIG_OF
static irqreturn_t hall_eint_func(int irq, void *data)
{
    HALL_DEBUG("hall_eint_func \n");
    if(cur_hall_status ==  HALL_CLOSE )
    {
        HALL_DEBUG("hall_eint_func  HALL_OPEN\n");
        notify_sendKeyEvent(HALL_OPEN);

        irq_set_irq_type(hall_irq, IRQ_TYPE_LEVEL_LOW);

        /* update the eint status */
        cur_hall_status = HALL_OPEN;
    }
    else
    {
        HALL_DEBUG("hall_eint_func  HALL_CLOSE\n");
        notify_sendKeyEvent(HALL_CLOSE);

        irq_set_irq_type(hall_irq, IRQ_TYPE_LEVEL_HIGH);

        cur_hall_status = HALL_CLOSE;
    }
    return IRQ_HANDLED;
}
#else
static void hall_eint_func(unsigned long data)
{

    TRACE_FUNC;

    mt65xx_eint_mask(HALL_SWITCH_EINT);

    if(cur_hall_status ==  HALL_CLOSE )
    {
        HALL_DEBUG("[hall_dev]:HALL_opened \n");
        notify_sendKeyEvent(HALL_OPEN);
        cur_hall_status = HALL_OPEN;
    }
    else
    {
        HALL_DEBUG("[hall_dev]:HALL_closed \n");
        notify_sendKeyEvent(HALL_CLOSE);
        cur_hall_status = HALL_CLOSE;
    }
    mt65xx_eint_set_polarity(HALL_SWITCH_EINT, cur_hall_status);
    mdelay(10);
    mt65xx_eint_unmask(HALL_SWITCH_EINT);
}
#endif

#ifndef HALL_DEVICE_CONTROAL_IMPL
static  int hall_setup_eint(void)
{
    int ret;
#ifdef CONFIG_OF
    u32 ints[2] = {0, 0};
    struct device_node *node;
#endif
    struct pinctrl_state *pins_cfg;

    /*configure to GPIO function, external interrupt*/
    HALL_DEBUG("[Hall]hall_setup_eint\n");

#if 1	

    pins_cfg = pinctrl_lookup_state(hallpinctrl, "pin_cfg");
    if (IS_ERR(pins_cfg))
    {
        HALL_DEBUG("Cannot find hall pinctrl pin_cfg!\n");
    }
#endif

#ifdef CONFIG_OF
    node = of_find_compatible_node(NULL, NULL, "mediatek, hall");
    if(node)
    {
        of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
        pinctrl_select_state(hallpinctrl, pins_cfg);

        hall_irq = irq_of_parse_and_map(node, 0);
	HALL_DEBUG("hall_irq = %d\n", hall_irq);
        ret = request_irq(hall_irq, hall_eint_func, IRQF_TRIGGER_NONE, "hall", NULL);
        if(ret > 0)
        {
            HALL_DEBUG("[Hall]EINT IRQ LINE NOT AVAILABLE\n");
        }
	else 
	{
	//	enable_irq(hall_irq);
        	HALL_DEBUG("[Hall]EINT IRQ request_irq OK\n");
	}
    }
    else
    {
        HALL_DEBUG("[Hall]%s can't find compatible node\n", __func__);
    }
#else
    mt_eint_set_sens(HALL_SWITCH_EINT, HALL_SWITCH_SENSITIVE);
    mt_eint_set_hw_debounce(HALL_SWITCH_EINT, HALL_SWITCH_DEBOUNCE);
    mt_eint_registration(HALL_SWITCH_EINT, HALL_SWITCH_DEBOUNCE, HALL_SWITCH_TYPE, hall_eint_func, 0);
    mt_eint_unmask(HALL_SWITCH_EINT);
#endif

    return 0;
}
#endif

static int hall_probe(struct platform_device *pdev)
{
    int ret = 0;
#ifndef HALL_DEVICE_CONTROAL_IMPL
    struct task_struct *keyEvent_thread = NULL;
#endif

    TRACE_FUNC;

    hallpinctrl = devm_pinctrl_get(&pdev->dev);
    if (IS_ERR(hallpinctrl))
    {
        HALL_DEBUG("hall_probe   Cannot find hall pinctrl!");
        return -ENOMEM;
    }

    hall_input_dev = input_allocate_device();

    if (!hall_input_dev)
    {
        HALL_DEBUG("[hall_dev]:hall_input_dev : fail!\n");
        return -ENOMEM;
    }

    __set_bit(EV_KEY, hall_input_dev->evbit);
    __set_bit(KEY_HALLOPEN, hall_input_dev->keybit);
    __set_bit(KEY_HALLCLOSE, hall_input_dev->keybit);

    hall_input_dev->id.bustype = BUS_HOST;
    hall_input_dev->name = "HALL_DEV";
    if(input_register_device(hall_input_dev))
    {
        HALL_DEBUG("[hall_dev]:hall_input_dev register : fail!\n");
    }
    else
    {
        HALL_DEBUG("[hall_dev]:hall_input_dev register : success!!\n");
    }

#ifdef HALL_DEVICE_CONTROAL_IMPL
    setIntPinConfig();
    ret = driver_create_file(&hall_driver.driver, &driver_attr_hall_state);
    ret = driver_create_file(&hall_driver.driver, &driver_attr_hall_enable);
    if (ret) {
        HALL_DEBUG("[hall_dev]:%s: sysfs_create_file failed\n", __func__);
    }
#else
    
#ifdef CONFIG_PM_WAKELOCKS
	wakeup_source_init(&hall_key_lock, "hall key wakelock");
#else
	wake_lock_init(&hall_key_lock, WAKE_LOCK_SUSPEND, "hall key wakelock");
#endif

    init_waitqueue_head(&send_event_wq);
    //start send key event thread
    keyEvent_thread = kthread_run(sendKeyEvent, 0, "keyEvent_send");
    if (IS_ERR(keyEvent_thread))
    {
        ret = PTR_ERR(keyEvent_thread);
        HALL_DEBUG("[hall_dev]:failed to create kernel thread: %d\n", ret);
    }

    if(g_hall_first)
    {
        ret = driver_create_file(&hall_driver.driver, &driver_attr_hall_state);
        if (ret)
        {
            HALL_DEBUG("[hall_dev]:%s: sysfs_create_file failed\n", __func__);
            driver_remove_file(&hall_driver.driver, &dev_attr_hall_state.attr);
        }
        hall_setup_eint();
        g_hall_first = 0;
    }
#endif

#ifdef CONFIG_TINNO_PRODUCT_INFO
    FULL_PRODUCT_DEVICE_CB(ID_HALL, get_hall_enable_state, NULL);
#endif

#ifdef HALL_SW_DEBOUCE
    init_timer(&hall_delay_report_timer);
    hall_delay_report_timer.function = hall_delay_timer_fn;
#endif


//hall_enable_set(1);
    return 0;
}

static int hall_remove(struct platform_device *dev)
{
    HALL_DEBUG("[hall_dev]:hall_remove begin!\n");
    driver_remove_file(&hall_driver.driver, &driver_attr_hall_state);
#ifdef HALL_DEVICE_CONTROAL_IMPL
    driver_remove_file(&hall_driver.driver, &driver_attr_hall_enable);
#endif
    input_unregister_device(hall_input_dev);
    HALL_DEBUG("[hall_dev]:hall_remove Done!\n");

    return 0;
}

static int __init hall_init(void)
{
    int ret = 0;
    TRACE_FUNC;

   // return -1;
#ifdef CONFIG_WIKO_UNIFY
    extern int Hall;
    if (Hall == 0)
    {
        HALL_DEBUG("[hall_dev]:Hall == 0!\n");
        return 0;
    }
#endif

#ifdef CONFIG_TINNO_PERSO
    extern int g_iHall_sensor;
    printk(KERN_INFO "Tinnod Board Config, g_iHall_sensor = %d\n", g_iHall_sensor);
    if (g_iHall_sensor == 0)
    {
        HALL_DEBUG("[hall_dev]: g_iHall_sensor == 0! \n");
        return 0;
    }
#endif

//#ifdef CONFIG_MTK_LEGACY
#if defined(CONFIG_OF)
    ret = platform_device_register(&hall_device);
    HALL_DEBUG("[%s]: hall_device, retval=%d \n!", __func__, ret);

    if (ret != 0)
    {
        HALL_DEBUG("platform_device_hall_device_register error:(%d)\n", ret);
        return ret;
    }
    else
    {
        HALL_DEBUG("platform_device_hall_device_register done!\n");
    }
#endif
//#endif
    platform_driver_register(&hall_driver);
#ifdef CONFIG_TINNO_PERSO
    extern int g_iHall_sensor;
    if (g_iHall_sensor == 1)
#endif
    {
        hall_enable_set(1);
    }
    return 0;
}

static void __exit hall_exit(void)
{
    TRACE_FUNC;
    platform_driver_unregister(&hall_driver);
}

module_init(hall_init);
module_exit(hall_exit);
MODULE_DESCRIPTION("HALL DEVICE driver");
MODULE_AUTHOR("liling <ling.li@tinno.com>");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("halldevice");

