/*
   this file shows the register map and structure data of HL7139 charger driver.
   this is free s/w, you can modify or recreate any code that you want. 
   it under the terms of the GNU General Public License version 2 as published by the Free Software Founation.
   */
#ifndef HL7139_CHARGER_H
#define HL7139_CHARGER_H

#define REG_ADC_VIN_0               0x42
#define BITS_ADC_VIN_0              BITS(7,0)

#define REG_ADC_VIN_1               0x43
#define BITS_ADC_VIN_LSB            BITS(3,0)

#define REG_ADC_IIN_0               0x44
#define BITS_ADC_IIN_0              BITS(7,0)

#define REG_ADC_IIN_1               0x45
#define BITS_ADC_IIN_LSB            BITS(3,0)

#define REG_ADC_VBAT_0              0x46
#define BITS_ADC_VBAT_0             BITS(7,0)

#define REG_ADC_VBAT_1              0x47
#define BITS_ADC_VBAT_LSB           BITS(3,0)

#define REG_ADC_IBAT_0              0x48
#define BITS_ADC_IBAT_0             BITS(7,0)

#define REG_ADC_IBAT_1              0x49
#define BITS_ADC_IBAT_LSB           BITS(3,0)

#define REG_ADC_VTS_0               0x4A
#define BITS_ADC_VTS_0              BITS(7,0)

#define REG_ADC_VTS_1               0x4B
#define BITS_ADC_VTS_LSB            BITS(3,0)

#define REG_ADC_VOUT_0              0x4C
#define BITS_ADC_VOUT_0             BITS(7,0)

#define REG_ADC_VOUT_1              0x4D
#define BITS_ADC_VOUT_LSB           BITS(3,0)

#define REG_ADC_TDIE_0              0x4E
#define BITS_ADC_TDIE_0             BITS(7,0)

#define REG_ADC_TDIE_1              0x4F
#define BITS_ADC_TDIE_LSB           BITS(3,0)

#define REG_DEVICE_ID               0x00
#define BITS_DEV_REV                BITS(7,4)
#define BITS_DEV_ID                 BITS(3,0)
#define DEVICE_ID                   0X0A

/* Register 05h */
#define HL7139_STATUS_A                 0x05
#define BIT_VIN_OVP_STS	                BIT(7)
#define BIT_VIN_UVLO_STS	        BIT(6)
#define BIT_TRACK_OV_STS                BIT(5)
#define BIT_TRACK_UV_STS                BIT(4)
#define BIT_VBAT_OVP_STS                BIT(3)
#define BIT_VOUT_OVP_STS                BIT(2)
#define BIT_PMID_QUAL_STS               BIT(1)
#define BIT_VBUS_UV_STS                 BIT(0)

/* Register 06h */
#define HL7139_STATUS_B                 0x06
#define BIT_IIN_OCP_STS                 BIT(7)
#define BIT_IBAT_OCP_STS                BIT(6)
#define BIT_IIN_UCP_STS	                BIT(5)
#define BIT_FET_SHORT_STS               BIT(4)
#define BIT_CFLY_SHORT_STS              BIT(3)
#define BITS_DEV_MODE_STS               BITS(2,1)
#define BIT_THSD_STS                    BIT(0)

/* Register 07h */
#define HL7139_STATUS_C                 0x07
#define BIT_QPUMP_STS                   BIT(7)

/* Register 08h */
#define HL7139_VBAT_REG			0x08
#define	HL7139_BAT_OVP_MASK	    	0x80
#define	HL7139_BAT_OVP_SHIFT            7
#define	HL7139_BAT_OVP_ENABLE		0
#define	HL7139_BAT_OVP_DISABLE		1

#define	HL7139_BAT_OVP_DEB_MASK	        0x40
#define	HL7139_BAT_OVP_DEB_SHIFT 	6
#define	HL7139_BAT_OVP_DEB_80US	 	0
#define	HL7139_BAT_OVP_DEB_2MS		1

#define HL7139_BAT_OVP_SET_MASK 	0x1F
#define HL7139_BAT_OVP_SET_SHIFT	0
#define HL7139_BAT_OVP_BASE		4000
#define HL7139_BAT_OVP_LSB		10


/* Register 0Ah */
#define HL7139_IBAT_REG       		0x0A
#define HL7139_BAT_OCP_MASK	        0x80
#define HL7139_BAT_OCP_SHIFT	        7
#define HL7139_BAT_OCP_ENABLE	     	0
#define HL7139_BAT_OCP_DISABLE	        1

#define	HL7139_BAT_OCP_DEB_MASK	        0x40
#define	HL7139_BAT_OCP_DEB_SHIFT	6
#define	HL7139_BAT_OCP_DEB_80US		0
#define	HL7139_BAT_OCP_DEB_200US	1

#define HL7139_BAT_OCP_ALM_MASK		0x1F
#define HL7139_BAT_OCP_ALM_SHIFT	0
#define HL7139_BAT_OCP_ALM_BASE		2000
#define HL7139_BAT_OCP_ALM_LSB		100


/* Register 0Bh */
#define HL7139_VBUS_OVP_REG             0x0B
#define HL7139_EXT_NFET_USE_MASK        0x80
#define HL7139_EXT_NFET_USE_SHIFT       7
#define HL7139_EXT_NFET_USE_USED        1
#define HL7139_EXT_NFET_USE_NOT_USED    0

#define HL7139_VBUS_PD_EN_MASK          0x40
#define HL7139_VBUS_PD_EN_SHIFT         6
#define HL7139_VBUS_PD_EN_DISABLE       0
#define HL7139_VBUS_PD_EN_ENABLE        1

#define HL7139_VGS_SEL_MASK             0x30
#define HL7139_VGS_SEL_SHIFT            4
#define HL7139_VGS_SEL_VOL_7V           0
#define HL7139_VGS_SEL_VOL_75V      	1
#define HL7139_VGS_SEL_VOL_8V       	2
#define HL7139_VGS_SEL_VOL_85V      	3

#define HL7139_VBUS_OVP_TH_MASK		0xF
#define HL7139_VBUS_OVP_TH_SHIFT	0
#define HL7139_VBUS_OVP_TH_BASE         4000
#define HL7139_VBUS_OVP_TH_LSB          1000

/* Register 0Ch */
#define HL7139_VIN_OVP_REG              0x0C
#define	HL7139_PD_CFG_MASK              0x20
#define HL7139_PD_CFG_SHIFT		5
#define HL7139_PD_CFG_AUTO		0
#define HL7139_PD_CFG_ENABLE		1

#define HL7139_OVP_DIS_MASK             0x10
#define HL7139_OVP_DIS_SHIFT            4
#define HL7139_OVP_DIS_ENABLE           0
#define HL7139_OVP_DIS_DISABLE          1

#define HL7139_OVP_VAL_MASK             0x7
#define HL7139_OVP_VAL_SHIFT            0
#define HL7139_OVP_VAL_BP_MIN 		5100
#define HL7139_OVP_VAL_BP_MAX  		5850
#define HL7139_OVP_VAL_BP_LSB  		50
#define HL7139_OVP_VAL_CP_MIN  		10200
#define HL7139_OVP_VAL_CP_MAX  		11700
#define HL7139_OVP_VAL_CP_LSB  		100

/* Register 0Eh */
#define HL7139_IIN_REG                  0x0E
#define HL7139_IIN_OCP_DIS_MASK         0x80
#define HL7139_IIN_OCP_DIS_SHIFT        7
#define HL7139_IIN_OCP_DIS_ENABLE       0
#define HL7139_IIN_OCP_DIS_DISABLE      1

#define HL7139_IIN_REG_TH_MASK          0x1F
#define HL7139_IIN_REG_TH_SHIFT         0
#define HL7139_IIN_REG_TH_BP_MIN	1000
#define HL7139_IIN_REG_TH_BP_MAX	3500
#define HL7139_IIN_REG_TH_BP_LSB	50
#define HL7139_IIN_REG_TH_CP_MIN	2000
#define HL7139_IIN_REG_TH_CP_MAX	7000
#define HL7139_IIN_REG_TH_CP_LSB	100

/* Register 0Fh */
#define HL7139_IIN_OC_REG               0x0F
#define HL7139_IIN_OC_DEB_MASK          0x80
#define HL7139_IIN_OC_DEB_SHIFT         7
#define HL7139_IIN_OC_DEB_80US          0
#define HL7139_IIN_OC_DEB_200US         1

#define HL7139_IIN_OCP_TH_MASK          0x60
#define HL7139_IIN_OCP_TH_SHIFT         5
#define HL7139_IIN_OCP_TH_BP_150MA      0
#define HL7139_IIN_OCP_TH_BP_200MA      1
#define HL7139_IIN_OCP_TH_BP_250MA      2
#define HL7139_IIN_OCP_TH_BP_300MA      3
#define HL7139_IIN_OCP_TH_CP_300MA      0
#define HL7139_IIN_OCP_TH_CP_400MA      1
#define HL7139_IIN_OCP_TH_CP_500MA      2
#define HL7139_IIN_OCP_TH_CP_600MA      3

/* Register 10h */
#define HL7139_REG_CTRL0                0x10
#define HL7139_TDIE_REG_DIS_MASK        0x80
#define HL7139_TDIE_REG_DIS_SHIFT       7
#define HL7139_TDIE_REG_DIS_ENABLE      0
#define HL7139_TDIE_REG_DIS_DISABLE     1

#define HL7139_IIN_REG_DIS_MASK         0x40
#define HL7139_IIN_REG_DIS_SHIFT        6
#define HL7139_IIN_REG_DIS_ENABLE       0
#define HL7139_IIN_REG_DIS_DISABLE      1

#define HL7139_TDIE_REG_TH_MASK         0x30
#define HL7139_TDIE_REG_TH_SHIFT        4
#define HL7139_TDIE_REG_TH_80           0
#define HL7139_TDIE_REG_TH_90		1
#define HL7139_TDIE_REG_TH_100		2
#define HL7139_TDIE_REG_TH_110		3


/* Register 11h */
#define HL7139_REG_CTRL1                0x11
#define HL7139_VBAT_REG_DIS_MASK        0X80
#define HL7139_VBAT_REG_DIS_SHIFT       7
#define HL7139_VBAT_REG_DIS_ENABLE      0
#define HL7139_VBAT_REG_DIS_DISABLE     1

#define HL7139_IBAT_REG_DIS_MASK        0x40
#define HL7139_IBAT_REG_DIS_SHIFT       6
#define HL7139_IBAT_REG_DIS_ENABLE      0
#define HL7139_IBAT_REG_DIS_DISABLE     1

#define HL7139_VBAT_OVP_TH_MASK         0x30
#define HL7139_VBAT_OVP_TH_SHIFT        4
#define HL7139_VBAT_OVP_TH_80           0
#define HL7139_VBAT_OVP_TH_90           1
#define HL7139_VBAT_OVP_TH_100          2
#define HL7139_VBAT_OVP_TH_110          3

#define HL7139_IBAT_OCP_TH_MASK         0xC
#define HL7139_IBAT_OCP_TH_SHIFT        2
#define HL7139_IBAT_OCP_TH_200          0
#define HL7139_IBAT_OCP_TH_300          1
#define HL7139_IBAT_OCP_TH_400          2
#define HL7139_IBAT_OCP_TH_500          3


/* Register 12h */
#define HL7139_CTRL0                    0x12
#define HL7139_CHG_EN_MASK              0x80
#define HL7139_CHG_EN_SHIFT             7
#define HL7139_CHG_EN_ENABLE            1
#define HL7139_CHG_EN_DISABLE           0

#define HL7139_FSW_SET_MASK             0x78
#define HL7139_FSW_SET_SHIFT            3
#define HL7139_FSW_SET_BASE             500
#define HL7139_FSW_SET_LB               100

#define HL7139_UNPLUG_DET_MASK          0x4
#define HL7139_UNPLUG_DET_SHIFT         2
#define HL7139_UNPLUG_DET_ENABLE        1
#define HL7139_UNPLUG_DET_DISABLE       0

#define HL7139_IIN_UCP_TH_MASK          0x3
#define HL7139_IIN_UCP_TH_SHIFT 	0
#define HL7139_IIN_UCP_TH_100		0
#define HL7139_IIN_UCP_TH_150		1
#define HL7139_IIN_UCP_TH_200		2
#define HL7139_IIN_UCP_TH_250		3

#define HL7139_IIN_UCP_TH_200_BP        0
#define HL7139_IIN_UCP_TH_300_BP	1
#define HL7139_IIN_UCP_TH_400_BP	2
#define HL7139_IIN_UCP_TH_500_BP	3


/* Register 13h */
#define HL7139_CTRL1			0x13
#define HL7139_UCP_DEB_SEL_MASK         0xC0
#define HL7139_UCP_DEB_SEL_SHIFT        6
#define HL7139_UCP_DEB_SEL_10           0
#define HL7139_UCP_DEB_SEL_100		1
#define HL7139_UCP_DEB_SEL_500		2
#define HL7139_UCP_DEB_SEL_1000		3

#define HL7139_VOUT_OVP_DIS_MASK        0x20
#define HL7139_VOUT_OVP_DIS_SHIFT       5
#define HL7139_VOUT_OVP_DIS_ENABLE      0
#define HL7139_VOUT_OVP_DIS_DISABLE     1

#define HL7139_TS_PORT_EN_MASK          0x10
#define HL7139_TS_PORT_EN_SHIFT         4
#define HL7139_TS_PORT_EN_ENABLE        1
#define HL7139_TS_PORT_EN_DISABLE       0

#define HL7139_AUTO_V_REC_EN_MASK       0x4
#define HL7139_AUTO_V_REC_EN_SHIFT      2
#define HL7139_AUTO_V_REC_EN_ENABLE     0
#define HL7139_AUTO_V_REC_EN_DISABLE    1 

#define HL7139_AUTO_I_REC_EN_MASK       0x2
#define HL7139_AUTO_I_REC_EN_SHIFT      1
#define HL7139_AUTO_I_REC_EN_ENABLE     0
#define HL7139_AUTO_I_REC_EN_DISABLE    1

#define HL7139_AUTO_UCP_REC_EN_MASK     0x1
#define HL7139_AUTO_UCP_REC_EN_SHIFT    0
#define HL7139_AUTO_UCP_REC_EN_ENABLE   0
#define HL7139_AUTO_UCP_REC_EN_DISABLE  1


/* Register 14h */
#define HL7139_CTRL2			0x14
#define HL7139_SFT_RST_MASK             0xF0
#define HL7139_SFT_RST_SHIFT            4
#define HL7139_SFT_RST_NO_RESET         0
#define HL7139_SFT_RST_SET_RESET        12

#define HL7139_WD_DIS_MASK              0x8
#define HL7139_WD_DIS_SHIFT             3
#define HL7139_WD_DIS_WATDOG_ENABLE     0
#define HL7139_WD_DIS_WATDOG_DISABLE    1

#define HL7139_WD_TMR_MASK              0x7
#define HL7139_WD_TMR_SHIFT             0
#define HL7139_WD_TMR_200MS             0
#define HL7139_WD_TMR_500MS             1
#define HL7139_WD_TMR_1000MS            2
#define HL7139_WD_TMR_2000MS            3
#define HL7139_WD_TMR_5000MS            4
#define HL7139_WD_TMR_10000MS           5
#define HL7139_WD_TMR_20000MS           6
#define HL7139_WD_TMR_40000MS           7

/* Register 15h */
#define HL7139_CTRL3                    0x15
#define HL7139_DEV_MODE_MASK            0x80
#define HL7139_DEV_MODE_SHIFT           7
#define HL7139_DEV_MODE_CP_MODE         0
#define HL7139_DEV_MODE_BP_MODE         1

#define HL7139_DPDM_CFG_MASK            0x4
#define HL7139_DPDM_CFG_SHIFT           2
#define HL7139_DPDM_CFG_DPDM            0
#define HL7139_DPDM_CFG_SYNC_TS         1

#define HL7139_GPP_CFG_MASK             0x3
#define HL7139_GPP_CFG_SHIFT            0
#define HL7139_GPP_CFG_NULL             0
#define HL7139_GPP_CFG_SNSP             1
#define HL7139_GPP_CFG_SYNC             2

/* Register 16h */
#define HL7139_TRACK_OV_UV              0x16
#define HL7139_TRACK_OV_DIS_MASK        0x80
#define HL7139_TRACK_OV_DIS_SHIFT       7
#define HL7139_TRACK_OV_DIS_ENABLE      0
#define HL7139_TRACK_OV_DIS_DISABLE     1

#define HL7139_TRACK_UV_DIS_MASK        0x40
#define HL7139_TRACK_UV_DIS_SHIFT	6
#define HL7139_TRACK_UV_DIS_ENABLE	0
#define HL7139_TRACK_UV_DIS_DISABLE	1

#define HL7139_TRACK_OV_MASK            0x38
#define HL7139_TRACK_OV_SHIFT           3
#define HL7139_TRACK_OV_200             0
#define	HL7139_TRACK_OV_300             1
#define	HL7139_TRACK_OV_400             2
#define	HL7139_TRACK_OV_500             3
#define	HL7139_TRACK_OV_600             4
#define	HL7139_TRACK_OV_700             5
#define	HL7139_TRACK_OV_800             6
#define	HL7139_TRACK_OV_900             7

#define HL7139_TRACK_UV_MASK            0x7
#define HL7139_TRACK_UV_SHIFT           0
#define HL7139_TRACK_UV_50              0
#define	HL7139_TRACK_UV_100             1
#define	HL7139_TRACK_UV_150             2
#define	HL7139_TRACK_UV_200             3
#define	HL7139_TRACK_UV_250             4
#define	HL7139_TRACK_UV_300             5
#define	HL7139_TRACK_UV_350             6
#define	HL7139_TRACK_UV_400             7


/* Register 40h */
#define HL7139_ADC_CTRL0                0x40
#define HL7139_ADC_REG_COPY_MASK    	0x80
#define HL7139_ADC_REG_COPY_SHIFT       7
#define HL7139_ADC_REG_COPY_FR          0
#define HL7139_ADC_REG_COPY_MM          1

#define HL7139_ADC_MAN_COPY_MASK        0x40
#define HL7139_ADC_MAN_COPY_SHIFT       6
#define HL7139_ADC_MAN_COPY_NO_CP       0
#define HL7139_ADC_MAN_COPY_CP          1
#define HL7139_ADC_MODE_CFG_MASK        0x18
#define HL7139_ADC_MODE_CFG_SHIFT       3
#define HL7139_ADC_MODE_CFG_AUTO        0
#define HL7139_ADC_MODE_CFG_FE          1
#define HL7139_ADC_MODE_CFG_FD          2

#define HL7139_ADC_AVG_TIME_MASK        0x6
#define HL7139_ADC_AVG_TIME_SHIFT       1
#define HL7139_ADC_AVG_TIME_NA          0
#define HL7139_ADC_AVG_TIME_2SA         1
#define HL7139_ADC_AVG_TIME_4SA         2

#define HL7139_ADC_READ_EN_MASK         0x1
#define HL7139_ADC_READ_EN_SHIFT        0
#define HL7139_ADC_READ_EN_ENABLE 	1
#define HL7139_ADC_READ_EN_DISABLE	0


/* Register 41h */
#define HL7139_ADC_CTRL1                0x41
#define HL7139_VIN_ADC_DIS_MASK         0x80
#define HL7139_VIN_ADC_DIS_SHIFT 	7
#define HL7139_VIN_ADC_DIS_ENABLE	0
#define HL7139_VIN_ADC_DIS_DISABLE	1
#define HL7139_ADC_DIS_SHIFT            7
#define HL7139_ADC_DIS_MASK		1
#define HL7139_ADC_CHANNEL_ENABLE       0
#define HL7139_ADC_CHANNEL_DISABLE      1

#define HL7139_IIN_ADC_DIS_MASK 	0x40
#define HL7139_IIN_ADC_DIS_SHIFT        6
#define HL7139_IIN_ADC_DIS_ENABLE       0
#define HL7139_IIN_ADC_DIS_DISABLE    	1

#define HL7139_VBAT_ADC_DIS_MASK    	0x20
#define HL7139_VBAT_ADC_DIS_SHIFT       5
#define HL7139_VBAT_ADC_DIS_ENABLE      0
#define HL7139_VBAT_ADC_DIS_DISABLE    	1

#define HL7139_IBAT_ADC_DIS_MASK    	0x10
#define HL7139_IBAT_ADC_DIS_SHIFT       4
#define HL7139_IBAT_ADC_DIS_ENABLE      0
#define HL7139_IBAT_ADC_DIS_DISABLE    	1

#define HL7139_TS_ADC_DIS_MASK    	0x8
#define HL7139_TS_ADC_DIS_SHIFT         3
#define HL7139_TS_ADC_DIS_ENABLE        0
#define HL7139_TS_ADC_DIS_DISABLE    	1

#define HL7139_TDIE_ADC_DIS_MASK    	0x4
#define HL7139_TDIE_ADC_DIS_SHIFT       2
#define HL7139_TDIE_ADC_DIS_ENABLE      0
#define HL7139_TDIE_ADC_DIS_DISABLE    	1

#define HL7139_VOUT_ADC_DIS_MASK    	0x2
#define HL7139_VOUT_ADC_DIS_SHIFT       1
#define HL7139_VOUT_ADC_DIS_ENABLE      0
#define HL7139_VOUT_ADC_DIS_DISABLE    	1
#define AL7139_GET_ADC_BASE_REG         0x4D
#define AL7139_IBUS_ADC_LSB  		11/10 //mA
#define AL7139_VBUS_ADC_LSB   		4  //mV
#define AL7139_VTS_ADC_LSB    		44/100 //mV
#define AL7139_VOUT_ADC_LSB   		125/100 //mV
#define AL7139_VBAT_ADC_LSB  		125/100 //mV
#define AL7139_IBAT_ADC_LSB   		22/10  //mA
#define AL7139_TDIE_ADC_LSB   		1/16   //℃
/*i2c regmap init setting */
#define REG_MAX         		0x4F
#define HL7139_I2C_NAME "hl7139"
#define HL7139_MODULE_VERSION "1.0.01.05042022"

/* LOG BUFFER for RPI */
#define LOG_BUFFER_ENTRIES	            2048 //1024
#define LOG_BUFFER_ENTRY_SIZE	            128

#define MASK2SHIFT(_mask)           __ffs(_mask)

/* PD Message Voltage and Current Step */
#define PD_MSG_TA_VOL_STEP                  20000 //20mV
#define PD_MSG_TA_CUR_STEP                  50000 //50mA

/* pre cc mode ta-vol step*/
#define HL7139_TA_VOL_STEP_PRE_CC           100000 //100mV
#define HL7139_TA_VOL_STEP_PRE_CV           20000 //20mV
/* TA OFFSET V */
#define HL7139_TA_V_OFFSET                  200000 //200mV

/******************************************************************************************/
/****************************  TA Voltage/Current setting  ********************************/
/******************************************************************************************/
/* Maximum TA voltage threshold */
#define HL7139_TA_MAX_VOL                   9800000 //uV
/* Maximum TA current threshold */
#define HL7139_TA_MAX_CUR                   2450000 //vA
/* Mimimum TA current threshold */
#define HL7139_TA_MIN_CUR                   1000000 //1000000uA - PPS minimum current 
/* Minimum TA voltage threshold in Preset mode */        
#define HL7139_TA_MIN_VOL_PRESET            7500000 //7.5V
/* Preset TA VOLtage offset*/
#define HL7139_TA_VOL_PRE_OFFSET            300000 //300mA

/* Denominator for TA Voltage and Current setting*/
#define HL7139_SEC_DENOM_U_M                1000 // 1000, denominator
/* PPS request Delay Time */
#define HL7139_PPS_PERIODIC_T	            7500 //ms


/******************************************************************************************/
/*************************** ADC STEP For register setting ********************************/
/******************************************************************************************/
/* ADC CTRL 0 Deafult setting , 4 samples, auto mode, ADC_READ_EN is 1 */
#define HL7139_ADC0_DFT                     0x05
/* VIN ADC STEP for Register setting */
#define HL7139_VIN_ADC_STEP                 3940 //3.94mV
/* IIN ADC STEP for register setting */
#define HL7139_BP_IIN_ADC_STEP              2150 //2.15mA for BP mode
#define HL7139_CP_IIN_ADC_STEP              1100 //1.1mA for CP mode
/* VBAT ADC STEP for register setting */
#define HL7139_VBAT_ADC_STEP                1250 //1.25mA
/* IBAT ADC STEP for Register setting */
#define HL7139_IBAT_ADC_STEP                2200 //2.2mA
/* VTS ADC STEP for register setting */
#define HL7139_VTS_ADC_STEP                 440 //0.44mA
/* VOUT ADC STEP for register setting */
#define HL7139_VOUT_ADC_STEP                1250 //1.25mA
/* TDIE ADC STEP for register setting */
#define HL7139_TDIE_ADC_STEP                25 //0.25mA

/******************************************************************************************/
/************************ HL7139 VBAT REG/IIN/TOPOFF setting ******************************/
/******************************************************************************************/
/* VBAT REGULATION VOLTAGE */
#define HL7139_VBAT_REG_DFT                 4350000 //4.35V
/* VBAT-REG MINIMUM */
#define HL7139_VBAT_REG_MIN                 4000000 //4V
/* TOPOFF Current */
#define HL7139_IIN_DONE_DFT                 500000 //500mA
/* INPUT CURRENT LIMIT CP MODE DEFAULT SETTING */
#define HL7139_CP_IIN_CFG_DFT               2000000 //2A
#define HL7139_CP_IIN_CFG_MIN               1000000 //1A
#define HL7139_CP_IIN_CFG_MAX               3500000 //3.5A
/* INPUT CURRENT LIMIT BP MODE DEFAULT SETTING */
#define HL7139_BP_IIN_CFG_DFT               4000000 //4A
#define HL7139_BP_IIN_CFG_MIN               2000000 //2A
#define HL7139_BP_IIN_CFG_MAX               7000000 //7.5A

/* One Step iin_reg for CP Mode */
#define HL7139_CP_IIN_CFG_STEP              50000 //50mA
/* One Step iin_reg for BP Mode */
#define HL7139_BP_IIN_CFG_STEP              100000 //100mA
#define HL7139_CP_IIN_TO_HEX(__iin)         ((__iin - 1000000)/HL7139_CP_IIN_CFG_STEP) //1A + DEC (5:0) * 50mA
#define HL7139_BP_IIN_TO_HEX(__iin)         ((__iin - 2000000)/HL7139_BP_IIN_CFG_STEP) //2A + DEC (5:0) * 100mA
/*input current limit threshold offset*/
#define HL7139_IIN_OFFSET_CUR               100000 //100mA

/*one step -> 40mA in Adjust mode */
#define HL7139_TA_VOL_STEP_ADJ_CC           40000 //40mA

/*Battery setting */
#define HL7139_DC_VBAT_MIN                  3600000 //3.6V

/* TS0, TS1  Threshold setting, Customer should change those value */
#define HL7139_TS0_TH_DFT                   0x0174
#define HL7139_TS1_TH_DFT                   0x0289

/******************************************************************************************/
/************************  HL7139 work queue delay setting   *****************************/
/******************************************************************************************/
/* Workqueue delay time for VBATMIN */
#define HL7139_VBATMIN_CHECK_T              1000
/* Workqueue delay time for CHECK ACTIVE */
#define HL7139_CHECK_ACTIVE_DELAY_T         150 //ms
/* Workqueue delay time for CCMODE */
#define HL7139_CCMODE_CHECK_T	            10000 //ms
/* Workqueue delay time for CVMODE */
#define HL7139_CVMODE_CHECK_T 	            10000 //ms
/* Delay Time after PDMSG */
#define HL7139_PDMSG_WAIT_T	            200 //ms


/* External FET VGS Voltage Selection , REG0xB VUGS_OVP, VGS_SEL[5:4] */
enum {
	EXT_FET_VGS_7V = 0,
	EXT_FET_VGS_7_5V,
	EXT_FET_VGS_8V,
	EXT_FET_VGS_8_5V,
};
enum {
	ADC_VIN = 0,
	ADC_IIN,
	ADC_VBAT,
	ADC_IBAT,
	ADC_TS,
	ADC_TDIE,
	ADC_VOUT,
	ADC_MAX_NUM,
};

/* VBUS OVP TH, REG0x0B VBUS_OVP_TH[3:0] */
enum {
	VBUS_OVP_TH_4V = 0,
	VBUS_OVP_TH_5V,
	VBUS_OVP_TH_6V,
	VBUS_OVP_TH_7V,
	VBUS_OVP_TH_8V,
	VBUS_OVP_TH_9V,
	VBUS_OVP_TH_10V,
	VBUS_OVP_TH_11V,
	VBUS_OVP_TH_12V, 
	VBUS_OVP_TH_13V, 
	VBUS_OVP_TH_14V,
	VBUS_OVP_TH_15V,
	VBUS_OVP_TH_16V,
	VBUS_OVP_TH_17V,
	VBUS_OVP_TH_18V,
	VBUS_OVP_TH_19V,    
};

/* Switching Frequency setting */
enum {
	FSW_CFG_500KHZ = 0,  
	FSW_CFG_600KHZ,
	FSW_CFG_700KHZ,
	FSW_CFG_800KHZ,
	FSW_CFG_900KHZ,
	FSW_CFG_1000KHZ,
	FSW_CFG_1100KHZ,
	FSW_CFG_1200KHZ,
	FSW_CFG_1300KHZ,
	FSW_CFG_1400KHZ,
	FSW_CFG_1500KHZ,
	FSW_CFG_1600KHZ,
};


/* Watchdog Timer */
enum {
	WD_TMR_200MS = 0,
	WD_TMR_500MS,
	WD_TMR_1S,
	WD_TMR_2S,
	WD_TMR_5S,
	WD_TMR_10S,
	WD_TMR_20S,
	WD_TMR_40S,
};

/* Increment check for Adjust MODE */
enum {
	INC_NONE, 
	INC_TA_VOL,
	INC_TA_CUR,
};

/* TIMER ID for Charging Mode */
enum {
	TIMER_ID_NONE = 0,
	TIMER_VBATMIN_CHECK,
	TIMER_PRESET_DC,
	TIMER_PRESET_CONFIG,
	TIMER_CHECK_ACTIVE,
	TIMER_ADJUST_CCMODE,
	TIMER_ENTER_CCMODE,
	TIMER_CHECK_CCMODE,
	TIMER_CHECK_CVMODE,
	TIMER_PDMSG_SEND,
	TIMER_NEW_TARGET_VBAT_REG,
	TIMER_NEW_TARGET_IIN,
};

/* DC STATE for matching Timer ID*/
enum {
	DC_STATE_NOT_CHARGING = 0,
	DC_STATE_CHECK_VBAT,
	DC_STATE_PRESET_DC,
	DC_STATE_CHECK_ACTIVE,
	DC_STATE_ADJUST_CC,
	DC_STATE_START_CC,
	DC_STATE_CC_MODE,
	DC_STATE_CV_MODE,
	DC_STATE_CHARGING_DONE,
};

/* DEV MODE */
enum {
	CP_MODE = 0,
	BP_MODE,
};

/* PD Message Type */
enum {
	PD_MSG_REQUEST_APDO,
	PD_MSG_REQUEST_FIXED_PDO,
};

/* Regulation Loop  */
enum {
	INACTIVE_LOOP    = 0,
	VBAT_REG_LOOP    = 1,
	IIN_REG_LOOP     = 2,
	IBAT_REG_LOOP    = 4,
	T_DIE_REG_LOOP   = 8,
};

/* Device State Status */
enum {
	DEV_STATE_SHUTDOWN = 0,
	DEV_STATE_STANDBY,
	DEV_STATE_CP_ACTIVE,
	DEV_STATE_BP_ACTIVE,
};


/*REGMAP CONFIG */
static const struct regmap_config hl7139_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = REG_MAX,
};

/* Power Supply Name */
static char *hl7139_supplied_to[] = {
	"hl7139-charger",
};

/*PLATFORM DATA */
struct hl7139_platform_data{
	int irq_gpio;

	unsigned int iin_cfg;
	unsigned int ichfg_cfg;
	unsigned int vbat_reg;
	unsigned int iin_topoff;
	unsigned int vbat_reg_max;

	unsigned int snsres;
	unsigned int fsw_cfg;
	unsigned int ntc_th;
	unsigned int wd_tmr;
	unsigned int ts0_th;
	unsigned int ts1_th;
	unsigned int adc_ctrl0;

	bool bat_ovp_disable;
	bool bat_ocp_disable;
	bool vin_ovp_disable;
	bool iin_ocp_disable;
	bool vout_ovp_disable;

	bool ts_prot_en;
	bool all_auto_recovery; 
	bool wd_dis;

	int bat_ovp_th;
	int bat_ocp_th;
	int ac_ovp_th;
	int bus_ovp_th;
};

/* DEV - CHIP DATA*/
struct hl7139_charger{
	struct regmap                   *regmap;
	struct device                   *dev;
	struct power_supply             *psy_chg;
	struct mutex                    i2c_lock;
	struct mutex                    lock;
	struct wakeup_source            wake_lock;
	struct hl7139_platform_data     *pdata; 
	struct i2c_client               *client;
	struct delayed_work             timer_work;
	struct delayed_work             pps_work;
	struct power_supply_desc psy_desc;
	struct power_supply_config psy_cfg;
	struct power_supply *fc2_psy;
#ifdef HL7139_STEP_CHARGING    
	struct delayed_work             step_work;
	unsigned int current_step;
#endif    

#ifdef HL7139_ENABLE_WATCHDOG_TIMER    
	/*WDT setting*/
	struct delayed_work             wdt_work;
	bool                            wdt_en;
#endif

	struct dentry                   *debug_root;

	u32  debug_address;

	unsigned int chg_state;
	unsigned int reg_state;

	unsigned int charging_state;
	unsigned int dc_state;
	unsigned int timer_id;
	unsigned long timer_period;

	int vbus_error;
	int charger_mode;
	int direct_charge;

	/* ADC CON */ 
	int adc_vin;
	int adc_iin;
	int adc_vbat;
	int adc_ibat;
	int adc_vts;
	int adc_tdie;
	int adc_vout;

	unsigned int iin_cc;
	unsigned int ta_cur;
	unsigned int ta_vol;
	unsigned int ta_objpos;
	unsigned int ta_target_vol;
	unsigned int ta_v_ofs;

	unsigned int new_vbat_reg;
	unsigned int new_iin;
	bool         req_new_vbat_reg;
	bool         req_new_iin;

	unsigned int ta_max_cur;
	unsigned int ta_max_vol;
	unsigned int ta_max_pwr;

	unsigned int ret_state;

	unsigned int prev_iin;  
	unsigned int prev_inc;
	bool         prev_dec;

	int dev_mode;

	bool usb_present;
    	bool charger_enable;
	bool vbus_present;
	bool hl7139_suspend_flag;
	unsigned int pdo_index;
	unsigned int pdo_max_voltage;
	unsigned int pdo_max_current;

	bool online;
	struct notifier_block pm_nb;
#ifdef CONFIG_DEBUG_FS
	struct dentry *dentry;
	/* lock for log buffer access */
	struct mutex logbuffer_lock;
	int logbuffer_head;
	int logbuffer_tail;
	u8 *logbuffer[LOG_BUFFER_ENTRIES];
#endif

};
extern void usbqc_suspend_notifier(bool suspend);
#endif
