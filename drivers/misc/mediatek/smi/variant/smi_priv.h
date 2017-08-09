#ifndef __SMI_PRIV_H__
#define __SMI_PRIV_H__

#include "smi_reg.h"

#define SMI_LARB_PORT_NR_MAX  21/* Max port num in current platform.*/
struct mtk_smi_priv;

struct mtk_smi_data {
	unsigned int larb_nr;
	struct device *larb[SMI_LARB_NR];
	struct device *smicommon;
	const struct mtk_smi_priv *smi_priv;
	unsigned long smi_common_base;
	unsigned long larb_base[SMI_LARB_NR];

	/*record the larb port register, please use the max value*/
	unsigned short int larb_port_backup[SMI_LARB_PORT_NR_MAX*SMI_LARB_NR];
};

struct mtk_smi_priv {
	unsigned int larb_port_num[SMI_LARB_NR];/* the port number in each larb */
	unsigned char larb_vc_setting[SMI_LARB_NR];
	void (*init_setting)(struct mtk_smi_data *, bool *,
				u32 *, unsigned int);
	void (*vp_setting)(struct mtk_smi_data *);
	void (*vr_setting)(struct mtk_smi_data *);
	void (*hdmi_setting)(struct mtk_smi_data *);
	void (*hdmi_4k_setting)(struct mtk_smi_data *);
};
extern struct mtk_smi_data *smi_data;

#define SMI_COMMON_EXT_BASE	smi_data->smi_common_base
#define LARB0_BASE		smi_data->larb_base[0]
#define LARB1_BASE		smi_data->larb_base[1]
#define LARB2_BASE		smi_data->larb_base[2]
#define LARB3_BASE		smi_data->larb_base[3]
#define LARB4_BASE		smi_data->larb_base[4]
#define LARB5_BASE		smi_data->larb_base[5]

extern const struct mtk_smi_priv smi_mt8173_priv;

#endif
