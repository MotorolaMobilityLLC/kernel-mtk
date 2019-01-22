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

#ifndef _FLASHLIGHT_DT_H
#define _FLASHLIGHT_DT_H

#ifdef mt6757
#define LM3643_DTNAME     "mediatek,flashlights_lm3643"
#define LM3643_DTNAME_I2C "mediatek,strobe_main"
#define RT5081_DTNAME     "mediatek,flashlights_rt5081"
#endif

#ifdef mt6799
#define MT6336_DTNAME     "mediatek,flashlights_mt6336"
#endif

#endif /* _FLASHLIGHT_DT_H */
