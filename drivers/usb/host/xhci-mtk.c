/*
 * MediaTek xHCI Host Controller Driver
 *
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>

#include "xhci.h"
#include "xhci-mtk.h"

#ifdef CONFIG_MTK_UAC_POWER_SAVING
#include "mtk-phy.h"
#endif

/* ip_pw_ctrl0 register */
#define CTRL0_IP_SW_RST	BIT(0)

/* ip_pw_ctrl1 register */
#define CTRL1_IP_HOST_PDN	BIT(0)

/* ip_pw_ctrl2 register */
#define CTRL2_IP_DEV_PDN	BIT(0)

/* ip_pw_sts1 register */
#define STS1_IP_SLEEP_STS	BIT(30)
#define STS1_XHCI_RST		BIT(11)
#define STS1_SYS125_RST	BIT(10)
#define STS1_REF_RST		BIT(8)
#define STS1_SYSPLL_STABLE	BIT(0)

/* ip_xhci_cap register */
#define CAP_U3_PORT_NUM(p)	((p) & 0xff)
#define CAP_U2_PORT_NUM(p)	(((p) >> 8) & 0xff)

/* u3_ctrl_p register */
#define CTRL_U3_PORT_HOST_SEL	BIT(2)
#define CTRL_U3_PORT_PDN	BIT(1)
#define CTRL_U3_PORT_DIS	BIT(0)

/* u2_ctrl_p register */
#define CTRL_U2_PORT_HOST_SEL	BIT(2)
#define CTRL_U2_PORT_PDN	BIT(1)
#define CTRL_U2_PORT_DIS	BIT(0)

/* u2_phy_pll register */
#define CTRL_U2_FORCE_PLL_STB	BIT(28)

#define PERI_WK_CTRL0		0x400
#define UWK_CTR0_0P_LS_PE	BIT(8)  /* posedge */
#define UWK_CTR0_0P_LS_NE	BIT(7)  /* negedge for 0p linestate*/
#define UWK_CTL1_1P_LS_C(x)	(((x) & 0xf) << 1)
#define UWK_CTL1_1P_LS_E	BIT(0)

#define PERI_WK_CTRL1		0x404
#define UWK_CTL1_IS_C(x)	(((x) & 0xf) << 26)
#define UWK_CTL1_IS_E		BIT(25)
#define UWK_CTL1_0P_LS_C(x)	(((x) & 0xf) << 21)
#define UWK_CTL1_0P_LS_E	BIT(20)
#define UWK_CTL1_IDDIG_C(x)	(((x) & 0xf) << 11)  /* cycle debounce */
#define UWK_CTL1_IDDIG_E	BIT(10) /* enable debounce */
#define UWK_CTL1_IDDIG_P	BIT(9)  /* polarity */
#define UWK_CTL1_0P_LS_P	BIT(7)
#define UWK_CTL1_IS_P		BIT(6)  /* polarity for ip sleep */

enum ssusb_wakeup_src {
	SSUSB_WK_IP_SLEEP = 1,
	SSUSB_WK_LINE_STATE = 2,
};

static int xhci_mtk_host_enable(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value, check_val;
	int ret;
	int i;

	if (ippc == NULL)
		return 0;

	/* power on host ip */
	value = readl(&ippc->ip_pw_ctr1);
	value &= ~CTRL1_IP_HOST_PDN;
	writel(value, &ippc->ip_pw_ctr1);

	/* power on and enable all u3 ports */
	for (i = 0; i < mtk->num_u3_ports; i++) {
		value = readl(&ippc->u3_ctrl_p[i]);
		value &= ~(CTRL_U3_PORT_PDN | CTRL_U3_PORT_DIS);
		value |= CTRL_U3_PORT_HOST_SEL;
		writel(value, &ippc->u3_ctrl_p[i]);
	}

	/* power on and enable all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		value = readl(&ippc->u2_ctrl_p[i]);
		value &= ~(CTRL_U2_PORT_PDN | CTRL_U2_PORT_DIS);
		value |= CTRL_U2_PORT_HOST_SEL;
		writel(value, &ippc->u2_ctrl_p[i]);
	}

	/*
	 * wait for clocks to be stable, and clock domains reset to
	 * be inactive after power on and enable ports
	 */
	check_val = STS1_SYSPLL_STABLE | STS1_REF_RST |
			STS1_SYS125_RST | STS1_XHCI_RST;

	ret = readl_poll_timeout(&ippc->ip_pw_sts1, value,
				(check_val == (value & check_val)), 100, 20000);
	if (ret) {
		dev_err(mtk->dev, "clocks are not stable (0x%x)\n", value);
		return ret;
	}
	return 0;
}

static int xhci_mtk_host_disable(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value;
	int ret;
	int i;

	/* power down all u3 ports */
	for (i = 0; i < mtk->num_u3_ports; i++) {
		value = readl(&ippc->u3_ctrl_p[i]);
		value |= CTRL_U3_PORT_PDN;
		writel(value, &ippc->u3_ctrl_p[i]);
	}

	/* power down all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		value = readl(&ippc->u2_ctrl_p[i]);
		value |= CTRL_U2_PORT_PDN;
		writel(value, &ippc->u2_ctrl_p[i]);
	}

	/* power down host ip */
	value = readl(&ippc->ip_pw_ctr1);
	value |= CTRL1_IP_HOST_PDN;
	writel(value, &ippc->ip_pw_ctr1);

	/* wait for host ip to sleep */
	ret = readl_poll_timeout(&ippc->ip_pw_sts1, value,
				(value & STS1_IP_SLEEP_STS), 100, 100000);
	if (ret) {
		dev_err(mtk->dev, "ip sleep failed!!!\n");
		return ret;
	}

	return 0;
}


static int xhci_mtk_host_power_down(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value;
	int i;
	struct device_node *of_node = mtk->dev->of_node;

	if (ippc == NULL)
		return 0;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		/* power down all u3 ports */
		for (i = 0; i < mtk->num_u3_ports; i++) {
			value = readl(&ippc->u3_ctrl_p[i]);
			value |= (CTRL_U3_PORT_PDN | CTRL_U3_PORT_DIS);
			writel(value, &ippc->u3_ctrl_p[i]);
		}

		/* power down all u2 ports */
		for (i = 0; i < mtk->num_u2_ports; i++) {
			value = readl(&ippc->u2_ctrl_p[i]);
			value |= (CTRL_U2_PORT_PDN | CTRL_U2_PORT_DIS);
			writel(value, &ippc->u2_ctrl_p[i]);
		}
	} else {
		;	/* TODO LIST */
	}
	return 0;
}

static int xhci_mtk_ssusb_config(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value;

	/* reset whole ip */
	value = readl(&ippc->ip_pw_ctr0);
	value |= CTRL0_IP_SW_RST;
	writel(value, &ippc->ip_pw_ctr0);
	udelay(1);
	value = readl(&ippc->ip_pw_ctr0);
	value &= ~CTRL0_IP_SW_RST;
	writel(value, &ippc->ip_pw_ctr0);

	/*
	 * device ip is default power-on in fact
	 * power down device ip, otherwise ip-sleep will fail
	 */
	value = readl(&ippc->ip_pw_ctr2);
	value |= CTRL2_IP_DEV_PDN;
	writel(value, &ippc->ip_pw_ctr2);

	value = readl(&ippc->ip_xhci_cap);
	mtk->num_u3_ports = CAP_U3_PORT_NUM(value);
	mtk->num_u2_ports = CAP_U2_PORT_NUM(value);
	dev_dbg(mtk->dev, "%s u2p:%d, u3p:%d\n", __func__,
			mtk->num_u2_ports, mtk->num_u3_ports);

	return xhci_mtk_host_enable(mtk);
}

#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
static void xhci_mtk_ssusb_ip_sleep(struct xhci_hcd_mtk *mtk)
{
	struct device_node *of_node = mtk->dev->of_node;
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		/* Set below sequence to avoid power leakage */
		xhci_mtk_host_power_down(mtk);
		udelay(50);
		/* reset whole ip */
		value = readl(&ippc->ip_pw_ctr0);
		value |= CTRL0_IP_SW_RST;
		writel(value, &ippc->ip_pw_ctr0);
	}
}
#endif

static int xhci_mtk_clks_enable(struct xhci_hcd_mtk *mtk)
{
	int ret;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		ret = clk_prepare_enable(mtk->sys_clk);
		if (ret) {
			dev_err(mtk->dev, "failed to enable sys_clk\n");
			goto sys_clk_err;
		}

		if (mtk->wakeup_src) {
			ret = clk_prepare_enable(mtk->wk_deb_p0);
			if (ret) {
				dev_err(mtk->dev, "failed to enable wk_deb_p0\n");
				goto usb_p0_err;
			}

			ret = clk_prepare_enable(mtk->wk_deb_p1);
			if (ret) {
				dev_err(mtk->dev, "failed to enable wk_deb_p1\n");
				goto usb_p1_err;
			}
		}
	}
	return 0;

usb_p1_err:
	clk_disable_unprepare(mtk->wk_deb_p0);
usb_p0_err:
	clk_disable_unprepare(mtk->sys_clk);
sys_clk_err:
	return -EINVAL;
}

static void xhci_mtk_clks_disable(struct xhci_hcd_mtk *mtk)
{
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		if (mtk->wakeup_src) {
			clk_disable_unprepare(mtk->wk_deb_p1);
			clk_disable_unprepare(mtk->wk_deb_p0);
		}
		clk_disable_unprepare(mtk->sys_clk);
	}
}

/* only clocks can be turn off for ip-sleep wakeup mode */
static void usb_wakeup_ip_sleep_en(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct regmap *pericfg = mtk->pericfg;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
		tmp &= ~UWK_CTL1_IS_P;
		tmp &= ~(UWK_CTL1_IS_C(0xf));
		tmp |= UWK_CTL1_IS_C(0x8);
		regmap_write(pericfg, PERI_WK_CTRL1, tmp);
		regmap_write(pericfg, PERI_WK_CTRL1, tmp | UWK_CTL1_IS_E);

		regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
		dev_dbg(mtk->dev, "%s(): WK_CTRL1[P6,E25,C26:29]=%#x\n",
			__func__, tmp);
	}
}

static void usb_wakeup_ip_sleep_dis(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		regmap_read(mtk->pericfg, PERI_WK_CTRL1, &tmp);
		tmp &= ~UWK_CTL1_IS_E;
		regmap_write(mtk->pericfg, PERI_WK_CTRL1, tmp);
	}
}

/*
* for line-state wakeup mode, phy's power should not power-down
* and only support cable plug in/out
*/
static void usb_wakeup_line_state_en(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct regmap *pericfg = mtk->pericfg;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		/* line-state of u2-port0 */
		regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
		tmp &= ~UWK_CTL1_0P_LS_P;
		tmp &= ~(UWK_CTL1_0P_LS_C(0xf));
		tmp |= UWK_CTL1_0P_LS_C(0x8);
		regmap_write(pericfg, PERI_WK_CTRL1, tmp);
		regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
		regmap_write(pericfg, PERI_WK_CTRL1, tmp | UWK_CTL1_0P_LS_E);

		/* line-state of u2-port1 */
		regmap_read(pericfg, PERI_WK_CTRL0, &tmp);
		tmp &= ~(UWK_CTL1_1P_LS_C(0xf));
		tmp |= UWK_CTL1_1P_LS_C(0x8);
		regmap_write(pericfg, PERI_WK_CTRL0, tmp);
		regmap_write(pericfg, PERI_WK_CTRL0, tmp | UWK_CTL1_1P_LS_E);
	}
}

static void usb_wakeup_line_state_dis(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct regmap *pericfg = mtk->pericfg;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		/* line-state of u2-port0 */
		regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
		tmp &= ~UWK_CTL1_0P_LS_E;
		regmap_write(pericfg, PERI_WK_CTRL1, tmp);

		/* line-state of u2-port1 */
		regmap_read(pericfg, PERI_WK_CTRL0, &tmp);
		tmp &= ~UWK_CTL1_1P_LS_E;
		regmap_write(pericfg, PERI_WK_CTRL0, tmp);
	}
}

static void usb_wakeup_enable(struct xhci_hcd_mtk *mtk)
{
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		enable_ipsleep_wakeup();	/* TODO THIS */
	} else {
		if (mtk->wakeup_src == SSUSB_WK_IP_SLEEP)
			usb_wakeup_ip_sleep_en(mtk);
		else if (mtk->wakeup_src == SSUSB_WK_LINE_STATE)
			usb_wakeup_line_state_en(mtk);
	}
}

static void usb_wakeup_disable(struct xhci_hcd_mtk *mtk)
{
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		if (mtk->wakeup_src == SSUSB_WK_IP_SLEEP)
			usb_wakeup_ip_sleep_dis(mtk);
		else if (mtk->wakeup_src == SSUSB_WK_LINE_STATE)
			usb_wakeup_line_state_dis(mtk);
	}
}

static int usb_wakeup_of_property_parse(struct xhci_hcd_mtk *mtk,
				struct device_node *dn)
{
	struct device *dev = mtk->dev;
	struct device_node *of_node = dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		/*
		* wakeup function is optional, so it is not an error if this property
		* does not exist, and in such case, no need to get relative
		* properties anymore.
		*/
		of_property_read_u32(dn, "mediatek,wakeup-src", &mtk->wakeup_src);
		if (!mtk->wakeup_src)
			return 0;

		mtk->wk_deb_p0 = devm_clk_get(dev, "wakeup_deb_p0");
		if (IS_ERR(mtk->wk_deb_p0)) {
			dev_err(dev, "fail to get wakeup_deb_p0\n");
			return PTR_ERR(mtk->wk_deb_p0);
		}

		mtk->wk_deb_p1 = devm_clk_get(dev, "wakeup_deb_p1");
		if (IS_ERR(mtk->wk_deb_p1)) {
			dev_err(dev, "fail to get wakeup_deb_p1\n");
			return PTR_ERR(mtk->wk_deb_p1);
		}

		mtk->pericfg = syscon_regmap_lookup_by_phandle(dn,
							"mediatek,syscon-wakeup");
		if (IS_ERR(mtk->pericfg)) {
			dev_err(dev, "fail to get pericfg regs\n");
			return PTR_ERR(mtk->pericfg);
		}
	}
	return 0;
}

static int xhci_mtk_setup(struct usb_hcd *hcd);
#ifdef CONFIG_USB_MTK_DUALMODE
static const struct xhci_driver_overrides xhci_mtk_overrides = {
	.extra_priv_size = sizeof(struct xhci_hcd),
	.reset = xhci_mtk_setup,
};
#else
static const struct xhci_driver_overrides xhci_mtk_overrides __initconst = {
	.extra_priv_size = sizeof(struct xhci_hcd),
	.reset = xhci_mtk_setup,
};
#endif

static struct hc_driver __read_mostly xhci_mtk_hc_driver;

static int xhci_mtk_phy_init(struct xhci_hcd_mtk *mtk)
{
	int i;
	int ret;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		for (i = 0; i < mtk->num_phys; i++) {
			ret = phy_init(mtk->phys[i]);
			if (ret)
				goto exit_phy;
		}
	}
	return 0;

exit_phy:
	for (; i > 0; i--)
		phy_exit(mtk->phys[i - 1]);

	return ret;
}

static int xhci_mtk_phy_exit(struct xhci_hcd_mtk *mtk)
{
	int i;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		for (i = 0; i < mtk->num_phys; i++)
			phy_exit(mtk->phys[i]);
	}

	return 0;
}

static int xhci_mtk_phy_power_on(struct xhci_hcd_mtk *mtk)
{
	int i;
	int ret;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		for (i = 0; i < mtk->num_phys; i++) {
			ret = phy_power_on(mtk->phys[i]);
			if (ret)
				goto power_off_phy;
		}
	}
	return 0;

power_off_phy:
	for (; i > 0; i--)
		phy_power_off(mtk->phys[i - 1]);

	return ret;
}

static void xhci_mtk_phy_power_off(struct xhci_hcd_mtk *mtk)
{
	unsigned int i;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		for (i = 0; i < mtk->num_phys; i++)
			phy_power_off(mtk->phys[i]);
	}
}

static int xhci_mtk_ldos_enable(struct xhci_hcd_mtk *mtk)
{
	int ret;
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		ret = regulator_enable(mtk->vbus);
		if (ret) {
			dev_err(mtk->dev, "failed to enable vbus\n");
			return ret;
		}

		ret = regulator_enable(mtk->vusb33);
		if (ret) {
			dev_err(mtk->dev, "failed to enable vusb33\n");
			regulator_disable(mtk->vbus);
			return ret;
		}
	}
	return 0;
}

static void xhci_mtk_ldos_disable(struct xhci_hcd_mtk *mtk)
{
	struct device_node *of_node = mtk->dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		regulator_disable(mtk->vbus);
		regulator_disable(mtk->vusb33);
	}
}

static int xhci_mtk_env_init(struct xhci_hcd_mtk *mtk)
{
	struct device *dev = mtk->dev;
	struct device_node *of_node = dev->of_node;

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		;	/* TODO THIS */
	} else {
		mtk->vbus = devm_regulator_get(dev, "vbus");
		if (IS_ERR(mtk->vbus)) {
			dev_err(dev, "fail to get vbus\n");
			return PTR_ERR(mtk->vbus);
		}

		mtk->vusb33 = devm_regulator_get(dev, "vusb33");
		if (IS_ERR(mtk->vusb33)) {
			dev_err(dev, "fail to get vusb33\n");
			return PTR_ERR(mtk->vusb33);
		}

		mtk->sys_clk = devm_clk_get(dev, "sys_ck");
		if (IS_ERR(mtk->sys_clk)) {
			dev_err(dev, "fail to get sys_ck\n");
			return PTR_ERR(mtk->sys_clk);
		}
	}
	return 0;
}

static void xhci_mtk_quirks(struct device *dev, struct xhci_hcd *xhci)
{
	struct usb_hcd *hcd = xhci_to_hcd(xhci);
	struct xhci_hcd_mtk *mtk = hcd_to_mtk(hcd);

	/*
	 * As of now platform drivers don't provide MSI support so we ensure
	 * here that the generic code does not try to make a pci_dev from our
	 * dev struct in order to setup MSI
	 */
	xhci->quirks |= XHCI_PLAT;
	xhci->quirks |= XHCI_MTK_HOST;
	/*
	 * MTK host controller gives a spurious successful event after a
	 * short transfer. Ignore it.
	 */
	xhci->quirks |= XHCI_SPURIOUS_SUCCESS;
	if (mtk->lpm_support)
		xhci->quirks |= XHCI_LPM_SUPPORT;
}

/* called during probe() after chip reset completes */
static int xhci_mtk_setup(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	struct xhci_hcd_mtk *mtk = hcd_to_mtk(hcd);
	struct device_node *of_node = mtk->dev->of_node;
	int ret;

	if (usb_hcd_is_primary_hcd(hcd)) {
		ret = xhci_mtk_ssusb_config(mtk);
		if (ret)
			return ret;
		ret = xhci_mtk_sch_init(mtk);
		if (ret)
			return ret;
	}

	ret = xhci_gen_setup(hcd, xhci_mtk_quirks);
	if (ret)
		return ret;

	if (usb_hcd_is_primary_hcd(hcd)) {
		mtk->num_u3_ports = xhci->num_usb3_ports;
		mtk->num_u2_ports = xhci->num_usb2_ports;
		ret = xhci_mtk_sch_init(mtk);
		if (ret)
			return ret;
	}

	/* allow runtime pm */
	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
#ifndef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
		pm_runtime_put_noidle(mtk->dev);
#endif
	}

	return ret;
}

static int xhci_mtk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct xhci_hcd_mtk *mtk;
	const struct hc_driver *driver;
	struct xhci_hcd *xhci;
	struct resource *res;
	struct usb_hcd *hcd;
	struct phy *phy;
	int phy_num;
	int ret = -ENODEV;
	int irq;

	if (usb_disabled())
		return -ENODEV;

	driver = &xhci_mtk_hc_driver;
	mtk = devm_kzalloc(dev, sizeof(*mtk), GFP_KERNEL);
	if (!mtk)
		return -ENOMEM;

	mtk->dev = dev;
#if 0
	mtk->vbus = devm_regulator_get(dev, "vbus");
	if (IS_ERR(mtk->vbus)) {
		dev_err(dev, "fail to get vbus\n");
		return PTR_ERR(mtk->vbus);
	}

	mtk->vusb33 = devm_regulator_get(dev, "vusb33");
	if (IS_ERR(mtk->vusb33)) {
		dev_err(dev, "fail to get vusb33\n");
		return PTR_ERR(mtk->vusb33);
	}

	mtk->sys_clk = devm_clk_get(dev, "sys_ck");
	if (IS_ERR(mtk->sys_clk)) {
		dev_err(dev, "fail to get sys_ck\n");
		return PTR_ERR(mtk->sys_clk);
	}
#else
	ret = xhci_mtk_env_init(mtk);
	if (ret)
		return ret;
#endif
	mtk->lpm_support = of_property_read_bool(node, "usb3-lpm-capable");

	ret = usb_wakeup_of_property_parse(mtk, node);
	if (ret)
		return ret;

	mtk->num_phys = of_count_phandle_with_args(node,
			"phys", "#phy-cells");
	if (mtk->num_phys > 0) {
		mtk->phys = devm_kcalloc(dev, mtk->num_phys,
					sizeof(*mtk->phys), GFP_KERNEL);
		if (!mtk->phys)
			return -ENOMEM;
	} else {
		mtk->num_phys = 0;
	}
	pm_runtime_enable(dev);
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	pm_runtime_get_sync(dev);
	device_disable_async_suspend(dev);
#else
	pm_runtime_get_noresume(dev);
	device_enable_async_suspend(dev);
#endif
	ret = xhci_mtk_ldos_enable(mtk);
	if (ret)
		goto disable_pm;

	ret = xhci_mtk_clks_enable(mtk);
	if (ret)
		goto disable_ldos;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		goto disable_clk;

	/* Initialize dma_mask and coherent_dma_mask to 32-bits */
	ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
	if (ret)
		goto disable_clk;

	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;
	else
		dma_set_mask(dev, DMA_BIT_MASK(32));

	hcd = usb_create_hcd(driver, dev, dev_name(dev));
	if (!hcd) {
		ret = -ENOMEM;
		goto disable_clk;
	}

	/*
	 * USB 2.0 roothub is stored in the platform_device.
	 * Swap it with mtk HCD.
	 */
	mtk->hcd = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, mtk);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hcd->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(hcd->regs)) {
		ret = PTR_ERR(hcd->regs);
		goto put_usb2_hcd;
	}
	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);

	mtk->ippc_regs = NULL;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res) {	/* ippc register is optional */
		mtk->ippc_regs = devm_ioremap_resource(dev, res);
		if (IS_ERR(mtk->ippc_regs)) {
			ret = PTR_ERR(mtk->ippc_regs);
			goto put_usb2_hcd;
		}
	}

	for (phy_num = 0; phy_num < mtk->num_phys; phy_num++) {
		phy = devm_of_phy_get_by_index(dev, node, phy_num);
		if (IS_ERR(phy)) {
			ret = PTR_ERR(phy);
			goto put_usb2_hcd;
		}
		mtk->phys[phy_num] = phy;
	}

	ret = xhci_mtk_phy_init(mtk);
	if (ret)
		goto put_usb2_hcd;

	ret = xhci_mtk_phy_power_on(mtk);
	if (ret)
		goto exit_phys;

	device_init_wakeup(dev, true);

	xhci = hcd_to_xhci(hcd);
	xhci->main_hcd = hcd;
	xhci->shared_hcd = usb_create_shared_hcd(driver, dev,
			dev_name(dev), hcd);
	if (!xhci->shared_hcd) {
		ret = -ENOMEM;
		goto power_off_phys;
	}

	if (HCC_MAX_PSA(xhci->hcc_params) >= 4)
		xhci->shared_hcd->can_do_streams = 1;

	ret = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (ret)
		goto put_usb3_hcd;

	ret = usb_add_hcd(xhci->shared_hcd, irq, IRQF_SHARED);
	if (ret)
		goto dealloc_usb2_hcd;

#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	device_set_wakeup_enable(&hcd->self.root_hub->dev, 1);
	device_set_wakeup_enable(&xhci->shared_hcd->self.root_hub->dev, 1);
#endif
	return 0;

dealloc_usb2_hcd:
	usb_remove_hcd(hcd);

put_usb3_hcd:
	xhci_mtk_sch_exit(mtk);
	usb_put_hcd(xhci->shared_hcd);

power_off_phys:
	xhci_mtk_phy_power_off(mtk);
	device_init_wakeup(dev, false);

exit_phys:
	xhci_mtk_phy_exit(mtk);

put_usb2_hcd:
	usb_put_hcd(hcd);

disable_clk:
	xhci_mtk_clks_disable(mtk);

disable_ldos:
	xhci_mtk_ldos_disable(mtk);
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	xhci_mtk_ssusb_ip_sleep(mtk);
#endif
disable_pm:
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	pm_runtime_put_sync(dev);
#else
	pm_runtime_put_noidle(dev);
#endif
	pm_runtime_disable(dev);
	return ret;
}

static int xhci_mtk_remove(struct platform_device *dev)
{
	struct xhci_hcd_mtk *mtk = platform_get_drvdata(dev);
	struct usb_hcd	*hcd = mtk->hcd;
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	xhci_mtk_host_enable(mtk);
#endif
	usb_remove_hcd(xhci->shared_hcd);
	xhci_mtk_phy_power_off(mtk);
	xhci_mtk_phy_exit(mtk);
	device_init_wakeup(&dev->dev, false);

	usb_remove_hcd(hcd);
	usb_put_hcd(xhci->shared_hcd);
	usb_put_hcd(hcd);
	xhci_mtk_sch_exit(mtk);
#ifndef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	xhci_mtk_host_power_down(mtk);
#endif
	xhci_mtk_clks_disable(mtk);
	xhci_mtk_ldos_disable(mtk);
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	pm_runtime_put_sync(&dev->dev);
#else
	pm_runtime_put_noidle(&dev->dev);
#endif
	pm_runtime_disable(&dev->dev);
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	xhci_mtk_ssusb_ip_sleep(mtk);
#endif
	return 0;
}

/* USB Audio Power Saving */
#ifdef CONFIG_MTK_UAC_POWER_SAVING
#define MIN_SLEEP_MS 10

static struct xhci_mtk_sram_block xhci_sram[XHCI_SRAM_BLOCK_NUM] = {
	[XHCI_EVENTRING] = {0, NULL, TRB_SEGMENT_SIZE, STATE_UNINIT},
	[XHCI_EPTX] = {0, NULL, TRB_SEGMENT_SIZE, STATE_UNINIT},
	[XHCI_EPRX] = {0, NULL, TRB_SEGMENT_SIZE, STATE_UNINIT},
	/* add 56 bytes for alignment */
	[XHCI_DCBAA] = {0, NULL, 8 * MAX_HC_SLOTS + 8 + 56, STATE_UNINIT},
	/* add 48 bytes for alignment */
	[XHCI_ERST] = {0, NULL, 16 * ERST_NUM_SEGS + 48, STATE_UNINIT}
};

static struct xhci_mtk_sram_block usb_audio_sram[USB_AUDIO_DATA_BLOCK_NUM] = {
	[USB_AUDIO_DATA_OUT_EP] = {0, NULL, 0, STATE_UNINIT},
	[USB_AUDIO_DATA_IN_EP] = {0, NULL, 0, STATE_UNINIT},
	[USB_AUDIO_DATA_SYNC_EP] = {0, NULL, 0, STATE_UNINIT}
};

static atomic_t power_status = ATOMIC_INIT(USB_DPIDLE_FORBIDDEN);

static int xhci_mtk_set_power_mode(int mode)
{
	/* TODO */
	if (atomic_read(&power_status) != mode) {
		usb_hal_dpidle_request(mode);
		atomic_set(&power_status, mode);
	}
	return 0;
}

static void xhci_mtk_wakeup_timer_func(unsigned long data)
{
	xhci_mtk_set_power_mode(USB_DPIDLE_FORBIDDEN);
}

static DEFINE_TIMER(xhci_wakeup_timer, xhci_mtk_wakeup_timer_func,
					0, 0);

void xhci_mtk_allow_sleep(unsigned int sleep_ms)
{
	int i;
	bool data_sram = false;

	/* step1: check if have enough sleep time */
	if (unlikely(sleep_ms <= MIN_SLEEP_MS))
		goto not_sleep;

	/* step2: check if xhci state is normal */
	for (i = 0; i < XHCI_SRAM_BLOCK_NUM; i++)
		if (unlikely(xhci_sram[i].state == STATE_NOMEM))
			goto not_sleep;

	/* setp3: check if usb audio data on sram */
	for (i = 0; i < USB_AUDIO_DATA_BLOCK_NUM; i++) {
		if (unlikely(usb_audio_sram[i].state == STATE_NOMEM))
			goto not_sleep;
		else if (usb_audio_sram[i].state == STATE_USE)
			data_sram = true;
	}

	if (likely(data_sram)) {
		static DEFINE_RATELIMIT_STATE(ratelimit, 2 * HZ, 1);

		if (__ratelimit(&ratelimit))
			pr_info("mtk_xhci_allow_sleep (%d) ms\n", sleep_ms);
		mod_timer(&xhci_wakeup_timer,
				  jiffies + msecs_to_jiffies(sleep_ms));
		xhci_mtk_set_power_mode(USB_DPIDLE_SRAM);
	}
	return;

not_sleep:
	xhci_mtk_set_power_mode(USB_DPIDLE_FORBIDDEN);
}

int xhci_mtk_init_sram(struct xhci_hcd *xhci)
{
	int i;
	int offset = 0;
	unsigned int xhci_sram_size = 0;

	/* init xhci sram */
	for (i = 0; i < XHCI_SRAM_BLOCK_NUM; i++)
		xhci_sram_size += xhci_sram[i].mlength;

	pr_info("%s size=%d\n", __func__, xhci_sram_size);

	if (mtk_audio_request_sram(&xhci->msram_phys_addr,
			(unsigned char **) &xhci->msram_virt_addr,
							   xhci_sram_size, &xhci_sram)) {

		for (i = 0; i < XHCI_SRAM_BLOCK_NUM; i++)
			xhci_sram[i].state = STATE_NOMEM;

		pr_err("mtk_audio_request_sram fail\n");
		return -ENOMEM;
	}

	for (i = 0; i < XHCI_SRAM_BLOCK_NUM; i++) {
		xhci_sram[i].msram_phys_addr =
			xhci->msram_phys_addr + offset;
		xhci_sram[i].msram_virt_addr =
			(void *)((char *)xhci->msram_virt_addr + offset);
		offset += xhci_sram[i].mlength;
		memset_io(xhci_sram[i].msram_virt_addr,
				  0, xhci_sram[i].mlength);
		xhci_sram[i].state = STATE_INIT;

		pr_debug("[%d] p :%llx, v=%p, len=%d\n",
				 i, xhci_sram[i].msram_phys_addr,
				 xhci_sram[i].msram_virt_addr,
				 xhci_sram[i].mlength);
	}
	return 0;
}

int xhci_mtk_deinit_sram(struct xhci_hcd *xhci)
{
	int i;

	if (xhci->msram_virt_addr)
		mtk_audio_free_sram(&xhci_sram);

	for (i = 0; i < XHCI_SRAM_BLOCK_NUM; i++) {
		xhci_sram[i].msram_phys_addr = 0;
		xhci_sram[i].msram_virt_addr = NULL;
		xhci_sram[i].state = STATE_UNINIT;
	}
	xhci->msram_virt_addr = NULL;

	pr_info("%s\n", __func__);
	return 0;
}

int xhci_mtk_allocate_sram(int id, dma_addr_t *sram_phys_addr,
						   unsigned char **msram_virt_addr)
{
	if (xhci_sram[id].state == STATE_NOMEM)
		return -ENOMEM;

	if (xhci_sram[id].state == STATE_USE) {
		/* use the same sram block, set state to nomem*/
		xhci_sram[id].state = STATE_NOMEM;
		return -ENOMEM;
	}

	*sram_phys_addr = xhci_sram[id].msram_phys_addr;
	*msram_virt_addr =
		(unsigned char *)xhci_sram[id].msram_virt_addr;

	memset_io(xhci_sram[id].msram_virt_addr,
			  0, xhci_sram[id].mlength);

	xhci_sram[id].state = STATE_USE;

	pr_info("%s get [%d] p :%llx, v=%p, len=%d\n",
			__func__, id, xhci_sram[id].msram_phys_addr,
			xhci_sram[id].msram_virt_addr,
			xhci_sram[id].mlength);
	return 0;
}

int xhci_mtk_free_sram(int id)
{
	xhci_sram[id].state = STATE_INIT;
	pr_info("%s, id=%d\n", __func__, id);
	return 0;
}

void *mtk_usb_alloc_sram(int id, size_t size, dma_addr_t *dma)
{
	void *sram_virt_addr = NULL;

	/* check if xhci control buffer on sram */
	if (xhci_sram[0].state != STATE_USE)
		return NULL;

	mtk_audio_request_sram(dma, (unsigned char **)&sram_virt_addr,
					   size, &usb_audio_sram[id]);

	if (sram_virt_addr) {
		usb_audio_sram[id].mlength = size;
		usb_audio_sram[id].msram_phys_addr = *dma;
		usb_audio_sram[id].msram_virt_addr =  sram_virt_addr;
		usb_audio_sram[id].state = STATE_USE;
		pr_debug("%s, id=%d\n", __func__, id);
	} else {
		usb_audio_sram[id].state = STATE_NOMEM;
		pr_err("%s fail id=%d\n", __func__, id);
	}

	return sram_virt_addr;
}

void mtk_usb_free_sram(id)
{
	if (usb_audio_sram[id].state == STATE_USE) {
		mtk_audio_free_sram(&usb_audio_sram[id]);
		usb_audio_sram[id].mlength = 0;
		usb_audio_sram[id].msram_phys_addr = 0;
		usb_audio_sram[id].msram_virt_addr =  NULL;
		pr_debug("%s, id=%d\n", __func__, id);
	} else {
		pr_err("%s, fail id=%d\n", __func__, id);
	}
	usb_audio_sram[id].state = STATE_UNINIT;
}
#endif

static int __maybe_unused xhci_mtk_runtime_suspend(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd *hcd = mtk->hcd;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	struct device_node *of_node = mtk->dev->of_node;

	xhci_info(xhci, "%s:\n", __func__);

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
		xhci_mtk_host_disable(mtk);
#ifdef CONFIG_MTK_UAC_POWER_SAVING
		xhci_mtk_set_power_mode(USB_DPIDLE_SRAM);
#endif
	}

	return 0;
}

static int __maybe_unused xhci_mtk_runtime_resume(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd *hcd = mtk->hcd;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	struct device_node *of_node = mtk->dev->of_node;

	xhci_info(xhci, "%s:\n", __func__);

	if (of_device_is_compatible(of_node, "mediatek,mt67xx-xhci")) {
#ifdef CONFIG_MTK_UAC_POWER_SAVING
		xhci_mtk_set_power_mode(USB_DPIDLE_FORBIDDEN);
#endif
		xhci_mtk_host_enable(mtk);
	}

	return 0;
}

/*
 * if ip sleep fails, and all clocks are disabled, access register will hang
 * AHB bus, so stop polling roothubs to avoid regs access on bus suspend.
 * and no need to check whether ip sleep failed or not; this will cause SPM
 * to wake up system immediately after system suspend complete if ip sleep
 * fails, it is what we wanted.
 */
static int __maybe_unused xhci_mtk_suspend(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd *hcd = mtk->hcd;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	int ret;
#endif

	xhci_info(xhci, "xhci_plat_suspend\n");
#ifdef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	ret = xhci_mtk_host_disable(mtk);
	if (ret) {
		xhci_mtk_host_enable(mtk);
		return -EBUSY;
	}
#endif
	xhci_dbg(xhci, "%s: stop port polling\n", __func__);
	clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	del_timer_sync(&hcd->rh_timer);
	clear_bit(HCD_FLAG_POLL_RH, &xhci->shared_hcd->flags);
	del_timer_sync(&xhci->shared_hcd->rh_timer);
#ifndef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	xhci_mtk_host_disable(mtk);
#endif
	xhci_mtk_phy_power_off(mtk);
	xhci_mtk_clks_disable(mtk);
	usb_wakeup_enable(mtk);
	return 0;
}

static int __maybe_unused xhci_mtk_resume(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd *hcd = mtk->hcd;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

	xhci_info(xhci, "xhci_plat_resume\n");
	usb_wakeup_disable(mtk);
	xhci_mtk_clks_enable(mtk);
	xhci_mtk_phy_power_on(mtk);
	xhci_mtk_host_enable(mtk);

	xhci_dbg(xhci, "%s: restart port polling\n", __func__);
	set_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	usb_hcd_poll_rh_status(hcd);
	set_bit(HCD_FLAG_POLL_RH, &xhci->shared_hcd->flags);
	usb_hcd_poll_rh_status(xhci->shared_hcd);
	return 0;
}

static const struct dev_pm_ops xhci_mtk_pm_ops = {
#ifndef CONFIG_USB_XHCI_MTK_SUSPEND_SUPPORT
	SET_SYSTEM_SLEEP_PM_OPS(xhci_mtk_suspend, xhci_mtk_resume)
	SET_RUNTIME_PM_OPS(xhci_mtk_runtime_suspend,
			xhci_mtk_runtime_resume, NULL)
#else
	.resume_noirq = xhci_mtk_resume,
	.suspend_noirq = xhci_mtk_suspend,
#endif
};

#define DEV_PM_OPS (IS_ENABLED(CONFIG_PM) ? &xhci_mtk_pm_ops : NULL)

#ifdef CONFIG_OF
static const struct of_device_id mtk_xhci_of_match[] = {
	{ .compatible = "mediatek,mt8173-xhci"},
	{ .compatible = "mediatek,mt67xx-xhci"},
	{ },
};

MODULE_DEVICE_TABLE(of, mtk_xhci_of_match);
#endif

static struct platform_driver mtk_xhci_driver = {
	.probe	= xhci_mtk_probe,
	.remove	= xhci_mtk_remove,
	.driver	= {
		.name = "xhci-mtk",
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(mtk_xhci_of_match),
	},
};
MODULE_ALIAS("platform:xhci-mtk");

#ifdef CONFIG_USB_MTK_DUALMODE
int xhci_mtk_register_plat(void)
{
	xhci_init_driver(&xhci_mtk_hc_driver, &xhci_mtk_overrides);
	return platform_driver_register(&mtk_xhci_driver);
}

void xhci_mtk_unregister_plat(void)
{
	platform_driver_unregister(&mtk_xhci_driver);
}
#else
static int __init xhci_mtk_init(void)
{
	xhci_init_driver(&xhci_mtk_hc_driver, &xhci_mtk_overrides);
	return platform_driver_register(&mtk_xhci_driver);
}
module_init(xhci_mtk_init);

static void __exit xhci_mtk_exit(void)
{
	platform_driver_unregister(&mtk_xhci_driver);
}
module_exit(xhci_mtk_exit);
#endif

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_DESCRIPTION("MediaTek xHCI Host Controller Driver");
MODULE_LICENSE("GPL v2");
