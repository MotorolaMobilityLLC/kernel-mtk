// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2015 MediaTek Inc.
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
#if defined(CONFIG_MOTO_LYRIQ_DRDI_RF_SET_INDEX)
extern int moto_drdi_rf_set_index;
#endif
/* must keep ARRAY_SIZE(swtp_of_match) = ARRAY_SIZE(irq_name) */
const struct of_device_id swtp_of_match[] = {
	{ .compatible = SWTP_COMPATIBLE_DEVICE_ID, },
	{ .compatible = SWTP1_COMPATIBLE_DEVICE_ID,},
	{ .compatible = SWTP2_COMPATIBLE_DEVICE_ID,},
	{ .compatible = SWTP3_COMPATIBLE_DEVICE_ID,},
	{ .compatible = SWTP4_COMPATIBLE_DEVICE_ID,},
	{},
};

static const char irq_name[][16] = {
	"swtp0-eint",
	"swtp1-eint",
	"swtp2-eint",
	"swtp3-eint",
	"swtp4-eint",
	"",
};

#define SWTP_MAX_SUPPORT_MD 1
struct swtp_t swtp_data[SWTP_MAX_SUPPORT_MD];
static const char rf_name[] = "RF_cable";
#define MAX_RETRY_CNT 30

#if defined(SWTP_GPIO_STATE_CUST) || defined(CONFIG_MOTO_GENEVA_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
static int swtp_tx_power_mode = SWTP_DO_TX_POWER;
static ssize_t swtp_gpio_state_show(struct class *class,
		struct class_attribute *attr,
		char *buf)
{
	int ret = 0;

	if (swtp_tx_power_mode == SWTP_NO_TX_POWER) {
		ret = 1;
	}

	return sprintf(buf, "%d\n", ret);
}

static CLASS_ATTR_RO(swtp_gpio_state);

static struct class swtp_class = {
	.name			= "swtp",
	.owner			= THIS_MODULE,
};
#endif

static int swtp_send_tx_power(struct swtp_t *swtp)
{
	unsigned long flags;
	int power_mode, ret = 0;
#ifdef CONFIG_MOTO_DISABLE_SWTP_FACTORY
        int factory_tx_power_mode = SWTP_NO_TX_POWER;
#endif
	if (swtp == NULL) {
		CCCI_LEGACY_ERR_LOG(-1, SYS, "%s:swtp is null\n", __func__);
		return -1;
	}

	spin_lock_irqsave(&swtp->spinlock, flags);
#ifdef CONFIG_MOTO_DISABLE_SWTP_FACTORY
	ret = exec_ccci_kern_func_by_md_id(swtp->md_id, ID_UPDATE_TX_POWER,
		(char *)&factory_tx_power_mode, sizeof(factory_tx_power_mode));
        #if defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
        CCCI_LEGACY_ERR_LOG(-1, SYS,"%s ret =%d\n",__func__, ret);
        #endif
	power_mode = factory_tx_power_mode;
        #if defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
        CCCI_LEGACY_ERR_LOG(-1, SYS,"%s factory_tx_power_mode =%d\n",__func__, factory_tx_power_mode);
        #endif
#else
	ret = exec_ccci_kern_func_by_md_id(swtp->md_id, ID_UPDATE_TX_POWER,
		(char *)&swtp->tx_power_mode, sizeof(swtp->tx_power_mode));
        #if defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
        CCCI_LEGACY_ERR_LOG(-1, SYS,"%s ret =%d\n",__func__, ret);
        #endif
	power_mode = swtp->tx_power_mode;
        #if defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
        CCCI_LEGACY_ERR_LOG(-1, SYS,"%s swtp->tx_power_mode =%d\n",__func__, swtp->tx_power_mode);
        #endif
#endif
	spin_unlock_irqrestore(&swtp->spinlock, flags);

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
       //EKELLIS-137 liangnengjie.wt, SWTP logic modify , 20210421, for RF swtp function fali, start
       //EKELLIS-890 liangnengjie.wt, SWTP logic modify , 20210529, for RF swtp function fali, start
       //+EKMAUI-7, zhouxin2.wt, RF Bring up swtp cfg, 20220402
       #if defined(CONFIG_MOTO_ELLIS_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_TONGA_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_MAUI_PROJECT_SWTP_SETING_APART)
       if (swtp->gpio_state[0] == SWTP_EINT_PIN_PLUG_IN) {
           swtp->tx_power_mode = SWTP_NO_TX_POWER;
       } else {
           swtp->tx_power_mode = SWTP_DO_TX_POWER;
       }
       CCCI_LEGACY_ERR_LOG(-1, SYS,"%s:swtp_switch_state : %d\n", __func__, swtp->tx_power_mode);
       //EKELLIS-890 liangnengjie.wt, SWTP logic modify , 20210529, for RF swtp function fali, start
	// modify by wt.longyili for swtp start
	#elif defined(CONFIG_MOTO_GENEVA_PROJECT_SWTP_SETING_APART)
	if ((swtp->gpio_state[0] == SWTP_EINT_PIN_PLUG_OUT)&&(swtp->gpio_state[1] == SWTP_EINT_PIN_PLUG_OUT)&&(swtp->gpio_state[2] == SWTP_EINT_PIN_PLUG_OUT)) {
		swtp->tx_power_mode = SWTP_DO_TX_POWER;
		CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"--------SWTP_DO_TX_POWER----------%s>>tx_power_mode = %d,gpio_state:Ant0=%d, Ant1=%d, Ant3=%d\n",
			__func__, swtp->tx_power_mode, swtp->gpio_state[0], swtp->gpio_state[1], swtp->gpio_state[2]);
	} else {
		swtp->tx_power_mode = SWTP_NO_TX_POWER;
		CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"--------SWTP_NO_TX_POWER----------%s>>tx_power_mode = %d,gpio_state:Ant0=%d, Ant1=%d, Ant3=%d\n",
			__func__, swtp->tx_power_mode, swtp->gpio_state[0], swtp->gpio_state[1], swtp->gpio_state[2]);
	}
	// modify by wt.longyili for swtp end
        #elif defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
	// swtp logic for jpn sku(rf index 1,3,5)
        if (moto_drdi_rf_set_index == 1 || moto_drdi_rf_set_index == 3 || moto_drdi_rf_set_index == 5)
        {
                CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"LYRIQ JPN %s moto_drdi_rf_set_index = %d\n",
			__func__, moto_drdi_rf_set_index);
        if ((swtp->gpio_state[1] == SWTP_EINT_PIN_PLUG_OUT)&&(swtp->gpio_state[2] == SWTP_EINT_PIN_PLUG_OUT)&&(swtp->gpio_state[3] == SWTP_EINT_PIN_PLUG_OUT)) {
		swtp->tx_power_mode = SWTP_DO_TX_POWER;
		CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"--------LYRIQ JPN SWTP_DO_TX_POWER----------%s>>tx_power_mode = %d,gpio_state:gpio169-Ant2=%d, gpio170-Ant1=%d, gpio171-Ant0=%d\n",
			__func__, swtp->tx_power_mode, swtp->gpio_state[1], swtp->gpio_state[2],swtp->gpio_state[3]);
	}
        else {
		swtp->tx_power_mode = SWTP_NO_TX_POWER;
		CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
      	     "-------LYRIQ JPN SWTP_NO_TX_POWER----------%s>>tx_power_mode = %d,gpio_state:gpio169-Ant2=%d, gpio170-Ant1=%d, gpio171-Ant0=%d\n",
			__func__, swtp->tx_power_mode,swtp->gpio_state[1], swtp->gpio_state[2],swtp->gpio_state[3]);
	}

        }
	// swtp logic for row sku(rf index 0,2,4)
        else {
                CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"LYRIQ ROW %s moto_drdi_rf_set_index = %d\n",
			__func__, moto_drdi_rf_set_index);
	if ((swtp->gpio_state[0] == SWTP_EINT_PIN_PLUG_OUT)&&(swtp->gpio_state[1] == SWTP_EINT_PIN_PLUG_OUT)&&(swtp->gpio_state[2] == SWTP_EINT_PIN_PLUG_OUT)&&(swtp->gpio_state[3] == SWTP_EINT_PIN_PLUG_OUT)) {
		swtp->tx_power_mode = SWTP_DO_TX_POWER;
		CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
			"--------LYRIQ ROW SWTP_DO_TX_POWER----------%s>>tx_power_mode = %d,gpio_state:gpio168-ANT8=%d, gpio169-Ant2=%d, gpio170-Ant1=%d, gpio171-Ant0=%d\n",
			__func__, swtp->tx_power_mode, swtp->gpio_state[0], swtp->gpio_state[1], swtp->gpio_state[2],swtp->gpio_state[3]);
	}
        else {
		swtp->tx_power_mode = SWTP_NO_TX_POWER;
		CCCI_LEGACY_ERR_LOG(swtp->md_id, SYS,
      	     "-------LYRIQ ROW SWTP_NO_TX_POWER----------%s>>tx_power_mode = %d,gpio_state:gpio168-ANT8=%d, gpio169-Ant2=%d, gpio170-Ant1=%d, gpio171-Ant0=%d\n",
			__func__, swtp->tx_power_mode, swtp->gpio_state[0], swtp->gpio_state[1], swtp->gpio_state[2],swtp->gpio_state[3]);
	}
        }

	#else
	for (i = 0; i < MAX_PIN_NUM; i++) {
		if (swtp->gpio_state[i] == SWTP_EINT_PIN_PLUG_IN) {
			swtp->tx_power_mode = SWTP_DO_TX_POWER;
			break;
		}
	}
	#endif
	//EKELLIS-137 liangnengjie.wt, SWTP logic modify , 20210421, for RF swtp function fali, end

       #if defined(CONFIG_MOTO_ELLIS_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_TONGA_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_MAUI_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_GENEVA_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
        inject_pin_status_event(swtp->tx_power_mode, rf_name);
       #else
	inject_pin_status_event(swtp->curr_mode, rf_name);
       #endif

	#if defined(SWTP_GPIO_STATE_CUST) || defined(CONFIG_MOTO_GENEVA_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
	swtp_tx_power_mode = swtp->tx_power_mode;
	#endif

	spin_unlock_irqrestore(&swtp->spinlock, flags);

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

	CCCI_NORMAL_LOG(-1, SYS, "%s start\n", __func__);
	CCCI_BOOTUP_LOG(-1, SYS, "%s start\n", __func__);

	md_id = swtp->md_id;

	if (md_id < 0 || md_id >= SWTP_MAX_SUPPORT_MD) {
		ret = -2;
		CCCI_LEGACY_ERR_LOG(-1, SYS,
			"%s: invalid md_id = %d\n", __func__, md_id);
		goto SWTP_INIT_END;
	}

#if defined(SWTP_GPIO_STATE_CUST) || defined(CONFIG_MOTO_GENEVA_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
	ret = class_register(&swtp_class);

	ret = class_create_file(&swtp_class, &class_attr_swtp_gpio_state);
#endif

	if (ARRAY_SIZE(swtp_of_match) != ARRAY_SIZE(irq_name) ||
		ARRAY_SIZE(swtp_of_match) > MAX_PIN_NUM + 1 ||
		ARRAY_SIZE(irq_name) > MAX_PIN_NUM + 1) {
		ret = -3;
		CCCI_LEGACY_ERR_LOG(-1, SYS,
			"%s: invalid array count = %lu(of_match), %lu(irq_name)\n",
			__func__, ARRAY_SIZE(swtp_of_match),
			ARRAY_SIZE(irq_name));
		goto SWTP_INIT_END;
	}

	for (i = 0; i < MAX_PIN_NUM; i++)
		swtp_data[md_id].gpio_state[i] = SWTP_EINT_PIN_PLUG_OUT;

	for (i = 0; i < MAX_PIN_NUM; i++) {
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

			ret = request_irq(swtp_data[md_id].irq[i],
				swtp_irq_handler, IRQF_TRIGGER_NONE,
				irq_name[i], &swtp_data[md_id]);
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
			ret = -4;
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

	//EKELLIS-890 liangnengjie.wt, SWTP logic modify , 20210529, for RF swtp function fali, start
        //+EKMAUI-7, zhouxin2.wt, RF Bring up swtp cfg, 20220402
	#if defined(CONFIG_MOTO_ELLIS_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_TONGA_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_MAUI_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_GENEVA_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
	swtp_data[md_id].tx_power_mode = SWTP_DO_TX_POWER;
	#else
	swtp_data[md_id].tx_power_mode = SWTP_NO_TX_POWER;
	#endif
    #ifdef CONFIG_MOTO_DISABLE_SWTP_FACTORY
    swtp_data[md_id].tx_power_mode = SWTP_NO_TX_POWER;
    #endif
	//EKELLIS-890 liangnengjie.wt, SWTP logic modify , 20210529, for RF swtp function fali, end

	#if defined(SWTP_GPIO_STATE_CUST) || defined(CONFIG_MOTO_GENEVA_PROJECT_SWTP_SETING_APART) || defined(CONFIG_MOTO_LYRIQ_PROJECT_SWTP_SETING_APART)
	swtp_tx_power_mode = swtp_data[md_id].tx_power_mode;
	#endif

	spin_lock_init(&swtp_data[md_id].spinlock);

	/* schedule init work */
	schedule_delayed_work(&swtp_data[md_id].init_delayed_work, HZ);

	CCCI_BOOTUP_LOG(md_id, SYS, "%s end, init_delayed_work scheduled\n",
		__func__);
	return 0;
}
