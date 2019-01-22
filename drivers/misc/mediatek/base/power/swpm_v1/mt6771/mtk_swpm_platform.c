/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <mtk_spm_vcore_dvfs_ipi.h>
#include <mtk_vcorefs_governor.h>
#include <mtk_swpm_common.h>
#include <mtk_swpm_platform.h>


/****************************************************************************
 *  Macro Definitions
 ****************************************************************************/

/****************************************************************************
 *  Type Definitions
 ****************************************************************************/

/****************************************************************************
 *  Local Variables
 ****************************************************************************/
static struct aphy_pwr_data aphy_def_pwr_tbl[] = {
	[APHY_VCORE_0P7V] = {
		.read_pwr = {
			[DDR_800] = {
				.bw = {480, 768, 1888, 3328, 5120},
				.coef = {954, 6980, 9408, 10387, 11053},
			},
			[DDR_1200] = {
				.bw = {480, 768, 1920, 3328, 7872},
				.coef = {1840, 9612, 13202, 14567, 16670},
			},
			[DDR_1600] = {
				.bw = {480, 768, 1920, 3360, 9216},
				.coef = {2334, 11838, 15836, 18198, 21684},
			},
			[DDR_1800] = {
				.bw = {560, 896, 2239, 3919, 10748},
				.coef = {2723, 13806, 18468, 21223, 25289},
			},
		},
		.write_pwr = {
			[DDR_800] = {
				.bw = {480, 768, 1888, 3328, 5120},
				.coef = {954, 6980, 9408, 10387, 11053},
			},
			[DDR_1200] = {
				.bw = {480, 768, 1920, 3328, 7872},
				.coef = {1840, 9612, 13202, 14567, 16670},
			},
			[DDR_1600] = {
				.bw = {480, 768, 1920, 3360, 9216},
				.coef = {2334, 11838, 15836, 18198, 21684},
			},
			[DDR_1800] = {
				.bw = {560, 896, 2239, 3919, 10748},
				.coef = {2723, 13806, 18468, 21223, 25289},
			},
		},
		.coef_idle = {4009, 4219, 4452, 5192},
	},
	[APHY_VDDQ_0P6V] = {
		.read_pwr = {
			[DDR_800] = {
				.bw = {0, 320, 640, 1280, 1920},
				.coef = {0, 890, 1430, 1610, 1993},
			},
			[DDR_1200] = {
				.bw = {0, 480, 960, 1920, 2880},
				.coef = {0, 755, 1368, 1445, 1793},
			},
			[DDR_1600] = {
				.bw = {0, 640, 1280, 2560, 3840},
				.coef = {0, 445, 875, 857, 1157},
			},
			[DDR_1800] = {
				.bw = {0, 746, 1493, 2986, 4478},
				.coef = {0, 519, 1020, 999, 1349},
			},
		},
		.write_pwr = {
			[DDR_800] = {
				.bw = {320, 640, 1280, 1920, 3200},
				.coef = {1632, 3703, 4330, 5642, 7297},
			},
			[DDR_1200] = {
				.bw = {480, 960, 1920, 2880, 4800},
				.coef = {1628, 3640, 4555, 5547, 7682},
			},
			[DDR_1600] = {
				.bw = {0, 640, 1280, 2560, 3840},
				.coef = {0, 1942, 3915, 6565, 8895},
			},
			[DDR_1800] = {
				.bw = {0, 746, 1493, 2986, 4478},
				.coef = {0, 2264, 4566, 7656, 10374},
			},
		},
		.coef_idle = {53, 37, 25, 29},
	},
	[APHY_VM_1P1V] = {
		.read_pwr = {
			[DDR_800] = {
				.bw = {0, 320, 640, 1280, 1920},
				.coef = {0, 491, 699, 896, 955},
			},
			[DDR_1200] = {
				.bw = {0, 480, 960, 1920, 2880},
				.coef = {0, 656, 1132, 1227, 1376},
			},
			[DDR_1600] = {
				.bw = {0, 640, 1280, 2560, 3840},
				.coef = {0, 753, 1471, 1605, 1893},
			},
			[DDR_1800] = {
				.bw = {0, 746, 1493, 2986, 4478},
				.coef = {0, 878, 1715, 1872, 2207},
			},
		},
		.write_pwr = {
			[DDR_800] = {
				.bw = {320, 640, 1280, 1920, 3200},
				.coef = {792, 1345, 1902, 2486, 3265},
			},
			[DDR_1200] = {
				.bw = {480, 960, 1920, 2880, 4800},
				.coef = {1151, 1935, 2953, 3525, 4742},
			},
			[DDR_1600] = {
				.bw = {0, 640, 1280, 2560, 3840},
				.coef = {0, 1418, 2601, 3916, 4836},
			},
			[DDR_1800] = {
				.bw = {0, 746, 1493, 2986, 4478},
				.coef = {0, 1654, 3033, 4567, 5640},
			},
		},
		.coef_idle = {64, 64, 64, 74},
	},
	[APHY_VIO_1P8V] = {
		.read_pwr = {
			[DDR_800] = {
				.bw = {0, 320, 640, 1280, 1920},
				.coef = {0, 141, 276, 442, 558},
			},
			[DDR_1200] = {
				.bw = {0, 480, 960, 1920, 2880},
				.coef = {0, 151, 292, 461, 587},
			},
			[DDR_1600] = {
				.bw = {0, 640, 1280, 2560, 3840},
				.coef = {0, 152, 316, 465, 624},
			},
			[DDR_1800] = {
				.bw = {0, 746, 1493, 2986, 4478},
				.coef = {0, 178, 368, 542, 728},
			},
		},
		.write_pwr = {
			[DDR_800] = {
				.bw = {320, 640, 1280, 1920, 3200},
				.coef = {0, 0, 0, 0, 0},
			},
			[DDR_1200] = {
				.bw = {480, 960, 1920, 2880, 4800},
				.coef = {0, 0, 0, 0, 0},
			},
			[DDR_1600] = {
				.bw = {0, 640, 1280, 2560, 3840},
				.coef = {0, 0, 0, 0, 0},
			},
			[DDR_1800] = {
				.bw = {0, 746, 1493, 2986, 4478},
				.coef = {0, 0, 0, 0, 0},
			},
		},
		.coef_idle = {102, 175, 198, 231},
	},
};

static struct dram_pwr_conf dram_def_pwr_conf[] = {
	[DRAM_VDD1_1P8V] = {
		.i_dd0 = 10500,
		.i_dd2p = 600,
		.i_dd2n = 900,
		.i_dd4r = 14700,
		.i_dd4w = 16000,
		.i_dd5 = 4100,
		.i_dd6 = 900,
	},
	[DRAM_VDD2_1P1V] = {
		.i_dd0 = 48400,
		.i_dd2p = 600,
		.i_dd2n = 21100,
		.i_dd4r = 125700,
		.i_dd4w = 171500,
		.i_dd5 = 27100,
		.i_dd6 = 1100,
	},
	[DRAM_VDDQ_0P6V] = {
		.i_dd0 = 200,
		.i_dd2p = 200,
		.i_dd2n = 200,
		.i_dd4r = 153100,
		.i_dd4w = 600,
		.i_dd5 = 200,
		.i_dd6 = 100,
	},
};

/****************************************************************************
 *  Global Variables
 ****************************************************************************/

/****************************************************************************
 *  Static Function
 ****************************************************************************/

/***************************************************************************
 *  API
 ***************************************************************************/
int swpm_platform_init(void)
{
	int ret = 0;

	/* copy pwr data */
	memcpy(swpm_info_ref->aphy_pwr_tbl, aphy_def_pwr_tbl,
		sizeof(aphy_def_pwr_tbl));
	memcpy(swpm_info_ref->dram_conf, dram_def_pwr_conf,
		sizeof(dram_def_pwr_conf));

	swpm_info("copy pwr data (size: aphy/dram = %ld/%ld) done!\n",
		(unsigned long)sizeof(aphy_def_pwr_tbl),
		(unsigned long)sizeof(dram_def_pwr_conf));

	return ret;
}

void swpm_send_init_ipi(unsigned int addr, unsigned int size,
	unsigned int ch_num)
{
	struct qos_data qos_d;

	qos_d.cmd = QOS_IPI_SWPM_INIT;
	qos_d.u.swpm_init.dram_addr = addr;
	qos_d.u.swpm_init.dram_size = size;
	qos_d.u.swpm_init.dram_ch_num = ch_num;
	qos_ipi_to_sspm_command(&qos_d, 4);
}

