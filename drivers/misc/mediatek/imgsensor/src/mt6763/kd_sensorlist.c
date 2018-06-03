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

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>   /* proc file use */
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <sync_write.h>
#include <linux/types.h>
#include "kd_camera_hw.h"
#include "kd_camera_typedef.h"
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif


#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"
#include "kd_imgsensor_errcode.h"

#include "kd_sensorlist.h"

#ifdef CONFIG_MTK_SMI_EXT
#include "mmdvfs_mgr.h"
#endif
#ifdef CONFIG_OF
/* device tree */
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif
/* #define CONFIG_COMPAT */
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#ifdef CONFIG_MTK_CCU
#include "ccu_inc.h"
#endif

#ifndef CONFIG_MTK_CLKMGR
/*CCF*/


#define _MAX_MCLK_ 2
char *_seninf_tg_mux_name[_MAX_MCLK_] = {
	"TOP_CAMTG_SEL",
	"TOP_CAMTG2_SEL",
	/* "TOP_CAMTG3_SEL", */
};

struct clk *g_seninf_tg_mux[_MAX_MCLK_];
atomic_t g_seninf_tg_mux_enable_cnt[_MAX_MCLK_];

typedef enum {
	MCLK_26MHZ,
	MCLK_24MHZ,
	MCLK_52MHZ,
	MCLK_48MHZ,
	MCLK_26MHZ_2,
	MCLK_13MHZ,
	MCLK_12MHZ,
	MCLK_6MHZ,
	MCLK_MAX,
} MCLK_TG_SRC_ENUM;

#define _MAX_TG_SRC_ MCLK_MAX
char *_seninf_tg_src_mclk_name[_MAX_TG_SRC_] = {
	"TOP_CLK26M",
	"TOP_UNIVPLL_D52",
	"TOP_UNIVPLL2_D8",
	"TOP_UNIVPLL_D26",
	"TOP_UNIVPLL2_D16",
	"TOP_UNIVPLL2_D32",
	"TOP_UNIVPLL_D104",
	"TOP_UNIVPLL_D208",
};
struct clk *g_seninf_tg_mclk_src[_MAX_TG_SRC_];
atomic_t g_seninf_tg_mclk_src_enable_cnt[_MAX_TG_SRC_];


#endif
/* kernel standard for PMIC*/
#if !defined(CONFIG_MTK_LEGACY)
/* PMIC */
#include <linux/regulator/consumer.h>
#endif

#define PROC_CAMERA_INFO "driver/camera_info"
#define camera_info_size 4096
#define PDAF_DATA_SIZE 4096
#define MAX_I2C_CMD_LEN          255
char mtk_ccm_name[camera_info_size] = {0};
static unsigned int gDrvIndex = 0;

/*E1(High):490, (Medium):364, (low):273*/
#define ISP_CLK_LOW 273
#define ISP_CLK_MEDIUM 364
#define ISP_CLK_HIGH 490

#ifdef CONFIG_MTK_SMI_EXT
static int current_mmsys_clk = MMSYS_CLK_MEDIUM;
#endif
/*Clock meter*/
/* \drivers\misc\mediatek\base\power\{platform}\mt_pm_init.c */
static DEFINE_SPINLOCK(kdsensor_drv_lock);
static DEFINE_SPINLOCK(kdsensor_drv_i2c_lock);

#define HW_TRIGGER_I2C_SUPPORT 1
/*I2C trigger header file. drivers\i2c\busses\i2c-mtk.h*/
#include "i2c-mtk.h"


#define CAMERA_HW_DRVNAME1  "kd_camera_hw"
#define CAMERA_HW_DRVNAME2  "kd_camera_hw_bus2"
#if HW_TRIGGER_I2C_SUPPORT
#define CAMERA_HW_DRVNAME3  "kd_camera_hw_trigger"
#endif

#if defined(CONFIG_MTK_LEGACY)
static struct i2c_board_info i2c_devs1 __initdata = {I2C_BOARD_INFO(CAMERA_HW_DRVNAME1, 0xfe>>1)};
static struct i2c_board_info i2c_devs2 __initdata = {I2C_BOARD_INFO(CAMERA_HW_DRVNAME2, 0xfe>>1)};
#endif

#if !defined(CONFIG_MTK_LEGACY)
    /*PMIC*/
	struct regulator *regVCAMA = NULL;
	struct regulator *regVCAMD = NULL;
	struct regulator *regVCAMIO = NULL;
	struct regulator *regVCAMAF = NULL;
	struct regulator *regSubVCAMA = NULL;
	struct regulator *regSubVCAMD = NULL;
	struct regulator *regSubVCAMIO = NULL;
	struct regulator *regMain2VCAMA = NULL;
	struct regulator *regMain2VCAMD = NULL;
	struct regulator *regMain2VCAMIO = NULL;
#endif

struct device *sensor_device = NULL;
#define SENSOR_WR32(addr, data)    mt65xx_reg_sync_writel(data, addr)    /* For 89 Only.   // NEED_TUNING_BY_PROJECT */
/* #define SENSOR_WR32(addr, data)    iowrite32(data, addr)    // For 89 Only.   // NEED_TUNING_BY_PROJECT */
#define SENSOR_RD32(addr)          ioread32(addr)

#define TEMPERATURE_INIT_STATE (SENSOR_TEMPERATURE_UNKNOWN_STATUS | \
	SENSOR_TEMPERATURE_CANNOT_SEARCH_SENSOR | \
	SENSOR_TEMPERATURE_NOT_SUPPORT_THERMAL  | \
	SENSOR_TEMPERATURE_NOT_POWER_ON)
/* To record sensor temperature state for thermal use */
MUINT8 gSensorTempState[DUAL_CAMERA_SENSOR_MAX];

/* Test Only!! Open this define for temperature meter UT */
/* Temperature workqueue */
//#define CONFIG_CAM_TEMPERATURE_WORKQUEUE
#ifdef CONFIG_CAM_TEMPERATURE_WORKQUEUE
	static void cam_temperature_report_wq_routine(struct work_struct *);
	struct delayed_work cam_temperature_wq;
#endif

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_sensorlist]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(PFX "[%s] " fmt, __FUNCTION__, ##arg)
//#define PK_INF(fmt, args...)     pr_debug(PFX "[%s] " fmt, __FUNCTION__, ##args)
#define PK_INF(fmt, args...)     pr_debug("[%s] " fmt, __FUNCTION__, ##args)

//#undef DEBUG_CAMERA_HW_K
#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         pr_err(fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
        do {    \
            pr_debug(fmt, ##args); \
        } while (0)
#else
#define PK_DBG(a, ...)
#define PK_ERR(fmt, arg...)             pr_err(fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...)

#endif

/*******************************************************************************
* Proifling
********************************************************************************/
#define PROFILE 1
#if PROFILE
static struct timeval tv1, tv2, tv3, tv4;
/*******************************************************************************
*
********************************************************************************/
inline void KD_IMGSENSOR_PROFILE_INIT(void)
{
    do_gettimeofday(&tv1);
}

/*******************************************************************************
*
********************************************************************************/
inline void KD_IMGSENSOR_PROFILE(char *tag)
{
    unsigned long TimeIntervalUS;

    spin_lock(&kdsensor_drv_lock);

    do_gettimeofday(&tv2);
    TimeIntervalUS = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
    tv1 = tv2;

    spin_unlock(&kdsensor_drv_lock);
    PK_DBG("[%s]Profile = %lu us\n", tag, TimeIntervalUS);
}
/*******************************************************************************
*
********************************************************************************/
inline void KD_IMGSENSOR_PROFILE_INIT_I2C(void)
{
    do_gettimeofday(&tv3);
}

inline void KD_IMGSENSOR_PROFILE_I2C(char *tag, int trans_num)
{
    unsigned long TimeIntervalUS;

    spin_lock(&kdsensor_drv_i2c_lock);

    do_gettimeofday(&tv4);
    TimeIntervalUS = (tv4.tv_sec - tv3.tv_sec) * 1000000 + (tv4.tv_usec - tv3.tv_usec);
    tv3 = tv4;

    spin_unlock(&kdsensor_drv_i2c_lock);
    PK_DBG("[%s]Profile = %lu, trans_num: %d\n", tag, TimeIntervalUS, trans_num);
}

#else
inline static void KD_IMGSENSOR_PROFILE_INIT(void) {}
inline static void KD_IMGSENSOR_PROFILE(char *tag) {}
inline static void KD_IMGSENSOR_PROFILE_INIT_I2C(void) {}
inline static void KD_IMGSENSOR_PROFILE_I2C(char *tag, int trans_num) {}
#endif


/*******************************************************************************
*
********************************************************************************/
extern int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char *mode_name);
/* extern ssize_t strobe_VDIrq(void);  //cotta : add for high current solution */

/*******************************************************************************
*
********************************************************************************/
#ifndef CONFIG_OF
static struct platform_device camerahw_platform_device = {
    .name = "image_sensor",
    .id = 0,
    .dev = {
    .coherent_dma_mask = DMA_BIT_MASK(32),
    }
};
#endif
struct platform_device* pCamerahw_platform_device;
static struct i2c_client *g_pstI2Cclient;
static struct i2c_client *g_pstI2Cclient2;
#if HW_TRIGGER_I2C_SUPPORT
static struct i2c_client *g_pstI2Cclient3;
#endif


/* 81 is used for V4L driver */
static dev_t g_CAMERA_HWdevno = MKDEV(250, 0);
static dev_t g_CAMERA_HWdevno2;
static struct cdev *g_pCAMERA_HW_CharDrv;
static struct cdev *g_pCAMERA_HW_CharDrv2;
static struct class *sensor_class;
static struct class *sensor2_class;

static atomic_t g_CamHWOpend;
static atomic_t g_CamHWOpend2;
static atomic_t g_CamHWOpening;
static atomic_t g_CamDrvOpenCnt;
static atomic_t g_CamDrvOpenCnt2;

/* static u32 gCurrI2CBusEnableFlag = 0; */
static u32 gI2CBusNum = SUPPORT_I2C_BUS_NUM1;

#define SET_I2CBUS_FLAG(_x_)        ((1<<_x_)|(gCurrI2CBusEnableFlag))
#define CLEAN_I2CBUS_FLAG(_x_)      ((~(1<<_x_))&(gCurrI2CBusEnableFlag))

static DEFINE_MUTEX(kdCam_Mutex);
static BOOL bSesnorVsyncFlag = FALSE;
static ACDK_KD_SENSOR_SYNC_STRUCT g_NewSensorExpGain = {128, 128, 128, 128, 1000, 640, 0xFF, 0xFF, 0xFF, 0};


extern MULTI_SENSOR_FUNCTION_STRUCT2 kd_MultiSensorFunc;
static MULTI_SENSOR_FUNCTION_STRUCT2 *g_pSensorFunc = &kd_MultiSensorFunc;
/* static SENSOR_FUNCTION_STRUCT *g_pInvokeSensorFunc[KDIMGSENSOR_MAX_INVOKE_DRIVERS] = {NULL,NULL}; */
/* static BOOL g_bEnableDriver[KDIMGSENSOR_MAX_INVOKE_DRIVERS] = {FALSE,FALSE}; */
BOOL g_bEnableDriver[KDIMGSENSOR_MAX_INVOKE_DRIVERS] = {FALSE, FALSE};
SENSOR_FUNCTION_STRUCT *g_pInvokeSensorFunc[KDIMGSENSOR_MAX_INVOKE_DRIVERS] = {NULL, NULL};
/* static CAMERA_DUAL_CAMERA_SENSOR_ENUM g_invokeSocketIdx[KDIMGSENSOR_MAX_INVOKE_DRIVERS] = {DUAL_CAMERA_NONE_SENSOR,DUAL_CAMERA_NONE_SENSOR}; */
/* static char g_invokeSensorNameStr[KDIMGSENSOR_MAX_INVOKE_DRIVERS][32] = {KDIMGSENSOR_NOSENSOR,KDIMGSENSOR_NOSENSOR}; */
CAMERA_DUAL_CAMERA_SENSOR_ENUM g_invokeSocketIdx[KDIMGSENSOR_MAX_INVOKE_DRIVERS] = {DUAL_CAMERA_NONE_SENSOR, DUAL_CAMERA_NONE_SENSOR};
char g_invokeSensorNameStr[KDIMGSENSOR_MAX_INVOKE_DRIVERS][32] = {KDIMGSENSOR_NOSENSOR, KDIMGSENSOR_NOSENSOR};
/* static int g_SensorExistStatus[3]={0,0,0}; */
static wait_queue_head_t kd_sensor_wait_queue;
bool setExpGainDoneFlag = 0;
static unsigned int g_CurrentSensorIdx;
static unsigned int g_IsSearchSensor;

MSDK_SENSOR_INFO_STRUCT ginfo[2];
MSDK_SENSOR_INFO_STRUCT ginfo1[2];
MSDK_SENSOR_INFO_STRUCT ginfo2[2];
MSDK_SENSOR_INFO_STRUCT ginfo3[2];
MSDK_SENSOR_INFO_STRUCT ginfo4[2];

/*=============================================================================

=============================================================================*/
/*******************************************************************************
* i2c relative start
* migrate new style i2c driver interfaces required by Kirby 20100827
********************************************************************************/
static const struct i2c_device_id CAMERA_HW_i2c_id[] = {{CAMERA_HW_DRVNAME1, 0}, {} };
static const struct i2c_device_id CAMERA_HW_i2c_id2[] = {{CAMERA_HW_DRVNAME2, 0}, {} };
#if HW_TRIGGER_I2C_SUPPORT
static const struct i2c_device_id CAMERA_HW_i2c_id3[] = {{CAMERA_HW_DRVNAME3, 0}, {} };
#endif


/*******************************************************************************
* general camera image sensor kernel driver
*******************************************************************************/
UINT32 kdGetSensorInitFuncList(ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT **ppSensorList)
{
    if (NULL == ppSensorList)
    {
    PK_ERR("[kdGetSensorInitFuncList]ERROR: NULL ppSensorList\n");
    return 1;
    }
    *ppSensorList = &kdSensorList[0];
    return 0;
} /* kdGetSensorInitFuncList() */


/*******************************************************************************
*iMultiReadReg
********************************************************************************/
int iMultiReadReg(u16 a_u2Addr , u8 *a_puBuff , u16 i2cId, u8 number)
{
    int  i4RetValue = 0;
    char puReadCmd[2] = {(char)(a_u2Addr >> 8) , (char)(a_u2Addr & 0xFF)};

    if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
    spin_lock(&kdsensor_drv_lock);

    g_pstI2Cclient->addr = (i2cId >> 1);

    spin_unlock(&kdsensor_drv_lock);

    /*  */
    i4RetValue = i2c_master_send(g_pstI2Cclient, puReadCmd, 2);
    if (i4RetValue != 2) {
        PK_ERR("[CAMERA SENSOR] I2C send failed, addr = 0x%x, data = 0x%x !!\n", a_u2Addr,  *a_puBuff);
        return -1;
    }
    /*  */
    i4RetValue = i2c_master_recv(g_pstI2Cclient, (char *)a_puBuff, number);
    if (i4RetValue != 1) {
        PK_ERR("[CAMERA SENSOR] I2C read failed!!\n");
        return -1;
    }
    }
    else {
    spin_lock(&kdsensor_drv_lock);
    g_pstI2Cclient2->addr = (i2cId >> 1);
    spin_unlock(&kdsensor_drv_lock);
    /*  */
    i4RetValue = i2c_master_send(g_pstI2Cclient2, puReadCmd, 2);
    if (i4RetValue != 2) {
        PK_ERR("[CAMERA SENSOR] I2C send failed, addr = 0x%x, data = 0x%x !!\n", a_u2Addr,  *a_puBuff);
        return -1;
    }
    /*  */
    i4RetValue = i2c_master_recv(g_pstI2Cclient2, (char *)a_puBuff, number);
    if (i4RetValue != 1) {
        PK_ERR("[CAMERA SENSOR] I2C read failed!!\n");
        return -1;
    }
    }
    return 0;
}


/*******************************************************************************
* iReadReg
********************************************************************************/
int iReadReg(u16 a_u2Addr , u8 *a_puBuff , u16 i2cId)
{
    int  i4RetValue = 0;
    char puReadCmd[2] = {(char)(a_u2Addr >> 8) , (char)(a_u2Addr & 0xFF)};

    if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
	    spin_lock(&kdsensor_drv_lock);

	    g_pstI2Cclient->addr = (i2cId >> 1);
#ifdef CONFIG_MTK_I2C_EXTENSION
	    g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_DMA_FLAG);

	    /* Remove i2c ack error log during search sensor */
	    if (g_IsSearchSensor == 1)
	        g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag) | I2C_A_FILTER_MSG;
	    else
	        g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_A_FILTER_MSG);
#endif

	    spin_unlock(&kdsensor_drv_lock);

	    /*  */
	    i4RetValue = i2c_master_send(g_pstI2Cclient, puReadCmd, 2);
	    if (i4RetValue != 2) {
	        PK_ERR("[CAMERA SENSOR] I2C send failed, addr = 0x%x, data = 0x%x !!\n", a_u2Addr,  *a_puBuff);
	        return -1;
	    }
	    /*  */
	    i4RetValue = i2c_master_recv(g_pstI2Cclient, (char *)a_puBuff, 1);
	    if (i4RetValue != 1) {
	        PK_ERR("[CAMERA SENSOR] I2C read failed!!\n");
	        return -1;
	    }
    }
    else {
	    spin_lock(&kdsensor_drv_lock);
	    g_pstI2Cclient2->addr = (i2cId >> 1);
#ifdef CONFIG_MTK_I2C_EXTENSION
	    /* Remove i2c ack error log during search sensor */
	    if (g_IsSearchSensor == 1)
	        g_pstI2Cclient2->ext_flag = (g_pstI2Cclient2->ext_flag) | I2C_A_FILTER_MSG;
	    else
	        g_pstI2Cclient2->ext_flag = (g_pstI2Cclient2->ext_flag)&(~I2C_A_FILTER_MSG);
#endif
	    spin_unlock(&kdsensor_drv_lock);
	    /*  */
	    i4RetValue = i2c_master_send(g_pstI2Cclient2, puReadCmd, 2);
	    if (i4RetValue != 2) {
	        PK_ERR("[CAMERA SENSOR] I2C send failed, addr = 0x%x, data = 0x%x !!\n", a_u2Addr,  *a_puBuff);
	        return -1;
	    }
	    /*  */
	    i4RetValue = i2c_master_recv(g_pstI2Cclient2, (char *)a_puBuff, 1);
	    if (i4RetValue != 1) {
	        PK_ERR("[CAMERA SENSOR] I2C read failed!!\n");
	        return -1;
	    }
    }
    return 0;
}

/*******************************************************************************
* iReadRegI2C
********************************************************************************/
int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId)
{
    int  i4RetValue = 0;
	if (g_IsSearchSensor != 1){
		if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
			spin_lock(&kdsensor_drv_lock);
			g_pstI2Cclient->addr = (i2cId >> 1);
#ifdef CONFIG_MTK_I2C_EXTENSION
			g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_DMA_FLAG);

			/* Remove i2c ack error log during search sensor */
			/* PK_ERR("g_pstI2Cclient->ext_flag: %d", g_IsSearchSensor); */
			if (g_IsSearchSensor == 1)
				g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag) | I2C_A_FILTER_MSG;
			else
				g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_A_FILTER_MSG);
#endif
			spin_unlock(&kdsensor_drv_lock);
			/*	*/
			i4RetValue = i2c_master_send(g_pstI2Cclient, a_pSendData, a_sizeSendData);
			if (i4RetValue != a_sizeSendData) {
				PK_ERR("[CAMERA SENSOR] I2C send failed!!, Addr = 0x%x\n", a_pSendData[0]);
				return -1;
			}

			i4RetValue = i2c_master_recv(g_pstI2Cclient, (char *)a_pRecvData, a_sizeRecvData);
			if (i4RetValue != a_sizeRecvData) {
				PK_ERR("[CAMERA SENSOR] I2C read failed!!\n");
				return -1;
			}
		}
		else{
			spin_lock(&kdsensor_drv_lock);
			g_pstI2Cclient2->addr = (i2cId >> 1);
#ifdef CONFIG_MTK_I2C_EXTENSION
			/* Remove i2c ack error log during search sensor */
			/* PK_ERR("g_pstI2Cclient2->ext_flag: %d", g_IsSearchSensor); */
			if (g_IsSearchSensor == 1)
				g_pstI2Cclient2->ext_flag = (g_pstI2Cclient2->ext_flag) | I2C_A_FILTER_MSG;
			else
				g_pstI2Cclient2->ext_flag = (g_pstI2Cclient2->ext_flag)&(~I2C_A_FILTER_MSG);
#endif
			spin_unlock(&kdsensor_drv_lock);
			i4RetValue = i2c_master_send(g_pstI2Cclient2, a_pSendData, a_sizeSendData);
			if (i4RetValue != a_sizeSendData) {
				PK_ERR("[CAMERA SENSOR] I2C send failed!!, Addr = 0x%x\n", a_pSendData[0]);
				return -1;
			}

			i4RetValue = i2c_master_recv(g_pstI2Cclient2, (char *)a_pRecvData, a_sizeRecvData);
			if (i4RetValue != a_sizeRecvData) {
				PK_ERR("[CAMERA SENSOR] I2C read failed!!\n");
				return -1;
			}
		}
	}
	else{
		int ret = 0;
		u32 speed_timing;
		u16 i2c_msg_size;
		struct i2c_client *pClient = NULL;
		struct i2c_msg msg[2];

		if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1)
			pClient = g_pstI2Cclient;
		else
			pClient = g_pstI2Cclient2;

		speed_timing = 300000;
		/* PK_DBG("Addr : 0x%x,Val : 0x%x\n",a_u2Addr,a_u4Data); */

		msg[0].addr = i2cId >> 1;
		msg[0].flags = 0; /*write flag = 0*/
		msg[0].len = a_sizeSendData;
		msg[0].buf = a_pSendData;

		msg[1].addr = i2cId >> 1;
		msg[1].flags = I2C_M_RD; /*Read flag = 0*/
		msg[1].len = a_sizeRecvData;
		msg[1].buf = a_pRecvData;
		i2c_msg_size = 2;
		ret = mtk_i2c_transfer(pClient->adapter, msg, i2c_msg_size, I2C_A_FILTER_MSG, speed_timing);

		if (i2c_msg_size != ret) {
			PK_ERR("[iReadRegI2C]I2C failed(0x%x)! Data[0]=0x%x, Data[1]=0x%x,timing(0=%d)\n",
				ret, a_pSendData[0], a_pSendData[1],speed_timing);
		}
	}

    return 0;
}


/*******************************************************************************
* iReadRegI2C with speed
********************************************************************************/
int iReadRegI2CTiming(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId, u16 timing)
{
	int ret = 0;
	u32 speed_timing;
	u16 i2c_msg_size;
	struct i2c_client *pClient = NULL;
	struct i2c_msg msg[2];

	if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1)
		pClient = g_pstI2Cclient;
	else
		pClient = g_pstI2Cclient2;

	if((timing > 0) && (timing <= 1000))
		speed_timing = timing*1000; /*unit:hz*/
	else
		speed_timing = 400000;
    /* PK_DBG("Addr : 0x%x,Val : 0x%x\n",a_u2Addr,a_u4Data); */

	msg[0].addr = i2cId >> 1;
	msg[0].flags = 0; /*write flag = 0*/
	msg[0].len = a_sizeSendData;
	msg[0].buf = a_pSendData;

	msg[1].addr = i2cId >> 1;
	msg[1].flags = I2C_M_RD; /*Read flag = 0*/
	msg[1].len = a_sizeRecvData;
	msg[1].buf = a_pRecvData;
	i2c_msg_size = 2;
	if(g_IsSearchSensor == 1)
		ret = mtk_i2c_transfer(pClient->adapter, msg, i2c_msg_size, I2C_A_FILTER_MSG, speed_timing);
	else
		ret = mtk_i2c_transfer(pClient->adapter, msg, i2c_msg_size, 0, speed_timing);

	if (i2c_msg_size != ret) {
		PK_ERR("[iReadRegI2CTiming] I2C send failed (0x%x)! timing(0=%d) \n", ret,speed_timing);
		ret = -1;
	}
    return ret;
}

/*******************************************************************************
* iWriteReg
********************************************************************************/
int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId)
{
    int  i4RetValue = 0;
    int u4Index = 0;
    u8 *puDataInBytes = (u8 *)&a_u4Data;
    int retry = 3;

    char puSendCmd[6] = {(char)(a_u2Addr >> 8) , (char)(a_u2Addr & 0xFF) ,
    0 , 0 , 0 , 0};

	/* PK_DBG("Addr : 0x%x,Val : 0x%x\n",a_u2Addr,a_u4Data); */

    /* KD_IMGSENSOR_PROFILE_INIT(); */
    spin_lock(&kdsensor_drv_lock);

    if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
    g_pstI2Cclient->addr = (i2cId >> 1);
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_DMA_FLAG);
    #endif
    }
    else {
    g_pstI2Cclient2->addr = (i2cId >> 1);
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient2->ext_flag = (g_pstI2Cclient2->ext_flag)&(~I2C_DMA_FLAG);
    #endif
    }
    spin_unlock(&kdsensor_drv_lock);


    if (a_u4Bytes > 2)
    {
    PK_ERR("[CAMERA SENSOR] exceed 2 bytes\n");
    return -1;
    }

    if (a_u4Data >> (a_u4Bytes << 3))
    {
    PK_DBG("[CAMERA SENSOR] warning!! some data is not sent!!\n");
    }

    for (u4Index = 0; u4Index < a_u4Bytes; u4Index += 1)
    {
    puSendCmd[(u4Index + 2)] = puDataInBytes[(a_u4Bytes - u4Index-1)];
    }
    /*  */
    do {
       if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
        i4RetValue = i2c_master_send(g_pstI2Cclient, puSendCmd, (a_u4Bytes + 2));
       }
       else {
        i4RetValue = i2c_master_send(g_pstI2Cclient2, puSendCmd, (a_u4Bytes + 2));
       }
    if (i4RetValue != (a_u4Bytes + 2)) {
    PK_ERR("[CAMERA SENSOR] I2C send failed addr = 0x%x, data = 0x%x !!\n", a_u2Addr, a_u4Data);
	i4RetValue = -1;
    }
    else {
	i4RetValue = 0;
        break;
    }
    uDELAY(50);
    } while ((retry--) > 0);
    /* KD_IMGSENSOR_PROFILE("iWriteReg"); */
	return i4RetValue;
}
/*******************************************************************************
* iWriteRegI2C
********************************************************************************/
int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId)
{
    int  i4RetValue = 0;

	if (g_IsSearchSensor != 1){
		int retry = 3;
		u32 speed_timing;

		struct i2c_client *pClient = NULL;

		if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1)
			pClient = g_pstI2Cclient;
		else
			pClient = g_pstI2Cclient2;

		speed_timing = 400000;

		/* PK_DBG("Addr : 0x%x,Val : 0x%x\n",a_u2Addr,a_u4Data); */

		/* KD_IMGSENSOR_PROFILE_INIT(); */
		spin_lock(&kdsensor_drv_lock);
		pClient->addr = (i2cId >> 1);
#ifdef CONFIG_MTK_I2C_EXTENSION
		pClient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_DMA_FLAG);
#endif
		spin_unlock(&kdsensor_drv_lock);
		/*	*/
		do {
			i4RetValue = i2c_master_send(pClient, a_pSendData, a_sizeSendData);
			if (i4RetValue != a_sizeSendData) {
				PK_ERR("[CAMERA SENSOR] I2C send failed!!, Addr = 0x%x, Data = 0x%x\n", a_pSendData[0], a_pSendData[1]);
				if (i4RetValue == -ETIMEDOUT) /*time out, do no retry*/
					break;
				i4RetValue = -1;
			}
			else {
				i4RetValue = 0;
				break;
			}
			uDELAY(50);
		} while ((retry--) > 0);
		/* KD_IMGSENSOR_PROFILE("iWriteRegI2C"); */
	}
	else
	{
		u32 speed_timing;
		int retry = 3;
		u16 i2c_msg_size;
		struct i2c_msg msg[2];
		struct i2c_client *pClient = NULL;

		if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1)
			pClient = g_pstI2Cclient;
		else
			pClient = g_pstI2Cclient2;

		speed_timing = 300000;

		/* PK_DBG("Addr : 0x%x,Val : 0x%x\n",a_u2Addr,a_u4Data); */
		msg[0].addr = i2cId >> 1;
		msg[0].flags = 0; /*write flag = 0*/
		msg[0].len = a_sizeSendData;
		msg[0].buf = a_pSendData;
		i2c_msg_size = 1;
		do {
			i4RetValue = mtk_i2c_transfer(pClient->adapter, msg, i2c_msg_size, I2C_A_FILTER_MSG, speed_timing);
			if (i4RetValue != i2c_msg_size) {
				PK_ERR("[iWriteRegI2CTiming]I2C failed(0x%x)! Data[0]=0x%x, Data[1]=0x%x,timing(0=%d)\n",
					i4RetValue, a_pSendData[0], a_pSendData[1],speed_timing);
				if (i4RetValue == -ETIMEDOUT) /*time out, do no retry*/
					break;
				i4RetValue = -1;
			}
			else {
				i4RetValue = 0;
				break;
			}
			uDELAY(50);
		}while (( --retry) > 0); // retry 3 times
	}

    return i4RetValue;
}

/*******************************************************************************
* iWriteRegI2C with speed control
********************************************************************************/
int iWriteRegI2CTiming(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId, u16 timing)
{
	u32 speed_timing;
	int retry = 3;
	int ret = 0;
	u16 i2c_msg_size;
	struct i2c_msg msg[2];
	struct i2c_client *pClient = NULL;

	if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1)
		pClient = g_pstI2Cclient;
	else
		pClient = g_pstI2Cclient2;

	if((timing > 0) && (timing <= 1000))
		speed_timing = timing*1000; /*unit:hz*/
	else
		speed_timing = 400000;

    /* PK_DBG("Addr : 0x%x,Val : 0x%x\n",a_u2Addr,a_u4Data); */
	msg[0].addr = i2cId >> 1;
	msg[0].flags = 0; /*write flag = 0*/
	msg[0].len = a_sizeSendData;
	msg[0].buf = a_pSendData;
	i2c_msg_size = 1;
	do {
		ret = mtk_i2c_transfer(pClient->adapter, msg, i2c_msg_size, 0, speed_timing);
		//PK_ERR("[iWriteRegI2CTiming]ret(%d) i2c_msg_size(%d)\n", ret, i2c_msg_size);
		if (ret != i2c_msg_size) {
			PK_ERR("[iWriteRegI2CTiming]i2c failed(0x%x)! Data[0]=0x%x, Data[1]=0x%x,timing(0=%d), retry(0=%d)\n",
				ret, a_pSendData[0], a_pSendData[1], speed_timing, retry);
			if (ret == -ETIMEDOUT) /*timedout; not retry*/
				break;
			ret = -1;
		}
		else {
			ret = 0;
			break;
		}
		uDELAY(50);
	}while ((retry--) > 0);

    return ret;
}

int kdSetI2CBusNum(u32 i2cBusNum) {
    //Main2 Support
    //if ((i2cBusNum != SUPPORT_I2C_BUS_NUM2) && (i2cBusNum != SUPPORT_I2C_BUS_NUM1)) {
    if ((i2cBusNum != SUPPORT_I2C_BUS_NUM3) && (i2cBusNum != SUPPORT_I2C_BUS_NUM2) && (i2cBusNum != SUPPORT_I2C_BUS_NUM1)) {
    PK_ERR("[kdSetI2CBusNum] i2c bus number is not correct(%d)\n", i2cBusNum);
    return -1;
    }
    spin_lock(&kdsensor_drv_lock);
    gI2CBusNum = i2cBusNum;
    spin_unlock(&kdsensor_drv_lock);

    return 0;
}
#if 0
void kdSetI2CSpeed(u32 i2cSpeed)
{
     if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
    spin_lock(&kdsensor_drv_lock);
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient->timing = i2cSpeed;
    #endif
    spin_unlock(&kdsensor_drv_lock);
     }
     else{
    spin_lock(&kdsensor_drv_lock);
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient2->timing = i2cSpeed;
    #endif
    spin_unlock(&kdsensor_drv_lock);
     }
}
#endif
/*******************************************************************************
* kdReleaseI2CTriggerLock
********************************************************************************/
int kdReleaseI2CTriggerLock(void)
{
    int ret = 0;

    /* ret = mt_wait4_i2c_complete(); */

    /* if (ret < 0 ) { */
    /* PK_DBG("[error]wait i2c fail\n"); */
    /* } */

    return ret;
}
/*******************************************************************************
* iBurstWriteReg
********************************************************************************/
struct i2c_msg msg[MAX_I2C_CMD_LEN];

#if HW_TRIGGER_I2C_SUPPORT
int iBurstWriteReg_HW(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length, u16 timing){

	int ret = 0;
	int i = 0 ;
	u32 speed_timing = 0;
	struct i2c_client *pClient = NULL;
	int trans_num = 0;
	trans_num =	bytes/transfer_length;
	memset(msg, 0, MAX_I2C_CMD_LEN*sizeof(struct i2c_msg));

	if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1)
		pClient = g_pstI2Cclient3;
	else {
		PK_ERR("iBurstWriteReg_HW only support main cam for now\n");
		return -1;
	}

	if((timing > 0) && (timing <= 1000))
		speed_timing = timing*1000; /*unit:hz*/
	else
		speed_timing = 400000;

	for(i = 0 ; i<trans_num ; i++){
		msg[i].addr = i2cId >> 1;
		msg[i].flags = 0;
		msg[i].len = transfer_length;
		msg[i].buf = pData+(i*transfer_length);
	}

	ret = hw_trig_i2c_transfer(pClient->adapter, msg, trans_num);
	return ret;
}

int i2c_buf_mode_en(int enable)
{
	int ret = 0;
	struct i2c_client *pClient = NULL;
	PK_ERR("i2c_buf_mode_en %d\n",enable);
	if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1)
		pClient = g_pstI2Cclient3;
	else {
		PK_ERR("i2c_buf_mode_en only support main cam for now\n");
		return -1;
	}
	if(enable)
		ret = hw_trig_i2c_enable(pClient->adapter);
	else
		ret = hw_trig_i2c_disable(pClient->adapter);
	return ret;
}
#else
int iBurstWriteReg_HW(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length, u16 timing){
	PK_ERR("plz #define HW_TRIGGER_I2C_SUPPORT 1\n");
        return -1;
}
int i2c_buf_mode_en(bool enable)
{
	PK_ERR("plz #define HW_TRIGGER_I2C_SUPPORT 1\n");
        return -1;
}

#endif


int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length, u16 timing)
{
#if 1//ndef CONFIG_MTK_LEGACY
	int ret = 0;
	int i = 0 ;
	u32 speed_timing = 0;
	struct i2c_client *pClient = NULL;
	int trans_num = 0;

    /* KD_IMGSENSOR_PROFILE_INIT_I2C();*/

    trans_num =	bytes/transfer_length;
	memset(msg, 0, MAX_I2C_CMD_LEN*sizeof(struct i2c_msg));

	if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1)
		pClient = g_pstI2Cclient;
	else
		pClient = g_pstI2Cclient2;

	if((timing > 0) && (timing <= 1000))
		speed_timing = timing*1000; /*unit:hz*/
	else
		speed_timing = 400000;


	for(i = 0 ; i<trans_num ; i++){
		msg[i].addr = i2cId >> 1;
		msg[i].flags = 0;
		msg[i].len = transfer_length;
		msg[i].buf = pData+(i*transfer_length);
	}


	ret = mtk_i2c_transfer(pClient->adapter, msg, trans_num, 0, speed_timing);
    /* KD_IMGSENSOR_PROFILE_I2C("i2c multi write", trans_num); */

	if (ret != trans_num) {
		PK_ERR("[iBurstWriteReg_multi] I2C send failed (0x%x)! timing(0=%d) \n", ret,speed_timing);
	}
	return ret;
#else
    uintptr_t phyAddr;
    u8 *buf = NULL;
    u32 old_addr = 0;
    int ret = 0;
    int retry = 0;

    if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
    if (bytes > MAX_I2C_CMD_LEN) {
        PK_ERR("[iBurstWriteReg] exceed the max write length\n");
        return 1;
    }

    phyAddr = 0;

    buf = dma_alloc_coherent(&(pCamerahw_platform_device->dev) , bytes, (dma_addr_t *)&phyAddr, GFP_KERNEL);

    if (NULL == buf) {
        PK_ERR("[iBurstWriteReg] Not enough memory\n");
        return -1;
    }
    memset(buf, 0, bytes);
    memcpy(buf, pData, bytes);
    /* PK_DBG("[iBurstWriteReg] bytes = %d, phy addr = 0x%x\n", bytes, phyAddr ); */
	PK_DBG("transfer_length %d\n",bytes == transfer_length ? transfer_length : (((bytes/transfer_length)<<16)|transfer_length));

    old_addr = g_pstI2Cclient->addr;
    spin_lock(&kdsensor_drv_lock);
       g_pstI2Cclient->addr = (i2cId >> 1);
       #ifdef CONFIG_MTK_I2C_EXTENSION
       g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);
       g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_POLLING_FLAG);
       #endif
    spin_unlock(&kdsensor_drv_lock);

    ret = 0;
    retry = 3;
    do {
        ret = i2c_master_send(g_pstI2Cclient, (u8 *)phyAddr,
        bytes == transfer_length ? transfer_length : ((bytes/transfer_length)<<16)|transfer_length);
        retry--;
        if ((ret&0xffff) != transfer_length) {
        PK_ERR("Error sent I2C ret = %d\n", ret);
        }
    } while (((ret&0xffff) != transfer_length) && (retry > 0));

    dma_free_coherent(&(pCamerahw_platform_device->dev), bytes, buf, phyAddr);
    spin_lock(&kdsensor_drv_lock);
    g_pstI2Cclient->addr = old_addr;
    spin_unlock(&kdsensor_drv_lock);
    }
    else
    {
    if (bytes > MAX_I2C_CMD_LEN) {
        PK_DBG("[iBurstWriteReg] exceed the max write length\n");
        return 1;
    }
    phyAddr = 0;
    buf = dma_alloc_coherent(&(pCamerahw_platform_device->dev), bytes, (dma_addr_t *)&phyAddr, GFP_KERNEL);

    if (NULL == buf) {
        PK_ERR("[iBurstWriteReg] Not enough memory\n");
        return -1;
    }
    memset(buf, 0, bytes);
    memcpy(buf, pData, bytes);
    /* PK_DBG("[iBurstWriteReg] bytes = %d, phy addr = 0x%x\n", bytes, phyAddr ); */

    old_addr = g_pstI2Cclient2->addr;
    spin_lock(&kdsensor_drv_lock);
    g_pstI2Cclient2->addr = (i2cId >> 1);
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient2->ext_flag = (g_pstI2Cclient2->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);
    g_pstI2Cclient2->ext_flag = (g_pstI2Cclient2->ext_flag)&(~I2C_POLLING_FLAG);
    #endif
    spin_unlock(&kdsensor_drv_lock);
    ret = 0;
    retry = 3;
    do {
        ret = i2c_master_send(g_pstI2Cclient2, (u8 *)phyAddr,
        bytes == transfer_length ? transfer_length : ((bytes/transfer_length)<<16)|transfer_length);
        retry--;
        if ((ret&0xffff) != transfer_length) {
        PK_ERR("Error sent I2C ret = %d\n", ret);
        }
    } while (((ret&0xffff) != transfer_length) && (retry > 0));


    dma_free_coherent(&(pCamerahw_platform_device->dev), bytes, buf, phyAddr);
    spin_lock(&kdsensor_drv_lock);
    g_pstI2Cclient2->addr = old_addr;
    spin_unlock(&kdsensor_drv_lock);
    }
    return 0;
#endif
}

int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId) {
    return  iBurstWriteReg_multi(pData, bytes, i2cId, bytes,0);
}


/*******************************************************************************
* iMultiWriteReg
********************************************************************************/

int iMultiWriteReg(u8 *pData, u16 lens, u16 i2cId)
{
    int ret = 0;

    if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
    g_pstI2Cclient->addr = (i2cId >> 1);
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)|(I2C_DMA_FLAG);
    #endif
    ret = i2c_master_send(g_pstI2Cclient, pData, lens);
    }
    else {
    g_pstI2Cclient2->addr = (i2cId >> 1);
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient2->ext_flag = (g_pstI2Cclient2->ext_flag)|(I2C_DMA_FLAG);
    #endif
    ret = i2c_master_send(g_pstI2Cclient2, pData, lens);
    }

    if (ret != lens) {
    PK_ERR("Error sent I2C ret = %d\n", ret);
    }
    return 0;
}

MUINT32 Get_Camera_Temperature(CAMERA_DUAL_CAMERA_SENSOR_ENUM senDevId, MUINT8 *invalid, INT32 *temp)
{

	MUINT32 ret = ERROR_NONE;
	MUINT32 i = 0, FeatureParaLen = 0;

	mutex_lock(&kdCam_Mutex);
	*invalid = gSensorTempState[senDevId];
	*temp = 0;
	FeatureParaLen = sizeof(unsigned long long);

	for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
	if (g_bEnableDriver[i] && g_pInvokeSensorFunc[i]) {
		if (senDevId == g_invokeSocketIdx[i]) {
			 /* If temperature is support, get temperature from sensor by I2C*/
			if (*invalid == SENSOR_TEMPERATURE_VALID) {
				/* Set I2CBus */
				if (DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
					 spin_lock(&kdsensor_drv_lock);
					 gI2CBusNum = SUPPORT_I2C_BUS_NUM3;
					 spin_unlock(&kdsensor_drv_lock);
				}
				else if (DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i]) {
					 spin_lock(&kdsensor_drv_lock);
					 gI2CBusNum = SUPPORT_I2C_BUS_NUM2;
					 spin_unlock(&kdsensor_drv_lock);
				}
				else {
					 spin_lock(&kdsensor_drv_lock);
					 gI2CBusNum = SUPPORT_I2C_BUS_NUM1;
					 spin_unlock(&kdsensor_drv_lock);
				}
				ret = g_pInvokeSensorFunc[i]->SensorFeatureControl(\
					SENSOR_FEATURE_GET_TEMPERATURE_VALUE, (MUINT8 *)temp, (MUINT32 *)&FeatureParaLen);
				 if (ERROR_NONE != ret) {
					 PK_ERR("[%s]\n", __func__);
					 return ret;
				}
			}
		}
	}
	}

	mutex_unlock(&kdCam_Mutex);

	PK_DBG("senDevId(%d), invalid(0X%x), temperature(%d)\n", senDevId, *invalid, *(INT32 *)temp);
	return ret;
}
EXPORT_SYMBOL(Get_Camera_Temperature);

#ifdef CONFIG_CAM_TEMPERATURE_WORKQUEUE
static void cam_temperature_report_wq_routine(struct work_struct *data)
{
	MUINT8 invalid[4] = {0, 0, 0, 0};
	INT32 temp[4] = {0, 0, 0, 0};
    MUINT32 ret = 0;

	PK_DBG("Temperature Meter Report.\n");

	/* Main cam */
	ret = Get_Camera_Temperature(DUAL_CAMERA_MAIN_SENSOR, &invalid[0], &temp[0]);
	PK_INF("senDevId(%d), invalid(0X%x), temperature(%d)\n", \
		DUAL_CAMERA_MAIN_SENSOR, invalid[0], temp[0]);
	if(ERROR_NONE != ret)
		PK_ERR("Get Main cam temperature error(%d)!\n", ret);

	/* Sub cam */
	ret = Get_Camera_Temperature(DUAL_CAMERA_SUB_SENSOR, &invalid[1], &temp[1]);
	PK_INF("senDevId(%d), invalid(0X%x), temperature(%d)\n", \
		DUAL_CAMERA_SUB_SENSOR, invalid[1], temp[1]);
	if(ERROR_NONE != ret)
		PK_ERR("Get Sub cam temperature error(%d)!\n", ret);

	/* Main2 cam */
	ret = Get_Camera_Temperature(DUAL_CAMERA_MAIN_2_SENSOR, &invalid[2], &temp[2]);
	PK_INF("senDevId(%d), invalid(0X%x), temperature(%d)\n", \
		DUAL_CAMERA_MAIN_2_SENSOR, invalid[2], temp[2]);
	if(ERROR_NONE != ret)
		PK_ERR("Get Main2 cam temperature error(%d)!\n", ret);

	/* Sub2 cam */
	ret = Get_Camera_Temperature(DUAL_CAMERA_SUB_2_SENSOR, &invalid[3], &temp[3]);
	PK_INF("senDevId(%d), invalid(0X%x), temperature(%d)\n", \
		DUAL_CAMERA_SUB_2_SENSOR, invalid[3], temp[3]);
	if(ERROR_NONE != ret)
		PK_ERR("Get Sub2 cam temperature error(%d)!\n", ret);

	schedule_delayed_work(&cam_temperature_wq, HZ);

}
#endif

/*******************************************************************************
* sensor function adapter
********************************************************************************/
#define KD_MULTI_FUNCTION_ENTRY()   /* PK_XLOG_INFO("[%s]:E\n",__FUNCTION__) */
#define KD_MULTI_FUNCTION_EXIT()    /* PK_XLOG_INFO("[%s]:X\n",__FUNCTION__) */
/*  */
MUINT32
kdSetI2CSlaveID(MINT32 i, MUINT32 socketIdx, MUINT32 firstSet) {
unsigned long long FeaturePara[4];
MUINT32 FeatureParaLen = 0;
    FeaturePara[0] = socketIdx;
    FeaturePara[1] = firstSet;
    FeatureParaLen = sizeof(unsigned long long)*2;
    return g_pInvokeSensorFunc[i]->SensorFeatureControl(SENSOR_FEATURE_SET_SLAVE_I2C_ID, (MUINT8 *)FeaturePara, (MUINT32 *)&FeatureParaLen);
}

/*  */
MUINT32
kd_MultiSensorOpen(void)
{
MUINT32 ret = ERROR_NONE;
MINT32 i = 0;

#ifdef CONFIG_MTK_CCU
	struct ccu_sensor_info ccuSensorInfo;
#endif
    KD_MULTI_FUNCTION_ENTRY();

    /* from hear to tail */
    /* for ( i = KDIMGSENSOR_INVOKE_DRIVER_0 ; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS ; i++ ) { */
    /* from tail to head. */
    for (i = (KDIMGSENSOR_MAX_INVOKE_DRIVERS-1); i >= KDIMGSENSOR_INVOKE_DRIVER_0; i--) {
    if (g_bEnableDriver[i] && g_pInvokeSensorFunc[i]) {
        if (0 != (g_CurrentSensorIdx & g_invokeSocketIdx[i])) {
        /* turn on power */
        KD_IMGSENSOR_PROFILE_INIT();
        ret = kdCISModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM)g_invokeSocketIdx[i], (char *)g_invokeSensorNameStr[i], true, CAMERA_HW_DRVNAME1);
        if (ERROR_NONE != ret) {
        PK_ERR("[%s]", __func__);
        return ret;
        }
        /* wait for power stable */
        mDELAY(5);
        KD_IMGSENSOR_PROFILE("kdCISModulePowerOn");

#if 0
        if (DUAL_CAMERA_MAIN_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SENSOR_I2C_BUS_NUM[g_invokeSocketIdx[i]];
        spin_unlock(&kdsensor_drv_lock);
        PK_XLOG_INFO("kd_MultiSensorOpen: switch I2C BUS%d\n", gI2CBusNum);
        }
#else
        if (DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
            spin_lock(&kdsensor_drv_lock);
            gI2CBusNum = SUPPORT_I2C_BUS_NUM3;
            spin_unlock(&kdsensor_drv_lock);
            PK_XLOG_INFO("kd_MultiSensorOpen: switch I2C BUS%d\n", gI2CBusNum);
        }

        else if (DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i]) {
            spin_lock(&kdsensor_drv_lock);
            gI2CBusNum = SUPPORT_I2C_BUS_NUM2;
            spin_unlock(&kdsensor_drv_lock);
            PK_XLOG_INFO("kd_MultiSensorOpen: switch I2C BUS%d\n", gI2CBusNum);
        }
        else {
            spin_lock(&kdsensor_drv_lock);
            gI2CBusNum = SUPPORT_I2C_BUS_NUM1;
            spin_unlock(&kdsensor_drv_lock);
            PK_XLOG_INFO("kd_MultiSensorOpen: switch I2C BUS%d\n", gI2CBusNum);
        }
#endif
        /*  */
        /* set i2c slave ID */
        /* KD_SET_I2C_SLAVE_ID(i,g_invokeSocketIdx[i],IMGSENSOR_SET_I2C_ID_STATE); */
        /*  */
        ret = g_pInvokeSensorFunc[i]->SensorOpen();
        if (ERROR_NONE != ret) {
        kdCISModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM)g_invokeSocketIdx[i], (char *)g_invokeSensorNameStr[i], false, CAMERA_HW_DRVNAME1);
        PK_ERR("SensorOpen");
        return ret;
		}
        KD_IMGSENSOR_PROFILE("SensorOpen");

		g_pInvokeSensorFunc[i]->sensorState = SENSOR_STATE_OPEN; /* State: sensor init but not streaming*/

		gSensorTempState[g_invokeSocketIdx[i]] &= ~SENSOR_TEMPERATURE_NOT_POWER_ON; /* Sensor is power on */
		if((gSensorTempState[g_invokeSocketIdx[i]] & TEMPERATURE_INIT_STATE) == 0) /* Set temperature valid */
			gSensorTempState[g_invokeSocketIdx[i]] |= SENSOR_TEMPERATURE_VALID;
        /* set i2c slave ID */
        /* SensorOpen() will reset i2c slave ID */
        /* KD_SET_I2C_SLAVE_ID(i,g_invokeSocketIdx[i],IMGSENSOR_SET_I2C_ID_FORCE); */
#ifdef CONFIG_MTK_CCU
					if (g_invokeSocketIdx[i] == DUAL_CAMERA_MAIN_SENSOR) {
						ccuSensorInfo.slave_addr = (g_pstI2Cclient->addr << 1);
						ccuSensorInfo.sensor_name_string = (char *)(g_invokeSensorNameStr[i]);
						ccu_set_sensor_info(g_invokeSocketIdx[i], &ccuSensorInfo);
					} else if (g_invokeSocketIdx[i] == DUAL_CAMERA_SUB_SENSOR) {
						ccuSensorInfo.slave_addr = (g_pstI2Cclient2->addr << 1);
						ccuSensorInfo.sensor_name_string = (char *)(g_invokeSensorNameStr[i]);
						ccu_set_sensor_info(g_invokeSocketIdx[i], &ccuSensorInfo);
	}
#endif
    }
    }
    }
    KD_MULTI_FUNCTION_EXIT();
    return ERROR_NONE;
}
/*  */

MUINT32
kd_MultiSensorGetInfo(
MUINT32 *pScenarioId[2],
MSDK_SENSOR_INFO_STRUCT *pSensorInfo[2],
MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData[2])
{
    MUINT32 ret = ERROR_NONE;
    u32 i = 0;

    KD_MULTI_FUNCTION_ENTRY();
    for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
	    if (g_bEnableDriver[i] && g_pInvokeSensorFunc[i]) {
	        if (DUAL_CAMERA_MAIN_SENSOR == g_invokeSocketIdx[i]) {
	        ret = g_pInvokeSensorFunc[i]->SensorGetInfo((MSDK_SCENARIO_ID_ENUM)(*pScenarioId[0]), pSensorInfo[0], pSensorConfigData[0]);
	        }
	        else if ((DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) || (DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i])) {
	        ret = g_pInvokeSensorFunc[i]->SensorGetInfo((MSDK_SCENARIO_ID_ENUM)(*pScenarioId[1]), pSensorInfo[1], pSensorConfigData[1]);
	        }

	        if (ERROR_NONE != ret) {
	        PK_ERR("[%s]\n", __func__);
	        return ret;
	        }

	    }
    }
    KD_MULTI_FUNCTION_EXIT();
    return ERROR_NONE;
}

/*  */

MUINT32
kd_MultiSensorGetResolution(
MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution[2])
{
    MUINT32 ret = ERROR_NONE;
    u32 i = 0;
    KD_MULTI_FUNCTION_ENTRY();
    PK_INF("g_bEnableDriver[%d][%d]\n", g_bEnableDriver[0], g_bEnableDriver[1]);
    for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
    if (g_bEnableDriver[i] && g_pInvokeSensorFunc[i]) {
        if (DUAL_CAMERA_MAIN_SENSOR == g_invokeSocketIdx[i]) {
        ret = g_pInvokeSensorFunc[i]->SensorGetResolution(pSensorResolution[0]);
        }
        else if ((DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) || (DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i])) {
        ret = g_pInvokeSensorFunc[i]->SensorGetResolution(pSensorResolution[1]);
        }

        if (ERROR_NONE != ret) {
        PK_ERR("[%s]\n", __func__);
        return ret;
        }
    }
    }

    KD_MULTI_FUNCTION_EXIT();
    return ERROR_NONE;
}


/*  */
MUINT32
kd_MultiSensorFeatureControl(
CAMERA_DUAL_CAMERA_SENSOR_ENUM InvokeCamera,
MSDK_SENSOR_FEATURE_ENUM FeatureId,
MUINT8 *pFeaturePara,
MUINT32 *pFeatureParaLen)
{
    MUINT32 ret = ERROR_NONE;
    u32 i = 0;
    KD_MULTI_FUNCTION_ENTRY();
	if (FeatureId == SENSOR_FEATURE_SET_STREAMING_SUSPEND)
		PK_DBG("SENSOR_FEATURE_SET_STREAMING_SUSPEND before sensor driver, InvokeCamera %d\n", InvokeCamera);
    for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
    if (g_bEnableDriver[i] && g_pInvokeSensorFunc[i]) {

        if (InvokeCamera == g_invokeSocketIdx[i]) {

#if 0
        if (DUAL_CAMERA_MAIN_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
            spin_lock(&kdsensor_drv_lock);
            gI2CBusNum = SENSOR_I2C_BUS_NUM[g_invokeSocketIdx[i]];
            spin_unlock(&kdsensor_drv_lock);
            PK_XLOG_INFO("kd_MultiSensorOpen: switch I2C BUS%d\n", gI2CBusNum);
        }
#else
        //Main2 Support
        if (DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
            spin_lock(&kdsensor_drv_lock);
            gI2CBusNum = SUPPORT_I2C_BUS_NUM3;
            spin_unlock(&kdsensor_drv_lock);
        }
        else if (DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i]) {
            spin_lock(&kdsensor_drv_lock);
            gI2CBusNum = SUPPORT_I2C_BUS_NUM2;
            spin_unlock(&kdsensor_drv_lock);
            /* PK_XLOG_INFO("kd_MultiSensorFeatureControl: switch I2C BUS2\n"); */
        }
        else {
            spin_lock(&kdsensor_drv_lock);
            gI2CBusNum = SUPPORT_I2C_BUS_NUM1;
            spin_unlock(&kdsensor_drv_lock);
             /* PK_XLOG_INFO("kd_MultiSensorFeatureControl: switch I2C BUS1\n"); */
        }
#endif
        /*  */
        /* set i2c slave ID */
        /* KD_SET_I2C_SLAVE_ID(i,g_invokeSocketIdx[i],IMGSENSOR_SET_I2C_ID_STATE); */
        /*  */


        ret = g_pInvokeSensorFunc[i]->SensorFeatureControl(FeatureId, pFeaturePara, pFeatureParaLen);
        if (ERROR_NONE != ret) {
            PK_ERR("[%s]\n", __func__);
            return ret;
		}

		if (SENSOR_FEATURE_SET_STREAMING_SUSPEND == FeatureId)
			g_pInvokeSensorFunc[i]->sensorState = SENSOR_STATE_STANDBY;
		else if (SENSOR_FEATURE_SET_STREAMING_RESUME == FeatureId)
			g_pInvokeSensorFunc[i]->sensorState = SENSOR_STATE_STREAMING;
        }
    }
    }
    KD_MULTI_FUNCTION_EXIT();
    return ERROR_NONE;
}

/*  */
MUINT32
kd_MultiSensorControl(
CAMERA_DUAL_CAMERA_SENSOR_ENUM InvokeCamera,
MSDK_SCENARIO_ID_ENUM ScenarioId,
MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    MUINT32 ret = ERROR_NONE;
    u32 i = 0;
    KD_MULTI_FUNCTION_ENTRY();
    for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
    if (g_bEnableDriver[i] && g_pInvokeSensorFunc[i]) {
        if (InvokeCamera == g_invokeSocketIdx[i]) {
        KD_IMGSENSOR_PROFILE_INIT();

#if 0
        if (DUAL_CAMERA_MAIN_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SENSOR_I2C_BUS_NUM[g_invokeSocketIdx[i]];
        spin_unlock(&kdsensor_drv_lock);
        PK_XLOG_INFO("kd_MultiSensorOpen: switch I2C BUS%d\n", gI2CBusNum);
        }
#else
        if (DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SUPPORT_I2C_BUS_NUM3;
        spin_unlock(&kdsensor_drv_lock);
        }
        else if (DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i]) {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SUPPORT_I2C_BUS_NUM2;
        spin_unlock(&kdsensor_drv_lock);
         /* PK_XLOG_INFO("kd_MultiSensorControl: switch I2C BUS2\n"); */
        }
        else {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SUPPORT_I2C_BUS_NUM1;
        spin_unlock(&kdsensor_drv_lock);
         /* PK_XLOG_INFO("kd_MultiSensorControl: switch I2C BUS1\n"); */
        }
#endif
        /*  */
        /* set i2c slave ID */
        /* KD_SET_I2C_SLAVE_ID(i,g_invokeSocketIdx[i],IMGSENSOR_SET_I2C_ID_STATE); */
        /*  */
        g_pInvokeSensorFunc[i]->ScenarioId = ScenarioId;
        memcpy(&g_pInvokeSensorFunc[i]->imageWindow, pImageWindow, sizeof(ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT));
        memcpy(&g_pInvokeSensorFunc[i]->sensorConfigData, pSensorConfigData, sizeof(ACDK_SENSOR_CONFIG_STRUCT));
        ret = g_pInvokeSensorFunc[i]->SensorControl(ScenarioId, pImageWindow, pSensorConfigData);
        if (ERROR_NONE != ret) {
        PK_ERR("ERR:SensorControl(), i =%d\n", i);
        return ret;
		}
        KD_IMGSENSOR_PROFILE("SensorControl");
		g_pInvokeSensorFunc[i]->sensorState = SENSOR_STATE_STREAMING;
    }
    }
    }
    KD_MULTI_FUNCTION_EXIT();

    return ERROR_NONE;
}
/*  */
MUINT32
kd_MultiSensorClose(void)
{
    MUINT32 ret = ERROR_NONE;
    u32 i = 0;
    KD_MULTI_FUNCTION_ENTRY();
    for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
    if (g_bEnableDriver[i] && g_pInvokeSensorFunc[i]) {
      if (0 != (g_CurrentSensorIdx & g_invokeSocketIdx[i])) {
#if 0
          if (DUAL_CAMERA_MAIN_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
          spin_lock(&kdsensor_drv_lock);
          gI2CBusNum = SENSOR_I2C_BUS_NUM[g_invokeSocketIdx[i]];
          spin_unlock(&kdsensor_drv_lock);
          PK_XLOG_INFO("kd_MultiSensorOpen: switch I2C BUS%d\n", gI2CBusNum);
          }
#else


        if (DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SUPPORT_I2C_BUS_NUM3;
        spin_unlock(&kdsensor_drv_lock);
        }
        else if (DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i]) {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SUPPORT_I2C_BUS_NUM2;
        spin_unlock(&kdsensor_drv_lock);
         PK_XLOG_INFO("kd_MultiSensorClose: switch I2C BUS2\n");
        }
        else {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SUPPORT_I2C_BUS_NUM1;
        spin_unlock(&kdsensor_drv_lock);
         PK_XLOG_INFO("kd_MultiSensorClose: switch I2C BUS1\n");
        }
#endif
        ret = g_pInvokeSensorFunc[i]->SensorClose();

        /* Change the close power flow to close power in this function & */
        /* directly call kdCISModulePowerOn to close the specific sensor */
        /* The original flow will close all opened sensors at once */
        kdCISModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM)g_invokeSocketIdx[i], (char *)g_invokeSensorNameStr[i], false, CAMERA_HW_DRVNAME1);

        if (ERROR_NONE != ret) {
        PK_ERR("[%s]", __func__);
        return ret;
        }

	g_pInvokeSensorFunc[i]->sensorState = SENSOR_STATE_CLOSE; /* State is close */
		gSensorTempState[g_invokeSocketIdx[i]] |= SENSOR_TEMPERATURE_NOT_POWER_ON; /* sensor power off */
		if((gSensorTempState[g_invokeSocketIdx[i]] & SENSOR_TEMPERATURE_VALID) > 0) /* clear temperature valid */
			gSensorTempState[g_invokeSocketIdx[i]] &= ~SENSOR_TEMPERATURE_VALID;
      }
    }
    }
    KD_MULTI_FUNCTION_EXIT();
    return ERROR_NONE;
}
/*  */
MULTI_SENSOR_FUNCTION_STRUCT2  kd_MultiSensorFunc =
{
    kd_MultiSensorOpen,
    kd_MultiSensorGetInfo,
    kd_MultiSensorGetResolution,
    kd_MultiSensorFeatureControl,
    kd_MultiSensorControl,
    kd_MultiSensorClose
};


/*******************************************************************************
* kdModulePowerOn
********************************************************************************/
int
kdModulePowerOn(
CAMERA_DUAL_CAMERA_SENSOR_ENUM socketIdx[KDIMGSENSOR_MAX_INVOKE_DRIVERS],
char sensorNameStr[KDIMGSENSOR_MAX_INVOKE_DRIVERS][32],
BOOL On,
char *mode_name)
{
MINT32 ret = ERROR_NONE;
u32 i = 0;

    for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
    if (g_bEnableDriver[i]) {
        /* PK_XLOG_INFO("[%s][%d][%d][%s][%s]\r\n",__FUNCTION__,g_bEnableDriver[i],socketIdx[i],sensorNameStr[i],mode_name); */
        ret = kdCISModulePowerOn(socketIdx[i], sensorNameStr[i], On, mode_name);
        if (ERROR_NONE != ret) {
        PK_ERR("[%s]", __func__);
        return ret;
        }
    }
    }
    return ERROR_NONE;
}

/*******************************************************************************
* kdSetDriver
********************************************************************************/
int kdSetDriver(unsigned int *pDrvIndex)
{
    ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT *pSensorList = NULL;
    u32 drvIdx[KDIMGSENSOR_MAX_INVOKE_DRIVERS] = {0, 0};
    u32 i;

    /* set driver for MAIN or SUB sensor */
    PK_INF("pDrvIndex:0x%08x/0x%08x\n", pDrvIndex[KDIMGSENSOR_INVOKE_DRIVER_0], pDrvIndex[KDIMGSENSOR_INVOKE_DRIVER_1]);
    gDrvIndex = pDrvIndex[KDIMGSENSOR_INVOKE_DRIVER_0];

    if (0 != kdGetSensorInitFuncList(&pSensorList))
    {
    PK_ERR("ERROR:kdGetSensorInitFuncList()\n");
    return -EIO;
    }

    for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
    /*  */
    spin_lock(&kdsensor_drv_lock);
    g_bEnableDriver[i] = FALSE;
    g_invokeSocketIdx[i] = (CAMERA_DUAL_CAMERA_SENSOR_ENUM)((pDrvIndex[i] & KDIMGSENSOR_DUAL_MASK_MSB)>>KDIMGSENSOR_DUAL_SHIFT);
    spin_unlock(&kdsensor_drv_lock);
    drvIdx[i] = (pDrvIndex[i] & KDIMGSENSOR_DUAL_MASK_LSB);
    /*  */
    if (DUAL_CAMERA_NONE_SENSOR == g_invokeSocketIdx[i]) { continue; }
#if 0
            if (DUAL_CAMERA_MAIN_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i] || DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
            spin_lock(&kdsensor_drv_lock);
            gI2CBusNum = SENSOR_I2C_BUS_NUM[g_invokeSocketIdx[i]];
            spin_unlock(&kdsensor_drv_lock);
            PK_XLOG_INFO("kd_MultiSensorOpen: switch I2C BUS%d\n", gI2CBusNum);
            }
#else

    if (DUAL_CAMERA_MAIN_2_SENSOR == g_invokeSocketIdx[i]) {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SUPPORT_I2C_BUS_NUM3;
        spin_unlock(&kdsensor_drv_lock);
    }
    else if (DUAL_CAMERA_SUB_SENSOR == g_invokeSocketIdx[i]) {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SUPPORT_I2C_BUS_NUM2;
        spin_unlock(&kdsensor_drv_lock);
         /* PK_XLOG_INFO("kdSetDriver: switch I2C BUS2\n"); */
    }
    else {
        spin_lock(&kdsensor_drv_lock);
        gI2CBusNum = SUPPORT_I2C_BUS_NUM1;
        spin_unlock(&kdsensor_drv_lock);
         /* PK_XLOG_INFO("kdSetDriver: switch I2C BUS1\n"); */
    }
#endif
    PK_XLOG_INFO("[kdSetDriver]g_invokeSocketIdx[%d] = %d\n", i, g_invokeSocketIdx[i]);
    PK_XLOG_INFO("[kdSetDriver]drvIdx[%d] = %d\n", i, drvIdx[i]);
    /*  */
    if (MAX_NUM_OF_SUPPORT_SENSOR > drvIdx[i]) {
        if (NULL == pSensorList[drvIdx[i]].SensorInit) {
        PK_ERR("ERROR:kdSetDriver()\n");
        return -EIO;
        }

        pSensorList[drvIdx[i]].SensorInit(&g_pInvokeSensorFunc[i]);
        if (NULL == g_pInvokeSensorFunc[i]) {
        PK_ERR("ERROR:NULL g_pSensorFunc[%d]\n", i);
        return -EIO;
        }
        /*  */
        spin_lock(&kdsensor_drv_lock);
        g_pInvokeSensorFunc[i]->sensorState = SENSOR_STATE_CLOSE; /* Init state is sensor close*/
        g_bEnableDriver[i] = TRUE;
        spin_unlock(&kdsensor_drv_lock);
        /* get sensor name */
        memcpy((char *)g_invokeSensorNameStr[i], (char *)pSensorList[drvIdx[i]].drvname, sizeof(pSensorList[drvIdx[i]].drvname));
        /* return sensor ID */
        /* pDrvIndex[0] = (unsigned int)pSensorList[drvIdx].SensorId; */
	PK_XLOG_INFO("[kdSetDriver] :[%d][%d][%d][%s][%u]\n", i, g_bEnableDriver[i],
		g_invokeSocketIdx[i], g_invokeSensorNameStr[i],
		(unsigned int)sizeof(pSensorList[drvIdx[i]].drvname));
    }
    }
    return 0;
}

int kdSetCurrentSensorIdx(unsigned int idx)
{
    g_CurrentSensorIdx = idx;
    return 0;
}
/*******************************************************************************
* kdGetSocketPostion
********************************************************************************/
int
kdGetSocketPostion(unsigned int *pSocketPos) {
    PK_XLOG_INFO("[%s][%d] \r\n", __func__, *pSocketPos);
    switch (*pSocketPos) {
    case DUAL_CAMERA_MAIN_SENSOR:
        /* ->this is a HW layout dependent */
        /* ToDo */
        *pSocketPos = IMGSENSOR_SOCKET_POS_RIGHT;
        break;
    case DUAL_CAMERA_MAIN_2_SENSOR:
        *pSocketPos = IMGSENSOR_SOCKET_POS_LEFT;
        break;
    default:
    case DUAL_CAMERA_SUB_SENSOR:
        *pSocketPos = IMGSENSOR_SOCKET_POS_NONE;
        break;
    }
    return 0;
}
/*******************************************************************************
* kdSetSensorSyncFlag
********************************************************************************/
int kdSetSensorSyncFlag(BOOL bSensorSync)
{
    spin_lock(&kdsensor_drv_lock);

    bSesnorVsyncFlag = bSensorSync;
    spin_unlock(&kdsensor_drv_lock);
/* PK_DBG("[Sensor] kdSetSensorSyncFlag:%d\n", bSesnorVsyncFlag); */

    /* strobe_VDIrq(); //cotta : added for high current solution */

    return 0;
}

/*******************************************************************************
* kdCheckSensorPowerOn
********************************************************************************/
int kdCheckSensorPowerOn(void)
{
    if (atomic_read(&g_CamHWOpening) == 0) {
    return 0;
    }
    else {/* sensor power on */
    return 1;
    }
}

/*******************************************************************************
* kdSensorSyncFunctionPtr
********************************************************************************/
/* ToDo: How to separate main/main2....who is caller? */
int kdSensorSyncFunctionPtr(void)
{
    unsigned int FeatureParaLen = 0;
    /* PK_DBG("[Sensor] kdSensorSyncFunctionPtr1:%d %d %d\n", g_NewSensorExpGain.uSensorExpDelayFrame, g_NewSensorExpGain.uSensorGainDelayFrame, g_NewSensorExpGain.uISPGainDelayFrame); */
    mutex_lock(&kdCam_Mutex);
    if (NULL == g_pSensorFunc) {
    PK_ERR("ERROR:NULL g_pSensorFunc\n");
    mutex_unlock(&kdCam_Mutex);
    return -EIO;
    }
    /* PK_DBG("[Sensor] Exposure time:%d, Gain = %d\n", g_NewSensorExpGain.u2SensorNewExpTime,g_NewSensorExpGain.u2SensorNewGain ); */
    /* exposure time */
    if (g_NewSensorExpGain.uSensorExpDelayFrame == 0) {
    FeatureParaLen = 2;
    g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_MAIN_SENSOR, SENSOR_FEATURE_SET_ESHUTTER, (unsigned char *)&g_NewSensorExpGain.u2SensorNewExpTime, (unsigned int *) &FeatureParaLen);
    g_NewSensorExpGain.uSensorExpDelayFrame = 0xFF; /* disable */
    }
    else if (g_NewSensorExpGain.uSensorExpDelayFrame != 0xFF) {
    g_NewSensorExpGain.uSensorExpDelayFrame--;
    }

    /* exposure gain */
    if (g_NewSensorExpGain.uSensorGainDelayFrame == 0) {
    FeatureParaLen = 2;
    g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_MAIN_SENSOR, SENSOR_FEATURE_SET_GAIN, (unsigned char *)&g_NewSensorExpGain.u2SensorNewGain, (unsigned int *) &FeatureParaLen);
    g_NewSensorExpGain.uSensorGainDelayFrame = 0xFF; /* disable */
     }
    else if (g_NewSensorExpGain.uSensorGainDelayFrame != 0xFF) {
    g_NewSensorExpGain.uSensorGainDelayFrame--;
     }

    /* if the delay frame is 0 or 0xFF, stop to count */
    if ((g_NewSensorExpGain.uISPGainDelayFrame != 0xFF) && (g_NewSensorExpGain.uISPGainDelayFrame != 0)) {
		spin_lock(&kdsensor_drv_lock);
		g_NewSensorExpGain.uISPGainDelayFrame--;
		spin_unlock(&kdsensor_drv_lock);
    }
    mutex_unlock(&kdCam_Mutex);
    return 0;
}

/*******************************************************************************
* kdGetRawGainInfo
********************************************************************************/
int kdGetRawGainInfoPtr(UINT16 *pRAWGain)
{
    *pRAWGain = 0x00;
    *(pRAWGain+1) = 0x00;
    *(pRAWGain+2) = 0x00;
    *(pRAWGain+3) = 0x00;

    if (g_NewSensorExpGain.uISPGainDelayFrame == 0)    {  /* synchronize the isp gain */
    *pRAWGain = g_NewSensorExpGain.u2ISPNewRGain;
    *(pRAWGain+1) = g_NewSensorExpGain.u2ISPNewGrGain;
    *(pRAWGain+2) = g_NewSensorExpGain.u2ISPNewGbGain;
    *(pRAWGain+3) = g_NewSensorExpGain.u2ISPNewBGain;
/* PK_DBG("[Sensor] ISP Gain:%d\n", g_NewSensorExpGain.u2ISPNewRGain, g_NewSensorExpGain.u2ISPNewGrGain, */
/* g_NewSensorExpGain.u2ISPNewGbGain, g_NewSensorExpGain.u2ISPNewBGain); */
    spin_lock(&kdsensor_drv_lock);
    g_NewSensorExpGain.uISPGainDelayFrame = 0xFF; /* disable */
    spin_unlock(&kdsensor_drv_lock);
    }

    return 0;
}




int kdSetExpGain(CAMERA_DUAL_CAMERA_SENSOR_ENUM InvokeCamera)
{
    unsigned int FeatureParaLen = 0;
    PK_DBG("[kd_sensorlist]enter kdSetExpGain\n");
    if (NULL == g_pSensorFunc) {
    PK_ERR("ERROR:NULL g_pSensorFunc\n");

    return -EIO;
    }

    setExpGainDoneFlag = 0;
    FeatureParaLen = 2;
    g_pSensorFunc->SensorFeatureControl(InvokeCamera, SENSOR_FEATURE_SET_ESHUTTER, (unsigned char *)&g_NewSensorExpGain.u2SensorNewExpTime, (unsigned int *) &FeatureParaLen);
    g_pSensorFunc->SensorFeatureControl(InvokeCamera, SENSOR_FEATURE_SET_GAIN, (unsigned char *)&g_NewSensorExpGain.u2SensorNewGain, (unsigned int *) &FeatureParaLen);

    setExpGainDoneFlag = 1;
    PK_DBG("[kd_sensorlist]before wake_up_interruptible\n");
    wake_up_interruptible(&kd_sensor_wait_queue);
    PK_DBG("[kd_sensorlist]after wake_up_interruptible\n");

    return 0;   /* No error. */

}

/*******************************************************************************
*
********************************************************************************/
static UINT32 ms_to_jiffies(MUINT32 ms)
{
    return ((ms * HZ + 512) >> 10);
}


int kdSensorSetExpGainWaitDone(int *ptime)
{
    int timeout;
    PK_DBG("[kd_sensorlist]enter kdSensorSetExpGainWaitDone: time: %d\n", *ptime);
    timeout = wait_event_interruptible_timeout(
        kd_sensor_wait_queue,
        (setExpGainDoneFlag & 1),
        ms_to_jiffies(*ptime));

    PK_DBG("[kd_sensorlist]after wait_event_interruptible_timeout\n");
    if (timeout == 0) {
    PK_ERR("[kd_sensorlist] kdSensorSetExpGainWait: timeout=%d\n", *ptime);

    return -EAGAIN;
    }

    return 0;   /* No error. */

}




/*******************************************************************************
* adopt_CAMERA_HW_Open
********************************************************************************/
inline static int adopt_CAMERA_HW_Open(void)
{
    UINT32 err = 0;

    /* KD_IMGSENSOR_PROFILE_INIT();*/
    /* power on sensor */
    /* if (atomic_read(&g_CamHWOpend) == 0  ) { */
    /* move into SensorOpen() for 2on1 driver */
    /* turn on power */
    /* kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM*) g_invokeSocketIdx, g_invokeSensorNameStr,true, CAMERA_HW_DRVNAME); */
    /* wait for power stable */
    /* mDELAY(10); */
    /* KD_IMGSENSOR_PROFILE("kdModulePowerOn"); */
    /*  */
     if (g_pSensorFunc) {
        err = g_pSensorFunc->SensorOpen();
        if (ERROR_NONE != err) {
            /*Multiopen fail would close power.*/
            //kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM *) g_invokeSocketIdx, g_invokeSensorNameStr, false, CAMERA_HW_DRVNAME1);
            PK_ERR("ERROR:SensorOpen(), turn off power\n");
        }
    }
    else {
        PK_DBG(" ERROR:NULL g_pSensorFunc\n");
    }

    /* KD_IMGSENSOR_PROFILE("SensorOpen"); */
    /* } */
    /* else { */
    /* PK_ERR("adopt_CAMERA_HW_Open Fail, g_CamHWOpend = %d\n ",atomic_read(&g_CamHWOpend) ); */
    /* } */

    /* if (err == 0 ) { */
    /* atomic_set(&g_CamHWOpend, 1); */

    /* } */

    return err ?  -EIO:err;
}   /* adopt_CAMERA_HW_Open() */

/*******************************************************************************
* adopt_CAMERA_HW_CheckIsAlive
********************************************************************************/
inline static int adopt_CAMERA_HW_CheckIsAlive(void)
{
    UINT32 err = 0;
    UINT32 err1 = 0;
    UINT32 i = 0;
    MUINT32 sensorID = 0;
    MUINT32 retLen = 0;

	KD_IMGSENSOR_PROFILE_INIT();
	/* power on sensor */
	kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM *)g_invokeSocketIdx, g_invokeSensorNameStr, true, CAMERA_HW_DRVNAME1);
	/* wait for power stable */
	mDELAY(5);
	KD_IMGSENSOR_PROFILE("kdModulePowerOn");

	/* initial for search sensor function */
	g_CurrentSensorIdx = 0;
	/* Search sensor keep i2c debug log */
	g_IsSearchSensor = 1;
	/* Camera information */
    if(gDrvIndex == 0x10000)
    {
		memset(mtk_ccm_name, 0, camera_info_size);
		/* Init gSensorTempState */
		for (i = 0; i < DUAL_CAMERA_SENSOR_MAX; i++)
			gSensorTempState[i] = TEMPERATURE_INIT_STATE;
    }

    if (g_pSensorFunc) {
    for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
        if (DUAL_CAMERA_NONE_SENSOR != g_invokeSocketIdx[i]) {
        err = g_pSensorFunc->SensorFeatureControl(g_invokeSocketIdx[i], SENSOR_FEATURE_CHECK_SENSOR_ID, (MUINT8 *)&sensorID, &retLen);
        if (sensorID == 0) {    /* not implement this feature ID */
            PK_DBG(" Not implement!!, use old open function to check\n");
            err = ERROR_SENSOR_CONNECT_FAIL;
        }
        else if (sensorID == 0xFFFFFFFF) {    /* fail to open the sensor */
            PK_DBG(" No Sensor Found");
            err = ERROR_SENSOR_CONNECT_FAIL;
        }
        else {

            PK_DBG(" Sensor found ID = 0x%x\n", sensorID);
            snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s CAM[%d]:%s;",mtk_ccm_name,g_invokeSocketIdx[i],g_invokeSensorNameStr[i]);

			gSensorTempState[g_invokeSocketIdx[i]] &= ~SENSOR_TEMPERATURE_CANNOT_SEARCH_SENSOR;
            err = ERROR_NONE;
        }
        if (ERROR_NONE != err)
        {
            PK_DBG("ERROR:adopt_CAMERA_HW_CheckIsAlive(), No imgsensor alive\n");
        }
        }
    }
    }
    else {
    PK_DBG("ERROR:NULL g_pSensorFunc\n");
    }

    /* reset sensor state after power off */
	if (g_pSensorFunc)
		err1 = g_pSensorFunc->SensorClose();
    if (ERROR_NONE != err1) {
    PK_DBG("SensorClose\n");
    }
    /*  */
    kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM *)g_invokeSocketIdx, g_invokeSensorNameStr, false, CAMERA_HW_DRVNAME1);
    /*  */
    KD_IMGSENSOR_PROFILE("CheckIsAlive");

    g_IsSearchSensor = 0;

    return err ?  -EIO:err;
}   /* adopt_CAMERA_HW_Open() */


/*******************************************************************************
* adopt_CAMERA_HW_GetResolution
********************************************************************************/
inline static int adopt_CAMERA_HW_GetResolution(void *pBuf)
{
    /* ToDo: remove print */
    ACDK_SENSOR_PRESOLUTION_STRUCT *pBufResolution =  (ACDK_SENSOR_PRESOLUTION_STRUCT *)pBuf;
	ACDK_SENSOR_RESOLUTION_INFO_STRUCT* pRes[2] = { NULL, NULL };
    PK_XLOG_INFO("[CAMERA_HW] adopt_CAMERA_HW_GetResolution, pBuf: %p\n", pBuf);
	pRes[0] = (ACDK_SENSOR_RESOLUTION_INFO_STRUCT* )kmalloc(sizeof(MSDK_SENSOR_RESOLUTION_INFO_STRUCT), GFP_KERNEL);
	if (pRes[0] == NULL) {
		PK_ERR(" ioctl allocate mem failed\n");
		return -ENOMEM;
	}
	pRes[1] = (ACDK_SENSOR_RESOLUTION_INFO_STRUCT* )kmalloc(sizeof(MSDK_SENSOR_RESOLUTION_INFO_STRUCT), GFP_KERNEL);
	if (pRes[1] == NULL) {
		kfree(pRes[0]);
		PK_ERR(" ioctl allocate mem failed\n");
		return -ENOMEM;
	}


    if (g_pSensorFunc) {
		g_pSensorFunc->SensorGetResolution(pRes);
		if (copy_to_user((void __user *) (pBufResolution->pResolution[0]) , (void *)pRes[0] , sizeof(MSDK_SENSOR_RESOLUTION_INFO_STRUCT))) {
			PK_ERR("copy to user failed\n");
		}
		if (copy_to_user((void __user *) (pBufResolution->pResolution[1]) , (void *)pRes[1] , sizeof(MSDK_SENSOR_RESOLUTION_INFO_STRUCT))) {
			PK_ERR("copy to user failed\n");
		}
    }
    else {
    PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
    }
	if (pRes[0] != NULL) {
		kfree(pRes[0]);
	}
	if (pRes[1] != NULL) {
		kfree(pRes[1]);
	}

    return 0;
}   /* adopt_CAMERA_HW_GetResolution() */


/*******************************************************************************
* adopt_CAMERA_HW_GetInfo
********************************************************************************/
inline static int adopt_CAMERA_HW_GetInfo(void *pBuf)
{
    ACDK_SENSOR_GETINFO_STRUCT *pSensorGetInfo = (ACDK_SENSOR_GETINFO_STRUCT *)pBuf;
    MSDK_SENSOR_INFO_STRUCT *pInfo[2];
    MSDK_SENSOR_CONFIG_STRUCT *pConfig[2];
    MUINT32 *pScenarioId[2];
    u32 i,j = 0;

    for (i = 0; i < 2; i++) {
	    pInfo[i] = NULL;
	    pConfig[i] =  NULL;
	    pScenarioId[i] =  &(pSensorGetInfo->ScenarioId[i]);
    }


	pInfo[0] = kmalloc(sizeof(MSDK_SENSOR_INFO_STRUCT), GFP_KERNEL);
	pInfo[1] = kmalloc(sizeof(MSDK_SENSOR_INFO_STRUCT), GFP_KERNEL);
	pConfig[0] = kmalloc(sizeof(MSDK_SENSOR_CONFIG_STRUCT), GFP_KERNEL);
	pConfig[1] = kmalloc(sizeof(MSDK_SENSOR_CONFIG_STRUCT), GFP_KERNEL);

	if (pInfo[0] == NULL || pInfo[1] == NULL || pConfig[0] == NULL || pConfig[1] == NULL){
		PK_ERR(" ioctl allocate mem failed\n");
		return -ENOMEM;
	}

	memset(pInfo[0], 0, sizeof(MSDK_SENSOR_INFO_STRUCT));
	memset(pInfo[1], 0, sizeof(MSDK_SENSOR_INFO_STRUCT));
	memset(pConfig[0], 0, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	memset(pConfig[1], 0, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    if (NULL == pSensorGetInfo) {
    PK_DBG("[CAMERA_HW] NULL arg.\n");
    return -EFAULT;
    }

    if ((NULL == pSensorGetInfo->pInfo[0]) || (NULL == pSensorGetInfo->pInfo[1]) ||
    (NULL == pSensorGetInfo->pConfig[0]) || (NULL == pSensorGetInfo->pConfig[1]))  {
    PK_DBG("[CAMERA_HW] NULL arg.\n");
    return -EFAULT;
    }

    if (g_pSensorFunc) {
	    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo, pConfig);
    }
    else {
	    PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
    }



    for (i = 0; i < 2; i++) {
	    /* SenorInfo */
	    if (copy_to_user((void __user *)(pSensorGetInfo->pInfo[i]), (void *)pInfo[i] , sizeof(MSDK_SENSOR_INFO_STRUCT))) {
	        PK_DBG("[CAMERA_HW][info] ioctl copy to user failed\n");
			for (j = 0; j < 2; j++) {
			  	if (pInfo[j] != NULL) kfree(pInfo[j]);
			  	if (pConfig[j] != NULL) kfree(pConfig[j]);
				pInfo[j] = NULL;
				pConfig[j] = NULL;
			}
	        return -EFAULT;
	    }

	    /* SensorConfig */
	    if (copy_to_user((void __user *) (pSensorGetInfo->pConfig[i]) , (void *)pConfig[i] , sizeof(MSDK_SENSOR_CONFIG_STRUCT))) {
	        PK_DBG("[CAMERA_HW][config] ioctl copy to user failed\n");
			for (j = 0; j < 2; j++) {
			  	if (pInfo[j] != NULL) kfree(pInfo[j]);
			  	if (pConfig[j] != NULL) kfree(pConfig[j]);
				pInfo[j] = NULL;
				pConfig[j] = NULL;
			}
	        return -EFAULT;
	    }
    }
	for (j = 0; j < 2; j++) {
		if (pInfo[j] != NULL) kfree(pInfo[j]);
		if (pConfig[j] != NULL) kfree(pConfig[j]);
		pInfo[j] = NULL;
		pConfig[j] = NULL;
	}

    return 0;
}   /* adopt_CAMERA_HW_GetInfo() */

/*******************************************************************************
* adopt_CAMERA_HW_GetInfo
********************************************************************************/
/* adopt_CAMERA_HW_GetInfo() */
inline static int adopt_CAMERA_HW_GetInfo2(void *pBuf)
{
    IMAGESENSOR_GETINFO_STRUCT *pSensorGetInfo = (IMAGESENSOR_GETINFO_STRUCT *)pBuf;
    ACDK_SENSOR_INFO2_STRUCT *pSensorInfo = NULL;//{0};
    MUINT32 IDNum = 0;
    MSDK_SENSOR_INFO_STRUCT *pInfo[2] = {NULL,NULL};
    MSDK_SENSOR_CONFIG_STRUCT  *pConfig[2] = {NULL,NULL};
    MSDK_SENSOR_INFO_STRUCT *pInfo1[2] = {NULL,NULL};
    MSDK_SENSOR_CONFIG_STRUCT  *pConfig1[2]= {NULL,NULL};
    MSDK_SENSOR_INFO_STRUCT *pInfo2[2] = {NULL,NULL};
    MSDK_SENSOR_CONFIG_STRUCT  *pConfig2[2]= {NULL,NULL};
    MSDK_SENSOR_INFO_STRUCT *pInfo3[2] = {NULL,NULL};
    MSDK_SENSOR_CONFIG_STRUCT  *pConfig3[2] = {NULL,NULL};
    MSDK_SENSOR_INFO_STRUCT *pInfo4[2] = {NULL,NULL};
    MSDK_SENSOR_CONFIG_STRUCT  *pConfig4[2]= {NULL,NULL};
    MSDK_SENSOR_RESOLUTION_INFO_STRUCT  *psensorResolution[2] = {NULL,NULL};

    MUINT32 ScenarioId[2], *pScenarioId[2];
    u32 i = 0;
    PK_DBG("[adopt_CAMERA_HW_GetInfo2]Entry\n");


    if (NULL == pSensorGetInfo) {
    PK_DBG("[CAMERA_HW] NULL arg.\n");
    return -EFAULT;
    }
    if (NULL == g_pSensorFunc) {
    PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
    return -EFAULT;
    }

	for (i = 0; i < 2; i++) {
	   pInfo[i] =  &ginfo[i];
	   pConfig[i] =  kmalloc(sizeof(MSDK_SENSOR_CONFIG_STRUCT), GFP_KERNEL);
	   pInfo1[i] =	&ginfo1[i];
	   pConfig1[i] =  kmalloc(sizeof(MSDK_SENSOR_CONFIG_STRUCT), GFP_KERNEL);
	   pInfo2[i] =	&ginfo2[i];
	   pConfig2[i] = kmalloc(sizeof(MSDK_SENSOR_CONFIG_STRUCT), GFP_KERNEL);
	   pInfo3[i] =	&ginfo3[i];
	   pConfig3[i] =  kmalloc(sizeof(MSDK_SENSOR_CONFIG_STRUCT), GFP_KERNEL);
	   pInfo4[i] =	&ginfo4[i];
	   pConfig4[i] =  kmalloc(sizeof(MSDK_SENSOR_CONFIG_STRUCT), GFP_KERNEL);
	   psensorResolution[i] = kmalloc(sizeof(MSDK_SENSOR_RESOLUTION_INFO_STRUCT), GFP_KERNEL);
	   pScenarioId[i] =  &ScenarioId[i];
	}
	pSensorInfo = kmalloc(sizeof(ACDK_SENSOR_INFO2_STRUCT), GFP_KERNEL);

	if (pConfig[0] == NULL || pConfig[1] == NULL || pConfig1[0] == NULL || pConfig1[1] == NULL ||
		pConfig2[0] == NULL || pConfig2[1] == NULL || pConfig3[0] == NULL || pConfig3[1] == NULL ||
		pConfig4[0] == NULL || pConfig4[1] == NULL || pSensorInfo ==NULL ||
		psensorResolution[0] == NULL || psensorResolution[1]==NULL) {
		PK_ERR(" ioctl allocate mem failed\n");
		for (i = 0; i < 2; i++) {
			if(pConfig[i] != NULL) kfree(pConfig[i]);
			if(pConfig1[i] != NULL) kfree(pConfig1[i]);
			if(pConfig2[i] != NULL) kfree(pConfig2[i]);
			if(pConfig3[i] != NULL) kfree(pConfig3[i]);
			if(pConfig4[i] != NULL) kfree(pConfig4[i]);
			if(psensorResolution[i] != NULL) kfree(psensorResolution[i]);
			pConfig[i] = NULL;
			pConfig1[i] = NULL;
			pConfig2[i] = NULL;
			pConfig3[i] =  NULL;
			pConfig4[i] =  NULL;
			psensorResolution[i] = NULL;
		}
		kfree(pSensorInfo);
		pSensorInfo = NULL;
		return -ENOMEM;
	}

    PK_DBG("[CAMERA_HW][Resolution] %p\n", pSensorGetInfo->pSensorResolution);

	/* To set sensor information */
	if (DUAL_CAMERA_MAIN_SENSOR == pSensorGetInfo->SensorId)
		IDNum = 0;
	else
		IDNum = 1;

    /* Set default value */
    pInfo[IDNum]->TEMPERATURE_SUPPORT = 0;

    /* TO get preview value */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo, pConfig);
    /*  */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo1, pConfig1);
    /*  */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_VIDEO_PREVIEW;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo2, pConfig2);
    /*  */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo3, pConfig3);
    /*  */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_SLIM_VIDEO;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo4, pConfig4);

    /* Basic information */
    pSensorInfo->SensorPreviewResolutionX                 = pInfo[IDNum]->SensorPreviewResolutionX;
    pSensorInfo->SensorPreviewResolutionY                 = pInfo[IDNum]->SensorPreviewResolutionY;
    pSensorInfo->SensorFullResolutionX                    = pInfo[IDNum]->SensorFullResolutionX;
    pSensorInfo->SensorFullResolutionY                    = pInfo[IDNum]->SensorFullResolutionY;
    pSensorInfo->SensorClockFreq                          = pInfo[IDNum]->SensorClockFreq;
    pSensorInfo->SensorCameraPreviewFrameRate             = pInfo[IDNum]->SensorCameraPreviewFrameRate;
    pSensorInfo->SensorVideoFrameRate                     = pInfo[IDNum]->SensorVideoFrameRate;
    pSensorInfo->SensorStillCaptureFrameRate              = pInfo[IDNum]->SensorStillCaptureFrameRate;
    pSensorInfo->SensorWebCamCaptureFrameRate             = pInfo[IDNum]->SensorWebCamCaptureFrameRate;
    pSensorInfo->SensorClockPolarity                      = pInfo[IDNum]->SensorClockPolarity;
    pSensorInfo->SensorClockFallingPolarity               = pInfo[IDNum]->SensorClockFallingPolarity;
    pSensorInfo->SensorClockRisingCount                   = pInfo[IDNum]->SensorClockRisingCount;
    pSensorInfo->SensorClockFallingCount                  = pInfo[IDNum]->SensorClockFallingCount;
    pSensorInfo->SensorClockDividCount                    = pInfo[IDNum]->SensorClockDividCount;
    pSensorInfo->SensorPixelClockCount                    = pInfo[IDNum]->SensorPixelClockCount;
    pSensorInfo->SensorDataLatchCount                     = pInfo[IDNum]->SensorDataLatchCount;
    pSensorInfo->SensorHsyncPolarity                      = pInfo[IDNum]->SensorHsyncPolarity;
    pSensorInfo->SensorVsyncPolarity                      = pInfo[IDNum]->SensorVsyncPolarity;
    pSensorInfo->SensorInterruptDelayLines                = pInfo[IDNum]->SensorInterruptDelayLines;
    pSensorInfo->SensorResetActiveHigh                    = pInfo[IDNum]->SensorResetActiveHigh;
    pSensorInfo->SensorResetDelayCount                    = pInfo[IDNum]->SensorResetDelayCount;
    pSensorInfo->SensroInterfaceType                      = pInfo[IDNum]->SensroInterfaceType;
    pSensorInfo->SensorOutputDataFormat                   = pInfo[IDNum]->SensorOutputDataFormat;
    pSensorInfo->SensorMIPILaneNumber                     = pInfo[IDNum]->SensorMIPILaneNumber;
    pSensorInfo->CaptureDelayFrame                        = pInfo[IDNum]->CaptureDelayFrame;
    pSensorInfo->PreviewDelayFrame                        = pInfo[IDNum]->PreviewDelayFrame;
    pSensorInfo->VideoDelayFrame                          = pInfo[IDNum]->VideoDelayFrame;
    pSensorInfo->HighSpeedVideoDelayFrame                 = pInfo[IDNum]->HighSpeedVideoDelayFrame;
    pSensorInfo->SlimVideoDelayFrame                      = pInfo[IDNum]->SlimVideoDelayFrame;
    pSensorInfo->Custom1DelayFrame                        = pInfo[IDNum]->Custom1DelayFrame;
    pSensorInfo->Custom2DelayFrame                        = pInfo[IDNum]->Custom2DelayFrame;
    pSensorInfo->Custom3DelayFrame                        = pInfo[IDNum]->Custom3DelayFrame;
    pSensorInfo->Custom4DelayFrame                        = pInfo[IDNum]->Custom4DelayFrame;
    pSensorInfo->Custom5DelayFrame                        = pInfo[IDNum]->Custom5DelayFrame;
    pSensorInfo->YUVAwbDelayFrame                         = pInfo[IDNum]->YUVAwbDelayFrame;
    pSensorInfo->YUVEffectDelayFrame                      = pInfo[IDNum]->YUVEffectDelayFrame;
    pSensorInfo->SensorGrabStartX_PRV                     = pInfo[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_PRV                     = pInfo[IDNum]->SensorGrabStartY;
    pSensorInfo->SensorGrabStartX_CAP                     = pInfo1[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_CAP                     = pInfo1[IDNum]->SensorGrabStartY;
    pSensorInfo->SensorGrabStartX_VD                      = pInfo2[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_VD                      = pInfo2[IDNum]->SensorGrabStartY;
    pSensorInfo->SensorGrabStartX_VD1                     = pInfo3[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_VD1                     = pInfo3[IDNum]->SensorGrabStartY;
    pSensorInfo->SensorGrabStartX_VD2                     = pInfo4[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_VD2                     = pInfo4[IDNum]->SensorGrabStartY;
    pSensorInfo->SensorDrivingCurrent                     = pInfo[IDNum]->SensorDrivingCurrent;
    pSensorInfo->SensorMasterClockSwitch                  = pInfo[IDNum]->SensorMasterClockSwitch;
    pSensorInfo->AEShutDelayFrame                         = pInfo[IDNum]->AEShutDelayFrame;
    pSensorInfo->AESensorGainDelayFrame                   = pInfo[IDNum]->AESensorGainDelayFrame;
    pSensorInfo->AEISPGainDelayFrame                      = pInfo[IDNum]->AEISPGainDelayFrame;
	pSensorInfo->FrameTimeDelayFrame                      = pInfo[IDNum]->FrameTimeDelayFrame;
    pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount   = pInfo[IDNum]->MIPIDataLowPwr2HighSpeedTermDelayCount;
    pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = pInfo[IDNum]->MIPIDataLowPwr2HighSpeedSettleDelayCount;
	pSensorInfo->MIPIDataLowPwr2HSSettleDelayM0           = pInfo[IDNum]->MIPIDataLowPwr2HighSpeedSettleDelayCount;
	pSensorInfo->MIPIDataLowPwr2HSSettleDelayM1           = pInfo1[IDNum]->MIPIDataLowPwr2HighSpeedSettleDelayCount;
	pSensorInfo->MIPIDataLowPwr2HSSettleDelayM2           = pInfo2[IDNum]->MIPIDataLowPwr2HighSpeedSettleDelayCount;
	pSensorInfo->MIPIDataLowPwr2HSSettleDelayM3           = pInfo3[IDNum]->MIPIDataLowPwr2HighSpeedSettleDelayCount;
	pSensorInfo->MIPIDataLowPwr2HSSettleDelayM4           = pInfo4[IDNum]->MIPIDataLowPwr2HighSpeedSettleDelayCount;
    pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount    = pInfo[IDNum]->MIPICLKLowPwr2HighSpeedTermDelayCount;
    pSensorInfo->SensorWidthSampling                      = pInfo[IDNum]->SensorWidthSampling;
    pSensorInfo->SensorHightSampling                      = pInfo[IDNum]->SensorHightSampling;
    pSensorInfo->SensorPacketECCOrder                     = pInfo[IDNum]->SensorPacketECCOrder;
    pSensorInfo->MIPIsensorType                           = pInfo[IDNum]->MIPIsensorType;
    pSensorInfo->IHDR_LE_FirstLine                        = pInfo[IDNum]->IHDR_LE_FirstLine;
    pSensorInfo->IHDR_Support                             = pInfo[IDNum]->IHDR_Support;
	pSensorInfo->ZHDR_Mode                                = pInfo[IDNum]->ZHDR_Mode;
	pSensorInfo->TEMPERATURE_SUPPORT                      = pInfo[IDNum]->TEMPERATURE_SUPPORT;
    pSensorInfo->SensorModeNum                            = pInfo[IDNum]->SensorModeNum;
    pSensorInfo->SettleDelayMode                          = pInfo[IDNum]->SettleDelayMode;
    pSensorInfo->PDAF_Support                             = pInfo[IDNum]->PDAF_Support;
	pSensorInfo->HDR_Support                              = pInfo[IDNum]->HDR_Support;
	pSensorInfo->IMGSENSOR_DPCM_TYPE_PRE                  = pInfo[IDNum]->DPCM_INFO;
	pSensorInfo->IMGSENSOR_DPCM_TYPE_CAP                  = pInfo1[IDNum]->DPCM_INFO;
	pSensorInfo->IMGSENSOR_DPCM_TYPE_VD                   = pInfo2[IDNum]->DPCM_INFO;
	pSensorInfo->IMGSENSOR_DPCM_TYPE_VD1                  = pInfo3[IDNum]->DPCM_INFO;
	pSensorInfo->IMGSENSOR_DPCM_TYPE_VD2                  = pInfo4[IDNum]->DPCM_INFO;
	/*Per-Frame conrol suppport or not */
	pSensorInfo->PerFrameCTL_Support                      = pInfo[IDNum]->PerFrameCTL_Support;
	/*SCAM number*/
	pSensorInfo->SCAM_DataNumber                          = pInfo[IDNum]->SCAM_DataNumber;
	pSensorInfo->SCAM_DDR_En                              = pInfo[IDNum]->SCAM_DDR_En;
	pSensorInfo->SCAM_CLK_INV                             = pInfo[IDNum]->SCAM_CLK_INV;
	pSensorInfo->SCAM_DEFAULT_DELAY                      = pInfo[IDNum]->SCAM_DEFAULT_DELAY;
	pSensorInfo->SCAM_CRC_En                             = pInfo[IDNum]->SCAM_CRC_En;
	pSensorInfo->SCAM_SOF_src                            = pInfo[IDNum]->SCAM_SOF_src;
	pSensorInfo->SCAM_Timout_Cali                        = pInfo[IDNum]->SCAM_Timout_Cali;
	/*Deskew*/
	pSensorInfo->SensorMIPIDeskew                       = pInfo[IDNum]->SensorMIPIDeskew;
    /* TO get preview value */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_CUSTOM1;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo, pConfig);
    /*  */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_CUSTOM2;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo1, pConfig1);
    /*  */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_CUSTOM3;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo2, pConfig2);
    /*  */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_CUSTOM4;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo3, pConfig3);
    /*  */
    ScenarioId[0] = ScenarioId[1] = MSDK_SCENARIO_ID_CUSTOM5;
    g_pSensorFunc->SensorGetInfo(pScenarioId, pInfo4, pConfig4);
    /* To set sensor information */
    if (DUAL_CAMERA_MAIN_SENSOR == pSensorGetInfo->SensorId) {
    IDNum = 0;
    }
    else
    {
    IDNum = 1;
    }
    pSensorInfo->SensorGrabStartX_CST1                    = pInfo[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_CST1                    = pInfo[IDNum]->SensorGrabStartY;
    pSensorInfo->SensorGrabStartX_CST2                    = pInfo1[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_CST2                    = pInfo1[IDNum]->SensorGrabStartY;
    pSensorInfo->SensorGrabStartX_CST3                    = pInfo2[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_CST3                    = pInfo2[IDNum]->SensorGrabStartY;
    pSensorInfo->SensorGrabStartX_CST4                    = pInfo3[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_CST4                    = pInfo3[IDNum]->SensorGrabStartY;
    pSensorInfo->SensorGrabStartX_CST5                    = pInfo4[IDNum]->SensorGrabStartX;
    pSensorInfo->SensorGrabStartY_CST5                    = pInfo4[IDNum]->SensorGrabStartY;

    if (copy_to_user((void __user *)(pSensorGetInfo->pInfo), (void *)(pSensorInfo), sizeof(ACDK_SENSOR_INFO2_STRUCT))) {
	    PK_DBG("[CAMERA_HW][info] ioctl copy to user failed\n");
		for (i = 0; i < 2; i++) {
			if(pConfig[i] != NULL) kfree(pConfig[i]);
			if(pConfig1[i] != NULL) kfree(pConfig1[i]);
			if(pConfig2[i] != NULL) kfree(pConfig2[i]);
			if(pConfig3[i] != NULL) kfree(pConfig3[i]);
			if(pConfig4[i] != NULL) kfree(pConfig4[i]);
			if(psensorResolution[i] != NULL) kfree(psensorResolution[i]);
			pConfig[i] = NULL;
			pConfig1[i] = NULL;
			pConfig2[i] = NULL;
			pConfig3[i] =  NULL;
			pConfig4[i] =  NULL;
			psensorResolution[i] = NULL;
		}
   		kfree(pSensorInfo);
		pSensorInfo = NULL;

    return -EFAULT;
    }

	/* Update tempeture support state */
    /* 1. Search sensor flow is done after get info. Clear SENSOR_TEMPERATURE_UNKNOWN_STATUS */
	gSensorTempState[pSensorGetInfo->SensorId] &= ~SENSOR_TEMPERATURE_UNKNOWN_STATUS;
    /* 2. Set not exist sensor state to clear SENSOR_TEMPERATURE_UNKNOWN_STATUS*/
	for (i = 1; i < DUAL_CAMERA_SENSOR_MAX; i <<= 1) {
		if(gSensorTempState[i] == TEMPERATURE_INIT_STATE)
			gSensorTempState[i] &= ~SENSOR_TEMPERATURE_UNKNOWN_STATUS;
	}
	if (pSensorInfo->TEMPERATURE_SUPPORT == 1)
		gSensorTempState[pSensorGetInfo->SensorId] &= ~SENSOR_TEMPERATURE_NOT_SUPPORT_THERMAL;

    /* Step2 : Get Resolution */
    g_pSensorFunc->SensorGetResolution(psensorResolution);
    PK_DBG("[CAMERA_HW][Pre]w=0x%x, h = 0x%x\n", psensorResolution[0]->SensorPreviewWidth, psensorResolution[0]->SensorPreviewHeight);
    PK_DBG("[CAMERA_HW][Full]w=0x%x, h = 0x%x\n", psensorResolution[0]->SensorFullWidth, psensorResolution[0]->SensorFullHeight);
    PK_DBG("[CAMERA_HW][VD]w=0x%x, h = 0x%x\n", psensorResolution[0]->SensorVideoWidth, psensorResolution[0]->SensorVideoHeight);


    /* Add info to proc: camera_info */
	for (i = KDIMGSENSOR_INVOKE_DRIVER_0; i < KDIMGSENSOR_MAX_INVOKE_DRIVERS; i++) {
	if (g_bEnableDriver[i] && g_pInvokeSensorFunc[i]) {

		if (pSensorGetInfo->SensorId == g_invokeSocketIdx[i]) {

			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \n\nCAM_Info[%d]:%s;",mtk_ccm_name,g_invokeSocketIdx[IDNum],g_invokeSensorNameStr[IDNum]);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nSensorModeNum: %d",mtk_ccm_name,pSensorInfo->SensorModeNum);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nPre:  TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorPreviewWidth,psensorResolution[IDNum]->SensorPreviewHeight, pSensorInfo->SensorGrabStartX_PRV, pSensorInfo->SensorGrabStartY_PRV, pSensorInfo->PreviewDelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nCap:  TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorFullWidth,psensorResolution[IDNum]->SensorFullHeight, pSensorInfo->SensorGrabStartX_CAP, pSensorInfo->SensorGrabStartY_CAP, pSensorInfo->CaptureDelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nVid:  TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorVideoWidth,psensorResolution[IDNum]->SensorVideoHeight, pSensorInfo->SensorGrabStartX_VD, pSensorInfo->SensorGrabStartY_VD, pSensorInfo->VideoDelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nHSV:  TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorHighSpeedVideoWidth,psensorResolution[IDNum]->SensorHighSpeedVideoHeight, pSensorInfo->SensorGrabStartX_VD1, pSensorInfo->SensorGrabStartY_VD1, pSensorInfo->HighSpeedVideoDelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nSLV:  TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorSlimVideoWidth,psensorResolution[IDNum]->SensorSlimVideoHeight, pSensorInfo->SensorGrabStartX_VD2, pSensorInfo->SensorGrabStartY_VD2, pSensorInfo->SlimVideoDelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nCST1: TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorCustom1Width,psensorResolution[IDNum]->SensorCustom1Height, pSensorInfo->SensorGrabStartX_CST1, pSensorInfo->SensorGrabStartY_CST1, pSensorInfo->Custom1DelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nCST2: TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorCustom2Width,psensorResolution[IDNum]->SensorCustom2Height, pSensorInfo->SensorGrabStartX_CST2, pSensorInfo->SensorGrabStartY_CST2, pSensorInfo->Custom2DelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nCST3: TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorCustom3Width,psensorResolution[IDNum]->SensorCustom3Height, pSensorInfo->SensorGrabStartX_CST3, pSensorInfo->SensorGrabStartY_CST3, pSensorInfo->Custom3DelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nCST4: TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorCustom4Width,psensorResolution[IDNum]->SensorCustom4Height, pSensorInfo->SensorGrabStartX_CST4, pSensorInfo->SensorGrabStartY_CST4, pSensorInfo->Custom4DelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nCST5: TgGrab_w,h,x_,y=%5d,%5d,%3d,%3d, delay_frm=%2d",mtk_ccm_name,psensorResolution[IDNum]->SensorCustom5Width,psensorResolution[IDNum]->SensorCustom5Height, pSensorInfo->SensorGrabStartX_CST5, pSensorInfo->SensorGrabStartY_CST5, pSensorInfo->Custom5DelayFrame);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nSeninf_Type(0:parallel,1:mipi,2:serial)=%d, output_format(0:B,1:Gb,2:Gr,3:R)=%2d",mtk_ccm_name, pSensorInfo->SensroInterfaceType, pSensorInfo->SensorOutputDataFormat);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nDriving_Current(0:2mA,1:4mA,2:6mA,3:8mA)=%d, mclk_freq=%2d, mipi_lane=%d",mtk_ccm_name, pSensorInfo->SensorDrivingCurrent, pSensorInfo->SensorClockFreq, pSensorInfo->SensorMIPILaneNumber + 1);
			snprintf(mtk_ccm_name,sizeof(mtk_ccm_name),"%s \nPDAF_Support(0:No PD,1:PD RAW,2:VC(Full),3:VC(Bin),4:Dual Raw,5:Dual VC=%2d",mtk_ccm_name, pSensorInfo->PDAF_Support);
			snprintf(mtk_ccm_name, sizeof(mtk_ccm_name), "%s \nHDR_Support(0:NO HDR,1: iHDR,2:mvHDR,3:zHDR)=%2d", mtk_ccm_name, pSensorInfo->HDR_Support);
		}
	}
	}

    if (DUAL_CAMERA_MAIN_SENSOR == pSensorGetInfo->SensorId) {
    /* Resolution */
    PK_DBG("[adopt_CAMERA_HW_GetInfo2]Resolution\n");
    if (copy_to_user((void __user *) (pSensorGetInfo->pSensorResolution) , (void *)psensorResolution[0] , sizeof(MSDK_SENSOR_RESOLUTION_INFO_STRUCT))) {
       PK_DBG("[CAMERA_HW][Resolution] ioctl copy to user failed\n");
	   for (i = 0; i < 2; i++) {
		   if(pConfig[i] != NULL) kfree(pConfig[i]);
		   if(pConfig1[i] != NULL) kfree(pConfig1[i]);
		   if(pConfig2[i] != NULL) kfree(pConfig2[i]);
		   if(pConfig3[i] != NULL) kfree(pConfig3[i]);
		   if(pConfig4[i] != NULL) kfree(pConfig4[i]);
		   if(psensorResolution[i] != NULL) kfree(psensorResolution[i]);
		   pConfig[i] = NULL;
		   pConfig1[i] = NULL;
		   pConfig2[i] = NULL;
		   pConfig3[i] =  NULL;
		   pConfig4[i] =  NULL;
		   psensorResolution[i] = NULL;
	   }
   		kfree(pSensorInfo);
		pSensorInfo = NULL;
       return -EFAULT;
    }
    } else{
     /* Resolution */
     if (copy_to_user((void __user *) (pSensorGetInfo->pSensorResolution) , (void *)psensorResolution[1] , sizeof(MSDK_SENSOR_RESOLUTION_INFO_STRUCT))) {
        PK_DBG("[CAMERA_HW][Resolution] ioctl copy to user failed\n");
		for (i = 0; i < 2; i++) {
			if(pConfig[i] != NULL) kfree(pConfig[i]);
			if(pConfig1[i] != NULL) kfree(pConfig1[i]);
			if(pConfig2[i] != NULL) kfree(pConfig2[i]);
			if(pConfig3[i] != NULL) kfree(pConfig3[i]);
			if(pConfig4[i] != NULL) kfree(pConfig4[i]);
			if(psensorResolution[i] != NULL) kfree(psensorResolution[i]);
			pConfig[i] = NULL;
			pConfig1[i] = NULL;
			pConfig2[i] = NULL;
			pConfig3[i] =  NULL;
			pConfig4[i] =  NULL;
			psensorResolution[i] = NULL;
		}
			kfree(pSensorInfo);
		pSensorInfo = NULL;
        return -EFAULT;
    }
    }

	for (i = 0; i < 2; i++) {
		if(pConfig[i] != NULL) kfree(pConfig[i]);
		if(pConfig1[i] != NULL) kfree(pConfig1[i]);
		if(pConfig2[i] != NULL) kfree(pConfig2[i]);
		if(pConfig3[i] != NULL) kfree(pConfig3[i]);
		if(pConfig4[i] != NULL) kfree(pConfig4[i]);
		if(psensorResolution[i] != NULL) kfree(psensorResolution[i]);
		   pConfig[i] = NULL;
		   pConfig1[i] = NULL;
		   pConfig2[i] = NULL;
		   pConfig3[i] =  NULL;
		   pConfig4[i] =  NULL;
		   psensorResolution[i] = NULL;
	}
	kfree(pSensorInfo);
	pSensorInfo = NULL;

    return 0;
}   /* adopt_CAMERA_HW_GetInfo() */


/*******************************************************************************
* adopt_CAMERA_HW_Control
********************************************************************************/
inline static int adopt_CAMERA_HW_Control(void *pBuf)
{
    int ret = 0;
    ACDK_SENSOR_CONTROL_STRUCT *pSensorCtrl = (ACDK_SENSOR_CONTROL_STRUCT *)pBuf;
    MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT imageWindow;
    MSDK_SENSOR_CONFIG_STRUCT sensorConfigData;
    memset(&imageWindow, 0, sizeof(ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT));
    memset(&sensorConfigData, 0, sizeof(ACDK_SENSOR_CONFIG_STRUCT));

    if (NULL == pSensorCtrl) {
    PK_DBG("[CAMERA_HW] NULL arg.\n");
    return -EFAULT;
    }

    if (NULL == pSensorCtrl->pImageWindow || NULL == pSensorCtrl->pSensorConfigData) {
    PK_DBG("[CAMERA_HW] NULL arg.\n");
    return -EFAULT;
    }

    if (copy_from_user((void *)&imageWindow , (void *) pSensorCtrl->pImageWindow, sizeof(ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT))) {
    PK_DBG("[CAMERA_HW][pFeatureData32] ioctl copy from user failed\n");
    return -EFAULT;
    }

    if (copy_from_user((void *)&sensorConfigData , (void *) pSensorCtrl->pSensorConfigData, sizeof(ACDK_SENSOR_CONFIG_STRUCT))) {
    PK_DBG("[CAMERA_HW][pFeatureData32] ioctl copy from user failed\n");
    return -EFAULT;
    }

    /*  */
    if (g_pSensorFunc) {
    ret = g_pSensorFunc->SensorControl(pSensorCtrl->InvokeCamera, pSensorCtrl->ScenarioId, &imageWindow, &sensorConfigData);
    }
    else {
    PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
    }

    /*  */
    if (copy_to_user((void __user *) pSensorCtrl->pImageWindow, (void *)&imageWindow , sizeof(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT))) {
    PK_DBG("[CAMERA_HW][imageWindow] ioctl copy to user failed\n");
    return -EFAULT;
    }

    /*  */
    if (copy_to_user((void __user *) pSensorCtrl->pSensorConfigData, (void *)&sensorConfigData , sizeof(MSDK_SENSOR_CONFIG_STRUCT))) {
    PK_DBG("[CAMERA_HW][imageWindow] ioctl copy to user failed\n");
    return -EFAULT;
    }
    return ret;
} /* adopt_CAMERA_HW_Control */

/*******************************************************************************
* adopt_CAMERA_HW_FeatureControl
********************************************************************************/
inline static int  adopt_CAMERA_HW_FeatureControl(void *pBuf)
{
    ACDK_SENSOR_FEATURECONTROL_STRUCT *pFeatureCtrl = (ACDK_SENSOR_FEATURECONTROL_STRUCT *)pBuf;
    unsigned int FeatureParaLen = 0;
    void *pFeaturePara = NULL;
#if 0
    ACDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo = NULL;
    char kernelGroupNamePtr[128];
    unsigned char *pUserGroupNamePtr = NULL;
#endif
	ACDK_KD_SENSOR_SYNC_STRUCT *pSensorSyncInfo = NULL;
    signed int ret = 0;



    if (NULL == pFeatureCtrl) {
    PK_ERR(" NULL arg.\n");
    return -EFAULT;
    }

    if (SENSOR_FEATURE_SINGLE_FOCUS_MODE == pFeatureCtrl->FeatureId || SENSOR_FEATURE_CANCEL_AF == pFeatureCtrl->FeatureId
    || SENSOR_FEATURE_CONSTANT_AF == pFeatureCtrl->FeatureId || SENSOR_FEATURE_INFINITY_AF == pFeatureCtrl->FeatureId) {/* YUV AF_init and AF_constent and AF_single has no params */
    }
    else
    {
		if (NULL == pFeatureCtrl->pFeaturePara || NULL == pFeatureCtrl->pFeatureParaLen) {
			PK_ERR(" NULL arg.\n");
			return -EFAULT;
		}
		if (copy_from_user((void *)&FeatureParaLen , (void *) pFeatureCtrl->pFeatureParaLen, sizeof(unsigned int))) {
			PK_ERR(" ioctl copy from user failed\n");
			return -EFAULT;
		}

		pFeaturePara = kmalloc(FeatureParaLen, GFP_KERNEL);
		if (NULL == pFeaturePara) {
			PK_ERR(" ioctl allocate mem failed\n");
			return -ENOMEM;
		}
		memset(pFeaturePara, 0x0, FeatureParaLen);
    }

    /* copy from user */
    switch (pFeatureCtrl->FeatureId)
    {
    case SENSOR_FEATURE_SET_ESHUTTER:
    case SENSOR_FEATURE_SET_GAIN:
	case SENSOR_FEATURE_SET_DUAL_GAIN:
        /* reset the delay frame flag */
        spin_lock(&kdsensor_drv_lock);
        g_NewSensorExpGain.uSensorExpDelayFrame = 0xFF;
        g_NewSensorExpGain.uSensorGainDelayFrame = 0xFF;
        g_NewSensorExpGain.uISPGainDelayFrame = 0xFF;
        spin_unlock(&kdsensor_drv_lock);
	case SENSOR_FEATURE_SET_I2C_BUF_MODE_EN:
	case SENSOR_FEATURE_SET_SHUTTER_BUF_MODE:
	case SENSOR_FEATURE_SET_GAIN_BUF_MODE:
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
    case SENSOR_FEATURE_SET_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER:
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
    case SENSOR_FEATURE_SET_VIDEO_MODE:
    case SENSOR_FEATURE_SET_YUV_CMD:
    case SENSOR_FEATURE_MOVE_FOCUS_LENS:
    case SENSOR_FEATURE_SET_AF_WINDOW:
    case SENSOR_FEATURE_SET_CALIBRATION_DATA:
    case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
    case SENSOR_FEATURE_GET_EV_AWB_REF:
    case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
    case SENSOR_FEATURE_SET_AE_WINDOW:
    case SENSOR_FEATURE_GET_EXIF_INFO:
    case SENSOR_FEATURE_GET_DELAY_INFO:
    case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
    case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
    case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
    case SENSOR_FEATURE_SET_TEST_PATTERN:
    case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
    case SENSOR_FEATURE_SET_OB_LOCK:
    case SENSOR_FEATURE_SET_SENSOR_OTP_AWB_CMD:
    case SENSOR_FEATURE_SET_SENSOR_OTP_LSC_CMD:
    case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
    case SENSOR_FEATURE_SET_FRAMERATE:
    case SENSOR_FEATURE_SET_HDR:
    case SENSOR_FEATURE_GET_CROP_INFO:
    case SENSOR_FEATURE_GET_VC_INFO:
    case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
	case SENSOR_FEATURE_SET_HDR_SHUTTER:
    case SENSOR_FEATURE_GET_AE_FLASHLIGHT_INFO:
    case SENSOR_FEATURE_GET_TRIGGER_FLASHLIGHT_INFO: /* return TRUE:play flashlight */
    case SENSOR_FEATURE_SET_YUV_3A_CMD: /* para: ACDK_SENSOR_3A_LOCK_ENUM */
    case SENSOR_FEATURE_SET_AWB_GAIN:
    case SENSOR_FEATURE_SET_MIN_MAX_FPS:
    case SENSOR_FEATURE_GET_PDAF_INFO:
    case SENSOR_FEATURE_GET_PDAF_DATA:
    case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
    case SENSOR_FEATURE_GET_SENSOR_HDR_CAPACITY:
    case SENSOR_FEATURE_SET_PDAF:
    case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
	case SENSOR_FEATURE_SET_PDFOCUS_AREA:
	case SENSOR_FEATURE_GET_PDAF_REG_SETTING:
	case SENSOR_FEATURE_SET_PDAF_REG_SETTING:
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
        /*  */
        if (copy_from_user((void *)pFeaturePara , (void *) pFeatureCtrl->pFeaturePara, FeatureParaLen)) {
        kfree(pFeaturePara);
        PK_ERR("[CAMERA_HW][pFeaturePara] ioctl copy from user failed\n");
        return -EFAULT;
        }
        break;
     case SENSOR_FEATURE_SET_SENSOR_SYNC:    /* Update new sensor exposure time and gain to keep */
        if (copy_from_user((void *)pFeaturePara , (void *) pFeatureCtrl->pFeaturePara, FeatureParaLen)) {
	 kfree(pFeaturePara);
         PK_ERR("[CAMERA_HW][pFeaturePara] ioctl copy from user failed\n");
         return -EFAULT;
    }
    /* keep the information to wait Vsync synchronize */
    pSensorSyncInfo = (ACDK_KD_SENSOR_SYNC_STRUCT *)pFeaturePara;
    spin_lock(&kdsensor_drv_lock);
    g_NewSensorExpGain.u2SensorNewExpTime = pSensorSyncInfo->u2SensorNewExpTime;
    g_NewSensorExpGain.u2SensorNewGain = pSensorSyncInfo->u2SensorNewGain;
    g_NewSensorExpGain.u2ISPNewRGain = pSensorSyncInfo->u2ISPNewRGain;
    g_NewSensorExpGain.u2ISPNewGrGain = pSensorSyncInfo->u2ISPNewGrGain;
    g_NewSensorExpGain.u2ISPNewGbGain = pSensorSyncInfo->u2ISPNewGbGain;
    g_NewSensorExpGain.u2ISPNewBGain = pSensorSyncInfo->u2ISPNewBGain;
    g_NewSensorExpGain.uSensorExpDelayFrame = pSensorSyncInfo->uSensorExpDelayFrame;
    g_NewSensorExpGain.uSensorGainDelayFrame = pSensorSyncInfo->uSensorGainDelayFrame;
    g_NewSensorExpGain.uISPGainDelayFrame = pSensorSyncInfo->uISPGainDelayFrame;
    /* AE smooth not change shutter to speed up */
    if ((0 == g_NewSensorExpGain.u2SensorNewExpTime) || (0xFFFF == g_NewSensorExpGain.u2SensorNewExpTime)) {
        g_NewSensorExpGain.uSensorExpDelayFrame = 0xFF;
    }

    if (g_NewSensorExpGain.uSensorExpDelayFrame == 0) {
        FeatureParaLen = 2;
        g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera, SENSOR_FEATURE_SET_ESHUTTER, (unsigned char *)&g_NewSensorExpGain.u2SensorNewExpTime, (unsigned int *) &FeatureParaLen);
        g_NewSensorExpGain.uSensorExpDelayFrame = 0xFF; /* disable */
    }
    else if (g_NewSensorExpGain.uSensorExpDelayFrame != 0xFF) {
        g_NewSensorExpGain.uSensorExpDelayFrame--;
    }
    /* exposure gain */
    if (g_NewSensorExpGain.uSensorGainDelayFrame == 0) {
        FeatureParaLen = 2;
        g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera, SENSOR_FEATURE_SET_GAIN, (unsigned char *)&g_NewSensorExpGain.u2SensorNewGain, (unsigned int *) &FeatureParaLen);
        g_NewSensorExpGain.uSensorGainDelayFrame = 0xFF; /* disable */
    }
    else if (g_NewSensorExpGain.uSensorGainDelayFrame != 0xFF) {
        g_NewSensorExpGain.uSensorGainDelayFrame--;
    }
    /* if the delay frame is 0 or 0xFF, stop to count */
    if ((g_NewSensorExpGain.uISPGainDelayFrame != 0xFF) && (g_NewSensorExpGain.uISPGainDelayFrame != 0)) {
        g_NewSensorExpGain.uISPGainDelayFrame--;
    }

	spin_unlock(&kdsensor_drv_lock);
     break;
#if 0
    case SENSOR_FEATURE_GET_GROUP_INFO:
        if (copy_from_user((void *)pFeaturePara , (void *) pFeatureCtrl->pFeaturePara, FeatureParaLen)) {
        kfree(pFeaturePara);
        PK_DBG("[CAMERA_HW][pFeaturePara] ioctl copy from user failed\n");
        return -EFAULT;
        }
        pSensorGroupInfo = (ACDK_SENSOR_GROUP_INFO_STRUCT *)pFeaturePara;
        pUserGroupNamePtr = pSensorGroupInfo->GroupNamePtr;
        /*  */
        if (NULL == pUserGroupNamePtr) {
        kfree(pFeaturePara);
        PK_DBG("[CAMERA_HW] NULL arg.\n");
        return -EFAULT;
        }
        pSensorGroupInfo->GroupNamePtr = kernelGroupNamePtr;
        break;
#endif
    case SENSOR_FEATURE_SET_ESHUTTER_GAIN:
        if (copy_from_user((void *)pFeaturePara , (void *) pFeatureCtrl->pFeaturePara, FeatureParaLen)) {
	 kfree(pFeaturePara);
        PK_ERR("[CAMERA_HW][pFeaturePara] ioctl copy from user failed\n");
        return -EFAULT;
        }
        /* keep the information to wait Vsync synchronize */
        pSensorSyncInfo = (ACDK_KD_SENSOR_SYNC_STRUCT *)pFeaturePara;
        spin_lock(&kdsensor_drv_lock);
        g_NewSensorExpGain.u2SensorNewExpTime = pSensorSyncInfo->u2SensorNewExpTime;
        g_NewSensorExpGain.u2SensorNewGain = pSensorSyncInfo->u2SensorNewGain;
        spin_unlock(&kdsensor_drv_lock);
        kdSetExpGain(pFeatureCtrl->InvokeCamera);
        break;
    /* copy to user */
    case SENSOR_FEATURE_GET_RESOLUTION:
    case SENSOR_FEATURE_GET_PERIOD:
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_GET_CONFIG_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
    /* do nothing */
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_SINGLE_FOCUS_MODE:
    case SENSOR_FEATURE_CANCEL_AF:
    case SENSOR_FEATURE_CONSTANT_AF:
    default:
        break;
    }

	/*in case that some structure are passed from user sapce by ptr */
	switch (pFeatureCtrl->FeatureId) {
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
    case SENSOR_FEATURE_GET_SENSOR_HDR_CAPACITY:
		{
			MUINT32 *pValue = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			pValue = kmalloc(sizeof(MUINT32), GFP_KERNEL);
			if (pValue == NULL) {
				PK_ERR(" ioctl allocate mem failed\n");
				kfree(pFeaturePara);
				return -ENOMEM;
			}

			memset(pValue, 0x0, sizeof(MUINT32));
			*(pFeaturePara_64 + 1) = (uintptr_t)pValue;

			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_ERR("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}
			*(pFeaturePara_64 + 1) = *pValue;
			kfree(pValue);
		}
		break;
	case SENSOR_FEATURE_GET_AE_STATUS:
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
	case SENSOR_FEATURE_GET_AF_STATUS:
	case SENSOR_FEATURE_GET_AWB_STATUS:
	case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
	case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
	case SENSOR_FEATURE_GET_TRIGGER_FLASHLIGHT_INFO:
	case SENSOR_FEATURE_GET_SENSOR_N3D_STREAM_TO_VSYNC_TIME:
	case SENSOR_FEATURE_GET_PERIOD:
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		{

			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}
		}
		break;
	case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
	case SENSOR_FEATURE_AUTOTEST_CMD:
		{
			MUINT32 *pValue0 = NULL;
			MUINT32 *pValue1 = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			pValue0 = kmalloc(sizeof(MUINT32), GFP_KERNEL);
			pValue1 = kmalloc(sizeof(MUINT32), GFP_KERNEL);

			if (pValue0 == NULL || pValue1 == NULL) {
				PK_ERR(" ioctl allocate mem failed\n");
				kfree(pValue0);
				kfree(pValue1);
				kfree(pFeaturePara);
				return -ENOMEM;
			}
			memset(pValue1, 0x0, sizeof(MUINT32));
			memset(pValue0, 0x0, sizeof(MUINT32));
			*(pFeaturePara_64) = (uintptr_t)pValue0;
			*(pFeaturePara_64 + 1) = (uintptr_t)pValue1;

			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_ERR("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}
			*(pFeaturePara_64) = *pValue0;
			*(pFeaturePara_64 + 1) = *pValue1;
			kfree(pValue0);
			kfree(pValue1);
		}
		break;


	case SENSOR_FEATURE_GET_EV_AWB_REF:
		{
			SENSOR_AE_AWB_REF_STRUCT *pAeAwbRef = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			void *usr_ptr = (void*)(uintptr_t)(*(pFeaturePara_64));
			pAeAwbRef = kmalloc(sizeof(SENSOR_AE_AWB_REF_STRUCT), GFP_KERNEL);
			if (pAeAwbRef == NULL) {
				kfree(pFeaturePara);
				PK_ERR(" ioctl allocate mem failed\n");
				return -ENOMEM;
			}
			memset(pAeAwbRef, 0x0, sizeof(SENSOR_AE_AWB_REF_STRUCT));
			*(pFeaturePara_64) = (uintptr_t)pAeAwbRef;
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}
			if (copy_to_user
			    ((void __user *)usr_ptr, (void *)pAeAwbRef,
			     sizeof(SENSOR_AE_AWB_REF_STRUCT))) {
				PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
			}
			kfree(pAeAwbRef);
			*(pFeaturePara_64) = (uintptr_t)usr_ptr;
		}
		break;

	case SENSOR_FEATURE_GET_CROP_INFO:
		{
			SENSOR_WINSIZE_INFO_STRUCT *pCrop = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			void *usr_ptr = (void *)(uintptr_t) (*(pFeaturePara_64 + 1));
			pCrop = kmalloc(sizeof(SENSOR_WINSIZE_INFO_STRUCT), GFP_KERNEL);
			if (pCrop == NULL) {
				kfree(pFeaturePara);
				PK_ERR(" ioctl allocate mem failed\n");
				return -ENOMEM;
			}
			memset(pCrop, 0x0, sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			*(pFeaturePara_64 + 1) = (uintptr_t)pCrop;
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}
			//PK_DBG("[CAMERA_HW]crop =%d\n",framerate);

			if (copy_to_user
			    ((void __user *)usr_ptr, (void *)pCrop,
			     sizeof(SENSOR_WINSIZE_INFO_STRUCT))) {
				PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
			}
			kfree(pCrop);
			*(pFeaturePara_64 + 1) = (uintptr_t)usr_ptr;
		}
		break;

	case SENSOR_FEATURE_GET_VC_INFO:
		{
			SENSOR_VC_INFO_STRUCT *pVcInfo = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			void *usr_ptr = (void *)(uintptr_t) (*(pFeaturePara_64 + 1));
			pVcInfo = kmalloc(sizeof(SENSOR_VC_INFO_STRUCT), GFP_KERNEL);
			if (pVcInfo == NULL) {
				PK_ERR(" ioctl allocate mem failed\n");
				kfree(pFeaturePara);
				return -ENOMEM;
			}
			memset(pVcInfo, 0x0, sizeof(SENSOR_VC_INFO_STRUCT));
			*(pFeaturePara_64 + 1) = (uintptr_t)pVcInfo;
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}

			if (copy_to_user
			    ((void __user *)usr_ptr, (void *)pVcInfo,
			     sizeof(SENSOR_VC_INFO_STRUCT))) {
				PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
			}
			kfree(pVcInfo);
			*(pFeaturePara_64 + 1) = (uintptr_t)usr_ptr;
		}
		break;

	case SENSOR_FEATURE_GET_PDAF_INFO:
		{

#if 1
			SET_PD_BLOCK_INFO_T *pPdInfo = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			void *usr_ptr = (void *)(uintptr_t) (*(pFeaturePara_64 + 1));
			pPdInfo = kmalloc(sizeof(SET_PD_BLOCK_INFO_T), GFP_KERNEL);
			if (pPdInfo == NULL) {
				kfree(pFeaturePara);
				PK_ERR(" ioctl allocate mem failed\n");
				return -ENOMEM;
			}
			memset(pPdInfo, 0x0, sizeof(SET_PD_BLOCK_INFO_T));
			*(pFeaturePara_64 + 1) = (uintptr_t)pPdInfo;
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}

			if (copy_to_user
			    ((void __user *)usr_ptr, (void *)pPdInfo,
			     sizeof(SET_PD_BLOCK_INFO_T))) {
				PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
			}
			kfree(pPdInfo);
			*(pFeaturePara_64 + 1) = (uintptr_t)usr_ptr;
#endif
		}
		break;

	case SENSOR_FEATURE_GET_PDAF_REG_SETTING:
	case SENSOR_FEATURE_SET_PDAF_REG_SETTING:
	{
		unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
		kal_uint32 u4RegLen = (*pFeaturePara_64);
		void *usr_ptr_Reg = (void *)(uintptr_t) (*(pFeaturePara_64 + 1));
		kal_uint32 *pReg = NULL;

		pReg = kmalloc(sizeof(kal_uint8)*u4RegLen, GFP_KERNEL);

		if (pReg == NULL) {
			kfree(pFeaturePara);
			PK_ERR(" ioctl allocate mem failed\n");
			return -ENOMEM;
		}

		memset(pReg, 0x0, sizeof(kal_uint8)*u4RegLen);

		if (copy_from_user
		((void *)pReg, (void *)usr_ptr_Reg, sizeof(kal_uint8)*u4RegLen)) {
			PK_ERR("[CAMERA_HW]ERROR: copy from user fail\n");
		}

		if (g_pSensorFunc) {
			ret =
				g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
					pFeatureCtrl->FeatureId,
					(unsigned char *)
					pReg,
					(unsigned int *)
					&u4RegLen);
		} else {
			PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
		}

		if (copy_to_user
			((void __user *)usr_ptr_Reg, (void *)pReg,
				sizeof(kal_uint8)*u4RegLen)) {
			PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
		}
		kfree(pReg);
	}

	break;

	case SENSOR_FEATURE_SET_AF_WINDOW:
	case SENSOR_FEATURE_SET_AE_WINDOW:
		{
			MUINT32 *pApWindows = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			void *usr_ptr = (void *)(uintptr_t) (*(pFeaturePara_64));
			pApWindows = kmalloc(sizeof(MUINT32) * 6, GFP_KERNEL);
			if (pApWindows == NULL) {
				kfree(pFeaturePara);
				PK_ERR(" ioctl allocate mem failed\n");
				return -ENOMEM;
			}
			memset(pApWindows, 0x0, sizeof(MUINT32) * 6);
			*(pFeaturePara_64) = (uintptr_t)pApWindows;

			if (copy_from_user
			    ((void *)pApWindows, (void *)usr_ptr, sizeof(MUINT32) * 6)) {
				PK_ERR("[CAMERA_HW]ERROR: copy from user fail \n");
			}
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_ERR("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}
			kfree(pApWindows);
			*(pFeaturePara_64) = (uintptr_t)usr_ptr;
		}
		break;

	case SENSOR_FEATURE_GET_EXIF_INFO:
		{
			SENSOR_EXIF_INFO_STRUCT *pExif = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			void *usr_ptr =  (void *)(uintptr_t) (*(pFeaturePara_64));
			pExif = kmalloc(sizeof(SENSOR_EXIF_INFO_STRUCT), GFP_KERNEL);
			if (pExif == NULL) {
				kfree(pFeaturePara);
				PK_ERR(" ioctl allocate mem failed\n");
				return -ENOMEM;
			}
			memset(pExif, 0x0, sizeof(SENSOR_EXIF_INFO_STRUCT));
			*(pFeaturePara_64) = (uintptr_t)pExif;
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}

			if (copy_to_user
			    ((void __user *)usr_ptr, (void *)pExif,
			     sizeof(SENSOR_EXIF_INFO_STRUCT))) {
				PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
			}
			kfree(pExif);
			*(pFeaturePara_64) = (uintptr_t)usr_ptr;
		}
		break;


	case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
		{

			SENSOR_AE_AWB_CUR_STRUCT *pCurAEAWB = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			void *usr_ptr = (void *)(uintptr_t) (*(pFeaturePara_64));
			pCurAEAWB = kmalloc(sizeof(SENSOR_AE_AWB_CUR_STRUCT), GFP_KERNEL);
			if (pCurAEAWB == NULL) {
				kfree(pFeaturePara);
				PK_ERR(" ioctl allocate mem failed\n");
				return -ENOMEM;
			}
			memset(pCurAEAWB, 0x0, sizeof(SENSOR_AE_AWB_CUR_STRUCT));
			*(pFeaturePara_64) = (uintptr_t)pCurAEAWB;
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}

			if (copy_to_user
			    ((void __user *)usr_ptr, (void *)pCurAEAWB,
			     sizeof(SENSOR_AE_AWB_CUR_STRUCT))) {
				PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
			}
			kfree(pCurAEAWB);
			*(pFeaturePara_64) = (uintptr_t)usr_ptr;
		}
		break;

	case SENSOR_FEATURE_GET_DELAY_INFO:
		{
			SENSOR_DELAY_INFO_STRUCT *pDelayInfo = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			void *usr_ptr = (void *)(uintptr_t) (*(pFeaturePara_64));
			pDelayInfo = kmalloc(sizeof(SENSOR_DELAY_INFO_STRUCT), GFP_KERNEL);

			if (pDelayInfo == NULL) {
				kfree(pFeaturePara);
				PK_ERR(" ioctl allocate mem failed\n");
				return -ENOMEM;
			}
			memset(pDelayInfo, 0x0, sizeof(SENSOR_DELAY_INFO_STRUCT));
			*(pFeaturePara_64) = (uintptr_t)pDelayInfo;
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}

			if (copy_to_user
			    ((void __user *)usr_ptr, (void *)pDelayInfo,
			     sizeof(SENSOR_DELAY_INFO_STRUCT))) {
				PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
			}
			kfree(pDelayInfo);
			*(pFeaturePara_64) = (uintptr_t)usr_ptr;

		}
		break;


	case SENSOR_FEATURE_GET_AE_FLASHLIGHT_INFO:
		{
			SENSOR_FLASHLIGHT_AE_INFO_STRUCT *pFlashInfo = NULL;
			unsigned long long *pFeaturePara_64 = (unsigned long long *)pFeaturePara;
			void *usr_ptr = (void *)(uintptr_t) (*(pFeaturePara_64));
			pFlashInfo = kmalloc(sizeof(SENSOR_FLASHLIGHT_AE_INFO_STRUCT), GFP_KERNEL);

			if (pFlashInfo == NULL) {
				kfree(pFeaturePara);
				PK_ERR(" ioctl allocate mem failed\n");
				return -ENOMEM;
			}
			memset(pFlashInfo, 0x0, sizeof(SENSOR_FLASHLIGHT_AE_INFO_STRUCT));
			*(pFeaturePara_64) = (uintptr_t)pFlashInfo;
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}

			if (copy_to_user
			    ((void __user *)usr_ptr, (void *)pFlashInfo,
			     sizeof(SENSOR_FLASHLIGHT_AE_INFO_STRUCT))) {
				PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
			}
			kfree(pFlashInfo);
			*(pFeaturePara_64) = (uintptr_t)usr_ptr;

		}
		break;


	case SENSOR_FEATURE_GET_PDAF_DATA:
		{
			char *pPdaf_data = NULL;

			unsigned long long *pFeaturePara_64=(unsigned long long *) pFeaturePara;
			void *usr_ptr = (void *)(uintptr_t)(*(pFeaturePara_64 + 1));
			#if 1
			pPdaf_data = kmalloc(sizeof(char) * PDAF_DATA_SIZE, GFP_KERNEL);
			if (pPdaf_data == NULL) {
				kfree(pFeaturePara);
				PK_ERR(" ioctl allocate mem failed\n");
				return -ENOMEM;
			}
			memset(pPdaf_data, 0xff, sizeof(char) * PDAF_DATA_SIZE);

			if (pFeaturePara_64 != NULL) {
				*(pFeaturePara_64 + 1) = (uintptr_t)pPdaf_data;//*(pFeaturePara_64 + 1) = (uintptr_t)pPdaf_data;
			}
			if (g_pSensorFunc) {
				ret =
				    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
									pFeatureCtrl->FeatureId,
									(unsigned char *)
									pFeaturePara,
									(unsigned int *)
									&FeatureParaLen);
			} else {
				PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
			}

			if (copy_to_user
			    ((void __user *)usr_ptr, (void *)pPdaf_data,
			     (kal_uint32) (*(pFeaturePara_64 + 2)))) {
				PK_DBG("[CAMERA_HW]ERROR: copy_to_user fail \n");
			}
			kfree(pPdaf_data);
			*(pFeaturePara_64 + 1) =(uintptr_t) usr_ptr;

#endif
		}
		break;
	default:

		if (g_pSensorFunc) {
			ret =
			    g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->InvokeCamera,
								pFeatureCtrl->FeatureId,
								(unsigned char *)pFeaturePara,
								(unsigned int *)&FeatureParaLen);
#ifdef CONFIG_MTK_CCU
			if (pFeatureCtrl->FeatureId == SENSOR_FEATURE_SET_FRAMERATE)
				ccu_set_current_fps(*((int32_t *) pFeaturePara));
#endif
		} else {
			PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
		}

		break;
    }
    /* copy to user */
    switch (pFeatureCtrl->FeatureId)
    {
	case SENSOR_FEATURE_SET_I2C_BUF_MODE_EN:
		i2c_buf_mode_en((*(unsigned long long *)pFeaturePara));
		break;
    case SENSOR_FEATURE_SET_ESHUTTER:
    case SENSOR_FEATURE_SET_GAIN:
	case SENSOR_FEATURE_SET_DUAL_GAIN:
	case SENSOR_FEATURE_SET_SHUTTER_BUF_MODE:
	case SENSOR_FEATURE_SET_GAIN_BUF_MODE:
    case SENSOR_FEATURE_SET_GAIN_AND_ESHUTTER:
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
    case SENSOR_FEATURE_SET_REGISTER:
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    /* do nothing */
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_PDAF_DATA:
	case SENSOR_FEATURE_GET_PDAF_REG_SETTING:
	case SENSOR_FEATURE_SET_PDAF_REG_SETTING:
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
        break;
    /* copy to user */
    case SENSOR_FEATURE_GET_EV_AWB_REF:
    case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
    case SENSOR_FEATURE_GET_EXIF_INFO:
    case SENSOR_FEATURE_GET_DELAY_INFO:
    case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
    case SENSOR_FEATURE_GET_RESOLUTION:
    case SENSOR_FEATURE_GET_PERIOD:
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
    case SENSOR_FEATURE_GET_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_GET_CONFIG_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
    case SENSOR_FEATURE_GET_AF_STATUS:
    case SENSOR_FEATURE_GET_AE_STATUS:
    case SENSOR_FEATURE_GET_AWB_STATUS:
    case SENSOR_FEATURE_GET_AF_INF:
    case SENSOR_FEATURE_GET_AF_MACRO:
    case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
    case SENSOR_FEATURE_GET_TRIGGER_FLASHLIGHT_INFO: /* return TRUE:play flashlight */
    case SENSOR_FEATURE_SET_YUV_3A_CMD: /* para: ACDK_SENSOR_3A_LOCK_ENUM */
    case SENSOR_FEATURE_GET_AE_FLASHLIGHT_INFO:
    case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
    case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
    case SENSOR_FEATURE_SET_TEST_PATTERN:
    case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
    case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
    case SENSOR_FEATURE_SET_FRAMERATE:
    case SENSOR_FEATURE_SET_HDR:
    case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
    case SENSOR_FEATURE_SET_HDR_SHUTTER:
    case SENSOR_FEATURE_GET_CROP_INFO:
    case SENSOR_FEATURE_GET_VC_INFO:
    case SENSOR_FEATURE_SET_MIN_MAX_FPS:
    case SENSOR_FEATURE_GET_PDAF_INFO:
    case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
    case SENSOR_FEATURE_GET_SENSOR_HDR_CAPACITY:
	case SENSOR_FEATURE_SET_ISO:
    case SENSOR_FEATURE_SET_PDAF:
    case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
	case SENSOR_FEATURE_SET_PDFOCUS_AREA:
        /*  */
        if (copy_to_user((void __user *) pFeatureCtrl->pFeaturePara, (void *)pFeaturePara , FeatureParaLen)) {
        kfree(pFeaturePara);
        PK_DBG("[CAMERA_HW][pSensorRegData] ioctl copy to user failed\n");
        return -EFAULT;
        }
        break;
#if 0
    /* copy from and to user */
    case SENSOR_FEATURE_GET_GROUP_INFO:
        /* copy 32 bytes */
        if (copy_to_user((void __user *) pUserGroupNamePtr, (void *)kernelGroupNamePtr , sizeof(char)*32)) {
        kfree(pFeaturePara);
        PK_DBG("[CAMERA_HW][pFeatureReturnPara32] ioctl copy to user failed\n");
        return -EFAULT;
        }
        pSensorGroupInfo->GroupNamePtr = pUserGroupNamePtr;
        if (copy_to_user((void __user *) pFeatureCtrl->pFeaturePara, (void *)pFeaturePara , FeatureParaLen)) {
        kfree(pFeaturePara);
        PK_DBG("[CAMERA_HW][pFeatureReturnPara32] ioctl copy to user failed\n");
        return -EFAULT;
        }
        break;
#endif
    default:
        break;
    }

    kfree(pFeaturePara);
    if (copy_to_user((void __user *) pFeatureCtrl->pFeatureParaLen, (void *)&FeatureParaLen , sizeof(unsigned int))) {
    PK_DBG("[CAMERA_HW][pFeatureParaLen] ioctl copy to user failed\n");
    return -EFAULT;
    }
    return ret;
}   /* adopt_CAMERA_HW_FeatureControl() */


/*******************************************************************************
* adopt_CAMERA_HW_Close
********************************************************************************/
inline static int adopt_CAMERA_HW_Close(void)
{
    /* if (atomic_read(&g_CamHWOpend) == 0) { */
    /* return 0; */
    /* } */
    /* else if(atomic_read(&g_CamHWOpend) == 1) { */
    if (g_pSensorFunc) {
        g_pSensorFunc->SensorClose();
    }
    else {
        PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
    }
    /* power off sensor */
    /* Should close power in kd_MultiSensorClose function
     * The following function will close all opened sensors.
     */
    /* kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM*)g_invokeSocketIdx, g_invokeSensorNameStr, false, CAMERA_HW_DRVNAME1); */
    /* } */
    /* atomic_set(&g_CamHWOpend, 0); */

    atomic_set(&g_CamHWOpening, 0);

    /* reset the delay frame flag */
    spin_lock(&kdsensor_drv_lock);
    g_NewSensorExpGain.uSensorExpDelayFrame = 0xFF;
    g_NewSensorExpGain.uSensorGainDelayFrame = 0xFF;
    g_NewSensorExpGain.uISPGainDelayFrame = 0xFF;
    spin_unlock(&kdsensor_drv_lock);

    return 0;
}   /* adopt_CAMERA_HW_Close() */

#ifdef CONFIG_MTK_CLKMGR
inline static int kdSetSensorMclk(int *pBuf)
{
/* #ifndef CONFIG_ARM64 */
    int ret = 0;
    ACDK_SENSOR_MCLK_STRUCT *pSensorCtrl = (ACDK_SENSOR_MCLK_STRUCT *)pBuf;

    PK_DBG("[CAMERA SENSOR] kdSetSensorMclk on=%d, freq= %d\n", pSensorCtrl->on, pSensorCtrl->freq);
    if (1 == pSensorCtrl->on) {
    enable_mux(MT_MUX_CAMTG, "CAMERA_SENSOR");
    clkmux_sel(MT_MUX_CAMTG, pSensorCtrl->freq, "CAMERA_SENSOR");
    }
    else {

    disable_mux(MT_MUX_CAMTG, "CAMERA_SENSOR");
    }
    return ret;
/* #endif */
}
#else

/*******************************************************************************
* Common Clock Framework (CCF)
********************************************************************************/
static inline void Get_ccf_clk(struct platform_device *pdev)
{
	int i = 0;

	if (pdev == NULL) {
		PK_ERR("[%s] pdev is null\n", __func__);
		return;
	}
	/* get all possible using clocks */
	for (i = 0; i < _MAX_MCLK_; i++)
		g_seninf_tg_mux[i] = devm_clk_get(&pdev->dev, _seninf_tg_mux_name[i]);

	for (i = 0; i < _MAX_TG_SRC_; i++)
		g_seninf_tg_mclk_src[i] = devm_clk_get(&pdev->dev, _seninf_tg_src_mclk_name[i]);


	/*g_camclk_cg_camtg = devm_clk_get(&pdev->dev, "CG_CAMTG");
	BUG_ON(IS_ERR(g_camclk_cg_camtg));*/
	return;
}

static inline void Check_ccf_clk(void)
{
	int i = 0;

	for (i = 0; i < _MAX_MCLK_; i++)
		WARN_ON(IS_ERR(g_seninf_tg_mux[i]));


	for (i = 0; i < _MAX_TG_SRC_; i++)
		WARN_ON(IS_ERR(g_seninf_tg_mclk_src[i]));

	return;
}

void ISP_MCLK1_EN(BOOL En)
{
}
void ISP_MCLK2_EN(BOOL En)
{
}
void ISP_MCLK3_EN(BOOL En)
{
}


static inline int kdSetSensorMclk(int *pBuf)
{
	int ret = 0;
#ifndef CONFIG_MTK_FPGA
	ACDK_SENSOR_MCLK_STRUCT *pSensorCtrl = (ACDK_SENSOR_MCLK_STRUCT *)pBuf;

	PK_DBG("[CAMERA SENSOR] CCF kdSetSensorMclk on=%d tg %d, freq= %d\n",
		pSensorCtrl->on, pSensorCtrl->TG, pSensorCtrl->freq);

	Check_ccf_clk();
	if (1 == pSensorCtrl->on) {
		if ((pSensorCtrl->TG < _MAX_MCLK_) && (pSensorCtrl->freq < _MAX_TG_SRC_)) {
			if (clk_prepare_enable(g_seninf_tg_mux[pSensorCtrl->TG]))
				PK_ERR("[CAMERA SENSOR] failed tg=%d, freq= %d\n",
				pSensorCtrl->TG, pSensorCtrl->freq);
			else
				atomic_inc(&g_seninf_tg_mux_enable_cnt[pSensorCtrl->TG]);

			if (clk_prepare_enable(g_seninf_tg_mclk_src[pSensorCtrl->freq]))
				PK_ERR("[CAMERA SENSOR] CCF kdSetSensorMclk tg=%d, freq= %d\n",
				pSensorCtrl->TG, pSensorCtrl->freq);
			else
				atomic_inc(&g_seninf_tg_mclk_src_enable_cnt[pSensorCtrl->freq]);
			ret = clk_set_parent(g_seninf_tg_mux[pSensorCtrl->TG], g_seninf_tg_mclk_src[pSensorCtrl->freq]);
		} else
			PK_ERR("[CAMERA SENSOR]kdSetSensorMclk out of range, tg=%d, freq= %d\n",
				pSensorCtrl->TG, pSensorCtrl->freq);
	} else {
		if ((pSensorCtrl->TG < _MAX_MCLK_) && (pSensorCtrl->freq < _MAX_TG_SRC_)) {
			clk_disable_unprepare(g_seninf_tg_mux[pSensorCtrl->TG]);
			atomic_dec(&g_seninf_tg_mux_enable_cnt[pSensorCtrl->TG]);
			clk_disable_unprepare(g_seninf_tg_mclk_src[pSensorCtrl->freq]);
			atomic_dec(&g_seninf_tg_mclk_src_enable_cnt[pSensorCtrl->freq]);
		}
	}
#endif
    return ret;

}

#endif
/*******************************************************************************
* GPIO
********************************************************************************/
inline static int kdSetSensorGpio(int *pBuf)
{
    int ret = 0;
#if 0// serial cam DVT
    unsigned int temp = 0;
    IMGSENSOR_GPIO_STRUCT *pSensorgpio = (IMGSENSOR_GPIO_STRUCT *)pBuf;

#define CAMERA_PIN_RDN0_A           GPIO172 | 0x80000000
#define CAMERA_PIN_RDP0_A           GPIO173 | 0x80000000
#define CAMERA_PIN_RDN1_A           GPIO174 | 0x80000000
#define CAMERA_PIN_RCN_A            GPIO176 | 0x80000000
#define CAMERA_PIN_RCP_A            GPIO177 | 0x80000000




#define CAMERA_PIN_CSI0_A_MODE_MIPI    GPIO_MODE_01
#define CAMERA_PIN_CSI0_A_MODE_SCAM    GPIO_MODE_02


    PK_DBG("[CAMERA SENSOR] kdSetSensorGpio enable=%d, type=%d\n",
    pSensorgpio->GpioEnable, pSensorgpio->SensroInterfaceType);
    /* Please use DCT to set correct GPIO setting (below message only for debug) */
    if (pSensorgpio->SensroInterfaceType == SENSORIF_SERIAL)
    {
    if (pSensorgpio->GpioEnable == 1)
    {
		mt_set_gpio_mode(CAMERA_PIN_RDN0_A,CAMERA_PIN_CSI0_A_MODE_SCAM);
		mt_set_gpio_mode(CAMERA_PIN_RDP0_A,CAMERA_PIN_CSI0_A_MODE_SCAM);
		mt_set_gpio_mode(CAMERA_PIN_RDN1_A,CAMERA_PIN_CSI0_A_MODE_SCAM);
		mt_set_gpio_mode(CAMERA_PIN_RCN_A,CAMERA_PIN_CSI0_A_MODE_SCAM);
		mt_set_gpio_mode(CAMERA_PIN_RCP_A,CAMERA_PIN_CSI0_A_MODE_SCAM);

    }
    else
    {
		mt_set_gpio_mode(CAMERA_PIN_RDN0_A,CAMERA_PIN_CSI0_A_MODE_SCAM);
		mt_set_gpio_mode(CAMERA_PIN_RDP0_A,CAMERA_PIN_CSI0_A_MODE_SCAM);
		mt_set_gpio_mode(CAMERA_PIN_RDN1_A,CAMERA_PIN_CSI0_A_MODE_SCAM);
		mt_set_gpio_mode(CAMERA_PIN_RCN_A,CAMERA_PIN_CSI0_A_MODE_SCAM);
		mt_set_gpio_mode(CAMERA_PIN_RCP_A,CAMERA_PIN_CSI0_A_MODE_SCAM);

    }
    }
#endif
    return ret;
}

/* PMIC */
#if !defined(CONFIG_MTK_LEGACY)
bool Get_Cam_Regulator(void)
{
	struct device_node *node = NULL, *kd_node;
	if (1) {
		/* check if customer camera node defined */
		node = of_find_compatible_node(NULL, NULL, "mediatek,camera_hw");

		if (node) {
			kd_node = sensor_device->of_node;
			sensor_device->of_node = node;
			/* name = of_get_property(node, "MAIN_CAMERA_POWER_A", NULL); */
			 regSubVCAMD = regulator_get(sensor_device, "vcamd_sub"); /*check customer definition*/
			if (regSubVCAMD == NULL) {
			    if (regVCAMA == NULL) {
				    regVCAMA = regulator_get(sensor_device, "vcama");
			    }
			    if (regVCAMD == NULL) {
				    regVCAMD = regulator_get(sensor_device, "vcamd");
			    }
			    if (regVCAMIO == NULL) {
				    regVCAMIO = regulator_get(sensor_device, "vcamio");
				}
			    if (regVCAMAF == NULL) {
				    regVCAMAF = regulator_get(sensor_device, "vcamaf");
			    }
			} else{
				PK_DBG("Camera customer regulator!\n");
				if (regVCAMA == NULL)
					regVCAMA = regulator_get(sensor_device, "vcama");
				if (regSubVCAMA == NULL)
					regSubVCAMA = regulator_get(sensor_device, "vcama_sub");
				if (regMain2VCAMA == NULL)
					regMain2VCAMA = regulator_get(sensor_device, "vcama_main2");
				if (regVCAMD == NULL)
					regVCAMD = regulator_get(sensor_device, "vcamd");
				if (regSubVCAMD == NULL)
					regSubVCAMD = regulator_get(sensor_device, "vcamd_sub");
				if (regMain2VCAMD == NULL)
					regMain2VCAMD = regulator_get(sensor_device, "vcamd_main2");
				if (regVCAMIO == NULL)
					regVCAMIO = regulator_get(sensor_device, "vcamio");
				if (regSubVCAMIO == NULL)
					regSubVCAMIO = regulator_get(sensor_device, "vcamio_sub");
				if (regMain2VCAMIO == NULL)
					regMain2VCAMIO = regulator_get(sensor_device, "vcamio_main2");
				if (regVCAMAF == NULL)
					regVCAMAF = regulator_get(sensor_device, "vcamaf");

			    /* restore original dev.of_node */
			}
			 sensor_device->of_node = kd_node;
		} else{
			PK_ERR("regulator get cust camera node failed!\n");
			return FALSE;
		}

		return TRUE;
	}
	return FALSE;
}


bool _hwPowerOn(PowerType type, int powerVolt)
{
    bool ret = FALSE;
    struct regulator *reg = NULL;

    PK_DBG("[_hwPowerOn]powertype:%d powerId:%d\n", type, powerVolt);
    if (type == AVDD) {
	reg = regVCAMA;
    } else if (type == DVDD) {
	reg = regVCAMD;
    } else if (type == DOVDD) {
	reg = regVCAMIO;
    } else if (type == AFVDD) {
	reg = regVCAMAF;
    }else if (type == SUB_AVDD) {
	reg = regSubVCAMA;
    } else if (type == SUB_DVDD) {
	reg = regSubVCAMD;
    } else if (type == SUB_DOVDD) {
	reg = regSubVCAMIO;
    } else if (type == MAIN2_AVDD) {
	reg = regMain2VCAMA;
    } else if (type == MAIN2_DVDD) {
	reg = regMain2VCAMD;
    } else if (type == MAIN2_DOVDD) {
	reg = regMain2VCAMIO;
    }else
    	return ret;

	if (!IS_ERR(reg)) {
		//if (type == SUB_DVDD) {
			//PK_DBG("pmic_get_register_value(PMIC_RG_VCAMD2_CAL) %d;\n", pmic_get_register_value(PMIC_RG_VCAMD2_CAL));
		//}
		if (regulator_set_voltage(reg , powerVolt, powerVolt) != 0) {
			PK_ERR("[_hwPowerOn]fail to regulator_set_voltage, powertype:%d powerId:%d\n", type, powerVolt);
			//return ret;
		}
		if (regulator_enable(reg) != 0) {
			PK_DBG("[_hwPowerOn]fail to regulator_enable, powertype:%d powerId:%d\n", type, powerVolt);
	    return ret;
	}
	ret = true;
    } else {
		PK_ERR("[_hwPowerOn]IS_ERR_OR_NULL powertype:%d reg %p\n", type,reg);
		return ret;
    }

	return ret;
}

bool _hwPowerDown(PowerType type)
{
    bool ret = FALSE;
	struct regulator *reg = NULL;
	PK_DBG("[_hwPowerDown]powertype:%d\n", type);

    if (type == AVDD) {
	reg = regVCAMA;
    } else if (type == DVDD) {
	reg = regVCAMD;
    } else if (type == DOVDD) {
	reg = regVCAMIO;
    } else if (type == AFVDD) {
	reg = regVCAMAF;
    }else if (type == SUB_AVDD) {
	reg = regSubVCAMA;
    } else if (type == SUB_DVDD) {
	reg = regSubVCAMD;
    } else if (type == SUB_DOVDD) {
	reg = regSubVCAMIO;
    } else if (type == MAIN2_AVDD) {
	reg = regMain2VCAMA;
    } else if (type == MAIN2_DVDD) {
	reg = regMain2VCAMD;
    } else if (type == MAIN2_DOVDD) {
	reg = regMain2VCAMIO;
    }else
    	return ret;

    if (!IS_ERR(reg)) {
	if (regulator_is_enabled(reg) != 0) {
			PK_DBG("[_hwPowerDown]%d is enabled\n", type);
	}
		if (regulator_disable(reg) != 0) {
			PK_DBG("[_hwPowerDown]fail to regulator_disable, powertype: %d\n\n", type);
			return ret;
		}
	ret = true;
    } else {
		PK_DBG("[_hwPowerDown]%d fail to power down  due to regVCAM == NULL\n", type);
		return ret;
    }
    return ret;
}


#endif

#ifdef CONFIG_COMPAT

static int compat_get_acdk_sensor_getinfo_struct(
        COMPAT_ACDK_SENSOR_GETINFO_STRUCT __user *data32,
        ACDK_SENSOR_GETINFO_STRUCT __user *data)
{
    compat_uint_t i;
    compat_uptr_t p;
    int err;

    err = get_user(i, &data32->ScenarioId[0]);
    err |= put_user(i, &data->ScenarioId[0]);
    err = get_user(i, &data32->ScenarioId[1]);
    err |= put_user(i, &data->ScenarioId[1]);
    err = get_user(p, &data32->pInfo[0]);
    err |= put_user(compat_ptr(p), &data->pInfo[0]);
    err = get_user(p, &data32->pInfo[1]);
    err |= put_user(compat_ptr(p), &data->pInfo[1]);
    err = get_user(p, &data32->pInfo[0]);
    err |= put_user(compat_ptr(p), &data->pConfig[0]);
    err = get_user(p, &data32->pInfo[1]);
    err |= put_user(compat_ptr(p), &data->pConfig[1]);

    return err;
}

static int compat_put_acdk_sensor_getinfo_struct(
        COMPAT_ACDK_SENSOR_GETINFO_STRUCT __user *data32,
        ACDK_SENSOR_GETINFO_STRUCT __user *data)
{
    compat_uint_t i;
    int err;

    err = get_user(i, &data->ScenarioId[0]);
    err |= put_user(i, &data32->ScenarioId[0]);
    err = get_user(i, &data->ScenarioId[1]);
    err |= put_user(i, &data32->ScenarioId[1]);
    return err;
}

static int compat_get_imagesensor_getinfo_struct(
        COMPAT_IMAGESENSOR_GETINFO_STRUCT __user *data32,
        IMAGESENSOR_GETINFO_STRUCT __user *data)
{
    compat_uptr_t p;
    compat_uint_t i;
    int err;

    err = get_user(i, &data32->SensorId);
    err |= put_user(i, &data->SensorId);
    err |= get_user(p, &data32->pInfo);
    err |= put_user(compat_ptr(p), &data->pInfo);
    err |= get_user(p, &data32->pSensorResolution);
    err |= put_user(compat_ptr(p), &data->pSensorResolution);
    return err;
}

static int compat_put_imagesensor_getinfo_struct(
        COMPAT_IMAGESENSOR_GETINFO_STRUCT __user *data32,
        IMAGESENSOR_GETINFO_STRUCT __user *data)
{
    /* compat_uptr_t p; */
    compat_uint_t i;
    int err;

    err = get_user(i, &data->SensorId);
    err |= put_user(i, &data32->SensorId);
    /* Assume pointer is not change */
#if 0
    err |= get_user(p, &data->pInfo);
    err |= put_user(p, &data32->pInfo);
    err |= get_user(p, &data->pSensorResolution);
    err |= put_user(p, &data32->pSensorResolution);  */
#endif
    return err;
}

static int compat_get_acdk_sensor_featurecontrol_struct(
        COMPAT_ACDK_SENSOR_FEATURECONTROL_STRUCT __user *data32,
        ACDK_SENSOR_FEATURECONTROL_STRUCT __user *data)
{
    compat_uptr_t p;
    compat_uint_t i;
    int err;

    err = get_user(i, &data32->InvokeCamera);
    err |= put_user(i, &data->InvokeCamera);
    err |= get_user(i, &data32->FeatureId);
    err |= put_user(i, &data->FeatureId);
    err |= get_user(p, &data32->pFeaturePara);
    err |= put_user(compat_ptr(p), &data->pFeaturePara);
    err |= get_user(p, &data32->pFeatureParaLen);
    err |= put_user(compat_ptr(p), &data->pFeatureParaLen);
    return err;
}

static int compat_put_acdk_sensor_featurecontrol_struct(
        COMPAT_ACDK_SENSOR_FEATURECONTROL_STRUCT __user *data32,
        ACDK_SENSOR_FEATURECONTROL_STRUCT __user *data)
{
    MUINT8 *p;
    MUINT32 *q;
    compat_uint_t i;
    int err;

    err = get_user(i, &data->InvokeCamera);
    err |= put_user(i, &data32->InvokeCamera);
    err |= get_user(i, &data->FeatureId);
    err |= put_user(i, &data32->FeatureId);
    /* Assume pointer is not change */

    err |= get_user(p, &data->pFeaturePara);
    err |= put_user(ptr_to_compat(p), &data32->pFeaturePara);
    err |= get_user(q, &data->pFeatureParaLen);
    err |= put_user(ptr_to_compat(q), &data32->pFeatureParaLen);

    return err;
}

static int compat_get_acdk_sensor_control_struct(
        COMPAT_ACDK_SENSOR_CONTROL_STRUCT __user *data32,
        ACDK_SENSOR_CONTROL_STRUCT __user *data)
{
    compat_uptr_t p;
    compat_uint_t i;
    int err;

    err = get_user(i, &data32->InvokeCamera);
    err |= put_user(i, &data->InvokeCamera);
    err |= get_user(i, &data32->ScenarioId);
    err |= put_user(i, &data->ScenarioId);
    err |= get_user(p, &data32->pImageWindow);
    err |= put_user(compat_ptr(p), &data->pImageWindow);
    err |= get_user(p, &data32->pSensorConfigData);
    err |= put_user(compat_ptr(p), &data->pSensorConfigData);
    return err;
}

static int compat_put_acdk_sensor_control_struct(
        COMPAT_ACDK_SENSOR_CONTROL_STRUCT __user *data32,
        ACDK_SENSOR_CONTROL_STRUCT __user *data)
{
    /* compat_uptr_t p; */
    compat_uint_t i;
    int err;

    err = get_user(i, &data->InvokeCamera);
    err |= put_user(i, &data32->InvokeCamera);
    err |= get_user(i, &data->ScenarioId);
    err |= put_user(i, &data32->ScenarioId);
    /* Assume pointer is not change */
#if 0
    err |= get_user(p, &data->pImageWindow);
    err |= put_user(p, &data32->pImageWindow);
    err |= get_user(p, &data->pSensorConfigData);
    err |= put_user(p, &data32->pSensorConfigData);
#endif
    return err;
}

static int compat_get_acdk_sensor_resolution_info_struct(
        COMPAT_ACDK_SENSOR_PRESOLUTION_STRUCT __user *data32,
        ACDK_SENSOR_PRESOLUTION_STRUCT __user *data)
{
    int err;
    compat_uptr_t p;
    err = get_user(p, &data32->pResolution[0]);
    err |= put_user(compat_ptr(p), &data->pResolution[0]);
    err = get_user(p, &data32->pResolution[1]);
    err |= put_user(compat_ptr(p), &data->pResolution[1]);

    /* err = copy_from_user((void*)data, (void*)data32, sizeof(compat_uptr_t) * 2); */
    /* err = copy_from_user((void*)data[0], (void*)data32[0], sizeof(ACDK_SENSOR_RESOLUTION_INFO_STRUCT)); */
    /* err = copy_from_user((void*)data[1], (void*)data32[1], sizeof(ACDK_SENSOR_RESOLUTION_INFO_STRUCT)); */
    return err;
}

static int compat_put_acdk_sensor_resolution_info_struct(
	COMPAT_ACDK_SENSOR_PRESOLUTION_STRUCT __user *data32,
	ACDK_SENSOR_PRESOLUTION_STRUCT __user *data)
{
	int err = 0;
    /* err = copy_to_user((void*)data, (void*)data32, sizeof(compat_uptr_t) * 2); */
    /* err = copy_to_user((void*)data[0], (void*)data32[0], sizeof(ACDK_SENSOR_RESOLUTION_INFO_STRUCT)); */
    /* err = copy_to_user((void*)data[1], (void*)data32[1], sizeof(ACDK_SENSOR_RESOLUTION_INFO_STRUCT)); */
    return err;
}



static long CAMERA_HW_Ioctl_Compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret;

    if (!filp->f_op || !filp->f_op->unlocked_ioctl)
    return -ENOTTY;

    switch (cmd) {
    case COMPAT_KDIMGSENSORIOC_X_GETINFO:
    {
    COMPAT_ACDK_SENSOR_GETINFO_STRUCT __user *data32;
    ACDK_SENSOR_GETINFO_STRUCT __user *data;
    int err;
    /*PK_DBG("[CAMERA SENSOR] CAOMPAT_KDIMGSENSORIOC_X_GETINFO E\n");*/

    data32 = compat_ptr(arg);
    data = compat_alloc_user_space(sizeof(*data));
    if (data == NULL)
        return -EFAULT;

    err = compat_get_acdk_sensor_getinfo_struct(data32, data);
    if (err)
        return err;

    ret = filp->f_op->unlocked_ioctl(filp, KDIMGSENSORIOC_X_GETINFO,(unsigned long)data);
    err = compat_put_acdk_sensor_getinfo_struct(data32, data);

    if(err != 0)
        PK_DBG("[CAMERA SENSOR] compat_put_acdk_sensor_getinfo_struct failed\n");
    return ret;
    }
    case COMPAT_KDIMGSENSORIOC_X_FEATURECONCTROL:
    {
    COMPAT_ACDK_SENSOR_FEATURECONTROL_STRUCT __user *data32;
    ACDK_SENSOR_FEATURECONTROL_STRUCT __user *data;
    int err;
    PK_DBG("[CAMERA SENSOR] CAOMPAT_KDIMGSENSORIOC_X_FEATURECONCTROL\n");

    data32 = compat_ptr(arg);
    data = compat_alloc_user_space(sizeof(*data));
    if (data == NULL)
        return -EFAULT;

    err = compat_get_acdk_sensor_featurecontrol_struct(data32, data);
    if (err)
        return err;

    ret = filp->f_op->unlocked_ioctl(filp, KDIMGSENSORIOC_X_FEATURECONCTROL, (unsigned long)data);
    err = compat_put_acdk_sensor_featurecontrol_struct(data32, data);


    if (err != 0)
        PK_ERR("[CAMERA SENSOR] compat_put_acdk_sensor_getinfo_struct failed\n");
    return ret;
    }
    case COMPAT_KDIMGSENSORIOC_X_CONTROL:
    {
    COMPAT_ACDK_SENSOR_CONTROL_STRUCT __user *data32;
    ACDK_SENSOR_CONTROL_STRUCT __user *data;
    int err;
    PK_DBG("[CAMERA SENSOR] CAOMPAT_KDIMGSENSORIOC_X_CONTROL\n");

    data32 = compat_ptr(arg);
    data = compat_alloc_user_space(sizeof(*data));
    if (data == NULL)
        return -EFAULT;

    err = compat_get_acdk_sensor_control_struct(data32, data);
    if (err)
        return err;
    ret = filp->f_op->unlocked_ioctl(filp, KDIMGSENSORIOC_X_CONTROL, (unsigned long)data);
    err = compat_put_acdk_sensor_control_struct(data32, data);

    if (err != 0)
        PK_ERR("[CAMERA SENSOR] compat_put_acdk_sensor_getinfo_struct failed\n");
    return ret;
    }
    case COMPAT_KDIMGSENSORIOC_X_GETINFO2:
    {
    COMPAT_IMAGESENSOR_GETINFO_STRUCT __user *data32;
    IMAGESENSOR_GETINFO_STRUCT __user *data;
    int err;
    PK_DBG("[CAMERA SENSOR] CAOMPAT_KDIMGSENSORIOC_X_GETINFO2\n");

    data32 = compat_ptr(arg);
    data = compat_alloc_user_space(sizeof(*data));
    if (data == NULL)
        return -EFAULT;

    err = compat_get_imagesensor_getinfo_struct(data32, data);
    if (err)
        return err;
    ret = filp->f_op->unlocked_ioctl(filp, KDIMGSENSORIOC_X_GETINFO2, (unsigned long)data);
    err = compat_put_imagesensor_getinfo_struct(data32, data);

    if (err != 0)
        PK_ERR("[CAMERA SENSOR] compat_put_acdk_sensor_getinfo_struct failed\n");
    return ret;
    }
    case COMPAT_KDIMGSENSORIOC_X_GETRESOLUTION2:
    {
    COMPAT_ACDK_SENSOR_PRESOLUTION_STRUCT __user *data32;
    ACDK_SENSOR_PRESOLUTION_STRUCT __user *data;
    int err;
    PK_DBG("[CAMERA SENSOR] KDIMGSENSORIOC_X_GETRESOLUTION2\n");

    data32 = compat_ptr(arg);
    data = compat_alloc_user_space(sizeof(data));
    if (data == NULL)
        return -EFAULT;
    PK_DBG("[CAMERA SENSOR] compat_get_acdk_sensor_resolution_info_struct\n");
    err = compat_get_acdk_sensor_resolution_info_struct(data32, data);
    if (err)
        return err;
    PK_DBG("[CAMERA SENSOR] unlocked_ioctl\n");
    ret = filp->f_op->unlocked_ioctl(filp, KDIMGSENSORIOC_X_GETRESOLUTION2, (unsigned long)data);

	err = compat_put_acdk_sensor_resolution_info_struct(data32, data);
    if (err != 0)
        PK_ERR("[CAMERA SENSOR] compat_get_Acdk_sensor_resolution_info_struct failed\n");
    return ret;
    }
    /* Data in the following commands is not required to be converted to kernel 64-bit & user 32-bit */
    case KDIMGSENSORIOC_T_OPEN:
    case KDIMGSENSORIOC_T_CLOSE:
    case KDIMGSENSORIOC_T_CHECK_IS_ALIVE:
    case KDIMGSENSORIOC_X_SET_DRIVER:
    case KDIMGSENSORIOC_X_GET_SOCKET_POS:
    case KDIMGSENSORIOC_X_SET_I2CBUS:
    case KDIMGSENSORIOC_X_RELEASE_I2C_TRIGGER_LOCK:
    case KDIMGSENSORIOC_X_SET_SHUTTER_GAIN_WAIT_DONE:
    case KDIMGSENSORIOC_X_SET_MCLK_PLL:
    case KDIMGSENSORIOC_X_SET_CURRENT_SENSOR:
    case KDIMGSENSORIOC_X_SET_GPIO:
    case KDIMGSENSORIOC_X_GET_ISP_CLK:
    case KDIMGSENSORIOC_X_GET_CSI_CLK:
    return filp->f_op->unlocked_ioctl(filp, cmd, arg);

    default:
    return -ENOIOCTLCMD;
    }
}


#endif

/*******************************************************************************
* CAMERA_HW_Ioctl
********************************************************************************/
static long CAMERA_HW_Ioctl(
    struct file *a_pstFile,
    unsigned int a_u4Command,
    unsigned long a_u4Param
)
{

    int i4RetValue = 0;
    void *pBuff = NULL;
    u32 *pIdx = NULL;

    mutex_lock(&kdCam_Mutex);


    if (_IOC_NONE == _IOC_DIR(a_u4Command)) {
    }
    else {
    pBuff = kmalloc(_IOC_SIZE(a_u4Command), GFP_KERNEL);

    if (NULL == pBuff) {
        PK_DBG("[CAMERA SENSOR] ioctl allocate mem failed\n");
        i4RetValue = -ENOMEM;
        goto CAMERA_HW_Ioctl_EXIT;
    }

    if (_IOC_WRITE & _IOC_DIR(a_u4Command)) {
        if (copy_from_user(pBuff , (void *) a_u4Param, _IOC_SIZE(a_u4Command))) {
        kfree(pBuff);
        PK_DBG("[CAMERA SENSOR] ioctl copy from user failed\n");
        i4RetValue =  -EFAULT;
        goto CAMERA_HW_Ioctl_EXIT;
        }
    }
    }

    pIdx = (u32 *)pBuff;
    switch (a_u4Command) {

#if 0
    case KDIMGSENSORIOC_X_POWER_ON:
        i4RetValue = kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) *pIdx, true, CAMERA_HW_DRVNAME);
        break;
    case KDIMGSENSORIOC_X_POWER_OFF:
        i4RetValue = kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) *pIdx, false, CAMERA_HW_DRVNAME);
        break;
#endif
    case KDIMGSENSORIOC_X_SET_DRIVER:
        i4RetValue = kdSetDriver((unsigned int *)pBuff);
        break;
    case KDIMGSENSORIOC_T_OPEN:
        i4RetValue = adopt_CAMERA_HW_Open();
        break;
    case KDIMGSENSORIOC_X_GETINFO:
        i4RetValue = adopt_CAMERA_HW_GetInfo(pBuff);
        break;
    case KDIMGSENSORIOC_X_GETRESOLUTION2:
        i4RetValue = adopt_CAMERA_HW_GetResolution(pBuff);
        break;
    case KDIMGSENSORIOC_X_GETINFO2:
        i4RetValue = adopt_CAMERA_HW_GetInfo2(pBuff);
        break;
    case KDIMGSENSORIOC_X_FEATURECONCTROL:
        i4RetValue = adopt_CAMERA_HW_FeatureControl(pBuff);
        break;
    case KDIMGSENSORIOC_X_CONTROL:
        i4RetValue = adopt_CAMERA_HW_Control(pBuff);
        break;
    case KDIMGSENSORIOC_T_CLOSE:
        i4RetValue = adopt_CAMERA_HW_Close();
        break;
    case KDIMGSENSORIOC_T_CHECK_IS_ALIVE:
        i4RetValue = adopt_CAMERA_HW_CheckIsAlive();
        break;
    case KDIMGSENSORIOC_X_GET_SOCKET_POS:
        i4RetValue = kdGetSocketPostion((unsigned int *)pBuff);
        break;
    case KDIMGSENSORIOC_X_SET_I2CBUS:
        /* i4RetValue = kdSetI2CBusNum(*pIdx); */
        break;
    case KDIMGSENSORIOC_X_RELEASE_I2C_TRIGGER_LOCK:
        /* i4RetValue = kdReleaseI2CTriggerLock(); */
        break;

    case KDIMGSENSORIOC_X_SET_SHUTTER_GAIN_WAIT_DONE:
        /*i4RetValue = kdSensorSetExpGainWaitDone((int *)pBuff);*/
        break;

    case KDIMGSENSORIOC_X_SET_CURRENT_SENSOR:
        i4RetValue = kdSetCurrentSensorIdx(*pIdx);
        break;

    case KDIMGSENSORIOC_X_SET_MCLK_PLL:
        i4RetValue = kdSetSensorMclk(pBuff);
        break;

    case KDIMGSENSORIOC_X_SET_GPIO:
        i4RetValue = kdSetSensorGpio(pBuff);
        break;

    case KDIMGSENSORIOC_X_GET_ISP_CLK:
#ifdef CONFIG_MTK_SMI_EXT
	PK_DBG("KDIMGSENSORIOC_X_GET_ISP_CLK current_mmsys_clk=%d\n", current_mmsys_clk);
	if (mmdvfs_get_stable_isp_clk() == MMSYS_CLK_HIGH)
		*(unsigned int *)pBuff = ISP_CLK_HIGH;
	else if (mmdvfs_get_stable_isp_clk() == MMSYS_CLK_MEDIUM)
		*(unsigned int *)pBuff = ISP_CLK_MEDIUM;
	else
		*(unsigned int *)pBuff = ISP_CLK_LOW;
#else
	*(unsigned int *)pBuff = ISP_CLK_HIGH;
#endif
	break;

    case KDIMGSENSORIOC_X_GET_CSI_CLK:
		*(unsigned int *) pBuff = mt_get_ckgen_freq(*(unsigned int *) pBuff);
		PK_DBG("f_fcamtg_ck = %d\n", mt_get_ckgen_freq(8));
		PK_DBG("f_fcamtg2_ck = %d\n", mt_get_ckgen_freq(41));
		PK_DBG("hf_fcam_ck = %d\n", mt_get_ckgen_freq(5));
		PK_DBG("f_fseninf_ck = %d\n", mt_get_ckgen_freq(35));

		PK_DBG("%s: AD_UNIV_192M_CK = %dHZ\r\n", __func__, mt_get_abist_freq(32));
		PK_DBG("%s: f52m_mfg_ck = %dHZ\r\n", __func__, mt_get_ckgen_freq(11));
		PK_DBG("%s: hd_faxi_ck = %dHZ\r\n", __func__, mt_get_ckgen_freq(1));
		PK_DBG("%s: AD_UNIVPLL_CK = %dHZ\r\n", __func__, mt_get_abist_freq(24));
	break;

    default:
        PK_DBG("No such command %d\n",a_u4Command);
        i4RetValue = -EPERM;
        break;

    }

    if (_IOC_READ & _IOC_DIR(a_u4Command)) {
    if (copy_to_user((void __user *) a_u4Param , pBuff , _IOC_SIZE(a_u4Command))) {
        kfree(pBuff);
        PK_DBG("[CAMERA SENSOR] ioctl copy to user failed\n");
        i4RetValue =  -EFAULT;
        goto CAMERA_HW_Ioctl_EXIT;
    }
    }

    kfree(pBuff);
CAMERA_HW_Ioctl_EXIT:
    mutex_unlock(&kdCam_Mutex);
    return i4RetValue;
}
#ifdef CONFIG_MTK_SMI_EXT
int mmsys_clk_change_cb(int ori_clk_mode, int new_clk_mode)
{
	PK_DBG("mmsys_clk_change_cb ori: %d, new: %d, current_mmsys_clk %d\n", ori_clk_mode, new_clk_mode, current_mmsys_clk);
	current_mmsys_clk = new_clk_mode;
	return 1;
}
#endif
/*******************************************************************************
*
********************************************************************************/
/*  */
/* below is for linux driver system call */
/* change prefix or suffix only */
/*  */

/*******************************************************************************
 * RegisterCAMERA_HWCharDrv
 * #define
 * Main jobs:
 * 1.check for device-specified errors, device not ready.
 * 2.Initialize the device if it is opened for the first time.
 * 3.Update f_op pointer.
 * 4.Fill data structures into private_data
 * CAM_RESET
********************************************************************************/
static int CAMERA_HW_Open(struct inode *a_pstInode, struct file *a_pstFile)
{
    /* reset once in multi-open */
    if (atomic_read(&g_CamDrvOpenCnt) == 0) {
	mipic_26m_en(1);
    /* default OFF state */
    /* MUST have */
    /* kdCISModulePowerOn(DUAL_CAMERA_MAIN_SENSOR,"",true,CAMERA_HW_DRVNAME1); */
    /* kdCISModulePowerOn(DUAL_CAMERA_SUB_SENSOR,"",true,CAMERA_HW_DRVNAME1); */

    /* kdCISModulePowerOn(DUAL_CAMERA_MAIN_SENSOR,"",false,CAMERA_HW_DRVNAME1); */
    /* kdCISModulePowerOn(DUAL_CAMERA_SUB_SENSOR,"",false,CAMERA_HW_DRVNAME1); */
#ifdef CONFIG_MTK_SMI_EXT
		mmdvfs_register_mmclk_switch_cb(mmsys_clk_change_cb, MMDVFS_CLIENT_ID_ISP);
#endif
    }
    /*  */
    atomic_inc(&g_CamDrvOpenCnt);
    return 0;
}

/*******************************************************************************
  * RegisterCAMERA_HWCharDrv
  * Main jobs:
  * 1.Deallocate anything that "open" allocated in private_data.
  * 2.Shut down the device on last close.
  * 3.Only called once on last time.
  * Q1 : Try release multiple times.
********************************************************************************/
static int CAMERA_HW_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	int i = 0;

    atomic_dec(&g_CamDrvOpenCnt);
	if (atomic_read(&g_CamDrvOpenCnt) == 0) {
#ifdef CONFIG_MTK_SMI_EXT
		current_mmsys_clk = MMSYS_CLK_MEDIUM;
#endif
		mipic_26m_en(0);

		for (i = 0; i < _MAX_MCLK_; i++) {
			PK_DBG("g_seninf_tg_mux_enable_cnt[%d]!! %d\n",
				i, atomic_read(&g_seninf_tg_mux_enable_cnt[i]));
			for (; atomic_read(&g_seninf_tg_mux_enable_cnt[i]) > 0; ) {
				clk_disable_unprepare(g_seninf_tg_mux[i]);
				atomic_dec(&g_seninf_tg_mux_enable_cnt[i]);
			}
		}
		for (i = 0; i < _MAX_TG_SRC_; i++) {
			PK_DBG("g_seninf_tg_mclk_src_enable_cnt[%d]!! %d\n",
				i, atomic_read(&g_seninf_tg_mclk_src_enable_cnt[i]));
			for (; atomic_read(&g_seninf_tg_mclk_src_enable_cnt[i]) > 0; ) {
				clk_disable_unprepare(g_seninf_tg_mclk_src[i]);
				atomic_dec(&g_seninf_tg_mclk_src_enable_cnt[i]);
			}
		}
	}
    return 0;
}

static const struct file_operations g_stCAMERA_HW_fops =
{
    .owner = THIS_MODULE,
    .open = CAMERA_HW_Open,
    .release = CAMERA_HW_Release,
    .unlocked_ioctl = CAMERA_HW_Ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = CAMERA_HW_Ioctl_Compat,
#endif

};

#define CAMERA_HW_DYNAMIC_ALLOCATE_DEVNO 1
/*******************************************************************************
* RegisterCAMERA_HWCharDrv
********************************************************************************/
inline static int RegisterCAMERA_HWCharDrv(void)
{

#if CAMERA_HW_DYNAMIC_ALLOCATE_DEVNO
    if (alloc_chrdev_region(&g_CAMERA_HWdevno, 0, 1, CAMERA_HW_DRVNAME1))
    {
    PK_DBG("[CAMERA SENSOR] Allocate device no failed\n");

    return -EAGAIN;
    }
#else
    if (register_chrdev_region(g_CAMERA_HWdevno , 1 , CAMERA_HW_DRVNAME1))
    {
    PK_DBG("[CAMERA SENSOR] Register device no failed\n");

    return -EAGAIN;
    }
#endif

    /* Allocate driver */
    g_pCAMERA_HW_CharDrv = cdev_alloc();

    if (NULL == g_pCAMERA_HW_CharDrv)
    {
    unregister_chrdev_region(g_CAMERA_HWdevno, 1);

    PK_DBG("[CAMERA SENSOR] Allocate mem for kobject failed\n");

    return -ENOMEM;
    }

    /* Attatch file operation. */
    cdev_init(g_pCAMERA_HW_CharDrv, &g_stCAMERA_HW_fops);

    g_pCAMERA_HW_CharDrv->owner = THIS_MODULE;

    /* Add to system */
    if (cdev_add(g_pCAMERA_HW_CharDrv, g_CAMERA_HWdevno, 1))
    {
    PK_DBG("[mt6516_IDP] Attatch file operation failed\n");

    unregister_chrdev_region(g_CAMERA_HWdevno, 1);

    return -EAGAIN;
    }

    sensor_class = class_create(THIS_MODULE, "sensordrv");
    if (IS_ERR(sensor_class)) {
    int ret = PTR_ERR(sensor_class);
    PK_DBG("Unable to create class, err = %d\n", ret);
    return ret;
    }
    sensor_device = device_create(sensor_class, NULL, g_CAMERA_HWdevno, NULL, CAMERA_HW_DRVNAME1);

    return 0;
}

/*******************************************************************************
* UnregisterCAMERA_HWCharDrv
********************************************************************************/
inline static void UnregisterCAMERA_HWCharDrv(void)
{
    /* Release char driver */
    cdev_del(g_pCAMERA_HW_CharDrv);

    unregister_chrdev_region(g_CAMERA_HWdevno, 1);

    device_destroy(sensor_class, g_CAMERA_HWdevno);
    class_destroy(sensor_class);
}
/*******************************************************************************
 * i2c relative start
********************************************************************************/
/*******************************************************************************
* CAMERA_HW_i2c_probe
********************************************************************************/
static int CAMERA_HW_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int i4RetValue = 0;
    PK_DBG("[CAMERA_HW] Attach I2C g_pstI2Cclient %p\n",client);

    /* get sensor i2c client */
    spin_lock(&kdsensor_drv_lock);
    g_pstI2Cclient = client;

    /* set I2C clock rate */
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient->timing = 100;/* 100k */
    g_pstI2Cclient->ext_flag &= ~I2C_POLLING_FLAG; /* No I2C polling busy waiting */
    #endif
    spin_unlock(&kdsensor_drv_lock);

    /* Register char driver */
    i4RetValue = RegisterCAMERA_HWCharDrv();

    if (i4RetValue) {
    PK_ERR("[CAMERA_HW] register char device failed!\n");
    return i4RetValue;
    }

    /* spin_lock_init(&g_CamHWLock); */
#if !defined(CONFIG_MTK_LEGACY)
	Get_Cam_Regulator();
#endif

    PK_DBG("[CAMERA_HW] Attached!!\n");
    return 0;
}


/*******************************************************************************
* CAMERA_HW_i2c_remove
********************************************************************************/
static int CAMERA_HW_i2c_remove(struct i2c_client *client)
{
    return 0;
}

#ifdef CONFIG_OF
    static const struct of_device_id CAMERA_HW_i2c_of_ids[] = {
    { .compatible = "mediatek,camera_main", },
    {}
    };
#endif

struct i2c_driver CAMERA_HW_i2c_driver = {
    .probe = CAMERA_HW_i2c_probe,
    .remove = CAMERA_HW_i2c_remove,
    .driver = {
    .name = CAMERA_HW_DRVNAME1,
    .owner = THIS_MODULE,

#ifdef CONFIG_OF
    .of_match_table = CAMERA_HW_i2c_of_ids,
#endif
    },
    .id_table = CAMERA_HW_i2c_id,
};


/*******************************************************************************
* i2c relative end
*****************************************************************************/



/*******************************************************************************
 * RegisterCAMERA_HWCharDrv
 * #define
 * Main jobs:
 * 1.check for device-specified errors, device not ready.
 * 2.Initialize the device if it is opened for the first time.
 * 3.Update f_op pointer.
 * 4.Fill data structures into private_data
 * CAM_RESET
********************************************************************************/
static int CAMERA_HW_Open2(struct inode *a_pstInode, struct file *a_pstFile)
{
    /*  */
     if (atomic_read(&g_CamDrvOpenCnt2) == 0) {
     /* kdCISModulePowerOn(DUAL_CAMERA_MAIN_2_SENSOR,"",true,CAMERA_HW_DRVNAME2); */

    /* kdCISModulePowerOn(DUAL_CAMERA_MAIN_2_SENSOR,"",false,CAMERA_HW_DRVNAME2); */
    }
    atomic_inc(&g_CamDrvOpenCnt2);
    return 0;
}

/*******************************************************************************
  * RegisterCAMERA_HWCharDrv
  * Main jobs:
  * 1.Deallocate anything that "open" allocated in private_data.
  * 2.Shut down the device on last close.
  * 3.Only called once on last time.
  * Q1 : Try release multiple times.
********************************************************************************/
static int CAMERA_HW_Release2(struct inode *a_pstInode, struct file *a_pstFile)
{
    atomic_dec(&g_CamDrvOpenCnt2);

    return 0;
}


static const struct file_operations g_stCAMERA_HW_fops0 =
{
    .owner = THIS_MODULE,
    .open = CAMERA_HW_Open2,
    .release = CAMERA_HW_Release2,
    .unlocked_ioctl = CAMERA_HW_Ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = CAMERA_HW_Ioctl_Compat,
#endif

};



/*******************************************************************************
* RegisterCAMERA_HWCharDrv
********************************************************************************/
inline static int RegisterCAMERA_HWCharDrv2(void)
{
    struct device *sensor_device = NULL;
    UINT32 major;

#if CAMERA_HW_DYNAMIC_ALLOCATE_DEVNO
    if (alloc_chrdev_region(&g_CAMERA_HWdevno2, 0, 1, CAMERA_HW_DRVNAME2))
    {
    PK_DBG("[CAMERA SENSOR] Allocate device no failed\n");

    return -EAGAIN;
    }
#else
    if (register_chrdev_region(g_CAMERA_HWdevno2 , 1 , CAMERA_HW_DRVNAME2))
    {
    PK_DBG("[CAMERA SENSOR] Register device no failed\n");

    return -EAGAIN;
    }
#endif

    major = MAJOR(g_CAMERA_HWdevno2);
    g_CAMERA_HWdevno2 = MKDEV(major, 0);

    /* Allocate driver */
    g_pCAMERA_HW_CharDrv2 = cdev_alloc();

    if (NULL == g_pCAMERA_HW_CharDrv2)
    {
    unregister_chrdev_region(g_CAMERA_HWdevno2, 1);

    PK_DBG("[CAMERA SENSOR] Allocate mem for kobject failed\n");

    return -ENOMEM;
    }

    /* Attatch file operation. */
    cdev_init(g_pCAMERA_HW_CharDrv2, &g_stCAMERA_HW_fops0);

    g_pCAMERA_HW_CharDrv2->owner = THIS_MODULE;

    /* Add to system */
    if (cdev_add(g_pCAMERA_HW_CharDrv2, g_CAMERA_HWdevno2, 1))
    {
    PK_DBG("[mt6516_IDP] Attatch file operation failed\n");

    unregister_chrdev_region(g_CAMERA_HWdevno2, 1);

    return -EAGAIN;
    }

    sensor2_class = class_create(THIS_MODULE, "sensordrv2");
    if (IS_ERR(sensor2_class)) {
    int ret = PTR_ERR(sensor2_class);
    PK_DBG("Unable to create class, err = %d\n", ret);
    return ret;
    }
    sensor_device = device_create(sensor2_class, NULL, g_CAMERA_HWdevno2, NULL, CAMERA_HW_DRVNAME2);

    return 0;
}

inline static void UnregisterCAMERA_HWCharDrv2(void)
{
    /* Release char driver */
    cdev_del(g_pCAMERA_HW_CharDrv2);

    unregister_chrdev_region(g_CAMERA_HWdevno2, 1);

    device_destroy(sensor2_class, g_CAMERA_HWdevno2);
    class_destroy(sensor2_class);
}


/*******************************************************************************
* CAMERA_HW_i2c_probe
********************************************************************************/
static int CAMERA_HW_i2c_probe2(struct i2c_client *client, const struct i2c_device_id *id)
{
    int i4RetValue = 0;
    PK_DBG("[CAMERA_HW] Attach I2C0\n");

    spin_lock(&kdsensor_drv_lock);

    /* get sensor i2c client */
    g_pstI2Cclient2 = client;

    /* set I2C clock rate */
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient2->timing = 100;/* 100k */
    g_pstI2Cclient2->ext_flag &= ~I2C_POLLING_FLAG; /* No I2C polling busy waiting */
    #endif
    spin_unlock(&kdsensor_drv_lock);

    /* Register char driver */
    i4RetValue = RegisterCAMERA_HWCharDrv2();

    if (i4RetValue) {
    PK_ERR("[CAMERA_HW] register char device failed!\n");
    return i4RetValue;
    }

    /* spin_lock_init(&g_CamHWLock); */

    PK_DBG("[CAMERA_HW] Attached!!\n");
    return 0;
}

/*******************************************************************************
* CAMERA_HW_i2c_remove
********************************************************************************/
static int CAMERA_HW_i2c_remove2(struct i2c_client *client)
{
    return 0;
}
#if HW_TRIGGER_I2C_SUPPORT
static int CAMERA_HW_i2c_probe3(struct i2c_client *client, const struct i2c_device_id *id)
{
    //int i4RetValue = 0;
    PK_DBG("[CAMERA_HW] Attach I2C for HW trriger g_pstI2Cclient3 %p\n",client);

    spin_lock(&kdsensor_drv_lock);

    /* get sensor i2c client */
    g_pstI2Cclient3 = client;

    /* set I2C clock rate */
    #ifdef CONFIG_MTK_I2C_EXTENSION
    g_pstI2Cclient3->timing = 100;/* 100k */
    g_pstI2Cclient3->ext_flag &= ~I2C_POLLING_FLAG; /* No I2C polling busy waiting */
    #endif
    spin_unlock(&kdsensor_drv_lock);
#if 0
    /* Register char driver */
    i4RetValue = RegisterCAMERA_HWCharDrv2();

    if (i4RetValue) {
    PK_ERR("[CAMERA_HW] register char device failed!\n");
    return i4RetValue;
    }
#endif
    /* spin_lock_init(&g_CamHWLock); */

    PK_ERR("[CAMERA_HW] Attached!!\n");
    return 0;
}
/*******************************************************************************
* CAMERA_HW_i2c_remove
********************************************************************************/
static int CAMERA_HW_i2c_remove3(struct i2c_client *client)
{
    return 0;
}
#endif

/*******************************************************************************
* I2C Driver structure
********************************************************************************/
#ifdef CONFIG_OF
    static const struct of_device_id CAMERA_HW2_i2c_driver_of_ids[] = {
	{ .compatible = "mediatek,camera_sub", },
	{}
    };
#endif

struct i2c_driver CAMERA_HW_i2c_driver2 = {
    .probe = CAMERA_HW_i2c_probe2,
    .remove = CAMERA_HW_i2c_remove2,
    .driver = {
    .name = CAMERA_HW_DRVNAME2,
    .owner = THIS_MODULE,
#ifdef CONFIG_OF
    .of_match_table = CAMERA_HW2_i2c_driver_of_ids,
#endif
    },
    .id_table = CAMERA_HW_i2c_id2,
};

#if HW_TRIGGER_I2C_SUPPORT
#ifdef CONFIG_OF
    static const struct of_device_id CAMERA_HW3_i2c_driver_of_ids[] = {
	{ .compatible = "mediatek,camera_main_hw", },
	{}
    };
#endif

struct i2c_driver CAMERA_HW_i2c_driver3 = {
    .probe = CAMERA_HW_i2c_probe3,
    .remove = CAMERA_HW_i2c_remove3,
    .driver = {
    .name = CAMERA_HW_DRVNAME3,
    .owner = THIS_MODULE,
#ifdef CONFIG_OF
    .of_match_table = CAMERA_HW3_i2c_driver_of_ids,
#endif
    },
	.id_table = CAMERA_HW_i2c_id3,
};
#endif

/*******************************************************************************
* CAMERA_HW_probe
********************************************************************************/
static int CAMERA_HW_probe(struct platform_device *pdev)
{
//#ifndef CONFIG_MTK_CLKMGR
#if !defined (CONFIG_MTK_CLKMGR) && !defined (CONFIG_FPGA_EARLY_PORTING)
		Get_ccf_clk(pdev);
#endif

#if !defined(CONFIG_MTK_LEGACY)/*GPIO Pin control*/
	mtkcam_gpio_init(pdev);
#endif

	pCamerahw_platform_device = pdev;
#if	HW_TRIGGER_I2C_SUPPORT
	i2c_add_driver(&CAMERA_HW_i2c_driver3);
#endif
    return i2c_add_driver(&CAMERA_HW_i2c_driver);
}

/*******************************************************************************
* CAMERA_HW_remove()
********************************************************************************/
static int CAMERA_HW_remove(struct platform_device *pdev)
{
#if	HW_TRIGGER_I2C_SUPPORT
		i2c_del_driver(&CAMERA_HW_i2c_driver3);
#endif

    i2c_del_driver(&CAMERA_HW_i2c_driver);
    return 0;
}

/*******************************************************************************
*CAMERA_HW_suspend()
********************************************************************************/
static int CAMERA_HW_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

/*******************************************************************************
  * CAMERA_HW_DumpReg_To_Proc()
  * Used to dump some critical sensor register
  ********************************************************************************/
static int CAMERA_HW_resume(struct platform_device *pdev)
{
    return 0;
}

/*******************************************************************************
* CAMERA_HW_remove
********************************************************************************/
static int CAMERA_HW_probe2(struct platform_device *pdev)
{
    return i2c_add_driver(&CAMERA_HW_i2c_driver2);
}

/*******************************************************************************
* CAMERA_HW_remove()
********************************************************************************/
static int CAMERA_HW_remove2(struct platform_device *pdev)
{
    i2c_del_driver(&CAMERA_HW_i2c_driver2);
    return 0;
}

static int CAMERA_HW_suspend2(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

/*******************************************************************************
  * CAMERA_HW_DumpReg_To_Proc()
  * Used to dump some critical sensor register
  ********************************************************************************/
static int CAMERA_HW_resume2(struct platform_device *pdev)
{
    return 0;
}

/*=======================================================================
  * platform driver
  *=======================================================================*/
/* It seems we don't need to use device tree to register device cause we just use i2C part */
/* You can refer to CAMERA_HW_probe & CAMERA_HW_i2c_probe */
#ifdef CONFIG_OF
static const struct of_device_id CAMERA_HW2_of_ids[] = {
    { .compatible = "mediatek,camera_hw2", },
    {}
};
#endif

static struct platform_driver g_stCAMERA_HW_Driver2 = {
    .probe      = CAMERA_HW_probe2,
    .remove     = CAMERA_HW_remove2,
    .suspend    = CAMERA_HW_suspend2,
    .resume     = CAMERA_HW_resume2,
    .driver     = {
		.name   = "image_sensor_bus2",
		.owner  = THIS_MODULE,
#ifdef CONFIG_OF
    .of_match_table = CAMERA_HW2_of_ids,
#endif

    }
};

/*******************************************************************************
* iWriteTriggerReg
********************************************************************************/
#if 0
int iWriteTriggerReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId)
{
    int  i4RetValue = 0;
    int u4Index = 0;
    u8 *puDataInBytes = (u8 *)&a_u4Data;
    int retry = 3;
    char puSendCmd[6] = {(char)(a_u2Addr >> 8) , (char)(a_u2Addr & 0xFF) , 0 , 0 , 0 , 0};



    SET_I2CBUS_FLAG(gI2CBusNum);

    if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
    spin_lock(&kdsensor_drv_lock);
    g_pstI2Cclient->addr = (i2cId >> 1);
    spin_unlock(&kdsensor_drv_lock);
    }
    else {
    spin_lock(&kdsensor_drv_lock);
    g_pstI2Cclient2->addr = (i2cId >> 1);
    spin_unlock(&kdsensor_drv_lock);
    }


    if (a_u4Bytes > 2) {
    PK_DBG("[CAMERA SENSOR] exceed 2 bytes\n");
    return -1;
    }

    if (a_u4Data >> (a_u4Bytes << 3)) {
    PK_DBG("[CAMERA SENSOR] warning!! some data is not sent!!\n");
    }

    for (u4Index = 0; u4Index < a_u4Bytes; u4Index += 1) {
    puSendCmd[(u4Index + 2)] = puDataInBytes[(a_u4Bytes - u4Index-1)];
    }

    do {
    if (gI2CBusNum == SUPPORT_I2C_BUS_NUM1) {
        i4RetValue = mt_i2c_master_send(g_pstI2Cclient, puSendCmd, (a_u4Bytes + 2), I2C_3DCAMERA_FLAG);
        if (i4RetValue < 0) {
        PK_DBG("[CAMERA SENSOR][ERROR]set i2c bus 1 master fail\n");
        CLEAN_I2CBUS_FLAG(gI2CBusNum);
        break;
        }
    }
    else {
        i4RetValue = mt_i2c_master_send(g_pstI2Cclient2, puSendCmd, (a_u4Bytes + 2), I2C_3DCAMERA_FLAG);
        if (i4RetValue < 0) {
        PK_DBG("[CAMERA SENSOR][ERROR]set i2c bus 0 master fail\n");
        CLEAN_I2CBUS_FLAG(gI2CBusNum);
        break;
        }
    }

    if (i4RetValue != (a_u4Bytes + 2)) {
        PK_DBG("[CAMERA SENSOR] I2C send failed addr = 0x%x, data = 0x%x !!\n", a_u2Addr, a_u4Data);
    }
    else {
        break;
    }
    uDELAY(50);
    } while ((retry--) > 0);

    return i4RetValue;
}
#endif
#if 0  /* linux-3.10 procfs API changed */
/*******************************************************************************
  * CAMERA_HW_Read_Main_Camera_Status()
  * Used to detect main camera status
  ********************************************************************************/
static int  CAMERA_HW_Read_Main_Camera_Status(char *page, char **start, off_t off,
                                               int count, int *eof, void *data)
{
    char *p = page;
    int len = 0;
    p += sprintf(page, "%d\n", g_SensorExistStatus[0]);

    PK_DBG("g_SensorExistStatus[0] = %d\n", g_SensorExistStatus[0]);
    *start = page + off;
    len = p - page;
    if (len > off)
    len -= off;
    else
    len = 0;
    return len < count ? len  : count;

}
/*******************************************************************************
  * CAMERA_HW_Read_Sub_Camera_Status()
  * Used to detect main camera status
  ********************************************************************************/
static int  CAMERA_HW_Read_Sub_Camera_Status(char *page, char **start, off_t off,
                                               int count, int *eof, void *data)
{
    char *p = page;
    int len = 0;
    p += sprintf(page, "%d\n", g_SensorExistStatus[1]);

    PK_DBG(" g_SensorExistStatus[1] = %d\n", g_SensorExistStatus[1]);
    *start = page + off;
    len = p - page;
    if (len > off)
    len -= off;
    else
    len = 0;
    return len < count ? len  : count;

}
/*******************************************************************************
  * CAMERA_HW_Read_3D_Camera_Status()
  * Used to detect main camera status
  ********************************************************************************/
static int  CAMERA_HW_Read_3D_Camera_Status(char *page, char **start, off_t off,
                                               int count, int *eof, void *data)
{
    char *p = page;
    int len = 0;
    p += sprintf(page, "%d\n", g_SensorExistStatus[2]);

    PK_DBG("g_SensorExistStatus[2] = %d\n", g_SensorExistStatus[2]);
    *start = page + off;
    len = p - page;
    if (len > off)
    len -= off;
    else
    len = 0;
    return len < count ? len  : count;

}
#endif


/*******************************************************************************
  * CAMERA_HW_DumpReg_To_Proc()
  * Used to dump some critical sensor register
  ********************************************************************************/
static ssize_t  CAMERA_HW_DumpReg_To_Proc(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    return 0;
}
static ssize_t  CAMERA_HW_DumpReg_To_Proc2(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    return 0;
}

static ssize_t  CAMERA_HW_DumpReg_To_Proc3(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    return 0;
}

/*******************************************************************************
  * CAMERA_HW_Reg_Debug()
  * Used for sensor register read/write by proc file
  ********************************************************************************/
static ssize_t  CAMERA_HW_Reg_Debug(struct file *file, const char *buffer, size_t count,
                         loff_t *data)
{
    char regBuf[64] = {'\0'};
    u32 u4CopyBufSize = (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

    MSDK_SENSOR_REG_INFO_STRUCT sensorReg;
    memset(&sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

    if (copy_from_user(regBuf, buffer, u4CopyBufSize))
    return -EFAULT;

    if (sscanf(regBuf, "%x %x",  &sensorReg.RegAddr, &sensorReg.RegData) == 2) {
    if (g_pSensorFunc != NULL) {
        g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_MAIN_SENSOR, SENSOR_FEATURE_SET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
        g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_MAIN_SENSOR, SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
        PK_DBG("write addr = 0x%08x, data = 0x%08x\n", sensorReg.RegAddr, sensorReg.RegData);
    }
    }
    else if (sscanf(regBuf, "%x", &sensorReg.RegAddr) == 1) {
    if (g_pSensorFunc != NULL) {
        g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_MAIN_SENSOR, SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
        PK_DBG("read addr = 0x%08x, data = 0x%08x\n", sensorReg.RegAddr, sensorReg.RegData);
    }
    }

    return count;
}


static ssize_t  CAMERA_HW_Reg_Debug2(struct file *file, const char *buffer, size_t count,
                                     loff_t *data)
{
    char regBuf[64] = {'\0'};
    u32 u4CopyBufSize = (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

    MSDK_SENSOR_REG_INFO_STRUCT sensorReg;
    memset(&sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

    if (copy_from_user(regBuf, buffer, u4CopyBufSize))
    return -EFAULT;

    if (sscanf(regBuf, "%x %x",  &sensorReg.RegAddr, &sensorReg.RegData) == 2) {
    if (g_pSensorFunc != NULL) {
        g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_MAIN_2_SENSOR, SENSOR_FEATURE_SET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
        g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_MAIN_2_SENSOR, SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
        PK_DBG("write addr = 0x%08x, data = 0x%08x\n", sensorReg.RegAddr, sensorReg.RegData);
    }
    }
    else if (sscanf(regBuf, "%x", &sensorReg.RegAddr) == 1) {
    if (g_pSensorFunc != NULL) {
        g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_MAIN_2_SENSOR, SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
        PK_DBG("read addr = 0x%08x, data = 0x%08x\n", sensorReg.RegAddr, sensorReg.RegData);
    }
    }

    return count;
}

static ssize_t  CAMERA_HW_Reg_Debug3(struct file *file, const char *buffer, size_t count,
                                     loff_t *data)
{
    char regBuf[64] = {'\0'};
    u32 u4CopyBufSize = (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

    MSDK_SENSOR_REG_INFO_STRUCT sensorReg;
    memset(&sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

    if (copy_from_user(regBuf, buffer, u4CopyBufSize))
    return -EFAULT;

    if (sscanf(regBuf, "%x %x",  &sensorReg.RegAddr, &sensorReg.RegData) == 2) {
    if (g_pSensorFunc != NULL) {
        g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_SUB_SENSOR, SENSOR_FEATURE_SET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
        g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_SUB_SENSOR, SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
        PK_DBG("write addr = 0x%08x, data = 0x%08x\n", sensorReg.RegAddr, sensorReg.RegData);
    }
    }
    else if (sscanf(regBuf, "%x", &sensorReg.RegAddr) == 1) {
    if (g_pSensorFunc != NULL) {
        g_pSensorFunc->SensorFeatureControl(DUAL_CAMERA_SUB_SENSOR, SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
        PK_DBG("read addr = 0x%08x, data = 0x%08x\n", sensorReg.RegAddr, sensorReg.RegData);
    }
    }

    return count;
}

//-----
static int pdaf_type_info_read(struct seq_file *m, void *v)
{
    #define bufsz 512

    unsigned int len = bufsz;
    char pdaf_type_info[bufsz];

	memset(pdaf_type_info, 0, 512);

	if(g_pInvokeSensorFunc[0] == NULL)
    {
        return 0;
    }

	g_pInvokeSensorFunc[0]->SensorFeatureControl(SENSOR_FEATURE_GET_PDAF_TYPE, pdaf_type_info, &len);
	seq_printf(m, "%s\n", pdaf_type_info);
    return 0;
};

static int proc_SensorType_open(struct inode *inode, struct file *file)
{
    return single_open(file, pdaf_type_info_read, NULL);
};


static ssize_t  proc_SensorType_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
    char regBuf[64] = {'\0'};
    u32 u4CopyBufSize = (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

    if( copy_from_user(regBuf, buffer, u4CopyBufSize))
    return -EFAULT;

	if( g_pInvokeSensorFunc[0])
		g_pInvokeSensorFunc[0]->SensorFeatureControl(SENSOR_FEATURE_SET_PDAF_TYPE, regBuf, &u4CopyBufSize);

    return count;
};
/*=======================================================================
  * platform driver
  *=======================================================================*/

/* It seems we don't need to use device tree to register device cause we just use i2C part */
/* You can refer to CAMERA_HW_probe & CAMERA_HW_i2c_probe */
#ifdef CONFIG_OF
static const struct of_device_id CAMERA_HW_of_ids[] = {
	{ .compatible = "mediatek,camera_hw", },
    {}
};
#endif

static struct platform_driver g_stCAMERA_HW_Driver = {
    .probe      = CAMERA_HW_probe,
    .remove     = CAMERA_HW_remove,
    .suspend    = CAMERA_HW_suspend,
    .resume     = CAMERA_HW_resume,
    .driver     = {
    .name   = "image_sensor",
    .owner  = THIS_MODULE,
#ifdef CONFIG_OF
    .of_match_table = CAMERA_HW_of_ids,
#endif
    }
};

#ifndef CONFIG_OF
static struct platform_device camerahw2_platform_device = {
    .name = "image_sensor_bus2",
    .id = 0,
    .dev = {
    }
};
#endif


static  struct file_operations fcamera_proc_fops = {
    .read = CAMERA_HW_DumpReg_To_Proc,
    .write = CAMERA_HW_Reg_Debug
};
static  struct file_operations fcamera_proc_fops2 = {
    .read = CAMERA_HW_DumpReg_To_Proc2,
    .write = CAMERA_HW_Reg_Debug2
};
static  struct file_operations fcamera_proc_fops3 = {
    .read = CAMERA_HW_DumpReg_To_Proc3,
    .write = CAMERA_HW_Reg_Debug3
};
static  struct file_operations fcamera_proc_fops_set_pdaf_type = {
    .owner = THIS_MODULE,
    .open  = proc_SensorType_open,
    .read = seq_read,
    .write = proc_SensorType_write
};

/* Camera information */
static int subsys_camera_info_read(struct seq_file *m, void *v)
{
   PK_ERR("subsys_camera_info_read %s\n",mtk_ccm_name);
   seq_printf(m, "%s\n",mtk_ccm_name);
   return 0;
};

static int proc_camera_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, subsys_camera_info_read, NULL);
};

static  struct file_operations fcamera_proc_fops1 = {
    .owner = THIS_MODULE,
    .open  = proc_camera_info_open,
    .read  = seq_read,
};

/*=======================================================================
  * CAMERA_HW_i2C_init()
  *=======================================================================*/
static int __init CAMERA_HW_i2C_init(void)
{
	int i;
#if 0
    struct proc_dir_entry *prEntry;
#endif
#if defined(CONFIG_MTK_LEGACY)
    /* i2c_register_board_info(CAMERA_I2C_BUSNUM, &kd_camera_dev, 1); */
    if(SUPPORT_I2C_BUS_NUM1 != SUPPORT_I2C_BUS_NUM2)  //Main2 Support
    {
        i2c_register_board_info(SUPPORT_I2C_BUS_NUM1, &i2c_devs1, 1);
        i2c_register_board_info(SUPPORT_I2C_BUS_NUM2, &i2c_devs2, 1);
    }
    else
    {
        i2c_register_board_info(SUPPORT_I2C_BUS_NUM1, &i2c_devs1, 1);
        i2c_register_board_info(SUPPORT_I2C_BUS_NUM3, &i2c_devs2, 1);
    }
#endif
    PK_DBG("[camerahw_probe] start\n");

#ifndef CONFIG_OF
	int ret = 0;
    ret = platform_device_register(&camerahw_platform_device);
    if (ret) {
    PK_ERR("[camerahw_probe] platform_device_register fail\n");
    return ret;
    }

    ret = platform_device_register(&camerahw2_platform_device);
    if (ret) {
    PK_ERR("[camerahw2_probe] platform_device_register fail\n");
    return ret;
    }
#endif

    if (platform_driver_register(&g_stCAMERA_HW_Driver)) {
    PK_ERR("failed to register CAMERA_HW driver\n");
    return -ENODEV;
    }
    if (platform_driver_register(&g_stCAMERA_HW_Driver2)) {
    PK_ERR("failed to register CAMERA_HW driver\n");
    return -ENODEV;
    }
/* FIX-ME: linux-3.10 procfs API changed */
#if 1
    proc_create("driver/camsensor", 0, NULL, &fcamera_proc_fops);
    proc_create("driver/camsensor2", 0, NULL, &fcamera_proc_fops2);
    proc_create("driver/camsensor3", 0, NULL, &fcamera_proc_fops3);
    proc_create("driver/pdaf_type", 0, NULL, &fcamera_proc_fops_set_pdaf_type);

    /* Camera information */
    memset(mtk_ccm_name,0,camera_info_size);
    proc_create(PROC_CAMERA_INFO, 0, NULL, &fcamera_proc_fops1);

#else
    /* Register proc file for main sensor register debug */
    prEntry = create_proc_entry("driver/camsensor", 0, NULL);
    if (prEntry) {
    prEntry->read_proc = CAMERA_HW_DumpReg_To_Proc;
    prEntry->write_proc = CAMERA_HW_Reg_Debug;
    }
    else {
    PK_ERR("add /proc/driver/camsensor entry fail\n");
    }

    /* Register proc file for main_2 sensor register debug */
    prEntry = create_proc_entry("driver/camsensor2", 0, NULL);
    if (prEntry) {
    prEntry->read_proc = CAMERA_HW_DumpReg_To_Proc;
    prEntry->write_proc = CAMERA_HW_Reg_Debug2;
    }
    else {
    PK_ERR("add /proc/driver/camsensor2 entry fail\n");
    }

    /* Register proc file for sub sensor register debug */
    prEntry = create_proc_entry("driver/camsensor3", 0, NULL);
    if (prEntry) {
        prEntry->read_proc = CAMERA_HW_DumpReg_To_Proc;
        prEntry->write_proc = CAMERA_HW_Reg_Debug3;
    }
    else {
        PK_ERR("add /proc/driver/camsensor entry fail\n");
    }

    /* Register proc file for main sensor register debug */
    prEntry = create_proc_entry("driver/maincam_status", 0, NULL);
    if (prEntry) {
    prEntry->read_proc = CAMERA_HW_Read_Main_Camera_Status;
    prEntry->write_proc = NULL;
    }
    else {
    PK_ERR("add /proc/driver/maincam_status entry fail\n");
    }

    /* Register proc file for sub sensor register debug */
    prEntry = create_proc_entry("driver/subcam_status", 0, NULL);
    if (prEntry) {
    prEntry->read_proc = CAMERA_HW_Read_Sub_Camera_Status;
    prEntry->write_proc = NULL;
    }
    else {
    PK_ERR("add /proc/driver/subcam_status entry fail\n");
    }

    /* Register proc file for 3d sensor register debug */
    prEntry = create_proc_entry("driver/3dcam_status", 0, NULL);
    if (prEntry) {
    prEntry->read_proc = CAMERA_HW_Read_3D_Camera_Status;
    prEntry->write_proc = NULL;
    }
    else {
    PK_ERR("add /proc/driver/3dcam_status entry fail\n");
    }

#endif
    atomic_set(&g_CamHWOpend, 0);
    atomic_set(&g_CamHWOpend2, 0);
    atomic_set(&g_CamDrvOpenCnt, 0);
    atomic_set(&g_CamDrvOpenCnt2, 0);
    atomic_set(&g_CamHWOpening, 0);

	/* Init gSensorTempState */
	for (i = 0; i < DUAL_CAMERA_SENSOR_MAX; i++)
		gSensorTempState[i] = TEMPERATURE_INIT_STATE;

#ifdef CONFIG_CAM_TEMPERATURE_WORKQUEUE
	memset((void *)&cam_temperature_wq, 0, sizeof(cam_temperature_wq));
	INIT_DELAYED_WORK(&cam_temperature_wq, cam_temperature_report_wq_routine);
    schedule_delayed_work(&cam_temperature_wq, HZ);
#endif
    return 0;
}

/*=======================================================================
  * CAMERA_HW_i2C_exit()
  *=======================================================================*/
static void __exit CAMERA_HW_i2C_exit(void)
{
    platform_driver_unregister(&g_stCAMERA_HW_Driver);
    platform_driver_unregister(&g_stCAMERA_HW_Driver2);
}


EXPORT_SYMBOL(kdSetSensorSyncFlag);
EXPORT_SYMBOL(kdSensorSyncFunctionPtr);
EXPORT_SYMBOL(kdGetRawGainInfoPtr);

module_init(CAMERA_HW_i2C_init);
module_exit(CAMERA_HW_i2C_exit);

MODULE_DESCRIPTION("CAMERA_HW driver");
MODULE_AUTHOR("MM");
MODULE_LICENSE("GPL");





