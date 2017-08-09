#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "kd_imgsensor.h"

/*Common EEPRom Driver*/
#include "common/BRCB032GWZ_3/BRCB032GWZ_3.h"
#include "common/cat24c16/cat24c16.h"
#include "common/GT24c32a/GT24c32a.h"


#define CAM_CAL_DEBUG
#ifdef CAM_CAL_DEBUG
/*#include <linux/log.h>*/
#include <linux/kern_levels.h>
#define PFX "cam_cal_list"

#define CAM_CALINF(format, args...)     pr_info(PFX "[%s] " format, __func__, ##args)
#define CAM_CALDB(format, args...)      pr_debug(PFX "[%s] " format, __func__, ##args)
#define CAM_CALERR(format, args...)     pr_info(format, ##args)
#else
#define CAM_CALINF(x, ...)
#define CAM_CALDB(x, ...)
#define CAM_CALERR(x, ...)
#endif


#define MTK_MAX_CID_NUM 1
unsigned int mtkCidList[MTK_MAX_CID_NUM] = {
	0x010b00ff
};

enum {
	AUTO_SEARCH = 0,
	/* #if defined(BRCB032GWZ_3) */
	BRCB032GWZ_3,
	/* #endif */
	/* #if defined(cat24c16) */
	CAT24C16,
	/* #endif */
	/* #if defined(GT24c32a) */
	GT24C32A,
	/* #endif */
	NUM_COUNT,
} CAM_CAL_CMD_TYPE;


stCAM_CAL_FUNC_STRUCT g_camCalCMDFunc[] = {
	/*#if defined(BRCB032GWZ_3)*/
	{BRCB032GWZ_3, brcb032gwz_selective_read_region},
	/*#endif*/
	/*#if defined(cat24c16)*/
	{CAT24C16, cat24c16_selective_read_region},
	/*#endif*/
	/*#if defined(GT24c32a)*/
	{GT24C32A, gt24c32a_selective_read_region},
	/*#endif*/

	/*      ADD before this line */
	{0, 0} /*end of list*/
};

stCAM_CAL_LIST_STRUCT g_camCalList[] = {
	{OV23850_SENSOR_ID, 0xA0, AUTO_SEARCH, cam_cal_check_mtk_cid},
	{OV23850_SENSOR_ID, 0xA8, AUTO_SEARCH, cam_cal_check_mtk_cid},
	{S5K3M2_SENSOR_ID, 0xA0, AUTO_SEARCH, cam_cal_check_mtk_cid},
	{IMX214_SENSOR_ID, 0xA0, AUTO_SEARCH, cam_cal_check_mtk_cid},
	{S5K2X8_SENSOR_ID, 0xA0, AUTO_SEARCH, cam_cal_check_mtk_cid},
	{IMX258_SENSOR_ID, 0xA0, AUTO_SEARCH, cam_cal_check_mtk_cid},
	{IMX377_SENSOR_ID, 0xA0, AUTO_SEARCH, cam_cal_check_mtk_cid},

	/*  ADD before this line */
	{0, 0, 0, 0} /*end of list*/
};


unsigned int cam_cal_get_sensor_list(stCAM_CAL_LIST_STRUCT **ppCamcalList)

{
	if (NULL == ppCamcalList)
		return 1;

	*ppCamcalList = &g_camCalList[0];
	return 0;
}


unsigned int cam_cal_get_func_list(stCAM_CAL_FUNC_STRUCT **ppCamcalFuncList)
{
	if (NULL == ppCamcalFuncList)
		return 1;

	*ppCamcalFuncList = &g_camCalCMDFunc[0];
	return 0;
}

unsigned int cam_cal_check_mtk_cid(struct i2c_client *client, unsigned int cmdIndx)
{
	unsigned int calibrationID = 0, ret = 0;
	int j = 0;

	CAM_CALDB("start cmdIndx=%d!\n", cmdIndx);
	if (g_camCalCMDFunc[cmdIndx].readCamCalData != NULL) {
		CAM_CALDB("cam_cal_cmd_func[%d].readCamCalData != NULL !\n", cmdIndx);
		g_camCalCMDFunc[cmdIndx].readCamCalData(client, 1, (unsigned char *)&calibrationID, 4);
		CAM_CALDB("calibrationID=%x\n", calibrationID);
	}

	if (calibrationID != 0)
		for (j = 0; j < MTK_MAX_CID_NUM; j++)
			if (mtkCidList[j] == calibrationID) {
				ret = 1;
				break;
			}

	CAM_CALDB("ret=%d\n", ret);
	return ret;
}


