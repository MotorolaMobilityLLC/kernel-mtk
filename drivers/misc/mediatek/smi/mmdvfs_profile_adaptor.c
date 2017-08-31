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

#include <linux/uaccess.h>
#include <linux/clk.h>
#include <mtk_vcorefs_manager.h>

#include "mmdvfs_config_mt6799.h"
#include "mtk_smi.h"
#include "mmdvfs_mgr.h"



/* Class: mmdvfs_step_util */
static int mmdvfs_get_legacy_mmclk_step_from_mmclk_opp(
struct mmdvfs_step_util *self, int mmclk_step);
static int mmdvfs_get_opp_from_legacy_step(struct mmdvfs_step_util *self,
int legacy_step);
static void mmdvfs_step_util_init(struct mmdvfs_step_util *self);
static int mmdvfs_step_util_set_step(struct mmdvfs_step_util *self, int step,
int client_id);

struct mmdvfs_step_util mmdvfs_step_util_obj = {
	{0},
	MMDVFS_SCEN_COUNT,
	{0},
	MMDVFS_OPP_MAX,
	mmdvfs_legacy_step_to_opp,
	MMDVFS_VOLTAGE_COUNT,
	mmdvfs_mmclk_opp_to_legacy_mmclk_step,
	MMDVFS_OPP_MAX,
	mmdvfs_step_util_init,
	mmdvfs_get_legacy_mmclk_step_from_mmclk_opp,
	mmdvfs_get_opp_from_legacy_step,
	mmdvfs_step_util_set_step
};

/* Class: mmdvfs_adaptor */
static void mmdvfs_single_profile_dump(struct mmdvfs_profile *profile);
static void mmdvfs_profile_dump(struct mmdvfs_adaptor *self);
static void mmdvfs_single_hw_configuration_dump(struct mmdvfs_adaptor *self,
struct mmdvfs_hw_configurtion *hw_configuration);
static void mmdvfs_hw_configuration_dump(struct mmdvfs_adaptor *self);
static int mmdvfs_determine_step(struct mmdvfs_adaptor *self, int smi_scenario,
struct mmdvfs_cam_property *cam_setting,
struct mmdvfs_video_property *codec_setting);
static int mmdvfs_apply_hw_configurtion_by_step(struct mmdvfs_adaptor *self, int mmdvfs_step, int current_step);
static int mmdvfs_apply_vcore_hw_configurtion_by_step(struct mmdvfs_adaptor *self, int mmdvfs_step);
static int mmdvfs_apply_clk_hw_configurtion_by_step(struct mmdvfs_adaptor *self, int mmdvfs_step);
static int mmdvfs_get_cam_sys_clk(struct mmdvfs_adaptor *self, int mmdvfs_step);

static int is_camera_profile_matched(struct mmdvfs_cam_property *cam_setting,
struct mmdvfs_cam_property *profile_property);

static int is_video_profile_matched(
struct mmdvfs_video_property *video_setting,
struct mmdvfs_video_property *profile_property);

struct mmdvfs_adaptor mmdvfs_adaptor_obj = {
	0, 0, 0,
	mmdvfs_clk_sources_setting, MMDVFS_CLK_SOURCE_NUM,
	mmdvfs_clk_hw_map_setting, MMDVFS_CLK_MUX_NUM,
	mmdvfs_step_to_profile_mappings_setting, MMDVFS_OPP_MAX,
	MMDVFS_SMI_USER_CONTROL_SCEN_MASK,
	mmdvfs_profile_dump,
	mmdvfs_single_hw_configuration_dump,
	mmdvfs_hw_configuration_dump,
	mmdvfs_determine_step,
	mmdvfs_apply_hw_configurtion_by_step,
	mmdvfs_apply_vcore_hw_configurtion_by_step,
	mmdvfs_apply_clk_hw_configurtion_by_step,
	mmdvfs_get_cam_sys_clk,
	mmdvfs_single_profile_dump,
};

/* Member function implementation */
static int mmdvfs_apply_hw_configurtion_by_step(struct mmdvfs_adaptor *self,
int mmdvfs_step, const int current_step)
{
	MMDVFSMSG("current = %d, target = %d\n", current_step, mmdvfs_step);
	if (current_step == -1 || ((mmdvfs_step != -1) && (mmdvfs_step
	<= current_step))) {
		MMDVFSMSG("Apply Vcore setting(%d --> %d):\n", current_step,
		mmdvfs_step);
		MMDVFSMSG("current = %d, target = %d\n", current_step, mmdvfs_step);
		self->apply_vcore_hw_configurtion_by_step(self, mmdvfs_step);
		MMDVFSMSG("Apply CLK setting:\n");
		self->apply_clk_hw_configurtion_by_step(self, mmdvfs_step);
	} else {
		MMDVFSMSG("Apply CLK setting:\n");
		self->apply_clk_hw_configurtion_by_step(self, mmdvfs_step);
		MMDVFSMSG("Apply Vcore setting:\n");
		self->apply_vcore_hw_configurtion_by_step(self, mmdvfs_step);
	}

	return 0;
}

static int mmdvfs_apply_vcore_hw_configurtion_by_step(
struct mmdvfs_adaptor *self, int mmdvfs_step)
{
	struct mmdvfs_hw_configurtion *hw_config_ptr = NULL;
	int vcore_step = 0;

	if (self->enable_vcore == 0) {
		MMDVFSMSG("Doesn't change Vcore setting: vcore module is disable in MMDVFS\n");
		return -1;
	}

	/* Check unrequest step */
	if (mmdvfs_step == MMDVFS_FINE_STEP_UNREQUEST) {
		vcore_step = OPP_UNREQ;
	} else {
		/* Check if the step is legall */
		if (mmdvfs_step < 0 || mmdvfs_step >= self->step_num
		|| (self->step_profile_mappings + mmdvfs_step) == NULL)
			return -1;

		/* Get hw configurtion fot the step */
		hw_config_ptr
		= &(self->step_profile_mappings + mmdvfs_step)->hw_config;

		/* Check if mmdvfs_hw_configurtion is found */
		if (hw_config_ptr == NULL)
			return -1;

		/* Get vcore step */
		vcore_step = hw_config_ptr->vcore_step;
	}
	vcorefs_request_dvfs_opp(KIR_MM, vcore_step);
	/* Set vcore step */
	MMDVFSMSG("Set vcore step: %d\n", vcore_step);

	return 0;

}

static int mmdvfs_apply_clk_hw_configurtion_by_step(
struct mmdvfs_adaptor *self, int mmdvfs_step_request)
{
	struct mmdvfs_hw_configurtion *hw_config_ptr = NULL;
	int clk_idx = 0;
	int mmdvfs_step = -1;

	if (self->enable_clk_mux == 0) {
		MMDVFSMSG("Doesn't change clk mux setting: clk_mux module is disable in MMDVFS\n");
		return -1;
	}

	/* Check unrequest step and reset the mmdvfs step to the lowest one */
	if (mmdvfs_step_request == -1)
		mmdvfs_step = self->step_num - 1;
	else
		mmdvfs_step = mmdvfs_step_request;

	/* Check if the step is legall */
	if (mmdvfs_step < 0 || mmdvfs_step >= self->step_num
	|| (self->step_profile_mappings + mmdvfs_step) == NULL)
		return -1;

	/* Get hw configurtion fot the step */
	hw_config_ptr = &(self->step_profile_mappings + mmdvfs_step)->hw_config;

	/* Check if mmdvfs_hw_configurtion is found */
	if (hw_config_ptr == NULL)
		return -1;

	MMDVFSMSG("CLK SWITCH: total = %d\n", hw_config_ptr->total_clks);

	/* Get each clk and setp it accord to config method */
	for (clk_idx = 0; clk_idx < hw_config_ptr->total_clks; clk_idx++) {
		/* Get the clk step setting of each mm clks */
		int clk_step = hw_config_ptr->clk_steps[clk_idx];
		/* Get the specific clk descriptor */
		struct mmdvfs_clk_hw_map *clk_hw_map_ptr =
		&(self->mmdvfs_clk_hw_maps[clk_idx]);

		if (clk_step < 0 || clk_step >= clk_hw_map_ptr->total_step) {
			MMDVFSMSG("invalid clk step (%d) for %s\n", clk_step,
			clk_hw_map_ptr->clk_mux.ccf_name);
		} else {
			int clk_mux_mask = get_mmdvfs_clk_mux_mask();

			if (!((1 << clk_idx) & clk_mux_mask)) {
				MMDVFSMSG("CLK %d(%s) swich is not enabled\n",
				clk_idx, clk_hw_map_ptr->clk_mux.ccf_name);
				continue;
			}
			/* Apply the clk setting, only support mux now */
			if (clk_hw_map_ptr->config_method
			== MMDVFS_CLK_CONFIG_BY_MUX) {
				/* Get clk source */
				int
				clk_source_id =
				clk_hw_map_ptr->step_clk_source_id_map[clk_step];

				if (clk_source_id < 0 || clk_source_id
				>= self->mmdvfs_clk_sources_num)
					MMDVFSMSG(
					"invalid clk source id: %d, step:%d, mux:%s\n",
					clk_source_id, mmdvfs_step,
					clk_hw_map_ptr->clk_mux.ccf_name);
				else {
					int ccf_ret = -1;

					MMDVFSMSG(
					"Change %s source to %s, expect clk = %d\n",
					clk_hw_map_ptr->clk_mux.ccf_name,
					self->mmdvfs_clk_sources[clk_source_id].ccf_name,
					self->mmdvfs_clk_sources[clk_source_id].requested_clk);

					if (clk_hw_map_ptr->clk_mux.ccf_handle == NULL ||
						self->mmdvfs_clk_sources[clk_source_id].ccf_handle == NULL) {
						MMDVFSMSG("CCF handle can't be NULL during MMDVFS\n");
						continue;
					}

					ccf_ret =
						clk_prepare_enable((struct clk *)clk_hw_map_ptr->clk_mux.ccf_handle);
					MMDVFSMSG("clk_prepare_enable: handle = %lx\n",
					((unsigned long)clk_hw_map_ptr->clk_mux.ccf_handle));

					if (ccf_ret) {
						MMDVFSMSG("Failed to prepare clk: %s\n",
						clk_hw_map_ptr->clk_mux.ccf_name);
						return -1;
					}

					ccf_ret = clk_set_parent((struct clk *)clk_hw_map_ptr->clk_mux.ccf_handle,
						(struct clk *)self->mmdvfs_clk_sources[clk_source_id].ccf_handle);
					MMDVFSMSG("clk_set_parent: handle = (%lx,%lx), src id = %dn",
					((unsigned long)clk_hw_map_ptr->clk_mux.ccf_handle),
					((unsigned long)self->mmdvfs_clk_sources[clk_source_id].ccf_handle),
					clk_source_id);


					if (ccf_ret) {
						MMDVFSMSG("Failed to set parent:%s,%s\n",
						clk_hw_map_ptr->clk_mux.ccf_name,
						self->mmdvfs_clk_sources[clk_source_id].ccf_name);
						return -1;
					}

					if ((clk_idx != MMDVFS_CLK_MUX_TOP_MM_SEL) &&
						(clk_idx != MMDVFS_CLK_MUX_TOP_IMG_SEL)) {
						clk_disable_unprepare((struct clk *)clk_hw_map_ptr->clk_mux.ccf_handle);
						MMDVFSMSG("clk_disable_unprepare: handle = %lx\n",
						((unsigned long)clk_hw_map_ptr->clk_mux.ccf_handle));
					} else {
						MMDVFSMSG("[Workaround]MM_SEL can't be unprepared now, handle=%lx\n",
						((unsigned long)clk_hw_map_ptr->clk_mux.ccf_handle));
					}
				}
			}
		}
	}
	return 0;

}

static int mmdvfs_get_cam_sys_clk(struct mmdvfs_adaptor *self, int mmdvfs_step)
{
	return self->step_profile_mappings[mmdvfs_step].hw_config.clk_steps[MMDVFS_CLK_MUX_TOP_CAM_SEL];
}

/* single_profile_dump_func: */
static void mmdvfs_single_profile_dump(struct mmdvfs_profile *profile)
{
	if (profile == NULL) {
		MMDVFSMSG("mmdvfs_single_profile_dump: NULL profile found\n");
		return;
	}
	MMDVFSMSG("%s, %d, (%d,%d,%d), (%d,%d,%d)\n", profile->profile_name,
	profile->smi_scenario_id, profile->cam_limit.sensor_size,
	profile->cam_limit.feature_flag, profile->cam_limit.fps,
	profile->video_limit.width, profile->video_limit.height,
	profile->video_limit.codec);
}

/* Profile Matching Util */
static void mmdvfs_profile_dump(struct mmdvfs_adaptor *self)
{
	int i = 0;

	struct mmdvfs_step_to_profile_mapping *profile_mapping =
	self->step_profile_mappings;
	if (profile_mapping == NULL)
		MMDVFSMSG(
		"mmdvfs_profile_dump: step_profile_mappings can't be NULL\n");
	MMDVFSMSG("MMDVFS DUMP (%d):\n", profile_mapping->mmdvfs_step);
	for (i = 0; i < profile_mapping->total_profiles; i++) {
		struct mmdvfs_profile *profile = profile_mapping->profiles + i;

		mmdvfs_single_profile_dump(profile);
	}
}

static void mmdvfs_single_hw_configuration_dump(struct mmdvfs_adaptor *self,
struct mmdvfs_hw_configurtion *hw_configuration)
{

	int i = 0;
	const int clk_soure_total = self->mmdvfs_clk_sources_num;

	struct mmdvfs_clk_source_desc *clk_sources = self->mmdvfs_clk_sources;

	struct mmdvfs_clk_hw_map *clk_hw_map = self->mmdvfs_clk_hw_maps;

	if (clk_hw_map == NULL) {
		MMDVFSMSG(
		"single_hw_configuration_dump: mmdvfs_clk_hw_maps can't be NULL\n");
		return;
	}

	if (hw_configuration == NULL) {
		MMDVFSMSG(
		"single_hw_configuration_dump: hw_configuration can't be NULL\n");
		return;
	}

	MMDVFSMSG("Vcore step: %d\n", hw_configuration->vcore_step);

	for (i = 0; i < hw_configuration->total_clks; i++) {
		char *ccf_clk_source_name = "NONE";
		char *ccf_clk_mux_name = "NONE";
		int requested_clk = -1;
		int clk_step = -1;
		int clk_source_id = -1;

		struct mmdvfs_clk_hw_map *map_item = clk_hw_map + i;

		clk_step = hw_configuration->clk_steps[i];

		if (map_item != NULL && map_item->clk_mux.ccf_name != NULL) {
			clk_source_id
			= map_item->step_clk_source_id_map[clk_step];
			ccf_clk_mux_name = map_item->clk_mux.ccf_name;
		}

		if (map_item->config_method != MMDVFS_CLK_CONFIG_NONE
		&& clk_source_id < clk_soure_total && clk_source_id >= 0) {
			ccf_clk_source_name
			= clk_sources[clk_source_id].ccf_name;
			requested_clk
			= clk_sources[clk_source_id].requested_clk;
		}

		MMDVFSMSG("\t%s, %s, %dMhz\n", map_item->clk_mux.ccf_name,
		ccf_clk_source_name, requested_clk);
	}
}

static int mmdvfs_determine_step(struct mmdvfs_adaptor *self, int smi_scenario,
struct mmdvfs_cam_property *cam_setting,
struct mmdvfs_video_property *codec_setting)
{
	/* Find the matching scenario from OPP 0 to Max OPP */
	int opp_index = 0;
	int profile_index = 0;
	struct mmdvfs_step_to_profile_mapping *profile_mappings =
	self->step_profile_mappings;
	const int opp_max_num = self->step_num;

	if (profile_mappings == NULL)
		MMDVFSMSG(
		"mmdvfs_mmdvfs_step: step_profile_mappings can't be NULL\n");

	for (opp_index = 0; opp_index < opp_max_num; opp_index++) {
		struct mmdvfs_step_to_profile_mapping *mapping_ptr =
		profile_mappings + opp_index;

		for (profile_index = 0; profile_index
		< mapping_ptr->total_profiles; profile_index++) {
			/* Check if the scenario matches any profile */
			struct mmdvfs_profile *profile_prt =
			mapping_ptr->profiles + profile_index;
			if (smi_scenario == profile_prt->smi_scenario_id) {
				/* Check cam setting */
				if (is_camera_profile_matched(cam_setting,
				&profile_prt->cam_limit) == 0)
					/* Doesn't match the camera property condition, skip this run */
					continue;
				if (is_video_profile_matched(codec_setting,
				&profile_prt->video_limit) == 0)
					/* Doesn't match the video property condition, skip this run */
					continue;
				/* Complete match, return the opp index */
				mmdvfs_single_profile_dump(profile_prt);
				return opp_index;
			}
		}
	}

	/* If there is no profile matched, return -1 (no dvfs request)*/
	return -1;
}

/* Show each setting of opp */
static void mmdvfs_hw_configuration_dump(struct mmdvfs_adaptor *self)
{
	int i = 0;
	struct mmdvfs_step_to_profile_mapping *mapping =
	self->step_profile_mappings;

	if (mapping == NULL) {
		MMDVFSMSG(
		"mmdvfs_hw_configuration_dump: mmdvfs_clk_hw_maps can't be NULL");
		return;
	}

	MMDVFSMSG("All OPP configurtion dump\n");
	for (i = 0; i < self->step_num; i++) {
		struct mmdvfs_step_to_profile_mapping *mapping_item = mapping
		+ i;
		MMDVFSMSG("MMDVFS OPP %d:\n", i);
		if (mapping_item != NULL)
			self->single_hw_configuration_dump_func(self,
			&mapping_item->hw_config);
	}
}

static int is_camera_profile_matched(struct mmdvfs_cam_property *cam_setting,
struct mmdvfs_cam_property *profile_property)
{
	int is_match = 1;

	/* Null pointer check: */
	/* If the scenario doesn't has cam_setting, then there is no need to check the cam property  */
	if (!cam_setting || !profile_property) {
		is_match = 1;
	} else {

		/* Check the minium sensor resolution */
		if (!(cam_setting->sensor_size >= profile_property->sensor_size))
			is_match = 0;

		/* Check the minium sensor resolution */
		if (!(cam_setting->fps >= profile_property->fps))
			is_match = 0;

		/* Check the if the feature match */
		/* Not match if there is no featue matching the profile's one */
		/* 1 ==> don't change */
		/* 0 ==> set is_match to 0 */
		if (profile_property->feature_flag != 0
		&& !(cam_setting->feature_flag & profile_property->feature_flag))
			is_match = 0;
	}

	return is_match;
}

static int is_video_profile_matched(
struct mmdvfs_video_property *video_setting,
struct mmdvfs_video_property *profile_property)
{
	int is_match = 1;
	/* Null pointer check: */
	/* If the scenario doesn't has video_setting, then there is no need to check the video property */

	if (!video_setting || !profile_property)
		is_match = 1;
	else {
		if (!(video_setting->height * video_setting->width
		>= profile_property->height * profile_property->width))
	/* Check the minium sensor resolution */
		is_match = 0;
	}

	return is_match;
}

static void mmdvfs_step_util_init(struct mmdvfs_step_util *self)
{
	int idx = 0;

	for (idx = 0; idx < self->total_scenario; idx++)
		self->mmdvfs_scenario_mmdvfs_opp[idx] = MMDVFS_FINE_STEP_UNREQUEST;

	for (idx = 0; idx < self->total_opps; idx++)
		self->mmdvfs_concurrency_of_opps[idx] = 0;

}

/* updat the step members only (HW independent part) */
/* return the final step */
static int mmdvfs_step_util_set_step(struct mmdvfs_step_util *self, int step,
int client_id)
{
	int scenario_idx = 0;
	int opp_idx = 0;
	int final_opp = -1;

	/* check step range here */
	if (step < -1 || step >= self->total_opps)
		return -1;

	/* check invalid scenario */
	if (client_id < 0 || client_id >= self->total_scenario)
		return -1;

	self->mmdvfs_scenario_mmdvfs_opp[client_id] = step;

	/* Reset the concurrency fileds before the calculation */
	for (opp_idx = 0; opp_idx < self->total_opps; opp_idx++)
		self->mmdvfs_concurrency_of_opps[opp_idx] = 0;

	for (scenario_idx = 0; scenario_idx < self->total_scenario; scenario_idx++) {
		for (opp_idx = 0; opp_idx < self->total_opps; opp_idx++) {
			if (self->mmdvfs_scenario_mmdvfs_opp[scenario_idx]
			== opp_idx)
				self->mmdvfs_concurrency_of_opps[opp_idx] |= 1
				<< scenario_idx;
		}
	}

	for (opp_idx = 0; opp_idx < self->total_opps; opp_idx++) {
		if (self->mmdvfs_concurrency_of_opps[opp_idx] != 0) {
			final_opp = opp_idx;
			break;
		}
	}

	return final_opp;
}

static int mmdvfs_get_opp_from_legacy_step(struct mmdvfs_step_util *self,
int legacy_step)
{
	if (self->legacy_step_to_oop == NULL || legacy_step < 0 || legacy_step
	>= self->legacy_step_to_oop_num)
		return -1;
	else
		return self->legacy_step_to_oop_num + legacy_step;
}

static int mmdvfs_get_legacy_mmclk_step_from_mmclk_opp(
struct mmdvfs_step_util *self, int mmclk_step)
{
	int step_ret = -1;

	if (self->mmclk_oop_to_legacy_step == NULL || mmclk_step < 0
	|| mmclk_step >= self->mmclk_oop_to_legacy_step_num) {
		step_ret = -1;
	} else {
		int *step_ptr = self->mmclk_oop_to_legacy_step + mmclk_step;

		if (step_ptr != NULL)
			step_ret = *step_ptr;
		else
			step_ret = -1;
	}
	return step_ret;
}

struct mmdvfs_adaptor *g_mmdvfs_adaptor = &mmdvfs_adaptor_obj;
struct mmdvfs_step_util *g_mmdvfs_step_util = &mmdvfs_step_util_obj;

void mmdvfs_config_util_init(void)
{
	MMDVFSMSG("g_mmdvfs_adaptor init\n");
	g_mmdvfs_adaptor->profile_dump_func(g_mmdvfs_adaptor);
	MMDVFSMSG("g_mmdvfs_step_util init\n");
	g_mmdvfs_step_util->init(g_mmdvfs_step_util);
}




