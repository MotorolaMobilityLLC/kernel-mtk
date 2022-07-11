#ifndef __WT6670F_H
#define __WT6670F_H

struct wt6670f {
	struct device *dev;
	struct i2c_client *client;

	int i2c_scl_pin;
	int i2c_sda_pin;
	int reset_pin;

	int count;
	int chg_type;
	bool chg_ready;
	struct notifier_block pm_nb;
	bool wt6670f_suspend_flag;
	struct mutex i2c_rw_lock;
};

enum {
	QC3P_WT6670F,
	QC3P_Z350,
};

#define WT6670_ISP_I2C_ADDR		0x34
#define POWER_SUPPLY_TYPE_USB_HVDCP_3P5    0x9

int wt6670f_do_reset(void);
int wt6670f_isp_flow(struct wt6670f *chip);
extern u16 wt6670f_get_vbus_voltage(void);
extern int wt6670f_set_voltage(u16 voltage);
extern int m_chg_type;

#endif
