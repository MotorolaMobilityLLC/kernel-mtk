/**
 * Copyright (C) Fourier Semiconductor Inc. 2016-2020. All rights reserved.
 * 2018-10-22 File created.
 */

#if defined(CONFIG_FSM_CODEC)
#include "fsm_public.h"
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <linux/miscdevice.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <linux/version.h>
#include "../mt6358.h"

static atomic_t fsm_amp_switch;
static atomic_t fsm_amp_select;

static int fsm_get_scene_index(uint16_t scene)
{
	int index = 0;

	while (scene) {
		scene = (scene >> 1);
		if (scene == 0) {
			break;
		}
		index++;
	}

	return index;
}

int fsm_init_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int state;

	state = (fsm_get_presets() != NULL) ? 1 : 0;
	pr_info("state:%d", state);
	ucontrol->value.integer.value[0] = state;

	return 0;
}

int fsm_init_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	fsm_init();
	return 0;
}

int fsm_scene_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	fsm_config_t *cfg = fsm_get_config();
	int scene_index;

	if (!cfg) {
		ucontrol->value.integer.value[0] = -1;
		return 0;
	}
	scene_index = fsm_get_scene_index(cfg->next_scene);
	pr_info("scene: %04X, BIT(%d)", cfg->next_scene, scene_index);
	ucontrol->value.integer.value[0] = scene_index;

	return 0;
}

void is_receiver_working(int scene)
{
	if (scene == 15) {
		handset_monitor_select(input,HANDSET_MONITOR_ON_KEY);
		handset_mode_flag = 1;
	} else if ((scene != 15)&&(handset_mode_flag == 1)) {
		handset_monitor_select(input,HANDSET_MONITOR_OFF_KEY);
		handset_mode_flag = 0;
	}
}

int fsm_scene_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int next_scene = ucontrol->value.integer.value[0];

	pr_info("next_scene: %d", next_scene);
	fsm_set_scene(next_scene);
	is_receiver_working(next_scene);

	return 0;
}

int fsm_volume_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	fsm_config_t *cfg = fsm_get_config();
	int volume;

	volume = ((cfg != NULL) ? cfg->volume : FSM_VOLUME_MAX);
	ucontrol->value.integer.value[0] = volume;
	pr_info("volume: %ld", ucontrol->value.integer.value[0]);

	return 0;
}

int fsm_volume_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int volume = ucontrol->value.integer.value[0];

	pr_info("volume: %d", volume);
	fsm_set_volume(volume);

	return 0;
}

int fsm_amp_switch_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int enable = atomic_read(&fsm_amp_switch);

	ucontrol->value.integer.value[0] = enable;
	pr_info("switch: %s", enable ? "On" : "Off");

	return 0;
}

int fsm_amp_switch_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int enable = ucontrol->value.integer.value[0];

	pr_info("switch: %s", enable ? "On" : "Off");
	atomic_set(&fsm_amp_switch, enable);
	if (enable) {
		fsm_speaker_onn();
	} else {
		fsm_speaker_off();
	}

	return 0;
}

int fsm_amp_select_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int mask = atomic_read(&fsm_amp_select);

	pr_info("MASK:%X", mask);
	ucontrol->value.integer.value[0] = mask;

	return 0;
}

int fsm_amp_select_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int sel_mask = ucontrol->value.integer.value[0];

	pr_info("MASK:%X", sel_mask);
	atomic_set(&fsm_amp_select, sel_mask);
	fsm_set_sel_mask(sel_mask);

	return 0;
}

static const struct snd_kcontrol_new fsm_snd_controls[] =
{
	SOC_SINGLE_EXT("FSM_Init", SND_SOC_NOPM, 0, 1, 0,
			fsm_init_get, fsm_init_put),
	SOC_SINGLE_EXT("FSM_Scene", SND_SOC_NOPM, 0, 17, 0,
			fsm_scene_get, fsm_scene_put),
	SOC_SINGLE_EXT("FSM_Volume", SND_SOC_NOPM, 0, FSM_VOLUME_MAX, 0,
			fsm_volume_get, fsm_volume_put),
	SOC_SINGLE_EXT("FSM_Amp_Switch", SND_SOC_NOPM, 0, 1, 0,
			fsm_amp_switch_get, fsm_amp_switch_put),
	SOC_SINGLE_EXT("FSM_Amp_Select", SND_SOC_NOPM, 0, 0xF, 0,
			fsm_amp_select_get, fsm_amp_select_put),
};

int fsm_speaker_controls(int index, int scene)
{
	if (index < 0 || index > 3 || scene < 0 || scene > 16) {
		pr_err("invalid parameter : index = %d, scene = %d", index, scene);
		return -EINVAL;
	}
	if (scene != 0) {
		fsm_set_scene(scene - 1);
		fsm_speaker_onn();
	} else {
		fsm_speaker_off();
	}
	return 0;
}
EXPORT_SYMBOL(fsm_speaker_controls);

void fsm_add_codec_controls(struct snd_soc_codec *codec)
{
	atomic_set(&fsm_amp_switch, 0);
	atomic_set(&fsm_amp_select, 0xF);
	snd_soc_add_codec_controls(codec, fsm_snd_controls,
			ARRAY_SIZE(fsm_snd_controls));
}
EXPORT_SYMBOL(fsm_add_codec_controls);


void fsm_add_card_controls(struct snd_soc_card *card)
{
	atomic_set(&fsm_amp_switch, 0);
	atomic_set(&fsm_amp_select, 0xF);
	snd_soc_add_card_controls(card, fsm_snd_controls,
			ARRAY_SIZE(fsm_snd_controls));
}
EXPORT_SYMBOL(fsm_add_card_controls);
#endif
