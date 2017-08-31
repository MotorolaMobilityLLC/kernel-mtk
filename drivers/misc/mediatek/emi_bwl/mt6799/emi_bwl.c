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
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/printk.h>

#define MET_USER_EVENT_SUPPORT
/* #include <linux/met_drv.h> */

#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>

#include "emi_bwl.h"

DEFINE_SEMAPHORE(emi_bwl_sem);
void __iomem *EMI_BASE_ADDR;

#ifdef LOW_POWER_CORRELATION
#include <linux/delay.h>
void __iomem *APMIX_BASE_ADDR;
void __iomem *CKGEN_BASE_ADDR;
void __iomem *MMSYS_BASE_ADDR;
void __iomem *SMI_COMMON_BASE;
void __iomem *SMI_LARB0_BASE;
void __iomem *SMI_LARB1_BASE;

static unsigned int bw_r, bw_w, bw_rw;
#endif

static struct platform_driver mem_bw_ctrl = {
	.driver     = {
		.name = "mem_bw_ctrl",
		.owner = THIS_MODULE,
	},
};

static struct platform_driver ddr_type = {
	.driver     = {
		.name = "ddr_type",
		.owner = THIS_MODULE,
	},
};

#ifdef LOW_POWER_CORRELATION
static struct platform_driver fake_engine = {
	.driver     = {
		.name = "fake_engine",
		.owner = THIS_MODULE,
	},
};
#endif

/* define EMI bandwiwth limiter control table */
static struct emi_bwl_ctrl ctrl_tbl[NR_CON_SCE];

/* current concurrency scenario */
static int cur_con_sce = 0x0FFFFFFF;


/* define concurrency scenario strings */
static const char * const con_sce_str[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, \
arbd, arbe, arbf, arbg, arbh) (#con_sce),
#include "con_sce_lpddr4_3200.h"
#undef X_CON_SCE
};

/****************** For LPDDR3-1866******************/

static const unsigned int emi_arba_lpddr4_3200_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh) arba,
#include "con_sce_lpddr4_3200.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbb_lpddr4_3200_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh) arbb,
#include "con_sce_lpddr4_3200.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbc_lpddr4_3200_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh) arbc,
#include "con_sce_lpddr4_3200.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbd_lpddr4_3200_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh) arbd,
#include "con_sce_lpddr4_3200.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbe_lpddr4_3200_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh) arbe,
#include "con_sce_lpddr4_3200.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbf_lpddr4_3200_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh) arbf,
#include "con_sce_lpddr4_3200.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbg_lpddr4_3200_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh) arbg,
#include "con_sce_lpddr4_3200.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbh_lpddr4_3200_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh) arbh,
#include "con_sce_lpddr4_3200.h"
#undef X_CON_SCE
};


int get_dram_type(void)
{
		return LPDDR4_3200;
}


/*
 * mtk_mem_bw_ctrl: set EMI bandwidth limiter for memory bandwidth control
 * @sce: concurrency scenario ID
 * @op: either ENABLE_CON_SCE or DISABLE_CON_SCE
 * Return 0 for success; return negative values for failure.
 */
int mtk_mem_bw_ctrl(int sce, int op)
{
	int i, highest;

	if (sce >= NR_CON_SCE)
		return -1;

	if (op != ENABLE_CON_SCE && op != DISABLE_CON_SCE)
		return -1;
	if (in_interrupt())
		return -1;

	down(&emi_bwl_sem);

	if (op == ENABLE_CON_SCE)
		ctrl_tbl[sce].ref_cnt++;
	else if (op == DISABLE_CON_SCE) {
		if (ctrl_tbl[sce].ref_cnt != 0)
			ctrl_tbl[sce].ref_cnt--;
	}

	/* find the scenario with the highest priority */
	highest = -1;
	for (i = 0; i < NR_CON_SCE; i++) {
		if (ctrl_tbl[i].ref_cnt != 0) {
			highest = i;
			break;
		}
	}
	if (highest == -1)
		highest = CON_SCE_NORMAL;

    /* set new EMI bandwidth limiter value */
	if (highest != cur_con_sce) {
		if (get_dram_type() == LPDDR4_3200) {
			writel(emi_arba_lpddr4_3200_val[highest], EMI_ARBA);
			writel(emi_arbb_lpddr4_3200_val[highest], EMI_ARBB);
			writel(emi_arbc_lpddr4_3200_val[highest], EMI_ARBC);
			writel(emi_arbd_lpddr4_3200_val[highest], EMI_ARBD);
			writel(emi_arbe_lpddr4_3200_val[highest], EMI_ARBE);
			writel(emi_arbf_lpddr4_3200_val[highest], EMI_ARBF);
			writel(emi_arbg_lpddr4_3200_val[highest], EMI_ARBG);
			mt_reg_sync_writel(emi_arbh_lpddr4_3200_val[highest],
			EMI_ARBH);
			}
			cur_con_sce = highest;
			}
		up(&emi_bwl_sem);
		return 0;
}

/*
 * ddr_type_show: sysfs ddr_type file show function.
 * @driver:
 * @buf: the string of ddr type
 * Return the number of read bytes.
 */
static ssize_t ddr_type_show(struct device_driver *driver, char *buf)
{
	if (get_dram_type() == LPDDR4_3200)
		sprintf(buf, "LPDDR4_3200\n");
	else if (get_dram_type() == LPDDR2_1066)
		sprintf(buf, "LPDDR2_1066\n");

	return strlen(buf);
}

/*
 * ddr_type_store: sysfs ddr_type file store function.
 * @driver:
 * @buf:
 * @count:
 * Return the number of write bytes.
 */
static ssize_t ddr_type_store(struct device_driver *driver,
const char *buf, size_t count)
{
    /*do nothing*/
		return count;
}

DRIVER_ATTR(ddr_type, 0644, ddr_type_show, ddr_type_store);

/*
 * con_sce_show: sysfs con_sce file show function.
 * @driver:
 * @buf:
 * Return the number of read bytes.
 */
static ssize_t con_sce_show(struct device_driver *driver, char *buf)
{
	char *ptr = buf;
	int i = 0;

	if (cur_con_sce >= NR_CON_SCE)
		ptr += sprintf(ptr, "none\n");
	else
		ptr += sprintf(ptr, "current scenario: %s\n",
		con_sce_str[cur_con_sce]);

#if 1
	ptr += sprintf(ptr, "%s\n", con_sce_str[cur_con_sce]);
	ptr += sprintf(ptr, "EMI_ARBA = 0x%x\n",  readl(IOMEM(EMI_ARBA)));
	ptr += sprintf(ptr, "EMI_ARBB = 0x%x\n",  readl(IOMEM(EMI_ARBB)));
	ptr += sprintf(ptr, "EMI_ARBC = 0x%x\n",  readl(IOMEM(EMI_ARBC)));
	ptr += sprintf(ptr, "EMI_ARBD = 0x%x\n",  readl(IOMEM(EMI_ARBD)));
	ptr += sprintf(ptr, "EMI_ARBE = 0x%x\n",  readl(IOMEM(EMI_ARBE)));
	ptr += sprintf(ptr, "EMI_ARBF = 0x%x\n",  readl(IOMEM(EMI_ARBF)));
	ptr += sprintf(ptr, "EMI_ARBG = 0x%x\n",  readl(IOMEM(EMI_ARBG)));
	ptr += sprintf(ptr, "EMI_ARBH = 0x%x\n",  readl(IOMEM(EMI_ARBH)));
	for (i = 0; i < NR_CON_SCE; i++)
		ptr += sprintf(ptr, "%s = 0x%x\n", con_sce_str[i],
		ctrl_tbl[i].ref_cnt);

	pr_debug("[EMI BWL] EMI_ARBA = 0x%x\n", readl(IOMEM(EMI_ARBA)));
	pr_debug("[EMI BWL] EMI_ARBB = 0x%x\n", readl(IOMEM(EMI_ARBB)));
	pr_debug("[EMI BWL] EMI_ARBC = 0x%x\n", readl(IOMEM(EMI_ARBC)));
	pr_debug("[EMI BWL] EMI_ARBD = 0x%x\n", readl(IOMEM(EMI_ARBD)));
	pr_debug("[EMI BWL] EMI_ARBE = 0x%x\n", readl(IOMEM(EMI_ARBE)));
	pr_debug("[EMI BWL] EMI_ARBF = 0x%x\n", readl(IOMEM(EMI_ARBF)));
	pr_debug("[EMI BWL] EMI_ARBG = 0x%x\n", readl(IOMEM(EMI_ARBG)));
	pr_debug("[EMI BWL] EMI_ARBH = 0x%x\n", readl(IOMEM(EMI_ARBH)));
#endif

		return strlen(buf);

}

/*
 * con_sce_store: sysfs con_sce file store function.
 * @driver:
 * @buf:
 * @count:
 * Return the number of write bytes.
 */
static ssize_t con_sce_store(struct device_driver *driver,
const char *buf, size_t count)
{
	int i;

		for (i = 0; i < NR_CON_SCE; i++) {
			if (!strncmp(buf, con_sce_str[i],
				strlen(con_sce_str[i]))) {
				if (!strncmp(buf + strlen(con_sce_str[i]) + 1,
					EN_CON_SCE_STR,
					strlen(EN_CON_SCE_STR))) {
					mtk_mem_bw_ctrl(i, ENABLE_CON_SCE);
					pr_debug("concurrency scenario %s ON\n",
					con_sce_str[i]);
					break;
				} else if (!strncmp
					(buf + strlen(con_sce_str[i]) + 1,
					DIS_CON_SCE_STR,
					strlen(DIS_CON_SCE_STR))) {
					mtk_mem_bw_ctrl(i, DISABLE_CON_SCE);
					pr_debug("concurrency scenario %s OFF\n",
					con_sce_str[i]);
					break;
				}
			}
		}
		return count;
}

DRIVER_ATTR(concurrency_scenario, 0644, con_sce_show, con_sce_store);

#ifdef LOW_POWER_CORRELATION
unsigned int bw_mon_in_ms(TRANS_TYPE trans_type, unsigned int ms)
{
	unsigned int temp;
	unsigned int total_bw;
	unsigned int cycle_cnt;
	unsigned int dram_data_rate;

	pr_err("[EMI BW] period %d\n", ms);

	/* Clear bus monitor */
	temp = readl(IOMEM(EMI_BMEN)) & 0xfffffffc;
	writel(temp, EMI_BMEN);

	/* Setup EMI bus monitor */
	writel(0x00ff0000, EMI_BMEN); /* Reset counter and set WSCT for ALL */
	writel(0x00240003, EMI_MSEL);
	writel(0x00000018, EMI_MSEL2);
	writel(0x02000000, EMI_BMEN2);
	switch (trans_type) { /* Set monitor for read */
	case R:
		writel(0x55555555, EMI_BMRW0);
		break;
	case W:
		writel(0xaaaaaaaa, EMI_BMRW0);
		break;
	case RW:
		writel(0xffffffff, EMI_BMRW0);
		break;
	default:
		writel(0x55555555, EMI_BMRW0);
		break;
	}

	temp = readl(IOMEM(EMI_BMEN));
	writel(temp | 0x1, EMI_BMEN); /* Enable bus monitor */

	/* Wait for a while */
	mdelay(ms);

	temp = readl(IOMEM(EMI_BMEN));
	writel(temp | 0x2, EMI_BMEN); /* Pause bus monitor */

	total_bw = readl(IOMEM(EMI_WSCT)); /* Unit: 8 bytes */
	cycle_cnt = readl(IOMEM(EMI_BCNT)); /* frequency = dram data rate /4 */
	dram_data_rate = get_dram_data_rate();

	pr_err("[BM] Total BW %d, cycle count %d, DRAM data rate %d\n", total_bw, cycle_cnt, dram_data_rate);

	return (total_bw * 8 / cycle_cnt * dram_data_rate * 1000 / 4000);
}

static void set_fake_engine(void)
{
	unsigned int temp;

	writel(0x80000000, CLK_CFG_0_CLR);
	writel(0xffffffff, MMSYS_CG_CLR0);
	writel(0xffffffff, MMSYS_CG_CLR1);
	writel(0xffffffff, MMSYS_CG_CLR2);

	temp = readl(IOMEM(CLK_CFG_0));
	temp &= ~(0xff << 24);
	temp |= (0x2 << 24);
	temp |= (0x0 << 28);
	writel(temp, CLK_CFG_0);
	writel(0xfffffffd, CLK_CFG_UPDATE);
	writel(0x00000000, MMSYS_CG_CON0);
	writel(0x00000000, MMSYS_CG_CON1);

	temp = readl(IOMEM(CLK_CFG_5));
	temp &= ~(0x7 << 24);
	temp |= (0x5 << 24);
	temp |= (0x0 << 28);
	writel(temp, CLK_CFG_5);
	writel(0xfffffffd, CLK_CFG_UPDATE);

	/* SMI init */
	writel(0x3f << 12, MMSYS_CG_CLR1);
	writel(0x03 << 11, MMSYS_CG_CLR2);
	writel(0x01, SMI0_MON_ENA);
	writel(0x00, SMI0_MON_ENA);
	writel(0x01, SMI0_MON_CLR);
	writel(0x4444, SMI0_BUS_SEL);
	writel(0x01 << 9, MMSYS_CG_CLR2);
	writel(0x01, SMI_LARB0_MON_EN);
	writel(0x00, SMI_LARB0_MON_EN);
	writel(0x01, SMI_LARB0_MON_CLR);
	writel(0x01 << 10, MMSYS_CG_CLR2);
	writel(0x01, SMI_LARB1_MON_EN);
	writel(0x00, SMI_LARB1_MON_EN);
	writel(0x01, SMI_LARB1_MON_CLR);

	/* SMI common select */
	temp = readl(IOMEM(SMI0_BUS_SEL)) & ~(0x3 << 0);
	writel(temp | (0x0 << 0), SMI0_BUS_SEL);
	temp = readl(IOMEM(SMI0_BUS_SEL)) & ~(0x3 << 2);
	writel(temp | (0x1 << 2), SMI0_BUS_SEL);

	writel(0x0, SMI_LARB0_BASE + 0x39C);
	writel(0x0, SMI_LARB0_BASE + 0xF9C);
	writel(0x0, SMI_LARB1_BASE + 0x3A8);
	writel(0x0, SMI_LARB1_BASE + 0xFA8);

	/* SMI outstanding */
	writel(0x3f, SMI_LARB0_OSTDL_PORT + 0x1C);
	writel(0x3f, SMI_LARB1_OSTDL_PORT + 0x28);

	/* EMI outstanding */
	writel(0xcfcfcfcf, EMI_IOCL);
	writel(0x77777777, EMI_IOCL_2ND);
	writel(0xcfcfcfcf, EMI_IOCM);
	writel(0x77777777, EMI_IOCM_2ND);
}

static void run_fake_engine(
	unsigned int latency, unsigned int toggle_bits,
	unsigned int dis_r0, unsigned int dis_w0,
	unsigned int dis_r1, unsigned int dis_w1, unsigned int loop)
{
	unsigned int length;
	unsigned int temp;

	length = 0xf;

	writel(0x1, DISP_FAKE_ENG_EN);
	writel(0x1, DISP_FAKE_ENG2_EN);

	writel(dram_rank0_addr, DISP_FAKE_ENG_RD_ADDR);
	writel(dram_rank0_addr, DISP_FAKE_ENG_WR_ADDR);
	writel(dram_rank0_addr + 0x800, DISP_FAKE_ENG2_RD_ADDR);
	writel(dram_rank0_addr + 0x800, DISP_FAKE_ENG2_WR_ADDR);

	temp = (toggle_bits << 24) | (loop << 22) | (length);
	writel(temp, DISP_FAKE_ENG_CON0);
	writel(temp, DISP_FAKE_ENG2_CON0);

	/* burst length = 8, disable write, disable read, latency */
	temp = (7 << 12) | (dis_w0 << 11) | (dis_r0 << 10) | latency;
	writel(temp, DISP_FAKE_ENG_CON1);
	temp = (7 << 12) | (dis_w1 << 11) | (dis_r1 << 10) | latency;
	writel(temp, DISP_FAKE_ENG2_CON1);

	writel(0x3, DISP_FAKE_ENG_EN);
	writel(0x3, DISP_FAKE_ENG2_EN);

	if (loop == 0) {
		do {
			temp = readl(IOMEM(DISP_FAKE_ENG_STATE));
			temp |= readl(IOMEM(DISP_FAKE_ENG2_STATE));
		} while (temp & 0x01);
		writel(0x1, DISP_FAKE_ENG_EN);
		writel(0x1, DISP_FAKE_ENG2_EN);
	} else {
		if (readl(IOMEM(DISP_FAKE_ENG_STATE)) & 0x01)
			pr_err("[Fake] FA1 is still busy\n");
		if (readl(IOMEM(DISP_FAKE_ENG2_STATE)) & 0x01)
			pr_err("[Fake] FA2 is still busy\n");
	}
}

static ssize_t fake_engine_bw_store(struct device_driver *driver, const char *buf, size_t count)
{
	unsigned int latency, toggle, dis_r0, dis_w0, dis_r1, dis_w1, period;
	int ret;

	ret = sscanf(buf, "%u %u %u %u %u %u %u", &latency, &toggle, &dis_r0, &dis_w0, &dis_r1, &dis_w1, &period);

	pr_err("[Fake] latency %d, toggle %d, dis_r0 %d, dis_w0 %d, dis_r1 %d, dis_w1 %d, period %d\n",
		latency, toggle, dis_r0, dis_w0, dis_r1, dis_w1, period);
	set_fake_engine();
	run_fake_engine(latency, toggle, 1, 0, 1, 0, 0);
	run_fake_engine(latency, toggle, dis_r0, dis_w0, dis_r1, dis_w1, 1);

	bw_r  = bw_mon_in_ms(R, period);
	bw_w  = bw_mon_in_ms(W, period);
	bw_rw = bw_mon_in_ms(RW, period);

	pr_err("[Fake] R %d, W %d, RW %d\n", bw_r, bw_w, bw_rw);

	return count;
}

static ssize_t fake_engine_bw_show(struct device_driver *driver, char *buf)
{
	/* echo lat tog dr0 dw0 dr1 dw1 period > fake_engine_bw */
	return sprintf(buf, "BW: R %d MB/s, W %d MB/s, RW %d MB/s\n", bw_r, bw_w, bw_rw);
}

DRIVER_ATTR(fake_engine_bw, 0644, fake_engine_bw_show, fake_engine_bw_store);
#endif /* LOW_POWER_CORRELATION */

/*
 * emi_bwl_mod_init: module init function.
 */
static int __init emi_bwl_mod_init(void)
{
	int ret;
	struct device_node *node;

	/* DTS version */
	if (EMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,EMI");
		if (node) {
			EMI_BASE_ADDR = of_iomap(node, 0);
			pr_err("get EMI_BASE_ADDR @ %p\n", EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	ret = mtk_mem_bw_ctrl(CON_SCE_NORMAL, ENABLE_CON_SCE);
	if (ret)
		pr_err("[EMI/BWL] fail to set EMI bandwidth limiter\n");

	/* Register BW ctrl interface */
	ret = platform_driver_register(&mem_bw_ctrl);
	if (ret)
		pr_err("[EMI/BWL] fail to register EMI_BW_LIMITER driver\n");

	ret = driver_create_file(&mem_bw_ctrl.driver,
	&driver_attr_concurrency_scenario);
	if (ret)
		pr_err("[EMI/BWL] fail to create EMI_BW_LIMITER sysfs file\n");


	/* Register DRAM type information interface */
	ret = platform_driver_register(&ddr_type);
	if (ret)
		pr_err("[EMI/BWL] fail to register DRAM_TYPE driver\n");

	ret = driver_create_file(&ddr_type.driver, &driver_attr_ddr_type);
	if (ret)
		pr_err("[EMI/BWL] fail to create DRAM_TYPE sysfs file\n");

#ifdef LOW_POWER_CORRELATION
	if (APMIX_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-apmixedsys");
		if (node) {
			APMIX_BASE_ADDR = of_iomap(node, 0);
			pr_err("get APMIX_BASE_ADDR @ %p\n", APMIX_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (CKGEN_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-topckgen");
		if (node) {
			CKGEN_BASE_ADDR = of_iomap(node, 0);
			pr_err("get CKGEN_BASE_ADDR @ %p\n", CKGEN_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (MMSYS_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-mmsys_config");
		if (node) {
			MMSYS_BASE_ADDR = of_iomap(node, 0);
			pr_err("get MMSYS_BASE_ADDR @ %p\n", MMSYS_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	SMI_COMMON_BASE = ioremap(0x14028000, 0x1000);
	if (SMI_COMMON_BASE == NULL)
		pr_err("can't remap SMI_COMMON_BASE\n");
	pr_err("get SMI_COMMON_BASE @ %p\n", SMI_COMMON_BASE);

	if (SMI_LARB0_BASE == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,smi_larb0");
		if (node) {
			SMI_LARB0_BASE = of_iomap(node, 0);
			pr_err("get SMI_LARB0_BASE @ %p\n", SMI_LARB0_BASE);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (SMI_LARB1_BASE == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,smi_larb1");
		if (node) {
			SMI_LARB1_BASE = of_iomap(node, 0);
			pr_err("get SMI_LARB1_BASE @ %p\n", SMI_LARB1_BASE);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	/* Register fake engine interface */
	ret = platform_driver_register(&fake_engine);
	if (ret)
		pr_err("[EMI/BWL] fail to register fake engine\n");
	ret = driver_create_file(&fake_engine.driver, &driver_attr_fake_engine_bw);
	if (ret)
		pr_err("[EMI/BWL] fail to create fake engine sysfs file\n");
#endif

		return 0;
}

/*
 * emi_bwl_mod_exit: module exit function.
 */
static void __exit emi_bwl_mod_exit(void)
{
}

/* EXPORT_SYMBOL(get_dram_type); */

late_initcall(emi_bwl_mod_init);
module_exit(emi_bwl_mod_exit);

