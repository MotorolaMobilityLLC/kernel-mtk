#ifndef __MT_UNCOMPRESS_H
#define __MT_UNCOMPRESS_H

#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/page.h>

static void arch_decomp_setup(void)
{

}

static inline void putc(int c)
{

}

static inline void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_wdog()

#endif
