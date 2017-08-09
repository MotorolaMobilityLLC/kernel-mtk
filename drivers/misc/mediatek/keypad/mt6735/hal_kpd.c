#include <mach/hal_pub_kpd.h>
#include <mach/hal_priv_kpd.h>
#include <mt-plat/kpd.h>
#include <mt-plat/aee.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#ifdef CONFIG_MTK_TC1_FM_AT_SUSPEND
#include <mt_soc_afe_control.h>
#endif
#define KPD_DEBUG	KPD_YES

#define KPD_SAY		"kpd: "
#if KPD_DEBUG
#define kpd_print(fmt, arg...)	pr_err(KPD_SAY fmt, ##arg)
#define kpd_info(fmt, arg...)	pr_warn(KPD_SAY fmt, ##arg)
#else
#define kpd_print(fmt, arg...)	do {} while (0)
#define kpd_info(fmt, arg...)	do {} while (0)
#endif

#if KPD_PWRKEY_USE_EINT
static u8 kpd_pwrkey_state = !KPD_PWRKEY_POLARITY;
#endif

static int kpd_show_hw_keycode = 1;
#ifndef EVB_PLATFORM
/*static int kpd_enable_lprst = 1;*/
#endif
static u16 kpd_keymap_state[KPD_NUM_MEMS] = {
	0xffff, 0xffff, 0xffff, 0xffff, 0x00ff
};

static bool kpd_sb_enable;

#ifdef CONFIG_MTK_SMARTBOOK_SUPPORT
static void sb_kpd_release_keys(struct input_dev *dev)
{
	int code;

	for (code = 0; code <= KEY_MAX; code++) {
		if (test_bit(code, dev->keybit)) {
			kpd_print("report release event for sb plug in! keycode:%d\n", code);
			input_report_key(dev, code, 0);
			input_sync(dev);
		}
	}
}

void sb_kpd_enable(void)
{
	kpd_sb_enable = true;
	kpd_print("sb_kpd_enable performed!\n");
	mt_reg_sync_writew(0x0, KP_EN);
	sb_kpd_release_keys(kpd_input_dev);
}

void sb_kpd_disable(void)
{
	kpd_sb_enable = false;
	kpd_print("sb_kpd_disable performed!\n");
	mt_reg_sync_writew(0x1, KP_EN);
}
#else
void sb_kpd_enable(void)
{
	kpd_print("sb_kpd_enable empty function for HAL!\n");
}

void sb_kpd_disable(void)
{
	kpd_print("sb_kpd_disable empty function for HAL!\n");
}
#endif

#ifndef EVB_PLATFORM
static void enable_kpd(int enable)
{
	if (enable == 1) {
		mt_reg_sync_writew((u16) (enable), KP_EN);
		kpd_print("KEYPAD is enabled\n");
	} else if (enable == 0) {
		mt_reg_sync_writew((u16) (enable), KP_EN);
		kpd_print("KEYPAD is disabled\n");
	}
}
#endif

void kpd_slide_qwerty_init(void)
{
#if KPD_HAS_SLIDE_QWERTY
	bool evdev_flag = false;
	bool power_op = false;
	struct input_handler *handler;
	struct input_handle *handle;

	handle = rcu_dereference(dev->grab);
	if (handle) {
		handler = handle->handler;
		if (strcmp(handler->name, "evdev") == 0)
			return -1;
	} else {
		list_for_each_entry_rcu(handle, &dev->h_list, d_node) {
			handler = handle->handler;
			if (strcmp(handler->name, "evdev") == 0) {
				evdev_flag = true;
				break;
			}
		}
		if (evdev_flag == false)
			return -1;
	}

	power_op = powerOn_slidePin_interface();
	if (!power_op)
		kpd_print(KPD_SAY "Qwerty slide pin interface power on fail\n");
	else
		kpd_print("Qwerty slide pin interface power on success\n");

	mt_eint_set_sens(KPD_SLIDE_EINT, KPD_SLIDE_SENSITIVE);
	mt_eint_set_hw_debounce(KPD_SLIDE_EINT, KPD_SLIDE_DEBOUNCE);
	mt_eint_registration(KPD_SLIDE_EINT, true, KPD_SLIDE_POLARITY, kpd_slide_eint_handler, false);

	power_op = powerOff_slidePin_interface();
	if (!power_op)
		kpd_print(KPD_SAY "Qwerty slide pin interface power off fail\n");
	else
		kpd_print("Qwerty slide pin interface power off success\n");
#endif
}

/************************************************************/
/**************************************/
#if defined(CONFIG_MTK_LEGACY)	/*This not need now */
#ifdef CONFIG_MTK_LDVT
void mtk_kpd_gpios_get(unsigned int ROW_REG[], unsigned int COL_REG[], unsigned int GPIO_MODE[])
{
	int i;

	for (i = 0; i < 3; i++) {
		ROW_REG[i] = 0;
		COL_REG[i] = 0;
		GPIO_MODE[i] = 0;
	}
#ifdef GPIO_KPD_KROW0_PIN
	ROW_REG[0] = GPIO_KPD_KROW0_PIN;
	GPIO_MODE[0] |= GPIO_KPD_KROW0_PIN_M_KROW;
#endif

#ifdef GPIO_KPD_KROW1_PIN
	ROW_REG[1] = GPIO_KPD_KROW1_PIN;
	GPIO_MODE[1] |= GPIO_KPD_KROW1_PIN_M_KROW;
#endif

#ifdef GPIO_KPD_KROW2_PIN
	ROW_REG[2] = GPIO_KPD_KROW2_PIN;
	GPIO_MODE[2] |= GPIO_KPD_KROW2_PIN_M_KROW;
#endif

#ifdef GPIO_KPD_KCOL0_PIN
	COL_REG[0] = GPIO_KPD_KCOL0_PIN;
	GPIO_MODE[0] |= (GPIO_KPD_KCOL0_PIN_M_KCOL << 4);
#endif

#ifdef GPIO_KPD_KCOL1_PIN
	COL_REG[1] = GPIO_KPD_KCOL1_PIN;
	GPIO_MODE[1] |= (GPIO_KPD_KCOL1_PIN_M_KCOL << 4);
#endif

#ifdef GPIO_KPD_KCOL2_PIN
	COL_REG[2] = GPIO_KPD_KCOL2_PIN;
	GPIO_MODE[2] |= (GPIO_KPD_KCOL2_PIN_M_KCOL << 4);
#endif
}

void mtk_kpd_gpio_set(void)
{
	unsigned int ROW_REG[3];
	unsigned int COL_REG[3];
	unsigned int GPIO_MODE[3];
	int i;

	kpd_print("Enter mtk_kpd_gpio_set!\n");
	mtk_kpd_gpios_get(ROW_REG, COL_REG, GPIO_MODE);

	for (i = 0; i < 3; i++) {
		if (COL_REG[i] != 0) {
			/* KCOL: GPIO INPUT + PULL ENABLE + PULL UP */
			mt_set_gpio_mode(COL_REG[i], ((GPIO_MODE[i] >> 4) & 0x0f));
			mt_set_gpio_dir(COL_REG[i], 0);
			mt_set_gpio_pull_enable(COL_REG[i], 1);
			mt_set_gpio_pull_select(COL_REG[i], 1);
		}

		if (ROW_REG[i] != 0) {
			/* KROW: GPIO output + pull disable + pull down */
			mt_set_gpio_mode(ROW_REG[i], (GPIO_MODE[i] & 0x0f));
			mt_set_gpio_dir(ROW_REG[i], 1);
			mt_set_gpio_pull_enable(ROW_REG[i], 0);
			mt_set_gpio_pull_select(ROW_REG[i], 0);
		}
	}
}
#endif
#endif
void kpd_ldvt_test_init(void)
{
#if defined(CONFIG_MTK_LEGACY)	/*This not need now */
#ifdef CONFIG_MTK_LDVT
	u16 temp_reg = 0;

	/* set kpd GPIO to kpd mode */
	mtk_kpd_gpio_set();

	temp_reg = readw(KP_SEL);
#if !defined(CONFIG_MTK_LEGACY)
	if (kpd_dts_data.kpd_use_extend_type) {
		/* select specific cols for double keypad */
#ifndef GPIO_KPD_KCOL0_PIN
		temp_reg &= ~(KP_COL0_SEL);
#endif

#ifndef GPIO_KPD_KCOL1_PIN
		temp_reg &= ~(KP_COL1_SEL);
#endif

#ifndef GPIO_KPD_KCOL2_PIN
		temp_reg &= ~(KP_COL2_SEL);
#endif

		temp_reg |= 0x1;
	} else {
		temp_reg &= ~(0x1);
	}
#else
#if KPD_USE_EXTEND_TYPE
	/* select specific cols for double keypad */
#ifndef GPIO_KPD_KCOL0_PIN
	temp_reg &= ~(KP_COL0_SEL);
#endif

#ifndef GPIO_KPD_KCOL1_PIN
	temp_reg &= ~(KP_COL1_SEL);
#endif

#ifndef GPIO_KPD_KCOL2_PIN
	temp_reg &= ~(KP_COL2_SEL);
#endif

	temp_reg |= 0x1;

#else
	temp_reg &= ~(0x1);
#endif
#endif
	/* set kpd enable and sel register */
	mt_reg_sync_writew(temp_reg, KP_SEL);
	mt_reg_sync_writew(0x1, KP_EN);
#endif
#endif
}

/*******************************kpd factory mode auto test *************************************/
/*
static void mtk_kpd_get_gpio_col(unsigned int COL_REG[])
{
	int i;
	for(i = 0; i< 3; i++)
	{
		COL_REG[i] = 0;
	}
	kpd_print("Enter mtk_kpd_get_gpio_col!\n");

	#ifdef GPIO_KPD_KCOL0_PIN
		kpd_print("checking GPIO_KPD_KCOL0_PIN!\n");
		COL_REG[0] = GPIO_KPD_KCOL0_PIN;
	#endif

	#ifdef GPIO_KPD_KCOL1_PIN
		kpd_print("checking GPIO_KPD_KCOL1_PIN!\n");
		COL_REG[1] = GPIO_KPD_KCOL1_PIN;
	#endif

	#ifdef GPIO_KPD_KCOL2_PIN
		kpd_print("checking GPIO_KPD_KCOL2_PIN!\n");
		COL_REG[2] = GPIO_KPD_KCOL2_PIN;
	#endif
}
*/

void kpd_get_keymap_state(u16 state[])
{
	state[0] = *(volatile u16 *)KP_MEM1;
	state[1] = *(volatile u16 *)KP_MEM2;
	state[2] = *(volatile u16 *)KP_MEM3;
	state[3] = *(volatile u16 *)KP_MEM4;
	state[4] = *(volatile u16 *)KP_MEM5;
	kpd_print(KPD_SAY "register = %x %x %x %x %x\n", state[0], state[1], state[2], state[3], state[4]);

}

static void kpd_factory_mode_handler(void)
{
	int i, j;
	bool pressed;
	u16 new_state[KPD_NUM_MEMS], change, mask;
	u16 hw_keycode, linux_keycode;

	for (i = 0; i < KPD_NUM_MEMS - 1; i++)
		kpd_keymap_state[i] = 0xffff;
	if (!kpd_dts_data.kpd_use_extend_type)
		kpd_keymap_state[KPD_NUM_MEMS - 1] = 0x00ff;
	else
		kpd_keymap_state[KPD_NUM_MEMS - 1] = 0xffff;

	kpd_get_keymap_state(new_state);

	for (i = 0; i < KPD_NUM_MEMS; i++) {
		change = new_state[i] ^ kpd_keymap_state[i];
		if (!change)
			continue;

		for (j = 0; j < 16; j++) {
			mask = 1U << j;
			if (!(change & mask))
				continue;

			hw_keycode = (i << 4) + j;
			/* bit is 1: not pressed, 0: pressed */
			pressed = !(new_state[i] & mask);
			if (kpd_show_hw_keycode) {
				kpd_print(KPD_SAY "(%s) factory_mode HW keycode = %u\n",
				       pressed ? "pressed" : "released", hw_keycode);
			}
			BUG_ON(hw_keycode >= KPD_NUM_KEYS);
			linux_keycode = kpd_dts_data.kpd_hw_init_map[hw_keycode];
			if (unlikely(linux_keycode == 0)) {
				kpd_print("Linux keycode = 0\n");
				continue;
			}
			input_report_key(kpd_input_dev, linux_keycode, pressed);
			input_sync(kpd_input_dev);
			kpd_print("factory_mode report Linux keycode = %u\n", linux_keycode);
		}
	}

	memcpy(kpd_keymap_state, new_state, sizeof(new_state));
	kpd_print("save new keymap state\n");
}

/********************************************************************/
void kpd_auto_test_for_factorymode(void)
{
/*
	unsigned int COL_REG[8];
	int i;
	int time = 500;
*/
	kpd_print("Enter kpd_auto_test_for_factorymode!\n");

	mdelay(1000);

	kpd_factory_mode_handler();
	kpd_print("begin kpd_auto_test_for_factorymode!\n");
	if (pmic_get_register_value(PMIC_PWRKEY_DEB) == 1) {
		kpd_print("power key release\n");
		/*kpd_pwrkey_pmic_handler(1);*/
		/*mdelay(time);*/
		/*kpd_pwrkey_pmic_handler(0);}*/
	} else {
		kpd_print("power key press\n");
		kpd_pwrkey_pmic_handler(1);
		/*mdelay(time);*/
		/*kpd_pwrkey_pmic_handler(0);*/
	}

#ifdef KPD_PMIC_RSTKEY_MAP
	if (pmic_get_register_value(PMIC_HOMEKEY_DEB) == 1) {
		/*kpd_print("home key release\n");*/
		/*kpd_pmic_rstkey_handler(1);*/
		/*mdelay(time);*/
		/*kpd_pmic_rstkey_handler(0);*/
	} else {
		kpd_print("home key press\n");
		kpd_pmic_rstkey_handler(1);
		/*mdelay(time);*/
		/*kpd_pmic_rstkey_handler(0);*/
	}
#endif
}

/********************************************************************/
void long_press_reboot_function_setting(void)
{
/*ZH CHEN*/
#if 0
#ifndef EVB_PLATFORM
	if (kpd_enable_lprst && get_boot_mode() == NORMAL_BOOT) {
		kpd_info("Normal Boot long press reboot selection\n");
#ifdef KPD_PMIC_LPRST_TD
		kpd_info("Enable normal mode LPRST\n");
#ifdef ONEKEY_REBOOT_NORMAL_MODE
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_EN, 0x01);
		pmic_set_register_value(PMIC_RG_HOMEKEY_RST_EN, 0x00);
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_TD, KPD_PMIC_LPRST_TD);
#endif

#ifdef TWOKEY_REBOOT_NORMAL_MODE
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_EN, 0x01);
		pmic_set_register_value(PMIC_RG_HOMEKEY_RST_EN, 0x01);
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_TD, KPD_PMIC_LPRST_TD);
#endif
#else
		kpd_info("disable normal mode LPRST\n");
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_EN, 0x00);
		pmic_set_register_value(PMIC_RG_HOMEKEY_RST_EN, 0x00);

#endif
	} else {
		kpd_info("Other Boot Mode long press reboot selection\n");
#ifdef KPD_PMIC_LPRST_TD
		kpd_info("Enable other mode LPRST\n");
#ifdef ONEKEY_REBOOT_OTHER_MODE
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_EN, 0x01);
		pmic_set_register_value(PMIC_RG_HOMEKEY_RST_EN, 0x00);
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_TD, KPD_PMIC_LPRST_TD);
#endif

#ifdef TWOKEY_REBOOT_OTHER_MODE
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_EN, 0x01);
		pmic_set_register_value(PMIC_RG_HOMEKEY_RST_EN, 0x01);
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_TD, KPD_PMIC_LPRST_TD);
#endif
#else
		kpd_info("disable other mode LPRST\n");
		pmic_set_register_value(PMIC_RG_PWRKEY_RST_EN, 0x00);
		pmic_set_register_value(PMIC_RG_HOMEKEY_RST_EN, 0x00);
#endif
	}
#else
	pmic_set_register_value(PMIC_RG_PWRKEY_RST_EN, 0x00);
	pmic_set_register_value(PMIC_RG_HOMEKEY_RST_EN, 0x00);
#endif
#endif
}

/********************************************************************/
void kpd_wakeup_src_setting(int enable)
{
#ifndef EVB_PLATFORM
#ifdef CONFIG_MTK_TC1_FM_AT_SUSPEND
	int is_fm_radio_playing = 0;

	/* If FM is playing, keep keypad as wakeup source */
	if (ConditionEnterSuspend() == true)
		is_fm_radio_playing = 0;
	else
		is_fm_radio_playing = 1;

	if (is_fm_radio_playing == 0) {
		if (enable == 1) {
			kpd_print("enable kpd work!\n");
			enable_kpd(1);
		} else {
			kpd_print("disable kpd work!\n");
			enable_kpd(0);
		}
	}
#else
	if (enable == 1) {
		kpd_print("enable kpd work!\n");
		enable_kpd(1);
	} else {
		kpd_print("disable kpd work!\n");
		enable_kpd(0);
	}
#endif
#endif
}

/********************************************************************/
void kpd_init_keymap(u16 keymap[])
{
	int i = 0;

	if (kpd_dts_data.kpd_use_extend_type)
		kpd_keymap_state[4] = 0xffff;
	for (i = 0; i < KPD_NUM_KEYS; i++) {
		keymap[i] = kpd_dts_data.kpd_hw_init_map[i];
		/*kpd_print(KPD_SAY "keymap[%d] = %d\n", i,keymap[i]);*/
	}
}

void kpd_init_keymap_state(u16 keymap_state[])
{
	int i = 0;

	for (i = 0; i < KPD_NUM_MEMS; i++)
		keymap_state[i] = kpd_keymap_state[i];
	kpd_info("init_keymap_state done: %x %x %x %x %x!\n", keymap_state[0], keymap_state[1], keymap_state[2],
		 keymap_state[3], keymap_state[4]);
}

/********************************************************************/

void kpd_set_debounce(u16 val)
{
	mt_reg_sync_writew((u16) (val & KPD_DEBOUNCE_MASK), KP_DEBOUNCE);
}

/********************************************************************/
void kpd_pmic_rstkey_hal(unsigned long pressed)
{
	if (kpd_dts_data.kpd_sw_rstkey != 0) {
		if (!kpd_sb_enable) {
			input_report_key(kpd_input_dev, kpd_dts_data.kpd_sw_rstkey, pressed);
			input_sync(kpd_input_dev);
			if (kpd_show_hw_keycode) {
				kpd_print(KPD_SAY "(%s) HW keycode =%d using PMIC\n",
				       pressed ? "pressed" : "released", kpd_dts_data.kpd_sw_rstkey);
			}
		}
	}
}

void kpd_pmic_pwrkey_hal(unsigned long pressed)
{
#if KPD_PWRKEY_USE_PMIC
	if (!kpd_sb_enable) {
		input_report_key(kpd_input_dev, kpd_dts_data.kpd_sw_pwrkey, pressed);
		input_sync(kpd_input_dev);
		if (kpd_show_hw_keycode) {
			kpd_print(KPD_SAY "(%s) HW keycode =%d using PMIC\n",
			       pressed ? "pressed" : "released", kpd_dts_data.kpd_sw_pwrkey);
		}
		/*ZH CHEN*/
		/*aee_powerkey_notify_press(pressed);*/
	}
#endif
}

/***********************************************************************/
void kpd_pwrkey_handler_hal(unsigned long data)
{
#if KPD_PWRKEY_USE_EINT
	bool pressed;
	u8 old_state = kpd_pwrkey_state;

	kpd_pwrkey_state = !kpd_pwrkey_state;
	pressed = (kpd_pwrkey_state == !!KPD_PWRKEY_POLARITY);
	if (kpd_show_hw_keycode)
		kpd_print(KPD_SAY "(%s) HW keycode = using EINT\n", pressed ? "pressed" : "released");
	kpd_backlight_handler(pressed, kpd_dts_data.kpd_sw_pwrkey);
	input_report_key(kpd_input_dev, kpd_dts_data.kpd_sw_pwrkey, pressed);
	kpd_print("report Linux keycode = %u\n", kpd_dts_data.kpd_sw_pwrkey);
	input_sync(kpd_input_dev);

	/* for detecting the return to old_state */
	mt_eint_set_polarity(KPD_PWRKEY_EINT, old_state);
	mt_eint_unmask(KPD_PWRKEY_EINT);
#endif
}

/***********************************************************************/
void mt_eint_register(void)
{
#if KPD_PWRKEY_USE_EINT
	mt_eint_set_sens(KPD_PWRKEY_EINT, KPD_PWRKEY_SENSITIVE);
	mt_eint_set_hw_debounce(KPD_PWRKEY_EINT, KPD_PWRKEY_DEBOUNCE);
	mt_eint_registration(KPD_PWRKEY_EINT, true, KPD_PWRKEY_POLARITY, kpd_pwrkey_eint_handler, false);
#endif
}

/************************************************************************/
