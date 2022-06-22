#ifndef _AW9610X_REG_H_
#define _AW9610X_REG_H_

/********************************************
* Register List
********************************************/

#define AFE_BASE_ADDR					(0x0000)
#define DSP_BASE_ADDR					(0x0000)
#define STAT_BASE_ADDR					(0x0000)
#define SFR_BASE_ADDR					(0x0000)
#define DATA_BASE_ADDR					(0x0000)
/* registers list */
//AW96105A_UI_REGISTER_AFE MAP
#define REG_SCANCTRL0					((0x0000) + AFE_BASE_ADDR)
#define REG_SCANCTRL1					((0x0004) + AFE_BASE_ADDR)
#define REG_AFECFG0_CH0					((0x0010) + AFE_BASE_ADDR)
#define REG_AFECFG1_CH0					((0x0014) + AFE_BASE_ADDR)
#define REG_AFECFG3_CH0					((0x001C) + AFE_BASE_ADDR)
#define REG_AFECFG4_CH0					((0x0020) + AFE_BASE_ADDR)
#define REG_AFECFG0_CH1					((0x0024) + AFE_BASE_ADDR)
#define REG_AFECFG1_CH1					((0x0028) + AFE_BASE_ADDR)
#define REG_AFECFG3_CH1					((0x0030) + AFE_BASE_ADDR)
#define REG_AFECFG4_CH1					((0x0034) + AFE_BASE_ADDR)
#define REG_AFECFG0_CH2					((0x0038) + AFE_BASE_ADDR)
#define REG_AFECFG1_CH2					((0x003C) + AFE_BASE_ADDR)
#define REG_AFECFG3_CH2					((0x0044) + AFE_BASE_ADDR)
#define REG_AFECFG4_CH2					((0x0048) + AFE_BASE_ADDR)
#define REG_AFECFG0_CH3					((0x004C) + AFE_BASE_ADDR)
#define REG_AFECFG1_CH3					((0x0050) + AFE_BASE_ADDR)
#define REG_AFECFG3_CH3					((0x0058) + AFE_BASE_ADDR)
#define REG_AFECFG4_CH3					((0x005C) + AFE_BASE_ADDR)
#define REG_AFECFG0_CH4					((0x0060) + AFE_BASE_ADDR)
#define REG_AFECFG1_CH4					((0x0064) + AFE_BASE_ADDR)
#define REG_AFECFG3_CH4					((0x006C) + AFE_BASE_ADDR)
#define REG_AFECFG4_CH4					((0x0070) + AFE_BASE_ADDR)
#define REG_AFECFG0_CH5					((0x0074) + AFE_BASE_ADDR)
#define REG_AFECFG1_CH5					((0x0078) + AFE_BASE_ADDR)
#define REG_AFECFG3_CH5					((0x0080) + AFE_BASE_ADDR)
#define REG_AFECFG4_CH5					((0x0084) + AFE_BASE_ADDR)

/* registers list */
//AW96105A_UI_REGISTER_DSP MAP
#define REG_DSPCFG0_CH0					((0x00A0) + DSP_BASE_ADDR)
#define REG_DSPCFG1_CH0					((0x00A4) + DSP_BASE_ADDR)
#define REG_BLFILT_CH0					((0x00A8) + DSP_BASE_ADDR)
#define REG_PROXCTRL_CH0				((0x00B0) + DSP_BASE_ADDR)
#define REG_BLRSTRNG_CH0				((0x00B4) + DSP_BASE_ADDR)
#define REG_PROXTH0_CH0					((0x00B8) + DSP_BASE_ADDR)
#define REG_PROXTH1_CH0					((0x00BC) + DSP_BASE_ADDR)
#define REG_PROXTH2_CH0					((0x00C0) + DSP_BASE_ADDR)
#define REG_PROXTH3_CH0					((0x00C4) + DSP_BASE_ADDR)
#define REG_STDDET_CH0					((0x00C8) + DSP_BASE_ADDR)
#define REG_INITPROX0_CH0				((0x00CC) + DSP_BASE_ADDR)
#define REG_INITPROX1_CH0				((0x00D0) + DSP_BASE_ADDR)
#define REG_DATAOFFSET_CH0				((0x00D4) + DSP_BASE_ADDR)
#define REG_AOTTAR_CH0					((0x00D8) + DSP_BASE_ADDR)
#define REG_DSPCFG0_CH1					((0x00DC) + DSP_BASE_ADDR)
#define REG_DSPCFG1_CH1					((0x00E0) + DSP_BASE_ADDR)
#define REG_BLFILT_CH1					((0x00E4) + DSP_BASE_ADDR)
#define REG_PROXCTRL_CH1				((0x00EC) + DSP_BASE_ADDR)
#define REG_BLRSTRNG_CH1				((0x00F0) + DSP_BASE_ADDR)
#define REG_PROXTH0_CH1					((0x00F4) + DSP_BASE_ADDR)
#define REG_PROXTH1_CH1					((0x00F8) + DSP_BASE_ADDR)
#define REG_PROXTH2_CH1					((0x00FC) + DSP_BASE_ADDR)
#define REG_PROXTH3_CH1					((0x0100) + DSP_BASE_ADDR)
#define REG_STDDET_CH1					((0x0104) + DSP_BASE_ADDR)
#define REG_INITPROX0_CH1				((0x0108) + DSP_BASE_ADDR)
#define REG_INITPROX1_CH1				((0x010C) + DSP_BASE_ADDR)
#define REG_RAWOFFSET_CH1				((0x0110) + DSP_BASE_ADDR)
#define REG_AOTTAR_CH1					((0x0114) + DSP_BASE_ADDR)
#define REG_DSPCFG0_CH2					((0x0118) + DSP_BASE_ADDR)
#define REG_DSPCFG1_CH2					((0x011C) + DSP_BASE_ADDR)
#define REG_BLFILT_CH2					((0x0120) + DSP_BASE_ADDR)
#define REG_PROXCTRL_CH2				((0x0128) + DSP_BASE_ADDR)
#define REG_BLRSTRNG_CH2				((0x012C) + DSP_BASE_ADDR)
#define REG_PROXTH0_CH2					((0x0130) + DSP_BASE_ADDR)
#define REG_PROXTH1_CH2					((0x0134) + DSP_BASE_ADDR)
#define REG_PROXTH2_CH2					((0x0138) + DSP_BASE_ADDR)
#define REG_PROXTH3_CH2					((0x013C) + DSP_BASE_ADDR)
#define REG_STDDET_CH2					((0x0140) + DSP_BASE_ADDR)
#define REG_INITPROX0_CH2				((0x0144) + DSP_BASE_ADDR)
#define REG_INITPROX1_CH2				((0x0148) + DSP_BASE_ADDR)
#define REG_DATAOFFSET_CH2				((0x014C) + DSP_BASE_ADDR)
#define REG_AOTTAR_CH2					((0x0150) + DSP_BASE_ADDR)
#define REG_DSPCFG0_CH3					((0x0154) + DSP_BASE_ADDR)
#define REG_DSPCFG1_CH3					((0x0158) + DSP_BASE_ADDR)
#define REG_BLFILT_CH3					((0x015C) + DSP_BASE_ADDR)
#define REG_PROXCTRL_CH3				((0x0164) + DSP_BASE_ADDR)
#define REG_BLRSTRNG_CH3				((0x0168) + DSP_BASE_ADDR)
#define REG_PROXTH0_CH3					((0x016C) + DSP_BASE_ADDR)
#define REG_PROXTH1_CH3					((0x0170) + DSP_BASE_ADDR)
#define REG_PROXTH2_CH3					((0x0174) + DSP_BASE_ADDR)
#define REG_PROXTH3_CH3					((0x0178) + DSP_BASE_ADDR)
#define REG_STDDET_CH3					((0x017C) + DSP_BASE_ADDR)
#define REG_INITPROX0_CH3				((0x0180) + DSP_BASE_ADDR)
#define REG_INITPROX1_CH3				((0x0184) + DSP_BASE_ADDR)
#define REG_DATAOFFSET_CH3				((0x0188) + DSP_BASE_ADDR)
#define REG_AOTTAR_CH3					((0x018C) + DSP_BASE_ADDR)
#define REG_DSPCFG0_CH4					((0x0190) + DSP_BASE_ADDR)
#define REG_DSPCFG1_CH4					((0x0194) + DSP_BASE_ADDR)
#define REG_BLFILT_CH4					((0x0198) + DSP_BASE_ADDR)
#define REG_PROXCTRL_CH4				((0x01A0) + DSP_BASE_ADDR)
#define REG_BLRSTRNG_CH4				((0x01A4) + DSP_BASE_ADDR)
#define REG_PROXTH0_CH4					((0x01A8) + DSP_BASE_ADDR)
#define REG_PROXTH1_CH4					((0x01AC) + DSP_BASE_ADDR)
#define REG_PROXTH2_CH4					((0x01B0) + DSP_BASE_ADDR)
#define REG_PROXTH3_CH4					((0x01B4) + DSP_BASE_ADDR)
#define REG_STDDET_CH4					((0x01B8) + DSP_BASE_ADDR)
#define REG_INITPROX0_CH4				((0x01BC) + DSP_BASE_ADDR)
#define REG_INITPROX1_CH4				((0x01C0) + DSP_BASE_ADDR)
#define REG_DATAOFFSET_CH4				((0x01C4) + DSP_BASE_ADDR)
#define REG_AOTTAR_CH4					((0x01C8) + DSP_BASE_ADDR)
#define REG_DSPCFG0_CH5					((0x01CC) + DSP_BASE_ADDR)
#define REG_DSPCFG1_CH5					((0x01D0) + DSP_BASE_ADDR)
#define REG_BLFILT_CH5					((0x01D4) + DSP_BASE_ADDR)
#define REG_PROXCTRL_CH5				((0x01DC) + DSP_BASE_ADDR)
#define REG_BLRSTRNG_CH5				((0x01E0) + DSP_BASE_ADDR)
#define REG_PROXTH0_CH5					((0x01E4) + DSP_BASE_ADDR)
#define REG_PROXTH1_CH5					((0x01E8) + DSP_BASE_ADDR)
#define REG_PROXTH2_CH5					((0x01EC) + DSP_BASE_ADDR)
#define REG_PROXTH3_CH5					((0x01F0) + DSP_BASE_ADDR)
#define REG_STDDET_CH5					((0x01F4) + DSP_BASE_ADDR)
#define REG_INITPROX0_CH5				((0x01F8) + DSP_BASE_ADDR)
#define REG_INITPROX1_CH5				((0x01FC) + DSP_BASE_ADDR)
#define REG_DATAOFFSET_CH5				((0x0200) + DSP_BASE_ADDR)
#define REG_AOTTAR_CH5					((0x0204) + DSP_BASE_ADDR)
#define REG_ATCCR0						((0x0208) + DSP_BASE_ADDR)
#define REG_ATCCR1						((0x020C) + DSP_BASE_ADDR)

/* registers list */
//AW96105A_UI_REGISTER_STAT MAP
#define REG_FWVER						((0x0088) + STAT_BASE_ADDR)
#define REG_WST							((0x008C) + STAT_BASE_ADDR)
#define REG_STAT0						((0x0090) + STAT_BASE_ADDR)
#define REG_STAT1						((0x0094) + STAT_BASE_ADDR)
#define REG_STAT2						((0x0098) + STAT_BASE_ADDR)
#define REG_CHINTEN						((0x009C) + STAT_BASE_ADDR)

/* registers list */
//AW96105A_UI_REGISTER_SFR MAP
#define REG_CMD							((0xF008) + SFR_BASE_ADDR)
#define REG_IRQSRC						((0xF080) + SFR_BASE_ADDR)
#define REG_IRQEN						((0xF084) + SFR_BASE_ADDR)
#define REG_I2CADDR						((0xF0F0) + SFR_BASE_ADDR)
#define REG_OSCEN						((0xFF00) + SFR_BASE_ADDR)
#define REG_RESET						((0xFF0C) + SFR_BASE_ADDR)
#define REG_CHIPID						((0xFF10) + SFR_BASE_ADDR)

/* registers list */
//AW96105A_UI_REGISTER_DATA MAP
#define REG_COMP_CH0					((0x0210) + DATA_BASE_ADDR)
#define REG_COMP_CH1					((0x0214) + DATA_BASE_ADDR)
#define REG_COMP_CH2					((0x0218) + DATA_BASE_ADDR)
#define REG_COMP_CH3					((0x021C) + DATA_BASE_ADDR)
#define REG_COMP_CH4					((0x0220) + DATA_BASE_ADDR)
#define REG_COMP_CH5					((0x0224) + DATA_BASE_ADDR)
#define REG_BASELINE_CH0				((0x0228) + DATA_BASE_ADDR)
#define REG_BASELINE_CH1				((0x022C) + DATA_BASE_ADDR)
#define REG_BASELINE_CH2				((0x0230) + DATA_BASE_ADDR)
#define REG_BASELINE_CH3				((0x0234) + DATA_BASE_ADDR)
#define REG_BASELINE_CH4				((0x0238) + DATA_BASE_ADDR)
#define REG_BASELINE_CH5				((0x023C) + DATA_BASE_ADDR)
#define REG_DIFF_CH0					((0x0240) + DATA_BASE_ADDR)
#define REG_DIFF_CH1					((0x0244) + DATA_BASE_ADDR)
#define REG_DIFF_CH2					((0x0248) + DATA_BASE_ADDR)
#define REG_DIFF_CH3					((0x024C) + DATA_BASE_ADDR)
#define REG_DIFF_CH4					((0x0250) + DATA_BASE_ADDR)
#define REG_DIFF_CH5					((0x0254) + DATA_BASE_ADDR)
#define REG_FWVER2						((0x0410) + DATA_BASE_ADDR)

#define REG_EEDA0			0x0408
#define REG_EEDA1			0x040C

struct aw_reg_data {
	unsigned char rw;
	unsigned short reg;
};
/********************************************
 * Register Access
 *******************************************/
#define REG_NONE_ACCESS					(0)
#define REG_RD_ACCESS					(1 << 0)
#define REG_WR_ACCESS					(1 << 1)

static const struct aw_reg_data aw9610x_reg_access[] = {
	//AFE MAP
	{ .reg = REG_SCANCTRL0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_SCANCTRL1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG0_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG1_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG3_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG4_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG0_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG1_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG3_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG4_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG0_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG1_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG3_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG4_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG0_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG1_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG3_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG4_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG0_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG1_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG3_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG4_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG0_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG1_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG3_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AFECFG4_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },

	//DSP MAP
	{ .reg = REG_DSPCFG0_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG1_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLFILT_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXCTRL_CH0,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLRSTRNG_CH0,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH0_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH1_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH2_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH3_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_STDDET_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX0_CH0,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX1_CH0,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DATAOFFSET_CH0,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AOTTAR_CH0,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG0_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG1_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLFILT_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXCTRL_CH1,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLRSTRNG_CH1,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH0_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH1_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH2_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH3_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_STDDET_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX0_CH1,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX1_CH1,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_RAWOFFSET_CH1,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AOTTAR_CH1,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG0_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG1_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLFILT_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXCTRL_CH2,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLRSTRNG_CH2,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH0_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH1_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH2_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH3_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_STDDET_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX0_CH2,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX1_CH2,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DATAOFFSET_CH2,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AOTTAR_CH2,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG0_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG1_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLFILT_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXCTRL_CH3,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLRSTRNG_CH3,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH0_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH1_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH2_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH3_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_STDDET_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX0_CH3,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX1_CH3,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DATAOFFSET_CH3,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AOTTAR_CH3,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG0_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG1_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLFILT_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXCTRL_CH4,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLRSTRNG_CH4,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH0_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH1_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH2_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH3_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_STDDET_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX0_CH4,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX1_CH4,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DATAOFFSET_CH4,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AOTTAR_CH4,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG0_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DSPCFG1_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLFILT_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXCTRL_CH5,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_BLRSTRNG_CH5,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH0_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH1_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH2_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_PROXTH3_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_STDDET_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX0_CH5,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_INITPROX1_CH5,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_DATAOFFSET_CH5,		.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_AOTTAR_CH5,			.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_ATCCR0,				.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_ATCCR1,				.rw = REG_RD_ACCESS | REG_WR_ACCESS, },

	//STAT MAP
	{ .reg = REG_FWVER,				.rw = REG_RD_ACCESS, },
	{ .reg = REG_WST,					.rw = REG_RD_ACCESS, },
	{ .reg = REG_STAT0,				.rw = REG_RD_ACCESS, },
	{ .reg = REG_STAT1,				.rw = REG_RD_ACCESS, },
	{ .reg = REG_STAT2,				.rw = REG_RD_ACCESS, },
	{ .reg = REG_CHINTEN,				.rw = REG_RD_ACCESS | REG_WR_ACCESS, },

	//SFR MAP
	{ .reg = REG_CMD,					.rw = REG_NONE_ACCESS, },
	{ .reg = REG_IRQSRC,				.rw = REG_RD_ACCESS, },
	{ .reg = REG_IRQEN,				.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_I2CADDR,				.rw = REG_RD_ACCESS, },
	{ .reg = REG_OSCEN,				.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_RESET,				.rw = REG_RD_ACCESS | REG_WR_ACCESS, },
	{ .reg = REG_CHIPID,				.rw = REG_RD_ACCESS, },

	//DATA MAP
	{ .reg = REG_COMP_CH0,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_COMP_CH1,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_COMP_CH2,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_COMP_CH3,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_COMP_CH4,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_COMP_CH5,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_BASELINE_CH0,		.rw = REG_RD_ACCESS, },
	{ .reg = REG_BASELINE_CH1,		.rw = REG_RD_ACCESS, },
	{ .reg = REG_BASELINE_CH2,		.rw = REG_RD_ACCESS, },
	{ .reg = REG_BASELINE_CH3,		.rw = REG_RD_ACCESS, },
	{ .reg = REG_BASELINE_CH4,		.rw = REG_RD_ACCESS, },
	{ .reg = REG_BASELINE_CH5,		.rw = REG_RD_ACCESS, },
	{ .reg = REG_DIFF_CH0,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_DIFF_CH1,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_DIFF_CH2,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_DIFF_CH3,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_DIFF_CH4,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_DIFF_CH5,			.rw = REG_RD_ACCESS, },
	{ .reg = REG_FWVER2,				.rw = REG_RD_ACCESS, },

};
#endif
