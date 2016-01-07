#include <asm/io.h>
#include <linux/string.h>
#include "smi_reg.h"
#include "mt_smi.h"
#include "smi_common.h"
#include "smi_configuration.h"
#include "smi_config_util.h"

int smi_bus_regs_setting(int profile, struct SMI_SETTING *settings)
{
	int i = 0;
	int j = 0;

	if (!settings || profile < 0 || profile >= SMI_BWC_SCEN_CNT)
		return -1;

	if (settings->smi_common_reg_num == 0)
		return -1;

	/* set regs of common */
	SMIMSG("Current Scen:%d", profile);
	for (i = 0 ; i < settings->smi_common_reg_num ;  ++i) {
		M4U_WriteReg32(SMI_COMMON_EXT_BASE,
		settings->smi_common_setting_vals[i].offset,
		settings->smi_common_setting_vals[i].value);
	}

	/* set regs of larbs */
	for (i = 0 ; i < SMI_LARB_NR ; ++i)
		for (j = 0 ; j < settings->smi_larb_reg_num[i] ; ++j) {
			M4U_WriteReg32(gLarbBaseAddr[i],
			settings->smi_larb_setting_vals[i][j].offset,
			settings->smi_larb_setting_vals[i][j].value);
		}
	return 0;
}

void save_default_common_val(int *is_default_value_saved, unsigned int *default_val_smi_array)
{
	if (!*is_default_value_saved) {
		int i = 0;

		SMIMSG("Save default config:\n");
		for (i = 0 ; i < SMI_LARB_NR ; ++i)
			default_val_smi_array[i] = M4U_ReadReg32(SMI_COMMON_EXT_BASE, smi_common_l1arb_offset[i]);

		*is_default_value_saved = 1;
	}
}
