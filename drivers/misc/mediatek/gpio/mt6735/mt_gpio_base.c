/******************************************************************************
 * mt_gpio_base.c - MTKLinux GPIO Device Driver
 *
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 *
 * DESCRIPTION:
 *     This file provid the other drivers GPIO relative functions
 *
 ******************************************************************************/


#include <6735_gpio.h>
#include <linux/types.h>
#include "mt-plat/sync_write.h"
#include <mt-plat/mt_gpio.h>
#include <mt-plat/mt_gpio_core.h>
#include <mt_gpio_base.h>
/* autogen */
#include <gpio_cfg.h>
#ifdef CONFIG_OF
#include <linux/of_address.h>
#endif

/* unsigned long GPIO_COUNT=0; */
/* unsigned long uart_base; */
/* #define GPIO_BRINGUP 1 */
long gpio_pull_select_unsupport[MAX_GPIO_PIN] = { 0 };
long gpio_pullen_unsupport[MAX_GPIO_PIN] = { 0 };
long gpio_smt_unsupport[MAX_GPIO_PIN] = { 0 };

struct mt_gpio_vbase gpio_vbase;

int mt6306_set_gpio_out(unsigned long pin, unsigned long output)
{
	GPIOERR("denali not support\n");
	return 0;
}

int mt6306_set_gpio_dir(unsigned long pin, unsigned long dir)
{
	GPIOERR("denali not support\n");
	return 0;
}


/*---------------------------------------------------------------------------*/
int mt_set_gpio_dir_base(unsigned long pin, unsigned long dir)
{
	unsigned long bit;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (dir >= GPIO_DIR_MAX)
		return -ERINVAL;
	/* GPIOERR("before  pin:%ld, dir:%ld\n",pin,dir); */
	/* pos = pin / MAX_GPIO_REG_BITS; */
	bit = DIR_offset[pin].offset;
#ifdef GPIO_BRINGUP
	reg = GPIO_RD32(DIR_addr[pin].addr);
	if (dir == GPIO_DIR_IN)
		reg &= (~(1 << bit));
	else
		reg |= (1 << bit);

	GPIO_WR32(DIR_addr[pin].addr, reg);
#else
	if (dir == GPIO_DIR_IN)
		GPIO_SET_BITS((1L << bit), DIR_addr[pin].addr + 8);
	else
		GPIO_SET_BITS((1L << bit), DIR_addr[pin].addr + 4);
#endif

	/* GPIOERR("%s:pin:%ld, dir:%ld, value:0x%x\n",__FUNCTION__, pin, dir, GPIO_RD32(DIR_addr[pin].addr)); */
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_dir_base(unsigned long pin)
{
	unsigned long bit;
	unsigned long reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	bit = DIR_offset[pin].offset;

	reg = GPIO_RD32(DIR_addr[pin].addr);
	return ((reg & (1L << bit)) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_enable_base(unsigned long pin, unsigned long enable)
{
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;



#ifdef GPIO_BRINGUP
	bit = PULLEN_offset[pin].offset;
	if (PULLEN_offset[pin].offset == -1)
		return GPIO_PULL_EN_UNSUPPORTED;
	reg = GPIO_RD32(PULLEN_addr[pin].addr);
	if (enable == GPIO_PULL_DISABLE)
		reg &= (~(1 << bit));
	else
		reg |= (1 << bit);

	GPIO_WR32(PULLEN_addr[pin].addr, reg);
#else

	if (PULLEN_offset[pin].offset == -1) {
		gpio_pullen_unsupport[pin] = -1;
		return GPIO_PULL_EN_UNSUPPORTED;
	}

	if (enable == GPIO_PULL_DISABLE)
		GPIO_SET_BITS((1L << (PULLEN_offset[pin].offset)),
				      PULLEN_addr[pin].addr + 8);
	else
		GPIO_SET_BITS((1L << (PULLEN_offset[pin].offset)),
				      PULLEN_addr[pin].addr + 4);

#endif

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_enable_base(unsigned long pin)
{
	unsigned long data;
	u32 bit = 0;


	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (PULLEN_offset[pin].offset == -1 && pupd_offset[pin].offset == -1)
		return GPIO_PULL_EN_UNSUPPORTED;

	if (PULLEN_offset[pin].offset != -1) {
		bit = PULLEN_offset[pin].offset;
		data = GPIO_RD32(PULLEN_addr[pin].addr);
		return ((data & (1L << bit)) != 0) ? 1 : 0;
	}

	bit = pupd_offset[pin].offset;
	data = GPIO_RD32(pupd_addr[pin].addr);
	return ((data & (0x7 << bit)) != 0) ? 1 : 0;

}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_smt_base(unsigned long pin, unsigned long enable)
{
	/* unsigned long flags; */

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
#ifdef GPIO_BRINGUP

	if (SMT_offset[pin].offset == -1) {
		return GPIO_SMT_UNSUPPORTED;

	} else {
		bit = SMT_offset[pin].offset;
		reg = GPIO_RD32(SMT_addr[pin].addr);
		if (enable == GPIO_SMT_DISABLE)
			reg &= (~(1 << bit));
		else
			reg |= (1 << bit);
	}
	/* printk("SMT addr(%x),value(%x)\n",SMT_addr[pin].addr,GPIO_RD32(SMT_addr[pin].addr)); */
	GPIO_WR32(SMT_addr[pin].addr, reg);
#else

	if (SMT_offset[pin].offset == -1) {
		gpio_smt_unsupport[pin] = -1;
		return GPIO_SMT_UNSUPPORTED;
	}

	if (enable == GPIO_SMT_DISABLE)
		GPIO_SET_BITS((1L << (SMT_offset[pin].offset)), SMT_addr[pin].addr + 8);
	else
		GPIO_SET_BITS((1L << (SMT_offset[pin].offset)), SMT_addr[pin].addr + 4);

#endif

	/* GPIOERR("%s:pin:%ld, enable:%ld, value:0x%x\n",__FUNCTION__, pin, enable, GPIO_RD32(SMT_addr[pin].addr)); */

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_smt_base(unsigned long pin)
{
	unsigned long data;
	unsigned long bit = 0;

	bit = SMT_offset[pin].offset;
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (SMT_offset[pin].offset == -1)
		return GPIO_SMT_UNSUPPORTED;
	data = GPIO_RD32(SMT_addr[pin].addr);
	return ((data & (1L << bit)) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_ies_base(unsigned long pin, unsigned long enable)
{
	/* unsigned long flags; */

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset == -1)
		return GPIO_IES_UNSUPPORTED;

	if (enable == GPIO_IES_DISABLE)
		GPIO_SET_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr + 8);
	else
		GPIO_SET_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr + 4);

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_ies_base(unsigned long pin)
{
	unsigned long data;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset == -1)
		return GPIO_IES_UNSUPPORTED;

	data = GPIO_RD32(IES_addr[pin].addr);
	return ((data & (1L << (IES_offset[pin].offset))) != 0) ? 1 : 0;

}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_select_base(unsigned long pin, unsigned long select)
{
	/* unsigned long flags; */

	unsigned long bit = 0;
	/*u32 mask = (1L << 4) - 1;*/
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (pin == 5) {
		pr_err("XXX MSDC SD CD PIN be touched!\n");
		dump_stack();
	}
#ifdef GPIO_BRINGUP


	if (PULL_offset[pin].offset == -1 && pupd_offset[pin].offset == -1)
		return GPIO_PULL_UNSUPPORTED;

	if (pin >= 160 && pin <= 165) {

		bit = pupd_offset[pin].offset;
		reg = GPIO_RD32(pupd_addr[pin].addr);
		if (select == GPIO_PULL_UP) {	/* 0x2 pull up */
			reg &= (~(mask << bit));
			reg |= (0x2 << bit);
		} else {	/* 0x6 pull down */

			reg &= (~(mask << bit));
			reg |= (0x6 << bit);
		}
		/* printk("fwq pullset pin=%d,3.............\n",pin); */
		GPIO_WR32(pupd_addr[pin].addr, reg);
		return RSUCCESS;

	}

	/* printk("fwq pullset pin=%d,select(%d)\n",pin,select); */
	if (PULL_offset[pin].offset != -1) {

		bit = PULL_offset[pin].offset;
		reg = GPIO_RD32(PULL_addr[pin].addr);
		if (select == GPIO_PULL_DOWN)
			reg &= (~(1 << bit));
		else
			reg |= (1 << bit);
		/* printk("fwq pullset pin=%d,2.............\n",pin); */
		GPIO_WR32(PULL_addr[pin].addr, reg);
	} else {
		bit = pupd_offset[pin].offset + 2;
		reg = GPIO_RD32(pupd_addr[pin].addr);
		if (select == GPIO_PULL_UP)
			reg &= (~(1 << bit));
		else
			reg |= (1 << bit);
		/* printk("fwq pullset pin=%d,3.............\n",pin); */
		GPIO_WR32(pupd_addr[pin].addr, reg);

	}

#else

	if ((PULL_offset[pin].offset == -1) && (pupd_offset[pin].offset == -1)) {
		gpio_pull_select_unsupport[pin] = -1;
		return GPIO_PULL_UNSUPPORTED;
	}


	/* printk("fwq pullset pin=%ld,1.............\n",pin); */
	if (pin >= 160 && pin <= 165) {

		if (select == GPIO_PULL_UP) {	/* 0x2 pull up */
			/* reg &= (~(mask << bit)); */
			/* reg |= (0x2 << bit); */
			GPIO_SET_BITS((2L << (pupd_offset[pin].offset)), pupd_addr[pin].addr + 4);
		} else {	/* 0x6 pull down */

			/* reg &= (~(mask << bit)); */
			/* reg |= (0x6 << bit); */
			GPIO_SET_BITS((6L << (pupd_offset[pin].offset)), pupd_addr[pin].addr + 4);
		}

		return RSUCCESS;

	}
	/* printk("fwq pullset pin=%ld,2.............\n",pin); */
	/* printk("fwq pullset pin=%d,select(%d)\n",pin,select); */
	if (PULL_offset[pin].offset != -1) {

		bit = PULL_offset[pin].offset;
		/* reg = GPIO_RD32(PULL_addr[pin].addr); */
		if (select == GPIO_PULL_DOWN)
			GPIO_SET_BITS((1L << bit), PULL_addr[pin].addr + 8);
		else
			GPIO_SET_BITS((1L << bit), PULL_addr[pin].addr + 4);
		/* printk("fwq pullset pin=%d,2.............\n",pin); */

	} else {

		bit = pupd_offset[pin].offset + 2;
		/* reg = GPIO_RD32(pupd_addr[pin].addr); */
		if (select == GPIO_PULL_UP)
			GPIO_SET_BITS((1L << (bit)), pupd_addr[pin].addr + 8);
		else
			GPIO_SET_BITS((1L << (bit)), pupd_addr[pin].addr + 4);

	}
#endif
/* GPIOERR("%s:pin:%ld, select:%ld, value:0x%x\n",__FUNCTION__, pin, select, GPIO_RD32(PULL_addr[pin].addr)); */

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_select_base(unsigned long pin)
{

	unsigned long data;
	unsigned long bit = 0;
	u32 mask = (1L << 4) - 1;

	if (pin >= 160 && pin <= 165) {

		bit = pupd_offset[pin].offset;
		data = GPIO_RD32(pupd_addr[pin].addr);
		return ((((data >> bit) & mask) == 0x2)) ? 1 : 0;
	}

	if ((PULL_offset[pin].offset == -1) && (pupd_offset[pin].offset == -1))
		return GPIO_PULL_UNSUPPORTED;
	if (PULL_offset[pin].offset == -1) {
		data = GPIO_RD32(pupd_addr[pin].addr);
		return ((data & (1L << (pupd_offset[pin].offset + 2))) != 0) ? 0 : 1;
	}

	data = GPIO_RD32(PULL_addr[pin].addr);
	return ((data & (1L << (PULL_offset[pin].offset))) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_inversion_base(unsigned long pin, unsigned long enable)
{				/*FIX-ME
				 */
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_inversion_base(unsigned long pin)
{				/*FIX-ME */
	return 0;		/* FIX-ME */
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_out_base(unsigned long pin, unsigned long output)
{
	unsigned long bit;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (output >= GPIO_OUT_MAX)
		return -ERINVAL;
/* GPIOERR("before pin:%ld, output:%ld\n",pin,output); */
	bit = DATAOUT_offset[pin].offset;

#ifdef GPIO_BRINGUP
	reg = GPIO_RD32(DATAOUT_addr[pin].addr);
	GPIOERR("before2 pin[%ld],addr=%lx\n", pin, DATAOUT_addr[pin].addr);
	if (output == GPIO_OUT_ZERO)
		reg &= (~(1 << bit));
	else
		reg |= (1 << bit);

	GPIO_WR32(DATAOUT_addr[pin].addr, reg);

#else

	if (output == GPIO_OUT_ZERO)
		GPIO_SET_BITS((1L << bit), DATAOUT_addr[pin].addr + 8);
	else
		GPIO_SET_BITS((1L << bit), DATAOUT_addr[pin].addr + 4);
#endif

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_out_base(unsigned long pin)
{
	unsigned long bit;
	unsigned long reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	bit = DATAOUT_offset[pin].offset;
	reg = GPIO_RD32(DATAOUT_addr[pin].addr);
	return ((reg & (1L << bit)) != 0) ? 1 : 0;



}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_in_base(unsigned long pin)
{

	unsigned long bit;
	unsigned long reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	bit = DIN_offset[pin].offset;
	reg = GPIO_RD32(DIN_addr[pin].addr);
	return ((reg & (1L << bit)) != 0) ? 1 : 0;

}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_mode_base(unsigned long pin, unsigned long mode)
{
	unsigned long bit;
	unsigned long reg;
	unsigned long mask = (1L << GPIO_MODE_BITS) - 1;


	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (mode >= GPIO_MODE_MAX)
		return -ERINVAL;

	/* pos = pin / MAX_GPIO_MODE_PER_REG; */
	bit = MODE_offset[pin].offset;

	/* GPIOERR("modebefore1 set pin[%ld],addr=%lx,(%p,%p)\n", pin, MODE_addr[pin].addr, GPIO_BASE,
		(GPIO_BASE + MODE_addr[pin].addr)); */

/* #ifdef GPIO_BRINGUP */
#if 1

	mode = mode & mask;
	/* printk("fwq pin=%d,mode=%d, offset=%d\n",pin,mode,bit); */

	reg = GPIO_RD32(MODE_addr[pin].addr);
	/* printk("fwq pin=%d,beforereg[%x]=%x\n",pin,MODE_addr[pin].addr,reg); */

	reg &= (~(mask << bit));
	/* printk("fwq pin=%d,clr=%x\n",pin,~(mask << (GPIO_MODE_BITS*bit))); */
	reg |= (mode << bit);
	/* printk("fwq pin=%d,reg[%x]=%x\n",pin,MODE_addr[pin].addr,reg); */

	GPIO_WR32(MODE_addr[pin].addr, reg);
	/* printk("fwq pin=%d,moderead=%x\n",pin,GPIO_RD32(MODE_addr[pin].addr)); */

#else

	if (0 == mode)
		GPIO_SET_BITS((mask << bit), MODE_addr[pin].addr + 8);
	else {
		/* GPIO_SET_BITS((mask << bit), MODE_addr[pin].addr+8); */
		GPIO_SET_BITS((mode << bit), MODE_addr[pin].addr + 4);
	}


#endif
	/* GPIOERR("%s:pin:%ld, mode:%ld, value:0x%x\n", __func__, pin, mode,
		GPIO_RD32(MODE_addr[pin].addr)); */

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_mode_base(unsigned long pin)
{

	unsigned long bit;
	unsigned long reg;
	unsigned long mask = (1L << GPIO_MODE_BITS) - 1;
	/* struct mt_gpio_obj *obj = gpio_obj; */

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;


	bit = MODE_offset[pin].offset;
	/* printf("fwqread bit=%d,offset=%d",bit,MODE_offset[pin].offset); */
	/* reg = GPIO_RD32(&obj->reg->mode[pos].val); */
	reg = GPIO_RD32(MODE_addr[pin].addr);
	/* printf("fwqread  pin=%d,moderead=%x, bit=%d\n",pin,GPIO_RD32(MODE_addr[pin].addr),bit); */
	return (reg >> bit) & mask;
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
		/* gpio_reg = (GPIO_REGS*)(GPIO_BASE); */
		GPIOERR("GPIO base add is 0x%p\n", gpio_vbase.gpio_regs);
	}
	GPIOERR("GPIO base addr is 0x%p, %s\n", gpio_vbase.gpio_regs, __func__);
}

/*-----------------------User need GPIO APIs before GPIO probe------------------*/

static int __init get_gpio_vbase_early(void)
{
	/* void __iomem *gpio_base = NULL; */
	struct device_node *np_gpio = NULL;

	gpio_vbase.gpio_regs = NULL;
	np_gpio = get_gpio_np();
	/* Setup IO addresses */
	gpio_vbase.gpio_regs = of_iomap(np_gpio, 0);
	if (!gpio_vbase.gpio_regs) {
		GPIOERR("GPIO base addr is NULL\n");
		return 0;
	}
	/* gpio_reg = (GPIO_REGS*)(GPIO_BASE); */
	GPIOERR("GPIO base addr is 0x%p, %s\n", gpio_vbase.gpio_regs, __func__);
	return 0;
}

postcore_initcall(get_gpio_vbase_early);
/*---------------------------------------------------------------------------*/
void get_io_cfg_vbase(void)
{
	/* compatible with HAL */
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
