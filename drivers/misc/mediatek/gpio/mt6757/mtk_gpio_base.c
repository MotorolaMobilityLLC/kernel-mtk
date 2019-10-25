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

#include "mt-plat/sync_write.h"
#include <6757_gpio.h>
#include <linux/types.h>
#include <mt-plat/mtk_gpio.h>
#include <mt-plat/mtk_gpio_core.h>
#include <mtk_gpio_base.h>
/* autogen */
#include <gpio_cfg.h>
#ifdef CONFIG_OF
#include <linux/of_address.h>
#endif

long gpio_pull_select_unsupport[MAX_GPIO_PIN] = {0};
long gpio_pullen_unsupport[MAX_GPIO_PIN] = {0};
long gpio_smt_unsupport[MAX_GPIO_PIN] = {0};

struct mt_gpio_vbase gpio_vbase;
#define REGSET (4)
#define REGCLR (8)

static GPIO_REGS *gpio_reg;
/*---------------------------------------------------------------------------*/
int mt_set_gpio_dir_base(unsigned long pin, unsigned long dir)
{
	unsigned long pos;
	unsigned long bit;
	GPIO_REGS *reg = gpio_reg;

	if (!reg)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (dir >= GPIO_DIR_MAX)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (dir == GPIO_DIR_IN)
		GPIO_SET_BITS((1L << bit), &reg->dir[pos].rst);
	else
		GPIO_SET_BITS((1L << bit), &reg->dir[pos].set);
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_dir_base(unsigned long pin)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;

	if (!reg)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	data = GPIO_RD32(&reg->dir[pos].val);
	return ((data & (1L << bit)) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_out_base(unsigned long pin, unsigned long output)
{

	unsigned long pos;
	unsigned long bit;
	GPIO_REGS *reg = gpio_reg;

	if (!reg)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (output >= GPIO_OUT_MAX)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (output == GPIO_OUT_ZERO)
		GPIO_SET_BITS((1L << bit), &reg->dout[pos].rst);
	else
		GPIO_SET_BITS((1L << bit), &reg->dout[pos].set);
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_out_base(unsigned long pin)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;

	if (!reg)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	data = GPIO_RD32(&reg->dout[pos].val);
	return ((data & (1L << bit)) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_in_base(unsigned long pin)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;

	if (!reg)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	data = GPIO_RD32(&reg->din[pos].val);
	return ((data & (1L << bit)) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_mode_base(unsigned long pin, unsigned long mode)
{
	unsigned long bit;
	unsigned long data;
	u32 mask;
	unsigned long base;
	GPIO_REGS *reg = gpio_reg;

	if (!reg)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (mode >= GPIO_MODE_MAX)
		return -ERINVAL;

#ifdef SUPPORT_MODE_MWR
	/*  This is most wanted version after CD1CD2 unification.
	 *  if MWR is supported, no need to read before write.
	 *  MWR[3:0] has 4 bits not 3 bits ; additional 1 bit,MWR[3], is for
	 * update enable
	 *  data[3:0] = MWR[3] | MWR[2:0].
	 */
	unsigned long pos;

	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;

	data = ((1L << (GPIO_MODE_BITS * bit)) << 3) |
	       (mode << (GPIO_MODE_BITS * bit));
	GPIO_WR32(&reg->mode[pos]._align1, data);

#else
	/* For DNL/JD, there is no mwr register for simple register setting.
	 * Need 1R+1W to set MODE registers
	 */
	mask = (1L << GPIO_MODE_BITS) - 1;
	mode = mode & mask;

#ifdef REGULAR_MODE_OFFSET
	unsigned long pos;

	pos = pin / MAX_GPIO_MODE_PER_REG;
	/*for bit to be valid, difference series offset must be provided by HW
	 */
	bit = pin % MAX_GPIO_MODE_PER_REG;

	data = GPIO_RD32(&reg->mode[pos].val);
	data &= (~(mask << bit));
	data |= (mode << bit);
	GPIO_WR32(&reg->mode[pos].val, data);
#else
	bit = MODE_offset[pin].offset;

	data = GPIO_RD32((GPIO_BASE + MODE_addr[pin].addr));
	data &= (~(mask << bit));
	data |= (mode << bit);

	base = (unsigned long)GPIO_BASE + MODE_addr[pin].addr;
	GPIO_WR32(base, data);
#endif

#endif
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_mode_base(unsigned long pin)
{
	unsigned long bit;
	unsigned long data;
	u32 mask;
	GPIO_REGS *reg = gpio_reg;

	if (!reg)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	mask = (1L << GPIO_MODE_BITS) - 1;

#ifdef SUPPORT_MODE_MWR
	unsigned long pos;

	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;

	data = GPIO_RD32(&reg->mode[pos].val);
	return (data >> (GPIO_MODE_BITS * bit)) & mask;
#else

	bit = MODE_offset[pin].offset;
	data = GPIO_RD32(GPIO_BASE + MODE_addr[pin].addr);
	return (data >> bit) & mask;
#endif
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_smt_base(unsigned long pin, unsigned long enable)
{
	unsigned long reg = 0;
	unsigned long bit = 0;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (SMT_offset[pin].offset == -1) {
		return GPIO_SMT_UNSUPPORTED;

	} else {
		bit = SMT_offset[pin].offset;
		reg = IOCFG_RD32(SMT_addr[pin].addr);
		if (enable == GPIO_SMT_DISABLE)
			reg &= (~(1 << bit));
		/* IOCFG_SET_BITS((1L << bit), SMT_addr[pin].addr + REGCLR); */
		else
			reg |= (1 << bit);
		/* IOCFG_SET_BITS((1L << bit), SMT_addr[pin].addr + REGSET); */
	}

	IOCFG_WR32(SMT_addr[pin].addr, reg);

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_smt_base(unsigned long pin)
{
	unsigned long data;
	unsigned long bit = 0;
	int smt;

	bit = SMT_offset[pin].offset;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (SMT_offset[pin].offset != -1) {
		data = IOCFG_RD32(SMT_addr[pin].addr);
		smt = ((data & (1L << bit)) != 0) ? 1 : 0;
	} else {
		return GPIO_SMT_UNSUPPORTED;
	}

	return smt;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_ies_base(unsigned long pin, unsigned long enable)
{

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset != -1) {
		if (enable == GPIO_IES_DISABLE)
			IOCFG_SET_BITS((1L << (IES_offset[pin].offset)),
				       IES_addr[pin].addr + REGCLR);
		else
			IOCFG_SET_BITS((1L << (IES_offset[pin].offset)),
				       IES_addr[pin].addr + REGSET);

	} else
		return GPIO_IES_UNSUPPORTED;

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_ies_base(unsigned long pin)
{
	unsigned long data;
	int ies;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset != -1) {
		data = IOCFG_RD32(IES_addr[pin].addr);
		ies = ((data & (1L << (IES_offset[pin].offset))) != 0) ? 1 : 0;
	} else {
		return GPIO_IES_UNSUPPORTED;
	}

	return ies;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_enable_base(unsigned long pin, unsigned long enable)
{
	unsigned long reg;
	u32 bit;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (PULLEN_offset[pin].offset == -1 && PUPD_offset[pin].offset == -1) {
		gpio_pullen_unsupport[pin] = -1;
		return GPIO_PULL_EN_UNSUPPORTED;
	}
	if (PULLEN_offset[pin].offset != -1) {
		if (enable == GPIO_PULL_DISABLE)
			IOCFG_SET_BITS((1L << (PULLEN_offset[pin].offset)),
				       PULLEN_addr[pin].addr + REGCLR);
		else
			IOCFG_SET_BITS((1L << (PULLEN_offset[pin].offset)),
				       PULLEN_addr[pin].addr + REGSET);
	} else {
		bit = PUPD_offset[pin].offset;
		reg = IOCFG_RD32(PUPD_addr[pin].addr);
		if (enable == GPIO_PULL_DISABLE)
			reg &= (~(0x7 << bit));
		else
			reg |= (1 << bit);
		IOCFG_WR32(PUPD_addr[pin].addr, reg);
	}
	/* GPIOERR("%s:pin:%ld, enable:%ld, value:0x%x\n",__FUNCTION__, pin,
	 * enable, GPIO_RD32(PULLEN_addr[pin].addr)); */

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_enable_base(unsigned long pin)
{
	unsigned long data;
	u32 bit = 0;
	int en;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (PULLEN_offset[pin].offset == -1 && PUPD_offset[pin].offset == -1)
		return GPIO_PULL_EN_UNSUPPORTED;

	if (PULLEN_offset[pin].offset != -1) {
		bit = PULLEN_offset[pin].offset;
		data = IOCFG_RD32(PULLEN_addr[pin].addr);
		en = ((data & (1L << bit)) != 0) ? 1 : 0;
	} else {
		bit = PUPD_offset[pin].offset;
		data = IOCFG_RD32(PUPD_addr[pin].addr);
		en = ((data & (0x3 << bit)) != 0) ? 1 : 0;
	}

	return en;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_select_base(unsigned long pin, unsigned long select)
{
	/* unsigned long flags; */

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if ((PULLSEL_offset[pin].offset == -1) &&
	    (PUPD_offset[pin].offset == -1)) {
		gpio_pull_select_unsupport[pin] = -1;
		return GPIO_PULL_UNSUPPORTED;
	} else if (PULLSEL_offset[pin].offset == -1) {
		/* spin_lock_irqsave(&mtk_gpio_lock, flags); */
		/*
		 *  SIM1, SIM2 may need special care for resistance selection
		 */
		if (select == GPIO_PULL_DOWN) {
			IOCFG_SET_BITS((1L << (PUPD_offset[pin].offset + 2)),
				       PUPD_addr[pin].addr + REGSET);
		} else {
			IOCFG_SET_BITS((1L << (PUPD_offset[pin].offset + 2)),
				       PUPD_addr[pin].addr + REGCLR);
		}
		/* spin_unlock_irqrestore(&mtk_gpio_lock, flags); */
	} else {
		if (select == GPIO_PULL_DOWN)
			IOCFG_SET_BITS((1L << (PULLSEL_offset[pin].offset)),
				       PULLSEL_addr[pin].addr + REGCLR);
		else
			IOCFG_SET_BITS((1L << (PULLSEL_offset[pin].offset)),
				       PULLSEL_addr[pin].addr + REGSET);
	}

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_select_base(unsigned long pin)
{
	unsigned long data;
	int pupd;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if ((PULLSEL_offset[pin].offset == -1) &&
	    (PUPD_offset[pin].offset == -1)) {
		return GPIO_PULL_UNSUPPORTED;
	} else if (PULLSEL_offset[pin].offset == -1) {
		data = IOCFG_RD32(PUPD_addr[pin].addr);
		pupd = ((data & (1L << (PUPD_offset[pin].offset + 2))) != 0)
			   ? 0
			   : 1;
	} else {
		data = IOCFG_RD32(PULLSEL_addr[pin].addr);
		pupd = ((data & (1L << (PULLSEL_offset[pin].offset))) != 0) ? 1
									    : 0;
	}

	return pupd;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_inversion_base(unsigned long pin, unsigned long enable)
{ /*FIX-ME
   */
	GPIOERR("%s:function not supprted", __func__);
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_inversion_base(unsigned long pin)
{ /*FIX-ME */
	GPIOERR("%s:function not supprted", __func__);
	return 0; /* FIX-ME */
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_slew_rate_base(unsigned long pin, unsigned long enable)
{
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_slew_rate_base(unsigned long pin) { return RSUCCESS; }
/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_resistor_base(unsigned long pin, unsigned long resistors)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_resistor_base(unsigned long pin) { return RSUCCESS; }
/*---------------------------------------------------------------------------*/
int mt_set_gpio_driving_base(unsigned long pin, unsigned long strength)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_driving_base(unsigned long pin) { return RSUCCESS; }

/*---------------------------------------------------------------------------*/
void get_gpio_vbase(struct device_node *node)
{
	/* compatible with HAL */
	if (!(gpio_vbase.gpio_regs)) {

		gpio_vbase.gpio_regs = of_iomap(node, 0);

		if (!gpio_vbase.gpio_regs) {
			GPIOERR("GPIO base addr is NULL\n");
			return;
		}
		gpio_reg = (GPIO_REGS *)gpio_vbase.gpio_regs;
		GPIOERR("GPIO base add is 0x%p\n", gpio_vbase.gpio_regs);
	}
	GPIOERR("GPIO base addr is 0x%p, %s\n", gpio_vbase.gpio_regs, __func__);
}

/*-----------------------User need GPIO APIs before GPIO
 * probe------------------*/
/*extern struct device_node *get_gpio_np(void);*/
/*extern struct device_node *get_iocfg_np(void);*/
#if 0
static int __init get_gpio_vbase_early(void)
{
	struct device_node *np_gpio = NULL;
	struct device_node *np_iocfg = NULL;

	gpio_vbase.gpio_regs = NULL;
	gpio_vbase.iocfg_regs = NULL;
	np_gpio = get_gpio_np();
	np_iocfg = get_iocfg_np();
	/* Setup IO addresses */
	gpio_vbase.gpio_regs = of_iomap(np_gpio, 0);
	if (!gpio_vbase.gpio_regs) {
		GPIOERR("GPIO base addr is NULL\n");
		return 0;
	}
	gpio_vbase.iocfg_regs = of_iomap(np_iocfg, 0);
	if (!gpio_vbase.iocfg_regs) {
		GPIOERR("GPIO base addr is NULL\n");
		return 0;
	}
	/* gpio_reg = (GPIO_REGS*)(GPIO_BASE); */
	GPIOERR("GPIO base addr is 0x%p, %s\n", gpio_vbase.gpio_regs, __func__);
	return 0;
}
postcore_initcall(get_gpio_vbase_early);
#endif
/*---------------------------------------------------------------------------*/
void get_io_cfg_vbase(void)
{
	struct device_node *np_iocfg = NULL;

	np_iocfg = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_0");
	if (np_iocfg) {
		/* Setup IOCFG addresses */
		gpio_vbase.iocfg_regs = of_iomap(np_iocfg, 0);
		GPIOLOG("IOCFG_BASE is gpio_vbase.iocfg_regs=0x%lx\n",
			(unsigned long)gpio_vbase.iocfg_regs);
	}
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
void mt_gpio_suspend(void) { /* compatible with HAL */ }

/*---------------------------------------------------------------------------*/
void mt_gpio_resume(void) { /* compatible with HAL */ }

/*---------------------------------------------------------------------------*/
#endif /*CONFIG_PM */
/*---------------------------------------------------------------------------*/
