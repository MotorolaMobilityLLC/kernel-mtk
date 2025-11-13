/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef PANEL_TD4165_614_FHD_DSI_VDO_120HZ_BOE
#define PANEL_TD4165_614_FHD_DSI_VDO_120HZ_BOE

#define PANEL_ESD_RECOVERY_NOFLASH	0x01
#define PANEL_ESD_RECOVERY_VDD          0x02
#define REGFLAG_DELAY           0xFFFC
#define REGFLAG_UDELAY          0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW       0xFFFE
#define REGFLAG_RESET_HIGH      0xFFFF

#define FRAME_WIDTH                 900
#define FRAME_HEIGHT                1984

#define PHYSICAL_WIDTH              64422
#define PHYSICAL_HEIGHT            142014

#define DATA_RATE                   700
#define HSA                         4
#define HBP                         32
#define HFP                         32
#define VSA                         4
#define VBP                         32

/*Parameter setting for mode 0 Start*/
#define MODE_60_FPS                  60
#define MODE_60_VFP                  2536
#define MODE_60_HFP                  32

#define MODE_0_DATA_RATE            700
/*Parameter setting for mode 0 End*/

/*Parameter setting for mode 2 Start*/
#define MODE_90_FPS                  90
#define MODE_90_VFP                  1020
#define MODE_90_HFP                  32

#define MODE_90_DATA_RATE            700
/*Parameter setting for mode 2 End*/

/*Parameter setting for mode 3 Start*/
#define MODE_120_FPS                  120
#define MODE_120_VFP                  260
#define MODE_120_HFP                  32

#define MODE_120_DATA_RATE            700
/*Parameter setting for mode 3 End*/



#define LFR_EN                      1
/* DSC RELATED */


#define DSC_ENABLE                  1
#define DSC_VER                     17
#define DSC_SLICE_MODE              1
#define DSC_RGB_SWAP                0
#define DSC_DSC_CFG                 34
#define DSC_RCT_ON                  1
#define DSC_BIT_PER_CHANNEL         8
#define DSC_DSC_LINE_BUF_DEPTH      9
#define DSC_BP_ENABLE               1
#define DSC_BIT_PER_PIXEL           128
#define DSC_SLICE_HEIGHT            8
#define DSC_SLICE_WIDTH             450
#define DSC_CHUNK_SIZE              450
#define DSC_XMIT_DELAY              512
#define DSC_DEC_DELAY               481
#define DSC_SCALE_VALUE             32
#define DSC_INCREMENT_INTERVAL      171
#define DSC_DECREMENT_INTERVAL      6
#define DSC_LINE_BPG_OFFSET         12
#define DSC_NFL_BPG_OFFSET          3511
#define DSC_SLICE_BPG_OFFSET        3905
#define DSC_INITIAL_OFFSET          6144
#define DSC_FINAL_OFFSET            4336
#define DSC_FLATNESS_MINQP          3
#define DSC_FLATNESS_MAXQP          12
#define DSC_RC_MODEL_SIZE           8192
#define DSC_RC_EDGE_FACTOR          6
#define DSC_RC_QUANT_INCR_LIMIT0    11
#define DSC_RC_QUANT_INCR_LIMIT1    11
#define DSC_RC_TGT_OFFSET_HI        3
#define DSC_RC_TGT_OFFSET_LO        3

int panel_esd_register_client(const char *source, struct notifier_block *nb);
int panel_esd_unregister_client(struct notifier_block *nb);
int panel_esd_notifier_call_chain(unsigned long val, void *v);
int panel_gesture_register_client(const char *source, struct notifier_block *nb);
int panel_gesture_unregister_client(struct notifier_block *nb);
int panel_gesture_notifier_call_chain(unsigned long val, void *v);
#endif //end of PANEL_TD4165_614_FHD_DSI_VDO_120HZ_BOE
