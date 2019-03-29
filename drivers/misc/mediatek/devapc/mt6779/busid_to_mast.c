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

#include <linux/kernel.h>
#include <linux/bug.h>

#include "busid_to_mast.h"
#include "devapc.h"

static struct PERIAXI_ID_INFO paxi_int_mi_id_to_master[] = {
	{"APDMA",       { 0, 1, 2, 0, 0 } },
	{"MD",          { 1, 0, 2, 2, 0 } },
	{"SPM",         { 0, 0, 1, 0, 0 } },
	{"CCU",         { 0, 0, 0, 0, 0 } },
	{"SPM",         { 0, 0, 0, 1, 0 } },
	{"N/A",         { 0, 0, 0, 0, 1 } },
	{"THERM",       { 0, 0, 0, 1, 1 } },
};

static struct TOPAXI_ID_INFO topaxi_mi0_id_to_master[] = {
	{"DebugTop",          { 1, 0, 0, 0,	0, 0, 2, 0,	0, 0, 0, 0 } },
	{"Audio",             { 1, 0, 0, 1,	0, 0, 0, 0,	0, 1, 0, 0 } },
	{"IPU",               { 1, 0, 0, 1,	0, 0, 0, 0,	0, 0, 1, 0 } },
	{"SPI1",              { 1, 0, 0, 1,	0, 0, 0, 0,	0, 1, 1, 0 } },
	{"SPI7",              { 1, 0, 0, 1,	0, 0, 0, 0,	0, 0, 0, 0 } },
	{"CCU",               { 1, 0, 0, 1,	0, 0, 1, 0,	0, 0, 0, 0 } },
	{"SPM ",              { 1, 0, 0, 1,	0, 0, 1, 0,	0, 1, 0, 0 } },
	{"N/A",               { 1, 0, 0, 1,	0, 0, 1, 0,	0, 0, 1, 0 } },
	{"THERM",             { 1, 0, 0, 1,	0, 0, 1, 0,	0, 1, 1, 0 } },
	{"UFS",               { 1, 0, 0, 1,	0, 0, 0, 1,	0, 2, 2, 0 } },
	{"DMA_EXT",           { 1, 0, 0, 1,	0, 0, 1, 1,	0, 2, 0, 0 } },
	{"SSUSB",             { 1, 0, 0, 1,	0, 0, 0, 0,	1, 0, 2, 2 } },
	{"PWM",               { 1, 0, 0, 1,	0, 0, 0, 0,	1, 1, 0, 0 } },
	{"MSDC1",             { 1, 0, 0, 1,	0, 0, 0, 0,	1, 1, 1, 0 } },
	{"SPI6",              { 1, 0, 0, 1,	0, 0, 0, 0,	1, 1, 0, 1 } },
	{"SPI0",              { 1, 0, 0, 1,	0, 0, 0, 0,	1, 1, 1, 1 } },
	{"SPI2",              { 1, 0, 0, 1,	0, 0, 1, 0,	1, 0, 0, 0 } },
	{"SPI3",              { 1, 0, 0, 1,	0, 0, 1, 0,	1, 1, 0, 0 } },
	{"SPI4",              { 1, 0, 0, 1,	0, 0, 1, 0,	1, 0, 1, 0 } },
	{"SPI5",              { 1, 0, 0, 1,	0, 0, 1, 0,	1, 1, 1, 0 } },
	{"MSDC0",             { 1, 0, 0, 1,	0, 0, 0, 1,	1, 2, 2, 2 } },
	{"DX_CC",             { 1, 0, 0, 0,	1, 0, 2, 2,	2, 2, 0, 0 } },
	{"CQ_DMA",            { 1, 0, 0, 1,	1, 0, 2, 2,	2, 0, 0, 0 } },
	{"GCE_M",             { 1, 0, 0, 0,	0, 1, 2, 2,	0, 0, 0, 0 } },
	{"HiFi3",             { 1, 1, 0, 0,	2, 2, 2, 2,	2, 0, 0, 0 } },
	{"CLDMA(dpmaif)",     { 1, 1, 0, 1,	2, 2, 2, 2,	0, 0, 0, 0 } },
	{"SSPM",              { 1, 0, 1, 1,	2, 2, 0, 0,	0, 0, 0, 0 } },
	{"SCP",               { 1, 0, 1, 0,	2, 2, 0, 0,	0, 0, 0, 0 } },
	{"CONNSYS",           { 1, 1, 1, 2,	2, 2, 0, 0,	0, 0, 0, 0 } },
	{"DSU_write",         { 0, 2, 2, 2,	2, 0, 0, 0,	0, 0, 0, 0 } },
	{"DSU_write",         { 0, 2, 2, 2,	2, 0, 0, 1,	0, 0, 0, 0 } },
	{"DSU_write",         { 0, 2, 2, 2,	2, 2, 2, 2,	2, 1, 0, 0 } },
	{"DSU_read",          { 0, 2, 2, 2,	2, 0, 0, 0,	0, 0, 0, 0 } },
	{"DSU_read",          { 0, 2, 2, 2,	2, 0, 0, 1,	0, 0, 0, 0 } },
	{"DSU_read",          { 0, 2, 0, 0,	0, 0, 0, 0,	1, 0, 0, 0 } },
	{"DSU_read",          { 0, 2, 1, 0,	0, 0, 0, 0,	1, 0, 0, 0 } },
	{"DSU_read",          { 0, 2, 2, 2,	2, 2, 2, 2,	2, 1, 0, 0 } },
	{"DSU_read",          { 0, 2, 2, 2,	2, 2, 2, 2,	2, 2, 1, 0 } },
};

static const char *topaxi_mi0_trans(int bus_id)
{
	const char *master = "UNKNOWN_MASTER_FROM_TOPAXI";
	int master_count = ARRAY_SIZE(topaxi_mi0_id_to_master);
	int i, j;

	for (i = 0 ; i < master_count; i++) {
		for (j = 0 ; j < TOPAXI_MI0_BIT_LENGTH ; j++) {
			if (topaxi_mi0_id_to_master[i].bit[j] == 2)
				continue;

			if (((bus_id >> j) & 0x1) ==
				topaxi_mi0_id_to_master[i].bit[j]) {
				continue;
			} else {
				break;
			}
		}
		if (j == TOPAXI_MI0_BIT_LENGTH) {
			DEVAPC_MSG("%s %s %s %s\n",
				"[DEVAPC]",
				"catch it from TOPAXI_MI0.",
				"Master is:",
				topaxi_mi0_id_to_master[i].master);
			master = topaxi_mi0_id_to_master[i].master;
		}
	}

	return master;
}

static const char *paxi_int_mi_trans(int bus_id)
{
	const char *master = "UNKNOWN_MASTER_FROM_PAXI";
	int master_count = ARRAY_SIZE(paxi_int_mi_id_to_master);
	int i, j;

	if ((bus_id & 0x3) == 0x3) {
		master = topaxi_mi0_trans(bus_id >> 2);
		return master;
	}

	for (i = 0 ; i < master_count; i++) {
		for (j = 0 ; j < PERIAXI_INT_MI_BIT_LENGTH ; j++) {
			if (paxi_int_mi_id_to_master[i].bit[j] == 2)
				continue;

			if (((bus_id >> j) & 0x1) ==
				paxi_int_mi_id_to_master[i].bit[j]) {
				continue;
			} else {
				break;
			}
		}
		if (j == PERIAXI_INT_MI_BIT_LENGTH) {
			DEVAPC_MSG("%s %s %s %s\n",
				"[DEVAPC]",
				"catch it from PERIAXI_INT_MI.",
				"Master is:",
				paxi_int_mi_id_to_master[i].master);
			master = paxi_int_mi_id_to_master[i].master;
		}
	}

	return master;
}

const char *bus_id_to_master(int bus_id, uint32_t vio_addr, int vio_idx)
{
	uint32_t h_byte;
	const char *master = "UNKNOWN_MASTER";

	DEVAPC_DBG_MSG("[DEVAPC] bus id = 0x%x, vio_addr = 0x%x\n",
		bus_id, vio_addr);

	/* SPM MTCMOS disable will set way_en[7:4] reg to block transaction,
	 * and it will triggered TOPAXI_SI0_DECERR instead of slave vio.
	 */
	if (vio_idx == TOPAXI_SI0_DECERR) {
		DEVAPC_DBG_MSG("[DEVAPC] vio is from TOPAXI_SI0_DECERR\n");
		master = topaxi_mi0_trans(bus_id);

		return master;
	}

	h_byte = (vio_addr >> 24) & 0xFF;

	/* to Infra/Peri/Audio/MD/CONN
	 * or MMSYS
	 * or MFG
	 */
	if (((h_byte >> 4) == 0x0) && h_byte != 0x0C && h_byte != 0x0D) {
		DEVAPC_DBG_MSG("[DEVAPC] vio addr is from on-chip SRAMROM\n");
		master = topaxi_mi0_trans(bus_id);

	} else if (h_byte == 0x10 || h_byte == 0x11 || h_byte == 0x18 ||
		h_byte == 0x0C || h_byte == 0x0D ||
		(h_byte >> 4) == 0x2 || (h_byte >> 4) == 0x3) {
		DEVAPC_DBG_MSG("[DEVAPC] vio addr is from Infra/Peri\n");

		/* CONN */
		if (h_byte == 0x18) {
			if ((bus_id & 0x1) == 1)
				return "CQ_DMA"; /*should not be walked */
			bus_id = bus_id >> 1;
		}
		/* HIFI3/SCP/SSPM */
		h_byte = (vio_addr >> 20) & 0xFFF;
		if (h_byte == 0x106 || h_byte == 0x105 || h_byte == 0x104) {
			if ((bus_id & 0x1) == 0)
				return "MD";
			bus_id = bus_id >> 1;
		}
		master = paxi_int_mi_trans(bus_id);

	} else if (h_byte == 0x14 || h_byte == 0x15 || h_byte == 0x16 ||
		h_byte == 0x17 || h_byte == 0x19 || h_byte == 0x1A ||
		h_byte == 0x1B) {
		DEVAPC_DBG_MSG("[DEVAPC] vio addr is from MM\n");
		if ((bus_id & 0x1) == 1)
			return "GCE";
		master = topaxi_mi0_trans(bus_id >> 1);

	} else if (h_byte == 0x13) {
		DEVAPC_DBG_MSG("[DEVAPC] vio addr is from MFG\n");
		master = topaxi_mi0_trans(bus_id);

	} else {
		master = "UNKNOWN_MASTER";
	}

	return master;
}

