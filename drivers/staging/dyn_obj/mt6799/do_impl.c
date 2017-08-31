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

#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include "do_impl.h"
#include "do_ipi.h"

#define TIMEOUT_TICK 5000 /* ~= 5s */

struct do_item *dos;
struct do_item *itail;
struct do_list_node *do_infos;
struct do_list_node *do_infos_tail;
struct do_list_node *ret_infos;
struct do_list_node *current_do[SCP_CORE_TOTAL];
char *do_info_buf;

int inited[SCP_CORE_TOTAL] = {0};
int is_loading;
struct mutex do_mutex; /* For do loading critical section */
struct completion loading;

/********************** API helpers *********************/
void clear_info(struct do_list_node *root, int deep_clean)
{
	struct do_list_node *del;
	struct do_list_node *ptr = root;

	if (!root)
		return;

	while (ptr) {
		del = ptr;
		ptr = ptr->next;
		if (deep_clean) {
			if (ptr->features)
				clear_info(ptr->features, deep_clean);
		}
		kfree(del);
	}
	root = NULL;
}

int contains_feat(struct do_list_node *features, char *target)
{
	struct do_list_node *ptr = features;

	while (ptr) {
		if (!strcmp(ptr->name, target))
			return 1;
		ptr = ptr->next;
	}
	return 0;
}

struct do_list_node *get_do(const char *target)
{
	struct do_list_node *ptr = do_infos;

	while (ptr) {
		if (!strcmp(ptr->name, target))
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

struct do_item *get_detail_do_infos(void)
{
	return dos;
}

/* scp_update_current_do
 * called from ISR
 */
void scp_update_current_do(char *str, u32 len, u32 scp)
{
	struct do_list_node *new;

	/* check scp number */
	if (scp >= SCP_CORE_TOTAL) {
		pr_err("update_current_do: scp num %u >= SCP_CORE_TOTAL: %u\n", scp, SCP_CORE_TOTAL);
		return;
	}

	/* case: unload do notification */
	if (len <= 0) {
		pr_debug("update_current_do: scp unload DO from core %u\n", scp);
		current_do[scp] = NULL;
		complete(&loading);
		return;
	}

	/* case: load do notification */
	new = get_do(str);
	if (new) {
		current_do[scp] = new;
		pr_debug("update_current_do: scp load DO %s to core %u\n", new->name, scp);

		if (!inited[scp]) {
			inited[scp] = 1;
			pr_debug("update_current_do: enable SCP %u DO APIs\n", scp);
#ifdef INIT_TIME_VERIFY
			complete(&loading);
#endif
			return; /* no need to pull up semaphore */
		}
		complete(&loading);
	} else {
		pr_err("invalid do name\n");
	}
}

/********************** Public APIs *********************/
struct do_list_node *mt_do_get_do_infos(void)
{
	return do_infos;
}

struct do_list_node *mt_do_get_loaded_do(SCP_ID id)
{
	return current_do[id];
}

struct do_list_node *mt_do_get_features(char *do_name)
{
	struct do_list_node *ptr = do_infos;

	while (ptr) {
		if (!strcmp(ptr->name, do_name))
			return ptr->features;
		ptr = ptr->next;
	}
	return NULL;
}

struct do_list_node *mt_do_get_do_with_feats(char **feat_list, int len)
{
	int i, ret;
	struct do_list_node *ptr = do_infos;
	struct do_list_node *tail = ret_infos;

	clear_info(ret_infos, 0);

	while (ptr) {
		ret = 1;
		/* for each name in feat_list */
		for (i = 0; i < len; i++) {
			if (!contains_feat(ptr->features, feat_list[i])) {
				ret = 0;
				break;
			}
		}
		if (ret) {
			/* insert node to ret_infos */
			if (!ret_infos) {
				ret_infos = kmalloc(sizeof(*ret_infos), GFP_KERNEL);
				tail = ret_infos;
			} else {
				tail->next = kmalloc(sizeof(*tail->next), GFP_KERNEL);
				tail = tail->next;
			}
			tail->name = ptr->name;
			tail->features = ptr->features;
			tail->next = NULL;
		}
		ptr = ptr->next;
	}
	return ret_infos;
}

/*
 * mt_do_load_do:
 * use IPI to make scp load the target DO
 * @param:
 * do_name: DO name string
 * wait: do_name
 * @ret: 1 = success
 *	   0 = failed
 */
int mt_do_load_do(char *do_name)
{
	int ret, res = 1;
	struct do_list_node *node;
	u32 scp;

	/* check if in loading state */
	if (is_loading) {
		pr_debug("mt_do_load_do: in loading state, return\n");
		return 0;
	}
	/* check if the do_name is in the do list first */
	node = get_do(do_name);
	if (!node) {
		pr_debug("mt_do_load_do: %s not in list\n", do_name);
		return 0;
	}

	scp = node->scp_num;
	/* check if DO info are already sent to scp(should not happen) */
	if (scp > SCP_CORE_TOTAL) {
		pr_err("mt_do_load_do: scp num > SCP_CORE_TOTAL, return\n");
		return 0;
	}
	if (!inited[scp])
		return 0;

	/* check current_do */
	if (current_do[scp] && !strcmp(current_do[scp]->name, do_name)) {
		pr_debug("mt_do_load_do: %s is already loaded in core %u\n", do_name, scp);
		return 0;
	}

	/***** critical section *****/
	ret = mutex_trylock(&do_mutex);
	if (!ret) {
		pr_debug("mt_do_load_do: try lock failed - in loading state, return\n");
		return 0;
	}
	is_loading = 1;

	pr_debug("mt_do_load_do: load DO %s to scp %u\n", do_name, scp);
	ret = do_ipi_send_do_name(do_name, 1, scp);
	if (!ret) {
		pr_err("mt_do_load_do: IPI timeout\n");
		res = 0;
	} else if (ret == -1) {
		pr_err("mt_do_load_do: IPI error\n");
		res = 0;
	} else if (!wait_for_completion_timeout(&loading, TIMEOUT_TICK)) {
		pr_err("mt_do_load_do: wait IPI timeout(max tick:%d)\n", TIMEOUT_TICK);
		res = 0;
	}
	/* check the updated DO name after scp ACK */
	if (res && (!current_do[scp] || strcmp(do_name, current_do[scp]->name))) {
		pr_err("mt_do_load_do: loaded do not matched\n");
		if (current_do[scp] && current_do[scp]->name)
			pr_err("mt_do_load_do: loaded do name not matched: %s\n", current_do[scp]->name);
		res = 0;
	}

	is_loading = 0;
	mutex_unlock(&do_mutex);
	/***** end critical section *****/

	if (res)
		pr_debug("mt_do_load_do: load DO %s done\n", do_name);
	return res;
}

int mt_do_unload_do(char *do_name)
{
	int ret, res = 1;
	u32 scp;
	struct do_list_node *node;

	/* check if in loading state */
	if (is_loading) {
		pr_debug("mt_do_unload_do: in loading state, return\n");
		return 0;
	}
	/* check if the do_name is in the do list first */
	node = get_do(do_name);
	if (!node) {
		pr_debug("mt_do_unload_do: %s not in list\n", do_name);
		return 0;
	}
	scp = node->scp_num;

	/* check if DO info are already sent to scp(should not happen) */
	if (scp > SCP_CORE_TOTAL) {
		pr_err("mt_do_unload_do: scp num %u > SCP_CORE_TOTAL, return\n", scp);
		return 0;
	}
	if (!inited[scp])
		return 0;

	/* check current_do */
	if (!current_do[scp]) {
		pr_debug("mt_do_unload_do: no DO loaded\n");
		return 0;
	}
	if (current_do[scp] && strcmp(current_do[scp]->name, do_name)) {
		pr_debug("mt_do_unload_do: %s is not loaded\n", do_name);
		return 0;
	}

	/***** critical section *****/
	ret = mutex_trylock(&do_mutex);
	if (!ret) {
		pr_debug("mt_do_unload_do: try lock failed - in loading state, return\n");
		return 0;
	}
	is_loading = 1;

	pr_debug("mt_do_unload_do: try unload %s\n", do_name);
	ret = do_ipi_send_do_name(do_name, 0, scp);
	if (!ret) {
		pr_err("mt_do_unload_do: IPI timeout\n");
		res = 0;
	} else if (ret == -1) {
		pr_err("mt_do_unload_do: IPI error\n");
		res = 0;
	} else if (!wait_for_completion_timeout(&loading, TIMEOUT_TICK)) {
		pr_err("mt_do_unload_do: wait IPI timeout(max tick:%d)\n", TIMEOUT_TICK);
		res = 0;
	}
	/* check the updated DO name after scp ACK */
	if (res && current_do[scp]) {
		pr_err("mt_do_unload_do: DO not unloaded: %s\n", current_do[scp]->name);
		res = 0;
	}

	is_loading = 0;
	mutex_unlock(&do_mutex);
	/***** end critical section *****/

	if (res)
		pr_debug("mt_do_unload_do: unload DO %s done\n", do_name);
	return res;
}

int mt_do_is_do_loaded(char *do_name)
{
	if (current_do[SCP_A_ID]) {
		if (!strcmp(current_do[SCP_A_ID]->name, do_name))
			return 1;
	} else if (current_do[SCP_B_ID]) {
		if (!strcmp(current_do[SCP_B_ID]->name, do_name))
			return 1;
	}
	return 0;
}

int mt_do_is_do_loading(void)
{
	return is_loading;
}

/************** Init DO infos  ***************/
static void insert_do(struct do_header *header, char *feat_list, u32 id)
{
	char *pch;
	char *sepstr;
	struct do_list_node *tail = NULL;

	/* insert struct do_item node */
	if (!dos) {
		dos = kmalloc(sizeof(*dos), GFP_KERNEL);
		itail = dos;
	} else {
		itail->next = kmalloc(sizeof(*itail->next), GFP_KERNEL);
		itail = itail->next;
	}

	itail->header = header;
	itail->next = NULL;

	sepstr = feat_list;
	pch = strsep(&sepstr, ",");
	while (pch) {
		/* insert node to feature list */
		if (!tail) {
			itail->feat_list = kmalloc(sizeof(*itail->feat_list), GFP_KERNEL);
			tail = itail->feat_list;
		} else {
			tail->next = kmalloc(sizeof(*tail->next), GFP_KERNEL);
			tail = tail->next;
		}

		tail->name = pch;
		tail->scp_num = id;
		tail->features = NULL;
		tail->next = NULL;

		pch = strsep(&sepstr, ",");
	}

	/* insert to do_infos */
	if (!do_infos) {
		do_infos = kmalloc(sizeof(*do_infos), GFP_KERNEL);
		do_infos_tail = do_infos;
	} else {
		do_infos_tail->next = kmalloc(sizeof(*do_infos_tail->next), GFP_KERNEL);
		do_infos_tail = do_infos_tail->next;
	}
	do_infos_tail->name = header->id;
	do_infos_tail->scp_num = id;
	do_infos_tail->features = itail->feat_list;
	do_infos_tail->next = NULL;
}

u32 mt_do_init_infos(struct do_info *info, u32 len, u32 scp)
{
	int i;
	struct do_header *info_header;
	char *features;
	u32 total_num_do, featlist_len;

	if (!info || info->num_of_do <= 0) {
		pr_warn("no DO to parse\n\r");
		return 0;
	}

	total_num_do = info->num_of_do;

	/* copy all header data into do_info_buf */
	do_info_buf = kmalloc(len, GFP_KERNEL);
	memcpy(do_info_buf, &info->headers, len - 4);

	/* parse the headers */
	info_header = (struct do_header *)do_info_buf;
	for (i = 0; i < total_num_do; i++) {
		/* get feature list string */
		featlist_len = info_header->featlist_len;
		features = (char *)info_header + sizeof(struct do_header);

		pr_debug("DO name: %s, dram: 0x%x, size: %u, scp num: %u\n",
			 (char *)info_header->id, info_header->dram_addr, info_header->size, scp);

		insert_do(info_header, features, scp);
		/* point to next header */
		info_header = (struct do_header *)(features + featlist_len);
	}

	pr_debug("Total number of DO: %d\n\r", total_num_do);
	return total_num_do;
}

void set_do_api_enable(void)
{
#ifdef INIT_TIME_VERIFY
	struct do_list_node *node = NULL;
#endif
	mutex_init(&do_mutex);
	init_completion(&loading);
	current_do[SCP_A_ID] = NULL;
	current_do[SCP_B_ID] = NULL;

#ifdef INIT_TIME_VERIFY
	/* NOTE: this will harm init time */
	pr_debug("[VERIFYY]set api enable: wait for first loaded DO\n");
	if (wait_for_completion_interruptible(&loading)) {
		pr_err("set api enable: semaphore down error\n");
		return;
	}

	node = mt_do_get_do_infos();
	while (node) {
		pr_debug("[VERIFYY]get infos: DO %s at scp %u\n", node->name, node->scp_num);
		node = node->next;
	}
	mt_do_load_do("SAMPLE2");
	node = mt_do_get_loaded_do(SCP_B_ID);
	if (node)
		pr_debug("[VERIFYY](1)B loaded DO: %s\n", node->name);

	mt_do_load_do("SAMPLE1");
	node = mt_do_get_loaded_do(SCP_B_ID);
	if (node)
		pr_debug("[VERIFYY](2)B loaded DO: %s\n", node->name);
#endif
}

void reset_do_api(void)
{
	int i;

	for (i = 0; i < SCP_CORE_TOTAL; i++)
		inited[i] = 0;
}
/************** deinit DO infos  ***************/
void deinit_do_infos(void)
{
	clear_info(ret_infos, 0);
	clear_info(do_infos, 1);
}
