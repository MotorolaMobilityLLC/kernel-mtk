#ifndef __BF_FP_H
#define __BF_FP_H


#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_OF
#ifndef MTK_ANDROID_L
#include <linux/of_gpio.h>
#endif
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 0))
#include <linux/pm_wakeup.h>
#define KERNEL_4_9
#else
#include <linux/wakelock.h>
#endif

#define BF_DEV_NAME "blfp"
#define BF_DEV_MAJOR 0	/* assigned */
#define BF_CLASS_NAME "blfp"

/*for read chip id*/
#define READ_CHIP_ID

/*for power on*/
//#define NEED_OPT_POWER_ON2V8	//power gpio
//#define NEED_OPT_POWER_ON1V8

/*for pinctrl*/
//#define BF_PINCTL

/* for netlink use */
#define MAX_NL_MSG_LEN 16
#define NETLINK_BF  31

/*for kernel log*/
#define BLESTECH_LOG
//#define FAST_VERISON
#define BUF_SIZE 200*200

#ifdef BLESTECH_LOG
#define BF_LOG(fmt,arg...)          do{printk("<blestech_fp>[%s:%d]"fmt"\n",__func__, __LINE__, ##arg);}while(0)
#else
#define BF_LOG(fmt,arg...)   	   do{}while(0)
#endif

#define BF3290				0x5183
#define BF3182				0x5283
#define BF3390				0x5383
#define BF3582P             0x5783
#define BF3582S             0x5483

typedef enum {
    BF_NETLINK_CMD_BASE = 100,

    BF_NETLINK_CMD_TEST  = BF_NETLINK_CMD_BASE + 1,
    BF_NETLINK_CMD_IRQ = BF_NETLINK_CMD_BASE + 2,
    BF_NETLINK_CMD_SCREEN_OFF = BF_NETLINK_CMD_BASE + 3,
    BF_NETLINK_CMD_SCREEN_ON = BF_NETLINK_CMD_BASE + 4
} fingerprint_socket_cmd_t;

typedef struct {
    int32_t ic_chipid;
    char ic_name[128];
    char ca_ver[128];
    char ta_ver[128];
} __attribute__ ((aligned(4))) bl_ic_info_t;

struct bf_device {
    struct platform_device *pdev;

    dev_t devno;
    struct cdev cdev;
    struct device *device;
    //struct class *class;
    int device_count;
    struct spi_device *spi;
    struct list_head device_entry;

    int32_t reset_gpio;
    int32_t irq_gpio;
    int32_t irq_num;
    u8 irq_count;
    u8 doing_reset;
    int32_t report_key;
    u8  need_report;
    int32_t power_gpio;
#ifdef BF_REE
    u8 *image_buf;
#endif
#ifdef NEED_OPT_POWER_ON2V8
    int32_t power_2v8_gpio;
#endif

#ifdef NEED_OPT_POWER_ON1V8
    int32_t power_1v8_gpio;
#endif
    struct pinctrl *pinctrl_gpios;
    struct pinctrl_state *pins_spi_default, *pins_fp_interrupt;
    struct pinctrl_state *pins_reset_high, *pins_reset_low;
#ifdef NEED_OPT_POWER_ON2V8
    struct pinctrl_state *pins_power_2v8_high, *pins_power_2v8_low;
#endif
#ifdef NEED_OPT_POWER_ON1V8
    struct pinctrl_state *pins_power_1v8_high, *pins_power_1v8_low;
#endif
#if defined(CONFIG_FB)
    struct notifier_block fb_notify;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend early_suspend;
#endif
    /* for netlink use */
    struct sock *netlink_socket;
#ifdef MTK_ANDROID_L
    struct work_struct fingerprint_work;
    struct workqueue_struct *fingerprint_workqueue;
#endif
};

#ifdef BF_REE
/********** the register addr and value ***********/
typedef struct {
    uint8_t data_tx[4];
    uint8_t data_rx[4];
    uint16_t len;
} __attribute__ ((aligned(4))) bl_read_write_reg_command_t;
#endif

typedef enum {
    // work mode
    MODE_IDLE       = 0x00,
    MODE_RC_DT      = 0x01,
    MODE_FG_DT      = 0x02,
    MODE_FG_PRINT   = 0x03,
    MODE_FG_CAP     = 0x04,
    MODE_NAVI       = 0x05,
    MODE_CLK_CALI   = 0x06,
    MODE_PIEXL_TEST = 0x07
} bf_work_mode_t;

typedef enum {
    REGA_ADC_OPTION                 = 0x07,
    REGA_TIME_INTERVAL_LOW          = 0x08,
    REGA_TIME_INTERVAL_HIGH         = 0x09,
    REGA_FINGER_DT_INTERVAL_LOW     = 0x0A,
    REGA_FINGER_DT_INTERVAL_HIGH    = 0x0B,
    REGA_FRAME_NUM                  = 0x0D, // number reset row vdd
    REGA_RC_THRESHOLD_LOW           = 0x0E,
    REGA_RC_THRESHOLD_MID           = 0x0F,
    REGA_RC_THRESHOLD_HIGH          = 0x10,
    REGA_FINGER_TD_THRED_LOW        = 0x11,
    REGA_FINGER_TD_THRED_HIGH       = 0x12,
    REGA_HOST_CMD                   = 0x13,
    REGA_PIXEL_MAX_DELTA            = 0x18,
    REGA_PIXEL_MIN_DELTA            = 0x19,
    REGA_PIXEL_ERR_NUM              = 0x1A,
    REGA_RX_DACP_LOW                = 0x1B, //
    REGA_RX_DACP_HIGH               = 0x1C,
    REGA_FINGER_CAP                 = 0x27,
    REGA_INTR_STATUS                = 0x28,
    REGA_GC_STAGE                   = 0x31, //
    REGA_IC_STAGE                   = 0x32,
    REGA_VERSION_RD_EN              = 0x3A,
    REGA_F32K_CALI_LOW              = 0x3B,
    REGA_F32K_CALI_HIGH             = 0x3C,
    REGA_F32K_CALI_EN               = 0x3F
} bf_register_t;

u8 bf_spi_write_reg(u8 reg, u8 value);
u8 bf_spi_read_reg(u8 reg);
u8 bf_spi_write_reg_bit(u8 nRegID, u8 bit, u8 value);
int bf_read_chipid(void);
void bf_chip_info(void);
void bf_send_netlink_msg(struct bf_device *bf_dev, const int command);
int bf_remove(struct platform_device *pdev);

#ifdef BF_REE
int spi_set_dma_en(int mode);
int spi_send_cmd(struct bf_device *bf_dev, u8 *tx, u8 *rx, u16 spilen);
#endif
#ifdef USE_SPI1_4GB_TEST
int spi_dma_exchange(struct bf_device *bl229x, bl_read_write_reg_command_t read_write_cmd, int dma_size);
#endif

#if defined(BF_REE) || defined(COMPATIBLE) || defined(CONFIG_MTK_CLK)
int32_t bf_spi_init(struct bf_device *bf_dev);
#endif

#ifdef CONFIG_MTK_CLK
void bf_spi_clk_enable(struct bf_device *bf_dev, u8 onoff);
#endif
int bf_init_dts_and_irq(struct bf_device *bf_dev);
void bf_spi_unregister(void);
extern void mt_spi_enable_master_clk(struct spi_device *spidev);
extern void mt_spi_disable_master_clk(struct spi_device *spidev);
#endif //__BF_SPI_TEE_H_
