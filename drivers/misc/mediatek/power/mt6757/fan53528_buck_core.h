/*
 *  fan53528_buck_core.h
 *  Buck Core header file for FAN53528
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  Sakya <jeff_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __FAN53528_BUCK_CORE_H
#define __FAN53528_BUCK_CORE_H

enum {
	FAN53528_SLEW_RATE_UP = 0,
};

extern int fan53528_get_chip_i2c_channel(void);
extern int fan53528_set_enable(unsigned char en);
extern int fan53528_is_enabled(void);
extern int fan53528_set_voltage(int volt_uv);
extern int fan53528_get_voltage(void);
extern int fan53528_enable_force_pwm(unsigned char en);
extern int fan53528_is_force_pwm_enabled(void);
extern int fan53528_enable_discharge(unsigned char en);
extern int fan53528_is_discharge_enabled(void);
/* up_down = FAN535328_SLEW_RATE_UP :
 * fan535328 only support low-to-high slew rate setting */
extern int fan53528_set_slew_rate(unsigned char up_down, unsigned char ms_uv);
extern int fan53528_get_slew_rate(unsigned char up_down);
extern void fan53528_dump_registers(void);

#define FAN53528_REG_VSEL0	(0x00)
#define FAN53528_REG_VSEL1	(0x01)
#define FAN53528_REG_CONTROL	(0x02)
#define FAN53528_REG_ID1	(0x03)
#define FAN53528_REG_ID2	(0x04)
#define FAN53528_REG_MONITOR	(0x05)

#define FAN53528_VSELEN_MASK          (0x80)
#define FAN53528_VOUT_SHIFT              (0)
#define FAN53528_VOUT_MASK             (0x7f)

#define FAN53528_PWMSEL0_MASK     (0x01)
#define FAN53528_PWMSEL1_MASK     (0x02)

#define FAN53528_DISCHG_MASK         (0x80)
#define FAN53528_SLEWRATE_MASK    (0x70)
#define FAN53528_SLEWRATE_SHIFT		(4)

extern int mtk_buck_init(void);

#endif /* __FAN53528_BUCK_CORE_H */
