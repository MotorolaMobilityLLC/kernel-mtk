#ifndef __GED_NOTIFY_SW_VSYNC_H__
#define __GED_NOTIFY_SW_VSYNC_H__

#include "ged_type.h"


GED_ERROR ged_notify_sw_vsync(GED_VSYNC_TYPE eType, GED_DVFS_UM_QUERY_PACK* psQueryData);

GED_ERROR ged_notify_sw_vsync_system_init(void);

void ged_notify_sw_vsync_system_exit(void);


void ged_sodi_start(void);
void ged_sodi_stop(void);

#endif
