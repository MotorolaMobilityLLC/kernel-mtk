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

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include "mt_sd.h"

#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

void __iomem *fpga_pwr_gpio;
void __iomem *fpga_pwr_gpio_eo;

#define FPGA_PWR_GPIO                    (0xF0001E84)
#define FPGA_PWR_GPIO_EO                 (0xF0001E88)
#define PWR_GPIO                         (fpga_pwr_gpio)
#define PWR_GPIO_EO                      (fpga_pwr_gpio_eo)
#define PWR_GPIO_L4_DIR                  (1 << 11)
#define PWR_MASK_VOL_33                  (1 << 10)
#define PWR_MASK_VOL_18                  (1 << 9)
#define PWR_MASK_CARD                    (1 << 8)
#define PWR_MASK_VOL_33_MASK             (~(1 << 10))
#define PWR_MASK_CARD_MASK               (~(1 << 8))
#define PWR_MASK_VOL_18_MASK             (~(1 << 9))

void msdc_set_pwr_gpio_dir(void __iomem *fpga_pwr_gpio,
	void __iomem *fpga_pwr_gpio_eo)
{
	volatile u16 l_val;

	l_val = MSDC_READ16(PWR_GPIO_EO);
	MSDC_WRITE16(PWR_GPIO_EO,
		(l_val | /* PWR_GPIO_L4_DIR | */
		 PWR_MASK_CARD | PWR_MASK_VOL_33 | PWR_MASK_VOL_18));

	l_val = MSDC_READ16(PWR_GPIO_EO);

	pr_debug("[%s]: pwr gpio dir = 0x%x\n", __func__, l_val);
}

void msdc_fpga_pwr_init(void)
{
	if (fpga_pwr_gpio == NULL) {
		fpga_pwr_gpio = ioremap(FPGA_PWR_GPIO, 8);
		fpga_pwr_gpio_eo = fpga_pwr_gpio + 0x4;
		pr_err("FPAG PWR_GPIO, PWR_GPIO_EO address 0x%p, 0x%p\n",
			fpga_pwr_gpio, fpga_pwr_gpio_eo);
	}

	msdc_set_pwr_gpio_dir(fpga_pwr_gpio, fpga_pwr_gpio_eo);
}

bool hwPowerOn_fpga(void)
{
	volatile u16 l_val;

	l_val = MSDC_READ16(PWR_GPIO);
	l_val &= ~PWR_GPIO_L4_DIR;
#ifdef CONFIG_MTK_EMMC_SUPPORT
	MSDC_WRITE16(PWR_GPIO, (l_val | PWR_MASK_VOL_18 | PWR_MASK_CARD));
		/* | PWR_GPIO_L4_DIR)); */
#else
	MSDC_WRITE16(PWR_GPIO, (l_val | PWR_MASK_VOL_33 | PWR_MASK_CARD));
		/* | PWR_GPIO_L4_DIR)); */
#endif
	l_val = MSDC_READ16(PWR_GPIO);
	pr_debug("[%s]: pwr gpio = 0x%x\n", __func__, l_val);
	return true;
}
EXPORT_SYMBOL(hwPowerOn_fpga);

bool hwPowerSwitch_fpga(void)
{
	volatile u16 l_val;

	l_val = MSDC_READ16(PWR_GPIO);
	MSDC_WRITE16(PWR_GPIO, (l_val & ~(PWR_MASK_VOL_33)));
	l_val = MSDC_READ16(PWR_GPIO);
	MSDC_WRITE16(PWR_GPIO, (l_val | PWR_MASK_VOL_18));

	l_val = MSDC_READ16(PWR_GPIO);
	pr_debug("[%s]: pwr gpio = 0x%x\n", __func__, l_val);
	return true;
}
EXPORT_SYMBOL(hwPowerSwitch_fpga);

bool hwPowerDown_fpga(void)
{
	volatile u16 l_val;

	l_val = MSDC_READ16(PWR_GPIO);
#ifdef MTK_EMMC_SUPPORT
	MSDC_WRITE16(PWR_GPIO,
		(l_val & ~(PWR_MASK_VOL_18 | PWR_MASK_CARD)));
#else
	MSDC_WRITE16(PWR_GPIO,
		(l_val & ~(PWR_MASK_VOL_18 | PWR_MASK_VOL_33 | PWR_MASK_CARD)));
#endif
	l_val = MSDC_READ16(PWR_GPIO);
	pr_debug("[%s]: pwr gpio = 0x%x\n", __func__, l_val);
	return true;
}
EXPORT_SYMBOL(hwPowerDown_fpga);

void msdc_fpga_power(struct msdc_host *host, u32 on)
{
	if (on)
		hwPowerOn_fpga();
	else
		hwPowerDown_fpga();
}

void msdc_sd_power_switch(struct msdc_host *host, u32 on)
{
	if (on)
		hwPowerSwitch_fpga();
}

/* clock source for host: global */
u32 hclks_msdc[] = {12000000, 12000000, 12000000, 12000000, 12000000,
		    12000000, 12000000, 12000000, 0};

void msdc_select_clksrc(struct msdc_host *host, int clksrc)
{
	u32 *hclks;

	hclks = msdc_get_hclks(host->id);

	pr_err("[%s]: msdc%d change clk_src from %dKHz to %d:%dKHz\n", __func__,
		host->id, (host->hclk/1000), clksrc, (hclks[clksrc]/1000));

	host->hclk = hclks[clksrc];
	host->hw->clk_src = clksrc;
}

void msdc_clk_status(int *status)
{
}

void msdc_polling_axi_status(int line, int dead)
{
}
