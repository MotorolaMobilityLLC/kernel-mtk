/*****************************************************************************
* Copyright (C) 2015 ARM Limited or its affiliates.	                     *
* This program is free software; you can redistribute it and/or modify it    *
* under the terms of the GNU General Public License as published by the Free *
* Software Foundation; either version 2 of the License, or (at your option)  * 
* any later version.							     *
* This program is distributed in the hope that it will be useful, but 	     *
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
* for more details.							     *	
* You should have received a copy of the GNU General Public License along    *
* with this program; if not, write to the Free Software Foundation, 	     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.        *
******************************************************************************/

/* Dummy pal_log_plat for test driver in kernel */

#ifndef _SSI_PAL_LOG_PLAT_H_
#define _SSI_PAL_LOG_PLAT_H_

#if defined(DEBUG)

#define __SASI_PAL_LOG_PLAT(level, format, ...) printk(level "cc63_test::" format , ##__VA_ARGS__)

#else /* Disable all prints */

#define __SASI_PAL_LOG_PLAT(...)  do {} while (0)

#endif

#endif /*_SASI_PAL_LOG_PLAT_H_*/

