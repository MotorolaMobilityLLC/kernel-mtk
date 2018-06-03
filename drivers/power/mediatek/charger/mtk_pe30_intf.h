/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __MTK_PE_30_INTF_H
#define __MTK_PE_30_INTF_H

struct mtk_pep30_cap {
	int cur;
	int vol;
	int temp;
};

extern bool mtk_pep30_init(void);
extern bool mtk_is_TA_support_pep30(void);
extern bool mtk_is_pep30_running(void);
extern void mtk_pep30_plugout_reset(void);
extern bool mtk_pep30_check_charger(void);
extern void mtk_pep30_set_pd_rdy(bool rdy);
extern void mtk_pep30_set_charging_current_limit(int cur);
extern int mtk_pep30_get_charging_current_limit(void);
extern void mtk_pep30_end(bool retry);
extern int mtk_cooler_is_abcct_unlimit(void);
extern void chrdet_int_handler(void);


extern int mtk_chr_enable_direct_charge(unsigned char charging_enable);
extern int mtk_chr_kick_direct_charge_wdt(void);
extern int mtk_chr_set_direct_charge_ibus_oc(unsigned int cur);
extern int mtk_chr_enable_charge(unsigned char charging_enable);
extern int mtk_chr_get_input_current(unsigned int *cur);
extern int mtk_chr_enable_power_path(unsigned char en);
extern int mtk_chr_get_tchr(int *min_temp, int *max_temp);
extern void wake_up_bat3(void);
extern int mtk_cooler_is_abcct_unlimit(void);


extern bool mtk_is_pd_chg_ready(void);
extern int tcpm_hard_reset(void *ptr);
extern int tcpm_set_direct_charge_en(void *tcpc_dev, bool en);
extern int tcpm_get_cable_capability(void *tcpc_dev, unsigned char *capability);
extern int mtk_clr_ta_pingcheck_fault(void *ptr);
extern bool mtk_is_pep30_en_unlock(void);


#endif /* __MTK_PE_30_INTF_H */

