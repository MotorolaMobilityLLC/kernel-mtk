/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: Test_lib.h
*
* Author: Software Development Team, AE
*
* Created: 2015-07-14
*
* Abstract: test entry for all IC
*
************************************************************************/
#ifndef _TEST_LIB_H
#define _TEST_LIB_H

#define boolean unsigned char
#define bool unsigned char
#define BYTE unsigned char
#define false 0
#define true  1

/////////////////////IIC communication
typedef int (*FTS_I2C_READ_FUNCTION)(unsigned char *, int , unsigned char *, int);
typedef int (*FTS_I2C_WRITE_FUNCTION)(unsigned char *, int);

extern FTS_I2C_READ_FUNCTION fts6436_i2c_read;
extern FTS_I2C_WRITE_FUNCTION fts6436_i2c_write;
extern struct i2c_client *g_i2c_adap;
extern int init_i2c_read_func(FTS_I2C_READ_FUNCTION fpI2C_Read);
extern int init_i2c_write_func(FTS_I2C_WRITE_FUNCTION fpI2C_Write);
extern ssize_t ftxxxx_ftslibtest_sample(void);
///////////////////////about test
int set_param_data(char *TestParamData);//load config
boolean start_test_tp(void);//test entry
int get_test_data(char *pTestData);//test result data. (pTestData, External application for memory, buff size >= 1024*80)

void free_test_param_data(void);//release 
int show_lib_ver(char *pLibVer);//lib version
	



#endif
