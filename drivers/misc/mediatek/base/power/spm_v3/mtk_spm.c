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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <mtk_spm_idle.h>
#include <mt-plat/upmu_common.h>
#include "include/pmic_api_buck.h"
#include <mtk_spm_vcore_dvfs.h>
#include <mtk_spm_internal.h>
#include <mtk_dramc.h> /* for lpDram_Register_Read() */
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/irqchip/mtk-eic.h>
#include <linux/suspend.h>
#ifdef CONFIG_MTK_SPM_IN_ATF
#include <mt-plat/mtk_secure_api.h>
#endif /* CONFIG_MTK_SPM_IN_ATF */
#ifdef CONFIG_MTK_WD_KICKER
#include <mach/wd_api.h>
#endif

#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <mtk_spm_misc.h>
#include <mtk_spm_resource_req_internal.h>

int spm_for_gps_flag;
static struct dentry *spm_dir;
static struct dentry *spm_file;
#if !defined(CONFIG_MTK_SPM_IN_ATF)
static int dyna_load_pcm_done __nosavedata;
static char *dyna_load_pcm_path[] = {
	[DYNA_LOAD_PCM_SUSPEND] = "pcm_suspend.bin",
	[DYNA_LOAD_PCM_MAX] = "pcm_path_max",
};
#endif /* CONFIG_MTK_SPM_IN_ATF */

struct dyna_load_pcm_t dyna_load_pcm[DYNA_LOAD_PCM_MAX];

void __iomem *spm_base;
#if !defined(CONFIG_MTK_SPM_IN_ATF)
void __iomem *spm_mcucfg;
#endif /* CONFIG_MTK_SPM_IN_ATF */
u32 spm_irq_0;

#define NF_EDGE_TRIG_IRQS	7
static u32 edge_trig_irqs[NF_EDGE_TRIG_IRQS];

/**************************************
 * Config and Parameter
 **************************************/

/**************************************
 * Define and Declare
 **************************************/
struct spm_irq_desc {
	unsigned int irq;
	irq_handler_t handler;
};

static twam_handler_t spm_twam_handler;

void __attribute__((weak)) spm_sodi3_init(void)
{
	pr_err("NO %s !!!\n", __func__);
}

void __attribute__((weak)) spm_sodi_init(void)
{
	pr_err("NO %s !!!\n", __func__);
}

void __attribute__((weak)) spm_deepidle_init(void)
{
	pr_err("NO %s !!!\n", __func__);
}

void __attribute__((weak)) spm_vcorefs_init(void)
{
	pr_err("NO %s !!!\n", __func__);
}

void __attribute__((weak)) mt_power_gs_dump_suspend(void)
{
	pr_err("NO %s !!!\n", __func__);
}

int __attribute__((weak)) spm_fs_init(void)
{
	pr_err("NO %s !!!\n", __func__);
	return 0;
}

/**************************************
 * Init and IRQ Function
 **************************************/
static irqreturn_t spm_irq0_handler(int irq, void *dev_id)
{
	u32 isr;
	unsigned long flags;
	struct twam_sig twamsig;

	spin_lock_irqsave(&__spm_lock, flags);
	/* get ISR status */
	isr = spm_read(SPM_IRQ_STA);
	if (isr & ISRS_TWAM) {
		twamsig.sig0 = spm_read(SPM_TWAM_LAST_STA0);
		twamsig.sig1 = spm_read(SPM_TWAM_LAST_STA1);
		twamsig.sig2 = spm_read(SPM_TWAM_LAST_STA2);
		twamsig.sig3 = spm_read(SPM_TWAM_LAST_STA3);
		udelay(40); /* delay 1T @ 32K */
	}

	/* clean ISR status */
#if !defined(CONFIG_MTK_SPM_IN_ATF)
	spm_write(SPM_IRQ_MASK, spm_read(SPM_IRQ_MASK) | ISRM_ALL_EXC_TWAM);
	spm_write(SPM_IRQ_STA, isr);
	spm_write(SPM_SWINT_CLR, PCM_SW_INT0);
#else
	mt_secure_call(MTK_SIP_KERNEL_SPM_IRQ0_HANDLER, isr, 0, 0);
#endif /* CONFIG_MTK_SPM_IN_ATF */
	spin_unlock_irqrestore(&__spm_lock, flags);

	if ((isr & ISRS_TWAM) && spm_twam_handler)
		spm_twam_handler(&twamsig);

	if (isr & (ISRS_SW_INT0 | ISRS_PCM_RETURN))
		spm_err("IRQ0 HANDLER SHOULD NOT BE EXECUTED (0x%x)\n", isr);

	return IRQ_HANDLED;
}

static int spm_irq_register(void)
{
	int i, err, r = 0;
	struct spm_irq_desc irqdesc[] = {
		{.irq = 0, .handler = spm_irq0_handler,}
	};
	irqdesc[0].irq = SPM_IRQ0_ID;
	for (i = 0; i < ARRAY_SIZE(irqdesc); i++) {
		if (cpu_present(i)) {
			err = request_irq(irqdesc[i].irq, irqdesc[i].handler,
					IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND | IRQF_PERCPU, "SPM", NULL);
			if (err) {
				spm_err("FAILED TO REQUEST IRQ%d (%d)\n", i, err);
				r = -EPERM;
			}
		}
	}
	return r;
}

static void spm_register_init(void)
{
#if !defined(CONFIG_MTK_SPM_IN_ATF)
	unsigned long flags;
#endif /* CONFIG_MTK_SPM_IN_ATF */
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	if (!node)
		spm_err("find SLEEP node failed\n");
	spm_base = of_iomap(node, 0);
	if (!spm_base)
		spm_err("base spm_base failed\n");

	spm_irq_0 = irq_of_parse_and_map(node, 0);
	if (!spm_irq_0)
		spm_err("get spm_irq_0 failed\n");

#if !defined(CONFIG_MTK_SPM_IN_ATF)
	/* mcucfg */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mcucfg");
	if (!node)
		spm_err("[MCUCFG] find node failed\n");
	spm_mcucfg = of_iomap(node, 0);
	if (!spm_mcucfg)
		spm_err("[MCUCFG] base failed\n");
#endif /* CONFIG_MTK_SPM_IN_ATF */

	spm_err("spm_base = %p, spm_irq_0 = %d\n", spm_base, spm_irq_0);

	/* msdc3_wakeup_ps */
	/*
	 * node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-mmc");
	 * if (!node) {
	 *         spm_err("find msdc3node failed\n");
	 * } else {
	 *         edge_trig_irqs[0] = irq_of_parse_and_map(node, 0);
	 *         if (!edge_trig_irqs[0])
	 *                 spm_err("get msdc3 failed\n");
	 * }
	 */

	/* emi_dfp_tx_irq */
	/*
	 * node = of_find_compatible_node(NULL, NULL, "mediatek,emi");
	 * if (!node) {
	 *         spm_err("find emi node failed\n");
	 * } else {
	 *         edge_trig_irqs[1] = irq_of_parse_and_map(node, 0);
	 *         if (!edge_trig_irqs[1])
	 *                 spm_err("get emi failed\n");
	 * }
	 */

	/* kp_irq_b */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-keypad");
	if (!node) {
		spm_err("find mt6799-keypad node failed\n");
	} else {
		edge_trig_irqs[2] = irq_of_parse_and_map(node, 0);
		if (!edge_trig_irqs[2])
			spm_err("get mt6799-keypad failed\n");
	}

	/* apb_async_scpsys_fhctl_irq */
	/*
	 * node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-scpsys");
	 * if (!node) {
	 *         spm_err("find mt6799-scpsys node failed\n");
	 * } else {
	 *         edge_trig_irqs[3] = irq_of_parse_and_map(node, 0);
	 *         if (!edge_trig_irqs[3])
	 *                 spm_err("get mt6799-scpsys failed\n");
	 * }
	 */

	/* c2k_wdt_irq_b */
	node = of_find_compatible_node(NULL, NULL, "mediatek,ap2c2k_ccif");
	if (!node) {
		spm_err("find ap2c2k_ccif node failed\n");
	} else {
		edge_trig_irqs[4] = irq_of_parse_and_map(node, 1);
		if (!edge_trig_irqs[4])
			spm_err("get ap2c2k_ccif failed\n");
	}

	/* md_wdt_int_ao */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mdcldma");
	if (!node) {
		spm_err("find mdcldma node failed\n");
	} else {
		edge_trig_irqs[5] = irq_of_parse_and_map(node, 2);
		if (!edge_trig_irqs[5])
			spm_err("get mdcldma failed\n");
	}

	/* lowbattery_irq_b */
	node = of_find_compatible_node(NULL, NULL, "mediatek,auxadc");
	if (!node) {
		spm_err("find auxadc node failed\n");
	} else {
		edge_trig_irqs[6] = irq_of_parse_and_map(node, 0);
		if (!edge_trig_irqs[6])
			spm_err("get auxadc failed\n");
	}

	spm_err("edge trigger irqs: %d, %d, %d, %d, %d, %d, %d\n",
		 edge_trig_irqs[0],
		 edge_trig_irqs[1],
		 edge_trig_irqs[2],
		 edge_trig_irqs[3],
		 edge_trig_irqs[4],
		 edge_trig_irqs[5],
		 edge_trig_irqs[6]);

#if !defined(CONFIG_MTK_SPM_IN_ATF)
	spin_lock_irqsave(&__spm_lock, flags);

	/* enable register control */
	spm_write(POWERON_CONFIG_EN, SPM_REGWR_CFG_KEY | BCLK_CG_EN_LSB);

	/* init power control register */
	/* dram will set this register */
	/* spm_write(SPM_POWER_ON_VAL0, 0); */
	spm_write(SPM_POWER_ON_VAL1, POWER_ON_VAL1_DEF);
	spm_write(PCM_PWR_IO_EN, 0);

	/* reset PCM */
	spm_write(PCM_CON0, SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB | PCM_SW_RESET_LSB);
	spm_write(PCM_CON0, SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB);
	WARN_ON((spm_read(PCM_FSM_STA) & 0x7fffff) != PCM_FSM_STA_DEF);	/* PCM reset failed */

	/* init PCM control register */
	spm_write(PCM_CON0, SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB | RG_EN_IM_SLEEP_DVS_LSB);
	spm_write(PCM_CON1, SPM_REGWR_CFG_KEY | REG_EVENT_LOCK_EN_LSB |
		  REG_SPM_SRAM_ISOINT_B_LSB | RG_AHBMIF_APBEN_LSB);
	spm_write(PCM_IM_PTR, 0);
	spm_write(PCM_IM_LEN, 0);

	spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) | (0x3 << 2) |
			REG_SPM_LOCK_INFRA_DCM_LSB | (1 << 6) |
			REG_CLKSQ1_SEL_CTRL_LSB | REG_SRCCLKEN0_EN_LSB |
			(0x20 << 23));

	/* clean wakeup event raw status */
	spm_write(SPM_WAKEUP_EVENT_MASK, SPM_WAKEUP_EVENT_MASK_DEF);

	/* clean ISR status */
	spm_write(SPM_IRQ_MASK, ISRM_ALL);
	spm_write(SPM_IRQ_STA, ISRC_ALL);
	spm_write(SPM_SWINT_CLR, PCM_SW_INT_ALL);

	/* init r7 with POWER_ON_VAL1 */
	spm_write(PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL1));
	spm_write(PCM_PWR_IO_EN, PCM_RF_SYNC_R7);
	spm_write(PCM_PWR_IO_EN, 0);

	spm_write(DDR_EN_DBC_LEN, 0x82 | (0x82 << 10) | (0x82 << 20));
	spm_write(SRCCLKENI2PWRAP_MASK_B, 0x11);
	spm_write(SPM_PC_TRACE_CON, 0x8 | (1 << 31));

	spin_unlock_irqrestore(&__spm_lock, flags);
#endif /* CONFIG_MTK_SPM_IN_ATF */

	spm_set_dummy_read_addr(true);
}

#if !defined(CONFIG_MTK_SPM_IN_ATF)
static char *local_buf;
static dma_addr_t local_buf_dma;
static unsigned int local_buf_size;

/*Reserved memory by device tree!*/
int reserve_memory_spm_fn(struct reserved_mem *rmem)
{
	pr_info(" name: %s, base: 0x%llx, size: 0x%llx\n", rmem->name,
			   (unsigned long long)rmem->base, (unsigned long long)rmem->size);

	local_buf_dma = rmem->base;
	local_buf_size = (unsigned int)rmem->size;
	return 0;
}
RESERVEDMEM_OF_DECLARE(reserve_memory_test, "mediatek,spm-reserve-memory", reserve_memory_spm_fn);

#define SPM_FW_MAGIC    0x53504D32

#pragma pack(push)
#pragma pack(2)
struct spm_fw_header {
	unsigned int magic;
	unsigned int size;
	char name[58];
};
#pragma pack(pop)

char *spm_load_firmware(char *name)
{
	struct spm_fw_header *header;
	char *addr;

	addr = 0;
	header = (struct spm_fw_header *) local_buf;

	while (header->magic == SPM_FW_MAGIC) {
		char fw_name[58];

		memcpy_toio(fw_name, header->name, sizeof(fw_name));
		if (!strcmp(fw_name, name))
			break;

		header = (struct spm_fw_header *) (((char *) header) + sizeof(*header) + header->size);
	}

	if (header->magic == SPM_FW_MAGIC) {
		addr = (((char *) header) + sizeof(*header));
		return addr;
	}

	return NULL;
}

int spm_load_pcm_firmware(void)
{
	int err = 0;
	int i;
	int offset = 0;
	int spm_fw_count = 0;

	if (dyna_load_pcm_done)
		return err;

	if (local_buf == NULL) {
		local_buf = (char *)ioremap_nocache(local_buf_dma, local_buf_size);
		if (!local_buf) {
			pr_err("Failed to dma_alloc_coherent(), %d.\n", err);
			return -ENOMEM;
		}
	}

	for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++) {
		u16 firmware_size = 0;
		int copy_size = 0;
		struct pcm_desc *pdesc = &(dyna_load_pcm[i].desc);
		char *ptr;

		ptr = spm_load_firmware(dyna_load_pcm_path[i]);
		if (ptr == NULL) {
			pr_err("Failed to load %s, err = %d.\n", dyna_load_pcm_path[i], err);
			err = -1;
			continue;
		}

		/* Do whatever it takes to load firmware into device. */
		/* start of binary size */
		offset = 0;
		copy_size = 2;
		memcpy_toio(&firmware_size, ptr + offset, copy_size);

		/* start of binary */
		offset += copy_size;
		copy_size = firmware_size * 4;
		dyna_load_pcm[i].buf = ptr + offset;
		dyna_load_pcm[i].buf_dma = local_buf_dma + (u32)(ptr + offset - local_buf);

		/* start of pcm_desc without pointer */
		offset += copy_size;
		copy_size = sizeof(struct pcm_desc) - offsetof(struct pcm_desc, size);
		memcpy_toio((void *)&(dyna_load_pcm[i].desc.size), ptr + offset, copy_size);

		/* start of pcm_desc version */
		offset += copy_size;
		snprintf(dyna_load_pcm[i].version, PCM_FIRMWARE_VERSION_SIZE - 1,
				"%s", ptr + offset);
		pdesc->version = dyna_load_pcm[i].version;
		pdesc->base = (u32 *) dyna_load_pcm[i].buf;
		pdesc->base_dma = dyna_load_pcm[i].buf_dma;

		pr_info("#@# %s(%d) use spmfw partition for %s - %s\n", __func__, __LINE__,
				dyna_load_pcm_path[i], pdesc->version);

		dyna_load_pcm[i].ready = 1;
		spm_fw_count++;
	}

	if (spm_fw_count == DYNA_LOAD_PCM_MAX) {
		spm_vcorefs_init();
		dyna_load_pcm_done = 1;
	}

	return err;
}

int spm_load_firmware_status(void)
{
	return dyna_load_pcm_done;
}

static int spm_dbg_show_firmware(struct seq_file *s, void *unused)
{
	int i;
	struct pcm_desc *pdesc = NULL;

	for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++) {
		pdesc = &(dyna_load_pcm[i].desc);
		seq_printf(s, "#@# %s\n", dyna_load_pcm_path[i]);

		if (pdesc->version) {
			seq_printf(s, "#@#  version = %s\n", pdesc->version);
			seq_printf(s, "#@#  base = 0x%p\n", pdesc->base);
			seq_printf(s, "#@#  size = %u\n", pdesc->size);
			seq_printf(s, "#@#  sess = %u\n", pdesc->sess);
			seq_printf(s, "#@#  replace = %u\n", pdesc->replace);
			seq_printf(s, "#@#  addr_2nd = %u\n", pdesc->addr_2nd);
			seq_printf(s, "#@#  vec0 = 0x%x\n", pdesc->vec0);
			seq_printf(s, "#@#  vec1 = 0x%x\n", pdesc->vec1);
			seq_printf(s, "#@#  vec2 = 0x%x\n", pdesc->vec2);
			seq_printf(s, "#@#  vec3 = 0x%x\n", pdesc->vec3);
			seq_printf(s, "#@#  vec4 = 0x%x\n", pdesc->vec4);
			seq_printf(s, "#@#  vec5 = 0x%x\n", pdesc->vec5);
			seq_printf(s, "#@#  vec6 = 0x%x\n", pdesc->vec6);
			seq_printf(s, "#@#  vec7 = 0x%x\n", pdesc->vec7);
			seq_printf(s, "#@#  vec8 = 0x%x\n", pdesc->vec8);
			seq_printf(s, "#@#  vec9 = 0x%x\n", pdesc->vec9);
			seq_printf(s, "#@#  vec10 = 0x%x\n", pdesc->vec10);
			seq_printf(s, "#@#  vec11 = 0x%x\n", pdesc->vec11);
			seq_printf(s, "#@#  vec12 = 0x%x\n", pdesc->vec12);
			seq_printf(s, "#@#  vec13 = 0x%x\n", pdesc->vec13);
			seq_printf(s, "#@#  vec14 = 0x%x\n", pdesc->vec14);
			seq_printf(s, "#@#  vec15 = 0x%x\n", pdesc->vec15);
		}
	}
	seq_puts(s, "\n\n");

	return 0;
}

static int spm_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, spm_dbg_show_firmware, &inode->i_private);
}

static const struct file_operations spm_debug_fops = {
	.open = spm_dbg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#else
static int local_spm_load_firmware_status = -1;
int spm_load_firmware_status(void)
{
	if (local_spm_load_firmware_status != -1)
		local_spm_load_firmware_status =
			mt_secure_call(MTK_SIP_KERNEL_SPM_FIRMWARE_STATUS, 0, 0, 0);
	return local_spm_load_firmware_status;
}
#endif /* CONFIG_MTK_SPM_IN_ATF */

static int spm_sleep_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, get_spm_sleep_count, &inode->i_private);
}

static const struct file_operations spm_sleep_count_fops = {
	.open = spm_sleep_count_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_PM
static int spm_pm_event(struct notifier_block *notifier, unsigned long pm_event,
			void *unused)
{
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	struct spm_data spm_d;
	int ret;
	unsigned long flags;
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		return NOTIFY_DONE;
	case PM_RESTORE_PREPARE:
		return NOTIFY_DONE;
	case PM_POST_HIBERNATION:
		return NOTIFY_DONE;
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	case PM_SUSPEND_PREPARE:
		spm_d.u.notify.root_id = hps_get_root_id();
		spin_lock_irqsave(&__spm_lock, flags);
		ret = spm_to_sspm_command(SPM_SUSPEND_PREPARE, &spm_d);
		spin_unlock_irqrestore(&__spm_lock, flags);
		if (ret < 0) {
			pr_err("#@# %s(%d) PM_SUSPEND_PREPARE return %d!!!\n", __func__, __LINE__, ret);
			return NOTIFY_BAD;
		}
		return NOTIFY_DONE;
	case PM_POST_SUSPEND:
		spin_lock_irqsave(&__spm_lock, flags);
		ret = spm_to_sspm_command(SPM_POST_SUSPEND, &spm_d);
		spin_unlock_irqrestore(&__spm_lock, flags);
		if (ret < 0) {
			pr_err("#@# %s(%d) PM_POST_SUSPEND return %d!!!\n", __func__, __LINE__, ret);
			return NOTIFY_BAD;
		}
		return NOTIFY_DONE;
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
	}
	return NOTIFY_OK;
}

static struct notifier_block spm_pm_notifier_func = {
	.notifier_call = spm_pm_event,
	.priority = 0,
};
#endif /* CONFIG_PM */
#endif /* CONFIG_FPGA_EARLY_PORTING */

static ssize_t show_debug_log(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *p = buf;

	p += sprintf(p, "for test\n");

	return p - buf;
}

static ssize_t store_debug_log(struct device *dev, struct device_attribute *attr, const char *buf,
				 size_t size)
{
	return size;
}

static DEVICE_ATTR(debug_log, 0664, show_debug_log, store_debug_log);	/*664*/

static int spm_probe(struct platform_device *pdev)
{
	int ret;

	ret = device_create_file(&(pdev->dev), &dev_attr_debug_log);

	return 0;
}

static int spm_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id spm_of_ids[] = {
	{.compatible = "mediatek,SLEEP",},
	{}
};

static struct platform_driver spm_dev_drv = {
	.probe = spm_probe,
	.remove = spm_remove,
	.driver = {
		   .name = "spm",
		   .owner = THIS_MODULE,
		   .of_match_table = spm_of_ids,
		   },
};

static struct platform_device *pspmdev;

int __init spm_module_init(void)
{
	int r = 0;
	int ret = -1;
#if !defined(CONFIG_MTK_SPM_IN_ATF)
	int i;
#endif /* CONFIG_MTK_SPM_IN_ATF */

	spm_register_init();
	if (spm_irq_register() != 0)
		r = -EPERM;
#if defined(CONFIG_PM)
	if (spm_fs_init() != 0)
		r = -EPERM;
#endif
	spm_sodi3_init();
	spm_sodi_init();
	spm_deepidle_init();

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_MTK_DRAMC
	if (spm_golden_setting_cmp(1) != 0)
		aee_kernel_warning("SPM Warring", "dram golden setting mismach");
#endif /* CONFIG_MTK_DRAMC */
#endif /* CONFIG_FPGA_EARLY_PORTING */

	ret = platform_driver_register(&spm_dev_drv);
	if (ret) {
		pr_debug("fail to register platform driver\n");
		return ret;
	}

	pspmdev = platform_device_register_simple("spm", -1, NULL, 0);
	if (IS_ERR(pspmdev)) {
		pr_debug("Failed to register platform device.\n");
		return -EINVAL;
	}

	spm_dir = debugfs_create_dir("spm", NULL);
	if (spm_dir == NULL) {
		pr_debug("Failed to create spm dir in debugfs.\n");
		return -EINVAL;
	}

#if !defined(CONFIG_MTK_SPM_IN_ATF)
	spm_file = debugfs_create_file("firmware", S_IRUGO, spm_dir, NULL, &spm_debug_fops);

	for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++)
		dyna_load_pcm[i].ready = 0;
#endif /* CONFIG_MTK_SPM_IN_ATF */

	spm_file = debugfs_create_file("spm_sleep_count", S_IRUGO, spm_dir, NULL, &spm_sleep_count_fops);
	spm_resource_req_debugfs_init(spm_dir);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_PM
	ret = register_pm_notifier(&spm_pm_notifier_func);
	if (ret) {
		pr_debug("Failed to register PM notifier.\n");
		return ret;
	}
#endif /* CONFIG_PM */
#endif /* CONFIG_FPGA_EARLY_PORTING */

#if !defined(CONFIG_MTK_SPM_IN_ATF)
	spm_load_pcm_firmware();
#else
	spm_vcorefs_init();
#endif /* CONFIG_MTK_SPM_IN_ATF */

	return 0;
}

/**************************************
 * TWAM Control API
 **************************************/
static unsigned int idle_sel;
void spm_twam_set_idle_select(unsigned int sel)
{
	idle_sel = sel & 0x3;
}
EXPORT_SYMBOL(spm_twam_set_idle_select);

static unsigned int window_len;
void spm_twam_set_window_length(unsigned int len)
{
	window_len = len;
}
EXPORT_SYMBOL(spm_twam_set_window_length);

static struct twam_sig mon_type;
void spm_twam_set_mon_type(struct twam_sig *mon)
{
	if (mon) {
		mon_type.sig0 = mon->sig0 & 0x3;
		mon_type.sig1 = mon->sig1 & 0x3;
		mon_type.sig2 = mon->sig2 & 0x3;
		mon_type.sig3 = mon->sig3 & 0x3;
	}
}
EXPORT_SYMBOL(spm_twam_set_mon_type);

void spm_twam_register_handler(twam_handler_t handler)
{
	spm_twam_handler = handler;
}
EXPORT_SYMBOL(spm_twam_register_handler);

void spm_twam_enable_monitor(const struct twam_sig *twamsig, bool speed_mode)
{
	u32 sig0 = 0, sig1 = 0, sig2 = 0, sig3 = 0;
	u32 mon0 = 0, mon1 = 0, mon2 = 0, mon3 = 0;
	unsigned int sel;
	unsigned int length;
	unsigned long flags;

	if (twamsig) {
		sig0 = twamsig->sig0 & 0x1f;
		sig1 = twamsig->sig1 & 0x1f;
		sig2 = twamsig->sig2 & 0x1f;
		sig3 = twamsig->sig3 & 0x1f;
	}

	/* Idle selection */
	sel = idle_sel;
	/* Window length */
	length = window_len;
	/* Monitor type */
	mon0 = mon_type.sig0 & 0x3;
	mon1 = mon_type.sig1 & 0x3;
	mon2 = mon_type.sig2 & 0x3;
	mon3 = mon_type.sig3 & 0x3;

	spin_lock_irqsave(&__spm_lock, flags);
	spm_write(SPM_IRQ_MASK, spm_read(SPM_IRQ_MASK) & ~ISRM_TWAM);
	/* Signal Select */
	spm_write(SPM_TWAM_IDLE_SEL, sel);
	/* Monitor Control */
	spm_write(SPM_TWAM_CON,
		  (sig3 << 27) |
		  (sig2 << 22) |
		  (sig1 << 17) |
		  (sig0 << 12) |
		  (mon3 << 10) |
		  (mon2 << 8) |
		  (mon1 << 6) |
		  (mon0 << 4) | (speed_mode ? REG_SPEED_MODE_EN_LSB : 0) | REG_TWAM_ENABLE_LSB);
	/* Window Length */
	/* 0x13DDF0 for 50ms, 0x65B8 for 1ms, 0x1458 for 200us, 0xA2C for 100us */
	/* in speed mode (26 MHz) */
	spm_write(SPM_TWAM_WINDOW_LEN, length);
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_debug("enable TWAM for signal %u, %u, %u, %u (%u)\n",
		  sig0, sig1, sig2, sig3, speed_mode);
}
EXPORT_SYMBOL(spm_twam_enable_monitor);

void spm_twam_disable_monitor(void)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	spm_write(SPM_TWAM_CON, spm_read(SPM_TWAM_CON) & ~REG_TWAM_ENABLE_LSB);
	spm_write(SPM_IRQ_MASK, spm_read(SPM_IRQ_MASK) | ISRM_TWAM);
	spm_write(SPM_IRQ_STA, ISRC_TWAM);
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_debug("disable TWAM\n");
}
EXPORT_SYMBOL(spm_twam_disable_monitor);

/**************************************
 * SPM Golden Seting API(MEMPLL Control, DRAMC)
 **************************************/
#ifdef CONFIG_MTK_DRAMC
struct ddrphy_golden_cfg {
	u32 base;
	u32 offset;
	u32 mask;
	u32 value;
};

static struct ddrphy_golden_cfg ddrphy_setting[] = {
	{DRAMC_AO_CHA, 0x038, 0xc0000027, 0xc0000007},
	{PHY_CHA, 0x284, 0x000bff00, 0x00000100},
	{PHY_CHA, 0x28c, 0xffffffff, 0x83e003be},
	{PHY_CHA, 0x088, 0xffffffff, 0x00000000},
	{PHY_CHA, 0x08c, 0xffffffff, 0x0002e800},
	{PHY_CHA, 0x108, 0xffffffff, 0x00000000},
	{PHY_CHA, 0x10c, 0xffffffff, 0x0002e800},
	{PHY_CHA, 0x188, 0xffffffff, 0x00000800},
	{PHY_CHA, 0x18c, 0xffffffff, 0x000ba000},
	{PHY_CHA, 0x274, 0xffffffff, 0xfffffe7f},
	{PHY_CHA, 0x27c, 0xffffffff, 0xffffffff},
};

int spm_golden_setting_cmp(bool en)
{
	int i, ddrphy_num, r = 0;

	if (!en)
		return r;

	/*Compare Dramc Goldeing Setting */
	ddrphy_num = ARRAY_SIZE(ddrphy_setting);
	for (i = 0; i < ddrphy_num; i++) {
		u32 value;

		value = lpDram_Register_Read(ddrphy_setting[i].base, ddrphy_setting[i].offset);
		/*
		 * spm_err("addr: 0x%.2x, offset: 0x%.3x, mask: 0x%.8x, val: 0x%x, read: 0x%x\n",
		 *                 ddrphy_setting[i].base, ddrphy_setting[i].offset,
		 *                 ddrphy_setting[i].mask, ddrphy_setting[i].value, value);
		 */
		if ((value & ddrphy_setting[i].mask) != ddrphy_setting[i].value) {
			spm_err("dramc setting mismatch addr: 0x%.2x, offset: 0x%.3x, mask: 0x%.8x, val: 0x%x, read: 0x%x\n",
				ddrphy_setting[i].base, ddrphy_setting[i].offset,
				ddrphy_setting[i].mask, ddrphy_setting[i].value, value);
			r = -EPERM;
		}
	}

	return r;

}
#endif /* CONFIG_MTK_DRAMC */

#if !(defined(CONFIG_MTK_SPM_IN_ATF) && defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT))
void spm_pmic_power_mode(int mode, int force, int lock)
{
	static int prev_mode = -1;

	if (mode < PMIC_PWR_NORMAL || mode >= PMIC_PWR_NUM) {
		pr_debug("wrong spm pmic power mode");
		return;
	}

	if (force == 0 && mode == prev_mode)
		return;

	switch (mode) {
	case PMIC_PWR_NORMAL:
		/* nothing */
		break;
	case PMIC_PWR_DEEPIDLE:
		/* nothing */
		break;
	case PMIC_PWR_SODI3:
		pmic_ldo_vsram_vcore_lp(SRCLKEN0, 1, HW_LP);
		pmic_ldo_vsram_dvfs1_lp(SRCLKEN0, 1, HW_LP);
		pmic_ldo_va10_lp(SRCLKEN0, 1, HW_LP);
		pmic_ldo_vbif28_lp(SRCLKEN0, 1, HW_LP);
		break;
	case PMIC_PWR_SODI:
		/* nothing */
		break;
	case PMIC_PWR_SUSPEND:
		pmic_ldo_vsram_vcore_lp(SRCLKEN0, 1, HW_LP);
		pmic_ldo_vsram_dvfs1_lp(SRCLKEN0, 0, HW_LP);
		pmic_ldo_vsram_dvfs1_lp(SPM, 1, SPM_OFF);
		/* pmic_ldo_va10_lp(SRCLKEN0, 1, HW_OFF); */
		pmic_ldo_va10_lp(SRCLKEN0, 1, HW_LP);
		pmic_ldo_vbif28_lp(SRCLKEN0, 1, HW_OFF);
		break;
	default:
		pr_debug("spm pmic power mode (%d) is not configured\n", mode);
	}

	prev_mode = mode;
}
#endif /* !(defined(CONFIG_MTK_SPM_IN_ATF) && defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)) */

void *mt_spm_base_get(void)
{
	return spm_base;
}
EXPORT_SYMBOL(mt_spm_base_get);

void mt_spm_for_gps_only(int enable)
{
	spm_for_gps_flag = !!enable;
	/* pr_debug("#@# %s(%d) spm_for_gps_flag %d\n", __func__, __LINE__, spm_for_gps_flag); */
}
EXPORT_SYMBOL(mt_spm_for_gps_only);

void mt_spm_dcs_s1_setting(int enable, int flags)
{
	flags &= 0xf;
#ifdef CONFIG_MTK_SPM_IN_ATF
	mt_secure_call(MTK_SIP_KERNEL_SPM_DCS_S1, enable, flags, 0);
#else
#if 0
	spm_write(DRAMC_DPY_CLK_SW_CON5, spm_read(DRAMC_DPY_CLK_SW_CON5) | flags);
	while ((spm_read(DRAMC_DPY_CLK_SW_CON5) & (0xf << 4)) != (flags << 4))
		;

	if (!!enable)
		spm_write(SPM_SPARE_CON_SET, flags << 24);
	else
		spm_write(SPM_SPARE_CON_CLR, flags << 24);

	spm_write(DRAMC_DPY_CLK_SW_CON5, spm_read(DRAMC_DPY_CLK_SW_CON5) & ~flags);
	while ((spm_read(DRAMC_DPY_CLK_SW_CON5) & (0xf << 4)) != 0)
		;
#endif
#endif /* CONFIG_MTK_SPM_IN_ATF */
}
EXPORT_SYMBOL(mt_spm_dcs_s1_setting);

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT

#include <sspm_ipi.h>

int spm_to_sspm_command(u32 cmd, struct spm_data *spm_d)
{
#define SPM_D_LEN		(8) /* # of cmd + arg0 + arg1 + ... */
#define SPM_VCOREFS_D_LEN	(4) /* # of cmd + arg0 + arg1 + ... */
	int ack_data;
	unsigned int ret = 0;
	/* struct spm_data _spm_d; */

	/* pr_debug("#@# %s(%d) cmd %x\n", __func__, __LINE__, cmd); */
	switch (cmd) {
	case SPM_SUSPEND:
	case SPM_RESUME:
	case SPM_DPIDLE_ENTER:
	case SPM_DPIDLE_LEAVE:
	case SPM_ENTER_SODI:
	case SPM_ENTER_SODI3:
	case SPM_LEAVE_SODI:
	case SPM_LEAVE_SODI3:
		spm_d->cmd = cmd;
		ret = sspm_ipi_send_sync(IPI_ID_SPM_SUSPEND, IPI_OPT_DEFAUT, spm_d, SPM_D_LEN, &ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync(cmd:0x%x) ret %d\n", __func__, __LINE__, cmd, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;
	case SPM_VCORE_PWARP_CMD:
		#if 0
		spm_d->cmd = cmd;
		ret = sspm_ipi_send_sync(IPI_ID_VCORE_DVFS, IPI_OPT_LOCK_POLLING, spm_d, SPM_VCOREFS_D_LEN, &ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync(cmd:0x%x) ret %d\n", __func__, __LINE__, cmd, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		#endif
		break;
	case SPM_SUSPEND_PREPARE:
	case SPM_POST_SUSPEND:
		spm_d->cmd = cmd;
		ret = sspm_ipi_send_sync(IPI_ID_SPM_SUSPEND, IPI_OPT_DEFAUT, spm_d, SPM_D_LEN, &ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync(cmd:0x%x) ret %d\n", __func__, __LINE__, cmd, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;
	case SPM_DPIDLE_PREPARE:
	case SPM_POST_DPIDLE:
		spm_d->cmd = cmd;
		ret = sspm_ipi_send_sync(IPI_ID_SPM_SUSPEND, IPI_OPT_LOCK_POLLING, spm_d, SPM_D_LEN, &ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync(cmd:0x%x) ret %d\n", __func__, __LINE__, cmd, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;
	case SPM_SODI_PREPARE:
	case SPM_POST_SODI:
		spm_d->cmd = cmd;
		ret = sspm_ipi_send_sync(IPI_ID_SPM_SUSPEND, IPI_OPT_LOCK_POLLING, spm_d, SPM_D_LEN, &ack_data);
		if (ret != 0) {
			pr_err("#@# %s(%d) sspm_ipi_send_sync(cmd:0x%x) ret %d\n", __func__, __LINE__, cmd, ret);
		} else if (ack_data < 0) {
			ret = ack_data;
			pr_err("#@# %s(%d) cmd(%d) return %d\n", __func__, __LINE__, cmd, ret);
		}
		break;
	default:
		pr_err("#@# %s(%d) cmd(%d) wrong!!!\n", __func__, __LINE__, cmd);
		break;
	}

	return ret;
}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

void unmask_edge_trig_irqs_for_cirq(void)
{
	int i;

	for (i = 0; i < NF_EDGE_TRIG_IRQS; i++) {
		if (edge_trig_irqs[i]) {
			/* unmask edge trigger irqs */
			mt_irq_unmask_for_sleep_ex(edge_trig_irqs[i]);
		}
	}
}

MODULE_DESCRIPTION("SPM Driver v0.1");
