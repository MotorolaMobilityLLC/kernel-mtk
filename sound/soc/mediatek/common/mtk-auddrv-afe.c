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


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   mtk-auddrv-afe.c
 *
 * Project:
 * --------
 *   MT6797  Audio Driver Afe Register setting
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include "mtk-auddrv-common.h"
#include <linux/types.h>
#include <linux/device.h>
#include <linux/regmap.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/


/*****************************************************************************
 *                         FUNCTION IMPLEMENTATION
 *****************************************************************************/

/*****************************************************************************
 * FUNCTION
 *  Auddrv_Reg_map
 *
 * DESCRIPTION
 * Auddrv_Reg_map
 *
 *****************************************************************************
 */

/*
 *    global variable control
 */
static DEFINE_SPINLOCK(clksys_set_reg_lock);
static const unsigned int SramCaptureOffSet = (16 * 1024);

static const struct regmap_config mtk_afe_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = AFE_MAXLENGTH,
	.cache_type = REGCACHE_NONE,
	.fast_io = true,
};

/* address for ioremap audio hardware register */
void *AFE_BASE_ADDRESS;
void *AFE_SRAM_ADDRESS;
void *AFE_TOP_ADDRESS;
void *APMIXEDSYS_ADDRESS;
void *CLKSYS_ADDRESS;

struct regmap *pregmap;

int Auddrv_Reg_map(struct device *pdev)
{
	int ret = 0;
#ifdef CONFIG_OF
	struct device_node *audio_sram_node = NULL;

	audio_sram_node = of_find_compatible_node(NULL, NULL, "mediatek,audio_sram");

	pr_err("%s\n", __func__);

	if (!pdev->of_node)
		pr_err("%s invalid of_node\n", __func__);

	if (audio_sram_node == NULL)
		pr_err("%s invalid audio_sram_node\n", __func__);

	/* mapping AFE reg*/
	AFE_BASE_ADDRESS = of_iomap(pdev->of_node, 0);
	if (AFE_BASE_ADDRESS == NULL) {
		pr_warn("AFE_BASE_ADDRESS=0x%p\n", AFE_BASE_ADDRESS);
		return -ENODEV;
	}

	AFE_SRAM_ADDRESS = of_iomap(audio_sram_node, 0);
	if (AFE_SRAM_ADDRESS == NULL) {
		pr_warn("AFE_SRAM_ADDRESS=0x%p\n", AFE_SRAM_ADDRESS);
		return -ENODEV;
	}

	pregmap =  devm_regmap_init_mmio(pdev, AFE_BASE_ADDRESS,
		&mtk_afe_regmap_config);
	if (IS_ERR(pregmap)) {
		pr_warn("devm_regmap_init_mmio error\n");
		AUDIO_AEE("devm_regmap_init_mmio error");
		return -ENODEV;
	}
#else
	AFE_BASE_ADDRESS = ioremap_nocache(AUDIO_HW_PHYSICAL_BASE, 0x1000);
	AFE_SRAM_ADDRESS = ioremap_nocache(AFE_INTERNAL_SRAM_PHY_BASE, AFE_INTERNAL_SRAM_SIZE);
#endif

	/* temp for hardawre code  set 0x1000629c = 0xd */
	AFE_TOP_ADDRESS = ioremap_nocache(AUDIO_POWER_TOP, 0x1000);
	APMIXEDSYS_ADDRESS = ioremap_nocache(APMIXEDSYS_BASE, 0x1000);
	CLKSYS_ADDRESS = ioremap_nocache(AUDIO_CLKCFG_PHYSICAL_BASE, 0x1000);

	return ret;
}

unsigned int Get_Afe_Sram_Length(void)
{
	return AFE_INTERNAL_SRAM_SIZE;
}

dma_addr_t Get_Afe_Sram_Phys_Addr(void)
{
	return (dma_addr_t) AFE_INTERNAL_SRAM_PHY_BASE;
}

dma_addr_t Get_Afe_Sram_Capture_Phys_Addr(void)
{
	return (dma_addr_t) (AFE_INTERNAL_SRAM_PHY_BASE + SramCaptureOffSet);
}

void *Get_Afe_SramBase_Pointer()
{
	return AFE_SRAM_ADDRESS;
}

void *Get_Afe_SramCaptureBase_Pointer()
{
	char *CaptureSramPointer = (char *)(AFE_SRAM_ADDRESS) + SramCaptureOffSet;

	return (void *)CaptureSramPointer;
}

void *Get_Afe_Powertop_Pointer()
{
	return AFE_TOP_ADDRESS;
}

/* function to access apmixed sys */
uint32 GetApmixedCfg(uint32 offset)
{
	volatile long address = (long)((char *)APMIXEDSYS_ADDRESS + offset);
	volatile uint32 *value;

	value = (volatile uint32 *)(address);
	/* pr_debug("GetApmixedCfg offset=%x address = %x value = 0x%x\n", offset, address, *value); */
	return *value;
}

void SetApmixedCfg(uint32 offset, uint32 value, uint32 mask)
{
	volatile long address = (long)((char *)APMIXEDSYS_ADDRESS + offset);
	volatile uint32 *AFE_Register = (volatile uint32 *)address;
	volatile uint32 val_tmp;
	/* pr_debug("SetApmixedCfg offset=%x, value=%x, mask=%x\n",offset,value,mask); */
	val_tmp = GetApmixedCfg(offset);
	val_tmp &= (~mask);
	val_tmp |= (value & mask);
	mt_reg_sync_writel(val_tmp, AFE_Register);
}

/* function to access clksys */
unsigned int clksys_get_reg(unsigned int offset)
{
	volatile long address = (long)((char *)CLKSYS_ADDRESS + offset);
	volatile unsigned int *value;

	if (CLKSYS_ADDRESS == NULL) {
		pr_err("%s(), CLKSYS_ADDRESS is null\n", __func__);
		return 0;
	}

	value = (volatile unsigned int *)(address);
	pr_aud("%s offset=%x address = %lx value = 0x%x\n", __func__, offset, address, *value);
	return *value;
}

void clksys_set_reg(unsigned int offset, unsigned int value, unsigned int mask)
{
	volatile long address = (long)((char *)CLKSYS_ADDRESS + offset);
	volatile unsigned int *val_addr = (volatile unsigned int *)address;
	volatile unsigned int val_tmp;
	unsigned long flags = 0;

	if (CLKSYS_ADDRESS == NULL) {
		pr_err("%s(), CLKSYS_ADDRESS is null\n", __func__);
		return;
	}

	pr_aud("%s offset=%x, value=%x, mask=%x\n", __func__, offset, value, mask);
	spin_lock_irqsave(&clksys_set_reg_lock, flags);
	val_tmp = clksys_get_reg(offset);
	val_tmp &= (~mask);
	val_tmp |= (value & mask);
	mt_reg_sync_writel(val_tmp, val_addr);
	spin_unlock_irqrestore(&clksys_set_reg_lock, flags);
}

void Afe_Set_Reg(uint32 offset, uint32 value, uint32 mask)
{
	int ret = 0;

	ret = regmap_update_bits(pregmap, offset, mask, value);
	if (ret) {
		pr_err("%s ret = %d offset = 0x%x value = 0x%x mask = 0x%x\n",
			__func__, ret, offset, value, mask);
	}
}
EXPORT_SYMBOL(Afe_Set_Reg);

uint32 Afe_Get_Reg(uint32 offset)
{
	uint32 value;
	int ret;

	ret = regmap_read(pregmap, offset, &value);
	if (ret)
		pr_err("%s ret = %d value = 0x%x mask = 0x%x\n", __func__, ret, offset, value);
	return value;
}
EXPORT_SYMBOL(Afe_Get_Reg);

