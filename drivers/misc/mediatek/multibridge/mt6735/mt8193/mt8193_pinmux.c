#if defined(CONFIG_MTK_MULTIBRIDGE_SUPPORT)

#define pr_fmt(fmt) "mt8193-pinmux: " fmt

#include <generated/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>

#include "mt8193_pinmux.h"
#include "mt8193.h"


/* select function according to pinmux table */
int mt8193_pinset(u32 pin_num, u32 function)
{
	u32 pin_mux_val = 0;
	u32 pin_mux_reg = 0;
	u32 pin_mux_mask = 0;
	u32 pin_mux_shift = 0;

	if (pin_num >= PIN_MAX) {
		pr_err("[PINMUX] INVALID PINMUX!!  %d\n", pin_num);
		return -1;
	}
	if (function >= PINMUX_FUNCTION_MAX) {
		pr_err("[PINMUX] INVALID FUNCTION NUM!!  %d\n", function);
		return -1;
	}

	pr_debug("[PINMUX] mt8193_pinset()  %d,  %d\n", pin_num, function);

	pin_mux_reg = _au4PinmuxFuncTbl[pin_num][0]; /* pinmux register */
	pin_mux_shift = _au4PinmuxFuncTbl[pin_num][1];  /*pinmux left shift */
	pin_mux_mask = _au4PinmuxFuncTbl[pin_num][2];  /* pinmux mask */

	pin_mux_val = CKGEN_READ32(pin_mux_reg);

	/* clear function first*/
	pin_mux_val &= (~pin_mux_mask);
	/* set function */
	pin_mux_val |= ((function << pin_mux_shift) & pin_mux_mask);
	CKGEN_WRITE32(pin_mux_reg, pin_mux_val);

	return PINMUX_RET_OK;
}

#endif





