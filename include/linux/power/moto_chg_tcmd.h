#ifndef __MOTO_CHG_TCMD_H__
#define __MOTO_CHG_TCMD_H__

enum MOTO_CHG_TCMD_CLIENT_ID {
	MOTO_CHG_TCMD_CLIENT_CHG,
	MOTO_CHG_TCMD_CLIENT_BAT,
	MOTO_CHG_TCMD_CLIENT_AP_ADC,
	MOTO_CHG_TCMD_CLIENT_PM_ADC,
	MOTO_CHG_TCMD_CLIENT_WLS,
	MOTO_CHG_TCMD_ClIENT_MAX
};

struct moto_chg_tcmd_client {
	void *data;
	int client_id;

	bool factory_kill_disable;

#ifdef USE_LIST_HEAD
	struct list_head list;
#endif

	int (*reg_read)(void *input, unsigned char reg, unsigned char *val);//hex
	int (*reg_write)(void *input, unsigned char reg, unsigned char val);//hex

	int (*get_chg_current)(void *input, int* val);//unit mA
	int (*set_chg_current)(void *input, int val);//unit mA
	int (*set_chg_enable)(void *input, int val);

	int (*get_usb_current)(void *input, int* val);//unit mA
	int (*set_usb_current)(void *input, int val);//unit mA
	int (*set_usb_enable)(void *input, int val);

	int (*get_usb_voltage)(void *input, int* val);//unit uV
	int (*get_charger_type)(void *input, int* val);
	int (*get_charger_watt)(void *input, int* val);

	int (*get_bat_temp)(void *input, int* val);//unit C
	int (*get_bat_voltage)(void *input, int* val);//unit uV
	int (*get_bat_ocv)(void *input, int* val);//unit mV
	int (*get_bat_id)(void *input, int* val);

	int (*get_bat_cycle)(void *input, int* val);
	int (*set_bat_cycle)(void *input, int val);

	int (*get_adc_value)(void *input, int channel, int* val);

	/*wireless*/
	int (*get_chip_id)(void *input);
	int (*wls_en)(void *input, bool val);
};

extern int moto_chg_tcmd_register(struct moto_chg_tcmd_client *client);

#endif
