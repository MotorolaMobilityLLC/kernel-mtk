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

#ifndef __M4U_PORT_H__
#define __M4U_PORT_H__

#if defined(CONFIG_ARCH_MT6735)
#include "mt6735/m4u_port.h"
#endif

#if defined(CONFIG_ARCH_MT6735M)
#include "mt6735m/m4u_port.h"
#endif

#if defined(CONFIG_ARCH_MT6753)
#include "mt6753/m4u_port.h"
#endif

#endif
