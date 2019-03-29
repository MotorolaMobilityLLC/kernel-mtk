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

#ifndef __BUSID_TO_MAST_H__
#define __BUSID_TO_MAST_H__

const char *bus_id_to_master(int bus_id, uint32_t vio_addr, int vio_idx);

#define PERIAXI_INT_MI_BIT_LENGTH	5
#define TOPAXI_MI0_BIT_LENGTH		12
#define TOPAXI_SI0_DECERR		503

/* bit == 2 means don't care */
struct PERIAXI_ID_INFO {
	const char	*master;
	uint8_t		bit[PERIAXI_INT_MI_BIT_LENGTH];
};

struct TOPAXI_ID_INFO {
	const char	*master;
	uint8_t		bit[TOPAXI_MI0_BIT_LENGTH];
};

#endif /* __BUSID_TO_MAST_H__ */
