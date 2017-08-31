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


#include <6799_gpio.h>
#include <linux/types.h>
#include "mt-plat/sync_write.h"
#include <mt-plat/mtk_gpio.h>
#include <mt-plat/mtk_gpio_core.h>
#include <mtk_gpio_base.h>
#include <gpio_cfg.h>
#ifdef CONFIG_OF
#include <linux/of_address.h>
#endif

#define SUPPORT_MODE_MWR

#define REGSET (4)
#define REGCLR (8)
#define REGMWR (0xC)

#define GPIO_WR32(addr, data)   mt_reg_sync_writel(data, addr)
#define GPIO_RD32(addr)         __raw_readl(addr)
#define GPIO_SET_BITS(REG, BIT) GPIO_WR32(REG + REGSET, BIT)
#define GPIO_CLR_BITS(REG, BIT) GPIO_WR32(REG + REGCLR, BIT)

long gpio_pull_select_unsupport[MAX_GPIO_PIN] = { 0 };
long gpio_pullen_unsupport[MAX_GPIO_PIN] = { 0 };
long gpio_smt_unsupport[MAX_GPIO_PIN] = { 0 };

struct mt_gpio_vbase gpio_vbase;

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
		GPIO_WR32(&reg->dir[pos].rst, 1L << bit);
	else
		GPIO_WR32(&reg->dir[pos].set, 1L << bit);
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
		GPIO_WR32(&reg->dout[pos].rst, 1L << bit);
	else
		GPIO_WR32(&reg->dout[pos].set, 1L << bit);
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
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;
	u32 mask;

	if (!reg)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (mode >= GPIO_MODE_MAX)
		return -ERINVAL;

#ifdef SUPPORT_MODE_MWR
	mask = (1L << (GPIO_MODE_BITS - 1)) - 1;
#else
	mask = (1L << GPIO_MODE_BITS) - 1;
#endif

	mode = mode & mask;
	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = (pin % MAX_GPIO_MODE_PER_REG) * GPIO_MODE_BITS;

#ifdef SUPPORT_MODE_MWR
	data = (1L << (bit + GPIO_MODE_BITS - 1)) | (mode << bit);
	GPIO_WR32(&reg->mode[pos]._align1, data);
#else
	data = GPIO_RD32(&reg->mode[pos].val);
	data &= (~(mask << bit));
	data |= (mode << bit);
	GPIO_WR32(&reg->mode[pos].val, data);
#endif

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_mode_base(unsigned long pin)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	u32 mask;
	GPIO_REGS *reg = gpio_reg;

	if (!reg)
		return -ERACCESS;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

#ifdef SUPPORT_MODE_MWR
	mask = (1L << (GPIO_MODE_BITS - 1)) - 1;
#else
	mask = (1L << GPIO_MODE_BITS) - 1;
#endif

	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = (pin % MAX_GPIO_MODE_PER_REG) * GPIO_MODE_BITS;

	data = GPIO_RD32(&reg->mode[pos].val);
	return (data >> bit) & mask;
}


/*---------------------------------------------------------------------------*/
int mt_set_gpio_smt_base(unsigned long pin, unsigned long enable)
{
	void __iomem *base;
	u8 bit;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (SMT_offset[pin].offset == (u8)-1)
		return GPIO_SMT_UNSUPPORTED;

	bit = SMT_offset[pin].offset;
	base = gpio_vbase.iocfg_regs_array[SMT_base_index[pin].index];

	if (enable == GPIO_SMT_DISABLE)
		GPIO_CLR_BITS(base + SMT_addr[pin].addr, 1 << bit);
	else
		GPIO_SET_BITS(base + SMT_addr[pin].addr, 1 << bit);

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_smt_base(unsigned long pin)
{
	unsigned long data;
	void __iomem *base;
	u8 bit;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (SMT_offset[pin].offset == (u8)-1)
		return GPIO_SMT_UNSUPPORTED;

	bit = SMT_offset[pin].offset;
	base = gpio_vbase.iocfg_regs_array[SMT_base_index[pin].index];
	data = GPIO_RD32(base + SMT_addr[pin].addr);
	return ((data & (1L << bit)) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_ies_base(unsigned long pin, unsigned long enable)
{
	void __iomem *base;
	u8 bit;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset == (u8)-1)
		return GPIO_IES_UNSUPPORTED;

	bit = IES_offset[pin].offset;
	base = gpio_vbase.iocfg_regs_array[IES_base_index[pin].index];

	if (enable == GPIO_IES_DISABLE)
		GPIO_CLR_BITS(base + IES_addr[pin].addr, 1 << bit);
	else
		GPIO_SET_BITS(base + IES_addr[pin].addr, 1 << bit);

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_ies_base(unsigned long pin)
{
	unsigned long data;
	void __iomem *base;
	u8 bit;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset == (u8)-1)
		return GPIO_IES_UNSUPPORTED;

	bit = IES_offset[pin].offset;
	base = gpio_vbase.iocfg_regs_array[IES_base_index[pin].index];
	data = GPIO_RD32(base + IES_addr[pin].addr);
	return ((data & (1L << bit)) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_enable_base(unsigned long pin, unsigned long enable)
{
	void __iomem *base;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (PULLEN_offset[pin].offset == (u8)-1 && PUPD_offset[pin].offset == (u8)-1) {
		gpio_pullen_unsupport[pin] = -1;
		return GPIO_PULL_EN_UNSUPPORTED;
	}

	if (PULLEN_offset[pin].offset != (u8)-1) {
		base = gpio_vbase.iocfg_regs_array[PULLEN_base_index[pin].index];
		if (enable == GPIO_PULL_DISABLE)
			GPIO_CLR_BITS(base + PULLEN_addr[pin].addr,
				1L << PULLEN_offset[pin].offset);
		else
			GPIO_SET_BITS(base + PULLEN_addr[pin].addr,
				1L << PULLEN_offset[pin].offset);
	} else {
		base = gpio_vbase.iocfg_regs_array[PUPD_base_index[pin].index];
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_CLR_BITS(base + PUPD_addr[pin].addr,
				3L << PUPD_offset[pin].offset);
		} else if (enable == GPIO_PULL_ENABLE_R0) {
			GPIO_SET_BITS(base + PUPD_addr[pin].addr,
				1L << PUPD_offset[pin].offset);
			GPIO_CLR_BITS(base + PUPD_addr[pin].addr,
				1L << (PUPD_offset[pin].offset + 1));
		} else if (enable == GPIO_PULL_ENABLE_R1) {
			GPIO_CLR_BITS(base + PUPD_addr[pin].addr,
				1L << PUPD_offset[pin].offset);
			GPIO_SET_BITS(base + PUPD_addr[pin].addr,
				1L << (PUPD_offset[pin].offset + 1));
		} else if (enable == GPIO_PULL_ENABLE_R0R1) {
			GPIO_SET_BITS(base + PUPD_addr[pin].addr,
				3L << PUPD_offset[pin].offset);
		} else {
			return -ERINVAL;
		}
	}

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_enable_base(unsigned long pin)
{
	unsigned long data;
	void __iomem *base;
	u8 bit;
	int en;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (PULLEN_offset[pin].offset == (u8)-1 && PUPD_offset[pin].offset == (u8)-1)
		return GPIO_PULL_EN_UNSUPPORTED;

	if (PULLEN_offset[pin].offset != (u8)-1) {
		base = gpio_vbase.iocfg_regs_array[PULLEN_base_index[pin].index];
		bit = PULLEN_offset[pin].offset;
		data = GPIO_RD32(base + PULLEN_addr[pin].addr);
		en = ((data & (1L << bit)) != 0) ? 1 : 0;
	} else {
		base = gpio_vbase.iocfg_regs_array[PUPD_base_index[pin].index];
		bit = PUPD_offset[pin].offset;
		data = GPIO_RD32(base + PUPD_addr[pin].addr);
		en = ((data & (0x3 << bit)) != 0) ? 1 : 0;
	}

	return en;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_select_base(unsigned long pin, unsigned long select)
{
	void __iomem *base;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if ((PULLSEL_offset[pin].offset == (u8)-1) && (PUPD_offset[pin].offset == (u8)-1)) {
		gpio_pull_select_unsupport[pin] = -1;
		return GPIO_PULL_UNSUPPORTED;
	}

	if (PULLSEL_offset[pin].offset != (u8)-1) {
		base = gpio_vbase.iocfg_regs_array[PULLSEL_base_index[pin].index];
		if (select == GPIO_PULL_DOWN)
			GPIO_CLR_BITS(base + PULLSEL_addr[pin].addr,
				1L << PULLSEL_offset[pin].offset);
		else
			GPIO_SET_BITS(base + PULLSEL_addr[pin].addr,
				1L << PULLSEL_offset[pin].offset);
	} else {
		base = gpio_vbase.iocfg_regs_array[PUPD_base_index[pin].index];
		if (select == GPIO_PULL_DOWN)
			GPIO_SET_BITS(base + PUPD_addr[pin].addr,
				1L << (PUPD_offset[pin].offset + 2));
		else
			GPIO_CLR_BITS(base + PUPD_addr[pin].addr,
				1L << (PUPD_offset[pin].offset + 2));
	}

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_select_base(unsigned long pin)
{
	unsigned long data;
	void __iomem *base;
	int pupd;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if ((PULLSEL_offset[pin].offset == (u8)-1) && (PUPD_offset[pin].offset == (u8)-1))
		return GPIO_PULL_UNSUPPORTED;

	if (PULLSEL_offset[pin].offset != (u8)-1) {
		base = gpio_vbase.iocfg_regs_array[PULLSEL_base_index[pin].index];
		data = GPIO_RD32(base + PULLSEL_addr[pin].addr);
		pupd = ((data & (1L << (PULLSEL_offset[pin].offset))) != 0) ? 1 : 0;
	} else {
		base = gpio_vbase.iocfg_regs_array[PUPD_base_index[pin].index];
		data = GPIO_RD32(base + PUPD_addr[pin].addr);
		pupd = ((data & (1L << (PUPD_offset[pin].offset + 2))) != 0) ? 0 : 1;
	}

	return pupd;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_inversion_base(unsigned long pin, unsigned long enable)
{
	GPIOERR("%s:function not supprted", __func__);
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_inversion_base(unsigned long pin)
{
	GPIOERR("%s:function not supprted", __func__);
	return 0;
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_slew_rate_base(unsigned long pin, unsigned long enable)
{
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_slew_rate_base(unsigned long pin)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_resistor_base(unsigned long pin, unsigned long resistors)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_resistor_base(unsigned long pin)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_driving_base(unsigned long pin, unsigned long strength)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_driving_base(unsigned long pin)
{
	return RSUCCESS;
}

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
		gpio_reg = (GPIO_REGS *) gpio_vbase.gpio_regs;
		GPIOMSG("GPIO base add is 0x%p\n", gpio_vbase.gpio_regs);
	}
	GPIOMSG("GPIO base addr is 0x%p, %s\n", gpio_vbase.gpio_regs, __func__);
}

/*-----------------------User need GPIO APIs before GPIO probe------------------*/
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
	GPIOMSG("GPIO base addr is 0x%p, %s\n", gpio_vbase.gpio_regs, __func__);
	return 0;
}
postcore_initcall(get_gpio_vbase_early);
#endif
/*---------------------------------------------------------------------------*/
void get_io_cfg_vbase(void)
{
	int i;
	struct device_node *np_iocfg;
	/* Attention: sequence of compat_names elements shall be the same
	 *            as sequence of IOCFG_XX_BASE_IDs
	 */
	static char const * const compat_names[] = {
		"mediatek,iocfg_rb", "mediatek,iocfg_br", "mediatek,iocfg_bl",
		"mediatek,iocfg_bm", "mediatek,iocfg_lt", "mediatek,iocfg_lb",
		"mediatek,iocfg_tr"
	};

	for (i = 0; i < 7; i++) {
		np_iocfg = NULL;
		np_iocfg = of_find_compatible_node(NULL, NULL, compat_names[i]);
		if (np_iocfg) {
			gpio_vbase.iocfg_regs_array[i] = of_iomap(np_iocfg, 0);
			GPIOLOG("%s reg base = 0x%lx\n", compat_names[i],
				(unsigned long)gpio_vbase.iocfg_regs_array[i]);
		}
	}
#if 0
	np_iocfg = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_rb");
	if (np_iocfg) {
		gpio_vbase.iocfg_regs_array[IOCFG_RB_BASE_ID] =
			= of_iomap(np_iocfg, 0);
		GPIOLOG("IOCFG_RB_BASE is gpio_vbase.IOCFG_RB_regs=0x%lx\n",
			(unsigned long)gpio_vbase.iocfg_regs_array[IOCFG_RB_BASE_ID]);
	}

	np_iocfg = NULL;
	np_iocfg = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_br");
	if (np_iocfg) {
		gpio_vbase.iocfg_regs_array[IOCFG_BR_BASE_ID] =
			= of_iomap(np_iocfg, 0);
		GPIOLOG("IOCFG_BR_BASE is gpio_vbase.IOCFG_BR_regs=0x%lx\n",
			(unsigned long)gpio_vbase.iocfg_regs_array[IOCFG_BR_BASE_ID]);
	}

	np_iocfg = NULL;
	np_iocfg = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_bl");
	if (np_iocfg) {
		gpio_vbase.iocfg_regs_array[IOCFG_BL_BASE_ID] =
			= of_iomap(np_iocfg, 0);
		GPIOLOG("IOCFG_BL_BASE is gpio_vbase.IOCFG_BL_regs=0x%lx\n",
			(unsigned long)gpio_vbase.iocfg_regs_array[IOCFG_BL_BASE_ID]);
	}

	np_iocfg = NULL;
	np_iocfg = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_bm");
	if (np_iocfg) {
		gpio_vbase.iocfg_regs_array[IOCFG_BM_BASE_ID] =
			= of_iomap(np_iocfg, 0);
		GPIOLOG("IOCFG_BM_BASE is gpio_vbase.IOCFG_BM_regs=0x%lx\n",
			(unsigned long)gpio_vbase.iocfg_regs_array[IOCFG_BM_BASE_ID]);
	}

	np_iocfg = NULL;
	np_iocfg = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_lt");
	if (np_iocfg) {
		gpio_vbase.iocfg_regs_array[IOCFG_LT_BASE_ID] =
			= of_iomap(np_iocfg, 0);
		GPIOLOG("IOCFG_LT_BASE is gpio_vbase.IOCFG_LT_regs=0x%lx\n",
			(unsigned long)gpio_vbase.iocfg_regs_array[IOCFG_LT_BASE_ID]);
	}

	np_iocfg = NULL;
	np_iocfg = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_lb");
	if (np_iocfg) {
		gpio_vbase.iocfg_regs_array[IOCFG_LB_BASE_ID] =
			= of_iomap(np_iocfg, 0);
		GPIOLOG("IOCFG_LB_BASE is gpio_vbase.IOCFG_LB_regs=0x%lx\n",
			(unsigned long)gpio_vbase.iocfg_regs_array[IOCFG_LB_BASE_ID]);
	}

	np_iocfg = NULL;
	np_iocfg = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_tr");
	if (np_iocfg) {
		gpio_vbase.iocfg_regs_array[IOCFG_TR_BASE_ID] =
			= of_iomap(np_iocfg, 0);
		GPIOLOG("IOCFG_TR_BASE is gpio_vbase.IOCFG_TR_regs=0x%lx\n",
			(unsigned long)gpio_vbase.iocfg_regs_array[IOCFG_TR_BASE_ID]);
	}
#endif
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
void mt_gpio_suspend(void)
{
	/* compatible with HAL */
}

/*---------------------------------------------------------------------------*/
void mt_gpio_resume(void)
{
	/* compatible with HAL */
}

/*---------------------------------------------------------------------------*/
#endif				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
