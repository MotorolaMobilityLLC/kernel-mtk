// SPDX-License-Identifier: GPL-2.0+
/*
 * WL2864C, Multi-Output Regulators
 * Copyright (C) 2019  Motorola Mobility LLC,
 *
 * Author: ChengLong, Motorola Mobility LLC,
 */

#ifndef __WL2864C_REGISTERS_H__
#define __WL2864C_REGISTERS_H__

/* Registers */
#define WL2864C_REG_NUM (WL2864C_SEQ_STATUS-WL2864C_CHIP_REV+1)

#define WL2864C_CHIP_REV 0x00
#define WL2864C_CURRENT_LIMITSEL 0x01
#define WL2864C_DISCHARGE_RESISTORS 0x02
#define WL2864C_LDO1_VOUT 0x03
#define WL2864C_LDO2_VOUT 0x04
#define WL2864C_LDO3_VOUT 0x05
#define WL2864C_LDO4_VOUT 0x06
#define WL2864C_LDO5_VOUT 0x07
#define WL2864C_LDO6_VOUT 0x08
#define WL2864C_LDO7_VOUT 0x09
#define WL2864C_LDO1_LDO2_SEQ 0x0a
#define WL2864C_LDO3_LDO4_SEQ 0x0b
#define WL2864C_LDO5_LDO6_SEQ 0x0c
#define WL2864C_LDO7_SEQ 0x0d
#define WL2864C_LDO_EN 0x0e
#define WL2864C_SEQ_STATUS 0x0f


/* WL2864C_LDO1_VSEL ~ WL2864C_LDO7_VSEL =
 * 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
 */
#define  WL2864C_LDO1_VSEL                      WL2864C_LDO1_VOUT
#define  WL2864C_LDO2_VSEL                      WL2864C_LDO2_VOUT
#define  WL2864C_LDO3_VSEL                      WL2864C_LDO3_VOUT
#define  WL2864C_LDO4_VSEL                      WL2864C_LDO4_VOUT
#define  WL2864C_LDO5_VSEL                      WL2864C_LDO5_VOUT
#define  WL2864C_LDO6_VSEL                      WL2864C_LDO6_VOUT
#define  WL2864C_LDO7_VSEL                      WL2864C_LDO7_VOUT

#define  WL2864C_VSEL_SHIFT                     0
#define  WL2864C_VSEL_MASK                      (0xff << 0)

#endif /* __WL2864C_REGISTERS_H__ */
