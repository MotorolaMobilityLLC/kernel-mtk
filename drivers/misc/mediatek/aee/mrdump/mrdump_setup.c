#include <linux/kconfig.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <mrdump.h>
#include <asm/memory.h>
#include <mach/wd_api.h>
#include "mrdump_private.h"

static void mrdump_hw_enable(bool enabled)
{
	struct wd_api *wd_api = NULL;

	get_wd_api(&wd_api);
	if (wd_api)
		wd_api->wd_dram_reserved_mode(enabled);
}

static void mrdump_reboot(void)
{
	int res;
	struct wd_api *wd_api = NULL;

	res = get_wd_api(&wd_api);
	if (res) {
		pr_alert("arch_reset, get wd api error %d\n", res);
		while (1)
			cpu_relax();
	} else {
		wd_api->wd_sw_reset(0);
	}
}

const struct mrdump_platform mrdump_v1_platform = {
	.hw_enable = mrdump_hw_enable,
	.reboot = mrdump_reboot
};

static int __init mrdump_init(void)
{
	return mrdump_platform_init(&mrdump_v1_platform);
}

module_init(mrdump_init);
