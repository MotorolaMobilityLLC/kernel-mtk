
#ifndef __CAPTOUCH_H__
#define __CAPTOUCH_H__


#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>

#include <batch.h>
#include <sensors_io.h>
#include <hwmsensor.h>
#include <hwmsen_dev.h>

#define CAPTOUCH_TAG					"<CAPTOUCH> "
#define CAPTOUCH_FUN(f)			printk(CAPTOUCH_TAG"%s\n", __func__)
#define CAPTOUCH_ERR(fmt, args...)	printk(CAPTOUCH_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define CAPTOUCH_LOG(fmt, args...)	printk(CAPTOUCH_TAG fmt, ##args)
#define CAPTOUCH_VER(fmt, args...)   printk(CAPTOUCH_TAG"%s: "fmt, __func__, ##args) //((void)0)

#define CAPTOUCH_INVALID_VALUE -1

#define EVENT_TYPE_CAPTOUCH_VALUE         		REL_Z
#define EVENT_TYPE_CAPTOUCH_STATUS        		REL_Y

#define MAX_CHOOSE_CAPTOUCH_NUM 5

struct captouch_control_path
{
	int (*open_report_data)(int open);//open data rerport to HAL
	int (*enable_nodata)(int en);//only enable not report event to HAL
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);//version2.used for flush operate
	int (*cap_calibration)(int type, int value);
	int (*cap_threshold_setting)(int type, int value[2]);
	bool is_report_input_direct;
	bool is_support_batch;//version2.used for batch mode support flag
	bool is_polling_mode;
	bool is_use_common_factory;
};

struct captouch_init_info
{
    	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver* platform_diver_addr;
};

struct captouch_data {
	struct hwm_sensor_data cap_data;
};

struct captouch_context {
	struct input_dev   		*idev;
	struct miscdevice   	mdev;
	struct work_struct  	report_captouch;
	struct mutex			captouch_op_mutex;
	struct timer_list		timer_captouch;

	atomic_t				delay_captouch;

	atomic_t                	early_suspend;

	struct captouch_data	drv_data;
	struct captouch_control_path captouch_ctl;

	bool is_captouch_first_data_after_enable;
	bool is_captouch_polling_run;
	bool is_captouch_batch_enable;
	bool is_get_valid_captouch_data_after_enable;
};

//for auto detect
extern int captouch_driver_add(struct captouch_init_info* obj) ;
extern int captouch_report_interrupt_data(int value);
extern int captouch_data_report(struct input_dev *dev, int value,int status);
extern int captouch_register_control_path(struct captouch_control_path *ctl);
extern struct platform_device *get_captouch_platformdev(void);
extern int captouch_eint_status;

#endif
