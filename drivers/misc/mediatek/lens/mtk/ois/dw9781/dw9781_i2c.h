#ifndef __DW9781_I2C_H__
#define __DW9781_I2C_H__
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

void dw9781_set_i2c_client(struct i2c_client *i2c_client);
int write_reg_16bit_value_16bit(unsigned short regAddr, unsigned short regData);
int read_reg_16bit_value_16bit(unsigned short regAddr, unsigned short *regData);
#endif
