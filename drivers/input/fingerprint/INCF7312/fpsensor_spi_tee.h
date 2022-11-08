#ifndef __FPSENSOR_SPI_TEE_H
#define __FPSENSOR_SPI_TEE_H

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/notifier.h>
#include "linux/version.h"

#define FPSENSOR_DEV_NAME           "fpsensor"
#define FPSENSOR_CLASS_NAME         "fpsensor"
#define FPSENSOR_DEV_MAJOR          0
#define N_SPI_MINORS                32    /* ... up to 256 */
#define FPSENSOR_NR_DEVS            1

/*************************driver version  **********************/ 
#define FPSENSOR_SPI_VERSION       "fpsensor_spi_tee_mtk_v1.23.1"

/**************** Custom device : platfotm  or spi **************/
#define  USE_SPI_BUS                1
#define  USE_PLATFORM_BUS           0


/*********************** debug log setting **********************/ 
#define FPSENSOR_LOG_ENABLE         1         // 0:error log    1: debug log

/*********************** power setting **********************/
#define FPSENSOR_USE_POWER_GPIO     1        // 0: not use power gpio  1£ºpower gpio

/*********************** SPI GPIO setting **********************/
#define FPSENSOR_SPI_PIN_SET        0        // 0: SPI pin is not configured in the driver    1: confinger spi pin in driver

/*********************** notify setting **********************/
#define FP_NOTIFY                   1        // 

/*********************** wakelock setting **********************/
#ifdef CONFIG_PM_WAKELOCKS
#define FPSENSOR_WAKEUP_SOURCE  1
#else
#define FPSENSOR_WAKEUP_SOURCE  0
// others
#endif

/*********************** spi clock setting **********************/
// Just macro definition, no need to modify
#define FPSENSOR_SPI_CLOCK_TYPE_MASTER_CLK    1     
#define FPSENSOR_SPI_CLOCK_TYPE_CLKMER    2
#define FPSENSOR_SPI_CLOCK_TYPE_CLK   3

#if 0//def CONFIG_MTK_CLKMGR
#define FPSENSOR_SPI_CLOCK_TYPE    FPSENSOR_SPI_CLOCK_TYPE_CLKMER 
   	  
#elif LINUX_VERSION_CODE > KERNEL_VERSION(4,9,0)
#define FPSENSOR_SPI_CLOCK_TYPE    FPSENSOR_SPI_CLOCK_TYPE_MASTER_CLK//FPSENSOR_SPI_CLOCK_TYPE_CLK

#else
#define FPSENSOR_SPI_CLOCK_TYPE    FPSENSOR_SPI_CLOCK_TYPE_CLK//FPSENSOR_SPI_CLOCK_TYPE_MASTER_CLK

#endif

/*********************** fingerprint compatible setting **********************/
#define FPSENSOR_TKCORE_COMPATIBLE        1   // 0:off    1: on



/*********************** dts setting ****************************/
// 
// dts node 
#define FPSENSOR_DTS_NODE            "mediatek,fpsensor"   

// compatible node 
#define FPSENSOR_COMPATIBLE_NODE     "mediatek,fingerprint"

// gpio pinctrl

#define FPSENSOR_RESET_HIGH          "fpsensor_rst_high"
#define FPSENSOR_RESET_LOW           "fpsensor_rst_low"

#define FPSENSOR_INT_SET             "fpsensor_eint"

#if FPSENSOR_USE_POWER_GPIO
#define FPSENSOR_POWER_ON            "fpsensor_power_high"
#define FPSENSOR_POWER_OFF           "fpsensor_power_low"
#endif

// spi gpio set 
#if FPSENSOR_SPI_PIN_SET
#define FPSENSOR_CS_SET             "fpsensor_cs_set"
#define FPSENSOR_MO_SET             "fpsensor_mo_set"
#define FPSENSOR_MI_SET             "fpsensor_mi_set"
#define FPSENSOR_CLK_SET            "fpsensor_clk_set"

#endif


/***************************IOCTL IO Magic***************************/

#define FPSENSOR_IOC_MAGIC    0xf0   

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

/* fp sensor has change to sleep mode while screen off */
#define FPSENSOR_IOC_ENTER_SLEEP_MODE           _IOWR(FPSENSOR_IOC_MAGIC,11,unsigned int)
#define FPSENSOR_IOC_REMOVE                     _IOWR(FPSENSOR_IOC_MAGIC,12,unsigned int)
#define FPSENSOR_IOC_CANCEL_WAIT                _IOWR(FPSENSOR_IOC_MAGIC,13,unsigned int)
#define FPSENSOR_IOC_GET_FP_STATUS              _IOWR(FPSENSOR_IOC_MAGIC,19,unsigned int)
#define FPSENSOR_IOC_ENABLE_REPORT_BLANKON      _IOWR(FPSENSOR_IOC_MAGIC,21,unsigned int)
#define FPSENSOR_IOC_UPDATE_DRIVER_SN           _IOWR(FPSENSOR_IOC_MAGIC,22,unsigned int)


/*********************** debug log *****************************/
#define ERR_LOG     (0)
#define INFO_LOG    (1)
#define DEBUG_LOG   (2)
/* check log debug */
#if FPSENSOR_LOG_ENABLE
#define FPSENSOR_LOG_LEVEL          DEBUG_LOG
#else
#define FPSENSOR_LOG_LEVEL          ERR_LOG
#endif
uint32_t g_cmd_sn = 0;
static u8 fpsensor_debug_level = FPSENSOR_LOG_LEVEL;
#define fpsensor_debug(level, fmt, args...) do { \
        if (fpsensor_debug_level >= level) {\
            printk("[fpsensor][SN=%d] " fmt, g_cmd_sn, ##args); \
        } \
    } while (0)
#define FUNC_ENTRY()  fpsensor_debug(DEBUG_LOG, "%s, %d, entry\n", __func__, __LINE__)
#define FUNC_EXIT()   fpsensor_debug(DEBUG_LOG, "%s, %d, exit\n", __func__, __LINE__)


typedef struct {
    dev_t devno;
    struct class *class;
    struct cdev cdev;
#if USE_SPI_BUS
    struct spi_device *spi;
#elif USE_PLATFORM_BUS
    struct platform_device *spi;
#endif

    #if FPSENSOR_WAKEUP_SOURCE
    struct wakeup_source *ttw_wl;
#else	
    struct wake_lock ttw_wl;
#endif
    wait_queue_head_t wq_irq_return;
    struct pinctrl *pinctrl;
    struct pinctrl_state   *fp_rst_high, *fp_rst_low, *eint_as_int,*fp_power_on,*fp_power_off,*fp_cs_set,*fp_mo_set,
                           *fp_mi_set,*fp_clk_set;
    struct notifier_block notifier;
    u8 device_available;    /* changed during fingerprint chip sleep and wakeup phase */
    u8 irq_enabled;
    u8 fb_status;
    volatile unsigned int RcvIRQ;
    unsigned int users;
    int irq;
    int irq_gpio;
    int cancel;
    int enable_report_blankon;
    int free_flag;
    #if FPSENSOR_TKCORE_COMPATIBLE
    u16 spi_freq_khz;
    #endif

} fpsensor_data_t;


#define     FPSENSOR_RST_PIN      1  // not gpio, only macro,not need modified!!
#define     FPSENSOR_SPI_CS_PIN   2  // not gpio, only macro,not need modified!!     
#define     FPSENSOR_SPI_MO_PIN   3  // not gpio, only macro,not need modified!!   
#define     FPSENSOR_SPI_MI_PIN   4  // not gpio, only macro,not need modified!!   
#define     FPSENSOR_SPI_CK_PIN   5  // not gpio, only macro,not need modified!! 
#define     FPSENSOR_POWER_PIN    6  // not gpio, only macro,not need modified!! 
#endif    /* __FPSENSOR_SPI_TEE_H */
