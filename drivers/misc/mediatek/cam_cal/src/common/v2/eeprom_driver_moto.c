#define PFX "CAM_CAL_OTP"

#include "eeprom_driver_moto.h"

struct stCAM_CAL_DATAINFO_STRUCT *g_eepromMainData = NULL;
struct stCAM_CAL_DATAINFO_STRUCT *g_eepromSubData = NULL;
struct stCAM_CAL_DATAINFO_STRUCT *g_eepromMainMicroData = NULL;

extern unsigned int read_region_fun(struct EEPROM_DRV_FD_DATA *pdata,unsigned char *buf,unsigned int offset, unsigned int size);

#if MAIN_OTP_DUMP
void dumpEEPROMData(int u4Length,u8* pu1Params)
{
	int i = 0;
	for(i = 0; i < u4Length; i += 16){
		if(u4Length - i  >= 16){
			LOG_INF("eeprom[%d-%d]:0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x ",
			i,i+15,pu1Params[i],pu1Params[i+1],pu1Params[i+2],pu1Params[i+3],pu1Params[i+4],pu1Params[i+5],pu1Params[i+6]
			,pu1Params[i+7],pu1Params[i+8],pu1Params[i+9],pu1Params[i+10],pu1Params[i+11],pu1Params[i+12],pu1Params[i+13],pu1Params[i+14]
			,pu1Params[i+15]);
		}else{
			int j = i;
			for(;j < u4Length;j++)
			LOG_INF("eeprom[%d] = 0x%2x ",j,pu1Params[j]);
		}
	}
	LOG_INF("\n");
}
#endif

int imgSensorCheckEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData, struct stCAM_CAL_CHECKSUM_STRUCT* cData){
	int i = 0;
	int length = 0;
	int count;
	u32 sum = 0;

	if((pData != NULL)&&(pData->dataBuffer != NULL)&&(cData != NULL)){
		u8* buffer = pData->dataBuffer;
		//verity validflag and checksum
		for((count = 0);count < MAX_ITEM;count++){
			if(cData[count].item < MAX_ITEM) {
				if(buffer[cData[count].flagAdrees]!= cData[count].validFlag){
					LOG_ERR("invalid otp data cItem=%d,flag=%d failed\n", cData[count].item,buffer[cData[count].flagAdrees]);
					return -ENODEV;
				} else {
					LOG_INF("check cTtem=%d,flag=%d otp flag data successful!\n", cData[count].item,buffer[cData[count].flagAdrees]);
				}
				sum = 0;
				length = cData[count].endAdress - cData[count].startAdress;
				for(i = 0;i <= length;i++){
					sum += buffer[cData[count].startAdress+i];
				}
				if(((sum%0xff)+1)!= buffer[cData[count].checksumAdress]){
					LOG_ERR("checksum cItem=%d,0x%x,length = 0x%x failed\n",cData[count].item,sum,length);
					return -ENODEV;
				} else {
					LOG_INF("checksum cItem=%d,0x%x,length = 0x%x successful!\n",cData[count].item,sum,length);
				}
			} else {
				break;
			}
		}
	} else {
		LOG_ERR("some data not inited!\n");
		return -ENODEV;
	}

	LOG_INF("sensor[0x%x][0x%x] eeprom checksum success\n", pData->sensorID, pData->deviceID);

	return 0;
}

int imgSensorReadEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData, struct stCAM_CAL_CHECKSUM_STRUCT* checkData){
	//struct EEPROM_DRV *pinst;
	struct EEPROM_DRV_FD_DATA *fd_pdata = NULL;
	int i4RetValue = -1;
	u32 vendorID = 0;
	u8 tmpBuf[4] = {0};
	struct file *f = NULL;
	unsigned int index = 0;
	char device_drv_name[DEV_NAME_STR_LEN_MAX] = { 0 };

	if((pData == NULL)||(checkData == NULL)){
		LOG_ERR("pData or checkData not inited!\n");
		return -EFAULT;
	}

	LOG_INF("SensorID=%x DeviceID=%x\n",pData->sensorID, pData->deviceID);
	index = IMGSENSOR_SENSOR_IDX_MAP(pData->deviceID);
	if (index >= MAX_EEPROM_NUMBER) {
			LOG_ERR("node index out of bound\n");
			return -EINVAL;
	}

	i4RetValue = snprintf(device_drv_name, DEV_NAME_STR_LEN_MAX - 1,
		DEV_NAME_FMT_DEV, index);
	LOG_INF("device_drv_name=%s",device_drv_name);
	if (i4RetValue < 0) {
			LOG_ERR(
			"[eeprom]%s error, ret = %d", __func__, i4RetValue);
			return -EFAULT;
	}

	//get i2c client
	/*index = IMGSENSOR_SENSOR_IDX_MAP(pData->deviceID);
	LOG_INF("zyk index=%d",index);
	pinst = &ginst_drv[index];*/

	//1st open file
	if (f == NULL){
		f = filp_open(device_drv_name, O_RDWR, 0);
	}
	if (IS_ERR(f)){
		LOG_ERR("fail to open %s\n", device_drv_name);
		return -EFAULT;
	}

	fd_pdata = (struct EEPROM_DRV_FD_DATA *) f->private_data;
	if(NULL == fd_pdata){
		LOG_ERR("fp_pdata is null %s\n");
		filp_close(f,NULL);
		return -EFAULT;
	}
	fd_pdata->sensor_info.sensor_id = pData->sensorID;
	//2nd verity vendorID
	//u8 *kbuf = kmalloc(1, GFP_KERNEL);
	LOG_INF("zyk read vendorId!\n");
	//f->f_pos = 1;
	i4RetValue = read_region_fun(fd_pdata, &tmpBuf[0], 1, 1);
	if (i4RetValue != 1) {
		LOG_ERR("vendorID read failed 0x%x != 0x%x,i4RetValue=%d\n",tmpBuf[0], pData->sensorVendorid >> 24,i4RetValue);
		filp_close(f,NULL);
		return -EFAULT;
	}
	vendorID = tmpBuf[0];
	if(vendorID != pData->sensorVendorid >> 24){
		LOG_ERR("vendorID cmp failed 0x%x != 0x%x\n",vendorID, pData->sensorVendorid >> 24);
		filp_close(f,NULL);
		return -EFAULT;
	}

	//3rd get eeprom data
	//f->f_pos = 0;
	if (pData->dataBuffer == NULL){
		pData->dataBuffer = kmalloc(pData->dataLength, GFP_KERNEL);
		if (pData->dataBuffer == NULL) {
			LOG_ERR("pData->dataBuffer is malloc fail\n");
			filp_close(f,NULL);
			return -EFAULT;
		}
	}
	i4RetValue = read_region_fun(fd_pdata, pData->dataBuffer, 0x0, pData->dataLength);
	if (i4RetValue != pData->dataLength) {
		kfree(pData->dataBuffer);
		pData->dataBuffer = NULL;
		LOG_ERR("all eeprom data read failed\n");
		filp_close(f,NULL);
		return -EFAULT;
	}else{
		//4th do checksum
		LOG_DBG("all eeprom data read ok\n");
		if(imgSensorCheckEepromData(pData,checkData) != 0){
			kfree(pData->dataBuffer);
			pData->dataBuffer = NULL;
			LOG_ERR("checksum failed\n");
			filp_close(f,NULL);
			return -EFAULT;
		}
		LOG_INF("SensorID=%x DeviceID=%x read otp data success\n",pData->sensorID, pData->deviceID);
	}
	filp_close(f,NULL);
	return i4RetValue;
}

int imgSensorSetEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData){
	int i4RetValue = 0;
	LOG_INF("pData->deviceID = %d\n",pData->deviceID);
	if(pData->deviceID == 0x01){
		if(g_eepromMainData != NULL){
			return -ETXTBSY;
		}
		g_eepromMainData = pData;
	}else if(pData->deviceID == 0x02){
		if(g_eepromSubData != NULL){
			return -ETXTBSY;
		}
		g_eepromSubData= pData;
	}else if(pData->deviceID == 0x10){
		if(g_eepromMainMicroData != NULL){
			return -ETXTBSY;
		}
		g_eepromMainMicroData= pData;
	}else{
		LOG_ERR("we don't have this devices\n");
		return -ENODEV;
	}
#if MAIN_OTP_DUMP
	if(pData->dataBuffer)
		dumpEEPROMData(pData->dataLength,pData->dataBuffer);
#endif
	return i4RetValue;
}

