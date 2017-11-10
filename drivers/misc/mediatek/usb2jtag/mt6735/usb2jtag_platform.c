#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include "../usb2jtag_v1.h"
#include <mach/upmu_hw.h>
#include <mt-plat/sync_write.h>
void __iomem *INFRA_AO_BASE;
void __iomem *USB_SIF_BASE;
static int mt_usb2jtag_hw_init(void)
{
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,usb2jtag_v1");
	if (!node) {
		pr_err("[USB2JTAG] map node @ mediatek,usb2jtag_v1 failed\n");
		return -1;
	}
	INFRA_AO_BASE = of_iomap(node, 0);
	if (!INFRA_AO_BASE) {
		pr_err("[USB2JTAG] iomap INFRA_AO_BASE failed\n");
		return -1;
	}
	USB_SIF_BASE = of_iomap(node, 1);
	if (!USB_SIF_BASE) {
		pr_err("[USB2JTAG] iomap USB_SIF_BASE failed\n");
		return -1;
	}

	/* set ap_usb2jtag_en: 0x1000_0700 bit[13] = 1 */
	writel(readl(INFRA_AO_BASE + 0x700) | (0x1 << 13), INFRA_AO_BASE + 0x700);

#ifndef CONFIG_MTK_CLKMGR
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735-usb20");
	if (!node) {
		pr_err("[USB2JTAG] map node @ mt6735-usb20 failed\n");
		return -1;
	}
	/* musb_clk = devm_clk_get(NULL, "usb0"); */
	musb_clk = of_clk_get_by_name(node, "usb0");
	if (IS_ERR(musb_clk)) {
		pr_err("[USB2JTAG] cannot get musb clock\n");
		return PTR_ERR(musb_clk);
	}
	pr_debug("[USB2JTAG] get musb clock ok, prepare it\n");
	if (clk_prepare(musb_clk) == 0)
		pr_debug("[USB2JTAG] prepare done\n");
	else {
		pr_err("[USB2JTAG] prepare fail\n");
		return -1;
	}
#endif
	/* enable use clock */
	usb_enable_clock(true);
	/* delay for enabling usb clock */
	udelay(50);
	/* Set rg_usb_gpio_ctrl: 0x11210821 bit[1] = 1 */
	writel(readl(USB_SIF_BASE + 0x0820) | (0x1 << 9), USB_SIF_BASE + 0x0820);
	/* clear RG_USB20_BC11_SW_EN: 0x1121081A bit[7] = 0 */
	writel(readl(USB_SIF_BASE + 0x0818) & ~(0x1 << 23), USB_SIF_BASE + 0x0818);
	/* Set BGR_EN 0x11210800 bit[0] = 1 */
	writel(readl(USB_SIF_BASE + 0x0800) | (0x1 << 0), USB_SIF_BASE + 0x0800);
	pr_err("[USB2JTAG] 0x11210820 = 0x%x\n", readl(USB_SIF_BASE + 0x0820));
	pr_err("[USB2JTAG] 0x11210818 = 0x%x\n", readl(USB_SIF_BASE + 0x0818));
	/* disable use clock */
	usb_enable_clock(false);
	pr_err("[USB2JTAG] setting done\n");
	return 0;
}

static int __init mt_usb2jtag_platform_init(void)
{
	struct mt_usb2jtag_driver *mt_usb2jtag_drv;

	mt_usb2jtag_drv = get_mt_usb2jtag_drv();
	mt_usb2jtag_drv->usb2jtag_init = mt_usb2jtag_hw_init;
	mt_usb2jtag_drv->usb2jtag_suspend = NULL;
	mt_usb2jtag_drv->usb2jtag_resume = NULL;

	return 0;
}

arch_initcall(mt_usb2jtag_platform_init);
