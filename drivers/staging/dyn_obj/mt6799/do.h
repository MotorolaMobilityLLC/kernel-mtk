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

typedef enum scp_id {
	SCP_A = 0,
	SCP_B,
	SCP_TOTAL_NUM,
} SCP_ID;

struct do_list_node {
	char *name;
	u32 scp_num;
	struct do_list_node *features;
	struct do_list_node *next;
};

/************ Public APIs **************/
int mt_do_load_do(char *do_name);
int mt_do_unload_do(char *do_name);
int mt_do_is_do_loaded(char *do_name);
int mt_do_is_do_loading(void);

struct do_list_node *mt_do_get_do_infos(void);
struct do_list_node *mt_do_get_loaded_do(SCP_ID id);
struct do_list_node *mt_do_get_features(char *do_name);
struct do_list_node *mt_do_get_do_with_feats(char **feat_list, int len);
