#define WT6670_FIRMWARE_VERSION  0x03

struct wt6670f {
	struct device *dev;
	struct i2c_client *client;

	int i2c_scl_pin;
	int i2c_sda_pin;
	int reset_pin;

	int count;
	int chg_type;
	bool chg_ready;

	struct mutex i2c_rw_lock;
};

int wt6670f_isp_flow(struct wt6670f *chip);
