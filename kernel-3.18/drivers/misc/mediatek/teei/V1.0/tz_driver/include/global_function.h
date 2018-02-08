
extern struct semaphore smc_lock;
extern int forward_call_flag;
extern int irq_call_flag;
extern int fp_call_flag;
extern int keymaster_call_flag;
//zhangheting@wind-mobi.com add teei patch 20170523 start
//extern int teei_vfs_flag;
extern unsigned long teei_vfs_flag;
//zhangheting@wind-mobi.com add teei patch 20170523 end
extern struct completion global_down_lock;
extern unsigned long teei_config_flag;

extern int get_current_cpuid(void);
