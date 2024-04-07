/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __SITUTATION_HUB_H__
#define __SITUTATION_HUB_H__

#include <linux/module.h>

#if IS_ENABLED(CONFIG_MTK_INPKHUB)
#include "inpocket/inpocket.h"
#endif

#if IS_ENABLED(CONFIG_MTK_STATHUB)
#include "stationary/stationary.h"
#endif

#if IS_ENABLED(CONFIG_MTK_WAKEHUB)
#include "wake_gesture/wake_gesture.h"
#endif

#if IS_ENABLED(CONFIG_MTK_GLGHUB)
#include "glance_gesture/glance_gesture.h"
#endif

#if IS_ENABLED(CONFIG_MTK_PICKUPHUB)
#include "pickup_gesture/pickup_gesture.h"
#endif

#if IS_ENABLED(CONFIG_MTK_ANSWER_CALL_HUB)
#include "answercall/ancallhub.h"
#endif

#if IS_ENABLED(CONFIG_MTK_DEVICE_ORIENTATION_HUB)
#include "device_orientation/device_orientation.h"
#endif

#if IS_ENABLED(CONFIG_MTK_MOTION_DETECT_HUB)
#include "motion_detect/motion_detect.h"
#endif

#if IS_ENABLED(CONFIG_MTK_TILTDETECTHUB)
#include "tilt_detector/tiltdetecthub.h"
#endif

#if IS_ENABLED(CONFIG_MTK_FLAT_HUB)
#include "flat/flat.h"
#endif

#if IS_ENABLED(CONFIG_MTK_SAR_HUB)
#include "sar/sarhub.h"
#endif

// moto add
#ifdef CONFIG_MOTO_HUB
#if IS_ENABLED(CONFIG_MOTO_CHOPCHOP)
#include "mot_hub/moto_chopchop/moto_chopchop.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_CAMGEST)
#include "mot_hub/moto_camgest/moto_camgest.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_FLATDOWN)
#include "mot_hub/moto_flatdown/moto_flatdown.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_FLATUP)
#include "mot_hub/moto_flatup/moto_flatup.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_FLATUP)
#include "mot_hub/moto_flatup/moto_flatup.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_FTM)
#include "mot_hub/moto_ftm/moto_ftm.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_GLANCE)
#include "mot_hub/moto_glance/moto_glance.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_LTS)
#include "mot_hub/moto_lts/moto_lts.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_LTV)
#include "mot_hub/moto_ltv/moto_ltv.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_OFFBODY)
#include "mot_hub/moto_offbody/moto_offbody.h"
#endif

#if IS_ENABLED(CONFIG_MOTO_STOWED)
#include "mot_hub/moto_stowed/moto_stowed.h"
#endif
#endif

#endif
