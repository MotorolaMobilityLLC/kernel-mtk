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
#include <linux/kernel.h>
#include <mt-plat/sync_write.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <mt-plat/mtk_io.h>
#include "mtk_emi_elm.h"
#include "mtk_emi_bm.h"
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#ifdef CONFIG_MTK_AEE_FEATURE
#include <mt-plat/aee.h>
#endif


static void __iomem *EMI_BASE_ADDR; /* not initialise statics to 0 or NULL */

void mt_emi_bwm_md_ltct_init(void)
{
/* 0x6B0 EMI_LTCT0:  LT_CMD_THR [27:8] =100
*   0x6B4 EMI_LTCT1:  LT_CYC_MODE0 [14:0]=80000  //3200: 200,000/2.5
*              LT_CYC_MODE1 [30:16]=66666  //2667:200,000 /3
*   0x6B8 EMI_LTCT2:  LT_CYC_MODE2 [14:0]=80000     //1600:200,000 /2.5
*              LT_CYC_MODE3 [30:16]=40000  //800:200,000 /5
*   0x6BC EMI_LTCT3:  LT_THR_MODE0[7:0]=160            //3200: 400/2.5
*              LT_THR_MODE0[15:8]=133        //2667: 400/3
*             LT_THR_MODE0[23:16]=160      //1600: 400/2.5
*              LT_THR_MODE0[31:24]=400     //800: 2000/5
*/
	unsigned int val;
	/*whitney E1 => (3200,2667,2667,800)
	*  lt_ctc[] =200,000/(2.5ns,3ns,3ns,5ns)=(80000,66666,66666,40000)
	*  lt_thr[]=(400,400,400,2000)/(2.5ns,3ns,3ns,5ns) =(160, 133, 133, 400)
	*/
	unsigned int lt_cyc[] = {80000, 66666, 66666, 40000};
	unsigned int lt_thr[] = {160, 133, 133, 400};

	val = readl(IOMEM(EMI_LTCT0));
	val = val & 0x0fffff00;
	val = val | (LT_CMD_THR_VAL << 8);
	mt_reg_sync_writel(val, EMI_LTCT0);

	val = readl(IOMEM(EMI_LTCT1));
	val = val & 80008000;
	val = val | lt_cyc[0] | (lt_cyc[1]<<16);
	mt_reg_sync_writel(val, EMI_LTCT1);

	val = readl(IOMEM(EMI_LTCT2));
	val = val & 80008000;
	val = val | lt_cyc[2] | (lt_cyc[3]<<16);
	mt_reg_sync_writel(val, EMI_LTCT2);

	val = lt_thr[0] | (lt_thr[1]<<8) | (lt_thr[2]<<16) | (lt_thr[3]<<24);
	mt_reg_sync_writel(val, EMI_LTCT3);

}
static int emi_ltct_bw_reg_dump(void)
{
	unsigned int emi_ltct0, emi_ltct1, emi_ltct2, emi_ltct3, emi_ltst;

	emi_ltct0 = readl(IOMEM(EMI_LTCT0));
	emi_ltct1 = readl(IOMEM(EMI_LTCT1));
	emi_ltct2 = readl(IOMEM(EMI_LTCT2));
	emi_ltct3 = readl(IOMEM(EMI_LTCT3));
	emi_ltst = readl(IOMEM(EMI_LTST));

	pr_err("emi_ltct_bw_reg_dump (EMI_LTCT0,EMI_LTCT1) 0x%x, 0x%x\n", emi_ltct0, emi_ltct1);
	pr_err("(EMI_LTCT2,EMI_LTCT3,EMI_LTST)0x%x, 0x%x, 0x%x\n", emi_ltct2, emi_ltct3, emi_ltst);

#ifdef CONFIG_MTK_AEE_FEATURE
	aee_kernel_exception("EMI MD BW violation",
	"emi_ltct_bw_reg_dump (EMI_LTCT0,EMI_LTCT1,EMI_LTCT2,EMI_LTCT3,EMI_LTST) 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n",
	emi_ltct0, emi_ltct1, emi_ltct2, emi_ltct3, emi_ltst);
#endif
	return BM_REQ_OK;
}
/*EMI bw_irq handler*/
static irqreturn_t emi_bw_irq(int irq, void *dev_id)
{
	pr_info("It's a MPU violation.\n");
	emi_ltct_bw_reg_dump();
	return IRQ_HANDLED;
}

static struct platform_driver emi_bw_ctrl = {
	.driver = {
		.name = "emi_bw_ctrl",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
	.id_table = NULL,
};

void emi_md_elm_Init(void)
{
	struct device_node *node;
	int ret;
	unsigned int emi_bw_irqnr = 0;

	/* DTS version */
	node = of_find_compatible_node(NULL, NULL, "mediatek,emi");
	if (node) {
		EMI_BASE_ADDR = of_iomap(node, 0);
		emi_bw_irqnr = irq_of_parse_and_map(node, 1);
		pr_err("get emi_bw_irq number= %d\n", emi_bw_irqnr);
		pr_err("get EMI_BASE_ADDR @ %p\n", EMI_BASE_ADDR);
	} else {
		pr_err("can't find compatible node\n");
		return;
	}
	pr_err("ella get emi_bw_irq number = %d\n", emi_bw_irqnr);


	if (emi_bw_irqnr != 0) {
		ret = request_irq(emi_bw_irqnr,
		(irq_handler_t)emi_bw_irq, IRQF_TRIGGER_NONE,
		"mt_emi_bw_md", &emi_bw_ctrl);
		if (ret != 0) {
			pr_err("Fail to request emi_bw_irqnr interrupt. Error = %d.\n", ret);
			return;
		}
	}

	mt_emi_bwm_md_ltct_init();

}
