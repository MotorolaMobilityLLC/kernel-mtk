/*
 *  drivers/misc/mediatek/power/mt6797/mtk_gpuregulator_intf.h
 *  Include header file to MTK Richtek Interface
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __LINUX_MTK_GPUREGULATOR_INTF_H
#define __LINUX_MTK_GPUREGULATOR_INTF_H

/* capability flag */
#define RT_CAPABILITY_ENABLE (1 << 0)
#define RT_CAPABILITY_VOLTAGE (1 << 1)
#define RT_CAPABILITY_MODE (1 << 2)
#define RT_CAPABILITY_DISCHARGE (1 << 3)

enum {
	RT_VSEL0 = 0,
	RT_VSEL1,
};

enum {
	RT_DVS_DOWN = 0,
	RT_DVS_UP = 1,
};

/* long term method */
extern int rt_is_hw_exist(void);
extern int rt_is_sw_ready(void);
extern const char *rt_get_chip_name(void);
extern const char *rt_get_compatiblel_name(void);
extern int rt_get_chip_i2c_channel(void);
extern int rt_get_chip_capability(void);
extern int rt_hw_init_once(void);
extern int rt_hw_init_before_enable(void);
extern int rt_set_vsel_enable(u8 vsel, u8 enable);
extern int rt_is_vsel_enabled(u8 vsel);
extern int rt_set_voltage(u8 vsel, int volt_uV);
extern int rt_get_voltage(u8 vsel);
extern int rt_enable_force_pwm(u8 vsel, u8 enable);
extern int rt_is_force_pwm_enabled(u8 vsel);
extern int rt_enable_discharge(u8 enable);
extern int rt_is_discharge_enabled(void);
/* deprecated for short term */
extern int rt_read_interface(u8 cmd, u8 *return_data, u8 mask, u8 shift);
extern int rt_read_byte(u8 cmd, u8 *return_data);
extern int rt_config_interface(u8 cmd, u8 val, u8 mask, u8 shift);
extern int rt_get_slew_rate(u8 up_down);
extern int rt_set_slew_rate(u8 up_down, u8 val);
extern void rt_dump_registers(void);

struct mtk_user_intf {
	const char*  (*get_chip_name)(void *info);
	const char* (*get_compatible_name)(void *info);
	int (*get_chip_i2c_channel)(void *info);
	int (*get_chip_capability)(void *info);
	/* long term method */
	int (*hw_init_once)(void *info);
	int (*hw_init_before_enable)(void *info);
	int (*set_vsel_enable)(void *info, u8 vsel, u8 enable);
	int (*is_vsel_enabled)(void *info, u8 vsel);
	int (*set_voltage)(void *info, u8 vsel, int volt_uV);
	int (*get_voltage)(void *info, u8 vsel);
	int (*enable_force_pwm)(void *info, u8 vsel, u8 mode);
	int (*is_force_pwm_enabled)(void *info, u8 vsel);
	int (*enable_discharge)(void *info, u8 enable);
	int (*is_discharge_enabled)(void *info);
	int (*get_slew_rate)(void *info, u8 up_down);
	int (*set_slew_rate)(void *info, u8 up_down, u8 val);
	void (*dump_registers)(void *info);
};

struct chip_io_intf {
	void (*io_lock)(void *info);
	void (*io_unlock)(void *info);
	int (*io_read)(void *info, u8 cmd);
	int (*io_write)(void *info, u8 cmd, u8 val);
};

struct mtk_gpuregulator_platform_data {
	const struct mtk_user_intf *user_intf;
	const struct chip_io_intf *io_intf;
};
#endif /* #ifndef __LINUX_MTK_GPUREGULATOR_INTF_H */
