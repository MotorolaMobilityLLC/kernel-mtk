/*
 * Copyright (C) 2021 MediaTek Inc.
 *
 * Mediatek sgm7220 Type-C Port Control Driver
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
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
#include "inc/pd_dbg_info.h"
#include "inc/tcpci.h"
#include "inc/tcpci_timer.h"
#include "inc/tcpci_typec.h"
#include "inc/sgm7220.h"
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/hrtimer.h>
#include <linux/sched/rt.h>

#define SGM7220_DRV_VERSION	"1.0.0_MTK"
#define SGM7220_IRQ_WAKE_TIME	(500) /* ms */

/******************************************************************************
 * Constants
 ******************************************************************************/

enum drp_toggle_type {
	TOGGLE_DFP_DRP_30 = 0,
	TOGGLE_DFP_DRP_40,
	TOGGLE_DFP_DRP_50,
	TOGGLE_DFP_DRP_60
};

enum current_adv_type {
	HOST_CUR_USB = 0,   /*default 500mA or 900mA*/
	HOST_CUR_1P5,      /*1.5A*/
	HOST_CUR_3A       /*3A*/
};

enum current_det_type {
	DET_CUR_USB = 0,    /*default 500mA or 900mA*/
	DET_CUR_1P5,
	DET_CUR_ACCESSORY,  /*charg through accessory 500mA*/
	DET_CUR_3A
};

enum accessory_attach_type {
	ACCESSORY_NOT_ATTACHED = 0,
	ACCESSORY_AUDIO = 4,
	ACCESSORY_CHG_THRU_AUDIO = 5,
	ACCESSORY_DEBUG = 6
};

enum cable_attach_type {
	CABLE_NOT_ATTACHED = 0,
	CABLE_ATTACHED
};

enum cable_state_type {
	CABLE_STATE_NOT_ATTACHED = 0,
	CABLE_STATE_AS_DFP,
	CABLE_STATE_AS_UFP,
	CABLE_STATE_TO_ACCESSORY
};

enum cable_dir_type {
	ORIENT_CC1 = 0,
	ORIENT_CC2
};

enum cc_modes_type {
	MODE_DEFAULT = 0,
	MODE_UFP,
	MODE_DFP,
	MODE_DRP
};

/* Type-C Attrs */
struct type_c_parameters {
	enum current_det_type current_det;         /*charging current on UFP*/
	enum accessory_attach_type accessory_attach;     /*if an accessory is attached*/
	enum cable_attach_type active_cable_attach;         /*if an active_cable is attached*/
	enum cable_state_type attach_state;        /*DFP->UFP or UFP->DFP*/
	enum cable_dir_type cable_dir;           /*cc1 or cc2*/
};

struct state_disorder_monitor {
	int count;
	int err_detected;
	enum cable_state_type former_state;
	unsigned long time_before;
};

struct sgm7220_chip {
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

	atomic_t poll_count;
	struct delayed_work	poll_work;
	struct delayed_work	first_check_typec_work;

	int irq_gpio;
	uint8_t     dev_id;
	uint8_t     dev_sub_id;
	int irq;
	int chip_id;
	struct type_c_parameters type_c_param;
	struct state_disorder_monitor monitor;
};

static struct i2c_client *w_client;

static int sgm7220_read_device(void *client, u32 reg, int len, void *dst)
{
	struct i2c_client *i2c = (struct i2c_client *)client;
	struct sgm7220_chip *chip = i2c_get_clientdata(i2c);
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

static int sgm7220_write_device(void *client, u32 reg, int len, const void *src)
{
	const u8 *data;
	struct i2c_client *i2c = (struct i2c_client *)client;
	struct sgm7220_chip *chip = i2c_get_clientdata(i2c);
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

static int sgm7220_reg_write(struct i2c_client *i2c, u8 reg, const u8 data)
{
	struct sgm7220_chip *chip = i2c_get_clientdata(i2c);
	int ret = 0;

	ret = sgm7220_write_device(chip->client, reg, 1, &data);
	if (ret < 0)
		dev_err(chip->dev, "sgm7220 reg write fail\n");
	return ret;
}

static int sgm7220_reg_update(struct i2c_client *i2c, u8 reg, const u8 data, u8 mask)
{
	struct sgm7220_chip *chip = i2c_get_clientdata(i2c);
	int ret;
	u8 old_val, new_val;

	ret = sgm7220_read_device(chip->client, reg, 1, &old_val);
	if (ret < 0) {
		dev_err(chip->dev, "sgm7220 reg read fail\n");
		return ret;
	}

	if (ret >= 0) {
		old_val = ret & 0xff;
		new_val = (data & mask) | (old_val & (~mask));
		ret = sgm7220_write_device(chip->client, reg, 1, &new_val);
		if (ret < 0)
			dev_err(chip->dev, "sgm7220 reg write fail\n");
	}
	return ret;
}

/* Detect non-DFP->DFP changes that happen more than 3 times within 10 Secs */
static void sgm7220_state_disorder_detect(struct sgm7220_chip *chip)
{
	unsigned long timeout;

	/* count the (non-DFP -> DFP) changes */
	if ((chip->monitor.former_state != chip->type_c_param.attach_state)
	    && (chip->type_c_param.attach_state == CABLE_STATE_AS_DFP)) {
		if (!chip->monitor.count) {
			chip->monitor.time_before = jiffies;
		}
		chip->monitor.count++;
	}

	/* store the state */
	chip->monitor.former_state = chip->type_c_param.attach_state;

	if (chip->monitor.count > 3) {
		timeout = msecs_to_jiffies(10 * 1000); /* 10 Seconds */
		if (time_before(jiffies, chip->monitor.time_before + timeout)) {
			chip->monitor.err_detected = 1;
			/* disbale id irq before qpnp react to cc chip's id output */
			//interfere_id_irq_from_usb(0);
		}
		chip->monitor.count = 0;
	}

	if ((chip->type_c_param.attach_state == CABLE_STATE_NOT_ATTACHED)
	    && chip->monitor.err_detected) {
		/* enable id irq */
		//interfere_id_irq_from_usb(1);
		chip->monitor.err_detected = 0;
	}
}

static void sgm7220_process_mode_register(struct sgm7220_chip *chip, u8 status)
{
	u8 val;
	u8 tmp = status;

	/* check current_detect */
	val = ((tmp & MOD_CURRENT_MODE_DETECT) >> MOD_CURRENT_MODE_DETECT_SHIFT);
	chip->type_c_param.current_det = val;

	/* check accessory attch */
	tmp = status;
	val = ((tmp & MOD_ACCESSORY_CONNECTED) >> MOD_ACCESSORY_CONNECTED_SHIFT);
	chip->type_c_param.accessory_attach = val;

	/* check cable attach */
	tmp = status;
	val = (tmp & MOD_ACTIVE_CABLE_DETECTION);
}

static void sgm7220_process_interrupt_register(struct sgm7220_chip *chip, u8 status)
{
	struct tcpc_device *tcpc;
	u8 val;
	u8 tmp = status;

	tcpc = chip->tcpc;

	/* check attach state */
	val = ((tmp & INT_ATTACHED_STATE) >> INT_ATTACHED_STATE_SHIFT);
	chip->type_c_param.attach_state = val;

	/* update current adv when act as DFP */
	if (chip->type_c_param.attach_state == CABLE_STATE_AS_DFP ||
	    chip->type_c_param.attach_state == CABLE_STATE_TO_ACCESSORY) {
		val = (HOST_CUR_USB << MOD_CURRENT_MODE_ADVERTISE_SHIFT);
	} else {
		val = (HOST_CUR_3A << MOD_CURRENT_MODE_ADVERTISE_SHIFT);
	}
	sgm7220_reg_update(chip->client, REG_MOD, val, MOD_CURRENT_MODE_ADVERTISE);

	/* in case configured as DRP may detect some non-standard SDP */
	/* chargers as UFP, which may lead to a cyclic switching of DFP */
	/* and UFP on state detection result. */
	sgm7220_state_disorder_detect(chip);

	/* check cable dir */
	tmp = status;
	val = ((tmp & INT_CABLE_DIR) >> INT_CABLE_DIR_SHIFT);
	chip->type_c_param.cable_dir = val;
	tcpc->typec_polarity = chip->type_c_param.cable_dir;
	pr_info("%s type=%d,cc_stat(%d)\n", __func__, chip->type_c_param.attach_state, tcpc->typec_polarity);
	pr_info("%s attach_new=%d,attach_old=%d\n", __func__, tcpc->typec_attach_new, tcpc->typec_attach_old);

	switch (chip->type_c_param.attach_state) {
	case SGM7220_TYPE_NOT_ATTACHED:
		tcpc->typec_attach_new = TYPEC_UNATTACHED;
		tcpci_report_usb_port_changed(tcpc);
		//tcpci_notify_typec_state(tcpc);
		if (tcpc->typec_attach_old == TYPEC_ATTACHED_SRC) {
		    tcpci_source_vbus(tcpc, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_0V, 0);
		}
		tcpc->typec_attach_old = TYPEC_UNATTACHED;
		break;
	case SGM7220_TYPE_SNK:
		if (tcpc->typec_attach_new != TYPEC_ATTACHED_SRC) {
				tcpc->typec_attach_new = TYPEC_ATTACHED_SRC;
				tcpci_report_usb_port_changed(tcpc);
				tcpci_source_vbus(tcpc, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_5V, 0);
				//tcpci_notify_typec_state(tcpc);
				tcpc->typec_attach_old = TYPEC_ATTACHED_SRC;
		}
		break;
	case SGM7220_TYPE_SRC:
		if (tcpc->typec_attach_new != TYPEC_ATTACHED_SNK) {
				tcpc->typec_attach_new = TYPEC_ATTACHED_SNK;
				tcpci_report_usb_port_changed(tcpc);
				//tcpci_notify_typec_state(tcpc);
				tcpc->typec_attach_old = TYPEC_ATTACHED_SNK;
		}
		break;
	default:
		pr_err("%s: Unknwon type[0x%02x]\n", __func__, chip->type_c_param.attach_state);
		break;
	}
}

static int first_check_flag = 0;
static void sgm7220_irq_work_handler(struct kthread_work *work)
{
	struct sgm7220_chip *chip = container_of(work, struct sgm7220_chip, irq_work);
	struct tcpc_device *tcpc;
	u8 reg_val;
	int ret;

	if (0 == first_check_flag)
		return;

	tcpc = chip->tcpc;
	tcpci_lock_typec(tcpc);

	pr_info("%s enter\n", __func__);

	ret = sgm7220_read_device(chip->client, REG_MOD, 1, &reg_val);
	if (ret < 0) {
		pr_err("%s: read reg fail!\n", __func__);
	}

	pr_info("%s mode_register:0x%x\n", __func__, reg_val);
	sgm7220_process_mode_register(chip, reg_val);

	ret = sgm7220_read_device(chip->client, REG_INT, 1, &reg_val);
	if (ret < 0) {
		pr_err("%s: read reg fail!\n", __func__);
	}

	pr_info("%s interrupt_register:0x%x\n", __func__, reg_val);
	sgm7220_process_interrupt_register(chip, reg_val);

	ret = sgm7220_reg_update(chip->client, REG_INT, (0x1 << INT_INTERRUPT_STATUS_SHIFT), INT_INTERRUPT_STATUS);
	if (ret < 0) {
		pr_err("%s: update reg fail!\n", __func__);
	}

	tcpci_unlock_typec(tcpc);
}


static irqreturn_t sgm7220_intr_handler(int irq, void *data)
{
	struct sgm7220_chip *chip = data;

	__pm_wakeup_event(chip->irq_wake_lock, SGM7220_IRQ_WAKE_TIME);

	kthread_queue_work(&chip->irq_worker, &chip->irq_work);
	return IRQ_HANDLED;
}

static int sgm7220_init_alert(struct tcpc_device *tcpc)
{
	struct sgm7220_chip *chip = tcpc_get_dev_data(tcpc);
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	int ret;
	char *name;
	int len;
	u8 reg_val;

	len = strlen(chip->tcpc_desc->name);
	name = devm_kzalloc(chip->dev, len+5, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	snprintf (name, PAGE_SIZE, "%s-IRQ", chip->tcpc_desc->name);

	pr_info("%s name = %s, gpio = %d\n", __func__,
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

	chip->irq = gpio_to_irq(chip->irq_gpio);
	if (chip->irq <= 0) {
		pr_err("%s gpio to irq fail, chip->irq(%d)\n",
						__func__, chip->irq);
		goto init_alert_err;
	}

	pr_info("%s : IRQ number = %d\n", __func__, chip->irq);

	kthread_init_worker(&chip->irq_worker);
	chip->irq_worker_task = kthread_run(kthread_worker_fn,
			&chip->irq_worker, "chip->tcpc_desc->name");
	if (IS_ERR(chip->irq_worker_task)) {
		pr_err("Error: Could not create tcpc task\n");
		goto init_alert_err;
	}

	sched_setscheduler(chip->irq_worker_task, SCHED_FIFO, &param);
	kthread_init_work(&chip->irq_work, sgm7220_irq_work_handler);

	ret = sgm7220_read_device(chip->client, REG_INT, 1, &reg_val);
	if (ret < 0) {
		pr_err("%s: read reg fail!\n", __func__);
	} else {
		pr_info("%s interrupt_register:0x%x\n", __func__, reg_val);
	}
	
	ret = i2c_smbus_write_byte_data(chip->client, REG_INT, reg_val&0xff);//first interrupt status
	if (ret < 0) {
		pr_err("%s: write reg fail!\n", __func__);
	}

	ret = request_irq(chip->irq, sgm7220_intr_handler,
		IRQF_TRIGGER_FALLING | IRQF_NO_THREAD |
		IRQF_NO_SUSPEND, name, chip);
	if (ret < 0) {
		pr_err("Error: failed to request irq%d (gpio = %d, ret = %d)\n",
			chip->irq, chip->irq_gpio, ret);
		goto init_alert_err;
	}

	enable_irq_wake(chip->irq);
	return 0;
init_alert_err:
	return -EINVAL;
}

int sgm7220_alert_status_clear(struct tcpc_device *tcpc, uint32_t mask)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

static int sgm7220_tcpc_init(struct tcpc_device *tcpc, bool sw_reset)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

int sgm7220_fault_status_clear(struct tcpc_device *tcpc, uint8_t status)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

int sgm7220_get_alert_mask(struct tcpc_device *tcpc, uint32_t *mask)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

int sgm7220_get_alert_status(struct tcpc_device *tcpc, uint32_t *alert)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

static int sgm7220_get_power_status(struct tcpc_device *tcpc, uint16_t *pwr_status)
{
	pr_info("%s enter \n", __func__);
	*pwr_status = 0;
	return 0;
}

int sgm7220_get_fault_status(struct tcpc_device *tcpc, uint8_t *status)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

static int sgm7220_get_cc(struct tcpc_device *tcpc, int *cc1, int *cc2)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

static int sgm7220_set_cc(struct tcpc_device *tcpc, int pull)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

static int sgm7220_set_polarity(struct tcpc_device *tcpc, int polarity)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

static int sgm7220_set_low_rp_duty(struct tcpc_device *tcpc, bool low_rp)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

static int sgm7220_set_vconn(struct tcpc_device *tcpc, int enable)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

static int sgm7220_tcpc_deinit(struct tcpc_device *tcpc_dev)
{
	pr_info("%s enter \n", __func__);
	return 0;
}

static struct tcpc_ops sgm7220_tcpc_ops = {
	.init = sgm7220_tcpc_init,
	.alert_status_clear = sgm7220_alert_status_clear,
	.fault_status_clear = sgm7220_fault_status_clear,
	.get_alert_mask = sgm7220_get_alert_mask,
	.get_alert_status = sgm7220_get_alert_status,
	.get_power_status = sgm7220_get_power_status,
	.get_fault_status = sgm7220_get_fault_status,
	.get_cc = sgm7220_get_cc,
	.set_cc = sgm7220_set_cc,
	.set_polarity = sgm7220_set_polarity,
	.set_low_rp_duty = sgm7220_set_low_rp_duty,
	.set_vconn = sgm7220_set_vconn,
	.deinit = sgm7220_tcpc_deinit,
};


static int mt_parse_dt(struct sgm7220_chip *chip, struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret;

	if (!np)
		return -EINVAL;

	pr_info("%s\n", __func__);

	np = of_find_node_by_name(NULL, "type_c_port0");
	if (!np) {
		pr_err("%s find node type_c_port0 fail\n", __func__);
		return -ENODEV;
	}

#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
	ret = of_get_named_gpio(np, "mt6370pd,intr_gpio", 0);
	if (ret < 0) {
		pr_err("%s no intr_gpio info\n", __func__);
		return ret;
	}
	chip->irq_gpio = ret;
#else
	ret = of_property_read_u32(
		np, "mt6370pd,intr_gpio_num", &chip->irq_gpio);
	if (ret < 0)
		pr_err("%s no intr_gpio info\n", __func__);
#endif
	return ret;
}

static void sgm7220_first_check_typec_work(struct work_struct *work)
{
	struct sgm7220_chip *chip = container_of(work, struct sgm7220_chip, first_check_typec_work.work);
	struct tcpc_device *tcpc;
	u8 reg_val;
	int ret;

	tcpc = chip->tcpc;
	tcpci_lock_typec(tcpc);

	first_check_flag = 1;

	pr_info("%s enter\n", __func__);

	ret = sgm7220_read_device(chip->client, REG_MOD, 1, &reg_val);
	if (ret < 0) {
		pr_err("%s: read reg fail!\n", __func__);
	}

	pr_info("%s mode_register:0x%x\n", __func__, reg_val);
	sgm7220_process_mode_register(chip, reg_val);

	ret = sgm7220_read_device(chip->client, REG_INT, 1, &reg_val);
	if (ret < 0) {
		pr_err("%s: read reg fail!\n", __func__);
	}

	pr_info("%s interrupt_register:0x%x\n", __func__, reg_val);
	sgm7220_process_interrupt_register(chip, reg_val);

	ret = sgm7220_reg_update(chip->client,
				   REG_INT, (0x1 << INT_INTERRUPT_STATUS_SHIFT), INT_INTERRUPT_STATUS);
	if (ret < 0) {
		pr_err("%s: update reg fail!\n", __func__);
	}

	tcpci_unlock_typec(tcpc);
}

static int sgm7220_tcpcdev_init(struct sgm7220_chip *chip, struct device *dev)
{
	struct tcpc_desc *desc;
	struct device_node *np;
	u32 val, len;

	const char *name = "default";

	np = of_find_node_by_name(NULL, "type_c_port0");
	if (!np) {
		pr_err("%s find node mt6370 fail\n", __func__);
		return -ENODEV;
	}

	desc = devm_kzalloc(dev, sizeof (*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	if (of_property_read_u32(np, "mt-tcpc,role_def", &val) >= 0) {
		if (val >= TYPEC_ROLE_NR)
			desc->role_def = TYPEC_ROLE_DRP;
		else
			desc->role_def = val;
	} else {
		dev_info(dev, "use default Role DRP\n");
		desc->role_def = TYPEC_ROLE_DRP;
	}

	if (of_property_read_u32(
		np, "mt-tcpc,notifier_supply_num", &val) >= 0) {
		if (val < 0)
			desc->notifier_supply_num = 0;
		else
			desc->notifier_supply_num = val;
	} else
		desc->notifier_supply_num = 0;

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
	desc->rp_lvl = TYPEC_CC_RP_1_5;

	of_property_read_string(np, "mt-tcpc,name", (char const **)&name);

	len = strlen(name);
	desc->name = kzalloc(len+1, GFP_KERNEL);
	if (!desc->name)
		return -ENOMEM;

	strlcpy((char *)desc->name, name, len+1);

	chip->tcpc_desc = desc;

	chip->tcpc = tcpc_device_register(dev,
			desc, &sgm7220_tcpc_ops, chip);
	if (IS_ERR(chip->tcpc))
		return -EINVAL;

	//chip->tcpc->typec_attach_old = !TYPEC_UNATTACHED;
	//chip->tcpc->typec_attach_new = TYPEC_UNATTACHED;
	//tcpci_report_usb_port_changed(chip->tcpc);
	schedule_delayed_work(&chip->first_check_typec_work, msecs_to_jiffies(3000));
	pr_info("%s end\n", __func__);
	return 0;
}

static int sgm7220_initialization(struct sgm7220_chip *chip)
{
	int ret = 0;
	u8 reg_val;
	/* do initialization here, before enable irq,
	 * clear irq,
	 * config DRP/UFP/DFP mode,
	 * and etc..
	 */
	pr_info("%s enter\n", __func__);
	ret = sgm7220_read_device(chip->client, REG_REVISION, 1, &reg_val);
	if (ret < 0) {
		return ret;
	}

	/* 1 Disable term(default 0) [bit 0]
	 * 2 Source pref(DRP performs try.SNK 01) [bit 2:1]
	 * 3 Soft reset(default 0) [bit 3]
	 * 4 Mode slect(DRP start from Try.SNK 00) [bit 5:4]
	 * 5 Debounce time of voltage change on CC pin(default 0) [bit 7:6]
	 */
	reg_val = 0x32;
	ret = sgm7220_reg_write(chip->client, REG_SET, reg_val);
	if (ret < 0) {
		pr_err("%s: init REG_SET fail!\n", __func__);
		return ret;
	}

	/* CURRENT MODE ADVERTISE 3A [bit 7:6] */
	reg_val = (HOST_CUR_3A << MOD_CURRENT_MODE_ADVERTISE_SHIFT);
	ret = sgm7220_reg_update(chip->client, REG_MOD, reg_val, MOD_CURRENT_MODE_ADVERTISE);
	if (ret < 0) {
		pr_err("%s: init REG_MOD fail!\n", __func__);
		return ret;
	}

	return ret;
}

/************************************************************************
 *
 *       chip_info_show
 *
 ************************************************************************/
static ssize_t chip_info_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sgm7220_chip *chip = i2c_get_clientdata(client);
	u8 reg_val;
	char name[ID_REG_LEN + 1] = { 0 };
	u8 revision = 0;
	int i, j, ret;

	for (i = 0, j = 0; i < ID_REG_LEN; i++) {
		reg_val = 0;
		ret = sgm7220_read_device(chip->client, (REG_BASE + (ID_REG_LEN - 1) - i), 1, &reg_val);
		if (ret < 0) {
			pr_err("%s: read reg fail!\n", __func__);
			snprintf(name, ID_REG_LEN, "%s", "Error");
			break;
		}
		if (!reg_val) {
			j++;
			continue;
		}
		name[i - j] = (char)reg_val;
	}

	ret = sgm7220_read_device(chip->client, REG_REVISION, 1, &revision);
	if (ret < 0) {
		pr_err("%s: read reg fail!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "USB TypeC chip info:\n"
				"	Manufacture: TI\n"
				"	Chip Name  : %s\n"
				"	Revision ID : Error\n",
				name);
	} else {
		return snprintf(buf, PAGE_SIZE, "USB TypeC chip info:\n"
				"	Manufacture: TI\n"
				"	Chip Name  : %s\n"
				"	Revision ID : %d\n",
				name, revision);
	}
}
DEVICE_ATTR(chip_info, S_IRUGO, chip_info_show, NULL);

/************************************************************************
 *
 *       cc_orientation_show
 *
 ************************************************************************/
static ssize_t cc_orientation_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sgm7220_chip *chip = i2c_get_clientdata(client);
	int ret;

	ret = snprintf (buf, PAGE_SIZE, "cc_sts (%d)\n", chip->type_c_param.cable_dir);

	return ret;
}
DEVICE_ATTR(cc_orientation, S_IRUGO, cc_orientation_show, NULL);

static int sgm7220_check_revision(struct i2c_client *client)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, REG_REVISION);
	if (ret < 0) {
		dev_err(&client->dev, "read chip ID fail\n");
		return -EIO;
	}

	return ret;
}

static int sgm7220_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct sgm7220_chip *chip;
	int ret = 0, chip_id = 0;
	bool use_dt = client->dev.of_node;

	pr_info("%s\n", __func__);
	if (i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_I2C_BLOCK | I2C_FUNC_SMBUS_BYTE_DATA))
		pr_info("I2C functionality : OK...\n");
	else
		pr_info("I2C functionality check : failuare...\n");
		
	chip_id = sgm7220_check_revision(client);
	if (chip_id < 0)
		return chip_id;

	chip = devm_kzalloc(&client->dev, sizeof (*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	if (use_dt)
		mt_parse_dt(chip, &client->dev);
	else {
		dev_err(&client->dev, "no dts node\n");
		return -ENODEV;
	}
	chip->dev = &client->dev;
	chip->client = client;
	sema_init(&chip->io_lock, 1);
	sema_init(&chip->suspend_lock, 1);
	i2c_set_clientdata(client, chip);
	w_client = chip->client;
	INIT_DELAYED_WORK(&chip->first_check_typec_work, sgm7220_first_check_typec_work);

	chip->irq_wake_lock = wakeup_source_register(chip->dev, "sgm7220_irq_wakelock");
	if (!chip->irq_wake_lock) {
		pr_err("%s: irq wakeup source request failed\n", __func__);
		goto err_irq_wake_lock;
	}

	chip->i2c_wake_lock = wakeup_source_register(chip->dev, "sgm7220_i2c_wakelock");
	if (!chip->i2c_wake_lock) {
		pr_err("%s: i2c wakeup source request failed\n", __func__);
		goto err_i2c_wake_lock;
	}

	chip->chip_id = chip_id;
	pr_info("sgm7220_chipID = 0x%0x\n", chip_id);

	/* try initialize device before request irq */
	ret = sgm7220_initialization(chip);
	if (ret < 0) {
		pr_err("%s: fails to do initialization %d\n", __func__, ret);
		goto err_init_dev;
	}

	ret = sgm7220_tcpcdev_init(chip, &client->dev);
	if (ret < 0) {
		dev_err(&client->dev, "sgm7220 tcpc dev init fail\n");
		goto err_tcpc_reg;
	}

	ret = sgm7220_init_alert(chip->tcpc);
	if (ret < 0) {
		pr_err("sgm7220 init alert fail\n");
		goto err_irq_init;
	}

	ret = device_create_file(&client->dev, &dev_attr_chip_info);
	if (ret < 0) {
		dev_err(&client->dev, "failed to create dev_attr_chip_info\n");
		ret = -ENODEV;
		goto err_create_chip_info_file;
	}

	ret = device_create_file(&client->dev, &dev_attr_cc_orientation);
	if (ret < 0) {
		dev_err(&client->dev, "failed to create dev_attr_cc_orientation\n");
		ret = -ENODEV;
		goto err_create_cc_orientation_file;
	}

	tcpc_schedule_init_work(chip->tcpc);
	pr_info("%s probe OK!\n", __func__);
	return 0;
err_create_cc_orientation_file:
	device_remove_file(&client->dev, &dev_attr_cc_orientation);
err_create_chip_info_file:
	device_remove_file(&client->dev, &dev_attr_chip_info);
err_irq_init:
	tcpc_device_unregister(chip->dev, chip->tcpc);
err_tcpc_reg:
	wakeup_source_unregister(chip->i2c_wake_lock);
err_i2c_wake_lock:
	wakeup_source_unregister(chip->irq_wake_lock);
err_irq_wake_lock:
err_init_dev:
	i2c_set_clientdata(client, NULL);

	return ret;
}

static int sgm7220_i2c_remove(struct i2c_client *client)
{
	struct sgm7220_chip *chip = i2c_get_clientdata(client);

	if (chip) {
		cancel_delayed_work_sync(&chip->first_check_typec_work);
		tcpc_device_unregister(chip->dev, chip->tcpc);
		device_remove_file(&client->dev, &dev_attr_cc_orientation);
		device_remove_file(&client->dev, &dev_attr_chip_info);
	}

	return 0;
}

#ifdef CONFIG_PM
static int sgm7220_i2c_suspend(struct device *dev)
{
	struct sgm7220_chip *chip;
	struct i2c_client *client = to_i2c_client(dev);

	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip) {
			down(&chip->suspend_lock);
		}
	}

	return 0;
}

static int sgm7220_i2c_resume(struct device *dev)
{
	struct sgm7220_chip *chip;
	struct i2c_client *client = to_i2c_client(dev);

	if (client) {
		chip = i2c_get_clientdata(client);
		if (chip)
			up(&chip->suspend_lock);
	}

	return 0;
}

static void sgm7220_shutdown(struct i2c_client *client)
{
	struct sgm7220_chip *chip = i2c_get_clientdata(client);

	if (chip != NULL) {
		if (chip->irq)
			disable_irq(chip->irq);
	}
}

#ifdef CONFIG_PM_RUNTIME
static int sgm7220_pm_suspend_runtime(struct device *device)
{
	dev_dbg(device, "pm_runtime: suspending...\n");
	return 0;
}

static int sgm7220_pm_resume_runtime(struct device *device)
{
	dev_dbg(device, "pm_runtime: resuming...\n");
	return 0;
}
#endif /* CONFIG_PM_RUNTIME */


static const struct dev_pm_ops sgm7220_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(
			sgm7220_i2c_suspend,
			sgm7220_i2c_resume)
#ifdef CONFIG_PM_RUNTIME
	SET_RUNTIME_PM_OPS(
		sgm7220_pm_suspend_runtime,
		sgm7220_pm_resume_runtime,
		NULL
	)
#endif /* CONFIG_PM_RUNTIME */
};
#define sgm7220_PM_OPS	(&sgm7220_pm_ops)
#else
#define sgm7220_PM_OPS	(NULL)
#endif /* CONFIG_PM */

static const struct i2c_device_id sgm7220_id_table[] = {
	{"sgm7220", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, sgm7220_id_table);

static const struct of_device_id rt_match_table[] = {
	{.compatible = "mediatek,usb_type_c_sgm7220",},
	{},
};

static struct i2c_driver sgm7220_driver = {
	.driver = {
		.name = "usb_type_c_sgm7220",
		.owner = THIS_MODULE,
		.of_match_table = rt_match_table,
		.pm = sgm7220_PM_OPS,
	},
	.probe = sgm7220_i2c_probe,
	.remove = sgm7220_i2c_remove,
	.shutdown = sgm7220_shutdown,
	.id_table = sgm7220_id_table,
};

static int __init sgm7220_init(void)
{
	struct device_node *np;

	pr_info("%s (%s): initializing...\n", __func__, SGM7220_DRV_VERSION);
	np = of_find_node_by_name(NULL, "usb_type_c_sgm7220");
	if (np != NULL)
		pr_info("usb_type_c node found...\n");
	else
		pr_info("usb_type_c node not found...\n");

	return i2c_add_driver(&sgm7220_driver);
}
subsys_initcall(sgm7220_init);

static void __exit sgm7220_exit(void)
{
	i2c_del_driver(&sgm7220_driver);
}
module_exit(sgm7220_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sgm7220 TCPC Driver");
MODULE_VERSION(SGM7220_DRV_VERSION);
