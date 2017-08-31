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
#ifndef __SH_SVP_H__
#define __SH_SVP_H__

#define SVP_REGION_IOC_MAGIC		'S'

#define SVP_REGION_IOC_ONLINE		_IOR(SVP_REGION_IOC_MAGIC, 2, int)
#define SVP_REGION_IOC_OFFLINE		_IOR(SVP_REGION_IOC_MAGIC, 4, int)

#define SVP_REGION_ACQUIRE			_IOR(SVP_REGION_IOC_MAGIC, 6, int)
#define SVP_REGION_RELEASE			_IOR(SVP_REGION_IOC_MAGIC, 8, int)

void show_pte(struct mm_struct *mm, unsigned long addr);

#define UPPER_LIMIT32 (1ULL << 32)
#define UPPER_LIMIT64 (1ULL << 63)

extern int _tui_region_offline(phys_addr_t *pa, unsigned long *size,
		u64 upper_limit);

int tui_region_offline64(phys_addr_t *pa, unsigned long *size)
{
	return _tui_region_offline(pa, size, UPPER_LIMIT64);
}
EXPORT_SYMBOL(tui_region_offline64);

int tui_region_offline(phys_addr_t *pa, unsigned long *size)
{
	return _tui_region_offline(pa, size, UPPER_LIMIT32);
}
EXPORT_SYMBOL(tui_region_offline);

extern int _svp_region_offline(phys_addr_t *pa, unsigned long *size,
		u64 upper_limit);

int svp_region_offline64(phys_addr_t *pa, unsigned long *size)
{
	return _svp_region_offline(pa, size, UPPER_LIMIT64);
}
EXPORT_SYMBOL(svp_region_offline64);

int svp_region_offline(phys_addr_t *pa, unsigned long *size)
{
	return _svp_region_offline(pa, size, UPPER_LIMIT32);
}
EXPORT_SYMBOL(svp_region_offline);
#endif
