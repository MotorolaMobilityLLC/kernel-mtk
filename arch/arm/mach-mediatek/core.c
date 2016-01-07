
#include <linux/pm.h>
#include <linux/bug.h>
#include <linux/memblock.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/smp_scu.h>
#include <asm/page.h>
#include <linux/irqchip.h>
#include <linux/of_platform.h>

#ifdef CONFIG_ARCH_MT6580
#include <mt-smp.h>
#endif

#ifdef CONFIG_OF
static const char *mt6580_dt_match[] __initconst = {
	"mediatek,MT6580",
	NULL
};

DT_MACHINE_START(MT6580_DT, "MT6580")
	.dt_compat	= mt6580_dt_match,
MACHINE_END

static const char *mt_dt_match[] __initconst = {
	"mediatek,mt6735",
	NULL
};

DT_MACHINE_START(MT6735_DT, "MT6735")
	.dt_compat	= mt_dt_match,
MACHINE_END

static const char *mt6755_dt_match[] __initconst = {
	"mediatek,MT6755",
	NULL
};
DT_MACHINE_START(MT6755_DT, "MT6755")
	.dt_compat	= mt6755_dt_match,
MACHINE_END

static const char *mt8127_dt_match[] __initconst = {
	"mediatek,mt8127",
	NULL
};

DT_MACHINE_START(MT8127_DT, "MT8127")
	.dt_compat	= mt8127_dt_match,
MACHINE_END

#endif
