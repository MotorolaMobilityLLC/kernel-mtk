/*
 * Copyright (C) 2025 Motorola Mobility LLC
 * All Rights Reserved.
 * Motorola Mobility Confidential Restricted.
 */

#ifndef PANEL_MOT_VDO_TM_ILI79505A_720_1600_DPHY_120HZ
#define PANEL_MOT_VDO_TM_ILI79505A_720_1600_DPHY_120HZ

#define PANEL_ESD_RECOVERY_NOFLASH   0x01
#define PANEL_ESD_RECOVERY_VDD       0x02
#define REGFLAG_DELAY             0xFFFC
#define REGFLAG_UDELAY            0xFFFB
#define REGFLAG_END_OF_TABLE      0xFFFD
#define REGFLAG_RESET_LOW         0xFFFE
#define REGFLAG_RESET_HIGH        0xFFFF

#define FRAME_WIDTH                 720
#define FRAME_HEIGHT                1604

#define PHYSICAL_WIDTH              69400
#define PHYSICAL_HEIGHT             154609

#define DATA_RATE                   1268
#define HSA                         4
#define HBP                         20
#define VSA                         4
#define VBP                         28

/*Parameter setting for mode 3 Start*/
#define MODE_120_FPS                120
#define MODE_120_VFP                390
#define MODE_120_HFP                26

/*Parameter setting for mode 3 End*/

//Parameter setting for mode 0 Start
#define MODE_60_FPS                 60
#define MODE_60_VFP                 2420
#define MODE_60_HFP                 26

//Parameter setting for mode 0 End
//Parameter setting for mode 2 Start
#define MODE_90_FPS                 90
#define MODE_90_VFP                 1060
#define MODE_90_HFP                 26

//Parameter setting for mode 2 End

#define LFR_EN                      1
/* DSC RELATED */

typedef struct {
    uint16_t start;
    uint16_t end;
    uint16_t hal_start;
    uint16_t hal_end;
    uint32_t hal_range;
    uint32_t dbv_range;
} DbvHalMap;

#define CALC_RANGES(start, end, hal_start, hal_end) \
    (hal_end - hal_start), (end - start)

#define MAP_SIZE (sizeof(dbv_hal_map) / sizeof(DbvHalMap))

int panel_esd_register_client(const char *source, struct notifier_block *nb);
int panel_esd_unregister_client(struct notifier_block *nb);
int panel_esd_notifier_call_chain(unsigned long val, void *v);
#endif
