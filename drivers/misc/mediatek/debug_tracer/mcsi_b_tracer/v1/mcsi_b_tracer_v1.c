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

#include <linux/circ_buf.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <mach/mt_secure_api.h>

#include "../mcsi_b_tracer_interface.h"
#include "mcsi_b_tracer_v1.h"

void __iomem *mcsi_b_ctrl_base;

static void wait_bus_idle_and_clear(void)
{
	unsigned int ret, count_down = 1000;

	do {
		ret = mcsi_reg_read(CFG_TRACE_STS);

		if ((get_bit_at(ret, ATB_IDLE_SHIFT) == 1)
			&& (get_bit_at(ret, AXI_IDLE_SHIFT) == 1))
			break;
		udelay(1);
	} while (--count_down >= 1);

	if (count_down == 0) {
		pr_err("[mcsi-b tracer] timeout for atb/axi idle\n");
		pr_err("[mcsi-b tracer] atb idle = 0x%x\n",
			get_bit_at(ret, ATB_IDLE_SHIFT));
		pr_err("[mcsi-b tracer] axi idle = 0x%x\n",
			get_bit_at(ret, AXI_IDLE_SHIFT));
	}

	/* clear */
	mcsi_reg_clear_bitmask(((1 << ATB_IDLE_SHIFT)|(1 << AXI_IDLE_SHIFT)), CFG_TRACE_STS);
}

irqreturn_t mcsi_b_tracer_isr(void)
{
	unsigned long ret, sts;

	if (!mcsi_b_ctrl_base) {
		pr_err("%s:%d: mcsi_b_ctrl_base == NULL\n", __func__, __LINE__);
		return IRQ_HANDLED;
	}

	sts = mcsi_reg_read(CFG_INT_STS);
	pr_warn("%s:%d: CFG_INT_STS = 0x%lx\n", __func__, __LINE__, sts);

	ret = mcsi_reg_read(CFG_ERR_FLAG);
	pr_warn("%s:%d: CFG_ERR_FLAG = 0x%lx\n", __func__, __LINE__, ret);

	ret = mcsi_reg_read(CFG_ERR_FLAG2);
	pr_warn("%s:%d: CFG_ERR_FLAG2 = 0x%lx\n", __func__, __LINE__, ret);

	ret = mcsi_reg_read(CFG_TRACE_STS);
	pr_warn("%s:%d: CFG_TRACE_STS = 0x%lx\n", __func__, __LINE__, ret);

	/* XXX: not to check MCSI hang here, which will be covered by DFD */

	mcsi_reg_write(INTF_ERR|COH_ERR|IMPRECISE_ERR, CFG_INT_STS);
	if (get_bit_at(sts, 19)) {
		pr_warn("%s:%d: get trace_done\n", __func__, __LINE__);

		/* disable tracing */
		mcsi_reg_clear_bitmask(TRACE_EN, CFG_TRACE_CTRL);

		/* clear trace_done status: also waits for axi_idle and atb_idle */
		mcsi_reg_write(TRACE_STS_DONE, CFG_TRACE_STS);
		wait_bus_idle_and_clear();

		/* clear trace_done interrupt status */
		mcsi_reg_write(TRACE_DONE, CFG_INT_STS);
		pr_warn("%s:%d: CFG_TRACE_STS = 0x%x\n",
			__func__, __LINE__, mcsi_reg_read(CFG_TRACE_STS));
	}

	sts = mcsi_reg_read(CFG_INT_STS);
	pr_warn("%s:%d: leaving ISR, CFG_INT_STS = 0x%lx\n", __func__, __LINE__, sts);

	return IRQ_HANDLED;
}

static int start(struct mcsi_b_tracer_plt *plt)
{
	struct device_node *node;
	unsigned int ret;
	unsigned int args[2];

	if (!plt) {
		pr_err("%s:%d: plt == NULL\n", __func__, __LINE__);
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,mcsi_b_tracer-v1");
	if (!node) {
		pr_err("can't find compatible node for mcsi_b_tracer\n");
		return -1;
	}

	plt->base = of_iomap(node, 0);
	plt->etb_base = of_iomap(node, 1);
	plt->dem_base = of_iomap(node, 2);

	if (of_property_read_u32(node, "mediatek,mode", &ret) == 0)
		plt->mode = (ret & 0x1) ? MCSI_NORMAL_TRACE_MODE : MCSI_SNAPSHOT_MODE;
	else
		plt->mode = MCSI_SNAPSHOT_MODE;

	if (of_property_read_u32_array(node, "mediatek,trace_port", args, 2) == 0) {
		plt->trace_port.tp_sel = (unsigned char)(args[0] & 0x7);
		plt->trace_port.tp_num = (unsigned char)(args[1] & 0x7);
	}

	if (of_property_read_u32_array(node, "mediatek,set_trigger", args, 2) == 0) {
		plt->trigger_point.cfg_trigger_ctrl0 = (unsigned long)(args[0]);
		plt->trigger_point.cfg_trigger_ctrl1 = (unsigned long)(args[1]);
	}

	if (of_property_read_u32(node, "mediatek,trigger_counter", &ret) == 0)
		plt->trigger_counter = ret;
	else
		plt->trigger_counter = 0; /* set trigger counter to 0 by default */

	mcsi_b_ctrl_base = plt->base;
	ret = irq_of_parse_and_map(node, 0);
	if (!ret) {
		pr_err("%s:%d: cannot find #IRQ from device node\n", __func__, __LINE__);
		return -1;
	}

	if (request_irq(ret, (irq_handler_t)mcsi_b_tracer_isr, IRQF_TRIGGER_NONE,
			"mcsi-b tracer", NULL)) {
		pr_err("%s:%d: mcsi-b tracer IRQ line not available\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int dump_trace(void __iomem *base, void __iomem *etb_base, char **ptr)
{
	unsigned int i;
	unsigned long depth, rp, wp, ret;
	unsigned long nr_words, nr_unaligned_bytes;
	unsigned long long packet = 0;

	if (!etb_base || !base)
		return -1;

	(*ptr) += sprintf((*ptr), "--- MCSI-B Trace Control Register Dump ---\n");
	(*ptr) += sprintf((*ptr), "0x7000: 0x%08lx\n", (unsigned long)mcsi_reg_read(CFG_TRACE_CTRL));

	(*ptr) += sprintf((*ptr), "--- ETB Buffer Begin ---\n");

	CS_UNLOCK(etb_base);

	ret = readl(etb_base + ETB_STATUS);
	rp = readl(etb_base + ETB_READADDR);
	wp = readl(etb_base + ETB_WRITEADDR);

	/* depth is counted in byte */
	if (ret & 0x1)
		depth = readl(etb_base + ETB_DEPTH) << 2;
	else
		depth = CIRC_CNT(wp, rp, readl(etb_base + ETB_DEPTH) << 2);

	pr_err("[bus tracer] etb depth = 0x%lx bytes (etb status = 0x%lx, rp = 0x%lx, wp = 0x%lx\n)",
			depth, ret, rp, wp);

	if (depth == 0)
		return -1;

	nr_words = depth / 4;
	nr_unaligned_bytes = depth % 4;
	for (i = 0; i < nr_words; i++) {
		if (i % 2 == 0)
			packet = readl(etb_base + ETB_READMEM);
		else {
			packet |= ((unsigned long long) readl(etb_base + ETB_READMEM) << 32);
			pr_err("[%d]64bit: 0x%016llx\n", i/2, packet);
			(*ptr) += sprintf((*ptr), "[%d]64bit: 0x%016llx\n", i/2, packet);
		}
	}

	if (nr_unaligned_bytes != 0) {
		/* mcsi-b tracer always generate word-aligned transactions -> should not be here */
		pr_err("etb depth is unaligned!\n");
		(*ptr) += sprintf((*ptr), "etb depth is unaligned!\n");
	}

	(*ptr) += sprintf((*ptr), "--- ETB Buffer End ---\n");

	writel(0x0, etb_base + ETB_CTRL);

	writel(0x0, etb_base + ETB_TRIGGERCOUNT);
	writel(0x0, etb_base + ETB_READADDR);
	writel(0x0, etb_base + ETB_WRITEADDR);

	CS_LOCK(etb_base);
	dsb(sy);
	return 0;
}

static int dump(struct mcsi_b_tracer_plt *plt, char *buf, int len)
{
	if (plt->recording) {
		pr_err("[mcsi-b tracer] error: tracer is running\n"
				"-> you must pause it before dumping!\n");
		buf += sprintf(buf, "[mcsi-b tracer] error: tracer is running\n"
				"-> you must pause it before dumping!\n");
		return -1;
	}

	return dump_trace(plt->base, plt->etb_base, &buf);
}

/*
 * if $force=0: enable only if plt->enabled
 * if $force=1: enable if $enable, disable otherwise
 */
static int setup(struct mcsi_b_tracer_plt *plt, unsigned char force, unsigned char enable)
{
	unsigned long ret;

	if (!plt) {
		pr_err("%s:%d: plt == NULL\n", __func__, __LINE__);
		return -1;
	}

	if (!plt->base) {
		pr_err("%s:%d: base == NULL\n", __func__, __LINE__);
		return -1;
	}

	if (!plt->etb_base) {
		pr_err("%s:%d: etb_base == NULL\n", __func__, __LINE__);
		return -1;
	}

	if (!plt->dem_base) {
		pr_err("%s:%d: dem_base == NULL\n", __func__, __LINE__);
		return -1;
	}

	if (force)
		plt->enabled = (enable) ? 1 : 0;

	/* enable ATB clk */
	writel(0x1, plt->dem_base + DEM_ATB_CLK);
	dsb(sy);

	/* enable ETB */
	writel(0xc5acce55, plt->etb_base + ETB_LAR);
	if (readl(plt->etb_base + ETB_LSR) != 0x1)
		pr_err("[mcsi-b tracer] unlock etb error\n");

	writel(0x0, plt->etb_base + ETB_FFSR);

	/* set trigger counter */
	ret = readl(plt->etb_base + ETB_DEPTH);
	if (plt->trigger_counter > ret) {
		pr_warn("[mcsi-b tracer] input trigger_counter is larger than ETB size (0x%lx)\n"
			"[mcsi-b tracer] set trigger_counter to the ETB size instead\n",
		ret);
		plt->trigger_counter = ret - 1;
	}
	writel(plt->trigger_counter, plt->etb_base + ETB_TRIGGERCOUNT);

	/* set formatter and flush control */
	writel(FCCR_FLUSH_ON_TRIGGER|FCCR_STOP_FLUSH, plt->etb_base + ETB_FFCR);

	writel(0x1, plt->etb_base + ETB_CTRL);
	dsb(sy);

	/* enable tracer */
	if (plt->enabled) {
		if (plt->mode == MCSI_SNAPSHOT_MODE) {
			/* snapshot mode */
			mcsi_reg_set_bitmask(DUMP_DBG_BY_WD_TO, CFG_TRACE_CTRL);
			mcsi_reg_clear_bitmask(TRACE_EN, CFG_TRACE_CTRL);
			/* enable interrupt */
			mcsi_reg_set_bitmask(DUMP_DBG_BY_WD_TO, CFG_INT_EN);
			mcsi_reg_clear_bitmask(TRACE_DONE, CFG_INT_EN);
		} else {
			/* normal trace mode */
			mcsi_reg_clear_bitmask(0x7E, CFG_TRACE_CTRL);
			mcsi_reg_set_bitmask(((plt->trace_port.tp_sel & 0x7) << TP_SEL_SHIFT)
				|((plt->trace_port.tp_num & 0x7) << TP_NUM_SHIFT)|
				VALID_XACT_ONLY, CFG_TRACE_CTRL);

			/* setup trigger point: CFG_TRIGGER_CTRL0 */
			mcsi_reg_write(plt->trigger_point.cfg_trigger_ctrl0, CFG_TRIGGER_CTRL0);
			/* setup trigger point: CFG_TRIGGER_CTRL1 */
			mcsi_reg_write(plt->trigger_point.cfg_trigger_ctrl1, CFG_TRIGGER_CTRL1);

			/* setup trigger point: addr match, other match */
			mcsi_reg_write(plt->trigger_point.tri_lo_addr, CFG_TRACE_TRI_LOW_ADDR);
			mcsi_reg_write(plt->trigger_point.tri_hi_addr, CFG_TRACE_TRI_HIGH_ADDR);
			mcsi_reg_write(plt->trigger_point.tri_lo_addr_mask, CFG_TRACE_TRI_LOW_MASK);
			mcsi_reg_write(plt->trigger_point.tri_hi_addr_mask, CFG_TRACE_TRI_HIGH_MASK);
			mcsi_reg_write(plt->trigger_point.tri_other_match,
					CFG_TRACE_TRI_OTHER_MATCH_FIELD);
			mcsi_reg_write(plt->trigger_point.tri_other_match_mask,
					CFG_TRACE_TRI_OTHER_MATCH_MASK);

			/* enable interrupt */
			mcsi_reg_set_bitmask(COH_ERR|TRACE_DONE, CFG_INT_EN);

			/* enable trace */
			mcsi_reg_set_bitmask(TRACE_EN, CFG_TRACE_CTRL);

			plt->recording = 1;
		}
	}
	dsb(sy);

	return 0;
}

static int set_port(struct mcsi_b_tracer_plt *plt, struct mcsi_trace_port port)
{
	if (!plt) {
		pr_err("%s:%d: plt == NULL\n", __func__, __LINE__);
		return -1;
	}

	if (!plt->enabled)
		return -1;

	if (plt->mode == MCSI_SNAPSHOT_MODE)
		return -1;

	plt->trace_port = port;
	mcsi_reg_clear_bitmask(0x7E, CFG_TRACE_CTRL);
	mcsi_reg_set_bitmask(((port.tp_sel & 0x7) << TP_SEL_SHIFT)
		|((port.tp_num & 0x7) << TP_NUM_SHIFT), CFG_TRACE_CTRL);

	return 0;
}


static int set_trigger(struct mcsi_b_tracer_plt *plt, struct mcsi_trace_trigger_point trigger)
{
	if (!plt) {
		pr_err("%s:%d: plt == NULL\n", __func__, __LINE__);
		return -1;
	}

	if (!plt->enabled)
		return -1;

	if (plt->mode == MCSI_SNAPSHOT_MODE)
		return -1;

	mcsi_reg_write(trigger.cfg_trigger_ctrl0, CFG_TRIGGER_CTRL0);
	mcsi_reg_write(trigger.cfg_trigger_ctrl1, CFG_TRIGGER_CTRL1);
	mcsi_reg_write(trigger.tri_lo_addr, CFG_TRACE_TRI_LOW_ADDR);
	mcsi_reg_write(trigger.tri_hi_addr, CFG_TRACE_TRI_HIGH_ADDR);
	mcsi_reg_write(trigger.tri_lo_addr_mask, CFG_TRACE_TRI_LOW_MASK);
	mcsi_reg_write(trigger.tri_hi_addr_mask, CFG_TRACE_TRI_HIGH_MASK);
	mcsi_reg_write(trigger.tri_other_match, CFG_TRACE_TRI_OTHER_MATCH_FIELD);
	mcsi_reg_write(trigger.tri_other_match_mask, CFG_TRACE_TRI_OTHER_MATCH_MASK);

	return 0;
}

static int set_filter(struct mcsi_b_tracer_plt *plt, struct mcsi_trace_filter f)
{
	if (!plt) {
		pr_err("%s:%d: plt == NULL\n", __func__, __LINE__);
		return -1;
	}

	if (!plt->enabled)
		return -1;

	if (plt->mode == MCSI_SNAPSHOT_MODE)
		return -1;

	mcsi_reg_write(f.low_content, CFG_TRACE_FILTER_LOW_CONTENT_FILED);
	mcsi_reg_write(f.high_content, CFG_TRACE_FILTER_HIGH_CONTENT_FILED);
	mcsi_reg_write(f.low_content_enable, CFG_TRACE_FILTER_LOW_CONTENT_MASK);
	mcsi_reg_write(f.high_content_enable, CFG_TRACE_FILTER_HIGH_CONTENT_MASK);

	return 0;
}

static int set_recording(struct mcsi_b_tracer_plt *plt, unsigned char pause)
{
	if (!plt) {
		pr_err("%s:%d: plt == NULL\n", __func__, __LINE__);
		return -1;
	}

	if (!plt->enabled)
		return -1;

	if (plt->mode == MCSI_SNAPSHOT_MODE)
		return -1;

	if (pause) {
		/* disable tracer */
		mcsi_reg_clear_bitmask(TRACE_EN, CFG_TRACE_CTRL);

		/* disable trace_done interrupt */
		mcsi_reg_clear_bitmask(TRACE_DONE, CFG_INT_EN);

		/* disable etb */
		writel(0x0, plt->etb_base + ETB_CTRL);
		dsb(sy);

		plt->recording = 0;
	} else {
		/* enable ETB */
		writel(0x1, plt->etb_base + ETB_CTRL);
		dsb(sy);

		/* enable trace_done interrupt */
		mcsi_reg_set_bitmask(TRACE_DONE, CFG_INT_EN);

		/* enable tracer */
		mcsi_reg_set_bitmask(TRACE_EN, CFG_TRACE_CTRL);

		plt->recording = 1;
	}

	return 0;
}


static int dump_setting(struct mcsi_b_tracer_plt *plt, char *buf, int len)
{
	unsigned long ret;
	int i;

	if (!plt) {
		pr_err("%s:%d: plt == NULL\n", __func__, __LINE__);
		return -1;
	}

	buf += sprintf(buf, "== dump setting of mcsi-b tracer ==\n");


	if (plt->mode == MCSI_NORMAL_TRACE_MODE) {
		buf += sprintf(buf, "mode = NORMAL TRACE\n");
		ret = mcsi_reg_read(CFG_TRACE_CTRL);
		buf += sprintf(buf, "trace_port.tp_sel = 0x%lx\ntrace_port.tp_num = 0x%lx\n",
			(ret >> TP_SEL_SHIFT) & 0x7, (ret >> TP_NUM_SHIFT) & 0x7);
		buf += sprintf(buf, "trace recording = %x\n", plt->recording);

		buf += sprintf(buf, "--------------------\ntrigger point setting:\n");
		ret = mcsi_reg_read(CFG_TRIGGER_CTRL0);
		buf += sprintf(buf, "cfg_trigger_ctrl0 = 0x%lx\n", ret);
		buf += sprintf(buf, "cfg_trigger_ctrl1 = 0x%x\n", mcsi_reg_read(CFG_TRIGGER_CTRL1));
		for (i = 0; i <= MCSI_TRACE_BP_NUM-1; ++i)
			buf += sprintf(buf, "break point %d type: %s(0x%lx)\n",
				i, tri_sel_to_type[(ret >> (TRIGGER_TRI0_SEL_SHIFT + i*4)) & 0x7],
				(ret >> (TRIGGER_TRI0_SEL_SHIFT + i*4)) & 0x7);
		if (((ret << TRIGGER_MODE_SHIFT) & 0x1) == 0) {
			/* AND/OR mode */
			buf += sprintf(buf, "trigger_point.mode = AND/OR\n");
			ret = mcsi_reg_read(CFG_TRIGGER_CTRL1);
			for (i = 0; i <= MCSI_TRACE_BP_NUM-1; ++i)
				buf += sprintf(buf, "trigger_point.and_gate[%d] = 0x%lx\n",
					i, (ret >> (TRIGGER_AND0_SHIFT + i*4)) & 0xf);
			buf += sprintf(buf, "trigger_point.or_gate = 0x%lx\n",
					(ret >> TRIGGER_OR_SHIFT) & 0xf);
		 } else {
			/* Sequence mode */
			buf += sprintf(buf, "trigger_point.mode = Sequence\n");
			buf += sprintf(buf, "trigger_point.link = 0x%lx\n",
				(ret >> TRIGGER_LINK_SHIFT) & 0xf);
		 }

		 buf += sprintf(buf, "trigger_point.tri_lo_addr = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_TRI_LOW_ADDR));
		 buf += sprintf(buf, "trigger_point.tri_hi_addr = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_TRI_HIGH_ADDR));
		 buf += sprintf(buf, "trigger_point.tri_lo_addr_mask = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_TRI_LOW_MASK));
		 buf += sprintf(buf, "trigger_point.tri_hi_addr_mask = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_TRI_HIGH_MASK));
		 buf += sprintf(buf, "trigger_point.tri_other_match = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_TRI_OTHER_MATCH_FIELD));
		 buf += sprintf(buf, "trigger_point.tri_other_match_mask = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_TRI_OTHER_MATCH_MASK));

		 buf += sprintf(buf, "trigger_point.trace_filter_low_content = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_FILTER_LOW_CONTENT_FILED));
		 buf += sprintf(buf, "trigger_point.trace_filter_high_content = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_FILTER_HIGH_CONTENT_FILED));
		 buf += sprintf(buf, "trigger_point.trace_filter_low_content_mask = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_FILTER_LOW_CONTENT_MASK));
		 buf += sprintf(buf, "trigger_point.trace_filter_high_content_mask = 0x%x\n",
			mcsi_reg_read(CFG_TRACE_FILTER_HIGH_CONTENT_MASK));
	} else {
		buf += sprintf(buf, "mode = SNAPSHOT\n");
	}
	buf += sprintf(buf, "CFG_INT_EN = 0x%x\n",
			mcsi_reg_read(CFG_INT_EN));
	buf += sprintf(buf, "ETB_TRG = 0x%x\n",
			readl(plt->etb_base + ETB_TRIGGERCOUNT));
	buf += sprintf(buf, "enabled = %x\n", plt->enabled);

	return 0;
}

static struct mcsi_b_tracer_plt_operations mcsi_b_tracer_ops = {
	.dump = dump,
	.start = start,
	.setup = setup,
	.set_recording = set_recording,
	.set_filter = set_filter,
	.set_trigger = set_trigger,
	.set_port = set_port,
	.dump_setting = dump_setting,
};

static int __init mcsi_b_tracer_init(void)
{
	struct mcsi_b_tracer_plt *plt = NULL;
	int ret = 0;

	plt = kzalloc(sizeof(struct mcsi_b_tracer_plt), GFP_KERNEL);
	if (!plt)
		return -ENOMEM;

	plt->ops = &mcsi_b_tracer_ops;
	plt->min_buf_len = 8192; /* 8K */

	ret = mcsi_b_tracer_register(plt);
	if (ret) {
		pr_err("%s:%d: mcsi_b_tracer_register failed\n", __func__, __LINE__);
		goto register_mcsi_b_tracer_err;
	}

	return 0;

register_mcsi_b_tracer_err:
	kfree(plt);
	return ret;
}

core_initcall(mcsi_b_tracer_init);
