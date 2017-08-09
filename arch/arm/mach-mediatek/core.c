
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
#include <mach/irqs.h>
#include <linux/irqchip.h>
#include <linux/of_platform.h>

#ifdef CONFIG_OF
static const char *mt_dt_match[] __initconst = {
	"mediatek,mt6735",
	NULL
};

DT_MACHINE_START(MT6735_DT, "MT6735")
	.dt_compat = mt_dt_match,
MACHINE_END
#endif
