/*
 * Copyright (C) 2025 Motorola Mobility LLC
 * All Rights Reserved.
 * Motorola Mobility Confidential Restricted.
 */

#ifndef PANEL_TD4160_667_HDPLUS_DSI_VDO_120HZ_TXD
#define PANEL_TD4160_667_HDPLUS_DSI_VDO_120HZ_TXD

#define PANEL_ESD_RECOVERY_NOFLASH	0x01
#define PANEL_ESD_RECOVERY_VDD          0x02
#define REGFLAG_DELAY           0xFFFC
#define REGFLAG_UDELAY          0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW       0xFFFE
#define REGFLAG_RESET_HIGH      0xFFFF

#define FRAME_WIDTH                 720
#define FRAME_HEIGHT                1604

#define PHYSICAL_WIDTH              69400
#define PHYSICAL_HEIGHT            154609

#define DATA_RATE                   1156
#define HSA                         4
#define HBP                         30
#define HFP                         30
#define VSA                         4
#define VBP                         32

/*Parameter setting for mode 0 Start*/

#define MODE_60_FPS                  60
#define MODE_60_VFP                  2040
#define MODE_60_HFP                  30


#define MODE_0_DATA_RATE            1156
/*Parameter setting for mode 0 End*/

/*Parameter setting for mode 2 Start*/
#define MODE_90_FPS                  90
#define MODE_90_VFP                  814
#define MODE_90_HFP                  30

#define MODE_90_DATA_RATE            1156
/*Parameter setting for mode 2 End*/

/*Parameter setting for mode 3 Start*/
#define MODE_120_FPS                  120
#define MODE_120_VFP                  200
#define MODE_120_HFP                  30

#define MODE_120_DATA_RATE            1156
/*Parameter setting for mode 3 End*/

#define LFR_EN                      1
/* DSC RELATED */

#define DSC_ENABLE                  0

int panel_esd_register_client(const char *source, struct notifier_block *nb);
int panel_esd_unregister_client(struct notifier_block *nb);
int panel_esd_notifier_call_chain(unsigned long val, void *v);
int panel_gesture_register_client(const char *source, struct notifier_block *nb);
int panel_gesture_unregister_client(struct notifier_block *nb);
int panel_gesture_notifier_call_chain(unsigned long val, void *v);
#endif //end of PANEL_TD4160_667_HDPLUS_DSI_VDO_120HZ_TXD
