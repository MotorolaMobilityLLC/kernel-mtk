#ifndef __MTK_HRT_H__
#define __MTK_HRT_H__

#include "disp_session.h"
#include "disp_lcm.h"
#include "disp_drv_log.h"
#include "primary_display.h"
#include "disp_drv_platform.h"
#include "display_recorder.h"

#define EMI_EXTREME_LOWER_BOUND 2
#define EMI_LOWER_BOUND 4
#define EMI_UPPER_BOUND 6
#define LARB_LOWER_BOUND 3
#define LARB_UPPER_BOUND 4
#define OD_EMI_LOWER_BOUND 3
#define OD_EMI_UPPER_BOUND 4
#define OD_LARB_LOWER_BOUND 2
#define OD_LARB_UPPER_BOUND 3

#define PRIMARY_OVL_LAYER_NUM PRIMARY_SESSION_INPUT_LAYER_COUNT
#define SECONDARY_OVL_LAYER_NUM EXTERNAL_SESSION_INPUT_LAYER_COUNT

/* #define HRT_DEBUG */

int dispsys_hrt_calc(disp_layer_info *disp_info);

typedef struct hrt_sort_entry_t {
	struct hrt_sort_entry_t *head, *tail;
	layer_config *layer_info;
	int key;
	int overlap_w;
} hrt_sort_entry;

typedef enum {
	HRT_LEVEL_EXTREME_LOW = 0,
	HRT_LEVEL_LOW,
	HRT_LEVEL_HIGH,
	HRT_OVER_LIMIT,
} HRT_LEVEL;

typedef enum {
	HRT_PRIMARY = 0,
	HRT_SECONDARY,
} HRT_DISP_TYPE;

extern int hdmi_get_dev_info(int is_sf, void *info);

#endif
