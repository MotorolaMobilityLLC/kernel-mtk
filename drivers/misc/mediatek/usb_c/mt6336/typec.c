/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/notifier.h>
#include <typec.h>
#include "usb_switch.h"

#ifdef CONFIG_MTK_LEGACY
#include <cust_gpio_usage.h>
#endif

#ifdef CONFIG_MTK_LEGACY
#include <cust_gpio_usage.h>
#endif

#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
#include <mt-plat/mtk_boot_common.h>
#endif

#include <mtk_auxadc_intf.h>
#include <mt6336/mt6336.h>

#include "typec-ioctl.h"
#include "mtk_typec.h"
#include "typec_reg.h"

struct typec_hba *g_hba;

#if USE_AUXADC

#define AUXADC_TYPEC_H_DEBT_MAX 0x37F
#define AUXADC_TYPEC_H_DET_PRD_15_0_L 0x381
#define AUXADC_TYPEC_H_DET_PRD_15_0_H 0x382
#define AUXADC_TYPEC_H_VOLT_MAX_L 0x384

#define AUXADC_TYPEC_H3_H 0x385
#define AUXADC_TYPEC_H_MAX_IRQ_B (0x1<<7)
#define AUXADC_TYPEC_H_EN_MAX (0x1<<5)
#define AUXADC_TYPEC_H_IRQ_EN_MAX (0x1<<4)
#define AUXADC_TYPEC_H_VOLT_MAX_H (0xF<<0)

#define AUXADC_TYPEC_L_DEBT_MIN 0x38D
#define AUXADC_TYPEC_L_DET_PRD_15_0_L 0x38E
#define AUXADC_TYPEC_L_DET_PRD_15_0_H 0x38F
#define AUXADC_TYPEC_L_VOLT_MIN_L 0x393

#define AUXADC_TYPEC_L4_H 0x394
#define AUXADC_TYPEC_L_MIN_IRQ_B (0x1<<7)
#define AUXADC_TYPEC_L_EN_MIN (0x1<<5)
#define AUXADC_TYPEC_L_IRQ_EN_MIN (0x1<<4)
#define AUXADC_TYPEC_L_VOLT_MIN_H (0xF<<0)

#define AUXADC_ADC_OUT_TYPEC_H_L 0x31E

#define AUXADC_ADC15_H 0x31F
#define AUXADC_ADC_RDY_TYPEC_H (0x1<<7)
#define AUXADC_ADC_OUT_TYPEC_H_H (0xF<<0)

#define AUXADC_ADC_OUT_TYPEC_L_L 0x320

#define AUXADC_ADC16_H 0x321
#define AUXADC_ADC_RDY_TYPEC_L (0x1<<7)
#define AUXADC_ADC_OUT_TYPEC_L_H (0xF<<0)

inline void typec_auxadc_set_event(struct typec_hba *hba)
{
	complete(&hba->auxadc_event);
}

inline uint16_t typec_auxadc_get_value(struct typec_hba *hba, int flag)
{
	uint8_t val = 0;
	uint8_t low_b = 0;

	if (flag == FLAGS_AUXADC_MIN) {

		while (1) {
			val = typec_read8(hba, AUXADC_ADC16_H);
			if (val & AUXADC_ADC_RDY_TYPEC_L)
				break;
			mdelay(2);
		}
		low_b = typec_read8(hba, AUXADC_ADC_OUT_TYPEC_L_L);

		return ((val & AUXADC_ADC_OUT_TYPEC_L_H)<<8 | low_b);
	}

	if (flag == FLAGS_AUXADC_MAX) {

		while (1) {
			val = typec_read8(hba, AUXADC_ADC15_H);
			if (val & AUXADC_ADC_RDY_TYPEC_H)
				break;
			mdelay(2);
		}
		low_b = typec_read8(hba, AUXADC_ADC_OUT_TYPEC_H_L);

		return ((val & AUXADC_ADC_OUT_TYPEC_H_H)<<8 | low_b);
	}

	return 0;
}

void typec_enable_auxadc(struct typec_hba *hba, int min, int max)
{
	uint8_t val = 0;

	if (min) {
		val = typec_read8(hba, AUXADC_TYPEC_L4_H);
		typec_write8(hba, (val | AUXADC_TYPEC_L_EN_MIN), AUXADC_TYPEC_L4_H);
	}

	if (max) {
		val = typec_read8(hba, AUXADC_TYPEC_H3_H);
		typec_write8(hba, (val | AUXADC_TYPEC_H_EN_MAX), AUXADC_TYPEC_H3_H);
	}
}

void typec_disable_auxadc(struct typec_hba *hba, int min, int max)
{
	uint8_t val = 0;

	if (min) {
		val = typec_read8(hba, AUXADC_TYPEC_L4_H);
		typec_write8(hba, (val & ~AUXADC_TYPEC_L_EN_MIN), AUXADC_TYPEC_L4_H);
	}

	if (max) {
		val = typec_read8(hba, AUXADC_TYPEC_H3_H);
		typec_write8(hba, (val & ~AUXADC_TYPEC_H_EN_MAX), AUXADC_TYPEC_H3_H);
	}
}

void typec_disable_auxadc_irq(struct typec_hba *hba)
{
	uint8_t val = 0;

	val = typec_read8(hba, AUXADC_TYPEC_H3_H);
	typec_write8(hba, val & ~AUXADC_TYPEC_H_IRQ_EN_MAX, AUXADC_TYPEC_H3_H);

	val = typec_read8(hba, AUXADC_TYPEC_L4_H);
	typec_write8(hba, val & ~AUXADC_TYPEC_L_IRQ_EN_MIN, AUXADC_TYPEC_L4_H);
}

void typec_enable_auxadc_irq(struct typec_hba *hba)
{
	uint8_t val = 0;

	val = typec_read8(hba, AUXADC_TYPEC_H3_H);
	typec_write8(hba, val | AUXADC_TYPEC_H_IRQ_EN_MAX, AUXADC_TYPEC_H3_H);

	val = typec_read8(hba, AUXADC_TYPEC_L4_H);
	typec_write8(hba, val | AUXADC_TYPEC_L_IRQ_EN_MIN, AUXADC_TYPEC_L4_H);
}

void typec_auxadc_set_thresholds(struct typec_hba *hba, uint16_t min, uint16_t max)
{
	uint8_t high_b = 0;
	uint8_t low_b = 0;
	uint8_t val = 0;

	if (min) {
		low_b = (min & 0xFF);
		high_b = (min >> 8) & 0xF;
		typec_write8(hba, low_b, AUXADC_TYPEC_L_VOLT_MIN_L);

		val = typec_read8(hba, AUXADC_TYPEC_L4_H) & ~0xF;
		typec_write8(hba, (val | high_b | AUXADC_TYPEC_L_EN_MIN), AUXADC_TYPEC_L4_H);

		dev_err(hba->dev, "Min=0x%04X\n", typec_readw(hba, AUXADC_TYPEC_L_VOLT_MIN_L));
	}

	if (max) {
		low_b = (max & 0xFF);
		high_b = (max >> 8) & 0xF;
		typec_write8(hba, low_b, AUXADC_TYPEC_H_VOLT_MAX_L);

		val = typec_read8(hba, AUXADC_TYPEC_H3_H) & ~0xF;
		typec_write8(hba, (val | high_b | AUXADC_TYPEC_H_EN_MAX), AUXADC_TYPEC_H3_H);

		dev_err(hba->dev, "Max=0x%04X\n", typec_readw(hba, AUXADC_TYPEC_H_VOLT_MAX_L));
	}
}

void typec_auxadc_set_state(struct typec_hba *hba, enum sink_power_states state)
{
	dev_err(hba->dev, "Enter %s\n", (state == 0)?"DFT":((state == 1)?"1.5A":"3.0A"));

	if (state == STATE_POWER_DEFAULT) {
		/*
		 * Type_C_H_MAX > 0.68v interrupt setting
		 * Voltage value (0.68/2/0.439 = 774)
		 *   AUXADC_TYPEC_H_VOLT_MAX_L 0x0382[7:0] set 8'h06
		 *   AUXADC_TYPEC_H_VOLT_MAX_H 0x0383[3:0] set 4'h3
		 * Enable ADC
		 *   AUXADC_TYPEC_H_EN_MAX 0x0383[5] set 1'b1
		 */
		typec_auxadc_set_thresholds(hba, SNK_VRPUSB_AUXADC_MIN_VAL, SNK_VRPUSB_AUXADC_MAX_VAL);
	} else if (state == STATE_POWER_1500MA) {
		/*
		 * Type_C_L_MIN < 0.68v interrupt setting
		 * Voltage value (0.68/2/0.439 = 774)
		 *   AUXADC_TYPEC_L_VOLT_MIN_L 0x0391[7:0] set 8'h06
		 *   AUXADC_TYPEC_L_VOLT_MIN_H 0x0392[3:0] set 4'h3
		 * Enable ADC
		 *   AUXADC_TYPEC_L_EN_MIN 0x0392[5] set 1'b1
		 *     Type_C_H_MAX > 1.28v interrupt setting
		 *     Voltage value (1.28/2/0.439 = 1456)
		 *     AUXADC_TYPEC_H_VOLT_MAX_L 0x0382[7:0] set 8'hB0
		 *     AUXADC_TYPEC_H_VOLT_MAX_H 0x0383[3:0] set 4'h5
		 * Enable ADC
		 *     AUXADC_TYPEC_H_EN_MAX 0x0383[5] set 1'b1
		 */

		typec_auxadc_set_thresholds(hba, SNK_VRP15_AUXADC_MIN_VAL, SNK_VRP15_AUXADC_MAX_VAL);
	} else if (state == STATE_POWER_3000MA) {
		/*
		 * Type_C_L_MIN < 1.28v interrupt setting
		 * Voltage value (1.28/2/0.439 = 1456)
		 *   AUXADC_TYPEC_L_VOLT_MIN_L 0x0391[7:0] set 8'hB0
		 *   AUXADC_TYPEC_L_VOLT_MIN_H 0x0392[3:0] set 4'h5
		 * Enable ADC
		 *   AUXADC_TYPEC_L_EN_MIN 0x0392[5] set 1'b1
		 */
		typec_auxadc_set_thresholds(hba, SNK_VRP30_AUXADC_MIN_VAL, 0);
	}
}

void typec_auxadc_register(struct typec_hba *hba)
{
	/*
	 * Initial setting
	 * 1. Disable type-c DAC path in Attach.SNK state
	 *      reg_type_c_adc_en 0x106[2] set 1'b1
	 * 2. Enable top AUXADC Type-C interrupt
	 *      RG_INT_EN_TYPE_C_L_MIN 0x006f[3] set 1'b1
	 *      RG_INT_EN_TYPE_C_H_MAX 0x006f[6] set 1'b1
	 * 3. AUXADC initial settings
	 *      Periodic measurement: 5ms
	 *        AUXADC_TYPEC_H_DET_PRD_15_0_L 0x037f set 8'd5 (unit: ms)
	 *        AUXADC_TYPEC_H_DET_PRD_15_0_H 0x0380 set 8'd0 (unit: ms)
	 *        AUXADC_TYPEC_L_DET_PRD_15_0_L 0x038c set 8'd5 (unit: ms)
	 *        AUXADC_TYPEC_L_DET_PRD_15_0_H 0x038d set 8'd0 (unit: ms)
	 *      Debounce count: 2 (debounce time 15ms)
	 *        AUXADC_TYPEC_H_DEBT_MAX 0x037d set  8'd2
	 *        AUXADC_TYPEC_L_DEBT_MIN 0x038b set  8'd2
	 *      AUXADC Type-C interrupt
	 *        AUXADC_TYPEC_H_IRQ_EN_MAX 0x0383[4] set 1'b1
	 *        AUXADC_TYPEC_L_IRQ_EN_MIN 0x0392[4] set 1'b1
	 */

	typec_writew(hba, AUXADC_INTERVAL_MS, AUXADC_TYPEC_H_DET_PRD_15_0_L);
	typec_writew(hba, AUXADC_INTERVAL_MS, AUXADC_TYPEC_L_DET_PRD_15_0_L);
	typec_write8(hba, AUXADC_DEBOUNCE_COUNT, AUXADC_TYPEC_H_DEBT_MAX);
	typec_write8(hba, AUXADC_DEBOUNCE_COUNT, AUXADC_TYPEC_L_DEBT_MIN);

	typec_enable_auxadc_irq(hba);
}

void typec_auxadc_low_register(struct typec_hba *hba)
{
	uint8_t val = 0;

	typec_writew(hba, AUXADC_INTERVAL_MS, AUXADC_TYPEC_L_DET_PRD_15_0_L);
	typec_write8(hba, AUXADC_INTERVAL_MS, AUXADC_TYPEC_L_DEBT_MIN);

	val = typec_read8(hba, AUXADC_TYPEC_L4_H);
	typec_write8(hba, val | AUXADC_TYPEC_L_IRQ_EN_MIN, AUXADC_TYPEC_L4_H);
}

void typec_auxadc_unregister(struct typec_hba *hba)
{
	typec_disable_auxadc_irq(hba);
	typec_disable_auxadc(hba, 1, 1);
}
#endif

void typec_write8(struct typec_hba *hba, uint8_t val, unsigned int reg)
{
	if (mt6336_set_register_value(reg, val) == -1)
		dev_err(hba->dev, "i2c write fail");
}

void typec_writew(struct typec_hba *hba, uint16_t val, unsigned int reg)
{
	uint8_t v[2];
	int ret;

	v[0] = val & 0xFF;
	v[1] = (val >> 8) & 0xFF;

	ret = mt6336_write_bytes(reg, v, 2);
	if (ret < 0)
		dev_err(hba->dev, "i2c write fail");
}

void typec_writedw(struct typec_hba *hba, uint32_t val, unsigned int reg)
{
	uint8_t v[4];
	int ret;

	v[0] = val & 0xFF;
	v[1] = (val >> 8) & 0xFF;
	v[2] = (val >> 16) & 0xFF;
	v[3] = (val >> 24) & 0xFF;

	ret = mt6336_write_bytes(reg, v, 4);
	if (ret < 0)
		dev_err(hba->dev, "i2c write fail");
}

uint8_t typec_read8(struct typec_hba *hba, unsigned int reg)
{
	unsigned int val = 0;

	val = mt6336_get_register_value(reg);

	return val;
}

#ifdef BURST_READ
uint16_t typec_readw(struct typec_hba *hba, unsigned int reg)
{
	uint16_t ret = 0;
	uint8_t v[2];

	mt6336_read_bytes(reg, v, 2);

	ret = ((v[1] << 8) | v[0]);

	return ret;
}

uint32_t typec_readdw(struct typec_hba *hba, unsigned int reg)
{
	uint8_t v[4];
	uint32_t ret = 0;

	mt6336_read_bytes(reg, v, 4);

	ret = ((v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0]);

	return ret;
}
#else
uint16_t typec_readw(struct typec_hba *hba, unsigned int reg)
{
	uint16_t ret = 0;

	ret = (typec_read8(hba, reg+1) << 8) | typec_read8(hba, reg);

	return ret;
}
#endif

void typec_writew_msk(struct typec_hba *hba, uint16_t msk, uint16_t val, unsigned int reg)
{
	uint16_t tmp;
	const int is_print = 0;

	tmp = typec_readw(hba, reg);

	if (is_print)
		dev_err(hba->dev, "%s 0x%03X=0x%X\n", __func__, reg, tmp);

	tmp = (tmp & ~msk) | (val & msk);
	typec_writew(hba, tmp, reg);

	if (is_print)
		dev_err(hba->dev, "0x%03X=0x%X Should be 0x%X\n", reg, typec_readw(hba, reg), tmp);

}

void typec_write8_msk(struct typec_hba *hba, uint8_t msk, uint8_t val, unsigned int reg)
{
	uint16_t tmp;
	const int is_print = 0;

	tmp = typec_read8(hba, reg);

	if (is_print)
		dev_err(hba->dev, "%s 0x%03X=0x%X\n", __func__, reg, tmp);

	tmp = (tmp & ~msk) | (val & msk);
	typec_write8(hba, tmp, reg);

	if (is_print)
		dev_err(hba->dev, "0x%03X=0x%X Should be 0x%X\n", reg, typec_read8(hba, reg), tmp);

}

void typec_set(struct typec_hba *hba, uint16_t val, unsigned int reg)
{
	uint16_t tmp;
	const int is_print = 0;

	tmp = typec_readw(hba, reg);

	if (is_print)
		dev_err(hba->dev, "%s 0x%03X=0x%X\n", __func__, reg, tmp);

	typec_writew(hba, tmp | val, reg);

	if (is_print)
		dev_err(hba->dev, "0x%03X=0x%X Should be 0x%X\n", reg, typec_readw(hba, reg), (tmp | val));
}

void typec_set8(struct typec_hba *hba, uint8_t val, unsigned int reg)
{
	uint16_t tmp;
	const int is_print = 0;

	tmp = typec_read8(hba, reg);

	if (is_print)
		dev_err(hba->dev, "%s 0x%03X=0x%X\n", __func__, reg, tmp);

	typec_write8(hba, tmp | val, reg);

	if (is_print)
		dev_err(hba->dev, "0x%03X=0x%X Should be 0x%X\n", reg, typec_read8(hba, reg), (tmp | val));
}


void typec_clear(struct typec_hba *hba, uint16_t val, unsigned int reg)
{
	uint16_t tmp;
	const int is_print = 0;

	tmp = typec_readw(hba, reg);

	if (is_print)
		dev_err(hba->dev, "%s 0x%03X=0x%X\n", __func__, reg, tmp);

	typec_writew(hba, tmp & (~val), reg);

	if (is_print)
		dev_err(hba->dev, "0x%03X=0x%X Should be 0x%X\n", reg, typec_readw(hba, reg), (tmp & (~val)));

}

void typec_enable_lowq(struct typec_hba *hba, char *str)
{
#if !COMPLIANCE
	mutex_lock(&hba->lowq_lock);

	if (atomic_read(&hba->lowq_cnt) > 0) {
		atomic_dec(&hba->lowq_cnt);
		mt6336_ctrl_disable(hba->core_ctrl);
		if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
			pr_info("%s %s LowQ cnt=%d\n", __func__, str, (int)atomic_read(&hba->lowq_cnt));
	} else {
		pr_info("%s %s Disable LowQ mode to much\n", __func__, str);
	}

	mutex_unlock(&hba->lowq_lock);
#endif
}

void typec_disable_lowq(struct typec_hba *hba, char *str)
{
#if !COMPLIANCE
	mutex_lock(&hba->lowq_lock);

	atomic_inc(&hba->lowq_cnt);
	mt6336_ctrl_enable(hba->core_ctrl);
	if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
		pr_info("%s %s LowQ cnt=%d\n", __func__, str, (int)atomic_read(&hba->lowq_cnt));

	mutex_unlock(&hba->lowq_lock);
#endif
}

int is_otg_en(void)
{
	struct typec_hba *hba = get_hba();

	return (hba->vbus_en == 1);
}
EXPORT_SYMBOL_GPL(is_otg_en);

int register_charger_det_callback(int (*func)(int))
{
	struct typec_hba *hba = get_hba();

	pr_info("%s Register charger det driver\n", __func__);

	if (!hba)
		return -1;

	hba->charger_det_notify = func;
	return 0;
}
EXPORT_SYMBOL_GPL(register_charger_det_callback);

#if 0
BLOCKING_NOTIFIER_HEAD(type_notifier_list);
void typec_notifier_register(struct notifier_block *n)
{
	blocking_notifier_chain_register(&type_notifier_list, n);
}
EXPORT_SYMBOL(typec_notifier_register);

void typec_notifier_unregister(struct notifier_block *n)
{
	blocking_notifier_chain_unregister(&type_notifier_list, n);
}
EXPORT_SYMBOL(typec_notifier_unregister);

void typec_notifier_call_chain(void)
{
	int type;
	int event;

	blocking_notifier_call_chain(&type_notifier_list, type, event);
}
#else

void disable_dfp(struct typec_hba *hba)
{
	struct usbtypc *typec = get_usbtypec();

	if (typec->host_driver && typec->host_driver->on == ENABLE) {
		typec->host_driver->disable(typec->host_driver->priv_data);
		typec->host_driver->on = DISABLE;
		dev_err(hba->dev, "%s disable host", __func__);
	}

}

void enable_dfp(struct typec_hba *hba)
{
	struct usbtypc *typec = get_usbtypec();

	if (typec->host_driver && typec->host_driver->on == DISABLE) {
		typec->host_driver->enable(typec->host_driver->priv_data);
		typec->host_driver->on = ENABLE;
		dev_err(hba->dev, "%s enable host", __func__);
	}
}

void disable_ufp(struct typec_hba *hba)
{
	struct usbtypc *typec = get_usbtypec();

	if (hba->charger_det_notify) {
		if (typec->device_driver && typec->device_driver->on == ENABLE) {
			typec->device_driver->disable(typec->device_driver->priv_data);
			typec->device_driver->on = DISABLE;
			dev_err(hba->dev, "%s disable device", __func__);
		}
	} else {
		dev_err(hba->dev, "%s skip disable device", __func__);
	}
}

void enable_ufp(struct typec_hba *hba)
{
	struct usbtypc *typec = get_usbtypec();

	if (hba->charger_det_notify) {
		if (typec->device_driver && typec->device_driver->on == DISABLE) {
			typec->device_driver->enable(typec->device_driver->priv_data);
			typec->device_driver->on = ENABLE;
			dev_err(hba->dev, "%s enable device", __func__);
		}
	} else {
		dev_err(hba->dev, "%s skip enable device", __func__);
	}
}

/*
 *               current data role
 *            |--UFP--|--DFP--|--NR--
 *       ---------------------------
 *       -UFP-|  X   0| SWAP 6| DIS 1
 *  last ---------------------------
 *  data -DFP-| SWAP 5|  X   0| DIS 2
 *  role ---------------------------
 *       --NR-|  EN  3|  EN  4|  X  0
 *   NR = No Role
 */
static void trigger_driver(struct work_struct *work)
{
#if !COMPLIANCE
	struct delayed_work *dwork = to_delayed_work(work);
	struct typec_hba *hba = container_of(dwork, struct typec_hba, usb_work);

	static int type = PD_NO_ROLE;
	const int action[3][3] = { { 0, 6, 1}, { 5, 0, 2 }, { 3, 4, 0} };

	struct usbtypc *typec = get_usbtypec();

	if (!typec)
		return;

	if (hba->cc != DONT_CARE)
		usb3_switch_sel(typec, hba->cc);

	dev_err(hba->dev, "%s %d -> %d", __func__, type, hba->data_role);

	switch (action[type][hba->data_role]) {
	case 0:
		dev_err(hba->dev, "%s Do nothing", __func__);
	break;
	case 1: /*Disable device driver*/
		disable_ufp(hba);
		usb3_switch_en(typec, DISABLE);
	break;
	case 2: /*Disable host driver*/
		disable_dfp(hba);
		usb3_switch_en(typec, DISABLE);
	break;
	case 3: /*Enable device driver*/
		usb3_switch_en(typec, ENABLE);
		enable_ufp(hba);
	break;
	case 4: /*Enable host driver*/
		usb3_switch_en(typec, ENABLE);
		enable_dfp(hba);
	break;
	case 5: /*Swap from host to device*/
		disable_dfp(hba);
		enable_ufp(hba);
	break;
	case 6: /*Swap from device to host*/
		disable_ufp(hba);
		enable_dfp(hba);
	break;
	}

	type = hba->data_role;
		/*
		 * As SNK, when disconnect Attached.SNK should go to Unattached.SNK
		 * As SRC, when disconnect Attached.SRC should go to Unattached.SRC
		 * However, As DRP, when disconnect Attached.SRC should go to Unattached.SNK
		 * Attached.SNK should go to Unattached.SRC. So just check both driver status
		 * And turn off the driver was on.
		 */
#endif
}

int register_typec_switch_callback(struct typec_switch_data *new_driver)
{
	struct usbtypc *typec = get_usbtypec();

	pr_info("Register driver %s %d\n", new_driver->name, new_driver->type);

	if (!typec)
		return -1;

	if (new_driver->type == DEVICE_TYPE) {
		typec->device_driver = new_driver;
		typec->device_driver->on = DISABLE;
#if SUPPORT_PD
		if (g_hba && g_hba->data_role == PD_ROLE_UFP)
#else
		if ((g_hba->cc > 0) && (g_hba->vbus_en == 0))
#endif
			typec->device_driver->enable(typec->device_driver->priv_data);
		return 0;
	}

	if (new_driver->type == HOST_TYPE) {
		typec->host_driver = new_driver;
		typec->host_driver->on = DISABLE;
#if SUPPORT_PD
		if (g_hba && g_hba->data_role == PD_ROLE_DFP)
#else
		if ((g_hba->cc > 0) && (g_hba->vbus_en == 1))
#endif
			typec->host_driver->enable(typec->host_driver->priv_data);
		return 0;
	}

	return -1;
}
EXPORT_SYMBOL_GPL(register_typec_switch_callback);

int unregister_typec_switch_callback(struct typec_switch_data *new_driver)
{
	struct usbtypc *typec = get_usbtypec();

	if (!typec)
		return -1;

	if ((new_driver->type == DEVICE_TYPE) && (typec->device_driver == new_driver))
		typec->device_driver = NULL;

	if ((new_driver->type == HOST_TYPE) && (typec->host_driver == new_driver))
		typec->host_driver = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(unregister_typec_switch_callback);
#endif

void typec_vbus_present(struct typec_hba *hba, uint8_t enable)
{
	typec_writew_msk(hba, TYPE_C_SW_VBUS_PRESENT,
		((enable) ? TYPE_C_SW_VBUS_PRESENT : 0), TYPE_C_CC_SW_CTRL);

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2 && (hba->vbus_present != enable))
		dev_err(hba->dev, "VBUS_PRESENT %s", ((enable) ? "ON" : "OFF"));

	hba->vbus_present = (enable ? 1 : 0);
}

void typec_vbus_det_enable(struct typec_hba *hba, uint8_t enable)
{
	if (hba->dbg_lvl >= TYPEC_DBG_LVL_2 && (hba->vbus_det_en != enable))
		dev_err(hba->dev, "VBUS_DET %s", ((enable) ? "ON" : "OFF"));

	hba->vbus_det_en = (enable ? 1 : 0);
}

void typec_set_vsafe5v(struct typec_hba *hba, int val)
{
	dev_err(hba->dev, "%s %dv", __func__, val);
	hba->vsafe_5v = val;
}

#ifdef MT6336_E1
#define MAIN_CON5 (0x0405) /*0x00*/
#define AUXADC_CON7 (0x0357) /*0x04*/
#define AUXADC_CON6_H (0x0356) /*0x31*/
#define AUXADC_CON6 (0x0355) /*0xE0*/
#define CLK_CKPDN_CON3 (0x0034) /*0x16*/
#define CLK_CKPDN_CON3_CLR (0x036) /*0x1*/
#define MAIN_CON8 (0x0409) /*0x01*/
#define AUXADC_RQST0 (0x033F) /*0x04*/
#define AUXADC_ADC2 (0x0304)
#define AUXADC_ADC2_H (0x0305)

#define AUXADC_ADC_RDY_CH2 (0x1<<7)

/*MT6336_AuxADC_setting_all_channel.txt from Jyun-Jia Huang (MTK_ADCT_ACD1_DE6)*/
unsigned int vbus_val(struct typec_hba *hba)
{
	static int is_global_setting;
	int vbus_val_h = 0;
	int vbus_val = 0;
	int val = 0;

	if (!is_global_setting) {
		/*
		 * ;Global setting
		 * WR 56 0405 00
		 * WR 55 0357 04
		 * WR 55 0356 31
		 * WR 55 0355 E0
		 * WR 52 0034 16
		 * WR 56 0409 01
		 */
		/* [2]Force off the VBUS discharging current
		 * [1]Discharging current period for input voltage detection setting = 30ms
		 * [0]Disable the VBUS discharging current during plug-in detection
		 */
		typec_write8(hba, 0x00, MAIN_CON5);

		/*AUXADC_TRIM_CH11~8_SEL*/
		typec_write8(hba, 0x04, AUXADC_CON7);

		/*AUXADC_TRIM_CH7~4_SEL*/
		typec_write8(hba, 0x31, AUXADC_CON6_H);

		/*AUXADC_TRIM_CH3~0_SEL*/
		typec_write8(hba, 0xE0, AUXADC_CON6);
		typec_write8(hba, 0x1, CLK_CKPDN_CON3_CLR);
		typec_write8(hba, 0x01, MAIN_CON8);

		is_global_setting = 1;
	}

	/*
	 * ;CH2 read 0304(L) 0305(H) 15bit
	 * WR 55 033F 04
	 */
	typec_write8(hba, 0x04, AUXADC_RQST0);

	/*
	 * 7 AUXADC_ADC_RDY_CH2 AUXADC channel 2 output data ready
	 * 0: AUXADC data proceeding
	 * 1: AUXADC data ready
	 */
	while (1) {

		val = typec_read8(hba, AUXADC_ADC2_H);
		if (val & AUXADC_ADC_RDY_CH2) {
			/*
			 *  RD 55 0305
			 *  RD 55 0304
			 *  total 15bit 0305(6:0),0304(7:0)
			 *  val = (the data/32768)*1.8 (unit=V)
			 */
			vbus_val_h = val & 0x7F;
			break;
		}
		mdelay(2);
	}

	/*vbus_val_h = typec_read8(hba, AUXADC_ADC2_H) & 0x7F;*/
	/*vbus_val_l = typec_read8(hba, AUXADC_ADC2);*/
	/*vbus_val = (vbus_val_h<<8) | vbus_val_h;*/

	/*
	 * The correct way is (vbus_val/2^15)*1.8
	 * But simplify to
	 * (vbus_val*9)/(2^7*2^8*5)
	 * =  (vbus_val) * (9/(2^7*5)) * (1/2^8)
	 * =  (vbus_val) * (9/640) * (1/2^8)
	 * ~=  (vbus_val) * (1/71) * (1/2^8)
	 * ~= (vbus_val >> 8)/71
	 * = (vbus_val_h)/71
	 */
	vbus_val = ((vbus_val_h)*1000)/7;

	return vbus_val;
}

#else

#define MAIN_CON5 (0x0405) /*0x00*/
#define AUXADC_CON7 (0x0359) /*0x54*/
#define AUXADC_CON6_H (0x0358) /*0x11*/
#define AUXADC_CON6 (0x0357) /*0xE0*/
#define AUXADC_RQST0 (0x0341) /*0x04*/
#define AUXADC_RQST0_SET (0x0342) /*0x04*/
#define AUXADC_CON2_H (0x0350) /*0x01*/
#define AUXADC_CON3 (0x0351) /*0x00*/
#define CLK_CKPDN_CON3 (0x0034) /*0x16*/
#define CLK_CKPDN_CON3_CLR (0x036) /*0x1*/
#define MAIN_CON8 (0x0409) /*0x01*/
#define AUXADC_ADC2 (0x0304)
#define AUXADC_ADC2_H (0x0305)
#define AUXADC_ADC_RDY_CH2 (0x1<<7)

/*MT6336_E2_AuxADC_setting_all_channel.txt from Jyun-Jia Huang (MTK_ADCT_ACD1_DE6)*/
unsigned int vbus_val_self(struct typec_hba *hba)
{
	static int is_global_setting;
	int vbus_val_h = 0;
	int vbus_val = 0;
	int val = 0;

	typec_disable_lowq(hba, "vbus_val_self");

	if (!is_global_setting) {
		/*
		 * ;Global setting
		 * WR 56 0405 00
		 * WR 52 0034 16
		 * WR 56 0409 01
		 * WR 55 0359 54
		 * WR 55 0358 11
		 * WR 55 0357 E0
		 * WR 55 0351 01
		 * WR 55 0350 00
		 */
		/* [2]Force off the VBUS discharging current
		 * [1]Discharging current period for input voltage detection setting = 30ms
		 * [0]Disable the VBUS discharging current during plug-in detection
		 */
		typec_write8(hba, 0x00, MAIN_CON5);
		typec_write8(hba, 0x1, CLK_CKPDN_CON3_CLR);
		/*typec_write8(hba, 0x01, MAIN_CON8);*/

		/*AUXADC_TRIM_CH11~8_SEL*/
		typec_write8(hba, 0x54, AUXADC_CON7);

		/*AUXADC_TRIM_CH7~4_SEL*/
		typec_write8(hba, 0x11, AUXADC_CON6_H);

		/*AUXADC_TRIM_CH3~0_SEL*/
		typec_write8(hba, 0xE0, AUXADC_CON6);

		/*typec_write8(hba, 0x1, AUXADC_CON3);*/
		typec_write8(hba, 0x0, AUXADC_CON2_H);
		typec_write8(hba, 0x1D, 0x034F);

		is_global_setting = 1;
	}

	/*
	 * ;CH2 read 0304(L) 0305(H) 15bit
	 * WR 55 033F 04
	 */
	typec_write8(hba, 0x04, AUXADC_RQST0_SET);

	/*
	 * 7 AUXADC_ADC_RDY_CH2 AUXADC channel 2 output data ready
	 * 0: AUXADC data proceeding
	 * 1: AUXADC data ready
	 */
	while (1) {

		val = typec_read8(hba, AUXADC_ADC2_H);
		if (val & AUXADC_ADC_RDY_CH2) {
			/*
			 * RD 55 0305
			 * RD 55 0304
			 * total 15bit 0305(6:0),0304(7:0)
			 * val = (the data/32768)*1.8 (unit=V)
			 */
			vbus_val_h = val & 0x7F;
			break;
		}
		mdelay(2);
	}

	typec_enable_lowq(hba, "vbus_val_self");

	/*vbus_val_h = typec_read8(hba, AUXADC_ADC2_H) & 0x7F;*/
	/*vbus_val_l = typec_read8(hba, AUXADC_ADC2);*/
	/*vbus_val = (vbus_val_h<<8) | vbus_val_h;*/

	/*
	 *  The correct way is (vbus_val/2^15)*1.8
	 *  But simplify to
	 *  (vbus_val*9)/(2^7*2^8*5)
	 *  =  (vbus_val) * (9/(2^7*5)) * (1/2^8)
	 *  =  (vbus_val) * (9/640) * (1/2^8)
	 *  ~=  (vbus_val) * (1/71) * (1/2^8)
	 *  ~= (vbus_val >> 8)/71
	 *  = (vbus_val_h)/71
	 */
	vbus_val = ((vbus_val_h)*1000)/7;

	return vbus_val;
}

unsigned int vbus_val(struct typec_hba *hba)
{
	int val = 0;

	typec_disable_lowq(hba, "vbus_val");
	val = (pmic_get_auxadc_value(AUXADC_LIST_VBUS) >> 3);
	typec_enable_lowq(hba, "vbus_val");

	return val;
}
#endif

#define ABS_SUB(x, y) ((x > y)?(x - y):(y - x))

int typec_vbus(struct typec_hba *hba)
{
#if COMPLIANCE
	unsigned int val = vbus_val_self(hba);
#else
	unsigned int val = vbus_val(hba);
#endif
	static unsigned int pre_val = UINT_MAX;
	static unsigned int cnt;

	if (ABS_SUB(val, pre_val) > 100) {
		dev_err(hba->dev, "ADC on VBUS=%d %d", val, pre_val);
		pre_val = val;
	} else if (cnt > 10) {
		dev_err(hba->dev, "ADC on VBUS=%d 0x%x %d", val, hba->wq_running, hba->wq_cnt);
		cnt = 0;
	}

	cnt++;

	return val;
}

#ifdef MT6336_E1

#define SWCHR_MAIN_CON0 (0x0400) /*0x0B-->0x03*/
#define SWCHR_MAIN_CON5 (0x0405) /*0x04*/
#define SWCHR_MAIN_CON8 (0x0409) /*0x01*/
#define SWCHR_OTG_CTRL0 (0x040F) /*0x0F*/
#define SWCHR_OTG_CTRL2 (0x0411) /*0x55*/
#define SWCHR_ICL_CON1 (0x0438) /*0x08*/

#define ANA_CORE_ANA_CON22 (0x0515) /*0x30-->0x30*/
#define ANA_CORE_ANA_CON26 (0x0519) /*0x0D-->0x1F*/
#define ANA_CORE_ANA_CON33 (0x0520) /*0x00-->0x34*/
#define ANA_CORE_ANA_CON34 (0x0521) /*0x44-->0x44*/
#define ANA_CORE_ANA_CON49 (0x0530) /*0xC0*/
#define ANA_CORE_ANA_CON63 (0x053D) /*0x42-->0x47*/
#define ANA_CORE_ANA_CON102 (0x055F) /*0xE6-->0x5E*/
#define ANA_CORE_ANA_CON103 (0x0560) /*0x0C-->0x1D*/

#define RG_EN_OTG	 (1<<3)

void typec_drive_vbus(struct typec_hba *hba, uint8_t on)
{
	if (hba->vbus_en == on)
		goto skip;
	/*
	 * From http://teams.mediatek.inc/sites/Power_Management/SP_PMIC/
	 * MT6336/Shared Documents/02_ChipVerification/2.4_HQA/
	 * Bring-up script/
	 * MT6336 OTG bring-up script(0214).txt
	 */
	if (on) {
		/*
		 * WR 56 0409 01
		 * WR 56 0438 08
		 * ;disable ICL150 pin
		 *
		 * ;Before entering OTG mode:
		 * WR 57 0519 0D
		 * WR 57 0521 44
		 * WR 57 0520 00
		 * WR 57 053D 42
		 *
		 * ->WR 57 506 12
		 * ;SET PRE-REGULATOR OUTPUT=5.0V
		 *
		 * ;ICL trim level increase 20 percents for OTG OLP
		 * ->WR 57 053B 07
		 *
		 * ;within fast transient(HQA Test)
		 * WR 57 055F E6
		 *
		 * ;Shoot_mode=1, Discharge_en=0(HQA prefer setting)
		 * WR 57 0560 0C
		 *
		 * ;switching freq to 1MHz
		 * ->WR 56 043A 02
		 *
		 * ;Drop FTR time extend(HQA prefer setting)
		 * WR 57 0530 C0
		 *
		 * WR 56 040F 0F
		 * ;RG_TOLP_OFF[2], 0=15ms, 1=30ms(default)
		 * ;RG_TOLP_ON[1:0], 00=200us, 01=800us, 10=3200us, 11=12800us,
		 *
		 * WR 56 0411 55
		 * ;RG_OTG_VCV[7:4], 0000=4.5V, 0101=5V(default), 1010=5.5V,
		 * ;RG_OTG_IOLP[2:0], 000=100mA, 001=500mA, 010=0.9A, 011=1.2A(default), 100=1.5A, 101=2A,
		 *
		 * ;RG_A_OTG_VM_UVLO_VTH,0:2V,1:4V
		 * WR 57 0515 30
		 *
		 * ;turn on the OTG
		 * WR 56 0400 0B
		 */
		typec_write8(hba, 0x01, SWCHR_MAIN_CON8);
		typec_write8(hba, 0x08, SWCHR_ICL_CON1);
		typec_write8(hba, 0x0D, ANA_CORE_ANA_CON26);
		typec_write8(hba, 0x44, ANA_CORE_ANA_CON34);
		typec_write8(hba, 0x00, ANA_CORE_ANA_CON33);
		typec_write8(hba, 0x42, ANA_CORE_ANA_CON63);
		typec_write8(hba, 0x12, 0x506);
		typec_write8(hba, 0x07, 0X53B);
		typec_write8(hba, 0xE6, ANA_CORE_ANA_CON102);
		typec_write8(hba, 0x0C, ANA_CORE_ANA_CON103);
		typec_write8(hba, 0x02, 0x43A);
		typec_write8(hba, 0xC0, ANA_CORE_ANA_CON49);
		typec_write8(hba, 0x0F, SWCHR_OTG_CTRL0);
		typec_write8(hba, 0x55, SWCHR_OTG_CTRL2);
		typec_write8(hba, 0x30, ANA_CORE_ANA_CON22);
		typec_write8(hba, 0x0B, SWCHR_MAIN_CON0);
	} else {
		/*
		 * ;Case B: Disable OTG mde
		 * ;WR 56 0405 04
		 * ;WR 56 0400 03
		 * ;Wait 30ms
		 * ;WR 56 0405 00
		 * ;Set initial setting to be default value
		 * ;WR 57 0519 1F
		 * ;WR 57 0521 44
		 * ;WR 57 0520 34
		 * ;WR 57 053D 47
		 * ;WR 57 055F 5E
		 * ;WR 57 0560 1D
		 * ;WR 57 0515 10
		 */
		/*Force on the VBUS discharging current*/
		typec_write8(hba, 0x05, SWCHR_MAIN_CON5);
		/*RG_EN_OTG off*/
		typec_write8(hba, 0x03, SWCHR_MAIN_CON0);
		mdelay(5);
		typec_write8(hba, 0x01, SWCHR_MAIN_CON5);
		typec_write8(hba, 0x1F, ANA_CORE_ANA_CON26);
		typec_write8(hba, 0x44, ANA_CORE_ANA_CON34);
		typec_write8(hba, 0x34, ANA_CORE_ANA_CON33);
		typec_write8(hba, 0x47, ANA_CORE_ANA_CON63);
		typec_write8(hba, 0x5E, ANA_CORE_ANA_CON102);
		typec_write8(hba, 0x1D, ANA_CORE_ANA_CON103);
		typec_write8(hba, 0x10, ANA_CORE_ANA_CON22);
	}

skip:
	if ((hba->dbg_lvl >= TYPEC_DBG_LVL_2) && (hba->vbus_en != on))
		dev_err(hba->dev, "VBUS %s", (on ? "ON" : "OFF"));

	hba->vbus_en = (on ? 1 : 0);
}

#else

#if COMPLIANCE

#define SWCHR_MAIN_CON0 (0x0400) /*0x0B-->0x03*/
#define SWCHR_MAIN_CON5 (0x0405) /*0x04*/
#define SWCHR_MAIN_CON8 (0x0409) /*0x01*/
#define SWCHR_OTG_CTRL0 (0x040F) /*0x0F*/
#define SWCHR_OTG_CTRL2 (0x0411) /*0x55*/
#define SWCHR_ICL_CON1 (0x0438) /*0x08*/

#define ANA_CORE_ANA_CON22 (0x0515) /*0x30-->0x30*/
#define ANA_CORE_ANA_CON26 (0x0519) /*0x0D-->0x1F*/
#define ANA_CORE_ANA_CON33 (0x0520) /*0x00-->0x34*/
#define ANA_CORE_ANA_CON34 (0x0521) /*0x44-->0x44*/
#define ANA_CORE_ANA_CON49 (0x0530) /*0xC0*/
#define ANA_CORE_ANA_CON63 (0x053D) /*0x42-->0x47*/
#define ANA_CORE_ANA_CON102 (0x055F) /*0xE6-->0x5E*/
#define ANA_CORE_ANA_CON103 (0x0560) /*0x0C-->0x1D*/

#define RG_EN_OTG	 (1<<3)

void typec_drive_vbus(struct typec_hba *hba, uint8_t on)
{
	if (hba->vbus_en == on)
		goto skip;
	/*
	 *  From http://teams.mediatek.inc/sites/Power_Management/SP_PMIC/
	 *  MT6336/Shared Documents/02_ChipVerification/2.4_HQA/
	 *  E2 bring-up script/
	 *  MT6336 OTG bring-up script_E2(0720).txt
	 *  MT6336 Leave OTG script_E2(0720).txt
	 */
	if (on) {
		/*
		 * ;Disable LOWQ mode
		 * WR 56 0409 01
		 *
		 * ;;Before enters OTG mode:
		 * ;;LOOP Stability, Fast Transient, Switching frequency setting
		 * ;[6:1]GM enable=000110
		 * WR 57 0519 0D
		 * ;GM MSB=00000000
		 * WR 57 0520 00
		 * ;GM LSB=01000100
		 * WR 57 0521 44
		 *
		 * ;[3:0]VCS_RTUNE=1101
		 * WR 57 053F 0B
		 * ;[6:4]VRAMP_SLP=100,[3:0]VRAMP DC offset=0010
		 * WR 57 053D 42
		 *
		 * ;[1:0] GM/4; GM/2, close GM/4 function for OTG IOLP
		 * WR 57 051E 00
		 *
		 * ;[3:0]ST_ITUNE_ICL=1110, OLP status comparator
		 * WR 57 0529 4E
		 *
		 * ;[7:6]FTR_RC=11,[5:3]FTR_DROP=100,[2]FTR_SHOOT_EN=1,[1]FTR_DROP_EN=1,
		 * ;Within Fast Transient(HQA Test)
		 * WR 57 055F E6
		 *
		 * ;[4]FTR_DISCHARGE_EN=0,[3]FTR_SHOOT_MODE=1,[2]FTR_DROP_MODE=1,[1:0]FTR_DELAY=00
		 * WR 57 0560 0C
		 *
		 * ;[0]SWITCHING FREQ SELECT=0(1MHz)
		 * WR 56 0463 02
		 *
		 * ;[7:6]FTR DROP time extend=11(20us)
		 * WR 57 0530 C0
		 *
		 * ;Set RG_A_LOGIC_RSV_TRIM_LSB[7:0]=8'b00010000, MINOFF function
		 * WR 57 054A 10
		 *
		 * ;[1]RG_FORCE_LSON_VPR=1, turn on LGATE during MINOFF
		 * WR 57 055A 01
		 *
		 * ;[7:6]RG_A_PWR_UG_DTC=11(same as E1 setting, smallest dead time)
		 * WR 57 0552 E8
		 *
		 * ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		 * ;;OTG hiccup TOLP setting
		 * ;[2]TOLP_OFF=1,[1:0]TOLP_ON=11
		 * WR 56 040F 07
		 * ;RG_TOLP_OFF[2], 0=15ms, 1=30ms(default)
		 * ;RG_TOLP_ON[1:0], 00=200us, 01=800us, 10=3200us, 11=12800us
		 *
		 * ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		 * ;;OTG Voltage and OLP setting
		 * ;[7:4]OTG_VCV,[2:0]OTG_IOLP=001
		 * WR 56 0411 51
		 * ;RG_OTG_VCV[7:4],0000=4.5V, 0101=5V(default), 1010=5.5V,
		 * ;RG_OTG_IOLP[2:0],000=100mA,001=500mA,010=0.9A,011=1.2A(default),100=1.5A,101=2A
		 *
		 * ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		 * ;;OTG mode ENABLE
		 * ;[3]RG_EN_OTG=1(0x0400=1011)
		 * WR 56 0400 0B
		 */
		typec_write8(hba, 0x01, SWCHR_MAIN_CON8);
		typec_write8(hba, 0x0D, ANA_CORE_ANA_CON26);
		typec_write8(hba, 0x00, ANA_CORE_ANA_CON33);
		typec_write8(hba, 0x44, ANA_CORE_ANA_CON34);
		typec_write8(hba, 0x0B, 0x53F);
		typec_write8(hba, 0x42, ANA_CORE_ANA_CON63);
		typec_write8(hba, 0x00, 0X51E);
		typec_write8(hba, 0x4E, 0X529);
		typec_write8(hba, 0xE6, ANA_CORE_ANA_CON102);
		typec_write8(hba, 0x0C, ANA_CORE_ANA_CON103);
		typec_write8(hba, 0x02, 0x463);
		typec_write8(hba, 0xC0, ANA_CORE_ANA_CON49);
		typec_write8(hba, 0x10, 0x54A);
		typec_write8(hba, 0x01, 0x55A);
		typec_write8(hba, 0xE8, 0x552);
		typec_write8(hba, 0x04, SWCHR_OTG_CTRL0);
		typec_write8(hba, 0x51, SWCHR_OTG_CTRL2);
		typec_write8(hba, 0x0B, SWCHR_MAIN_CON0);
	} else {
		/*
		 * ;;Disable OTG mde
		 * ;;Set initial setting to be default value
		 *
		 * ;[3]RG_EN_OTG=0, Leave OTG Mode
		 * WR 56 0400 03
		 *
		 * ;[6:1]GM enable=111111
		 * WR 57 0519 3F
		 * ;GM MSB=00110100
		 * WR 57 0520 34
		 * ;[6:4]VRAMP_SLP=000,[3:0]VRAMP DC offset=0010
		 * WR 57 053D 02
		 * ;[2:0]ICL_TRIM=000
		 * WR 57 053C 00
		 * ;[1:0] GM/4; GM/2=10
		 * WR 57 051E 02
		 * ;[3:0]ST_ITUNE_ICL=0100
		 * WR 57 0529 44
		 * ;[2]FTR_SHOOT_EN=0, [1]FTR_DROP_EN=0, without Fast Transient
		 * WR 57 055F E0
		 */
		typec_write8(hba, 0x03, SWCHR_MAIN_CON0);
		typec_write8(hba, 0x3F, ANA_CORE_ANA_CON26);
		typec_write8(hba, 0x34, ANA_CORE_ANA_CON33);
		typec_write8(hba, 0x02, ANA_CORE_ANA_CON63);
		typec_write8(hba, 0x00, 0x53C);
		typec_write8(hba, 0x02, 0x51E);
		typec_write8(hba, 0x44, 0x529);
		typec_write8(hba, 0xE0, 0x55F);
	}

skip:
	if ((hba->dbg_lvl >= TYPEC_DBG_LVL_2) && (hba->vbus_en != on))
		dev_err(hba->dev, "VBUS %s", (on ? "ON" : "OFF"));

	hba->vbus_en = (on ? 1 : 0);
}

#else
#define RG_EN_OTG	 (0x1<<0x3)

void typec_drive_vbus(struct typec_hba *hba, uint8_t on)
{
	if (hba->vbus_en == on)
		goto skip;

	/*
	 *  From Script_20160823.xlsx provided by Lynch Lin
	 */
	if (on) {
		uint8_t tmp;
		/*
		 * ==Initial Setting==
		 * WR	56	411	51	OTG CV and IOLP setting
		 *
		 * ==Enable OTG==
		 * 1. Check 0x612[4] (DA_QI_FLASH_MODE) and 0x0613[1] (DA_QI_VBUS_PLUGIN)
		 *
		 * 2. If they are both low, apply below settings
		 * 3. If any of them is high, do nothing (chip does not support OTG mode while under CHR or Flash mode)"
		 *
		 * WR	57	519	0D
		 * WR	57	520	00
		 * WR	57	55A	01
		 * WR	56	455	00
		 * WR	55	3C9	00
		 *
		 * WR	55	3CF	00
		 * WR	57	553	14
		 * WR	57	55F	E6
		 * WR	57	53D	47
		 * WR	57	529	8E
		 *
		 * WR	57	560	0C
		 * WR	56	40F	04
		 *
		 * WR	56	400	[3]=1'b1	Enable OTG
		 */

		typec_write8(hba, 0x55, 0x411);

		tmp = typec_read8(hba, 0x612);
		if (tmp & (0x1<<4)) {
			dev_err(hba->dev, "At Flash mode, Can not turn on OTG\n");
			return;
		}

		tmp = typec_read8(hba, 0x613);
		if (tmp & (0x1<<1)) {
			dev_err(hba->dev, "VBUS present, Can not turn on OTG\n");
			return;
		}

		typec_write8(hba, 0x0D, 0x519);
		typec_write8(hba, 0x00, 0x520);
		typec_write8(hba, 0x01, 0x55A);
		typec_write8(hba, 0x00, 0x455);
		typec_write8(hba, 0x00, 0x3C9);

		typec_write8(hba, 0x00, 0x3CF);
		typec_write8(hba, 0x14, 0x553);
		typec_write8(hba, 0xE6, 0x55F);
		typec_write8(hba, 0x47, 0x53D);
		typec_write8(hba, 0x8E, 0x529);

		typec_write8(hba, 0x0C, 0x560);
		typec_write8(hba, 0x04, 0x40F);

		tmp = typec_read8(hba, 0x400);
		typec_write8(hba, (tmp | RG_EN_OTG), 0x400);
	} else {
		uint8_t tmp;
		/*
		 * ==Disable OTG==
		 *
		 * WR	57	52A	88
		 * WR	57	553	14
		 * WR	57	519	3F
		 * WR	57	51E	02
		 *
		 * WR	57	520	04
		 * WR	57	55A	00
		 * WR	56	455	01
		 * WR	55	3C9	10
		 *
		 * WR	55	3CF	03
		 * WR	57	5AF	02
		 * WR	58	64E	02
		 * WR	56	402	03
		 *
		 * WR	57	529	88
		 *
		 * WR	56	400	[3]=1'b0	Disable OTG
		*/
		typec_write8(hba, 0x88, 0x52A);
		typec_write8(hba, 0x14, 0x553);
		typec_write8(hba, 0x3F, 0x519);
		typec_write8(hba, 0x02, 0x51E);

		typec_write8(hba, 0x04, 0x520);
		typec_write8(hba, 0x00, 0x55A);
		typec_write8(hba, 0x01, 0x455);
		typec_write8(hba, 0x10, 0x3C9);

		typec_write8(hba, 0x03, 0x3CF);
		typec_write8(hba, 0x02, 0x5AF);
		typec_write8(hba, 0x02, 0x64E);
		typec_write8(hba, 0x03, 0x402);

		typec_write8(hba, 0x88, 0x529);

		tmp = typec_read8(hba, 0x400);
		typec_write8(hba, (tmp & (~RG_EN_OTG)), 0x400);
	}

skip:
	if ((hba->dbg_lvl >= TYPEC_DBG_LVL_2) && (hba->vbus_en != on))
		dev_err(hba->dev, "VBUS %s", (on ? "ON" : "OFF"));

	hba->vbus_en = (on ? 1 : 0);
}
#endif
#endif

#if SUPPORT_PD
void control_ldo(struct typec_hba *hba, uint8_t on)
{
	uint8_t val = 0;
	/* set to GPIO mode GPIO_MODE5[0:2] = 0*/
	val = typec_read8(hba, 0x731);
	typec_write8(hba, val & ~0x7, 0x731);

	/* set to OUTPUT mode GPIO_DIR1_SET[2] = 1*/
	typec_write8(hba, 0x4, 0x704);

	/* output high / low*/
	if (on) /* GPIO_DOUT1_SET[2] = 1 */
		typec_write8(hba, 0x4, 0x71C);
	else /* GPIO_DOUT1_CLR[2] = 1*/
		typec_write8(hba, 0x4, 0x71D);
}

void typec_drive_vconn(struct typec_hba *hba, uint8_t on)
{
	/*TEST:read ro_type_c_drive_vconn_capable@TYPE_C_PWR_STATUS*/
	if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
		dev_err(hba->dev, "VCONN CAPABLE = %d",
			(typec_readw(hba, TYPE_C_PWR_STATUS) & RO_TYPE_C_DRIVE_VCONN_CAPABLE));

	/*Use MT6336 GPIO6 to control outside LDO*/
	if (on)
		control_ldo(hba, on);

	typec_writew_msk(hba, TYPE_C_SW_DA_DRIVE_VCONN_EN,
		((on) ? TYPE_C_SW_DA_DRIVE_VCONN_EN : 0), TYPE_C_CC_SW_CTRL);

	/*Use MT6336 GPIO6 to control outside LDO*/
	if (!on)
		control_ldo(hba, on);

	if ((hba->dbg_lvl >= TYPEC_DBG_LVL_2) && (hba->vconn_en != on))
		dev_err(hba->dev, "VCONN %s", (on ? "ON" : "OFF"));

	hba->vconn_en = (on ? 1 : 0);
}
#endif

#define CORE_ANA_CON109 (0x566)
#define RG_A_ANABASE_RSV (0xFF<<0)
#define ENABLE_AVDD33_TYPEC_MSK (0x1<<3)
#define ENABLE_AVDD33_TYPEC_OFST (3)

#define CLK_LOWQ_PDN_DIS_CON1 (0x044)
#define CLK_LOWQ_PDN_DIS_CON1_SET (0x045)
#define CLK_TYPE_C_CSR_LOWQ_PDN_DIS_MSK (0x1<<0)
#define CLK_TYPE_C_CSR_LOWQ_PDN_DIS_OFST (0)

#define CLK_LOWQ_PDN_DIS_CON0 (0x041)
#define CLK_LOWQ_PDN_DIS_CON0_SET (0x042)
#define CLK_TYPE_C_CC_LOWQ_PDN_DIS_MSK (0x1<<6)
#define CLK_TYPE_C_CC_LOWQ_PDN_DIS_OFST (6)

static void typec_basic_settings(struct typec_hba *hba)
{
/*
 * 5.1 Basic Settings @ MT6336 Type-C programming guide v0.1.docx
 * 1. 0x0566[3]: RG_A_ANABASE_RSV[3] set 1'b1.  ' Enable the AVDD33 detection.
 * 2. 0x0045[0] set 1'b1, turn on csr_ck. (0x0046[0] set 1'b1 can turn off this clock in lowQ)
 * 3. 0x0042[6] set 1'b1, turn on cc_ck. (0x0043[6] set 1'b1 can turn off this clock in lowQ)
 */
	const int is_print = 0;

#if COMPLIANCE
	typec_write8(hba, 0x01, MAIN_CON8);
	if (is_print)
		dev_err(hba->dev, "MAIN_CON8(0x409)=0x%x [0x1]\n", typec_read8(hba, MAIN_CON8));
#endif

	typec_write8(hba, (1<<ENABLE_AVDD33_TYPEC_OFST), CORE_ANA_CON109);
	if (is_print)
		dev_err(hba->dev, "CORE_ANA_CON109(0x566)=0x%x [0x8]\n", typec_read8(hba, CORE_ANA_CON109));

#ifdef MT6336_E2
	/*
	 * For E2. Request from Chun-Kai Tseng 20160531_1607
	 * RG_PD_UVP_VTH[2:0] set 2'b011b at initial.
	 */
	typec_write8(hba, 0x3, PD_PHY_RG_PD_0+1);
	if (is_print)
		dev_err(hba->dev, "0x02F1=0x%x [0x20]\n", typec_read8(hba, PD_PHY_RG_PD_0+1));
#endif

	typec_write8(hba, (1<<CLK_TYPE_C_CSR_LOWQ_PDN_DIS_OFST), CLK_LOWQ_PDN_DIS_CON1_SET);
	if (is_print)
		dev_err(hba->dev, "CLK_LOWQ_PDN_DIS_CON1=0x%x\n", typec_read8(hba, CLK_LOWQ_PDN_DIS_CON1));

	typec_write8(hba, (1<<CLK_TYPE_C_CC_LOWQ_PDN_DIS_OFST), CLK_LOWQ_PDN_DIS_CON0_SET);
	if (is_print)
		dev_err(hba->dev, "CLK_LOWQ_PDN_DIS_CON0=0x%x\n", typec_read8(hba, CLK_LOWQ_PDN_DIS_CON0));
}

static void typec_set_default_param(struct typec_hba *hba)
{
	uint32_t val;

	/* timing parameters */
	val = ZERO_INDEXED_DIV_AND_RND_UP(REF_CLK_DIVIDEND * CC_VOL_PERIODIC_MEAS_VAL, REF_CLK_DIVIDER);
	typec_writew(hba, val, TYPE_C_CC_VOL_PERIODIC_MEAS_VAL);
	val = DIV_AND_RND_UP(CC_VOL_PD_DEBOUNCE_CNT_VAL, CC_VOL_PERIODIC_MEAS_VAL);
	typec_writew_msk(hba, REG_TYPE_C_CC_VOL_PD_DEBOUNCE_CNT_VAL,
		(val<<REG_TYPE_C_CC_VOL_PD_DEBOUNCE_CNT_VAL_OFST), TYPE_C_CC_VOL_DEBOUCE_CNT_VAL);
	val = DIV_AND_RND_UP(CC_VOL_CC_DEBOUNCE_CNT_VAL, CC_VOL_PERIODIC_MEAS_VAL);
	typec_writew_msk(hba, REG_TYPE_C_CC_VOL_CC_DEBOUNCE_CNT_VAL,
		(val<<REG_TYPE_C_CC_VOL_CC_DEBOUNCE_CNT_VAL_OFST), TYPE_C_CC_VOL_DEBOUCE_CNT_VAL);
	val = ZERO_INDEXED_DIV_AND_RND_UP(REF_CLK_DIVIDEND * DRP_SRC_CNT_VAL, REF_CLK_DIVIDER);
	typec_writew(hba, val, TYPE_C_DRP_SRC_CNT_VAL_0);
	val = ZERO_INDEXED_DIV_AND_RND_UP(REF_CLK_DIVIDEND * DRP_SNK_CNT_VAL, REF_CLK_DIVIDER);
	typec_writew(hba, val, TYPE_C_DRP_SNK_CNT_VAL_0);
	val = ZERO_INDEXED_DIV_AND_RND_UP(REF_CLK_DIVIDEND * DRP_TRY_CNT_VAL, REF_CLK_DIVIDER);
	typec_writew(hba, val, TYPE_C_DRP_TRY_CNT_VAL_0);
	val = ZERO_INDEXED_DIV_AND_RND_UP(REF_CLK_DIVIDEND * DRP_TRY_WAIT_CNT_VAL, REF_CLK_DIVIDER);
	typec_writew(hba, val & 0xffff, TYPE_C_DRP_TRY_WAIT_CNT_VAL_0);
	typec_writew(hba, (val>>16), TYPE_C_DRP_TRY_WAIT_CNT_VAL_1);


	/* SRC reference voltages */
	typec_writew(hba, (SRC_VRD_DEFAULT_DAC_VAL<<REG_TYPE_C_CC_SRC_VRD_DEFAULT_DAC_VAL_OFST |
		SRC_VOPEN_DEFAULT_DAC_VAL<<REG_TYPE_C_CC_SRC_VOPEN_DEFAULT_DAC_VAL_OFST),
		TYPE_C_CC_SRC_DEFAULT_DAC_VAL);

	typec_writew(hba, (SRC_VRD_15_DAC_VAL<<REG_TYPE_C_CC_SRC_VRD_15_DAC_VAL_OFST |
		SRC_VOPEN_15_DAC_VAL<<REG_TYPE_C_CC_SRC_VOPEN_15_DAC_VAL_OFST),
		TYPE_C_CC_SRC_15_DAC_VAL);

	typec_writew(hba, (SRC_VRD_30_DAC_VAL<<REG_TYPE_C_CC_SRC_VRD_30_DAC_VAL_OFST |
		SRC_VOPEN_30_DAC_VAL<<REG_TYPE_C_CC_SRC_VOPEN_30_DAC_VAL_OFST),
		TYPE_C_CC_SRC_30_DAC_VAL);

	/* SNK reference voltages */
	typec_writew(hba, (SNK_VRP15_DAC_VAL<<REG_TYPE_C_CC_SNK_VRP15_DAC_VAL_OFST |
		SNK_VRP30_DAC_VAL<<REG_TYPE_C_CC_SNK_VRP30_DAC_VAL_OFST),
		TYPE_C_CC_SNK_DAC_VAL_0);

	typec_writew(hba, SNK_VRPUSB_DAC_VAL<<REG_TYPE_C_CC_SNK_VRPUSB_DAC_VAL_OFST,
		TYPE_C_CC_SNK_DAC_VAL_1);

	/* mode configuration */
	/*AUXADC or DAC (preferred)*/
	#if USE_AUXADC
	typec_set(hba, REG_TYPE_C_ADC_EN, TYPE_C_CTRL);
	#else
	typec_clear(hba, REG_TYPE_C_ADC_EN, TYPE_C_CTRL);
	#endif
	/*termination decided by controller*/
	typec_clear(hba, TYPEC_TERM_CC, TYPE_C_CC_SW_FORCE_MODE_ENABLE);
	/*enable/disable accessory mode*/
	#if ENABLE_ACC
	typec_set(hba, TYPEC_ACC_EN, TYPE_C_CTRL);
	#else
	typec_clear(hba, TYPEC_ACC_EN, TYPE_C_CTRL);
	#endif
}

int typec_enable(struct typec_hba *hba, int enable)
{
	if (enable) {
		/*#if SUPPORT_PD*/
		typec_vbus_det_enable(hba, 1);
		/*#endif*/

		switch (hba->support_role) {
		case TYPEC_ROLE_SINK:
			typec_set(hba, W1_TYPE_C_SW_ENT_UNATCH_SNK_CMD, TYPE_C_CC_SW_CTRL);
			break;
		case TYPEC_ROLE_SOURCE:
			typec_set(hba, W1_TYPE_C_SW_ENT_UNATCH_SRC_CMD, TYPE_C_CC_SW_CTRL);
			break;
		case TYPEC_ROLE_DRP:
			typec_set(hba, W1_TYPE_C_SW_ENT_UNATCH_SNK_CMD, TYPE_C_CC_SW_CTRL);
			break;
		default:
			return 1;
		}
	} else {
		/*#if SUPPORT_PD*/
		typec_vbus_det_enable(hba, 0);
		/*#endif*/

		typec_set(hba, W1_TYPE_C_SW_ENT_DISABLE_CMD, TYPE_C_CC_SW_CTRL);
		typec_clear(hba, TYPEC_TERM_CC, TYPE_C_CC_SW_FORCE_MODE_ENABLE);
		typec_clear(hba, (TYPEC_TERM_CC1 | TYPEC_TERM_CC2), TYPE_C_CC_SW_FORCE_MODE_VAL_0);
	}
	return 0;
}

void typec_select_rp(struct typec_hba *hba, enum enum_typec_rp rp_val)
{
	typec_writew_msk(hba, TYPE_C_PHY_RG_CC_RP_SEL, rp_val, TYPE_C_PHY_RG_0);
}

void typec_force_term(struct typec_hba *hba, enum enum_typec_term cc1_term, enum enum_typec_term cc2_term)
{
	uint32_t val;

	val = 0;
	val |= (cc1_term == TYPEC_TERM_RP) ? REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC1_EN :
		(cc1_term == TYPEC_TERM_RD) ? REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC1_EN :
		(cc1_term == TYPEC_TERM_RA) ? REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC1_EN : 0;
	val |= (cc2_term == TYPEC_TERM_RP) ? REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC2_EN :
		(cc2_term == TYPEC_TERM_RD) ? REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC2_EN :
		(cc2_term == TYPEC_TERM_RA) ? REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC2_EN : 0;

	typec_set(hba, TYPEC_TERM_CC, TYPE_C_CC_SW_FORCE_MODE_ENABLE);
	typec_writew_msk(hba, (TYPEC_TERM_CC1 | TYPEC_TERM_CC2), val, TYPE_C_CC_SW_FORCE_MODE_VAL_0);
}

int typec_set_mode(struct typec_hba *hba, enum enum_typec_role role, int param1, int param2)
{
	enum enum_typec_rp rp_val = param1;
	enum enum_try_mode try_mode = param2;

	/*go back to disable state*/
	typec_enable(hba, 0);

	switch (role) {
	case TYPEC_ROLE_SINK_W_ACTIVE_CABLE:
		if (param1 == 1)
			typec_force_term(hba, TYPEC_TERM_RD, TYPEC_TERM_RA);
		else if (param1 == 2)
			typec_force_term(hba, TYPEC_TERM_RA, TYPEC_TERM_RD);
		else
			goto err_handle;

		break;

	case TYPEC_ROLE_SINK:
		typec_writew_msk(hba, REG_TYPE_C_PORT_SUPPORT_ROLE, TYPEC_ROLE_SINK, TYPE_C_CTRL);
		break;

	case TYPEC_ROLE_SOURCE:
		typec_writew_msk(hba, REG_TYPE_C_PORT_SUPPORT_ROLE, role, TYPE_C_CTRL);
		typec_select_rp(hba, rp_val);
		break;

	case TYPEC_ROLE_DRP:
		typec_writew_msk(hba, REG_TYPE_C_PORT_SUPPORT_ROLE, role, TYPE_C_CTRL);
		typec_select_rp(hba, rp_val);

		switch (try_mode) {
		case TYPEC_TRY_DISABLE:
			typec_clear(hba, TYPEC_TRY, TYPE_C_CTRL);
			break;
		case TYPEC_TRY_ENABLE:
			typec_set(hba, TYPEC_TRY, TYPE_C_CTRL);
			break;
		default:
			goto err_handle;
		}
		break;

	case TYPEC_ROLE_ACCESSORY_AUDIO:
		typec_force_term(hba, TYPEC_TERM_RA, TYPEC_TERM_RA);
		break;

	case TYPEC_ROLE_ACCESSORY_DEBUG:
		typec_force_term(hba, TYPEC_TERM_RD, TYPEC_TERM_RD);
		break;

	case TYPEC_ROLE_OPEN:
		typec_force_term(hba, TYPEC_TERM_NA, TYPEC_TERM_NA);
		break;

	default:
		goto err_handle;
	}
	return 0;

err_handle:
	return 1;
}

static void typec_cc_state(struct typec_hba *hba)
{
	uint8_t pwr_status = typec_read8(hba, TYPE_C_PWR_STATUS);

	hba->cc = (((typec_readw(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_ROUTED_CC) == 0) ? CC1_SIDE : CC2_SIDE);

	hba->power_role = (pwr_status & RO_TYPE_C_CC_PWR_ROLE) ? PD_ROLE_SOURCE : PD_ROLE_SINK;

	hba->src_rp = pwr_status & RO_TYPE_C_CC_SNK_PWR_ST;

	hba->ra = !!(pwr_status & RO_TYPE_C_DRIVE_VCONN_CAPABLE);

	dev_err(hba->dev, "%s CC%d %s %s\n", hba->power_role?"SRC":"SNK",
				hba->cc, SRC_CUR(hba->src_rp), ((hba->ra == 1)?"Ra":""));
}

static void typec_wait_vbus_on_try_wait_snk(struct work_struct *work)
{
	int i = 0;
	struct typec_hba *hba = container_of(work, struct typec_hba, wait_vbus_on_try_wait_snk);

	hba->wq_running |= WQ_FLAGS_VBUSON_TRYWAIT_SNK;
	hba->wq_cnt = 0;

	typec_disable_lowq(hba, "typec_wait_vbus_on_try_wait_snk");

	while (((typec_read8(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_CC_ST) == TYPEC_STATE_TRY_WAIT_SNK)
		&& (i < POLLING_MAX_TIME(hba->vbus_on_polling))) {

		hba->wq_cnt++;

		if (hba->vbus_det_en && (typec_vbus(hba) > PD_VSAFE5V_LOW)) {
			typec_vbus_present(hba, 1);
			dev_err(hba->dev, "Vbus ON in TryWait.SNK state\n");

			break;
		}
		i++;
		msleep(hba->vbus_on_polling);
	}

	hba->wq_running &= ~WQ_FLAGS_VBUSON_TRYWAIT_SNK;

	typec_enable_lowq(hba, "typec_wait_vbus_on_try_wait_snk");
}

static void typec_wait_vbus_on_attach_wait_snk(struct work_struct *work)
{
	int i = 0;
	struct typec_hba *hba = container_of(work, struct typec_hba, wait_vbus_on_attach_wait_snk);

	hba->wq_running |= WQ_FLAGS_VBUSON_ATTACHWAIT_SNK;
	hba->wq_cnt = 0;

	typec_disable_lowq(hba, "typec_wait_vbus_on_attach_wait_snk");

	while (((typec_read8(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_CC_ST) == TYPEC_STATE_ATTACH_WAIT_SNK)
		&& (i < POLLING_MAX_TIME(hba->vbus_on_polling))) {

		hba->wq_cnt++;

		if (hba->vbus_det_en && (typec_vbus(hba) > PD_VSAFE5V_LOW)) {

			if (hba->vbus_en == 1) {
				typec_drive_vbus(hba, 0);
			} else {
				typec_vbus_present(hba, 1);

				dev_err(hba->dev, "Vbus ON in AttachWait.SNK state\n");

				break;
			}
		}
		i++;
		msleep(hba->vbus_on_polling);
	}

	hba->wq_running &= ~WQ_FLAGS_VBUSON_ATTACHWAIT_SNK;

	typec_enable_lowq(hba, "typec_wait_vbus_on_attach_wait_snk");
}

static int keep_low(struct typec_hba *hba, int vbus)
{
	static ktime_t last_t_vsafe5v;

	if (vbus > PD_VSAFE0V_HIGH) {
		last_t_vsafe5v = ktime_get();
		return 0;
	}

	if (ktime_ms_delta(ktime_get(), last_t_vsafe5v) > 5000) {
		dev_err(hba->dev, "%s VBUS Keep LOW lasting 5sec", __func__);
		return 1;
	}

	return 0;
}

static void typec_wait_vbus_off_attached_snk(struct work_struct *work)
{
	struct typec_hba *hba = container_of(work, struct typec_hba, wait_vbus_off_attached_snk);

	hba->wq_running |= WQ_FLAGS_VBUSOFF_ATTACHED_SNK;
	hba->wq_cnt = 0;

	typec_disable_lowq(hba, "typec_wait_vbus_off_attached_snk");

	while ((typec_read8(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_CC_ST) == TYPEC_STATE_ATTACHED_SNK) {
		int val = typec_vbus(hba);

		hba->wq_cnt++;

		if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
			dev_err(hba->dev, "%s VBUS = %d, DET_EN=%d", __func__, val, hba->vbus_det_en);

		if ((hba->vbus_det_en && (val < hba->vsafe_5v)) || keep_low(hba, val)) {

#if COMPLIANCE
			if (val > PD_VSAFE0V_HIGH) {
				typec_set(hba, TYPE_C_SW_CC_DET_DIS, TYPE_C_CC_SW_CTRL);
				queue_work(hba->pd_wq, &hba->wait_vsafe0v);
			}
#endif
			/* disable RX */
			if (hba->mode == 2)
				pd_rx_enable(hba, 0);

			/* notify IP VBUS is gone.*/
			typec_vbus_present(hba, 0);
			dev_err(hba->dev, "Vbus OFF in Attachd.SNK state\n");
			#if USE_AUXADC
			typec_auxadc_set_event(hba);
			#endif
			break;
		}
		msleep(hba->vbus_off_polling);
	}

	typec_enable_lowq(hba, "typec_wait_vbus_off_attached_snk");

	hba->wq_running &= ~WQ_FLAGS_VBUSOFF_ATTACHED_SNK;

	dev_err(hba->dev, "Exit %s st=%d", __func__, typec_read8(hba, TYPE_C_CC_STATUS));
}

#define INIT_VBUS_OFF_PERIOD 100
#define INIT_VBUS_OFF_CNT 5

static void typec_init_vbus_off(struct work_struct *work)
{
	struct typec_hba *hba = container_of(work, struct typec_hba, init_vbus_off);
	int cnt = 0;

	dev_err(hba->dev, "typec_init_vbus_off+\n");

	typec_disable_lowq(hba, "typec_wait_vsafe0v");

	while (cnt < INIT_VBUS_OFF_CNT) {
		int val = typec_vbus(hba);

		if (val < PD_VSAFE5V_LOW)
			cnt++;
		else
			break;

		msleep(INIT_VBUS_OFF_PERIOD);
	}

	if (cnt == INIT_VBUS_OFF_CNT && hba->charger_det_notify)
		hba->charger_det_notify(0);

	dev_err(hba->dev, "typec_init_vbus_off-\n");

	typec_enable_lowq(hba, "typec_wait_vsafe0v");
}

#define WAIT_VSAFE0V_PERIOD 1000
#define WAIT_VSAFE0V_POLLING_CNT 20

static void typec_wait_vsafe0v(struct work_struct *work)
{
	struct typec_hba *hba = container_of(work, struct typec_hba, wait_vsafe0v);
	int cnt = 0;

	hba->wq_running |= WQ_FLAGS_VSAFE0V;
	hba->wq_cnt = 0;

	typec_disable_lowq(hba, "typec_wait_vsafe0v");

	while (cnt < WAIT_VSAFE0V_POLLING_CNT) {
		int val = typec_vbus(hba);

		hba->wq_cnt++;

		if (val < PD_VSAFE0V_HIGH) {
			typec_clear(hba, TYPE_C_SW_CC_DET_DIS, TYPE_C_CC_SW_CTRL);
			dev_err(hba->dev, "At vSafe0v\n");
			break;
		}

		msleep(WAIT_VSAFE0V_PERIOD / WAIT_VSAFE0V_POLLING_CNT);
		cnt++;
	}

	hba->wq_running &= ~WQ_FLAGS_VSAFE0V;

	typec_enable_lowq(hba, "typec_wait_vsafe0v");
}

static void typec_wait_vbus_off_then_drive_attached_src(struct work_struct *work)
{
	struct typec_hba *hba = container_of(work, struct typec_hba, wait_vbus_off_then_drive_attached_src);

	hba->wq_running |= WQ_FLAGS_VBUSOFF_THEN_VBUSON;
	hba->wq_cnt = 0;

	typec_disable_lowq(hba, "typec_wait_vbus_off_then_drive_attached_src");

	while ((typec_read8(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_CC_ST) == TYPEC_STATE_ATTACHED_SRC) {

		hba->wq_cnt++;

		if (hba->vbus_det_en && (typec_vbus(hba) < PD_VSAFE0V_HIGH)) {

#ifdef CONFIG_USBC_VCONN
			/* drive Vconn ONLY when there is Ra */
			if (hba->ra)
				typec_drive_vconn(hba, 1);
#endif

			typec_drive_vbus(hba, 1);

			break;
		}
		msleep(20);
	}

	hba->wq_running &= ~WQ_FLAGS_VBUSOFF_THEN_VBUSON;

	typec_enable_lowq(hba, "typec_wait_vbus_off_then_drive_attached_src");
}

#if USE_AUXADC
static void typec_auxadc_voltage_mon_attached_snk(struct work_struct *work)
{
	uint16_t val;
	int ret = 0;
	int flag = FLAGS_AUXADC_NONE;
	struct typec_hba *hba = container_of(work, struct typec_hba, auxadc_voltage_mon_attached_snk);

	dev_err(hba->dev, "Enter typec_auxadc_voltage_mon_attached_snk\n");

	typec_auxadc_register(hba);
	typec_auxadc_set_state(hba, STATE_POWER_DEFAULT);

	while (1) {
		ret = wait_for_completion_interruptible_timeout(&hba->auxadc_event, msecs_to_jiffies(1000));

		flag = hba->auxadc_flags;

		hba->auxadc_flags = FLAGS_AUXADC_NONE;

		if (!ret || !flag) {
			if ((typec_readw(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_CC_ST) != TYPEC_STATE_ATTACHED_SNK) {
				dev_err(hba->dev, "Leave Attached.SNK\n");
				break;
			}
		} else {
			if (hba->dbg_lvl >= TYPEC_DBG_LVL_3)
				dev_err(hba->dev, "Event coming in %d\n", flag);

			val = typec_auxadc_get_value(hba, flag);

			dev_err(hba->dev, "ADC VAL=%d\n", val);

			typec_disable_auxadc(hba, 1, 1);

			/*SNK_VRP30_AUXADC_MIN_VAL < tmp <= SNK_VRP30_AUXADC_MAX_VAL*/
			if (val > SNK_VRP30_AUXADC_MIN_VAL) {
				typec_set(hba, W1_TYPE_C_SW_ADC_RESULT_MET_VRD_30_CMD, TYPE_C_CC_SW_CTRL);
				typec_auxadc_set_state(hba, STATE_POWER_3000MA);
			}
			/*SNK_VRP15_AUXADC_MIN_VAL < tmp <= SNK_VRP15_AUXADC_MAX_VAL*/
			else if (val > SNK_VRP15_AUXADC_MIN_VAL) {
				typec_set(hba, W1_TYPE_C_SW_ADC_RESULT_MET_VRD_15_CMD, TYPE_C_CC_SW_CTRL);
				typec_auxadc_set_state(hba, STATE_POWER_1500MA);
			}
			/*SNK_VRPUSB_AUXADC_MIN_VAL <= tmp <= SNK_VRPUSB_AUXADC_MAX_VAL*/
			else if (val > SNK_VRPUSB_AUXADC_MIN_VAL) {
				typec_set(hba, W1_TYPE_C_SW_ADC_RESULT_MET_VRD_DEFAULT_CMD, TYPE_C_CC_SW_CTRL);
				typec_auxadc_set_state(hba, STATE_POWER_DEFAULT);
			} else {
				if ((typec_readw(hba, TYPE_C_CC_STATUS) & RO_TYPE_C_CC_ST) !=
					TYPEC_STATE_ATTACHED_SNK) {

					dev_err(hba->dev, "Leave Attached.SNK\n");
					break;
				}
				dev_err(hba->dev, "CC_STATUS=0x%X\n", typec_readw(hba, TYPE_C_CC_STATUS));
			}

			typec_enable_auxadc_irq(hba);
		}
	}

	typec_auxadc_unregister(hba);
}
#endif

void typec_int_enable(struct typec_hba *hba, uint16_t msk0, uint16_t msk2)
{
	typec_set(hba, msk0, TYPE_C_INTR_EN_0);
	typec_set(hba, msk2, TYPE_C_INTR_EN_2);
}

void typec_int_disable(struct typec_hba *hba, uint16_t msk0, uint16_t msk2)
{
	typec_clear(hba, msk0, TYPE_C_INTR_EN_0);
	typec_clear(hba, msk2, TYPE_C_INTR_EN_2);
}

#ifdef NEVER
static void typec_dump_intr(struct typec_hba *hba, uint16_t is0, uint16_t is2)
{
	int i;
	const struct bit_mapping is0_mapping[] = {
		{TYPE_C_CC_ENT_ATTACH_SRC_INTR, "CC_ENT_ATTACH_SRC"},
		{TYPE_C_CC_ENT_ATTACH_SNK_INTR, "CC_ENT_ATTACH_SNK"},
		#if ENABLE_ACC
		{TYPE_C_CC_ENT_AUDIO_ACC_INTR, "CC_ENT_AUDIO_ACC"},
		{TYPE_C_CC_ENT_DBG_ACC_INTR, "CC_ENT_DBG_ACC"},
		#endif
		{TYPE_C_CC_ENT_TRY_SRC_INTR, "CC_ENT_TRY_SRC"},
		{TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR, "CC_ENT_TRY_WAIT_SNK"},
	};
	const struct bit_mapping is0_mapping_dbg[] = {
		{TYPE_C_CC_ENT_DISABLE_INTR, "CC_ENT_DISABLE"},
		{TYPE_C_CC_ENT_UNATTACH_SRC_INTR, "CC_ENT_UNATTACH_SRC"},
		{TYPE_C_CC_ENT_UNATTACH_SNK_INTR, "CC_ENT_UNATTACH_SNK"},
		{TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR, "CC_ENT_ATTACH_WAIT_SRC"},
		{TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR, "CC_ENT_ATTACH_WAIT_SNK"},
		#if ENABLE_ACC
		{TYPE_C_CC_ENT_UNATTACH_ACC_INTR, "CC_ENT_UNATTACH_ACC"},
		{TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR, "CC_ENT_ATTACH_WAIT_ACC"},
		#endif
	};
	const struct bit_mapping is2_mapping[] = {
		{TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR, "CC_ENT_SNK_PWR_DEFAULT"},
		{TYPE_C_CC_ENT_SNK_PWR_15_INTR, "CC_ENT_SNK_PWR_15"},
		{TYPE_C_CC_ENT_SNK_PWR_30_INTR, "CC_ENT_SNK_PWR_30"},
	};
	const struct bit_mapping is2_mapping_dbg[] = {
		{TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR, "CC_ENT_SNK_PWR_REDETECT"},
		{TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR, "CC_ENT_SNK_PWR_IDLE"},
	};


	if (hba->dbg_lvl >= TYPEC_DBG_LVL_1) {
		for (i = 0; i < sizeof(is0_mapping)/sizeof(struct bit_mapping); i++) {
			if (is0 & is0_mapping[i].mask)
				dev_err(hba->dev, "%s\n", is0_mapping[i].name);
		}

		for (i = 0; i < sizeof(is2_mapping)/sizeof(struct bit_mapping); i++) {
			if (is2 & is2_mapping[i].mask)
				dev_err(hba->dev, "%s\n", is2_mapping[i].name);
		}
	}

	if (hba->dbg_lvl >= TYPEC_DBG_LVL_3) {
		for (i = 0; i < sizeof(is0_mapping_dbg)/sizeof(struct bit_mapping); i++) {
			if (is0 & is0_mapping_dbg[i].mask)
				dev_err(hba->dev, "%s\n", is0_mapping_dbg[i].name);
		}

		for (i = 0; i < sizeof(is2_mapping_dbg)/sizeof(struct bit_mapping); i++) {
			if (is2 & is2_mapping_dbg[i].mask)
				dev_err(hba->dev, "%s\n", is2_mapping_dbg[i].name);
		}
	}
}
#endif

/**
 * typec_intr - Interrupt service routine
 * @hba: per adapter instance
 * @is0: interrupt status 0
 * @is2: interrupt status 2
 */
static void typec_intr(struct typec_hba *hba, uint16_t cc_is0, uint16_t cc_is2)
{
	#if ENABLE_ACC
	uint16_t toggle = TYPE_C_INTR_DRP_TOGGLE | TYPE_C_INTR_ACC_TOGGLE;
	#else
	uint16_t toggle = TYPE_C_INTR_DRP_TOGGLE;
	#endif

	/*serve interrupts according to power role*/
	if (cc_is0 & (TYPE_C_CC_ENT_DISABLE_INTR | toggle)) {
		/*Move trigger host/device driver to pd_task, if support PD*/
		if ((hba->mode == 1) && (hba->cc > DONT_CARE)) {
			hba->data_role = PD_NO_ROLE;
			cancel_delayed_work_sync(&hba->usb_work);
			schedule_delayed_work(&hba->usb_work, 0);
		}

		hba->cc = DONT_CARE;

		/*ignore SNK<->SRC & SNK<->ACC*/
		typec_int_disable(hba, toggle, 0);

		typec_vbus_present(hba, 0);
		typec_drive_vbus(hba, 0);

		if (hba->mode == 1)
			typec_enable_lowq(hba, "UNATTACH");
	}

	if (cc_is0 & TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR) {
		/* At AttachWait.SNK, continue checking vSafe5V is presented or not?
		 * If Vbus detected, set TYPE_C_SW_VBUS_PRESENT@TYPE_C_CC_SW_CTRL(0xA) as 1
		 * to notify MAC layer.
		 */
		queue_work(hba->pd_wq, &hba->wait_vbus_on_attach_wait_snk);
	}

#if ENABLE_ACC
	if (cc_is0 & ((TYPE_C_CC_ENT_ATTACH_SNK_INTR | TYPE_C_CC_ENT_ATTACH_SRC_INTR) |
		(TYPE_C_CC_ENT_AUDIO_ACC_INTR | TYPE_C_CC_ENT_DBG_ACC_INTR))) {
#else
	if (cc_is0 & (TYPE_C_CC_ENT_ATTACH_SNK_INTR | TYPE_C_CC_ENT_ATTACH_SRC_INTR)) {
#endif

#if ENABLE_ACC
		/*SNK<->ACC toggle happens ONLY for sink*/
		if (hba->support_role == TYPEC_ROLE_SINK)
			typec_int_enable(hba, TYPE_C_INTR_ACC_TOGGLE, TYPE_C_INTR_SRC_ADVERTISE);
		else
#endif
			typec_int_enable(hba, TYPE_C_INTR_DRP_TOGGLE, TYPE_C_INTR_SRC_ADVERTISE);
	}

	if (cc_is0 & TYPE_C_CC_ENT_ATTACH_SNK_INTR) {
		typec_cc_state(hba);

		/*Move trigger host/device driver to pd_task, if support PD*/
		if (hba->mode == 1) {
			hba->data_role = PD_ROLE_UFP;
			schedule_delayed_work(&hba->usb_work, 0);
		}

		if (!pd_is_power_swapping(hba))
			typec_disable_lowq(hba, "ATTACH.SNK");

		/* At Attached.SNK, continue checking vSafe5V is presented or not?
		 * If Vbus is removed, set TYPE_C_SW_VBUS_PRESENT@TYPE_C_CC_SW_CTRL(0xA) as 0
		 * to notify MAC layer.
		 */
		queue_work(hba->pd_wq, &hba->wait_vbus_off_attached_snk);

		#if USE_AUXADC
		if (hba->is_kpoc && hba->charger_det_notify) {
			typec_auxadc_low_register(hba);
			typec_auxadc_set_thresholds(hba, SNK_VRPUSB_AUXADC_MIN_VAL, 0);
			mt6336_enable_interrupt(TYPE_C_L_MIN, "TYPE_C_L_MIN");
			pmic_enable_chrdet(0);
		}
		/*schedule_work(&hba->auxadc_voltage_mon_attached_snk);*/
		#endif
	}

	if (cc_is0 & TYPE_C_CC_ENT_ATTACH_SRC_INTR) {
		typec_cc_state(hba);

		/*Move trigger host/device driver to pd_task, if support PD*/
		if (hba->mode == 1) {
			hba->data_role = PD_ROLE_DFP;
			schedule_delayed_work(&hba->usb_work, msecs_to_jiffies(100));
		}

		if (!pd_is_power_swapping(hba))
			typec_disable_lowq(hba, "ATTACH.SRC");

		/* At Attached.SRC, continue checking Vbus is vSafe0V or not?
		 * If Vbus stays at 0v, turn on Vbus to vSafe5V.
		 */
		if (hba->task_state != PD_STATE_SNK_SWAP_STANDBY)
			queue_work(hba->pd_wq, &hba->wait_vbus_off_then_drive_attached_src);
	}

	/*transition from Attached.SRC to TryWait.SNK*/
	if (cc_is0 & TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR) {
		typec_drive_vbus(hba, 0);
		/* At TryWait.SNK, continue checking vSafe5V is presented or not?
		 * If Vbus detected, set TYPE_C_SW_VBUS_PRESENT@TYPE_C_CC_SW_CTRL(0xA) as 1
		 * to notify MAC layer.
		 */
		queue_work(hba->pd_wq, &hba->wait_vbus_on_try_wait_snk);
	}

	if (cc_is2 & TYPE_C_INTR_SRC_ADVERTISE) {
		if (cc_is2 != (0x1 << hba->src_rp))
			typec_cc_state(hba);
	}

	if (cc_is0 & TYPE_C_CC_ENT_AUDIO_ACC_INTR)
		dev_err(hba->dev, "%s Audio Adapter Accessory Mode attached\n", __func__);

}

/**
 * typec_top_intr - main interrupt service routine
 * @irq: irq number
 * @__hba: pointer to adapter instance
 *
 * Returns IRQ_HANDLED - If interrupt is valid
 *		IRQ_NONE - If invalid interrupt
 */
static irqreturn_t typec_top_intr(int irq, void *__hba)
{
	uint16_t cc_is0, cc_is2;
	#if SUPPORT_PD
	uint16_t pd_is0, pd_is1;
	#endif
	uint16_t handled;
	irqreturn_t retval = IRQ_NONE;
	struct typec_hba *hba = __hba;

#ifdef PROFILING
	ktime_t start, end;
#endif

	mutex_lock(&hba->typec_lock);

#ifdef PROFILING
	start = ktime_get();
#endif

	/* TYPEC */
	cc_is0 = typec_readw(hba, TYPE_C_INTR_0);
	cc_is2 = typec_read8(hba, TYPE_C_INTR_2);

	typec_writew(hba, cc_is0, TYPE_C_INTR_0);
	typec_write8(hba, cc_is2, TYPE_C_INTR_2);

	cc_is0 = cc_is0 & typec_readw(hba, TYPE_C_INTR_EN_0);
	cc_is2 = cc_is2 & typec_read8(hba, TYPE_C_INTR_EN_2);

	if (cc_is0 | cc_is2)
		typec_intr(hba, cc_is0, cc_is2);
	handled = (cc_is0 | cc_is2);

#if SUPPORT_PD
	if (hba->mode == 2) {
		/* PD */
		pd_is0 = typec_readw(hba, PD_INTR_0);
		pd_is1 = typec_readw(hba, PD_INTR_1);
#if PD_SW_WORKAROUND1_2
		if (pd_is0 & PD_RX_RCV_MSG_INTR)
			typec_clear(hba, REG_PD_RX_RCV_MSG_INTR_EN, PD_INTR_EN_0);
		typec_writew(hba, (pd_is0 & ~PD_RX_RCV_MSG_INTR), PD_INTR_0);
#else
		/*typec_writew(hba, pd_is0, PD_INTR_0);*/
		typec_writew(hba, (pd_is0 & ~PD_RX_RCV_MSG_INTR), PD_INTR_0);
#endif
		typec_writew(hba, pd_is1, PD_INTR_1);

		if (pd_is0 | pd_is1 | (cc_is0 & PD_INTR_IS0_LISTEN))
			pd_intr(hba, pd_is0, pd_is1, cc_is0, cc_is2);
		handled |= (pd_is0 | pd_is1);
	}
#endif

	/*check if at least 1 interrupt has been served this time*/
	if (handled)
		retval = IRQ_HANDLED;

#ifdef PROFILING
	end = ktime_get();
	dev_err(hba->dev, "INTR took %lld us", ktime_to_ns(ktime_sub(end, start))/1000);
#endif

	mutex_unlock(&hba->typec_lock);

	if (cc_is0 || cc_is2)
		dev_err(hba->dev, "%s cc0=0x%X cc2=0x%X\n", __func__, cc_is0, cc_is2);

	return retval;
}

/**
 * typec_init - Driver initialization routine
 * @dev: pointer to device handle
 * @hba_handle: driver private handle
 * @mmio_base: base register address
 * @irq: Interrupt line of device
 * @id: device id
 * Returns 0 on success, non-zero value on failure
 */

static void typec_irq_work(struct work_struct *work)
{
	struct typec_hba *hba = container_of(work, struct typec_hba, irq_work);

	typec_top_intr(0, hba);
}

#if USE_AUXADC
void auxadc_detect_cableout_hanlder(void)
{
	struct typec_hba *hba = get_hba();
	uint16_t val;
	unsigned int vbus = 0;
	static unsigned int pre_vbus;

	val = typec_auxadc_get_value(hba, FLAGS_AUXADC_MIN);

	dev_err(hba->dev, "ADC VAL=%d\n", val);

	typec_disable_auxadc(hba, 1, 0);

	vbus = typec_vbus(hba);

	if ((val < SNK_VRPUSB_AUXADC_MIN_VAL) && ((vbus < (PD_VSAFE5V_LOW)) || (vbus < pre_vbus))) {
		if (hba->charger_det_notify)
			hba->charger_det_notify(0);
	} else {
		pre_vbus = vbus;
		typec_enable_auxadc(hba, 1, 0);
	}
}

void auxadc_min_hanlder(void)
{
	mutex_lock(&g_hba->typec_lock);
	typec_disable_auxadc_irq(g_hba);
	g_hba->auxadc_flags = FLAGS_AUXADC_MIN;
	typec_auxadc_set_event(g_hba);
	mutex_unlock(&g_hba->typec_lock);
}

void auxadc_max_hanlder(void)
{
	mutex_lock(&g_hba->typec_lock);
	typec_disable_auxadc_irq(g_hba);
	g_hba->auxadc_flags = FLAGS_AUXADC_MAX;
	typec_auxadc_set_event(g_hba);
	mutex_unlock(&g_hba->typec_lock);
}
#endif

void typec_hanlder(void)
{
	typec_top_intr(0, g_hba);
}

struct typec_hba *get_hba(void)
{
	if (!g_hba)
		g_hba = kzalloc(sizeof(struct typec_hba), GFP_KERNEL);

	return g_hba;
}
EXPORT_SYMBOL_GPL(get_hba);

int get_u32(struct device_node *np, const char *name, unsigned int *val)
{
	uint32_t tmp;
	int ret = -1;

	if (of_property_read_u32(np, name, &tmp) == 0) {
		*val = tmp;
		pr_err("%s get %s = %d\n", __func__, name, *val);
		ret = 0;
	} else {
		pr_err("%s get %s fail\n", __func__, name);
	}
	return ret;
}

void parse_dts(struct device_node *np, struct typec_hba *hba)
{
	uint32_t tmp;

	if (get_u32(np, "mode", &hba->mode) != 0)
		hba->mode = 1;

	if (of_property_read_u32(np, "support_role", &tmp) == 0) {
		hba->support_role = tmp;
		pr_err("%s get support_role = %d\n", __func__, hba->support_role);
	} else {
		hba->support_role = TYPEC_ROLE_DRP;
		pr_err("%s get support_role fail\n", __func__);
	}

	if (get_u32(np, "prefer_role", &hba->prefer_role) != 0)
		hba->prefer_role = 2;

	if (of_property_read_u32(np, "rp_val", &tmp) == 0) {
		hba->rp_val = tmp;
		pr_err("%s get rp_val = %d\n", __func__, hba->rp_val);
	} else {
		hba->rp_val = TYPEC_RP_DFT;
		pr_err("%s get rp_val fail\n", __func__);
	}

	if (get_u32(np, "cc_irq", &hba->cc_irq) != 0)
		hba->cc_irq = 0;

	if (get_u32(np, "pd_irq", &hba->pd_irq) != 0)
		hba->pd_irq = 0;

	if (get_u32(np, "vbus_on_polling", &hba->vbus_on_polling) != 0)
		hba->vbus_on_polling = 100;

	if (get_u32(np, "vbus_off_polling", &hba->vbus_off_polling) != 0)
		hba->vbus_off_polling = 100;

	if (get_u32(np, "discover_vmd", &hba->discover_vmd) != 0)
		hba->discover_vmd = 0;

	if (get_u32(np, "kpoc_delay", &hba->kpoc_delay) != 0)
		hba->kpoc_delay = 8*1000;

}

int typec_init(struct device *dev, struct typec_hba **hba_handle,
		void __iomem *mmio_base, unsigned int irq, int id)
{
	int err;
	struct typec_hba *hba = NULL;
	int _id = 0;

	/*check arguments*/
	if (!dev) {
		dev_err(dev,
		"Invalid memory reference for dev is NULL\n");
		err = -ENODEV;
		goto out_error;
	}

	/*initialize controller data*/
	if (!g_hba)
		g_hba = kzalloc(sizeof(struct typec_hba), GFP_KERNEL);
	hba = g_hba;
	hba->dev = dev;
	hba->mmio_base = mmio_base;
	hba->irq = irq;
	hba->id = id;


	dev_set_drvdata(dev, hba);

	parse_dts(dev->of_node, hba);

	if (hba->mode == 0)
		dev_err(hba->dev, "Disable Type-C & PD\n");
	else if (hba->mode == 1)
		dev_err(hba->dev, "Enable Type-C\n");
	else if (hba->mode == 2)
		dev_err(hba->dev, "Enable PD\n");

	mutex_init(&hba->ioctl_lock);
	mutex_init(&hba->typec_lock);

	hba->pd_wq = create_singlethread_workqueue("pd_wq");
	if (!hba->pd_wq)
		return -ENOMEM;

	INIT_WORK(&hba->wait_vbus_on_attach_wait_snk, typec_wait_vbus_on_attach_wait_snk);
	INIT_WORK(&hba->wait_vbus_on_try_wait_snk, typec_wait_vbus_on_try_wait_snk);
	INIT_WORK(&hba->wait_vbus_off_attached_snk, typec_wait_vbus_off_attached_snk);
	INIT_WORK(&hba->wait_vbus_off_then_drive_attached_src, typec_wait_vbus_off_then_drive_attached_src);
	INIT_WORK(&hba->wait_vsafe0v, typec_wait_vsafe0v);
	INIT_WORK(&hba->init_vbus_off, typec_init_vbus_off);

	#if USE_AUXADC
	INIT_WORK(&hba->auxadc_voltage_mon_attached_snk, typec_auxadc_voltage_mon_attached_snk);
	#endif

	if (hba->mode == 0)
		goto next;

	/*trigger by PMIC INTR*/
	INIT_WORK(&hba->irq_work, typec_irq_work);

	/*trigger usb device/host driver*/
	INIT_DELAYED_WORK(&hba->usb_work, trigger_driver);

#if USE_AUXADC
	/*mt6336_enable_interrupt(TYPE_C_L_MIN, "TYPE_C_L_MIN");*/
	/*mt6336_register_interrupt_callback(TYPE_C_L_MIN, auxadc_min_hanlder);*/
	mt6336_register_interrupt_callback(TYPE_C_L_MIN, auxadc_detect_cableout_hanlder);

	/*mt6336_enable_interrupt(TYPE_C_H_MAX, "TYPE_C_H_MAX");*/
	mt6336_register_interrupt_callback(TYPE_C_H_MAX, auxadc_max_hanlder);
#endif
	/*create character device for CLI*/
	/*err = typec_cdev_init(dev, hba, id);*/
	err = typec_cdev_init(dev, hba, _id);
	if (err) {
		dev_err(hba->dev, "char dev failed\n");
		goto out_error;
	}

#if !COMPLIANCE
	mutex_init(&hba->lowq_lock);
	hba->core_ctrl = mt6336_ctrl_get("mt6336_pd");
	atomic_set(&hba->lowq_cnt, 1);
	mt6336_ctrl_enable(hba->core_ctrl);
	dev_err(hba->dev, "Disable LowQ\n");
#endif
	dev_err(hba->dev, "SW RESET CC&PD\n");
	mt6336_set_flag_register_value(MT6336_RG_TYPE_C_CC_RST, 1);
	mt6336_set_flag_register_value(MT6336_RG_TYPE_C_PD_RST, 1);

	mt6336_set_flag_register_value(MT6336_RG_TYPE_C_CC_RST, 0);
	mt6336_set_flag_register_value(MT6336_RG_TYPE_C_PD_RST, 0);

	/*For bring-up, check the i2c communucation*/
	/* PD*/
	dev_err(hba->dev, "PD_TX_PARAMETER(16b)=0x%x Should be 0x732B\n",
		typec_readw(hba, PD_TX_PARAMETER));
	/* Type-c*/
	dev_err(hba->dev, "PERIODIC_MEAS_VAL(16b)=0x%x Should be 0x2ED\n",
		typec_readw(hba, TYPE_C_CC_VOL_PERIODIC_MEAS_VAL));

	typec_basic_settings(hba);
	pd_basic_settings(hba);

	/*initialize TYPEC*/
	typec_set_default_param(hba);

	dev_err(hba->dev, "TYPE_C_INTR_EN_0=0x%x\n",
		typec_readw(hba, TYPE_C_INTR_EN_0));

	dev_err(hba->dev, "TYPE_C_INTR_EN_2=0x%x\n",
		typec_read8(hba, TYPE_C_INTR_EN_2));

	hba->pd_rp_val = TYPEC_RP_15A;
	hba->dbg_lvl = TYPEC_DBG_LVL_2;
	hba->hr_auto_sent = 0;
	hba->vbus_en = 0;
	hba->vsafe_5v = PD_VSAFE5V_LOW;
	hba->task_state = PD_STATE_DISABLED;
	hba->is_kpoc = false;
	hba->is_boost = false;
	hba->wq_running = 0;
	hba->wq_cnt = 0;

#if USE_AUXADC
	init_completion(&hba->auxadc_event);
#endif

#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if (get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT
		|| get_boot_mode() == LOW_POWER_OFF_CHARGING_BOOT) {
		dev_err(hba->dev, "%s, in KPOC\n", __func__);
		hba->support_role = TYPEC_ROLE_SINK;
		hba->is_kpoc = true;
		queue_work(hba->pd_wq, &hba->init_vbus_off);
	}
#endif

#if SUPPORT_PD
	/*initialize PD*/
	if (hba->mode == 2)
		pd_init(hba);
#endif

	if (hba->is_kpoc) {
		hba->vbus_off_polling = hba->vbus_off_polling/2;
		typec_set(hba, REG_TYPE_C_ADC_EN, TYPE_C_CTRL);

		mt_ppm_sysboost_set_core_limit(BOOST_BY_USB_PD, 1, 4, 4);
		hba->is_boost = true;
	}

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	mt_dual_role_phy_init(hba);
	hba->dual_role_supported_modes = (hba->support_role == TYPEC_ROLE_SINK) ?
		DUAL_ROLE_SUPPORTED_MODES_UFP : DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
#endif

	/*initialization completes*/
	*hba_handle = hba;

#ifdef MT6336_E1
		/*INT_STATUS5 8th*/
#define TYPE_C_CC_IRQ_NUM (5*8+7)
#else
		/*INT_STATUS5 5th*/
#define TYPE_C_CC_IRQ_NUM (5*8+4)
#endif
		mt6336_register_interrupt_callback(TYPE_C_CC_IRQ_NUM, typec_hanlder);

#if SUPPORT_PD
#ifdef MT6336_E1
		/*INT_STATUS6 1st*/
#define TYPE_C_PD_IRQ_NUM (6*8)
#else
		/*INT_STATUS5 6th*/
#define TYPE_C_PD_IRQ_NUM (5*8+5)
#endif
		mt6336_register_interrupt_callback(TYPE_C_PD_IRQ_NUM, typec_hanlder);
#endif

	if (hba->mode > 0) {
		typec_int_enable(hba, TYPE_C_INTR_EN_0_MSK, TYPE_C_INTR_EN_2_MSK);

		mt6336_enable_interrupt(TYPE_C_CC_IRQ_NUM, "TYPE_C_CC_IRQ");

		if (hba->mode == 2)
			mt6336_enable_interrupt(TYPE_C_PD_IRQ_NUM, "TYPE_C_PD_IRQ");

		/*Prefer Role 0: SNK Only, 1: SRC Only, 2: DRP, 3: Try.SRC, 4: Try.SNK */
		typec_set_mode(hba, hba->support_role, hba->rp_val, ((hba->prefer_role == 3)?1:0));

		if (hba->is_kpoc)
			typec_enable(hba, 0);
		else
			typec_enable(hba, 1);
	} else {
		typec_enable(hba, 0);
	}

	typec_enable_lowq(hba, "typec_init");

next:
	return 0;

out_error:
	if (!hba)
		kfree(hba);

	return err;
}
EXPORT_SYMBOL_GPL(typec_init);

/**
 * typec_remove - de-allocate data structure memory
 * @hba - per adapter instance
 */
void typec_remove(struct typec_hba *hba)
{
	/* disable interrupts */
	typec_int_disable(hba, TYPE_C_INTR_EN_0_MSK, TYPE_C_INTR_EN_2_MSK);

	typec_cdev_remove(hba);

	kfree(hba);
}
EXPORT_SYMBOL_GPL(typec_remove);

/**
 * typec_suspend - suspend power management function
 * @hba: per adapter instance
 * @state: power state
 *
 * Returns 0
 */
int typec_suspend(struct typec_hba *hba, pm_message_t state)
{
	return 0;
}
EXPORT_SYMBOL_GPL(typec_suspend);

/**
 * typec_resume - resume power management function
 * @hba: per adapter instance
 *
 * Returns 0
 */
int typec_resume(struct typec_hba *hba)
{
	return 0;
}
EXPORT_SYMBOL_GPL(typec_resume);

int typec_runtime_suspend(struct typec_hba *hba)
{
	return 0;
}
EXPORT_SYMBOL(typec_runtime_suspend);

int typec_runtime_resume(struct typec_hba *hba)
{
	return 0;
}
EXPORT_SYMBOL(typec_runtime_resume);

int typec_runtime_idle(struct typec_hba *hba)
{
	return 0;
}
EXPORT_SYMBOL(typec_runtime_idle);

/**
 * typec_exit - Driver init routine
 */
static int __init typec_module_init(void)
{
	int err;

	err = typec_cdev_module_init();
	if (err)
		return err;

	err = usbc_pinctrl_init();
	if (err)
		return err;

	err = typec_pltfrm_init();
	if (err)
		goto err_handle;

#ifdef CONFIG_RT7207_ADAPTER
	err = mtk_direct_charge_vdm_init();
	if (err)
		goto err_handle;
#endif

	return 0;

err_handle:
	typec_cdev_module_exit();

	return err;
}
late_initcall(typec_module_init);

