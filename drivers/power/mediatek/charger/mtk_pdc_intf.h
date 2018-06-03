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

#ifndef __MTK_PD_INTF_H
#define __MTK_PD_INTF_H

struct pdc_power_cap {
	uint8_t selected_cap_idx;
	uint8_t nr;
	int max_mv[TCPM_PDO_MAX_SIZE];
	int min_mv[TCPM_PDO_MAX_SIZE];
	int ma[TCPM_PDO_MAX_SIZE];
	int maxwatt[TCPM_PDO_MAX_SIZE];
	int minwatt[TCPM_PDO_MAX_SIZE];
};

struct mtk_pdc {
	struct tcpc_device *tcpc;
	struct pdc_power_cap cap;
	unsigned int pdc_max_watt;


	struct mutex access_lock;
	struct mutex pmic_sync_lock;
	struct wake_lock suspend_lock;
	int ta_vchr_org;
	bool to_check_chr_type;
	bool to_tune_ta_vchr;
	bool is_cable_out_occur;
	bool is_connect;
	bool is_enabled;
};

extern bool mtk_pdc_check_charger(struct charger_manager *info);
extern void mtk_pdc_plugout_reset(struct charger_manager *info);
extern void mtk_pdc_set_max_watt(struct charger_manager *info, unsigned int watt);
extern unsigned int mtk_pdc_get_max_watt(struct charger_manager *info);
extern int mtk_pdc_get_setting(struct charger_manager *info, int *vbus, int *cur, int *idx);
extern void mtk_pdc_init_table(struct charger_manager *info);
extern bool mtk_pdc_init(struct charger_manager *info);
extern int mtk_pdc_setup(struct charger_manager *info, int idx);
extern void mtk_pdc_plugout(struct charger_manager *info);
extern void mtk_pdc_check_cable_impedance(struct charger_manager *pinfo);


#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT



#else /* NOT CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT */


#endif /* CONFIG_MTK_PUMP_EXPRESS_PLUS_SUPPORT */


#endif /* __MTK_PD_INTF_H */
