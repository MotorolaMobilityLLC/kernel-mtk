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

#include "6797_gpio.h"
#include <linux/types.h>
#include "mt-plat/sync_write.h"
#include <mt-plat/mt_gpio.h>
#include <mt-plat/mt_gpio_core.h>
#include "mt_gpio_base.h"
#include "gpio_cfg.h"
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

void __iomem *getpinaddr(unsigned long pin, PIN_addr pin_addr[])
{
	if (0 == strcmp(pin_addr[pin].s1, "GPIO_BASE"))
		return GPIO_BASE + pin_addr[pin].offset;
	if (0 == strcmp(pin_addr[pin].s1, "IOCFG_L_BASE"))
		return IOCFG_L_BASE + pin_addr[pin].offset;
	if (0 == strcmp(pin_addr[pin].s1, "IOCFG_B_BASE"))
		return IOCFG_B_BASE + pin_addr[pin].offset;
	if (0 == strcmp(pin_addr[pin].s1, "IOCFG_R_BASE"))
		return IOCFG_R_BASE + pin_addr[pin].offset;
	if (0 == strcmp(pin_addr[pin].s1, "IOCFG_T_BASE"))
		return IOCFG_T_BASE + pin_addr[pin].offset;
	if (0 == strcmp(pin_addr[pin].s1, "MIPI_TX0_BASE"))
		return MIPI_TX0_BASE + pin_addr[pin].offset;
	if (0 == strcmp(pin_addr[pin].s1, "MIPI_TX1_BASE"))
		return MIPI_TX1_BASE + pin_addr[pin].offset;
	if (0 == strcmp(pin_addr[pin].s1, "MIPI_RX_ANA_BASE"))
		return MIPI_RX_ANA_BASE + pin_addr[pin].offset;

	return (void __iomem *)(-1);

}

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
#ifdef GPIO_BRINGUP
	unsigned long reg;
#endif
	void __iomem *addr;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (dir >= GPIO_DIR_MAX)
		return -ERINVAL;
	addr = getpinaddr(pin, DIR_addr);
	bit = DIR_offset[pin].offset;
#ifdef GPIO_BRINGUP
	reg = GPIO_RD32(addr);
	if (dir == GPIO_DIR_IN)
		reg &= (~(1 << bit));
	else
		reg |= (1 << bit);

	GPIO_WR32(addr, reg);
#else
	if (dir == GPIO_DIR_IN)
		GPIO_SET_BITS((1L << bit), addr + 8);
	else
		GPIO_SET_BITS((1L << bit), addr + 4);
#endif

	/* GPIOERR("%s:pin:%ld, dir:%ld, value:0x%x\n",__FUNCTION__, pin, dir, GPIO_RD32(DIR_addr[pin].addr)); */
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_dir_base(unsigned long pin)
{
	unsigned long bit;
	unsigned long reg;
	void __iomem *addr;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;


	addr = getpinaddr(pin, DIR_addr);
	bit = DIR_offset[pin].offset;

	reg = GPIO_RD32(addr);
	return ((reg & (1L << bit)) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_enable_base(unsigned long pin, unsigned long enable)
{
	int err = 0;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
	if ((PUPD_offset[pin].offset == -1) && -1 == PU_offset[pin].offset
	    && -1 == PD_offset[pin].offset)
		return GPIO_PULL_UNSUPPORTED;

	if (GPIO_PULL_DISABLE == enable) {
		/*r0==0 && r1 ==0 */
		err = mt_set_gpio_pull_select_base(pin, GPIO_NO_PULL);
		return err;
	}
	if (GPIO_PULL_ENABLE == enable) {
		err = mt_set_gpio_pull_select_base(pin, GPIO_PULL_UP);
		return err;
	}
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_enable_base(unsigned long pin)
{
	int res = 0;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
	res = mt_get_gpio_pull_select_base(pin);

	if (2 == res)
		return 0;	/*disable */
	if (1 == res || 0 == res)
		return 1;	/*enable */
	if (-1 == res)
		return -1;
	return -ERWRAPPER;

}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_smt_base(unsigned long pin, unsigned long enable)
{
	/* unsigned long flags; */
	void __iomem *addr;
#ifdef GPIO_BRINGUP
	unsigned long reg;
	unsigned long bit = 0;
#endif

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	addr = getpinaddr(pin, SMT_addr);
#ifdef GPIO_BRINGUP

	if (SMT_offset[pin].offset == -1) {
		return GPIO_SMT_UNSUPPORTED;

	} else {
		bit = SMT_offset[pin].offset;
		reg = GPIO_RD32(addr);
		if (enable == GPIO_SMT_DISABLE)
			reg &= (~(1 << bit));
		else
			reg |= (1 << bit);
	}
	/* printk("SMT addr(%x),value(%x)\n",SMT_addr[pin].addr,GPIO_RD32(SMT_addr[pin].addr)); */
	GPIO_WR32(addr, reg);
#else

	if (SMT_offset[pin].offset == -1) {
		gpio_smt_unsupport[pin] = -1;
		return GPIO_SMT_UNSUPPORTED;
	}

	if (enable == GPIO_SMT_DISABLE)
		GPIO_SET_BITS((1L << (SMT_offset[pin].offset)), addr + 8);
	else
		GPIO_SET_BITS((1L << (SMT_offset[pin].offset)), addr + 4);

#endif

	/* GPIOERR("%s:pin:%ld, enable:%ld, value:0x%x\n",__FUNCTION__, pin, enable, GPIO_RD32(SMT_addr[pin].addr)); */

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_smt_base(unsigned long pin)
{
	unsigned long data;
	unsigned long bit = 0;
	void __iomem *addr;

	addr = getpinaddr(pin, SMT_addr);

	bit = SMT_offset[pin].offset;
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (SMT_offset[pin].offset == -1)
		return GPIO_SMT_UNSUPPORTED;
	data = GPIO_RD32(addr);
	return ((data & (1L << bit)) != 0) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_ies_base(unsigned long pin, unsigned long enable)
{
	/* unsigned long flags; */
	void __iomem *addr;

	addr = getpinaddr(pin, IES_addr);

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset == -1)
		return GPIO_IES_UNSUPPORTED;

	if (enable == GPIO_IES_DISABLE)
		GPIO_SET_BITS((1L << (IES_offset[pin].offset)), addr + 8);
	else
		GPIO_SET_BITS((1L << (IES_offset[pin].offset)), addr + 4);

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_ies_base(unsigned long pin)
{
	unsigned long data;
	void __iomem *addr;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (IES_offset[pin].offset == -1)
		return GPIO_IES_UNSUPPORTED;
	addr = getpinaddr(pin, IES_addr);

	data = GPIO_RD32(addr);
	return ((data & (1L << (IES_offset[pin].offset))) != 0) ? 1 : 0;

}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_slew_rate_base(unsigned long pin, unsigned long enable)
{
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_slew_rate_base(unsigned long pin)
{
	return GPIO_SLEW_RATE_UNSUPPORTED;
}

s32 mt_set_gpio_pull_select_rx_chip(u32 pin, u32 r0, u32 r1)
{
#ifdef GPIO_BRINGUP
	u32 reg = 0;
#endif
	void __iomem *r0_addr;
	void __iomem *r1_addr;

	r0_addr = getpinaddr(pin, R0_addr);
	r1_addr = getpinaddr(pin, R1_addr);
#ifdef GPIO_BRINGUP
	if (0 == r0) {
		reg = GPIO_RD32(r0_addr);
		reg &= (~(1 << R0_offset[pin].offset));
		GPIO_WR32(r0_addr, reg);
	}
	if (1 == r0) {
		reg = GPIO_RD32(r0_addr);
		reg |= (1 << R0_offset[pin].offset);
		GPIO_WR32(r0_addr, reg);
	}

	if (0 == r1) {
		reg = GPIO_RD32(r1_addr);
		reg &= (~(1 << R1_offset[pin].offset));
		GPIO_WR32(r1_addr, reg);
	}
	if (1 == r1) {
		reg = GPIO_RD32(r1_addr);
		reg |= (1 << R1_offset[pin].offset);
		GPIO_WR32(r1_addr, reg);
	}
#else
	if (0 == r0)
		GPIO_SET_BITS((1L << (R0_offset[pin].offset)), r0_addr + 8);
	if (1 == r0)
		GPIO_SET_BITS((1L << (R0_offset[pin].offset)), r0_addr + 4);

	if (0 == r1)
		GPIO_SET_BITS((1L << (R1_offset[pin].offset)), r1_addr + 8);
	if (1 == r1)
		GPIO_SET_BITS((1L << (R1_offset[pin].offset)), r1_addr + 4);
#endif
	return 0;
}


/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_select_base(unsigned long pin, unsigned long select)
{

#ifdef GPIO_BRINGUP
	u32 reg = 0;
	u32 pu_reg = 0;
	u32 pd_reg = 0;
	unsigned long pupd_bit = 0;
	unsigned long pu_bit = 0;
	unsigned long pd_bit = 0;
#endif
	void __iomem *puaddr;
	void __iomem *pdaddr;
	void __iomem *pupdaddr;


	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
	puaddr = getpinaddr(pin, PU_addr);
	pdaddr = getpinaddr(pin, PD_addr);
	pupdaddr = getpinaddr(pin, PUPD_addr);
#ifdef GPIO_BRINGUP


	if ((PUPD_offset[pin].offset == -1) && -1 == PU_offset[pin].offset
	    && -1 == PD_offset[pin].offset)
		return GPIO_PULL_UNSUPPORTED;

	if (-1 != PUPD_offset[pin].offset) {

		/*PULL UP */
		mt_set_gpio_pull_select_rx_chip(pin, 0, 1);	/*must first set default value r0=0, r1 =1 */
		pupd_bit = PUPD_offset[pin].offset;
		reg = GPIO_RD32(pupdaddr);
		if (select == GPIO_PULL_UP)
			reg &= (~(1 << pupd_bit));
		else if (select == GPIO_PULL_DOWN)
			reg |= (1 << pupd_bit);
		else if (GPIO_NO_PULL == select)
			mt_set_gpio_pull_select_rx_chip(pin, 0, 0);
		else
			return -ERWRAPPER;
		GPIO_WR32(pupdaddr, reg);
		return RSUCCESS;
	}

	if (-1 != PU_offset[pin].offset && -1 != PD_offset[pin].offset) {

		pu_bit = PU_offset[pin].offset;
		pd_bit = PD_offset[pin].offset;

		pu_reg = GPIO_RD32(puaddr);
		pd_reg = GPIO_RD32(pdaddr);

		if (select == GPIO_PULL_UP) {
			pu_reg |= (1 << pu_bit);
			pd_reg &= (~(1 << pd_bit));
		} else if (GPIO_PULL_DOWN == select) {
			pu_reg &= (~(1 << pu_bit));
			pd_reg |= (1 << pd_bit);
		} else if (GPIO_NO_PULL) {
			pd_reg &= (~(1 << pd_bit));
			pu_reg &= (~(1 << pu_bit));
		} else {
			return -ERWRAPPER;
		}
		GPIO_WR32(puaddr, pu_reg);
		GPIO_WR32(pdaddr, pd_reg);

		return RSUCCESS;

	} else if (-1 != PU_offset[pin].offset) {	/*pu==1 is pull up, pu ==0  is no pull */
		pu_reg = GPIO_RD32(puaddr);
		pu_bit = PU_offset[pin].offset;
		if (select == GPIO_PULL_UP)
			pu_reg |= (1 << pu_bit);
		else if (GPIO_PULL_DOWN == select)
			return GPIO_NOPULLDOWN;
		else if (GPIO_NO_PULL)
			pu_reg &= (~(1 << pu_bit));
		else
			return -ERWRAPPER;

		GPIO_WR32(puaddr, pu_reg);
		return RSUCCESS;
	} else if (-1 != PD_offset[pin].offset) {
		pd_reg = GPIO_RD32(pdaddr);
		pd_bit = PD_offset[pin].offset;
		if (select == GPIO_PULL_UP)
			return GPIO_NOPULLUP;
		else if (GPIO_PULL_DOWN == select)
			pd_reg |= (1 << pd_bit);
		else if (GPIO_NO_PULL)
			pd_reg &= (~(1 << pd_bit));
		else
			return -ERWRAPPER;

		GPIO_WR32(pdaddr, pd_reg);
		return RSUCCESS;
	}

#else

	if ((PUPD_offset[pin].offset == -1) && (-1 == PU_offset[pin].offset)
	    && (-1 == PD_offset[pin].offset)) {
		return GPIO_PULL_UNSUPPORTED;
	}

	if (-1 != PUPD_offset[pin].offset) {
		if (select == GPIO_PULL_UP)
			GPIO_SET_BITS((1L << (PUPD_offset[pin].offset)), pupdaddr + 8);
		else if (GPIO_PULL_DOWN == select)
			GPIO_SET_BITS((1L << (PUPD_offset[pin].offset)), pupdaddr + 4);
		else if (GPIO_NO_PULL)
			mt_set_gpio_pull_select_rx_chip(pin, 0, 0);
		else
			return -ERWRAPPER;

		return RSUCCESS;
	}

	if (-1 != PU_offset[pin].offset && -1 != PD_offset[pin].offset) {
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), puaddr + 4);
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), pdaddr + 8);
		} else if (GPIO_PULL_DOWN == select) {
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), pdaddr + 4);
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), puaddr + 8);
		} else if (GPIO_NO_PULL) {
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), pdaddr + 8);
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), puaddr + 8);
		} else
			return -ERWRAPPER;

		return RSUCCESS;
	} else if (-1 != PU_offset[pin].offset) {
		if (select == GPIO_PULL_UP)
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), puaddr + 4);
		else if (GPIO_PULL_DOWN == select)
			return GPIO_NOPULLDOWN;
		else if (GPIO_NO_PULL)
			GPIO_SET_BITS((1L << (PU_offset[pin].offset)), puaddr + 8);
		else
			return -ERWRAPPER;

		return RSUCCESS;
	} else if (-1 != PD_offset[pin].offset) {
		if (select == GPIO_PULL_UP)
			return GPIO_NOPULLUP;
		else if (GPIO_PULL_DOWN == select)
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), pdaddr + 4);
		else if (GPIO_NO_PULL)
			GPIO_SET_BITS((1L << (PD_offset[pin].offset)), pdaddr + 8);
		else
			return -ERWRAPPER;

		return RSUCCESS;
	}

#endif
	return -ERINVAL;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_select_base(unsigned long pin)
{
	unsigned long data;
	unsigned long r0;
	unsigned long r1;

	unsigned long pu = 0;
	unsigned long pd = 0;
	void __iomem *puaddr;
	void __iomem *pdaddr;
	void __iomem *pupdaddr;
	void __iomem *r0_addr;
	void __iomem *r1_addr;

	if ((PUPD_offset[pin].offset == -1) && -1 == PU_offset[pin].offset
	    && -1 == PD_offset[pin].offset) {
		return GPIO_PULL_UNSUPPORTED;
	}
	puaddr = getpinaddr(pin, PU_addr);
	pdaddr = getpinaddr(pin, PD_addr);
	pupdaddr = getpinaddr(pin, PUPD_addr);
	r0_addr = getpinaddr(pin, R0_addr);
	r1_addr = getpinaddr(pin, R1_addr);

	/*pupd==0 is pull up or no pull */
	if (-1 != PUPD_offset[pin].offset) {
		data = GPIO_RD32(pupdaddr);
		r0 = GPIO_RD32(r0_addr);
		r1 = GPIO_RD32(r1_addr);
		if (0 == (data & (1L << (PUPD_offset[pin].offset)))) {
			if (0 == (r0 & (1L << (R0_offset[pin].offset)))
			    && 0 == (r1 & (1L << (R1_offset[pin].offset))))
				return GPIO_NO_PULL;	/*High Z(no pull) */
			else
				return GPIO_PULL_UP;	/* pull up */
		}
		/*pupd==1 is pull down or no pull */
		if (0 != (data & (1L << (PUPD_offset[pin].offset)))) {
			if (0 == (r0 & (1L << (R0_offset[pin].offset)))
			    && 0 == (r1 & (1L << (R1_offset[pin].offset))))
				return GPIO_NO_PULL;	/*High Z(no pull) */
			else
				return GPIO_PULL_DOWN;	/* pull down */
		}
	}

	if (-1 != PU_offset[pin].offset && -1 != PD_offset[pin].offset) {
		pu = GPIO_RD32(puaddr);
		pd = GPIO_RD32(pdaddr);
		if (0 != (pu & (1L << (PU_offset[pin].offset))))
			pu = 1;
		else
			pu = 0;
		if (0 != (pd & (1L << (PD_offset[pin].offset))))
			pd = 1;
		else
			pd = 0;


		if (0 == pu && 0 == pd)	/*no pull */
			return GPIO_NO_PULL;
		else if (1 == pu && 1 == pd)
			return -1;	/* err */
		else if (1 == pu && 0 == pd)
			return GPIO_PULL_UP;	/* pull up */
		else
			return GPIO_PULL_DOWN;	/* pull down */

	} else if (-1 != PU_offset[pin].offset) {	/*pu==1 is pull up, pu ==0  is no pull */
		pu = GPIO_RD32(puaddr);

		if (0 != (pu & (1L << (PU_offset[pin].offset))))
			return GPIO_PULL_UP;	/* pull up */
		else
			return GPIO_NO_PULL;	/*High Z(no pull) */
	} else {
		pd = GPIO_RD32(pdaddr);
		if (0 != (pd & (1L << (PD_offset[pin].offset))))
			return GPIO_PULL_DOWN;	/* pull down */
		else
			return GPIO_NO_PULL;	/*High Z(no pull) */
	}
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_resistor_base(unsigned long pin, unsigned long resistors)
{
	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (-1 != PUPD_offset[pin].offset) {
		return mt_set_gpio_pull_select_rx_chip(pin, resistors & GPIO_PULL_R0,
			(resistors & GPIO_PULL_R1) >> 1);
	}
	return GPIO_PULL_RESISTOR_UNSUPPORTED;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_resistor_base(unsigned long pin)
{
	unsigned long resistors = 0;
	void __iomem *addr;
	unsigned long bit;
	unsigned long reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (-1 != PUPD_offset[pin].offset) {
		addr = getpinaddr(pin, R0_addr);
		bit = R0_offset[pin].offset;
		reg = GPIO_RD32(addr);
		resistors |= ((reg & (1L << bit)) != 0) ? GPIO_PULL_R0 : 0;
		addr = getpinaddr(pin, R1_addr);
		bit = R1_offset[pin].offset;
		reg = GPIO_RD32(addr);
		return resistors |= ((reg & (1L << bit)) != 0) ? GPIO_PULL_R1 : 0;
	}
	return GPIO_PULL_RESISTOR_UNSUPPORTED;
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
	void __iomem *addr;
#ifdef GPIO_BRINGUP
	unsigned long reg;
#endif

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (output >= GPIO_OUT_MAX)
		return -ERINVAL;
/* GPIOERR("before pin:%ld, output:%ld\n",pin,output); */
	bit = DATAOUT_offset[pin].offset;
	addr = getpinaddr(pin, DATAOUT_addr);

#ifdef GPIO_BRINGUP
	reg = GPIO_RD32(addr);
	if (output == GPIO_OUT_ZERO)
		reg &= (~(1 << bit));
	else
		reg |= (1 << bit);

	GPIO_WR32(addr, reg);

#else

	if (output == GPIO_OUT_ZERO)
		GPIO_SET_BITS((1L << bit), addr + 8);
	else
		GPIO_SET_BITS((1L << bit), addr + 4);
#endif

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_out_base(unsigned long pin)
{
	unsigned long bit;
	unsigned long reg;
	void __iomem *addr;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
	addr = getpinaddr(pin, DATAOUT_addr);
	bit = DATAOUT_offset[pin].offset;
	reg = GPIO_RD32(addr);
	return ((reg & (1L << bit)) != 0) ? 1 : 0;



}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_in_base(unsigned long pin)
{

	unsigned long bit;
	unsigned long reg;
	void __iomem *addr;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
	addr = getpinaddr(pin, DATAIN_addr);
	bit = DATAIN_offset[pin].offset;
	reg = GPIO_RD32(addr);
	return ((reg & (1L << bit)) != 0) ? 1 : 0;

}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_mode_base(unsigned long pin, unsigned long mode)
{
	unsigned long bit;
	unsigned long reg;
	unsigned long mask = (1L << GPIO_MODE_BITS) - 1;
	void __iomem *addr;


	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (mode >= GPIO_MODE_MAX)
		return -ERINVAL;

	/* pos = pin / MAX_GPIO_MODE_PER_REG; */
	bit = MODE_offset[pin].offset;
	addr = getpinaddr(pin, MODE_addr);

/* #ifdef GPIO_BRINGUP */
#if 1
	if (MODEXT_offset[pin].offset != -1) {
		reg = GPIO_RD32(GPIO_BASE + 0x600);
		reg &= (~(0x1 << MODEXT_offset[pin].offset));
		reg |= ((mode >> 0x3) << MODEXT_offset[pin].offset);
		GPIO_WR32(GPIO_BASE + 0x600, reg);
	}

	mode = mode & 0x7;
	/* printk("fwq pin=%d,mode=%d, offset=%d\n",pin,mode,bit); */

	reg = GPIO_RD32(addr);
	/* printk("fwq pin=%d,beforereg[%x]=%x\n",pin,MODE_addr[pin].addr,reg); */

	reg &= (~(mask << bit));
	/* printk("fwq pin=%d,clr=%x\n",pin,~(mask << (GPIO_MODE_BITS*bit))); */
	reg |= (mode << bit);
	/* printk("fwq pin=%d,reg[%x]=%x\n",pin,MODE_addr[pin].addr,reg); */

	GPIO_WR32(addr, reg);
	/* printk("fwq pin=%d,moderead=%x\n",pin,GPIO_RD32(MODE_addr[pin].addr)); */

#else

	if (0 == mode)
		GPIO_SET_BITS((mask << bit), MODE_addr[pin].addr + 8);
	else {
		/* GPIO_SET_BITS((mask << bit), MODE_addr[pin].addr+8); */
		GPIO_SET_BITS((mode << bit), MODE_addr[pin].addr + 4);
	}


#endif
	/* GPIOERR("%s:pin:%ld, mode:%ld, value:0x%x\n", __func__, pin, mode, GPIO_RD32(addr)); */

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_mode_base(unsigned long pin)
{

	unsigned long bit;
	unsigned long reg;
	unsigned long mask = (1L << GPIO_MODE_BITS) - 1;
	void __iomem *addr;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	addr = getpinaddr(pin, MODE_addr);
	bit = MODE_offset[pin].offset;
	/* printf("fwqread bit=%d,offset=%d",bit,MODE_offset[pin].offset); */
	/* reg = GPIO_RD32(&obj->reg->mode[pos].val); */
	reg = GPIO_RD32(addr);
	/* printf("fwqread  pin=%d,moderead=%x, bit=%d\n",pin,GPIO_RD32(MODE_addr[pin].addr),bit); */
	if (MODEXT_offset[pin].offset != -1 &&
		(GPIO_RD32(GPIO_BASE + 0x600) >> MODEXT_offset[pin].offset) & 0x1)
		return ((reg >> bit) & mask) | 0x8;
	return (reg >> bit) & mask;
}

int mt_set_gpio_driving_base(unsigned long pin, unsigned long strength)
{
	u32 bit;
#ifdef GPIO_BRINGUP
	u32 reg = 0;
#endif
	void __iomem *addr;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	addr = getpinaddr(pin, DRV_addr);
	bit = DRV_offset[pin].offset;
	if (-1 == DRV_width[pin].width)
		return -ERWRAPPER;

#ifdef GPIO_BRINGUP
	if (3 == DRV_width[pin].width) {
		if (strength > 0x7)
			return -ERWRAPPER;
		reg = GPIO_RD32(addr);
		reg &= ~(0x7 << bit);
		GPIO_WR32(addr, reg);

		reg = GPIO_RD32(addr);
		reg |= (strength << bit);
		GPIO_WR32(addr, reg);
	} else if (2 == DRV_width[pin].width) {
		if (strength > 0x3)
			return -ERWRAPPER;
		reg = GPIO_RD32(addr);
		reg &= ~(0x3 << bit);
		GPIO_WR32(addr, reg);

		reg = GPIO_RD32(addr);
		reg |= (strength << bit);
		GPIO_WR32(addr, reg);

	} else if (1 == DRV_width[pin].width) {
		if (strength > 1)
			return -ERWRAPPER;
		reg = GPIO_RD32(addr);
		reg &= ~(0x1 << bit);
		GPIO_WR32(addr, reg);

		reg = GPIO_RD32(addr);
		reg |= (strength << bit);
		GPIO_WR32(addr, reg);
	}

#else
	if (3 == DRV_width[pin].width) {
		if (strength > 0x7)
			return -ERWRAPPER;

		GPIO_SET_BITS((0x7 << bit), addr + 8);
		GPIO_SET_BITS((strength << bit), addr + 4);

	} else if (2 == DRV_width[pin].width) {
		if (strength > 0x3)
			return -ERWRAPPER;

		GPIO_SET_BITS((0x3 << bit), addr + 8);
		GPIO_SET_BITS((strength << bit), addr + 4);

	} else if (1 == DRV_width[pin].width) {
		if (strength > 1)
			return -ERWRAPPER;

		GPIO_SET_BITS((0x1 << bit), addr + 8);
		GPIO_SET_BITS((strength << bit), addr + 4);

	}


#endif
	return RSUCCESS;

}

int mt_get_gpio_driving_base(unsigned long pin)
{
	unsigned long bit;
	void __iomem *addr;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;
	if (-1 == DRV_width[pin].width)
		return -ERWRAPPER;

	addr = getpinaddr(pin, DRV_addr);
	bit = DRV_offset[pin].offset;

	if (3 == DRV_width[pin].width)
		return (GPIO_RD32(addr) >> bit) & 0x7;
	else if (2 == DRV_width[pin].width)
		return (GPIO_RD32(addr) >> bit) & 0x3;
	else if (1 == DRV_width[pin].width)
		return (GPIO_RD32(addr) >> bit) & 0x1;

	return -ERINVAL;
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
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_l");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.IOCFG_L_regs = of_iomap(node, 0);
		GPIOLOG("gpio_vbase.IOCFG_L_regs=0x%p\n", gpio_vbase.IOCFG_L_regs);
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_b");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.IOCFG_B_regs = of_iomap(node, 0);
		GPIOLOG("gpio_vbase.IOCFG_B_regs=0x%p\n", gpio_vbase.IOCFG_B_regs);
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_r");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.IOCFG_R_regs = of_iomap(node, 0);
		GPIOLOG("gpio_vbase.IOCFG_R_regs=0x%p\n", gpio_vbase.IOCFG_R_regs);
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,iocfg_t");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.IOCFG_T_regs = of_iomap(node, 0);
		GPIOLOG("gpio_vbase.IOCFG_T_regs=0x%p\n", gpio_vbase.IOCFG_T_regs);
	}
/*
	node = of_find_compatible_node(NULL, NULL, "mediatek,MIPI_TX0");
	if (node) {
		gpio_vbase.MIPI_TX0_regs = of_iomap(node, 0);
		GPIOLOG("gpio_vbase.MIPI_TX0_regs=0x%p\n", gpio_vbase.MIPI_TX0_regs);
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,MIPI_RX_ANA_CSI0");
	if (node) {
		gpio_vbase.MIPI_RX_CSI0_regs = of_iomap(node, 0);
		GPIOLOG("gpio_vbase.MIPI_RX_CSI0_regs=0x%p\n", gpio_vbase.MIPI_RX_CSI0_regs);
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,MIPI_RX_ANA_CSI1");
	if (node) {
		gpio_vbase.MIPI_RX_CSI1_regs = of_iomap(node, 0);
		GPIOLOG("gpio_vbase.MIPI_RX_CSI1_regs=0x%p\n", gpio_vbase.MIPI_RX_CSI1_regs);
	}
*/
#ifdef SELF_TEST

	gpio_vbase.gpio_regs = kzalloc(4096*sizeof(void __iomem *), GFP_KERNEL);
	GPIOLOG("test gpio_vbase.gpio_regs=0x%p\n", gpio_vbase.gpio_regs);

	gpio_vbase.IOCFG_L_regs = kzalloc(4096*sizeof(void __iomem *), GFP_KERNEL);
	GPIOLOG("test gpio_vbase.IOCFG_L_regs=0x%p\n", gpio_vbase.IOCFG_L_regs);

	gpio_vbase.IOCFG_B_regs = kzalloc(4096*sizeof(void __iomem *), GFP_KERNEL);
	GPIOLOG("test gpio_vbase.IOCFG_B_regs=0x%p\n", gpio_vbase.IOCFG_B_regs);

	gpio_vbase.IOCFG_R_regs = kzalloc(4096*sizeof(void __iomem *), GFP_KERNEL);
	GPIOLOG("test gpio_vbase.IOCFG_R_regs=0x%p\n", gpio_vbase.IOCFG_R_regs);

	gpio_vbase.IOCFG_T_regs = kzalloc(4096*sizeof(void __iomem *), GFP_KERNEL);
	GPIOLOG("test gpio_vbase.IOCFG_T_regs=0x%p\n", gpio_vbase.IOCFG_T_regs);
/*
	gpio_vbase.MIPI_TX0_regs = kzalloc(4096*sizeof(void __iomem *), GFP_KERNEL);
	GPIOLOG("test gpio_vbase.MIPI_TX0_regs=0x%lx\n", gpio_vbase.MIPI_TX0_regs);

	gpio_vbase.MIPI_RX_CSI0_regs = kzalloc(4096*sizeof(void __iomem *), GFP_KERNEL);
	GPIOLOG("test gpio_vbase.MIPI_RX_CSI0_regs=0x%lx\n", gpio_vbase.MIPI_RX_CSI0_regs);

	gpio_vbase.MIPI_RX_CSI1_regs = kzalloc(4096*sizeof(void __iomem *), GFP_KERNEL);
	GPIOLOG("test gpio_vbase.MIPI_RX_CSI1_regs=0x%lx\n", gpio_vbase.MIPI_RX_CSI1_regs);
*/
	mt_gpio_self_test_base();

#endif

}

#ifdef SELF_TEST
int smt_test(void)
{

	int i, val;
	int res;

	GPIOLOG("smt_test test+++++\n");

	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0);
		if (res)
			return -1;
		/*test*/
		for (val = 0; val < GPIO_SMT_MAX; val++) {
			GPIOLOG("test gpio[%d],smt[%d]\n", i, val);
			if (-1 == mt_set_gpio_smt(i|0x80000000, val)) {
				GPIOERR(" set smt unsupport\n");
				continue;
			}
			res = mt_set_gpio_smt(i|0x80000000, val);
			if (res != RSUCCESS) {
				GPIOERR(" set smt[%d] fail: %d\n", val, res);
				return -1;
			}
			if (val != mt_get_gpio_smt(i|0x80000000)) {
				GPIOERR(" get smt[%d] fail: real get %d\n", val, mt_get_gpio_smt(i|0x80000000));
				return -1;
			}
			if (mt_get_gpio_smt(i|0x80000000) > 1) {
				GPIOERR(" get smt[%d] value fail: real get %d\n", val, mt_get_gpio_smt(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOLOG("smt_test test----- PASS!\n");
	return 0;

}


int output_test(void)
{

	int i, val;
	int res;

	GPIOLOG("output test+++++\n");
	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0);
		if (res)
			return -1;
		res = mt_set_gpio_dir(i|0x80000000, GPIO_DIR_OUT);
		if (res)
			return -1;
		/*test*/
		for (val = 0; val < GPIO_OUT_MAX; val++) {
			GPIOLOG("test gpio[%d],output[%d]\n", i, val);
			res = mt_set_gpio_out(i|0x80000000, val);
			if (res != RSUCCESS) {
				GPIOERR(" set out[%d] fail: %d\n", val, res);
				return -1;
			}
			if (val != mt_get_gpio_out(i|0x80000000)) {
				GPIOERR(" get out[%d] fail: real get %d\n", val, mt_get_gpio_out(i|0x80000000));
				return -1;
			}
			if (mt_get_gpio_out(i|0x80000000) > 1) {
				GPIOERR(" get out[%d] value fail: real get %d\n", val, mt_get_gpio_out(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOLOG("output test----- PASS!\n");
	return 0;

}

int direction_test(void)
{
	int i, val;
	int res;

	GPIOLOG("direction_test test+++++\n");
	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0);
		if (res)
			return -1;

		/*test*/
		for (val = 0; val < GPIO_DIR_MAX; val++) {
			GPIOLOG("test gpio[%d],direction[%d]\n", i, val);
			res = mt_set_gpio_dir(i|0x80000000, val);
			if (res != RSUCCESS) {
				GPIOERR(" set direction[%d] fail: %d\n", val, res);
				return -1;
			}
			if (val != mt_get_gpio_dir(i|0x80000000)) {
				GPIOERR(" get direction[%d] fail1 real get %d\n",
					val, mt_get_gpio_dir(i|0x80000000));
				return -1;
			}
			if (mt_get_gpio_dir(i|0x80000000) > 1) {
				GPIOERR(" get direction[%d] value fail2 real get %d\n",
					val, mt_get_gpio_dir(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOLOG("direction_test----- PASS!\n");

	return 0;
}

int mode_test(void)
{
	int i, val;
	int res;

	GPIOLOG("mode_test test+++++\n");
	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*test*/
		for (val = 0; val < GPIO_MODE_MAX; val++) {
			GPIOLOG("test gpio[%d],mode[%d]\n", i, val);
			res = mt_set_gpio_mode(i|0x80000000, val);
			if (res != RSUCCESS) {
				GPIOERR(" set mode[%d] fail: %d\n", val, res);
				return -1;
			}
			if (val != mt_get_gpio_mode(i|0x80000000)) {
				GPIOERR(" get mode[%d] fail: real get %d\n", val, mt_get_gpio_mode(i|0x80000000));
				return -1;
			}
			if (mt_get_gpio_mode(i|0x80000000) > 7) {
				GPIOERR(" get mode[%d] value fail: real get %d\n", val, mt_get_gpio_mode(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOLOG("mode_test----- PASS!\n");

	return 0;
}


int pullen_test(void)
{
	int i, val;
	int res;

	GPIOLOG("pullen_test  +++++\n");
	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0);
		if (res)
			return -1;

		/*test*/
		for (val = 0; val < GPIO_PULL_EN_MAX; val++) {
			GPIOLOG("test gpio[%d],pullen[%d]\n", i, val);
			if (-1 == mt_set_gpio_pull_enable(i|0x80000000, val)) {
				GPIOERR(" set pull_enable unsupport\n");
				continue;
			}
			if (GPIO_NOPULLDOWN == mt_set_gpio_pull_enable(i|0x80000000, val)) {
				GPIOERR(" set pull_down unsupport\n");
				continue;
			}
			if (GPIO_NOPULLUP == mt_set_gpio_pull_enable(i|0x80000000, val)) {
				GPIOERR(" set pull_up unsupport\n");
				continue;
			}

			res = mt_set_gpio_pull_enable(i|0x80000000, val);
			if (res != RSUCCESS) {
				GPIOERR(" set pull_enable[%d] fail1 %d\n", val, res);
				return -1;
			}
			if (val != mt_get_gpio_pull_enable(i|0x80000000)) {
				GPIOERR(" get pull_enable[%d] fail2 real get %d\n",
					val, mt_get_gpio_pull_enable(i|0x80000000));
				return -1;
			}

			if (mt_get_gpio_pull_enable(i|0x80000000) > 1) {
				GPIOERR(" get pull_enable[%d] value fail3: real get %d\n",
					val, mt_get_gpio_pull_enable(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOLOG("pullen_test----- PASS!\n");

	return 0;
}

int pullselect_test(void)
{
	int i, val;
	int res;

	GPIOLOG("pullselect_test  +++++\n");
	for (i = 0; i < MT_GPIO_EXT_MAX; i++) {
		/*prepare test */
		res = mt_set_gpio_mode(i|0x80000000, 0);
		if (res)
			return -1;

		/*test*/
		for (val = 0; val < GPIO_PULL_MAX; val++) {
			GPIOLOG("test gpio[%d],pull_select[%d]\n", i, val);
			res = mt_set_gpio_pull_select(i|0x80000000, val);
			if (GPIO_PULL_UNSUPPORTED == res
				|| GPIO_NOPULLUP == res
				|| GPIO_NOPULLDOWN == res) {
				GPIOERR(" set gpio[%d] pull_select[%d] unsupport\n", i, val);
				continue;
			}

			res = mt_set_gpio_pull_select(i|0x80000000, val);
			if (res != RSUCCESS) {
				GPIOERR(" set pull_select[%d] fail1: %d\n", val, res);
				return -1;
			}
			if (val != mt_get_gpio_pull_select(i|0x80000000)) {
				GPIOERR(" get pull_select[%d] fail2: real get %d\n",
					val, mt_get_gpio_pull_select(i|0x80000000));
				return -1;
			}
			if (-1 == mt_get_gpio_pull_select(i|0x80000000)) {
				GPIOERR(" set gpio[%d] pull_select not support\n", i);
			} else if (mt_get_gpio_pull_select(i|0x80000000) > 2) {
				GPIOERR(" get pull_select[%d] value fail: real get %d\n",
					val, mt_get_gpio_pull_select(i|0x80000000));
				return -1;
			}
		}

	}
	GPIOLOG("pullselect_test----- PASS!\n");

	return 0;
}



void mt_gpio_self_test_base(void)
{
	int err = 0;

	GPIOLOG("GPIO self_test start 0817\n");
	err = mode_test();
	if (err) {
		GPIOLOG("GPIO self_test FAIL\n");
		return;
	}

	err = direction_test();
	if (err) {
		GPIOLOG("GPIO self_test FAIL\n");
		return;
	}

	err = output_test();
	if (err) {
		GPIOLOG("GPIO self_test FAIL\n");
		return;
	}

	err = smt_test();
	if (err) {
		GPIOLOG("GPIO self_test FAIL\n");
		return;
	}

	err = pullen_test();
	if (err) {
		GPIOLOG("GPIO self_test FAIL\n");
		return;
	}

	err = pullselect_test();
	if (err) {
		GPIOLOG("GPIO self_test FAIL\n");
		return;
	}
	GPIOLOG("GPIO self_test PASS\n");
}

#endif


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
