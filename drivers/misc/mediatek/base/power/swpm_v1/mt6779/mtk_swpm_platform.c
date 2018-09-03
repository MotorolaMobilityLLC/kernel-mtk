/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#ifdef CONFIG_CPU_PM
#include <linux/cpu_pm.h>
#endif
#include <helio-dvfsrc-ipi.h>
#include <mtk_swpm_common.h>
#include <mtk_swpm_platform.h>
#include <mtk_swpm.h>


/****************************************************************************
 *  Macro Definitions
 ****************************************************************************/
#ifdef USE_PMU
#define swpm_pmu_read(cpu, offs)		\
	__raw_readl(pmu_base[cpu] + (offs))
#define swpm_pmu_write(cpu, offs, val)	\
	mt_reg_sync_writel(val, pmu_base[cpu] + (offs))
#endif
#ifdef USE_MDLA_PMU
#define SET_MDLA_EVENT(itf, event)	(((itf) << 16) | (event))
#endif

/****************************************************************************
 *  Type Definitions
 ****************************************************************************/

/****************************************************************************
 *  Local Variables
 ****************************************************************************/
static struct aphy_pwr_data aphy_def_pwr_tbl[] = {
	[APHY_VCORE_0P7V] = {
		.read_pwr = {
			[DDR_400] = {
				.bw = {123, 833, 1238, 2407, 3173},
				.coef = {7607, 9760, 10266, 11073, 11396},
			},
			[DDR_600] = {
				.bw = {254, 823, 1309, 2470, 4668},
				.coef = {4484, 5514, 6060, 6944, 7687},
			},
			[DDR_800] = {
				.bw = {215, 884, 1195, 2545, 6335},
				.coef = {3483, 4818, 5362, 6437, 7860},
			},
			[DDR_1200] = {
				.bw = {307, 865, 1252, 2531, 6245},
				.coef = {5448, 6634, 7292, 8887, 10832},
			},
			[DDR_1600] = {
				.bw = {362, 884, 1212, 2424, 5939},
				.coef = {11393, 12678, 13242, 15327, 17899},
			},
			[DDR_1866] = {
				.bw = {362, 884, 1212, 2424, 5939},
				.coef = {9820, 11534, 12252, 14461, 17645},
			},
		},
		.write_pwr = {
			[DDR_400] = {
				.bw = {123, 833, 1238, 2407, 3173},
				.coef = {7607, 9760, 10266, 11073, 11396},
			},
			[DDR_600] = {
				.bw = {254, 823, 1309, 2470, 4668},
				.coef = {4484, 5514, 6060, 6944, 7687},
			},
			[DDR_800] = {
				.bw = {215, 884, 1195, 2545, 6335},
				.coef = {3483, 4818, 5362, 6437, 7860},
			},
			[DDR_1200] = {
				.bw = {307, 865, 1252, 2531, 6245},
				.coef = {5448, 6634, 7292, 8887, 10832},
			},
			[DDR_1600] = {
				.bw = {362, 884, 1212, 2424, 5939},
				.coef = {11393, 12678, 13242, 15327, 17899},
			},
			[DDR_1866] = {
				.bw = {362, 884, 1212, 2424, 5939},
				.coef = {9820, 11534, 12252, 14461, 17645},
			},
		},
		.coef_idle = {2478, 2498, 2498, 2574, 3850, 4607},
	},
	[APHY_VDDQ_0P6V] = {
		.read_pwr = {
			[DDR_400] = {
				.bw = {160, 640, 1280, 1920, 2880},
				.coef = {231, 706, 1078, 1246, 1326},
			},
			[DDR_600] = {
				.bw = {240, 720, 1440, 2880, 4320},
				.coef = {346, 901, 1350, 1735, 1843},
			},
			[DDR_800] = {
				.bw = {320, 960, 1600, 3200, 5120},
				.coef = {590, 1243, 1723, 2065, 2285},
			},
			[DDR_1200] = {
				.bw = {480, 960, 1920, 2880, 5760},
				.coef = {686, 1086, 1713, 1906, 2316},
			},
			[DDR_1600] = {
				.bw = {512, 1280, 1920, 3200, 6400},
				.coef = {361, 666, 856, 1110, 1386},
			},
			[DDR_1866] = {
				.bw = {448, 1493, 2240, 3733, 5973},
				.coef = {281, 631, 806, 1023, 1156},
			},
		},
		.write_pwr = {
			[DDR_400] = {
				.bw = {160, 640, 1280, 1920, 2880},
				.coef = {823, 2811, 4846, 6556, 8746},
			},
			[DDR_600] = {
				.bw = {240, 720, 1440, 2880, 4320},
				.coef = {1160, 3043, 5293, 8460, 11210},
			},
			[DDR_800] = {
				.bw = {320, 960, 1600, 3200, 5120},
				.coef = {1785, 4276, 6060, 9535, 11826},
			},
			[DDR_1200] = {
				.bw = {480, 960, 1920, 2880, 5760},
				.coef = {1981, 3420, 5601, 6856, 10915},
			},
			[DDR_1600] = {
				.bw = {512, 1280, 1920, 3200, 6400},
				.coef = {1553, 3386, 4723, 6986, 11043},
			},
			[DDR_1866] = {
				.bw = {448, 1493, 2240, 3733, 5973},
				.coef = {1085, 3090, 4293, 6326, 8693},
			},
		},
		.coef_idle = {6, 6, 6, 6, 6, 6},
	},
	[APHY_VM_1P1V] = {
		.read_pwr = {
			[DDR_400] = {
				.bw = {160, 640, 1280, 1920, 2880},
				.coef = {140, 355, 482, 521, 538},
			},
			[DDR_600] = {
				.bw = {240, 720, 1440, 2880, 4320},
				.coef = {157, 369, 516, 619, 634},
			},
			[DDR_800] = {
				.bw = {320, 960, 1600, 3200, 5120},
				.coef = {282, 534, 689, 776, 823},
			},
			[DDR_1200] = {
				.bw = {480, 960, 1920, 2880, 5760},
				.coef = {459, 687, 985, 1060, 1212},
			},
			[DDR_1600] = {
				.bw = {512, 1280, 1920, 3200, 6400},
				.coef = {562, 983, 1206, 1453, 1643},
			},
			[DDR_1866] = {
				.bw = {448, 1493, 2240, 3733, 5973},
				.coef = {523, 1149, 1416, 1659, 1769},
			},
		},
		.write_pwr = {
			[DDR_400] = {
				.bw = {160, 640, 1280, 1920, 2880},
				.coef = {332, 1023, 1689, 2243, 2972},
			},
			[DDR_600] = {
				.bw = {240, 720, 1440, 2880, 4320},
				.coef = {413, 1048, 1822, 2943, 4074},
			},
			[DDR_800] = {
				.bw = {320, 960, 1600, 3200, 5120},
				.coef = {753, 1753, 2489, 3889, 4943},
			},
			[DDR_1200] = {
				.bw = {480, 960, 1920, 2880, 5760},
				.coef = {1143, 1928, 3143, 3984, 6162},
			},
			[DDR_1600] = {
				.bw = {512, 1280, 1920, 3200, 6400},
				.coef = {1393, 2847, 3867, 5501, 8543},
			},
			[DDR_1866] = {
				.bw = {448, 1493, 2240, 3733, 5973},
				.coef = {1260, 3257, 4389, 6162, 8253},
			},
		},
		.coef_idle = {56, 56, 56, 56, 56, 56},
	},
	[APHY_VIO_1P8V] = {
		.read_pwr = {
			[DDR_400] = {
				.bw = {160, 640, 1280, 1920, 2880},
				.coef = {65, 249, 475, 649, 845},
			},
			[DDR_600] = {
				.bw = {240, 720, 1440, 2880, 4320},
				.coef = {66, 193, 377, 645, 843},
			},
			[DDR_800] = {
				.bw = {320, 960, 1600, 3200, 5120},
				.coef = {81, 238, 388, 662, 842},
			},
			[DDR_1200] = {
				.bw = {480, 960, 1920, 2880, 5760},
				.coef = {80, 159, 309, 442, 714},
			},
			[DDR_1600] = {
				.bw = {512, 1280, 1920, 3200, 6400},
				.coef = {64, 154, 223, 363, 622},
			},
			[DDR_1866] = {
				.bw = {448, 1493, 2240, 3733, 5973},
				.coef = {49, 158, 234, 372, 545},
			},
		},
		/* TODO: need to update power */
		.write_pwr = {
			[DDR_400] = {
				.bw = {160, 640, 1280, 1920, 2880},
				.coef = {332, 1023, 1689, 2243, 2972},
			},
			[DDR_600] = {
				.bw = {240, 720, 1440, 2880, 4320},
				.coef = {413, 1048, 1822, 2943, 4074},
			},
			[DDR_800] = {
				.bw = {320, 960, 1600, 3200, 5120},
				.coef = {753, 1753, 2489, 3889, 4943},
			},
			[DDR_1200] = {
				.bw = {480, 960, 1920, 2880, 5760},
				.coef = {1143, 1928, 3143, 3984, 6162},
			},
			[DDR_1600] = {
				.bw = {512, 1280, 1920, 3200, 6400},
				.coef = {1393, 2847, 3867, 5501, 8543},
			},
			[DDR_1866] = {
				.bw = {448, 1493, 2240, 3733, 5973},
				.coef = {1260, 3257, 4389, 6162, 8253},
			},
		},
		.coef_idle = {96, 84, 96, 197, 216, 230},
	},
};

static struct dram_pwr_conf dram_def_pwr_conf[] = {
	[DRAM_VDD1_1P8V] = {
		.i_dd0 = 10500,
		.i_dd2p = 600,
		.i_dd2n = 900,
		.i_dd4r = 14700,
		.i_dd4w = 16000,
		.i_dd5 = 4100,
		.i_dd6 = 900,
	},
	[DRAM_VDD2_1P1V] = {
		.i_dd0 = 48400,
		.i_dd2p = 600,
		.i_dd2n = 21100,
		.i_dd4r = 125700,
		.i_dd4w = 171500,
		.i_dd5 = 27100,
		.i_dd6 = 1100,
	},
	[DRAM_VDDQ_0P6V] = {
		.i_dd0 = 200,
		.i_dd2p = 200,
		.i_dd2n = 200,
		.i_dd4r = 153100,
		.i_dd4w = 600,
		.i_dd5 = 200,
		.i_dd6 = 100,
	},
};
#ifdef USE_PMU
static void __iomem *pmu_base[NR_CPUS];
static const struct of_device_id dbgapb_of_ids[] = {
	{.compatible = "mediatek,dbgapb_pmu",},
	{}
};
#endif
#ifdef USE_MDLA_PMU
static void __iomem *mdla_pmu_base;
/* TODO: check DT */
static const struct of_device_id mdla_of_ids[] = {
	{.compatible = "mediatek,mdla_biu_cfg",},
	{}
};
#endif

/****************************************************************************
 *  Global Variables
 ****************************************************************************/

/****************************************************************************
 *  Static Function
 ****************************************************************************/
static void swpm_send_enable_ipi(unsigned int type, unsigned int enable)
{
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	struct qos_ipi_data qos_d;

	qos_d.cmd = QOS_IPI_SWPM_ENABLE;
	qos_d.u.swpm_enable.type = type;
	qos_d.u.swpm_enable.enable = enable;
	qos_ipi_to_sspm_command(&qos_d, 3);
#endif
}

static void swpm_pmu_set_enable(int cpu, int enable)
{
#ifdef USE_PMU
	if (cpu >= num_possible_cpus())
		return;

	if (!pmu_base[cpu])
		return;

	if (enable) {
		/* set event type/count */
		if (cpu < 6) {
			swpm_pmu_write(cpu, OFF_PMEVTYPER0,
				PMU_EVTYPER_INC | PMU_EVENT_L3D_CACHE_RD);
			swpm_pmu_write(cpu, OFF_PMCNTENSET,
				PMU_CNTENSET_C | PMU_CNTENSET_EVT_CNT_L);
		} else {
			swpm_pmu_write(cpu, OFF_PMEVTYPER0,
				PMU_EVTYPER_INC | PMU_EVENT_L3D_CACHE_RD);
			swpm_pmu_write(cpu, OFF_PMEVTYPER1,
				PMU_EVTYPER_INC | PMU_EVENT_INST_SPEC);
			swpm_pmu_write(cpu, OFF_PMCNTENSET,
				PMU_CNTENSET_C | PMU_CNTENSET_EVT_CNT_B);
		}
		/* enable counters */
		swpm_pmu_write(cpu, OFF_PMCR,
			swpm_pmu_read(cpu, OFF_PMCR) | PMU_PMCR_E);
	} else {
		/* disable counters */
		swpm_pmu_write(cpu, OFF_PMCR,
			swpm_pmu_read(cpu, OFF_PMCR) & ~PMU_PMCR_E);
	}
#endif
}

static void swpm_pmu_set_enable_all(unsigned int enable)
{
	int i;

	for (i = 0; i < num_possible_cpus(); i++)
		swpm_pmu_set_enable(i, enable);
}

#ifdef CONFIG_CPU_PM
static int swpm_cpu_pm_notifier(struct notifier_block *self,
			       unsigned long cmd, void *v)
{
	unsigned int cur_cpu = smp_processor_id();
	unsigned long flags;

	swpm_lock(&swpm_spinlock, flags);

	if (!swpm_get_status(CPU_POWER_METER))
		goto end;

	switch (cmd) {
	case CPU_PM_ENTER:
		/* Do nothing */
		break;
	case CPU_PM_EXIT:
	case CPU_PM_ENTER_FAILED:
		/* re-enable PMU */
		swpm_pmu_set_enable(cur_cpu, 1);
		break;
	default:
		break;
	}

end:
	swpm_unlock(&swpm_spinlock, flags);

	return NOTIFY_OK;
}

static struct notifier_block swpm_cpu_pm_notifier_block = {
	.notifier_call = swpm_cpu_pm_notifier,
};
#endif

static int swpm_cpu_notifier(struct notifier_block *nfb,
				   unsigned long action, void *hcpu)
{
	unsigned int cur_cpu = (long)hcpu;
	unsigned long flags;

	swpm_lock(&swpm_spinlock, flags);

	if (!swpm_get_status(CPU_POWER_METER))
		goto end;

	switch (action) {
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:
		/* Do nothing */
		break;
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
	case CPU_DOWN_FAILED:
	case CPU_DOWN_FAILED_FROZEN:
		/* re-enable PMU */
		swpm_pmu_set_enable(cur_cpu, 1);
		break;
	default:
		break;
	}

end:
	swpm_unlock(&swpm_spinlock, flags);

	return NOTIFY_OK;
}

static struct notifier_block swpm_cpu_notifier_block = {
	.notifier_call = swpm_cpu_notifier,
	.priority   = INT_MIN,
};

/***************************************************************************
 *  API
 ***************************************************************************/
char *swpm_power_rail_to_string(enum power_rail p)
{
	char *s;

	switch (p) {
	case VPROC12:
		s = "VPROC12";
		break;
	case VPROC11:
		s = "VPROC11";
		break;
	case VGPU:
		s = "VGPU";
		break;
	case VCORE:
		s = "VCORE";
		break;
	case VDRAM1:
		s = "VDRAM1";
		break;
	case VIO18_DDR:
		s = "VIO18_DDR";
		break;
	case VIO18_DRAM:
		s = "VIO18_DRAM";
		break;
	case VVPU:
		s = "VVPU";
		break;
	case VMDLA:
		s = "VMDLA";
		break;
	default:
		s = "None";
		break;
	}

	return s;
}

int swpm_platform_init(void)
{
	int ret = 0;
#ifdef USE_PMU
	{
		int i;
		struct device_node *node;

		node = of_find_matching_node(NULL, dbgapb_of_ids);
		if (!node) {
			swpm_err("no dbgapb node found in device tree\n");
			return -1;
		}

		for (i = 0; i < num_possible_cpus(); i++) {
			pmu_base[i] = of_iomap(node, i);

			if (pmu_base[i] == NULL)
				swpm_err("MAP CPU%d PMU addr failed!\n", i);
		}
	}
#endif
#ifdef USE_MDLA_PMU
	{
		struct device_node *node;

		node = of_find_matching_node(NULL, mdla_of_ids);
		if (!node) {
			swpm_err("no mdla node found in device tree\n");
			return -1;
		}

		mdla_pmu_base = of_iomap(node, 0);
		if (mdla_pmu_base == NULL)
			swpm_err("MAP MDLA PMU addr failed!\n");
	}
#endif

	/* copy pwr data */
	memcpy(swpm_info_ref->aphy_pwr_tbl, aphy_def_pwr_tbl,
		sizeof(aphy_def_pwr_tbl));
	memcpy(swpm_info_ref->dram_conf, dram_def_pwr_conf,
		sizeof(dram_def_pwr_conf));

#ifdef CONFIG_CPU_PM
	cpu_pm_register_notifier(&swpm_cpu_pm_notifier_block);
#endif

	register_cpu_notifier(&swpm_cpu_notifier_block);

	swpm_info("copy pwr data (size: aphy/dram = %ld/%ld) done!\n",
		(unsigned long)sizeof(aphy_def_pwr_tbl),
		(unsigned long)sizeof(dram_def_pwr_conf));

	return ret;
}

void swpm_send_init_ipi(unsigned int addr, unsigned int size,
	unsigned int ch_num)
{
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	struct qos_ipi_data qos_d;

	qos_d.cmd = QOS_IPI_SWPM_INIT;
	qos_d.u.swpm_init.dram_addr = addr;
	qos_d.u.swpm_init.dram_size = size;
	qos_d.u.swpm_init.dram_ch_num = ch_num;
	qos_ipi_to_sspm_command(&qos_d, 4);
#endif
}

void swpm_set_enable(unsigned int type, unsigned int enable)
{
	unsigned long flags;

	swpm_lock(&swpm_spinlock, flags);

	if (type == 0xFFFF) {
		int i;

		for_each_pwr_mtr(i) {
			if (enable) {
				if (swpm_get_status(i))
					continue;

				if (i == CPU_POWER_METER)
					swpm_pmu_set_enable_all(1);
				swpm_set_status(i);
			} else {
				if (!swpm_get_status(i))
					continue;

				if (i == CPU_POWER_METER)
					swpm_pmu_set_enable_all(0);
				swpm_clr_status(i);
			}
		}
		swpm_send_enable_ipi(type, enable);
	} else if (type < NR_POWER_METER) {
		if (enable && !swpm_get_status(type)) {
			if (type == CPU_POWER_METER)
				swpm_pmu_set_enable_all(1);
			swpm_set_status(type);
		} else if (!enable && swpm_get_status(type)) {
			if (type == CPU_POWER_METER)
				swpm_pmu_set_enable_all(0);
			swpm_clr_status(type);
		}
		swpm_send_enable_ipi(type, enable);
	}

	swpm_unlock(&swpm_spinlock, flags);
}

void swpm_mdla_onoff_notify(unsigned int on)
{
#ifdef USE_MDLA_PMU
	unsigned long flags;

	swpm_lock(&swpm_spinlock, flags);

	if (on) {
		/* set event */
		/* POOL */
		mt_reg_sync_writel(SET_MDLA_EVENT(0x1A, 3), MDLA_PMU_PMU1_SEL);
		/* DW */
		mt_reg_sync_writel(SET_MDLA_EVENT(0x18, 3), MDLA_PMU_PMU2_SEL);
		/* FC */
		mt_reg_sync_writel(SET_MDLA_EVENT(0x18, 4), MDLA_PMU_PMU3_SEL);
		/* CONV */
		mt_reg_sync_writel(SET_MDLA_EVENT(0x18, 2), MDLA_PMU_PMU4_SEL);
		/* EWE */
		mt_reg_sync_writel(SET_MDLA_EVENT(0x1B, 3), MDLA_PMU_PMU5_SEL);
		mt_reg_sync_writel(SET_MDLA_EVENT(0x1B, 4), MDLA_PMU_PMU6_SEL);
		/* STE */
		mt_reg_sync_writel(SET_MDLA_EVENT(0x14, 2), MDLA_PMU_PMU7_SEL);
		mt_reg_sync_writel(SET_MDLA_EVENT(0x15, 2), MDLA_PMU_PMU8_SEL);
		/* enable */
		mt_reg_sync_writel(0x1FE0001, MDLA_PMU_CFG);
	} else {
		/* disable PMU */
		mt_reg_sync_writel(0, MDLA_PMU_CFG);
	}

	swpm_unlock(&swpm_spinlock, flags);
#endif
}

