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

#include <linux/types.h>
#include <mt-plat/charging.h>
#include <mt-plat/upmu_common.h>

#include <linux/delay.h>
#include <linux/reboot.h>
#include <mt-plat/mt_boot.h>
#include <mach/mt_charging.h>
#include <mach/mt_pmic.h>
#include <linux/power_supply.h>

/* ============================================================ */
/* define */
/* ============================================================ */
#define STATUS_OK	0
#define STATUS_UNSUPPORTED	-1

/* ============================================================ */
/* global variable */
/* ============================================================ */
kal_bool chargin_hw_init_done = KAL_FALSE;


/* ============================================================ */
/* internal variable */
/* ============================================================ */
static char *support_charger[] = {
	"rt9536",
};

static struct power_supply *external_charger;

static kal_bool charging_type_det_done = KAL_TRUE;
static CHARGER_TYPE g_charger_type = CHARGER_UNKNOWN;

static unsigned int g_charging_enabled;
static unsigned int g_charging_current;
static unsigned int g_charging_current_limit;
static unsigned int g_charging_voltage;
static unsigned int g_charging_setting_chagned;

/* ============================================================ */
/* extern variable */
/* ============================================================ */

/* ============================================================ */
/* extern function */
/* ============================================================ */
/* extern unsigned int upmu_get_reg_value(unsigned int reg); */
/* extern void Charger_Detect_Init(void); */
/* extern void Charger_Detect_Release(void); */
/* extern void mt_power_off(void); */

/* Internal APIs for PowerSupply */
static int is_property_support(enum power_supply_property prop)
{
	int support = 0;
	int i;

	if (!external_charger)
		return 0;

	if (!external_charger->get_property)
		return 0;

	for (i = 0; i < external_charger->num_properties; i++) {
		if (external_charger->properties[i] == prop) {
			support = 1;
			break;
		}
	}

	return support;
}

static int is_property_writeable(enum power_supply_property prop)
{
	if (!external_charger->set_property)
		return 0;

	if (!external_charger->property_is_writeable)
		return 0;

	return external_charger->property_is_writeable(external_charger, prop);
}

static int get_property(enum power_supply_property prop, int *data)
{
	union power_supply_propval val;
	int rc = 0;

	*(int *)data = STATUS_UNSUPPORTED;

	if (!is_property_support(prop))
		return 0;

	rc = external_charger->get_property(external_charger, prop, &val);
	if (rc) {
		battery_log(BAT_LOG_CRTI, "[CHARGER] failed to get property %d\n", prop);
		*(int *)data = 0;
		return rc;
	}

	*(int *)data = val.intval;

	battery_log(BAT_LOG_CRTI, "[CHARGER] set property %d to %d\n", prop, val.intval);
	return rc;
}

static int set_property(enum power_supply_property prop, int data)
{
	union power_supply_propval val;
	int rc = 0;

	if (!is_property_writeable(prop))
		return 0;

	val.intval = data;
	rc = external_charger->set_property(external_charger, prop, &val);
	if (rc)
		battery_log(BAT_LOG_CRTI, "[CHARGER] failed to set property %d\n", prop);

	battery_log(BAT_LOG_CRTI, "[CHARGER] set property %d to %d\n", prop, data);
	return rc;
}

/* Charger Control Interface Handler */
static unsigned int charging_hw_init(void *data)
{
	static int hw_initialized;

	battery_log(BAT_LOG_CRTI, "[CHARGER] charging_hw_init\n");

	if (hw_initialized)
		return STATUS_OK;

	pmic_set_register_value(PMIC_RG_USBDL_SET, 0x0); /* force leave USBDL mode */
	pmic_set_register_value(PMIC_RG_USBDL_RST, 0x1); /* force leave USBDL mode */

	hw_initialized = 1;
	battery_log(BAT_LOG_CRTI, "[CHARGER] initialized.\n");

	return STATUS_OK;
}

static unsigned int charging_dump_register(void *data)
{
	/* nothing to do */
	return STATUS_OK;
}

static unsigned int charging_enable(void *data)
{
	int status;
	int enable = *(int *)(data);
	int rc = 0;

	battery_log(BAT_LOG_CRTI, "[CHARGER] charging_enable (enable=%d) ", enable);
	battery_log(BAT_LOG_CRTI, "(g_charging_enabled=%d) ", g_charging_enabled);
	battery_log(BAT_LOG_CRTI, "(!g_charging_setting_chagned=%d)\n", (!g_charging_setting_chagned));

#if 1
	if (enable == g_charging_enabled && !g_charging_setting_chagned) {
		battery_log(BAT_LOG_FULL, "[CHARGER] charging_enable RETURN !!!!!!!!!\n");
		return STATUS_OK;
	}
#endif /* 0 */

	/* Do not disable charging when battery disconnected */
	if (!enable && pmic_get_register_value(PMIC_RGS_BATON_UNDET))
		return STATUS_OK;

	if (enable)
		status = POWER_SUPPLY_STATUS_CHARGING;
	else
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;

	rc = set_property(POWER_SUPPLY_PROP_STATUS, status);
	if (rc) {
		battery_log(BAT_LOG_CRTI,
			"[CHARGER] failed to %s charging.(%d)\n",
			(enable ? "start" : "stop"), rc);
		return STATUS_UNSUPPORTED;
	}

	g_charging_enabled = enable;

	/* clear charging setting */
	if (!g_charging_enabled) {
		g_charging_current = 0;
		g_charging_current_limit = 0;
		g_charging_voltage = 0;
	}

	g_charging_setting_chagned = 0;

	battery_log(BAT_LOG_CRTI, "[CHARGER] %s charging.\n",
				(g_charging_enabled ? "start" : "stop"));

	return STATUS_OK;
}

static unsigned int charging_set_cv_voltage(void *data)
{
	int voltage = *(int *)(data);
	int rc = 0;

	if (voltage == g_charging_voltage)
		return STATUS_OK;

	rc = set_property(POWER_SUPPLY_PROP_VOLTAGE_MAX, voltage);
	if (rc) {
		battery_log(BAT_LOG_CRTI, "[CHARGER] Set CV Voltage failed.(%d)\n", rc);
		return STATUS_UNSUPPORTED;
	}

	g_charging_voltage = voltage;
	g_charging_setting_chagned = 1;

	battery_log(BAT_LOG_CRTI, "[CHARGER] charging_set_cv_voltage : Set CV Voltage to %duV\n", g_charging_voltage);

	return STATUS_OK;
}

static unsigned int charging_get_current(void *data)
{
	int cur;
	int rc = 0;

	if (!g_charging_enabled) {
		*(int *)data = 0;
		return STATUS_OK;
	}

	rc = get_property(POWER_SUPPLY_PROP_CURRENT_NOW, &cur);
	if (rc)
		*(int *)data = min(g_charging_current, g_charging_current_limit);
	else if (cur < 0)
		*(int *)data = min(g_charging_current, g_charging_current_limit);
	else
		*(int *)data = cur;

	/* match unit with CHR_CURRENT_ENUM */
	*(int *)data *= 100;

	return STATUS_OK;
}

static unsigned int charging_set_current(void *data)
{
	enum power_supply_property prop = POWER_SUPPLY_PROP_CURRENT_NOW;
	int cur = *(int *)(data);
	int rc = 0;

	/* convert unit to mA */
	cur = cur / 100;

	/* charging current & current limit is not separated */
	if (!is_property_writeable(POWER_SUPPLY_PROP_CURRENT_NOW)) {
		prop = POWER_SUPPLY_PROP_CURRENT_MAX;

		/* use current limit value here */
		if (cur > g_charging_current_limit)
			cur = g_charging_current_limit;
	}

	if (cur == g_charging_current) {
		battery_log(BAT_LOG_CRTI, "[CHARGER] cur(%d) == g_charging_current(%d)\n", cur, g_charging_current);
		return STATUS_OK;
	}

	rc = set_property(prop, cur);
	if (rc) {
		battery_log(BAT_LOG_CRTI, "[CHARGER] Set Current failed.(%d)\n", rc);
		return STATUS_UNSUPPORTED;
	}

	g_charging_current = cur;
	g_charging_setting_chagned = 1;

	battery_log(BAT_LOG_CRTI, "[CHARGER] charging_set_current : Set Current to %dmA", g_charging_current);
	battery_log(BAT_LOG_CRTI, "cur=%d, g_charging_current_limit=%d\n", cur, g_charging_current_limit);

	return STATUS_OK;
}

static unsigned int charging_set_input_current(void *data)
{
	int cur = *(int *)(data);
	int rc = 0;

	/* convert unit to mA */
	cur = cur / 100;

	if (cur == g_charging_current_limit)
		return STATUS_OK;

	rc = set_property(POWER_SUPPLY_PROP_CURRENT_MAX, cur);
	if (rc) {
		battery_log(BAT_LOG_CRTI, "[CHARGER] Set Current Limit failed.(%d)\n", rc);
		return STATUS_UNSUPPORTED;
	}

	g_charging_current_limit = cur;
	g_charging_setting_chagned = 1;

	battery_log(BAT_LOG_CRTI, "[CHARGER] Set Current Limit to %dmA\n", g_charging_current_limit);

	return STATUS_OK;
}

static unsigned int charging_get_charging_status(void *data)
{
	int voltage = *(int *)data;
	int status;
	int rc = 0;

	if (g_charger_type == CHARGER_UNKNOWN) {
		*(int *)data = 0;
		return STATUS_OK;
	}

	rc = get_property(POWER_SUPPLY_PROP_STATUS, &status);
	if (rc) {
		battery_log(BAT_LOG_CRTI, "[CHARGER] failed to get charging status.(%d)\n", rc);
		return STATUS_UNSUPPORTED;
	}

	/* if eoc check is not supported in charger ic, check battery voltage instead */
	if (status == STATUS_UNSUPPORTED) {
		/* battery voltage is invalid range */
		if (voltage > 5000) {
			*(int *)data = 0;
			return STATUS_OK;
		}

		if (voltage > RECHARGING_VOLTAGE)
			*(int *)data = 1;
		else
			*(int *)data = 0;

		return STATUS_OK;
	}

	if (status == POWER_SUPPLY_STATUS_FULL)
		*(int *)data = 1;
	else
		*(int *)data = 0;

	battery_log(BAT_LOG_CRTI, "[CHARGER] End of Charging : %d\n", *(int *)data);

	return STATUS_OK;
}

static unsigned int charging_reset_watch_dog_timer(void *data)
{
	/* nothing to do */
	return STATUS_OK;
}

static unsigned int charging_set_hv_threshold(void *data)
{
	/* nothing to do */
	return STATUS_OK;
}

static unsigned int charging_get_hv_status(void *data)
{
	/* nothing to do */
	*(kal_bool *)data = KAL_FALSE;
	return STATUS_OK;
}

static unsigned int charging_get_battery_status(void *data)
{

	pmic_set_register_value(PMIC_BATON_TDET_EN, 1);
	pmic_set_register_value(PMIC_RG_BATON_EN, 1);

	*(kal_bool *)(data) = pmic_get_register_value(PMIC_RGS_BATON_UNDET);

	return STATUS_OK;
}

static unsigned int charging_get_charger_det_status(void *data)
{
	int online;
	int rc;

	if (pmic_get_register_value_nolock(PMIC_RGS_CHRDET) == 0) {
		*(kal_bool *)(data) = KAL_FALSE;
		g_charger_type = CHARGER_UNKNOWN;
	} else {
		*(kal_bool *)(data) = KAL_TRUE;
		rc = get_property(POWER_SUPPLY_PROP_ONLINE, &online);
		if (rc) {
			/* error reading online status. use pmic value */
			battery_log(BAT_LOG_CRTI, "[CHARGER] cannot read online.\n");
			return STATUS_OK;
		}

		if (!online) {
			/* OVP detected in external charger */
			*(kal_bool *)(data) = KAL_FALSE;
			g_charger_type = CHARGER_UNKNOWN;
		}
	}

	battery_log(BAT_LOG_FULL, "[CHARGER] g_charger_type = %d\n", g_charger_type);

	return STATUS_OK;
}

/* extern int hw_charging_get_charger_type(void); */

static unsigned int charging_get_charger_type(void *data)
{
	unsigned int status = STATUS_OK;
#if defined(CONFIG_POWER_EXT)
	*(CHARGER_TYPE *)(data) = STANDARD_HOST;
#else

	if ((g_charger_type != CHARGER_UNKNOWN) && (g_charger_type != WIRELESS_CHARGER)) {
		*(CHARGER_TYPE *)(data) = g_charger_type;
		battery_log(BAT_LOG_CRTI, "[CHARGER] charging_get_charger_type : return %d!\n", g_charger_type);

		return status;
	}

	charging_type_det_done = KAL_FALSE;

	g_charger_type = hw_charging_get_charger_type();

	charging_type_det_done = KAL_TRUE;

	*(CHARGER_TYPE *)(data) = g_charger_type;
#endif

	return STATUS_OK;
}

static unsigned int charging_get_is_pcm_timer_trigger(void *data)
{
	unsigned int status = STATUS_OK;
#if 0
	if (slp_get_wake_reason() == WR_PCM_TIMER)
		*(kal_bool *)(data) = KAL_TRUE;
	else
	*(kal_bool *)(data) = KAL_FALSE;

	battery_log(BAT_LOG_CRTI, "[CHARGER] slp_get_wake_reason=%d\n", slp_get_wake_reason());
#endif
	return status;
}

static unsigned int charging_set_platform_reset(void *data)
{
	battery_log(BAT_LOG_CRTI, "[CHARGER] charging_set_platform_reset\n");

	kernel_restart("battery service reboot system");

	return STATUS_OK;
}

static unsigned int charging_get_platform_boot_mode(void *data)
{
	*(unsigned int *)(data) = get_boot_mode();

	battery_log(BAT_LOG_CRTI, "[CHARGER] get_boot_mode=%d\n", get_boot_mode());

	return STATUS_OK;
}

static unsigned int charging_set_power_off(void *data)
{
	unsigned int status = STATUS_OK;

	battery_log(BAT_LOG_CRTI, "[CHARGER] charging_set_power_off\n");
	kernel_power_off();

	return status;
}

static unsigned int charging_get_power_source(void *data)
{
	unsigned int status = STATUS_OK;

	*(kal_bool *)data = KAL_FALSE;

	return status;
}
static unsigned int charging_get_csdac_full_flag(void *data)
{
	unsigned int status = STATUS_OK;

	*(kal_bool *)data = KAL_FALSE;

	return status;
}
static unsigned int charging_set_ta_current_pattern(void *data)
{
	unsigned int status = STATUS_OK;

	return status;
}

static unsigned int charging_set_error_state(void *data)
{
	return STATUS_UNSUPPORTED;
}

static unsigned int(*charging_func[CHARGING_CMD_NUMBER]) (void *data);

/*
 * FUNCTION
 *		Internal_chr_control_handler
 *
 * DESCRIPTION
 *		 This function is called to set the charger hw
 *
 * CALLS
 *
 * PARAMETERS
 *		None
 *
 * RETURNS
 *
 *
 * GLOBALS AFFECTED
 *	   None
 */
signed int chr_control_interface(CHARGING_CTRL_CMD cmd, void *data)
{
	signed int status = 0;
	int i;

	static int chr_control_interface_init;

    /* battery_log(BAT_LOG_CRTI, "[CHARGER] cmd %d\n",cmd); */

	if (!chr_control_interface_init) {
		charging_func[CHARGING_CMD_INIT] = charging_hw_init;
		charging_func[CHARGING_CMD_DUMP_REGISTER] = charging_dump_register;
		charging_func[CHARGING_CMD_ENABLE] = charging_enable;
		charging_func[CHARGING_CMD_SET_CV_VOLTAGE] = charging_set_cv_voltage;
		charging_func[CHARGING_CMD_GET_CURRENT] = charging_get_current;
		charging_func[CHARGING_CMD_SET_CURRENT] = charging_set_current;
		charging_func[CHARGING_CMD_SET_INPUT_CURRENT] = charging_set_input_current;
		charging_func[CHARGING_CMD_GET_CHARGING_STATUS] = charging_get_charging_status;
		charging_func[CHARGING_CMD_RESET_WATCH_DOG_TIMER] = charging_reset_watch_dog_timer;
		charging_func[CHARGING_CMD_SET_HV_THRESHOLD] = charging_set_hv_threshold;
		charging_func[CHARGING_CMD_GET_HV_STATUS] = charging_get_hv_status;
		charging_func[CHARGING_CMD_GET_BATTERY_STATUS] = charging_get_battery_status;
		charging_func[CHARGING_CMD_GET_CHARGER_DET_STATUS] = charging_get_charger_det_status;
		charging_func[CHARGING_CMD_GET_CHARGER_TYPE] = charging_get_charger_type;
		charging_func[CHARGING_CMD_GET_IS_PCM_TIMER_TRIGGER] = charging_get_is_pcm_timer_trigger;
		charging_func[CHARGING_CMD_SET_PLATFORM_RESET] = charging_set_platform_reset;
		charging_func[CHARGING_CMD_GET_PLATFORM_BOOT_MODE] = charging_get_platform_boot_mode;
		charging_func[CHARGING_CMD_SET_POWER_OFF] = charging_set_power_off;
		charging_func[CHARGING_CMD_GET_POWER_SOURCE] = charging_get_power_source;
		charging_func[CHARGING_CMD_GET_CSDAC_FALL_FLAG] = charging_get_csdac_full_flag;
		charging_func[CHARGING_CMD_SET_TA_CURRENT_PATTERN] = charging_set_ta_current_pattern;
		charging_func[CHARGING_CMD_SET_ERROR_STATE] = charging_set_error_state;

		chr_control_interface_init = 1;
	}

	switch (cmd) {
	/* these commands does not need external_charger, so jump to do_cmd */
	case CHARGING_CMD_INIT:
	case CHARGING_CMD_RESET_WATCH_DOG_TIMER:
	case CHARGING_CMD_SET_HV_THRESHOLD:
	case CHARGING_CMD_GET_HV_STATUS:
	case CHARGING_CMD_GET_BATTERY_STATUS:
	case CHARGING_CMD_GET_IS_PCM_TIMER_TRIGGER:
	case CHARGING_CMD_SET_PLATFORM_RESET:
	case CHARGING_CMD_GET_PLATFORM_BOOT_MODE:
	case CHARGING_CMD_SET_POWER_OFF:
	case CHARGING_CMD_GET_POWER_SOURCE:
	case CHARGING_CMD_GET_CSDAC_FALL_FLAG:
	case CHARGING_CMD_SET_TA_CURRENT_PATTERN:
	case CHARGING_CMD_SET_ERROR_STATE:
		/* battery_log(BAT_LOG_CRTI, "[CHARGER] 1-fail cmd (%d) goto control interface .\n", cmd); */
		goto chr_control_interface_do_cmd;
		/* break; */
	default:
		/* battery_log(BAT_LOG_CRTI, "[CHARGER] 2-fail cmd (%d) goto control interface .\n", cmd); */
		break;
	}

	if (!external_charger) {
		/* find charger */
		for (i = 0; i < ARRAY_SIZE(support_charger); i++) {
			external_charger = power_supply_get_by_name(support_charger[i]);
			if (external_charger) {
				battery_log(BAT_LOG_CRTI, "[CHARGER] found charger %s\n", support_charger[i]);
				break;
			}
		}

		/* if not found, cannot control charger */
		if (!external_charger) {
			battery_log(BAT_LOG_CRTI, "[CHARGER] failed to get external_charger.\n");
			return STATUS_UNSUPPORTED;
		}

		/* If power source is attached, assume that device is charing now */
		if (pmic_get_register_value_nolock(PMIC_RGS_CHRDET))
			g_charging_enabled = true;

	}

chr_control_interface_do_cmd:
#if 0
	status = charging_func[cmd](data);
#else
	if (cmd < CHARGING_CMD_NUMBER) {
		if (charging_func[cmd] != NULL) {
			status = charging_func[cmd](data);
		} else {
			/* battery_log(BAT_LOG_CRTI, "[CHARGER] chr_control_interface NULL!!!!! cmd(%d).\n", cmd); */
			status = 0;
		}
	}
#endif

	return status;
}

