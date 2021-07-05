/*
 * Copyright (C) 2015 MediaTek Inc.
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
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#include <mt-plat/mtk_boot_common.h>
#include "ccci_debug.h"
#include "ccci_config.h"
#include "ccci_modem.h"
#include "ccci_swtp.h"
#include "ccci_fsm.h"

const struct of_device_id swtp_of_match[] = {
	{ .compatible = SWTP_COMPATIBLE_DEVICE_ID, },
	{ .compatible = SWTP1_COMPATIBLE_DEVICE_ID,},
	{ .compatible = SWTP2_COMPATIBLE_DEVICE_ID, }, /*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp */
	{ .compatible = SWTP3_COMPATIBLE_DEVICE_ID,},  /*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp */
	{},
};
#define SWTP_MAX_SUPPORT_MD 1
struct swtp_t swtp_data[SWTP_MAX_SUPPORT_MD];
static const char rf_name[] = "RF_cable";
/*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp start*/
#define MAX_RETRY_CNT 30
/*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp end*/
static int swtp_send_tx_power(struct swtp_t *swtp)
{
	unsigned long flags;
	int power_mode, ret = 0;

	if (swtp == NULL) {
		CCCI_LEGACY_ERR_LOG(-1, SYS, "%s:swtp is null\n", __func__);
		return -1;
	}

	spin_lock_irqsave(&swtp->spinlock, flags);

	ret = exec_ccci_kern_func_by_md_id(swtp->md_id, ID_UPDATE_TX_POWER,
		(char *)&swtp->tx_power_mode, sizeof(swtp->tx_power_mode));
	power_mode = swtp->tx_power_mode;
	spin_unlock_irqrestore(&swtp->spinlock, flags);
/*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp start */
        CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"%s to MD%d,state=%d,ret=%d wttest\n",
			__func__, swtp->md_id + 1, power_mode, ret);
/*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp end*/
	if (ret != 0)
		CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"%s to MD%d,state=%d,ret=%d\n",
			__func__, swtp->md_id + 1, power_mode, ret);

	return ret;
}

static int swtp_switch_state(int irq, struct swtp_t *swtp)
{
	unsigned long flags;
	int i;
    CCCI_LEGACY_ERR_LOG(-1, SYS, "%s:test-2\n", __func__); /*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp */
	if (swtp == NULL) {
		CCCI_LEGACY_ERR_LOG(-1, SYS, "%s:data is null\n", __func__);
		return -1;
	}

	spin_lock_irqsave(&swtp->spinlock, flags);
	for (i = 0; i < MAX_PIN_NUM; i++) {
		if (swtp->irq[i] == irq)
			break;
	}
	if (i == MAX_PIN_NUM) {
		spin_unlock_irqrestore(&swtp->spinlock, flags);
		CCCI_LEGACY_ERR_LOG(-1, SYS,
			"%s:can't find match irq\n", __func__);
		return -1;
	}

	if (swtp->eint_type[i] == IRQ_TYPE_LEVEL_LOW) {
		irq_set_irq_type(swtp->irq[i], IRQ_TYPE_LEVEL_HIGH);
		swtp->eint_type[i] = IRQ_TYPE_LEVEL_HIGH;
	} else {
		irq_set_irq_type(swtp->irq[i], IRQ_TYPE_LEVEL_LOW);
		swtp->eint_type[i] = IRQ_TYPE_LEVEL_LOW;
	}

	if (swtp->gpio_state[i] == SWTP_EINT_PIN_PLUG_IN)
		swtp->gpio_state[i] = SWTP_EINT_PIN_PLUG_OUT;
	else
		swtp->gpio_state[i] = SWTP_EINT_PIN_PLUG_IN;

	swtp->tx_power_mode = SWTP_NO_TX_POWER;
         /*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp start*/
         //CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,"wttest1-swtp->tx_power_mode = %d\n", swtp->tx_power_mode);
         CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"wttest12-swtp-%s>>tx_power_mode = %d,swtp->gpio_state[0]=%d\n", __func__,swtp->tx_power_mode,swtp->gpio_state[0]);
         CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"wttest13-swtp-%s>>tx_power_mode = %d,swtp->gpio_state[1]=%d\n", __func__,swtp->tx_power_mode,swtp->gpio_state[1]);
         CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"wttest14-swtp-%s>>tx_power_mode = %d,swtp->gpio_state[2]=%d\n", __func__,swtp->tx_power_mode,swtp->gpio_state[2]);
        CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"wttest15-swtp-%s>>tx_power_mode = %d,swtp->gpio_state[3]=%d\n", __func__,swtp->tx_power_mode,swtp->gpio_state[3]);

       if ((swtp->gpio_state[0] == SWTP_EINT_PIN_PLUG_IN)&&(swtp->gpio_state[1] == SWTP_EINT_PIN_PLUG_IN)&&(swtp->gpio_state[2] == SWTP_EINT_PIN_PLUG_OUT)&&(swtp->gpio_state[3] == SWTP_EINT_PIN_PLUG_OUT))
       {
               swtp->tx_power_mode = SWTP_DO_TX_POWER;
               CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"wttest15-swtp-%s>>tx_power_mode =SWTP_DO_TX_POWER= %d\n", __func__,swtp->tx_power_mode);
        }
        else
        {
               swtp->tx_power_mode = SWTP_NO_TX_POWER;
                CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"wttest16-swtp-%s>>tx_power_mode =SWTP_NO_TX_POWER= %d\n", __func__,swtp->tx_power_mode);
        }
       /*
	for (i = 0; i < MAX_PIN_NUM; i++) {
        CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"wttest2-i = %d\n", i);

		if (swtp->gpio_state[i] == SWTP_EINT_PIN_PLUG_IN) {
			swtp->tx_power_mode = SWTP_DO_TX_POWER;
        CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"wttest3-swtp-%s>>tx_power_mode = %d,swtp->gpio_state[%d]=%d\n", __func__,swtp->tx_power_mode,i,swtp->gpio_state[i]);
			break;
		}
	}
	//MMI_STOPSHIP
	//For the MTK base line is truck line. It do not include MOT special feature. So this function could not be called.
	//Need to restore it after MTK Moto specail line is ready.
	inject_pin_status_event(swtp->tx_power_mode, rf_name);
        */
        spin_unlock_irqrestore(&swtp->spinlock, flags);
        //CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,"wttest4-swtp->tx_power_mode = %d\n", swtp->tx_power_mode);
        /*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp end*/
	return swtp->tx_power_mode;
}

static void swtp_send_tx_power_state(struct swtp_t *swtp)
{
	int ret = 0;

	if (!swtp) {
		CCCI_LEGACY_ERR_LOG(-1, SYS,
			"%s:swtp is null\n", __func__);
		return;
	}

	if (swtp->md_id == 0) {
		ret = swtp_send_tx_power(swtp);
		if (ret < 0) {
			CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
				"%s send tx power failed, ret=%d, schedule delayed work\n",
				__func__, ret);
			schedule_delayed_work(&swtp->delayed_work, 5 * HZ);
		}
	} else
		CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"%s:md is no support\n", __func__);

}

static irqreturn_t swtp_irq_handler(int irq, void *data)
{
	struct swtp_t *swtp = (struct swtp_t *)data;
	int ret = 0;

	ret = swtp_switch_state(irq, swtp);
	if (ret < 0) {
		CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"%s swtp_switch_state failed in irq, ret=%d\n",
			__func__, ret);
	} else
		swtp_send_tx_power_state(swtp);

	return IRQ_HANDLED;
}

static void swtp_tx_delayed_work(struct work_struct *work)
{
	struct swtp_t *swtp = container_of(to_delayed_work(work),
		struct swtp_t, delayed_work);
	int ret, retry_cnt = 0;

	while (retry_cnt < MAX_RETRY_CNT) {
		ret = swtp_send_tx_power(swtp);
		if (ret != 0) {
			msleep(2000);
			retry_cnt++;
		} else
			break;
	}
}

int swtp_md_tx_power_req_hdlr(int md_id, int data)
{
	struct swtp_t *swtp = NULL;

	if (md_id < 0 || md_id >= SWTP_MAX_SUPPORT_MD) {
		CCCI_LEGACY_ERR_LOG(md_id, SYS,
		"%s:md_id=%d not support\n",
		__func__, md_id);
		return -1;
	}

	swtp = &swtp_data[md_id];
	swtp_send_tx_power_state(swtp);

	return 0;
}

static void swtp_init_delayed_work(struct work_struct *work)
{
	struct swtp_t *swtp = container_of(to_delayed_work(work),
		struct swtp_t, init_delayed_work);
	int md_id;
	int i, ret = 0;
#ifdef CONFIG_MTK_EIC
	u32 ints[2] = { 0, 0 };
#else
	u32 ints[1] = { 0 };
#endif
	u32 ints1[2] = { 0, 0 };
	struct device_node *node = NULL;

/*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp start */
        char irq_name[12];
        int has_write = 0;
/*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp end */

	CCCI_NORMAL_LOG(-1, SYS, "%s start\n", __func__);
	CCCI_BOOTUP_LOG(-1, SYS, "%s start\n", __func__);

	md_id = swtp->md_id;

	if (md_id < 0 || md_id >= SWTP_MAX_SUPPORT_MD) {
		ret = -2;
		CCCI_LEGACY_ERR_LOG(-1, SYS,
			"%s: invalid md_id = %d\n", __func__, md_id);
		goto SWTP_INIT_END;
	}

	for (i = 0; i < MAX_PIN_NUM; i++)
		swtp_data[md_id].gpio_state[i] = SWTP_EINT_PIN_PLUG_OUT;

	for (i = 0; i < MAX_PIN_NUM; i++) {
				CCCI_LEGACY_ERR_LOG(md_id, SYS,
				"%s:read swtp number%d counts%d pin num\n",
				__func__, i, MAX_PIN_NUM);
		node = of_find_matching_node(NULL, &swtp_of_match[i]);
		if (node) {
			ret = of_property_read_u32_array(node, "debounce",
				ints, ARRAY_SIZE(ints));
			if (ret) {
				CCCI_LEGACY_ERR_LOG(md_id, SYS,
					"%s:swtp%d get debounce fail\n",
					__func__, i);
				break;
			}

			ret = of_property_read_u32_array(node, "interrupts",
				ints1, ARRAY_SIZE(ints1));
			if (ret) {
				CCCI_LEGACY_ERR_LOG(md_id, SYS,
					"%s:swtp%d get interrupts fail\n",
					__func__, i);
				break;
			}
#ifdef CONFIG_MTK_EIC /* for chips before mt6739 */
			swtp_data[md_id].gpiopin[i] = ints[0];
			swtp_data[md_id].setdebounce[i] = ints[1];
#else /* for mt6739,and chips after mt6739 */
			swtp_data[md_id].setdebounce[i] = ints[0];
			swtp_data[md_id].gpiopin[i] =
				of_get_named_gpio(node, "deb-gpios", 0);
#endif
			gpio_set_debounce(swtp_data[md_id].gpiopin[i],
				swtp_data[md_id].setdebounce[i]);
			swtp_data[md_id].eint_type[i] = ints1[1];
			swtp_data[md_id].irq[i] = irq_of_parse_and_map(node, 0);
/*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp start*/
            memset(irq_name, 0, sizeof(irq_name));
            has_write = snprintf(irq_name, 12, "swtp%d-eint", i);
            if (has_write <= 0 || has_write > 12) {
                CCCI_LEGACY_ERR_LOG(md_id, SYS,
                    "get swtp%d-eint fail\n",
                    i);
                break;
            }
			ret = request_irq(swtp_data[md_id].irq[i],
				swtp_irq_handler, IRQF_TRIGGER_NONE,
            irq_name, &swtp_data[md_id]);
/*ExtB EKSAIPAN-82 yangchanghui.wt 20210129 modify for swtp end */
			if (ret) {
				CCCI_LEGACY_ERR_LOG(md_id, SYS,
					"swtp%d-eint IRQ LINE NOT AVAILABLE\n",
					i);
				break;
			}
		} else {
			CCCI_LEGACY_ERR_LOG(md_id, SYS,
				"%s:can't find swtp%d compatible node\n",
				__func__, i);
			ret = -3;
		}
	}
	register_ccci_sys_call_back(md_id, MD_SW_MD1_TX_POWER_REQ,
		swtp_md_tx_power_req_hdlr);

SWTP_INIT_END:
	CCCI_BOOTUP_LOG(md_id, SYS, "%s end: ret = %d\n", __func__, ret);
	CCCI_NORMAL_LOG(md_id, SYS, "%s end: ret = %d\n", __func__, ret);

	return;
}

int swtp_init(int md_id)
{
	/* parameter check */
	if (md_id < 0 || md_id >= SWTP_MAX_SUPPORT_MD) {
		CCCI_LEGACY_ERR_LOG(-1, SYS,
			"%s: invalid md_id = %d\n", __func__, md_id);
		return -1;
	}
	/* init woke setting */
	swtp_data[md_id].md_id = md_id;

	INIT_DELAYED_WORK(&swtp_data[md_id].init_delayed_work,
		swtp_init_delayed_work);
	/* tx work setting */
	INIT_DELAYED_WORK(&swtp_data[md_id].delayed_work,
		swtp_tx_delayed_work);
	swtp_data[md_id].tx_power_mode = SWTP_NO_TX_POWER;

	spin_lock_init(&swtp_data[md_id].spinlock);

	/* schedule init work */
	schedule_delayed_work(&swtp_data[md_id].init_delayed_work, HZ);

	CCCI_BOOTUP_LOG(md_id, SYS, "%s end, init_delayed_work scheduled\n",
		__func__);
	return 0;
}
