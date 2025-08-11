/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-06-14 File created.
 */

#ifndef __FRSM_DEV_H__
#define __FRSM_DEV_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/pinctrl/consumer.h>

#define FRSM_DRV_2IN1_SUPPORT 1
#define CONFIG_SND_SOC_FRSM_AMP 1
#define FRSM_I2CA_EXPORT_SUPPORT 1

#define FRSM_I2C_VERSION "v5.2.2.3"

#define FRSM_STRING_MAX   (32)
#define FRSM_DEV_MAX      (8)
#define FRSM_I2C_RETRY    (5)
#define FRSM_I2C_MDELAY   (2)
#define FRSM_WAIT_TIMES   (20)
#define FRSM_VOLUME_MAX   (0x3FF)
#define TABLE_NAME_LEN    (4)
#define HIGH8(x)          (((x) >> 8) & 0xFF)
#define LOW8(x)           ((x) & 0xFF)
#define HIGH16(x)         (((x) >> 16) & 0xFFFF)
#define LOW16(x)          ((x) & 0xFFFF)
#define UINT16_MAX        (0xFFFF)
#define UINT32_MAX        (0xFFFFFFFF)
#define ENUM_TO_STR(x)    (#x)
#define FRSM_REPROBE_MAX  (1)

#define FRSM_REG_DEVID    (0x03)
#define FRSM_REG_DELAY    (0xFF)
#define FRSM_REG_BURST    (0xFE)
#define FRSM_REG_UPDATE   (0xFD)
#define FRSM_REG_MAX      (0xF8)

#define FRSM_HAS_IRQ      BIT(0)
#define FRSM_SWAP_CHN     BIT(1)

#define FRSM_DELAY_US(x) \
	do { \
		if ((x) > 0) \
			usleep_range((x), (x)+10); \
	} while (0)
#define FRSM_DELAY_MS(x) FRSM_DELAY_US((x) * 1000)
#define FRSM_FUNC_EXIT(dev, ret) \
	do { \
		if (ret) \
			dev_err(dev, "%s: ret:%d\n", __func__, ret); \
	} while (0)

#define KERNEL_VERSION_HIGHER(a, b, c) \
	(KERNEL_VERSION((a), (b), (c)) <= LINUX_VERSION_CODE)

#if KERNEL_VERSION_HIGHER(6, 1, 0)
#define i2c_remove_type void
#define i2c_remove_val
#else
#define i2c_remove_type int
#define i2c_remove_val (0)
#endif

#if KERNEL_VERSION_HIGHER(6, 4, 0)
#define const_t const
#else
#define const_t
#endif

enum frsm_devid {
	FRSM_DEVID_FS18DH = 0x05,
	FRSM_DEVID_FS19NH = 0x17,
	FRSM_DEVID_FS19AM = 0x29,
	FRSM_DEVID_FS18YN = 0x31,
	FRSM_DEVID_FS18HS = 0x34,
	FRSM_DEVID_FS19MS = 0x37,
	FRSM_DEVID_FS19SG = 0x43,
	FRSM_DEVID_FS18SF = 0x57,
	FRSM_DEVID_MAX,
};

enum frsm_event {
	EVENT_DEV_INIT = 0,
	EVENT_SET_SCENE,
	EVENT_HW_PARAMS,
	EVENT_START_UP,
	EVENT_STREAM_ON,
	EVENT_STAT_MNTR,
	EVENT_STREAM_OFF,
	EVENT_SHUT_DOWN,
	EVENT_SET_IDLE,
	EVENT_SET_CHANN,
	EVENT_SET_VOL,
	EVENT_GET_DTYPE,
	EVENT_MAX,
	STATE_TUNING,
	STATE_TS_ON,
	STATE_LNM_ON,
	STATE_MAX,
};

enum frsm_pin {
	FRSM_PIN_SDZ = 0,
	FRSM_PIN_INTZ,
	FRSM_PIN_MOD,
	FRSM_PIN_MAX,
};

enum frsm_index_type {
	INDEX_INFO = 0,
	INDEX_STCOEF,
	INDEX_SCENE,
	INDEX_MODEL,
	INDEX_REG,
	INDEX_EFFECT,
	INDEX_STRING,
	INDEX_MAX,
};

enum frsm_prot_type {
	FRSM_PROT_BSG = 0,
	FRSM_PROT_CSG,
	FRSM_PROT_MAX,
};

#pragma pack(push, 1)
struct cmd_pkg {
	uint8_t cmd;
	uint8_t buf[];
};

struct reg_val {
	uint8_t reg;
	uint16_t val;
};

struct reg_update {
	uint8_t cmd;
	uint8_t reg;
	uint16_t val;
	uint16_t mask;
};

struct reg_burst {
	uint8_t cmd;
	uint8_t size;
	uint8_t buf[];
};

struct fwm_index {
	uint16_t type;
	uint16_t offset;
};

struct fwm_table {
	char name[TABLE_NAME_LEN];
	uint16_t size;
	char buf[];
};

struct scene_table {
	uint16_t name;
	uint16_t reg;
	uint16_t model;
	uint16_t effect;
};

struct reg_table {
	uint16_t size;
	uint8_t buf[];
};

struct file_table {
	uint16_t name;
	uint16_t size;
	uint8_t buf[];
};

struct fwm_date {
	uint32_t year  : 12;
	uint32_t month : 4;
	uint32_t day   : 5;
	uint32_t hour  : 5;
	uint32_t minute: 6;
};

struct fwm_header {
	uint16_t version;
	uint16_t project;
	uint16_t device;
	struct fwm_date date;
	uint16_t crc16;
	uint16_t crc_size;
	uint16_t chip_type;
	uint16_t addr;
	/*rsvd[]: [0]:spkr_id */
	uint16_t rsvd[7];
	uint8_t params[];
};
#pragma pack(pop)

struct live_data {
	int spkre;
	int spkr0;
	int spkt0;
	int spkf0;
	int spkQ;
};

struct spkr_info {
	int ndev;
	struct live_data data[FRSM_DEV_MAX];
};

struct frsm_rate {
	unsigned int rate;
	uint16_t i2ssr;
};

struct frsm_pll {
	unsigned int bclk;
	uint16_t pll1;
	uint16_t pll2;
	uint16_t pll3;
};

struct frsm_format {
	char pcm_format;
	char i2sf;
};

struct frsm_hw_params {
	int bclk;
	int rate;
	uint8_t channels;
	uint8_t bit_width;
	uint8_t format;
	uint8_t offset;
};

struct frsm_argv {
	int size;
	void *buf;
};

struct frsm_dev;

struct frsm_ops {
	int (*dev_init)(struct frsm_dev *frsm_dev);
	int (*set_scene)(struct frsm_dev *frsm_dev, struct scene_table *stbl);
	int (*set_volume)(struct frsm_dev *frsm_dev, uint16_t volume);
	int (*hw_params)(struct frsm_dev *frsm_dev);
	int (*start_up)(struct frsm_dev *frsm_dev);
	int (*set_mute)(struct frsm_dev *frsm_dev, int mute);
	int (*set_tsmute)(struct frsm_dev *frsm_dev, bool mute);
	int (*set_channel)(struct frsm_dev *frsm_dev);
	int (*shut_down)(struct frsm_dev *frsm_dev);
	int (*stat_monitor)(struct frsm_dev *frsm_dev);
	int (*get_livedata)(struct frsm_dev *frsm_dev, struct live_data *data);
	int (*set_spkr_prot)(struct frsm_dev *frsm_dev);
};

struct frsm_action_map {
	const char *event_name;
	int (*ops_action)(struct frsm_dev *frsm_dev);
};

struct frsm_gpio {
	struct gpio_desc *gpiod;
	struct pinctrl_state *state_sleep;
	struct pinctrl_state *state_active;
};

struct frsm_prot_tbl {
	int threshold;
	int volume;
};

struct frsm_prot_params {
	int tbl_count;
	struct frsm_prot_tbl *tbl;
};

/* DTS platform data */
struct frsm_platform_data {
	struct frsm_gpio gpio[FRSM_PIN_MAX];
	struct frsm_prot_params prot_params[FRSM_PROT_MAX];
	int vrtl_addr; /* virtual address */
	int spkr_id;
	int rx_channel;
	int irq_polarity;
	uint32_t ref_rdc;
	uint32_t mntr_scenes;
	uint32_t mntr_period; //ms
	uint32_t soft_reset;
	uint32_t fs15wt_series; //bit0 fs15wt series, bit1 en agl init
	bool mntr_enable;
	bool bsg_volume_v2;
	bool rx_volume_v2;
	const char *fwm_name;
	const char *name_prefix;
};

struct frsm_dev {
	struct device *dev;
	struct i2c_client *i2c;
	struct regmap *regmap;
	struct pinctrl *pinc;
	struct snd_soc_dai_driver *dai_drv;
#if KERNEL_VERSION_HIGHER(4, 17, 0)
	struct snd_soc_component *cmpnt;
#else
	struct snd_soc_codec *codec;
#endif
	struct workqueue_struct *thread_wq;
	struct delayed_work delay_work;
	struct delayed_work irq_work;
	struct list_head list;
	struct mutex io_lock;
	spinlock_t spin_lock;

	struct frsm_platform_data *pdata;
	struct frsm_hw_params hw_params;
	struct frsm_ops ops;

	const struct fwm_header *fwm_data;
	const struct fwm_table *tbl_scene;
	const struct scene_table *cur_scene;

	unsigned long state;
	unsigned long event;
	int func;
	int dev_id;
	int irq_id;
	int volume;
	int safe_vol;
	int batt_vol;
	int next_scene;
	int reg_amp_mute;
	int spkre;
	uint16_t bst_wcam_len; // burst write len
	uint16_t dtype;
	bool force_init;
	bool state_ts;
	bool swap_channel;
	bool prot_batt;
	bool calib_mode;
	bool save_otp_spkre;
	bool state_lnm;
};

#endif // __FRSM_DEV_H__
