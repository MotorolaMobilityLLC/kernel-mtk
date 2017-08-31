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

#ifndef _SMI_DEBUG_H_
#define _SMI_DEBUG_H_


enum smi_subsys_id {
	SMI_DISP,
	SMI_VDEC,
	SMI_IMG,
	SMI_VENC,
	SMI_MJC,
	SMI_SUBSYS_NUM
};

#define SMI_DBG_DISPSYS smi_subsys_larb_mapping_table[SMI_DISP]
#define SMI_DBG_VDEC smi_subsys_larb_mapping_table[SMI_VDEC]
#define SMI_DBG_IMGSYS smi_subsys_larb_mapping_table[SMI_IMG]
#define SMI_DBG_VENC smi_subsys_larb_mapping_table[SMI_VENC]
#define SMI_DBG_MJC smi_subsys_larb_mapping_table[SMI_MJC]
#define SMI_DBG_ALL_SUBSYS (SMI_DBG_DISPSYS | SMI_DBG_VDEC | SMI_DBG_IMGSYS | SMI_DBG_VENC | SMI_DBG_MJC)

#define SMI_DGB_LARB_SELECT(smi_dbg_larb, n) ((smi_dbg_larb) & (1<<n))

#ifndef CONFIG_MTK_SMI_EXT
#define smi_debug_bus_hanging_detect(larbs, show_dump) {}
#define smi_debug_bus_hanging_detect_ext(larbs, show_dump, output_gce_buffer) {}
#else
int smi_debug_bus_hanging_detect(unsigned int larbs, int show_dump);
    /* output_gce_buffer = 1, pass log to CMDQ error dumping messages */
int smi_debug_bus_hanging_detect_ext(unsigned short larbs, int show_dump, int output_gce_buffer);

#endif
void smi_dumpCommonDebugMsg(int output_gce_buffer);
void smi_dumpLarbDebugMsg(unsigned int u4Index, int output_gce_buffer);
void smi_dumpDebugMsg(void);

extern int smi_larb_clock_is_on(unsigned int larb_index);

extern unsigned short smi_subsys_larb_mapping_table[SMI_SUBSYS_NUM];

extern void smi_dump_clk_status(void);
extern void smi_dump_larb_m4u_register(int larb);
#endif				/* _SMI_DEBUG_H__ */
