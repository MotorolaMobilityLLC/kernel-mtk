/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PARSER_H
#define __PARSER_H
#include "config.h"

#define BENCHMARK_KEY_NAME "benchmark_data"
#define NODE_TYPE_KEY_NAME "node type"
#define VALUE 0

enum mp_test_catalog {
	MUTUAL_TEST = 0,
	SELF_TEST = 1,
	KEY_TEST = 2,
	ST_TEST = 3,
	TX_RX_DELTA = 4,
	UNTOUCH_P2P = 5,
	PIXEL = 6,
	OPEN_TEST = 7,
	PEAK_TO_PEAK_TEST = 8,
	SHORT_TEST = 9,
};


extern int core_parser_get_tdf_value(char *str, int catalog);
extern void core_parser_nodetype(int32_t *type_ptr, char *desp, size_t frame_len);
extern void core_parser_benchmark(int32_t *max_ptr, int32_t *min_ptr, int8_t type, char *desp, size_t frame_len);
extern int core_parser_get_u8_array(char *key, uint8_t *buf, uint16_t base, size_t len);
extern int core_parser_get_int_data(char *section, char *keyname, char *rv);
extern int core_parser_path(char *path);

#endif
