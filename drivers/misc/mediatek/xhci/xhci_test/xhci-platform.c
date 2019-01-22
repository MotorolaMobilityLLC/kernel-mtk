/*
 * xHCI host controller driver PCI Bus Glue.
 *
 * Copyright (C) 2008 Intel Corp.
 *
 * Author: Sarah Sharp
 * Some code borrowed from the Linux EHCI driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */



#include <linux/platform_device.h>
#include <linux/clk.h>

#include "xhci.h"
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include "mtk-test-lib.h"
#include "mtk-usb-hcd.h"

#include <linux/interrupt.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

/* Device for a quirk */
static const char hcd_name[] = "xhc_mtk";

static struct platform_device *g_pdev;


void mtktest_xhci_vbus_on(void)
{
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinctrl_drvvbus_high;

	if (g_pdev == NULL) {
		pr_notice("g_pdev is not ready\n");
		return;
	}

	pinctrl = devm_pinctrl_get(&g_pdev->dev);

	if (IS_ERR(pinctrl)) {
		dev_err(&g_pdev->dev, "Cannot find usb pinctrl!\n");
		return;
	}

	pinctrl_drvvbus_high = pinctrl_lookup_state(pinctrl, "drvvbus_high");

	if (IS_ERR(pinctrl_drvvbus_high)) {
		dev_err(&g_pdev->dev, "Cannot find usb pinctrl drvvbus_high\n");
		return;
	}
	pinctrl_select_state(pinctrl, pinctrl_drvvbus_high);
}

void mtktest_xhci_vbus_off(void)
{
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinctrl_drvvbus_low;

	if (g_pdev == NULL) {
		pr_notice("g_pdev is not ready\n");
		return;
	}

	pinctrl = devm_pinctrl_get(&g_pdev->dev);
	if (IS_ERR(pinctrl)) {
		dev_err(&g_pdev->dev, "Cannot find usb pinctrl!\n");
		return;
	}

	pinctrl_drvvbus_low = pinctrl_lookup_state(pinctrl, "drvvbus_low");

	if (IS_ERR(pinctrl_drvvbus_low)) {
		dev_err(&g_pdev->dev, "Cannot find usb pinctrl drvvbus_low\n");
		return;
	}
	pinctrl_select_state(pinctrl, pinctrl_drvvbus_low);
}

/*-------------------------------------------------------------------------*/

/* called during probe() after chip reset completes */
static int xhci_mtk_setup(struct usb_hcd *hcd)
{
	struct xhci_hcd   *xhci;
	int     retval;
	/*u32     temp;*/

	hcd->self.sg_tablesize = TRBS_PER_SEGMENT - 2;
		/* xHCI private pointer was set in xhci_pci_probe for the second
		 * registered roothub.
		 */
	xhci = hcd_to_xhci(hcd);
	xhci->cap_regs = hcd->regs;
	xhci->op_regs = hcd->regs +
		HC_LENGTH(xhci_readl(xhci, &xhci->cap_regs->hc_capbase));
	xhci->run_regs = hcd->regs +
		(xhci_readl(xhci, &xhci->cap_regs->run_regs_off) & RTSOFF_MASK);
	/* Cache read-only capability registers */
	xhci->hcs_params1 = xhci_readl(xhci, &xhci->cap_regs->hcs_params1);
	xhci->hcs_params2 = xhci_readl(xhci, &xhci->cap_regs->hcs_params2);
	xhci->hcs_params3 = xhci_readl(xhci, &xhci->cap_regs->hcs_params3);
	xhci->hcc_params = xhci_readl(xhci, &xhci->cap_regs->hc_capbase);
	xhci->hci_version = HC_VERSION(xhci->hcc_params);
	xhci->hcc_params = xhci_readl(xhci, &xhci->cap_regs->hcc_params);
	mtktest_xhci_print_registers(xhci);

	/* Make sure the HC is halted. */
	retval = mtktest_xhci_halt(xhci);
	if (retval)
		goto error;

	xhci_dbg(xhci, "Resetting HCD\n");
	/* Reset the internal HC memory state and registers. */
	retval = mtktest_xhci_reset(xhci);
	if (retval)
		goto error;
	xhci_dbg(xhci, "Reset complete\n");
	xhci_dbg(xhci, "Calling HCD init\n");

/*  mtktest_setInitialReg();*/

	/* Initialize HCD and host controller data structures. */
	retval = mtktest_xhci_init(hcd);
	if (retval)
		goto error;
	xhci_dbg(xhci, "Called HCD init\n");
	return retval;
error:
	kfree(xhci);
	return retval;
}


static const struct hc_driver xhci_versatile_hc_driver;
/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */

/**
 * ehci_hcd_omap_probe - initialize TI-based HCDs
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 */
static int usb_hcd_versatile_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	/*int i;*/
	int irq;
	/*char supply[7];*/
	struct device_node *node = NULL;
	struct resource res;
	int ret = -ENODEV;

	pr_notice("hcd_versatile_probe is called\n");

	g_pdev = pdev;

	/* Chiachun: don't u$se platform_device API first */
		/* got related info from dtsi */
	node = of_find_compatible_node(NULL, NULL, USB_XHCI_COMPATIBLE_NAME);
	if (node == NULL) {
		pr_notice("xhci_test get node failed\n");
		return ret;
	}
	if (of_address_to_resource(node, 0, &res)) {
		pr_notice("xhci_test get res failed\n");
		return ret;
	}
		/* special resource get*/
	irq = xhci_usbif_nirq;
	hcd = mtk_usb_create_hcd(&xhci_versatile_hc_driver, &pdev->dev,
		dev_name(&pdev->dev));
	if (!hcd) {
		dev_dbg(&pdev->dev, "failed to create hcd with err %d\n", ret);
		ret = -ENOMEM;
		goto err_create_hcd;
	}
	pr_notice("Creat HCD success!\n");
	hcd->rsrc_start = res.start;
	hcd->rsrc_len = resource_size(&res);

	pr_notice("====%s(%d)==== hcd->pool[0] = 0x%p!\n", __func__, __LINE__, hcd->pool[0]);

	pr_notice("Create xHC Base address 0x%p!\n", (void *)(unsigned long)hcd->rsrc_start);
	/* special resource get*/
	hcd->regs = xhci_usbif_base;

	if (!hcd->regs) {
		dev_err(&pdev->dev, "XHCI ioremap failed\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}
	hcd->self.sg_tablesize = TRBS_PER_SEGMENT - 2;
	hcd->self.uses_dma = 1;
		/* we know this is the memory we want, no need to ioremap again */
		ret = mtk_usb_add_hcd(hcd, irq, IRQF_SHARED);
	my_hcd = hcd;
	if (ret) {
		dev_dbg(&pdev->dev, "failed to add hcd with err %d\n", ret);
		goto err_add_hcd;
	}
	pr_notice("====%s(%d)==== hcd->pool[0] = 0x%p!\n", __func__, __LINE__, hcd->pool[0]);
	pr_notice("usb_add_hcd success!\n");
	return 0;
err_add_hcd:
/*err_start:*/
/*err_tll_ioremap:*/
/*err_uhh_ioremap:*/
	iounmap(hcd->regs);
err_ioremap:
	usb_put_hcd(hcd);
err_create_hcd:
/*err_disabled:*/
/*err_pdata:*/
	return ret;
}


static int usb_hcd_versatile_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd;

	pr_notice("hcd_versatile_remove is called\n");
	g_pdev = NULL;
	hcd = platform_get_drvdata(pdev);
	mtk_usb_remove_hcd(hcd);
	usb_put_hcd(hcd);
	/* USBIF , should we free hcd ?  or it will be clean by kernel cause usb_put_hcd? */
	/* kfree(hcd);*/
	/* kfree(vers);*/

	pr_notice("hcd_versatile_remove is completed\n");

	/* special resource release */
	iounmap(xhci_usbif_base);
	iounmap(xhci_usbif_sif_base);
	return 0;
}

#if 0
/* called during probe() after chip reset completes */
static int xhci_setup(struct usb_hcd *hcd)
{
	struct xhci_hcd   *xhci;
/*  struct pci_dev    *pdev = to_pci_dev(hcd->self.controller);*/
	int     retval;
	/*u32     temp;*/

	hcd->self.sg_tablesize = TRBS_PER_SEGMENT - 2;
		/* xHCI private pointer was set in xhci_pci_probe for the second
		 * registered roothub.
		 */
		xhci = hcd_to_xhci(hcd);
	xhci->cap_regs = hcd->regs;
	xhci->op_regs = hcd->regs +
		HC_LENGTH(xhci_readl(xhci, &xhci->cap_regs->hc_capbase));
	xhci->run_regs = hcd->regs +
		(xhci_readl(xhci, &xhci->cap_regs->run_regs_off) & RTSOFF_MASK);
	/* Cache read-only capability registers */
	xhci->hcs_params1 = xhci_readl(xhci, &xhci->cap_regs->hcs_params1);
	xhci->hcs_params2 = xhci_readl(xhci, &xhci->cap_regs->hcs_params2);
	xhci->hcs_params3 = xhci_readl(xhci, &xhci->cap_regs->hcs_params3);
	xhci->hcc_params = xhci_readl(xhci, &xhci->cap_regs->hc_capbase);
	xhci->hci_version = HC_VERSION(xhci->hcc_params);
	xhci->hcc_params = xhci_readl(xhci, &xhci->cap_regs->hcc_params);
	mtktest_xhci_print_registers(xhci);
	/* Make sure the HC is halted. */
	retval = mtktest_xhci_halt(xhci);
	if (retval)
		goto error;

	xhci_dbg(xhci, "Resetting HCD\n");
	/* Reset the internal HC memory state and registers. */
	retval = mtktest_xhci_reset(xhci);
	if (retval)
		goto error;
	xhci_dbg(xhci, "Reset complete\n");
	xhci_dbg(xhci, "Calling HCD init\n");
	/* Initialize HCD and host controller data structures. */
	retval = mtktest_xhci_init(hcd);
	if (retval)
		goto error;
	xhci_dbg(xhci, "Called HCD init\n");
	return retval;
error:
	kfree(xhci);
	return retval;
}
#endif

static void usb_hcd_versatile_shutdown(struct platform_device *pdev)
{
	pr_notice("hcd_versatile_shutdown is called\n");
	if (my_hcd->driver->shutdown)
		my_hcd->driver->shutdown(my_hcd);
	pr_notice("hcd_versatile_shutdown is completed\n");
}
static const struct hc_driver xhci_versatile_hc_driver = {
	.description =    hcd_name,
	.product_desc =   "xHCI MTK Test Host Controller",
	.hcd_priv_size =  sizeof(struct xhci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =      mtktest_xhci_mtk_irq,
	.flags =    HCD_MEMORY | HCD_USB3,

	/*
	 * basic lifecycle operations
	 */
	.reset =    xhci_mtk_setup,
	.start =    mtktest_xhci_mtk_run,
	/* suspend and resume implemented later */
	.stop =     mtktest_xhci_mtk_stop,
	.shutdown =   mtktest_xhci_mtk_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =    mtktest_xhci_mtk_urb_enqueue,
	.urb_dequeue =    mtktest_xhci_mtk_urb_dequeue,
	.alloc_dev =    mtktest_xhci_mtk_alloc_dev,
	.free_dev =   mtktest_xhci_mtk_free_dev,
	.alloc_streams =  mtktest_xhci_mtk_alloc_streams,
	.free_streams =   mtktest_xhci_mtk_free_streams,
	.add_endpoint =   mtktest_xhci_mtk_add_endpoint,
	.drop_endpoint =  mtktest_xhci_mtk_drop_endpoint,
	.endpoint_reset = mtktest_xhci_mtk_endpoint_reset,
	.check_bandwidth =  mtktest_xhci_mtk_check_bandwidth,
	.reset_bandwidth =  mtktest_xhci_mtk_reset_bandwidth,
	.address_device = mtktest_xhci_mtk_address_device,
	.update_hub_device =  mtktest_xhci_mtk_update_hub_device,
	.reset_device =   mtktest_xhci_mtk_reset_device,

	/*
	 * scheduling support
	 */
	.get_frame_number = mtktest_xhci_mtk_get_frame,

	/* Root hub support */
	.hub_control =    mtktest_xhci_mtk_hub_control,
	.hub_status_data =  mtktest_xhci_mtk_hub_status_data,
};


#ifdef CONFIG_OF
static const struct of_device_id mtk_xhci_of_match[] = {
	{
		.compatible = USB_XHCI_COMPATIBLE_NAME,
	},
	{ },
};
#endif

static struct platform_driver xhci_versatile_driver = {

	.probe =  usb_hcd_versatile_probe,
	.remove = usb_hcd_versatile_remove,
	.shutdown = usb_hcd_versatile_shutdown,

	.driver = {
		.name =   (char *) hcd_name,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(mtk_xhci_of_match),
#endif
	}

};
