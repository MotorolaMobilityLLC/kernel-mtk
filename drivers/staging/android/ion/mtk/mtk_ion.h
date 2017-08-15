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
#ifndef _MTK_LINUX_ION_H
#define _MTK_LINUX_ION_H
enum mtk_ion_heap_type {
	ION_HEAP_TYPE_MULTIMEDIA = 10,
	ION_HEAP_TYPE_FB = 11,
	ION_HEAP_TYPE_MULTIMEDIA_FOR_CAMERA = 12,
	ION_HEAP_TYPE_MULTIMEDIA_SEC = 13,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define ION_HEAP_MULTIMEDIA_MASK (1 << ION_HEAP_TYPE_MULTIMEDIA)
#define ION_HEAP_FB_MASK         (1 << ION_HEAP_TYPE_FB)
#define ION_HEAP_CAMERA_MASK (1 << ION_HEAP_TYPE_MULTIMEDIA_FOR_CAMERA)
#define ION_HEAP_MULTIMEDIA_SEC_MASK (1 << ION_HEAP_TYPE_MULTIMEDIA_SEC)

#define ION_NUM_HEAP_IDS (sizeof(unsigned int) * 8)
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
