 /*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/* lct_print.h
 * This document is from Longcheer (LCT) software.
 * It is designed to standardize debug logs.
 * Version 		: 1.0
 * Created By   : shaohui@longcheer.com
 * Date 		: 2016/10
 * */

#ifndef __LCT_PRINT_H
#define __LCT_PRINT_H


#define LCT_LOG_CO  "[LCT]"
#define LCT_LOG_MOD "[DRV]"
#define LCT_LOG_TAG LCT_LOG_CO LCT_LOG_MOD  

/*
 * LOG_MOD can be redefined before print log
 * example:
 *
 * #ifdef LCT_LOG_MOD
 * #undef LCT_LOG_MOD
 * #define LCT_LOG_MOD "[LCM]" 
 * #endif
 * */


#define lct_pr_err(fmt, args...) \
	pr_err(LCT_LOG_TAG fmt, ##args)

#define lct_pr_info(fmt, args...) \
	pr_info(LCT_LOG_TAG fmt, ##args)

#define lct_pr_debug(fmt, args...) \
	pr_debug(LCT_LOG_TAG fmt, ##args)

#define lct_pr_warn(fmt, args...) \
	pr_warn(LCT_LOG_TAG fmt, ##args)

#endif
