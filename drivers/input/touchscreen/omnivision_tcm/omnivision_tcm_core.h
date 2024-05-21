/*
 * Omnivision TCM touchscreen driver
 *
 * Copyright (C) 2017-2018 OmniVision Incorporated. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND OmniVision
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL OmniVision BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF OmniVision WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION DOES
 * NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES, OmniVision'
 * TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S.
 * DOLLARS.
 */

#ifndef _OMNIVISION_TCM_CORE_H_
#define _OMNIVISION_TCM_CORE_H_

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#ifdef CONFIG_FB
#include <linux/fb.h>
#include <linux/notifier.h>
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/printk.h>
#include <linux/power_supply.h>

#define I2C_MODULE_NAME "omniVision_tcm_i2c"
#define SPI_MODULE_NAME "omnivision_tcm_spi"

#define TD4160_INFO_PROC_FILE "tp_info"
#define TDINFO_NAME "tp_wake_switch"
#define OVT_CHARGER_NOTIFIER (1)
extern int td_ito_test(void);
extern void ovt_tcm_charger_init(void);
struct ovt_tcm_tp_info{
	char tp_firmware_name[30];
	char tp_module_name[20];
	char tp_limits_name[30];
};
struct ovt_tcm_board_data {
	bool x_flip;
	bool y_flip;
	bool swap_axes;
	int irq_gpio;
	int irq_on_state;
	int power_gpio;
	int power_on_state;
	int reset_gpio;
	int reset_on_state;
	int tpio_reset_gpio;
	unsigned int spi_mode;
	unsigned int power_delay_ms;
	unsigned int reset_delay_ms;
	unsigned int reset_active_ms;
	unsigned int byte_delay_us;
	unsigned int block_delay_us;
	unsigned int ubl_i2c_addr;
	unsigned int ubl_max_freq;
	unsigned int ubl_byte_delay_us;
	unsigned long irq_flags;
	const char *pwr_reg_name;
	const char *bus_reg_name;
	struct mutex touch_mutex;
#if OVT_CHARGER_NOTIFIER
/* add_for_charger_start */
	struct notifier_block notifier_charger;
	struct workqueue_struct *charger_notify_wq;
	struct work_struct	update_charger;
	int usb_plug_status;
/*  add_for_charger_end  */
#endif
};
#if OVT_CHARGER_NOTIFIER
extern struct ovt_tcm_board_data *bdatas;
#endif
#define OMNIVISION_TCM_ID_PRODUCT (1 << 0)
#define OMNIVISION_TCM_ID_VERSION 0x0300
#define OMNIVISION_TCM_ID_SUBVERSION 2

#define PLATFORM_DRIVER_NAME "omnivision_tcm"

#define TOUCH_INPUT_NAME "omnivision_tcm_touch"
#define TOUCH_INPUT_PHYS_PATH "omnivision_tcm/touch_input"

#define WAKEUP_GESTURE (0)

#define SPEED_UP_RESUME 1

/* The chunk size RD_CHUNK_SIZE/WR_CHUNK_SIZE will not apply in HDL sensors */
#define RD_CHUNK_SIZE 256 /* read length limit in bytes, 0 = unlimited */
#define WR_CHUNK_SIZE 256 /* write length limit in bytes, 0 = unlimited */
#define HDL_RD_CHUNK_SIZE 0 /* For HDL, 0 = unlimited */
#define HDL_WR_CHUNK_SIZE 0 /* For HDL, 0 = unlimited */

#define MESSAGE_HEADER_SIZE 4
#define MESSAGE_MARKER 0xa5
#define MESSAGE_PADDING 0x5a

/*
#define REPORT_NOTIFIER
*/

/*
#define WATCHDOG_SW
*/
#ifdef WATCHDOG_SW
#define RUN_WATCHDOG false
#define WATCHDOG_TRIGGER_COUNT 2
#define WATCHDOG_DELAY_MS 1000
#endif

#define HOST_DOWNLOAD_WAIT_MS 100
#define HOST_DOWNLOAD_TIMEOUT_MS 5000

#define LOGx(func, dev, log, ...) \
	func(dev, "%s info: " log, __func__, ##__VA_ARGS__)

#define LOGy(func, dev, log, ...) \
	func(dev, "%s error: (line %d) " log, __func__, __LINE__, ##__VA_ARGS__)

#define LOGD(dev, log, ...) LOGx(dev_dbg, dev, log, ##__VA_ARGS__)
#define LOGI(dev, log, ...) LOGx(dev_info, dev, log, ##__VA_ARGS__)
#define LOGN(dev, log, ...) LOGx(dev_notice, dev, log, ##__VA_ARGS__)
#define LOGW(dev, log, ...) LOGy(dev_warn, dev, log, ##__VA_ARGS__)
#define LOGE(dev, log, ...) LOGy(dev_err, dev, log, ##__VA_ARGS__)

#define INIT_BUFFER(buffer, is_clone) \
	mutex_init(&buffer.buf_mutex); \
	buffer.clone = is_clone

#define LOCK_BUFFER(buffer) \
	mutex_lock(&buffer.buf_mutex)

#define UNLOCK_BUFFER(buffer) \
	mutex_unlock(&buffer.buf_mutex)

#define RELEASE_BUFFER(buffer) \
	do { \
		if (buffer.clone == false) { \
			kfree(buffer.buf); \
			buffer.buf_size = 0; \
			buffer.data_length = 0; \
		} \
	} while (0)

#define MAX(a, b) \
	({__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a > _b ? _a : _b; })

#define MIN(a, b) \
	({__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a < _b ? _a : _b; })

#define STR(x) #x

#define CONCAT(a, b) a##b

#define IS_NOT_FW_MODE(mode) \
	((mode != MODE_APPLICATION_FIRMWARE) && (mode != MODE_HOSTDOWNLOAD_FIRMWARE))

#define IS_FW_MODE(mode) \
	((mode == MODE_APPLICATION_FIRMWARE) || (mode == MODE_HOSTDOWNLOAD_FIRMWARE))

#define SHOW_PROTOTYPE(m_name, a_name) \
static ssize_t CONCAT(m_name##_sysfs, _##a_name##_show)(struct device *dev, \
		struct device_attribute *attr, char *buf); \
\
static struct device_attribute dev_attr_##a_name = \
		__ATTR(a_name, S_IRUGO, \
		CONCAT(m_name##_sysfs, _##a_name##_show), \
		ovt_tcm_store_error);

#define STORE_PROTOTYPE(m_name, a_name) \
static ssize_t CONCAT(m_name##_sysfs, _##a_name##_store)(struct device *dev, \
		struct device_attribute *attr, const char *buf, size_t count); \
\
static struct device_attribute dev_attr_##a_name = \
		__ATTR(a_name, (S_IWUSR | S_IWGRP), \
		ovt_tcm_show_error, \
		CONCAT(m_name##_sysfs, _##a_name##_store));

#define SHOW_STORE_PROTOTYPE(m_name, a_name) \
static ssize_t CONCAT(m_name##_sysfs, _##a_name##_show)(struct device *dev, \
		struct device_attribute *attr, char *buf); \
\
static ssize_t CONCAT(m_name##_sysfs, _##a_name##_store)(struct device *dev, \
		struct device_attribute *attr, const char *buf, size_t count); \
\
static struct device_attribute dev_attr_##a_name = \
		__ATTR(a_name, (S_IRUGO | S_IWUSR | S_IWGRP), \
		CONCAT(m_name##_sysfs, _##a_name##_show), \
		CONCAT(m_name##_sysfs, _##a_name##_store));

#define ATTRIFY(a_name) (&dev_attr_##a_name)

enum module_type {
	TCM_ZEROFLASH = 0,
	TCM_REFLASH = 1,
	TCM_DEVICE = 2,
	TCM_TESTING = 3,
	TCM_RECOVERY = 4,
	TCM_DIAGNOSTICS = 5,
	TCM_LAST,
};

enum boot_mode {
	MODE_APPLICATION_FIRMWARE = 0x01,
	MODE_HOSTDOWNLOAD_FIRMWARE = 0x02,
	MODE_ROMBOOTLOADER = 0x04,
	MODE_BOOTLOADER = 0x0b,
	MODE_TDDI_BOOTLOADER = 0x0c,
	MODE_TDDI_HOSTDOWNLOAD_BOOTLOADER = 0x0d,
	MODE_PRODUCTIONTEST_FIRMWARE = 0x0e,
};

enum sensor_types {
	TYPE_UNKNOWN = 0,
	TYPE_FLASH = 1,
	TYPE_F35 = 2,
	TYPE_ROMBOOT = 3,
};

enum boot_status {
	BOOT_STATUS_OK = 0x00,
	BOOT_STATUS_BOOTING = 0x01,
	BOOT_STATUS_APP_BAD_DISPLAY_CRC = 0xfc,
	BOOT_STATUS_BAD_DISPLAY_CONFIG = 0xfd,
	BOOT_STATUS_BAD_APP_FIRMWARE = 0xfe,
	BOOT_STATUS_WARM_BOOT = 0xff,
};

enum app_status {
	APP_STATUS_OK = 0x00,
	APP_STATUS_BOOTING = 0x01,
	APP_STATUS_UPDATING = 0x02,
	APP_STATUS_BAD_APP_CONFIG = 0xff,
};

enum firmware_mode {
	FW_MODE_BOOTLOADER = 0,
	FW_MODE_APPLICATION = 1,
	FW_MODE_PRODUCTION_TEST = 2,
};

enum dynamic_config_id {
	DC_UNKNOWN = 0x00,
	DC_NO_DOZE,
	DC_DISABLE_NOISE_MITIGATION,
	DC_INHIBIT_FREQUENCY_SHIFT,
	DC_REQUESTED_FREQUENCY,
	DC_DISABLE_HSYNC,
	DC_REZERO_ON_EXIT_DEEP_SLEEP,
	DC_CHARGER_CONNECTED,
	DC_NO_BASELINE_RELAXATION,
	DC_IN_WAKEUP_GESTURE_MODE,
	DC_STIMULUS_FINGERS,
	DC_GRIP_SUPPRESSION_ENABLED,
	DC_ENABLE_THICK_GLOVE,
	DC_ENABLE_GLOVE,
};

enum command {
	CMD_NONE = 0x00,
	CMD_CONTINUE_WRITE = 0x01,
	CMD_IDENTIFY = 0x02,
	CMD_RESET = 0x04,
	CMD_ENABLE_REPORT = 0x05,
	CMD_DISABLE_REPORT = 0x06,
	CMD_GET_BOOT_INFO = 0x10,
	CMD_ERASE_FLASH = 0x11,
	CMD_WRITE_FLASH = 0x12,
	CMD_READ_FLASH = 0x13,
	CMD_RUN_APPLICATION_FIRMWARE = 0x14,
	CMD_SPI_MASTER_WRITE_THEN_READ = 0x15,
	CMD_REBOOT_TO_ROM_BOOTLOADER = 0x16,
	CMD_RUN_BOOTLOADER_FIRMWARE = 0x1f,
	CMD_GET_APPLICATION_INFO = 0x20,
	CMD_GET_STATIC_CONFIG = 0x21,
	CMD_SET_STATIC_CONFIG = 0x22,
	CMD_GET_DYNAMIC_CONFIG = 0x23,
	CMD_SET_DYNAMIC_CONFIG = 0x24,
	CMD_GET_TOUCH_REPORT_CONFIG = 0x25,
	CMD_SET_TOUCH_REPORT_CONFIG = 0x26,
	CMD_REZERO = 0x27,
	CMD_COMMIT_CONFIG = 0x28,
	CMD_DESCRIBE_DYNAMIC_CONFIG = 0x29,
	CMD_PRODUCTION_TEST = 0x2a,
	CMD_SET_CONFIG_ID = 0x2b,
	CMD_ENTER_DEEP_SLEEP = 0x2c,
	CMD_EXIT_DEEP_SLEEP = 0x2d,
	CMD_GET_TOUCH_INFO = 0x2e,
	CMD_GET_DATA_LOCATION = 0x2f,
	CMD_DOWNLOAD_CONFIG = 0x30,
	CMD_ENTER_PRODUCTION_TEST_MODE = 0x31,
	CMD_GET_FEATURES = 0x32,
	CMD_GET_ROMBOOT_INFO = 0x40,
	CMD_WRITE_PROGRAM_RAM = 0x41,
	CMD_ROMBOOT_RUN_BOOTLOADER_FIRMWARE = 0x42,
	CMD_SPI_MASTER_WRITE_THEN_READ_EXTENDED = 0x43,
	CMD_ENTER_IO_BRIDGE_MODE = 0x44,
	CMD_ROMBOOT_DOWNLOAD = 0x45,
};

enum status_code {
	STATUS_IDLE = 0x00,
	STATUS_OK = 0x01,
	STATUS_BUSY = 0x02,
	STATUS_CONTINUED_READ = 0x03,
	STATUS_NOT_EXECUTED_IN_DEEP_SLEEP = 0x0b,
	STATUS_RECEIVE_BUFFER_OVERFLOW = 0x0c,
	STATUS_PREVIOUS_COMMAND_PENDING = 0x0d,
	STATUS_NOT_IMPLEMENTED = 0x0e,
	STATUS_ERROR = 0x0f,
	STATUS_INVALID = 0xff,
};

enum report_type {
	REPORT_IDENTIFY = 0x10,
	REPORT_TOUCH = 0x11,
	REPORT_DELTA = 0x12,
	REPORT_RAW = 0x13,
	REPORT_STATUS = 0x1b,
	REPORT_PRINTF = 0x82,
	REPORT_FW_PRINTF = 0x84,
	REPORT_HDL_ROMBOOT = 0xfd,
	REPORT_HDL_F35 = 0xfe,
};

enum command_status {
	CMD_IDLE = 0,
	CMD_BUSY = 1,
	CMD_ERROR = -1,
};

enum flash_area {
	BOOTLOADER = 0,
	BOOT_CONFIG,
	APP_FIRMWARE,
	APP_CONFIG,
	DISP_CONFIG,
	CUSTOM_OTP,
	CUSTOM_LCM,
	CUSTOM_OEM,
	PPDT,
};

enum flash_data {
	LCM_DATA = 1,
	OEM_DATA,
	PPDT_DATA,
};

enum helper_task {
	HELP_NONE = 0,
	HELP_RUN_APPLICATION_FIRMWARE,
	HELP_SEND_REINIT_NOTIFICATION,
	HELP_TOUCH_REINIT,
	HELP_SEND_ROMBOOT_HDL,
};

struct ovt_tcm_helper {
	atomic_t task;
	struct work_struct work;
	struct workqueue_struct *workqueue;
	struct completion *helper_completion;
};

struct ovt_tcm_watchdog {
	bool run;
	unsigned char count;
	struct delayed_work work;
	struct workqueue_struct *workqueue;
};

struct ovt_tcm_buffer {
	bool clone;
	unsigned char *buf;
	unsigned int buf_size;
	unsigned int data_length;
	struct mutex buf_mutex;
};

struct ovt_tcm_report {
	unsigned char id;
	struct ovt_tcm_buffer buffer;
};

struct ovt_tcm_identification {
	unsigned char version;
	unsigned char mode;
	unsigned char part_number[16];
	unsigned char build_id[4];
	unsigned char max_write_size[2];
};

struct ovt_tcm_boot_info {
	unsigned char version;
	unsigned char status;
	unsigned char asic_id[2];
	unsigned char write_block_size_words;
	unsigned char erase_page_size_words[2];
	unsigned char max_write_payload_size[2];
	unsigned char last_reset_reason;
	unsigned char pc_at_time_of_last_reset[2];
	unsigned char boot_config_start_block[2];
	unsigned char boot_config_size_blocks[2];
	unsigned char display_config_start_block[4];
	unsigned char display_config_length_blocks[2];
	unsigned char backup_display_config_start_block[4];
	unsigned char backup_display_config_length_blocks[2];
	unsigned char custom_otp_start_block[2];
	unsigned char custom_otp_length_blocks[2];
};

struct ovt_tcm_app_info {
	unsigned char version[2];
	unsigned char status[2];
	unsigned char static_config_size[2];
	unsigned char dynamic_config_size[2];
	unsigned char app_config_start_write_block[2];
	unsigned char app_config_size[2];
	unsigned char max_touch_report_config_size[2];
	unsigned char max_touch_report_payload_size[2];
	unsigned char customer_config_id[16];
	unsigned char max_x[2];
	unsigned char max_y[2];
	unsigned char max_objects[2];
	unsigned char num_of_buttons[2];
	unsigned char num_of_image_rows[2];
	unsigned char num_of_image_cols[2];
	unsigned char has_hybrid_data[2];
	unsigned char num_of_force_elecs[2];
};

struct ovt_tcm_romboot_info {
	unsigned char version;
	unsigned char status;
	unsigned char asic_id[2];
	unsigned char write_block_size_words;
	unsigned char max_write_payload_size[2];
	unsigned char last_reset_reason;
	unsigned char pc_at_time_of_last_reset[2];
};

struct ovt_tcm_touch_info {
	unsigned char image_2d_scale_factor[4];
	unsigned char image_0d_scale_factor[4];
	unsigned char hybrid_x_scale_factor[4];
	unsigned char hybrid_y_scale_factor[4];
};

struct ovt_tcm_message_header {
	unsigned char marker;
	unsigned char code;
	unsigned char length[2];
};

struct ovt_tcm_features {
	unsigned char byte_0_reserved;
	unsigned char byte_1_reserved;
	unsigned char dual_firmware:1;
	unsigned char byte_2_reserved:7;
} __packed;

struct ovt_tcm_hcd {
	pid_t isr_pid;
	atomic_t command_status;
	atomic_t host_downloading;
	atomic_t firmware_flashing;
	wait_queue_head_t hdl_wq;
	wait_queue_head_t reflash_wq;
	int irq;
	bool do_polling;
	bool ever_hdl_done_flag;
	bool in_suspend;
	bool irq_enabled;
	bool in_hdl_mode;
	bool is_detected;
	bool wakeup_gesture_enabled;
    bool ovt_tcm_driver_removing;
	unsigned char sensor_type;
	unsigned char fb_ready;
	unsigned char command;
	unsigned char async_report_id;
	unsigned char status_report_code;
	unsigned char response_code;
	unsigned int read_length;
	unsigned int payload_length;
	unsigned int packrat_number;
	unsigned int rd_chunk_size;
	unsigned int wr_chunk_size;
	unsigned int app_status;
	struct platform_device *pdev;
	struct regulator *pwr_reg;
	struct regulator *bus_reg;
	struct kobject *sysfs_dir;
	struct kobject *dynamnic_config_sysfs_dir;
	struct mutex extif_mutex;
	struct mutex reset_mutex;
	struct mutex irq_en_mutex;
	struct mutex io_ctrl_mutex;
	struct mutex rw_ctrl_mutex;
	struct mutex command_mutex;
	struct mutex identify_mutex;
	struct delayed_work polling_work;
	struct workqueue_struct *polling_workqueue;
	struct task_struct *notifier_thread;
#ifdef CONFIG_FB
	struct notifier_block fb_notifier;
#endif
	struct ovt_tcm_buffer in;
	struct ovt_tcm_buffer out;
	struct ovt_tcm_buffer resp;
	struct ovt_tcm_buffer temp;
	struct ovt_tcm_buffer config;
	struct ovt_tcm_report report;
	struct ovt_tcm_app_info app_info;
	struct ovt_tcm_boot_info boot_info;
	struct ovt_tcm_romboot_info romboot_info;
	struct ovt_tcm_touch_info touch_info;
	struct ovt_tcm_identification id_info;
	struct ovt_tcm_helper helper;
#if SPEED_UP_RESUME
	struct work_struct     speed_up_work;
	struct workqueue_struct *speed_up_resume_workqueue;
#endif
	struct ovt_tcm_watchdog watchdog;
	struct ovt_tcm_features features;
	const struct ovt_tcm_hw_interface *hw_if;
	int (*reset)(struct ovt_tcm_hcd *tcm_hcd);
	int (*reset_n_reinit)(struct ovt_tcm_hcd *tcm_hcd, bool hw, bool update_wd);
	int (*sleep)(struct ovt_tcm_hcd *tcm_hcd, bool en);
	int (*identify)(struct ovt_tcm_hcd *tcm_hcd, bool id);
	int (*enable_irq)(struct ovt_tcm_hcd *tcm_hcd, bool en, bool ns);
	int (*switch_mode)(struct ovt_tcm_hcd *tcm_hcd,
			enum firmware_mode mode);
	int (*read_message)(struct ovt_tcm_hcd *tcm_hcd,
			unsigned char *in_buf, unsigned int length);
	int (*write_message)(struct ovt_tcm_hcd *tcm_hcd,
			unsigned char command, unsigned char *payload,
			unsigned int length, unsigned char **resp_buf,
			unsigned int *resp_buf_size, unsigned int *resp_length,
			unsigned char *response_code,
			unsigned int polling_delay_ms);
	int (*get_dynamic_config)(struct ovt_tcm_hcd *tcm_hcd,
			enum dynamic_config_id id, unsigned short *value);
	int (*set_dynamic_config)(struct ovt_tcm_hcd *tcm_hcd,
			enum dynamic_config_id id, unsigned short value);
	int (*get_data_location)(struct ovt_tcm_hcd *tcm_hcd,
			enum flash_area area, unsigned int *addr,
			unsigned int *length);
	int (*read_flash_data)(enum flash_area area, bool run_app_firmware,
			struct ovt_tcm_buffer *output);
	void (*report_touch)(void);
	void (*update_watchdog)(struct ovt_tcm_hcd *tcm_hcd, bool en);
};

struct ovt_tcm_module_cb {
	enum module_type type;
	int (*init)(struct ovt_tcm_hcd *tcm_hcd);
	int (*remove)(struct ovt_tcm_hcd *tcm_hcd);
	int (*syncbox)(struct ovt_tcm_hcd *tcm_hcd);
#ifdef REPORT_NOTIFIER
	int (*asyncbox)(struct ovt_tcm_hcd *tcm_hcd);
#endif
	int (*reinit)(struct ovt_tcm_hcd *tcm_hcd);
	int (*suspend)(struct ovt_tcm_hcd *tcm_hcd);
	int (*resume)(struct ovt_tcm_hcd *tcm_hcd);
	int (*early_suspend)(struct ovt_tcm_hcd *tcm_hcd);
};

struct ovt_tcm_module_handler {
	bool insert;
	bool detach;
	struct list_head link;
	struct ovt_tcm_module_cb *mod_cb;
};

struct ovt_tcm_module_pool {
	bool initialized;
	bool queue_work;
	struct mutex mutex;
	struct list_head list;
	struct work_struct work;
	struct workqueue_struct *workqueue;
	struct ovt_tcm_hcd *tcm_hcd;
};

struct ovt_tcm_bus_io {
	unsigned char type;
	int (*rmi_read)(struct ovt_tcm_hcd *tcm_hcd, unsigned short addr,
			unsigned char *data, unsigned int length);
	int (*rmi_write)(struct ovt_tcm_hcd *tcm_hcd, unsigned short addr,
			unsigned char *data, unsigned int length);
	int (*read)(struct ovt_tcm_hcd *tcm_hcd, unsigned char *data,
			unsigned int length);
	int (*write)(struct ovt_tcm_hcd *tcm_hcd, unsigned char *data,
			unsigned int length);
};

struct ovt_tcm_hw_interface {
	struct ovt_tcm_board_data *bdata;
	const struct ovt_tcm_bus_io *bus_io;
};

int ovt_tcm_bus_init(void);

void ovt_tcm_bus_exit(void);

int ovt_tcm_add_module(struct ovt_tcm_module_cb *mod_cb, bool insert);

int touch_init(struct ovt_tcm_hcd *tcm_hcd);
int touch_remove(struct ovt_tcm_hcd *tcm_hcd);
int touch_reinit(struct ovt_tcm_hcd *tcm_hcd);
int touch_early_suspend(struct ovt_tcm_hcd *tcm_hcd);
int touch_suspend(struct ovt_tcm_hcd *tcm_hcd);
int touch_resume(struct ovt_tcm_hcd *tcm_hcd);

static inline int ovt_tcm_rmi_read(struct ovt_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	return tcm_hcd->hw_if->bus_io->rmi_read(tcm_hcd, addr, data, length);
}

static inline int ovt_tcm_rmi_write(struct ovt_tcm_hcd *tcm_hcd,
		unsigned short addr, unsigned char *data, unsigned int length)
{
	return tcm_hcd->hw_if->bus_io->rmi_write(tcm_hcd, addr, data, length);
}

static inline int ovt_tcm_read(struct ovt_tcm_hcd *tcm_hcd,
		unsigned char *data, unsigned int length)
{
	return tcm_hcd->hw_if->bus_io->read(tcm_hcd, data, length);
}

static inline int ovt_tcm_write(struct ovt_tcm_hcd *tcm_hcd,
		unsigned char *data, unsigned int length)
{
	return tcm_hcd->hw_if->bus_io->write(tcm_hcd, data, length);
}

static inline ssize_t ovt_tcm_show_error(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	pr_err("%s: Attribute not readable\n",
			__func__);

	return -EPERM;
}

static inline ssize_t ovt_tcm_store_error(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	pr_err("%s: Attribute not writable\n",
			__func__);

	return -EPERM;
}

static inline int secure_memcpy(unsigned char *dest, unsigned int dest_size,
		const unsigned char *src, unsigned int src_size,
		unsigned int count)
{
	if (dest == NULL || src == NULL)
		return -EINVAL;

	if (count > dest_size || count > src_size) {
		pr_err("%s: src_size = %d, dest_size = %d, count = %d\n",
				__func__, src_size, dest_size, count);
		return -EINVAL;
	}

	memcpy((void *)dest, (const void *)src, count);

	return 0;
}

static inline int ovt_tcm_realloc_mem(struct ovt_tcm_hcd *tcm_hcd,
		struct ovt_tcm_buffer *buffer, unsigned int size)
{
	int retval;
	unsigned char *temp;

	if (size > buffer->buf_size) {
		temp = buffer->buf;

		buffer->buf = kmalloc(size, GFP_KERNEL);
		if (!(buffer->buf)) {
			dev_err(tcm_hcd->pdev->dev.parent,
					"%s: Failed to allocate memory\n",
					__func__);
			kfree(temp);
			buffer->buf_size = 0;
			return -ENOMEM;
		}

		retval = secure_memcpy(buffer->buf,
				size,
				temp,
				buffer->buf_size,
				buffer->buf_size);
		if (retval < 0) {
			dev_err(tcm_hcd->pdev->dev.parent,
					"%s: Failed to copy data\n",
					__func__);
			kfree(temp);
			kfree(buffer->buf);
			buffer->buf_size = 0;
			return retval;
		}

		kfree(temp);
		buffer->buf_size = size;
	}

	return 0;
}

static inline int ovt_tcm_alloc_mem(struct ovt_tcm_hcd *tcm_hcd,
		struct ovt_tcm_buffer *buffer, unsigned int size)
{
	if (size > buffer->buf_size) {
		unsigned char *temp;
		temp = buffer->buf;
		buffer->buf = kmalloc(size, GFP_KERNEL);
		if (!(buffer->buf)) {
			dev_err(tcm_hcd->pdev->dev.parent,
					"%s: Failed to allocate memory\n",
					__func__);
			dev_err(tcm_hcd->pdev->dev.parent,
					"%s: Allocation size = %d\n",
					__func__, size);
			buffer->buf = temp;
			return -ENOMEM;
		}
		if (temp && buffer->buf_size) {
			kfree(temp);
		}
		buffer->buf_size = size;
	}

	memset(buffer->buf, 0x00, buffer->buf_size);
	buffer->data_length = 0;

	return 0;
}

static inline unsigned int le2_to_uint(const unsigned char *src)
{
	return (unsigned int)src[0] +
			(unsigned int)src[1] * 0x100;
}

static inline unsigned int le4_to_uint(const unsigned char *src)
{
	return (unsigned int)src[0] +
			(unsigned int)src[1] * 0x100 +
			(unsigned int)src[2] * 0x10000 +
			(unsigned int)src[3] * 0x1000000;
}

static inline unsigned int ceil_div(unsigned int dividend, unsigned divisor)
{
	return (dividend + divisor - 1) / divisor;
}

#endif

