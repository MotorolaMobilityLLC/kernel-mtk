/* Goodix's GF516/GF318/GF516M/GF318M/GF518M/GF3118M/GF5118M
 *  fingerprint sensor linux driver for TEE
 *
 * 2010 - 2015 Goodix Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/kthread.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/uaccess.h>
#include <linux/ktime.h>

#include <linux/interrupt.h>
#include <linux/irq.h>
/* #include <mach/irqs.h> */
#include <linux/completion.h>
#include <linux/gpio.h>
/* #include <mach/gpio.h> */
/* #include <plat/gpio-cfg.h> */
#include <linux/timer.h>
#include <linux/fb.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#else
#include <linux/notifier.h>
#endif

#ifdef CONFIG_MTK_CLKMGR
#include "mach/mt_clkmgr.h"
#else
#include <linux/clk.h>
#endif

/* #include <mach/hardware.h> */
/* #include <mach/mt_irq.h> */
/* #include <mach/mt_gpio.h> */
/* #include <mach/mt_pm_ldo.h> */
/* #include <mach/mt_spi.h> */
/* #include <mach/upmu_common.h> */
/* #include <mach/eint.h> */
/* #include <mach/mt_spi.h> */
/* #include <mach/gpio_const.h> */

/* #include <mach/memory.h> */
/* #include <mach/mt_clkmgr.h> */
#include <net/sock.h>

/* #include <cust_eint.h> */
/* #include <cust_gpio_usage.h> */

/* MTK header */
#include "mt_spi.h"
#include "mt_spi_hal.h"
#include "mt_gpio.h"
#include "mach/gpio_const.h"

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#endif

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include "gf_spi_tee.h"

/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/spidevB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */

/* spi device name */
#define SPI_DEV_NAME   "gf_spi"
/* device name after register in character */
#define DEV_NAME "goodix_fp"

#define CHRD_DRIVER_NAME		"goodix_fp"
#define CLASS_NAME			"goodix_fp"
#define SPIDEV_MAJOR			158	/* assigned */
#define N_SPI_MINORS			32	/* ... up to 256 */
static DECLARE_BITMAP(minors, N_SPI_MINORS);

#define GF_SPI_VERSION "gf_spi_tee_v0.92"

/**************************feature control******************************/
#define NETLINK_GF_TEST 26   /* for GF test only, need defined in include/uapi/linux/netlink.h */
#define MAX_NL_MSG_LEN 16

/***********************GPIO setting port layer*************************/
/* customer hardware port layer, please change according to customer's hardware */
/* #ifdef CONFIG_MTK_LEGACY */

#ifndef GPIO_FP_INT_PIN
#define GPIO_FP_INT_PIN			(GPIO251 | 0x80000000)
#define GPIO_FP_INT_PIN_M_GPIO	GPIO_MODE_00
#define GPIO_FP_INT_PIN_M_EINT	GPIO_FP_INT_PIN_M_GPIO
#endif

/* using SPI1(0-5) on MT6797 platform */
#define GPIO_FP_SPICLK_PIN		   (GPIO234 | 0x80000000)
#define GPIO_FP_SPICLK_PIN_M_GPIO  GPIO_MODE_00
#define GPIO_FP_SPICLK_PIN_M_PWM  GPIO_MODE_03
#define GPIO_FP_SPICLK_PIN_M_SPI_CK   GPIO_MODE_01

#define GPIO_FP_SPIMISO_PIN			(GPIO235 | 0x80000000)
#define GPIO_FP_SPIMISO_PIN_M_GPIO	GPIO_MODE_00
#define GPIO_FP_SPIMISO_PIN_M_PWM  GPIO_MODE_03
#define GPIO_FP_SPIMISO_PIN_M_SPI_MI   GPIO_MODE_01

#define GPIO_FP_SPIMOSI_PIN			(GPIO236 | 0x80000000)
#define GPIO_FP_SPIMOSI_PIN_M_GPIO	GPIO_MODE_00
#define GPIO_FP_SPIMOSI_PIN_M_MDEINT  GPIO_MODE_02
#define GPIO_FP_SPIMOSI_PIN_M_PWM  GPIO_MODE_03
#define GPIO_FP_SPIMOSI_PIN_M_SPI_MO   GPIO_MODE_01

#define GPIO_FP_SPICS_PIN		  (GPIO237 | 0x80000000)
#define GPIO_FP_SPICS_PIN_M_GPIO  GPIO_MODE_00
#define GPIO_FP_SPICS_PIN_M_MDEINT	GPIO_MODE_02
#define GPIO_FP_SPICS_PIN_M_PWM  GPIO_MODE_03
#define GPIO_FP_SPICS_PIN_M_SPI_CS	 GPIO_MODE_01

/* GPIO122 used for VOLUMN_UP & VOLUMN_DOWN, change to GPIO8 temporary */
/* #define GPIO_FP_RESET_PIN		  (GPIO122 | 0x80000000) */
/* #define GPIO_FP_RESET_PIN		  (GPIO8 | 0x80000000) */
#define GPIO_FP_RESET_PIN		  (GPIO252 | 0x80000000)
#define GPIO_FP_RESET_PIN_M_GPIO  GPIO_MODE_00

/* #define GPIO_FP_SPI_BYPASS_PIN		   (GPIO133 | 0x80000000) */
/* #define GPIO_FP_SPI_BYPASS_M_GPIO  GPIO_MODE_00 */

#ifndef	CUST_EINT_FP_EINT_NUM
#define CUST_EINT_FP_EINT_NUM			   5
#define CUST_EINT_FP_EINT_DEBOUNCE_CN	   0
#define CUST_EINT_FP_EINT_TYPE			   EINTF_TRIGGER_RISING /* EINTF_TRIGGER_LOW */
#define CUST_EINT_FP_EINT_DEBOUNCE_EN	   0  /* CUST_EINT_DEBOUNCE_DISABLE */
#endif

/* #endif */ /* CONFIG_MTK_LEGACY */

#ifndef GF_INPUT_HOME_KEY
/* on MTK 6795 EVB board, home key has been redefine to KEY_HOMEPAGE!!! */
/* double check the define on customer board!!! */
#define GF_INPUT_HOME_KEY KEY_HOMEPAGE /* KEY_HOME */

#define GF_INPUT_MENU_KEY  KEY_MENU
#define GF_INPUT_BACK_KEY  KEY_BACK

#define GF_INPUT_FF_KEY  KEY_POWER

#define GF_INPUT_OTHER_KEY KEY_VOLUMEDOWN  /* temporary key value for capture use */
#endif

/*************************************************************/
struct gf_dev *g_gf_dev = NULL;

/* align=2, 2 bytes align */
/* align=4, 4 bytes align */
/* align=8, 8 bytes align */
#define ROUND_UP(x, align)		((x+(align-1))&~(align-1))

/**************************debug******************************/
#define ERR_LOG  (0)
#define INFO_LOG (1)
#define DEBUG_LOG (2)

/* debug log setting */
u8 g_debug_level = DEBUG_LOG;

#define gf_debug(level, fmt, args...) do { \
			if (g_debug_level >= level) {\
				pr_debug("[gf] " fmt, ##args); \
			} \
		} while (0)

#define FUNC_ENTRY()  gf_debug(DEBUG_LOG, "%s, %d, entry\n", __func__, __LINE__)
#define FUNC_EXIT()  gf_debug(DEBUG_LOG, "%s, %d, exit\n", __func__, __LINE__)

/*************************************************************/
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = (150*150);
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

/* -------------------------------------------------------------------- */
/* timer function								*/
/* -------------------------------------------------------------------- */
#define TIME_START	   0
#define TIME_STOP	   1

static long int prev_time, cur_time;

long int kernel_time(unsigned int step)
{
	cur_time = ktime_to_us(ktime_get());
	if (step == TIME_START) {
		prev_time = cur_time;
		return 0;
	} else if (step == TIME_STOP) {
		gf_debug(DEBUG_LOG, "%s, use: %ld us\n", __func__, (cur_time - prev_time));
		return cur_time - prev_time;
	}
	/* else { */
		prev_time = cur_time;
		return -1;
	/* } */
}

/* -------------------------------------------------------------------- */
/* fingerprint chip hardware configuration								  */
/* -------------------------------------------------------------------- */
static int gf_get_gpio_info(void)
{
#ifdef CONFIG_OF
	int ret;

	struct device_node *node;
	struct platform_device *pdev;

	gf_debug(DEBUG_LOG, "%s, from dts pinctrl\n", __func__);

	node = of_find_compatible_node(NULL, NULL, "mediatek,goodix-fp");
	if (node) {
		pdev = of_find_device_by_node(node);
		if (pdev) {
			g_gf_dev->pinctrl_gpios = devm_pinctrl_get(&pdev->dev);
			if (IS_ERR(g_gf_dev->pinctrl_gpios)) {
				ret = PTR_ERR(g_gf_dev->pinctrl_gpios);
				gf_debug(ERR_LOG, "%s can't find fingerprint pinctrl\n", __func__);
				return ret;
			}
		} else {
			gf_debug(ERR_LOG, "%s platform device is null\n", __func__);
		}
	} else {
		gf_debug(ERR_LOG, "%s device node is null\n", __func__);
	}

	/* it's normal that get "default" will failed */
	g_gf_dev->pins_default = pinctrl_lookup_state(g_gf_dev->pinctrl_gpios, "default");
	if (IS_ERR(g_gf_dev->pins_default)) {
		ret = PTR_ERR(g_gf_dev->pins_default);
		gf_debug(ERR_LOG, "%s can't find fingerprint pinctrl default\n", __func__);
		/* return ret; */
	}
	g_gf_dev->pins_miso_spi = pinctrl_lookup_state(g_gf_dev->pinctrl_gpios, "miso_spi");
	if (IS_ERR(g_gf_dev->pins_miso_spi)) {
		ret = PTR_ERR(g_gf_dev->pins_miso_spi);
		gf_debug(ERR_LOG, "%s can't find fingerprint pinctrl miso_spi\n", __func__);
		return ret;
	}
	g_gf_dev->pins_miso_pullhigh = pinctrl_lookup_state(g_gf_dev->pinctrl_gpios, "miso_pullhigh");
	if (IS_ERR(g_gf_dev->pins_miso_pullhigh)) {
		ret = PTR_ERR(g_gf_dev->pins_miso_pullhigh);
		gf_debug(ERR_LOG, "%s can't find fingerprint pinctrl miso_pullhigh\n", __func__);
		return ret;
	}
	g_gf_dev->pins_miso_pulllow = pinctrl_lookup_state(g_gf_dev->pinctrl_gpios, "miso_pulllow");
	if (IS_ERR(g_gf_dev->pins_miso_pulllow)) {
		ret = PTR_ERR(g_gf_dev->pins_miso_pulllow);
		gf_debug(ERR_LOG, "%s can't find fingerprint pinctrl miso_pulllow\n", __func__);
		return ret;
	}
	g_gf_dev->pins_reset_high = pinctrl_lookup_state(g_gf_dev->pinctrl_gpios, "reset_high");
	if (IS_ERR(g_gf_dev->pins_reset_high)) {
		ret = PTR_ERR(g_gf_dev->pins_reset_high);
		gf_debug(ERR_LOG, "%s can't find fingerprint pinctrl reset_high\n", __func__);
		return ret;
	}
	g_gf_dev->pins_reset_low = pinctrl_lookup_state(g_gf_dev->pinctrl_gpios, "reset_low");
	if (IS_ERR(g_gf_dev->pins_reset_low)) {
		ret = PTR_ERR(g_gf_dev->pins_reset_low);
		gf_debug(ERR_LOG, "%s can't find fingerprint pinctrl reset_low\n", __func__);
		return ret;
	}
	gf_debug(DEBUG_LOG, "%s, get pinctrl success!\n", __func__);

#endif
	return 0;
}

static void gf_hw_power_enable(u8 bonoff)
{
	/* TODO: LDO configure */
#if 0
	if (bonoff)
		hwPowerOn(MT6331_POWER_LDO_VIBR, VOL_2800, "fingerprint");
	else
		hwPowerDown(MT6331_POWER_LDO_VIBR, "fingerprint");
#endif
	/* return; */
}

static void gf_spi_clk_enable(u8 bonoff)
{
#ifdef CONFIG_MTK_CLKMGR
	if (bonoff)
		enable_clock(MT_CG_PERI_SPI0, "spi");
	else
		disable_clock(MT_CG_PERI_SPI0, "spi");

#else
	/* changed after MT6797 platform */
	struct mt_spi_t *ms;

	ms = spi_master_get_devdata(g_gf_dev->spi->master);

	if (bonoff)
		mt_spi_enable_clk(ms);
	else
		mt_spi_disable_clk(ms);
#endif
	/* return; */
}

static void gf_bypass_flash_gpio_cfg(void)
{
	/* TODO: by pass flash IO config, default connect to GND */
	/* return; */
}

/* Confure gpio for SPI if necessary */
static void gf_spi_gpio_cfg(void)
{
#ifdef CONFIG_MTK_LEGACY
	mt_set_gpio_mode(GPIO_FP_SPICLK_PIN, GPIO_FP_SPICLK_PIN_M_SPI_CK);
	mt_set_gpio_mode(GPIO_FP_SPIMISO_PIN, GPIO_FP_SPIMISO_PIN_M_SPI_MI);
	mt_set_gpio_mode(GPIO_FP_SPIMOSI_PIN, GPIO_FP_SPIMOSI_PIN_M_SPI_MO);
	mt_set_gpio_mode(GPIO_FP_SPICS_PIN, GPIO_FP_SPICS_PIN_M_SPI_CS);
#else
	/* TODO: config SPI1(0 to 5)to SPI mode using functions provided by SPI module */

#endif
	/* return; */
}

/* pull high miso, or change to SPI mode */
static void gf_miso_gpio_cfg(u8 pullhigh)
{
#ifdef CONFIG_MTK_LEGACY
	if (pullhigh) {
		mt_set_gpio_pull_select(GPIO_FP_SPIMISO_PIN, GPIO_PULL_UP);
		mt_set_gpio_pull_enable(GPIO_FP_SPIMISO_PIN, GPIO_PULL_ENABLE);
	} else {
		mt_set_gpio_pull_enable(GPIO_FP_SPIMISO_PIN, GPIO_PULL_DISABLE);
	}
#endif

#ifdef CONFIG_OF
	if (pullhigh)
		pinctrl_select_state(g_gf_dev->pinctrl_gpios, g_gf_dev->pins_miso_pullhigh);
	else
		pinctrl_select_state(g_gf_dev->pinctrl_gpios, g_gf_dev->pins_miso_spi);

#endif
	/* return; */
}

static void gf_irq_gpio_cfg(void)
{
#ifdef CONFIG_MTK_LEGACY
	mt_set_gpio_mode(GPIO_FP_INT_PIN, GPIO_FP_INT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_FP_INT_PIN, GPIO_PULL_DISABLE);
	/* mt_set_gpio_dir(GPIO_FP_INT_PIN, GPIO_DIR_IN); */
	/* mt_eint_set_hw_debounce(CUST_EINT_FP_EINT_NUM, CUST_EINT_FP_EINT_DEBOUNCE_CN); */

#if 0
	mt_eint_registration(CUST_EINT_FP_EINT_NUM, CUST_EINT_FP_EINT_TYPE, gf_irq, 0);
	mt_eint_unmask(CUST_EINT_FP_EINT_NUM);
#endif

#endif

#ifdef CONFIG_OF
	struct device_node *node;

	/* use fpc1145 eint defination temporary */
	node = of_find_compatible_node(NULL, NULL, "mediatek,fpc1145");
	if (node) {
		g_gf_dev->irq_num = irq_of_parse_and_map(node, 0);
		gf_debug(INFO_LOG, "%s, gf_irq = %d\n", __func__, g_gf_dev->irq_num);
		g_gf_dev->spi->irq = g_gf_dev->irq_num;
	} else
		gf_debug(ERR_LOG, "%s can't find compatible node\n", __func__);

	/* return; */
#endif
}

static void gf_reset_gpio_cfg(void)
{
#ifdef CONFIG_MTK_LEGACY
	mt_set_gpio_mode(GPIO_FP_RESET_PIN, GPIO_FP_RESET_PIN_M_GPIO);
	mt_set_gpio_pull_select(GPIO_FP_RESET_PIN, GPIO_PULL_UP);
	mt_set_gpio_pull_enable(GPIO_FP_RESET_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(GPIO_FP_RESET_PIN, GPIO_DIR_OUT);
#endif

#ifdef CONFIG_OF
	pinctrl_select_state(g_gf_dev->pinctrl_gpios, g_gf_dev->pins_reset_high);
#endif

	/* return; */
}

/* delay ms after reset */
static void gf_hw_reset(u8 delay)
{
#ifdef CONFIG_MTK_LEGACY
	mt_set_gpio_out(GPIO_FP_RESET_PIN, GPIO_OUT_ZERO);
	mdelay(5);
	mt_set_gpio_out(GPIO_FP_RESET_PIN, GPIO_OUT_ONE);
#endif

#ifdef CONFIG_OF
	pinctrl_select_state(g_gf_dev->pinctrl_gpios, g_gf_dev->pins_reset_low);
	mdelay(5);
	pinctrl_select_state(g_gf_dev->pinctrl_gpios, g_gf_dev->pins_reset_high);
#endif

	if (delay) {
		/* delay is configurable */
		mdelay(delay);
	}
	/* return; */
}

static void gf_enable_irq(struct gf_dev *gf_dev)
{
	if (0 == gf_dev->device_available) {
		gf_debug(ERR_LOG, "%s, devices not available\n", __func__);
	} else {
		if (1 == gf_dev->irq_count) {
			gf_debug(ERR_LOG, "%s, irq already enabled\n", __func__);
		} else {
			enable_irq(gf_dev->spi->irq);
			gf_dev->irq_count = 1;
			gf_debug(DEBUG_LOG, "%s enable interrupt!\n", __func__);
		}
	}
	/* return; */
}

static void gf_disable_irq(struct gf_dev *gf_dev)
{
	if (0 == gf_dev->device_available) {
		gf_debug(ERR_LOG, "%s, devices not available\n", __func__);
	} else {
		if (0 == gf_dev->irq_count) {
			gf_debug(ERR_LOG, "%s, irq already disabled\n", __func__);
		} else {
			disable_irq(gf_dev->spi->irq);
			gf_dev->irq_count = 0;
			gf_debug(DEBUG_LOG, "%s disable interrupt!\n", __func__);
		}
	}
	/* return; */
}

#ifdef GF_SPI_REE_OPERATION
/* -------------------------------------------------------------------- */
/* normal world SPI read/write function                 */
/* -------------------------------------------------------------------- */
static u8 tmp_buf[64];
/* read out data start from offset 4!! */
static int gf_spi_read_bytes_ree(u16 addr, u32 data_len, u8 *rx_buf)
{
	struct spi_message msg;
	struct spi_transfer *xfer;

	xfer = kzalloc(sizeof(*xfer)*2, GFP_KERNEL);
	if (xfer == NULL) {
		/* gf_debug(ERR_LOG, "%s, no memory for SPI transfer\n", __func__); */
		return -ENOMEM;
	}

	spi_message_init(&msg);
	rx_buf[0] = 0xF0;
	rx_buf[1] = (u8)((addr>>8)&0xFF);
	rx_buf[2] = (u8)(addr&0xFF);
	xfer[0].tx_buf = rx_buf;
	xfer[0].len = 3;
	xfer[0].delay_usecs = 5;
	spi_message_add_tail(&xfer[0], &msg);
	spi_sync(g_gf_dev->spi, &msg);

	spi_message_init(&msg);
	memset(rx_buf, 0xff, data_len + 1);
	rx_buf[3] = 0xF1;
	xfer[1].tx_buf = &rx_buf[3];
	xfer[1].rx_buf = &rx_buf[3];
	xfer[1].len = data_len + 1;
	xfer[1].delay_usecs = 5;
	spi_message_add_tail(&xfer[1], &msg);
	spi_sync(g_gf_dev->spi, &msg);

	kfree(xfer);
	if (xfer != NULL)
		xfer = NULL;

	return 0;
}

static int gf_spi_write_byte_ree(u16 addr, u8 value)
{
	struct spi_message msg;
	struct spi_transfer *xfer;

	xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
	if (xfer == NULL) {
		/* gf_debug(ERR_LOG, "%s, no memory for SPI transfer\n", __func__); */
		return -ENOMEM;
	}

	spi_message_init(&msg);
	tmp_buf[0] = 0xF0;
	tmp_buf[1] = (u8)((addr>>8)&0xFF);
	tmp_buf[2] = (u8)(addr&0xFF);
	tmp_buf[3] = value;

	xfer[0].tx_buf = tmp_buf;
	xfer[0].len = 3 + 1;
	xfer[0].delay_usecs = 5;
	spi_message_add_tail(&xfer[0], &msg);
	spi_sync(g_gf_dev->spi, &msg);

	kfree(xfer);
	if (xfer != NULL)
		xfer = NULL;

	return 0;
}
#endif

#ifdef GF_NETLINK
/* -------------------------------------------------------------------- */
/* netlink functions                 */
/* -------------------------------------------------------------------- */
void gf_netlink_send(const int command)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
	int ret;

	gf_debug(INFO_LOG, "[%s] : enter, send command %d\n", __func__, command);
	if (NULL == g_gf_dev->nl_sk) {
		gf_debug(ERR_LOG, "[%s] : invalid socket\n", __func__);
		return;
	}

	if (0 == g_gf_dev->pid) {
		gf_debug(ERR_LOG, "[%s] : invalid native process pid\n", __func__);
		return;
	}

	/*alloc data buffer for sending to native*/
	/*malloc data space at least 1500 bytes, which is ethernet data length*/
	skb = alloc_skb(NLMSG_SPACE(MAX_NL_MSG_LEN), GFP_ATOMIC);
	if (skb == NULL) {
		/* gf_debug(ERR_LOG, "[%s] : allocate skb failed\n", __func__); */
		return;
	}

	nlh = nlmsg_put(skb, 0, 0, 0, MAX_NL_MSG_LEN, 0);

	NETLINK_CB(skb).portid = 0;
	NETLINK_CB(skb).dst_group = 0;

	*(char *)NLMSG_DATA(nlh) = command;
	ret = netlink_unicast(g_gf_dev->nl_sk, skb, g_gf_dev->pid, MSG_DONTWAIT);
	if (ret == 0) {
		gf_debug(ERR_LOG, "[%s] : send failed\n", __func__);
		return;
	}
	/* else { */
		gf_debug(INFO_LOG, "[%s] : send done, data length is %d\n", __func__, ret);
	/* return; */
	/* } */
}

static void gf_netlink_recv(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	char str[128];

	gf_debug(INFO_LOG, "[%s] : enter\n", __func__);

	skb = skb_get(__skb);
	if (skb == NULL) {
		gf_debug(ERR_LOG, "[%s] : skb_get return NULL\n", __func__);
		return;
	}

	/* presume there is 5byte payload at leaset */
	if (skb->len >= NLMSG_SPACE(0)) {
		nlh = nlmsg_hdr(skb);
		memcpy(str, NLMSG_DATA(nlh), sizeof(str));
		g_gf_dev->pid = nlh->nlmsg_pid;
		gf_debug(INFO_LOG, "[%s] : pid: %d, msg: %s\n", __func__, g_gf_dev->pid, str);

	} else {
		gf_debug(ERR_LOG, "[%s] : not enough data length\n", __func__);
	}

	kfree_skb(skb);
	/* return; */
}

static int gf_netlink_init(void)
{
	struct netlink_kernel_cfg cfg;

	memset(&cfg, 0, sizeof(struct netlink_kernel_cfg));
	cfg.input = gf_netlink_recv;

	g_gf_dev->nl_sk = netlink_kernel_create(&init_net, NETLINK_GF_TEST, &cfg);
	if (g_gf_dev->nl_sk == NULL) {
		gf_debug(ERR_LOG, "[%s] : netlink create failed\n", __func__);
		return -1;
	}
	/* else { */
		gf_debug(INFO_LOG, "[%s] : netlink create success\n", __func__);
		return 0;
	/* } */
}

static int gf_netlink_destroy(void)
{
	if (g_gf_dev->nl_sk != NULL) {
		/* sock_release(g_gf_dev->nl_sk->sk_socket); */
		netlink_kernel_release(g_gf_dev->nl_sk);
		return 0;
	}
	/* else { */
		gf_debug(ERR_LOG, "[%s] : no netlink socket yet\n", __func__);
		return -1;
	/* } */
}
#endif


/* -------------------------------------------------------------------- */
/* early suspend callback and suspend/resume functions          */
/* -------------------------------------------------------------------- */
#ifdef CONFIG_HAS_EARLYSUSPEND
static void gf_early_suspend(struct early_suspend *handler)
{
	struct gf_dev *gf_dev;

	gf_dev = container_of(handler, struct gf_dev, early_suspend);
	gf_debug(INFO_LOG, "[%s] enter\n", __func__);

	gf_dev->is_sleep_mode = 0;
	gf_netlink_send(GF_NETLINK_SCREEN_OFF);
	gf_dev->device_available = 0;
}

static void gf_late_resume(struct early_suspend *handler)
{
	struct gf_dev *gf_dev;

	gf_dev = container_of(handler, struct gf_dev, early_suspend);
	gf_debug(INFO_LOG, "[%s] enter\n", __func__);

	/* first check whether chip is still in sleep mode */
	if (gf_dev->is_sleep_mode == 1) {
		gf_hw_reset(60);
		gf_dev->is_sleep_mode = 0;
	}

	gf_netlink_send(GF_NETLINK_SCREEN_ON);
	gf_dev->device_available = 1;
}
#else

static int gf_fb_notifier_callback(struct notifier_block *self,
			unsigned long event, void *data)
{
	struct gf_dev *gf_dev;
	struct fb_event *evdata = data;
	unsigned int blank;
	int retval = 0;

	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK /* FB_EARLY_EVENT_BLANK */)
		return 0;

	gf_dev = container_of(self, struct gf_dev, notifier);
	blank = *(int *)evdata->data;

	gf_debug(INFO_LOG, "[%s] : enter, blank=0x%x\n", __func__, blank);

	switch (blank) {
	case FB_BLANK_UNBLANK:
		gf_debug(INFO_LOG, "[%s] : lcd on notify\n", __func__);
		/* first check whether chip is still in sleep mode */
		if (gf_dev->is_sleep_mode == 1) {
			gf_hw_reset(60);
			gf_dev->is_sleep_mode = 0;
		}
		gf_netlink_send(GF_NETLINK_SCREEN_ON);
		gf_dev->device_available = 1;
		break;

	case FB_BLANK_POWERDOWN:
		gf_debug(INFO_LOG, "[%s] : lcd off notify\n", __func__);
		gf_dev->is_sleep_mode = 0;
		gf_netlink_send(GF_NETLINK_SCREEN_OFF);
		gf_dev->device_available = 0;
		break;

	default:
		gf_debug(INFO_LOG, "[%s] : other notifier, ignore\n", __func__);
		break;
	}
	return retval;
}

#endif

#ifdef CONFIG_PM
static int gf_suspend(struct spi_device *spi, pm_message_t msg)
{
	/* do nothing while enter suspend */
	gf_debug(INFO_LOG, "%s: enter\n", __func__);
	return 0;
}

static int gf_resume(struct spi_device *spi)
{
	struct gf_dev	 *gf_dev;

	gf_dev = spi_get_drvdata(spi);

	gf_debug(INFO_LOG, "%s: enter\n", __func__);

	if (gf_dev->is_sleep_mode == 1) {
		/* wake up sensor... */
		gf_hw_reset(5);
		gf_dev->is_sleep_mode = 0;
	}
	return 0;
}
#endif

/* -------------------------------------------------------------------- */
/* file operation function                                              */
/* -------------------------------------------------------------------- */

static ssize_t gf_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	gf_debug(ERR_LOG, "Not support read opertion in TEE version\n");
	return -EFAULT;
}

static ssize_t gf_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	gf_debug(ERR_LOG, "Not support write opertion in TEE version\n");
	return -EFAULT;
}

static irqreturn_t gf_irq(int irq, void *handle)
{
	struct gf_dev *gf_dev = (struct gf_dev *)handle;
	/* struct gf_dev *dev = g_gf_dev; */

	FUNC_ENTRY();

#ifdef GF_FASYNC
	kill_fasync(&dev->async, SIGIO, POLL_IN);
#endif

#ifdef GF_NETLINK
	gf_netlink_send(GF_NETLINK_IRQ);
#endif
	gf_dev->sig_count++;

#if 0
	tmp_buf[4] = 0x55;
	tmp_buf[5] = 0x55;
	gf_spi_read_bytes_ree(0x8140, 2, tmp_buf);
	gf_debug(INFO_LOG, "%s, read status is 0x%x, 0x%x\n", __func__, tmp_buf[4], tmp_buf[5]);

	gf_spi_write_byte_ree(0x8140, tmp_buf[4]&0x7F);
#endif

	FUNC_EXIT();
	return IRQ_HANDLED;
}


static long gf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct gf_dev *gf_dev = NULL;
	struct gf_key gf_key;
	uint32_t key_event;
	int retval = 0;

	FUNC_ENTRY();
	if (_IOC_TYPE(cmd) != GF_IOC_MAGIC)
		return -ENOTTY;

	/* Check access direction once here; don't repeat below.
	* IOC_DIR is from the user perspective, while access_ok is
	* from the kernel perspective; so they look reversed.
	*/
	if (_IOC_DIR(cmd) & _IOC_READ)
		retval = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));

	if (retval == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		retval = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (retval)
		return -EFAULT;

	gf_dev = (struct gf_dev *)filp->private_data;

	switch (cmd) {
	case GF_IOC_INIT:
		gf_debug(INFO_LOG, "%s: gf init started======\n", __func__);
		gf_irq_gpio_cfg();
		retval = request_threaded_irq(gf_dev->spi->irq, NULL, gf_irq,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT, dev_name(&(gf_dev->spi->dev)), gf_dev);
		if (!retval)
			gf_debug(INFO_LOG, "%s irq thread request success!\n", __func__);
		else
			gf_debug(ERR_LOG, "%s irq thread request failed, retval=%d\n", __func__, retval);

		gf_dev->device_available = 1;
		gf_dev->irq_count = 1;
		gf_disable_irq(gf_dev);

#if defined(CONFIG_HAS_EARLYSUSPEND)
		gf_debug(INFO_LOG, "[%s] : register_early_suspend\n", __func__);
		gf_dev->early_suspend.level = (EARLY_SUSPEND_LEVEL_DISABLE_FB - 1);
		gf_dev->early_suspend.suspend = gf_early_suspend,
		gf_dev->early_suspend.resume = gf_late_resume,
		register_early_suspend(&gf_dev->early_suspend);
#else
		/* register screen on/off callback */
		gf_dev->notifier.notifier_call = gf_fb_notifier_callback;
		fb_register_client(&gf_dev->notifier);
#endif

		gf_dev->sig_count = 0;

		gf_debug(INFO_LOG, "%s: gf init finished======\n", __func__);
		break;

	case GF_IOC_EXIT:
		gf_disable_irq(gf_dev);
		if (gf_dev->spi->irq) {
			free_irq(gf_dev->spi->irq, gf_dev);
			gf_dev->irq_count = 0;
		}
		gf_dev->device_available = 0;

		#ifdef CONFIG_HAS_EARLYSUSPEND
		if (gf_dev->early_suspend.suspend)
			unregister_early_suspend(&gf_dev->early_suspend);
		#endif
		gf_debug(INFO_LOG, "%s: gf exit finished======\n", __func__);

		break;

	case GF_IOC_RESET:
		gf_debug(INFO_LOG, "%s: chip reset command\n", __func__);
		gf_hw_reset(60);
		break;

	case GF_IOC_ENABLE_IRQ:
		gf_enable_irq(gf_dev);
		break;

	case GF_IOC_DISABLE_IRQ:
		gf_disable_irq(gf_dev);
		break;

	case GF_IOC_ENABLE_SPI_CLK:
		gf_spi_clk_enable(1);
		break;

	case GF_IOC_DISABLE_SPI_CLK:
		gf_spi_clk_enable(0);
		break;

	case GF_IOC_INPUT_KEY_EVENT:
		if (copy_from_user(&gf_key, (struct gf_key *)arg, sizeof(struct gf_key))) {
			gf_debug(ERR_LOG, "Failed to copy input key event from user to kernel\n");
			retval = -EFAULT;
			break;
		}

		if (GF_KEY_HOME == gf_key.key) {
			key_event = GF_INPUT_HOME_KEY;
		} else if (GF_KEY_POWER == gf_key.key) {
			key_event = GF_INPUT_FF_KEY;
		} else {
			/* add special key define */
			key_event = GF_INPUT_OTHER_KEY;
		}
		gf_debug(INFO_LOG, "%s: received key event[%d], key=%d, value=%d\n",
								 __func__, key_event, gf_key.key, gf_key.value);
		input_report_key(gf_dev->input, key_event, gf_key.value);
		input_sync(gf_dev->input);
		break;

	case GF_IOC_ENTER_SLEEP_MODE:
		gf_dev->is_sleep_mode = 1;
		break;

	default:
		gf_debug(ERR_LOG, "gf doesn't support this command(%d)\n", cmd);
		break;
	}

	FUNC_EXIT();
	return retval;
}

#ifdef CONFIG_COMPAT
static long gf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;

	FUNC_ENTRY();

	retval = filp->f_op->unlocked_ioctl(filp, cmd, arg);
	FUNC_EXIT();
	return retval;
}
#endif

static unsigned int gf_poll(struct file *filp, struct poll_table_struct *wait)
{
	gf_debug(ERR_LOG, "Not support poll opertion in TEE version\n");
	return -EFAULT;
}


/* -------------------------------------------------------------------- */
/* devfs                                                              */
/* -------------------------------------------------------------------- */
static ssize_t gf_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	gf_debug(INFO_LOG, "%s: Show debug_level = 0x%x\n", __func__, g_debug_level);
	return sprintf(buf, "0x%x\n", g_debug_level);
}

static uint8_t test_flag;

static ssize_t gf_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct gf_dev *gf_dev =  dev_get_drvdata(dev);
	int retval = 0;

	if (!strncmp(buf, "-10", 3)) {
		gf_debug(INFO_LOG, "%s: parameter is -10, gf init start===============\n", __func__);

		gf_dev->device_available = 1;
		gf_irq_gpio_cfg();
		retval = request_threaded_irq(gf_dev->spi->irq, NULL, gf_irq,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT, dev_name(&(gf_dev->spi->dev)), gf_dev);
		if (!retval)
			gf_debug(INFO_LOG, "%s irq thread request success!\n", __func__);
		else
			gf_debug(ERR_LOG, "%s irq thread request failed, retval=%d\n", __func__, retval);

		gf_dev->irq_count = 1;
		gf_disable_irq(gf_dev);

#if defined(CONFIG_HAS_EARLYSUSPEND)
		gf_debug(INFO_LOG, "[%s] : register_early_suspend\n", __func__);
		gf_dev->early_suspend.level = (EARLY_SUSPEND_LEVEL_DISABLE_FB - 1);
		gf_dev->early_suspend.suspend = gf_early_suspend,
		gf_dev->early_suspend.resume = gf_late_resume,
		register_early_suspend(&gf_dev->early_suspend);
#endif
		gf_dev->sig_count = 0;

		gf_debug(INFO_LOG, "%s: gf init finished======\n", __func__);

	} else if (!strncmp(buf, "-11", 3)) {
		gf_debug(INFO_LOG, "%s: parameter is -11, enable irq===============\n", __func__);
		gf_enable_irq(gf_dev);

	} else if (!strncmp(buf, "-12", 3)) {
		gf_debug(INFO_LOG, "%s: parameter is -12, REE SPI read/write test=======\n", __func__);
#ifdef GF_SPI_REE_OPERATION
		gf_spi_read_bytes_ree(0x4220, 4, tmp_buf);
		gf_debug(INFO_LOG, "%s, 9p chp version is 0x%x, 0x%x, 0x%x, 0x%x\n", __func__,
				tmp_buf[4], tmp_buf[5], tmp_buf[6], tmp_buf[7]);

		gf_spi_read_bytes_ree(0x8000, 10, tmp_buf);
		gf_debug(INFO_LOG, "%s, read chip version 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, %02d.%02d.%02d\n", __func__,
				tmp_buf[4], tmp_buf[5], tmp_buf[6], tmp_buf[7], tmp_buf[8], tmp_buf[9],
				tmp_buf[11], tmp_buf[12], tmp_buf[13]);
#endif

	} else if (!strncmp(buf, "-13", 3)) {
		gf_debug(INFO_LOG, "%s: parameter is -13, set/read sensor mode==========\n", __func__);
#ifdef GF_SPI_REE_OPERATION
		gf_spi_read_bytes_ree(0x8043, 1, tmp_buf);
		gf_debug(INFO_LOG, "%s, read out chip mode=0x%x\n", __func__, tmp_buf[4]);

		gf_spi_write_byte_ree(0x8043, 1);
		gf_spi_read_bytes_ree(0x8043, 1, tmp_buf);
		gf_debug(INFO_LOG, "%s, read out chip mode=0x%x after set to KEY mode\n", __func__, tmp_buf[4]);
#endif

	} else if (!strncmp(buf, "-14", 3)) {
		gf_debug(INFO_LOG, "%s: parameter is -14, GPIO test===============\n", __func__);
		gf_reset_gpio_cfg();
#ifdef CONFIG_MTK_LEGACY
		if (test_flag == 0) {
			mt_set_gpio_out(GPIO_FP_RESET_PIN, GPIO_OUT_ZERO);
			gf_debug(INFO_LOG, "%s: set reset PIN to zero\n", __func__);
			test_flag = 1;
		} else {
			mt_set_gpio_out(GPIO_FP_RESET_PIN, GPIO_OUT_ONE);
			gf_debug(INFO_LOG, "%s: set reset PIN to one\n", __func__);
			test_flag = 0;
		}
#endif

#ifdef CONFIG_OF
		if (test_flag == 0) {
			pinctrl_select_state(g_gf_dev->pinctrl_gpios, g_gf_dev->pins_miso_pulllow);
			gf_debug(INFO_LOG, "%s: set miso PIN to low\n", __func__);
			test_flag = 1;
		} else {
			pinctrl_select_state(g_gf_dev->pinctrl_gpios, g_gf_dev->pins_miso_pullhigh);
			gf_debug(INFO_LOG, "%s: set miso PIN to high\n", __func__);
			test_flag = 0;
		}
#endif

	} else {
		gf_debug(ERR_LOG, "%s: wrong parameter!===============\n", __func__);
	}

	return count;
}


static DEVICE_ATTR(debug, S_IRUGO|S_IWUSR, gf_debug_show, gf_debug_store);

static struct attribute *gf_debug_attrs[] = {
	&dev_attr_debug.attr,
	NULL
};

static const struct attribute_group gf_debug_attr_group = {
	.attrs = gf_debug_attrs,
	.name = "debug"
};

/* -------------------------------------------------------------------- */
/* device function								  */
/* -------------------------------------------------------------------- */
static int gf_open(struct inode *inode, struct file *filp)
{
	struct gf_dev *gf_dev;
	int status = -ENXIO;

	FUNC_ENTRY();
	mutex_lock(&device_list_lock);

	list_for_each_entry(gf_dev, &device_list, device_entry) {
		if (gf_dev->devt == inode->i_rdev) {
			gf_debug(INFO_LOG, "%s, Found\n", __func__);
			status = 0;
			break;
		}
	}

	if (status == 0) {
		mutex_lock(&gf_dev->buf_lock);
		if (gf_dev->buffer == NULL) {
			gf_dev->buffer = kzalloc(bufsiz, GFP_KERNEL);
			if (gf_dev->buffer == NULL) {
				gf_debug(ERR_LOG, "%s, allocate dev->buffer failed\n", __func__);
				status = -ENOMEM;
			}
		}
		mutex_unlock(&gf_dev->buf_lock);

		if (status == 0) {
			gf_dev->users++;
			filp->private_data = gf_dev;
			nonseekable_open(inode, filp);
			gf_debug(INFO_LOG, "%s, Succeed to open device. irq = %d\n", __func__, gf_dev->spi->irq);
		}
	} else {
		gf_debug(ERR_LOG, "%s, No device for minor %d\n", __func__, iminor(inode));
	}
	mutex_unlock(&device_list_lock);
	FUNC_EXIT();
	return status;
}

#ifdef GF_FASYNC
static int gf_fasync(int fd, struct file *filp, int mode)
{
	struct gf_dev *gf_dev = filp->private_data;
	int ret;

	FUNC_ENTRY();
	ret = fasync_helper(fd, filp, mode, &gf_dev->async);
	FUNC_EXIT();
	return ret;
}
#endif

static int gf_release(struct inode *inode, struct file *filp)
{
	struct gf_dev *gf_dev;
	int    status = 0;

	FUNC_ENTRY();
	mutex_lock(&device_list_lock);
	gf_dev = filp->private_data;
	filp->private_data = NULL;

	/*last close??*/
	gf_dev->users--;
	if (!gf_dev->users) {
		gf_debug(INFO_LOG, "%s, disble_irq. irq = %d\n", __func__, gf_dev->spi->irq);
		gf_disable_irq(gf_dev);
	}
	mutex_unlock(&device_list_lock);
	gf_dev->device_available = 0;
	FUNC_EXIT();
	return status;
}


static struct class *gf_spi_class;

static const struct file_operations gf_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	* gets more complete API coverage.	It'll simplify things
	* too, except for the locking.
	*/
	.write =	gf_write,
	.read =		gf_read,
	.unlocked_ioctl = gf_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gf_compat_ioctl,
#endif
	.open =		gf_open,
	.release =	gf_release,
	.poll	= gf_poll,
#ifdef GF_FASYNC
	.fasync = gf_fasync,
#endif
};

/*-------------------------------------------------------------------------*/

static int gf_probe(struct spi_device *spi)
{
	struct gf_dev *gf_dev = NULL;
	unsigned long minor;
	int status = -EINVAL;

	FUNC_ENTRY();
	/* Allocate driver data */
	gf_dev = kzalloc(sizeof(struct gf_dev), GFP_KERNEL);
	if (!gf_dev) {
		/* gf_debug(ERR_LOG, "%s, Failed to alloc memory for gf device.\n", __func__); */
		FUNC_EXIT();
		return -ENOMEM;
	}

	g_gf_dev = gf_dev;
	/* Initialize the driver data */
	gf_dev->spi = spi;
	spin_lock_init(&gf_dev->spi_lock);
	mutex_init(&gf_dev->buf_lock);

	INIT_LIST_HEAD(&gf_dev->device_entry);
	gf_dev->device_available = 0;
	gf_dev->spi->irq = 0;
	gf_dev->probe_finish = 0;

	/*setup gf configurations.*/
	gf_debug(INFO_LOG, "%s, Setting gf device configuration==========\n", __func__);

	/*SPI parameters.*/
	/* CPOL=CPHA=0, speed 1MHz */
	gf_dev->spi->mode = SPI_MODE_0;
	gf_dev->spi->max_speed_hz = 1*1000*1000;

	/* TODO: confirm gpio pins!!! */
	gf_dev->spi->bits_per_word = 8;

	/* not work for platform */
	spi_setup(gf_dev->spi);
	spi_set_drvdata(spi, gf_dev);

	/* If we can allocate a minor number, hook up this device.
	* Reusing minors is fine so long as udev or mdev is working.
	*/
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		status = sysfs_create_group(&spi->dev.kobj, &gf_debug_attr_group);
		if (status) {
			gf_debug(ERR_LOG, "%s, Failed to create sysfs file.\n", __func__);
			status = -ENODEV;
			goto err;
		}
		gf_debug(INFO_LOG, "%s, Success create sysfs file.\n", __func__);

		gf_dev->devt = MKDEV(SPIDEV_MAJOR, minor);
		gf_dev->dev = device_create(gf_spi_class, &spi->dev, gf_dev->devt, gf_dev, DEV_NAME);
		status = IS_ERR(gf_dev->dev) ? PTR_ERR(gf_dev->dev) : 0;
	} else {
		gf_debug(ERR_LOG, "%s, no minor number available!\n", __func__);
		goto err;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&gf_dev->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);
	if (status == 0) {
		gf_dev->buffer = kzalloc(bufsiz, GFP_KERNEL);
		if (gf_dev->buffer == NULL) {
			kfree(gf_dev);
			status = -ENOMEM;
			goto err;
		}

		/*register device within input system.*/
		gf_dev->input = input_allocate_device();
		if (gf_dev->input == NULL) {
			gf_debug(ERR_LOG, "%s, Failed to allocate input device.\n", __func__);
			status = -ENOMEM;
			kfree(gf_dev->buffer);
			kfree(gf_dev);
			goto err;
		}

		__set_bit(EV_KEY, gf_dev->input->evbit);
		__set_bit(GF_INPUT_HOME_KEY, gf_dev->input->keybit);

		__set_bit(GF_INPUT_MENU_KEY, gf_dev->input->keybit);
		__set_bit(GF_INPUT_BACK_KEY, gf_dev->input->keybit);
		__set_bit(GF_INPUT_FF_KEY, gf_dev->input->keybit);

		gf_dev->input->name = "gf-keys";
		if (input_register_device(gf_dev->input)) {
			gf_debug(ERR_LOG, "%s, Failed to register input device.\n", __func__);
			goto err;
		}
	} else {
		gf_debug(ERR_LOG, "%s, device create failed", __func__);
		goto err;
	}

	/* get gpio info from dts or defination */
	gf_get_gpio_info();

	/*enable the power*/
	gf_hw_power_enable(1);
	gf_bypass_flash_gpio_cfg();

	/* gpio function setting */
	gf_spi_gpio_cfg();
	gf_reset_gpio_cfg();

	/* put miso high to select SPI transfer */
	gf_miso_gpio_cfg(1);
	gf_hw_reset(60);
	gf_miso_gpio_cfg(0);

	gf_irq_gpio_cfg();
	gf_spi_clk_enable(1);

#ifdef GF_NETLINK
	/* netlink interface init */
	gf_netlink_init();
#endif

	gf_dev->probe_finish = 1;
	gf_dev->is_sleep_mode = 0;
	gf_debug(INFO_LOG, "%s probe finished, normal driver version: %s\n", __func__, GF_SPI_VERSION);

	FUNC_EXIT();
	return 0;

err:
	device_destroy(gf_spi_class, gf_dev->devt);

	kfree(gf_dev);
	FUNC_EXIT();
	return status;
}

static int gf_remove(struct spi_device *spi)
{
	struct gf_dev *gf_dev = spi_get_drvdata(spi);

	FUNC_ENTRY();
	/* make sure ops on existing fds can abort cleanly */
	if (gf_dev->spi->irq)
		free_irq(gf_dev->spi->irq, gf_dev);


#ifdef CONFIG_HAS_EARLYSUSPEND
	if (gf_dev->early_suspend.suspend)
		unregister_early_suspend(&gf_dev->early_suspend);
#endif

	gf_netlink_destroy();

	spin_lock_irq(&gf_dev->spi_lock);
	gf_dev->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&gf_dev->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	sysfs_remove_group(&spi->dev.kobj, &gf_debug_attr_group);
	list_del(&gf_dev->device_entry);
	device_destroy(gf_spi_class, gf_dev->devt);
	clear_bit(MINOR(gf_dev->devt), minors);
	if (gf_dev->users == 0) {
		if (gf_dev->input != NULL)
			input_unregister_device(gf_dev->input);

		if (gf_dev->buffer != NULL)
			kfree(gf_dev->buffer);

		kfree(gf_dev);
	}
	mutex_unlock(&device_list_lock);

	gf_debug(INFO_LOG, "%s remove finished\n", __func__);

	FUNC_EXIT();
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id gf_of_match[] = {
	{ .compatible = "mediatek,fingerprint", },
	{ .compatible = "mediatek,goodix-fp", },
	{ .compatible = "goodix,goodix-fp", },
	{},
};
#endif

static struct spi_driver gf_spi_driver = {
	.driver = {
		.name = SPI_DEV_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = gf_of_match,
#endif
	},
	.probe = gf_probe,
	.remove = gf_remove,
#ifdef CONFIG_PM
	.suspend = gf_suspend,
	.resume = gf_resume,
#endif
};

struct mt_chip_conf spi_ctrdata = {
	.setuptime = 10,
	.holdtime = 10,
	.high_time = 50, /* 1MHz */
	.low_time = 50,
	.cs_idletime = 10,
	.ulthgh_thrsh = 0,

	.cpol = SPI_CPOL_0,
	.cpha = SPI_CPHA_0,

	.rx_mlsb = SPI_MSB,
	.tx_mlsb = SPI_MSB,

	.tx_endian = SPI_LENDIAN,
	.rx_endian = SPI_LENDIAN,

	.com_mod = FIFO_TRANSFER,
	/* .com_mod = DMA_TRANSFER, */

	.pause = 0,
	.finish_intr = 1,
	.deassert = 0,
	.ulthigh = 0,
	.tckdly = 0,
};

static struct spi_board_info spi_fp_board_info[] __initdata = {
	[0] = {
		.modalias		= "gf_spi",
		.platform_data		= NULL,
		.chip_select = 0,
		.bus_num		= 1,  /* use spi1 for fingerprint sensor */
		.controller_data	= &spi_ctrdata,
	},
};

/*-------------------------------------------------------------------------*/
static int __init gf_init(void)
{
	int status;

	FUNC_ENTRY();
	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	*/
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, CHRD_DRIVER_NAME, &gf_fops);
	if (status < 0) {
		gf_debug(ERR_LOG, "%s, Failed to register char device!\n", __func__);
		FUNC_EXIT();
		return status;
	}
	gf_spi_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(gf_spi_class)) {
		unregister_chrdev(SPIDEV_MAJOR, gf_spi_driver.driver.name);
		gf_debug(ERR_LOG, "%s, Failed to create class.\n", __func__);
		FUNC_EXIT();
		return PTR_ERR(gf_spi_class);
	}

	spi_register_board_info(spi_fp_board_info, ARRAY_SIZE(spi_fp_board_info));

	status = spi_register_driver(&gf_spi_driver);
	if (status < 0) {
		class_destroy(gf_spi_class);
		unregister_chrdev(SPIDEV_MAJOR, gf_spi_driver.driver.name);
		gf_debug(ERR_LOG, "%s, Failed to register SPI driver.\n", __func__);
	}
	FUNC_EXIT();
	return status;
}
module_init(gf_init);

static void __exit gf_exit(void)
{
	FUNC_ENTRY();
	spi_unregister_driver(&gf_spi_driver);
	class_destroy(gf_spi_class);
	unregister_chrdev(SPIDEV_MAJOR, gf_spi_driver.driver.name);
	FUNC_EXIT();
}
module_exit(gf_exit);


MODULE_AUTHOR("Ke Yang<yangke@goodix.com>");
MODULE_DESCRIPTION("Goodix Fingerprint chip GF516/GF318M TEE driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:gf_spi");
