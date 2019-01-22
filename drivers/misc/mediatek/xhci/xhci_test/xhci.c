/*
 * xHCI host controller driver
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

#include <linux/irq.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#include <linux/kernel.h>
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
/*#include <linux/pci.h>*/
#include <asm/unaligned.h>
/*#include <linux/usb/hcd.h>*/
#include "xhci.h"
#include "mtk-test.h"
#include "mtk-test-lib.h"
#include "xhci-platform.c"
#include "mtk-usb-hcd.h"
#include "xhci-mtk-power.h"
#include "xhci-mtk-scheduler.h"

#if 1 /* USBIF*/
#include <linux/irq.h>
/*#include <linux/switch.h>*/
#endif
/*#include <mach/upmu_common.h>*/
#include <mt-plat/mtk_gpio.h>
/*#include <cust_gpio_usage.h>*/

/* Some 0.95 hardware can't handle the chain bit on a Link TRB being cleared */
static int link_quirk;

static void xhci_work(struct xhci_hcd *xhci)
{
	u32 temp;
	u64 temp_64;
	dma_addr_t deq;
	union xhci_trb *event_ring_deq;

	/*
	 * Clear the op reg interrupt status first,
	 * so we can receive interrupts from other MSI-X interrupters.
	 * Write 1 to clear the interrupt status.
	 */
	temp = xhci_readl(xhci, &xhci->op_regs->status);
	log_debug("[OTG_H][xhci_work] read status 0x%x\n", temp);

	temp |= STS_EINT;
	xhci_writel(xhci, temp, &xhci->op_regs->status);

	/* Acknowledge the interrupt */
	temp = xhci_readl(xhci, &xhci->ir_set->irq_pending);
	log_debug("[OTG_H][xhci_work] read irq_pending 0x%x\n", temp);

	temp |= 0x3;
	xhci_writel(xhci, temp, &xhci->ir_set->irq_pending);

	/* Flush posted writes */
	xhci_readl(xhci, &xhci->ir_set->irq_pending);
	event_ring_deq = xhci->event_ring->dequeue;

	while (mtktest_xhci_handle_event(xhci) > 0)
		;


	/* Clear the event handler busy flag (RW1C); the event ring should be empty. */
	temp_64 = xhci_read_64(xhci, &xhci->ir_set->erst_dequeue);
	/* If necessary, update the HW's version of the event ring deq ptr. */
	if (event_ring_deq != xhci->event_ring->dequeue) {
		deq = mtktest_xhci_trb_virt_to_dma(xhci->event_ring->deq_seg,
				xhci->event_ring->dequeue);
		if (deq == 0)
			xhci_warn(xhci, "WARN something wrong with SW event ring dequeue ptr.\n");
		/* Update HC event ring dequeue pointer */
		temp_64 &= ERST_PTR_MASK;
		temp_64 |= ((u64) deq & (u64) ~ERST_PTR_MASK);
	}
	xhci_dbg(xhci, "Clear EHB bit (RW1C)");
	xhci_write_64(xhci, temp_64 | ERST_EHB, &xhci->ir_set->erst_dequeue);
	/* Flush posted writes -- FIXME is this necessary? */
	xhci_readl(xhci, &xhci->ir_set->irq_pending);
}

irqreturn_t mtktest_xhci_mtk_irq(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	u32 temp, temp2;
	union xhci_trb *trb;
#if TEST_OTG
	u32 temp3;
#endif

	spin_lock(&xhci->lock);
#if TEST_OTG
	temp3 = readl(SSUSB_OTG_STS);
	/* log_err("[OTG_H][IRQ] OTG_STS 0x%x\n", temp3); */
#if 0
	if (temp3 & SSUSB_IDDIG)
		g_otg_iddig = 1;
	else
		g_otg_iddig = 0;

#endif
#if 0
	if (temp3 & SSUSB_ATTACH_A_ROLE) {
#if 0
		/*set OTG_VBUSVALID_SEL for host*/
		temp = readl(SSUSB_U2_CTRL(0));
		temp = temp | SSUSB_U2_PORT_OTG_HOST_VBUSVALID_SEL;
		writel(temp, SSUSB_U2_CTRL(0));
#endif
		/*attached as device-A, turn on port power of all xhci port*/
		mtktest_enableXhciAllPortPower(xhci);
		writel(SSUSB_ATTACH_A_ROLE_CLR, SSUSB_OTG_STS_CLR);
		spin_unlock(&xhci->lock);
		return IRQ_HANDLED;
	}
#endif
	if (temp3 & SSUSB_CHG_A_ROLE_A) {
		g_otg_wait_con = true;
		g_otg_hnp_become_dev = false;
		g_otg_hnp_become_host = true;
		mb();
		writel(SSUSB_CHG_A_ROLE_A_CLR, SSUSB_OTG_STS_CLR);
		/*set host sel*/
		log_err("[OTG_H] going to set dma to host\n");


#if 0
		writel(0x0f0f0f0f, 0xf00447bc);
		while ((readl(0xf00447c4) & 0x2000) == 0x2000)
			;

		log_err("[OTG_H] can set dma to host\n");
#endif
		temp = readl(SSUSB_U2_CTRL(0));
		temp = temp | SSUSB_U2_PORT_HOST_SEL;
		writel(temp, SSUSB_U2_CTRL(0));
		spin_unlock(&xhci->lock);
		return IRQ_HANDLED;
	}
	if (temp3 & SSUSB_CHG_B_ROLE_A) {
		log_notice("[OTG_H] get SSUSB_CHG_B_ROLE_A\n");
		g_otg_hnp_become_dev = true;
		mb();
		writel(SSUSB_CHG_B_ROLE_A_CLR, SSUSB_OTG_STS_CLR);
		spin_unlock(&xhci->lock);
		return IRQ_HANDLED;
	}
	if (temp3 & SSUSB_SRP_REQ_INTR) {
		/*set port_power*/
		/*g_otg_srp_pend = false;*/
		/*mb() ;*/
		/*while(g_otg_srp_pend);*/
		log_notice("[OTG_H] get srp interrupt, just clear it\n");

		writel(SSUSB_SRP_REQ_INTR_CLR, SSUSB_OTG_STS_CLR);
		spin_unlock(&xhci->lock);
		return IRQ_HANDLED;
	}
#endif
	trb = xhci->event_ring->dequeue;

	temp = xhci_readl(xhci, &xhci->op_regs->status);
	temp2 = xhci_readl(xhci, &xhci->ir_set->irq_pending);

	/*log_err("[OTG_H]mtktest_xhci_mtk_irq , status: %x\n", temp);*/

	/*log_err("[OTG_H]mtktest_xhci_mtk_irq , irq_pending: %x\n", temp2);*/

	if (!(temp & STS_EINT) && !ER_IRQ_PENDING(temp2)) {
		spin_unlock(&xhci->lock);
		return IRQ_NONE;
	}
	xhci_dbg(xhci, "--> interrupt in: op_reg(%08x), irq_pend(%08x)\n", temp, temp2);

	#if 0
	xhci_dbg(xhci, "Got interrupt\n");
	xhci_dbg(xhci, "op reg status = %08x\n", temp);
	xhci_dbg(xhci, "ir set irq_pending = %08x\n", temp2);
	xhci_dbg(xhci, "Event ring dequeue ptr:\n");
	xhci_dbg(xhci, "@%llx %08x %08x %08x %08x\n",
			(unsigned long long)mtktest_xhci_trb_virt_to_dma(xhci->event_ring->deq_seg, trb),
			lower_32_bits(trb->link.segment_ptr),
			upper_32_bits(trb->link.segment_ptr),
			(unsigned int) trb->link.intr_target,
			(unsigned int) trb->link.control);
	#endif
	if (g_intr_handled != -1)
		g_intr_handled++;

	xhci_work(xhci);
	spin_unlock(&xhci->lock);
	xhci_dbg(xhci, "<-- interrupt out\n");
	return IRQ_HANDLED;
}


#if 1 /* USBIF*/
static int mtk_idpin_irqnum;

#if 0
#define IDPIN_IN MT_EINT_POL_NEG
#define IDPIN_OUT MT_EINT_POL_POS
#else
enum usbif_idpin_state {
	IDPIN_IN,
	IDPIN_OUT,
};

#endif

static struct xhci_hcd  *mtk_xhci;

void mtktest_mtk_xhci_set(struct xhci_hcd *xhci)
{
	mtk_xhci = xhci;
}


static bool mtk_id_nxt_state = IDPIN_IN;
#define IDDIG_EINT_PIN (16)

static bool mtktest_set_iddig_out_detect(void)
{
	irq_set_irq_type(mtk_idpin_irqnum, IRQF_TRIGGER_HIGH);

	return 0;
}

static bool mtktest_set_iddig_in_detect(void)
{
	irq_set_irq_type(mtk_idpin_irqnum, IRQF_TRIGGER_LOW);
	return 0;
}

static irqreturn_t mtktest_xhci_eint_iddig_isr(int irqnum, void *data)
{
	bool cur_id_state = mtk_id_nxt_state;

	if (cur_id_state == IDPIN_IN) { /* HOST*/
		/* open port power and switch resource to host */
		/* mtk_switch2host();*/
		/* assert port power bit to drive drv_vbus */
		/* mtktest_enableXhciAllPortPower(mtk_xhci);*/

		/* expect next isr is for id-pin out action */
		mtk_id_nxt_state = IDPIN_OUT;

		g_otg_iddig = 0;
		mb();

		/* make id pin to detect the plug-out */
		mtktest_set_iddig_out_detect();

		/*writel(SSUSB_ATTACH_A_ROLE, SSUSB_OTG_STS);*/
	} else { /* IDPIN_OUT */
		/* deassert port power bit to drop off vbus */
		/*mtktest_disableXhciAllPortPower(mtk_xhci);*/
		/* close all port power, but not switch the resource */
		/*mtk_switch2device(false);*/
		/* expect next isr is for id-pin in action */
		mtk_id_nxt_state = IDPIN_IN;

		g_otg_iddig = 1;
		mb();
		/* make id pin to detect the plug-in */
		mtktest_set_iddig_in_detect();

		/*writel(SSUSB_ATTACH_B_ROLE, SSUSB_OTG_STS);*/
	}

	/*log_notice("[OTG_H] xhci switch resource to %s\n", (cur_id_state == IDPIN_IN)? "host": "device");*/

	return 0;
}


void mtktest_mtk_xhci_eint_iddig_init(void)
{
	int retval = 0;
	struct device_node *node;
	int iddig_gpio, iddig_debounce;
	u32 ints[2] = {0, 0};

	node = of_find_compatible_node(NULL, NULL, "mediatek,usb3_xhci");
	if (node) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,usb_iddig_bi_eint");
		if (node) {
			retval = of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
			if (!retval) {
				iddig_gpio = ints[0];
				iddig_debounce = ints[1];
				mtk_idpin_irqnum = irq_of_parse_and_map(node, 0);
				 log_notice("iddig gpio num = %d\n", mtk_idpin_irqnum);
			}
		}
		if (retval) {
			log_notice("get iddig-gpio-num failed, retval(%d)!!!\n", retval);
			return;
		}
	} else {
		log_notice("cannot get the node\n");
		return;
	}
	/* microseconds */
	/* mt_gpio_set_debounce(iddig_gpio, 0); */
	g_otg_iddig = 1;

	retval =
		request_irq(mtk_idpin_irqnum, mtktest_xhci_eint_iddig_isr, IRQF_TRIGGER_LOW, "usbif_iddig_eint",
			NULL);


	log_notice("[OTG_H] XHCI test driver GPIO iddig setting done.\n");
}

void mtktest_mtk_xhci_eint_iddig_deinit(void)
{
	disable_irq_nosync(mtk_idpin_irqnum);

	free_irq(mtk_idpin_irqnum, NULL);

	log_notice("[OTG_H] XHCI test driver GPIO iddig deinit done.\n");
}

#endif

/* xhci original functions*/

/* TODO: copied from ehci-hcd.c - can this be refactored? */
/*
 * handshake - spin reading hc until handshake completes or fails
 * @ptr: address of hc register to be read
 * @mask: bits to look at in result of read
 * @done: value of those bits when handshake succeeds
 * @usec: timeout in microseconds
 *
 * Returns negative errno, or zero on success
 *
 * Success happens when the "mask" bits have the specified value (hardware
 * handshake done).  There are two failure modes:  "usec" have passed (major
 * hardware flakeout), or the register reads as all-ones (hardware removed).
 */
static int handshake(struct xhci_hcd *xhci, void __iomem *ptr,
				u32 mask, u32 done, int usec)
{
	u32 result;

	do {
		result = xhci_readl(xhci, ptr);
		if (result == ~(u32)0)    /* card removed */
			return -ENODEV;
		result &= mask;
		if (result == done)
			return 0;
		udelay(1);
		usec--;
	} while (usec > 0);
	return -ETIMEDOUT;
}

/*
 * Disable interrupts and begin the xHCI halting process.
 */
void mtktest_xhci_quiesce(struct xhci_hcd *xhci)
{
	u32 halted;
	u32 cmd;
	u32 mask;

	mask = ~(XHCI_IRQS);
	halted = xhci_readl(xhci, &xhci->op_regs->status) & STS_HALT;
	if (!halted)
		mask &= ~CMD_RUN;

	cmd = xhci_readl(xhci, &xhci->op_regs->command);
	cmd &= mask;
	xhci_writel(xhci, cmd, &xhci->op_regs->command);
}

/*
 * Force HC into halt state.
 *
 * Disable any IRQs and clear the run/stop bit.
 * HC will complete any current and actively pipelined transactions, and
 * should halt within 16 microframes of the run/stop bit being cleared.
 * Read HC Halted bit in the status register to see when the HC is finished.
 * XXX: shouldn't we set HC_STATE_HALT here somewhere?
 */
int mtktest_xhci_halt(struct xhci_hcd *xhci)
{
	xhci_dbg(xhci, "// Halt the HC\n");
	mtktest_xhci_quiesce(xhci);

	return handshake(xhci, &xhci->op_regs->status,
			STS_HALT, STS_HALT, XHCI_MAX_HALT_USEC);
}
/*
 * Set the run bit and wait for the host to be running.
 */
int xhci_start(struct xhci_hcd *xhci)
{
	u32 temp;
	int ret;

	temp = xhci_readl(xhci, &xhci->op_regs->command);
	temp |= (CMD_RUN);
	xhci_dbg(xhci, "// Turn on HC, cmd = 0x%x.\n",
			temp);
	xhci_writel(xhci, temp, &xhci->op_regs->command);

	/*
	 * Wait for the HCHalted Status bit to be 0 to indicate the host is
	 * running.
	 */
	ret = handshake(xhci, &xhci->op_regs->status,
			STS_HALT, 0, XHCI_MAX_HALT_USEC);
	if (ret == -ETIMEDOUT)
		xhci_err(xhci, "[ERROR]Host took too long to start, waited %u microseconds.\n",
				XHCI_MAX_HALT_USEC);
	return ret;
}

/*
 * Reset a halted HC, and set the internal HC state to HC_STATE_HALT.
 *
 * This resets pipelines, timers, counters, state machines, etc.
 * Transactions will be terminated immediately, and operational registers
 * will be set to their defaults.
 */
int mtktest_xhci_reset(struct xhci_hcd *xhci)
{
	u32 command;
	u32 state;
	int ret;

	state = xhci_readl(xhci, &xhci->op_regs->status);
	if ((state & STS_HALT) == 0) {
		xhci_warn(xhci, "Host controller not halted, aborting reset.\n");
		return 0;
	}

	xhci_dbg(xhci, "// Reset the HC\n");
	command = xhci_readl(xhci, &xhci->op_regs->command);
	command |= CMD_RESET;
	xhci_writel(xhci, command, &xhci->op_regs->command);
	/* XXX: Why does EHCI set this here?  Shouldn't other code do this? */
	xhci_to_hcd(xhci)->state = HC_STATE_HALT;

	ret = handshake(xhci, &xhci->op_regs->command,
			CMD_RESET, 0, 250 * 1000);
	if (ret)
		return ret;

	xhci_dbg(xhci, "Wait for controller to be ready for doorbell rings\n");
	/*
	 * xHCI cannot write to any doorbells or operational registers other
	 * than status until the "Controller Not Ready" flag is cleared.
	 */
	return handshake(xhci, &xhci->op_regs->status, STS_CNR, 0, 250 * 1000);
}

/*
 * Initialize memory for HCD and xHC (one-time init).
 *
 * Program the PAGESIZE register, initialize the device context array, create
 * device contexts (?), set up a command ring segment (or two?), create event
 * ring (one for now).
 */
int mtktest_xhci_init(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	int retval = 0;

	xhci_dbg(xhci, "mtktest_xhci_init\n");
	spin_lock_init(&xhci->lock);
	if (link_quirk) {
		xhci_dbg(xhci, "QUIRK: Not clearing Link TRB chain bits.\n");
		xhci->quirks |= XHCI_LINK_TRB_QUIRK;
	} else
		xhci_dbg(xhci, "xHCI doesn't need link TRB QUIRK\n");

	retval = mtktest_xhci_mem_init(xhci, GFP_KERNEL);
	xhci_dbg(xhci, "Finished mtktest_xhci_init\n");

	return retval;
}

/*-------------------------------------------------------------------------*/

/**
 * mtktest_xhci_get_endpoint_index - Used for passing endpoint bitmasks between the core and
 * HCDs.  Find the index for an endpoint given its descriptor.  Use the return
 * value to right shift 1 for the bitmask.
 *
 * Index  = (epnum * 2) + direction - 1,
 * where direction = 0 for OUT, 1 for IN.
 * For control endpoints, the IN index is used (OUT index is unused), so
 * index = (epnum * 2) + direction - 1 = (epnum * 2) + 1 - 1 = (epnum * 2)
 */
unsigned int mtktest_xhci_get_endpoint_index(struct usb_endpoint_descriptor *desc)
{
	unsigned int index;

	if (usb_endpoint_xfer_control(desc))
		index = (unsigned int) (usb_endpoint_num(desc)*2);
	else
		index = (unsigned int) (usb_endpoint_num(desc)*2) +
			(usb_endpoint_dir_in(desc) ? 1 : 0) - 1;
	return index;
}

/* Find the flag for this endpoint (for use in the control context).  Use the
 * endpoint index to create a bitmask.  The slot context is bit 0, endpoint 0 is
 * bit 1, etc.
 */
unsigned int mtktest_xhci_get_endpoint_flag_from_index(unsigned int ep_index)
{
	return 1 << (ep_index + 1);
}


/* Compute the last valid endpoint context index.  Basically, this is the
 * endpoint index plus one.  For slot contexts with more than valid endpoint,
 * we find the most significant bit set in the added contexts flags.
 * e.g. ep 1 IN (with epnum 0x81) => added_ctxs = 0b1000
 * fls(0b1000) = 4, but the endpoint context index is 3, so subtract one.
 */
unsigned int mtktest_xhci_last_valid_endpoint(u32 added_ctxs)
{
	return fls(added_ctxs) - 1;
}

static void xhci_setup_input_ctx_for_config_ep(struct xhci_hcd *xhci,
		struct xhci_container_ctx *in_ctx,
		struct xhci_container_ctx *out_ctx,
		u32 add_flags, u32 drop_flags)
{
	struct xhci_input_control_ctx *ctrl_ctx;

	ctrl_ctx = mtktest_xhci_get_input_control_ctx(xhci, in_ctx);
	ctrl_ctx->add_flags = add_flags;
	ctrl_ctx->drop_flags = drop_flags;
	mtktest_xhci_slot_copy(xhci, in_ctx, out_ctx);
	ctrl_ctx->add_flags |= SLOT_FLAG;

	xhci_dbg(xhci, "Input Context:\n");
	mtktest_xhci_dbg_ctx(xhci, in_ctx, mtktest_xhci_last_valid_endpoint(add_flags));
}

void xhci_setup_input_ctx_for_quirk(struct xhci_hcd *xhci,
		unsigned int slot_id, unsigned int ep_index,
		struct xhci_dequeue_state *deq_state)
{
	struct xhci_container_ctx *in_ctx;
	struct xhci_ep_ctx *ep_ctx;
	u32 added_ctxs;
	dma_addr_t addr;

	mtktest_xhci_endpoint_copy(xhci, xhci->devs[slot_id]->in_ctx,
			xhci->devs[slot_id]->out_ctx, ep_index);
	in_ctx = xhci->devs[slot_id]->in_ctx;
	ep_ctx = mtktest_xhci_get_ep_ctx(xhci, in_ctx, ep_index);
	addr = mtktest_xhci_trb_virt_to_dma(deq_state->new_deq_seg,
			deq_state->new_deq_ptr);
	if (addr == 0) {
		xhci_warn(xhci, "WARN Cannot submit config ep after reset ep command\n");
		xhci_warn(xhci, "WARN deq seg = %p, deq ptr = %p\n",
				deq_state->new_deq_seg,
				deq_state->new_deq_ptr);
		return;
	}
	ep_ctx->deq = addr | deq_state->new_cycle_state;

	added_ctxs = mtktest_xhci_get_endpoint_flag_from_index(ep_index);
	xhci_setup_input_ctx_for_config_ep(xhci, xhci->devs[slot_id]->in_ctx,
			xhci->devs[slot_id]->out_ctx, added_ctxs, added_ctxs);
}

/* hc interface non-used functions */
int mtktest_xhci_mtk_run(struct usb_hcd *hcd)
{

	u32 temp;
	u64 temp_64;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	void (*doorbell)(struct xhci_hcd *) = NULL;

	log_notice("mtktest_xhci_mtk_run is called\n");

	hcd->uses_new_polling = 1;
/*  hcd->poll_rh = 0;*/

	xhci_dbg(xhci, "mtktest_xhci_run\n");

#ifdef CONFIG_USB_XHCI_HCD_DEBUGGING
	init_timer(&xhci->event_ring_timer);
	xhci->event_ring_timer.data = (unsigned long) xhci;
	xhci->event_ring_timer.function = xhci_event_ring_work;
	/* Poll the event ring */
	xhci->event_ring_timer.expires = jiffies + POLL_TIMEOUT * HZ;
	xhci->zombie = 0;
	xhci_dbg(xhci, "Setting event ring polling timer\n");
	add_timer(&xhci->event_ring_timer);
#endif

	xhci_dbg(xhci, "Command ring memory map follows:\n");
	mtktest_xhci_debug_ring(xhci, xhci->cmd_ring);
	mtktest_xhci_dbg_ring_ptrs(xhci, xhci->cmd_ring);
	mtktest_xhci_dbg_cmd_ptrs(xhci);

	xhci_dbg(xhci, "ERST memory map follows:\n");
	mtktest_xhci_dbg_erst(xhci, &xhci->erst);
	xhci_dbg(xhci, "Event ring:\n");
	mtktest_xhci_debug_ring(xhci, xhci->event_ring);
	mtktest_xhci_dbg_ring_ptrs(xhci, xhci->event_ring);
	temp_64 = xhci_read_64(xhci, &xhci->ir_set->erst_dequeue);
	temp_64 &= ~ERST_PTR_MASK;
	xhci_dbg(xhci, "ERST deq = 64'h%0lx\n", (unsigned long int) temp_64);

	xhci_dbg(xhci, "// Set the interrupt modulation register\n");
	temp = xhci_readl(xhci, &xhci->ir_set->irq_control);
	temp &= ~ER_IRQ_INTERVAL_MASK;
	temp |= (u32) 160;
	xhci_writel(xhci, temp, &xhci->ir_set->irq_control);

	/* Set the HCD state before we enable the irqs */
	hcd->state = HC_STATE_RUNNING;
	temp = xhci_readl(xhci, &xhci->op_regs->command);
	temp |= (CMD_EIE);
	xhci_dbg(xhci, "// Enable interrupts, cmd = 0x%x.\n",
			temp);
	xhci_writel(xhci, temp, &xhci->op_regs->command);

	temp = xhci_readl(xhci, &xhci->ir_set->irq_pending);
	xhci_dbg(xhci, "// Enabling event ring interrupter %p by writing 0x%x to irq_pending\n",
			xhci->ir_set, (unsigned int) ER_IRQ_ENABLE(temp));
	xhci_writel(xhci, ER_IRQ_ENABLE(temp),
			&xhci->ir_set->irq_pending);
	mtktest_xhci_print_ir_set(xhci, xhci->ir_set, 0);

	if (NUM_TEST_NOOPS > 0)
		doorbell = mtktest_xhci_setup_one_noop(xhci);
#if 0
	if (xhci->quirks & XHCI_NEC_HOST)
		mtktest_xhci_queue_address_device(xhci, 0, 0, 0,
				TRB_TYPE(TRB_NEC_GET_FW));
#endif
	if (xhci_start(xhci)) {
		mtktest_xhci_halt(xhci);
		return -ENODEV;
	}

	xhci_dbg(xhci, "// @%p = 0x%x\n", &xhci->op_regs->command, temp);
	if (doorbell)
		(*doorbell)(xhci);
#if 0
	if (xhci->quirks & XHCI_NEC_HOST)
		mtktest_xhci_ring_cmd_db(xhci);
#endif

#if 0 /* USBIF*/
	mtktest_mtk_xhci_set(xhci);
	mtktest_mtk_xhci_eint_iddig_init();
#else
	mtktest_mtk_xhci_set(xhci);

	#if TEST_OTG
	mb();
	if (!g_otg_test) {
	#endif
		mtktest_enableXhciAllPortPower(xhci);
	#if TEST_OTG
	}
	#endif
#endif
	msleep(50);

/*  mtktest_disableAllClockPower(); */
	xhci_dbg(xhci, "Finished mtktest_xhci_run\n");
	return 0;
}

void mtktest_xhci_mtk_stop(struct usb_hcd *hcd)
{
	u32 temp;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

	log_notice("mtktest_xhci_mtk_stop is called\n");

/*USBIF*/
#ifdef TEST_OTG
	mtktest_disableXhciAllPortPower(xhci);
#ifdef CONFIG_USB_MTK_IDDIG
	mtktest_mtk_xhci_eint_iddig_deinit();
#endif
#endif

	spin_lock_irq(&xhci->lock);
	mtktest_xhci_halt(xhci);
	mtktest_xhci_reset(xhci);
	spin_unlock_irq(&xhci->lock);

#if 0 /* No MSI yet */
	xhci_cleanup_msix(xhci);
#endif
#ifdef CONFIG_USB_XHCI_HCD_DEBUGGING
	/* Tell the event ring poll function not to reschedule */
	xhci->zombie = 1;
	del_timer_sync(&xhci->event_ring_timer);
#endif

	xhci_dbg(xhci, "// Disabling event ring interrupts\n");
	temp = xhci_readl(xhci, &xhci->op_regs->status);
	xhci_writel(xhci, temp & ~STS_EINT, &xhci->op_regs->status);
	temp = xhci_readl(xhci, &xhci->ir_set->irq_pending);
	xhci_writel(xhci, ER_IRQ_DISABLE(temp),
			&xhci->ir_set->irq_pending);
	mtktest_xhci_print_ir_set(xhci, xhci->ir_set, 0);

	xhci_dbg(xhci, "cleaning up memory\n");
	mtktest_xhci_mem_cleanup(xhci);
	xhci_dbg(xhci, "mtktest_xhci_stop completed - status = %x\n",
			xhci_readl(xhci, &xhci->op_regs->status));
	/*mtktest_resetIP();*/
}

void mtktest_xhci_mtk_shutdown(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

	log_notice("mtktest_xhci_mtk_shutdown is called\n");

	spin_lock_irq(&xhci->lock);
	mtktest_xhci_halt(xhci);
	spin_unlock_irq(&xhci->lock);

#if 0
	xhci_cleanup_msix(xhci);
#endif

	xhci_dbg(xhci, "mtktest_xhci_shutdown completed - status = %x\n",
			xhci_readl(xhci, &xhci->op_regs->status));
}

int mtktest_xhci_mtk_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags)
{
	log_notice("mtktest_xhci_mtk_urb_enqueue is called\n");
	return 0;
}

int mtktest_xhci_mtk_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
{
	log_notice("mtktest_xhci_mtk_urb_dequeue is called\n");
	return 0;
}

int mtktest_xhci_mtk_alloc_dev(struct usb_hcd *hcd, struct usb_device *udev)
{
	log_notice("mtktest_xhci_mtk_alloc_dev is called\n");
	return 0;
}

void mtktest_xhci_mtk_free_dev(struct usb_hcd *hcd, struct usb_device *udev)
{
	log_notice("mtktest_xhci_mtk_free_dev is called\n");
}

int mtktest_xhci_mtk_alloc_streams(struct usb_hcd *hcd, struct usb_device *udev
		, struct usb_host_endpoint **eps, unsigned int num_eps,
		unsigned int num_streams, gfp_t mem_flags)
{
	log_notice("mtktest_xhci_mtk_alloc_streams is called\n");
	return 0;
}

int mtktest_xhci_mtk_free_streams(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		gfp_t mem_flags)
{
	log_notice("mtktest_xhci_mtk_free_streams is called\n");
	return 0;
}

int mtktest_xhci_mtk_add_endpoint(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint *ep)
{
	struct xhci_hcd *xhci;
	struct xhci_container_ctx *in_ctx, *out_ctx;
	unsigned int ep_index;
	struct xhci_ep_ctx *ep_ctx;
	struct xhci_slot_ctx *slot_ctx;
	struct xhci_input_control_ctx *ctrl_ctx;
	u32 added_ctxs;
	unsigned int last_ctx;
	u32 new_add_flags, new_drop_flags, new_slot_info;
	/*int ret = 0;*/
#if 0
	ret = xhci_check_args(hcd, udev, ep, 1, __func__);
	if (ret <= 0) {
		/* So we won't queue a reset ep command for a root hub */
		ep->hcpriv = NULL;
		return ret;
	}
#endif
	xhci = hcd_to_xhci(hcd);

	added_ctxs = mtktest_xhci_get_endpoint_flag(&ep->desc);
	last_ctx = mtktest_xhci_last_valid_endpoint(added_ctxs);
	if (added_ctxs == SLOT_FLAG || added_ctxs == EP0_FLAG) {
		/* FIXME when we have to issue an evaluate endpoint command to
		 * deal with ep0 max packet size changing once we get the
		 * descriptors
		 */
		xhci_dbg(xhci, "xHCI %s - can't add slot or ep 0 %#x\n",
				__func__, added_ctxs);
		return 0;
	}

	if (!xhci->devs || !xhci->devs[udev->slot_id]) {
		xhci_warn(xhci, "xHCI %s called with unaddressed device\n",
				__func__);
		return -EINVAL;
	}

	in_ctx = xhci->devs[udev->slot_id]->in_ctx;
	out_ctx = xhci->devs[udev->slot_id]->out_ctx;
	ctrl_ctx = mtktest_xhci_get_input_control_ctx(xhci, in_ctx);
	ep_index = mtktest_xhci_get_endpoint_index(&ep->desc);
	ep_ctx = mtktest_xhci_get_ep_ctx(xhci, out_ctx, ep_index);
	/* If the HCD has already noted the endpoint is enabled,
	 * ignore this request.
	 */
	if (ctrl_ctx->add_flags & mtktest_xhci_get_endpoint_flag(&ep->desc)) {
		xhci_warn(xhci, "xHCI %s called with enabled ep %p\n",
				__func__, ep);
		return 0;
	}

	/*
	 * Configuration and alternate setting changes must be done in
	 * process context, not interrupt context (or so documenation
	 * for usb_set_interface() and usb_set_configuration() claim).
	 */
	if (mtktest_xhci_endpoint_init(xhci, xhci->devs[udev->slot_id],
				udev, ep, GFP_NOIO) < 0) {
		dev_dbg(&udev->dev, "%s - could not initialize ep %#x\n",
				__func__, ep->desc.bEndpointAddress);
		return -ENOMEM;
	}

	ctrl_ctx->add_flags |= added_ctxs;
	new_add_flags = ctrl_ctx->add_flags;

	/* If xhci_endpoint_disable() was called for this endpoint, but the
	 * xHC hasn't been notified yet through the check_bandwidth() call,
	 * this re-adds a new state for the endpoint from the new endpoint
	 * descriptors.  We must drop and re-add this endpoint, so we leave the
	 * drop flags alone.
	 */
	new_drop_flags = ctrl_ctx->drop_flags;

	slot_ctx = mtktest_xhci_get_slot_ctx(xhci, in_ctx);
	/* Update the last valid endpoint context, if we just added one past */
	if ((slot_ctx->dev_info & LAST_CTX_MASK) < LAST_CTX(last_ctx)) {
		slot_ctx->dev_info &= ~LAST_CTX_MASK;
		slot_ctx->dev_info |= LAST_CTX(last_ctx);
	}
	new_slot_info = slot_ctx->dev_info;

	/* Store the usb_device pointer for later use */
	ep->hcpriv = udev;

	xhci_dbg(xhci, "add ep 0x%x, slot id %d, new drop flags = %#x, new add flags = %#x\n",
			(unsigned int) ep->desc.bEndpointAddress,
			udev->slot_id,
			(unsigned int) new_drop_flags,
			(unsigned int) new_add_flags);
	xhci_dbg(xhci, "new slot context 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n"
		, slot_ctx->dev_info, slot_ctx->dev_info2, slot_ctx->tt_info, slot_ctx->dev_state
		, slot_ctx->reserved[0], slot_ctx->reserved[1], slot_ctx->reserved[2], slot_ctx->reserved[3]);
	return 0;
}

int mtktest_xhci_mtk_drop_endpoint(struct usb_hcd *hcd, struct usb_device *udev
		, struct usb_host_endpoint *ep){
	struct xhci_hcd *xhci;
	struct xhci_container_ctx *in_ctx, *out_ctx;
	struct xhci_input_control_ctx *ctrl_ctx;
	struct xhci_slot_ctx *slot_ctx;
	unsigned int last_ctx;
	unsigned int ep_index;
	struct xhci_ep_ctx *ep_ctx;
	u32 drop_flag;
	u32 new_add_flags, new_drop_flags, new_slot_info;
	/*int ret;*/

	xhci = hcd_to_xhci(hcd);
	if (xhci->xhc_state & XHCI_STATE_DYING)
		return -ENODEV;
	xhci_dbg(xhci, "%s called for udev %p\n", __func__, udev);
	drop_flag = mtktest_xhci_get_endpoint_flag(&ep->desc);
	if (drop_flag == SLOT_FLAG || drop_flag == EP0_FLAG) {
		xhci_dbg(xhci, "xHCI %s - can't drop slot or ep 0 %#x\n",
				__func__, drop_flag);
		return 0;
	}

	in_ctx = xhci->devs[udev->slot_id]->in_ctx;
	out_ctx = xhci->devs[udev->slot_id]->out_ctx;
	ctrl_ctx = mtktest_xhci_get_input_control_ctx(xhci, in_ctx);
	ep_index = mtktest_xhci_get_endpoint_index(&ep->desc);
	ep_ctx = mtktest_xhci_get_ep_ctx(xhci, out_ctx, ep_index);

	/* If the HC already knows the endpoint is disabled,
	 * or the HCD has noted it is disabled, ignore this request
	 */
	if ((le32_to_cpu(ep_ctx->ep_info) & EP_STATE_MASK) ==
		EP_STATE_DISABLED ||
		le32_to_cpu(ctrl_ctx->drop_flags) &
		mtktest_xhci_get_endpoint_flag(&ep->desc)) {
		xhci_warn(xhci, "xHCI %s called with disabled ep %p\n",
				__func__, ep);
		return 0;
	}

	ctrl_ctx->drop_flags |= cpu_to_le32(drop_flag);
	new_drop_flags = le32_to_cpu(ctrl_ctx->drop_flags);

	ctrl_ctx->add_flags &= cpu_to_le32(~drop_flag);
	new_add_flags = le32_to_cpu(ctrl_ctx->add_flags);

	last_ctx = mtktest_xhci_last_valid_endpoint(le32_to_cpu(ctrl_ctx->add_flags));
	slot_ctx = mtktest_xhci_get_slot_ctx(xhci, in_ctx);
	/* Update the last valid endpoint context, if we deleted the last one */
	if ((le32_to_cpu(slot_ctx->dev_info) & LAST_CTX_MASK) >
		LAST_CTX(last_ctx)) {
		slot_ctx->dev_info &= cpu_to_le32(~LAST_CTX_MASK);
		slot_ctx->dev_info |= cpu_to_le32(LAST_CTX(last_ctx));
	}
	new_slot_info = le32_to_cpu(slot_ctx->dev_info);

	mtktest_xhci_endpoint_zero(xhci, xhci->devs[udev->slot_id], ep);

	xhci_dbg(xhci, "drop ep 0x%x, slot id %d, new drop flags = %#x, new add flags = %#x, new slot info = %#x\n",
			(unsigned int) ep->desc.bEndpointAddress,
			udev->slot_id,
			(unsigned int) new_drop_flags,
			(unsigned int) new_add_flags,
			(unsigned int) new_slot_info);
	return 0;
}
void mtktest_xhci_cleanup_stalled_ring(struct xhci_hcd *xhci,
		struct usb_device *udev, unsigned int ep_index)
{
	struct xhci_dequeue_state deq_state;
	struct xhci_virt_ep *ep;

	xhci_dbg(xhci, "Cleaning up stalled endpoint ring\n");
	ep = &xhci->devs[udev->slot_id]->eps[ep_index];
	/* We need to move the HW's dequeue pointer past this TD,
	 * or it will attempt to resend it on the next doorbell ring.
	 */
	mtktest_xhci_find_new_dequeue_state(xhci, udev->slot_id,
			ep_index, ep->stopped_stream, ep->stopped_td,
			&deq_state);

	/* HW with the reset endpoint quirk will use the saved dequeue state to
	 * issue a configure endpoint command later.
	 */
	if (!(xhci->quirks & XHCI_RESET_EP_QUIRK)) {
		xhci_dbg(xhci, "Queueing new dequeue state\n");
		mtktest_xhci_queue_new_dequeue_state(xhci, udev->slot_id,
				ep_index, ep->stopped_stream, &deq_state);
	} else {
		/* Better hope no one uses the input context between now and the
		 * reset endpoint completion!
		 * XXX: No idea how this hardware will react when stream rings
		 * are enabled.
		 */
		xhci_dbg(xhci, "Setting up input context for configure endpoint command\n");
		xhci_setup_input_ctx_for_quirk(xhci, udev->slot_id,
				ep_index, &deq_state);
	}
}

void mtktest_xhci_zero_in_ctx(struct xhci_hcd *xhci, struct xhci_virt_device *virt_dev)
{
	struct xhci_input_control_ctx *ctrl_ctx;
	struct xhci_ep_ctx *ep_ctx;
	struct xhci_slot_ctx *slot_ctx;
	int i;

	/* When a device's add flag and drop flag are zero, any subsequent
	 * configure endpoint command will leave that endpoint's state
	 * untouched.  Make sure we don't leave any old state in the input
	 * endpoint contexts.
	 */
	ctrl_ctx = mtktest_xhci_get_input_control_ctx(xhci, virt_dev->in_ctx);
	ctrl_ctx->drop_flags = 0;
	ctrl_ctx->add_flags = 0;
	slot_ctx = mtktest_xhci_get_slot_ctx(xhci, virt_dev->in_ctx);
	slot_ctx->dev_info &= cpu_to_le32(~LAST_CTX_MASK);
	/* Endpoint 0 is always valid */
	slot_ctx->dev_info |= cpu_to_le32(LAST_CTX(1));
	for (i = 1; i < 31; ++i) {
		ep_ctx = mtktest_xhci_get_ep_ctx(xhci, virt_dev->in_ctx, i);
		ep_ctx->ep_info = 0;
		ep_ctx->ep_info2 = 0;
		ep_ctx->deq = 0;
		ep_ctx->tx_info = 0;
	}
}


void mtktest_xhci_mtk_endpoint_reset(struct usb_hcd *hcd, struct usb_host_endpoint *ep)
{
	log_notice("mtktest_xhci_mtk_endpoint_reset is called\n");
}

int mtktest_xhci_mtk_check_bandwidth(struct usb_hcd *hcd, struct usb_device *udev)
{
	log_notice("mtktest_xhci_mtk_check_bandwidth is called\n");
	return 0;
}

void mtktest_xhci_mtk_reset_bandwidth(struct usb_hcd *hcd, struct usb_device *udev)
{
	log_notice("mtktest_xhci_mtk_reset_bandwidth is called\n");
}

int mtktest_xhci_mtk_address_device(struct usb_hcd *hcd, struct usb_device *udev)
{
	log_notice("mtktest_xhci_mtk_address_device is called\n");
	return 0;
}

int mtktest_xhci_mtk_update_hub_device(struct usb_hcd *hcd, struct usb_device *hdev,
			struct usb_tt *tt, gfp_t mem_flags)
{
	log_notice("mtktest_xhci_mtk_update_hub_device is called\n");
	return 0;
}

int mtktest_xhci_mtk_reset_device(struct usb_hcd *hcd, struct usb_device *udev)
{
	log_notice("mtktest_xhci_mtk_reset_device is called\n");
	return 0;
}

int mtktest_xhci_mtk_hub_control(struct usb_hcd *hcd, u16 typeReq, u16 wValue,
		u16 wIndex, char *buf, u16 wLength)
{
	log_notice("mtktest_xhci_mtk_hub_control is called\n");
	return 0;
}

int mtktest_xhci_mtk_hub_status_data(struct usb_hcd *hcd, char *buf)
{
	log_notice("mtktest_xhci_mtk_hub_status_data is called\n");
	return 0;
}

int mtktest_xhci_mtk_get_frame(struct usb_hcd *hcd)
{
	log_notice("mtktest_xhci_mtk_get_frame is called\n");
	return 0;
}

static void xhci_hcd_release(struct device *dev)
{
	pr_notice("xhci_hcd_release\n");
	dev->parent = NULL;
}

#if defined(CONFIG_MTK_LM_MODE)
#define MTK_XHCI_DMA_BIT_MASK DMA_BIT_MASK(64)
#else
#define MTK_XHCI_DMA_BIT_MASK DMA_BIT_MASK(32)
#endif

/*static u64 dummy_mask = MTK_XHCI_DMA_BIT_MASK;*/

static struct platform_device xhci_platform_dev = {
		.name = hcd_name,
		.id   = -1,
		.dev = {
			.coherent_dma_mask = MTK_XHCI_DMA_BIT_MASK,
			.release = xhci_hcd_release,
		},
};

#if 0
#define U3_MAC_TX_FIFO_WAIT_EMPTY_ADDR  0xf0041144

void setMacFIFOWaitEmptyValue(void)
{
		__u32 __iomem *mac_tx_fifo_wait_empty_addr;
		u32 mac_tx_fifo_wait_empty_value;

		mac_tx_fifo_wait_empty_addr = U3_MAC_TX_FIFO_WAIT_EMPTY_ADDR;
		mac_tx_fifo_wait_empty_value = 0x5;
		writel(mac_tx_fifo_wait_empty_value, mac_tx_fifo_wait_empty_addr);
}
#endif

#ifdef CONFIG_MTK_FPGA
/*
 * the arrays (eof_cnt_table, frame_ref_clk, itp_delta_ratio) must
 * correspond to sequence as below define

#define FRAME_CK_60M    0
#define FRAME_CK_20M    1
#define FRAME_CK_24M    2
#define FRAME_CK_32M    3
#define FRAME_CK_48M    4

*/
enum {
	LS_OFFSET = 0,
	LS_BANK,
	FS_OFFSET,
	FS_BANK,
	HS_SYNC_OFFSET,
	HS_SYNC_BANK,
	HS_ASYNC_OFFSET,
	HS_ASYNC_BANK,
	SS_OFFSET,
	SS_BANK,
};
static int eof_cnt_table[][10] = {
	/* LS offset, bank; FS offset, bank; HS_sync_offset, bank; HS_async_offset,bank; SS_offset,bank*/
	{171, 19, 58, 10, 0, 4, 0, 2, 150, 0}, /* 60 MHz */
	{57, 19, 19, 10, 0, 4, 0, 2, 50, 0}, /* 20MHz */
	{68, 19, 23, 10, 0, 4, 0, 2, 60, 0}, /* 24 MHz */
	{91, 19, 31, 10, 0, 4, 0, 2, 80, 0},  /* 32 MHz */
	{137, 19, 46, 10, 0, 4, 0, 2, 120, 0} /* 48 MHz */
	};

static int frame_ref_clk[] = {60, 20, 24, 32, 48};

/*
 * calculate ITP_delta_value
 * from register map
 * 60MHz: 60/60 = 5'b01000
 * 20MHz: 60/20 = 5'b11000
 * 24MHz: 60/24 = 5'b10100
 * 32MHz: 60/32 = 5'b01111
 * 48MHz: 60/48 = 5'b01010
 */
static int itp_delta_ratio[] = {0x8, 0x18, 0x14, 0xf, 0xa};

void set_frame_cnt_ck(void)
{
	u32 level1_cnt;
	__u32 __iomem *addr;
	u32 temp;

	pr_notice("%s, cur frame_ck = %dMHz\n", __func__, frame_ref_clk[FRAME_CNT_CK_VAL]);

	/*
	 * set two-level count wrapper boundary
	 * e.g. frame_ck = 60MHz
	 * total count value = 125us * 60MHz = 7500
	 * the level-2 counter wrapper boundary is bounded to 20
	 * so level-1 counter wrapper boundary = 7500 / 20 - 1 = 374
	 */

	/* u3h_writelmsk(xhci_reg + HFCNTR_CFG, (FRAME_LEVEL2_CNT - 1) << 17,
	 *	INIT_FRMCNT_LEV2_FULL_RANGE);
	 */
	addr = SSUSB_XHCI_EXCLUDE_HFCNTR_CFG;
	temp = readl(addr);
	temp &= ~(INIT_FRMCNT_LEV2_FULL_RANGE);
	temp |= ((FRAME_LEVEL2_CNT - 1) << 17);
	writel(temp, addr);

	level1_cnt = (125 * frame_ref_clk[FRAME_CNT_CK_VAL]) / FRAME_LEVEL2_CNT;
	/* double confirm integer divider */
	/* WARN_ON(level1_cnt * FRAME_LEVEL2_CNT != 125 * frame_ref_clk[FRAME_CNT_CK_VAL]);*/

	/* u3h_writelmsk(xhci_reg + HFCNTR_CFG, (level1_cnt - 1) << 8,
	 *	INIT_FRMCNT_LEV1_FULL_RANGE);
	 */
	addr = SSUSB_XHCI_EXCLUDE_HFCNTR_CFG;
	temp = readl(addr);
	temp &= ~(INIT_FRMCNT_LEV1_FULL_RANGE);
	temp |= ((level1_cnt - 1) << 8);
	writel(temp, addr);

	/* set ITP delta ratio */
	/* u3h_writelmsk(xhci_reg + HFCNTR_CFG, (itp_delta_ratio[FRAME_CNT_CK_VAL] << 1),
	 *	ITP_DELTA_CLK_RATIO);
	 */
	addr = SSUSB_XHCI_EXCLUDE_HFCNTR_CFG;
	temp = readl(addr);
	temp &= ~(ITP_DELTA_CLK_RATIO);
	temp |= (itp_delta_ratio[FRAME_CNT_CK_VAL] << 1);
	writel(temp, addr);

	/*
	 * set EOF generated point
	 * e.g., LS EOF, frame_ref_ck = 60 MHz, LS_EOF_BANK = 19, LS_EOF_OFFSET = 171
	 * when level-2 counter == 19, level-1 count == 171, HW generates LS_EOF signal
	 * the value has no special concern, just suggested by IC designer
	 * xx_BANK: alias of level-2 count value
	 * xx_OFFSET: alias of level-1 count value
	 */
	/* u3h_writelmsk(xhci_reg + LS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][LS_OFFSET],
	 *	LS_EOF_OFFSET);
	 */
	addr = SSUSB_XHCI_EXCLUDE_LS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(LS_EOF_OFFSET);
	temp |= eof_cnt_table[FRAME_CNT_CK_VAL][LS_OFFSET];
	writel(temp, addr);

	/* u3h_writelmsk(xhci_reg + LS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][LS_BANK] << 16,
	 *	LS_EOF_BANK);
	 */
	addr = SSUSB_XHCI_EXCLUDE_LS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(LS_EOF_BANK);
	temp |= (eof_cnt_table[FRAME_CNT_CK_VAL][LS_BANK] << 16);
	writel(temp, addr);

	/* u3h_writelmsk(xhci_reg + FS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][FS_OFFSET],
	 *	FS_EOF_OFFSET);
	 */
	addr = SSUSB_XHCI_EXCLUDE_FS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(FS_EOF_OFFSET);
	temp |= eof_cnt_table[FRAME_CNT_CK_VAL][FS_OFFSET];
	writel(temp, addr);

	/* u3h_writelmsk(xhci_reg + FS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][FS_BANK] << 16,
	 *	FS_EOF_BANK);
	*/
	addr = SSUSB_XHCI_EXCLUDE_FS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(FS_EOF_BANK);
	temp |= (eof_cnt_table[FRAME_CNT_CK_VAL][FS_BANK] << 16);
	writel(temp, addr);

	/* u3h_writelmsk(xhci_reg + SYNC_HS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][HS_SYNC_OFFSET],
	 *	SYNC_HS_EOF_OFFSET);
	 */
	addr = SSUSB_XHCI_EXCLUDE_SYNC_HS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(SYNC_HS_EOF_OFFSET);
	temp |= eof_cnt_table[FRAME_CNT_CK_VAL][HS_SYNC_OFFSET];
	writel(temp, addr);

	/* u3h_writelmsk(xhci_reg + SYNC_HS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][HS_SYNC_BANK] << 16,
	 *	SYNC_HS_EOF_BANK);
	 */
	addr = SSUSB_XHCI_EXCLUDE_SYNC_HS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(SYNC_HS_EOF_BANK);
	temp |= (eof_cnt_table[FRAME_CNT_CK_VAL][HS_SYNC_BANK] << 16);
	writel(temp, addr);

	/* u3h_writelmsk(xhci_reg + ASYNC_HS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][HS_ASYNC_OFFSET],
	 *	ASYNC_HS_EOF_OFFSET);
	 */
	addr = SSUSB_XHCI_EXCLUDE_ASYNC_HS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(ASYNC_HS_EOF_OFFSET);
	temp |= (eof_cnt_table[FRAME_CNT_CK_VAL][HS_ASYNC_OFFSET]);
	writel(temp, addr);

	/* u3h_writelmsk(xhci_reg + ASYNC_HS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][HS_ASYNC_BANK] << 16,
	 *	ASYNC_HS_EOF_BANK);
	 */
	addr = SSUSB_XHCI_EXCLUDE_ASYNC_HS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(ASYNC_HS_EOF_BANK);
	temp |= (eof_cnt_table[FRAME_CNT_CK_VAL][HS_ASYNC_BANK] << 16);
	writel(temp, addr);

	/* u3h_writelmsk(xhci_reg + SS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][SS_OFFSET],
	 *	SS_EOF_OFFSET);
	 */
	addr = SSUSB_XHCI_EXCLUDE_SS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(SS_EOF_OFFSET);
	temp |= eof_cnt_table[FRAME_CNT_CK_VAL][SS_OFFSET];
	writel(temp, addr);

	/* u3h_writelmsk(xhci_reg + SS_EOF, eof_cnt_table[FRAME_CNT_CK_VAL][SS_BANK] << 16,
	 *	SS_EOF_BANK);
	 */
	addr = SSUSB_XHCI_EXCLUDE_SS_EOF_CFG;
	temp = readl(addr);
	temp &= ~(SS_EOF_BANK);
	temp |= (eof_cnt_table[FRAME_CNT_CK_VAL][SS_BANK] << 16);
	writel(temp, addr);
}

#else
/*
 * XXX have suggested that IC designers shall set the default value due to ASIC clock
 * i.e., ASIC can use the default value
 */
void set_frame_cnt_ck(void) { }
#endif

static bool not_chk_frm;

/* automatilcally check frame cnt value in case that SA forgets to confirm */
int chk_frmcnt_clk(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci;
	int frame_id, frame_id_new, delta, ret;

	ret = RET_SUCCESS;
	if (not_chk_frm)
		return ret;
	not_chk_frm = true;
	xhci = hcd_to_xhci(hcd);
	frame_id = xhci_readl(xhci, &xhci->run_regs->microframe_index);
	xhci_err(xhci, "frame timing delta1\n");
	msleep(1000);
	frame_id_new = xhci_readl(xhci, &xhci->run_regs->microframe_index);
	xhci_err(xhci, "frame timing delta2\n");
	if (frame_id_new < frame_id)
		frame_id_new += 1<<14;
	delta = (frame_id_new - frame_id) >> 3;
	xhci_err(xhci, "frame timing delta = %d\n",
		delta);
	if (abs(delta-1000) > 10) {
		ret = RET_FAIL;
		xhci_err(xhci, "\n\n+++++++++\nERROR! maybe frame cnt clock is wrong!\n");
		xhci_err(xhci, "start=%d, end=%d\n+++++++++\n\n", frame_id, frame_id_new);
	}
	return ret;
}


/*initial MAC3 register, should be called after HC reset and before set PP=1 of each port*/
void mtktest_setInitialReg(void)
{
#ifdef CONFIG_MTK_FPGA

	__u32 __iomem *addr;
	u32 temp = 0;
	int num_u3_port;

	num_u3_port = SSUSB_U3_PORT_NUM(readl((void __iomem *)SSUSB_IP_CAP));

	log_notice("[OTG_H] mtktest_setInitialReg , num_u3_port = %d\n", num_u3_port);
	/* USBIF , we should enable it in real chip*/
	if (num_u3_port) {
		/*set MAC reference clock speed*/
		addr = SSUSB_U3_MAC_BASE+U3_UX_EXIT_LFPS_TIMING_PAR;
		temp = readl(addr);
		temp &= ~(0xff << U3_RX_UX_EXIT_LFPS_REF_OFFSET);
		temp |= (U3_RX_UX_EXIT_LFPS_REF << U3_RX_UX_EXIT_LFPS_REF_OFFSET);
		writel(temp, addr);
		addr = SSUSB_U3_MAC_BASE+U3_REF_CK_PAR;
		temp = readl(addr);
		temp &= ~(0xff);
		temp |= U3_REF_CK_VAL;
		writel(temp, addr);

		/*set SYS_CK*/
		addr = SSUSB_U3_SYS_BASE+U3_TIMING_PULSE_CTRL;
		temp = readl(addr);
		temp &= ~(0xff);
		temp |= MTK_CNT_1US_VALUE;
		writel(temp, addr);
	}

	addr = SSUSB_U2_SYS_BASE+USB20_TIMING_PARAMETER;
	temp &= ~(0xff);
	temp |= MTK_TIME_VALUE_1US;
	writel(temp, addr);

	/* USBIF , USBIF , we should enable it in real chip*/
	if (num_u3_port) {
		/* set LINK_PM_TIMER=3*/
		addr = SSUSB_U3_SYS_BASE+LINK_PM_TIMER;
		temp = readl(addr);
		temp &= ~(0xf);
		temp |= MTK_PM_LC_TIMEOUT_VALUE;
		writel(temp, addr);
	}

	u3phy_init();

/*  set_frame_cnt_ck();*/
#endif
}

void mtktest_setLatchSel(void)
{
	__u32 __iomem *latch_sel_addr;
	u32 latch_sel_value;

	if (g_num_u3_port <= 0)
		return;

	latch_sel_addr = U3_PIPE_LATCH_SEL_ADD;
	latch_sel_value = ((U3_PIPE_LATCH_TX)<<2) | (U3_PIPE_LATCH_RX);
	writel(latch_sel_value, latch_sel_addr);
}

void mtktest_reinitIP(void)
{
	/*__u32 __iomem *ip_reset_addr;*/
	/*u32 ip_reset_value;*/

	/* enable clockgating, include re-init IP in IPPC */
	mtktest_enableAllClockPower();
	/* set MAC3 PIPE latch */
	mtktest_setLatchSel();
	mtktest_mtk_xhci_scheduler_init();

}



int mtk_xhci_hcd_init(void)
{
	int retval = 0;
	/*__u32 __iomem *ip_reset_addr;*/
	/* u32 ip_reset_value;*/
	struct platform_device *pPlatformDev;


	log_err("Module Init start!\n");

	mtktest_reinitIP();

	retval = platform_driver_register(&xhci_versatile_driver);
	if (retval < 0) {
		log_err("Problem registering platform driver.");
		return retval;
	}

	pPlatformDev = &xhci_platform_dev;
	memset(pPlatformDev, 0, sizeof(struct platform_device));
	pPlatformDev->name = hcd_name;
	pPlatformDev->id = -1;
	pPlatformDev->dev.coherent_dma_mask = MTK_XHCI_DMA_BIT_MASK;
	pPlatformDev->dev.release = xhci_hcd_release;
#ifndef CONFIG_OF
	retval = platform_device_register(&xhci_platform_dev);
	if (retval < 0)
		platform_driver_unregister(&xhci_versatile_driver);

#endif
	log_err("Module Init success!\n");
#ifdef CONFIG_USB_MTK_DUALMODE
#if TEST_OTG
	mtktest_mtk_xhci_eint_iddig_init();
#endif
#endif
	/*
	 * Check the compiler generated sizes of structures that must be laid
	 * out in specific ways for hardware access.
	 */
	BUILD_BUG_ON(sizeof(struct xhci_doorbell_array) != 256*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_slot_ctx) != 8*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_ep_ctx) != 8*32/8);
	/* xhci_device_control has eight fields, and also
	 * embeds one xhci_slot_ctx and 31 xhci_ep_ctx
	 */
	BUILD_BUG_ON(sizeof(struct xhci_stream_ctx) != 4*32/8);
	BUILD_BUG_ON(sizeof(union xhci_trb) != 4*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_erst_entry) != 4*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_cap_regs) != 7*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_intr_reg) != 8*32/8);
	/* xhci_run_regs has eight fields and embeds 128 xhci_intr_regs */
	BUILD_BUG_ON(sizeof(struct xhci_run_regs) != (8+8*128)*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_doorbell_array) != 256*32/8);
	return 0;
}

void mtk_xhci_hcd_cleanup(void)
{
	/*uint32_t nCount;*/
	/*uint32_t i;*/
	/*struct platform_device *pPlatformDev;*/

	mtktest_mtk_xhci_eint_iddig_deinit();

	/*platform_device_unregister(&xhci_platform_dev);*/
	platform_driver_unregister(&xhci_versatile_driver);
}

