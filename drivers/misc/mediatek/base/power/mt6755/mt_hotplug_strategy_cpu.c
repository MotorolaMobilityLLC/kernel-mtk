/**
* @file    mt_hotplug_strategy_cpu.c
* @brief   hotplug strategy(hps) - cpu
*/

/*============================================================================*/
/* Include files */
/*============================================================================*/
/* system includes */
#include <linux/kernel.h>	/* printk */
#include <linux/module.h>	/* MODULE_DESCRIPTION, MODULE_LICENSE */
#include <linux/init.h>		/* module_init, module_exit */
#include <linux/sched.h>	/* sched_get_percpu_load, sched_get_nr_heavy_task */

/* local includes */
#include "mt_hotplug_strategy_internal.h"

/*============================================================================*/
/* Macro definition */
/*============================================================================*/
/*
 * static
 */
#define STATIC
/* #define STATIC static */

/*
 * config
 */

/*============================================================================*/
/* Local type definition */
/*============================================================================*/

/*============================================================================*/
/* Local function declarition */
/*============================================================================*/

/*============================================================================*/
/* Local variable definition */
/*============================================================================*/

/*============================================================================*/
/* Global variable definition */
/*============================================================================*/

/*============================================================================*/
/* Local function definition */
/*============================================================================*/

/*============================================================================*/
/* Gobal function definition */
/*============================================================================*/
/*
 * hps cpu interface - cpumask
 */
int hps_cpu_is_cpu_big(int cpu)
{
	if (!cpumask_empty(&hps_ctxt.big_cpumask)) {
		if (cpumask_test_cpu(cpu, &hps_ctxt.big_cpumask))
			return 1;
		else
			return 0;
	} else {
		return 0;
	}
}

int hps_cpu_is_cpu_little(int cpu)
{
	if (!cpumask_empty(&hps_ctxt.little_cpumask)) {
		if (cpumask_test_cpu(cpu, &hps_ctxt.little_cpumask))
			return 1;
		else
			return 0;
	} else {
		return 0;
	}
}

unsigned int num_online_little_cpus(void)
{
	struct cpumask dst_cpumask;

	cpumask_and(&dst_cpumask, &hps_ctxt.little_cpumask, cpu_online_mask);
	return cpumask_weight(&dst_cpumask);
}

unsigned int num_online_big_cpus(void)
{
	struct cpumask dst_cpumask;

	cpumask_and(&dst_cpumask, &hps_ctxt.big_cpumask, cpu_online_mask);
	return cpumask_weight(&dst_cpumask);
}

/* int hps_cpu_get_arch_type(void) */
/* { */
/* if(!cluster_numbers) */
/* return ARCH_TYPE_NO_CLUSTER; */
/* if(cpumask_empty(&hps_ctxt.little_cpumask) && cpumask_empty(&hps_ctxt.big_cpumask) ) */
/* return ARCH_TYPE_NOT_READY; */
/* if(!cpumask_empty(&hps_ctxt.little_cpumask) && !cpumask_empty(&hps_ctxt.big_cpumask)) */
/* return ARCH_TYPE_big_LITTLE; */
/* if(!cpumask_empty(&hps_ctxt.little_cpumask) && cpumask_empty(&hps_ctxt.big_cpumask)) */
/* return ARCH_TYPE_LITTLE_LITTLE; */
/* return ARCH_TYPE_NOT_READY; */
/* } */

/*
 * hps cpu interface - scheduler
 */
unsigned int hps_cpu_get_percpu_load(int cpu)
{
#ifdef CONFIG_MTK_SCHED_RQAVG_US
	return sched_get_percpu_load(cpu, 1, 0);
#else
	return 100;
#endif
}

unsigned int hps_cpu_get_nr_heavy_task(void)
{
#ifdef CONFIG_MTK_SCHED_RQAVG_US
	return sched_get_nr_heavy_task();
#else
	return 0;
#endif
}

void hps_cpu_get_tlp(unsigned int *avg, unsigned int *iowait_avg)
{
#ifdef CONFIG_MTK_SCHED_RQAVG_KS
	sched_get_nr_running_avg((int *)avg, (int *)iowait_avg);
#else
	*avg = 0;
	*iowait_avg = 0;
#endif
}


/*
 * init
 */
int hps_cpu_init(void)
{
	int r = 0;
	char str1[32];

	hps_warn("hps_cpu_init\n");

	/* init cpu arch in hps_ctxt */
	/* init cpumask */
	cpumask_clear(&hps_ctxt.little_cpumask);
	cpumask_clear(&hps_ctxt.big_cpumask);

	/* a. call api */
	arch_get_cluster_cpus(&hps_ctxt.little_cpumask, 0);
	arch_get_cluster_cpus(&hps_ctxt.big_cpumask, 1);
	/* b. fix 2L2b */
	/* cpulist_parse("0-1", &hps_ctxt.little_cpumask); */
	/* cpulist_parse("2-3", &hps_ctxt.big_cpumask); */
	/* c. 4L */
	/* cpulist_parse("0-3", &hps_ctxt.little_cpumask); */

	cpulist_parse("0-3", &hps_ctxt.little_cpumask);
	cpulist_parse("4-7", &hps_ctxt.big_cpumask);
	if (cpumask_weight(&hps_ctxt.little_cpumask) == 0) {
		cpumask_copy(&hps_ctxt.little_cpumask, &hps_ctxt.big_cpumask);
		cpumask_clear(&hps_ctxt.big_cpumask);
	}
	/* for(i=0;i<4;i++) */
	/* cpumask_set_cpu(i,&hps_ctxt.little_cpumask); */
	/* for(i=4;i<8;i++); */
	/* cpumask_set_cpu(i,&hps_ctxt.big_cpumask); */
	cpulist_scnprintf(str1, sizeof(str1), &hps_ctxt.little_cpumask);
	hps_warn("hps_ctxt.little_cpumask: %s\n", str1);
	cpulist_scnprintf(str1, sizeof(str1), &hps_ctxt.big_cpumask);
	hps_warn("hps_ctxt.big_cpumask: %s\n", str1);

	/* verify arch is hmp or amp or smp */
	if (!cpumask_empty(&hps_ctxt.little_cpumask) && !cpumask_empty(&hps_ctxt.big_cpumask)) {
		unsigned int cpu;

		hps_ctxt.is_amp = 1;

		for_each_cpu((cpu), &hps_ctxt.little_cpumask) {
			if (cpu < hps_ctxt.little_cpu_id_min)
				hps_ctxt.little_cpu_id_min = cpu;
			if (cpu > hps_ctxt.little_cpu_id_max)
				hps_ctxt.little_cpu_id_max = cpu;
		}
		for_each_cpu((cpu), &hps_ctxt.big_cpumask) {
			if (cpu < hps_ctxt.big_cpu_id_min)
				hps_ctxt.big_cpu_id_min = cpu;
			if (cpu > hps_ctxt.big_cpu_id_max)
				hps_ctxt.big_cpu_id_max = cpu;
		}
	} else {
		hps_ctxt.is_hmp = 0;
		hps_ctxt.is_amp = 0;
		hps_ctxt.little_cpu_id_min = 0;
		hps_ctxt.little_cpu_id_max = num_possible_little_cpus() - 1;
	}

	/* init bound in hps_ctxt */
	hps_ctxt.little_num_base_perf_serv = 1;
	hps_ctxt.little_num_limit_thermal = cpumask_weight(&hps_ctxt.little_cpumask);
	hps_ctxt.little_num_limit_low_battery = cpumask_weight(&hps_ctxt.little_cpumask);
	hps_ctxt.little_num_limit_ultra_power_saving = cpumask_weight(&hps_ctxt.little_cpumask);
	hps_ctxt.little_num_limit_power_serv = cpumask_weight(&hps_ctxt.little_cpumask);
	hps_ctxt.big_num_base_perf_serv = 0;
	hps_ctxt.big_num_limit_thermal = cpumask_weight(&hps_ctxt.big_cpumask);
	hps_ctxt.big_num_limit_low_battery = cpumask_weight(&hps_ctxt.big_cpumask);
	hps_ctxt.big_num_limit_ultra_power_saving = cpumask_weight(&hps_ctxt.big_cpumask);
	hps_ctxt.big_num_limit_power_serv = cpumask_weight(&hps_ctxt.big_cpumask);

	hps_warn
	    ("%s: little_cpu_id_min: %u, little_cpu_id_max: %u, big_cpu_id_min: %u, big_cpu_id_max: %u\n",
	     __func__, hps_ctxt.little_cpu_id_min, hps_ctxt.little_cpu_id_max,
	     hps_ctxt.big_cpu_id_min, hps_ctxt.big_cpu_id_max);

	return r;
}

/*
 * deinit
 */
int hps_cpu_deinit(void)
{
	int r = 0;

	hps_warn("hps_cpu_deinit\n");

	return r;
}
