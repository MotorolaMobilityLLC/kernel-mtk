/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#ifdef CONFIG_USB_MTK_OTG
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/io.h>
#ifndef CONFIG_OF
#include <mach/irqs.h>
#endif
#include "musb_core.h"
#include <linux/platform_device.h>
#include "musbhsdma.h"
#include <linux/switch.h>
#include "usb20.h"
#ifdef CONFIG_OF
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include <mt-plat/mtk_boot_common.h>

#ifdef CONFIG_OF
struct device_node		*usb_node;
static unsigned int iddig_pin;
static unsigned int iddig_pin_mode;
static unsigned int iddig_if_config = 1;
static unsigned int drvvbus_pin;
static unsigned int drvvbus_pin_mode;
static unsigned int drvvbus_if_config = 1;
#endif

struct pinctrl *pinctrl;
struct pinctrl_state *pinctrl_iddig;
struct pinctrl_state *pinctrl_drvvbus;
struct pinctrl_state *pinctrl_drvvbus_low;
struct pinctrl_state *pinctrl_drvvbus_high;

static int usb_iddig_number;
static ktime_t ktime_start, ktime_end;

static struct musb_fifo_cfg fifo_cfg_host[] = {
{ .hw_ep_num =  1, .style = MUSB_FIFO_TX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =  1, .style = MUSB_FIFO_RX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =  2, .style = MUSB_FIFO_TX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =  2, .style = MUSB_FIFO_RX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =  3, .style = MUSB_FIFO_TX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =  3, .style = MUSB_FIFO_RX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =  4, .style = MUSB_FIFO_TX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =  4, .style = MUSB_FIFO_RX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =  5, .style = MUSB_FIFO_TX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =	5, .style = MUSB_FIFO_RX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =  6, .style = MUSB_FIFO_TX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =	6, .style = MUSB_FIFO_RX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =	7, .style = MUSB_FIFO_TX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =	7, .style = MUSB_FIFO_RX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =	8, .style = MUSB_FIFO_TX,   .maxpacket = 512, .mode = MUSB_BUF_SINGLE},
{ .hw_ep_num =	8, .style = MUSB_FIFO_RX,   .maxpacket = 64,  .mode = MUSB_BUF_SINGLE},
};

u32 delay_time = 15;
module_param(delay_time, int, 0644);
u32 delay_time1 = 55;
module_param(delay_time1, int, 0644);
u32 iddig_cnt;
module_param(iddig_cnt, int, 0644);

void mt_usb_set_vbus(struct musb *musb, int is_on)
{
	DBG(0, "mt65xx_usb20_vbus++,is_on=%d\r\n", is_on);
#ifndef FPGA_PLATFORM
	if (is_on) {
		/* power on VBUS, implement later... */
	#ifdef CONFIG_MTK_FAN5405_SUPPORT
		fan5405_set_opa_mode(1);
		fan5405_set_otg_pl(1);
		fan5405_set_otg_en(1);
	#elif defined(CONFIG_MTK_BQ24261_SUPPORT)
		bq24261_set_en_boost(1);
	#elif defined(CONFIG_MTK_BQ24296_SUPPORT)
		bq24296_set_otg_config(0x1); /* OTG */
		bq24296_set_boostv(0x7); /* boost voltage 4.998V */
		bq24296_set_boost_lim(0x1); /* 1.5A on VBUS */
		bq24296_set_en_hiz(0x0);
	#elif defined(CONFIG_MTK_BQ24196_SUPPORT)
		bq24196_set_otg_config(0x01);	/* OTG */
		bq24196_set_boost_lim(0x01);	/* 1.3A on VBUS */
	#elif defined(CONFIG_MTK_NCP1854_SUPPORT)
		ncp1854_set_otg_en(0);
		ncp1854_set_chg_en(0);
		ncp1854_set_otg_en(1);
	#else
		#ifdef CONFIG_OF
		pr_debug("****%s:%d Drive VBUS HIGH KS!!!!!\n", __func__, __LINE__);
		pinctrl_select_state(pinctrl, pinctrl_drvvbus_high);
		#else
		mt_set_gpio_mode(GPIO_OTG_DRVVBUS_PIN, GPIO_OTG_DRVVBUS_PIN_M_GPIO);
		mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN, GPIO_OUT_ONE);
		#endif
		#endif
	} else {
		/* power off VBUS, implement later... */
	#ifdef CONFIG_MTK_FAN5405_SUPPORT
		fan5405_reg_config_interface(0x01, 0x30);
		fan5405_reg_config_interface(0x02, 0x8e);
	#elif defined(CONFIG_MTK_BQ24261_SUPPORT)
		bq24261_set_en_boost(0);
	#elif defined(CONFIG_MTK_BQ24296_SUPPORT)
		bq24296_set_otg_config(0);
	#elif defined(CONFIG_MTK_BQ24196_SUPPORT)
		bq24196_set_otg_config(0x0);	/* OTG disabled */
	#elif defined(CONFIG_MTK_NCP1854_SUPPORT)
		ncp1854_set_otg_en(0x0);
	#else
		#ifdef CONFIG_OF
		pr_debug("****%s:%d Drive VBUS LOW KS!!!!!\n", __func__, __LINE__);
		pinctrl_select_state(pinctrl, pinctrl_drvvbus_low);
		#else
		mt_set_gpio_mode(GPIO_OTG_DRVVBUS_PIN, GPIO_OTG_DRVVBUS_PIN_M_GPIO);
		mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN, GPIO_OUT_ZERO);
		#endif
	#endif
	}
#endif
}

int mt_usb_get_vbus_status(struct musb *musb)
{
#if 1
	return true;
#else
	int	ret = 0;

	if ((musb_readb(musb->mregs, MUSB_DEVCTL) & MUSB_DEVCTL_VBUS) != MUSB_DEVCTL_VBUS)
		ret = 1;
	else
		DBG(0, "VBUS error, devctl=%x, power=%d\n", musb_readb(musb->mregs, MUSB_DEVCTL), musb->power);
	pr_debug("vbus ready = %d\n", ret);
	return ret;
#endif
}

void mt_usb_init_drvvbus(void)
{
#if !(defined(SWITCH_CHARGER) || defined(FPGA_PLATFORM))
	#ifdef CONFIG_OF
	int ret = 0;

	pr_debug("****%s:%d before Init Drive VBUS KS!!!!!\n", __func__, __LINE__);

	pinctrl_drvvbus = pinctrl_lookup_state(pinctrl, "drvvbus_init");
	if (IS_ERR(pinctrl_drvvbus)) {
		ret = PTR_ERR(pinctrl_drvvbus);
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl drvvbus\n");
	}

	pinctrl_drvvbus_low = pinctrl_lookup_state(pinctrl, "drvvbus_low");
	if (IS_ERR(pinctrl_drvvbus_low)) {
		ret = PTR_ERR(pinctrl_drvvbus_low);
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl drvvbus_low\n");
	}

	pinctrl_drvvbus_high = pinctrl_lookup_state(pinctrl, "drvvbus_high");
	if (IS_ERR(pinctrl_drvvbus_high)) {
		ret = PTR_ERR(pinctrl_drvvbus_high);
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl drvvbus_high\n");
	}

	pinctrl_select_state(pinctrl, pinctrl_drvvbus);
	pr_debug("****%s:%d end Init Drive VBUS KS!!!!!\n", __func__, __LINE__);
	#else
	mt_set_gpio_mode(GPIO_OTG_DRVVBUS_PIN, GPIO_OTG_DRVVBUS_PIN_M_GPIO); /* should set GPIO2 as gpio mode. */
	mt_set_gpio_dir(GPIO_OTG_DRVVBUS_PIN, GPIO_DIR_OUT);
	mt_get_gpio_pull_enable(GPIO_OTG_DRVVBUS_PIN);
	mt_set_gpio_pull_select(GPIO_OTG_DRVVBUS_PIN, GPIO_PULL_UP);
#endif
#endif
}

#if defined(CONFIG_USBIF_COMPLIANCE)
u32 sw_deboun_time = 1;
#else
u32 sw_deboun_time = 400;
#endif
module_param(sw_deboun_time, int, 0644);
struct switch_dev otg_state;

u32 typec_control;
module_param(typec_control, int, 0644);
static bool typec_req_host;

void musb_typec_host_connect(int delay)
{
	typec_req_host = true;
	queue_delayed_work(mtk_musb->st_wq, &mtk_musb->host_work, msecs_to_jiffies(delay));
}
void musb_typec_host_disconnect(int delay)
{
	typec_req_host = false;
	queue_delayed_work(mtk_musb->st_wq, &mtk_musb->host_work, msecs_to_jiffies(delay));
}

static bool musb_is_host(void)
{
	u8 devctl = 0;
	int iddig_state = 1;
	bool usb_is_host = 0;

	DBG(0, "will mask PMIC charger detection\n");
#ifndef FPGA_PLATFORM
	pmic_chrdet_int_en(0);
#endif

	musb_platform_enable(mtk_musb);

	if (typec_control) {
		usb_is_host = typec_req_host;
		goto decision_done;
	}

#ifdef ID_PIN_USE_EX_EINT
#ifndef FPGA_PLATFORM
	#ifdef CONFIG_OF
	iddig_state = __gpio_get_value(iddig_pin);
	#else
	iddig_state = mt_get_gpio_in(GPIO_OTG_IDDIG_EINT_PIN);
	#endif
	DBG(0, "iddig_state = %d\n", iddig_state);
#endif
#else
	iddig_state = 0;
	devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
	DBG(0, "devctl = %x before end session\n", devctl);
	devctl &= ~MUSB_DEVCTL_SESSION;	/* this will cause A-device change back to B-device after A-cable plug out */
	musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, devctl);
	msleep(delay_time);

	devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
	DBG(0, "devctl = %x before set session\n", devctl);

	devctl |= MUSB_DEVCTL_SESSION;
	musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, devctl);
	msleep(delay_time1);
	devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
	DBG(0, "devclt = %x\n", devctl);
#endif

	if (devctl & MUSB_DEVCTL_BDEVICE || iddig_state) {
		DBG(0, "will unmask PMIC charger detection\n");
		usb_is_host = false;
	} else {
		usb_is_host = true;
	}

decision_done:
#ifndef FPGA_PLATFORM
	if (usb_is_host == false)
		pmic_chrdet_int_en(1);
#endif

	DBG(0, "usb_is_host = %d\n", usb_is_host);
	return usb_is_host;
}

void musb_session_restart(struct musb *musb)
{
	void __iomem	*mbase = musb->mregs;

	musb_writeb(mbase, MUSB_DEVCTL, (musb_readb(mbase, MUSB_DEVCTL) & (~MUSB_DEVCTL_SESSION)));
	DBG(0, "[MUSB] stopped session for VBUSERROR interrupt\n");
	USBPHY_SET32(0x6c, (0x3c<<8));
	USBPHY_SET32(0x6c, (0x10<<0));
	USBPHY_CLR32(0x6c, (0x2c<<0));
	DBG(0, "[MUSB] force PHY to idle, 0x6c=%x\n", USBPHY_READ32(0x6c));
	mdelay(5);
	USBPHY_CLR32(0x6c, (0x3c<<8));
	USBPHY_CLR32(0x6c, (0x3c<<0));
	DBG(0, "[MUSB] let PHY resample VBUS, 0x6c=%x\n", USBPHY_READ32(0x6c));
	musb_writeb(mbase, MUSB_DEVCTL, (musb_readb(mbase, MUSB_DEVCTL) | MUSB_DEVCTL_SESSION));
	DBG(0, "[MUSB] restart session\n");
}

static struct workqueue_struct *host_plug_test_wq;
static struct delayed_work host_plug_test_work;
int host_plug_test_enable; /* default disable */
module_param(host_plug_test_enable, int, 0644);
int host_plug_in_test_period_ms = 7000;
module_param(host_plug_in_test_period_ms, int, 0644);
int host_plug_out_test_period_ms = 13000;	/* give AEE 13 seconds */
module_param(host_plug_out_test_period_ms, int, 0644);
int host_test_vbus_off_time_us = 3000;
module_param(host_test_vbus_off_time_us, int, 0644);
static int host_plug_test_triggered;
void switch_int_to_device(struct musb *musb)
{
	if (typec_control || host_plug_test_triggered) {
		DBG(1, "directly return\n");
		return;
	}
#ifdef ID_PIN_USE_EX_EINT
	irq_set_irq_type(usb_iddig_number, IRQF_TRIGGER_HIGH);
	enable_irq(usb_iddig_number);
#else
	 musb_writel(musb->mregs, USB_L1INTP, 0);
	 musb_writel(musb->mregs, USB_L1INTM, IDDIG_INT_STATUS|musb_readl(musb->mregs, USB_L1INTM));
#endif
	 DBG(0, "switch_int_to_device is done\n");
}

void switch_int_to_host(struct musb *musb)
{
	if (typec_control || host_plug_test_triggered) {
		DBG(1, "directly return\n");
		return;
	}
#ifdef ID_PIN_USE_EX_EINT
	irq_set_irq_type(usb_iddig_number, IRQF_TRIGGER_LOW);
	enable_irq(usb_iddig_number);
#else
	musb_writel(musb->mregs, USB_L1INTP, IDDIG_INT_STATUS);
	musb_writel(musb->mregs, USB_L1INTM, IDDIG_INT_STATUS|musb_readl(musb->mregs, USB_L1INTM));
#endif
	DBG(0, "switch_int_to_host is done\n");

}

void switch_int_to_host_and_mask(struct musb *musb)
{
	if (typec_control || host_plug_test_triggered) {
		DBG(1, "directly return\n");
		return;
	}
#ifdef ID_PIN_USE_EX_EINT
	disable_irq(usb_iddig_number);
	irq_set_irq_type(usb_iddig_number, IRQF_TRIGGER_LOW);
#else
	musb_writel(musb->mregs, USB_L1INTM, (~IDDIG_INT_STATUS)&musb_readl(musb->mregs, USB_L1INTM));
	mb();
	musb_writel(musb->mregs, USB_L1INTP, IDDIG_INT_STATUS);
#endif
	DBG(0, "swtich_int_to_host_and_mask is done\n");
}
static void do_host_plug_test_work(struct work_struct *data)
{
	static ktime_t ktime_begin, ktime_end;
	static s64 diff_time;

	disable_irq(usb_iddig_number);
	host_plug_test_triggered = 1;
	mb();
	DBG(0, "BEGIN");
	ktime_begin = ktime_get();

	while (1) {
		if (!musb_is_host())
			break;

		msleep(50);
		DBG(1, "mtk_musb->is_host:%d\n", mtk_musb->is_host);

		ktime_end = ktime_get();
		diff_time = ktime_to_ms(ktime_sub(ktime_end, ktime_begin));
		if (mtk_musb->is_host && diff_time >= host_plug_in_test_period_ms) {
			DBG(0, "OFF\n");
			ktime_begin = ktime_get();

			/* simulate plug out */
			musb_platform_set_vbus(mtk_musb, 0);
			udelay(host_test_vbus_off_time_us);

			queue_delayed_work(mtk_musb->st_wq, &mtk_musb->host_work, 0);
		} else if (!mtk_musb->is_host && diff_time >= host_plug_out_test_period_ms) {
			DBG(0, "ON\n");
			ktime_begin = ktime_get();
			queue_delayed_work(mtk_musb->st_wq, &mtk_musb->host_work, 0);
		}
	}

	mb();

	/* make it to ON */
	if (!mtk_musb->is_host) {
		DBG(0, "rollback to ON\n");
		queue_delayed_work(mtk_musb->st_wq, &mtk_musb->host_work, 0);
		DBG(0, "wait manual begin\n");
		msleep(1000);	/* wait manual-trigger ip_pin_work done */
		DBG(0, "wait manual end\n");
	}
	host_plug_test_triggered = 0;
	mb();

	DBG(0, "wait auto begin\n");
	enable_irq(usb_iddig_number);
	msleep(1000);	/* wait auto-trigger ip_pin_work done */
	DBG(0, "wait auto end\n");

	DBG(0, "END\n");
}

#define ID_PIN_WORK_RECHECK_TIME 30	/* 30 ms */
#define ID_PIN_WORK_BLOCK_TIMEOUT 30000 /* 30000 ms */
static void musb_host_work(struct work_struct *data)
{
	u8 devctl = 0;
	unsigned long flags;
	static int inited, timeout; /* default to 0 */
	static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 3);
	static s64 diff_time;

	/* kernel_init_done should be set in early-init stage through init.$platform.usb.rc */
	if (!inited && !kernel_init_done && !mtk_musb->is_ready && !timeout) {
		ktime_end = ktime_get();
		diff_time = ktime_to_ms(ktime_sub(ktime_end, ktime_start));
		if (__ratelimit(&ratelimit)) {
			DBG(0, "init_done:%d, is_ready:%d, inited:%d, TO:%d, diff:%lld\n",
					kernel_init_done, mtk_musb->is_ready, inited, timeout,
					diff_time);
		}

		if (diff_time > ID_PIN_WORK_BLOCK_TIMEOUT) {
			DBG(0, "diff_time:%lld\n", diff_time);
			timeout = 1;
		}

		queue_delayed_work(mtk_musb->st_wq, &mtk_musb->host_work, msecs_to_jiffies(ID_PIN_WORK_RECHECK_TIME));
		return;
	} else if (!inited) {
		DBG(0, "PASS, init_done:%d, is_ready:%d, inited:%d, TO:%d\n",
				kernel_init_done,  mtk_musb->is_ready, inited, timeout);
	}

	inited = 1;

	spin_lock_irqsave(&mtk_musb->lock, flags);
	musb_generic_disable(mtk_musb);
	spin_unlock_irqrestore(&mtk_musb->lock, flags);

	down(&mtk_musb->musb_lock);
	DBG(0, "work start, is_host=%d\n", mtk_musb->is_host);

	if (mtk_musb->in_ipo_off) {
		DBG(0, "do nothing due to in_ipo_off\n");
		goto out;
	}

	if (host_plug_test_triggered) {
		/* flip */
		if (mtk_musb->is_host)
			mtk_musb->is_host = false;
		else
			mtk_musb->is_host = true;
	} else
		mtk_musb->is_host = musb_is_host();
	DBG(0, "musb is as %s\n", mtk_musb->is_host?"host":"device");
	switch_set_state((struct switch_dev *)&otg_state, mtk_musb->is_host);

	if (mtk_musb->is_host) {
		/* setup fifo for host mode */
		ep_config_from_table_for_host(mtk_musb);
		wake_lock(&mtk_musb->usb_lock);
		musb_platform_set_vbus(mtk_musb, 1);

	/* for no VBUS sensing IP*/
	#if 1
		/* wait VBUS ready */
		msleep(100);
		/* clear session*/
		devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, (devctl&(~MUSB_DEVCTL_SESSION)));
		/* USB MAC OFF*/
		/* VBUSVALID=0, AVALID=0, BVALID=0, SESSEND=1, IDDIG=X, IDPULLUP=1 */
		USBPHY_SET32(0x6c, (0x11<<0));
		USBPHY_CLR32(0x6c, (0x2e<<0));
		USBPHY_SET32(0x6c, (0x3f<<8));
		DBG(0, "force PHY to idle, 0x6c=%x\n", USBPHY_READ32(0x6c));
		/* wait */
		mdelay(5);
		/* restart session */
		devctl = musb_readb(mtk_musb->mregs, MUSB_DEVCTL);
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, (devctl | MUSB_DEVCTL_SESSION));
		/* USB MAC ONand Host Mode*/
		/* VBUSVALID=1, AVALID=1, BVALID=1, SESSEND=0, IDDIG=0, IDPULLUP=1 */
		USBPHY_CLR32(0x6c, (0x10<<0));
		USBPHY_SET32(0x6c, (0x2d<<0));
		USBPHY_SET32(0x6c, (0x3f<<8));
		DBG(0, "force PHY to host mode, 0x6c=%x\n", USBPHY_READ32(0x6c));
	#endif

		musb_start(mtk_musb);
		MUSB_HST_MODE(mtk_musb);
		switch_int_to_device(mtk_musb);

		if (host_plug_test_enable && !host_plug_test_triggered)
			queue_delayed_work(host_plug_test_wq, &host_plug_test_work, 0);
	} else {
		/* for device no disconnect interrupt */
		spin_lock_irqsave(&mtk_musb->lock, flags);
		if (mtk_musb->is_active) {
			DBG(0, "for not receiving disconnect interrupt\n");
			usb_hcd_resume_root_hub(musb_to_hcd(mtk_musb));
			musb_root_disconnect(mtk_musb);
		}
		spin_unlock_irqrestore(&mtk_musb->lock, flags);

		DBG(0, "devctl is %x\n", musb_readb(mtk_musb->mregs, MUSB_DEVCTL));
		musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, 0);
		if (wake_lock_active(&mtk_musb->usb_lock))
			wake_unlock(&mtk_musb->usb_lock);
		musb_platform_set_vbus(mtk_musb, 0);

	/* for no VBUS sensing IP */
	#if 1
	/* USB MAC OFF*/
		/* VBUSVALID=0, AVALID=0, BVALID=0, SESSEND=1, IDDIG=X, IDPULLUP=1 */
		USBPHY_SET32(0x6c, (0x11<<0));
		USBPHY_CLR32(0x6c, (0x2e<<0));
		USBPHY_SET32(0x6c, (0x3f<<8));
		DBG(0, "force PHY to idle, 0x6c=%x\n", USBPHY_READ32(0x6c));
	#endif

		musb_stop(mtk_musb);
		mtk_musb->xceiv->otg->state = OTG_STATE_B_IDLE;
		MUSB_DEV_MODE(mtk_musb);
		switch_int_to_host(mtk_musb);
	}
out:
	DBG(0, "work end, is_host=%d\n", mtk_musb->is_host);
	up(&mtk_musb->musb_lock);

}
static irqreturn_t mt_usb_ext_iddig_int(int irq, void *dev_id)
{
	iddig_cnt++;

	queue_delayed_work(mtk_musb->st_wq, &mtk_musb->host_work, msecs_to_jiffies(sw_deboun_time));
	DBG(0, "id pin interrupt assert\n");
	disable_irq_nosync(usb_iddig_number);
	return IRQ_HANDLED;
}

void mt_usb_iddig_int(struct musb *musb)
{
	u32 usb_l1_ploy = musb_readl(musb->mregs, USB_L1INTP);

	DBG(0, "id pin interrupt assert,polarity=0x%x\n", usb_l1_ploy);
	if (usb_l1_ploy & IDDIG_INT_STATUS)
		usb_l1_ploy &= (~IDDIG_INT_STATUS);
	else
		usb_l1_ploy |= IDDIG_INT_STATUS;

	musb_writel(musb->mregs, USB_L1INTP, usb_l1_ploy);
	musb_writel(musb->mregs, USB_L1INTM, (~IDDIG_INT_STATUS)&musb_readl(musb->mregs, USB_L1INTM));

	queue_delayed_work(mtk_musb->st_wq, &mtk_musb->host_work, msecs_to_jiffies(sw_deboun_time));
	DBG(0, "id pin interrupt assert\n");
}

static void iddig_int_init(void)
{
#ifdef ID_PIN_USE_EX_EINT
	int	ret = 0;
#ifndef CONFIG_MTK_FPGA
	#ifdef CONFIG_OF
	pr_debug("****%s:%d before Init IDDIG KS!!!!!\n", __func__, __LINE__);

	pinctrl_iddig = pinctrl_lookup_state(pinctrl, "iddig_irq_init");
	if (IS_ERR(pinctrl_iddig)) {
		ret = PTR_ERR(pinctrl_iddig);
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl iddig_irq_init\n");
	}

	pinctrl_select_state(pinctrl, pinctrl_iddig);

	pr_debug("****%s:%d end Init IDDIG KS!!!!!\n", __func__, __LINE__);
	#else
	mt_set_gpio_mode(GPIO_OTG_IDDIG_EINT_PIN, GPIO_OTG_IDDIG_EINT_PIN_M_IDDIG);
	mt_set_gpio_dir(GPIO_OTG_IDDIG_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_OTG_IDDIG_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_OTG_IDDIG_EINT_PIN, GPIO_PULL_UP);
	#endif
#endif
	/*gpio_request(iddig_pin, "USB_IDDIG");*/
	gpio_set_debounce(iddig_pin, 64000);
	usb_iddig_number = mt_gpio_to_irq(iddig_pin);
	ret = request_irq(usb_iddig_number, mt_usb_ext_iddig_int, IRQF_TRIGGER_LOW, "USB_IDDIG", NULL);
	if (ret > 0)
		pr_err("USB IDDIG IRQ LINE not available!!\n");
	else
		pr_debug("USB IDDIG IRQ LINE available!!\n");
#else
	u32 phy_id_pull = 0;

	phy_id_pull = __raw_readl(U2PHYDTM1);
	phy_id_pull |= ID_PULL_UP;
	__raw_writel(phy_id_pull, U2PHYDTM1);

	musb_writel(mtk_musb->mregs, USB_L1INTM, IDDIG_INT_STATUS|musb_readl(mtk_musb->mregs, USB_L1INTM));
#endif
}

void mt_usb_otg_init(struct musb *musb)
{
	/* BYPASS OTG function in special mode */
	if (get_boot_mode() == META_BOOT
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
			|| get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT
			|| get_boot_mode() == LOW_POWER_OFF_CHARGING_BOOT
#endif
	   ) {
		DBG(0, "in special mode %d\n", get_boot_mode());
		return;
	}

	/* test */
	host_plug_test_wq = create_singlethread_workqueue("host_plug_test_wq");
	INIT_DELAYED_WORK(&host_plug_test_work, do_host_plug_test_work);

#ifdef CONFIG_OF
	usb_node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-usb20");
	if (usb_node == NULL) {
		pr_err("USB OTG - get USB0 node failed\n");
	} else {
		if (of_property_read_u32_index(usb_node, "iddig_gpio", 0, &iddig_pin)) {
			iddig_if_config = 0;
			pr_err("get dtsi iddig_pin fail\n");
		}
		if (of_property_read_u32_index(usb_node, "iddig_gpio", 1, &iddig_pin_mode))
			pr_err("get dtsi iddig_pin_mode fail\n");
		if (of_property_read_u32_index(usb_node, "drvvbus_gpio", 0, &drvvbus_pin)) {
			drvvbus_if_config = 0;
			pr_err("get dtsi drvvbus_pin fail\n");
		}
		if (of_property_read_u32_index(usb_node, "drvvbus_gpio", 1, &drvvbus_pin_mode))
			pr_err("get dtsi drvvbus_pin_mode fail\n");
	}

	pinctrl = devm_pinctrl_get(mtk_musb->controller);
	if (IS_ERR(pinctrl))
		dev_err(mtk_musb->controller, "Cannot find usb pinctrl!\n");
#endif
	/*init drrvbus*/
	mt_usb_init_drvvbus();

	/* init idpin interrupt */
	ktime_start = ktime_get();
	INIT_DELAYED_WORK(&musb->host_work, musb_host_work);

	/* FIXME, fill in TYPEC management */
	if (0) {
		DBG(0, "host controlled by TYPEC\n");
		typec_control = 1;
		/* related resource init & register */
	} else {
		DBG(0, "host controlled by IDDIG\n");
		iddig_int_init();
	}

	/* EP table */
	musb->fifo_cfg_host = fifo_cfg_host;
	musb->fifo_cfg_host_size = ARRAY_SIZE(fifo_cfg_host);

	otg_state.name = "otg_state";
	otg_state.index = 0;
	otg_state.state = 0;

	if (switch_dev_register(&otg_state))
		pr_err("switch_dev_register fail\n");
	else
		pr_debug("switch_dev register success\n");
}
static int option;
static int set_option(const char *val, const struct kernel_param *kp)
{
	int local_option;
	int rv;

	/* update module parameter */
	rv = param_set_int(val, kp);
	if (rv)
		return rv;

	/* update local_option */
	rv = kstrtoint(val, 10, &local_option);
	if (rv != 0)
		return rv;

	DBG(0, "option:%d, local_option:%d\n", option, local_option);

	switch (local_option) {
	case 0:
		DBG(0, "case %d\n", local_option);
		iddig_int_init();
		break;
	case 1:
		DBG(0, "case %d\n", local_option);
		musb_typec_host_connect(0);
		break;
	case 2:
		DBG(0, "case %d\n", local_option);
		musb_typec_host_disconnect(0);
		break;
	case 3:
		DBG(0, "case %d\n", local_option);
		musb_typec_host_connect(3000);
		break;
	case 4:
		DBG(0, "case %d\n", local_option);
		musb_typec_host_disconnect(3000);
		break;
	default:
		break;
	}
	return 0;
}
static struct kernel_param_ops option_param_ops = {
	.set = set_option,
	.get = param_get_int,
};
module_param_cb(option, &option_param_ops, &option, 0644);
#else
#include "musb_core.h"
/* for not define CONFIG_USB_MTK_OTG */
void mt_usb_otg_init(struct musb *musb) {}
void mt_usb_init_drvvbus(void){}
void mt_usb_set_vbus(struct musb *musb, int is_on) {}
int mt_usb_get_vbus_status(struct musb *musb) {return 1; }
void mt_usb_iddig_int(struct musb *musb) {}
void switch_int_to_device(struct musb *musb) {}
void switch_int_to_host(struct musb *musb) {}
void switch_int_to_host_and_mask(struct musb *musb) {}
void musb_session_restart(struct musb *musb) {}
#endif
