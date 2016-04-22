#ifndef _PMIC_THROTTLING_DLPT_H_
#define _PMIC_THROTTLING_DLPT_H_

extern int pmic_throttling_dlpt_init(void);
extern void low_battery_protect_init(void);
extern void battery_oc_protect_init(void);
extern void bat_percent_notify_init(void);
extern void dlpt_notify_init(void);
extern void pmic_throttling_dlpt_suspend(void);
extern void pmic_throttling_dlpt_resume(void);
extern void pmic_throttling_dlpt_debug_init(struct platform_device *dev, struct dentry *debug_dir);
extern void bat_h_int_handler(void);
extern void bat_l_int_handler(void);
extern void fg_cur_h_int_handler(void);
extern void fg_cur_l_int_handler(void);
#endif				/* _PMIC_THROTTLING_DLPT_H_ */
