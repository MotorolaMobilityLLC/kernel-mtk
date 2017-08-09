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

#ifndef _MT_PMIC_REGULATOR_H_
#define _MT_PMIC_REGULATOR_H_

typedef enum {
	VCORE,
	VDRAM,
	VMODEM,
	VMD1,
	VS1,
	VS2,
	VPA1,
	VPA2,
	VSRAM_DVFS1,
	VSRAM_DVFS2,
	VSRAM_VCORE,
	VSRAM_VGPU,
	VSRAM_VMD,
} BUCK_TYPE;


#endif				/* _MT_PMIC_REGULATOR_H_ */
