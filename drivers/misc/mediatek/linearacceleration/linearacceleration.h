#ifndef __LA_H__
#define __LA_H__


#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/hwmsensor.h>
#include <linux/earlysuspend.h>
#include <linux/hwmsen_dev.h>

/* #define DEBUG */

#ifdef DEBUG
#define LA_TAG					"<LINEARACCEL> "
#define LA_FUN(f)				pr_debug(LA_TAG"%s\n", __func__)
#define LA_ERR(fmt, args...)		pr_err(LA_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define LA_LOG(fmt, args...)		pr_debug(LA_TAG fmt, ##args)
#define LA_VER(fmt, args...)		pr_debug(LA_TAG"%s: "fmt, __func__, ##args)	/* ((void)0) */
#else
#define LA_TAG					"<LINEARACCEL> "
#define LA_FUN(f)
#define LA_ERR(fmt, args...)
#define LA_LOG(fmt, args...)
#define LA_VER(fmt, args...)
#endif
#define OP_LA_DELAY	0X01
#define	OP_LA_ENABLE	0X02
#define	OP_LA_GET_DATA	0X04

#define LA_INVALID_VALUE -1

#define EVENT_TYPE_LA_X					ABS_RX
#define EVENT_TYPE_LA_Y					ABS_Y
#define EVENT_TYPE_LA_Z					ABS_Z
#define EVENT_TYPE_LA_STATUS				REL_X

#define LA_VALUE_MAX (32767)
#define LA_VALUE_MIN (-32768)
#define LA_STATUS_MIN (0)
#define LA_STATUS_MAX (64)
#define LA_DIV_MAX (32767)
#define LA_DIV_MIN (1)
#define LA_AXIS_X 0
#define LA_AXIS_Y 1
#define LA_AXIS_Z 2

#define MAX_CHOOSE_LA_NUM 5
#define LA_AXES_NUM 3
struct la_control_path {
	int (*open_report_data)(int open);	/* open data rerport to HAL */
	int (*enable_nodata)(int en);	/* only enable not report event to HAL */
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);	/* version2.used for flush operate */
	bool is_report_input_direct;
	bool is_support_batch;	/* version2.used for batch mode support flag */
	int (*la_calibration)(int type, int cali[3]);	/* version3 sensor common layer factory mode API1 */
};

struct la_data_path {
	int (*get_data)(int *x, int *y, int *z, int *status);
	int (*get_raw_data)(int *x, int *y, int *z);	/* version3 sensor common layer factory mode API2 */
	int vender_div;
};

struct la_init_info {
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver *platform_diver_addr;
};

struct la_data {
	hwm_sensor_data la_data;
	int data_updata;
	/* struct mutex lock; */
};

struct la_drv_obj {
	void *self;
	int polling;
	int (*la_operate)(void *self, uint32_t command, void *buff_in, int size_in,
			   void *buff_out, int size_out, int *actualout);
};

struct la_context {
	struct input_dev *idev;
	struct miscdevice mdev;
	struct work_struct report;
	struct mutex la_op_mutex;
	atomic_t delay;		/*polling period for reporting input event */
	atomic_t wake;		/*user-space request to wake-up, used with stop */
	struct timer_list timer;	/* polling timer */
	atomic_t trace;

	struct early_suspend early_drv;
	atomic_t early_suspend;
	/* struct la_drv_obj    drv_obj; */
	struct la_data drv_data;
	int cali_sw[LA_AXES_NUM + 1];
	struct la_control_path la_ctl;
	struct la_data_path la_data;
	bool is_active_nodata;	/* Active, but HAL don't need data sensor. such as orientation need */
	bool is_active_data;	/* Active and HAL need data . */
	bool is_first_data_after_enable;
	bool is_polling_run;
	bool is_batch_enable;	/* version2.this is used for judging whether sensor is in batch mode */
};

/* driver API for internal */
/* extern int la_enable_nodata(int enable); */
/* extern int la_attach(struct la_drv_obj *obj); */
/* driver API for third party vendor */

/* for auto detect */
extern int la_driver_add(struct la_init_info *obj);
extern int la_data_report(int x, int y, int z, int status);
extern int la_register_control_path(struct la_control_path *ctl);
extern int la_register_data_path(struct la_data_path *data);
#endif
