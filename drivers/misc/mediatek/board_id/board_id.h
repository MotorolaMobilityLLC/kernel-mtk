// liupeng@wind-mobi.com 20161121 begin

#ifndef BOARD_ID_H
#define BOARD_ID_H

#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>


 enum BOARD_ID{
	AP_DS_NA_EVT=0x00, //00
    EMEA_DS_NA_EVT,		//01
	EMEA_SS_NA_EVT,		//02
	EMEA_SS_NFC_EVT,	//03
	LATAM_DS_NA_EVT,	//04
	ROLA_SS_NA_EVT,		//05
	AP_DS_NA_DVT,		//06
	EMEA_DS_NA_DVT,		//07
	EMEA_SS_NA_DVT,		//08
	EMEA_SS_NFC_DVT,	//09
	LATAM_DS_NA_DVT,	//0a
	ROLA_SS_NA_DVT,		//0B
	AP_DS_NA_DVT2,           //0c
	EMEA_DS_NA_DVT2,         //0d
	EMEA_SS_NFC_DVT2,        //0e
	LATAM_DS_NA_SKY77643_DVT2,        //0f
	ROLA_SS_NA_SKY77643_DVT2,          //10
	LATAM_DS_NA_SKY77638_DVT2,      //11 
	ROLA_SS_NA_SKY77638_DVT2,	//12
	AP_B28_DS_NA_DVT2,        //13
//sunjingtao@wind-mobi.com at 20170119 begin
	AP_DS_NA_DVT2_1,		//14
	EMEA_DS_NA_DVT2_1,		//15
	EMEA_SS_NFC_DVT2_1,     //16
	LATAM_DS_NA_SKY77643_DVT2_1,        //17
	ROLA_SS_NA_SKY77643_DVT2_1,          //18
	LATAM_DS_NA_SKY77638_DVT2_1,      //19
	ROLA_SS_NA_SKY77638_DVT2_1,	//1a
	AP_B28_DS_NA_DVT2_1,		//1b
//sunjingtao@wind-mobi.com at 20170119 end
//sunjingtao@wind-mobi.com at 20170322 begin
	AP_B28_DS_SKY77638_DVT2_1,	//1c
	AP_DS_NA_MP,		//1d
	EMEA_DS_NA_MP,		//1e
	EMEA_SS_NFC_MP,     //1f
	LATAM_DS_NA_SKY77643_MP,        //20
	ROLA_SS_NA_SKY77643_MP,          //21
	LATAM_DS_NA_SKY77638_MP,      //22
	ROLA_SS_NA_SKY77638_MP,	//23
	AP_B28_DS_NA_MP,		//24
	AP_B28_DS_SKY77638_MP	//25
//sunjingtao@wind-mobi.com at 20170322 end
 };
 
 enum{
	BOARD_TYPE_EVT=0x00, //EVT
	BOARD_TYPE_DVT
 };
 
 struct board_id_dev{
	int val;
	int board_gpio;
	//struct kobject kobj;
	struct semaphore sem;
	struct cdev dev;
	struct mutex board_mutex;
 };
 
 #define BOARD_ID_DRVNAME		"board_id_z168"
 #define BOARD_ID_MAGIC        86
 #define BOARD_ID_SET_PWR		_IOW(BOARD_ID_MAGIC, 0x01, unsigned int)
 //use to read board_id  --sunsiyuan@wind-mobi.com add at 20161229 begin
 #define BOARD_ID_GET_VALUE		_IOR(BOARD_ID_MAGIC, 0x02, unsigned int)
 //use to read board_id  --sunsiyuan@wind-mobi.com add at 20161229 end
 #define BID_MAX_SIZE 				256
 
#define GPIO_TYPE_ID5         (GPIO73 | 0x80000000)
#define GPIO_TYPE_ID4         (GPIO70 | 0x80000000)
#define GPIO_TYPE_ID3         (GPIO71 | 0x80000000)
#define GPIO_TYPE_ID2         (GPIO60 | 0x80000000)
#define GPIO_TYPE_ID1         (GPIO59 | 0x80000000)
#define GPIO_TYPE_ID0         (GPIO99 | 0x80000000)
 /*
 static ssize_t  bid_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset);
 static ssize_t bid_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset);
 static loff_t bid_dev_llseek(struct file *filp ,loff_t *offset,int orig);
 static int bid_dev_open(struct inode *inode,struct file *filp);
 int bid_dev_unlocked_ioctl(struct file *filp ,unsigned int cmd ,unsigned long args);
 static ssize_t bid_num_show(struct device *dev,struct device_attribute *attr,  char *buf);
 static ssize_t bid_num_store(struct device *dev,struct device_attribute *attr, const char *buf,size_t count);
 */
#endif
// liupeng@wind-mobi.com 20161121 end