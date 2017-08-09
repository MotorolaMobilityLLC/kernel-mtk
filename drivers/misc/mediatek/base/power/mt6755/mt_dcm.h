#ifndef __MT_DCM_H__
#define __MT_DCM_H__

void mt_dcm_emi_1pll_mode(void);
void mt_dcm_emi_off(void);
void mt_dcm_emi_3pll_mode(void);

void mt_dcm_disable(void);
void mt_dcm_restore(void);
void mt_dcm_topckg_disable(void);
void mt_dcm_topckg_enable(void);
void mt_dcm_topck_off(void);
void mt_dcm_topck_on(void);
void mt_dcm_peri_off(void);
void mt_dcm_peri_on(void);

int sync_dcm_set_cci_freq(unsigned int cci);
int sync_dcm_set_mp0_freq(unsigned int mp0);
int sync_dcm_set_mp1_freq(unsigned int mp1);

#endif /* #ifndef __MT_DCM_H__ */

