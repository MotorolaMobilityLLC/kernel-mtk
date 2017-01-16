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

#ifndef _PERFMGR_BOOST_H_
#define _PERFMGR_BOOST_H_

/*-----------------------------------------------*/
/* prototype                                     */
/*-----------------------------------------------*/

#if defined(CONFIG_CPUSETS) && !defined(CONFIG_MTK_ACAO)
extern void __attribute__((weak))save_set_exclusive_task(int pid);
#endif

#endif

