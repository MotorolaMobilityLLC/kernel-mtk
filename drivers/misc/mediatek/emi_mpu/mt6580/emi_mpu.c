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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/list.h>
#ifdef CONFIG_MTK_AEE_FEATURE
#include <mt-plat/aee.h>
#endif
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <mt-plat/mt_device_apc.h>
#include <mt-plat/sync_write.h>
#include "mach/irqs.h"
#include <mt-plat/dma.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <mt-plat/mt_io.h>
#include <mach/mtk_ccci_helper.h>
#include <mach/emi_mpu.h>
#include <primary_display.h>

#define ENABLE_EMI_CHKER
#define ENABLE_EMI_WATCH_POINT

#define NR_REGION_ABORT 8
#define MAX_EMI_MPU_STORE_CMD_LEN 128
#define TIMEOUT 100
#define AXI_VIO_MONITOR_TIME    (1 * HZ)

static struct work_struct emi_mpu_work;
static struct workqueue_struct *emi_mpu_workqueue;

static unsigned int vio_addr;
static unsigned int emi_physical_offset;

struct mst_tbl_entry {
	u32 master;
	u32 port;
	u32 id_mask;
	u32 id_val;
	char *note;
	char *name;
};

struct emi_mpu_notifier_block {
	struct list_head list;
	emi_mpu_notifier notifier;
};

static const struct mst_tbl_entry mst_tbl[] = {
	/* apmcu write */
	{
		.master    = MST_ID_APMCU_W0,
		.port      = MONITOR_PORT_APMCU,
		.id_mask   = 0x3F8,
		.id_val    = 0x000,
		.note      = "CA7: Processor nn Non-Cacheable or STREX",
		.name      = "APMCU",
	},
	{
		.master    = MST_ID_APMCU_W1,
		.port      = MONITOR_PORT_APMCU,
		.id_mask   = 0x3F8,
		.id_val    = 0x008,
		.note      = "CA7: Processor nn write to device",
		.name      = "APMCU"
	},
	{
		.master = MST_ID_APMCU_W2,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x010,
		.note = "CA7: processor nn write barrier transactions",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_W3,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3FE,
		.id_val = 0x01E,
		.note = "CA7: Write portion of barrier external DVM sync",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_W4,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3E0,
		.id_val = 0x020,
		.note = "CA7: Write to cachenable write address buffer bbbb",
		.name = "APMCU"
	},
	/* apmcu read */
	{
		.master = MST_ID_APMCU_R0,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x000,
		.note = "CA7: Processor nn Non-cacheable or LDREX instruction",
		.name = "APMCU"
	},

	{
		.master = MST_ID_APMCU_R1,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x008,
		.note = "CA7: Processor nn TLB",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R2,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x010,
		.note = "CA7: Processor nn read portion of barrier",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R3,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3FE,
		.id_val = 0x01E,
		.note = "CA7: Read portion of barrier by external DVM sync",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R4,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x020,
		.note = "CA7: Processor nn Line-Fill Buffer 0",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R5,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x028,
		.note = "CA7: Processor nn Line-Fill Buffer 1",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R6,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x030,
		.note = "CA7: Processor nn instruction",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R7,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x040,
		.note = "CA7: Processor nn STB0",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R8,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x048,
		.note = "CA7: Processor nn STB1",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R9,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x050,
		.note = "CA7: Processor nn STB2",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R10,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x058,
		.note = "CA7: Processor nn STB3",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R11,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x060,
		.note = "CA7: Processor nn DVM request",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R12,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F8,
		.id_val = 0x068,
		.note = "CA7: DVM complete message",
		.name = "APMCU"
	},
	{
		.master = MST_ID_APMCU_R13,
		.port = MONITOR_PORT_APMCU,
		.id_mask = 0x3F0,
		.id_val = 0x070,
		.note = "CA7: L2 linefill from Line-Fill buffer mmm",
		.name = "APMCU"
	},

	/* MM */
	{
		.master = MST_ID_MM_0,
		.port = MONITOR_PORT_MM,
		.id_mask = 0x380,
		.id_val = 0x000,
		.note = "MM: Larb0"
	},
	{
		.master = MST_ID_MM_1,
		.port = MONITOR_PORT_MM,
		.id_mask = 0x380,
		.id_val = 0x080,
		.note = "MM: Larb1"
	},
	{
		.master = MST_ID_MM_5,
		.port = MONITOR_PORT_MM,
		.id_mask = 0x3FF,
		.id_val = 0x3FC,
		.note = "MM IOMMU Internal Used",
		.name = "MM IOMMU"
	},
	{
		.master = MST_ID_MM_6,
		.port = MONITOR_PORT_MM,
		.id_mask = 0x3FF,
		.id_val = 0x3FD,
		.note = "MM IOMMU Internal Used",
		.name = "MM IOMMU"
	},

	/* Periperal */
	{
		.master = MST_ID_PERI_0,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FB,
		.id_val = 0x000,
		.note = "PERI: GCE",
		.name = "GCE"
	},
	{
		.master = MST_ID_PERI_1,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FF,
		.id_val = 0x002,
		.note = "PERI: PWM",
		.name = "PWM"
	},
	{
		.master = MST_ID_PERI_2,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FF,
		.id_val = 0x012,
		.note = "PERI: AUDIO",
		.name = "AUDIO"
	},
	{
		.master = MST_ID_PERI_3,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FF,
		.id_val = 0x022,
		.note = "PERI: SPI0",
		.name = "SPI0"

	},
	{
		.master = MST_ID_PERI_4,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FF,
		.id_val = 0x032,
		.note = "PERI: MSDC1",
		.name = "MSDC1"
	},
	{
		.master = MST_ID_PERI_5,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FF,
		.id_val = 0x042,
		.note = "PERI: SPM",
		.name = "SPM"
	},
	{
		.master = MST_ID_PERI_6,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FF,
		.id_val = 0x006,
		.note = "PERI: USB0",
		.name = "USB0"
	},
	{
		.master = MST_ID_PERI_7,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FF,
		.id_val = 0x016,
		.note = "PERI: MSDC0",
		.name = "MSDC0"
	},
	{
		.master = MST_ID_PERI_8,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FF,
		.id_val = 0x026,
		.note = "PERI: NFI",
		.name = "NFI"
	},
	{
		.master = MST_ID_PERI_9,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3FF,
		.id_val = 0x00A,
		.note = "PERI: APDMA",
		.name = "APDMA"
	},
	{
		.master = MST_ID_PERI_10,
		.port = MONITOR_PORT_PERI,
		.id_mask = 0x3E3,
		.id_val = 0x003,
		.note = "PERI: DEBUGTOP",
		.name = "DEBUGTOP"
	},

	/* MD MCU */
	{
		.master = MST_ID_MDMCU_0,
		.port = MONITOR_PORT_MDMCU,
		.id_mask = 0x3E0,
		.id_val = 0x000,
		.note = "MD MCU",
		.name = "MD MCU"
	},
	/* MD HW*/
	{
		.master = MST_ID_MDHW_0,
		.port = MONITOR_PORT_MDHW,
		.id_mask = 0x3E0,
		.id_val = 0x000,
		.note = "MD HW",
		.name = "MD HW"
	},

	/* MFG  */
	{
		.master = MST_ID_MFG_0,
		.port = MONITOR_PORT_MFG,
		.id_mask = 0x3E0,
		.id_val = 0x000,
		.note = "MFG",
		.name = "MFG"
	},

	/* Conn */
	{
		.master = MST_ID_CONN_0,
		.port = MONITOR_PORT_CONN,
		.id_mask = 0x3FF,
		.id_val = 0x000,
		.note = "CONNSYS",
		.name = "CONNSYS"
	},
};

static const char *UNKNOWN_MASTER = "unknown";
static spinlock_t emi_mpu_lock;

#ifdef ENABLE_EMI_CHKER
struct timer_list emi_axi_vio_timer;
#endif

char *smi_larb0_port[] = {
	"disp_ovl0",
	"disp_rdma0",
	"disp_wdma0",
	"mdp_rdma",
	"mdp_wdma",
	"mdp_wrot",
	"disp_fake"
};

char *smi_larb1_port[] = {
	"cam_imgo",
	"cam_img2o",
	"cam_lsci",
	"venc_bsdma_vdec_post0",
	"cam_imgi",
	"cam_esfko",
	"cam_aao",
	"venc_mvqp",
	"venc_mc",
	"venc_cdma_vdec_cdma",
	"venc_rec_vdec_wdma"
};


static char *__id2name_smi_larb(u32 axi_id)
{
	u32 mm_larb;
	u32 smi_port;
	char *name = NULL;

	mm_larb = axi_id >> 7;
	smi_port = (axi_id >> 2) & 0x1F;

	switch (mm_larb) {
	case 0:
		if (smi_port < ARRAY_SIZE(smi_larb0_port))
			name = smi_larb0_port[smi_port];
		break;
	case 1:
		if (smi_port < ARRAY_SIZE(smi_larb1_port))
			name = smi_larb1_port[smi_port];
		break;
	}

	return name;
}

static int __match_id(u32 axi_id, int tbl_idx, u32 port_ID, u32 read_vio)
{
	u32 mm_larb;
	char *smi_name;
	int ret = 0;

	if (((axi_id & mst_tbl[tbl_idx].id_mask) == mst_tbl[tbl_idx].id_val) &&
	    (port_ID == mst_tbl[tbl_idx].port)) {
		switch (port_ID) {
		case MONITOR_PORT_APMCU:
			if (read_vio && port_ID < MST_ID_APMCU_R0)
				goto out;

		case MONITOR_PORT_PERI:
		case MONITOR_PORT_MFG:
		case MONITOR_PORT_CONN:
			pr_err("Violation master name is %s.\n",
			       mst_tbl[tbl_idx].name);
			ret = 1;
			break;

		case MONITOR_PORT_MM:
			mm_larb = axi_id >> 7;
			if (mm_larb == 0x0 || mm_larb == 0x1) {
				smi_name = __id2name_smi_larb(axi_id);
				if (!smi_name) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					goto out;
				}
				pr_err("Violation master name is %s (%s).\n",
				       mst_tbl[tbl_idx].name, smi_name);
			} else {
				/*MM IOMMU Internal Used*/
				pr_err("Violation master name is %s.\n",
				       mst_tbl[tbl_idx].name);
			}
			ret = 1;
			break;
		default:
			pr_err("[EMI MPU ERROR] Invalidate port ID! lookup bus ID table failed!\n");
			break;
		}
	}

out:
	return ret;
}

static char *__id2name(u32 id)
{
	int i;
	u32 axi_ID;
	u32 port_ID;
	u32 read_vio;

	axi_ID   = (id >> 3) & 0x00003FF;
	port_ID  = id & 0x00000007;
	read_vio = (id >> 29) & 0x1;
	pr_err("[EMI MPU] axi_id = %x, port_id = %x\n", axi_ID, port_ID);

	if (port_ID == MONITOR_PORT_MM)	{
		char *name = __id2name_smi_larb(axi_ID);

		if (name)
			return name;
	}

	for (i = 0; i < ARRAY_SIZE(mst_tbl); i++) {
		if (__match_id(axi_ID, i, port_ID, read_vio))
			return mst_tbl[i].name;
	}
	return (char *)UNKNOWN_MASTER;
}

static void __clear_emi_mpu_vio(void)
{
	u32 dbg_s, dbg_t;

	/* clear violation status */
	mt_emi_reg_write(0x00FF03FF, EMI_MPUP);
	mt_emi_reg_write(0x00FF03FF, EMI_MPUQ);
	mt_emi_reg_write(0x00FF03FF, EMI_MPUR);
	mt_emi_reg_write(0x00FF03FF, EMI_MPUY);
	/* clear debug info */
	mt_emi_reg_write(0x80000000 , EMI_MPUS);
	dbg_s = mt_emi_reg_read(EMI_MPUS);
	dbg_t = mt_emi_reg_read(EMI_MPUT);

	if (dbg_s) {
		pr_err("Fail to clear EMI MPU violation\n");
		pr_err("EMI_MPUS = %x, EMI_MPUT = %x", dbg_s, dbg_t);
	}
}


static int mpu_check_violation(void)
{
	u32 dbg_s, dbg_t, dbg_pqry;
	u32 master_ID, domain_ID, wr_vio;
	s32 region;
	char *master_name;

	dbg_s = mt_emi_reg_read(EMI_MPUS);
	dbg_t = mt_emi_reg_read(EMI_MPUT);

	pr_alert("Clear status.\n");

	/*in 6580 Master ID valid bit is [12:0] */
	master_ID = (dbg_s & 0x00001FFF);
	domain_ID = (dbg_s >> 21) & 0x00000007;
	wr_vio = (dbg_s >> 28) & 0x00000003;
	region = (dbg_s >> 16) & 0xF;

	switch (domain_ID) {
	case 0:
		dbg_pqry = mt_emi_reg_read(EMI_MPUP);
		break;
	case 1:
		dbg_pqry = mt_emi_reg_read(EMI_MPUQ);
		break;
	case 2:
		dbg_pqry = mt_emi_reg_read(EMI_MPUR);
		break;
	case 3:
		dbg_pqry = mt_emi_reg_read(EMI_MPUY);
		break;

	default:
		dbg_pqry = 0;
		break;
	}

	/*TBD: print the abort region*/

	pr_err("EMI MPU violation.\n");
	pr_err("[EMI MPU] Debug info start ----------------------------------------\n");

	pr_err("EMI_MPUS = %x, EMI_MPUT = %x.\n", dbg_s, dbg_t);
	pr_err("Current process is \"%s \" (pid: %i).\n",
	       current->comm, current->pid);
	pr_err("Violation address is 0x%x.\n", dbg_t + emi_physical_offset);
	pr_err("Violation master ID is 0x%x.\n", master_ID);
	/*print out the murderer name*/
	master_name = __id2name(master_ID);
	pr_err("Violation domain ID is 0x%x.\n", domain_ID);
	pr_err("%s violation.\n", (wr_vio == 1) ? "Write" : "Read");
	pr_err("Corrupted region is %d\n\r", region);
	if (dbg_pqry & OOR_VIO)
		pr_err("Out of range violation.\n");

#if 1
	pr_err("[EMI MPU] EMI_MPUI = 0x%x\n", mt_emi_reg_read(EMI_MPUI));
	pr_err("[EMI MPU] EMI_MPUJ = 0x%x\n", mt_emi_reg_read(EMI_MPUJ));
	pr_err("[EMI MPU] EMI_MPUK = 0x%x\n", mt_emi_reg_read(EMI_MPUK));
	pr_err("[EMI MPU] EMI_MPUL = 0x%x\n", mt_emi_reg_read(EMI_MPUL));
	pr_err("[EMI MPU] EMI_MPUP = 0x%x\n", mt_emi_reg_read(EMI_MPUP));
	pr_err("[EMI MPU] EMI_MPUQ = 0x%x\n", mt_emi_reg_read(EMI_MPUQ));
	pr_err("[EMI MPU] EMI_MPUR = 0x%x\n", mt_emi_reg_read(EMI_MPUR));
	pr_err("[EMI MPU] EMI_MPUY = 0x%x\n", mt_emi_reg_read(EMI_MPUY));
	pr_err("[EMI MPU] EMI_MPUS = 0x%x\n", mt_emi_reg_read(EMI_MPUS));
	pr_err("[EMI MPU] EMI_MPUT = 0x%x\n", mt_emi_reg_read(EMI_MPUT));
	pr_err("[EMI MPU] EMI_WP_ADR = 0x%x\n", mt_emi_reg_read(EMI_WP_ADR));
	pr_err("[EMI MPU] EMI_WP_CTRL = 0x%x\n", mt_emi_reg_read(EMI_WP_CTRL));
	pr_err("[EMI MPU] EMI_CHKER = 0x%x\n", mt_emi_reg_read(EMI_CHKER));
	pr_err("[EMI MPU] EMI_CHKER_TYPE = 0x%x\n",
	       mt_emi_reg_read(EMI_CHKER_TYPE));
	pr_err("[EMI MPU] EMI_CHKER_ADR = 0x%x\n",
	       mt_emi_reg_read(EMI_CHKER_ADR));
#endif
	pr_err("[EMI MPU] Debug info end------------------------------------------\n");

#ifdef CONFIG_MTK_AEE_FEATURE
	if (wr_vio != 0) {
		u32 port_ID;

		port_ID  = master_ID & 0x00000007;
#if 1
		/*not support ccci md dump correctly */
		if (port_ID == MONITOR_PORT_MDMCU ||
		    port_ID == MONITOR_PORT_MDHW) {
			/* dump modem */
			int md_id = 0;

			exec_ccci_kern_func_by_md_id(md_id,
						     ID_DUMP_MD_REG,
						     NULL,
						     0);
		}
#endif

		if (!strncmp(master_name, "disp", 4))
			primary_display_diagnose();

		aee_kernel_exception("EMI MPU",
				     "%sEMI_MPUS = 0x%x,EMI_MPUT = 0x%x\n%s%s\n",
				     "EMI MPU violation.\n",
				     dbg_s,
				     dbg_t,
				     "CRDISPATCH_KEY:EMI MPU Violation Issue/",
				     master_name);
	}
#endif

	__clear_emi_mpu_vio();

	mt_devapc_clear_emi_violation();
	vio_addr = dbg_t + emi_physical_offset;
	return 0;

}


/*EMI MPU violation handler*/
static irqreturn_t mpu_violation_irq(int irq, void *dev_id)
{
	int res;

	/* Need DEVAPC owner porting */
	res = mt_devapc_check_emi_violation();
	if (res)
		return IRQ_NONE;

	pr_info("It's a MPU violation.\n");
	mpu_check_violation();
	return IRQ_HANDLED;
}

/*
 * emi_mpu_set_region_protection: protect a region.
 * @start: start address of the region
 * @end: end address of the region
 * @region: EMI MPU region id
 * @access_permission: EMI MPU access permission
 * Return 0 for success, otherwise negative status code.
 */
int emi_mpu_set_region_protection(unsigned int start,
				  unsigned int end,
				  int region,
				  unsigned int access_permission)
{
	int ret = 0;
	unsigned int tmp;
	unsigned int ax_pm, ax_pm2;
	unsigned long flags;

	if ((end != 0) || (start != 0)) {
		/*Address 64KB alignment*/
		start -= emi_physical_offset;
		end -= emi_physical_offset;
		start = start >> 16;
		end = end >> 16;

		if (end < start)
			return -EINVAL;
	}

	pr_err("[%s] start = 0x%x end = 0x%x region=%d\n",
	       __func__, start, end, region);


	ax_pm = (access_permission << 16) >> 16;
	ax_pm2 = (access_permission >> 16);

	spin_lock_irqsave(&emi_mpu_lock, flags);
	switch (region) {
	case 0:
		tmp = mt_emi_reg_read(EMI_MPUI) & 0xFFFF0000;
		mt_emi_reg_write(0, EMI_MPUI);
		mt_emi_reg_write((start << 16) | end, EMI_MPUA);
		mt_emi_reg_write(tmp | ax_pm, EMI_MPUI);
		break;

	case 1:
		tmp = mt_emi_reg_read(EMI_MPUI) & 0x0000FFFF;
		mt_emi_reg_write(0, EMI_MPUI);
		mt_emi_reg_write((start << 16) | end, EMI_MPUB);
		mt_emi_reg_write(tmp | (ax_pm << 16), EMI_MPUI);
		break;

	case 2:
		tmp = mt_emi_reg_read(EMI_MPUJ) & 0xFFFF0000;
		mt_emi_reg_write(0, EMI_MPUJ);
		mt_emi_reg_write((start << 16) | end, EMI_MPUC);
		mt_emi_reg_write(tmp | ax_pm, EMI_MPUJ);
		break;

	case 3:
		tmp = mt_emi_reg_read(EMI_MPUJ) & 0x0000FFFF;
		mt_emi_reg_write(0, EMI_MPUJ);
		mt_emi_reg_write((start << 16) | end, EMI_MPUD);
		mt_emi_reg_write(tmp | (ax_pm << 16), EMI_MPUJ);
		break;

	case 4:
		tmp = mt_emi_reg_read(EMI_MPUK) & 0xFFFF0000;
		mt_emi_reg_write(0, EMI_MPUK);
		mt_emi_reg_write((start << 16) | end, EMI_MPUE);
		mt_emi_reg_write(tmp | ax_pm, EMI_MPUK);
		break;

	case 5:
		tmp = mt_emi_reg_read(EMI_MPUK) & 0x0000FFFF;
		mt_emi_reg_write(0, EMI_MPUK);
		mt_emi_reg_write((start << 16) | end, EMI_MPUF);
		mt_emi_reg_write(tmp | (ax_pm << 16), EMI_MPUK);
		break;

	case 6:
		tmp = mt_emi_reg_read(EMI_MPUL) & 0xFFFF0000;
		mt_emi_reg_write(0, EMI_MPUL);
		mt_emi_reg_write((start << 16) | end, EMI_MPUG);
		mt_emi_reg_write(tmp | ax_pm, EMI_MPUL);
		break;

	case 7:
		tmp = mt_emi_reg_read(EMI_MPUL) & 0x0000FFFF;
		mt_emi_reg_write(0, EMI_MPUL);
		mt_emi_reg_write((start << 16) | end, EMI_MPUH);
		mt_emi_reg_write(tmp | (ax_pm << 16), EMI_MPUL);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	spin_unlock_irqrestore(&emi_mpu_lock, flags);

#if 1
	pr_err("[EMI MPU] emi_physical_offset = 0x%x\n", emi_physical_offset);
	pr_err("[EMI MPU] EMI_MPUA = 0x%x\n", mt_emi_reg_read(EMI_MPUA));
	pr_err("[EMI MPU] EMI_MPUB = 0x%x\n", mt_emi_reg_read(EMI_MPUB));
	pr_err("[EMI MPU] EMI_MPUC = 0x%x\n", mt_emi_reg_read(EMI_MPUC));
	pr_err("[EMI MPU] EMI_MPUD = 0x%x\n", mt_emi_reg_read(EMI_MPUD));
	pr_err("[EMI MPU] EMI_MPUE = 0x%x\n", mt_emi_reg_read(EMI_MPUE));
	pr_err("[EMI MPU] EMI_MPUF = 0x%x\n", mt_emi_reg_read(EMI_MPUF));
	pr_err("[EMI MPU] EMI_MPUG = 0x%x\n", mt_emi_reg_read(EMI_MPUG));
	pr_err("[EMI MPU] EMI_MPUH = 0x%x\n", mt_emi_reg_read(EMI_MPUH));
#endif
	return ret;
}
EXPORT_SYMBOL(emi_mpu_set_region_protection);

static ssize_t emi_mpu_show(struct device_driver *driver, char *buf)
{
	char *ptr = buf;
	unsigned int start, end;
	unsigned int reg_value;
	unsigned int d0, d1, d2, d3;
	static char * const permission[] = {
		"No protect",
		"Only R/W for secure access",
		"Only R/W for secure access, and non-secure read access",
		"Only R/W for secure access, and non-secure write access",
		"Only R for secure/non-secure",
		"Both R/W are forbidden",
		"Only secure W is forbidden"
	};

	reg_value = mt_emi_reg_read(EMI_MPUA);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 0 --> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUB);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 1 --> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUC);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 2 --> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUD);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 3 --> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUE);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 4 --> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUF);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 5 --> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUG);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 6 --> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUH);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 7 --> 0x%x to 0x%x\n", start, end);

	ptr += sprintf(ptr, "\n");

	reg_value = mt_emi_reg_read(EMI_MPUI);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 0 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 1 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	reg_value = mt_emi_reg_read(EMI_MPUJ);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 2 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 3 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	reg_value = mt_emi_reg_read(EMI_MPUK);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 4 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 5 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	reg_value = mt_emi_reg_read(EMI_MPUL);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 6 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 7 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	return strlen(buf);
}

static ssize_t emi_mpu_store(struct device_driver *driver,
			     const char *buf,
			     size_t count)
{
	int i;
	unsigned int start_addr;
	unsigned int end_addr;
	unsigned int region;
	unsigned int access_permission;
	char *command;
	char *ptr;
	char *token[5];
	int err = 0;

	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("emi_mpu_store command overflow.");
		return count;
	}
	pr_err("emi_mpu_store: %s\n", buf);

	command = kmalloc((size_t)MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strcpy(command, buf);
	ptr = (char *)buf;

	if (!strncmp(buf, EN_MPU_STR, strlen(EN_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_devel("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 5; i++)
			pr_devel("token[%d] = %s\n", i, token[i]);

		err += kstrtoul(token[1], 16, (unsigned long *)&start_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&end_addr);
		err += kstrtoul(token[3], 16, (unsigned long *)&region);
		err += kstrtoul(token[4], 16,
				(unsigned long *)&access_permission);

		if (err)
			goto out;


		emi_mpu_set_region_protection(start_addr,
					      end_addr,
					      region,
					      access_permission);
		pr_err("Set EMI_MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x.\n",
		       start_addr,
		       end_addr,
		       region,
		       access_permission);
	} else if (!strncmp(buf, DIS_MPU_STR, strlen(DIS_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_devel("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 5; i++)
			pr_devel("token[%d] = %s\n", i, token[i]);

		err += kstrtoul(token[1], 16, (unsigned long *)&start_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&end_addr);
		err += kstrtoul(token[3], 16, (unsigned long *)&region);

		if (err)
			goto out;

		access_permission = SET_ACCESS_PERMISSON(NO_PROTECTION,
#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
							 NO_PROTECTION,
							 NO_PROTECTION,
							 NO_PROTECTION,
							 NO_PROTECTION,
#endif
							 NO_PROTECTION,
							 NO_PROTECTION,
							 NO_PROTECTION);
		emi_mpu_set_region_protection(0x0,
					      0x0,
					      region,
					      access_permission);
		pr_err("set EMI MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x\n",
		       0,
		       0,
		       region,
		       access_permission);
	} else {
		pr_err("Unknown emi_mpu command.\n");
	}

out:
	kfree(command);

	return count;
}

DRIVER_ATTR(mpu_config, 0644, emi_mpu_show, emi_mpu_store);

void mtk_search_full_pgtab(void)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	unsigned long addr;
#ifndef CONFIG_ARM_LPAE
	pte_t *pte;
	unsigned long addr_2nd, addr_2nd_end;
#endif
	unsigned int v_addr = vio_addr;

	/*FIXME: testing*/

	for (addr = 0xC0000000; addr < 0xFFF00000; addr += 0x100000) {
		pgd = pgd_offset(&init_mm, addr);
		if (pgd_none(*pgd) || !pgd_present(*pgd))
			continue;

		pud = pud_offset(pgd, addr);
		if (pud_none(*pud) || !pud_present(*pud))
			continue;

		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd) || !pmd_present(*pmd))
			continue;
#ifndef CONFIG_ARM_LPAE
		if ((pmd_val(*pmd) & PMD_TYPE_MASK) == PMD_TYPE_TABLE) {
			/* Page table entry*/

			addr_2nd = addr;
			addr_2nd_end = addr_2nd + 0x100000;
			for (; addr_2nd < (addr_2nd_end); addr_2nd += 0x1000) {
				pte = pte_offset_map(pmd, addr_2nd);

				if (((unsigned long)v_addr & PAGE_MASK)
				    == ((unsigned long)
					pte_val(*(pte)) & PAGE_MASK)) {
					pr_err("%s%lx. violation address = 0x%x\n",
					       "[EMI MPU] Find page entry section at pte: ",
					       (unsigned long)(pte),
					       v_addr);
					return;
				}
			}
		} else {
			/* Section */
			if (((unsigned long)pmd_val(*(pmd)) & SECTION_MASK)
			    == ((unsigned long)v_addr & SECTION_MASK)) {
				pr_err("%s%lx. violation address = 0x%x\n",
				       "[EMI MPU] Find page entry section at pmd: ",
				       (unsigned long)(pmd),
				       v_addr);
				return;
			}
		}
#endif
	}
	pr_err("[EMI MPU] ****** Can not find page table entry!");
	pr_err(" violation address = 0x%x ******\n", v_addr);

}

void emi_mpu_work_callback(struct work_struct *work)
{
	pr_err("[EMI MPU] Enter EMI MPU workqueue!\n");
	mtk_search_full_pgtab();
	pr_err("[EMI MPU] Exit EMI MPU workqueue!\n");
}

static ssize_t pgt_scan_show(struct device_driver *driver, char *buf)
{
	return 0;
}

static ssize_t pgt_scan_store(struct device_driver *driver,
			      const char *buf,
			      size_t count)
{
	unsigned int value;
	unsigned int ret;

	if (unlikely(kstrtoint(buf, 0, &value) != 1))
		return -EINVAL;

	if (value == 1)	{
		ret = queue_work(emi_mpu_workqueue, &emi_mpu_work);
		if (!ret)
			pr_devel("[EMI MPU] submit workqueue failed, ret = %d\n",
				 ret);
	}

	return count;
}
DRIVER_ATTR(pgt_scan, 0644, pgt_scan_show, pgt_scan_store);

#ifdef ENABLE_EMI_CHKER
static void emi_axi_set_chker(const unsigned int setting)
{
	int value;

	value = mt_emi_reg_read(EMI_CHKER);
	value &= ~(0x7 << 16);
	value |= (setting);

	mt_emi_reg_write(value, EMI_CHKER);
}

static void emi_axi_set_master(const unsigned int setting)
{
	int value;

	value = mt_emi_reg_read(EMI_CHKER);
	value &= ~(0x0F << AXI_NON_ALIGN_CHK_MST);
	value |= (setting & 0xF) << AXI_NON_ALIGN_CHK_MST;

	mt_emi_reg_write(value, EMI_CHKER);
}

static void emi_axi_dump_info(int aee_ke_en)
{
	int value, master_ID;
	char *master_name;

	value = mt_emi_reg_read(EMI_CHKER);
	master_ID = (value & 0x0000FFFF);

	if (value & 0x0000FFFF)	{
		pr_err("AXI violation.\n");
		pr_err("[EMI MPU AXI] Debug info start ------\n");

		pr_err("EMI_CHKER = %x.\n", value);
		pr_err("Violation address is 0x%x.\n",
		       mt_emi_reg_read(EMI_CHKER_ADR));
		pr_err("Violation master ID is 0x%x.\n", master_ID);
		pr_err("Violation type is: AXI_ADR_CHK_EN(%d)",
		       (value & (1 << AXI_ADR_VIO)) ? 1 : 0);
		pr_err("AXI_LOCK_CHK_EN(%d), ",
		       (value & (1 << AXI_LOCK_ISSUE)) ? 1 : 0);
		pr_err("AXI_NON_ALIGN_CHK_EN(%d).\n",
		       (value & (1 << AXI_NON_ALIGN_ISSUE)) ? 1 : 0);
		pr_err("%s violation.\n",
		       (value & (1 << AXI_VIO_WR)) ? "Write" : "Read");
		pr_err("[EMI MPU AXI] Debug info end ---------\n");

		master_name = __id2name(master_ID);
#ifdef CONFIG_MTK_AEE_FEATURE
		if (aee_ke_en)
			aee_kernel_exception("EMI MPU AXI",
					     "AXI violation.\nEMI_CHKER = 0x%x\nCRDISPATCH_KEY:EMI MPU Violation Issue/%s\n",
					     value, master_name);

#endif
		mt_emi_reg_write((1 << AXI_VIO_CLR) |
				 mt_emi_reg_read(EMI_CHKER), EMI_CHKER);
	}
}

static void emi_axi_vio_timer_func(unsigned long a)
{
	emi_axi_dump_info(1);

	mod_timer(&emi_axi_vio_timer, jiffies + AXI_VIO_MONITOR_TIME);
}

static ssize_t emi_axi_vio_show(struct device_driver *driver, char *buf)
{
	int value;

	value = mt_emi_reg_read(EMI_CHKER);

	emi_axi_dump_info(0);

	return snprintf(buf,
			PAGE_SIZE,
			"AXI vio setting is: ADR_CHK_EN %s, LOCK_CHK_EN %s, NON_ALIGN_CHK_EN %s\n",
			(value & (1 << AXI_ADR_CHK_EN)) ? "ON" : "OFF",
			(value & (1 << AXI_LOCK_CHK_EN)) ? "ON" : "OFF",
			(value & (1 << AXI_NON_ALIGN_CHK_EN)) ? "ON" : "OFF");
}

ssize_t emi_axi_vio_store(struct device_driver *driver,
			  const char *buf,
			  size_t count)
{
	int value;
	int cpu = 0;

	value = mt_emi_reg_read(EMI_CHKER);

	if (!strncmp(buf, "ADR_CHK_ON", strlen("ADR_CHK_ON"))) {
		emi_axi_set_chker(1 << AXI_ADR_CHK_EN);
		add_timer_on(&emi_axi_vio_timer, cpu);
	} else if (!strncmp(buf, "LOCK_CHK_ON", strlen("LOCK_CHK_ON"))) {
		emi_axi_set_chker(1 << AXI_LOCK_CHK_EN);
		add_timer_on(&emi_axi_vio_timer, cpu);
	} else if (!strncmp(buf, "NON_ALIGN_CHK_ON",
			    strlen("NON_ALIGN_CHK_ON"))) {
		emi_axi_set_chker(1 << AXI_NON_ALIGN_CHK_EN);
		add_timer_on(&emi_axi_vio_timer, cpu);
	} else if (!strncmp(buf, "OFF", strlen("OFF"))) {
		emi_axi_set_chker(0);
		del_timer(&emi_axi_vio_timer);
	} else {
		pr_err("invalid setting\n");
	}

	return count;
}

DRIVER_ATTR(emi_axi_vio,    0644, emi_axi_vio_show,     emi_axi_vio_store);

#endif /*#ifdef ENABLE_EMI_CHKER*/

#ifdef ENABLE_EMI_WATCH_POINT
static void emi_wp_set_address(unsigned int address)
{
	mt_emi_reg_write(address - emi_physical_offset, EMI_WP_ADR);
}

static void emi_wp_set_range(unsigned int range)
{
	unsigned int value;

	value = mt_emi_reg_read(EMI_WP_CTRL);
	value = (value & (~EMI_WP_RANGE)) | range;
	mt_emi_reg_write(value, EMI_WP_CTRL);
}

static void emi_wp_set_monitor_type(unsigned int type)
{
	unsigned int value;

	value = mt_emi_reg_read(EMI_WP_CTRL);
	value = (value & (~EMI_WP_RW_MONITOR)) |
		(type << EMI_WP_RW_MONITOR_SHIFT);
	mt_emi_reg_write(value, EMI_WP_CTRL);
}

#if 0
static void emi_wp_set_rw_disable(unsigned int type)
{
	unsigned int value;

	value = mt_emi_reg_read(EMI_WP_CTRL);
	value = (value & (~EMI_WP_RW_DISABLE)) |
		(type << EMI_WP_RW_DISABLE_SHIFT);
	mt_emi_reg_write(value, EMI_WP_CTRL);
}
#endif

static void emi_wp_enable(int enable)
{
	unsigned int value;

	/* Enable WP */
	value = mt_emi_reg_read(EMI_CHKER);
	value = (value & ~(1 << EMI_WP_ENABLE_SHIFT)) |
		(enable << EMI_WP_ENABLE_SHIFT);
	mt_emi_reg_write(value, EMI_CHKER);
}

static void emi_wp_slave_error_enable(unsigned int enable)
{
	unsigned int value;

	value = mt_emi_reg_read(EMI_WP_CTRL);
	value = (value & ~(1 << EMI_WP_SLVERR_SHIFT)) |
		(enable << EMI_WP_SLVERR_SHIFT);
	mt_emi_reg_write(value, EMI_WP_CTRL);
}

static void emi_wp_int_enable(unsigned int enable)
{
	unsigned int value;

	value = mt_emi_reg_read(EMI_WP_CTRL);
	value = (value & ~(1 << EMI_WP_INT_SHIFT)) |
		(enable << EMI_WP_INT_SHIFT);
	mt_emi_reg_write(value, EMI_WP_CTRL);
}

static void emi_wp_clr_status(void)
{
	unsigned int value;
	int result;

	value = mt_emi_reg_read(EMI_CHKER);
	value |= 1 << EMI_WP_VIO_CLR_SHIFT;
	mt_emi_reg_write(value, EMI_CHKER);

	result = mt_emi_reg_read(EMI_CHKER) & EMI_WP_AXI_ID;
	result |= mt_emi_reg_read(EMI_CHKER_TYPE);
	result |= mt_emi_reg_read(EMI_CHKER_ADR);

	if (result)
		pr_err("[EMI_WP] Clear WP status fail!!!!!!!!!!!!!!\n");
}

void emi_wp_get_status(void)
{
	unsigned int value, master_ID;
	char *master_name;

	value = mt_emi_reg_read(EMI_CHKER);

	if ((value & 0x80000000) == 0) {
		pr_err("[EMI_WP] No watch point hit\n");
		return;
	}

	master_ID = (value & EMI_WP_AXI_ID);
	pr_err("[EMI_WP] Violation master ID is 0x%x.\n", master_ID);
	pr_err("[EMI_WP] Violation Address is : 0x%X\n",
	       mt_emi_reg_read(EMI_CHKER_ADR) + emi_physical_offset);

	master_name = __id2name(master_ID);
	pr_err("[EMI_WP] EMI_CHKER = 0x%x, module is %s.\n",
	       value, master_name);

	value = mt_emi_reg_read(EMI_CHKER_TYPE);
	pr_err("[EMI_WP] Transaction Type is : %d beat, %d byte, %s burst type (0x%X)\n",
	       (value & 0xF) + 1,
	       1 << ((value >> 4) & 0x7),
	       (value >> 7 & 1) ? "INCR" : "WRAP",
	       value);

	emi_wp_clr_status();
}

static int emi_wp_set(unsigned int enable,
		      unsigned int address,
		      unsigned int range,
		      unsigned int rw)
{
	if (address < emi_physical_offset) {
		pr_err("[EMI_WP] Address error, you can't set address less than 0x%X\n",
		       emi_physical_offset);
		return -1;
	}
	if (range < 4 || range > 32) {
		pr_err("[EMI_WP] Range error, you can't set range less %s",
		       "than 16 bytes and more than 4G bytes\n");
		return -1;
	}

	emi_wp_set_monitor_type(rw);
	emi_wp_set_address(address);
	emi_wp_set_range(range);
	emi_wp_slave_error_enable(1);
	emi_wp_int_enable(0);
	emi_wp_enable(enable);

	return 0;
}

static ssize_t emi_wp_vio_show(struct device_driver *driver, char *buf)
{
	unsigned int value, master_ID, type, vio_addr;
	char *master_name;
	char *ptr = buf;

	value = mt_emi_reg_read(EMI_CHKER);

	if ((value & 0x80000000) == 0)
		return snprintf(buf, PAGE_SIZE,
				"[EMI_WP] No watch point hit\n");


	master_ID = (value & EMI_WP_AXI_ID);
	master_name = __id2name(master_ID);

	type = mt_emi_reg_read(EMI_CHKER_TYPE);
	vio_addr = mt_emi_reg_read(EMI_CHKER_ADR) + emi_physical_offset;
	emi_wp_clr_status();
	ptr += snprintf(ptr, PAGE_SIZE, "[EMI WP] vio setting is: CHKER 0x%X, ",
			value);

	ptr += snprintf(ptr, PAGE_SIZE, "module is %s, Address is : 0x%X," ,
			master_name,
			vio_addr);

	ptr += snprintf(ptr, PAGE_SIZE, "Transaction Type is : %d beat, ",
			(type & 0xF) + 1);

	ptr += snprintf(ptr, PAGE_SIZE, "%d byte, %s burst type (0x%X)\n",
			1 << ((type >> 4) & 0x7),
			(type >> 7 & 1) ? "INCR" : "WRAP",
			type);
	return ptr - buf;
}

ssize_t emi_wp_vio_store(struct device_driver *driver,
			 const char *buf,
			 size_t count)
{
	int i;
	unsigned int wp_addr;
	unsigned int range, start_addr, end_addr;
	unsigned int rw;
	char *command;
	char *ptr;
	char *token[5];
	int err = 0;

	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("emi_wp_store command overflow.");
		return count;
	}
	pr_err("emi_wp_store: %s\n", buf);

	command = kmalloc((size_t)MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strcpy(command, buf);
	ptr = (char *)buf;

	if (!strncmp(buf, EN_WP_STR, strlen(EN_WP_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_devel("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 4; i++)
			pr_devel("token[%d] = %s\n", i, token[i]);


		err += kstrtoul(token[1], 16, (unsigned long *)&wp_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&range);
		err += kstrtoul(token[3], 16, (unsigned long *)&rw);
		if (err)
			goto out;

		emi_wp_set(1, wp_addr, range, rw);

		start_addr = (wp_addr >> range) << range;
		end_addr = start_addr + (1 << range)     - 1;
		pr_err("Set EMI_WP: address: 0x%x, range:%d, start addr: 0x%x, end addr: 0x%x,  rw: %d .\n",
		       wp_addr, range, start_addr, end_addr, rw);
	} else if (!strncmp(buf, DIS_WP_STR, strlen(DIS_WP_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_devel("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 4; i++)
			pr_devel("token[%d] = %s\n", i, token[i]);



		err += kstrtoul(token[1], 16, (unsigned long *)&wp_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&range);
		err += kstrtoul(token[3], 16, (unsigned long *)&rw);
		if (err)
			goto out;

		emi_wp_set(0, 0x40000000, 4, 2);
		pr_err("disable EMI WP\n");
	} else {
		pr_err("Unknown emi_wp command.\n");
	}

out:
	kfree(command);

	return count;
}


DRIVER_ATTR(emi_wp_vio, 0644, emi_wp_vio_show, emi_wp_vio_store);
#endif /*#ifdef ENABLE_EMI_WATCH_POINT*/

#define AP_REGION_ID   7

static void protect_ap_region(void)
{
	unsigned int ap_mem_mpu_id, ap_mem_mpu_attr;
	unsigned int kernel_base;
	phys_addr_t dram_size;

	kernel_base = PHYS_OFFSET;
	dram_size = get_max_DRAM_size();

	ap_mem_mpu_id = AP_REGION_ID;
	ap_mem_mpu_attr = SET_ACCESS_PERMISSON(NO_PROTECTION,
					       FORBIDDEN,
					       FORBIDDEN,
					       NO_PROTECTION);

	pr_err("[EMI] protect_ap_region 0x%x~0x%x   ,dram size=0x%x\n",
	       kernel_base,
	       (kernel_base + (unsigned int)dram_size - 1),
	       (unsigned int)dram_size);
	emi_mpu_set_region_protection(kernel_base,
				      (kernel_base+dram_size-1),
				      ap_mem_mpu_id,
				      ap_mem_mpu_attr);
}


static struct platform_driver emi_mpu_ctrl = {
	.driver = {
		.name = "emi_mpu_ctrl",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
	.id_table = NULL,
};
/*
  static struct notifier_block emi_mpu_blk = {
  .notifier_call    = emi_mpu_panic_cb,
  };*/


static int __init emi_mpu_mod_init(void)
{
	int ret;
	struct device_node *node;
	unsigned int mpu_irq;

	pr_err("[EMI MPU] Initialize EMI MPU.\n");
	emi_physical_offset = 0x80000000;

	/* DTS version */

	node = of_find_compatible_node(NULL, NULL, "mediatek,EMI");
	if (node) {
		mt_emi_reg_base_set(of_iomap(node, 0));
		pr_notice("get EMI_BASE_ADDR @ %p\n", mt_emi_reg_base_get());
	} else {
		pr_err("can't find compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,DEVAPC");
	if (node) {
		mpu_irq = irq_of_parse_and_map(node, 0);
		pr_notice("get EMI_MPU irq = %d\n", mpu_irq);
	} else {
		pr_err("can't find compatible node\n");
		return -1;
	}
	spin_lock_init(&emi_mpu_lock);

	pr_err("[EMI MPU] EMI_MPUP = 0x%x\n", mt_emi_reg_read(EMI_MPUP));
	pr_err("[EMI MPU] EMI_MPUQ = 0x%x\n", mt_emi_reg_read(EMI_MPUQ));
	pr_err("[EMI MPU] EMI_MPUR = 0x%x\n", mt_emi_reg_read(EMI_MPUR));
	pr_err("[EMI MPU] EMI_MPUY = 0x%x\n", mt_emi_reg_read(EMI_MPUY));
	pr_err("[EMI MPU] EMI_MPUS = 0x%x\n", mt_emi_reg_read(EMI_MPUS));
	pr_err("[EMI MPU] EMI_MPUT = 0x%x\n", mt_emi_reg_read(EMI_MPUT));
	pr_err("[EMI MPU] EMI_WP_ADR = 0x%x\n", mt_emi_reg_read(EMI_WP_ADR));
	pr_err("[EMI MPU] EMI_WP_CTRL = 0x%x\n", mt_emi_reg_read(EMI_WP_CTRL));
	pr_err("[EMI MPU] EMI_CHKER = 0x%x\n", mt_emi_reg_read(EMI_CHKER));
	pr_err("[EMI MPU] EMI_CHKER_TYPE = 0x%x\n",
	       mt_emi_reg_read(EMI_CHKER_TYPE));
	pr_err("[EMI MPU] EMI_CHKER_ADR = 0x%x\n",
	       mt_emi_reg_read(EMI_CHKER_ADR));

	/* Set Device APC initialization for EMI-MPU. */
	mt_devapc_emi_initial();

	if (mt_emi_reg_read(EMI_MPUS)) {
		pr_err("[EMI MPU] get MPU violation in driver init\n");
		mpu_check_violation();
	}
	__clear_emi_mpu_vio();

	/*
	 * NoteXXX: Interrupts of violation (including SPC in SMI, or EMI MPU)
	 *          are triggered by the device APC.
	 *          Need to share the interrupt with the SPC driver.
	 */

	ret = request_irq(mpu_irq,
			  (irq_handler_t)mpu_violation_irq,
			  IRQF_TRIGGER_LOW | IRQF_SHARED,
			  "mt_emi_mpu",
			  &emi_mpu_ctrl);
	if (ret != 0) {
		pr_err("Fail to request EMI_MPU interrupt. Error = %d.\n", ret);
		return ret;
	}

	/* need to protect ap region */
	protect_ap_region();


#ifdef ENABLE_EMI_CHKER
	/* AXI violation monitor setting and timer function create */
	mt_emi_reg_write((1 << AXI_VIO_CLR) |
			 mt_emi_reg_read(EMI_CHKER), EMI_CHKER);
	emi_axi_set_master(MONITOR_PORT_ALL);
	init_timer(&emi_axi_vio_timer);
	emi_axi_vio_timer.expires = jiffies + AXI_VIO_MONITOR_TIME;
	emi_axi_vio_timer.function = &emi_axi_vio_timer_func;
	emi_axi_vio_timer.data = ((unsigned long) 0);
#endif /*#ifdef ENABLE_EMI_CHKER*/

#if !defined(USER_BUILD_KERNEL)
#ifdef ENABLE_EMI_CHKER
	/* Enable AXI 4KB boundary violation monitor timer */
	/* emi_axi_set_chker(1 << AXI_ADR_CHK_EN); */
	/* add_timer_on(&emi_axi_vio_timer, 0); */
#endif

	/* register driver and create sysfs files */
	ret = platform_driver_register(&emi_mpu_ctrl);
	if (ret)
		pr_err("Fail to register EMI_MPU driver.\n");

	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_mpu_config);
	if (ret)
		pr_err("Fail to create MPU config sysfs file.\n");
#ifdef ENABLE_EMI_CHKER
	ret = driver_create_file(&emi_mpu_ctrl.driver,
				 &driver_attr_emi_axi_vio);
	if (ret)
		pr_err("Fail to create AXI violation monitor sysfs file.\n");
#endif
	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_pgt_scan);
	if (ret)
		pr_err("Fail to create pgt scan sysfs file.\n");

#ifdef ENABLE_EMI_WATCH_POINT
	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_emi_wp_vio);
	if (ret)
		pr_err("Fail to create WP violation monitor sysfs file.\n");
#endif
#endif

	/* atomic_notifier_chain_register(&panic_notifier_list,
	   &emi_mpu_blk); */

	/* Create a workqueue to search pagetable entry */
	emi_mpu_workqueue = create_singlethread_workqueue("emi_mpu");
	INIT_WORK(&emi_mpu_work, emi_mpu_work_callback);
	return 0;
}

static void __exit emi_mpu_mod_exit(void)
{
}

subsys_initcall(emi_mpu_mod_init);
module_exit(emi_mpu_mod_exit);

