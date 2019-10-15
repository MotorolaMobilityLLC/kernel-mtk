/*****************************************************************************
*
* Filename:
* ---------
*   bq24157.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   bq24157 header file
*
* Author:
* -------
*
****************************************************************************/

#ifndef _bq24157_SW_H_
#define _bq24157_SW_H_

#define bq24157_CON0      0x00
#define bq24157_CON1      0x01
#define bq24157_CON2      0x02
#define bq24157_CON3      0x03
#define bq24157_CON4      0x04
#define bq24157_CON5      0x05
#define bq24157_CON6      0x06
#define bq24157_REG_NUM 7

/**********************************************************
  *
  *   [MASK/SHIFT]
  *
  *********************************************************/
/* CON0 */
#define CON0_TMR_RST_MASK   0x01
#define CON0_TMR_RST_SHIFT  7

#define CON0_OTG_MASK       0x01
#define CON0_OTG_SHIFT      7

#define CON0_EN_STAT_MASK   0x01
#define CON0_EN_STAT_SHIFT  6

#define CON0_STAT_MASK      0x03
#define CON0_STAT_SHIFT     4

#define CON0_BOOST_MASK     0x01
#define CON0_BOOST_SHIFT    3

#define CON0_FAULT_MASK     0x07
#define CON0_FAULT_SHIFT    0

/* CON1 */
#define CON1_LIN_LIMIT_MASK     0x03
#define CON1_LIN_LIMIT_SHIFT    6

#define CON1_LOW_V_MASK     0x03
#define CON1_LOW_V_SHIFT    4

#define CON1_TE_MASK        0x01
#define CON1_TE_SHIFT       3

#define CON1_CE_MASK        0x01
#define CON1_CE_SHIFT       2

#define CON1_HZ_MODE_MASK   0x01
#define CON1_HZ_MODE_SHIFT  1

#define CON1_OPA_MODE_MASK  0x01
#define CON1_OPA_MODE_SHIFT 0

/* CON2 */
#define CON2_OREG_MASK    0x3F
#define CON2_OREG_SHIFT   2

#define CON2_OTG_PL_MASK    0x01
#define CON2_OTG_PL_SHIFT   1

#define CON2_OTG_EN_MASK    0x01
#define CON2_OTG_EN_SHIFT   0

/* CON3 */
#define CON3_VENDER_CODE_MASK   0x07
#define CON3_VENDER_CODE_SHIFT  5

#define CON3_PIN_MASK           0x03
#define CON3_PIN_SHIFT          3

#define CON3_REVISION_MASK      0x07
#define CON3_REVISION_SHIFT     0

/* CON4 */
#define CON4_RESET_MASK     0x01
#define CON4_RESET_SHIFT    7

#define CON4_I_CHR_MASK     0x07
#define CON4_I_CHR_SHIFT    4

#define CON4_I_TERM_MASK    0x07
#define CON4_I_TERM_SHIFT   0

/* CON5 */
#define CON5_DIS_VREG_MASK      0x01
#define CON5_DIS_VREG_SHIFT     6

#define CON5_IO_LEVEL_MASK      0x01
#define CON5_IO_LEVEL_SHIFT     5

#define CON5_SP_STATUS_MASK     0x01
#define CON5_SP_STATUS_SHIFT    4

#define CON5_EN_LEVEL_MASK      0x01
#define CON5_EN_LEVEL_SHIFT     3

#define CON5_VSP_MASK           0x07
#define CON5_VSP_SHIFT          0

/* CON6 */
#define CON6_ISAFE_MASK     0x07
#define CON6_ISAFE_SHIFT    4

#define CON6_VSAFE_MASK     0x0F
#define CON6_VSAFE_SHIFT    0

struct bq24157_platform_data {
	const char *chg_name;
	u32 ichg;
	u32 aicr;
	u32 mivr;
	u32 ieoc;
	u32 voreg;
	u32 vmreg;
	int intr_gpio;
	u8 enable_te:1;
	u8 enable_eoc_shdn:1;
};

/**********************************************************
  *
  *   [Extern Function]
  *
  *********************************************************/
/* CON0---------------------------------------------------- */
extern void bq24157_set_tmr_rst(unsigned int val);
extern unsigned int bq24157_get_otg_status(void);
extern void bq24157_set_en_stat(unsigned int val);
extern unsigned int bq24157_get_chip_status(void);
extern unsigned int bq24157_get_boost_status(void);
extern unsigned int bq24157_get_fault_status(void);
/* CON1---------------------------------------------------- */
extern void bq24157_set_input_charging_current(unsigned int val);
extern void bq24157_set_v_low(unsigned int val);
extern void bq24157_set_te(unsigned int val);
extern void bq24157_set_ce(unsigned int val);
extern void bq24157_set_hz_mode(unsigned int val);
extern void bq24157_set_opa_mode(unsigned int val);
/* CON2---------------------------------------------------- */
extern void bq24157_set_oreg(unsigned int val);
extern void bq24157_set_otg_pl(unsigned int val);
extern void bq24157_set_otg_en(unsigned int val);
/* CON3---------------------------------------------------- */
extern unsigned int bq24157_get_vender_code(void);
extern unsigned int bq24157_get_pn(void);
extern unsigned int bq24157_get_revision(void);
/* CON4---------------------------------------------------- */
extern void bq24157_set_reset(unsigned int val);
extern void bq24157_set_iocharge(unsigned int val);
extern void bq24157_set_iterm(unsigned int val);
/* CON5---------------------------------------------------- */
extern void bq24157_set_dis_vreg(unsigned int val);
extern void bq24157_set_io_level(unsigned int val);
extern unsigned int bq24157_get_sp_status(void);
extern unsigned int bq24157_get_en_level(void);
extern void bq24157_set_vsp(unsigned int val);
/* CON6---------------------------------------------------- */
extern void bq24157_set_i_safe(unsigned int val);
extern void bq24157_set_v_safe(unsigned int val);
/* --------------------------------------------------------- */
extern void bq24157_dump_register(void);
extern unsigned int bq24157_reg_config_interface(unsigned char RegNum, unsigned char val);

extern unsigned int bq24157_read_interface(unsigned char RegNum, unsigned char *val,
	unsigned char MASK, unsigned char SHIFT);
extern unsigned int bq24157_config_interface(unsigned char RegNum, unsigned char val,
	unsigned char MASK, unsigned char SHIFT);
#endif				/* _bq24157_SW_H_ */
