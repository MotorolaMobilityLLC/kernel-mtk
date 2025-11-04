/*
 * Copyright (C) 2018-2025, SI-IN, Yun Shi (yun.shi@si-in.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#define DEBUG
#define LOG_FLAG	"sipa_driver"


#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/gameport.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
//#include <linux/wakelock.h>
#include <linux/cdev.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm_runtime.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/soc-dapm.h>
#include <sound/core.h>
#include <linux/time.h>
#include <linux/cpufreq.h>
#include <linux/syscalls.h>
#include <linux/regmap.h>
#include <linux/module.h>
#include <linux/version.h>

#include "sipa_common.h"
#include "sipa_regmap.h"
#include "sipa_tuning_if.h"
#include "sipa_91xx.h"
#include "sipa_parameter.h"
#ifdef CONFIG_SND_SOC_SYDNEY_SYDNYL_MULTI_AUDIO_PA
#include "../../mediatek/common/mtk-sp-spk-amp.h"
#endif

#define SIPA_NAME							("sipa")
#define SIPA_I2C_NAME						(SIPA_NAME)

#define SIPA_CMD_POWER_ON					(1)
#define SIPA_CMD_POWER_OFF					(2)
#define SIPA_CMD_GET_MODE					(3)
#define SIPA_CMD_SET_MODE					(4)
#define SIPA_CMD_GET_REG					(5)
#define SIPA_CMD_SET_REG					(6)
#define SIPA_CMD_WRITE_BYTE					(100)
#define SIPA_CMD_SET_OWI_DELAY				(101)
#define SIPA_CMD_CLR_ERROR					(200)
#define SIPA_CMD_GET_RST_PIN				(612)
#define SIPA_CMD_SET_RST_PIN				(613)
#define SIPA_CMD_GET_SRAM   				(614)
#define SIA81XX_ENABLE_LEVEL				(1)
#define SIA81XX_DISABLE_LEVEL				(0)


/* 100us > pulse width > 2us, suggest 10 us*/
#define MIN_OWI_PULSE_GAP_TIME_US			(10)
#define MAX_OWI_PULSE_GAP_TIME_US			(160)
#define MAX_OWI_RETRY_TIMES					(10)
#define MIN_OWI_MODE						(1)
#define MAX_OWI_MODE						(35)
#define DEFAULT_OWI_MODE					(6)
/* OWI_POLARITY 0 : pulse level == high, 1 : pulse level == low */
#define OWI_POLARITY						(SIA81XX_DISABLE_LEVEL)

// #define DISTINGUISH_CHIP_TYPE
// #define OWI_SUPPORT_WRITE_DATA
#ifdef OWI_SUPPORT_WRITE_DATA
#define OWI_DATA_BIG_END
#endif

/* error list */
/* pulse width time out */
#define EPTOUT								(100)
/* pulse electrical level opposite with the polarity */
#define EPOLAR								(101)

#define SIPA_READ_REG_DUMP_LEN          (40)
#define SIPA_READ_REG_DUMP_BUF_SIZE     (4096)

static ssize_t sipa_cmd_show(struct device *cd,
	struct device_attribute *attr, char *buf);
static ssize_t sipa_cmd_store(struct device *cd,
	struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR_RW(sipa_cmd);

#ifdef DISTINGUISH_CHIP_TYPE
static ssize_t sipa_device_show(struct device *cd,
	struct device_attribute *attr, char *buf);
static ssize_t sipa_device_store(struct device *cd,
	struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR_RW(sipa_device);

#endif

static DEFINE_MUTEX(sipa_mutex);
static sipa_dev_t *g_sipa_dev[SIPA_CHANNEL_NUM];

static const char *support_chip_type_name_table[] = {
	[CHIP_TYPE_SIA8101]  = "sia8101",
	[CHIP_TYPE_SIA8109]  = "sia8109",
	[CHIP_TYPE_SIA8152]  = "sia8152",
	[CHIP_TYPE_SIA8152S] = "sia8152s",
	[CHIP_TYPE_SIA8100X] = "sia8100x",
	[CHIP_TYPE_SIA8001]  = "sia8001",
	[CHIP_TYPE_SIA8102]  = "sia8102,sia8000",
	[CHIP_TYPE_SIA8159]  = "sia8159",
	[CHIP_TYPE_SIA8159A] = "sia8159a",
	[CHIP_TYPE_SIA81X9]  = "sia81x9",
	[CHIP_TYPE_SIA8152X] = "sia8152x",
	[CHIP_TYPE_SIA9195]  = "sia9195",
	[CHIP_TYPE_SIA9196]  = "sia9196",
	[CHIP_TYPE_SIA9175]  = "sia9175",
	[CHIP_TYPE_SIA9177]  = "sia9177",
	[CHIP_TYPE_SIA9255]  = "sia9255",
	[CHIP_TYPE_SIA917X]  = "sia917x",
	[CHIP_TYPE_SIA8150]  = "sia8150",
	[CHIP_TYPE_SIA815T]  = "sia815T",
	[CHIP_TYPE_SIA9187]  = "sia9187",
	[CHIP_TYPE_SIA5118]  = "sia5118",
	[CHIP_TYPE_SIA9197X]  = "sia9197x,sia9197,sia9198,sia9189",
	[CHIP_TYPE_SIA8132]  = "sia8132",
	[CHIP_TYPE_SIA5118S]  = "sia5118s",
	[CHIP_TYPE_SIA5118X]  = "sia5118x",
	[CHIP_TYPE_SIA9306]  = "sia9306",
	[CHIP_TYPE_SIA8168]  = "sia8167,sia8168,sia8169",
	[CHIP_TYPE_SIA9287]  = "sia9287",
	[CHIP_TYPE_SIA9165]  = "sia9165",
};

uint32_t g_dyn_ud_vdd_port;
static int sipa_resume(struct sipa_dev_s *si_pa);
static int sipa_suspend(struct sipa_dev_s *si_pa);

static char sipa_dump_addr[] = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
			0x0A, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
			0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
			0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
			0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
			0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x59, 0x5a,
			0x6E, 0x6F, 0x70, 0x71, 0x72, 0x77};

static int sipa_91xx_reg_dump(struct sipa_dev_s *si_pa, char *buff)
{
	int i, arrSize;
	int off = 0;
	int reg = 0, ret = 0;
	char *regAddr;

	regAddr = sipa_dump_addr;
	arrSize = sizeof(sipa_dump_addr) / sizeof(char);

	off += snprintf(buff + off, SIPA_READ_REG_DUMP_LEN, "channel_num = %d \r\n", si_pa->channel_num);

	if (IS_DIGITAL_PA_PULL_RST_TYPE(si_pa->chip_type)) {
		if (SIA91XX_LOW_LEVEL == gpio_get_value(si_pa->rst_pin)) {
			gpio_set_value(si_pa->rst_pin, SIA91XX_HIGH_LEVEL);
			usleep_range(10000, 12000);
		}
	}

	for (i = 0; i < arrSize; i++) {
		ret = sipa_read_reg(si_pa->regmap, regAddr[i], &reg);
		if (off + SIPA_READ_REG_DUMP_LEN > SIPA_READ_REG_DUMP_BUF_SIZE) {
			break;
		}

		if (ret == 0) {
			off += snprintf(buff + off, SIPA_READ_REG_DUMP_LEN, "reg addr = %02x, value = 0x%x \r\n", regAddr[i], reg);
			reg = 0;
		} else {
			off += snprintf(buff + off, SIPA_READ_REG_DUMP_LEN, "reg addr = %02x, value = XXXXXX \r\n", regAddr[i]);
		}
	}

	if (IS_DIGITAL_PA_PULL_RST_TYPE(si_pa->chip_type)) {
		if (false == sipa_regmap_get_chip_en(si_pa))
			gpio_set_value(si_pa->rst_pin, SIA91XX_LOW_LEVEL);
	}

	return 0;
}

/********************************************************************
 * si_pa GPIO option
 ********************************************************************/
static __inline void gpio_flipping(
	int pin,
	s64 *intervel_ns)
{
	static struct timespec64 cur_time, last_time, temp_time;

	gpio_set_value(pin, !gpio_get_value(pin));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	ktime_get_real_ts64(&cur_time);
#else
	getnstimeofday64(&cur_time);
#endif
	temp_time = timespec64_sub(cur_time, last_time);
	if (NULL != intervel_ns)
		*intervel_ns = timespec64_to_ns(&temp_time);
	last_time = cur_time;
}

static __inline int gpio_produce_one_pulse(
	int pin,
	int polarity,
	unsigned int width_us,
	s64 *real_duty_ns,
	s64 *real_idle_ns)
{
	if (polarity != gpio_get_value(pin)) {
		pr_err("[  err][%s] %s: EPOLAR \r\n", LOG_FLAG, __func__);
		return -EPOLAR;
	}

	gpio_flipping(pin, real_idle_ns);
	udelay(width_us);	//must use udelay.
	gpio_flipping(pin, real_duty_ns);

	return 0;
}

static __inline int __gpio_produce_one_pulse_cycle(
	int pin,
	int polarity,
	unsigned int duty_time_us,
	unsigned int idle_time_us,
	s64 *real_duty_ns,
	s64 *real_idle_ns)
{
	int ret = gpio_produce_one_pulse(pin, polarity,
		duty_time_us, real_duty_ns, real_idle_ns);
	if (0 == ret) /* if pulse produce suceess */
		udelay(idle_time_us);	//must use udelay.

	return ret;
}

static __inline int gpio_produce_one_pulse_cycle(
	int pin,
	int polarity,
	unsigned int delay_us,
	s64 *real_duty_ns,
	s64 *real_idle_ns)
{
	return __gpio_produce_one_pulse_cycle(pin, polarity,
		delay_us, delay_us, real_duty_ns, real_idle_ns);
}

static __inline int gpio_produce_pulse_cycles(
	int pin,
	int polarity,
	unsigned int delay_us,
	unsigned int cycles,
	s64 *max_real_delay_ns) {

	s64 real_duty_ns = 0, real_idle_ns = 0, max_pulse_time = 0;
	bool is_fst_pulse = true;
	int ret = 0;

	if (0 == pin) {
		pr_err("[  err][%s] %s: 0 == pin \r\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

	while (cycles) {
		cycles--;

		ret = gpio_produce_one_pulse_cycle(pin, polarity,
			delay_us, &real_duty_ns, &real_idle_ns);
		if (0 != ret)
			break;

		/* monitor pulse with overtime */
		if (false == is_fst_pulse) {
			max_pulse_time =
				max_pulse_time > real_idle_ns ? max_pulse_time : real_idle_ns;
		}
		max_pulse_time =
			max_pulse_time > real_duty_ns ? max_pulse_time : real_duty_ns;
		is_fst_pulse = false;

	}

	if (NULL != max_real_delay_ns)
		*max_real_delay_ns = max_pulse_time;

	return ret;
}
/********************************************************************
 * end - si_pa GPIO option
 ********************************************************************/


/********************************************************************
 * si_pa owi option
 ********************************************************************/
#ifdef OWI_SUPPORT_WRITE_DATA
static void sia81xx_owi_calc_bit_duty_ns(
	s64 duty_ns,
	s64 idle_ns,
	s64 *duty_time_ns,
	s64 *idle_time_ns,
	const char bit)
{
	if (0 == !!(bit & (char)0x01)) {
		*duty_time_ns = idle_ns;
		*idle_time_ns = duty_ns;
	} else {
		*duty_time_ns = duty_ns;
		*idle_time_ns = idle_ns;
	}
}

static void sia81xx_owi_calc_bit_duty_us(
	u32 duty_us,
	u32 idle_us,
	u32 *duty_time_us,
	u32 *idle_time_us,
	const char bit)
{
	s64 duty_time_ns, idle_time_ns;

	sia81xx_owi_calc_bit_duty_ns((s64)duty_us * NSEC_PER_USEC,
		(s64)idle_us * NSEC_PER_USEC, &duty_time_ns, &idle_time_ns, bit);

	*duty_time_us = duty_time_ns / NSEC_PER_USEC;
	*idle_time_us = idle_time_ns / NSEC_PER_USEC;
}

static int sia81xx_owi_write_one_bit(
	struct sipa_dev_s *si_pa,
	const char bit,
	s64 *real_duty_ns,
	s64 *real_idle_ns)
{
	int ret = 0;
	unsigned int duty_time_us, idle_time_us;

	sia81xx_owi_calc_bit_duty_us(si_pa->owi_delay_us,
		MIN_OWI_PULSE_GAP_TIME_US, &duty_time_us, &idle_time_us, bit);

	ret = __gpio_produce_one_pulse_cycle(si_pa->owi_pin,
		si_pa->owi_polarity, duty_time_us, idle_time_us,
		real_duty_ns, real_idle_ns);

	return ret;
}

static int sia81xx_owi_start_write_byte(
	struct sipa_dev_s *si_pa)
{
	/* before start write, gpio level must be opposite with OWI_POLARITY */
	if (gpio_get_value(si_pa->owi_pin) == si_pa->owi_polarity) {
		pr_err("[  err][%s] %s: EPOLAR !!! \r\n", LOG_FLAG, __func__);
		return -EPOLAR;
	}

	/* *******************************************************************
	 * for reset flipping timer,
	 * otherwise only need gpio_set_value(dev->owi_pin, OWI_POLARITY);
	 * *******************************************************************/
	gpio_flipping(si_pa->owi_pin, NULL);

	return 0;
}

static void sia81xx_owi_end_write_byte(
	struct sipa_dev_s *si_pa,
	s64 *real_idle_ns)
{
	gpio_flipping(si_pa->owi_pin, real_idle_ns);
}

static s64 sia81xx_owi_calc_deviation_ns(
	struct sipa_dev_s *dev,
	s64 duty_time_ns,
	s64 idle_time_ns,
	const char bit)
{
	s64 deviation = 0;

	dev->err_info.owi_max_gap =
		dev->err_info.owi_max_gap > duty_time_ns ?
		dev->err_info.owi_max_gap : duty_time_ns;

	dev->err_info.owi_max_gap =
		dev->err_info.owi_max_gap > idle_time_ns ?
		dev->err_info.owi_max_gap : idle_time_ns;

	sia81xx_owi_calc_bit_duty_ns(duty_time_ns, idle_time_ns,
		&duty_time_ns, &idle_time_ns, bit);

	if (duty_time_ns < idle_time_ns)
		deviation = idle_time_ns - duty_time_ns;

	return deviation;
}

static int sia81xx_owi_write_one_byte(
	struct sipa_dev_s *si_pa,
	const char data)
{
	int ret = 0, i = 0;
	s64 real_duty_ns = 0, real_idle_ns = 0, last_real_duty_ns = 0;
	s64 max_deviation_ns = 0, cur_deviation_ns = 0;
	char last_bit = 0;
	unsigned long flags;

	spin_lock_irqsave(&si_pa->owi_lock, flags);

	ret = sia81xx_owi_start_write_byte(si_pa);
	if (0 != ret)
		goto owi_end_write_one_byte;

#ifdef OWI_DATA_BIG_END
	for (i = 7; i >= 0; i--) {
		ret = sia81xx_owi_write_one_bit(si_pa, (data >> i),
			&real_duty_ns, &real_idle_ns);
		if (0 != ret)
			goto owi_end_write_one_byte;

		/* when second bit has been sent,
		 * can be get first bit's real idle time */
		if (7 > i) {
			cur_deviation_ns = sia81xx_owi_calc_deviation_ns(si_pa,
				last_real_duty_ns, real_idle_ns, last_bit);
			max_deviation_ns = max_deviation_ns > cur_deviation_ns ?
				max_deviation_ns : cur_deviation_ns;

			/* occur write error */
			if (0 != cur_deviation_ns)
				break;
		}
		last_bit = (data >> i);
		last_real_duty_ns = real_duty_ns;
	}
#else
	for (i = 0; i < 8; i++) {
		ret = sia81xx_owi_write_one_bit(si_pa, (data >> i),
			&real_duty_ns, &real_idle_ns);
		if (0 != ret)
			goto owi_end_write_one_byte;

		/* when second bit has been sent,
		 * can be get first bit's real idle time */
		if (0 < i) {
			cur_deviation_ns = sia81xx_owi_calc_deviation_ns(si_pa,
				last_real_duty_ns, real_idle_ns, last_bit);
			max_deviation_ns = max_deviation_ns > cur_deviation_ns ?
				max_deviation_ns : cur_deviation_ns;

			/* occur write error */
			if (0 != cur_deviation_ns)
				break;
		}
		last_bit = (data >> i);
		last_real_duty_ns = real_duty_ns;
	}
#endif

	/* end write byte, and get last bit real idle time */
	sia81xx_owi_end_write_byte(si_pa, &real_idle_ns);

	/* no write error */
	if (0 == cur_deviation_ns) {
		/* then can be get the last bit's real idle time */
		cur_deviation_ns = sia81xx_owi_calc_deviation_ns(si_pa,
			last_real_duty_ns, real_idle_ns, last_bit);
		max_deviation_ns = max_deviation_ns > cur_deviation_ns ?
			max_deviation_ns : cur_deviation_ns;
	}

owi_end_write_one_byte:

	spin_unlock_irqrestore(&si_pa->owi_lock, flags);

	if (0 != cur_deviation_ns)
		ret = -EPTOUT;

	if (0 != ret) {
		if (-EPOLAR == ret)
			/* record polarity err */
			si_pa->err_info.owi_polarity_err_cnt++;
		else
			/* record wirte err */
			si_pa->err_info.owi_write_err_cnt++;
	}

	/* record history max deviation time */
	si_pa->err_info.owi_max_deviation =
		si_pa->err_info.owi_max_deviation > max_deviation_ns ?
		si_pa->err_info.owi_max_deviation : max_deviation_ns;

	return ret;
}

static int sipa_owi_write_data(
	struct sipa_dev_s *si_pa,
	unsigned int len,
	const char *buf)
{
	int ret = 0, i = 0, retry = 0;

	si_pa->err_info.owi_write_data_cnt++;

	for (i = 0; i < len; i++) {

		retry = 0;

		while (retry < MAX_OWI_RETRY_TIMES) {

			ret = sia81xx_owi_write_one_byte(si_pa, buf[i]);
			if (0 == ret)
				break;

			/* must be ust msleep, do not use mdelay */
			usleep_range(1000, 2000);

			retry++;
		}

		/* record max retry time */
		si_pa->err_info.owi_max_retry_cnt =
			si_pa->err_info.owi_max_retry_cnt > retry ?
			si_pa->err_info.owi_max_retry_cnt : retry;

		if (retry >= MAX_OWI_RETRY_TIMES) {
			si_pa->err_info.owi_write_data_err_cnt++;
			return -EPTOUT;
		}
	}

	return ret;
}
#endif

static void sia81xx_set_owi_polarity(
	struct sipa_dev_s *si_pa)
{
	gpio_set_value(si_pa->owi_pin, si_pa->owi_polarity);
}

static int __sipa_owi_write_mode(
	struct sipa_dev_s *si_pa,
	unsigned int cycles)
{
	int ret = 0;
	s64 max_flipping_ns = 0, last_flipping_ns = 0;
	unsigned long flags;

	sia81xx_set_owi_polarity(si_pa);
	// mdelay(5);  /* wait for owi reset, longer than 5ms */
	usleep_range(5000, 6000);

	spin_lock_irqsave(&si_pa->owi_lock, flags);
	/* last pulse only flipping once, so pulses = cycles - 1 */
	ret = gpio_produce_pulse_cycles(si_pa->owi_pin, si_pa->owi_polarity,
			si_pa->owi_delay_us, cycles - 1, &max_flipping_ns);
	/* last pulse */
	gpio_flipping(si_pa->owi_pin, &last_flipping_ns);

	spin_unlock_irqrestore(&si_pa->owi_lock, flags);

	if (0 != ret) {
		if (-EPOLAR == ret)
			/* record polarity err */
			si_pa->err_info.owi_polarity_err_cnt++;
		else
			/* record wirte err */
			si_pa->err_info.owi_write_err_cnt++;
		return ret;
	}

	/* if cycles == 1, then only flipping gpio, and do not care flipping time */
	if (0 == (cycles - 1))
		return 0;

	/* get max flipping time */
	max_flipping_ns = max_flipping_ns > last_flipping_ns ?
		max_flipping_ns : last_flipping_ns;

	/* record history max flipping time */
	si_pa->err_info.owi_max_gap =
		si_pa->err_info.owi_max_gap > max_flipping_ns ?
		si_pa->err_info.owi_max_gap : max_flipping_ns;

	if ((MAX_OWI_PULSE_GAP_TIME_US * NSEC_PER_USEC) <= max_flipping_ns) {
		/* record wirte err */
		si_pa->err_info.owi_write_err_cnt++;
		return -EPTOUT;
	}

	return 0;
}

static int sipa_owi_write_mode(
	struct sipa_dev_s *si_pa,
	unsigned int mode)
{
	unsigned int retry = 0;

	if ((MAX_OWI_MODE < mode) || (MIN_OWI_MODE > mode))
		return -EINVAL;

	si_pa->err_info.owi_set_mode_cnt++;

	while (retry < MAX_OWI_RETRY_TIMES) {

		if (0 == __sipa_owi_write_mode(si_pa, mode))
			break;

		/* must be use msleep, do not use mdelay */
		usleep_range(1000, 2000);

		retry++;
	}

	/* record max retry time */
	si_pa->err_info.owi_max_retry_cnt =
		si_pa->err_info.owi_max_retry_cnt > retry ?
		si_pa->err_info.owi_max_retry_cnt : retry;

	if (retry >= MAX_OWI_RETRY_TIMES) {
		si_pa->err_info.owi_set_mode_err_cnt++;
		return -EPTOUT;
	}

	return 0;
}

static int sipa_owi_init(
	sipa_dev_t *si_pa,
	unsigned int *cfg_mode)
{
	unsigned int owi_mode = DEFAULT_OWI_MODE;
	int i = 0;

	si_pa->owi_delay_us = MIN_OWI_PULSE_GAP_TIME_US;
	si_pa->owi_polarity = OWI_POLARITY;

	for (i = 0; i < AUDIO_SCENE_NUM; i++) {
		owi_mode = cfg_mode[i];
		if ((owi_mode >= MIN_OWI_MODE) && (owi_mode <= MAX_OWI_MODE))
			si_pa->owi_cur_mode[i] = owi_mode;
		else
			si_pa->owi_cur_mode[i] = DEFAULT_OWI_MODE;
	}

	return 0;
}
/********************************************************************
* end - si_pa owis option
********************************************************************/


/********************************************************************
 * si_pa chip option
 ********************************************************************/

static void sipa_get_rst_value(sipa_dev_t *si_pa)
{
	unsigned int gpio_value = 0xff;
	if (0 == si_pa->disable_pin) {
		gpio_value = gpio_get_value(si_pa->rst_pin);
		pr_debug("[debug][%s] %s: reset pin num:%u, value:%u \r\n",
			LOG_FLAG, __func__, si_pa->rst_pin, gpio_value);
	}
}

static bool is_chip_type_supported(unsigned int chip_type)
{
	if (chip_type >= ARRAY_SIZE(support_chip_type_name_table))
		return false;

	return true;
}

static bool sipa_is_chip_en(sipa_dev_t *si_pa)
{
	if (0 == si_pa->disable_pin) {
		if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
			if (sipa_regmap_get_chip_en(si_pa))
				return true;
		} else if (IS_ANALOG_PA_HAVE_RST_AND_CHIP_EN(si_pa->chip_type)) {
			if ((SIA81XX_ENABLE_LEVEL == gpio_get_value(si_pa->rst_pin))
				&& sipa_regmap_get_chip_en(si_pa))
				return true;
		} else {
			if (SIA81XX_ENABLE_LEVEL == gpio_get_value(si_pa->rst_pin))
				return true;
		}
	} else {
		if (sipa_regmap_get_chip_en(si_pa))
			return true;
	}

	return false;
}

int sipa_reg_init(
	struct sipa_dev_s *si_pa)
{
	if (NULL == si_pa->client)
		return 0;

	sipa_regmap_defaults(si_pa->regmap,
		si_pa->chip_type, si_pa->scene, si_pa->channel_num);

	usleep_range(100,200);

	return 0;
}

static int sipa_analog_rst_resume(
	struct sipa_dev_s *si_pa)
{
	if (IS_SUPPORT_OWI_TYPE(si_pa->chip_type)) {
		/* power up chip */
		if (0 != sipa_owi_write_mode(si_pa, si_pa->owi_cur_mode[si_pa->scene]))
			goto err_sipa_analog_rst_resume;
	}

	if (IS_SIPA_RST_KEEP_HIGH(si_pa->chip_type)) {
		if (gpio_get_value(si_pa->rst_pin) == SIA81XX_ENABLE_LEVEL)
			return 0;
	}
	gpio_set_value(si_pa->rst_pin, SIA81XX_ENABLE_LEVEL);
	usleep_range(1000, 1200);  /* wait chip power up, the time must be > 1ms */

	if (CHIP_TYPE_SIA8109 == si_pa->chip_type || CHIP_TYPE_SIA81X9 == si_pa->chip_type)
		msleep(39); /* for sia8109 gain rising. */

	return 0;
err_sipa_analog_rst_resume:
	pr_err("[  err][%s] %s: error !!! \r\n", LOG_FLAG, __func__);
	return -EINVAL;
}

static int sipa_digital_rst_resume(
	struct sipa_dev_s *si_pa)
{
	if (IS_DIGITAL_PA_PULL_RST_TYPE(si_pa->chip_type))
		gpio_set_value(si_pa->rst_pin, SIA81XX_ENABLE_LEVEL);
	else
		gpio_set_value(si_pa->rst_pin, SIA81XX_DISABLE_LEVEL);

	usleep_range(1000, 1200);  /* wait chip power up, the time must be > 1ms */
	return 0;
}

static int sipa_resume(
	struct sipa_dev_s *si_pa)
{
	int rst_pin_val;

	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL !!! \r\n", LOG_FLAG, __func__);
		return -ENODEV;
	}

	pr_debug("[debug][%s] %s: si_pa->chip_type = %d, channel:%d, running \r\n",
			 LOG_FLAG, __func__, si_pa->chip_type, si_pa->channel_num);

	if (is_chip_type_supported(si_pa->chip_type) &&
		!sipa_is_chip_en(si_pa)) {
		if (0 == si_pa->disable_pin) {
			if (IS_DIGITAL_PA_TYPE(si_pa->chip_type))
				sipa_digital_rst_resume(si_pa);
			else
				sipa_analog_rst_resume(si_pa);

			rst_pin_val = gpio_get_value(si_pa->rst_pin);
			pr_debug("[debug][%s] %s: channel = %d, rst_pin_val = %d, owi_mode = %d \r\n",
					 LOG_FLAG, __func__, si_pa->channel_num, rst_pin_val, si_pa->owi_cur_mode[si_pa->scene]);
		}

		sipa_reg_init(si_pa);

		if (sipa_regmap_set_chip_on(si_pa))
			si_pa->power_mode = true;

		sipa_regmap_check_trimming(si_pa);
	}

	return 0;
}

static int sipa_analog_rst_suspend(
	struct sipa_dev_s *si_pa)
{
	/* power off chip */
	if (IS_SIPA_RST_KEEP_HIGH(si_pa->chip_type)) {
		return 0;
	}
	gpio_set_value(si_pa->rst_pin, SIA81XX_DISABLE_LEVEL);
	usleep_range(5000, 6000);  /* wait chip power off, the time must be > 5ms */
	return 0;
}

static int sipa_digital_rst_suspend(
	struct sipa_dev_s *si_pa)
{
	if (IS_DIGITAL_PA_PULL_RST_TYPE(si_pa->chip_type)) {
		/* power off chip */
		gpio_set_value(si_pa->rst_pin, SIA81XX_DISABLE_LEVEL);
		usleep_range(5000, 6000);  /* wait chip power off, the time must be > 5ms */
	}

	return 0;
}

static int sipa_suspend(
	struct sipa_dev_s *si_pa)
{
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL !!! \r\n", LOG_FLAG, __func__);
		return -ENODEV;
	}

	pr_debug("[debug][%s] %s: running channel:%d\r\n", LOG_FLAG, __func__, si_pa->channel_num);

	if (is_chip_type_supported(si_pa->chip_type) &&
		sipa_is_chip_en(si_pa)) {
		si_pa->power_mode = false;
		sipa_regmap_set_chip_off(si_pa);

		if (0 == si_pa->disable_pin) {
			if (IS_DIGITAL_PA_TYPE(si_pa->chip_type))
				sipa_digital_rst_suspend(si_pa);
			else
				sipa_analog_rst_suspend(si_pa);
		}
		sipa_get_rst_value(si_pa);
	}

	return 0;
}

static int sipa_reboot(
	struct sipa_dev_s *si_pa)
{
	int ret = 0;

	ret = sipa_suspend(si_pa);
	if (0 != ret)
		return ret;

	usleep_range(5000, 6000);

	return sipa_resume(si_pa);
}

static int sipa_set_mode(
	struct sipa_dev_s *si_pa,
	unsigned int mode)
{
	int ret = 0;
	if (!IS_SUPPORT_OWI_TYPE (si_pa->chip_type)) {
		pr_info("[ info][%s] %s: needn't set mode \n",LOG_FLAG, __func__);
		return ret;
	}

	if ((MAX_OWI_MODE < mode) || (MIN_OWI_MODE > mode)) {
		pr_err("[  err][%s] %s: error mode = %u !!! \r\n",
			LOG_FLAG, __func__, mode);
		return -EINVAL;
	}

	si_pa->owi_cur_mode[si_pa->scene] = mode;

	ret = sipa_reboot(si_pa);

	return ret;
}

static int sipa_scene_set(struct sipa_dev_s *si_pa, unsigned int scene)
{
#if PROTECT_ULTRASONIC == 1
	unsigned int last_scene;
#endif

	if (si_pa == NULL) {
		pr_err("[  err][%s] %s: si_pa is NULL !!! \r\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

#if PROTECT_ULTRASONIC == 1
	last_scene = si_pa->scene;
#endif

	if (AUDIO_SCENE_NUM <= scene) {
		si_pa->scene = AUDIO_SCENE_PLAYBACK;
		pr_err("[  err][%s] %s: set audio scene val = %u !!! \r\n",
			   LOG_FLAG, __func__, scene);
	} else
		si_pa->scene = scene;

	if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
		// The digital PA switches scenes after the music is turned off.
		// Additionally, once the i2s signal stops, the i2c cannot be operated anymore
		// Therefore, block this code.
		// sia91xx_smartpa_soft_mute(si_pa);
		// sipa_reg_init(si_pa);
		// sia91xx_smartpa_start(si_pa, SNDRV_PCM_STREAM_PLAYBACK);
#if PROTECT_ULTRASONIC == 1
		if (((si_pa->scene == AUDIO_SCENE_VOICE) && (last_scene == AUDIO_SCENE_RECEIVER))
			|| ((si_pa->scene == AUDIO_SCENE_RECEIVER) && (last_scene == AUDIO_SCENE_VOICE))) {
			if (si_pa->power_mode == true) {
				sia91xx_smartpa_soft_mute(si_pa);
				sipa_reg_init(si_pa);
				sia91xx_smartpa_start(si_pa, SNDRV_PCM_STREAM_PLAYBACK);
			}
		}
#endif
	} else {
		if (sipa_is_chip_en(si_pa))
			sipa_reboot(si_pa);
	}

	return 0;
}

/********************************************************************
 * device attr option
 ********************************************************************/
#ifdef DISTINGUISH_CHIP_TYPE
static ssize_t sipa_device_show(
	struct device *cd,
	struct device_attribute *attr,
	char *buf)
{
	return 0;
}

static ssize_t sipa_device_store(
	struct device *cd,
	struct device_attribute *attr,
	const char *buf,
	size_t len)
{
	return 0;
}
#endif

static ssize_t sipa_cmd_show(
	struct device *cd,
	struct device_attribute *attr,
	char *buf)
{
	sipa_dev_t *si_pa = (sipa_dev_t *)dev_get_drvdata(cd);
	char chip_id = 0;
	char vals[0x64];
	int owi_pin_val = 0;

	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

	if (NULL == buf) {
		pr_err("[  err][%s] %s: buf is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

	if (IS_SUPPORT_OWI_TYPE(si_pa->chip_type)) {
		owi_pin_val = gpio_get_value(si_pa->owi_pin);

		snprintf(buf, 1024, "sipa_cmd_show : \r\n"
			"channel_num = %u\r\n"
			"rst_pin = %d,\r\n"
			"owi_pin = %d,\r\n"
			"owi_mode = %u, delay = %u,\r\n",
			si_pa->channel_num,
			si_pa->rst_pin,
			si_pa->owi_pin,
			si_pa->owi_cur_mode[si_pa->scene], si_pa->owi_delay_us);
		return strlen(buf);
	}

	if (si_pa->regmap == NULL) {
		pr_err("[  err][%s] %s: regmap is NULL !!! \r\n",
					LOG_FLAG, __func__);
		return -1;
	}

	if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
		sipa_91xx_reg_dump(si_pa, buf);
		return strlen(buf);
	}

	switch (si_pa->chip_type) {
	case CHIP_TYPE_SIA8109:
		sipa_regmap_read(
			si_pa->regmap, si_pa->chip_type, 0x41, 1, &chip_id);
		break;
	default:
		sipa_regmap_read(
			si_pa->regmap, si_pa->chip_type, 0x00, 1, &chip_id);
		break;
	}

	memset(vals, 0, sizeof(vals));
	sipa_regmap_read(
		si_pa->regmap, si_pa->chip_type, 0x00, 0x2f + 1, vals);

	snprintf(buf, 2048, "sipa_cmd_show : \r\n"
		"channel_num = %u, chip id = 0x%02x \r\n"
		"      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n"
		"00:  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n"
		"01:  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n"
		"02:  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n"
		"rst_pin = %d, \r\n"
		"si_pa_disable_pin = %d,  \r\n"
		"dynamic_updata_vdd_port = 0x%x \r\n",
		si_pa->channel_num, chip_id,
		vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7],
		vals[8], vals[9], vals[10], vals[11], vals[12], vals[13], vals[14], vals[15],
		vals[16], vals[17], vals[18], vals[19], vals[20], vals[21], vals[22], vals[23],
		vals[24], vals[25], vals[26], vals[27], vals[28], vals[29], vals[30], vals[31],
		vals[32], vals[33], vals[34], vals[35], vals[36], vals[37], vals[38], vals[39],
		vals[40], vals[41], vals[42], vals[43], vals[44], vals[45], vals[46], vals[47],
		si_pa->rst_pin,
		si_pa->disable_pin,
		si_pa->dyn_ud_vdd_port);

	return strlen(buf);
}

static ssize_t sipa_cmd_store(
	struct device *cd,
	struct device_attribute *attr,
	const char *buf,
	size_t len)
{
	const char *split_symb = ",";
	/* in strsep will be modify "cur" value, nor cur[i] value,
	so this point shoud not be defined with "char * const" */
	char *cur = (char *)buf;
	char *after;
	sipa_dev_t *si_pa;

	if (cur == NULL) {
		pr_err("[  err][%s] %s: Command format is error !!! \r\n",
			LOG_FLAG, __func__);
		return -ENOIOCTLCMD;
	}

	si_pa = (sipa_dev_t *)dev_get_drvdata(cd);
	if (si_pa == NULL) {
		return -ENOIOCTLCMD;
	}

	switch (simple_strtoul(strsep(&cur, split_symb), &after, 10)) {
	case SIPA_CMD_POWER_ON:
		sipa_resume(si_pa);
		break;
	case SIPA_CMD_POWER_OFF:
		sipa_suspend(si_pa);
		break;
	case SIPA_CMD_GET_MODE:
	{
		pr_debug("[debug][%s] %s: mode = %u \r\n",
				LOG_FLAG, __func__, si_pa->owi_cur_mode[si_pa->scene]);
		break;
	}
	case SIPA_CMD_SET_MODE:
	{
		if (cur == NULL) {
			pr_err("[  err][%s] %s: Command format is error !!! \r\n",
				LOG_FLAG, __func__);
			return -ENOIOCTLCMD;
		}

		sipa_set_mode(si_pa,
			simple_strtoul(strsep(&cur, split_symb), &after, 10));
		break;
	}
	case SIPA_CMD_GET_REG:
	{
		unsigned char addr;
		unsigned int val;

		if (cur == NULL) {
			pr_err("[  err][%s] %s: Command format is error !!! \r\n",
				LOG_FLAG, __func__);
			return -ENOIOCTLCMD;
		}
		if (si_pa->regmap == NULL) {
			pr_err("[  err][%s] %s: regmap is NULL !!! \r\n",
					LOG_FLAG, __func__);
			return -ENOIOCTLCMD;
		}
		addr = (unsigned char)simple_strtoul(
			strsep(&cur, split_symb), &after, 16);

		if (0 != regmap_read(si_pa->regmap, addr, &val)) {
			pr_debug("[debug][%s] %s: err regmap_read \r\n",
				LOG_FLAG, __func__);
		} else {
			pr_debug("[debug][%s] %s: addr = 0x%x, val = 0x%x\r\n",
				LOG_FLAG, __func__, addr, val);
		}
		break;
	}
	case SIPA_CMD_SET_REG:
	{
		unsigned char addr;
		unsigned int val;

		if (cur == NULL) {
			pr_err("[  err][%s] %s: Command format is error !!! \r\n",
				LOG_FLAG, __func__);
			return -ENOIOCTLCMD;
		}
		if (si_pa->regmap == NULL) {
			pr_err("[  err][%s] %s: regmap is NULL !!! \r\n",
					LOG_FLAG, __func__);
			return -ENOIOCTLCMD;
		}
		addr = (unsigned char)simple_strtoul(
			strsep(&cur, split_symb), &after, 16);

		if (cur == NULL) {
			pr_err("[  err][%s] %s: Command format is error !!! \r\n",
				LOG_FLAG, __func__);
			return -ENOIOCTLCMD;
		}
		val = (unsigned int)simple_strtoul(
			strsep(&cur, split_symb), &after, 16);
		pr_info("[ info][%s] %s: addr:0x%x, val:0x%x !!\r\n", LOG_FLAG, __func__, addr, val);
		if (0 != regmap_write(si_pa->regmap, addr, val)) {
			pr_err("[  err][%s] %s: err regmap_write \r\n",
				LOG_FLAG, __func__);
		}
		break;
	}
	case SIPA_CMD_SET_OWI_DELAY:
	{
		unsigned int temp_us;

		if (cur == NULL) {
			pr_err("[  err][%s] %s: Command format is error !!! \r\n",
				LOG_FLAG, __func__);
			return -ENOIOCTLCMD;
		}
		temp_us = (unsigned int)simple_strtoul(
									strsep(&cur, split_symb), &after, 10);
		if (temp_us < MAX_OWI_PULSE_GAP_TIME_US) /* only for test, pulse width must be < 1ms */
			si_pa->owi_delay_us = temp_us;
		else
			si_pa->owi_delay_us = MAX_OWI_PULSE_GAP_TIME_US;
		break;
	}
	case SIPA_CMD_CLR_ERROR:
	{
		memset(&si_pa->err_info, 0, sizeof(si_pa->err_info));
		break;
	}
	case SIPA_CMD_WRITE_BYTE:
	{
#ifdef OWI_SUPPORT_WRITE_DATA
		char data;

		if (cur == NULL) {
			pr_err("[  err][%s] %s: Command format is error !!! \r\n",
				LOG_FLAG, __func__);
			return -ENOIOCTLCMD;
		}
		data = simple_strtoul(strsep(&cur, split_symb), &after, 10);
		sipa_owi_write_data(si_pa, 1, &data);
#else
		pr_info("[ info][%s] %s: the option "
			"OWI_SUPPORT_WRITE_DATA unsupport!! \r\n", LOG_FLAG, __func__);
#endif
		break;
	}
	case SIPA_CMD_GET_RST_PIN:
	{
		if (0 == si_pa->disable_pin) {
			unsigned int gpio_value = 0xff;

			gpio_value = gpio_get_value(si_pa->rst_pin);
			pr_debug("[debug][%s] %s: get reset pin value, pin num:%u, value:%u \r\n",
					LOG_FLAG, __func__, si_pa->rst_pin, gpio_value);
		}

		break;
	}
	case SIPA_CMD_SET_RST_PIN:
	{
		unsigned char value;

		if (cur == NULL) {
			pr_err("[  err][%s] %s: Command format is error !!! \r\n",
				LOG_FLAG, __func__);
			return -ENOIOCTLCMD;
		}

		if (0 == si_pa->disable_pin) {
			value = (unsigned int)simple_strtoul(
				strsep(&cur, split_symb), &after, 16);

			/* power on/off chip */
			gpio_set_value(si_pa->rst_pin, value);
			usleep_range(1000, 2000); /* wait chip power off, the time must be > 1ms */

			pr_debug("[debug][%s] %s: set reset pin %u to %u \r\n",
					LOG_FLAG, __func__, si_pa->rst_pin, value);
		}
		break;
	}
	case SIPA_CMD_GET_SRAM:
	{
		sipa_regmap_read_sram(si_pa);
		break;
	}

	default:
		return -ENOIOCTLCMD;
	}

	return len;
}
/********************************************************************
 * end - device attr option
 ********************************************************************/


/********************************************************************
 * si_pa codec driver
 ********************************************************************/
static int sipa_power_get(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	ucontrol->value.integer.value[0] = si_pa->power_mode;

	pr_debug("[debug][%s] %s: ucontrol = %ld, channel_num = %d \r\n",
		LOG_FLAG, __func__, ucontrol->value.integer.value[0], si_pa->channel_num);

	return 0;
}

static int sipa_power_set(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
    sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	pr_debug("[debug][%s] %s: ucontrol = %ld, rst = %d, channel_num = %d \r\n",
		LOG_FLAG, __func__, ucontrol->value.integer.value[0],
		si_pa->rst_pin, si_pa->channel_num);

	if (1 == ucontrol->value.integer.value[0]) {
		sipa_resume(si_pa);
		si_pa->power_mode = true;
	} else {
		sipa_suspend(si_pa);
		si_pa->power_mode = false;
	}

	return 0;
}

static int sipa_audio_scene_get(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	ucontrol->value.integer.value[0] = si_pa->scene;

	pr_debug("[debug][%s] %s: ucontrol = %ld, channle = %d \r\n",
		LOG_FLAG, __func__, ucontrol->value.integer.value[0], si_pa->channel_num);

	return 0;
}

static int sipa_audio_scene_set(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	uint32_t scene = 0;
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	scene = ucontrol->value.integer.value[0];
	pr_debug("[debug][%s] %s: ucontrol = %u, rst = %d, channel = %d \r\n",
		LOG_FLAG, __func__, scene, si_pa->rst_pin, si_pa->channel_num);

	sipa_scene_set(si_pa, scene);
	return 0;
}

static int sipa_pvdd_limit_get(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int sipa_pvdd_limit_set(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	pr_debug("[debug][%s] %s: ucontrol = %ld, channle = %d \r\n",
		LOG_FLAG, __func__, ucontrol->value.integer.value[0],si_pa->channel_num);

	sipa_regmap_set_pvdd_limit(si_pa->regmap, si_pa->chip_type,
					si_pa->channel_num, ucontrol->value.integer.value[0]);

	return 0;
}

static int sipa_status_get(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	ucontrol->value.integer.value[0] = si_pa->pa_status;

	pr_debug("[debug][%s] %s: ucontrol = 0x%ld, channle = %d \r\n",
		LOG_FLAG, __func__, ucontrol->value.integer.value[0],si_pa->channel_num);

	return 0;
}

static int sipa_status_set(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sipa_chip_type_get(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

	ucontrol->value.integer.value[0] = si_pa->chip_type;

	pr_debug("[debug][%s] %s: ucontrol = 0x%ld, channle = %d \r\n",
		LOG_FLAG, __func__, ucontrol->value.integer.value[0], si_pa->channel_num);

	return 0;
}

static int sipa_chip_type_set(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sipa_mute_get(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	ucontrol->value.integer.value[0] = si_pa->mute;

	pr_debug("[debug][%s] %s: ucontrol = 0x%ld, channle = %d \r\n",
		LOG_FLAG, __func__, ucontrol->value.integer.value[0], si_pa->channel_num);

	return 0;
}

static int sipa_mute_set(
	struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
    sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}
	pr_debug("[debug][%s] %s: ucontrol = %ld, channel_num = %d \r\n",
		LOG_FLAG, __func__, ucontrol->value.integer.value[0],
		si_pa->channel_num);

	if (si_pa->power_mode == false) {
		if (1 == ucontrol->value.integer.value[0]) {
			si_pa->mute = SIPA_DEVICE_MUTE_ON;
		} else {
			si_pa->mute = SIPA_DEVICE_MUTE_OFF;
		}
	}

	return 0;
}

static const char *const power_function[] = { "Off", "On" };
static const char *const audio_scene[] = { "Playback", "Voice", "Voip", "Receiver", "Factory", "FM" };

static const struct soc_enum power_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(power_function), power_function);
static const struct soc_enum audio_scene_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(audio_scene), audio_scene);
#if SIPA_CREAT_CONTRILS_MODE == 1
static struct soc_mixer_control pa_pvdd_limit = {
	.reg = SND_SOC_NOPM, .rreg = SND_SOC_NOPM, .shift = 0,
	.rshift = 0, .max = 5000000, .platform_max = 5000000,
	.invert = 0, .autodisable = 0};

static struct soc_mixer_control pa_status = {
	.reg = SND_SOC_NOPM, .rreg = SND_SOC_NOPM, .shift = 0,
	.rshift = 0, .max = 0xff, .platform_max = 0xff,
	.invert = 0, .autodisable = 0};

static struct soc_mixer_control pa_type = {
		.reg = SND_SOC_NOPM, .rreg = SND_SOC_NOPM, .shift = 0,
		.rshift = 0, .max = 100, .platform_max = 100,
		.invert = 0, .autodisable = 0};
static int sipa_dynamic_create_controls(sipa_dev_t *si_pa)
{
	struct snd_kcontrol_new *sipa_dev_control = NULL;
	struct soc_bytes_ext *sipa_dev_bytes_ext = NULL;
	char *kctl_name = NULL;
	pr_debug("[ info][%s][%s]: enter \r\n", LOG_FLAG, __func__);

	if(NULL == si_pa->codec){
		pr_err("[ err][%s][%s]: st si_pa->codec is null \r\n", LOG_FLAG, __func__);
		return -ENOMEM;
	}

	sipa_dev_control = devm_kzalloc(si_pa->codec->dev,
			 sizeof(struct snd_kcontrol_new) * SIPA_KCTL_NUM, GFP_KERNEL);
	sipa_dev_bytes_ext = devm_kzalloc(si_pa->codec->dev, sizeof(struct soc_bytes_ext), GFP_KERNEL);

	if (sipa_dev_control == NULL)
		return -ENOMEM;

	if (sipa_dev_bytes_ext == NULL)
		return -ENOMEM;

	kctl_name = devm_kzalloc(si_pa->codec->dev, SIPA_NAME_BUF_MAX, GFP_KERNEL);
	if (!kctl_name)
		return -ENOMEM;

	snprintf(kctl_name, SIPA_NAME_BUF_MAX, "Spkr_ch_%d Sipa Power", si_pa->channel_num);

	sipa_dev_control[KCTL_TYPE_POWER].name          = kctl_name;
	sipa_dev_control[KCTL_TYPE_POWER].iface         = SNDRV_CTL_ELEM_IFACE_MIXER;
	sipa_dev_control[KCTL_TYPE_POWER].info          = snd_soc_info_enum_double;
	sipa_dev_control[KCTL_TYPE_POWER].get           = sipa_power_get;
	sipa_dev_control[KCTL_TYPE_POWER].put           = sipa_power_set;
	sipa_dev_control[KCTL_TYPE_POWER].private_value = (unsigned long)&power_enum;

	kctl_name = devm_kzalloc(si_pa->codec->dev, SIPA_NAME_BUF_MAX, GFP_KERNEL);
	if (!kctl_name)
		return -ENOMEM;

	snprintf(kctl_name, SIPA_NAME_BUF_MAX, "Spkr_ch_%d Sipa Audio Scene", si_pa->channel_num);

	sipa_dev_control[KCTL_TYPE_SCENE].name          = kctl_name;
	sipa_dev_control[KCTL_TYPE_SCENE].iface         = SNDRV_CTL_ELEM_IFACE_MIXER;
	sipa_dev_control[KCTL_TYPE_SCENE].info          = snd_soc_info_enum_double;
	sipa_dev_control[KCTL_TYPE_SCENE].get           = sipa_audio_scene_get;
	sipa_dev_control[KCTL_TYPE_SCENE].put           = sipa_audio_scene_set;
	sipa_dev_control[KCTL_TYPE_SCENE].private_value = (unsigned long)&audio_scene_enum;

	kctl_name = devm_kzalloc(si_pa->codec->dev, SIPA_NAME_BUF_MAX, GFP_KERNEL);
	if (!kctl_name)
		return -ENOMEM;

	snprintf(kctl_name, SIPA_NAME_BUF_MAX, "Spkr_ch_%d Sipa Pvdd Limit", si_pa->channel_num);

	sipa_dev_control[KCTL_TYPE_LIMIT].name          = kctl_name;
	sipa_dev_control[KCTL_TYPE_LIMIT].iface         = SNDRV_CTL_ELEM_IFACE_MIXER;
	sipa_dev_control[KCTL_TYPE_LIMIT].info          = snd_soc_info_volsw;
	sipa_dev_control[KCTL_TYPE_LIMIT].get           = sipa_pvdd_limit_get;
	sipa_dev_control[KCTL_TYPE_LIMIT].put           = sipa_pvdd_limit_set;
	sipa_dev_control[KCTL_TYPE_LIMIT].private_value = (unsigned long)&pa_pvdd_limit;

	kctl_name = devm_kzalloc(si_pa->codec->dev, SIPA_NAME_BUF_MAX, GFP_KERNEL);
	if (!kctl_name)
		return -ENOMEM;

	snprintf(kctl_name, SIPA_NAME_BUF_MAX, "Spkr_ch_%d Sipa Status", si_pa->channel_num);

	sipa_dev_control[KCTL_TYPE_STATUS].name          = kctl_name;
	sipa_dev_control[KCTL_TYPE_STATUS].iface         = SNDRV_CTL_ELEM_IFACE_MIXER;
	sipa_dev_control[KCTL_TYPE_STATUS].info          = snd_soc_info_volsw;
	sipa_dev_control[KCTL_TYPE_STATUS].get           = sipa_status_get;
	sipa_dev_control[KCTL_TYPE_STATUS].put           = sipa_status_set;
	sipa_dev_control[KCTL_TYPE_STATUS].private_value = (unsigned long)&pa_status;

	kctl_name = devm_kzalloc(si_pa->codec->dev, SIPA_NAME_BUF_MAX, GFP_KERNEL);
	if (!kctl_name)
		return -ENOMEM;

	snprintf(kctl_name, SIPA_NAME_BUF_MAX, "Spkr_ch_%d Sipa Type", si_pa->channel_num);

	sipa_dev_control[KCTL_TYPE_TYPE].name          = kctl_name;
	sipa_dev_control[KCTL_TYPE_TYPE].iface         = SNDRV_CTL_ELEM_IFACE_MIXER;
	sipa_dev_control[KCTL_TYPE_TYPE].info          = snd_soc_info_volsw;
	sipa_dev_control[KCTL_TYPE_TYPE].get           = sipa_chip_type_get;
	sipa_dev_control[KCTL_TYPE_TYPE].put           = sipa_chip_type_set;
	sipa_dev_control[KCTL_TYPE_TYPE].private_value = (unsigned long)&pa_type;

	kctl_name = devm_kzalloc(si_pa->codec->dev, SIPA_NAME_BUF_MAX, GFP_KERNEL);
	if (!kctl_name)
		return -ENOMEM;

	snprintf(kctl_name, SIPA_NAME_BUF_MAX, "Spkr_ch_%d Sipa Mute", si_pa->channel_num);

	sipa_dev_control[KCTL_TYPE_MUTE].name          = kctl_name;
	sipa_dev_control[KCTL_TYPE_MUTE].iface         = SNDRV_CTL_ELEM_IFACE_MIXER;
	sipa_dev_control[KCTL_TYPE_MUTE].info          = snd_soc_info_enum_double;
	sipa_dev_control[KCTL_TYPE_MUTE].get           = sipa_mute_get;
	sipa_dev_control[KCTL_TYPE_MUTE].put           = sipa_mute_set;
 	sipa_dev_control[KCTL_TYPE_MUTE].private_value = (unsigned long)&power_enum;

#ifdef SIPA_KERNEL_VER_OVER_4_16_28
	snd_soc_add_component_controls(si_pa->codec, sipa_dev_control, SIPA_KCTL_NUM);
#else
	snd_soc_add_codec_controls(si_pa->codec, sipa_dev_control, SIPA_KCTL_NUM);
#endif

	pr_debug("[ info][%s][%s]: end \r\n", LOG_FLAG, __func__);

	return 0;
 }
#endif
static const struct snd_kcontrol_new sipa_controls[] = {

	SOC_ENUM_EXT("Sipa Power", power_enum,
			sipa_power_get, sipa_power_set),

	SOC_ENUM_EXT("Sipa Audio Scene", audio_scene_enum,
			sipa_audio_scene_get, sipa_audio_scene_set),

	SOC_SINGLE_EXT("Sipa Pvdd Limit", SND_SOC_NOPM, 0, 5000000, 0,
			sipa_pvdd_limit_get, sipa_pvdd_limit_set),

	SOC_SINGLE_EXT("Sipa Status", SND_SOC_NOPM, 0, 0xff, 0,
			sipa_status_get, sipa_status_set),

	SOC_SINGLE_EXT("Sipa Type", SND_SOC_NOPM, 0, 100, 0,
		sipa_chip_type_get, sipa_chip_type_set),

	SOC_SINGLE_EXT("Sipa Mute", SND_SOC_NOPM, 0, 1, 0,
		sipa_mute_get, sipa_mute_set),


};

static const struct snd_soc_dai_ops sia91xx_dai_ops = {
	.startup = sia91xx_startup,
	.set_fmt = sia91xx_set_fmt,
	.hw_params = sia91xx_hw_params,
	.mute_stream = sia91xx_mute,
};

static struct snd_soc_dai_driver sia91xx_dai[] = {
	{
		.name = "sia91xx-aif",
		.id = 1,
		.playback = {
			.stream_name = "AIF Playback",
			.channels_min = 1,
			.channels_max = SIPA_CHANNEL_NUM,
			.rates = SIA91XX_RATES,
			.formats = SIA91XX_FORMATS,
		},
		.capture = {
			 .stream_name = "AIF Capture",
			 .channels_min = 1,
			 .channels_max = SIPA_CHANNEL_NUM,
			 .rates = SIA91XX_RATES,
			 .formats = SIA91XX_FORMATS,
		 },
		 .ops = &sia91xx_dai_ops,
	}
};

static int sipa_spkr_pa_event(
	struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol,
	int event)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	sipa_dev_t *si_pa = snd_soc_component_get_drvdata(component);
#else
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	sipa_dev_t *si_pa = snd_soc_codec_get_drvdata(codec);
#endif
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

	pr_debug("[debug][%s] %s: msg = %d \r\n", LOG_FLAG, __func__, event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		sipa_resume(si_pa);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		sipa_suspend(si_pa);
		break;
	default:
		pr_err("[  err][%s] %s: msg = %d \r\n", LOG_FLAG, __func__, event);
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget sipa_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("IN"),

	SND_SOC_DAPM_OUT_DRV_E("SPKR DRV", 0, 0, 0, NULL, 0,
			sipa_spkr_pa_event, SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_OUTPUT("SPKR")
};

/*static const struct snd_soc_dapm_route sia81xx_audio_map[] = {
	{"SPKR DRV", NULL, "IN"},
	{"SPKR", NULL, "SPKR DRV"}
};*/

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
static int sipa_component_probe(
	struct snd_soc_component *component)
{
	sipa_dev_t *si_pa = NULL;
	if (NULL == component) {
		pr_err("[  err][%s] %s: component is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

	si_pa = snd_soc_component_get_drvdata(component);
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

    pr_info("[debug][%s] %s: running \r\n", LOG_FLAG, __func__);

	if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
		sia91xx_component_probe(component);
	} else {
		si_pa->codec = component;
	}

#if SIPA_CREAT_CONTRILS_MODE == 1
	sipa_dynamic_create_controls(si_pa);
#endif

	return 0;
}

static void sipa_component_remove(
	struct snd_soc_component *component)
{
	pr_info("[debug][%s] %s: running \r\n", LOG_FLAG, __func__);
	return;
}
#else
static int sipa_codec_probe(
	struct snd_soc_codec *codec)
{
	sipa_dev_t *si_pa = NULL;
	if (NULL == codec) {
		pr_err("[  err][%s] %s: codec is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

	si_pa = snd_soc_codec_get_drvdata(codec);
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL\n", LOG_FLAG, __func__);
		return -EINVAL;
	}

    pr_info("[debug][%s] %s: running \r\n", LOG_FLAG, __func__);
	if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
		sia91xx_codec_probe(codec);
	} else {
		si_pa->codec = codec;
	}

#if SIPA_CREAT_CONTRILS_MODE == 1
	sipa_dynamic_create_controls(si_pa);
#endif

	return 0;
}

static int sipa_codec_remove(
	struct snd_soc_codec *codec)
{
	pr_info("[debug][%s] %s: running \r\n", LOG_FLAG, __func__);
	return 0;
}

static struct regmap *sipa_codec_get_regmap(struct device *dev)
{
	return NULL;
}
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
static int sipa_component_write(struct snd_soc_component *component,
				 unsigned int reg, unsigned int val)
{
	return 0;
}

static unsigned int sipa_component_read(struct snd_soc_component *component,
			     unsigned int reg)
{
	return 0;
}

static struct snd_soc_component_driver soc_component_dev_sipa = {
	.probe = sipa_component_probe,
	.remove = sipa_component_remove,
#if SIPA_CREAT_CONTRILS_MODE  == 0
	.controls = sipa_controls,
	.num_controls = ARRAY_SIZE(sipa_controls),
#endif
	.dapm_widgets = sipa_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(sipa_dapm_widgets),
	.dapm_routes = NULL, //sia81xx_audio_map,
	.num_dapm_routes = 0,//ARRAY_SIZE(sia81xx_audio_map),
	.write = sipa_component_write,
	.read = sipa_component_read,
};
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(4, 8, 17) && LINUX_VERSION_CODE <= KERNEL_VERSION(4, 16, 28))
static struct snd_soc_codec_driver soc_codec_dev_sipa = {
	.probe = sipa_codec_probe,
	.remove = sipa_codec_remove,
	.get_regmap = sipa_codec_get_regmap,
	.component_driver = {
#if SIPA_CREAT_CONTRILS_MODE  == 0
		.controls = sipa_controls,
		.num_controls = ARRAY_SIZE(sipa_controls),
#endif
		.dapm_widgets = sipa_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(sipa_dapm_widgets),
		.dapm_routes = NULL, //sia81xx_audio_map,
		.num_dapm_routes = 0,//ARRAY_SIZE(sia81xx_audio_map),
	},
};
#else
static struct snd_soc_codec_driver soc_codec_dev_sipa = {
	.probe = sipa_codec_probe,
	.remove = sipa_codec_remove,
	.get_regmap = sipa_codec_get_regmap,
#if SIPA_CREAT_CONTRILS_MODE  == 0
	.controls = sipa_controls,
	.num_controls = ARRAY_SIZE(sipa_controls),
#endif
	.dapm_widgets = sipa_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(sipa_dapm_widgets),
	.dapm_routes = NULL, //sia81xx_audio_map,
	.num_dapm_routes = 0,//ARRAY_SIZE(sia81xx_audio_map),
};
#endif

/********************************************************************
 * sipa driver common
 ********************************************************************/

sipa_dev_t *find_sipa_dev(struct device_node *of_node)
{
	sipa_dev_t *si_pa = NULL, *find = NULL;
	int i = 0;

	if (NULL == of_node) {
		pr_err("[  err][%s] %s: NULL == input parameter \r\n",
			LOG_FLAG, __func__);
		return NULL;
	}

	mutex_lock(&sipa_mutex);

	for (i = 0; i < ARRAY_SIZE(g_sipa_dev); i++) {
		si_pa = g_sipa_dev[i];
		if (si_pa == NULL) {
			continue;
		}

		/* make sure that the sipa platform dev had been created */
		if ((NULL != of_node) && (NULL != si_pa->pdev)) {
			if (of_node == si_pa->pdev->dev.of_node) {
				find = si_pa;
				break;
			}
		}
	}

	mutex_unlock(&sipa_mutex);

	return find;
}

static sipa_dev_t *create_sipa_dev(struct device *dev)
{
	sipa_dev_t *si_pa = NULL;
	int i = 0;

	si_pa = kzalloc(sizeof(sipa_dev_t), GFP_KERNEL);
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: dev[%s] cannot create memory for si_pa\n",
			LOG_FLAG, __func__, dev->init_name);
		return NULL;
	}

	mutex_lock(&sipa_mutex);
	for (i = 0; i < ARRAY_SIZE(g_sipa_dev); i++) {
		if (NULL == g_sipa_dev[i]) {
			g_sipa_dev[i] = si_pa;
			break;
		}
	}
	mutex_unlock(&sipa_mutex);

	if (i >= ARRAY_SIZE(g_sipa_dev)) {
		kfree(si_pa);
		return NULL;
	}

	/* default name */
	snprintf(si_pa->name, strlen("sipa-spk.%u"),
		"sipa-spk.%u", i);

	pr_debug("[debug][%s] %s: add si_pa dev : %s, index = %u \r\n",
		LOG_FLAG, __func__, si_pa->name, i);
	return si_pa;
}

static void put_sipa_dev(sipa_dev_t *si_pa)
{
	int i = 0;

	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: NULL == si_pa \r\n", LOG_FLAG, __func__);
		return ;
	}

	pr_debug("[debug][%s] %s: put dev name : %s, pdev = %p, client = %p \r\n",
				LOG_FLAG, __func__,
				si_pa->name, si_pa->pdev, si_pa->client);

	if ((NULL != si_pa->pdev) || (NULL != si_pa->client))
		return ;

	for (i = 0; i < ARRAY_SIZE(g_sipa_dev); i++) {
		if (si_pa == g_sipa_dev[i]) {
			kfree(si_pa);
			g_sipa_dev[i] = NULL;
			break;
		}
	}

	if (i >= ARRAY_SIZE(g_sipa_dev)) {
		pr_err("[  err][%s] %s: dev[%s] cannot find in device array\n",
			LOG_FLAG, __func__, si_pa->name);
	}
}

static unsigned int get_chip_type(const char *name)
{
	int i = 0, len = 0;
	char *temp, *copy_p;
	const char *delim = ",";
	char chip_type_copy[64];

	if (NULL == name) {
		return CHIP_TYPE_UNKNOWN;
	}

	pr_info("[ info][%s] %s: chip name: %s \r\n",
			LOG_FLAG, __func__, name);
	len = strlen(name);

	for (i = 0; i < ARRAY_SIZE(support_chip_type_name_table); i++) {
		if (strstr(support_chip_type_name_table[i], delim) == NULL) {
			if (strlen(support_chip_type_name_table[i]) == len &&
				0 == memcmp(support_chip_type_name_table[i], name, len)) {
				pr_info("[ info][%s] %s: chip_type = %d\r\n", LOG_FLAG, __func__, i);
				return i;
			}
		}
		else {
			if (strlen(support_chip_type_name_table[i]) > ARRAY_SIZE(chip_type_copy))
			{
				pr_err("[  err][%s] %s: chip name: %s out of memory range \r\n",LOG_FLAG, __func__,
					support_chip_type_name_table[i]);
				break;
			}

			memset((void *)chip_type_copy, '\0', sizeof(chip_type_copy));
			memcpy((void *)chip_type_copy, support_chip_type_name_table[i], strlen(support_chip_type_name_table[i]));
			copy_p = chip_type_copy;
			temp = strsep(&copy_p, delim);

			while (temp) {
				if (strcmp(temp, name)) {
					temp = strsep(&copy_p, delim);
				}
				else {
					return i;
				}
			}
		}
	}

	return CHIP_TYPE_UNKNOWN;
}

/* CHIP_TYPE_SIA81X9 */
static const uint32_t sia81x9_list[] = {
	CHIP_TYPE_SIA815T,
	CHIP_TYPE_SIA8159,
	CHIP_TYPE_SIA8159A,
	CHIP_TYPE_SIA8109,
	CHIP_TYPE_SIA8168
};

/* CHIP_TYPE_SIA8152X */
static const uint32_t sia8152x_list[] = {
	CHIP_TYPE_SIA8152S,	// first chip reg range should cover all other chips
	CHIP_TYPE_SIA8152,
	CHIP_TYPE_SIA8150,
	CHIP_TYPE_SIA8132
};

/* CHIP_TYPE_SIA917X */
static const uint32_t sia917x_list[] = {
	CHIP_TYPE_SIA9175,	// first chip reg range should cover all other chips
	CHIP_TYPE_SIA9177
};

/* CHIP_TYPE_SIA5118X */
static const uint32_t sia5118x_list[] = {
	CHIP_TYPE_SIA5118,	// first chip reg range should cover all other chips
	CHIP_TYPE_SIA5118S
};


/* compatible chips should have same i2c address */
static const struct sipa_chip_compat sipa_compat_table[] = {
	{
		CHIP_TYPE_SIA81X9,
		{sia81x9_list, ARRAY_SIZE(sia81x9_list)}
	},
	{
		CHIP_TYPE_SIA8152X,
		{sia8152x_list, ARRAY_SIZE(sia8152x_list)}
	},
	{
		CHIP_TYPE_SIA917X,
		{sia917x_list, ARRAY_SIZE(sia917x_list)}
	},
	{
		CHIP_TYPE_SIA5118X,
		{sia5118x_list, ARRAY_SIZE(sia5118x_list)}
	}
};

unsigned int get_one_available_chip_type(unsigned int chip_type)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(sipa_compat_table); i++) {
		if (chip_type == sipa_compat_table[i].sub_type) {
			if (0 < sipa_compat_table[i].num)
				//pr_info("[ info][%s] %s: chip_type = %u \r\n",
				//	LOG_FLAG, __func__, sipa_compat_table[i].chips[0]);
				return sipa_compat_table[i].chips[0];
		}
	}

	return chip_type;
}

static void sipa_set_rst(sipa_dev_t *si_pa, int value)
{
	if (0 == si_pa->disable_pin && IS_NEED_PULL_RST_TYPE(si_pa->chip_type)) {
		if (IS_SIPA_RST_KEEP_HIGH(si_pa->chip_type)) {
			if (gpio_get_value(si_pa->rst_pin) == SIA81XX_ENABLE_LEVEL)
				return;
		}
		gpio_set_value(si_pa->rst_pin, value);
		usleep_range(3000, 3200);
	}
	sipa_get_rst_value(si_pa);
}

#ifdef DISTINGUISH_CHIP_TYPE
static int check_sipa_status(sipa_dev_t *si_pa)
{
	int ret = 0;

	sipa_set_rst(si_pa, SIA81XX_ENABLE_LEVEL);

	if (1 == si_pa->en_dyn_id) {
		ret = gpio_get_value(si_pa->id_pin);
		pr_info("[ info][%s] ch %d id_pin_val = %d\r\n", __func__, si_pa->channel_num, ret);
	} else {
		ret = sipa_regmap_check_chip_id(
			si_pa->regmap, si_pa->channel_num, si_pa->chip_type, true);
	}

	sipa_set_rst(si_pa, SIA81XX_DISABLE_LEVEL);

	return ret;
}
#endif

static void distinguish_chip_type(sipa_dev_t *si_pa)
{
#ifdef DISTINGUISH_CHIP_TYPE
	/* check sia81xx is available */

	if (CHIP_TYPE_UNKNOWN == si_pa->chip_type) {
		pr_info("[ info][%s] %s: there is no si_pa device \r\n",
			LOG_FLAG, __func__);
	} else {
		if (1 == si_pa->en_dyn_id) {
			if (1 == check_sipa_status(si_pa)) {
				si_pa->chip_type = CHIP_TYPE_SIA8001;
			} else if (0 == check_sipa_status(si_pa)) {
				si_pa->chip_type = CHIP_TYPE_SIA8102;
			}
			device_create_file(&si_pa->pdev->dev, &dev_attr_sipa_device);
			pr_info("[ info][%s] %s: sipa device is available, chip_type = %d .\r\n",
				LOG_FLAG, __func__, si_pa->chip_type);
		} else {
			if (0 != check_sipa_status(si_pa)) {
				si_pa->chip_type = CHIP_TYPE_UNKNOWN;
				pr_info("[ info][%s] %s: there is no si_pa device \r\n",
					LOG_FLAG, __func__);
			} else {
				device_create_file(&si_pa->pdev->dev, &dev_attr_sipa_device);
				pr_info("[ info][%s] %s: sipa device is available, chip_type = %d .\r\n",
					LOG_FLAG, __func__, si_pa->chip_type);
			}
		}
	}
#endif
}

void sipa_compatible_chips_adapt(
	sipa_dev_t *si_pa)
{
	int i = 0, j = 0;

	for (i = 0; i < ARRAY_SIZE(sipa_compat_table); i++) {
		if (si_pa->chip_type == sipa_compat_table[i].sub_type) {

			sipa_set_rst(si_pa, SIA81XX_ENABLE_LEVEL);
			for (j = 0; j < sipa_compat_table[i].num; j++) {
				if (NULL != sipa_compat_table[i].chips
					&& 0 == sipa_regmap_check_chip_id(si_pa->regmap,
					si_pa->channel_num, sipa_compat_table[i].chips[j], false)) {
					si_pa->chip_type = sipa_compat_table[i].chips[j];
					break;
				}
			}

			sipa_set_rst(si_pa, SIA81XX_DISABLE_LEVEL);

			if (j >= sipa_compat_table[i].num)
				si_pa->chip_type = CHIP_TYPE_UNKNOWN;

			pr_info("[ info][%s] %s: chip_type = %u \r\n",
				LOG_FLAG, __func__, si_pa->chip_type);
			break;
		}
	}

	distinguish_chip_type(si_pa);
}

int sipa_analog_detect_chip(
	sipa_dev_t *si_pa)
{
	int ret;

	sipa_set_rst(si_pa, SIA81XX_ENABLE_LEVEL);
	ret = sipa_regmap_check_chip_id(si_pa->regmap, si_pa->channel_num,
									si_pa->chip_type, true);
	sipa_set_rst(si_pa, SIA81XX_DISABLE_LEVEL);

	if (ret < 0) {
		pr_err("[  err][%s] %s: sipa_regmap_check_chip_id error. ret : %d\r\n",
			   LOG_FLAG, __func__, ret);
		return ret;
	}

	return 0;
}
/********************************************************************
 * end - sipa driver common
 ********************************************************************/

/********************************************************************
 * i2c bus driver
 ********************************************************************/
int sipa_pending_actions(sipa_dev_t *si_pa)
{
	SIPA_CHIP_CFG chip_cfg;
	SIPA_EXTRA_CFG extra_cfg;

	if (NULL == si_pa)
		return -EINVAL;

	/* copy settings */
	if (NULL == sipa_param_read_chip_cfg(
		si_pa->channel_num, si_pa->chip_type, &chip_cfg)) {
		pr_err("[  err][%s] %s: read_chip_cfg return NULL \r\n",
			LOG_FLAG, __func__);
		return -EFAULT;
	}

	if (0 != sipa_param_read_extra_cfg(
		si_pa->channel_num, &extra_cfg))
		return -EFAULT;

	si_pa->dyn_ud_vdd_port = extra_cfg.dyn_ud_vdd_port;
	g_dyn_ud_vdd_port = extra_cfg.dyn_ud_vdd_port; //qcom need afe port 0x1004

	if (IS_SUPPORT_OWI_TYPE(si_pa->chip_type)) {
		sipa_owi_init(si_pa, chip_cfg.owi_mode);
	}

	/* power down chip in any case when phone start up */
	if (!IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
		sipa_suspend(si_pa);
	}

	pr_info("[ info][%s] %s: sipa %d driver init done \r\n",
		LOG_FLAG, __func__, si_pa->channel_num);

	return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,6,0))
int sipa_i2c_probe(
	struct i2c_client *client,
	const struct i2c_device_id *id)
#else
int sipa_i2c_probe(
	struct i2c_client *client)
#endif
{
	sipa_dev_t *si_pa = NULL;
	struct device_node *sipa_of_node = NULL;
	char *sipa_fw_name = SIPA_FW_BIN;
	int ret = 0;
	int addr_offset = 0;
	struct snd_soc_dai_driver *dai = NULL;
	unsigned int value;

	if (NULL == client) {
		pr_err("[  err][%s] %s: client is NULL\n", LOG_FLAG, __func__);
		return -ENODEV;
	}

	pr_info("[ info][%s] %s: enter, i2c addr = 0x%02x \r\n",
			LOG_FLAG, __func__, client->addr);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[  err][%s] %s: i2c_check_functionality return -ENODEV \r\n",
			   LOG_FLAG, __func__);
		return -ENODEV;
	}

	sipa_of_node = of_parse_phandle(client->dev.of_node,
									"si,sipa-dev", 0);

	if (!sipa_of_node) {
		pr_err("[  err][%s] %s: failed to get si,sipa-dev !!! \r\n",
			   LOG_FLAG, __func__);
		return -EINVAL;
	}

	si_pa = find_sipa_dev(sipa_of_node);

	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: can't find si_pa\n", LOG_FLAG, __func__);
		return -ENODEV;
	}

	if (CHIP_TYPE_UNKNOWN == si_pa->chip_type) {
		pr_err("[  err][%s] %s: CHIP_TYPE_UNKNOWN == chip_type !!! \r\n",
			   LOG_FLAG, __func__);
		return -ENODEV;
	}

	ret = of_property_read_s32(client->dev.of_node, "si,sipa-addr_offset", &addr_offset);

	if (0 != ret) {
		pr_info("[ info][%s] %s: addr_offset default value!!!\r\n", LOG_FLAG, __func__);
		addr_offset = 0;
	}

	client->addr += addr_offset;

	if (client->addr > 0x7f) {
		pr_err("[ err][%s] %s: client addr error!\r\n", LOG_FLAG, __func__);
		return -ENODEV;
	}

	si_pa->regmap = sipa_regmap_init(client, si_pa->channel_num,
									 get_one_available_chip_type(si_pa->chip_type));

	if (IS_ERR(si_pa->regmap)) {
		pr_err("[  err][%s] %s: regmap_init_i2c error !!! \r\n", LOG_FLAG, __func__);
		return PTR_ERR(si_pa->regmap);
	}

	//series of 52 chip i2c addr compatible
	if (si_pa->chip_type == CHIP_TYPE_SIA8152X) {
		ret = regmap_read(si_pa->regmap, 0x00, &value);

		if (ret < 0) {
			sipa_regmap_remove(si_pa);
			client->addr -= 0x30;
			si_pa->regmap = sipa_regmap_init(client, si_pa->channel_num,
											 get_one_available_chip_type(si_pa->chip_type));

			if (IS_ERR(si_pa->regmap)) {
				pr_err("[  err][%s] %s: regmap_init_i2c error !!! \r\n", LOG_FLAG, __func__);
				return PTR_ERR(si_pa->regmap);
			}

			ret = regmap_read(si_pa->regmap, 0x00, &value);
		}

		pr_info("[ info][%s] %s: read chip id = 0x%x !!! \r\n", LOG_FLAG, __func__, value);
	}

	/* save i2c client */
	si_pa->client = client;

	/* sava driver private data to the dev's driver data */
	dev_set_drvdata(&client->dev, si_pa);

	if (si_pa->en_compatible_type){
		sipa_compatible_chips_adapt(si_pa);
		if (si_pa->chip_type == CHIP_TYPE_UNKNOWN) {
			pr_err("[  err][%s] %s: CHIP_TYPE_UNKNOWN == chip_type !!! \r\n",
					LOG_FLAG, __func__);
			return -ENODEV;
		}
	} else {
		if (IS_DIGITAL_PA_TYPE(si_pa->chip_type))
			ret = sia91xx_detect_chip(si_pa);
		else
			ret = sipa_analog_detect_chip(si_pa);

		if (ret < 0) {
			pr_err("[  err][%s] %s: sipa chip detect fail !!! \r\n", LOG_FLAG,    __func__);
			return ret;
		}
	}

	if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
		dai = devm_kzalloc(&client->dev, sizeof(sia91xx_dai), GFP_KERNEL);

		if (NULL == dai)
			return -ENOMEM;

		memcpy(dai, sia91xx_dai, sizeof(sia91xx_dai));

		sia91xx_append_i2c_address(&client->dev,
								   client,
								   NULL,
								   0,
								   dai,
								   ARRAY_SIZE(sia91xx_dai));

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
		ret = devm_snd_soc_register_component(&client->dev,
											  &soc_component_dev_sipa, dai, ARRAY_SIZE(sia91xx_dai));
#else
		ret = snd_soc_register_codec(&client->dev,
									 &soc_codec_dev_sipa, dai, ARRAY_SIZE(sia91xx_dai));
#endif

	} else {
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
		ret = devm_snd_soc_register_component(&si_pa->pdev->dev, &soc_component_dev_sipa, NULL, 0);
#else
		ret = snd_soc_register_codec(&si_pa->pdev->dev, &soc_codec_dev_sipa, NULL, 0);
#endif
	}

	if (ret < 0) {
		dev_err(&client->dev, "Failed to register sipa codec: %d\n", ret);
		return ret;
	}

	pr_info("[info][%s] register codec success, ret = %d !\n", __func__, ret);

	if ((CHIP_TYPE_UNKNOWN != si_pa->chip_type) && (!IS_SUPPORT_OWI_TYPE(si_pa->chip_type))) {
		device_create_file(&si_pa->pdev->dev, &dev_attr_sipa_cmd);
		/* load firmware */
		sipa_param_load_fw(&client->dev, sipa_fw_name);
	}
#ifdef CONFIG_SND_SOC_SYDNEY_SYDNYL_MULTI_AUDIO_PA
	audiopa_set_type(AUDIOPA_SIA8168);
#endif
	pr_info("[ info][%s] %s: finish, i2c addr = 0x%02x, name:%s, channel:%d\r\n",
			LOG_FLAG, __func__, client->addr, si_pa->name, si_pa->channel_num);

	return 0;
}
EXPORT_SYMBOL(sipa_i2c_probe);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,1,0))
int sipa_i2c_remove(struct i2c_client *client)
{
	int ret = 0;
	sipa_dev_t *si_pa = NULL;

	pr_info("[ info][%s] %s: remove \r\n", LOG_FLAG, __func__);

	if (NULL == client) {
		pr_err("[  err][%s] %s: client is NULL\n", LOG_FLAG, __func__);
		return -ENODEV;
	}
	si_pa = (sipa_dev_t *)dev_get_drvdata(&client->dev);
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL \r\n",
			LOG_FLAG, __func__);
		return 0;
	}

	if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
		cancel_delayed_work_sync(&si_pa->interrupt_work);
	}

	// sipa_regmap_remove(si_pa);   //devm_regmap_init_i2c
	si_pa->regmap = NULL;
	si_pa->client = NULL;

	put_sipa_dev(si_pa);

	return ret;
}
EXPORT_SYMBOL(sipa_i2c_remove);

#else

void sipa_i2c_remove(
	struct i2c_client *client)
{
	sipa_dev_t *si_pa = NULL;

	if (NULL == client) {
		pr_err("[  err][%s] %s: client is NULL\n", LOG_FLAG, __func__);
		return;
	}

	pr_info("[ info][%s] %s: remove \r\n", LOG_FLAG, __func__);

	si_pa = (sipa_dev_t *)dev_get_drvdata(&client->dev);
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL \r\n",
			LOG_FLAG, __func__);
		return;
	}
	if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
		if (gpio_is_valid(si_pa->irq_pin)) {
			cancel_delayed_work_sync(&si_pa->interrupt_work);
		}
		// cancel_delayed_work_sync(&si_pa->monitor_work);
	}

	//sipa_regmap_remove(si_pa);
	si_pa->regmap = NULL;
	si_pa->client = NULL;

	put_sipa_dev(si_pa);

	return;
}
EXPORT_SYMBOL(sipa_i2c_remove);
#endif

void sipa_i2c_shutdown(struct i2c_client *client)
{
	sipa_dev_t *si_pa = NULL;

	pr_info("[ info][%s] %s: running \r\n", LOG_FLAG, __func__);

	if (NULL == client) {
		pr_err("[  err][%s] %s: client is NULL\n", LOG_FLAG, __func__);
		return;
	}

	si_pa = (sipa_dev_t *)dev_get_drvdata(&client->dev);
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL \r\n",
			LOG_FLAG, __func__);
		return;
	}

	if (!IS_SUPPORT_OWI_TYPE(si_pa->chip_type)) {
		sipa_suspend(si_pa);
	}
	return;
}
EXPORT_SYMBOL(sipa_i2c_shutdown);

static const struct i2c_device_id si_sipa_i2c_id[] = {
	{ SIPA_I2C_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static const struct of_device_id si_sipa_i2c_match[] = {
	{.compatible = "si,sia81xx-i2c"},
	{.compatible = "si,sia8109-i2c"},
	{.compatible = "si,sia8152-i2c"},
	{.compatible = "si,sia8152s-i2c"},
	{.compatible = "si,sia8159-i2c"},
	{.compatible = "si,sia81x9-i2c"},
	{.compatible = "si,sia8152x-i2c"},
	{.compatible = "si,sia91xx-i2c"},
	{.compatible = "si,sia8132-i2c"},
	{},
};
#endif

static struct i2c_driver si_sipa_i2c_driver = {
	.driver = {
		.name = SIPA_I2C_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = si_sipa_i2c_match,
#endif
	},
	.probe		= sipa_i2c_probe,
	.remove		= sipa_i2c_remove,
	.shutdown   = sipa_i2c_shutdown,
	.id_table	= si_sipa_i2c_id,
};
/********************************************************************
 * end - i2c bus driver
 ********************************************************************/

/********************************************************************
 * sipa dev driver
 ********************************************************************/

/* power manage options */
static int sipa_pm_suspend(
	struct device *dev)
{
#if 0
	sipa_dev_t *si_pa = NULL;

	pr_debug("[debug][%s] %s: running \r\n", LOG_FLAG, __func__);

	si_pa = (sipa_dev_t *)dev_get_drvdata(dev);

	return sipa_suspend(si_pa);
#else
	pr_debug("[debug][%s] %s: running \r\n", LOG_FLAG, __func__);

	return 0;
#endif
}

static int sipa_pm_resume(struct device *dev)
{
#if 0
	sipa_dev_t *si_pa = NULL;

	pr_debug("[debug][%s] %s: running \r\n", LOG_FLAG, __func__);

	si_pa = (sipa_dev_t *)dev_get_drvdata(dev);

	return sipa_resume(si_pa);
#else
	pr_debug("[debug][%s] %s: running \r\n", LOG_FLAG, __func__);

	return 0;
#endif
}

static const struct dev_pm_ops si_sipa_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sipa_pm_suspend, sipa_pm_resume)
};

static int sipa_property_init(struct device_node *sipa_of_node, sipa_dev_t *si_pa)
{
	int ret = 0;
	const char *chip_type_name = NULL;
	unsigned int chip_type = CHIP_TYPE_UNKNOWN;
	int disable_pin = 0;
	int channel_num = 0;
	int en_compatible_type = 0;
	int en_dyn_id = 0;
	int rst_pin = 0;
	int id_pin = 0;
	int irq_pin = 0;
	int en_irq_pin = 0;
	int owi_pin = 0;

	/* get chip type name */
	ret = of_property_read_string_index(sipa_of_node,
				"si,si_pa_type",
				0,
				&chip_type_name);
	if (0 != ret) {
		pr_err("[  err][%s] %s: get si,si_pa_type return %d !!! \r\n",
			LOG_FLAG, __func__, ret);
		return -ENODEV;
	}

	/* get chip type value,
	 * and check the chip type Whether or not to be supported */
	chip_type = get_chip_type(chip_type_name);
	if (CHIP_TYPE_UNKNOWN == chip_type) {
		pr_err("[  err][%s] %s: CHIP_TYPE_UNKNOWN == chip_type !!! \r\n",
			LOG_FLAG, __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(sipa_of_node,
			"si,si_pa_disable_pin", &disable_pin);
	if (0 != ret) {
		pr_err("[  err][%s] %s: get si,si_pa_disable_pin return %d !!! \r\n",
			LOG_FLAG, __func__, ret);
		return -ENODEV;
	}

	ret = of_property_read_u32(sipa_of_node, "channel_num", &channel_num);
	if (0 != ret) {
		channel_num = 0;
	}

	ret = of_property_read_u32(sipa_of_node, "en_compatible_type_func", &en_compatible_type);
	if (0 != ret) {
		en_compatible_type = 0;
	}

	ret = of_property_read_u32(sipa_of_node, "en_dynamic_id", &en_dyn_id);
	if ((0 != ret) || (1 != en_dyn_id)) {
		en_dyn_id = 0;
	}
	si_pa->en_dyn_id = (unsigned int)en_dyn_id;

	memset(&si_pa->err_info, 0, sizeof(si_pa->err_info));
	si_pa->channel_num = (uint32_t)channel_num;
	si_pa->en_compatible_type = (uint32_t)en_compatible_type;
	si_pa->scene = AUDIO_SCENE_PLAYBACK;
	si_pa->chip_type = chip_type;
	si_pa->disable_pin = disable_pin;
	si_pa->pa_status = 0;

	pr_debug("[debug][%s] %s: channel_num = %d, en_compatible_type = %d, scene = %d, "
						"disable_pin = %d, en_dyn_id = %d\r\n",
					LOG_FLAG, __func__,
					si_pa->channel_num,
					si_pa->en_compatible_type,
					si_pa->scene,
					si_pa->disable_pin,
					si_pa->en_dyn_id);

	if (0 == si_pa->disable_pin) {

		/* get reset pin's sn */
		rst_pin = of_get_named_gpio(sipa_of_node, "si,si_pa_reset", 0);
		if (rst_pin < 0) {
			pr_err("[  err][%s] %s: rst_pin < 0 !!! \r\n", LOG_FLAG, __func__);
			return -ENODEV;
		}
		pr_debug("[debug][%s] %s: rst_pin = %d \r\n", LOG_FLAG, __func__, rst_pin);

		if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
			ret = of_property_read_u32(sipa_of_node,
					"en_irq_func", &en_irq_pin);
			if ((0 != ret) || (1 != en_irq_pin)) {
				si_pa->irq_pin = -1;
				pr_warn("[ warn] %s: No IRQ GPIO provided !\r\n", __func__);
			} else {
				irq_pin = of_get_named_gpio(sipa_of_node, "si,si_pa_irq", 0);
				if (irq_pin < 0) {
					pr_err("[  err][%s] %s: irq_pin < 0 !!! \r\n", LOG_FLAG, __func__);
					return -ENODEV;
				}
				si_pa->irq_pin = irq_pin;
				pr_debug("[debug][%s] %s: irq_pin = %d \r\n", LOG_FLAG, __func__, irq_pin);
			}
		}

		/* get owi pin's sn */
		if (IS_SUPPORT_OWI_TYPE(si_pa->chip_type)) {
			owi_pin = of_get_named_gpio(sipa_of_node, "si,si_pa_owi", 0);
			if (owi_pin < 0) {
				pr_err("[  err][%s] %s: owi_pin < 0 !!! \r\n", LOG_FLAG, __func__);
				return -ENODEV;
			}
			pr_debug("[debug][%s] %s: owi_pin = %d \r\n", LOG_FLAG, __func__, owi_pin);
		}

		if (1 == si_pa->en_dyn_id) {
			/* get id pin's sn */
			id_pin = of_get_named_gpio(sipa_of_node, "si,si_pa_id", 0);
			if (id_pin < 0) {
				pr_err("[  err][%s] %s: id_pin < 0 !!! \r\n", LOG_FLAG, __func__);
				return -ENODEV;
			}
			si_pa->id_pin = id_pin;
			pr_debug("[debug][%s] %s: id_pin = %d \r\n", LOG_FLAG, __func__, si_pa->id_pin);
		}
	}

	si_pa->rst_pin = rst_pin;
	si_pa->owi_pin = owi_pin;

	return 0;
}

static int sipa_pinctrl_select(struct platform_device *pdev, sipa_dev_t *si_pa)
{
	struct pinctrl *si_pa_pinctrl = NULL;
	struct pinctrl_state *pinctrl_state = NULL;
	int ret = 0;

	if (0 == si_pa->disable_pin) {
		/* get reset gpio pin's pinctrl info */
		si_pa_pinctrl = devm_pinctrl_get(&pdev->dev);
		if (NULL == si_pa_pinctrl) {
			pr_err("[  err][%s] %s: NULL == pinctrl !!! \r\n",
				LOG_FLAG, __func__);
			return -ENODEV;
		}

		/* get owi gpio pin's specify pinctrl state */
		pinctrl_state = pinctrl_lookup_state(si_pa_pinctrl, "si_pa_gpio");
		if (NULL == pinctrl_state) {
			pr_err("[  err][%s] %s: NULL == pinctrl_state !!! \r\n",
				LOG_FLAG, __func__);
			ret = -ENODEV;
			goto err;
		}

		/* set this pinctrl state, make this pin works in the gpio mode */
		ret = pinctrl_select_state(si_pa_pinctrl, pinctrl_state);
		if (0 != ret) {
			pr_err("[  err][%s] %s: error pinctrl_select_state return %d \r\n",
				LOG_FLAG, __func__, ret);
			ret = -ENODEV;
			goto err;
		}

		if (IS_DIGITAL_PA_TYPE(si_pa->chip_type)) {
			if (si_pa->irq_pin > 0) {
				pinctrl_state = pinctrl_lookup_state(si_pa_pinctrl, "si_pa_irq");
				if (NULL == pinctrl_state) {
					pr_err("[  err][%s] %s: NULL == pinctrl_state !!! \r\n", LOG_FLAG, __func__);
					ret = -ENODEV;
					goto err;
				}

				ret = pinctrl_select_state(si_pa_pinctrl, pinctrl_state);
				if (0 != ret) {
					pr_err("[  err][%s] %s: error pinctrl_select_state return %d \r\n", LOG_FLAG, __func__, ret);
					ret = -ENODEV;
					goto err;
				}
			}
		}

		if (1 == si_pa->en_dyn_id) {
			pinctrl_state = pinctrl_lookup_state(si_pa_pinctrl, "si_pa_id");
			if (NULL == pinctrl_state) {
				pr_err("[  err][%s] %s: NULL == pinctrl_state !!! \r\n",
					LOG_FLAG, __func__);
				ret = -ENODEV;
				goto err;
			}

			/* set this pinctrl state, make this pin works in the gpio mode */
			ret = pinctrl_select_state(si_pa_pinctrl, pinctrl_state);
			if (0 != ret) {
				pr_err("[  err][%s] %s: error pinctrl_select_state return %d \r\n",
					LOG_FLAG, __func__, ret);
				ret = -ENODEV;
				goto err;
			}
			gpio_direction_input(si_pa->id_pin);
		}

		/* set rst pin's direction */
		gpio_direction_output(si_pa->rst_pin, SIA81XX_DISABLE_LEVEL);

		/* set owi pin's direction */
		if (IS_SUPPORT_OWI_TYPE(si_pa->chip_type)) {
			gpio_direction_output(si_pa->owi_pin, OWI_POLARITY);
		}
	}

err:
	if (0 == si_pa->disable_pin) {
		devm_pinctrl_put(si_pa_pinctrl);
	}
	return ret;
}

static int sipa_probe(struct platform_device *pdev)
{
	int ret = 0;
	sipa_dev_t *si_pa = NULL;
	char *work_name = NULL;
	char *sipa_fw_name = SIPA_FW_BIN;

	pr_info("[ info][%s] %s: enter \r\n", LOG_FLAG, __func__);

	if (NULL == pdev) {
		pr_err("[  err][%s] %s: pdev is NULL !!! \r\n",
			LOG_FLAG, __func__);
		return -ENODEV;
	}

	si_pa = create_sipa_dev(&pdev->dev);
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: get_si_pa_dev error !!! \r\n",
			LOG_FLAG, __func__);
		return -ENODEV;
	}

	if (sipa_property_init(pdev->dev.of_node, si_pa)) {
		pr_err("[  err][%s] %s: sipa_property_init error !!! \r\n",
			LOG_FLAG, __func__);
		ret = -ENODEV;
		goto out0;
	}

	if (sipa_pinctrl_select(pdev, si_pa)) {
		pr_err("[  err][%s] %s: sipa_pinctrl_select error !!! \r\n",
			LOG_FLAG, __func__);
		ret = -ENODEV;
		goto out0;
	}

	spin_lock_init(&si_pa->owi_lock);

	if (IS_SUPPORT_OWI_TYPE(si_pa->chip_type)) {
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 16, 28))
		ret = devm_snd_soc_register_component(&pdev->dev, &soc_component_dev_sipa, NULL, 0);
#else
		ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sipa, NULL, 0);
#endif
		if (0 != ret) {
			pr_err("[  err][%s] %s: snd_soc_register_codec fail \r\n",
				LOG_FLAG, __func__);
			goto out0;
		}
	}

	/* save platform dev */
	si_pa->pdev = pdev;

	/* sava driver private data to the dev's driver data */
	dev_set_drvdata(&pdev->dev, si_pa);

	if (1 == si_pa->en_dyn_id)
		distinguish_chip_type(si_pa);

	work_name = devm_kzalloc(&pdev->dev, 20, GFP_KERNEL);
	if (work_name != NULL) {
		snprintf(work_name, 20, "sipa_%d", si_pa->channel_num);
		si_pa->sipa_wq = create_singlethread_workqueue(work_name);
		if (si_pa->sipa_wq == NULL) {
			pr_err("[  err][%s] %s: create singlethread fail \r\n",	LOG_FLAG, __func__);
			ret = -ENOMEM;
			goto out0;
		}
	}

	if (IS_SUPPORT_OWI_TYPE(si_pa->chip_type)) {
		device_create_file(&pdev->dev, &dev_attr_sipa_cmd);
		/* load firmware */
		sipa_param_load_fw(&pdev->dev, sipa_fw_name);
	}
	pr_info("[ info][%s] %s: finish, channel:%d\r\n", LOG_FLAG, __func__, si_pa->channel_num);

	return 0;

out0:
	put_sipa_dev(si_pa);

	return ret;
}

static int sipa_remove(struct platform_device *pdev)
{
	int ret = 0;
	sipa_dev_t *si_pa = NULL;

	if (NULL == pdev) {
		pr_err("[  err][%s] %s: pdev is NULL !!! \r\n",
			LOG_FLAG, __func__);
		return -ENODEV;
	}

	pr_debug("[debug][%s] %s: remove \r\n", LOG_FLAG, __func__);

	si_pa = (sipa_dev_t *)dev_get_drvdata(&pdev->dev);
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL \r\n",
			LOG_FLAG, __func__);
		return 0;
	}

	ret = sipa_suspend(si_pa);
	if (0 != ret) {
		pr_err("[  err][%s] %s: si_pa_dev_remove return : %d \r\n",
			LOG_FLAG, __func__, ret);
	}

	sipa_param_release(si_pa);

	if (si_pa->sipa_wq != NULL)
		destroy_workqueue(si_pa->sipa_wq);

	si_pa->pdev = NULL;

	put_sipa_dev(si_pa);

	return 0;
}

static void sipa_shutdown(struct platform_device *pdev)
{
	sipa_dev_t *si_pa = NULL;

	pr_debug("[debug][%s] %s: running \r\n", LOG_FLAG, __func__);

	if (NULL == pdev)
		return ;

	si_pa = (sipa_dev_t *)dev_get_drvdata(&pdev->dev);
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is NULL \r\n",
			LOG_FLAG, __func__);
		return ;
	}

	if (IS_SUPPORT_OWI_TYPE(si_pa->chip_type)) {
		sipa_suspend(si_pa);
	}
}

#ifdef CONFIG_OF
static const struct of_device_id si_sipa_dt_match[] = {
	{ .compatible = "si,sia81xx" },
	{ .compatible = "si,sia8100" },
	{ .compatible = "si,sia8109" },
	{ .compatible = "si,sia8152" },
	{ .compatible = "si,sia8152s" },
	{ .compatible = "si,sia8159" },
	{ .compatible = "si,sia81x9" },
	{ .compatible = "si,sia8152x" },
	{ .compatible = "si,sia91xx" },
	{ .compatible = "si,sia8132" },
	{ .compatible = "si,sia51xx" },
	{ }
};
MODULE_DEVICE_TABLE(of, si_sipa_dt_match);
#endif

static struct platform_driver si_sipa_dev_driver = {
	.probe  = sipa_probe,
	.remove = sipa_remove,
	.shutdown = sipa_shutdown,
	.driver = {
		.name  = SIPA_NAME,
		.owner = THIS_MODULE,
		.pm = &si_sipa_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = si_sipa_dt_match,
#endif
	},
};

/********************************************************************
 * end - sia81xx dev driver
 ********************************************************************/

static int __init sipa_pa_init(void)
{
	int ret = 0;

	pr_info("[ info][%s] %s: si_pa driver version : %s \r\n",
		LOG_FLAG, __func__, SIPA_DRIVER_VERSION);

	ret = platform_driver_register(&si_sipa_dev_driver);
	if (ret) {
		pr_err("[  err][%s] %s: si_sipa_dev error, ret = %d !!! \r\n",
			LOG_FLAG, __func__, ret);
		return ret;
	}

	ret = i2c_add_driver(&si_sipa_i2c_driver);
	if (ret) {
		pr_err("[  err][%s] %s: i2c_add_driver error, ret = %d !!! \r\n",
			LOG_FLAG, __func__, ret);
		return ret;
	}

	return 0;
}

static void __exit sipa_pa_exit(void)
{
	pr_info("[ info][%s] %s: running \r\n", LOG_FLAG, __func__);

	i2c_del_driver(&si_sipa_i2c_driver);
	platform_driver_unregister(&si_sipa_dev_driver);
}

module_init(sipa_pa_init);
module_exit(sipa_pa_exit);

MODULE_AUTHOR("yun shi <yun.shi@si-in.com>");
MODULE_DESCRIPTION("SI-IN SIA81xx ASoC driver");
MODULE_LICENSE("GPL");
