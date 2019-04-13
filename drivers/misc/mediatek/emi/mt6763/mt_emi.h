/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __MT_EMI_H__
#define __MT_EMI_H__

/* submoduce control */
#define ENABLE_BWL	1
#define ENABLE_MPU	1
#define ENABLE_ELM	0
#define ENABLE_MBW	0
#define DECS_ON_SSPM

/* IRQ from device tree */
#define MPU_IRQ_INDEX	0
#define ELM_IRQ_INDEX	1 /* mt6763 uses CGM IRQ as ELM IRQ */

/* macro for MPU */
#define ENABLE_AP_REGION	1
#define AP_REGION_ID		31

#define EMI_MPUD0_ST		(CEN_EMI_BASE + 0x160)
#define EMI_MPUD_ST(domain)	(EMI_MPUD0_ST + (4*domain))
#define EMI_MPUD0_ST2		(CEN_EMI_BASE + 0x200)
#define EMI_MPUD_ST2(domain)	(EMI_MPUD0_ST2 + (4*domain))
#define EMI_MPUS		(CEN_EMI_BASE + 0x01F0)
#define EMI_MPUT		(CEN_EMI_BASE + 0x01F8)
#define EMI_MPUT_2ND		(CEN_EMI_BASE + 0x01FC)

#define EMI_MPU_SA0		(0x100)
#define EMI_MPU_EA0		(0x200)
#define EMI_MPU_SA(region)	(EMI_MPU_SA0 + (region*4))
#define EMI_MPU_EA(region)	(EMI_MPU_EA0 + (region*4))
#define EMI_MPU_APC0		(0x300)
#define EMI_MPU_APC(region, dgroup) \
	(EMI_MPU_APC0 + (region*4) + ((dgroup)*0x100))

/* macro for ELM */
#define EMI_CONM	(CEN_EMI_BASE + 0x060)
#define EMI_BMEN	(CEN_EMI_BASE + 0x400)
#define EMI_BMEN2	(CEN_EMI_BASE + 0x4E8)
#define EMI_BCNT	(CEN_EMI_BASE + 0x408)
#define EMI_WSCT	(CEN_EMI_BASE + 0x428)
#define EMI_WSCT2	(CEN_EMI_BASE + 0x458)
#define EMI_WSCT3	(CEN_EMI_BASE + 0x460)
#define EMI_WSCT4	(CEN_EMI_BASE + 0x464)
#define EMI_MSEL	(CEN_EMI_BASE + 0x440)
#define EMI_MSEL2	(CEN_EMI_BASE + 0x468)
#define EMI_BMRW0	(CEN_EMI_BASE + 0x4F8)
#define EMI_CGMA	(CEN_EMI_BASE + 0x720)
#define EMI_CGMA_ST0	(CEN_EMI_BASE + 0x724)
#define EMI_CGMA_ST1	(CEN_EMI_BASE + 0x728)
#define EMI_CGMA_ST2	(CEN_EMI_BASE + 0x72C)
#define EMI_EBMINT_ST	(CEN_EMI_BASE + 0x744)
#define EMI_LTCT0_2ND	(CEN_EMI_BASE + 0x750)
#define EMI_LTCT1_2ND	(CEN_EMI_BASE + 0x754)
#define EMI_LTCT2_2ND	(CEN_EMI_BASE + 0x758)
#define EMI_LTCT3_2ND	(CEN_EMI_BASE + 0x75C)
#define EMI_LTST0_2ND	(CEN_EMI_BASE + 0x760)
#define EMI_LTST1_2ND	(CEN_EMI_BASE + 0x764)
#define EMI_LTST2_2ND	(CEN_EMI_BASE + 0x768)
#define EMI_CGMA_ST0	(CEN_EMI_BASE + 0x724)
#define EMI_CGMA_ST1	(CEN_EMI_BASE + 0x728)
#define EMI_CGMA_ST2	(CEN_EMI_BASE + 0x72C)
#define EMI_TTYPE1	(CEN_EMI_BASE + 0x500)
#define EMI_TTYPE(i)	(EMI_TTYPE1 + (i*8))

/* Camera workaround */
#define EMI_BWCT0	(CEN_EMI_BASE + 0x5B0)

#include <mt_emi_api.h>
#include <bwl_v1.h>
#include <elm_v1.h>

#endif /* __MT_EMI_H__ */

