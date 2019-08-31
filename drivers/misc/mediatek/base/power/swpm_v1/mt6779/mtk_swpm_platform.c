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
#include <linux/perf_event.h>
#ifdef CONFIG_MTK_QOS_FRAMEWORK
#include <mtk_qos_ipi.h>
#endif
#include <mtk_common_static_power.h>
#ifdef CONFIG_THERMAL
#include <mtk_thermal.h>
#endif
#ifdef CONFIG_MTK_MDLA_SUPPORT
#include <mdla_pmu.h>
#endif
#include <mach/mtk_cpufreq_api.h>
#include <mtk_swpm_common.h>
#include <mtk_swpm_platform.h>
#include <mtk_swpm.h>


/****************************************************************************
 *  Macro Definitions
 ****************************************************************************/

/****************************************************************************
 *  Type Definitions
 ****************************************************************************/

/****************************************************************************
 *  Local Variables
 ****************************************************************************/
static DEFINE_PER_CPU(struct perf_event *, l3dc_events);
static DEFINE_PER_CPU(struct perf_event *, inst_spec_events);
static DEFINE_PER_CPU(struct perf_event *, cycle_events);
static struct perf_event_attr l3dc_event_attr = {
	.type           = PERF_TYPE_RAW,
	.config         = 0x2B,
	.size           = sizeof(struct perf_event_attr),
	.pinned         = 1,
/*	.disabled       = 1, */
	.sample_period  = 0, /* 1000000000, */ /* ns ? */
};
static struct perf_event_attr inst_spec_event_attr = {
	.type           = PERF_TYPE_RAW,
	.config         = 0x1B,
	.size           = sizeof(struct perf_event_attr),
	.pinned         = 1,
/*	.disabled       = 1, */
	.sample_period  = 0, /* 1000000000, */ /* ns ? */
};
static struct perf_event_attr cycle_event_attr = {
	.type           = PERF_TYPE_HARDWARE,
	.config         = PERF_COUNT_HW_CPU_CYCLES,
	.size           = sizeof(struct perf_event_attr),
	.pinned         = 1,
/*	.disabled       = 1, */
	.sample_period  = 0, /* 1000000000, */ /* ns ? */
};

static struct aphy_pwr_data aphy_def_pwr_tbl[] = {
	[APHY_VCORE_0P7V] = {
		.read_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {8032, 9904, 10944, 12864, 14384},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {7168, 12640, 14320, 16528, 18800},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {7952, 10912, 12992, 14432, 16992},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {8608, 10528, 12128, 16608, 20128},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {12043, 14043, 19543, 22900, 26757},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {11712, 14650, 17088, 23838, 29963},
			},
		},
		.write_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {8032, 9904, 10944, 12864, 14384},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {7168, 12640, 14320, 16528, 18800},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {7952, 10912, 12992, 14432, 16992},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {8608, 10528, 12128, 16608, 20128},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {12043, 14043, 19543, 22900, 26757},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {11712, 14650, 17088, 23838, 29963},
			},
		},
		.coef_idle = {1898, 3030, 3570, 4195, 5359, 6264},
	},
	[APHY_VDDQ_0P6V] = {
		.read_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {877, 1530, 2037, 2837, 3703},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {465, 1887, 2607, 3523, 4440},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {722, 1777, 2865, 3648, 4707},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {397, 632, 882, 2107, 3373},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {318, 507, 1197, 1988, 2697},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {385, 615, 852, 1877, 3002},
			},
		},
		.write_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {877, 1530, 2037, 2837, 3703},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {465, 1887, 2607, 3523, 4440},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {722, 1777, 2865, 3648, 4707},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {397, 632, 882, 2107, 3373},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {318, 507, 1197, 1988, 2697},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {385, 615, 852, 1877, 3002},
			},
		},
		.coef_idle = {13, 10, 18, 10, 37, 32},
	},
	[APHY_VM_0P75V] = {
		.read_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {372, 687, 959, 1439, 2019},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {185, 735, 1089, 1579, 2152},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {281, 709, 1212, 1644, 2352},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {276, 420, 572, 1353, 2207},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {553, 735, 1593, 2197, 2953},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {449, 676, 893, 1897, 2897},
			},
		},
		.write_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {372, 687, 959, 1439, 2019},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {185, 735, 1089, 1579, 2152},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {281, 709, 1212, 1644, 2352},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {276, 420, 572, 1353, 2207},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {553, 735, 1593, 2197, 2953},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {449, 676, 893, 1897, 2897},
			},
		},
		.coef_idle = {95, 101, 103, 100, 100, 103},
	},
	[APHY_VIO_1P2V] = {
		.read_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {138, 263, 373, 573, 813},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {51, 222, 338, 500, 704},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {52, 178, 325, 461, 704},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {52, 82, 115, 283, 475},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {94, 128, 294, 428, 623},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {61, 94, 128, 298, 501},
			},
		},
		.write_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {138, 263, 373, 573, 813},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {51, 222, 338, 500, 704},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {52, 178, 325, 461, 704},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {52, 82, 115, 283, 475},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {94, 128, 294, 428, 623},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {61, 94, 128, 298, 501},
			},
		},
		.coef_idle = {91, 71, 109, 288, 323, 351},
	},
	[APHY_VIO_1P8V] = {
		.read_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {14, 14, 14, 14, 14},
			},
		},
		.write_pwr = {
			[DDR_400] = {
				.bw = {320, 640, 960, 1600, 2560},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_600] = {
				.bw = {240, 720, 1200, 1920, 2880},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_800] = {
				.bw = {256, 640, 1280, 1920, 3200},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_1200] = {
				.bw = {288, 480, 960, 1920, 3840},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_1600] = {
				.bw = {384, 640, 1920, 3200, 5120},
				.coef = {14, 14, 14, 14, 14},
			},
			[DDR_1866] = {
				.bw = {448, 746, 1493, 2986, 5971},
				.coef = {14, 14, 14, 14, 14},
			},
		},
		.coef_idle = {2, 2, 2, 14, 14, 14},
	},
};

static struct dram_pwr_conf dram_def_pwr_conf[] = {
	[DRAM_VDD1_1P8V] = {
		.i_dd0 = 6500,
		.i_dd2p = 600,
		.i_dd2n = 900,
		.i_dd4r = 8000,
		.i_dd4w = 9000,
		.i_dd5 = 15000,
		.i_dd6 = 900,
	},
	[DRAM_VDD2_1P1V] = {
		.i_dd0 = 57400,
		.i_dd2p = 600,
		.i_dd2n = 16000,
		.i_dd4r = 160000,
		.i_dd4w = 210000,
		.i_dd5 = 40000,
		.i_dd6 = 1100,
	},
	[DRAM_VDDQ_0P6V] = {
		.i_dd0 = 200,
		.i_dd2p = 200,
		.i_dd2n = 200,
		.i_dd4r = 250000,
		.i_dd4w = 600,
		.i_dd5 = 200,
		.i_dd6 = 100,
	},
};

#ifdef USE_GPU_POWER_TABLE
static struct mt_gpufreq_power_table_info *gpu_pwr_tbl;
static unsigned int gpu_opp_num;
#endif

/****************************************************************************
 *  Global Variables
 ****************************************************************************/

/****************************************************************************
 *  Static Function
 ****************************************************************************/
static void swpm_send_enable_ipi(unsigned int type, unsigned int enable)
{
#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) &&		\
	defined(CONFIG_MTK_QOS_FRAMEWORK)
	struct qos_ipi_data qos_d;

	qos_d.cmd = QOS_IPI_SWPM_ENABLE;
	qos_d.u.swpm_enable.type = type;
	qos_d.u.swpm_enable.enable = enable;
	qos_ipi_to_sspm_command(&qos_d, 3);
#endif
}

static void swpm_pmu_start(int cpu)
{
	struct perf_event *l3_event = per_cpu(l3dc_events, cpu);
	struct perf_event *i_event = per_cpu(inst_spec_events, cpu);
	struct perf_event *c_event = per_cpu(cycle_events, cpu);

	if (l3_event)
		perf_event_enable(l3_event);
	if (i_event)
		perf_event_enable(i_event);
	if (c_event)
		perf_event_enable(c_event);
}

static void swpm_pmu_stop(int cpu)
{
	struct perf_event *l3_event = per_cpu(l3dc_events, cpu);
	struct perf_event *i_event = per_cpu(inst_spec_events, cpu);
	struct perf_event *c_event = per_cpu(cycle_events, cpu);

	if (l3_event)
		perf_event_disable(l3_event);
	if (i_event)
		perf_event_disable(i_event);
	if (c_event)
		perf_event_disable(c_event);
}

static void swpm_pmu_set_enable(int cpu, int enable)
{
	struct perf_event *event;
	struct perf_event *l3_event = per_cpu(l3dc_events, cpu);
	struct perf_event *i_event = per_cpu(inst_spec_events, cpu);
	struct perf_event *c_event = per_cpu(cycle_events, cpu);

	if (enable) {
		if (!l3_event) {
			event = perf_event_create_kernel_counter(
				&l3dc_event_attr, cpu, NULL, NULL, NULL);
			if (IS_ERR(event)) {
				swpm_err("create l3dc counter error!\n");
				return;
			}
			per_cpu(l3dc_events, cpu) = event;
		}
		if (!i_event && cpu >= NR_CPU_L_CORE) {
			event = perf_event_create_kernel_counter(
				&inst_spec_event_attr, cpu, NULL, NULL, NULL);
			if (IS_ERR(event)) {
				swpm_err("create inst_spec counter error!\n");
				return;
			}
			per_cpu(inst_spec_events, cpu) = event;
		}
		if (!c_event && cpu >= NR_CPU_L_CORE) {
			event = perf_event_create_kernel_counter(
				&cycle_event_attr, cpu,	NULL, NULL, NULL);
			if (IS_ERR(event)) {
				swpm_err("create cycle counter error!\n");
				return;
			}
			per_cpu(cycle_events, cpu) = event;
		}

		swpm_pmu_start(cpu);
	} else {
		swpm_pmu_stop(cpu);

		if (l3_event) {
			per_cpu(l3dc_events, cpu) = NULL;
			perf_event_release_kernel(l3_event);
		}
		if (i_event) {
			per_cpu(inst_spec_events, cpu) = NULL;
			perf_event_release_kernel(i_event);
		}
		if (c_event) {
			per_cpu(cycle_events, cpu) = NULL;
			perf_event_release_kernel(c_event);
		}
	}
}

static void swpm_pmu_set_enable_all(unsigned int enable)
{
	int i;

#ifdef CONFIG_MTK_CACHE_CONTROL
	if (enable)
		ca_force_stop_set_in_kernel(1);
#endif

	for (i = 0; i < num_possible_cpus(); i++)
		swpm_pmu_set_enable(i, enable);

#ifdef CONFIG_MTK_CACHE_CONTROL
	if (!enable)
		ca_force_stop_set_in_kernel(0);
#endif
}

static void swpm_mdla_pmu_set_enable(unsigned int enable)
{
#ifdef CONFIG_MTK_MDLA_SUPPORT
	int i;

	if (enable) {
		/* register MDLA PMU */
		/* POOL */
		swpm_info_ref->mdla_pmu_idx[POOL] = pmu_counter_alloc(0x1A, 3);
		/* DW */
		swpm_info_ref->mdla_pmu_idx[DW] = pmu_counter_alloc(0x18, 3);
		/* FC */
		swpm_info_ref->mdla_pmu_idx[FC] = pmu_counter_alloc(0x18, 4);
		/* CONV */
		swpm_info_ref->mdla_pmu_idx[CONV] = pmu_counter_alloc(0x18, 2);
		/* EWE */
		swpm_info_ref->mdla_pmu_idx[EWE_L] =
			pmu_counter_alloc(0x1B, 3);
		swpm_info_ref->mdla_pmu_idx[EWE_H] =
			pmu_counter_alloc(0x1B, 4);
		/* STE */
		swpm_info_ref->mdla_pmu_idx[STE_L] =
			pmu_counter_alloc(0x14, 2);
		swpm_info_ref->mdla_pmu_idx[STE_H] =
			pmu_counter_alloc(0x15, 2);
	} else {
		/* release MDLA PMU */
		for (i = 0; i < NR_MDLA_PMU; i++)
			pmu_counter_free(swpm_info_ref->mdla_pmu_idx[i]);
	}
#endif
}

static unsigned int swpm_get_cpu_temp(enum cpu_lkg_type type)
{
	unsigned int temp;

	switch (type) {
	case CPU_L_LKG:
		temp = get_immediate_cpuL_wrap() / 1000;
		break;
	case CPU_B_LKG:
		temp = get_immediate_cpuB_wrap() / 1000;
		break;
	case DSU_LKG:
	default:
		temp = get_immediate_mcucci_wrap() / 1000;
		break;
	}

	return temp;
}

static int swpm_get_spower_devid(enum cpu_lkg_type type)
{
	int devid;

	switch (type) {
	case CPU_L_LKG:
		devid = MTK_SPOWER_CPULL;
		break;
	case CPU_B_LKG:
		devid = MTK_SPOWER_CPUL;
		break;
	case DSU_LKG:
	default:
		devid = MTK_SPOWER_CCI;
		break;
	}

	return devid;
}


/***************************************************************************
 *  API
 ***************************************************************************/
char *swpm_power_rail_to_string(enum power_rail p)
{
	char *s;

	switch (p) {
	case VPROC2:
		s = "VPROC2";
		break;
	case VPROC1:
		s = "VPROC1";
		break;
	case VGPU:
		s = "VGPU";
		break;
	case VCORE:
		s = "VCORE";
		break;
	case VDRAM:
		s = "VDRAM";
		break;
	case VIO12_DDR:
		s = "VIO12_DDR";
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
#ifdef USE_GPU_POWER_TABLE
	gpu_pwr_tbl = pass_gpu_table_to_eara();
	if (!gpu_pwr_tbl)
		swpm_err("Ptr of GPU power table is NULL!\n");
	else
		gpu_opp_num = mt_gpufreq_get_dvfs_table_num();
#endif

	/* copy pwr data */
	memcpy(swpm_info_ref->aphy_pwr_tbl, aphy_def_pwr_tbl,
		sizeof(aphy_def_pwr_tbl));
	memcpy(swpm_info_ref->dram_conf, dram_def_pwr_conf,
		sizeof(dram_def_pwr_conf));

	swpm_info("copy pwr data (size: aphy/dram = %ld/%ld) done!\n",
		(unsigned long)sizeof(aphy_def_pwr_tbl),
		(unsigned long)sizeof(dram_def_pwr_conf));

	return 0;
}

void swpm_send_init_ipi(unsigned int addr, unsigned int size,
	unsigned int ch_num)
{
#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) &&		\
	defined(CONFIG_MTK_QOS_FRAMEWORK)
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
	if (type == 0xFFFF) {
		int i;

		for_each_pwr_mtr(i) {
			if (enable) {
				if (swpm_get_status(i))
					continue;

				if (i == CPU_POWER_METER)
					swpm_pmu_set_enable_all(1);
				else if (i == MDLA_POWER_METER)
					swpm_mdla_pmu_set_enable(1);
				swpm_set_status(i);
			} else {
				if (!swpm_get_status(i))
					continue;

				if (i == CPU_POWER_METER)
					swpm_pmu_set_enable_all(0);
				else if (i == MDLA_POWER_METER)
					swpm_mdla_pmu_set_enable(0);
				swpm_clr_status(i);
			}
		}
		swpm_send_enable_ipi(type, enable);
	} else if (type < NR_POWER_METER) {
		if (enable && !swpm_get_status(type)) {
			if (type == CPU_POWER_METER)
				swpm_pmu_set_enable_all(1);
			else if (type == MDLA_POWER_METER)
				swpm_mdla_pmu_set_enable(1);
			swpm_set_status(type);
		} else if (!enable && swpm_get_status(type)) {
			if (type == CPU_POWER_METER)
				swpm_pmu_set_enable_all(0);
			else if (type == MDLA_POWER_METER)
				swpm_mdla_pmu_set_enable(0);
			swpm_clr_status(type);
		}
		swpm_send_enable_ipi(type, enable);
	}
}

void swpm_set_update_cnt(unsigned int type, unsigned int cnt)
{
	if (type != 0xFFFF && type >= NR_POWER_METER)
		return;

	swpm_lock(&swpm_mutex);
#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) &&		\
	defined(CONFIG_MTK_QOS_FRAMEWORK)
	{
		struct qos_ipi_data qos_d;

		qos_d.cmd = QOS_IPI_SWPM_SET_UPDATE_CNT;
		qos_d.u.swpm_set_update_cnt.type = type;
		qos_d.u.swpm_set_update_cnt.cnt = cnt;
		qos_ipi_to_sspm_command(&qos_d, 3);
	}
#endif
	swpm_unlock(&swpm_mutex);
}

void swpm_update_lkg_table(void)
{
	int temp, dev_id, volt, i, j;
	unsigned int lkg = 0;

	for (i = 0; i < NR_CPU_LKG_TYPE; i++) {
#ifdef CONFIG_THERMAL
		temp = swpm_get_cpu_temp((enum cpu_lkg_type)i);
#else
		temp = 85;
#endif
		for (j = 0; j < NR_CPU_OPP; j++) {
			volt = mt_cpufreq_get_volt_by_idx(i, j) / 100;
			dev_id = swpm_get_spower_devid((enum cpu_lkg_type)i);
			lkg = mt_spower_get_leakage(dev_id, volt, temp) * 1000;

			if (swpm_info_ref)
				swpm_info_ref->cpu_lkg_pwr[i][j] = lkg;
		}
	}
}

#ifdef USE_GPU_POWER_TABLE
unsigned int swpm_get_gpu_power(void)
{
	unsigned int pwr, loading, freq, i;

	if (!gpu_pwr_tbl || !gpu_opp_num) {
		swpm_err("Lookup GPU pwr tbl failed!\n");
		return 0;
	}

	if (!mtk_get_gpu_loading(&loading)) {
		swpm_err("Get GPU loading failed!\n");
		return 0;
	}

	freq = mt_gpufreq_get_cur_freq();
	for (i = (gpu_opp_num - 1); i >= 0; i--) {
		if (gpu_pwr_tbl[i].gpufreq_khz >= freq || i == 0)
			break;
	}

	pwr = gpu_pwr_tbl[i].gpufreq_power * loading * 130 / 32;

	swpm_dbg("GPU power = %duA, opp = %d, loading = %d\n",
		pwr, i, loading);

	return pwr;
}
#endif

