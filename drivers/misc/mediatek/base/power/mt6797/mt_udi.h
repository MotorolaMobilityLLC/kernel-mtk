#ifndef _MT_UDI_H
#define _MT_UDI_H

#include <linux/kernel.h>
#include "mach/mt_secure_api.h"

#ifdef	__MT_UDI_C__
#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define mt_secure_call_udi	mt_secure_call
#else
/* This is workaround for idvfs use */
static noinline int mt_secure_call_udi(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
	register u64 reg0 __asm__("x0") = function_id;
	register u64 reg1 __asm__("x1") = arg0;
	register u64 reg2 __asm__("x2") = arg1;
	register u64 reg3 __asm__("x3") = arg2;
	int ret = 0;

	asm volatile ("smc    #0\n" : "+r" (reg0) :
		"r"(reg1), "r"(reg2), "r"(reg3));

	ret = (int)reg0;
	return ret;
}
#endif
#endif

/* define for UDI service */
#ifdef CONFIG_ARM64
#define MTK_SIP_KERNEL_UDI_WRITE	0xC200035E /* OCP_WRITE */
#define MTK_SIP_KERNEL_UDI_READ		0xC200035F /* OCP_READ */
#define MTK_SIP_KERNEL_UDI_JTAG_CLOCK	0xC20003A0
#else
#define MTK_SIP_KERNEL_UDI_WRITE	0x8200035E /* OCP_WRITE */
#define MTK_SIP_KERNEL_UDI_READ		0x8200035F /* OCP_READ */
#define MTK_SIP_KERNEL_UDI_JTAG_CLOCK	0x820003A0
#endif

/* dfine for UDI register service */
#define udi_reg_read(addr)	\
				mt_secure_call_udi(MTK_SIP_KERNEL_UDI_READ, addr, 0, 0)
#define udi_reg_write(addr, val)	\
				mt_secure_call_udi(MTK_SIP_KERNEL_UDI_WRITE, addr, val, 0)
#define udi_jtag_clock(sw_tck, i_trst, i_tms, i_tdi, count)	\
				mt_secure_call_udi(MTK_SIP_KERNEL_UDI_JTAG_CLOCK,	\
									(((0x1 << (sw_tck & 0x03)) << 3) |	\
									((i_trst & 0x01) << 2) |	\
									((i_tms & 0x01) << 1) |	\
									(i_tdi & 0x01)),	\
									count, 0)

/* UDI Extern Function */
/* UDI_EXTERN unsigned int mt_udi_get_level(void); */
/* UDI_EXTERN void mt_udi_lock(unsigned long *flags); */
/* UDI_EXTERN void mt_udi_unlock(unsigned long *flags); */
/* UDI_EXTERN int mt_udi_idle_can_enter(void); */
/* UDI_EXTERN int mt_udi_status(ptp_det_id id); */

#ifdef CONFIG_ARM64
/* UDI_EXTERN int get_udi_status(void); */
/* UDI_EXTERN unsigned int get_vcore_ptp_volt(int uv); */
#endif /* CONFIG_ARM64 */


#endif /* _MT_UDI_H */
