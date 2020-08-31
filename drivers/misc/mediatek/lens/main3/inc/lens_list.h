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

#ifndef _LENS_LIST_H

#define _LENS_LIST_H

#if 0
#define AK7371AF_SetI2Cclient AK7371AF_SetI2Cclient_Main3
#define AK7371AF_Ioctl AK7371AF_Ioctl_Main3
#define AK7371AF_Release AK7371AF_Release_Main3
#define AK7371AF_PowerDown AK7371AF_PowerDown_Main3
#define AK7371AF_GetFileName AK7371AF_GetFileName_Main3
extern int AK7371AF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				 spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long AK7371AF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
			   unsigned long a_u4Param);
extern int AK7371AF_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int AK7371AF_PowerDown(struct i2c_client *pstAF_I2Cclient,
				int *pAF_Opened);
extern int AK7371AF_GetFileName(unsigned char *pFileName);
#endif
#define DW9714K_SetI2Cclient DW9714K_SetI2Cclient_Main3
#define DW9714K_Ioctl DW9714K_Ioctl_Main3
#define DW9714K_Release DW9714K_Release_Main3
#define DW9714K_PowerDown DW9714K_PowerDown_Main3
#define DW9714K_GetFileName DW9714K_GetFileName_Main3
extern int DW9714K_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
                                spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long DW9714K_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
                          unsigned long a_u4Param);
extern int DW9714K_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int DW9714K_PowerDown(struct i2c_client *pstAF_I2Cclient,
                               int *pAF_Opened);
extern int DW9714K_GetFileName(unsigned char *pFileName);

#define DW9714KAF_SetI2Cclient DW9714KAF_SetI2Cclient_Main3
#define DW9714KAF_Ioctl DW9714KAF_Ioctl_Main3
#define DW9714KAF_Release DW9714KAF_Release_Main3
#define DW9714KAF_PowerDown DW9714KAF_PowerDown_Main3
#define DW9714KAF_GetFileName DW9714KAF_GetFileName_Main3
extern int DW9714KAF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
                                spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long DW9714KAF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
                          unsigned long a_u4Param);
extern int DW9714KAF_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int DW9714KAF_PowerDown(struct i2c_client *pstAF_I2Cclient,
                               int *pAF_Opened);
extern int DW9714KAF_GetFileName(unsigned char *pFileName);

#define CN3927EAF_SetI2Cclient CN3927EAF_SetI2Cclient_Main3
#define CN3927EAF_Ioctl CN3927EAF_Ioctl_Main3
#define CN3927EAF_Release CN3927EAF_Release_Main3
#define CN3927EAF_GetFileName CN3927EAF_GetFileName_Main3
extern int CN3927EAF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				 spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long CN3927EAF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
			   unsigned long a_u4Param);
extern int CN3927EAF_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int CN3927EAF_GetFileName(unsigned char *pFileName);

extern void AFRegulatorCtrl(int Stage);
#endif
