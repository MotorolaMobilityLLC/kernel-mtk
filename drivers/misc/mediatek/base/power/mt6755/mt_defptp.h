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

#ifndef _MT_DEFEEM_
#define _MT_DEFEEM_

#ifdef __KERNEL__
	#include <linux/kernel.h>
#endif

#ifdef __KERNEL__
	#define EEMCONF_S       0x0011c010
	#define EEMCONF_E       0x0011c210
	#define EEMCONF_SIZE    (EEMCONF_E - EEMCONF_S)

	extern void __iomem *eem_base;
	#define EEM_BASEADDR eem_base

	#ifdef CONFIG_OF
	struct devinfo_ptp_tag {
		u32 size;
		u32 tag;
		u32 volt0;
		u32 volt1;
		u32 volt2;
		u32 have_550;
	};
	#endif
#else
	typedef unsigned char       bool;
	#define EEM_BASEADDR        (0x1100B000)
	#define eem_base EEM_BASEADDR
#endif

#define EEM_TEMPMONCTL0         (EEM_BASEADDR + 0x000)
#define EEM_TEMPMONCTL1         (EEM_BASEADDR + 0x004)
#define EEM_TEMPMONCTL2         (EEM_BASEADDR + 0x008)
#define EEM_TEMPMONINT          (EEM_BASEADDR + 0x00C)
#define EEM_TEMPMONINTSTS       (EEM_BASEADDR + 0x010)
#define EEM_TEMPMONIDET0        (EEM_BASEADDR + 0x014)
#define EEM_TEMPMONIDET1        (EEM_BASEADDR + 0x018)
#define EEM_TEMPMONIDET2        (EEM_BASEADDR + 0x01C)
#define EEM_TEMPH2NTHRE         (EEM_BASEADDR + 0x024)
#define EEM_TEMPHTHRE           (EEM_BASEADDR + 0x028)
#define EEM_TEMPCTHRE           (EEM_BASEADDR + 0x02C)
#define EEM_TEMPOFFSETH         (EEM_BASEADDR + 0x030)
#define EEM_TEMPOFFSETL         (EEM_BASEADDR + 0x034)
#define EEM_TEMPMSRCTL0         (EEM_BASEADDR + 0x038)
#define EEM_TEMPMSRCTL1         (EEM_BASEADDR + 0x03C)
#define EEM_TEMPAHBPOLL         (EEM_BASEADDR + 0x040)
#define EEM_TEMPAHBTO           (EEM_BASEADDR + 0x044)
#define EEM_TEMPADCPNP0         (EEM_BASEADDR + 0x048)
#define EEM_TEMPADCPNP1         (EEM_BASEADDR + 0x04C)
#define EEM_TEMPADCPNP2         (EEM_BASEADDR + 0x050)
#define EEM_TEMPADCMUX          (EEM_BASEADDR + 0x054)
#define EEM_TEMPADCEXT          (EEM_BASEADDR + 0x058)
#define EEM_TEMPADCEXT1         (EEM_BASEADDR + 0x05C)
#define EEM_TEMPADCEN           (EEM_BASEADDR + 0x060)
#define EEM_TEMPPNPMUXADDR      (EEM_BASEADDR + 0x064)
#define EEM_TEMPADCMUXADDR      (EEM_BASEADDR + 0x068)
#define EEM_TEMPADCEXTADDR      (EEM_BASEADDR + 0x06C)
#define EEM_TEMPADCEXT1ADDR     (EEM_BASEADDR + 0x070)
#define EEM_TEMPADCENADDR       (EEM_BASEADDR + 0x074)
#define EEM_TEMPADCVALIDADDR    (EEM_BASEADDR + 0x078)
#define EEM_TEMPADCVOLTADDR     (EEM_BASEADDR + 0x07C)
#define EEM_TEMPRDCTRL          (EEM_BASEADDR + 0x080)
#define EEM_TEMPADCVALIDMASK    (EEM_BASEADDR + 0x084)
#define EEM_TEMPADCVOLTAGESHIFT (EEM_BASEADDR + 0x088)
#define EEM_TEMPADCWRITECTRL    (EEM_BASEADDR + 0x08C)
#define EEM_TEMPMSR0            (EEM_BASEADDR + 0x090)
#define EEM_TEMPMSR1            (EEM_BASEADDR + 0x094)
#define EEM_TEMPMSR2            (EEM_BASEADDR + 0x098)
#define EEM_TEMPIMMD0           (EEM_BASEADDR + 0x0A0)
#define EEM_TEMPIMMD1           (EEM_BASEADDR + 0x0A4)
#define EEM_TEMPIMMD2           (EEM_BASEADDR + 0x0A8)
#define EEM_TEMPMONIDET3        (EEM_BASEADDR + 0x0B0)
#define EEM_TEMPADCPNP3         (EEM_BASEADDR + 0x0B4)
#define EEM_TEMPMSR3            (EEM_BASEADDR + 0x0B8)
#define EEM_TEMPIMMD3           (EEM_BASEADDR + 0x0BC)
#define EEM_TEMPPROTCTL         (EEM_BASEADDR + 0x0C0)
#define EEM_TEMPPROTTA          (EEM_BASEADDR + 0x0C4)
#define EEM_TEMPPROTTB          (EEM_BASEADDR + 0x0C8)
#define EEM_TEMPPROTTC          (EEM_BASEADDR + 0x0CC)
#define EEM_TEMPSPARE0          (EEM_BASEADDR + 0x0F0)
#define EEM_TEMPSPARE1          (EEM_BASEADDR + 0x0F4)
#define EEM_TEMPSPARE2          (EEM_BASEADDR + 0x0F8)
#define EEM_REVISIONID		(EEM_BASEADDR + 0x0FC)
#define DESCHAR			(EEM_BASEADDR + 0x200)
#define TEMPCHAR		(EEM_BASEADDR + 0x204)
#define DETCHAR			(EEM_BASEADDR + 0x208)
#define AGECHAR			(EEM_BASEADDR + 0x20C)
#define EEM_DCCONFIG		(EEM_BASEADDR + 0x210)
#define EEM_AGECONFIG		(EEM_BASEADDR + 0x214)
#define FREQPCT30		(EEM_BASEADDR + 0x218)
#define FREQPCT74		(EEM_BASEADDR + 0x21C)
#define LIMITVALS		(EEM_BASEADDR + 0x220)
#define EEM_VBOOT		(EEM_BASEADDR + 0x224)
#define EEM_DETWINDOW		(EEM_BASEADDR + 0x228)
#define EEMCONFIG		(EEM_BASEADDR + 0x22C)
#define TSCALCS			(EEM_BASEADDR + 0x230)
#define RUNCONFIG		(EEM_BASEADDR + 0x234)
#define EEMEN			(EEM_BASEADDR + 0x238)
#define INIT2VALS		(EEM_BASEADDR + 0x23C)
#define DCVALUES		(EEM_BASEADDR + 0x240)
#define AGEVALUES		(EEM_BASEADDR + 0x244)
#define VOP30			(EEM_BASEADDR + 0x248)
#define VOP74			(EEM_BASEADDR + 0x24C)
#define TEMP			(EEM_BASEADDR + 0x250)
#define EEMINTSTS		(EEM_BASEADDR + 0x254)
#define EEMINTSTSRAW		(EEM_BASEADDR + 0x258)
#define EEMINTEN		(EEM_BASEADDR + 0x25C)
#define VDESIGN30		(EEM_BASEADDR + 0x26C)
#define VDESIGN74		(EEM_BASEADDR + 0x270)
#define AGECOUNT		(EEM_BASEADDR + 0x27C)
#define SMSTATE0		(EEM_BASEADDR + 0x280)
#define SMSTATE1		(EEM_BASEADDR + 0x284)
#define EEMCORESEL		(EEM_BASEADDR + 0x400)
/*
#define EEMODCORE3EN		19:19
#define EEMODCORE2EN		18:18
#define EEMODCORE1EN		17:17
#define EEMODCORE0EN		16:16
#define APBSEL			3:0
#define APBSEL			2:0
*/
#define EEM_THERMINTST		(EEM_BASEADDR + 0x404)
#define EEMODINTST		(EEM_BASEADDR + 0x408)
/*
#define EEMODINT3		3:3
#define EEMODINT2		2:2
#define EEMODINT1		1:1
#define EEMODINT0		0:0
*/
#define EEM_THSTAGE0ST		(EEM_BASEADDR + 0x40C)
#define EEM_THSTAGE1ST		(EEM_BASEADDR + 0x410)
#define EEM_THSTAGE2ST		(EEM_BASEADDR + 0x414)
#define EEM_THAHBST0		(EEM_BASEADDR + 0x418)
#define EEM_THAHBST1		(EEM_BASEADDR + 0x41C)
#define EEMSPARE0		(EEM_BASEADDR + 0x420)
#define EEMSPARE1		(EEM_BASEADDR + 0x424)
#define EEMSPARE2		(EEM_BASEADDR + 0x428)
#define EEMSPARE3		(EEM_BASEADDR + 0x42C)
#define EEM_THSLPEVEB		(EEM_BASEADDR + 0x430)

#endif
