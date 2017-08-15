/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
/*#ifndef CONFIG_OF
#include <mach/mt_reg_base.h>
#endif*/
#include "6755_gpio.h"
#include <mt-plat/mt_gpio.h>
#include <mt-plat/mt_gpio_core.h>
/*#include <mach/dcl_sim_gpio.h>*/

/*----------------------------------------------------------------------------*/
typedef struct {		/*FIXME: check GPIO spec */
	unsigned int no:16;
	unsigned int mode:3;
	unsigned int pullsel:1;
	unsigned int din:1;
	unsigned int dout:1;
	unsigned int pullen:1;
	unsigned int dir:1;
/*    unsigned int dinv   : 1;*/
	unsigned int ies:1;
	unsigned int _align:7;
} GPIO_CFG;

/* #define MAX_GPIO_REG_BITS      16 */
/* #define MAX_GPIO_MODE_PER_REG  5 */
/* #define GPIO_MODE_BITS         3 */
/******************************************************************************
*clock out module
*******************************************************************************/
int mt_set_clock_output(unsigned long num, unsigned long src, unsigned long div)
{
	GPIOERR("GPIO CLKM module not be implement any more!\n");
	return RSUCCESS;
}

int mt_get_clock_output(unsigned long num, unsigned long *src, unsigned long *div)
{
	GPIOERR("GPIO CLKM module not be implement any more!\n");
	return RSUCCESS;
}

/*****************************************************************************/
/* sysfs operation                                                           */
/*****************************************************************************/


/*----------------------------------------------------------------------------
void mt_gpio_load_ext(GPIOEXT_REGS *regs)
{
    GPIOEXT_REGS *pReg = (GPIOEXT_REGS*)(GPIOEXT_BASE);
    int idx;

    if (!regs)
	GPIOERR("%s: null pointer\n", __func__);
    memset(regs, 0x00, sizeof(*regs));
    for (idx = 0; idx < sizeof(pReg->dir)/sizeof(pReg->dir[0]); idx++)
	regs->dir[idx].val = GPIOEXT_RD(&pReg->dir[idx]);
    for (idx = 0; idx < sizeof(pReg->pullen)/sizeof(pReg->pullen[0]); idx++)
	regs->pullen[idx].val = GPIOEXT_RD(&pReg->pullen[idx]);
    for (idx = 0; idx < sizeof(pReg->pullsel)/sizeof(pReg->pullsel[0]); idx++)
	regs->pullsel[idx].val =GPIOEXT_RD(&pReg->pullsel[idx]);
    for (idx = 0; idx < sizeof(pReg->dinv)/sizeof(pReg->dinv[0]); idx++)
	regs->dinv[idx].val =GPIOEXT_RD(&pReg->dinv[idx]);
    for (idx = 0; idx < sizeof(pReg->dout)/sizeof(pReg->dout[0]); idx++)
	regs->dout[idx].val = GPIOEXT_RD(&pReg->dout[idx]);
    for (idx = 0; idx < sizeof(pReg->mode)/sizeof(pReg->mode[0]); idx++)
	regs->mode[idx].val = GPIOEXT_RD(&pReg->mode[idx]);
    for (idx = 0; idx < sizeof(pReg->din)/sizeof(pReg->din[0]); idx++)
	regs->din[idx].val = GPIOEXT_RD(&pReg->din[idx]);
}
EXPORT_SYMBOL(mt_gpio_load_ext);
----------------------------------------------------------------------------*/
void mt_gpio_load_base(GPIO_REGS *regs)
{
	GPIO_REGS *pReg = (GPIO_REGS *) (GPIO_BASE);
	int idx;

	if (!regs) {
		GPIOERR("%s: null pointer\n", __func__);
		return;
	}
	memset(regs, 0x00, sizeof(*regs));
	for (idx = 0; idx < sizeof(pReg->dir) / sizeof(pReg->dir[0]); idx++)
		regs->dir[idx].val = __raw_readl(&pReg->dir[idx]);
	/* for (idx = 0; idx < sizeof(pReg->ies)/sizeof(pReg->ies[0]); idx++) */
	/* regs->ies[idx].val = __raw_readl(&pReg->ies[idx]); */
	/* for (idx = 0; idx < sizeof(pReg->pullen)/sizeof(pReg->pullen[0]); idx++) */
	/* regs->pullen[idx].val = __raw_readl(&pReg->pullen[idx]); */
	/* for (idx = 0; idx < sizeof(pReg->pullsel)/sizeof(pReg->pullsel[0]); idx++) */
	/* regs->pullsel[idx].val =__raw_readl(&pReg->pullsel[idx]); */
	/* for (idx = 0; idx < sizeof(pReg->dinv)/sizeof(pReg->dinv[0]); idx++) */
	/* regs->dinv[idx].val =__raw_readl(&pReg->dinv[idx]); */
	for (idx = 0; idx < sizeof(pReg->dout) / sizeof(pReg->dout[0]); idx++)
		regs->dout[idx].val = __raw_readl(&pReg->dout[idx]);
	for (idx = 0; idx < sizeof(pReg->mode) / sizeof(pReg->mode[0]); idx++)
		regs->mode[idx].val = __raw_readl(&pReg->mode[idx]);
	for (idx = 0; idx < sizeof(pReg->din) / sizeof(pReg->din[0]); idx++)
		regs->din[idx].val = __raw_readl(&pReg->din[idx]);
}

/* EXPORT_SYMBOL(mt_gpio_load_base); */
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/
void mt_gpio_dump_base(GPIO_REGS *regs)
{
	GPIO_REGS *cur = NULL;
	int idx;

	GPIOMSG("%s\n", __func__);
	if (regs == NULL) {	/*if arg is null, load & dump; otherwise, dump only */
		cur = kzalloc(sizeof(*cur), GFP_KERNEL);
		if (cur == NULL) {
			GPIOERR("null pointer\n");
			return;
		}
		regs = cur;
		mt_gpio_load_base(regs);
		GPIOMSG("dump current: %p\n", regs);
	} else {
		GPIOMSG("dump %p ...\n", regs);
	}

	GPIOMSG("---# dir #-----------------------------------------------------------------\n");
#ifdef CONFIG_64BIT
	GPIOMSG("Offset 0x%lx\n", (void *)(&regs->dir[0]) - (void *)regs);
#else
	GPIOMSG("Offset 0x%04X\n", (void *)(&regs->dir[0]) - (void *)regs);
#endif
	for (idx = 0; idx < sizeof(regs->dir) / sizeof(regs->dir[0]); idx++) {
		GPIOMSG("0x%04X ", regs->dir[idx].val);
		if (7 == (idx % 8))
			GPIOMSG("\n");
	}
	/* GPIOMSG("\n---# ies #-----------------------------------------------------------------\n"); */
	/* GPIOMSG("Offset 0x%04X\n",(void *)(&regs->ies[0])-(void *)regs); */
	/* for (idx = 0; idx < sizeof(regs->ies)/sizeof(regs->ies[0]); idx++) { */
	/* GPIOMSG("0x%04X ", regs->ies[idx].val); */
	/* if (7 == (idx % 8)) GPIOMSG("\n"); */
	/* } */
	/* GPIOMSG("\n---# pullen #--------------------------------------------------------------\n"); */
	/* GPIOMSG("Offset 0x%04X\n",(void *)(&regs->pullen[0])-(void *)regs); */
	/* for (idx = 0; idx < sizeof(regs->pullen)/sizeof(regs->pullen[0]); idx++) { */
	/* GPIOMSG("0x%04X ", regs->pullen[idx].val); */
	/* if (7 == (idx % 8)) GPIOMSG("\n"); */
	/* } */
	/* GPIOMSG("\n---# pullsel #-------------------------------------------------------------\n"); */
	/* GPIOMSG("Offset 0x%04X\n",(void *)(&regs->pullsel[0])-(void *)regs); */
	/* for (idx = 0; idx < sizeof(regs->pullsel)/sizeof(regs->pullsel[0]); idx++) { */
	/* GPIOMSG("0x%04X ", regs->pullsel[idx].val); */
	/* if (7 == (idx % 8)) GPIOMSG("\n"); */
	/* } */
	/* GPIOMSG("\n---# dinv #----------------------------------------------------------------\n"); */
	/* GPIOMSG("Offset 0x%04X\n",(void *)(&regs->dinv[0])-(void *)regs); */
	/* for (idx = 0; idx < sizeof(regs->dinv)/sizeof(regs->dinv[0]); idx++) { */
	/* GPIOMSG("0x%04X ", regs->dinv[idx].val); */
	/* if (7 == (idx % 8)) GPIOMSG("\n"); */
	/* } */
	GPIOMSG("\n---# dout #----------------------------------------------------------------\n");
#ifdef CONFIG_64BIT
	GPIOMSG("Offset 0x%lx\n", (void *)(&regs->dout[0]) - (void *)regs);
#else
	GPIOMSG("Offset 0x%04X\n", (void *)(&regs->dout[0]) - (void *)regs);
#endif
	for (idx = 0; idx < sizeof(regs->dout) / sizeof(regs->dout[0]); idx++) {
		GPIOMSG("0x%04X ", regs->dout[idx].val);
		if (7 == (idx % 8))
			GPIOMSG("\n");
	}
	GPIOMSG("\n---# din  #----------------------------------------------------------------\n");
#ifdef CONFIG_64BIT
	GPIOMSG("Offset 0x%lx\n", (void *)(&regs->din[0]) - (void *)regs);
#else
	GPIOMSG("Offset 0x%04X\n", (void *)(&regs->din[0]) - (void *)regs);
#endif
	for (idx = 0; idx < sizeof(regs->din) / sizeof(regs->din[0]); idx++) {
		GPIOMSG("0x%04X ", regs->din[idx].val);
		if (7 == (idx % 8))
			GPIOMSG("\n");
	}
	GPIOMSG("\n---# mode #----------------------------------------------------------------\n");
#ifdef CONFIG_64BIT
	GPIOMSG("Offset 0x%lx\n", (void *)(&regs->mode[0]) - (void *)regs);
#else
	GPIOMSG("Offset 0x%04X\n", (void *)(&regs->mode[0]) - (void *)regs);
#endif
	for (idx = 0; idx < sizeof(regs->mode) / sizeof(regs->mode[0]); idx++) {
		GPIOMSG("0x%04X ", regs->mode[idx].val);
		if (7 == (idx % 8))
			GPIOMSG("\n");
	}
	GPIOMSG("\n---------------------------------------------------------------------------\n");


	if (cur != NULL)
		kfree(cur);

}

/*----------------------------------------------------------------------------*/

void mt_gpio_dump(void)
{
	mt_gpio_dump_base(NULL);
	/* mt_gpio_dump_ext(NULL); */
}

/*----------------------------------------------------------------------------*/
void gpio_dump_regs(void)
{
	int idx = 0;

	GPIOMSG("PIN: [MODE] [PULL_SEL] [DIN] [DOUT] [PULL EN] [DIR] [IES]\n");
	for (idx = MT_GPIO_BASE_START; idx < MT_GPIO_BASE_MAX; idx++) {
		pr_debug("idx = %3d: %d %d %d %d %d %d %d\n",
		       idx, mt_get_gpio_mode_base(idx), mt_get_gpio_pull_select_base(idx),
		       mt_get_gpio_in_base(idx), mt_get_gpio_out_base(idx),
		       mt_get_gpio_pull_enable_base(idx), mt_get_gpio_dir_base(idx),
		       mt_get_gpio_ies_base(idx));
	}
}

/* EXPORT_SYMBOL(gpio_dump_regs); */
/*----------------------------------------------------------------------------*/
/*
static ssize_t mt_gpio_dump_regs(char *buf, ssize_t bufLen)
{
    int idx = 0, len = 0;
	char tmp[]="PIN: [MODE] [PULL_SEL] [DIN] [DOUT] [PULL EN] [DIR] [INV] [IES]\n";
	len += snprintf(buf+len, bufLen-len, "%s",tmp);
    for (idx = 0; idx < MT_GPIO_MAX_PIN; idx++) {
		len += snprintf(buf+len, bufLen-len, "%3d:%d%d%d%d%d%d%d%d\n",
		   idx,mt_get_gpio_mode(idx), mt_get_gpio_pull_select(idx), mt_get_gpio_in(idx),mt_get_gpio_out(idx),
		   mt_get_gpio_pull_enable(idx),mt_get_gpio_dir(idx),mt_get_gpio_inversion(idx),mt_get_gpio_ies(idx));
    }
    return len;
}
*/
/*---------------------------------------------------------------------------*/
static void mt_gpio_read_pin_base(GPIO_CFG *cfg, int method)
{
	if (method == 0) {
		GPIO_REGS *cur = (GPIO_REGS *) GPIO_BASE;
		u32 mask = (1L << GPIO_MODE_BITS) - 1;
		int num, bit;

		num = cfg->no / MAX_GPIO_REG_BITS;
		bit = cfg->no % MAX_GPIO_REG_BITS;
		if (cfg->no < MT_GPIO_BASE_MAX) {
			/* cfg->pullsel= (cur->pullsel[num].val & (1L << bit)) ? (1) : (0); */
			cfg->din = (cur->din[num].val & (1L << bit)) ? (1) : (0);
			cfg->dout = (cur->dout[num].val & (1L << bit)) ? (1) : (0);
			/* cfg->pullen = (cur->pullen[num].val & (1L << bit)) ? (1) : (0); */
			cfg->dir = (cur->dir[num].val & (1L << bit)) ? (1) : (0);
			/* cfg->dinv   = (cur->dinv[num].val & (1L << bit)) ? (1) : (0); */
			num = cfg->no / MAX_GPIO_MODE_PER_REG;
			bit = cfg->no % MAX_GPIO_MODE_PER_REG;
			cfg->mode = (cur->mode[num].val >> (GPIO_MODE_BITS * bit)) & mask;
		}
	} else if (method == 1) {
		/* cfg->pullsel= mt_get_gpio_pull_select(cfg->no); */
		cfg->din = mt_get_gpio_in(cfg->no);
		cfg->dout = mt_get_gpio_out(cfg->no);
		/* cfg->pullen = mt_get_gpio_pull_enable(cfg->no); */
		cfg->dir = mt_get_gpio_dir(cfg->no);
		/* cfg->dinv   = mt_get_gpio_inversion(cfg->no); */
		/* cfg->ies    = mt_get_gpio_ies(cfg->no); */
		cfg->mode = mt_get_gpio_mode(cfg->no);
	}
}

/*
static void mt_gpio_read_pin_ext(GPIO_CFG* cfg, int method)
{
    if (method == 0) {
	GPIOEXT_REGS *cur = (GPIOEXT_REGS*)GPIOEXT_BASE;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;
	int num, bit;
		num = cfg->no / MAX_GPIO_REG_BITS;
		bit = cfg->no % MAX_GPIO_REG_BITS;
		if(cfg->no >= MT_GPIO_EXT_START){
			//
			cfg->pullsel= (cur->pullsel[num].val & (1L << bit)) ? (1) : (0);
			cfg->din    = (cur->din[num].val & (1L << bit)) ? (1) : (0);
			cfg->dout   = (cur->dout[num].val & (1L << bit)) ? (1) : (0);
			cfg->pullen = (cur->pullen[num].val & (1L << bit)) ? (1) : (0);
			cfg->dir    = (cur->dir[num].val & (1L << bit)) ? (1) : (0);
			cfg->dinv   = (cur->dinv[num].val & (1L << bit)) ? (1) : (0);
			num = cfg->no / MAX_GPIO_MODE_PER_REG;
			bit = cfg->no % MAX_GPIO_MODE_PER_REG;
			cfg->mode   = (cur->mode[num].val >> (GPIO_MODE_BITS*bit)) & mask;

		}
    } else if (method == 1) {
	cfg->pullsel= mt_get_gpio_pull_select(cfg->no);
	cfg->din    = mt_get_gpio_in(cfg->no);
	cfg->dout   = mt_get_gpio_out(cfg->no);
	cfg->pullen = mt_get_gpio_pull_enable(cfg->no);
	cfg->dir    = mt_get_gpio_dir(cfg->no);
	cfg->dinv   = mt_get_gpio_inversion(cfg->no);
	cfg->mode   = mt_get_gpio_mode(cfg->no);
    }
}*/

static ssize_t mt_gpio_dump_addr_base(void)
{
	int idx;
	GPIO_REGS *reg = (GPIO_REGS *) GPIO_BASE;

	GPIOMSG("# direction\n");
	for (idx = 0; idx < sizeof(reg->dir) / sizeof(reg->dir[0]); idx++)
		GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->dir[idx].val, idx,
			&reg->dir[idx].set, idx, &reg->dir[idx].rst);
	/* GPIOMSG("# ies\n"); */
	/* for (idx = 0; idx < sizeof(reg->ies)/sizeof(reg->ies[0]); idx++) */
	/* GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->ies[idx].val,
idx, &reg->ies[idx].set, idx, &reg->ies[idx].rst); */
	/* GPIOMSG("# pull enable\n"); */
	/* for (idx = 0; idx < sizeof(reg->pullen)/sizeof(reg->pullen[0]); idx++) */
	/* GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->pullen[idx].val,
idx, &reg->pullen[idx].set, idx, &reg->pullen[idx].rst); */
	/* GPIOMSG("# pull select\n"); */
	/* for (idx = 0; idx < sizeof(reg->pullsel)/sizeof(reg->pullsel[0]); idx++) */
	/* GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->pullsel[idx].val,
idx, &reg->pullsel[idx].set, idx, &reg->pullsel[idx].rst); */
	/* GPIOMSG("# data inversion\n"); */
	/* for (idx = 0; idx < sizeof(reg->dinv)/sizeof(reg->dinv[0]); idx++) */
	/* GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->dinv[idx].val,
idx, &reg->dinv[idx].set, idx, &reg->dinv[idx].rst); */
	GPIOMSG("# data output\n");
	for (idx = 0; idx < sizeof(reg->dout) / sizeof(reg->dout[0]); idx++)
		GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->dout[idx].val, idx,
			&reg->dout[idx].set, idx, &reg->dout[idx].rst);
	GPIOMSG("# data input\n");
	for (idx = 0; idx < sizeof(reg->din) / sizeof(reg->din[0]); idx++)
		GPIOMSG("val[%2d] %p\n", idx, &reg->din[idx].val);
	GPIOMSG("# mode\n");
	for (idx = 0; idx < sizeof(reg->mode) / sizeof(reg->mode[0]); idx++)
		GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->mode[idx].val, idx,
			&reg->mode[idx].set, idx, &reg->mode[idx].rst);
	return 0;
}

/*---------------------------------------------------------------------------
static void mt_gpio_compare_ext(void)
{
    int idx;
    GPIOEXT_REGS *reg = (GPIOEXT_REGS*)GPIOEXT_BASE;
    GPIOEXT_REGS *cur = kzalloc(sizeof(*cur), GFP_KERNEL);

    if (!cur)
	return;

    mt_gpio_load_ext(cur);
    for (idx = 0; idx < sizeof(reg->dir)/sizeof(reg->dir[0]); idx++)
	if (reg->dir[idx].val != cur->dir[idx].val)
	    GPIOERR("GPIOEXT mismatch dir[%2d]: %x <> %x\n", idx, reg->dir[idx].val, cur->dir[idx].val);
    for (idx = 0; idx < sizeof(reg->pullen)/sizeof(reg->pullen[0]); idx++)
	if (reg->pullen[idx].val != cur->pullen[idx].val)
	    GPIOERR("GPIOEXT mismatch pullen[%2d]: %x <> %x\n", idx, reg->pullen[idx].val, cur->pullen[idx].val);
    for (idx = 0; idx < sizeof(reg->pullsel)/sizeof(reg->pullsel[0]); idx++)
	if (reg->pullsel[idx].val != cur->pullsel[idx].val)
	    GPIOERR("GPIOEXT mismatch pullsel[%2d]: %x <> %x\n", idx, reg->pullsel[idx].val, cur->pullsel[idx].val);
    for (idx = 0; idx < sizeof(reg->dinv)/sizeof(reg->dinv[0]); idx++)
	if (reg->dinv[idx].val != cur->dinv[idx].val)
	    GPIOERR("GPIOEXT mismatch dinv[%2d]: %x <> %x\n", idx, reg->dinv[idx].val, cur->dinv[idx].val);
    for (idx = 0; idx < sizeof(reg->dout)/sizeof(reg->dout[0]); idx++)
	if (reg->dout[idx].val != cur->dout[idx].val)
	    GPIOERR("GPIOEXT mismatch dout[%2d]: %x <> %x\n", idx, reg->dout[idx].val, cur->dout[idx].val);
    for (idx = 0; idx < sizeof(reg->din)/sizeof(reg->din[0]); idx++)
	if (reg->din[idx].val != cur->din[idx].val)
	    GPIOERR("GPIOEXT mismatch din[%2d]: %x <> %x\n", idx, reg->din[idx].val, cur->din[idx].val);
    for (idx = 0; idx < sizeof(reg->mode)/sizeof(reg->mode[0]); idx++)
	if (reg->mode[idx].val != cur->mode[idx].val)
	    GPIOERR("GPIOEXT mismatch mode[%2d]: %x <> %x\n", idx, reg->mode[idx].val, cur->mode[idx].val);

    kfree(cur);
    return;
}*/
static ssize_t mt_gpio_compare_base(void)
{
	int idx;
	GPIO_REGS *reg = (GPIO_REGS *) GPIO_BASE;
	GPIO_REGS *cur = kzalloc(sizeof(*cur), GFP_KERNEL);

	if (!cur)
		return 0;

	mt_gpio_load_base(cur);
	for (idx = 0; idx < sizeof(reg->dir) / sizeof(reg->dir[0]); idx++)
		if (reg->dir[idx].val != cur->dir[idx].val)
			GPIOERR("mismatch dir[%2d]: %x <> %x\n", idx, reg->dir[idx].val,
				cur->dir[idx].val);
	/* for (idx = 0; idx < sizeof(reg->pullen)/sizeof(reg->pullen[0]); idx++) */
	/* if (reg->pullen[idx].val != cur->pullen[idx].val) */
	/* GPIOERR("mismatch pullen[%2d]: %x <> %x\n", idx, reg->pullen[idx].val, cur->pullen[idx].val); */
	/* for (idx = 0; idx < sizeof(reg->pullsel)/sizeof(reg->pullsel[0]); idx++) */
	/* if (reg->pullsel[idx].val != cur->pullsel[idx].val) */
	/* GPIOERR("mismatch pullsel[%2d]: %x <> %x\n", idx, reg->pullsel[idx].val, cur->pullsel[idx].val); */
	/* for (idx = 0; idx < sizeof(reg->dinv)/sizeof(reg->dinv[0]); idx++) */
	/* if (reg->dinv[idx].val != cur->dinv[idx].val) */
	/* GPIOERR("mismatch dinv[%2d]: %x <> %x\n", idx, reg->dinv[idx].val, cur->dinv[idx].val); */
	for (idx = 0; idx < sizeof(reg->dout) / sizeof(reg->dout[0]); idx++)
		if (reg->dout[idx].val != cur->dout[idx].val)
			GPIOERR("mismatch dout[%2d]: %x <> %x\n", idx, reg->dout[idx].val,
				cur->dout[idx].val);
	for (idx = 0; idx < sizeof(reg->din) / sizeof(reg->din[0]); idx++)
		if (reg->din[idx].val != cur->din[idx].val)
			GPIOERR("mismatch din[%2d]: %x <> %x\n", idx, reg->din[idx].val,
				cur->din[idx].val);
	for (idx = 0; idx < sizeof(reg->mode) / sizeof(reg->mode[0]); idx++)
		if (reg->mode[idx].val != cur->mode[idx].val)
			GPIOERR("mismatch mode[%2d]: %x <> %x\n", idx, reg->mode[idx].val,
				cur->mode[idx].val);

	kfree(cur);
	return 0;
}

static ssize_t mt_gpio_dump_regs(char *buf, ssize_t bufLen)
{
	int idx = 0, len = 0;
#ifdef CONFIG_MTK_FPGA
	char tmp[] = "PIN: [DIN] [DOUT] [DIR]\n";

	len += snprintf(buf + len, bufLen - len, "%s", tmp);
	for (idx = MT_GPIO_BASE_START; idx < MT_GPIO_BASE_MAX; idx++) {
		len += snprintf(buf + len, bufLen - len, "%3d:%d%d%d\n",
				idx, mt_get_gpio_in_base(idx), mt_get_gpio_out_base(idx),
				mt_get_gpio_dir_base(idx));
	}
#else
	/*char tmp[]="PIN: [MODE] [PULL_SEL] [DIN] [DOUT] [PULL EN] [DIR] [INV] [IES]\n"; */
	char tmp[] = "PIN: [MODE] [PULL_SEL] [DIN] [DOUT] [PULL EN] [DIR] [IES] [SMT]\n";

	len += snprintf(buf + len, bufLen - len, "%s", tmp);
	for (idx = MT_GPIO_BASE_START; idx < MT_GPIO_BASE_MAX; idx++) {
		len += snprintf(buf + len, bufLen - len, "%3d:%d%d%d%d%d%d%d%d\n",
				idx, mt_get_gpio_mode_base(idx), mt_get_gpio_pull_select_base(idx),
				mt_get_gpio_in_base(idx), mt_get_gpio_out_base(idx),
				mt_get_gpio_pull_enable_base(idx), mt_get_gpio_dir_base(idx),
				mt_get_gpio_ies_base(idx), mt_get_gpio_smt_base(idx));
		/* printk("%3d:%d%d%d%d%d%d%d%d\n",idx, */
		/* mt_get_gpio_mode_base(idx), mt_get_gpio_pull_select_base(idx),
mt_get_gpio_in_base(idx),mt_get_gpio_out_base(idx), */
		/* mt_get_gpio_pull_enable_base(idx),mt_get_gpio_dir_base(idx),
mt_get_gpio_inversion_base(idx),mt_get_gpio_ies_base(idx)); */

	}
#endif
/*	len += snprintf(buf+len, bufLen-len, "%s","EXT GPIO\n");
    for (idx = MT_GPIO_EXT_START; idx < MT_GPIO_EXT_MAX; idx++) {
		len += snprintf(buf+len, bufLen-len, "%3d:%d%d%d%d%d%d%d%d\n",
		   idx,mt_get_gpio_mode_ext(idx), mt_get_gpio_pull_select_ext(idx),
mt_get_gpio_in_ext(idx),mt_get_gpio_out_ext(idx),
		   mt_get_gpio_pull_enable_ext(idx),mt_get_gpio_dir_ext(idx),
mt_get_gpio_inversion_ext(idx),mt_get_gpio_ies_ext(idx));
    }*/
	return len;
}

/*---------------------------------------------------------------------------*/
ssize_t mt_gpio_show_pin(struct device *dev, struct device_attribute *attr, char *buf)
{
	return mt_gpio_dump_regs(buf, PAGE_SIZE);
}

/******************************************************************************
*MD convert gpio-name to gpio-number
*******************************************************************************/
struct mt_gpio_modem_info {
	char name[40];
	int num;
};

static struct mt_gpio_modem_info mt_gpio_info[] = {
	{"GPIO_MD_TEST", 800},
#ifdef GPIO_AST_CS_PIN
	{"GPIO_AST_HIF_CS", GPIO_AST_CS_PIN},
#endif
#ifdef GPIO_AST_CS_PIN_NCE
	{"GPIO_AST_HIF_CS_ID", GPIO_AST_CS_PIN_NCE},
#endif
#ifdef GPIO_AST_RST_PIN
	{"GPIO_AST_Reset", GPIO_AST_RST_PIN},
#endif
#ifdef GPIO_AST_CLK32K_PIN
	{"GPIO_AST_CLK_32K", GPIO_AST_CLK32K_PIN},
#endif
#ifdef GPIO_AST_CLK32K_PIN_CLK
	{"GPIO_AST_CLK_32K_CLKM", GPIO_AST_CLK32K_PIN_CLK},
#endif
#ifdef GPIO_AST_WAKEUP_PIN
	{"GPIO_AST_Wakeup", GPIO_AST_WAKEUP_PIN},
#endif
#ifdef GPIO_AST_INTR_PIN
	{"GPIO_AST_INT", GPIO_AST_INTR_PIN},
#endif
#ifdef GPIO_AST_WAKEUP_INTR_PIN
	{"GPIO_AST_WAKEUP_INT", GPIO_AST_WAKEUP_INTR_PIN},
#endif
#ifdef GPIO_AST_AFC_SWITCH_PIN
	{"GPIO_AST_AFC_Switch", GPIO_AST_AFC_SWITCH_PIN},
#endif
#ifdef GPIO_FDD_BAND_SUPPORT_DETECT_1ST_PIN
	{"GPIO_FDD_Band_Support_Detection_1", GPIO_FDD_BAND_SUPPORT_DETECT_1ST_PIN},
#endif
#ifdef GPIO_FDD_BAND_SUPPORT_DETECT_2ND_PIN
	{"GPIO_FDD_Band_Support_Detection_2", GPIO_FDD_BAND_SUPPORT_DETECT_2ND_PIN},
#endif
#ifdef GPIO_FDD_BAND_SUPPORT_DETECT_3RD_PIN
	{"GPIO_FDD_Band_Support_Detection_3", GPIO_FDD_BAND_SUPPORT_DETECT_3RD_PIN},
#endif
#ifdef GPIO_FDD_BAND_SUPPORT_DETECT_4TH_PIN
	{"GPIO_FDD_Band_Support_Detection_4", GPIO_FDD_BAND_SUPPORT_DETECT_4TH_PIN},
#endif
#ifdef GPIO_FDD_BAND_SUPPORT_DETECT_5TH_PIN
	{"GPIO_FDD_Band_Support_Detection_5", GPIO_FDD_BAND_SUPPORT_DETECT_5TH_PIN},
#endif
#ifdef GPIO_FDD_BAND_SUPPORT_DETECT_6TH_PIN
	{"GPIO_FDD_Band_Support_Detection_6", GPIO_FDD_BAND_SUPPORT_DETECT_6TH_PIN},
#endif
#ifdef GPIO_SIM_SWITCH_CLK_PIN
	{"GPIO_SIM_SWITCH_CLK", GPIO_SIM_SWITCH_CLK_PIN},
#endif
#ifdef GPIO_SIM_SWITCH_DAT_PIN
	{"GPIO_SIM_SWITCH_DAT", GPIO_SIM_SWITCH_DAT_PIN},
#endif
#ifdef GPIO_SIM1_HOT_PLUG
	{"GPIO_SIM1_INT", GPIO_SIM1_HOT_PLUG},
#endif
#ifdef GPIO_SIM2_HOT_PLUG
	{"GPIO_SIM2_INT", GPIO_SIM2_HOT_PLUG},
#endif
/*if you have new GPIO pin add bellow*/

};

int mt_get_md_gpio(char *gpio_name, int len)
{
	unsigned int i;
	unsigned long number;

	for (i = 0; i < ARRAY_SIZE(mt_gpio_info); i++) {
		if (!strncmp(gpio_name, mt_gpio_info[i].name, len)) {
			number = mt_gpio_info[i].num;
			GPIOMSG("Modern get number=%d, name:%s\n", mt_gpio_info[i].num, gpio_name);
			mt_gpio_pin_decrypt(&number);
			return number;
		}
	}
	GPIOERR("Modem gpio name can't match!!!\n");
	return -1;
}

void mt_get_md_gpio_debug(char *str)
{
	if (strcmp(str, "ALL") == 0) {
		int i;

		for (i = 0; i < ARRAY_SIZE(mt_gpio_info); i++)
			GPIOMSG("GPIO number=%d,%s\n", mt_gpio_info[i].num, mt_gpio_info[i].name);

	} else {
		GPIOMSG("GPIO number=%d,%s\n", mt_get_md_gpio(str, strlen(str)), str);
	}

}
/*---------------------------------------------------------------------------*/
ssize_t mt_gpio_store_pin(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	int pin;
#ifdef MTK_MT6306_SUPPORT
	int group, on;
#endif
	int mode, pullsel, dout, pullen, dir, ies, smt;
	u32 num, src, div;
	/*char md_str[128] = "GPIO_MD_TEST";*/
	/* struct mt_gpio_obj *obj = (struct mt_gpio_obj*)dev_get_drvdata(dev); */
	if (!strncmp(buf, "-h", 2)) {
		GPIOMSG("cat pin  #show all pin setting\n");
		GPIOMSG("echo -wmode num x > pin #num:pin,x:the mode 0~7\n");
		GPIOMSG("echo -wpsel num x > pin #x: 1,pull-up; 0,pull-down\n");
		GPIOMSG("echo -wdout num x > pin #x: 1,high; 0, low\n");
		GPIOMSG("echo -wpen num x > pin  #x: 1,pull enable; 0 pull disable\n");
		GPIOMSG("echo -wies num x > pin  #x: 1,ies enable; 0 ies disable\n");
		GPIOMSG("echo -wdir num x > pin  #x: 1, output; 0, input\n");
		/*GPIOMSG("echo -wdinv num x > pin #x: 1, inversion enable; 0, disable\n"); */
		GPIOMSG("echo -w=num x x x x x x > pin #set all property one time\n");
		GPIOMSG("PIN: [MODE] [PSEL] [DIN] [DOUT] [PEN] [DIR] [IES]\n");
	} else if (!strncmp(buf, "-r0", 3) && (1 == sscanf(buf + 3, "%d", &pin))) {
		GPIO_CFG cfg = {.no = pin };
		/*if pmic */
		mt_gpio_read_pin_base(&cfg, 0);
		GPIOMSG("%3d: %d %d %d %d %d %d\n", cfg.no, cfg.mode, cfg.pullsel,
			cfg.din, cfg.dout, cfg.pullen, cfg.dir);
	} else if (!strncmp(buf, "-r1", 3) && (1 == sscanf(buf + 3, "%d", &pin))) {
		GPIO_CFG cfg = {.no = pin };

		mt_gpio_read_pin_base(&cfg, 1);
		GPIOMSG("%3d: %d %d %d %d %d %d %d\n", cfg.no, cfg.mode, cfg.pullsel,
			cfg.din, cfg.dout, cfg.pullen, cfg.dir, cfg.ies);
	} else if (!strncmp(buf, "-w", 2)) {
		buf += 2;
		if (!strncmp(buf, "mode", 4) && (2 == sscanf(buf + 4, "%d %d", &pin, &mode)))
			GPIOMSG("set mode(%3d, %d)=%d\n", pin, mode, mt_set_gpio_mode(pin, mode));
		else if (!strncmp(buf, "psel", 4)
			 && (2 == sscanf(buf + 4, "%d %d", &pin, &pullsel)))
			GPIOMSG("set psel(%3d, %d)=%d\n", pin, pullsel,
				mt_set_gpio_pull_select(pin, pullsel));
		else if (!strncmp(buf, "dout", 4) && (2 == sscanf(buf + 4, "%d %d", &pin, &dout)))
			GPIOMSG("set dout(%3d, %d)=%d\n", pin, dout, mt_set_gpio_out(pin, dout));
		else if (!strncmp(buf, "pen", 3) && (2 == sscanf(buf + 3, "%d %d", &pin, &pullen)))
			GPIOMSG("set pen (%3d, %d)=%d\n", pin, pullen,
				mt_set_gpio_pull_enable(pin, pullen));
		else if (!strncmp(buf, "ies", 3) && (2 == sscanf(buf + 3, "%d %d", &pin, &ies)))
			GPIOMSG("set ies (%3d, %d)=%d\n", pin, ies, mt_set_gpio_ies(pin, ies));
		else if (!strncmp(buf, "dir", 3) && (2 == sscanf(buf + 3, "%d %d", &pin, &dir)))
			GPIOMSG("set dir (%3d, %d)=%d\n", pin, dir, mt_set_gpio_dir(pin, dir));
		/* else if (!strncmp(buf, "dinv", 4) && (2 == sscanf(buf+4, "%d %d", &pin, &dinv))) */
		/* GPIOMSG("set dinv(%3d, %d)=%d\n", pin, dinv, mt_set_gpio_inversion(pin, dinv)); */
#ifdef CONFIG_MTK_FPGA
		else if (3 == sscanf(buf, "=%d:%d %d", &pin, &dout, &dir)) {
			GPIOMSG("set dout(%3d, %d)=%d\n", pin, dout, mt_set_gpio_out(pin, dout));
			GPIOMSG("set dir (%3d, %d)=%d\n", pin, dir, mt_set_gpio_dir(pin, dir));
#else
		else if (8 == sscanf(buf, "=%d:%d %d %d %d %d %d %d", &pin, &mode, &pullsel, &dout,
				&pullen, &dir, &ies, &smt)) {
			GPIOMSG("set mode(%3d, %d)=%d\n", pin, mode, mt_set_gpio_mode(pin, mode));
			GPIOMSG("set psel(%3d, %d)=%d\n", pin, pullsel,
				mt_set_gpio_pull_select(pin, pullsel));
			GPIOMSG("set dout(%3d, %d)=%d\n", pin, dout, mt_set_gpio_out(pin, dout));
			GPIOMSG("set pen (%3d, %d)=%d\n", pin, pullen,
				mt_set_gpio_pull_enable(pin, pullen));
			GPIOMSG("set dir (%3d, %d)=%d\n", pin, dir, mt_set_gpio_dir(pin, dir));
			/* GPIOMSG("set dinv(%3d, %d)=%d\n", pin, dinv, mt_set_gpio_inversion(pin, dinv)); */
			GPIOMSG("set ies (%3d, %d)=%d\n", pin, ies, mt_set_gpio_ies(pin, ies));
			GPIOMSG("set smt (%3d, %d)=%d\n", pin, smt, mt_set_gpio_smt(pin, smt));
#endif
		} else
			GPIOMSG("invalid format: '%s'", buf);
#ifdef MTK_MT6306_SUPPORT
	} else if (!strncmp(buf, "ww", 2)) {
		/* MT6306 GPIO */
		buf += 2;
		if (3 == sscanf(buf, "=%d:%d %d", &pin, &dout, &dir)) {
			GPIOMSG("[MT6306] set dout(%3d, %d)=%d\n", pin, dout,
				mt6306_set_gpio_out(pin, dout));
			GPIOMSG("[MT6306] set dir (%3d, %d)=%d\n", pin, dir,
				mt6306_set_gpio_dir(pin, dir));
		} else
			GPIOMSG("invalid format: '%s'", buf);
	} else if (!strncmp(buf, "wy", 2)) {
		/* MT6306 GPIO */
		buf += 2;
		if (2 == sscanf(buf, "=%d:%d", &group, &on)) {
			GPIOMSG("[MT6306] set pin group vccen (%3d, %d)=%d\n", group, on,
				mt6306_set_GPIO_pin_group_power(group, on));
			GPIOMSG("[MT6306] set pin group vccen (%3d, %d)=%d\n", group, on,
				mt6306_set_GPIO_pin_group_power(group, on));
		} else
			GPIOMSG("invalid format: '%s'", buf);
#endif
	} else if (!strncmp(buf, "-t", 2)) {
		/*mt_gpio_self_test();*/
	} else if (!strncmp(buf, "-c", 2)) {
		mt_gpio_compare_base();
		/* mt_gpio_compare_ext(); */
	} else if (!strncmp(buf, "-da", 3)) {
		mt_gpio_dump_addr_base();
		/* mt_gpio_dump_addr_ext(); */
	} else if (!strncmp(buf, "-dp", 3)) {
		gpio_dump_regs();
	} else if (!strncmp(buf, "-d", 2)) {
		mt_gpio_dump();
	} else if (!strncmp(buf, "tt", 2)) {
		/* GPIOMSG("gpio reg test for next chip!\n"); */
		/* mt_reg_test(); */
	} else if (!strncmp(buf, "-md", 3)) {
		/* buf +=3; */
		/* sscanf(buf,"%s",md_str); */
		/* mt_get_md_gpio_debug(md_str); */
	} else if (!strncmp(buf, "-k", 2)) {
		buf += 2;
		if (!strncmp(buf, "s", 1) && (3 == sscanf(buf + 1, "%d %d %d", &num, &src, &div)))
			GPIOMSG("set num(%d, %d, %d)=%d\n", num, src, div,
				mt_set_clock_output(num, src, div));
	} else if (!strncmp(buf, "g", 1) && (1 == sscanf(buf + 1, "%d", &num))) {
		/* ret = mt_get_clock_output(num, &src,&div); */
		/* GPIOMSG("get num(%ld, %ld, %ld)=%d\n", num, src, div,ret); */
	} else {
		GPIOMSG("invalid format: '%s'", buf);
	}
	return count;
}




