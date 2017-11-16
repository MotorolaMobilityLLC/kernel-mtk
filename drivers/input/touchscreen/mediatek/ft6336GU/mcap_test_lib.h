#ifndef _FOCAL_MCAPTESTLIB_H
#define _FOCAL_MCAPTESTLIB_H

#define boolean unsigned char
#define bool unsigned char
#define BYTE unsigned char
#define false 0
#define true  1

/////////////////////IIC communication
typedef int (*FTS_I2C_READ_FUNCTION)(unsigned char *, int , unsigned char *, int);
typedef int (*FTS_I2C_WRITE_FUNCTION)(unsigned char *, int);

extern FTS_I2C_READ_FUNCTION fts_i2c_read_test;
extern FTS_I2C_WRITE_FUNCTION fts_i2c_write_test;

extern int init_i2c_read_func(FTS_I2C_READ_FUNCTION fpI2C_Read);
extern int init_i2c_write_func(FTS_I2C_WRITE_FUNCTION fpI2C_Write);

///////////////////////about test
int set_param_data(char *TestParamData);//load config
boolean start_test_tp(void);//test entry
int get_test_data(char *pTestData);//test result data. (pTestData, External application for memory, buff size >= 1024*80)

void free_test_param_data(void);//release 
int show_lib_ver(char *pLibVer);//lib version

#endif
