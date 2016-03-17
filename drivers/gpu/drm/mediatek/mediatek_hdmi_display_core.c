/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "mediatek_hdmi_display_core.h"

static LIST_HEAD(display_nodes);

struct mediatek_hdmi_display_node *mtk_hdmi_display_create_node(
	struct mediatek_hdmi_display_ops *display_ops,
	struct device_node *np, void *data)
{
	struct mediatek_hdmi_display_node *display_node = NULL;

	display_node =
	    kzalloc(sizeof(struct mediatek_hdmi_display_node), GFP_KERNEL);
	if (!display_node) {
		mtk_hdmi_err("alloc mediatek hdmi display node failed!\n");
		return ERR_PTR(-ENOMEM);
	}

	display_node->display_ops = display_ops;
	display_node->np = np;
	display_node->private_data = data;
	display_node->pre_node = NULL;
	display_node->next_node = NULL;

	list_add_tail(&display_node->node, &display_nodes);

	return display_node;
}

struct mediatek_hdmi_display_node *mtk_hdmi_display_find_node(struct device_node
							      *np)
{
	struct mediatek_hdmi_display_node *display_node;

	list_for_each_entry(display_node, &display_nodes, node) {
		if (display_node->np == np)
			return display_node;
	}

	return NULL;
}

static struct mediatek_hdmi_display_node *mtk_hdmi_display_get_pre_node(
	struct mediatek_hdmi_display_node *display_node)
{
	if (display_node != NULL && display_node->pre_node != NULL)
		return display_node->pre_node;

	return NULL;
}

static struct mediatek_hdmi_display_node *mtk_hdmi_display_get_next_node(
	struct mediatek_hdmi_display_node *display_node)
{
	if (display_node != NULL && display_node->next_node != NULL)
		return display_node->next_node;

	return NULL;
}

int mtk_hdmi_display_add_pre_node(struct mediatek_hdmi_display_node *node,
				  struct mediatek_hdmi_display_node *pre_node)
{
	if (node != NULL && pre_node != NULL) {
		node->pre_node = pre_node;
		pre_node->next_node = node;
		return 0;
	}

	return -EINVAL;
}

bool mtk_hdmi_display_init(struct mediatek_hdmi_display_node *display_node)
{
	struct mediatek_hdmi_display_node *node = display_node;
	struct mediatek_hdmi_display_node *pre_node = NULL;
	bool ret = true;

	while (node != NULL) {
		mtk_hdmi_info("node : %p", node);
		pre_node = node;
		node = mtk_hdmi_display_get_pre_node(node);
	}

	node = pre_node;

	while (node != NULL) {
		mtk_hdmi_info
		    ("node:%p,display_ops: %p,display_ops->init:%p\n",
		     node, node->display_ops, node->display_ops->init);
		if (node != NULL && node->display_ops != NULL &&
		    node->display_ops->init != NULL) {
			ret &= node->display_ops->init(node->private_data);
		}
		node = mtk_hdmi_display_get_next_node(node);
	}

	return ret;
}

bool mtk_hdmi_display_set_vid_format(struct mediatek_hdmi_display_node *
				     display_node,
				     struct drm_display_mode *mode)
{
	struct mediatek_hdmi_display_node *node = display_node;
	struct mediatek_hdmi_display_node *pre_node = NULL;
	bool ret = true;

	while (node != NULL) {
		mtk_hdmi_info("node : %p\n", node);
		pre_node = node;
		node = mtk_hdmi_display_get_pre_node(node);
	}
	node = pre_node;

	while (node != NULL) {
		mtk_hdmi_info
		    ("node:%p,display_ops:%p,display_ops->set_format:%p\n",
		     node, node->display_ops, node->display_ops->set_format);
		if (node != NULL && node->display_ops != NULL &&
		    node->display_ops->set_format != NULL) {
			ret &=
			    node->display_ops->set_format(mode,
							  node->private_data);
		}
		node = mtk_hdmi_display_get_next_node(node);
	}

	return ret;
}

void mtk_hdmi_display_del_node(struct mediatek_hdmi_display_node *display_node)
{
	if (display_node != NULL)
		kfree(display_node);
}
