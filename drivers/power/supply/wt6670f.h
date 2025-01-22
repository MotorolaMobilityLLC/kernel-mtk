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

	struct adapter_device *qc_dev;
	const char *qc_dev_name;
	bool not_register_qc_dev;
};

enum {
	QC3P_WT6670F,
	QC3P_Z350,
};

enum {
	DP_DM_UNKNOWN = 0,
	DP_DM_FORCE_QC2_5V,
	DP_DM_FORCE_QC2_9V,
	DP_DM_FORCE_QC3_5V,
	DP_DM_FORCE_QC3P_5V,
	DP_DM_DP_PULSE,
	DP_DM_DM_PULSE,
};

#define WT6670_ISP_I2C_ADDR		0x34
#define POWER_SUPPLY_TYPE_USB_HVDCP_3P5    0x9

int wt6670f_do_reset(void);
int wt6670f_isp_flow(struct wt6670f *chip);
extern u16 wt6670f_get_vbus_voltage(void);
extern int wt6670f_set_voltage(u16 voltage);
extern int m_chg_type;

#endif
