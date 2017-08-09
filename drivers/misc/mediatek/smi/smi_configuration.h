#ifndef _SMI_CONFIGURATION_H_
#define _SMI_CONFIGURATION_H_

#include "smi_reg.h"
#include "mt_smi.h"
/* ***********debug parameters*********** */

#define SMI_COMMON_DEBUG_OFFSET_NUM 16
#define SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM 69


#if defined(SMI_D1) || defined(SMI_D3) || defined(SMI_J)
#define SMI_LARB0_DEBUG_OFFSET_NUM SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM
#define SMI_LARB1_DEBUG_OFFSET_NUM SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM
#define SMI_LARB2_DEBUG_OFFSET_NUM SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM
#define SMI_LARB3_DEBUG_OFFSET_NUM SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM

#elif defined(SMI_D2)
#define SMI_LARB0_DEBUG_OFFSET_NUM SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM
#define SMI_LARB1_DEBUG_OFFSET_NUM SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM
#define SMI_LARB2_DEBUG_OFFSET_NUM SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM

#elif defined(SMI_R)
#define SMI_LARB0_DEBUG_OFFSET_NUM SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM
#define SMI_LARB1_DEBUG_OFFSET_NUM SMI_LARB_DEFAULT_DEBUG_OFFSET_NUM

#endif


struct SMI_SETTING_VALUE {
	unsigned int offset;
	int value;
};

struct SMI_SETTING {
	unsigned int smi_common_reg_num;
	struct SMI_SETTING_VALUE *smi_common_setting_vals;
	unsigned int smi_larb_reg_num[SMI_LARB_NR];
	struct SMI_SETTING_VALUE *smi_larb_setting_vals[SMI_LARB_NR];
};

struct SMI_PROFILE_CONFIG {
	int smi_profile;
	struct SMI_SETTING *setting;
};

#define SMI_PROFILE_CONFIG_NUM SMI_BWC_SCEN_CNT

extern unsigned long smi_common_debug_offset[SMI_COMMON_DEBUG_OFFSET_NUM];
extern int smi_larb_debug_offset_num[SMI_LARB_NR];
extern unsigned long *smi_larb_debug_offset[SMI_LARB_NR];

#endif
