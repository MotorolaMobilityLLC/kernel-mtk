/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/
#ifndef __SSPM_IPI_PIN_H__
#define __SSPM_IPI_PIN_H__

/* define module id here ... */
/* use bit 0 - 7 of IN_IRQ of mailbox 0 */
#define IPI_ID_DCS          0  /* the following will use mbox 0 */
#define IPI_ID_MET          1
#define IPI_ID_PLATFORM     2
#define IPI_ID_PTPOD        3
#define IPI_ID_CPU_DVFS     4
#define IPI_ID_GPU_DVFS     5
#define IPI_ID_UNUSED1      6
#define IPI_ID_UNUSED2      7
/* use bit 8 - 15 of IN_IRQ of mailbox 1 */
#define IPI_ID_PMIC_WRAP    8  /* the following will use mbox 1 */
#define IPI_ID_CLOCK        9
#define IPI_ID_THERMAL      10
#define IPI_ID_DEEP_IDLE    11
#define IPI_ID_MCDI         12
#define IPI_ID_SODI         13
#define IPI_ID_SPM_SUSPEND  14
#define IPI_ID_FHCTL        15
#define IPI_ID_PMIC         16
#define IPI_ID_PPM          17
#define IPI_ID_OCP          18
#define IPI_ID_EXT_BUCK     19
#define IPI_ID_TOTAL        20

#define IPI_OPT_REDEF_MASK      0x1
#define IPI_OPT_LOCK_MASK       0x2
#define IPI_OPT_POLLING_MASK    0x4

#endif /* __SSPM_IPI_PIN_H__ */
