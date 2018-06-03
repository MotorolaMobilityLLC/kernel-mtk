/*
 * arch/arm64/kernel/topology.c
 *
 * Copyright (C) 2011,2013,2014 Linaro Limited.
 *
 * Based on the arm32 version written by Vincent Guittot in turn based on
 * arch/sh/kernel/topology.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/node.h>
#include <linux/nodemask.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <asm/cputype.h>
#include <asm/topology.h>

/*
 * cpu capacity scale management
 */

/*
 * cpu capacity table
 * This per cpu data structure describes the relative capacity of each core.
 * On a heteregenous system, cores don't have the same computation capacity
 * and we reflect that difference in the cpu_capacity field so the scheduler
 * can take this difference into account during load balance. A per cpu
 * structure is preferred because each CPU updates its own cpu_capacity field
 * during the load balance except for idle cores. One idle core is selected
 * to run the rebalance_domains for all idle cores and the cpu_capacity can be
 * updated during this sequence.
 */
static DEFINE_PER_CPU(unsigned long, cpu_scale);

unsigned long arch_scale_cpu_capacity(struct sched_domain *sd, int cpu)
{
	return per_cpu(cpu_scale, cpu);
}

static void set_capacity_scale(unsigned int cpu, unsigned long capacity)
{
	per_cpu(cpu_scale, cpu) = capacity;
}

static int __init get_cpu_for_node(struct device_node *node)
{
	struct device_node *cpu_node;
	int cpu;

	cpu_node = of_parse_phandle(node, "cpu", 0);
	if (!cpu_node)
		return -1;

	for_each_possible_cpu(cpu) {
		if (of_get_cpu_node(cpu, NULL) == cpu_node) {
			of_node_put(cpu_node);
			return cpu;
		}
	}

	pr_crit("Unable to find CPU node for %s\n", cpu_node->full_name);

	of_node_put(cpu_node);
	return -1;
}

static int __init parse_core(struct device_node *core, int cluster_id,
			     int core_id)
{
	char name[10];
	bool leaf = true;
	int i = 0;
	int cpu;
	struct device_node *t;

	do {
		snprintf(name, sizeof(name), "thread%d", i);
		t = of_get_child_by_name(core, name);
		if (t) {
			leaf = false;
			cpu = get_cpu_for_node(t);
			if (cpu >= 0) {
				cpu_topology[cpu].cluster_id = cluster_id;
				cpu_topology[cpu].core_id = core_id;
				cpu_topology[cpu].thread_id = i;
			} else {
				pr_err("%s: Can't get CPU for thread\n",
				       t->full_name);
				of_node_put(t);
				return -EINVAL;
			}
			of_node_put(t);
		}
		i++;
	} while (t);

	cpu = get_cpu_for_node(core);
	if (cpu >= 0) {
		if (!leaf) {
			pr_err("%s: Core has both threads and CPU\n",
			       core->full_name);
			return -EINVAL;
		}

		cpu_topology[cpu].cluster_id = cluster_id;
		cpu_topology[cpu].core_id = core_id;
	} else if (leaf) {
		pr_err("%s: Can't get CPU for leaf core\n", core->full_name);
		return -EINVAL;
	}

	return 0;
}

static int __init parse_cluster(struct device_node *cluster, int depth)
{
	char name[10];
	bool leaf = true;
	bool has_cores = false;
	struct device_node *c;
	static int cluster_id __initdata;
	int core_id = 0;
	int i, ret;

	/*
	 * First check for child clusters; we currently ignore any
	 * information about the nesting of clusters and present the
	 * scheduler with a flat list of them.
	 */
	i = 0;
	do {
		snprintf(name, sizeof(name), "cluster%d", i);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			leaf = false;
			ret = parse_cluster(c, depth + 1);
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
		i++;
	} while (c);

	/* Now check for cores */
	i = 0;
	do {
		snprintf(name, sizeof(name), "core%d", i);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			has_cores = true;

			if (depth == 0) {
				pr_err("%s: cpu-map children should be clusters\n",
				       c->full_name);
				of_node_put(c);
				return -EINVAL;
			}

			if (leaf) {
				ret = parse_core(c, cluster_id, core_id++);
			} else {
				pr_err("%s: Non-leaf cluster with core %s\n",
				       cluster->full_name, name);
				ret = -EINVAL;
			}

			of_node_put(c);
			if (ret != 0)
				return ret;
		}
		i++;
	} while (c);

	if (leaf && !has_cores)
		pr_warn("%s: empty cluster\n", cluster->full_name);

	if (leaf)
		cluster_id++;

	return 0;
}

struct cpu_efficiency {
	const char *compatible;
	unsigned long efficiency;
};

/*
 * Table of relative efficiency of each processors
 * The efficiency value must fit in 20bit and the final
 * cpu_scale value must be in the range
 *   0 < cpu_scale < SCHED_CAPACITY_SCALE.
 * Processors that are not defined in the table,
 * use the default SCHED_CAPACITY_SCALE value for cpu_scale.
 */
static const struct cpu_efficiency table_efficiency[] = {
	{ "arm,cortex-a72", 4186 },
	{ "arm,cortex-a57", 3891 },
	{ "arm,cortex-a53", 2048 },
	{ NULL, },
};

static unsigned long *__cpu_capacity;
#define cpu_capacity(cpu)	__cpu_capacity[cpu]

static unsigned long max_cpu_perf, min_cpu_perf;

static int __init parse_dt_topology(void)
{
	struct device_node *cn, *map;
	int ret = 0;
	int cpu;

	cn = of_find_node_by_path("/cpus");
	if (!cn) {
		pr_err("No CPU information found in DT\n");
		return 0;
	}

	/*
	 * When topology is provided cpu-map is essentially a root
	 * cluster with restricted subnodes.
	 */
	map = of_get_child_by_name(cn, "cpu-map");
	if (!map)
		goto out;

	ret = parse_cluster(map, 0);
	if (ret != 0)
		goto out_map;

	/*
	 * Check that all cores are in the topology; the SMP code will
	 * only mark cores described in the DT as possible.
	 */
	for_each_possible_cpu(cpu)
		if (cpu_topology[cpu].cluster_id == -1)
			ret = -EINVAL;

out_map:
	of_node_put(map);
out:
	of_node_put(cn);
	return ret;
}

/*
 * Look for a customed capacity of a CPU in the cpu_capacity table during the
 * boot. The update of all CPUs is in O(n^2) for heteregeneous system but the
 * function returns directly for SMP systems or if there is no complete set
 * of cpu efficiency, clock frequency data for each cpu.
 */
static void update_cpu_capacity(unsigned int cpu)
{
	unsigned long capacity = cpu_capacity(cpu);

#ifdef CONFIG_MTK_SCHED_EAS_PLUS
	if (cpu_core_energy(cpu)) {
#else
	if (0) {
#endif
		/* if power table is found, get capacity of CPU from it */
		int max_cap_idx = cpu_core_energy(cpu)->nr_cap_states - 1;

		capacity = cpu_core_energy(cpu)->cap_states[max_cap_idx].cap;
	} else {
		if (!capacity || !max_cpu_perf) {
			cpu_capacity(cpu) = 0;
			return;
		}

		capacity *= SCHED_CAPACITY_SCALE;
		capacity /= max_cpu_perf;
	}
	set_capacity_scale(cpu, capacity);

	pr_info("CPU%d: update cpu_capacity %lu\n",
		cpu, arch_scale_cpu_capacity(NULL, cpu));
}

#ifdef CONFIG_MTK_SCHED_RQAVG_KS
/* To add this function for sched_avg.c */
unsigned long get_cpu_orig_capacity(unsigned int cpu)
{
	unsigned long capacity = cpu_capacity(cpu);

	if (!capacity || !max_cpu_perf)
		return 1024;

	capacity *= SCHED_CAPACITY_SCALE;
	capacity /= max_cpu_perf;

	return capacity;
}
#endif

static void __init parse_dt_cpu_capacity(void)
{
	const struct cpu_efficiency *cpu_eff;
	struct device_node *cn = NULL;
	int cpu = 0, i = 0;

	__cpu_capacity = kcalloc(nr_cpu_ids, sizeof(*__cpu_capacity),
				 GFP_NOWAIT);

	min_cpu_perf = ULONG_MAX;
	max_cpu_perf = 0;

	min_cpu_perf = ULONG_MAX;
	max_cpu_perf = 0;
	for_each_possible_cpu(cpu) {
		const u32 *rate;
		int len;
		unsigned long cpu_perf;

		/* too early to use cpu->of_node */
		cn = of_get_cpu_node(cpu, NULL);
		if (!cn) {
			pr_err("missing device node for CPU %d\n", cpu);
			continue;
		}

		for (cpu_eff = table_efficiency; cpu_eff->compatible; cpu_eff++)
			if (of_device_is_compatible(cn, cpu_eff->compatible))
				break;

		if (cpu_eff->compatible == NULL)
			continue;

		rate = of_get_property(cn, "clock-frequency", &len);
		if (!rate || len != 4) {
			pr_err("%s missing clock-frequency property\n",
				cn->full_name);
			continue;
		}

		cpu_perf = ((be32_to_cpup(rate)) >> 20) * cpu_eff->efficiency;
		cpu_capacity(cpu) = cpu_perf;

		max_cpu_perf = max(max_cpu_perf, cpu_perf);
		min_cpu_perf = min(min_cpu_perf, cpu_perf);
		i++;
	}

	if (i < num_possible_cpus()) {
		max_cpu_perf = 0;
		min_cpu_perf = 0;
	}
}

/*
 * Scheduler load-tracking scale-invariance
 *
 * Provides the scheduler with a scale-invariance correction factor that
 * compensates for frequency scaling.
 */

static DEFINE_PER_CPU(atomic_long_t, cpu_freq_capacity);
static DEFINE_PER_CPU(atomic_long_t, cpu_max_freq);

/* cpufreq callback function setting current cpu frequency */
void arch_scale_set_curr_freq(int cpu, unsigned long freq)
{
	unsigned long max = atomic_long_read(&per_cpu(cpu_max_freq, cpu));
	unsigned long curr;

	if (!max)
		return;

	curr = (freq * SCHED_CAPACITY_SCALE) / max;

	atomic_long_set(&per_cpu(cpu_freq_capacity, cpu), curr);
}

/* cpufreq callback function setting max cpu frequency */
void arch_scale_set_max_freq(int cpu, unsigned long freq)
{
	atomic_long_set(&per_cpu(cpu_max_freq, cpu), freq);
}

unsigned long arch_scale_freq_capacity(struct sched_domain *sd, int cpu)
{
	unsigned long curr = atomic_long_read(&per_cpu(cpu_freq_capacity, cpu));

	if (!curr)
		return SCHED_CAPACITY_SCALE;

	return curr;
}

unsigned long arch_get_max_cpu_capacity(int cpu)
{
	return per_cpu(cpu_scale, cpu);
}

unsigned long arch_get_cur_cpu_capacity(int cpu)
{
	unsigned long scale_freq = atomic_long_read(&per_cpu(cpu_freq_capacity, cpu));

	if (!scale_freq)
		scale_freq = SCHED_CAPACITY_SCALE;

	return (per_cpu(cpu_scale, cpu) * scale_freq / SCHED_CAPACITY_SCALE);
}

/*
 * cpu topology table
 */
struct cpu_topology cpu_topology[NR_CPUS];
EXPORT_SYMBOL_GPL(cpu_topology);

/*
 * MTK static specific energy cost model data. There are no unit requirements for
 * the data. Data can be normalized to any reference point, but the
 * normalization must be consistent. That is, one bogo-joule/watt must be the
 * same quantity for all data, but we don't care what it is.
 */

static struct idle_state idle_states_cluster_0[] = {
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
};

static struct idle_state idle_states_cluster_1[] = {
	{ .power = 0},
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0},
};

static struct capacity_state cap_states_cluster_0[] = {
	/* Power per cluster */
	{ .cap =   97, .power =   2, .leak_power = 0, .volt = 60}, /*  [0] P15 MHz */
	{ .cap =  148, .power =   3, .leak_power = 0, .volt = 60}, /*  [1] P14 MHz */
	{ .cap =  193, .power =   3, .leak_power = 0, .volt = 60}, /*  [2] P13 MHz */
	{ .cap =  245, .power =   4, .leak_power = 0, .volt = 60}, /*  [3] P12 MHz */
	{ .cap =  296, .power =   5, .leak_power = 0, .volt = 63}, /*  [4] P11 MHz */
	{ .cap =  353, .power =   7, .leak_power = 0, .volt = 66}, /*  [5] P10 MHz */
	{ .cap =  415, .power =   9, .leak_power = 0, .volt = 70}, /*  [6] P09 MHz */
	{ .cap =  466, .power =  11, .leak_power = 0, .volt = 74}, /*  [7] P08 MHz */
	{ .cap =  518, .power =  13, .leak_power = 0, .volt = 78}, /*  [8] P07 MHz */
	{ .cap =  540, .power =  15, .leak_power = 0, .volt = 79}, /*  [9] P06 MHz */
	{ .cap =  569, .power =  16, .leak_power = 0, .volt = 81}, /*  [10] P05 MHz */
	{ .cap =  603, .power =  19, .leak_power = 0, .volt = 84}, /*  [11] P04 MHz */
	{ .cap =  631, .power =  21, .leak_power = 0, .volt = 87}, /*  [12] P03 MHz */
	{ .cap =  660, .power =  23, .leak_power = 0, .volt = 89}, /*  [13] P02 MHz */
	{ .cap =  683, .power =  25, .leak_power = 0, .volt = 92}, /*  [14] P01 MHz */
	{ .cap =  717, .power =  29, .leak_power = 0, .volt = 95}, /*  [15] P00 MHz */
};

static struct capacity_state cap_states_cluster_1[] = {
	/* Power per cluster */
	{ .cap =  193, .power =   5, .leak_power = 0, .volt = 60}, /*  [0] P15 MHz */
	{ .cap =  233, .power =   6, .leak_power = 0, .volt = 60}, /*  [1] P14 MHz */
	{ .cap =  324, .power =   8, .leak_power = 0, .volt = 60}, /*  [2] P13 MHz */
	{ .cap =  393, .power =  10, .leak_power = 0, .volt = 60}, /*  [3] P12 MHz */
	{ .cap =  455, .power =  12, .leak_power = 0, .volt = 63}, /*  [4] P11 MHz */
	{ .cap =  546, .power =  15, .leak_power = 0, .volt = 66}, /*  [5] P10 MHz */
	{ .cap =  637, .power =  20, .leak_power = 0, .volt = 70}, /*  [6] P09 MHz */
	{ .cap =  717, .power =  25, .leak_power = 0, .volt = 74}, /*  [7] P08 MHz */
	{ .cap =  785, .power =  30, .leak_power = 0, .volt = 78}, /*  [8] P07 MHz */
	{ .cap =  819, .power =  33, .leak_power = 0, .volt = 79}, /*  [9] P06 MHz */
	{ .cap =  859, .power =  36, .leak_power = 0, .volt = 81}, /*  [10] P05 MHz */
	{ .cap =  905, .power =  42, .leak_power = 0, .volt = 84}, /*  [11] P04 MHz */
	{ .cap =  933, .power =  46, .leak_power = 0, .volt = 87}, /*  [12] P03 MHz */
	{ .cap =  961, .power =  50, .leak_power = 0, .volt = 89}, /*  [13] P02 MHz */
	{ .cap =  990, .power =  54, .leak_power = 0, .volt = 92}, /*  [14] P01 MHz */
	{ .cap = 1024, .power =  60, .leak_power = 0, .volt = 95}, /*  [15] P00 MHz */
};

static struct sched_group_energy energy_cluster_0 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_cluster_0),
	.idle_states    = idle_states_cluster_0,
	.nr_cap_states  = ARRAY_SIZE(cap_states_cluster_0),
	.cap_states     = cap_states_cluster_0,
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

static struct sched_group_energy energy_cluster_1 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_cluster_1),
	.idle_states    = idle_states_cluster_1,
	.nr_cap_states  = ARRAY_SIZE(cap_states_cluster_1),
	.cap_states     = cap_states_cluster_1,
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

static struct sched_group_energy energy_cluster_2 = {
	.nr_idle_states = 0,
	.idle_states    = NULL,
	.nr_cap_states  = 0,
	.cap_states     = NULL,
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

static struct idle_state idle_states_core_0[] = {
	{ .power = 0 }, /* 0: active idle = WFI, [P8].leak */
	{ .power = 0 }, /* 1: disabled */
	{ .power = 0 }, /* 2: disabled */
	{ .power = 0 }, /* 3: disabled */
	{ .power = 0 }, /* 4: MCDI */
	{ .power = 0 }, /* 5: disabled */
	{ .power = 0 }, /* 6: WFI/SPARK */
};

static struct idle_state idle_states_core_1[] = {
	{ .power = 0 }, /* 0: active idle = WFI, [P8].leak */
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 }, /* 4: MCDI */
	{ .power = 0 },
	{ .power = 0 }, /* 6: WFI/SPARK */
};

static struct capacity_state cap_states_core_0[] = {
	/* Power per cpu */
	{ .cap =   97, .power =    7, .leak_power =  0, .volt = 60}, /*  [0] 221 MHz */
	{ .cap =  148, .power =   11, .leak_power =  0, .volt = 60}, /*  [1] 338 MHz */
	{ .cap =  193, .power =   14, .leak_power =  0, .volt = 60}, /*  [2] 442 MHz */
	{ .cap =  245, .power =   18, .leak_power =  0, .volt = 60}, /*  [3] 559 MHz */
	{ .cap =  296, .power =   22, .leak_power =  0, .volt = 63}, /*  [4] 676 Mhz */
	{ .cap =  353, .power =   29, .leak_power =  0, .volt = 66}, /*  [5] 806 Mhz */
	{ .cap =  415, .power =   38, .leak_power =  0, .volt = 70}, /*  [6] 949 Mhz */
	{ .cap =  466, .power =   47, .leak_power =  0, .volt = 74}, /*  [7] 1.066 Ghz */
	{ .cap =  518, .power =   58, .leak_power =  0, .volt = 78}, /*  [8] 1.183 Ghz */
	{ .cap =  540, .power =   64, .leak_power =  0, .volt = 79}, /*  [9] 1.235 Ghz */
	{ .cap =  569, .power =   71, .leak_power =  0, .volt = 81}, /*  [10] 1.300 Ghz */
	{ .cap =  603, .power =   81, .leak_power =  0, .volt = 84}, /*  [11] 1.378 Ghz */
	{ .cap =  631, .power =   90, .leak_power =  0, .volt = 87}, /*  [12] 1.443 Ghz */
	{ .cap =  660, .power =  100, .leak_power =  0, .volt = 89}, /*  [13] 1.508 Ghz */
	{ .cap =  683, .power =  110, .leak_power =  0, .volt = 92}, /*  [14] 1.560 Ghz */
	{ .cap =  717, .power =  124, .leak_power =  0, .volt = 95}, /*  [15] 1.638 Ghz */
};

static struct capacity_state cap_states_core_1[] = {
	/* Power per cpu */
	{ .cap =  193, .power =   20, .leak_power =  0, .volt = 60}, /*  [0] 442 MHz */
	{ .cap =  233, .power =   24, .leak_power =  0, .volt = 60}, /*  [1] 533 MHz */
	{ .cap =  324, .power =   33, .leak_power =  0, .volt = 60}, /*  [2] 741 MHz */
	{ .cap =  393, .power =   40, .leak_power =  0, .volt = 60}, /*  [3] 897 MHz */
	{ .cap =  455, .power =   46, .leak_power =  0, .volt = 63}, /*  [4] 1.040 Mhz */
	{ .cap =  546, .power =   60, .leak_power =  0, .volt = 66}, /*  [5] 1.248 Mhz */
	{ .cap =  637, .power =   79, .leak_power =  0, .volt = 70}, /*  [6] 1.456 Mhz */
	{ .cap =  717, .power =   99, .leak_power =  0, .volt = 74}, /*  [7] 1.638 Ghz */
	{ .cap =  785, .power =  121, .leak_power =  0, .volt = 78}, /*  [8] 1.794 Ghz */
	{ .cap =  819, .power =  132, .leak_power =  0, .volt = 79}, /*  [9] 1.872 Ghz */
	{ .cap =  859, .power =  146, .leak_power =  0, .volt = 81}, /*  [10] 1.963 Ghz */
	{ .cap =  905, .power =  166, .leak_power =  0, .volt = 84}, /*  [11] 2.067 Ghz */
	{ .cap =  933, .power =  182, .leak_power =  0, .volt = 87}, /*  [12] 2.132 Ghz */
	{ .cap =  961, .power =  199, .leak_power =  0, .volt = 89}, /*  [13] 2.197 Ghz */
	{ .cap =  990, .power =  217, .leak_power =  0, .volt = 92}, /*  [14] 2.262 Ghz */
	{ .cap = 1024, .power =  241, .leak_power =  0, .volt = 95}, /*  [15] 2.340 Ghz */
};

static struct sched_group_energy energy_core_0 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_core_0),
	.idle_states    = idle_states_core_0,
	.nr_cap_states  = ARRAY_SIZE(cap_states_core_0),
	.cap_states     = cap_states_core_0,
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

static struct sched_group_energy energy_core_1 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_core_1),
	.idle_states    = idle_states_core_1,
	.nr_cap_states  = ARRAY_SIZE(cap_states_core_1),
	.cap_states     = cap_states_core_1,
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

static struct sched_group_energy energy_core_2 = {
	.nr_idle_states = 0,
	.idle_states    = NULL,
	.nr_cap_states  = 0,
	.cap_states     = NULL,
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

/* sd energy functions */
inline
const struct sched_group_energy * const cpu_cluster_energy(int cpu)
{
	if (cpu_topology[cpu].cluster_id == 0)
		return &energy_cluster_0;
	else if (cpu_topology[cpu].cluster_id == 1)
		return &energy_cluster_1;
	else if (cpu_topology[cpu].cluster_id == 2)
		return &energy_cluster_2;
	else
		return NULL;
}

inline
const struct sched_group_energy * const cpu_core_energy(int cpu)
{
	if (cpu_topology[cpu].cluster_id == 0)
		return &energy_core_0;
	else if (cpu_topology[cpu].cluster_id == 1)
		return &energy_core_1;
	else if (cpu_topology[cpu].cluster_id == 2)
		return &energy_core_2;
	else
		return NULL;
}

const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return &cpu_topology[cpu].core_sibling;
}

static inline int cpu_corepower_flags(void)
{
	return SD_SHARE_PKG_RESOURCES  | SD_SHARE_POWERDOMAIN | SD_SHARE_CAP_STATES;
}

static struct sched_domain_topology_level arm64_topology[] = {
#ifdef CONFIG_SCHED_MC
	{ cpu_coregroup_mask, cpu_corepower_flags, cpu_core_energy, SD_INIT_NAME(MC) },
#endif
	{ cpu_cpu_mask, NULL, cpu_cluster_energy, SD_INIT_NAME(DIE) },
	{ NULL, },
};

static void update_siblings_masks(unsigned int cpuid)
{
	struct cpu_topology *cpu_topo, *cpuid_topo = &cpu_topology[cpuid];
	int cpu;

	/* update core and thread sibling masks */
	for_each_possible_cpu(cpu) {
		cpu_topo = &cpu_topology[cpu];

		if (cpuid_topo->cluster_id != cpu_topo->cluster_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->core_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->core_sibling);

		if (cpuid_topo->core_id != cpu_topo->core_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->thread_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->thread_sibling);
	}
}

void store_cpu_topology(unsigned int cpuid)
{
	struct cpu_topology *cpuid_topo = &cpu_topology[cpuid];
	u64 mpidr;

	if (cpuid_topo->cluster_id != -1)
		goto topology_populated;

	mpidr = read_cpuid_mpidr();

	/* Uniprocessor systems can rely on default topology values */
	if (mpidr & MPIDR_UP_BITMASK)
		return;

	/* Create cpu topology mapping based on MPIDR. */
	if (mpidr & MPIDR_MT_BITMASK) {
		/* Multiprocessor system : Multi-threads per core */
		cpuid_topo->thread_id  = MPIDR_AFFINITY_LEVEL(mpidr, 0);
		cpuid_topo->core_id    = MPIDR_AFFINITY_LEVEL(mpidr, 1);
		cpuid_topo->cluster_id = MPIDR_AFFINITY_LEVEL(mpidr, 2) |
					 MPIDR_AFFINITY_LEVEL(mpidr, 3) << 8;
	} else {
		/* Multiprocessor system : Single-thread per core */
		cpuid_topo->thread_id  = -1;
		cpuid_topo->core_id    = MPIDR_AFFINITY_LEVEL(mpidr, 0);
		cpuid_topo->cluster_id = MPIDR_AFFINITY_LEVEL(mpidr, 1) |
					 MPIDR_AFFINITY_LEVEL(mpidr, 2) << 8 |
					 MPIDR_AFFINITY_LEVEL(mpidr, 3) << 16;
	}
#ifndef CONFIG_SCHED_HMP
	cpuid_topo->partno = read_cpuid_part_number();
#endif

	pr_debug("CPU%u: cluster %d core %d thread %d mpidr %#016llx\n",
		 cpuid, cpuid_topo->cluster_id, cpuid_topo->core_id,
		 cpuid_topo->thread_id, mpidr);

topology_populated:
#ifdef CONFIG_SCHED_HMP
	cpuid_topo->partno = read_cpuid_part_number();
#endif
	update_siblings_masks(cpuid);
	update_cpu_capacity(cpuid);
}

static void __init reset_cpu_topology(void)
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct cpu_topology *cpu_topo = &cpu_topology[cpu];

		cpu_topo->thread_id = -1;
		cpu_topo->core_id = 0;
		cpu_topo->cluster_id = -1;

		cpumask_clear(&cpu_topo->core_sibling);
		cpumask_set_cpu(cpu, &cpu_topo->core_sibling);
		cpumask_clear(&cpu_topo->thread_sibling);
		cpumask_set_cpu(cpu, &cpu_topo->thread_sibling);

		set_capacity_scale(cpu, SCHED_CAPACITY_SCALE);
	}
}

static int cpu_topology_init;
/*
 * init_cpu_topology is called at boot when only one cpu is running
 * which prevent simultaneous write access to cpu_topology array
 */

/*
 * init_cpu_topology is called at boot when only one cpu is running
 * which prevent simultaneous write access to cpu_topology array
 */
void __init init_cpu_topology(void)
{
	if (cpu_topology_init)
		return;
	reset_cpu_topology();

	/*
	 * Discard anything that was parsed if we hit an error so we
	 * don't use partial information.
	 */
	if (of_have_populated_dt() && parse_dt_topology())
		reset_cpu_topology();
	else
		set_sched_topology(arm64_topology);

	parse_dt_cpu_capacity();
}

#ifdef CONFIG_MTK_CPU_TOPOLOGY
void __init arch_build_cpu_topology_domain(void)
{
	init_cpu_topology();
	cpu_topology_init = 1;
}

#endif

/*
 * Extras of CPU & Cluster functions
 */
int arch_cpu_is_big(unsigned int cpu)
{
	struct cpu_topology *cpu_topo = &cpu_topology[cpu];

	switch (cpu_topo->partno) {
	case ARM_CPU_PART_CORTEX_A57:
	case ARM_CPU_PART_CORTEX_A72:
		return 1;
	default:
		return 0;
	}
}

int arch_cpu_is_little(unsigned int cpu)
{
	return !arch_cpu_is_big(cpu);
}

int arch_is_smp(void)
{
	static int __arch_smp = -1;

	if (__arch_smp != -1)
		return __arch_smp;

	__arch_smp = (max_cpu_perf != min_cpu_perf) ? 0 : 1;

	return __arch_smp;
}

int arch_get_nr_clusters(void)
{
	static int __arch_nr_clusters = -1;
	int max_id = 0;
	unsigned int cpu;

	if (__arch_nr_clusters != -1)
		return __arch_nr_clusters;

	/* assume socket id is monotonic increasing without gap. */
	for_each_possible_cpu(cpu) {
		struct cpu_topology *cpu_topo = &cpu_topology[cpu];

		if (cpu_topo->cluster_id > max_id)
			max_id = cpu_topo->cluster_id;
	}
	__arch_nr_clusters = max_id + 1;
	return __arch_nr_clusters;
}

int arch_is_multi_cluster(void)
{
	return arch_get_nr_clusters() > 1 ? 1 : 0;
}

int arch_get_cluster_id(unsigned int cpu)
{
	struct cpu_topology *cpu_topo = &cpu_topology[cpu];

	return cpu_topo->cluster_id < 0 ? 0 : cpu_topo->cluster_id;
}

void arch_get_cluster_cpus(struct cpumask *cpus, int cluster_id)
{
	unsigned int cpu;

	cpumask_clear(cpus);
	for_each_possible_cpu(cpu) {
		struct cpu_topology *cpu_topo = &cpu_topology[cpu];

		if (cpu_topo->cluster_id == cluster_id)
			cpumask_set_cpu(cpu, cpus);
	}
}

int arch_better_capacity(unsigned int cpu)
{
	return cpu_capacity(cpu) > min_cpu_perf;
}

#ifdef CONFIG_SCHED_HMP
void __init arch_get_hmp_domains(struct list_head *hmp_domains_list)
{
	struct hmp_domain *domain;
	struct cpumask cpu_mask;
	int id, maxid;

	cpumask_clear(&cpu_mask);
	maxid = arch_get_nr_clusters();

	/*
	 * Initialize hmp_domains
	 * Must be ordered with respect to compute capacity.
	 * Fastest domain at head of list.
	 */
	for (id = 0; id < maxid; id++) {
		arch_get_cluster_cpus(&cpu_mask, id);
		domain = (struct hmp_domain *)
			kmalloc(sizeof(struct hmp_domain), GFP_KERNEL);
		cpumask_copy(&domain->possible_cpus, &cpu_mask);
		cpumask_and(&domain->cpus, cpu_online_mask, &domain->possible_cpus);
		list_add(&domain->hmp_domains, hmp_domains_list);
	}
}
#else
void __init arch_get_hmp_domains(struct list_head *hmp_domains_list) {}
#endif /* CONFIG_SCHED_HMP */
