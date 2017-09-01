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

#define LOG_TAG "ddp_drv"

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <generated/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
/* ION */
/* #include <linux/ion.h> */
/* #include <linux/ion_drv.h> */
/* #include <mach/m4u.h> */
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/io.h>
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include "ddp_clkmgr.h"
#endif
#include <mt-plat/sync_write.h>
#include "m4u.h"

#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_hal.h"
#include "disp_log.h"
#include "ddp_irq.h"
#include "ddp_info.h"
#include "ddp_dpi_reg.h"
#include "disp_helper.h"


#define DISP_DEVNAME "DISPSYS"
/* device and driver */
static dev_t disp_devno;
static struct cdev *disp_cdev;
static struct class *disp_class;

typedef struct {
	pid_t open_pid;
	pid_t open_tgid;
	struct list_head testList;
	spinlock_t node_lock;
} disp_node_struct;

static struct platform_device mydev;
static struct dispsys_device *dispsys_dev;
unsigned int dispsys_irq[DISP_REG_NUM] = { 0 };
volatile unsigned long dispsys_reg[DISP_REG_NUM] = { 0 };

volatile unsigned long mipi_tx_reg = 0;
volatile unsigned long dsi_reg_va = 0;
/* from DTS, for debug */
unsigned long ddp_reg_pa_base[DISP_REG_NUM] = {
	0x14000000,	/*CONFIG*/
	0x14008000,	/*OVL0 */
	0x14009000,	/*OVL1 */
	0x1400A000,	/*RDMA0 */
	0x1400B000,	/*RDMA1 */
	0x1400C000,	/*WDMA0 */
	0x1400D000,	/*COLOR*/
	0x1400E000,	/*CCORR*/
	0x1400F000,	/*AAL*/
	0x14010000,	/*GAMMA*/
	0x14011000,	/*DITHER*/
	0x14012000,	/*DSI0 */
	0x14013000,	/*DPI0 */
	0x1100E000,	/*PWM*/
	0x14014000,	/*MUTEX*/
	0x14015000,	/*SMI_LARB0 */
	0x14016000,	/*SMI_COMMON */
	0x14017000,	/*WDMA1 */
	0x14018000,	/*OVL0_2L */
	0x14019000,	/*OVL1_2L */
	0x10215000	/*MIPITX*/
};

#ifndef CONFIG_MTK_CLKMGR
/*
 * Note: The name order of the disp_clk_name[] must be synced with
 * the enum ddp_clk_id(ddp_clkmgr.h) in case get the wrong clock.
 */
const char *disp_clk_name[MAX_DISP_CLK_CNT] = {
	"DISP0_SMI_COMMON",
	"DISP0_SMI_LARB0",
	"DISP0_DISP_OVL0",
	"DISP0_DISP_OVL1",
	"DISP0_DISP_RDMA0",
	"DISP0_DISP_RDMA1",
	"DISP0_DISP_WDMA0",
	"DISP0_DISP_COLOR",
	"DISP0_DISP_CCORR",
	"DISP0_DISP_AAL",
	"DISP0_DISP_GAMMA",
	"DISP0_DISP_DITHER",
	"DISP0_DISP_UFOE_MOUT",
	"DISP0_DISP_WDMA1",
	"DISP0_DISP_2L_OVL0",
	"DISP0_DISP_2L_OVL1",
	"DISP0_DISP_OVL0_MOUT",
	"DISP1_DSI_ENGINE",
	"DISP1_DSI_DIGITAL",
	"DISP1_DPI_ENGINE",
	"DISP1_DPI_PIXEL",
	"DISP_PWM",
	"DISP_MTCMOS_CLK",
	"MUX_DPI0",
	"TVDPLL_D2",
	"TVDPLL_D4",
	"TVDPLL_D8",
	"TVDPLL_D16",
	"DPI_CK",
	"MUX_PWM",
	"UNIVPLL2_D4",
	"OSC_D2",
	"OSC_D8",
	"MUX_MM",
	"MM_VENCPLL",
	"SYSPLL2_D2"
};

#endif
/*
static unsigned int ddp_ms2jiffies(unsigned long ms)
{
	return (ms * HZ + 512) >> 10;
}
*/

struct dispsys_device {
	void __iomem *regs[DISP_REG_NUM];
	struct device *dev;
	int irq[DISP_REG_NUM];
#ifndef CONFIG_MTK_CLKMGR
	struct clk *disp_clk[MAX_DISP_CLK_CNT];
#endif
};


static int disp_is_intr_enable(DISP_REG_ENUM module)
{
	switch (module) {
	case DISP_REG_OVL0:
	case DISP_REG_OVL1:
	case DISP_REG_OVL0_2L:
	case DISP_REG_OVL1_2L:
	case DISP_REG_RDMA0:
	case DISP_REG_RDMA1:
	/*case DISP_REG_WDMA0:*/
	case DISP_REG_WDMA1:
	case DISP_REG_MUTEX:
	case DISP_REG_DSI0:
	case DISP_REG_DPI0:
	case DISP_REG_AAL:
	case DISP_REG_CCORR:
		return 1;

	case DISP_REG_COLOR:
	case DISP_REG_GAMMA:
	case DISP_REG_DITHER:
	case DISP_REG_PWM:
	case DISP_REG_CONFIG:
	case DISP_REG_SMI_LARB0:
	case DISP_REG_SMI_COMMON:
	case DISP_REG_MIPI:
		return 0;

	case DISP_REG_WDMA0:
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
		return 0;
#else
		return 1;
#endif
	default:
		return 0;
	}
}

m4u_callback_ret_t disp_m4u_callback(int port, unsigned int mva, void *data)
{
	DISP_MODULE_ENUM module = DISP_MODULE_OVL0;
	m4u_callback_ret_t ret;

	ret = M4U_CALLBACK_HANDLED;
	DISPERR("fault call port=%d, mva=0x%x, data=0x%p\n", port, mva, data);
	switch (port) {
	case M4U_PORT_DISP_OVL0:
		module = DISP_MODULE_OVL0;
		break;
	case M4U_PORT_DISP_RDMA0:
		module = DISP_MODULE_RDMA0;
		break;
	case M4U_PORT_DISP_WDMA0:
		module = DISP_MODULE_WDMA0;
		break;
	case M4U_PORT_DISP_OVL1:
		module = DISP_MODULE_OVL1;
		break;
	case M4U_PORT_DISP_RDMA1:
		module = DISP_MODULE_RDMA1;
		break;
	case M4U_PORT_DISP_WDMA1:
		module = DISP_MODULE_WDMA1;
		break;
	case M4U_PORT_DISP_2L_OVL0:
		module = DISP_MODULE_OVL0_2L;
		break;
	case M4U_PORT_DISP_2L_OVL1:
		module = DISP_MODULE_OVL1_2L;
		break;
	default:
	ret = M4U_CALLBACK_NOT_HANDLED;
		DISPERR("unknown port=%d\n", port);
	}
	ddp_dump_analysis(module);
	ddp_dump_reg(module);
	return ret;
}


struct device *disp_get_device(void)
{
	return dispsys_dev->dev;
}

static int disp_probe(struct platform_device *pdev)
{
	struct class_device;
	int i;
	static unsigned int disp_probe_cnt;

	if (disp_probe_cnt != 0)
		return 0;

	/* save pdev for disp_probe_1 */
	memcpy(&mydev, pdev, sizeof(mydev));

	if (dispsys_dev) {
		DISPERR("%s: dispsys_dev=0x%p\n", __func__, dispsys_dev);
		BUG();
	}

	dispsys_dev = kmalloc(sizeof(struct dispsys_device), GFP_KERNEL);
	if (!dispsys_dev) {
		DISPERR("Unable to allocate dispsys_dev\n");
		return -ENOMEM;
	}

#ifndef CONFIG_MTK_CLKMGR
	for (i = 0; i < MAX_DISP_CLK_CNT; i++) {
		DISPMSG("DISPSYS get clock %s\n", disp_clk_name[i]);
		dispsys_dev->disp_clk[i] = devm_clk_get(&pdev->dev, disp_clk_name[i]);
		if (IS_ERR(dispsys_dev->disp_clk[i]))
			DISPERR("%s:%d, DISPSYS get %d,%s clock error!!!\n",
				   __FILE__, __LINE__, i, disp_clk_name[i]);
		else {
				if (!ddp_set_clk_handle(dispsys_dev->disp_clk[i], i)) {
					switch (i) {
					case MUX_MM:
					case MM_VENCPLL:
					case SYSPLL2_D2:
						break; /* no need prepare_enable here */
					case DISP0_SMI_COMMON:
					case DISP0_SMI_LARB0:
					case DISP_MTCMOS_CLK:
						ddp_clk_prepare_enable(i);
						break;
					default:
						ddp_clk_prepare(i);
						break;
					}
				}
		}
	}
#endif /* CONFIG_MTK_CLKMGR */
	disp_probe_cnt++;

	return 0;
}
static int __init disp_probe_1(void)
{
	struct class_device;
	int ret;
	int i;
	struct platform_device *pdev = &mydev;


	disp_helper_option_init();

	if (!dispsys_dev) {
		DISPERR("%s: dispsys_dev=NULL\n", __func__);
		BUG();
	}

	dispsys_dev->dev = &pdev->dev;

	/* iomap registers and irq */
	for (i = 0; i < DISP_REG_NUM; i++) {
		struct resource res;

		dispsys_dev->regs[i] = of_iomap(pdev->dev.of_node, i);
		if (!dispsys_dev->regs[i]) {
			DISPERR("Unable to ioremap registers, of_iomap fail, i=%d\n", i);
			return -ENOMEM;
		}
		dispsys_reg[i] = (unsigned long)dispsys_dev->regs[i];
		/* check physical register */
		of_address_to_resource(pdev->dev.of_node, i, &res);
		if (ddp_reg_pa_base[i] != res.start)
			DISPERR("DT err, i=%d, module=%s, map_addr=%p, reg_pa=0x%lx!=0x%pa\n",
			       i, ddp_get_reg_module_name(i), dispsys_dev->regs[i],
			       ddp_reg_pa_base[i], &res.start);

		/* get IRQ ID and request IRQ */
		dispsys_dev->irq[i] = irq_of_parse_and_map(pdev->dev.of_node, i);
		dispsys_irq[i] = dispsys_dev->irq[i];

		DISPMSG("DT, i=%d, module=%s, map_addr=%p, map_irq=%d, reg_pa=0x%lx\n",
		       i, ddp_get_reg_module_name(i), dispsys_dev->regs[i], dispsys_dev->irq[i],
		       ddp_reg_pa_base[i]);
	}
	/* mipi tx reg map here */
	dsi_reg_va = dispsys_reg[DISP_REG_DSI0];
	mipi_tx_reg = dispsys_reg[DISP_REG_MIPI];
	DPI_REG = (struct DPI_REGS *)dispsys_reg[DISP_REG_DPI0];

	/* //// power on MMSYS for early porting */
#ifdef CONFIG_MTK_FPGA
	pr_debug("[DISP Probe] power MMSYS:0x%lx,0x%lx\n", DISP_REG_CONFIG_MMSYS_CG_CLR0,
	       DISP_REG_CONFIG_MMSYS_CG_CLR1);
	DISP_REG_SET(NULL, DISP_REG_CONFIG_MMSYS_CG_CLR0, 0xFFFFFFFF);
	DISP_REG_SET(NULL, DISP_REG_CONFIG_MMSYS_CG_CLR1, 0xFFFFFFFF);
	/* DISP_REG_SET(NULL,DISPSYS_CONFIG_BASE + 0xC04,0x1C000);//fpga should set this register */
#endif
	/* init arrays */
	ddp_path_init();

	for (i = 0; i < DISP_REG_NUM; i++) {
		if (disp_is_intr_enable(i) == 1) {
			/* IRQF_TRIGGER_NONE dose not take effect here, real trigger mode set in dts file */
			ret = request_irq(dispsys_dev->irq[i], (irq_handler_t) disp_irq_handler,
				IRQF_TRIGGER_NONE, ddp_get_reg_module_name(i), NULL);
			if (ret) {
				DISPERR
				    ("Unable to request IRQ, request_irq fail, i=%d, irq=%d\n",
				     i, dispsys_dev->irq[i]);
				return ret;
			}
			DISPMSG("irq enabled, module=%s, irq=%d\n",
			       ddp_get_reg_module_name(i), dispsys_dev->irq[i]);
		}
	}

	/* init M4U callback */
	DISPMSG("register m4u callback\n");
	m4u_register_fault_callback(M4U_PORT_DISP_OVL0, disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_RDMA0, disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_WDMA0, disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_OVL1, disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_RDMA1, disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_WDMA1, disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_2L_OVL0, disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_2L_OVL1, disp_m4u_callback, 0);


	DISPMSG("dispsys probe done.\n");
	/* NOT_REFERENCED(class_dev); */
	return 0;
}

static int disp_remove(struct platform_device *pdev)
{
	return 0;
}

static void disp_shutdown(struct platform_device *pdev)
{
	/* Nothing yet */
}


/* PM suspend */
static int disp_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

/* PM resume */
static int disp_resume(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id dispsys_of_ids[] = {
	{.compatible = "mediatek,DISPSYS",},
	{}
};

static struct platform_driver dispsys_of_driver = {
	.driver = {
		   .name = DISP_DEVNAME,
		   .owner = THIS_MODULE,
		   .of_match_table = dispsys_of_ids,
		   },
	.probe = disp_probe,
	.remove = disp_remove,
	.shutdown = disp_shutdown,
	.suspend = disp_suspend,
	.resume = disp_resume,
};

static int __init disp_init(void)
{
	int ret = 0;

	DISPMSG("Register the disp driver\n");
	init_log_buffer();
	if (platform_driver_register(&dispsys_of_driver)) {
		DISPERR("failed to register disp driver\n");
		/* platform_device_unregister(&disp_device); */
		ret = -ENODEV;
		return ret;
	}
	DISPMSG("disp driver init done\n");
	return 0;
}

static void __exit disp_exit(void)
{
#ifndef CONFIG_MTK_CLKMGR
	int i = 0;

	for (i = 0; i < MAX_DISP_CLK_CNT; i++)
		ddp_clk_unprepare(i);
#endif
	cdev_del(disp_cdev);
	unregister_chrdev_region(disp_devno, 1);

	platform_driver_unregister(&dispsys_of_driver);

	device_destroy(disp_class, disp_devno);
	class_destroy(disp_class);

}

static int __init disp_late(void)
{
	int ret = 0;

	DISPMSG("disp driver(1) disp_late begin\n");
	/* for rt5081 */
	ret = display_bias_regulator_init();
	if (ret < 0)
		pr_err("get dsv_pos fail, ret = %d\n", ret);

	display_bias_enable();

	DISPMSG("disp driver(1) disp_late end\n");
	return 0;
}

#ifndef MTK_FB_DO_NOTHING
arch_initcall(disp_init);
module_init(disp_probe_1);
module_exit(disp_exit);
late_initcall(disp_late);
#endif
MODULE_AUTHOR("Tzu-Meng, Chung <Tzu-Meng.Chung@mediatek.com>");
MODULE_DESCRIPTION("Display subsystem Driver");
MODULE_LICENSE("GPL");
