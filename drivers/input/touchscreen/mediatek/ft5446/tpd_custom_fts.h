#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

#include <linux/hrtimer.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
//#include <linux/io.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/byteorder/generic.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/rtpm_prio.h>

#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_gpio.h>
//#include <pmic_drv.h>
#include <cust_eint.h>
#include <linux/jiffies.h>

struct Upgrade_Info {
        u8 CHIP_ID;
        u8 FTS_NAME[20];
        u8 TPD_MAX_POINTS;
        u8 AUTO_CLB;
	u16 delay_aa;		/*delay of write FT_UPGRADE_AA */
	u16 delay_55;		/*delay of write FT_UPGRADE_55 */
	u8 upgrade_id_1;	/*upgrade id 1 */
	u8 upgrade_id_2;	/*upgrade id 2 */
	u16 delay_readid;	/*delay of read id */
	u16 delay_earse_flash; /*delay of earse flash*/
};

extern struct Upgrade_Info fts_updateinfo_curr;
#if 1
extern int fts_i2c_Read(struct i2c_client *client, char *writebuf,int writelen, char *readbuf, int readlen);
extern int fts_i2c_Write(struct i2c_client *client, char *writebuf, int writelen);
#else
extern int  fts_i2c_Read(struct i2c_client *client, u8 * wbuffer, s32 wlen, u8 * buffer, s32 len);
extern int fts_i2c_Write(struct i2c_client *client,  u8 * buffer, s32 len);
#endif
extern int fts_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue);
extern int fts_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue);
extern void focaltech_get_upgrade_array(void);
extern void fts_reset_tp(int HighOrLow);

/**********************Custom define begin**********************************************/
#define TPD_POWER_SOURCE_CUSTOM         PMIC_APP_CAP_TOUCH_VDD
#define IIC_PORT                   1//MT6572: 1  MT6589:0 , Based on the I2C index you choose for TPM

//#define FTS_GESTRUE                                  // if need the gesture funtion,enable this MACRO
//#define TPD_PROXIMITY					// if need the PS funtion,enable this MACRO

/*
///// ***** virtual key  definition  ***** /////

Below are the recommend  virtual key definition for different resolution TPM. 

HVGA  320x480    2key ( (80,530);(240,530) )           3key  ( (80,530);(160;530);(240,530) )          4key   ( (40,530);(120;530);(200,530);(280,530)  ) 
WVGA  480x800   2key ( (80,900);(400,900) )           3key  ( (80,900);(240,900);(400,900) )          4key   ( (60,900);(180;900);(300,900);(420,900)  ) 
FWVGA 480x854  2key ( (80,900);(400,900) )           3key  ( (80,900);(240,900);(400,900) )          4key   ( (60,900);(180;900);(300,900);(420,900)  ) 
QHD  540x960     2key ( (90,1080);(450,1080) )           3key  ( (90,1080);(270,1080);(450,1080) )          4key   ( (90,1080);(180;1080);(360,1080);(450,1080)  ) 
HD    1280x720    2key ( (120,1350);(600,1350) )           3key  ( (120,1350);(360,1350);(600,1350) )          4key   ( (120,1080);(240;1080);(480,1080);(600,1080)  )
FHD   1920x1080  2key ( (160,2100);(920,2100) )           3key  ( (160,2100);(540,2100);(920,2100) )          4key   ( (160,2100);(320;1080);(600,1080);(920,2100)  )
*/
#define TPD_HAVE_BUTTON	// if have virtual key,need define the MACRO
#define TPD_BUTTON_HEIGH        (40)  //100
#define TPD_KEY_COUNT           3    //  4
#define TPD_KEYS                { KEY_MENU, KEY_HOMEPAGE, KEY_BACK}
#define TPD_KEYS_DIM            	{{270,2500,20,TPD_BUTTON_HEIGH}, {540,2500,20,TPD_BUTTON_HEIGH}, {810,2500,20,TPD_BUTTON_HEIGH}}

/*********************Custom Define end*************************************************/

#define TPD_NAME    "FTS"

/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE
#define TPD_POWER_SOURCE         
#define TPD_I2C_NUMBER           		0
#define TPD_WAKEUP_TRIAL         		60
#define TPD_WAKEUP_DELAY         		100

#define TPD_VELOCITY_CUSTOM_X 			15
#define TPD_VELOCITY_CUSTOM_Y 			20

#define CFG_MAX_TOUCH_POINTS	5
#define MT_MAX_TOUCH_POINTS	10
#define FT_MAX_ID	0x0F
#define FT_TOUCH_STEP	6
#define FT_FACE_DETECT_POS		1
#define FT_TOUCH_X_H_POS		3
#define FT_TOUCH_X_L_POS		4
#define FT_TOUCH_Y_H_POS		5
#define FT_TOUCH_Y_L_POS		6
#define FT_TOUCH_EVENT_POS		3
#define FT_TOUCH_ID_POS			5

#define POINT_READ_BUF	(3 + FT_TOUCH_STEP * CFG_MAX_TOUCH_POINTS)


#define TPD_DELAY                		(2*HZ/100)
#define TPD_RES_X                		1080//480
#define TPD_RES_Y                		1920//800 change by lixh10 
#define TPD_CALIBRATION_MATRIX  		{962,0,0,0,1600,0,0,0};

//#define TPD_HAVE_CALIBRATION
//#define TPD_HAVE_TREMBLE_ELIMINATION

#define TPD_CLOSE_POWER_IN_SLEEP


/******************************************************************************/
/*Chip Device Type*/
#define IC_FT5X06						0	/*x=2,3,4*/
#define IC_FT5606						1	/*ft5506/FT5606/FT5816*/
#define IC_FT5316						2	/*ft5x16*/
#define IC_FT6208						3  	/*ft6208*/
#define IC_FT6x06     					4	/*ft6206/FT6306*/
#define IC_FT5x06i     					5	/*ft5306i*/
#define IC_FT5x36     					6	/*ft5336/ft5436/FT5436i*/



/*register address*/
#define FTS_REG_CHIP_ID				0xA3    //chip ID 
#define FTS_REG_FW_VER				0xA6   //FW  version 
#define FTS_REG_VENDOR_ID			0xA8   // TP vendor ID 
#define FTS_REG_POINT_RATE			0x88   //report rate	


#define TPD_MAX_POINTS_2                        2
#define TPD_MAX_POINTS_5                        5
#define TPD_MAXPOINTS_10                        10
#define AUTO_CLB_NEED                           1
#define AUTO_CLB_NONEED                         0


#define FTS_DBG
#ifdef FTS_DBG
#define DBG(fmt, args...) 				printk("[FTS]" fmt, ## args)
#else
#define DBG(fmt, args...) 				do{}while(0)
#endif

#endif /* TOUCHPANEL_H__ */
