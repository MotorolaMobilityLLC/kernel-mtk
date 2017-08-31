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

#ifndef _MT_EEM_TURBO_
#define _MT_EEM_TURBO_
/* #define EEM_ENABLE_VTURBO */
/* #define EEM_ENABLE_TTURBO */
/* #define EEM_ENABLE_ITURBO */


#ifdef __KERNEL__
#if defined(EEM_ENABLE_VTURBO)
	extern unsigned int *get_turbo(unsigned int binLevel, unsigned int binLevelEng);
#endif
#endif
#endif
