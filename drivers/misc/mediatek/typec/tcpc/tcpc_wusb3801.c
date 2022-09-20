/************************************************************************
 *
 *  WILLSEMI TypeC Chipset Driver for Linux & Android.
 *
 *
 * ######################################################################
 *
 *  Author: lei.huang (lhuang@sh-willsemi.com)
 *
 * Copyright (c) 2021, WillSemi Inc. All rights reserved.
 *
 ************************************************************************/


#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/pm_runtime.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/cpu.h>
#include <linux/version.h>
#include <linux/pm_wakeup.h>
#include <linux/sched/clock.h>
#include <uapi/linux/sched/types.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
//#include <linux/wakelock.h>
#include "inc/pd_dbg_info.h"
#include "inc/tcpci.h"
#include "inc/tcpci_timer.h"
#include "inc/tcpci_typec.h"
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/hrtimer.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/sched/rt.h>
#endif

#include <ontim/ontim_dev_dgb.h>
char typec_vendor_name[50]="wusb3801";
DEV_ATTR_DECLARE(typec)
DEV_ATTR_DEFINE("vendor",typec_vendor_name)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(typec,typec,8);


#define __TEST_CC_PATCH__

/*
 *Bit operations if we don't want to include #include <linux/bitops.h>
 */

#undef  __CONST_FFS
#define __CONST_FFS(_x) \
        ((_x) & 0x0F ? ((_x) & 0x03 ? ((_x) & 0x01 ? 0 : 1) :\
                                      ((_x) & 0x04 ? 2 : 3)) :\
                       ((_x) & 0x30 ? ((_x) & 0x10 ? 4 : 5) :\
                                      ((_x) & 0x40 ? 6 : 7)))

#undef  FFS
#define FFS(_x) \
        ((_x) ? __CONST_FFS(_x) : 0)

#undef  BITS
#define BITS(_end, _start) \
        ((BIT(_end) - BIT(_start)) + BIT(_end))

#undef  __BITS_GET
#define __BITS_GET(_byte, _mask, _shift) \
        (((_byte) & (_mask)) >> (_shift))

#undef  BITS_GET
#define BITS_GET(_byte, _bit) \
        __BITS_GET(_byte, _bit, FFS(_bit))

#undef  __BITS_SET
#define __BITS_SET(_byte, _mask, _shift, _val) \
        (((_byte) & ~(_mask)) | (((_val) << (_shift)) & (_mask)))

#undef  BITS_SET
#define BITS_SET(_byte, _bit, _val) \
        __BITS_SET(_byte, _bit, FFS(_bit), _val)

#undef  BITS_MATCH
#define BITS_MATCH(_byte, _bit) \
        (((_byte) & (_bit)) == (_bit))

/* Register Map */
#define WUSB3801_DRV_VERSION	"2.1.0_MTK"

#define WUSB3801_REG_VERSION_ID         0x01
#define WUSB3801_REG_CONTROL0           0x02
#define WUSB3801_REG_INTERRUPT          0x03
#define WUSB3801_REG_STATUS             0x04
#define WUSB3801_REG_CONTROL1           0x05
#define WUSB3801_REG_TEST0              0x06
#define WUSB3801_REG_TEST_01            0x07
#define WUSB3801_REG_TEST_02            0x08
#define WUSB3801_REG_TEST_03            0x09
#define WUSB3801_REG_TEST_04            0x0A
#define WUSB3801_REG_TEST_05            0x0B
#define WUSB3801_REG_TEST_06            0x0C
#define WUSB3801_REG_TEST_07            0x0D
#define WUSB3801_REG_TEST_08            0x0E
#define WUSB3801_REG_TEST_09            0x0F
#define WUSB3801_REG_TEST_0A            0x10
#define WUSB3801_REG_TEST_0B            0x11
#define WUSB3801_REG_TEST_0C            0x12
#define WUSB3801_REG_TEST_0D            0x13
#define WUSB3801_REG_TEST_0E            0x14
#define WUSB3801_REG_TEST_0F            0x15
#define WUSB3801_REG_TEST_10            0x16
#define WUSB3801_REG_TEST_11            0x17
#define WUSB3801_REG_TEST_12            0x18


#define WUSB3801_SLAVE_ADDR0            0xc0
#define WUSB3801_SLAVE_ADDR1            0xd0

/*Available modes*/
#define WUSB3801_DRP_ACC                (BIT_REG_CTRL0_RLE_DRP)
#define WUSB3801_DRP                    (BIT_REG_CTRL0_RLE_DRP | BIT_REG_CTRL0_DIS_ACC)
#define WUSB3801_SNK_ACC                (BIT_REG_CTRL0_RLE_SNK)
#define WUSB3801_SNK                    (BIT_REG_CTRL0_RLE_SNK | BIT_REG_CTRL0_DIS_ACC)
#define WUSB3801_SRC_ACC                (BIT_REG_CTRL0_RLE_SRC)
#define WUSB3801_SRC                    (BIT_REG_CTRL0_RLE_SRC | BIT_REG_CTRL0_DIS_ACC)
#define WUSB3801_DRP_PREFER_SRC_ACC     (WUSB3801_DRP_ACC | BIT_REG_CTRL0_TRY_SRC)
#define WUSB3801_DRP_PREFER_SRC         (WUSB3801_DRP     | BIT_REG_CTRL0_TRY_SRC)
#define WUSB3801_DRP_PREFER_SNK_ACC     (WUSB3801_DRP_ACC | BIT_REG_CTRL0_TRY_SNK)
#define WUSB3801_DRP_PREFER_SNK         (WUSB3801_DRP     | BIT_REG_CTRL0_TRY_SNK)


/*TODO: redefine your prefer role here*/
#define WUSB3801_INIT_MODE              (WUSB3801_DRP_PREFER_SNK_ACC)

/*Registers relevant values*/
#define WUSB3801_VENDOR_ID              0x06

/*Switch to enable/disable feature of specified Registers*/
#define BIT_REG_CTRL0_DIS_ACC           (0x01 << 7)
#define BIT_REG_CTRL0_TRY_SRC           (0x02 << 5)
#define BIT_REG_CTRL0_TRY_SNK           (0x01 << 5)
#define BIT_REG_CTRL0_CUR_DEF           (0x00 << 3)
#define BIT_REG_CTRL0_CUR_1P5           (0x01 << 3)
#define BIT_REG_CTRL0_CUR_3P0           (0x02 << 3)
#define BIT_REG_CTRL0_RLE_SNK           (0x00 << 1)
#define BIT_REG_CTRL0_RLE_SRC           (0x01 << 1)
#define BIT_REG_CTRL0_RLE_DRP           (0x02 << 1)
#define BIT_REG_CTRL0_INT_MSK           (0x01 << 0)

#define BIT_REG_STATUS_VBUS             (0x01 << 7)
#define BIT_REG_STATUS_STANDBY          (0x00 << 5)
#define BIT_REG_STATUS_CUR_DEF          (0x01 << 5)
#define BIT_REG_STATUS_CUR_MID          (0x02 << 5)
#define BIT_REG_STATUS_CUR_HIGH         (0x03 << 5)

#define BIT_REG_STATUS_ATC_STB          (0x00 << 1)
#define BIT_REG_STATUS_ATC_SNK          (0x01 << 1)
#define BIT_REG_STATUS_ATC_SRC          (0x02 << 1)
#define BIT_REG_STATUS_ATC_ACC          (0x03 << 1)
#define BIT_REG_STATUS_ATC_DACC         (0x04 << 1)

#define BIT_REG_STATUS_PLR_STB          (0x00 << 0)
#define BIT_REG_STATUS_PLR_CC1          (0x01 << 0)
#define BIT_REG_STATUS_PLR_CC2          (0x02 << 0)
#define BIT_REG_STATUS_PLR_BOTH         (0x03 << 0)

#define BIT_REG_CTRL1_SW02_DIN          (0x01 << 4)
#define BIT_REG_CTRL1_SW02_EN           (0x01 << 3)
#define BIT_REG_CTRL1_SW01_DIN          (0x01 << 2)
#define BIT_REG_CTRL1_SW01_EN           (0x01 << 1)
#define BIT_REG_CTRL1_SM_RST            (0x01 << 0)
#define BIT_REG_TEST02_FORCE_ERR_RCY    (0x01)

#define WUSB3801_WAIT_VBUS               0x40
/*Fixed duty cycle period. 40ms:40ms*/
#define WUSB3801_TGL_40MS                0
#define WUSB3801_HOST_DEFAULT            0
#define WUSB3801_HOST_1500MA             1
#define WUSB3801_HOST_3000MA             2
#define WUSB3801_INT_ENABLE              0x00
#define WUSB3801_INT_DISABLE             0x01
#define WUSB3801_DISABLED                0x0A
#define WUSB3801_ERR_REC                 0x01
#define WUSB3801_VBUS_OK                 0x80

#define WUSB3801_SNK_0MA                (0x00 << 5)
#define WUSB3801_SNK_DEFAULT            (0x01 << 5)
#define WUSB3801_SNK_1500MA             (0x02 << 5)
#define WUSB3801_SNK_3000MA             (0x03 << 5)
#define WUSB3801_ATTACH                  0x1C

//#define WUSB3801_TYPE_PWR_ACC           (0x00 << 2) /*Ra/Rd treated as Open*/
#define WUSB3801_TYPE_INVALID           (0x00)
#define WUSB3801_TYPE_SNK               (0x01 << 2)
#define WUSB3801_TYPE_SRC               (0x02 << 2)
#define WUSB3801_TYPE_AUD_ACC           (0x03 << 2)
#define WUSB3801_TYPE_DBG_ACC           (0x04 << 2)

#define WUSB3801_INT_DETACH              (0x01 << 1)
#define WUSB3801_INT_ATTACH              (0x01 << 0)

#define WUSB3801_REV20                   0x02

/* Masks for Read-Modified-Write operations*/
#define WUSB3801_HOST_CUR_MASK           0x18  /*Host current for IIC*/
#define WUSB3801_INT_MASK                0x01
#define WUSB3801_BCLVL_MASK              0x60
#define WUSB3801_TYPE_MASK               0x1C
#define WUSB3801_MODE_MASK               0xE6  /*Roles relevant bits*/
#define WUSB3801_INT_STS_MASK            0x03
#define WUSB3801_FORCE_ERR_RCY_MASK      0x80  /*Force Error recovery*/
#define WUSB3801_ROLE_MASK               0x06
#define WUSB3801_VENDOR_ID_MASK          0x07
#define WUSB3801_VERSION_ID_MASK         0xF8
#define WUSB3801_VENDOR_SUB_ID_MASK         0xA0
#define WUSB3801_POLARITY_CC_MASK        0x03
#define WUSB3801_CC_STS_MASK            0x03


/* WUSB3801 STATES MACHINES */
#define WUSB3801_STATE_DISABLED             0x00
#define WUSB3801_STATE_ERROR_RECOVERY       0x01
#define WUSB3801_STATE_UNATTACHED_SNK       0x02
#define WUSB3801_STATE_UNATTACHED_SRC       0x03
#define WUSB3801_STATE_ATTACHWAIT_SNK       0x04
#define WUSB3801_STATE_ATTACHWAIT_SRC       0x05
#define WUSB3801_STATE_ATTACHED_SNK         0x06
#define WUSB3801_STATE_ATTACHED_SRC         0x07
#define WUSB3801_STATE_AUDIO_ACCESSORY      0x08
#define WUSB3801_STATE_DEBUG_ACCESSORY      0x09
#define WUSB3801_STATE_TRY_SNK              0x0A
#define WUSB3801_STATE_TRYWAIT_SRC          0x0B
#define WUSB3801_STATE_TRY_SRC              0x0C
#define WUSB3801_STATE_TRYWAIT_SNK          0x0D

#define WUSB3801_CC2_CONNECTED 1
#define WUSB3801_CC1_CONNECTED 0
#define WUSB3801_IRQ_WAKE_TIME	(1000) /* ms */
/*1.5 Seconds timeout for force detection*/
#define ROLE_SWITCH_TIMEOUT		              1500
#define DEBOUNCE_TIME_OUT 	50

#if defined(__MEDIATEK_PLATFORM__)

enum dual_role_supported_modes {
  DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP = 0,
  DUAL_ROLE_SUPPORTED_MODES_DFP,
  DUAL_ROLE_SUPPORTED_MODES_UFP,
  /*The following should be the last element*/
  DUAL_ROLE_PROP_SUPPORTED_MODES_TOTAL,
};

enum {
  DUAL_ROLE_PROP_MODE_UFP = 0,
  DUAL_ROLE_PROP_MODE_DFP,
  DUAL_ROLE_PROP_MODE_NONE,
  /*The following should be the last element*/
  DUAL_ROLE_PROP_MODE_TOTAL,
};

enum {
   DUAL_ROLE_PROP_PR_SRC = 0,
   DUAL_ROLE_PROP_PR_SNK,
   DUAL_ROLE_PROP_PR_NONE,
   /*The following should be the last element*/
   DUAL_ROLE_PROP_PR_TOTAL,
};
enum {
   DUAL_ROLE_PROP_DR_HOST = 0,
   DUAL_ROLE_PROP_DR_DEVICE,
   DUAL_ROLE_PROP_DR_NONE,
  /*The following should be the last element*/
   DUAL_ROLE_PROP_DR_TOTAL,
};

enum {
   DUAL_ROLE_PROP_VCONN_SUPPLY_NO = 0,
   DUAL_ROLE_PROP_VCONN_SUPPLY_YES,
   /*The following should be the last element*/
   DUAL_ROLE_PROP_VCONN_SUPPLY_TOTAL,
};

enum dual_role_property {
   DUAL_ROLE_PROP_SUPPORTED_MODES = 0,
   DUAL_ROLE_PROP_MODE,
   DUAL_ROLE_PROP_PR,
   DUAL_ROLE_PROP_DR,
   DUAL_ROLE_PROP_VCONN_SUPPLY,
};

struct dual_role_phy_instance {
	/* Driver private data */
	void *drv_data;
};

void *dual_role_get_drvdata(struct dual_role_phy_instance *dual_role)
{
	return dual_role->drv_data;
}

static struct dual_role_phy_instance dual_role_service;


#endif /*__MEDIATEK_PLATFORM__*/

/*Private data*/
typedef struct wusb3801_data
{
	uint32_t  int_gpio;
	uint8_t  init_mode;
	uint8_t  dfp_power;
	uint8_t  dttime;
}wusb3801_data_t;
#ifdef  __WITH_KERNEL_VER4__
struct wusb3801_wakeup_source {
        struct wakeup_source    source;
        unsigned long           enabled_bitmap;
        spinlock_t              ws_lock;
};
#endif /* __WITH_KERNEL_VER4__ */
#ifdef __DEBOUNCE_TIMER__
typedef struct __WUSB3801_DRV_TIMER
{
	struct timer_list tl;
	void (*timer_function) (void *context);
	void *function_context;
	uint32_t time_period;
	uint8_t timer_is_canceled;
} WUSB3801_DRV_TIMER, *PWUSB3801_DRV_TIMER;

typedef struct
{
	struct task_struct *task;
	struct completion comp;
	pid_t pid;
	void *chip;
} wusb3801_thread;
#endif /* __DEBOUNCE_TIMER__ */
/*Working context structure*/
struct wusb3801_chip {
	struct i2c_client *client;
	struct device *dev;
	struct semaphore io_lock;
	struct semaphore suspend_lock;
	struct tcpc_desc *tcpc_desc;
	struct tcpc_device *tcpc;
	struct kthread_worker irq_worker;
	struct kthread_work irq_work;
	struct task_struct *irq_worker_task;
	struct wakeup_source *irq_wake_lock;
	struct wakeup_source *i2c_wake_lock;

	int irq_gpio;
#ifdef __TEST_CC_PATCH__
	uint8_t     cc_test_flag;
	uint8_t     cc_sts;
#endif	/* __TEST_CC_PATCH__ */
	uint8_t     dev_id;
	uint8_t     dev_sub_id;
	int irq;
	int chip_id;
};

#ifdef __TEST_CC_PATCH__
uint8_t	typec_cc_orientation;
#endif	/* __TEST_CC_PATCH__ */

static int wusb3801_read_device(void *client, u32 reg, int len, void *dst)
{
	struct i2c_client *i2c = (struct i2c_client *)client;
	struct wusb3801_chip *chip = i2c_get_clientdata(i2c);
	int ret = 0, count = 5;

	__pm_stay_awake(chip->i2c_wake_lock);
	down(&chip->suspend_lock);
	while (count) {
		if (len > 1) {
			ret = i2c_smbus_read_i2c_block_data(i2c, reg, len, dst);
			if (ret < 0)
				count--;
			else
				goto out;
		} else {
			ret = i2c_smbus_read_byte_data(i2c, reg);
			if (ret < 0)
				count--;
			else {
				*(u8 *)dst = (u8)ret;
				goto out;
			}
		}
		udelay(100);
	}
out:
	up(&chip->suspend_lock);
	__pm_relax(chip->i2c_wake_lock);
	return ret;
}

static int wusb3801_write_device(void *client, u32 reg, int len, const void *src)
{
	const u8 *data;
	struct i2c_client *i2c = (struct i2c_client *)client;
	struct wusb3801_chip *chip = i2c_get_clientdata(i2c);
	int ret = 0, count = 5;

	__pm_stay_awake(chip->i2c_wake_lock);
	down(&chip->suspend_lock);
	while (count) {
		if (len > 1) {
			ret = i2c_smbus_write_i2c_block_data(i2c,
							reg, len, src);
			if (ret < 0)
				count--;
			else
				goto out;
		} else {
			data = src;
			ret = i2c_smbus_write_byte_data(i2c, reg, *data);
			if (ret < 0)
				count--;
			else
				goto out;
		}
		udelay(100);
	}
out:
	up(&chip->suspend_lock);
	__pm_relax(chip->i2c_wake_lock);
	return ret;
}

static int wusb3801_reg_read(struct i2c_client *i2c, u8 reg)
{
	struct wusb3801_chip *chip = i2c_get_clientdata(i2c);
	u8 val = 0;
	int ret = 0;

	ret = wusb3801_read_device(chip->client, reg, 1, &val);
	if (ret < 0) {
		dev_err(chip->dev, "wusb3801 reg read fail\n");
		return ret;
	}
	return val;
}

static int wusb3801_reg_write(struct i2c_client *i2c, u8 reg, const u8 data)
{
	struct wusb3801_chip *chip = i2c_get_clientdata(i2c);
	int ret = 0;

	ret = wusb3801_write_device(chip->client, reg, 1, &data);
	if (ret < 0)
		dev_err(chip->dev, "wusb3801 reg write fail\n");
	return ret;
}

static inline int wusb3801_i2c_write8(
	struct tcpc_device *tcpc, u8 reg, const u8 data)
{
	struct wusb3801_chip *chip = tcpc_get_dev_data(tcpc);

	return wusb3801_reg_write(chip->client, reg, data);
}

static inline int wusb3801_i2c_read8(struct tcpc_device *tcpc, u8 reg)
{
	struct wusb3801_chip *chip = tcpc_get_dev_data(tcpc);

	return wusb3801_reg_read(chip->client, reg);
}

#ifdef __TEST_CC_PATCH__
static int test_cc_patch(struct wusb3801_chip *chip)
{
	int rc_reg_06 = 0;
	int rc_reg_08 = 0, rc_reg_0f = 0, i = 0;

	struct device *cdev = &chip->client->dev;

	rc_reg_08 = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_TEST_02);
	dev_err(cdev, "WUSB3801_REG_08 = 0x%02x-------\n", rc_reg_08);

	wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_02, 0x82);
	do{
		rc_reg_08 = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_TEST_02);
		dev_err(cdev, "WUSB3801_REG_08 = 0x%02x-------000\n", rc_reg_08);
		if(rc_reg_08 != 0x82)
			wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_02, 0x82);
		i++;
	}while(rc_reg_08 != 0x82 && i < 5);

	wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_09, 0xC0);
	i = 0;
	do{
	    rc_reg_0f = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_TEST_09);
        dev_err(cdev, "WUSB3801_REG_0f = 0x%02x-------111\n", rc_reg_0f);
		if(rc_reg_0f != 0xC0)
			wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_09, 0xC0);
		i++;
	}while(rc_reg_0f != 0xC0 && i < 5);

	if((rc_reg_08 == 0x82) && (rc_reg_0f == 0xC0)){
		rc_reg_06 = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_TEST0);
		dev_err(cdev, "WUSB3801_REG_06 = 0x%02x\n", rc_reg_06);
	}
	else
		dev_err(cdev, "WUSB3801_REG_06 error!!!\n");

	wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_09, 0);
	i = 0;
	do{
	    rc_reg_0f = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_TEST_09);
        dev_err(cdev, "WUSB3801_REG_0f = 0x%02x-------222\n", rc_reg_0f);
		if(rc_reg_0f != 0x00)
			wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_09, 0);
		i++;
	}while(rc_reg_0f != 0 && i < 5);

	wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_02, 0x80);
	i = 0;
	do{
		rc_reg_08 = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_TEST_02);
		dev_err(cdev, "WUSB3801_REG_08 = 0x%02x-------333\n", rc_reg_08);
		if(rc_reg_08 != 0x80)
			wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_02, 0x80);
		i++;
	}while(rc_reg_08 != 0x80 && i < 5);

	wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_02, 0);
	i = 0;
	do{
		rc_reg_08 = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_TEST_02);
		dev_err(cdev, "WUSB3801_REG_08 = 0x%02x-------444\n", rc_reg_08);
		if(rc_reg_08 != 0)
			wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_02, 0);
		i++;
	}while(rc_reg_08 != 0 && i < 5);
	msleep(80);
    return BITS_GET(rc_reg_06, 0x40);
}
#endif /* __TEST_CC_PATCH__ */

#if 0
static int wusb3801_regdump(struct wusb3801_chip *chip)
{
	int i, rc = 0;

	tcpci_lock_typec(chip->tcpc);
	pr_err("hzn: wusb3801_regdump\n");
	for (i = WUSB3801_REG_VERSION_ID ; i <= WUSB3801_REG_TEST_12; i++) {
		if(i!=3){
			rc = i2c_smbus_read_byte_data(chip->client, (uint8_t)i);
			if (rc < 0) {
				pr_err("cannot read 0x%02x\n", i);
				rc = 0;
			}
			pr_err("reg[0x%x]=0x%x\n", i, rc);
		}
	}
	tcpci_unlock_typec(chip->tcpc);
	return 0;
}
#endif

static void wusb3801_irq_work_handler(struct kthread_work *work)
{
	struct wusb3801_chip *chip =
			container_of(work, struct wusb3801_chip, irq_work);
    int rc;
    int int_sts;
    uint8_t status, type;

	pr_info("wusb3801:irq_work_handler\n");

	tcpci_lock_typec(chip->tcpc);
	rc = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_INTERRUPT);
	if (rc < 0) {
		pr_err("%s: failed to read interrupt\n", __func__);
		goto work_unlock;
	}

	int_sts = rc & WUSB3801_INT_STS_MASK;

	rc = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_STATUS);
	if (rc < 0) {
		pr_err("%s: failed to read reg status\n", __func__);
		goto work_unlock;
	}

    status = (rc & WUSB3801_ATTACH) ? true : false;
	type = status ? \
			rc & WUSB3801_TYPE_MASK : WUSB3801_TYPE_INVALID;

	pr_err("int_sts[0x%02x], type[0x%02x]\n", int_sts, type);

	if (int_sts & WUSB3801_INT_DETACH) {
		pr_err("wusb3801: WUSB3801_INT_DETACH\n");
#ifdef __TEST_CC_PATCH__
		if (chip->cc_test_flag == 1) {
			pr_err("%s: test_cc_patch not used int and return \n", __func__);
			//tcpci_unlock_typec(chip->tcpc);
			goto work_unlock;
		}
		typec_cc_orientation = 0x0;
#endif	/* __TEST_CC_PATCH__ */

		chip->tcpc->typec_attach_new = TYPEC_UNATTACHED;
		tcpci_report_usb_port_detached(chip->tcpc);
		tcpci_notify_typec_state(chip->tcpc);

		if (chip->tcpc->typec_attach_old == TYPEC_ATTACHED_SRC) {
		    tcpci_source_vbus(chip->tcpc, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_0V, 0);
		}
        chip->tcpc->typec_attach_old = TYPEC_UNATTACHED;
	}

	if (int_sts & WUSB3801_INT_ATTACH) {
		pr_err("wusb3801: WUSB3801_INT_ATTACH\n");
#ifdef __TEST_CC_PATCH__
		if (chip->dev_sub_id != 0xA0) {
			if (chip->cc_test_flag == 0 &&  BITS_GET(rc, WUSB3801_CC_STS_MASK) == 0) {
				chip->cc_sts = test_cc_patch(chip);
				chip->cc_test_flag = 1;
				pr_err("%s: cc_sts[0x%02x]\n", __func__, chip->cc_sts);

				goto work_unlock;
			}
			if (chip->cc_test_flag == 1) {
				chip->cc_test_flag = 0;
				if (BITS_GET(rc, WUSB3801_CC_STS_MASK) == 0) {
					if (chip->cc_sts == WUSB3801_CC2_CONNECTED)
						rc = rc | 0x02;
					else if (chip->cc_sts == WUSB3801_CC1_CONNECTED)
						rc = rc | 0x01;
					pr_err("%s: cc_test_patch rc[0x%02x]\n",
							__func__, rc);
				} else
					pr_err("%s:rc[0x%x},cc is not null\n",
							__func__, rc);
			}
		}
		typec_cc_orientation = BITS_GET(rc, WUSB3801_CC_STS_MASK);
		pr_err("wusb3801: typec_cc_orientation = %d\n", typec_cc_orientation);
#endif	/* __TEST_CC_PATCH__ */

		switch (type) {
			case WUSB3801_TYPE_SNK:

				if(chip->tcpc->typec_attach_new != TYPEC_ATTACHED_SRC){
					chip->tcpc->typec_attach_new = TYPEC_ATTACHED_SRC;
					tcpci_report_usb_port_attached(chip->tcpc);
					tcpci_notify_typec_state(chip->tcpc);
					tcpci_source_vbus(chip->tcpc, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_5V, 0);
					chip->tcpc->typec_attach_old = TYPEC_ATTACHED_SRC;
					pr_err("wusb3801: typec_attached(otg src)\n");
				}
				break;
			case WUSB3801_TYPE_SRC:

				if(chip->tcpc->typec_attach_new != TYPEC_ATTACHED_SNK) {
					chip->tcpc->typec_attach_new = TYPEC_ATTACHED_SNK;
					tcpci_report_usb_port_attached(chip->tcpc);
					tcpci_notify_typec_state(chip->tcpc);
					chip->tcpc->typec_attach_old = TYPEC_ATTACHED_SNK;
					pr_err("wusb3801: typec_attached(usb snk)\n");
				}
				break;
			default:
				pr_err("%s: Unknwon type[0x%02x]\n", __func__, type);
				break;
		}
	}
work_unlock:
	tcpci_unlock_typec(chip->tcpc);
	//wusb3801_regdump(chip);
	enable_irq(chip->irq);
}

static irqreturn_t wusb3801_intr_handler(int irq, void *data)
{
	struct wusb3801_chip *chip = data;

	pr_info("wusb3801:intr_handler\n");

	disable_irq_nosync(irq);
	__pm_wakeup_event(chip->irq_wake_lock, WUSB3801_IRQ_WAKE_TIME);

	kthread_queue_work(&chip->irq_worker, &chip->irq_work);

	return IRQ_HANDLED;
}

int wusb3801_alert_status_clear(struct tcpc_device *tcpc, uint32_t mask)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

static int wusb3801_tcpc_init(struct tcpc_device *tcpc, bool sw_reset)
{
	struct wusb3801_chip *chip = tcpc_get_dev_data(tcpc);
	u8 ret = 0;

	ret = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_CONTROL1);
	if(ret == 0){
		wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_CONTROL1, 0x01);
		pr_err("%s write reg05 0x01\n",__func__);
		mdelay(1);
	}

	enable_irq_wake(chip->irq);

	pr_info("IRQ enable!\n");

	return 0;
}

int wusb3801_fault_status_clear(struct tcpc_device *tcpc, uint8_t status)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

int wusb3801_get_alert_mask(struct tcpc_device *tcpc, uint32_t *mask)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

int wusb3801_get_alert_status(struct tcpc_device *tcpc, uint32_t *alert)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

static int wusb3801_get_power_status(
		struct tcpc_device *tcpc, uint16_t *pwr_status)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

int wusb3801_get_fault_status(struct tcpc_device *tcpc, uint8_t *status)
{

        pr_err("%s enter \n",__func__);
	return 0;
}

static int wusb3801_get_cc(struct tcpc_device *tcpc, int *cc1, int *cc2)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

static int wusb3801_set_cc(struct tcpc_device *tcpc, int pull)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

static int wusb3801_set_polarity(struct tcpc_device *tcpc, int polarity)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

static int wusb3801_set_low_rp_duty(struct tcpc_device *tcpc, bool low_rp)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

static int wusb3801_set_vconn(struct tcpc_device *tcpc, int enable)
{
        pr_err("%s enter \n",__func__);
	return 0;
}

static int wusb3801_tcpc_deinit(struct tcpc_device *tcpc_dev)
{
        pr_err("%s enter \n",__func__);
	return 0;
}


static struct tcpc_ops wusb3801_tcpc_ops = {
	.init = wusb3801_tcpc_init,
	.alert_status_clear = wusb3801_alert_status_clear,
	.fault_status_clear = wusb3801_fault_status_clear,
	.get_alert_mask = wusb3801_get_alert_mask,
	.get_alert_status = wusb3801_get_alert_status,
	.get_power_status = wusb3801_get_power_status,
	.get_fault_status = wusb3801_get_fault_status,
	.get_cc = wusb3801_get_cc,
	.set_cc = wusb3801_set_cc,

	.set_polarity = wusb3801_set_polarity,
	.set_low_rp_duty = wusb3801_set_low_rp_duty,
	.set_vconn = wusb3801_set_vconn,
	.deinit = wusb3801_tcpc_deinit,
};

static int wusb3801_parse_dt(struct wusb3801_chip *chip, struct device *dev)
{
	struct device_node *np = NULL;
	int ret;

	pr_err("%s\n", __func__);

	np = of_find_node_by_name(NULL, "wusb3801_type_c_port0");
	if (!np) {
		pr_err("wusb3801: %s find node wusb3801_type_c_port0 fail\n", __func__);
		return -ENODEV;
	}

	dev->of_node = np;

#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
	ret = of_get_named_gpio(np, "wusb3801,intr_gpio", 0);
	if (ret < 0) {
		pr_err("%s no intr_gpio info\n", __func__);
		return ret;
	}
	chip->irq_gpio = ret;
#else
	ret = of_property_read_u32(np, "wusb3801,irq-gpio-num", &chip->irq_gpio);
	if (ret < 0)
		pr_err("%s no intr_gpio info\n", __func__);
#endif
	return ret;
}

static int wusb3801_tcpcdev_init(struct wusb3801_chip *chip, struct device *dev)
{
	struct tcpc_desc *desc;
	struct device_node *np = dev->of_node;
	u32 val, len;
	const char *name = "default";

	desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	pr_err("wusb3801: %s\n", __func__);

	if (of_property_read_u32(np, "mt-tcpc,role_def", &val) >= 0) {
		if (val >= TYPEC_ROLE_NR)
			desc->role_def = TYPEC_ROLE_DRP;
		else
			desc->role_def = val;
	} else {
		dev_info(dev, "use default Role DRP\n");
		desc->role_def = TYPEC_ROLE_DRP;
	}

	if (of_property_read_u32(np, "mt-tcpc,notifier_supply_num", &val) >= 0) {
		if (val < 0)
			desc->notifier_supply_num = 0;
		else
			desc->notifier_supply_num = val;
	} else{
		desc->notifier_supply_num = 0;
		dev_info(dev, "use default notifier_supply_num\n");
	}

	if (of_property_read_u32(np, "mt-tcpc,rp_level", &val) >= 0) {
		switch (val) {
		case 0: /* RP Default */
			desc->rp_lvl = TYPEC_CC_RP_DFT;
			break;
		case 1: /* RP 1.5V */
			desc->rp_lvl = TYPEC_CC_RP_1_5;
			break;
		case 2: /* RP 3.0V */
			desc->rp_lvl = TYPEC_CC_RP_3_0;
			break;
		default:
			break;
		}
	}

	if (of_property_read_string(np, "wusb-tcpc,name", (char const **)&name) < 0) {
		dev_info(dev, "use default name\n");
	}
	
	len = strlen(name);
	desc->name = kzalloc(len+1, GFP_KERNEL);
	if (!desc->name)
		return -ENOMEM;

	strlcpy((char *)desc->name, name, len+1);

	chip->tcpc_desc = desc;

	chip->tcpc = tcpc_device_register(dev,
			desc, &wusb3801_tcpc_ops, chip);
	if (IS_ERR(chip->tcpc))
		return -EINVAL;

    //chip->tcpc->typec_attach_old = TYPEC_UNATTACHED;
    //chip->tcpc->typec_attach_new = TYPEC_UNATTACHED;
    //tcpci_report_usb_port_detached(chip->tcpc);

	return 0;
}

static int wusb3801_init_alert(struct tcpc_device *tcpc)
{
	struct wusb3801_chip *chip = tcpc_get_dev_data(tcpc);
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	int ret;
	char *name;
	int len;

	len = strlen(chip->tcpc_desc->name);
	name = devm_kzalloc(chip->dev, len+5, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	snprintf(name, PAGE_SIZE, "%s-IRQ", chip->tcpc_desc->name);

	pr_err("%s name = %s, gpio = %d\n", __func__,
				chip->tcpc_desc->name, chip->irq_gpio);

	ret = devm_gpio_request(chip->dev, chip->irq_gpio, name);
	if (ret < 0) {
		pr_err("Error: failed to request GPIO%d (ret = %d)\n",
		chip->irq_gpio, ret);
		goto init_alert_err;
	}

	ret = gpio_direction_input(chip->irq_gpio);
	if (ret < 0) {
		pr_err("Error: failed to set GPIO%d as input pin(ret = %d)\n",
		chip->irq_gpio, ret);
		goto init_alert_err;
	}

	ret = gpio_get_value(chip->irq_gpio);
	pr_err("wusb3801 : IRQ gpio val = %d\n", ret);

	chip->irq = gpio_to_irq(chip->irq_gpio);
	if (chip->irq <= 0) {
		pr_err("%s gpio to irq fail, chip->irq(%d)\n",
						__func__, chip->irq);
		goto init_alert_err;
	}

	pr_err("%s : IRQ number = %d\n", __func__, chip->irq);

	kthread_init_worker(&chip->irq_worker);
	chip->irq_worker_task = kthread_run(kthread_worker_fn,
			&chip->irq_worker, "%s", chip->tcpc_desc->name);
	if (IS_ERR(chip->irq_worker_task)) {
		pr_err("Error: Could not create tcpc task\n");
		goto init_alert_err;
	}

	sched_setscheduler(chip->irq_worker_task, SCHED_FIFO, &param);
	kthread_init_work(&chip->irq_work, wusb3801_irq_work_handler);

	//huanglei add for reg 0x08& 0x0F write zero fail begin
	wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_02, 0x00);
	wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_TEST_09, 0x00);
	wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_CONTROL0, 0x24);
	mdelay(1);
	//huanglei add for reg 0x08& 0x0F write zero fail end

	pr_info("wusb3801: IRQF_NO_THREAD Test\n");

	ret = request_irq(chip->irq, wusb3801_intr_handler,
		IRQF_TRIGGER_LOW | IRQF_NO_THREAD | IRQF_NO_SUSPEND, name, chip);
	if (ret < 0) {
		pr_err("wusb3801: failed to request irq%d (gpio = %d, ret = %d)\n",
			chip->irq, chip->irq_gpio, ret);
		goto init_alert_err;
	}

	return 0;
init_alert_err:
	return -EINVAL;
}


static uint8_t dev_sub_id;
static inline int wusb3801_check_revision(struct i2c_client *client)
{
	int rc;
	rc = i2c_smbus_read_byte_data(client, WUSB3801_REG_VERSION_ID);
	if (rc < 0)
		return rc;

	pr_err("VendorID register: 0x%02x\n", rc );
	if((rc & WUSB3801_VENDOR_ID_MASK) != WUSB3801_VENDOR_ID){
		return -EINVAL;
	}
	pr_err("Vendor id: 0x%02x, Version id: 0x%02x\n", rc & WUSB3801_VENDOR_ID_MASK,
	                                                         (rc & WUSB3801_VERSION_ID_MASK) >> 3);

	rc = i2c_smbus_read_byte_data(client, WUSB3801_REG_TEST_01);
	if (rc > 0)
		dev_sub_id = rc & WUSB3801_VENDOR_SUB_ID_MASK;
	pr_err("VendorSUBID register: 0x%02x\n", rc & WUSB3801_VENDOR_SUB_ID_MASK );

	return WUSB3801_VENDOR_ID;
}
/************************************************************************
 *
 *       fregdump_show
 *
 *  Description :
 *  -------------
 *  Dump registers to user space. there is side-effects for Read/Clear
 *  registers. For example interrupt status.
 *
 ************************************************************************/
static ssize_t wusb3801_regdump_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct wusb3801_chip *chip = i2c_get_clientdata(client);
	int i, rc, ret = 0;

	tcpci_lock_typec(chip->tcpc);
	for (i = WUSB3801_REG_VERSION_ID ; i <= WUSB3801_REG_TEST_12; i++) {
		rc = i2c_smbus_read_byte_data(chip->client, (uint8_t)i);
		if (rc < 0) {
			pr_err("cannot read 0x%02x\n", i);
			rc = 0;
		}
		ret += snprintf (buf + ret, 1024 - ret, "from 0x%02x read 0x%02x\n", (uint8_t)i, rc);
	}
	tcpci_unlock_typec(chip->tcpc);
	return ret;
}

DEVICE_ATTR(wusb3801_regdump, S_IRUGO, wusb3801_regdump_show, NULL);

#ifdef __TEST_CC_PATCH__
/************************************************************************
 *
 *       typec_cc_orientation_show
 *
 *  Description :
 *  -------------
 *  show cc_status
 *
 ************************************************************************/
static ssize_t typec_cc_orientation_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "cc_%d\n", typec_cc_orientation);
	return ret;
}

DEVICE_ATTR(typec_cc_orientation, S_IRUGO, typec_cc_orientation_show, NULL);
#endif /*  __TEST_CC_PATCH__	 */

static int wusb3801_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct wusb3801_chip *chip;
	int ret = 0, chip_id;
	bool use_dt = client->dev.of_node;

	pr_err("%s\n", __func__);

	//+add by hzb for ontim debug
	if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
	{
		return -EIO;
	}
	//-add by hzb for ontim debug

	if (i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_I2C_BLOCK | I2C_FUNC_SMBUS_BYTE_DATA))
		pr_err("I2C functionality : OK...\n");
	else
		pr_err("I2C functionality check : failuare...\n");

	chip_id = wusb3801_check_revision(client);
	// Retry to avoid not receiving the stop bit in some extreme cases
	if (chip_id < 0) {
		chip_id = wusb3801_check_revision(client);
		if (chip_id < 0)
			return chip_id;
	}

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	if (use_dt) {
		ret = wusb3801_parse_dt(chip, &client->dev);
		if (ret < 0)
			return ret;
	} else {
		dev_err(&client->dev, "no dts node\n");
		return -ENODEV;
	}

	chip->dev = &client->dev;
	chip->client = client;
#ifdef __TEST_CC_PATCH__
	chip->cc_sts = 0xFF;
	chip->cc_test_flag = 0;
	chip->dev_sub_id = dev_sub_id;
#endif /* __TEST_CC_PATCH__ */
	sema_init(&chip->io_lock, 1);
	sema_init(&chip->suspend_lock, 1);
	i2c_set_clientdata(client, chip);

	chip->irq_wake_lock = wakeup_source_register(chip->dev, "wusb3801_irq_wakelock");
	chip->i2c_wake_lock = wakeup_source_register(chip->dev, "wusb3801_i2c_wakelock");

	chip->chip_id = chip_id;
	pr_err("wusb3801_chipID = 0x%0x\n", chip_id);

	ret = wusb3801_tcpcdev_init(chip, &client->dev);
	if (ret < 0) {
		dev_err(&client->dev, "wusb3801 tcpc dev init fail\n");
		goto err_tcpc_reg;
	}

	ret = wusb3801_init_alert(chip->tcpc);
	if (ret < 0) {
		pr_err("wusb3801 init alert fail\n");
		goto err_irq_init;
	}

	ret = device_create_file(&client->dev, &dev_attr_wusb3801_regdump);
	if (ret < 0) {
		dev_err(&client->dev, "failed to create dev_attr_fregdump\n");
		ret = -ENODEV;
		goto err_create_fregdump_file;
	}
#ifdef __TEST_CC_PATCH__
	ret = device_create_file(&client->dev, &dev_attr_typec_cc_orientation);
	if (ret < 0) {
		dev_err(&client->dev, "failed to create dev_attr_typec_cc_orientation\n");
		ret = -ENODEV;
		goto err_create_file;
	}
#endif /* __TEST_CC_PATCH__ */

	//+add by hzb for ontim debug
	REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
	//-add by hzb for ontim debug

	pr_err("%s probe OK!\n", __func__);

	return 0;

#ifdef __TEST_CC_PATCH__
err_create_file:
	device_remove_file(&client->dev, &dev_attr_typec_cc_orientation);
#endif /* __TEST_CC_PATCH__ */
err_create_fregdump_file:
	disable_irq(chip->irq);
	free_irq(chip->irq, chip);
	device_remove_file(&client->dev, &dev_attr_wusb3801_regdump);
err_irq_init:
	tcpc_device_unregister(chip->dev, chip->tcpc);
err_tcpc_reg:
	wakeup_source_unregister(chip->i2c_wake_lock);
	wakeup_source_unregister(chip->irq_wake_lock);
	return ret;
}

static int wusb3801_i2c_remove(struct i2c_client *client)
{
	struct wusb3801_chip *chip = i2c_get_clientdata(client);

	if (chip) {
		tcpc_device_unregister(chip->dev, chip->tcpc);
#ifdef __TEST_CC_PATCH__
		device_remove_file(&client->dev, &dev_attr_typec_cc_orientation);
#endif /* __TEST_CC_PATCH__ */
		device_remove_file(&client->dev, &dev_attr_wusb3801_regdump);
	}

	return 0;
}

#ifdef CONFIG_PM
static int wusb3801_i2c_suspend(struct device *dev)
{
	struct wusb3801_chip *chip;
	struct i2c_client *client = to_i2c_client(dev);

	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip) {
			down(&chip->suspend_lock);
		}
	}

	return 0;
}

static int wusb3801_i2c_resume(struct device *dev)
{
	struct wusb3801_chip *chip;
	struct i2c_client *client = to_i2c_client(dev);

	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip)
			up(&chip->suspend_lock);
	}

	return 0;
}

#ifdef CONFIG_ARM64
#define IS_ERR_VALUE_64(x) IS_ERR_VALUE((unsigned long)x)
#else
#define IS_ERR_VALUE_64(x) IS_ERR_VALUE(x)
#endif

/*
 *  Spec lets transitioning to below states from any state
 *  WUSB3801_STATE_DISABLED
 *  WUSB3801_STATE_ERROR_RECOVERY
 */
static int wusb3801_set_chip_state(struct wusb3801_chip *chip, uint8_t state)
{
	int rc = 0;

	if(state > WUSB3801_STATE_UNATTACHED_SRC)
		return -EINVAL;

  	rc = i2c_smbus_write_byte_data(chip->client,
			   WUSB3801_REG_CONTROL1,
			   (state == WUSB3801_STATE_DISABLED) ? \
			             WUSB3801_DISABLED :        \
			             0);

	if (IS_ERR_VALUE_64(rc)) {
		pr_err("failed to write state machine(%d)\n", rc);
	}

	return rc;
}

static int wusb3801_set_mode(struct wusb3801_chip *chip, uint8_t mode)
{
	int rc = 0;

	rc = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_CONTROL0);
	if (rc < 0) {
		pr_err("%s: fail to read mode\n", __func__);
		return rc;
	}
	rc &= ~WUSB3801_MODE_MASK;
        rc &= ~WUSB3801_INT_MASK;//Disable the chip interrupt
	rc |= mode;
        rc = wusb3801_i2c_write8(chip->tcpc,
			   WUSB3801_REG_CONTROL0, rc);

	if (rc < 0) {
		pr_err("failed to write mode(%d)\n", rc);
		return rc;
	}

	//Clear the chip interrupt
	rc = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_INTERRUPT);
	if (rc < 0) {
		pr_err("%s: fail to clear chip interrupt\n", __func__);
		return rc;
	}

	rc = wusb3801_i2c_read8(chip->tcpc, WUSB3801_REG_CONTROL0);
	if (rc < 0) {
		pr_err("%s: fail to read chip interrupt\n", __func__);
		return rc;
	}

	rc |= WUSB3801_INT_MASK;//enable the chip interrupt
	rc = wusb3801_i2c_write8(chip->tcpc, WUSB3801_REG_CONTROL0, rc);
	if (rc < 0) {
		pr_err("failed to enable chip interrupt(%d)\n", rc);
		return rc;
	}

	return rc;
}

static void wusb3801_shutdown(struct i2c_client *client)
{
	struct wusb3801_chip *chip = i2c_get_clientdata(client);

	pr_info("wusb3801: wusb3801_shutdown\n");

	if (chip != NULL) {
		if (chip->irq){
			disable_irq(chip->irq);
			free_irq(chip->irq, chip);
		}
		tcpm_shutdown(chip->tcpc);
	}

	if (IS_ERR_VALUE_64(wusb3801_set_mode(chip, WUSB3801_SNK)) ||
			IS_ERR_VALUE_64(wusb3801_set_chip_state(chip, WUSB3801_STATE_ERROR_RECOVERY)))
		pr_err("%s: failed to set sink mode\n", __func__);

}


static const struct dev_pm_ops wusb3801_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(
			wusb3801_i2c_suspend,
			wusb3801_i2c_resume)
};

#define wusb3801_PM_OPS	(&wusb3801_pm_ops)
#else
#define wusb3801_PM_OPS	(NULL)
#endif /* CONFIG_PM */

static const struct i2c_device_id wusb3801_id_table[] = {
	{"wusb3801", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, wusb3801_id_table);

static const struct of_device_id wusb3801_match_table[] = {
	{.compatible = "mediatek,usb_type_c",},
	{},
};


static struct i2c_driver wusb3801_driver = {
	.driver = {
		.name = "usb_type_c",
		.owner = THIS_MODULE,
		.of_match_table = wusb3801_match_table,
		.pm = wusb3801_PM_OPS,
	},
	.probe = wusb3801_i2c_probe,
	.remove = wusb3801_i2c_remove,
	.shutdown = wusb3801_shutdown,
	.id_table = wusb3801_id_table,
};

static int __init wusb3801_init(void)
{
	struct device_node *np;

	pr_err("%s (%s): initializing...\n", __func__, WUSB3801_DRV_VERSION);
	np = of_find_node_by_name(NULL, "usb_type_c");
	if (np != NULL)
		pr_err("usb_type_c node found...\n");
	else
		pr_err("usb_type_c node not found...\n");

	return i2c_add_driver(&wusb3801_driver);
}
subsys_initcall(wusb3801_init);

static void __exit wusb3801_exit(void)
{
	i2c_del_driver(&wusb3801_driver);
}
module_exit(wusb3801_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("wusb3801 TCPC Driver");
MODULE_VERSION(WUSB3801_DRV_VERSION);
