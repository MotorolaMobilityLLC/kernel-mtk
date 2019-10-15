#ifdef TINNO_FINGERPRINT_SUPPORT

#ifndef __FP_DRV_H
#define __FP_DRV_H

#include <linux/types.h>

///////////////////////////////////////////////////////////////////////////////////////////

extern int full_fp_chip_name(const char *name);
extern int full_fp_chip_info(const char *name);
extern int read_fpId_pin_value(struct device *dev, char *label);
extern int fp_event_record(int key);
extern int fp_key_event_record(int key);
///////////////////////////////////////////////////////////////////////////////////////////
#undef LOG_TAG
#define LOG_TAG  "[fingerprint][fp_drv]:" 
#define __FUN(f)   printk(KERN_ERR LOG_TAG "~~~~~~~~~~~~ %s ~~~~~~~~~\n", __FUNCTION__)
#define klog(fmt, args...)    printk(KERN_ERR LOG_TAG fmt, ##args)


#define __HIGH   (1) 
#define __LOW   (2)
#define __HIGH_IMPEDANCE   (3) 
///////////////////////////////////////////////////////////////////////////////////////////

#define ELAN_EVT_LEFT          301
#define ELAN_EVT_RIGHT        302
#define ELAN_EVT_UP              303
#define ELAN_EVT_DOWN        304

#define HW_EVT_DOWN                1
#define HW_EVT_UP                     2
#define HW_EVT_MOVE_UP          3
#define HW_EVT_MOVE_DOWN     4
#define HW_EVT_MOVE_LEFT       5
#define HW_EVT_MOVE_RIGHT     6


#define HW_EVENT_DOWN (1)
#define HW_EVENT_UP (2)
#define HW_EVENT_WAKEUP (99)

#define FP_EVT_REPORT(x) \
    do { \
        fp_event_record(x); \
    } while(0)

#define REGISTER_FP_DEV_INFO(name, version, args, fs, cid) \
    do { \
        register_fp_dev_info(name, version, args, fs, cid); \
    } while(0)

typedef int (*FpFuncptr)(char *buf, void *args); 

typedef struct fingerprint_dev_info_t {
    char dev_name[64];
    char version[64];
    void *args;
    FpFuncptr finger_state;
    FpFuncptr chip_id;
} fingerprint_dev_info;

extern int register_fp_dev_info(const char *dev_name, char *version, void *args, FpFuncptr fs, FpFuncptr cid);
#endif /* __FP_DRV_H */

#endif
