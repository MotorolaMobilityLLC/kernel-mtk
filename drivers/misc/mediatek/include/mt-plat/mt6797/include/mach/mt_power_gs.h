#ifndef _MT_POWER_GS_H
#define _MT_POWER_GS_H

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/*****************
* extern variable
******************/
extern struct proc_dir_entry *mt_power_gs_dir;
extern unsigned int *AP_CG_gs_dpidle;
extern unsigned int AP_CG_gs_dpidle_len;

extern unsigned int *AP_DCM_gs_dpidle;
extern unsigned int AP_DCM_gs_dpidle_len;
/*****************
* extern function
******************/
extern void mt_power_gs_compare(char *, const unsigned int *, unsigned int);

extern void mt_power_gs_dump_suspend(void);
extern const unsigned int *MT6351_PMIC_REG_gs_flightmode_suspend_mode;
extern unsigned int MT6351_PMIC_REG_gs_flightmode_suspend_mode_len;
extern void mt_power_gs_dump_dpidle(void);
extern const unsigned int *MT6351_PMIC_REG_gs_early_suspend_deep_idle__mode;
extern unsigned int MT6351_PMIC_REG_gs_early_suspend_deep_idle__mode_len;
/*extern void mt_power_gs_dump_idle(void);*/
/*extern void mt_power_gs_dump_audio_playback(void);*/
#endif
