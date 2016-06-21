#include <linux/kernel.h>
#include "mt-plat/mt_hotplug_strategy.h"
#include "gl_typedef.h"

INT32 kalBoostCpu(UINT_32 core_num)
{
	UINT_32 cpu_num;

	if (core_num != 0)
		core_num += 2; /* For denali only, additional 2 cores for HT20 peak throughput*/
	if (core_num > 4)
		cpu_num = 4; /* There are only 4 cores for denali */
	pr_warn("enter kalBoostCpu, core_num:%d\n", core_num);

	hps_set_cpu_num_base(BASE_WIFI, core_num, 0);

	return 0;
}
