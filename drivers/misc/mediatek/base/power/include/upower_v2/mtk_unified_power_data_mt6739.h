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

/* Ver: Zion_FY_UPT_20170414 */
/* Ver: Zion_SB_UPT_20170414 */

#ifndef MTK_UNIFIED_POWER_DATA_MT6739_H
#define MTK_UNIFIED_POWER_DATA_MT6739_H

/* Zion_FY_WAT0_85C */
struct upower_tbl upower_tbl_ll_FY = {
	.row = {
	{.cap = 240, .volt = 95000, .dyn_pwr = 28752, .lkg_pwr = {34612, 34612, 34612, 34612, 34612, 34612} },
	{.cap = 272, .volt = 95000, .dyn_pwr = 32503, .lkg_pwr = {34612, 34612, 34612, 34612, 34612, 34612} },
	{.cap = 334, .volt = 95000, .dyn_pwr = 40003, .lkg_pwr = {34612, 34612, 34612, 34612, 34612, 34612} },
	{.cap = 439, .volt = 95000, .dyn_pwr = 52504, .lkg_pwr = {34612, 34612, 34612, 34612, 34612, 34612} },
	{.cap = 502, .volt = 97100, .dyn_pwr = 62687, .lkg_pwr = {35836, 35836, 35836, 35836, 35836, 35836} },
	{.cap = 564, .volt = 99400, .dyn_pwr = 73903, .lkg_pwr = {37177, 37177, 37177, 37177, 37177, 37177} },
	{.cap = 679, .volt = 103500, .dyn_pwr = 96448, .lkg_pwr = {39585, 39585, 39585, 39585, 39585, 39585} },
	{.cap = 731, .volt = 105200, .dyn_pwr = 107307, .lkg_pwr = {40584, 40584, 40584, 40584, 40584, 40584} },
	{.cap = 773, .volt = 107000, .dyn_pwr = 117354, .lkg_pwr = {41635, 41635, 41635, 41635, 41635, 41635} },
	{.cap = 805, .volt = 108700, .dyn_pwr = 126022, .lkg_pwr = {42629, 42629, 42629, 42629, 42629, 42629} },
	{.cap = 846, .volt = 111000, .dyn_pwr = 138238, .lkg_pwr = {43984, 43984, 43984, 43984, 43984, 43984} },
	{.cap = 888, .volt = 113300, .dyn_pwr = 151139, .lkg_pwr = {45355, 45355, 45355, 45355, 45355, 45355} },
	{.cap = 940, .volt = 115600, .dyn_pwr = 166592, .lkg_pwr = {46749, 46749, 46749, 46749, 46749, 46749} },
	{.cap = 961, .volt = 117400, .dyn_pwr = 175639, .lkg_pwr = {47892, 47892, 47892, 47892, 47892, 47892} },
	{.cap = 993, .volt = 119100, .dyn_pwr = 186657, .lkg_pwr = {48972, 48972, 48972, 48972, 48972, 48972} },
	{.cap = 1024, .volt = 120800, .dyn_pwr = 198088, .lkg_pwr = {50093, 50093, 50093, 50093, 50093, 50093} },
	},
	.lkg_idx = DEFAULT_LKG_IDX,
	.row_num = UPOWER_OPP_NUM,
	.nr_idle_states = NR_UPOWER_CSTATES,
	.idle_states = {
		{{0}, {18433} },
		{{0}, {18433} },
		{{0}, {18433} },
		{{0}, {18433} },
		{{0}, {18433} },
		{{0}, {18433} },
	},
};

struct upower_tbl upower_tbl_cluster_ll_FY = {
	.row = {
	{.cap = 240, .volt = 95000, .dyn_pwr = 7004, .lkg_pwr = {16461, 16461, 16461, 16461, 16461, 16461} },
	{.cap = 272, .volt = 95000, .dyn_pwr = 7917, .lkg_pwr = {16461, 16461, 16461, 16461, 16461, 16461} },
	{.cap = 334, .volt = 95000, .dyn_pwr = 9744, .lkg_pwr = {16461, 16461, 16461, 16461, 16461, 16461} },
	{.cap = 439, .volt = 95000, .dyn_pwr = 12789, .lkg_pwr = {16461, 16461, 16461, 16461, 16461, 16461} },
	{.cap = 502, .volt = 97100, .dyn_pwr = 15270, .lkg_pwr = {17269, 17269, 17269, 17269, 17269, 17269} },
	{.cap = 564, .volt = 99400, .dyn_pwr = 18002, .lkg_pwr = {18153, 18153, 18153, 18153, 18153, 18153} },
	{.cap = 679, .volt = 103500, .dyn_pwr = 23494, .lkg_pwr = {19991, 19991, 19991, 19991, 19991, 19991} },
	{.cap = 731, .volt = 105200, .dyn_pwr = 26139, .lkg_pwr = {20803, 20803, 20803, 20803, 20803, 20803} },
	{.cap = 773, .volt = 107000, .dyn_pwr = 28586, .lkg_pwr = {21917, 21917, 21917, 21917, 21917, 21917} },
	{.cap = 805, .volt = 108700, .dyn_pwr = 30698, .lkg_pwr = {22969, 22969, 22969, 22969, 22969, 22969} },
	{.cap = 846, .volt = 111000, .dyn_pwr = 33673, .lkg_pwr = {24546, 24546, 24546, 24546, 24546, 24546} },
	{.cap = 888, .volt = 113300, .dyn_pwr = 36816, .lkg_pwr = {26325, 26325, 26325, 26325, 26325, 26325} },
	{.cap = 940, .volt = 115600, .dyn_pwr = 40580, .lkg_pwr = {28176, 28176, 28176, 28176, 28176, 28176} },
	{.cap = 961, .volt = 117400, .dyn_pwr = 42784, .lkg_pwr = {29784, 29784, 29784, 29784, 29784, 29784} },
	{.cap = 993, .volt = 119100, .dyn_pwr = 45468, .lkg_pwr = {31303, 31303, 31303, 31303, 31303, 31303} },
	{.cap = 1024, .volt = 120800, .dyn_pwr = 48252, .lkg_pwr = {32917, 32917, 32917, 32917, 32917, 32917} },
	},
	.lkg_idx = DEFAULT_LKG_IDX,
	.row_num = UPOWER_OPP_NUM,
	.nr_idle_states = NR_UPOWER_CSTATES,
	.idle_states = {
		{{0}, {9806} },
		{{0}, {9806} },
		{{0}, {9806} },
		{{0}, {9806} },
		{{0}, {9806} },
		{{0}, {9806} },
	},
};


/* Zion_SB_WAT0_85C */
struct upower_tbl upower_tbl_ll_SB = {
	.row = {
	{.cap = 191, .volt = 95000, .dyn_pwr = 28752, .lkg_pwr = {34612, 34612, 34612, 34612, 34612, 34612} },
	{.cap = 216, .volt = 97500, .dyn_pwr = 34236, .lkg_pwr = {36069, 36069, 36069, 36069, 36069, 36069} },
	{.cap = 266, .volt = 100000, .dyn_pwr = 44325, .lkg_pwr = {37527, 37527, 37527, 37527, 37527, 37527} },
	{.cap = 350, .volt = 102500, .dyn_pwr = 61122, .lkg_pwr = {38997, 38997, 38997, 38997, 38997, 38997} },
	{.cap = 400, .volt = 105000, .dyn_pwr = 73302, .lkg_pwr = {40467, 40467, 40467, 40467, 40467, 40467} },
	{.cap = 450, .volt = 107500, .dyn_pwr = 86439, .lkg_pwr = {41927, 41927, 41927, 41927, 41927, 41927} },
	{.cap = 500, .volt = 110000, .dyn_pwr = 100562, .lkg_pwr = {43388, 43388, 43388, 43388, 43388, 43388} },
	{.cap = 549, .volt = 112500, .dyn_pwr = 115703, .lkg_pwr = {44878, 44878, 44878, 44878, 44878, 44878} },
	{.cap = 608, .volt = 115000, .dyn_pwr = 133726, .lkg_pwr = {46368, 46368, 46368, 46368, 46368, 46368} },
	{.cap = 641, .volt = 117500, .dyn_pwr = 147253, .lkg_pwr = {47956, 47956, 47956, 47956, 47956, 47956} },
	{.cap = 674, .volt = 120000, .dyn_pwr = 161564, .lkg_pwr = {49544, 49544, 49544, 49544, 49544, 49544} },
	{.cap = 708, .volt = 122500, .dyn_pwr = 176680, .lkg_pwr = {51260, 51260, 51260, 51260, 51260, 51260} },
	{.cap = 749, .volt = 125000, .dyn_pwr = 194787, .lkg_pwr = {52975, 52975, 52975, 52975, 52975, 52975} },
	{.cap = 841, .volt = 126900, .dyn_pwr = 225290, .lkg_pwr = {54895, 54895, 54895, 54895, 54895, 54895} },
	{.cap = 932, .volt = 128700, .dyn_pwr = 256964, .lkg_pwr = {56713, 56713, 56713, 56713, 56713, 56713} },
	{.cap = 1024, .volt = 130600, .dyn_pwr = 290595, .lkg_pwr = {58569, 58569, 58569, 58569, 58569, 58569} },
	},
	.lkg_idx = DEFAULT_LKG_IDX,
	.row_num = UPOWER_OPP_NUM,
	.nr_idle_states = NR_UPOWER_CSTATES,
	.idle_states = {
		{{0}, {18433} },
		{{0}, {18433} },
		{{0}, {18433} },
		{{0}, {18433} },
		{{0}, {18433} },
		{{0}, {18433} },
	},
};

struct upower_tbl upower_tbl_cluster_ll_SB = {
	.row = {
	{.cap = 191, .volt = 95000, .dyn_pwr = 7004, .lkg_pwr = {16461, 16461, 16461, 16461, 16461, 16461} },
	{.cap = 216, .volt = 97500, .dyn_pwr = 8339, .lkg_pwr = {17422, 17422, 17422, 17422, 17422, 17422} },
	{.cap = 266, .volt = 100000, .dyn_pwr = 10797, .lkg_pwr = {18384, 18384, 18384, 18384, 18384, 18384} },
	{.cap = 350, .volt = 102500, .dyn_pwr = 14889, .lkg_pwr = {19532, 19532, 19532, 19532, 19532, 19532} },
	{.cap = 400, .volt = 105000, .dyn_pwr = 17856, .lkg_pwr = {20679, 20679, 20679, 20679, 20679, 20679} },
	{.cap = 450, .volt = 107500, .dyn_pwr = 21056, .lkg_pwr = {22226, 22226, 22226, 22226, 22226, 22226} },
	{.cap = 500, .volt = 110000, .dyn_pwr = 24496, .lkg_pwr = {23773, 23773, 23773, 23773, 23773, 23773} },
	{.cap = 549, .volt = 112500, .dyn_pwr = 28184, .lkg_pwr = {25706, 25706, 25706, 25706, 25706, 25706} },
	{.cap = 608, .volt = 115000, .dyn_pwr = 32574, .lkg_pwr = {27640, 27640, 27640, 27640, 27640, 27640} },
	{.cap = 641, .volt = 117500, .dyn_pwr = 35869, .lkg_pwr = {29873, 29873, 29873, 29873, 29873, 29873} },
	{.cap = 674, .volt = 120000, .dyn_pwr = 39355, .lkg_pwr = {32107, 32107, 32107, 32107, 32107, 32107} },
	{.cap = 708, .volt = 122500, .dyn_pwr = 43038, .lkg_pwr = {34636, 34636, 34636, 34636, 34636, 34636} },
	{.cap = 749, .volt = 125000, .dyn_pwr = 47448, .lkg_pwr = {37165, 37165, 37165, 37165, 37165, 37165} },
	{.cap = 841, .volt = 126900, .dyn_pwr = 54878, .lkg_pwr = {38257, 38257, 38257, 38257, 38257, 38257} },
	{.cap = 932, .volt = 128700, .dyn_pwr = 62594, .lkg_pwr = {39291, 39291, 39291, 39291, 39291, 39291} },
	{.cap = 1024, .volt = 130600, .dyn_pwr = 70786, .lkg_pwr = {40667, 40667, 40667, 40667, 40667, 40667} },
	},
	.lkg_idx = DEFAULT_LKG_IDX,
	.row_num = UPOWER_OPP_NUM,
	.nr_idle_states = NR_UPOWER_CSTATES,
	.idle_states = {
		{{0}, {9806} },
		{{0}, {9806} },
		{{0}, {9806} },
		{{0}, {9806} },
		{{0}, {9806} },
		{{0}, {9806} },
	},
};

#endif /* MTK_UNIFIED_POWER_DATA_MT6739_H */
