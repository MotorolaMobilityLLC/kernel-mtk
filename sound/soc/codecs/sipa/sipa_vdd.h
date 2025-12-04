/*
 * Copyright (C) 2022, SI-IN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SIPA_VDD_H__
#define __SIPA_VDD_H__

#include "sipa_common.h"

#define SIPA_VOL_GET_START_TIME         100

void sipa_vol_get(sipa_dev_t *si_pa);
void sipa_vol_get_release(sipa_dev_t *si_pa);


#endif	/* SIPA_VDD_H */
