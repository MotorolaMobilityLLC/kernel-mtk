/*
 * MUSB OTG driver core code
 *
 * Copyright 2005 Mentor Graphics Corporation
 * Copyright (C) 2005-2006 by Texas Instruments
 * Copyright (C) 2006-2007 Nokia Corporation
 *
 * Copyright 2015 Mediatek Inc.
 *	Marvin Lin <marvin.lin@mediatek.com>
 *	Arvin Wang <arvin.wang@mediatek.com>
 *	Vincent Fan <vincent.fan@mediatek.com>
 *	Bryant Lu <bryant.lu@mediatek.com>
 *	Yu-Chang Wang <yu-chang.wang@mediatek.com>
 *	Macpaul Lin <macpaul.lin@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Inventra (Multipoint) Dual-Role Controller Driver for Linux.
 *
 * This consists of a Host Controller Driver (HCD) and a peripheral
 * controller driver implementing the "Gadget" API; OTG support is
 * in the works.  These are normal Linux-USB controller drivers which
 * use IRQs and have no dedicated thread.
 *
 * This version of the driver has only been used with products from
 * Texas Instruments.  Those products integrate the Inventra logic
 * with other DMA, IRQ, and bus modules, as well as other logic that
 * needs to be reflected in this driver.
 *
 *
 * NOTE:  the original Mentor code here was pretty much a collection
 * of mechanisms that don't seem to have been fully integrated/working
 * for *any* Linux kernel version.  This version aims at Linux 2.6.now,
 * Key open issues include:
 *
 *  - Lack of host-side transaction scheduling, for all transfer types.
 *    The hardware doesn't do it; instead, software must.
 *
 *    This is not an issue for OTG devices that don't support external
 *    hubs, but for more "normal" USB hosts it's a user issue that the
 *    "multipoint" support doesn't scale in the expected ways.  That
 *    includes DaVinci EVM in a common non-OTG mode.
 *
 *      * Control and bulk use dedicated endpoints, and there's as
 *        yet no mechanism to either (a) reclaim the hardware when
 *        peripherals are NAKing, which gets complicated with bulk
 *        endpoints, or (b) use more than a single bulk endpoint in
 *        each direction.
 *
 *        RESULT:  one device may be perceived as blocking another one.
 *
 *      * Interrupt and isochronous will dynamically allocate endpoint
 *        hardware, but (a) there's no record keeping for bandwidth;
 *        (b) in the common case that few endpoints are available, there
 *        is no mechanism to reuse endpoints to talk to multiple devices.
 *
 *        RESULT:  At one extreme, bandwidth can be overcommitted in
 *        some hardware configurations, no faults will be reported.
 *        At the other extreme, the bandwidth capabilities which do
 *        exist tend to be severely undercommitted.  You can't yet hook
 *        up both a keyboard and a mouse to an external USB hub.
 */

/*
 * This gets many kinds of configuration information:
 *	- Kconfig for everything user-configurable
 *	- platform_device for addressing, irq, and platform_data
 *	- platform_data is mostly for board-specific informarion
 *	  (plus recentrly, SOC or family details)
 *
 * Most of the conditional compilation will (someday) vanish.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/wakelock.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>

#ifdef	CONFIG_ARM
#include <mach/hardware.h>
#include <mach/memory.h>
#include <asm/mach-types.h>
#endif

#include "musbfsh_core.h"
#include "musbfsh_host.h"
#include "musbfsh_dma.h"
#include "musbfsh_hsdma.h"
#include "musbfsh_mt65xx.h"
#include "usb.h"

#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <mach/mt_clkmgr.h>

#ifdef CONFIG_MTK_ICUSB_SUPPORT

struct my_attr skip_session_req_attr = {
	.attr.name = "skip_session_req",
	.attr.mode = 0644,
#ifdef MTK_ICUSB_SKIP_SESSION_REQ
	.value = 1
#else
	.value = 0
#endif
};

struct my_attr skip_enable_session_attr = {
	.attr.name = "skip_enable_session",
	.attr.mode = 0644,
#ifdef MTK_ICUSB_SKIP_ENABLE_SESSION
	.value = 1
#else
	.value = 0
#endif
};

struct my_attr skip_mac_init_attr = {
	.attr.name = "skip_mac_init",
	.attr.mode = 0644,
#ifdef MTK_ICUSB_SKIP_MAC_INIT
	.value = 1
#else
	.value = 0
#endif
};

struct my_attr hw_dbg_attr = {
	.attr.name = "hw_dbg",
	.attr.mode = 0644,
#ifdef MTK_ICUSB_HW_DBG
	.value = 1
#else
	.value = 0
#endif
};

#endif

static int __init musbfsh_dts_probe(struct platform_device *pdev);

static const struct of_device_id apusb11_of_ids[] = {
	{.compatible = "mediatek,icusb",},
	{},
};

static struct platform_driver musbfsh_dts_driver = {
	.driver = {
		.name = "musb11_dts",
		.of_match_table = apusb11_of_ids,
	},
	.probe = musbfsh_dts_probe,
};

#ifdef CONFIG_MTK_USBFSH

#define DRIVER_AUTHOR "Mentor Graphics, Texas Instruments, Nokia, Mediatek"
#define DRIVER_DESC "MT65xx USB1.1 Host Controller Driver"

#define MUSBFSH_VERSION "6.0"

#define DRIVER_INFO DRIVER_DESC ", v" MUSBFSH_VERSION

#define MUSBFSH_DRIVER_NAME "musbfsh_hdrc"
const char musbfsh_driver_name[] = MUSBFSH_DRIVER_NAME;

MODULE_DESCRIPTION(DRIVER_INFO);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" MUSBFSH_DRIVER_NAME);

struct wake_lock musbfsh_suspend_lock;
struct musbfsh *g_musbfsh;
static u64 usb11_dmamask = DMA_BIT_MASK(32);

#ifdef CONFIG_MTK_ICUSB_SUPPORT
/* extern void slp_infra_en(bool en); */
#ifdef CONFIG_PM
static int suspend_cnt = 0, resume_cnt;
#endif
#endif

DEFINE_SPINLOCK(musbfs_io_lock);

static struct musbfsh *mtk_musbfsh;
int is_musbfsh_rh(struct usb_device *udev)
{
	struct usb_device *rhdev;
	struct usb_hcd *hcd;

	if (!mtk_musbfsh) {
		MYDBG("mtk_musbfsh is NULL!\n");
		return 0;
	}

	hcd = musbfsh_to_hcd(mtk_musbfsh);
	rhdev = hcd->self.root_hub;
	return (udev == rhdev) ? 1 : 0;
}

#ifdef MTK_DT_USB_SUPPORT
extern void request_wakeup_md_timeout(unsigned int dev_id,
				      unsigned int dev_sub_id);
#endif
void mt65xx_usb11_suspend_resume_test(void)
{
	u8 power;
	u32 frame_number;

	MYDBG("begin\n");

	power = musbfsh_readb((unsigned char __iomem *)mtk_musbfsh->mregs,
			      MUSBFSH_POWER);
	/* power &= ~MUSBFSH_POWER_RESUME; */
	power |= MUSBFSH_POWER_ENSUSPEND;
	power |= MUSBFSH_POWER_SUSPENDM;
	musbfsh_writeb((unsigned char __iomem *)mtk_musbfsh->mregs,
		       MUSBFSH_POWER, power);

	frame_number =
		musbfsh_readw((unsigned char __iomem *)mtk_musbfsh->mregs,
			      MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	mdelay(12);
	MYDBG("suspend done\n");
	frame_number =
		musbfsh_readw((unsigned char __iomem *)mtk_musbfsh->mregs,
			      MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	mdelay(1000);
	MYDBG("start resuming\n");

#ifdef MTK_DT_USB_SUPPORT
	MYDBG("request md wake up\n");
	request_wakeup_md_timeout(0, 0);
#endif

	power = musbfsh_readb((unsigned char __iomem *)mtk_musbfsh->mregs,
			      MUSBFSH_POWER);
	power &= ~MUSBFSH_POWER_SUSPENDM;
	power |= MUSBFSH_POWER_RESUME;
	musbfsh_writeb((unsigned char __iomem *)mtk_musbfsh->mregs,
		       MUSBFSH_POWER, power);
	MYDBG("");

	mdelay(12);
	MYDBG("stop resuming\n");

	power = musbfsh_readb((unsigned char __iomem *)mtk_musbfsh->mregs,
			      MUSBFSH_POWER);
	power &= ~MUSBFSH_POWER_RESUME;
	musbfsh_writeb((unsigned char __iomem *)mtk_musbfsh->mregs,
		       MUSBFSH_POWER, power);

	frame_number =
		musbfsh_readw((unsigned char __iomem *)mtk_musbfsh->mregs,
			      MUSBFSH_FRAME);
	MYDBG("frame_number: %d\n", frame_number);

	mdelay(3);

	frame_number =
		musbfsh_readw((unsigned char __iomem *)mtk_musbfsh->mregs,
			      MUSBFSH_FRAME);
	MYDBG("frame_number: %d\n", frame_number);

	MYDBG("end\n");
}

#ifdef MTK_DT_USB_SUPPORT
static int ext_usb_wkup_irq;

/*
 * wx, seconds for hold wake lock and suspend schedule,
 * please also check cdc-acm.c to sync, they should be the same
 */
#define USB_WAKE_TIME 5
struct wake_lock usb_resume_lock;

void release_usb11_wakelock(void)
{
	MYDBG("\n");
	if (musbfsh_wake_lock_hold)
		wake_unlock(&musbfsh_suspend_lock);
}

irqreturn_t remote_wakeup_irq(unsigned irq, struct irq_desc *desc)
{
	musbfsh_eint_cnt++;
	MYDBG("cpuid:%d\n", smp_processor_id());
#if 0
	u8 power, devctl;
	unsigned long flags;

	if (!mtk_musbfsh) {
		MYDBG("\n");	/* 04404_test */
		return;
	}
	MYDBG("\n");		/* 04404_test */
	spin_lock_irqsave(&mtk_musbfsh->lock, flags);

	devctl = musbfsh_readb(mtk_musbfsh->mregs, MUSBFSH_DEVCTL);
	if (devctl & MUSBFSH_DEVCTL_HM) {
		wake_lock_timeout(&usb_resume_lock, USB_WAKE_TIME * HZ);
		musbfsh_set_power(mtk_musbfsh, 1);
		power = musbfsh_readb(mtk_musbfsh->mregs, MUSBFSH_POWER);
		WARNING("Remote wakeup! power=0x%x\n", power);
		if (!(power & MUSBFSH_POWER_SUSPENDM)) {
			MYDBG("\n");	/* 04404_test */
			goto out;
		}

		/* remote wakeup?  later, GetPortStatus
		 * will stop RESUME signaling
		 */
		power &= ~MUSBFSH_POWER_SUSPENDM;
		musbfsh_writeb(mtk_musbfsh->mregs, MUSBFSH_POWER,
			       power | MUSBFSH_POWER_RESUME);

		mtk_musbfsh->port1_status |= (USB_PORT_STAT_C_SUSPEND << 16)
		    | MUSBFSH_PORT_STAT_RESUME;
		mtk_musbfsh->rh_timer = jiffies + msecs_to_jiffies(20);

		mtk_musbfsh->is_active = 1;
		usb_hcd_resume_root_hub(musbfsh_to_hcd(mtk_musbfsh));
	}
out:
	spin_unlock_irqrestore(&mtk_musbfsh->lock, flags);
#endif

	/* disable_irq_nosync(irq);              // for non-automatic mask */
	return IRQ_HANDLED;
}

void get_eint_ext_usb_wkup_irq(void)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL,
				       "mediatek, DT_EXT_MD_WK_UP_USB-eint");
	if (node) {
		/* this step will set irq feature by dtsi */
		ext_usb_wkup_irq = irq_of_parse_and_map(node, 0);
		if (!ext_usb_wkup_irq) {
			MYDBG("can't irq_of_parse_and_map!\n");
			return;
		}

	} else {
		MYDBG("can't find node\n");
	}
}

void register_eint_usb_p1_wakeup_callback(void)
{
	int ret;

	if (!ext_usb_wkup_irq) {
		MYDBG("no ext_usb_wkup_irq\n");
		return;
	}
	/* still could chg feature here */
	ret = request_irq(ext_usb_wkup_irq, (irq_handler_t)remote_wakeup_irq,
			  IRQF_TRIGGER_FALLING,
			  "DT_EXT_MD_WK_UP_USB-eint", NULL);
	if (ret > 0) {
		MYDBG("eint irq %d not available\n", ext_usb_wkup_irq);
		return;
	}

}
#endif


/* #define ORG_SUSPEND_RESUME_TEST */
#ifdef ORG_SUSPEND_RESUME_TEST
#define USB_BASE                   0xF1200000
void mt65xx_usb20_suspend_resume_test(void)
{
	u8 power;
	u32 frame_number;
	unsigned char __iomem *mregs = g_musbfsh->mregs;

	MYDBG("begin\n");

	power = musbfsh_readb(mregs, MUSBFSH_POWER);
	/* power &= ~MUSBFSH_POWER_RESUME; */
	power |= MUSBFSH_POWER_ENSUSPEND;
	power |= MUSBFSH_POWER_SUSPENDM;
	musbfsh_writeb(mregs, MUSBFSH_POWER, power);

	frame_number = musbfsh_readw(mregs, MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	mdelay(12);
	MYDBG("suspend done\n");
	frame_number = musbfsh_readw(mregs, MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	mdelay(1000);
	MYDBG("start resuming\n");

	power = musbfsh_readb(mregs, MUSBFSH_POWER);
	power &= ~MUSBFSH_POWER_SUSPENDM;
	power |= MUSBFSH_POWER_RESUME;
	musbfsh_writeb(mregs, MUSBFSH_POWER, power);
	MYDBG("");

	mdelay(12);
	MYDBG("stop resuming\n");

	power = musbfsh_readb(mregs, MUSBFSH_POWER);
	power &= ~MUSBFSH_POWER_RESUME;
	musbfsh_writeb(mregs, MUSBFSH_POWER, power);

	frame_number = musbfsh_readw(mregs, MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	mdelay(3);

	frame_number = musbfsh_readw(mregs, MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	MYDBG("end\n");
}
#endif

#ifdef CONFIG_MTK_ICUSB_SUPPORT
static int usb11_enabled;

unsigned char g_usb_clk_reg_before, g_usb_clk_reg_after;
unsigned char g_FS_EOF1_before, g_FS_EOF1_after;

#define USB_BASE                   0xF1200000
void mt65xx_usb20_suspend_resume_test(void)
{
	u8 power;
	u32 frame_number;
	unsigned char __iomem *mregs = g_musbfsh->mregs;

	MYDBG("begin\n");

	power = musbfsh_readb(mregs, MUSBFSH_POWER);
	/* power &= ~MUSBFSH_POWER_RESUME; */
	power |= MUSBFSH_POWER_ENSUSPEND;
	power |= MUSBFSH_POWER_SUSPENDM;
	musbfsh_writeb(mregs, MUSBFSH_POWER, power);

	frame_number = musbfsh_readw(mregs, MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	mdelay(12);
	MYDBG("suspend done\n");
	frame_number = musbfsh_readw(mregs, MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	mdelay(1000);
	MYDBG("start resuming\n");

	power = musbfsh_readb(mregs, MUSBFSH_POWER);
	power &= ~MUSBFSH_POWER_SUSPENDM;
	power |= MUSBFSH_POWER_RESUME;
	musbfsh_writeb(mregs, MUSBFSH_POWER, power);
	MYDBG("");

	mdelay(12);
	MYDBG("stop resuming\n");

	power = musbfsh_readb(mregs, MUSBFSH_POWER);
	power &= ~MUSBFSH_POWER_RESUME;
	musbfsh_writeb(mregs, MUSBFSH_POWER, power);

	frame_number = musbfsh_readw(mregs, MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	mdelay(3);

	frame_number = musbfsh_readw(mregs, MUSBFSH_FRAME);
	MYDBG("frame_number : %d\n", frame_number);

	MYDBG("end\n");

}


void musbfsh_root_disc_procedure(void)
{
	MYDBG("");
	usb_hcd_resume_root_hub(musbfsh_to_hcd(g_musbfsh));
	musbfsh_root_disconnect(g_musbfsh);
}

void ic_usb_clk_chg(void)
{
	/* b01 for 69 MHZ, b10 for 41.6MHZ */
	g_usb_clk_reg_before = __raw_readb((void __iomem *)0xF000014C);
	g_usb_clk_reg_after = g_usb_clk_reg_before & (~0x1);
	g_usb_clk_reg_after = g_usb_clk_reg_after | 0x2;
	__raw_writeb(g_usb_clk_reg_after, (void __iomem *)0xF000014C);
}

void musbfsh_open_vsim_power(int on)
{
	if (on) {
		pmic_set_register_value(PMIC_RG_VSIM1_VOSEL, 0x7);
		pmic_set_register_value(PMIC_RG_VSIM1_EN, 1);
		/*
		 * hwPowerOn(MT6351_POWER_LDO_VSIM1,
		 *	     VOL_3100*1000, "USB11-SIM");
		 */
	} else {
		pmic_set_register_value(PMIC_RG_VSIM1_EN, 0);
		/* hwPowerDown(MT6351_POWER_LDO_VSIM1, "USB11-SIM"); */
	}
	MYDBG("%s = %d\n", __func__, on);
}

void musbfsh_open_vsim_power1(int on)
{
	if (on) {
		pmic_set_register_value(PMIC_RG_VSIM1_VOSEL, 0x7);
		pmic_set_register_value(PMIC_RG_VSIM1_EN, 1);
		/*
		 * hwPowerOn(MT6351_POWER_LDO_VSIM1,
		 *	     VOL_3100*1000, "USB11-SIM");
		 */

	} else {
		pmic_set_register_value(PMIC_RG_VSIM1_EN, 0);
		/*hwPowerDown(MT6351_POWER_LDO_VSIM1, "USB11-SIM"); */
	}
	MYDBG("musbfsh_open_vsim_power = %d\n", on);
}

void musbfsh_start_session_pure(void)
{
	u8 devctl;
	unsigned char __iomem *mregs;

	MYDBG("");
	mregs = g_musbfsh->mregs;
	devctl = musbfsh_readb(mregs, MUSBFSH_DEVCTL);
	devctl |= MUSBFSH_DEVCTL_SESSION;
	musbfsh_writeb(mregs, MUSBFSH_DEVCTL, devctl);

	MYDBG("[IC-USB]start session PURE\n");
}

void musbfsh_init_phy_by_voltage(enum PHY_VOLTAGE_TYPE phy_volt)
{
	MYDBG("");
	if (!usb11_enabled) {
		usb11_enabled = 1;
		mt65xx_usb11_enable_clk_pll_pw();
	}

	set_usb_phy_voltage(phy_volt);
	mt65xx_usb11_phy_poweron();

}

void musbfsh_start_session(void)
{
	u8 devctl;
	unsigned char __iomem *mregs;

	mregs = g_musbfsh->mregs;
	/* clear session */
	devctl = musbfsh_readb(mregs, MUSBFSH_DEVCTL);
	musbfsh_writeb(mregs, MUSBFSH_DEVCTL,
		       (devctl & (~MUSBFSH_DEVCTL_SESSION)));

	USB11PHY_SET8(0x6c, 0x10);
	USB11PHY_CLR8(0x6c, 0x2E);
	USB11PHY_SET8(0x6d, 0x3E);
	mdelay(5);
	devctl = musbfsh_readb(mregs, MUSBFSH_DEVCTL);
	devctl |= MUSBFSH_DEVCTL_SESSION;
	musbfsh_writeb(mregs, MUSBFSH_DEVCTL, devctl);

	USB11PHY_CLR8(0x6c, 0x10);
	USB11PHY_SET8(0x6c, 0x2c);
	USB11PHY_SET8(0x6d, 0x3e);

#ifdef MTK_ICUSB_TAKE_WAKE_LOCK
	MYDBG("[IC-USB]start session, wake_lock taken feature enalbed\n");
#else
	MYDBG("[IC-USB]start session, wake_lock taken feature disalbed\n");
#endif
}

void musbfsh_stop_session(void)
{
	u8 devctl;
	unsigned char __iomem *mregs;

	MYDBG("");
	mregs = g_musbfsh->mregs;
	devctl = musbfsh_readb(mregs, MUSBFSH_DEVCTL);
	devctl &= ~(MUSBFSH_DEVCTL_SESSION);
	musbfsh_writeb(mregs, MUSBFSH_DEVCTL, devctl);

	MYDBG("[IC_USB] stop session\n");
}

#if defined(MTK_ICUSB_BABBLE_RECOVER) || defined(MTK_SMARTBOOK_SUPPORT)
static struct timer_list babble_recover_timer;
static void babble_recover_func(unsigned long _musbfsh)
{
	int flags = 0;
	u8 devctl = 0;
	struct musbfsh *musbfsh = (struct musbfsh *)_musbfsh;

	WARNING("execute babble_recover_func!\n");

	/*
	 * mdelay here for waiting hardware state machine,
	 * jiffies controlled call-back might not be accurate.
	 */
	mdelay(20);

	spin_lock_irqsave(&musbfsh->lock, flags);

	mt65xx_usb11_mac_phy_babble_recover(musbfsh);

	mdelay(1);
	devctl |= MUSBFSH_DEVCTL_SESSION;
	musbfsh_writeb(musbfsh->mregs, MUSBFSH_DEVCTL, devctl);

	del_timer(&babble_recover_timer);

	spin_unlock_irqrestore(&musbfsh->lock, flags);
}
#endif




static int musbfsh_core_init1(struct musbfsh *musbfsh);
static void musbfsh_generic_disable(struct musbfsh *musbfsh);
void musbfsh_mac_init(void)
{
	struct timeval tv_begin, tv_end;

	do_gettimeofday(&tv_begin);

	MYDBG("");

	musbfsh_generic_disable(g_musbfsh);

	musbfsh_writeb(g_musbfsh->mregs,
		       MUSBFSH_HSDMA_DMA_INTR_UNMASK_SET, 0xff);

	musbfsh_core_init1(g_musbfsh);

	musbfsh_start(g_musbfsh);

	do_gettimeofday(&tv_end);
	MYDBG("time spent, sec : %d, usec : %d\n",
	      (unsigned int)(tv_end.tv_sec - tv_begin.tv_sec),
	      (unsigned int)(tv_end.tv_usec - tv_begin.tv_usec));

}

int is_usb11_enabled(void)
{
	return usb11_enabled;
}

void set_usb11_enabled(void)
{
	usb11_enabled = 1;
}
#endif

#if 0		/* IC_USB from SS5 */
#ifdef IC_USB
static ssize_t show_start(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "start session under IC-USB mode\n");
}

static ssize_t store_start(struct device *dev, struct device_attribute *attr,
			   const char *buf,
			   size_t size)
{
	char *pvalue = NULL;
	unsigned long value = 0;
	int ret;
	size_t count = 0;
	unsigned char __iomem *usb11_base = USB11_BASE;
	u8 devctl = musbfsh_readb(usb11_base, MUSBFSH_DEVCTL);

	ret = kstrtoul(&pvalue, 10, &value);

	if (ret)
		return ERR_PTR(-EINVAL);

	count = pvalue - buf;

	if (*pvalue && isspace(*pvalue))
		count++;

	if (count == size) {
		if (value) {
			WARNING("[IC-USB]start session\n");
			/* wx? why not wait until device connected */
			devctl |= MUSBFSH_DEVCTL_SESSION;
			musbfsh_writeb(usb11_base, MUSBFSH_DEVCTL, devctl);
			WARNING("[IC-USB]power on VSIM\n");
			hwPowerOn(MT65XX_POWER_LDO_VSIM, VOL_3000, "USB11-SIM");
		}
	}
	return size;
}

static DEVICE_ATTR(start, 0666, show_start, store_start);
#endif
#endif

/*-------------------------------------------------------------------------*/

static inline struct musbfsh *dev_to_musbfsh(struct device *dev)
{
	return hcd_to_musbfsh(dev_get_drvdata(dev));
}


/*-------------------------------------------------------------------------*/
/* #ifdef CONFIG_MUSBFSH_PIO_ONLY */
/*
 * Load an endpoint's FIFO
 */
void musbfsh_write_fifo(struct musbfsh_hw_ep *hw_ep, u16 len, const u8 *src)
{
	void __iomem *fifo = hw_ep->fifo;

	prefetch((u8 *) src);
	INFO("%cX ep%d fifo %p count %d buf %p\n", 'T',
	     hw_ep->epnum, fifo, len, src);

	/* we can't assume unaligned reads work */
	if (likely((0x01 & (unsigned long)src) == 0)) {
		u16 index = 0;

		/* best case is 32bit-aligned source address */
		if ((0x02 & (unsigned long)src) == 0) {
			if (len >= 4) {
				/* writesl(fifo, src + index, len >> 2); */
				iowrite32_rep(fifo, src + index, len >> 2);
				index += len & ~0x03;
			}
			if (len & 0x02) {
				musbfsh_writew(fifo, 0, *(u16 *) &src[index]);
				index += 2;
			}
		} else {
			if (len >= 2) {
				/* writesw(fifo, src + index, len >> 1); */
				iowrite16_rep(fifo, src + index, len >> 1);
				index += len & ~0x01;
			}
		}
		if (len & 0x01)
			musbfsh_writeb(fifo, 0, src[index]);
	} else {
		/* byte aligned */
		/* writesb(fifo, src, len); */
		iowrite8_rep(fifo, src, len);
	}

}

/*
 * Unload an endpoint's FIFO
 */
void musbfsh_read_fifo(struct musbfsh_hw_ep *hw_ep, u16 len, u8 *dst)
{
	void __iomem *fifo = hw_ep->fifo;

	INFO("%cX ep%d fifo %p count %d buf %p\n", 'R',
	     hw_ep->epnum, fifo, len, dst);

	/* we can't assume unaligned writes work */
	if (likely((0x01 & (unsigned long)dst) == 0)) {
		u16 index = 0;

		/* best case is 32bit-aligned destination address */
		if ((0x02 & (unsigned long)dst) == 0) {
			if (len >= 4) {
				/* readsl(fifo, dst, len >> 2); */
				ioread32_rep(fifo, dst, len >> 2);
				index = len & ~0x03;
			}
			if (len & 0x02) {
				*(u16 *) &dst[index] = musbfsh_readw(fifo, 0);
				index += 2;
			}
		} else {
			if (len >= 2) {
				/* readsw(fifo, dst, len >> 1); */
				ioread16_rep(fifo, dst, len >> 1);
				index = len & ~0x01;
			}
		}
		if (len & 0x01)
			dst[index] = musbfsh_readb(fifo, 0);
	} else {
		/* byte aligned */
		/* readsb(fifo, dst, len); */
		ioread8_rep(fifo, dst, len);
	}
}


/* for high speed test mode; see USB 2.0 spec 7.1.20 */
static const u8 musbfsh_test_packet[53] = {
	/* implicit SYNC then DATA0 to start */

	/* JKJKJKJK x9 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* JJKKJJKK x8 */
	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	/* JJJJKKKK x8 */
	0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
	/* JJJJJJJKKKKKKK x8 */
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	/* JJJJJJJK x8 */
	0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd,
	/* JKKKKKKK x10, JK */
	0xfc, 0x7e, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0x7e
	/* implicit CRC16 then EOP to end */
};

void musbfsh_load_testpacket(struct musbfsh *musbfsh)
{
	void __iomem *regs = musbfsh->endpoints[0].regs;

	musbfsh_ep_select(musbfsh->mregs, 0);	/* should be implemented */
	musbfsh_write_fifo(musbfsh->control_ep, sizeof(musbfsh_test_packet),
			   musbfsh_test_packet);
	musbfsh_writew(regs, MUSBFSH_CSR0, MUSBFSH_CSR0_TXPKTRDY);
}

/*
 * Interrupt Service Routine to record USB "global" interrupts.
 * Since these do not happen often and signify things of
 * paramount importance, it seems OK to check them individually;
 * the order of the tests is specified in the manual
 *
 * @param musb instance pointer
 * @param int_usb register contents
 * @param devctl
 * @param power
 */

/* #define MUSBFSH_DBG_REG */
static irqreturn_t musbfsh_stage0_irq(struct musbfsh *musbfsh, u8 int_usb,
				      u8 devctl, u8 power)
{
	void __iomem *mregs = musbfsh->mregs;

#if 0
	u8 intusbe = musbfsh_readb(musbfsh->mregs, MUSBFSH_INTRUSBE);

	MYDBG("devctl : %x, power : %x, int_usb : %x, intusbe : %x\n",
	      devctl, power, int_usb,
	      intusbe);
#endif

#ifdef MUSBFSH_DBG_REG
	int i;
	u32 addr[] = {0x60, 0x74, 0x620, 0x630};

	for (i = 0; i < sizeof(addr) / sizeof(u32); i++) {
		WARNING("dbg addr : %x, val : %x\n", addr[i],
			musbfsh_readl(musbfsh->mregs, addr[i]));
	}
#endif

	irqreturn_t handled = IRQ_NONE;

	/* in host mode, the peripheral may issue remote wakeup.
	 * in peripheral mode, the host may resume the link.
	 * spurious RESUME irqs happen too, paired with SUSPEND.
	 */
	if (int_usb & MUSBFSH_INTR_RESUME) {
		handled = IRQ_HANDLED;
		WARNING("RESUME!\n");

		if (devctl & MUSBFSH_DEVCTL_HM) {
			/*
			 * remote wakeup?  later, GetPortStatus
			 * will stop RESUME signaling
			 */

			if (power & MUSBFSH_POWER_SUSPENDM) {
				/* spurious */
				musbfsh->int_usb &= ~MUSBFSH_INTR_SUSPEND;
				WARNING("Spurious SUSPENDM\n");
			}

			power &= ~MUSBFSH_POWER_SUSPENDM;
			musbfsh_writeb(mregs, MUSBFSH_POWER, power |
				       MUSBFSH_POWER_RESUME);

			musbfsh->port1_status |= (USB_PORT_STAT_C_SUSPEND << 16)
			    | MUSBFSH_PORT_STAT_RESUME;
			musbfsh->rh_timer = jiffies + msecs_to_jiffies(20);

			musbfsh->is_active = 1;
			usb_hcd_resume_root_hub(musbfsh_to_hcd(musbfsh));
		}
	}

	/* see manual for the order of the tests */
	if (int_usb & MUSBFSH_INTR_SESSREQ) {
		/* will not run to here */
		WARNING("SESSION_REQUEST\n");

		/* IRQ arrives from ID pin sense or (later, if VBUS power
		 * is removed) SRP.  responses are time critical:
		 *  - turn on VBUS (with silicon-specific mechanism)
		 *  - go through A_WAIT_VRISE
		 *  - ... to A_WAIT_BCON.
		 * a_wait_vrise_tmout triggers VBUS_ERROR transitions
		 */

#ifdef CONFIG_MTK_ICUSB_SUPPORT
		if (skip_session_req_attr.value) {
			MYDBG("SESSION_REQUEST SKIPPED FOR IC USB\n");
		} else {
			devctl |= MUSBFSH_DEVCTL_SESSION;
			musbfsh_writeb(mregs, MUSBFSH_DEVCTL, devctl);
			musbfsh->ep0_stage = MUSBFSH_EP0_START;
			musbfsh_set_vbus(musbfsh, 1);
		}
#else
		devctl |= MUSBFSH_DEVCTL_SESSION;
		musbfsh_writeb(mregs, MUSBFSH_DEVCTL, devctl);
		musbfsh->ep0_stage = MUSBFSH_EP0_START;
		musbfsh_set_vbus(musbfsh, 1);
#endif


		handled = IRQ_HANDLED;
	}

	if (int_usb & MUSBFSH_INTR_VBUSERROR) {
		int ignore = 0;
		char *s;

		/* During connection as an A-Device, we may see a short
		 * current spikes causing voltage drop, because of cable
		 * and peripheral capacitance combined with vbus draw.
		 * (So: less common with truly self-powered devices, where
		 * vbus doesn't act like a power supply.)
		 *
		 * Such spikes are short; usually less than ~500 usec, max
		 * of ~2 msec.  That is, they're not sustained overcurrent
		 * errors, though they're reported using VBUSERROR irqs.
		 *
		 * Workarounds:  (a) hardware: use self powered devices.
		 * (b) software:  ignore non-repeated VBUS errors.
		 *
		 * REVISIT:  do delays from lots of DEBUG_KERNEL checks
		 * make trouble here, keeping VBUS < 4.4V ?
		 */
		if (musbfsh->vbuserr_retry) {
			musbfsh->vbuserr_retry--;
			ignore = 1;
			devctl |= MUSBFSH_DEVCTL_SESSION;
			musbfsh_writeb(mregs, MUSBFSH_DEVCTL, devctl);
		} else {
			musbfsh->port1_status |=
				USB_PORT_STAT_OVERCURRENT |
				(USB_PORT_STAT_C_OVERCURRENT << 16);
		}

		switch (devctl & MUSBFSH_DEVCTL_VBUS) {
		case 0 << MUSBFSH_DEVCTL_VBUS_SHIFT:
			s = "<SessEnd";
			break;
		case 1 << MUSBFSH_DEVCTL_VBUS_SHIFT:
			s = "<AValid";
			break;
		case 2 << MUSBFSH_DEVCTL_VBUS_SHIFT:
			s = "<VBusValid";
			break;
		/* case 3 << MUSBFSH_DEVCTL_VBUS_SHIFT: */
		default:
			s = "VALID";
			break;
		}

		ERR("VBUS_ERROR (%02x, %s), retry #%d, port1_status 0x%08x\n",
		    devctl, s, VBUSERR_RETRY_COUNT - musbfsh->vbuserr_retry,
		    musbfsh->port1_status);

		/* go through A_WAIT_VFALL then start a new session */
		if (!ignore)
			musbfsh_set_vbus(musbfsh, 0);
		handled = IRQ_HANDLED;
	}

	if (int_usb & MUSBFSH_INTR_CONNECT) {
		struct usb_hcd *hcd = musbfsh_to_hcd(musbfsh);

#ifdef CONFIG_MTK_ICUSB_SUPPORT
		set_usb11_sts_connect();
#endif

#ifndef CONFIG_MTK_ICUSB_SUPPORT
#ifndef MTK_DT_USB_SUPPORT
		wake_lock(&musbfsh_suspend_lock);
#endif
#endif

		handled = IRQ_HANDLED;
		musbfsh->is_active = 1;
		musbfsh->ep0_stage = MUSBFSH_EP0_START;
		musbfsh->port1_status &=
			~(USB_PORT_STAT_LOW_SPEED | USB_PORT_STAT_HIGH_SPEED |
			USB_PORT_STAT_ENABLE);
		musbfsh->port1_status |=
			USB_PORT_STAT_CONNECTION |
			(USB_PORT_STAT_C_CONNECTION << 16);

		/* high vs full speed is just a guess until after reset */
		if (devctl & MUSBFSH_DEVCTL_LSDEV)
			musbfsh->port1_status |= USB_PORT_STAT_LOW_SPEED;

		if (hcd->status_urb)
			usb_hcd_poll_rh_status(hcd);
		else
			usb_hcd_resume_root_hub(hcd);


#ifdef CONFIG_MTK_ICUSB_SUPPORT
#define OPSTATE_MODE 0x04
#define OSR_INTE 0x80
#define OSR_START 0x01

		if (hw_dbg_attr.value) {
			/* start hw debugging */
			u8 opr_csr = musbfsh_readb(mregs, 0x610);

			opr_csr |= OPSTATE_MODE;
			opr_csr |= OSR_INTE;
			opr_csr |= OSR_START;
			musbfsh_writeb(mregs, 0x610, opr_csr);
		} else {
			MYDBG("");
		}
#if 0
		WARNING("CONNECT! devctl 0x%02x, g_usb_clk_reg_before: %x\n",
			devctl, g_usb_clk_reg_before);
		WARNING("g_usb_clk_reg_after: %x, 0xF000014C: %x\n",
			g_usb_clk_reg_after, __raw_readb(0xF000014C));
		WARNING("g_FS_EOF1_before: %x, g_FS_EOF1_after :%x\n",
			g_FS_EOF1_before, g_FS_EOF1_after);
#endif
#endif
		mtk_musbfsh = musbfsh;
#ifdef MTK_DT_USB_SUPPORT
		/* wx, need to be done before EINT unmask */
		wake_lock_init(&usb_resume_lock, WAKE_LOCK_SUSPEND,
			       "USB11 wakelock");
		register_eint_usb_p1_wakeup_callback();
#endif
		WARNING("CONNECT ! devctl 0x%02x\n", devctl);

	}

	if (int_usb & MUSBFSH_INTR_DISCONNECT) {
		WARNING("DISCONNECT !devctl %02x\n", devctl);
#ifdef CONFIG_MTK_ICUSB_SUPPORT
		if (!check_usb11_sts_disconnect_done())
			set_usb11_sts_disconnecting();
		mt65xx_usb11_mac_reset_and_phy_stress_set();
#endif
		handled = IRQ_HANDLED;
		usb_hcd_resume_root_hub(musbfsh_to_hcd(musbfsh));
		musbfsh_root_disconnect(musbfsh);


#ifndef CONFIG_MTK_ICUSB_SUPPORT
#ifndef MTK_DT_USB_SUPPORT
		wake_unlock(&musbfsh_suspend_lock);
#endif
#endif
	}

	/* mentor saves a bit: bus reset and babble share the same irq.
	 * only host sees babble; only peripheral sees bus reset.
	 */
	if (int_usb & MUSBFSH_INTR_BABBLE) {
		handled = IRQ_HANDLED;
		/*
		 * Looks like non-HS BABBLE can be ignored, but
		 * HS BABBLE is an error condition. For HS the solution
		 * is to avoid babble in the first place and fix what
		 * caused BABBLE. When HS BABBLE happens we can only
		 * stop the session.
		 */

#ifdef CONFIG_MTK_ICUSB_SUPPORT
#define OSR_START 0x01

		if (hw_dbg_attr.value) {
			unsigned int dbg_regs[] = {	/* debug registers */
				0x640, 0x644, 0x648, 0x64C,
				0x650, 0x654, 0x658, 0x65C
			};
			int i;
			u8 op_state;
			u8 opr_csr = musbfsh_readb(mregs, 0x610);

			/* stop hw debugging */
			opr_csr &= ~OSR_START;
			musbfsh_writeb(mregs, 0x610, opr_csr);

			udelay(100);

			/* dump result */
			WARNING("BABBLE!, dump dbg register\n");

			for (i = 0; i < sizeof(dbg_regs) / sizeof(int); i++)
				pr_err("offset=%x, value=%x\n", dbg_regs[i],
				       musbfsh_readl(mregs, dbg_regs[i]));

			op_state = (u8) musbfsh_readb(mregs, (unsigned)0x620);
			WARNING("BABBLE!, op_state : %x\n",
				(unsigned int)op_state);
		} else {
			MYDBG("");
		}
#endif
#if defined(MTK_ICUSB_BABBLE_RECOVER) || defined(MTK_SMARTBOOK_SUPPORT)
		/* call mt65xx_usb11_mac_phy_babble_clear at first */
		mt65xx_usb11_mac_phy_babble_clear(musbfsh);

		/* run babble_recover 20ms later */
		/* musbfsh->rh_timer = jiffies + msecs_to_jiffies(100); */

		WARNING("BABBLE! schedule babble_recover\n");

		usb_hcd_resume_root_hub(musbfsh_to_hcd(musbfsh));
		musbfsh_root_disconnect(musbfsh);

		mod_timer(&babble_recover_timer,
			  jiffies + msecs_to_jiffies(5000));
#else
		if (devctl & (MUSBFSH_DEVCTL_FSDEV | MUSBFSH_DEVCTL_LSDEV)) {
			ERR("BABBLE devctl: %02x\n", devctl);
			/*
			 * musbfsh_writeb(musbfsh->mregs,
			 * MUSBFSH_INTRUSBE, 0xf3);
			 */
		} else {
			ERR("Stopping host session -- babble, devctl : %x\n",
			    devctl);
			musbfsh_writeb(musbfsh->mregs, MUSBFSH_DEVCTL, 0);
		}
#endif
	}

	return handled;
}


/*
* Program the HDRC to start (enable interrupts, dma, etc.).
*/
void musbfsh_start(struct musbfsh *musbfsh)
{
	void __iomem *regs = musbfsh->mregs;	/* base address of usb mac */
	u8 devctl = musbfsh_readb(regs, MUSBFSH_DEVCTL);
	u8 power = musbfsh_readb(regs, MUSBFSH_POWER);
	int int_level1 = 0;

	WARNING("<== devctl 0x%02x\n", devctl);

#ifdef CONFIG_MTK_ICUSB_SUPPORT
#if 0
	/* no automatically stop session when babble */
	unsigned char result = musbfsh_readb(regs, 0x74);

	result &= ~(0x40);
	musbfsh_writeb(regs, 0x74, result);

	/* macpual test from web , TI solution */
	g_FS_EOF1_before = musbfsh_readb(regs, 0x7D);
	musbfsh_writeb(regs, 0x7D, 0xB4);
	g_FS_EOF1_after = musbfsh_readb(regs, 0x7D);
#endif
#endif

	/*  Set INT enable registers, enable interrupts */
	musbfsh_writew(regs, MUSBFSH_INTRTXE, musbfsh->epmask);
	musbfsh_writew(regs, MUSBFSH_INTRRXE, musbfsh->epmask & 0xfffe);

	musbfsh_writeb(regs, MUSBFSH_INTRUSBE, 0xf7);

	/* enable level 1 interrupts */
	musbfsh_writew(regs, USB11_L1INTM, 0x000f);
	int_level1 = musbfsh_readw(musbfsh->mregs, USB11_L1INTM);
	INFO("Level 1 Interrupt Mask 0x%x\n", int_level1);
	int_level1 = musbfsh_readw(musbfsh->mregs, USB11_L1INTP);
	INFO("Level 1 Interrupt Polarity 0x%x\n", int_level1);

	/* flush pending interrupts */
	musbfsh_writew(regs, MUSBFSH_INTRTX, 0xffff);
	musbfsh_writew(regs, MUSBFSH_INTRRX, 0xffff);
	musbfsh_writeb(regs, MUSBFSH_INTRUSB, 0xff);
	musbfsh_writeb(regs, MUSBFSH_HSDMA_INTR, 0xff);

	musbfsh->is_active = 0;
	musbfsh->is_multipoint = 1;

	/* need to enable the VBUS */
#ifndef MTK_DT_USB_SUPPORT
	musbfsh_set_vbus(musbfsh, 1);
#endif
	musbfsh_platform_enable(musbfsh);

#if 0	/* IC_USB from SS5 */
#ifndef IC_USB
	/* start session, assume ID pin is hard-wired to ground */
	/* wx? why not wait until device connected */
	devctl |= MUSBFSH_DEVCTL_SESSION;
	musbfsh_writeb(regs, MUSBFSH_DEVCTL, devctl);
#endif
#endif

#ifdef CONFIG_MTK_ICUSB_SUPPORT
	if (skip_enable_session_attr.value) {
		MYDBG("SESSION ENABLE SKIPPED FOR IC USB\n");
	} else {
		/* start session, assume ID pin is hard-wired to ground */
		/* wx? why not wait until device connected */
		devctl |= MUSBFSH_DEVCTL_SESSION;
		musbfsh_writeb(regs, MUSBFSH_DEVCTL, devctl);
	}
#else
	/* start session, assume ID pin is hard-wired to ground */
	/* wx? why not wait until device connected */
	devctl |= MUSBFSH_DEVCTL_SESSION;
	musbfsh_writeb(regs, MUSBFSH_DEVCTL, devctl);
#endif

	/* disable high speed negotiate */
	power |= MUSBFSH_POWER_HSENAB;
	power |= MUSBFSH_POWER_SOFTCONN;

	/* wx, not necessary for USB11 host */
	/* power &= ~(MUSBFSH_POWER_HSENAB); */
	/*
	 * enable SUSPENDM, this will put PHY into low power mode,
	 * not so "low" as save current mode,
	 * but it will be able to detect line state
	 * (remote wakeup/connect/disconnect)
	 */
	power |= MUSBFSH_POWER_ENSUSPEND;
	musbfsh_writeb(regs, MUSBFSH_POWER, power);

#ifdef MUSBFSH_DBG_REG
	/* stop auto clear session */
	u32 tmp = musbfsh_readl(regs, 0x74);

	WARNING("tmp :%x, 1\n", tmp);
	tmp &= ~(1 << 6);
	WARNING("tmp :%x, 2\n", tmp);
	musbfsh_writeb(regs, 0x74, tmp);
	tmp = musbfsh_readl(regs, 0x74);
	WARNING("tmp :%x, 3\n", tmp);

	int i;
	u32 addr[] = {0x60, 0x74, 0x620, 0x630};

	for (i = 0; i < sizeof(addr) / sizeof(u32); i++) {
		WARNING("dbg addr : %x, val : %x\n",
			addr[i], musbfsh_readl(regs, addr[i]));
	}
#endif


#ifndef CONFIG_MTK_ICUSB_SUPPORT
	devctl = musbfsh_readb(regs, MUSBFSH_DEVCTL);
	power = musbfsh_readb(regs, MUSBFSH_POWER);
	WARNING(" musb ready. devctl=0x%x, power=0x%x\n", devctl, power);
	mdelay(500);		/* wx? */
#endif

}


static void musbfsh_generic_disable(struct musbfsh *musbfsh)
{
	void __iomem *mregs = musbfsh->mregs;
	u16 temp;

	INFO("%s++\r\n", __func__);
	/* disable interrupts */
	musbfsh_writeb(mregs, MUSBFSH_INTRUSBE, 0);
	musbfsh_writew(mregs, MUSBFSH_INTRTXE, 0);
	musbfsh_writew(mregs, MUSBFSH_INTRRXE, 0);

	/* off */
	musbfsh_writeb(mregs, MUSBFSH_DEVCTL, 0);

	/*  flush pending interrupts */
	temp = musbfsh_readb(mregs, MUSBFSH_INTRUSB);
	temp = musbfsh_readw(mregs, MUSBFSH_INTRTX);
	temp = musbfsh_readw(mregs, MUSBFSH_INTRRX);
}


/*
 * Make the HDRC stop (disable interrupts, etc.);
 * reversible by musbfsh_start
 * called on gadget driver unregister
 * with controller locked, irqs blocked
 * acts as a NOP unless some role activated the hardware
 */
void musbfsh_stop(struct musbfsh *musbfsh)
{
	/* stop IRQs, timers, ... */
	musbfsh_platform_disable(musbfsh);
	musbfsh_generic_disable(musbfsh);
	INFO("HDRC disabled\n");

	/* FIXME
	 *  - mark host and/or peripheral drivers unusable/inactive
	 *  - disable DMA (and enable it in HdrcStart)
	 *  - make sure we can musbfsh_start() after musbfsh_stop(); with
	 *    OTG mode, gadget driver module rmmod/modprobe cycles that
	 *  - ...
	 */
	musbfsh_platform_try_idle(musbfsh, 0);
}

static void musbfsh_shutdown(struct platform_device *pdev)
{
	struct musbfsh *musbfsh = dev_to_musbfsh(&pdev->dev);
	unsigned long flags;

	INFO("%s++\r\n", __func__);
	spin_lock_irqsave(&musbfsh->lock, flags);
	musbfsh_platform_disable(musbfsh);
	musbfsh_generic_disable(musbfsh);
	musbfsh_set_vbus(musbfsh, 0);
	musbfsh_set_power(musbfsh, 0);
	spin_unlock_irqrestore(&musbfsh->lock, flags);

	/* FIXME power down */
}

/*
 * tables defining fifo_mode values.  define more if you like.
 * for host side, make sure both halves of ep1 are set up.
 */

/* fits in 4KB */
#define MAXFIFOSIZE 8096

static struct musbfsh_fifo_cfg epx_cfg[] __initdata = {
	{.hw_ep_num = 1, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 1, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 2, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 2, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 3, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 3, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 4, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 4, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 5, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 5, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
};

static struct musbfsh_fifo_cfg ep0_cfg __initdata = {
	.style = FIFO_RXTX, .maxpacket = 64,
};

#ifdef CONFIG_MTK_ICUSB_SUPPORT
static struct musbfsh_fifo_cfg ep0_cfg1 = {
	.style = FIFO_RXTX, .maxpacket = 64,
};

static struct musbfsh_fifo_cfg epx_cfg1[] = {
	{.hw_ep_num = 1, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 1, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 2, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 2, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 3, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 3, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 4, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 4, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 5, .style = FIFO_TX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
	{.hw_ep_num = 5, .style = FIFO_RX, .maxpacket = 512,
	 .mode = BUF_SINGLE
	},
};

static int fifo_setup1(struct musbfsh *musbfsh, struct musbfsh_hw_ep *hw_ep,
		       const struct musbfsh_fifo_cfg *cfg, u16 offset)
{
	void __iomem *mbase = musbfsh->mregs;
	int size = 0;
	u16 maxpacket = cfg->maxpacket;
	u16 c_off = offset >> 3;
	u8 c_size;		/* will be written into the fifo register */

	INFO("%s++,hw_ep->epnum=%d,cfg->hw_ep_num=%d\r\n",
	     __func__, hw_ep->epnum,
	     cfg->hw_ep_num);
	/* expect hw_ep has already been zero-initialized */

	size = ffs(max_t(u16, maxpacket, 8)) - 1;
	maxpacket = 1 << size;

	c_size = size - 3;
	if (cfg->mode == BUF_DOUBLE) {
		if ((offset + (maxpacket << 1)) > MAXFIFOSIZE)
			return -EMSGSIZE;
		c_size |= MUSBFSH_FIFOSZ_DPB;
	} else {
		if ((offset + maxpacket) > MAXFIFOSIZE)
			return -EMSGSIZE;
	}

	/* configure the FIFO */
	musbfsh_writeb(mbase, MUSBFSH_INDEX, hw_ep->epnum);
	/* EP0 reserved endpoint for control, bidirectional;
	 * EP1 reserved for bulk, two unidirection halves.
	 */
	if (hw_ep->epnum == 1)
		musbfsh->bulk_ep = hw_ep;
	/* REVISIT error check:  be sure ep0 can both rx and tx ... */
	switch (cfg->style) {
	case FIFO_TX:
		musbfsh_write_txfifosz(mbase, c_size);
		musbfsh_write_txfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = !!(c_size & MUSBFSH_FIFOSZ_DPB);
		hw_ep->max_packet_sz_tx = maxpacket;
		break;
	case FIFO_RX:
		musbfsh_write_rxfifosz(mbase, c_size);
		musbfsh_write_rxfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSBFSH_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;
		break;
	case FIFO_RXTX:
		musbfsh_write_txfifosz(mbase, c_size);
		musbfsh_write_txfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSBFSH_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;

		musbfsh_write_rxfifosz(mbase, c_size);
		musbfsh_write_rxfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = hw_ep->rx_double_buffered;
		hw_ep->max_packet_sz_tx = maxpacket;
		hw_ep->is_shared_fifo = true;
		break;
	}

	/* NOTE rx and tx endpoint irqs aren't managed separately,
	 * which happens to be ok
	 */
	musbfsh->epmask |= (1 << hw_ep->epnum);

	return offset + (maxpacket << ((c_size & MUSBFSH_FIFOSZ_DPB) ? 1 : 0));
}
#endif
/*
 * configure a fifo; for non-shared endpoints, this may be called
 * once for a tx fifo and once for an rx fifo.
 *
 * returns negative errno or offset for next fifo.
 */
static int __init fifo_setup(struct musbfsh *musbfsh,
			     struct musbfsh_hw_ep *hw_ep,
			     const struct musbfsh_fifo_cfg *cfg, u16 offset)
{
	void __iomem *mbase = musbfsh->mregs;
	int size = 0;
	u16 maxpacket = cfg->maxpacket;
	u16 c_off = offset >> 3;
	u8 c_size;		/* will be written into the fifo register */

	INFO("%s++,hw_ep->epnum=%d,cfg->hw_ep_num=%d\r\n",
	     __func__, hw_ep->epnum, cfg->hw_ep_num);

	/* expect hw_ep has already been zero-initialized */

	size = ffs(max_t(u16, maxpacket, 8)) - 1;
	maxpacket = 1 << size;

	c_size = size - 3;
	if (cfg->mode == BUF_DOUBLE) {
		if ((offset + (maxpacket << 1)) > MAXFIFOSIZE)
			return -EMSGSIZE;
		c_size |= MUSBFSH_FIFOSZ_DPB;
	} else {
		if ((offset + maxpacket) > MAXFIFOSIZE)
			return -EMSGSIZE;
	}

	/* configure the FIFO */
	musbfsh_writeb(mbase, MUSBFSH_INDEX, hw_ep->epnum);
	/* EP0 reserved endpoint for control, bidirectional;
	 * EP1 reserved for bulk, two unidirection halves.
	 */
	if (hw_ep->epnum == 1)
		musbfsh->bulk_ep = hw_ep;
	/* REVISIT error check:  be sure ep0 can both rx and tx ... */
	switch (cfg->style) {
	case FIFO_TX:
		musbfsh_write_txfifosz(mbase, c_size);
		musbfsh_write_txfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = !!(c_size & MUSBFSH_FIFOSZ_DPB);
		hw_ep->max_packet_sz_tx = maxpacket;
		break;
	case FIFO_RX:
		musbfsh_write_rxfifosz(mbase, c_size);
		musbfsh_write_rxfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSBFSH_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;
		break;
	case FIFO_RXTX:
		musbfsh_write_txfifosz(mbase, c_size);
		musbfsh_write_txfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSBFSH_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;

		musbfsh_write_rxfifosz(mbase, c_size);
		musbfsh_write_rxfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = hw_ep->rx_double_buffered;
		hw_ep->max_packet_sz_tx = maxpacket;
		hw_ep->is_shared_fifo = true;
		break;
	}

	/* NOTE rx and tx endpoint irqs aren't managed separately,
	 * which happens to be ok
	 */
	musbfsh->epmask |= (1 << hw_ep->epnum);

	return offset + (maxpacket << ((c_size & MUSBFSH_FIFOSZ_DPB) ? 1 : 0));
}

#ifdef CONFIG_MTK_ICUSB_SUPPORT
static int ep_config_from_table1(struct musbfsh *musbfsh)
{
	const struct musbfsh_fifo_cfg *cfg = NULL;
	unsigned i = 0;
	unsigned n = 0;
	int offset;
	struct musbfsh_hw_ep *hw_ep = musbfsh->endpoints;

	INFO("musbfsh: %s++\r\n", __func__);

	/* modification from org */
	musbfsh->config->fifo_cfg = epx_cfg1;
	musbfsh->config->fifo_cfg_size =
		sizeof(epx_cfg1) / sizeof(struct musbfsh_fifo_cfg);

	if (musbfsh->config->fifo_cfg) {
		cfg = musbfsh->config->fifo_cfg;
		n = musbfsh->config->fifo_cfg_size;
		INFO("musbfsh:fifo_cfg, n=%d\n", n);
		goto done;
	}

done:
	offset = fifo_setup1(musbfsh, hw_ep, &ep0_cfg1, 0);
	/* assert(offset > 0) */

	/* NOTE:  for RTL versions >= 1.400 EPINFO and RAMINFO would
	 * be better than static musbfsh->config->num_eps and DYN_FIFO_SIZE...
	 */

	for (i = 0; i < n; i++) {
		u8 epn = cfg->hw_ep_num;

		if (epn >= MUSBFSH_C_NUM_EPS) {
			ERR("%s: invalid ep %d\n", musbfsh_driver_name, epn);
			return -EINVAL;
		}
		offset = fifo_setup1(musbfsh, hw_ep + epn, cfg++, offset);
		if (offset < 0) {
			ERR("%s: mem overrun, ep %d\n",
			    musbfsh_driver_name, epn);
			return -EINVAL;
		}

		epn++;		/* include ep0 */
		musbfsh->nr_endpoints = max(epn, musbfsh->nr_endpoints);
	}
	INFO("%s: %d/%d max ep, %d/%d memory\n",
	     musbfsh_driver_name, n + 1, musbfsh->config->num_eps * 2 - 1,
	     offset, MAXFIFOSIZE);

	if (!musbfsh->bulk_ep) {
		ERR("%s: missing bulk\n", musbfsh_driver_name);
		return -EINVAL;
	}
	return 0;
}
#endif

static int __init ep_config_from_table(struct musbfsh *musbfsh)
{
	const struct musbfsh_fifo_cfg *cfg = NULL;
	unsigned i = 0;
	unsigned n = 0;
	int offset;
	struct musbfsh_hw_ep *hw_ep = musbfsh->endpoints;

	INFO("musbfsh: %s: ++\r\n", __func__);
	if (musbfsh->config->fifo_cfg) {
		cfg = musbfsh->config->fifo_cfg;
		n = musbfsh->config->fifo_cfg_size;
		INFO("musbfsh:fifo_cfg, n=%d\n", n);
		goto done;
	}

done:
	offset = fifo_setup(musbfsh, hw_ep, &ep0_cfg, 0);
	/* assert(offset > 0) */

	/*
	 * NOTE:  for RTL versions >= 1.400 EPINFO and RAMINFO would
	 * be better than static musbfsh->config->num_eps and DYN_FIFO_SIZE...
	 */

	for (i = 0; i < n; i++) {
		u8 epn = cfg->hw_ep_num;

		if (epn >= MUSBFSH_C_NUM_EPS) {
			ERR("%s: invalid ep %d\n", musbfsh_driver_name, epn);
			return -EINVAL;
		}
		offset = fifo_setup(musbfsh, hw_ep + epn, cfg++, offset);
		if (offset < 0) {
			ERR("%s: mem overrun, ep %d\n",
			    musbfsh_driver_name, epn);
			return -EINVAL;
		}

		epn++;		/* include ep0 */
		musbfsh->nr_endpoints = max(epn, musbfsh->nr_endpoints);
	}
	INFO("%s: %d/%d max ep, %d/%d memory\n",
	     musbfsh_driver_name, n + 1, musbfsh->config->num_eps * 2 - 1,
	     offset, MAXFIFOSIZE);

	if (!musbfsh->bulk_ep) {
		ERR("%s: missing bulk\n", musbfsh_driver_name);
		return -EINVAL;
	}
	return 0;
}

#ifdef CONFIG_MTK_ICUSB_SUPPORT
static int musbfsh_core_init1(struct musbfsh *musbfsh)
{
	void __iomem *mbase = musbfsh->mregs;
	int status = 0;
	int i;

	INFO("%s\r\n", __func__);
	/* configure ep0 */
	musbfsh_configure_ep0(musbfsh);

	/* discover endpoint configuration */
	/* will update in func: ep_config_from_table */
	musbfsh->nr_endpoints = 1;
	musbfsh->epmask = 1;

	status = ep_config_from_table1(musbfsh);
	if (status < 0)
		return status;

	/* finish init, and print endpoint config */
	for (i = 0; i < musbfsh->nr_endpoints; i++) {
		struct musbfsh_hw_ep *hw_ep = musbfsh->endpoints + i;

		hw_ep->fifo = MUSBFSH_FIFO_OFFSET(i) + mbase;
		hw_ep->regs = MUSBFSH_EP_OFFSET(i, 0) + mbase;
		hw_ep->rx_reinit = 1;
		hw_ep->tx_reinit = 1;

		if (hw_ep->max_packet_sz_tx) {
			INFO("%s: hw_ep %d%s, %smax %d,and hw_ep->epnum=%d\n",
			     musbfsh_driver_name, i,
			     hw_ep->is_shared_fifo ? "shared" : "tx",
			     hw_ep->tx_double_buffered
			     ? "doublebuffer, " : "", hw_ep->max_packet_sz_tx,
			     hw_ep->epnum);
		}
		if (hw_ep->max_packet_sz_rx && !hw_ep->is_shared_fifo) {
			INFO("%s: hw_ep %d%s, %smax %d,and hw_ep->epnum=%d\n",
			     musbfsh_driver_name, i,
			     "rx",
			     hw_ep->rx_double_buffered
			     ? "doublebuffer, " : "", hw_ep->max_packet_sz_rx,
			     hw_ep->epnum);
		}
		if (!(hw_ep->max_packet_sz_tx || hw_ep->max_packet_sz_rx))
			INFO("hw_ep %d not configured\n", i);
	}
	return 0;
}
#endif


/* Initialize MUSB (M)HDRC part of the USB hardware subsystem;
 * configure endpoints, or take their config from silicon
 */
static int __init musbfsh_core_init(struct musbfsh *musbfsh)
{
	void __iomem *mbase = musbfsh->mregs;
	int status = 0;
	int i;

	INFO("%s\r\n", __func__);
	/* configure ep0 */
	musbfsh_configure_ep0(musbfsh);

	/* discover endpoint configuration */
	/* will update in func: ep_config_from_table */
	musbfsh->nr_endpoints = 1;
	musbfsh->epmask = 1;

	status = ep_config_from_table(musbfsh);
	if (status < 0)
		return status;

	/* finish init, and print endpoint config */
	for (i = 0; i < musbfsh->nr_endpoints; i++) {
		struct musbfsh_hw_ep *hw_ep = musbfsh->endpoints + i;

		hw_ep->fifo = MUSBFSH_FIFO_OFFSET(i) + mbase;
		hw_ep->regs = MUSBFSH_EP_OFFSET(i, 0) + mbase;
		hw_ep->rx_reinit = 1;
		hw_ep->tx_reinit = 1;

		if (hw_ep->max_packet_sz_tx) {
			INFO("%s: hw_ep %d%s, %smax %d,and hw_ep->epnum=%d\n",
			     musbfsh_driver_name, i,
			     hw_ep->is_shared_fifo ? "shared" : "tx",
			     hw_ep->tx_double_buffered
			     ? "doublebuffer, " : "", hw_ep->max_packet_sz_tx,
			     hw_ep->epnum);
		}
		if (hw_ep->max_packet_sz_rx && !hw_ep->is_shared_fifo) {
			INFO("%s: hw_ep %d%s, %smax %d,and hw_ep->epnum=%d\n",
			     musbfsh_driver_name, i,
			     "rx",
			     hw_ep->rx_double_buffered
			     ? "doublebuffer, " : "", hw_ep->max_packet_sz_rx,
			     hw_ep->epnum);
		}
		if (!(hw_ep->max_packet_sz_tx || hw_ep->max_packet_sz_rx))
			INFO("hw_ep %d not configured\n", i);
	}
	return 0;
}

void musbfsh_read_clear_generic_interrupt(struct musbfsh *musbfsh)
{
	musbfsh->int_usb = musbfsh_readb(musbfsh->mregs, MUSBFSH_INTRUSB);
	musbfsh->int_tx = musbfsh_readw(musbfsh->mregs, MUSBFSH_INTRTX);
	musbfsh->int_rx = musbfsh_readw(musbfsh->mregs, MUSBFSH_INTRRX);
	musbfsh->int_dma = musbfsh_readb(musbfsh->mregs, MUSBFSH_HSDMA_INTR);
	INFO("** musbfsh::IRQ! usb%04x tx%04x rx%04x dma%04x\n",
	     musbfsh->int_usb, musbfsh->int_tx, musbfsh->int_rx,
	     musbfsh->int_dma);
	/* clear interrupt status */
	musbfsh_writew(musbfsh->mregs, MUSBFSH_INTRTX, musbfsh->int_tx);
	musbfsh_writew(musbfsh->mregs, MUSBFSH_INTRRX, musbfsh->int_rx);
	musbfsh_writeb(musbfsh->mregs, MUSBFSH_INTRUSB, musbfsh->int_usb);
	musbfsh_writeb(musbfsh->mregs, MUSBFSH_HSDMA_INTR, musbfsh->int_dma);
}

void musbfsh_read_clear_generic_interrupt_tmp(struct musbfsh *musbfsh)
{
	int i = 0;

	while (i++ < 5) {
		musbfsh->int_usb =
			musbfsh_readb(musbfsh->mregs, MUSBFSH_INTRUSB);
		musbfsh->int_tx =
			musbfsh_readw(musbfsh->mregs, MUSBFSH_INTRTX);
		musbfsh->int_rx =
			musbfsh_readw(musbfsh->mregs, MUSBFSH_INTRRX);
		musbfsh->int_dma =
			musbfsh_readb(musbfsh->mregs, MUSBFSH_HSDMA_INTR);
		WARNING("** musbfsh::IRQ! usb%04x tx%04x rx%04x dma%04x\n",
			musbfsh->int_usb, musbfsh->int_tx, musbfsh->int_rx,
			musbfsh->int_dma);
		/* clear interrupt status */
		musbfsh_writew(musbfsh->mregs, MUSBFSH_INTRTX, musbfsh->int_tx);
		musbfsh_writew(musbfsh->mregs, MUSBFSH_INTRRX, musbfsh->int_rx);
		musbfsh_writeb(musbfsh->mregs,
			       MUSBFSH_INTRUSB, musbfsh->int_usb);
		musbfsh_writeb(musbfsh->mregs,
			       MUSBFSH_HSDMA_INTR, musbfsh->int_dma);
	}
}

static irqreturn_t generic_interrupt(int irq, void *__hci)
{
	unsigned long flags;
	irqreturn_t retval = IRQ_NONE;
	struct musbfsh *musbfsh = __hci;
	u16 int_level1 = 0;

	INFO("musbfsh:generic_interrupt++\r\n");
#ifdef CONFIG_MTK_ICUSB_SUPPORT
#if 0
	if (mt65xx_check_usb11_clk_status()) {
		dump_stack();
		BUG();
	}
#endif
#endif
	spin_lock_irqsave(&musbfsh->lock, flags);

	musbfsh_read_clear_generic_interrupt(musbfsh);
	int_level1 = musbfsh_readw(musbfsh->mregs, USB11_L1INTS);
	INFO("Level 1 Interrupt Status 0x%x\n", int_level1);

	if (musbfsh->int_usb || musbfsh->int_tx || musbfsh->int_rx)
		retval = musbfsh_interrupt(musbfsh);
	if (musbfsh->int_dma)
		retval = musbfsh_dma_controller_irq(irq,
			musbfsh->musbfsh_dma_controller);

	spin_unlock_irqrestore(&musbfsh->lock, flags);
	return retval;
}

/*
 * handle all the irqs defined by the HDRC core. for now we expect:  other
 * irq sources (phy, dma, etc) will be handled first, musbfsh->int_* values
 * will be assigned, and the irq will already have been acked.
 *
 * called in irq context with spinlock held, irqs blocked
 */
irqreturn_t musbfsh_interrupt(struct musbfsh *musbfsh)
{
	irqreturn_t retval = IRQ_NONE;
	u8 devctl, power;
	int ep_num;
	u32 reg;

	devctl = musbfsh_readb(musbfsh->mregs, MUSBFSH_DEVCTL);
	power = musbfsh_readb(musbfsh->mregs, MUSBFSH_POWER);

	INFO("** musbfsh::devctl 0x%x power 0x%x\n", devctl, power);

	/* the core can interrupt us for multiple reasons; docs have
	 * a generic interrupt flowchart to follow
	 */
	if (musbfsh->int_usb) {
		retval |= musbfsh_stage0_irq(musbfsh, musbfsh->int_usb,
					     devctl, power);
	}

	/* "stage 1" is handling endpoint irqs */

	/* handle endpoint 0 first */
	if (musbfsh->int_tx & 1)
		retval |= musbfsh_h_ep0_irq(musbfsh);

	/* RX on endpoints 1-15 */
	reg = musbfsh->int_rx >> 1;
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			/* musbfsh_ep_select(musbfsh->mregs, ep_num); */
			/* REVISIT just retval = ep->rx_irq(...) */
			retval = IRQ_HANDLED;
			/* the real ep_num */
			musbfsh_host_rx(musbfsh, ep_num);
		}

		reg >>= 1;
		ep_num++;
	}

	/* TX on endpoints 1-15 */
	reg = musbfsh->int_tx >> 1;
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			/* musbfsh_ep_select(musbfsh->mregs, ep_num); */
			/* REVISIT just retval |= ep->tx_irq(...) */
			retval = IRQ_HANDLED;
			musbfsh_host_tx(musbfsh, ep_num);
		}
		reg >>= 1;
		ep_num++;
	}
	return retval;
}

#ifndef CONFIG_MUSBFSH_PIO_ONLY
static bool use_dma __initdata = 1;

/* "modprobe ... use_dma=0" etc */
module_param(use_dma, bool, 0);
MODULE_PARM_DESC(use_dma, "enable/disable use of DMA");

#else
#define use_dma			0
#endif

void musbfsh_dma_completion(struct musbfsh *musbfsh, u8 epnum, u8 transmit)
{
	INFO("%s++\r\n", __func__);
	/* called with controller lock already held */

	/* endpoints 1..15 */
	if (transmit)
		musbfsh_host_tx(musbfsh, epnum);
	else
		musbfsh_host_rx(musbfsh, epnum);
}

/*
 * Init support
 */

static struct musbfsh *__init alloc_instance(struct device *dev,
					     struct musbfsh_hdrc_config *config,
					     void __iomem *mbase)
{
	struct musbfsh *musbfsh;
	struct musbfsh_hw_ep *ep;
	int epnum;
	struct usb_hcd *hcd;

	INFO("%s++\r\n", __func__);
	hcd = usb_create_hcd(&musbfsh_hc_driver, dev, dev_name(dev));
	if (!hcd)
		return NULL;
	/* usbcore sets dev->driver_data to hcd, and sometimes uses that... */

	musbfsh = hcd_to_musbfsh(hcd);
	INIT_LIST_HEAD(&musbfsh->control);
	INIT_LIST_HEAD(&musbfsh->in_bulk);
	INIT_LIST_HEAD(&musbfsh->out_bulk);

	hcd->uses_new_polling = 1;
	hcd->has_tt = 1;

	musbfsh->vbuserr_retry = VBUSERR_RETRY_COUNT;

	musbfsh->mregs = mbase;
	musbfsh->ctrl_base = mbase;
	/* will be update after return from this func */
	musbfsh->nIrq = -ENODEV;
	musbfsh->config = config;
	BUG_ON(musbfsh->config->num_eps > MUSBFSH_C_NUM_EPS);
	for (epnum = 0, ep = musbfsh->endpoints;
		epnum < musbfsh->config->num_eps; epnum++, ep++) {
		ep->musbfsh = musbfsh;
		ep->epnum = epnum;
	}

	musbfsh->controller = dev;
	return musbfsh;
}

static void musbfsh_free(struct musbfsh *musbfsh)
{
	/* this has multiple entry modes. it handles fault cleanup after
	 * probe(), where things may be partially set up, as well as rmmod
	 * cleanup after everything's been de-activated.
	 */
	INFO("%s++\r\n", __func__);
	if (musbfsh->nIrq >= 0) {
		if (musbfsh->irq_wake)
			disable_irq_wake(musbfsh->nIrq);
		free_irq(musbfsh->nIrq, musbfsh);
	}
	if (is_dma_capable() && musbfsh->dma_controller) {
		struct dma_controller *c = musbfsh->dma_controller;

		(void)c->stop(c);
		musbfsh_dma_controller_destroy(c);
	}
	usb_put_hcd(musbfsh_to_hcd(musbfsh));
	kfree(musbfsh);
}

/*
 * Perform generic per-controller initialization.
 *
 * @pDevice: the controller (already clocked, etc)
 * @nIrq: irq
 * @mregs: virtual address of controller registers,
 *	not yet corrected for platform-specific offsets
 */
static int __init musbfsh_init_controller(struct device *dev,
					  int nIrq, void __iomem *ctrl,
					  void __iomem *pbase)
{
	int status;
	struct musbfsh *musbfsh;
	struct musbfsh_hdrc_platform_data *plat = dev->platform_data;
	struct usb_hcd *hcd;

#if 0
	struct musbfsh_hdrc_config *config;
	struct device_node *np = dev->of_node;
#endif

	WARNING("%s++\r\n", __func__);
	/* The driver might handle more features than the board; OK.
	 * Fail when the board needs a feature that's not enabled.
	 */

#if 0
	plat = devm_kzalloc(dev, sizeof(*plat), GFP_KERNEL);
	if (!plat)
		goto fail0;

	config = devm_kzalloc(dev, sizeof(*config), GFP_KERNEL);
	if (!config)
		goto fail0;

	of_property_read_u32(np, "mode", (u32 *) &plat->mode);

	/* __deprecated */
	/* of_property_read_u32(np, "dma_channels",
		(u32 *)&config->dma_channels); */
	of_property_read_u32(np, "num_eps", (u32 *) &config->num_eps);
	config->multipoint = of_property_read_bool(np, "multipoint");
	/* __deprecated */
	/* config->dyn_fifo = of_property_read_bool(np, "dyn_fifo"); */
	/* __deprecated */
	/* config->soft_con = of_property_read_bool(np, "soft_con"); */
	/* config->dma = of_property_read_bool(np, "dma"); //__deprecated */

	plat->config = config;

#endif

	dev->platform_data = plat;
	if (!plat) {
		dev_dbg(dev, "no platform_data?\n");
		status = -ENODEV;
		goto fail0;
	}

	/* allocate */
	musbfsh = alloc_instance(dev, plat->config, ctrl);
	if (!musbfsh) {
		status = -ENOMEM;
		goto fail0;
	}
	spin_lock_init(&musbfsh->lock);
	musbfsh->phy_base = pbase;

	/* bugfix: g_musbfsh should be assigned as earlier as possible */
	g_musbfsh = musbfsh;

	musbfsh->board_mode = plat->mode;
	musbfsh->config->fifo_cfg = epx_cfg;
	musbfsh->config->fifo_cfg_size =
		sizeof(epx_cfg) / sizeof(struct musbfsh_fifo_cfg);
	/*
	 * The musbfsh_platform_init() call:
	 *   - adjusts musbfsh->mregs and musbfsh->isr if needed,
	 *   - may initialize an integrated tranceiver
	 *   - initializes musbfsh->xceiv, usually by otg_get_transceiver()
	 *   - activates clocks.
	 *   - stops powering VBUS
	 *   - assigns musbfsh->board_set_vbus if host mode is enabled
	 *
	 * There are various transceiver configurations.  Blackfin,
	 * DaVinci, TUSB60x0, and others integrate them.  OMAP3 uses
	 * external/discrete ones in various flavors (twl4030 family,
	 * isp1504, non-OTG, etc) mostly hooking up through ULPI.
	 */
	musbfsh->isr = generic_interrupt;
	status = musbfsh_platform_init(musbfsh);
	if (status < 0) {
		ERR("musbfsh_platform_init fail!status=%d", status);
		goto fail1;
	}

	if (!musbfsh->isr) {
		status = -ENODEV;
		goto fail2;
	}
#ifndef CONFIG_MUSBFSH_PIO_ONLY
	INFO("DMA mode\n");
	if (use_dma && dev->dma_mask) {
		struct dma_controller *c;

		/* only software config */
		c = musbfsh_dma_controller_create(musbfsh, musbfsh->mregs);
		musbfsh->dma_controller = c;
		if (c)
			(void)c->start(c);	/* do nothing in fact */
	}
#else
	INFO("PIO mode\n");
#endif

	/* ideally this would be abstracted in platform setup */
	if (!is_dma_capable() || !musbfsh->dma_controller)
		dev->dma_mask = NULL;

	/* be sure interrupts are disabled before connecting ISR */
	/* wz, need implement in MT65xx, but not power off! */
	musbfsh_platform_disable(musbfsh);

#ifdef CONFIG_MTK_ICUSB_SUPPORT
	if (skip_mac_init_attr.value) {
		MYDBG("skip musbfsh_generic_disable(), musbfsh_core_init()\n");
	} else {
		/* must power on the USB module */
		musbfsh_generic_disable(musbfsh);

		/* setup musb parts of the core (especially endpoints) */
		status = musbfsh_core_init(musbfsh);
		if (status < 0) {
			ERR("musbfsh_core_init fail!");
			goto fail2;
		}
	}
#else
	/* must power on the USB module */
	musbfsh_generic_disable(musbfsh);

	/* setup musb parts of the core (especially endpoints) */
	status = musbfsh_core_init(musbfsh);
	if (status < 0) {
		ERR("musbfsh_core_init fail!");
		goto fail2;
	}
#endif

	/* attach to the IRQ */
	/*
	 * wx? usb_add_hcd will also try do request_irq,
	 * if hcd_driver.irq is set
	 */
	if (request_irq(nIrq, musbfsh->isr, IRQF_TRIGGER_LOW,
			dev_name(dev), musbfsh)) {
		dev_err(dev, "musbfsh::request_irq %d failed!\n", nIrq);
		status = -ENODEV;
		goto fail2;
	}

	/* update the musbfsh->nIrq after request_irq ! */
	musbfsh->nIrq = nIrq;

	/* FIXME this handles wakeup irqs wrong */
	/* wx, need to be replaced by modifying kernel/core/mt6573_ost.c */
	if (enable_irq_wake(nIrq) == 0) {
		musbfsh->irq_wake = 1;
		/* wx? usb_add_hcd will do this any way */
		device_init_wakeup(dev, 1);
	} else {
		musbfsh->irq_wake = 0;
	}

	/* host side needs more setup */
	hcd = musbfsh_to_hcd(musbfsh);
	/* plat->power can be set to 0,so the power is set to 500ma. */
	hcd->power_budget = 2 * (plat->power ? plat->power : 250);

	/*
	 * For the host-only role, we can activate right away.
	 * (We expect the ID pin to be forcibly grounded!!)
	 * Otherwise, wait till the gadget driver hooks up.
	 */
	/* Important!! */
	status = usb_add_hcd(musbfsh_to_hcd(musbfsh), -1, 0);

	if (status < 0) {
		ERR("musbfsh::usb_add_hcd fail!");
		goto fail2;
	}

	dev_info(dev, "USB controller at %p using %s, IRQ %d\n",
		 ctrl, (is_dma_capable() && musbfsh->dma_controller)
		 ? "DMA" : "PIO", musbfsh->nIrq);

#if defined(MTK_ICUSB_BABBLE_RECOVER) || defined(MTK_SMARTBOOK_SUPPORT)
	init_timer(&babble_recover_timer);
	babble_recover_timer.function = babble_recover_func;
	babble_recover_timer.data = (unsigned long)musbfsh;
#endif

#ifdef CONFIG_MUSBFSH_DEBUG_FS
	status = musbfsh_init_debugfs(musbfsh);
	if (status < 0)
		goto fail2;
	create_musbfsh_cmd_proc_entry();
#endif

#ifdef CONFIG_MTK_ICUSB_SUPPORT
	set_usb_phy_voltage(VOL_50);
	mt65xx_usb11_phy_savecurrent();
	mt65xx_usb11_disable_clk_pll_pw();
	create_ic_usb_cmd_proc_entry();

#ifdef MTK_ICUSB_TAKE_WAKE_LOCK
	wake_lock(&musbfsh_suspend_lock);
#else
	MYDBG("skip wake_lock(&musbfsh_suspend_lock)\n");
#endif

	/* MYDBG("end of %s(), build time : %s\n", __func__, __TIME__); */

#endif
#ifdef MTK_DT_USB_SUPPORT
	get_eint_ext_usb_wkup_irq();

	/* temporialy for DSDA bring up */
	if (musbfsh_wake_lock_hold) {
		wake_lock(&musbfsh_suspend_lock);
		MYDBG("force wake_lock(&musbfsh_suspend_lock)\n");
	}
	create_dsda_tmp_entry();
#endif

	return 0;


fail2:
	if (musbfsh->irq_wake)
		device_init_wakeup(dev, 0);
	musbfsh_platform_exit(musbfsh);

fail1:
	dev_err(musbfsh->controller, "%s: failed with status %d\n",
		__func__, status);
	musbfsh_free(musbfsh);

fail0:
	dev_err(dev, "failed to initialize musbfsh\n");
	return status;

}

#define MUSBFSH_BASE_REGS_ADDR_RES_NAME "musbfsh_base"
#define MUSBFSH_SIF_REGS_ADDR_RES_NAME "musbfsh_sif"

#if 0
static void __iomem *acquire_reg_base(struct platform_device *pdev, const char *res_name)
{
	struct resource *iomem;
	void __iomem *base = NULL;

	iomem = platform_get_resource_byname(pdev, IORESOURCE_MEM, res_name);
	if (!iomem) {
		pr_err("Can't get resource for %s\n", res_name);
		goto end;
	}

	base = ioremap(iomem->start, resource_size(iomem));
	if (!(uintptr_t)base) {
		pr_err("Can't remap %s\n", res_name);
		goto end;
	}
	INFO("%s=0x%lx\n", res_name, (uintptr_t)(base));
end:
	return base;
}
#endif
/*-------------------------------------------------------------------------*/

/* all implementations (PCI bridge to FPGA, VLYNQ, etc) should just
 * bridge to a platform device; this driver then suffices.
 */

#ifndef CONFIG_MUSBFSH_PIO_ONLY
static u64 *orig_dma_mask;
#endif

#ifndef CONFIG_MTK_CLKMGR
struct clk *musb_clk11;
struct clk *musb_clk30;
#endif

static int __init musbfsh_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int status;
	int irq;
	void __iomem *base;
	void __iomem *pbase;
	struct device_node *node = NULL;

	INFO("%s++\r\n", __func__);

#ifdef CONFIG_MTK_ICUSB_SUPPORT
	/* test usb clk effect */
	/* ic_usb_clk_chg(); */
#endif

#ifndef CONFIG_MUSBFSH_PIO_ONLY	/* using DMA */
	/* clobbered by use_dma=n */
#ifdef CONFIG_OF
	dev->dma_mask = &usb11_dmamask;
	dev->coherent_dma_mask = usb11_dmamask;
#endif
	orig_dma_mask = dev->dma_mask;

#endif

#ifdef CONFIG_OF
	node = of_find_compatible_node(NULL, NULL, "mediatek,icusb");

	if (node == NULL) {
		pr_info("USB get node failed\n");
		return -1;
	}

	irq = irq_of_parse_and_map(node, 0);

	base = of_iomap(node, 0);
	pbase = of_iomap(node, 1);
#else
	irq = MT_USB1_IRQ_ID;
	base = (unsigned char __iomem *)USB11_BASE;
	pbase = (unsigned char __iomem *)USB11_PHY_ADDR;
#endif
	/*
	 * after init the controller,
	 * the USB should already be powered on!
	 */
	status = musbfsh_init_controller(dev, irq, base, pbase);
	if (status < 0)
		ERR("%s failed with status %d\n", __func__, status);
	INFO("%s--\r\n", __func__);
#if 0				/* IC_USB from SS5 */
#ifdef IC_USB
	device_create_file(dev, &dev_attr_start);
	WARNING("IC-USB is enabled\n");
#endif
#endif
	return status;
}

static int __exit musbfsh_remove(struct platform_device *pdev)
{
	struct musbfsh *musbfsh = dev_to_musbfsh(&pdev->dev);
	void __iomem *ctrl_base = musbfsh->ctrl_base;

	INFO("%s++\r\n", __func__);
	/* this gets called on rmmod.
	 *  - Host mode: host may still be active
	 *  - Peripheral mode: peripheral is deactivated (or never-activated)
	 *  - OTG mode: both roles are deactivated (or never-activated)
	 */
#ifdef CONFIG_MUSBFSH_DEBUG_FS
	musbfsh_exit_debugfs(musbfsh);
#endif
	musbfsh_shutdown(pdev);
	if (musbfsh->board_mode == MUSBFSH_HOST)
		usb_remove_hcd(musbfsh_to_hcd(musbfsh));
	musbfsh_writeb(musbfsh->mregs, MUSBFSH_DEVCTL, 0);
	musbfsh_platform_exit(musbfsh);
	musbfsh_writeb(musbfsh->mregs, MUSBFSH_DEVCTL, 0);

	musbfsh_free(musbfsh);
	iounmap(ctrl_base);
	device_init_wakeup(&pdev->dev, 0);
#ifndef CONFIG_MUSBFSH_PIO_ONLY
	pdev->dev.dma_mask = orig_dma_mask;
#endif
	return 0;
}

#ifdef	CONFIG_PM
static void musbfsh_save_context(struct musbfsh *musbfsh)
{
	int i;
	void __iomem *musbfsh_base = musbfsh->mregs;
	void __iomem *epio;

	musbfsh->context.power = musbfsh_readb(musbfsh_base, MUSBFSH_POWER);
	musbfsh->context.intrtxe = musbfsh_readw(musbfsh_base, MUSBFSH_INTRTXE);
	musbfsh->context.intrrxe = musbfsh_readw(musbfsh_base, MUSBFSH_INTRRXE);
	musbfsh->context.intrusbe =
		musbfsh_readb(musbfsh_base, MUSBFSH_INTRUSBE);
	musbfsh->context.index = musbfsh_readb(musbfsh_base, MUSBFSH_INDEX);
	musbfsh->context.devctl = musbfsh_readb(musbfsh_base, MUSBFSH_DEVCTL);

	musbfsh->context.l1_int = musbfsh_readl(musbfsh_base, USB11_L1INTM);

	for (i = 0; i < MUSBFSH_C_NUM_EPS - 1; ++i) {
		struct musbfsh_hw_ep *hw_ep;

		hw_ep = &musbfsh->endpoints[i];
		if (!hw_ep)
			continue;

		epio = hw_ep->regs;
		if (!epio)
			continue;

		musbfsh_writeb(musbfsh_base, MUSBFSH_INDEX, i);
		musbfsh->context.index_regs[i].txmaxp =
			musbfsh_readw(epio, MUSBFSH_TXMAXP);
		musbfsh->context.index_regs[i].txcsr =
			musbfsh_readw(epio, MUSBFSH_TXCSR);
		musbfsh->context.index_regs[i].rxmaxp =
			musbfsh_readw(epio, MUSBFSH_RXMAXP);
		musbfsh->context.index_regs[i].rxcsr =
			musbfsh_readw(epio, MUSBFSH_RXCSR);

		if (musbfsh->dyn_fifo) {
			musbfsh->context.index_regs[i].txfifoadd =
				musbfsh_read_txfifoadd(musbfsh_base);
			musbfsh->context.index_regs[i].rxfifoadd =
				musbfsh_read_rxfifoadd(musbfsh_base);
			musbfsh->context.index_regs[i].txfifosz =
				musbfsh_read_txfifosz(musbfsh_base);
			musbfsh->context.index_regs[i].rxfifosz =
				musbfsh_read_rxfifosz(musbfsh_base);
		}
	}
}

static void musbfsh_write_dynfifo_size(struct musbfsh *musbfsh, int i)
{
	void __iomem *musbfsh_base = musbfsh->mregs;

	musbfsh_write_txfifosz(musbfsh_base,
			       musbfsh->context.index_regs[i].txfifosz);
	musbfsh_write_rxfifosz(musbfsh_base,
			       musbfsh->context.index_regs[i].rxfifosz);
	musbfsh_write_txfifoadd(musbfsh_base,
				musbfsh->context.index_regs[i].txfifoadd);
	musbfsh_write_rxfifoadd(musbfsh_base,
				musbfsh->context.index_regs[i].rxfifoadd);
}

static void musbfsh_restore_context(struct musbfsh *musbfsh)
{
	int i;
	void __iomem *musbfsh_base = musbfsh->mregs;
	void __iomem *epio;

	musbfsh_writeb(musbfsh_base, MUSBFSH_POWER, musbfsh->context.power);
	musbfsh_writew(musbfsh_base, MUSBFSH_INTRTXE, musbfsh->context.intrtxe);
	musbfsh_writew(musbfsh_base, MUSBFSH_INTRRXE, musbfsh->context.intrrxe);
	musbfsh_writeb(musbfsh_base, MUSBFSH_INTRUSBE,
		       musbfsh->context.intrusbe);
	musbfsh_writeb(musbfsh_base, MUSBFSH_DEVCTL, musbfsh->context.devctl);

	for (i = 0; i < MUSBFSH_C_NUM_EPS - 1; ++i) {
		struct musbfsh_hw_ep *hw_ep;

		hw_ep = &musbfsh->endpoints[i];
		if (!hw_ep)
			continue;

		epio = hw_ep->regs;
		if (!epio)
			continue;

		musbfsh_writeb(musbfsh_base, MUSBFSH_INDEX, i);
		musbfsh_writew(epio, MUSBFSH_TXMAXP,
			       musbfsh->context.index_regs[i].txmaxp);
		musbfsh_writew(epio, MUSBFSH_TXCSR,
			       musbfsh->context.index_regs[i].txcsr);
		musbfsh_writew(epio, MUSBFSH_RXMAXP,
			       musbfsh->context.index_regs[i].rxmaxp);
		musbfsh_writew(epio, MUSBFSH_RXCSR,
			       musbfsh->context.index_regs[i].rxcsr);

		if (musbfsh->dyn_fifo)
			musbfsh_write_dynfifo_size(musbfsh, i);
	}

	musbfsh_writeb(musbfsh_base, MUSBFSH_INDEX, musbfsh->context.index);
	mb();	/* need to sync registers for later DMA transfer */
	/*
	 * Enable all interrupts at DMA
	 * Caution: The DMA Reg type is WRITE to SET or CLEAR
	 */
	musbfsh_writel(musbfsh->mregs, MUSBFSH_HSDMA_INTR,
		       0xFF | (0xFF << MUSBFSH_DMA_INTR_UNMASK_SET_OFFSET));
	/* mb(); */
	musbfsh_writel(musbfsh_base, USB11_L1INTM, musbfsh->context.l1_int);
}

static int musbfsh_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	/* unsigned long flags; */
	struct musbfsh *musbfsh = dev_to_musbfsh(&pdev->dev);

	MYDBG("");
	/* MYDBG("cpuid:%d\n", smp_processor_id()); */

#ifdef CONFIG_MTK_ICUSB_SUPPORT
	if (!usb11_enabled) {
		MYDBG("usb11 is not enabled");
		return 0;
	}
#endif

	/* spin_lock_irqsave(&musbfsh->lock, flags); */
#ifndef CONFIG_MTK_ICUSB_SUPPORT
	musbfsh_set_vbus(musbfsh, 0);	/* disable VBUS */
#endif

#ifdef CONFIG_SMP
	/*
	 * wx, clearn IRQ before closing clock.
	 * Prevent SMP issue: clock is disabled on CPU1,
	 * but ISR is running on CPU0 and failed to clear interrupt
	 */
	disable_irq(musbfsh->nIrq);
	musbfsh_save_context(musbfsh);
	musbfsh_read_clear_generic_interrupt(musbfsh);
	/* musbfsh_read_clear_generic_interrupt_tmp(musbfsh); */
#endif

#ifdef CONFIG_MTK_ICUSB_SUPPORT
	/* mt65xx_usb11_mac_phy_dump(); */
	mt65xx_usb11_phy_savecurrent();
	mt65xx_usb11_disable_clk_pll_pw();
	suspend_cnt++;
#else
	musbfsh_set_power(musbfsh, 0);
#endif
	/* spin_unlock_irqrestore(&musbfsh->lock, flags); */
	return 0;
}

static int musbfsh_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	/* unsigned long flags; */
	struct musbfsh *musbfsh = dev_to_musbfsh(&pdev->dev);

	MYDBG("");

#ifdef CONFIG_MTK_ICUSB_SUPPORT
	MYDBG("suspend_cnt : %d, resume_cnt :%d\n", suspend_cnt, resume_cnt);
	if (!usb11_enabled) {
		MYDBG("usb11 is not enabled");
		return 0;
	}
#endif
	/* spin_lock_irqsave(&musbfsh->lock, flags); */
#ifdef CONFIG_MTK_ICUSB_SUPPORT
	resume_cnt++;
	mt65xx_usb11_enable_clk_pll_pw();
	mt65xx_usb11_phy_recover();
	/* mt65xx_usb11_mac_phy_dump(); */
#else
	musbfsh_set_vbus(musbfsh, 1);	/* enable VBUS */
	musbfsh_set_power(musbfsh, 1);
#endif

#ifdef CONFIG_SMP
	musbfsh_restore_context(musbfsh);
	enable_irq(musbfsh->nIrq);
#endif
	/* spin_unlock_irqrestore(&musbfsh->lock, flags); */
	return 0;
}

static const struct dev_pm_ops musbfsh_dev_pm_ops = {
	.suspend = musbfsh_suspend,
	.resume = musbfsh_resume,
};

#define MUSBFSH_DEV_PM_OPS (&musbfsh_dev_pm_ops)
#else
#define	MUSBFSH_DEV_PM_OPS	NULL
#endif

static struct platform_driver musbfsh_driver = {
	.driver = {
		   .name = "musbfsh",
		   .bus = &platform_bus_type,
		   .owner = THIS_MODULE,
		   .pm = MUSBFSH_DEV_PM_OPS,
		   },
	.probe = musbfsh_probe,
	.remove = __exit_p(musbfsh_remove),
	.shutdown = musbfsh_shutdown,
};

static struct musbfsh_hdrc_config musbfsh_config_mt65xx = {
	.multipoint = false,
	.dyn_fifo = true,
	.soft_con = true,
	.dma = true,
	.num_eps = 6,
	.dma_channels = 4,
};

static struct musbfsh_hdrc_platform_data usb_data_mt65xx = {
	.mode = 1,
	.config = &musbfsh_config_mt65xx,
};


static struct platform_device mt_usb11_dev = {
	.name = "musbfsh",
	.id = -1,
	.dev = {
		.platform_data = &usb_data_mt65xx,
		.dma_mask = &usb11_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.release = musbfsh_hcd_release,
		},
};

static int __init musbfsh_dts_probe(struct platform_device *pdev)
{
	int retval;

	musb_clk11 = devm_clk_get(&pdev->dev, "infra_icusb");
	musb_clk30 = devm_clk_get(&pdev->dev, "sssub_ref_clk");

	if (IS_ERR(musb_clk11)) {
		dev_err(&pdev->dev, "USB11 cannot get musb clock\n");
	} else {
		dev_err(&pdev->dev, "USB11 get musb clock ok, prepare it\n");
		if (clk_prepare(musb_clk11) == 0)
			dev_err(&pdev->dev, "musb11 clock prepare done\n");
		else
			dev_err(&pdev->dev, "musb11 clock prepare fail\n");
	}

	if (IS_ERR(musb_clk30)) {
		dev_err(&pdev->dev, "USB30 cannot get musb clock\n");
	} else {
		dev_err(&pdev->dev, "USB30 get musb clock ok, prepare it\n");

		if (clk_prepare(musb_clk30) == 0)
			dev_err(&pdev->dev, "musb30 clock prepare done\n");
		else
			dev_err(&pdev->dev, "musb30 clock prepare fail\n");
	}

	retval = platform_device_register(&mt_usb11_dev);
	if (retval != 0)
		ERR("%s: failed with status %d\n", __func__, retval);
	return retval;
}

static int __init musbfsh_init(void)
{
	int retval;

	if (usb_disabled())	/* based on the config variable. */
		return 0;
	WARNING("MUSBFSH is enabled\n");
	wake_lock_init(&musbfsh_suspend_lock, WAKE_LOCK_SUSPEND, "usb11 host");

	retval = platform_driver_register(&musbfsh_driver);
	if (retval != 0) {
		ERR("musbfsh_init with status %d\n", retval);
		return retval;
	}

	retval = platform_driver_register(&musbfsh_dts_driver);
	if (retval != 0) {
		ERR("musbfsh_init with status %d\n", retval);
		platform_driver_unregister(&musbfsh_driver);
		return retval;
	}
	return 0;

}

/*
 * make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
module_init(musbfsh_init);

static void __exit musbfsh_cleanup(void)
{
	wake_lock_destroy(&musbfsh_suspend_lock);
	platform_driver_unregister(&musbfsh_driver);
	platform_driver_unregister(&musbfsh_dts_driver);
}
module_exit(musbfsh_cleanup);

int musbfsh_debug = 0;
module_param(musbfsh_debug, int, 0644);
#ifdef MTK_DT_USB_SUPPORT
module_param(musbfsh_eint_cnt, int, 0644);
module_param(musbfsh_skip_phy, int, 0644);
module_param(musbfsh_skip_clk, int, 0644);
module_param(musbfsh_wake_lock_hold, int, 0644);
module_param(musbfsh_skip_port_suspend, int, 0644);
module_param(musbfsh_skip_port_resume, int, 0644);
#endif

#else

static void usb11_phy_savecurrent(unsigned char __iomem *pbase)
{
	/*
	 * 4 1. swtich to USB function.
	 * (system register, force ip into usb mode.)
	 */
	USB11PHY_CLR8(pbase + 0x96b, 0x04);
	USB11PHY_CLR8(pbase + 0x96e, 0x01);
	USB11PHY_CLR8(pbase + 0x921, 0x2);
	USB11PHY_CLR8(pbase + 0x921, 0x1);

	/* RG_SUSPENDM=1; */
	USB11PHY_SET8(pbase + 0x968, 0x08);
	/* 4 2. release force suspendm. */
	USB11PHY_SET8(pbase + 0x96a, 0x04);
	/* 4 3. RG_DPPULLDOWN./RG_DMPULLDOWN. */
	USB11PHY_SET8(pbase + 0x968, 0xc0);
	/* 4 4. RG_XCVRSEL[1:0] =2'b01. */
	USB11PHY_CLR8(pbase + 0x968, 0x30);
	USB11PHY_SET8(pbase + 0x968, 0x10);
	/* 4 5. RG_TERMSEL = 1'b1 */
	USB11PHY_SET8(pbase + 0x968, 0x04);
	/* 4 6. RG_DATAIN[3:0]=4'b0000 */
	USB11PHY_CLR8(pbase + 0x969, 0x3c);
	/*
	 * 4 7. force_dp_pulldown, force_dm_pulldown,
	 *	force_xcversel, force_termsel.
	 */
	USB11PHY_SET8(pbase + 0x96a, 0xba);

	/* 4 8.RG_USB20_BC11_SW_EN 1'b0 */
	USB11PHY_CLR8(pbase + 0x91a, 0x80);
	/* 4 9.RG_USB20_OTG_VBUSSCMP_EN 1'b0 */
	USB11PHY_CLR8(pbase + 0x91a, 0x10);
	/* 4 10. delay 800us. */
	udelay(800);
	/* 4 11. rg_usb20_pll_stable = 1 */
	USB11PHY_SET8(pbase + 0x963, 0x02);

	udelay(1);

	USB11PHY_CLR8(pbase + 0x96C, 0x2C);
	USB11PHY_SET8(pbase + 0x96C, 0x10);
	USB11PHY_CLR8(pbase + 0x96D, 0x3C);

	/* RG_SUSPENDM=0; */
	USB11PHY_CLR8(pbase + 0x968, 0x08);

	/* 4 13.  wait 1us */
	udelay(1);
}


#if 0
static int __init musbfsh_dts_probe(struct platform_device *pdev)
{
	unsigned char __iomem *pbase;

#ifdef CONFIG_OF
	if (pdev->dev.of_node == NULL) {
		pr_info("USB get node failed\n");
		return -1;
	}
	pbase = of_iomap(pdev->dev.of_node, 1);
	musb_clk11 = devm_clk_get(&pdev->dev, "USB11-CLOCK");
	if (IS_ERR(musb_clk11)) {
		dev_err(&pdev->dev, "USB11 cannot get musb clock\n");
	} else {
		dev_err(&pdev->dev, "USB11 get musb clock ok, prepare it\n");
		if (clk_prepare(musb_clk11) == 0)
			dev_err(&pdev->dev, "musb11 clock prepare done\n");
		else
			dev_err(&pdev->dev, "musb11 clock prepare fail\n");
	}
#else
	pbase = (unsigned char __iomem *)USB11_PHY_ADDR;
#endif
/* enable_clock (MT_CG_INFRA_USB, "USB11"); */
/* enable_clock (MT_CG_INFRA_ICUSB, "USB11"); */
/* enable_clock (MT_CG_INFRA_USB_MCU, "USB11"); */
	clk_enable(musb_clk11);
	usb11_phy_savecurrent(pbase);
/* disable_clock (MT_CG_INFRA_USB_MCU, "USB11"); */
/* disable_clock (MT_CG_INFRA_ICUSB, "USB11"); */
/* disable_clock (MT_CG_INFRA_USB, "USB11"); */
	clk_disable(musb_clk11);
	return 0;
}
#endif
/*-------------------------------------------------------------------------*/

static int __init musbfsh_init(void)
{
	pr_info("MUSBFSH DUMMY Driver is enabled\n");
	return platform_driver_register(&musbfsh_dts_driver);
}

/* make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
module_init(musbfsh_init);

static void __exit musbfsh_cleanup(void)
{
	platform_driver_unregister(&musbfsh_dts_driver);
}
module_exit(musbfsh_cleanup);

#endif
