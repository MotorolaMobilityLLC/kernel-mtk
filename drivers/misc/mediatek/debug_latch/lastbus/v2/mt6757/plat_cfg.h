#include "../lastbus-mtk-common.h"

/* timeout set to around 130 ms */
#define PERI_TIMEOUT	0x3fff
#define PERI_ENABLE	0xc

const struct lastbus_plt_cfg plt_cfg = {
	.num_master_port = 3,
	.num_slave_port = 4,
	.num_perisys_mon = 5,
	.peri_enable_setting = PERI_ENABLE,
	.peri_timeout_setting = PERI_TIMEOUT,
	.mcusys_offsets = {
		.bus_mcu_m0 = 0x05A4,
		.bus_mcu_s1 = 0x05B0,
		.bus_mcu_m0_m = 0x05C0,
		.bus_mcu_s1_m = 0x05CC,
	},
	.perisys_offsets = {
		.bus_peri_r0 = 0x0500,
		.bus_peri_r1 = 0x0504,
		.bus_peri_mon = 0x0508,
	},
};
