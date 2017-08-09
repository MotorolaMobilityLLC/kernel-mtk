#include <linux/kconfig.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <mrdump.h>
#include <asm/memory.h>
#include "mrdump_private.h"

static void mrdump_hw_enable(bool enabled)
{
}

static void mrdump_reboot(void)
{
	emergency_restart();
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
