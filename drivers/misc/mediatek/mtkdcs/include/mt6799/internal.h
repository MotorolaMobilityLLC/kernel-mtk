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

#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#define MPU_ACCESS_PERMISSON_FORBIDDEN  SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, \
		FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_R_NSEC_R)
#define MPU_ACCESS_PERMISSON_NO_PROTECTION  SET_ACCESS_PERMISSON(NO_PROTECTION, NO_PROTECTION, \
		NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION)
#define DCS_MPU_REGION	(28)

#endif /* _INTERNAL_H_ */
