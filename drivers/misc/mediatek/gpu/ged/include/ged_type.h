#ifndef __GED_TYPE_H__
#define __GED_TYPE_H__

#include "ged_error.h"

typedef void* GED_HANDLE;

typedef unsigned int GED_LOG_BUF_HANDLE;

typedef	enum GED_BOOL_TAG
{
	GED_FALSE,
	GED_TRUE
} GED_BOOL;

typedef enum GED_INFO_TAG
{
    GED_LOADING,
    GED_IDLE,
    GED_BLOCKING,
    GED_PRE_FREQ,
    GED_PRE_FREQ_IDX,
    GED_CUR_FREQ,
    GED_CUR_FREQ_IDX,
    GED_MAX_FREQ_IDX,
    GED_MAX_FREQ_IDX_FREQ,
    GED_MIN_FREQ_IDX,
    GED_MIN_FREQ_IDX_FREQ,    
    GED_3D_FENCE_DONE_TIME,
    GED_VSYNC_OFFSET,
    GED_EVENT_STATUS,
    GED_EVENT_DEBUG_STATUS,
    GED_EVENT_GAS_MODE,
    GED_SRV_SUICIDE,
    GED_PRE_HALF_PERIOD,
    GED_LATEST_START,
    GED_UNDEFINED
} GED_INFO;

typedef enum GED_VSYNC_TYPE_TAG
{
    GED_VSYNC_SW_EVENT,
    GED_VSYNC_HW_EVENT
} GED_VSYNC_TYPE;

typedef struct GED_DVFS_UM_QUERY_PACK_TAG
{
    unsigned int ui32GPULoading;
    unsigned int ui32GPUFreqID;
    unsigned long gpu_cur_freq;
    unsigned long gpu_pre_freq;
    long long usT;
    long long nsOffset;
    unsigned long ul3DFenceDoneTime;    
    unsigned long ulPreCalResetTS_us;
    unsigned long ulWorkingPeriod_us;
}GED_DVFS_UM_QUERY_PACK;

enum {
	GAS_CATEGORY_GAME,
	GAS_CATEGORY_OTHERS,
};

#endif
