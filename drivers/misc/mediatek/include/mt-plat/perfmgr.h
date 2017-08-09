#ifndef __PERFMGR_H__
#define __PERFMGR_H__

extern int init_perfmgr_touch(void);
extern int perfmgr_touch_suspend(void);

extern int  perfmgr_get_target_core(void);
extern int  perfmgr_get_target_freq(void);
extern void perfmgr_boost(int enable, int core, int freq);

#endif				/* !__PERFMGR_H__ */
