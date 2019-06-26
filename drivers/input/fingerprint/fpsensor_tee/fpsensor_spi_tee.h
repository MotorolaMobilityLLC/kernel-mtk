#ifndef __FPSENSOR_SPI_TEE_H
#define __FPSENSOR_SPI_TEE_H

#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <mt_spi.h>
#define FP_NOTIFY         1
#if FP_NOTIFY
#include <linux/fb.h>
#include <linux/notifier.h>
#endif
/**********************IO Magic**********************/
#define FPSENSOR_IOC_MAGIC    0xf0    //CHIP

typedef enum fpsensor_key_event {
    FPSENSOR_KEY_NONE = 0,
    FPSENSOR_KEY_HOME,
    FPSENSOR_KEY_POWER,
    FPSENSOR_KEY_MENU,
    FPSENSOR_KEY_BACK,
    FPSENSOR_KEY_CAPTURE,
    FPSENSOR_KEY_UP,
    FPSENSOR_KEY_DOWN,
    FPSENSOR_KEY_RIGHT,
    FPSENSOR_KEY_LEFT,
    FPSENSOR_KEY_TAP,
    FPSENSOR_KEY_HEAVY
} fpsensor_key_event_t;

struct fpsensor_key {
    enum fpsensor_key_event key;
    uint32_t value;   /* key down = 1, key up = 0 */
};

/* define commands */
#define FPSENSOR_IOC_INIT                       _IOWR(FPSENSOR_IOC_MAGIC,0,unsigned int)
#define FPSENSOR_IOC_EXIT                       _IOWR(FPSENSOR_IOC_MAGIC,1,unsigned int)
#define FPSENSOR_IOC_RESET                      _IOWR(FPSENSOR_IOC_MAGIC,2,unsigned int)
#define FPSENSOR_IOC_ENABLE_IRQ                 _IOWR(FPSENSOR_IOC_MAGIC,3,unsigned int)
#define FPSENSOR_IOC_DISABLE_IRQ                _IOWR(FPSENSOR_IOC_MAGIC,4,unsigned int)
#define FPSENSOR_IOC_GET_INT_VAL                _IOWR(FPSENSOR_IOC_MAGIC,5,unsigned int)
#define FPSENSOR_IOC_DISABLE_SPI_CLK            _IOWR(FPSENSOR_IOC_MAGIC,6,unsigned int)
#define FPSENSOR_IOC_ENABLE_SPI_CLK             _IOWR(FPSENSOR_IOC_MAGIC,7,unsigned int)
#define FPSENSOR_IOC_ENABLE_POWER               _IOWR(FPSENSOR_IOC_MAGIC,8,unsigned int)
#define FPSENSOR_IOC_DISABLE_POWER              _IOWR(FPSENSOR_IOC_MAGIC,9,unsigned int)
#define FPSENSOR_IOC_INPUT_KEY_EVENT            _IOWR(FPSENSOR_IOC_MAGIC,10,struct fpsensor_key)
/* fp sensor has change to sleep mode while screen off */
#define FPSENSOR_IOC_ENTER_SLEEP_MODE           _IOWR(FPSENSOR_IOC_MAGIC,11,unsigned int)
#define FPSENSOR_IOC_REMOVE                     _IOWR(FPSENSOR_IOC_MAGIC,12,unsigned int)
#define FPSENSOR_IOC_CANCEL_WAIT                _IOWR(FPSENSOR_IOC_MAGIC,13,unsigned int)
#define FPSENSOR_IOC_GET_FP_STATUS              _IOWR(FPSENSOR_IOC_MAGIC,19,unsigned int*)
#define FPSENSOR_IOC_MAXNR    24  /* THIS MACRO IS NOT USED NOW... */
#define SUPPORT_REE_SPI   0
typedef struct {
    dev_t devno;
    struct class           *class;
    struct device          *device;
    struct cdev            cdev;

    spinlock_t    spi_lock;
    struct spi_device *spi;
    struct list_head device_entry;
    struct input_dev *input;

    /* buffer is NULL unless this device is open (users > 0) */
    unsigned int users;
    u8 *buffer;
    struct mutex buf_lock;
    u8 buf_status;
    u8 device_available;    /* changed during fingerprint chip sleep and wakeup phase */
    int device_count;
    u8 probe_finish;
    u8 irq_count;
    /* bit24-bit32 of signal count */
    /* bit16-bit23 of event type, 1: key down; 2: key up; 3: fp data ready; 4: home key */
    /* bit0-bit15 of event type, buffer status register */
    u32 event_type;
    u8 sig_count;
    u8 is_sleep_mode;
    volatile unsigned int RcvIRQ;
    //irq
    int irq;
    u32 irq_gpio;
    /*pinctrl*/
    /*struct pinctrl *pinctrl_gpios;
    struct pinctrl_state *pins_reset_high, *pins_reset_low;
    struct pinctrl_state *pins_power_on, *pins_power_off, *pins_fpsnesor_irq;*/
    //wait queue
    wait_queue_head_t wq_irq_return;
    int cancel;
    struct pinctrl *pinctrl1;
    struct pinctrl_state *eint_as_int, *fp_rst_low, *fp_rst_high, *fp_cs_high, *fp_cs_low, *fp_mo_low,
            *fp_mo_high, *fp_mi_low, *fp_mi_high, *fp_ck_low, *fp_ck_high;
#if  FP_NOTIFY
    struct notifier_block notifier;
    u8 fb_status;
#endif
} fpsensor_data_t;
#define     FPSENSOR_RST_PIN      1  // not gpio, only macro,not need modified!!
#define     FPSENSOR_SPI_CS_PIN   2  // not gpio, only macro,not need modified!!     
#define     FPSENSOR_SPI_MO_PIN   3  // not gpio, only macro,not need modified!!   
#define     FPSENSOR_SPI_MI_PIN   4  // not gpio, only macro,not need modified!!   
#define     FPSENSOR_SPI_CK_PIN   5  // not gpio, only macro,not need modified!!   

#define DBG_FPSENSOR_TRACE
#define DBG_FPSENSOR_ERROR
#define DBG_FPSENSOR_PRINTK

#ifdef DBG_FPSENSOR_TRACE
#define fpsensor_trace(fmt, args...)     \
    do{                                 \
        printk("fpDebug "fmt, ##args);            \
    }while(0)
#else
#define fpsensor_trace(fmt, args...)
#endif

#ifdef DBG_FPSENSOR_ERROR
#define fpsensor_error(fmt, args...)     \
    do{                                 \
        printk("fpDebug "fmt, ##args);            \
    }while(0)
#else
#define fpsensor_error(fmt, args...)
#endif


#ifdef DBG_FPSENSOR_PRINTK
#define fpsensor_printk(fmt, args...)     \
    do{                                 \
        printk("fpDebug "fmt, ##args);            \
    }while(0)
#else
#define fpsensor_printk(fmt, args...)
#endif


extern void setRcvIRQ( int  val );





#endif    /* __FPSENSOR_SPI_TEE_H */



