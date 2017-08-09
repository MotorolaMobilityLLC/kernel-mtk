#ifndef _KD_CAMERA_HW_H_
#define _KD_CAMERA_HW_H_

//#include <linux/types.h>
//#include <mach/mt_typedefs.h>

#if defined CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include "pmic_drv.h"

#ifndef FALSE
  #define FALSE (0)
#endif

#ifndef TRUE
  #define TRUE  (1)
#endif

/*  */
/* Analog */
#define CAMERA_POWER_VCAM_A         PMIC_APP_MAIN_CAMERA_POWER_A
/* Digital */
#define CAMERA_POWER_VCAM_D         PMIC_APP_MAIN_CAMERA_POWER_D
/* AF */
#define CAMERA_POWER_VCAM_AF        PMIC_APP_MAIN_CAMERA_POWER_AF
/* digital io */
#define CAMERA_POWER_VCAM_IO        PMIC_APP_MAIN_CAMERA_POWER_IO
/* Digital for Sub */
#define SUB_CAMERA_POWER_VCAM_D     PMIC_APP_SUB_CAMERA_POWER_D

/* FIXME, should defined in DCT tool */
/* Main sensor */
#define CAMERA_CMRST_PIN            GPIO_CAMERA_CMRST_PIN
#define CAMERA_CMRST_PIN_M_GPIO     GPIO_CAMERA_CMRST_PIN_M_GPIO

#define CAMERA_CMPDN_PIN            GPIO_CAMERA_CMPDN_PIN
#define CAMERA_CMPDN_PIN_M_GPIO     GPIO_CAMERA_CMPDN_PIN_M_GPIO

/* FRONT sensor */
#define CAMERA_CMRST1_PIN           GPIO_CAMERA_CMRST1_PIN
#define CAMERA_CMRST1_PIN_M_GPIO    GPIO_CAMERA_CMRST1_PIN_M_GPIO

#define CAMERA_CMPDN1_PIN           GPIO_CAMERA_CMPDN1_PIN
#define CAMERA_CMPDN1_PIN_M_GPIO    GPIO_CAMERA_CMPDN1_PIN_M_GPIO

/* Define I2C Bus Num */
//#define SUPPORT_I2C_BUS_NUM1        0
//#define SUPPORT_I2C_BUS_NUM2        0
#else
#if 1
#define CAMERA_CMRST_PIN            0
#define CAMERA_CMRST_PIN_M_GPIO     0

#define CAMERA_CMPDN_PIN            0
#define CAMERA_CMPDN_PIN_M_GPIO     0

/* FRONT sensor */
#define CAMERA_CMRST1_PIN           0
#define CAMERA_CMRST1_PIN_M_GPIO    0

#define CAMERA_CMPDN1_PIN           0
#define CAMERA_CMPDN1_PIN_M_GPIO    0

/* Main2 sensor */
#define CAMERA_CMRST2_PIN           0 /*gpio35*/
#define CAMERA_CMRST2_PIN_M_GPIO    0

#define CAMERA_CMPDN2_PIN           0 /*gpio34*/
#define CAMERA_CMPDN2_PIN_M_GPIO    0


#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0
#endif
#endif /* End of #if defined CONFIG_MTK_LEGACY */

#ifndef CONFIG_PINCTRL_MT6797 
#include <mach/gpio_const.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>

/* Main sensor */
#define CAMERA_CMRST_PIN            GPIO_CAMERA_CMRST_PIN  /*gpio32*/
#define CAMERA_CMRST_PIN_M_GPIO     GPIO_CAMERA_CMRST_PIN_M_GPIO

#define CAMERA_CMPDN_PIN            GPIO_CAMERA_CMPDN_PIN  /*gpio28*/
#define CAMERA_CMPDN_PIN_M_GPIO     GPIO_CAMERA_CMPDN_PIN_M_GPIO

/* FRONT sensor */
#define CAMERA_CMRST1_PIN           GPIO_CAMERA_CMRST1_PIN /*gpio33*/
#define CAMERA_CMRST1_PIN_M_GPIO    GPIO_CAMERA_CMRST1_PIN_M_GPIO

#define CAMERA_CMPDN1_PIN           GPIO_CAMERA_CMPDN1_PIN /*gpio29*/
#define CAMERA_CMPDN1_PIN_M_GPIO    GPIO_CAMERA_CMPDN1_PIN_M_GPIO

/* Main2 sensor */
#define CAMERA_CMRST2_PIN           GPIO_CAMERA_2_CMRST_PIN /*gpio35*/
#define CAMERA_CMRST2_PIN_M_GPIO    GPIO_CAMERA_2_CMRST_PIN_M_GPIO

#define CAMERA_CMPDN2_PIN           GPIO_CAMERA_2_CMPDN_PIN /*gpio34*/
#define CAMERA_CMPDN2_PIN_M_GPIO    GPIO_CAMERA_2_CMPDN_PIN_M_GPIO


#endif



typedef enum {
	VDD_None,
	PDN,
	RST,
	SensorMCLK,
	AVDD,
	DVDD,
	DOVDD,
	AFVDD,
	SUB_DVDD,
	MAIN2_DVDD,
} PowerType;

typedef enum {
	CAMPDN,
	CAMRST,
	CAM1PDN,
	CAM1RST,
	CAMLDO,
	CAMLDO1
} CAMPowerType;

extern void ISP_MCLK1_EN(bool En);
extern void ISP_MCLK2_EN(bool En);
extern void ISP_MCLK3_EN(bool En);

extern bool _hwPowerDown(PowerType type);
extern bool _hwPowerOn(PowerType type, int powerVolt);


int mtkcam_gpio_set(int PinIdx, int PwrType, int Val);
int mtkcam_gpio_init(struct platform_device *pdev);

#endif
