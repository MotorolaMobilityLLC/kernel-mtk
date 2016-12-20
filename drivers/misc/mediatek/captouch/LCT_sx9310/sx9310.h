/* 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*
 * Definitions for IQS263 PS sensor chip.
 */
#ifndef __SX9311_H__
#define __SX9311_H__

extern struct platform_device *get_captouch_platformdev(void);

/*
 *  I2C Registers
 */
#define SX9310_IRQSTAT_REG    	0x00
#define SX9310_STAT0_REG    	0x01
#define SX9310_STAT1_REG    	0x02
#define SX9310_IRQ_ENABLE_REG	0x03
#define SX9310_IRQFUNC_REG		0x04
	
#define SX9310_CPS_CTRL0_REG    0x10
#define SX9310_CPS_CTRL1_REG    0x11
#define SX9310_CPS_CTRL2_REG    0x12
#define SX9310_CPS_CTRL3_REG    0x13
#define SX9310_CPS_CTRL4_REG    0x14
#define SX9310_CPS_CTRL5_REG    0x15
#define SX9310_CPS_CTRL6_REG    0x16
#define SX9310_CPS_CTRL7_REG    0x17
#define SX9310_CPS_CTRL8_REG    0x18
#define SX9310_CPS_CTRL9_REG	0x19
#define SX9310_CPS_CTRL10_REG	0x1A
#define SX9310_CPS_CTRL11_REG	0x1B
#define SX9310_CPS_CTRL12_REG	0x1C
#define SX9310_CPS_CTRL13_REG	0x1D
#define SX9310_CPS_CTRL14_REG	0x1E
#define SX9310_CPS_CTRL15_REG	0x1F
#define SX9310_CPS_CTRL16_REG	0x20
#define SX9310_CPS_CTRL17_REG	0x21
#define SX9310_CPS_CTRL18_REG	0x22
#define SX9310_CPS_CTRL19_REG	0x23
#define SX9310_SAR_CTRL0_REG	0x2A
#define SX9310_SAR_CTRL1_REG	0x2B
#define SX9310_SAR_CTRL2_REG	0x2C

#define SX9310_SOFTRESET_REG  0x7F

/*      Sensor Readback */
#define SX9310_CPSRD          0x30

#define SX9310_USEMSB         0x31
#define SX9310_USELSB         0x32

#define SX9310_AVGMSB         0x33
#define SX9310_AVGLSB         0x34

#define SX9310_DIFFMSB        0x35
#define SX9310_DIFFLSB        0x36
#define SX9310_OFFSETMSB		0x37
#define SX9310_OFFSETLSB		0x38

#define SX9310_SARMSB			0x39
#define SX9310_SARLSB			0x3A



/*      IrqStat 0:Inactive 1:Active     */
#define SX9310_IRQSTAT_RESET_FLAG      0x80
#define SX9310_IRQSTAT_TOUCH_FLAG      0x40
#define SX9310_IRQSTAT_RELEASE_FLAG    0x20
#define SX9310_IRQSTAT_COMPDONE_FLAG   0x10
#define SX9310_IRQSTAT_CONV_FLAG       0x08
#define SX9310_IRQSTAT_CLOSEALL_FLAG   0x04
#define SX9310_IRQSTAT_FARALL_FLAG     0x02
#define SX9310_IRQSTAT_SMARTSAR_FLAG   0x01


/* CpsStat  */
#define SX9310_TCHCMPSTAT_TCHCOMB_FLAG    0x08
#define SX9310_TCHCMPSTAT_TCHSTAT2_FLAG   0x04
#define SX9310_TCHCMPSTAT_TCHSTAT1_FLAG   0x02
#define SX9310_TCHCMPSTAT_TCHSTAT0_FLAG   0x01


/*      SoftReset */
#define SX9310_SOFTRESET  0xDE



struct smtc_reg_data {
  unsigned char reg;
  unsigned char val;
};
typedef struct smtc_reg_data smtc_reg_data_t;
typedef struct smtc_reg_data *psmtc_reg_data_t;

static struct smtc_reg_data sx9310_i2c_reg_setup[] = {
	{
		.reg = SX9310_IRQ_ENABLE_REG,
		.val = 0x70,
	},
	{
		.reg = SX9310_IRQFUNC_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL1_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL2_REG,
		.val = 0x04,
	},
	{
		.reg = SX9310_CPS_CTRL3_REG,
		.val = 0x0A,
	},
	{
		.reg = SX9310_CPS_CTRL4_REG,
#ifdef 	CONFIG_WIND_DEVICE_INFO
		.val = 0x15,
#else	
		.val = 0x0D,
#endif
	},
	{
		.reg = SX9310_CPS_CTRL5_REG,
#ifdef 	CONFIG_WIND_DEVICE_INFO
		.val = 0x41,
#else	
		.val = 0xD1,   //0xC1 -> 0XD1  8*30ma read cycle 
#endif		
	},
	{
		.reg = SX9310_CPS_CTRL6_REG,
		.val = 0x20,
	},
	{
		.reg = SX9310_CPS_CTRL7_REG,
		.val = 0x4C,
	},
	{
		.reg = SX9310_CPS_CTRL8_REG,
#ifdef 	CONFIG_WIND_DEVICE_INFO
		.val = 0x26,
#else	
		.val = 0x7E,
#endif		
	},
	{
		.reg = SX9310_CPS_CTRL9_REG,
		.val = 0x7D,
	},
	{
		.reg = SX9310_CPS_CTRL10_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL11_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL12_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL13_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL14_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL15_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL16_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL17_REG,
		.val = 0x04,
	},
	{
		.reg = SX9310_CPS_CTRL18_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_CPS_CTRL19_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_SAR_CTRL0_REG,
		.val = 0x00,
	},
	{
		.reg = SX9310_SAR_CTRL1_REG,
		.val = 0x80,
	},
	{
		.reg = SX9310_SAR_CTRL2_REG,
		.val = 0x0C,
	},
	{
		.reg = SX9310_CPS_CTRL0_REG,
#ifdef 	CONFIG_WIND_DEVICE_INFO
		.val = 0x21,
#else			
		.val = 0x22, //0x57  0x22 for cs1 only 
#endif		
	},
};

static void  sx9310_reg_config(void);
//static void  sx9310_reg_config_enable(void);
#endif
