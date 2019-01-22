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
#ifdef CONFIG_CPU_FREQ
	unsigned long max_freq_scale = cpufreq_scale_max_freq_capacity(cpu);

	return per_cpu(cpu_scale, cpu) * max_freq_scale >> SCHED_CAPACITY_SHIFT;
#else
	return per_cpu(cpu_scale, cpu);
#endif
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
	{ "arm,cortex-a73", 3630 },
	{ "arm,cortex-a72", 4186 },
	{ "arm,cortex-a57", 3891 },
	{ "arm,cortex-a53", 2048 },
	{ "arm,cortex-a35", 1661 },
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
static DEFINE_PER_CPU(atomic_long_t, cpu_min_freq);

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

void arch_scale_set_min_freq(int cpu, unsigned long freq)
{
	atomic_long_set(&per_cpu(cpu_min_freq, cpu), freq);
}

unsigned long arch_scale_get_max_freq(int cpu)
{
	unsigned long max = atomic_long_read(&per_cpu(cpu_max_freq, cpu));

	return max;
}

unsigned long arch_scale_get_min_freq(int cpu)
{
	unsigned long min = atomic_long_read(&per_cpu(cpu_min_freq, cpu));

	return min;
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
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
};

static struct idle_state idle_states_cluster_2[] = {
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
};

#ifndef CONFIG_MTK_UNIFY_POWER
static struct capacity_state cap_states_cluster_0[] = {
	/* Power per cpu */
	{ .cap =   80, .dyn_pwr =   13, .lkg_pwr[0] =  0, .volt = 56}, /*  [0] 221 MHz */
	{ .cap =  108, .dyn_pwr =   18, .lkg_pwr[0] =  0, .volt = 56}, /*  [1] 338 MHz */
	{ .cap =  129, .dyn_pwr =   22, .lkg_pwr[0] =  0, .volt = 58}, /*  [2] 442 MHz */
	{ .cap =  151, .dyn_pwr =   29, .lkg_pwr[0] =  0, .volt = 61}, /*  [3] 559 MHz */
	{ .cap =  172, .dyn_pwr =   36, .lkg_pwr[0] =  0, .volt = 64}, /*  [4] 676 Mhz */
	{ .cap =  194, .dyn_pwr =   44, .lkg_pwr[0] =  0, .volt = 67}, /*  [5] 806 Mhz */
	{ .cap =  215, .dyn_pwr =   54, .lkg_pwr[0] =  0, .volt = 70}, /*  [6] 949 Mhz */
	{ .cap =  231, .dyn_pwr =   61, .lkg_pwr[0] =  0, .volt = 72}, /*  [7] 1.066 Ghz */
	{ .cap =  249, .dyn_pwr =   72, .lkg_pwr[0] =  0, .volt = 75}, /*  [8] 1.183 Ghz */
	{ .cap =  265, .dyn_pwr =   81, .lkg_pwr[0] =  0, .volt = 77}, /*  [9] 1.235 Ghz */
	{ .cap =  283, .dyn_pwr =   92, .lkg_pwr[0] =  0, .volt = 79}, /*  [10] 1.300 Ghz */
	{ .cap =  298, .dyn_pwr =  102, .lkg_pwr[0] =  0, .volt = 81}, /*  [11] 1.378 Ghz */
	{ .cap =  314, .dyn_pwr =  114, .lkg_pwr[0] =  0, .volt = 83}, /*  [12] 1.443 Ghz */
	{ .cap =  329, .dyn_pwr =  126, .lkg_pwr[0] =  0, .volt = 86}, /*  [13] 1.508 Ghz */
	{ .cap =  341, .dyn_pwr =  136, .lkg_pwr[0] =  0, .volt = 87}, /*  [14] 1.560 Ghz */
	{ .cap =  357, .dyn_pwr =  149, .lkg_pwr[0] =  0, .volt = 90}, /*  [15] 1.638 Ghz */
};

static struct capacity_state cap_states_cluster_1[] = {
	/* Power per cpu */
	{ .cap =   96, .dyn_pwr =   27, .lkg_pwr[0] =  0, .volt = 56}, /*  [0] 442 MHz */
	{ .cap =  133, .dyn_pwr =   37, .lkg_pwr[0] =  0, .volt = 56}, /*  [1] 533 MHz */
	{ .cap =  159, .dyn_pwr =   46, .lkg_pwr[0] =  0, .volt = 58}, /*  [2] 741 MHz */
	{ .cap =  189, .dyn_pwr =   60, .lkg_pwr[0] =  0, .volt = 61}, /*  [3] 897 MHz */
	{ .cap =  215, .dyn_pwr =   74, .lkg_pwr[0] =  0, .volt = 64}, /*  [4] 1.040 Mhz */
	{ .cap =  245, .dyn_pwr =   92, .lkg_pwr[0] =  0, .volt = 67}, /*  [5] 1.248 Mhz */
	{ .cap =  271, .dyn_pwr =  111, .lkg_pwr[0] =  0, .volt = 70}, /*  [6] 1.456 Mhz */
	{ .cap =  297, .dyn_pwr =  130, .lkg_pwr[0] =  0, .volt = 72}, /*  [7] 1.638 Ghz */
	{ .cap =  326, .dyn_pwr =  154, .lkg_pwr[0] =  0, .volt = 75}, /*  [8] 1.794 Ghz */
	{ .cap =  348, .dyn_pwr =  175, .lkg_pwr[0] =  0, .volt = 77}, /*  [9] 1.872 Ghz */
	{ .cap =  374, .dyn_pwr =  199, .lkg_pwr[0] =  0, .volt = 79}, /*  [10] 1.963 Ghz */
	{ .cap =  397, .dyn_pwr =  223, .lkg_pwr[0] =  0, .volt = 81}, /*  [11] 2.067 Ghz */
	{ .cap =  419, .dyn_pwr =  249, .lkg_pwr[0] =  0, .volt = 83}, /*  [12] 2.132 Ghz */
	{ .cap =  445, .dyn_pwr =  279, .lkg_pwr[0] =  0, .volt = 86}, /*  [13] 2.197 Ghz */
	{ .cap =  463, .dyn_pwr =  302, .lkg_pwr[0] =  0, .volt = 87}, /*  [14] 2.262 Ghz */
	{ .cap =  486, .dyn_pwr =  333, .lkg_pwr[0] =  0, .volt = 90}, /*  [15] 2.340 Ghz */
};

static struct capacity_state cap_states_cluster_2[] = {
	/* Power per cpu */
	{ .cap =  175, .dyn_pwr =   32, .lkg_pwr[0] =  0, .volt = 56},  /*  [0] 442 MHz */
	{ .cap =  243, .dyn_pwr =   44, .lkg_pwr[0] =  0, .volt = 56}, /*  [1] 533 MHz */
	{ .cap =  296, .dyn_pwr =   56, .lkg_pwr[0] =  0, .volt = 58}, /*  [2] 741 MHz */
	{ .cap =  344, .dyn_pwr =   71, .lkg_pwr[0] =  0, .volt = 61}, /*  [3] 897 MHz */
	{ .cap =  397, .dyn_pwr =   90, .lkg_pwr[0] =  0, .volt = 64}, /*  [4] 1.040 Mhz */
	{ .cap =  445, .dyn_pwr =  110, .lkg_pwr[0] =  0, .volt = 67}, /*  [5] 1.248 Mhz */
	{ .cap =  499, .dyn_pwr =  134, .lkg_pwr[0] =  0, .volt = 70}, /*  [6] 1.456 Mhz */
	{ .cap =  559, .dyn_pwr =  161, .lkg_pwr[0] =  0, .volt = 72}, /*  [7] 1.638 Ghz */
	{ .cap =  633, .dyn_pwr =  197, .lkg_pwr[0] =  0, .volt = 75}, /*  [8] 1.794 Ghz */
	{ .cap =  694, .dyn_pwr =  229, .lkg_pwr[0] =  0, .volt = 77}, /*  [9] 1.872 Ghz */
	{ .cap =  748, .dyn_pwr =  261, .lkg_pwr[0] =  0, .volt = 79}, /*  [10] 1.963 Ghz */
	{ .cap =  808, .dyn_pwr =  299, .lkg_pwr[0] =  0, .volt = 81}, /*  [11] 2.067 Ghz */
	{ .cap =  869, .dyn_pwr =  339, .lkg_pwr[0] =  0, .volt = 83}, /*  [12] 2.132 Ghz */
	{ .cap =  923, .dyn_pwr =  380, .lkg_pwr[0] =  0, .volt = 86}, /*  [13] 2.197 Ghz */
	{ .cap =  970, .dyn_pwr =  415, .lkg_pwr[0] =  0, .volt = 87}, /*  [14] 2.262 Ghz */
	{ .cap = 1024, .dyn_pwr =  461, .lkg_pwr[0] =  0, .volt = 90}, /*  [15] 2.340 Ghz */
};
#endif

static struct sched_group_energy energy_cluster_0 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_cluster_0),
	.idle_states    = idle_states_cluster_0,
#ifndef CONFIG_MTK_UNIFY_POWER
	.nr_cap_states  = ARRAY_SIZE(cap_states_cluster_0),
	.cap_states     = cap_states_cluster_0,
	.lkg_idx	= 0,
#endif
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

static struct sched_group_energy energy_cluster_1 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_cluster_1),
	.idle_states    = idle_states_cluster_1,
#ifndef CONFIG_MTK_UNIFY_POWER
	.nr_cap_states  = ARRAY_SIZE(cap_states_cluster_1),
	.cap_states     = cap_states_cluster_1,
	.lkg_idx	= 0,
#endif
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

static struct sched_group_energy energy_cluster_2 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_cluster_2),
	.idle_states    = idle_states_cluster_2,
#ifndef CONFIG_MTK_UNIFY_POWER
	.nr_cap_states  = ARRAY_SIZE(cap_states_cluster_2),
	.cap_states     = cap_states_cluster_2,
	.lkg_idx	= 0,
#endif
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
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
};

static struct idle_state idle_states_core_2[] = {
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
	{ .power = 0 },
};

#ifndef CONFIG_MTK_UNIFY_POWER
static struct capacity_state cap_states_core_0[] = {
	/* Power per cpu */
	{ .cap =   80, .dyn_pwr =   51, .lkg_pwr[0] =  0, .volt = 56}, /*  [0] 221 MHz */
	{ .cap =  108, .dyn_pwr =   69, .lkg_pwr[0] =  0, .volt = 56}, /*  [1] 338 MHz */
	{ .cap =  129, .dyn_pwr =   85, .lkg_pwr[0] =  0, .volt = 58}, /*  [2] 442 MHz */
	{ .cap =  151, .dyn_pwr =  110, .lkg_pwr[0] =  0, .volt = 61}, /*  [3] 559 MHz */
	{ .cap =  172, .dyn_pwr =  137, .lkg_pwr[0] =  0, .volt = 64}, /*  [4] 676 Mhz */
	{ .cap =  194, .dyn_pwr =  168, .lkg_pwr[0] =  0, .volt = 67}, /*  [5] 806 Mhz */
	{ .cap =  215, .dyn_pwr =  203, .lkg_pwr[0] =  0, .volt = 70}, /*  [6] 949 Mhz */
	{ .cap =  231, .dyn_pwr =  232, .lkg_pwr[0] =  0, .volt = 72}, /*  [7] 1.066 Ghz */
	{ .cap =  249, .dyn_pwr =  271, .lkg_pwr[0] =  0, .volt = 75}, /*  [8] 1.183 Ghz */
	{ .cap =  265, .dyn_pwr =  305, .lkg_pwr[0] =  0, .volt = 77}, /*  [9] 1.235 Ghz */
	{ .cap =  283, .dyn_pwr =  346, .lkg_pwr[0] =  0, .volt = 79}, /*  [10] 1.300 Ghz */
	{ .cap =  298, .dyn_pwr =  386, .lkg_pwr[0] =  0, .volt = 81}, /*  [11] 1.378 Ghz */
	{ .cap =  314, .dyn_pwr =  428, .lkg_pwr[0] =  0, .volt = 83}, /*  [12] 1.443 Ghz */
	{ .cap =  329, .dyn_pwr =  474, .lkg_pwr[0] =  0, .volt = 86}, /*  [13] 1.508 Ghz */
	{ .cap =  341, .dyn_pwr =  511, .lkg_pwr[0] =  0, .volt = 87}, /*  [14] 1.560 Ghz */
	{ .cap =  357, .dyn_pwr =  562, .lkg_pwr[0] =  0, .volt = 90}, /*  [15] 1.638 Ghz */
};

static struct capacity_state cap_states_core_1[] = {
	/* Power per cpu */
	{ .cap =   96, .dyn_pwr =   99, .lkg_pwr[0] =  0, .volt = 56}, /*  [0] 442 MHz */
	{ .cap =  133, .dyn_pwr =  139, .lkg_pwr[0] =  0, .volt = 56}, /*  [1] 533 MHz */
	{ .cap =  159, .dyn_pwr =  169, .lkg_pwr[0] =  0, .volt = 58}, /*  [2] 741 MHz */
	{ .cap =  189, .dyn_pwr =  220, .lkg_pwr[0] =  0, .volt = 61}, /*  [3] 897 MHz */
	{ .cap =  215, .dyn_pwr =  274, .lkg_pwr[0] =  0, .volt = 64}, /*  [4] 1.040 Mhz */
	{ .cap =  245, .dyn_pwr =  341, .lkg_pwr[0] =  0, .volt = 67}, /*  [5] 1.248 Mhz */
	{ .cap =  271, .dyn_pwr =  409, .lkg_pwr[0] =  0, .volt = 70}, /*  [6] 1.456 Mhz */
	{ .cap =  297, .dyn_pwr =  478, .lkg_pwr[0] =  0, .volt = 72}, /*  [7] 1.638 Ghz */
	{ .cap =  326, .dyn_pwr =  568, .lkg_pwr[0] =  0, .volt = 75}, /*  [8] 1.794 Ghz */
	{ .cap =  348, .dyn_pwr =  644, .lkg_pwr[0] =  0, .volt = 77}, /*  [9] 1.872 Ghz */
	{ .cap =  374, .dyn_pwr =  733, .lkg_pwr[0] =  0, .volt = 79}, /*  [10] 1.963 Ghz */
	{ .cap =  397, .dyn_pwr =  821, .lkg_pwr[0] =  0, .volt = 81}, /*  [11] 2.067 Ghz */
	{ .cap =  419, .dyn_pwr =  916, .lkg_pwr[0] =  0, .volt = 83}, /*  [12] 2.132 Ghz */
	{ .cap =  445, .dyn_pwr = 1026, .lkg_pwr[0] =  0, .volt = 86}, /*  [13] 2.197 Ghz */
	{ .cap =  463, .dyn_pwr = 1111, .lkg_pwr[0] =  0, .volt = 87}, /*  [14] 2.262 Ghz */
	{ .cap =  486, .dyn_pwr = 1225, .lkg_pwr[0] =  0, .volt = 90}, /*  [15] 2.340 Ghz */
};

static struct capacity_state cap_states_core_2[] = {
	/* Power per cpu */
	{ .cap =  175, .dyn_pwr =  313, .lkg_pwr[0] =  0, .volt = 56},  /*  [0] 442 MHz */
	{ .cap =  243, .dyn_pwr =  433, .lkg_pwr[0] =  0, .volt = 56}, /*  [1] 533 MHz */
	{ .cap =  296, .dyn_pwr =  544, .lkg_pwr[0] =  0, .volt = 58}, /*  [2] 741 MHz */
	{ .cap =  344, .dyn_pwr =  693, .lkg_pwr[0] =  0, .volt = 61}, /*  [3] 897 MHz */
	{ .cap =  397, .dyn_pwr =  878, .lkg_pwr[0] =  0, .volt = 64}, /*  [4] 1.040 Mhz */
	{ .cap =  445, .dyn_pwr = 1070, .lkg_pwr[0] =  0, .volt = 67}, /*  [5] 1.248 Mhz */
	{ .cap =  499, .dyn_pwr = 1303, .lkg_pwr[0] =  0, .volt = 70}, /*  [6] 1.456 Mhz */
	{ .cap =  559, .dyn_pwr = 1558, .lkg_pwr[0] =  0, .volt = 72}, /*  [7] 1.638 Ghz */
	{ .cap =  633, .dyn_pwr = 1905, .lkg_pwr[0] =  0, .volt = 75}, /*  [8] 1.794 Ghz */
	{ .cap =  694, .dyn_pwr = 2215, .lkg_pwr[0] =  0, .volt = 77}, /*  [9] 1.872 Ghz */
	{ .cap =  748, .dyn_pwr = 2529, .lkg_pwr[0] =  0, .volt = 79}, /*  [10] 1.963 Ghz */
	{ .cap =  808, .dyn_pwr = 2892, .lkg_pwr[0] =  0, .volt = 81}, /*  [11] 2.067 Ghz */
	{ .cap =  869, .dyn_pwr = 3283, .lkg_pwr[0] =  0, .volt = 83}, /*  [12] 2.132 Ghz */
	{ .cap =  923, .dyn_pwr = 3677, .lkg_pwr[0] =  0, .volt = 86}, /*  [13] 2.197 Ghz */
	{ .cap =  970, .dyn_pwr = 4018, .lkg_pwr[0] =  0, .volt = 87}, /*  [14] 2.262 Ghz */
	{ .cap = 1024, .dyn_pwr = 4462, .lkg_pwr[0] =  0, .volt = 90}, /*  [15] 2.340 Ghz */
};
#endif

static struct sched_group_energy energy_core_0 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_core_0),
	.idle_states    = idle_states_core_0,
#ifndef CONFIG_MTK_UNIFY_POWER
	.nr_cap_states  = ARRAY_SIZE(cap_states_core_0),
	.cap_states     = cap_states_core_0,
	.lkg_idx	= 0,
#endif
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

static struct sched_group_energy energy_core_1 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_core_1),
	.idle_states    = idle_states_core_1,
#ifndef CONFIG_MTK_UNIFY_POWER
	.nr_cap_states  = ARRAY_SIZE(cap_states_core_1),
	.cap_states     = cap_states_core_1,
	.lkg_idx	= 0,
#endif
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

static struct sched_group_energy energy_core_2 = {
	.nr_idle_states = ARRAY_SIZE(idle_states_core_2),
	.idle_states    = idle_states_core_2,
#ifndef CONFIG_MTK_UNIFY_POWER
	.nr_cap_states  = ARRAY_SIZE(cap_states_core_2),
	.cap_states     = cap_states_core_2,
	.lkg_idx	= 0,
#endif
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	.idle_power     = mtk_idle_power,
	.busy_power     = mtk_busy_power,
#endif
};

/* sd energy functions */
inline
const struct sched_group_energy *cpu_cluster_energy(int cpu)
{
	int cluster_id = cpu_topology[cpu].cluster_id;
	struct sched_group_energy *cpu_cluster_ptr;
#ifdef CONFIG_MTK_UNIFY_POWER
	struct upower_tbl_info **addr_ptr_tbl_info;
	struct upower_tbl_info *ptr_tbl_info;
	struct upower_tbl *ptr_tbl;
#endif

	if (cluster_id == 0)
		cpu_cluster_ptr = &energy_cluster_0;
	else if (cluster_id == 1)
		cpu_cluster_ptr = &energy_cluster_1;
	else if (cluster_id == 2)
		cpu_cluster_ptr = &energy_cluster_2;
	else
		return NULL;

#ifdef CONFIG_MTK_UNIFY_POWER
	addr_ptr_tbl_info = upower_get_tbl();
	ptr_tbl_info = *addr_ptr_tbl_info;

	ptr_tbl = ptr_tbl_info[UPOWER_BANK_CLS_BASE+cluster_id].p_upower_tbl;

	cpu_cluster_ptr->nr_cap_states = ptr_tbl->row_num;
	cpu_cluster_ptr->cap_states = ptr_tbl->row;
	cpu_cluster_ptr->lkg_idx = ptr_tbl->lkg_idx;
#endif

	return cpu_cluster_ptr;
}

inline
const struct sched_group_energy *cpu_core_energy(int cpu)
{
	int cluster_id = cpu_topology[cpu].cluster_id;
	struct sched_group_energy *cpu_core_ptr;
#ifdef CONFIG_MTK_UNIFY_POWER
	struct upower_tbl *ptr_tbl;
#endif

	if (cluster_id == 0)
		cpu_core_ptr = &energy_core_0;
	else if (cluster_id == 1)
		cpu_core_ptr = &energy_core_1;
	else if (cluster_id == 2)
		cpu_core_ptr = &energy_core_2;
	else
		return NULL;

#ifdef CONFIG_MTK_UNIFY_POWER
	ptr_tbl = upower_get_core_tbl(cpu);

	cpu_core_ptr->nr_cap_states = ptr_tbl->row_num;
	cpu_core_ptr->cap_states = ptr_tbl->row;
	cpu_core_ptr->lkg_idx = ptr_tbl->lkg_idx;
#endif

	return cpu_core_ptr;
}

const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return &cpu_topology[cpu].core_sibling;
}

static int cpu_cpu_flags(void)
{
	return SD_ASYM_CPUCAPACITY;
}

static inline int cpu_corepower_flags(void)
{
	return SD_SHARE_PKG_RESOURCES  | SD_SHARE_POWERDOMAIN | SD_SHARE_CAP_STATES;
}

static struct sched_domain_topology_level arm64_topology[] = {
#ifdef CONFIG_SCHED_MC
	{ cpu_coregroup_mask, cpu_corepower_flags, cpu_core_energy, SD_INIT_NAME(MC) },
#endif
	{ cpu_cpu_mask, cpu_cpu_flags, cpu_cluster_energy, SD_INIT_NAME(DIE) },
	{ NULL, },
};

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
}


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
		if (domain) {
			cpumask_copy(&domain->possible_cpus, &cpu_mask);
			cpumask_and(&domain->cpus, cpu_online_mask, &domain->possible_cpus);
			list_add(&domain->hmp_domains, hmp_domains_list);
		}
	}
}
#else
void __init arch_get_hmp_domains(struct list_head *hmp_domains_list) {}
#endif /* CONFIG_SCHED_HMP */


#ifdef CONFIG_MTK_UNIFY_POWER
static int
update_all_cpu_capacity(void)
{
	int cpu;

	for (cpu = 0; cpu < nr_cpu_ids ; cpu++)
		update_cpu_capacity(cpu);

	return 0;
}

late_initcall_sync(update_all_cpu_capacity)
#endif
