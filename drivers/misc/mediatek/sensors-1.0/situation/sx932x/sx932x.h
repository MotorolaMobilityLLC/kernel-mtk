/*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*/
#ifndef SX932x_H
#define SX932x_H

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#if 0
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#define MULTILEVEL 1
//#define RFDEBUG
/*
*  I2C Registers
*/
//-Interrupt and status
#define SX932x_IRQSTAT_REG		0x00
#define SX932x_STAT0_REG		0x01
#define SX932x_STAT1_REG		0x02
#define SX932x_STAT2_REG		0x03
#define SX932x_STAT3_REG		0x04
#define SX932x_IRQ_ENABLE_REG	0x05
#define SX932x_IRQCFG0_REG		0x06
#define SX932x_IRQCFG1_REG		0x07
#define SX932x_IRQCFG2_REG		0x08
//-General control
#define SX932x_CTRL0_REG		0x10
#define SX932x_CTRL1_REG		0x11
#define SX932x_I2CADDR_REG		0x14
#define SX932x_CLKSPRD			0x15
//-AFE Control
#define SX932x_AFE_CTRL0_REG	0x20
#define SX932x_AFE_CTRL1_REG	0x21
#define SX932x_AFE_CTRL2_REG	0x22
#define SX932x_AFE_CTRL3_REG	0x23
#define SX932x_AFE_CTRL4_REG	0x24
#define SX932x_AFE_CTRL5_REG	0x25
#define SX932x_AFE_CTRL6_REG	0x26
#define SX932x_AFE_CTRL7_REG	0x27
#define SX932x_AFE_PH0_REG		0x28
#define SX932x_AFE_PH1_REG		0x29
#define SX932x_AFE_PH2_REG		0x2A
#define SX932x_AFE_PH3_REG		0x2B
#define SX932x_AFE_CTRL8		0x2C
#define SX932x_AFE_CTRL9		0x2D
//-Main Digital Processing (Prox) control
#define SX932x_PROX_CTRL0_REG	0x30
#define SX932x_PROX_CTRL1_REG	0x31
#define SX932x_PROX_CTRL2_REG	0x32
#define SX932x_PROX_CTRL3_REG	0x33
#define SX932x_PROX_CTRL4_REG	0x34
#define SX932x_PROX_CTRL5_REG	0x35
#define SX932x_PROX_CTRL6_REG	0x36
#define SX932x_PROX_CTRL7_REG	0x37
//-Advanced Digital Processing control
#define SX932x_ADV_CTRL0_REG	0x40
#define SX932x_ADV_CTRL1_REG	0x41
#define SX932x_ADV_CTRL2_REG	0x42
#define SX932x_ADV_CTRL3_REG	0x43
#define SX932x_ADV_CTRL4_REG	0x44
#define SX932x_ADV_CTRL5_REG	0x45
#define SX932x_ADV_CTRL6_REG	0x46
#define SX932x_ADV_CTRL7_REG	0x47
#define SX932x_ADV_CTRL8_REG	0x48
#define SX932x_ADV_CTRL9_REG	0x49
#define SX932x_ADV_CTRL10_REG	0x4A
#define SX932x_ADV_CTRL11_REG	0x4B
#define SX932x_ADV_CTRL12_REG	0x4C
#define SX932x_ADV_CTRL13_REG	0x4D
#define SX932x_ADV_CTRL14_REG	0x4E
#define SX932x_ADV_CTRL15_REG	0x4F
#define SX932x_ADV_CTRL16_REG	0x50
#define SX932x_ADV_CTRL17_REG	0x51
#define SX932x_ADV_CTRL18_REG	0x52
#define SX932x_ADV_CTRL19_REG	0x53
#define SX932x_ADV_CTRL20_REG	0x54
/*      Sensor Readback */
#define SX932x_CPSRD			0x60
#define SX932x_USEMSB			0x61
#define SX932x_USELSB			0x62
#define SX932x_AVGMSB			0x63
#define SX932x_AVGLSB			0x64
#define SX932x_DIFFMSB			0x65
#define SX932x_DIFFLSB			0x66
#define SX932x_OFFSETMSB		0x67
#define SX932x_OFFSETLSB		0x68
#define SX932x_SARMSB			0x69
#define SX932x_SARLSB			0x6A

#define SX932x_SOFTRESET_REG	0x9F
#define SX932x_WHOAMI_REG		0xFA
#define SX932x_REV_REG			0xFB

/*      IrqStat 0:Inactive 1:Active     */
#define SX932x_IRQSTAT_RESET_FLAG		0x80
#define SX932x_IRQSTAT_TOUCH_FLAG		0x40
#define SX932x_IRQSTAT_RELEASE_FLAG		0x20
#define SX932x_IRQSTAT_COMPDONE_FLAG	0x10
#define SX932x_IRQSTAT_CONV_FLAG		0x08
#define SX932x_IRQSTAT_PROG2_FLAG		0x04
#define SX932x_IRQSTAT_PROG1_FLAG		0x02
#define SX932x_IRQSTAT_PROG0_FLAG		0x01


/* RegStat0  */
#define SX932x_PROXSTAT_PH3_FLAG		0x08
#define SX932x_PROXSTAT_PH2_FLAG		0x04
#define SX932x_PROXSTAT_PH1_FLAG		0x02
#define SX932x_PROXSTAT_PH0_FLAG		0x01

/*      SoftReset */
#define SX932x_SOFTRESET				0xDE
#define SX932x_WHOAMI_VALUE				0x22  //just for sx9325
//#define SX932x_WHOAMI_VALUE				0x23  //just for sx9323
#define SX932x_REV_VALUE				0x22 //just for sx9325

#define LGE_SENSOR

/**************************************
*   define platform data
*
**************************************/
struct smtc_reg_data {
	unsigned char reg;
	unsigned char val;
};
typedef struct smtc_reg_data smtc_reg_data_t;
typedef struct smtc_reg_data *psmtc_reg_data_t;


struct _buttonInfo {
	/*! The Key to send to the input */
	int keycode;
	/*! Mask to look for on Touch Status */
	int mask;
	/*! Current state of button. */
	int state;
};

struct totalButtonInformation {
	struct _buttonInfo *buttons;
	int buttonSize;
	struct input_dev *input;
};

typedef struct totalButtonInformation buttonInformation_t;
typedef struct totalButtonInformation *pbuttonInformation_t;

/* Define Registers that need to be initialized to values different than
* default
*/
static struct smtc_reg_data sx932x_i2c_reg_setup[] = {
//Interrupt and config
	{
		.reg = SX932x_IRQ_ENABLE_REG,	//0x05
#ifdef MULTILEVEL
		.val = 0x64,
#else
#ifdef RFDEBUG
		.val = 0x68,					// Enavle Close and Far -> enable compensation interrupt
#else
		.val = 0x60,
#endif
#endif
	},
	{
		.reg = SX932x_IRQCFG0_REG, 		//0x06
		.val = 0x00,       				//
	},
	{
		.reg = SX932x_IRQCFG1_REG,		//0x07  
		.val = 0x00,
	},
	{
		.reg = SX932x_IRQCFG2_REG,		//0x08
		.val = 0x00,					//Activ Low
	},
	//--------General control
	{
		.reg = SX932x_CTRL0_REG,    //0x10
#ifdef RFDEBUG
		.val = 0x1a,       
#else
		.val = 0x16,	   // Scanperiod : 100ms(10110)
#endif
	},
	{
		.reg = SX932x_I2CADDR_REG,   //0x14
		.val = 0x00,       //I2C Address : 0x28
	},
	{
		.reg = SX932x_CLKSPRD,    //0x15
		.val = 0x00,       //
	},
	//--------AFE Control
	{
		.reg = SX932x_AFE_CTRL0_REG,   //0x20
		.val = 0x00,       // CSx pin during sleep mode : HZ
	},
	{
		.reg = SX932x_AFE_CTRL1_REG,   //0x21
		.val = 0x10,       //reserved
	},
	{
		.reg = SX932x_AFE_CTRL2_REG,   //0x22
		.val = 0x00,       //reserved
	},
	{
		.reg = SX932x_AFE_CTRL3_REG,   //0x23
		.val = 0x00,       //Analog Range(ph0/1) : Small
	},
	{
		.reg = SX932x_AFE_CTRL4_REG,   //0x24
		.val = 0x44,       //Sampling Freq(ph0/1) : 83.33khz(01000), Resolution(ph0/1) : 128(100)
	},
	{
		.reg = SX932x_AFE_CTRL5_REG,   //0x25
		.val = 0x00,       //reserved
	},
	{
		.reg = SX932x_AFE_CTRL6_REG,   //0x26
		.val = 0x01,       //big//Analog Range(ph2/3) : Small
	},
	{
		.reg = SX932x_AFE_CTRL7_REG,   //0x27
		.val = 0x44,       //Sampling Freq(ph2/3) : 83.33khz(01000), Resolution(ph2/3) : 128(100)
	},
	{
		.reg = SX932x_AFE_PH0_REG,   //0x28
		.val = 0x01,
	},
	{
		.reg = SX932x_AFE_PH1_REG,     //0x29
		.val = 0x10,       // CS2:Input CS1:HZ Shield CS0 :HZ
	},
	{
		.reg = SX932x_AFE_PH2_REG,   //0x2A
		.val = 0x1B,       //CS2:HZ CS1:HZ CS0 :HZ  
	},
	{
		.reg = SX932x_AFE_PH3_REG,   //0x2B
		.val = 0x00,       //CS2:HZ CS1:HZ CS0 :HZ
	},
	{
		.reg = SX932x_AFE_CTRL8,    //0x2C
		.val = 0x12,       // input register(kohm) 4(0010)
	},
	{
		.reg = SX932x_AFE_CTRL9,    //0x2D
		.val = 0x08,       // Analg gain : x1(1000)
	},
	//--------PROX control
	{
		.reg = SX932x_PROX_CTRL0_REG,  //0x30
		.val = 0x09,       // Digital Gain(ph0/1) : off(001) Digital Filter(ph0/1) : 1-1/2(001)
	},
	{
		.reg = SX932x_PROX_CTRL1_REG,  //0x31
		.val = 0x09,       // Digital Gain(ph2/3) : off(001) Digital Filter(ph2/3) : 1-1/2(001)
	},
	{
		.reg = SX932x_PROX_CTRL2_REG,  //0x32
		.val = 0x08,       //AVGNEGTHRESH : 16384
	},
	{
		.reg = SX932x_PROX_CTRL3_REG,  // 0x33 
		.val = 0x20,       //AVGPOSTHRESH : 16384
	},
	{
		.reg = SX932x_PROX_CTRL4_REG,  //0x34 
		.val = 0x0C,       //AVGFREEZEDIS : on(0) ,AVGNEGFILT :1-1/2(001) ,AVGPOSFILT : 1-1/256(100)
	},
	{
		.reg = SX932x_PROX_CTRL5_REG,  //0x35
		.val = 0x00,       //FARCOND: PROXDIFF < (THRESH.HYST), HYST : None, CLOSEDEB : off ,FARDEB : off
	},
	{
		.reg = SX932x_PROX_CTRL6_REG,  //0x36
		.val = 0x25,       // Prox Theshold(ph0/1) : 200
	},
	{
		.reg = SX932x_PROX_CTRL7_REG,  //0x37
		.val = 0x1B,       // Prox Theshold(ph2/3) : 200
	},
	//--------Advanced control (defult)
	{
		.reg = SX932x_ADV_CTRL0_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL1_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL2_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL3_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL4_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL5_REG,
		.val = 0x05,
	},
	{
		.reg = SX932x_ADV_CTRL6_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL7_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL8_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL9_REG,
		.val = 0x80,
	},
	{
		.reg = SX932x_ADV_CTRL10_REG,
#ifdef MULTILEVEL
		.val = 0x10,
#else
		.val = 0x00,
#endif
	},
	{
		.reg = SX932x_ADV_CTRL11_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL12_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL13_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL14_REG,
		.val = 0x80,
	},
	{
		.reg = SX932x_ADV_CTRL15_REG,
		.val = 0x0C,
	},
	{
		.reg = SX932x_ADV_CTRL16_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL17_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL18_REG,
		.val = 0x00,
	},
	{
		.reg = SX932x_ADV_CTRL19_REG,
		.val = 0xF0,
	},
	{
		.reg = SX932x_ADV_CTRL20_REG,
		.val = 0xF0,
	},
	//--------Sensor enable
	{
		.reg = SX932x_CTRL1_REG,    //0x11
		.val = 0x21,
	},
};

static struct _buttonInfo psmtcButtons[] = {
	{
                .keycode = KEY_0,
                .mask = SX932x_PROXSTAT_PH0_FLAG,
        },
#if 0
	{
		.keycode = KEY_0,
		.mask = SX932x_PROXSTAT_PH0_FLAG,
	},
	{
		.keycode = KEY_1,
		.mask = SX932x_PROXSTAT_PH1_FLAG,
	},
	{
		.keycode = KEY_2,
		.mask = SX932x_PROXSTAT_PH2_FLAG,
	},
	{
		.keycode = KEY_3,
		.mask = SX932x_PROXSTAT_PH3_FLAG,
	},
#endif
};

struct sx932x_platform_data {
	int i2c_reg_num;
	struct smtc_reg_data *pi2c_reg;
	int irq_gpio;
	pbuttonInformation_t pbuttonInformation;

	int (*get_is_nirq_low)(void);
	u8 input_mainsensor;  // 0x01 ~ 0x02,//Startup function
	u8 input_refsensor;   // 0x00 ~ 0x02,//Startup function
  	
	int     (*init_platform_hw)(struct i2c_client *client);
	void    (*exit_platform_hw)(struct i2c_client *client);
};
typedef struct sx932x_platform_data sx932x_platform_data_t;
typedef struct sx932x_platform_data *psx932x_platform_data_t;

/***************************************
*  define data struct/interrupt
*
***************************************/

#define MAX_NUM_STATUS_BITS (8)

typedef struct sx93XX sx93XX_t, *psx93XX_t;
struct sx93XX 
{
	void * bus; /* either i2c_client or spi_client */

	struct device *pdev; /* common device struction for linux */

	void *pDevice; /* device specific struct pointer */

	/* Function Pointers */
	int (*init)(psx93XX_t this); /* (re)initialize device */

	/* since we are trying to avoid knowing registers, create a pointer to a
	* common read register which would be to read what the interrupt source
	* is from 
	*/
	int (*refreshStatus)(psx93XX_t this); /* read register status */

	int (*get_nirq_low)(void); /* get whether nirq is low (platform data) */

	/* array of functions to call for corresponding status bit */
	void (*statusFunc[MAX_NUM_STATUS_BITS])(psx93XX_t this); 
	
	/* Global variable */
	u8 		failStatusCode;	/*Fail status code*/
	bool	reg_in_dts;

	spinlock_t       lock; /* Spin Lock used for nirq worker function */
	int irq; /* irq number used */

	/* whether irq should be ignored.. cases if enable/disable irq is not used
	* or does not work properly */
	char irq_disabled;

	u8 useIrqTimer; /* older models need irq timer for pen up cases */

	int irqTimeout; /* msecs only set if useIrqTimer is true */

	/* struct workqueue_struct *ts_workq;  */  /* if want to use non default */
	struct delayed_work dworker; /* work struct for worker function */
};

void sx93XX_suspend(psx93XX_t this);
void sx93XX_resume(psx93XX_t this);
int sx93XX_IRQ_init(psx93XX_t this);
int sx93XX_remove(psx93XX_t this);

#endif
