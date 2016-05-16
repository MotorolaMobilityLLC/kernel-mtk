#ifndef _CAM_CAL_DATA_H
#define _CAM_CAL_DATA_H

#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

typedef struct{
	u32 u4Offset;
	u32 u4Length;
#if !defined(S5K3M2_MIPI_RAW)||!defined(S5K3M2_SUNNY_MIPI_RAW)
	u32 sensorID;
	u32 deviceID;
	u8 *pu1Params;
#else
	u32 *pu1Params;  //wuyt3 modify for compile error(warnning)	
#endif
	
} stCAM_CAL_INFO_STRUCT, *stPCAM_CAL_INFO_STRUCT;

#ifdef CONFIG_COMPAT

typedef struct{
	u32 u4Offset;
	u32 u4Length;
#if !defined(S5K3M2_MIPI_RAW)||!defined(S5K3M2_SUNNY_MIPI_RAW)
	u32 sensorID;
	u32 deviceID;
#endif
	compat_uptr_t pu1Params;
} COMPAT_stCAM_CAL_INFO_STRUCT;
#endif

#endif/*_CAM_CAL_DATA_H*/

