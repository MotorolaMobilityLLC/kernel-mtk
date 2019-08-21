/*
 *  ocp2131.h
 *
 *  Copyright (C) 2019 TS Software Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */
#ifndef _I2C_OCP_H_
#define _I2C_OCP_H_

#ifdef CONFIG_MTK_LCM_BIAS_OCP2131

int OCP2131_write_bytes(unsigned char addr, unsigned char value);
void OCP2131_GPIO_ENP_enable(void);
void OCP2131_GPIO_ENN_enable(void);
void OCP2131_GPIO_ENP_disable(void);
void OCP2131_GPIO_ENN_disable(void);

#endif
#endif
