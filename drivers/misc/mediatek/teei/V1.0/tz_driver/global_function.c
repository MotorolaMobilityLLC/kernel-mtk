#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>

#include "nt_smc_call.h"
#include "global_function.h"
#include "sched_status.h"
#include "utdriver_macro.h"
#include "switch_queue.h"
#include <imsg_log.h>

static void secondary_nt_sched_t(void *info)
{
	unsigned long smc_type = 2;

	nt_sched_t((uint64_t *)(&smc_type));
	while (smc_type == 0x54) {
		nt_sched_t((uint64_t *)(&smc_type));
	}
}
void nt_sched_t_call(void)
{
	int retVal = 0;
	retVal = add_work_entry(SCHED_CALL, NULL);

	if (retVal != 0) {
		IMSG_ERROR("[%s][%d] add_work_entry function failed!\n", __func__, __LINE__);
	}
}
