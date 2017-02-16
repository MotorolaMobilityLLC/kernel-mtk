#ifndef __UT_TUI_H_
#define __UT_TUI_H_

#include "utdriver_macro.h"

struct message_head {
        unsigned int invalid_flag;
        unsigned int message_type;
        unsigned int child_type;
        unsigned int param_length;
};

struct fdrv_message_head {
        unsigned int driver_type;
        unsigned int fdrv_param_length;
};

struct create_fdrv_struct {
        unsigned int fdrv_type;
        unsigned int fdrv_phy_addr;
        unsigned int fdrv_size;
};

struct ack_fast_call_struct {
        int retVal;
};

extern unsigned long message_buff;
extern unsigned long fdrv_message_buff;
extern int fp_call_flag;
extern int forward_call_flag;
extern struct semaphore fdrv_lock;
extern struct semaphore smc_lock;
extern void ut_down_low(struct semaphore *sema);
extern struct semaphore boot_sema;
extern struct semaphore fdrv_sema;
extern struct mutex pm_mutex;
extern struct completion global_down_lock;
extern struct semaphore api_lock;
extern struct semaphore tui_notify_sema;
extern unsigned long teei_config_flag;

extern void invoke_fastcall(void);
extern void ut_pm_mutex_lock(struct mutex *lock);
extern void ut_pm_mutex_unlock(struct mutex *lock);

int try_send_tui_command(void);
int send_tui_display_command(unsigned long share_memory_size);
int send_tui_notice_command(unsigned long share_memory_size);



#endif
