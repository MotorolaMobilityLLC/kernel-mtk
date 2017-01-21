/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 * Copyright (C) 2010 Js HA <js.ha@stericsson.com>
 * Copyright (C) 2010 Naveen Kumar G <naveen.gaddipati@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

/* Pre-defined definition */

/* Register */
#define FD_ADDR_MAX	0xE9
#define FD_ADDR_MIN	0xDD
#define FD_BYTE_COUNT	6
/*all custom setting has removed to *.dtsi and {project}_defconfig*/
#ifdef CONFIG_SYNA_RMI4_AUTO_UPDATE
#define TPD_UPDATE_FIRMWARE
#endif

#define VELOCITY_CUSTOM
#define TPD_VELOCITY_CUSTOM_X 15
#define TPD_VELOCITY_CUSTOM_Y 15

/* #define TPD_HAVE_CALIBRATION */
/* #define TPD_CALIBRATION_MATRIX  {2680,0,0,0,2760,0,0,0}; */
/* #define TPD_WARP_START */
/* #define TPD_WARP_END */

/* #define TPD_RESET_ISSUE_WORKAROUND */
/* #define TPD_MAX_RESET_COUNT 3 */
/* #define TPD_WARP_Y(y) ( TPD_Y_RES - 1 - y ) */
/* #define TPD_WARP_X(x) ( x ) */

#endif				/* TOUCHPANEL_H__ */
