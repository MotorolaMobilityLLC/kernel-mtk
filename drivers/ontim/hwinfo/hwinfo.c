#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/kobject.h>
#include <asm-generic/bug.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/errno.h>

#include <linux/fs.h>
#include <asm/system_misc.h>
#include "../../misc/mediatek/include/mt-plat/mtk_boot_common.h"

#include "SCP_sensorHub.h" 
#include "sensor_list.h"
#include "SCP_power_monitor.h"
#include "hwmsensor.h"

#undef  pr_fmt
#define pr_fmt(fmt) "[hwinfo] " fmt

#define BUF_SIZE 64

char front_cam_name[64] = "Unknown";
char frontaux_cam_name[64] = "Unknown";
char back_cam_name[64] = "Unknown";
char backaux_cam_name[64] = "Unknown";
char backaux2_cam_name[64] = "Unknown";

char backaux2_cam_otp_status[64] = "Unknown";
char backaux_cam_otp_status[64] = "Unknown";
char back_cam_otp_status[64] = "Unknown";
char front_cam_otp_status[64] = "Unknown";

char front_cam_efuse_id[64] = {0};
char frontaux_cam_efuse_id[64] = {0};
char back_cam_efuse_id[64] = {0};
char backaux_cam_efuse_id[64] = {0};
char backaux2_cam_efuse_id[64] = {0};

extern unsigned int get_dram_mr(unsigned int index);

struct device_node *node;
int discrete_memory_gpio = -1;

#define GPIO_DISCRETE_MEMORY 163
typedef struct mid_match {
	int index;
	const char *name;
} mid_match_t;

static mid_match_t emmc_table[] = {
	{ 0, "Unknown" },
	{ 0x11, "Toshiba" },
	{ 0x13, "Micron" },
	{ 0x15, "Samsung" },
	{ 0x45, "Sandisk" },
	{ 0x70, "KSI" },
	{ 0x88, "FORESEE" },
	{ 0x90, "Hynix" },
	{ 0x98, "Toshiba" },
	{ 0xAD, "Hynix" },
	{ 0xCE, "Samsung" },
	{ 0xD6, "Longsys" },
};

#define MAX_HWINFO_SIZE 64
#include "hwinfo.h"
typedef struct {
	char *hwinfo_name;
	char hwinfo_buf[MAX_HWINFO_SIZE];
} hwinfo_t;

#define KEYWORD(_name) \
	[_name] = {.hwinfo_name = __stringify(_name), \
		   .hwinfo_buf = {0}},

static hwinfo_t hwinfo[HWINFO_MAX] =
{
#include "hwinfo.h"
};
#undef KEYWORD


static const char *foreach_emmc_table(int index)
{
	int i = 0;
	for (; i < sizeof(emmc_table) / sizeof(mid_match_t); i++) {
		if (index == emmc_table[i].index)
			return emmc_table[i].name;
	}

	return emmc_table[0].name;;
}

static int hwinfo_read_file(char *file_name, char buf[], int buf_size)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos = 0;
	ssize_t len = 0;

	if (file_name == NULL || buf == NULL)
		return -1;

	fp = filp_open(file_name, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		printk(KERN_CRIT "file not found/n");
		return -1;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	memset(buf, 0x00, buf_size);
	len = vfs_read(fp, buf, buf_size, &pos);
	buf[buf_size - 1] = '\n';
	printk(KERN_INFO "buf= %s,size = %ld \n", buf, (long int)len);
	filp_close(fp, NULL);
	set_fs(fs);

	return 0;
}

static int hwinfo_write_file(char *file_name, const char buf[], int buf_size)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos = 0;
	ssize_t len = 0;

	if (file_name == NULL || buf == NULL || buf_size < 1)
		return -1;

	fp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		printk(KERN_CRIT "file not found/n");
		return -1;

	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	pos = fp->f_pos;
	len = vfs_write(fp, buf, buf_size, &pos);
	fp->f_pos = pos;
	printk(KERN_INFO "buf = %s,size = %ld \n", buf, (long int)len);
	filp_close(fp, NULL);
	set_fs(fs);
	return 0;
}

#define MODEL_FILE "/sys/firmware/devicetree/base/model"
static void get_cpu_type(void)
{
	char data[16];
	char *buf = hwinfo[CPU_TYPE].hwinfo_buf;
	int ret = hwinfo_read_file(MODEL_FILE, buf, MAX_HWINFO_SIZE);
	if (ret != 0) {
		pr_err("read " MODEL_FILE " fail, ret=%d\n", ret);
		strcpy(buf, "Unknown");
		return;
	}
}

/* BEGIN, Ontim,  wzx, 19/04/19, St-result :PASS,LCD and TP Device information */
#define LCD_INFO_FILE "/sys/ontim_dev_debug/touch_screen/lcdvendor"
static int get_lcd_type(void)
{
	char buf[200] = {0};
	int  ret = 0;

	ret = hwinfo_read_file(LCD_INFO_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get lcd_type read file failed.\n");
		return -1;
	}
	printk(KERN_INFO "lcd %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	memcpy(hwinfo[LCD_MFR].hwinfo_buf, buf, strlen(buf));

	return 0;
}

#define TP_VENDOR_FILE "/sys/ontim_dev_debug/touch_screen/vendor"
#define TP_VERSION_FILE "/sys/ontim_dev_debug/touch_screen/version"
static int get_tp_info(void)
{
	char buf[BUF_SIZE] = {0};
	char buf2[BUF_SIZE] = {0};
	char str[BUF_SIZE + BUF_SIZE] = {0};
	int  ret = 0;

	ret = hwinfo_read_file(TP_VENDOR_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_tp_info failed.");
		return -1;
	}
	printk(KERN_INFO "tp %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	ret = hwinfo_read_file(TP_VERSION_FILE, buf2, sizeof(buf2));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_tp_info failed.");
		return -1;
	}
	printk(KERN_INFO "tp version %s\n", buf2);
	if (buf2[strlen(buf2) - 1] == '\n')
		buf2[strlen(buf2) - 1] = '\0';

	sprintf(str, "%s-version#%s", buf, buf2);
	memcpy(hwinfo[TP_MFR].hwinfo_buf, str , strlen(str));
	return 0;
}

#define POWER_USB_ONLINE_FILE "/sys/class/power_supply/usb/online"
#define POWER_AC_ONLINE_FILE "/sys/class/power_supply/ac/online"
static int get_power_usb_type(void)
{
	char buf[64] = {0};
	char online[64] = {0};
	int ret = 0;

	memset(buf, 0x00, 64);
	ret = hwinfo_read_file(POWER_AC_ONLINE_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_power usb type failed.");
		return -1;
	}
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	printk(KERN_INFO "power ac %s\n", buf);

	memset(online, 0x00, 64);
	ret = hwinfo_read_file(POWER_USB_ONLINE_FILE, online, sizeof(online));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_power usb online failed.");
		return -1;
	}
	if (online[strlen(online) - 1] == '\n')
		online[strlen(online) - 1] = '\0';
	printk(KERN_INFO "power usb online %s\n", online);

	if (!strcmp(online, "1"))
		strcpy(hwinfo[POWER_USB_TYPE].hwinfo_buf, "USB");
	else if (!strcmp(buf, "1"))
		strcpy(hwinfo[POWER_USB_TYPE].hwinfo_buf, "USB_DCP");
	else
		strcpy(hwinfo[POWER_USB_TYPE].hwinfo_buf, "Unknow");

	return 0;
}

#define BATTARY_RESISTANCE_FILE "/sys/ontim_dev_debug/battery/vendor"
static int get_battary_mfr(void)
{
	char buf[64] = {0};
	int ret = 0;

	ret = hwinfo_read_file(BATTARY_RESISTANCE_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_battary_mfr failed.");
		return -1;
	}
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	printk(KERN_INFO "Battary %s\n", buf);

	strcpy(hwinfo[BATTARY_MFR].hwinfo_buf, buf);

	return 0;
}

#define BATTARY_CAPACITY_FILE "/sys/class/power_supply/battery/capacity"
static int get_battary_cap(void)
{
	char buf[20] = {0};
	int ret = 0;
	int capacity_value = 0;

	ret = hwinfo_read_file(BATTARY_CAPACITY_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_battary_cap failed.");
		return -1;
	}
	printk(KERN_INFO "Battary cap %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	sscanf(buf, "%d", &capacity_value);

	strcpy(hwinfo[BATTARY_CAP].hwinfo_buf, buf);

	return 0;
}

#define BATTARY_VOL_FILE "/sys/class/power_supply/battery/voltage_now"
static int get_battary_vol(void)
{
	char buf[20] = {0};
	int ret = 0;
	int voltage_now_value = 0;

	ret = hwinfo_read_file(BATTARY_VOL_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_battary_vol failed.");
		return -1;
	}
	printk(KERN_INFO "Battary vol %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	sscanf(buf, "%d", &voltage_now_value);

	strcpy(hwinfo[BATTARY_VOL].hwinfo_buf, buf);

	return 0;
}

static int set_backaux2_camera_otp_status(const char * buf, int n)
{
	strncpy(backaux2_cam_otp_status, buf, n);
	printk(KERN_INFO  "buf = %s n = %d\n", buf, n);
	backaux2_cam_otp_status[n] = '\0';
	return 0;
}

static int set_backaux_camera_otp_status(const char * buf, int n)
{
	int i = 0;
	strncpy(backaux_cam_otp_status, buf, n);
	printk(KERN_INFO "buf = %s n = %d\n", buf, n);
	backaux_cam_otp_status[n] = '\0';
	for (; i< n+2; i++) 
	{
		printk("buf[%d] = %x\n", i, backaux_cam_otp_status[i]);
	}
	return 0;
}

static int set_back_camera_otp_status(const char * buf, int n)
{
	strncpy(back_cam_otp_status, buf, n);
	printk(KERN_INFO "buf = %s n = %d\n", buf, n);
	back_cam_otp_status[n] = '\0';
	return 0;
}

static int set_front_camera_otp_status(const char * buf, int n)
{
	strncpy(front_cam_otp_status, buf, n);
	printk(KERN_INFO "buf = %s n = %d\n", buf, n);
	front_cam_otp_status[n] = '\0';
	return 0;
}

#define BATTARY_CHARGING_EN_FILE "/sys/devices/platform/charger/charge_onoff_ctrl"
static int get_battery_charging_enabled(void)
{
	char buf[64] = {0};
	int ret = 0;
	int charging_enabled_value = 0;

	ret = hwinfo_read_file(BATTARY_CHARGING_EN_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "battery_charging_enabled failed.");
		return -1;
	}
	printk(KERN_INFO "Battary battery_charging_enabled %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	sscanf(buf, "%d", &charging_enabled_value);

	strcpy(hwinfo[battery_charging_enabled].hwinfo_buf, buf);

	return 0;
}

static int put_battery_charging_enabled(const char * buf, int n)
{
	int ret = 0;

	ret = hwinfo_write_file(BATTARY_CHARGING_EN_FILE, buf, n > 0 ? n : 1);
	if (ret != 0)
	{
		printk(KERN_CRIT "battery_charging_enabled failed.");
		return -1;
	}

	return 0;
}

#define TYPEC_VENDOR_FILE "/sys/ontim_dev_debug/typec/vendor"
static ssize_t get_typec_vendor(void)
{
	char buf[64] = {0};
	int ret = 0;

	ret = hwinfo_read_file(TYPEC_VENDOR_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "get_typec_vendor failed.");
		return -1;
	}
	buf[63] = '\n';
	printk(KERN_INFO "Typec vendor: %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	memcpy(hwinfo[TYPEC_MFR].hwinfo_buf, buf, 64);

	return 0;
}

#define TYPEC_CC_STATUS_FILE "/sys/class/tcpc/type_c_port0/device/typec_cc_orientation"
static ssize_t get_typec_cc_status(void)
{
	char buf[BUF_SIZE] = {0};
	int ret = 0;

	ret = hwinfo_read_file(TYPEC_CC_STATUS_FILE, buf, BUF_SIZE);
	if (ret != 0) {
		printk(KERN_CRIT "get_typec_cc status failed.");
		return -1;
	}
	buf[63] = '\n';
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	printk(KERN_INFO "Typec cc status: %s\n", buf);

	memcpy(hwinfo[TYPEC_CC_STATUS].hwinfo_buf, buf, BUF_SIZE);

	return 0;
}

#define ADB_SN_FILE "/config/usb_gadget/g1/strings/0x409/serialnumber"
static ssize_t get_adb_sn(void)
{
	char buf[BUF_SIZE] = {};
	int ret = 0;

	ret = hwinfo_read_file(ADB_SN_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "get adb sn failed.");
		return -1;
	}
	printk(KERN_INFO "Adb SN: %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	strcpy(hwinfo[ADB_SN].hwinfo_buf, buf);

	return 0;
}

#define SPEAKER_MFR_FILE "/proc/asound/cards"
static int get_speaker_mfr(void)
{
	char buf[MAX_HWINFO_SIZE] = {0};
	char* p = buf;
	char* q = buf;
	int len = 0;
	int ret = -1;

	ret = hwinfo_read_file(SPEAKER_MFR_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_speaker_mfr failed.");
		return -1;
	}

	while ((*p++ != '[') && (p < (buf + MAX_HWINFO_SIZE)));
	while ((*p != ']') && (p < (buf + MAX_HWINFO_SIZE)))
	{
		*q++ = *p++;
		len++;
	}

	buf[len - 1] = '\0';
	printk(KERN_INFO "speaker %s\n", buf);

	memcpy(hwinfo[SPEAKER_MFR].hwinfo_buf, buf, len);
	return 0;
}

//houzn add
#define CHARGER_IC_VENDOR_FILE "/sys/ontim_dev_debug/charge_ic/vendor"
static void get_charger_ic(void)
{
	int ret = 0;
	char buf[MAX_HWINFO_SIZE] = {0};

	sprintf(buf, "%s", "Unknow");
	ret = hwinfo_read_file(CHARGER_IC_VENDOR_FILE, buf, sizeof(buf));
	if (ret)
		pr_err("get %s finger type failed.\n", CHARGER_IC_VENDOR_FILE);

	if (likely(buf[strlen(buf) - 1] == '\n')) {
		buf[strlen(buf) - 1] = '\0';
	}
	strcpy(hwinfo[CHARGER_IC_MFR].hwinfo_buf, buf);
	pr_err("charge_ic:%s .\n", hwinfo[CHARGER_IC_MFR].hwinfo_buf);
}
//add end

#ifdef CONFIG_USB_CABLE
extern unsigned int get_rf_gpio_value(void);
static void get_rfgpio_state(void)
{
	int ret = 0;
	ret = get_rf_gpio_value();
	strcpy(hwinfo[RF_GPIO].hwinfo_buf, ret?"1":"0");
}
#endif

#define FINGERPRINT_VENDOR_FILE "/sys/ontim_dev_debug/fingersensor/vendor"
static ssize_t get_fingerprint_id(void)
{
	char buf[MAX_HWINFO_SIZE] = {};
	int ret = 0;

	ret = hwinfo_read_file(FINGERPRINT_VENDOR_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "get_fingerprint_vendor failed.");
		return -1;
	}
	printk(KERN_INFO "Fingerprint vendor: %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	strcpy(hwinfo[FP_MFR].hwinfo_buf, buf);

	return 0;
}

static ssize_t get_gsensor_id(void)
{
	int err = 0;
	struct sensorInfo_t devinfo;

	memset(&devinfo, 0, sizeof(struct sensorInfo_t));

  	err = sensor_set_cmd_to_hub(ID_ACCELEROMETER,
        	CUST_ACTION_GET_SENSOR_INFO, &devinfo);
	if (err < 0) {
		pr_err("sensor(%d) not register\n", ID_ACCELEROMETER);
		return -1;
	}

	printk(KERN_INFO "gsensor vendor: %s\n", devinfo.name);
	if (devinfo.name[strlen(devinfo.name) - 1] == '\n')
	devinfo.name[strlen(devinfo.name) - 1] = '\0';

	strcpy(hwinfo[GSENSOR_MFR].hwinfo_buf, devinfo.name);
	return 0;
}


static ssize_t get_alsps_id(void)
{
	int err = 0, sensor = -1;
	struct sensorInfo_t devinfo;
	memset(&devinfo, 0, sizeof(struct sensorInfo_t));
	err = sensor_set_cmd_to_hub(ID_PROXIMITY,
	CUST_ACTION_GET_SENSOR_INFO, &devinfo);
	if (err < 0) {
		pr_err("sensor(%d) not register\n", ID_PROXIMITY);
		return -1;
	}

	printk(KERN_INFO "als_prox vendor: %s\n", devinfo.name);
	if (devinfo.name[strlen(devinfo.name) - 1] == '\n')
		devinfo.name[strlen(devinfo.name) - 1] = '\0';

	strcpy(hwinfo[ALSPS_MFR].hwinfo_buf, devinfo.name);

	return 0;
}

static void get_front_camera_id(void)
{
	if (front_cam_name != NULL)
		strncpy(hwinfo[FRONT_CAM_MFR].hwinfo_buf, front_cam_name,
		        ((strlen(front_cam_name) >= sizeof(hwinfo[FRONT_CAM_MFR].hwinfo_buf) ?
		          sizeof(hwinfo[FRONT_CAM_MFR].hwinfo_buf) : strlen(front_cam_name))));
}
static void get_frontaux_camera_id(void)
{
	if (frontaux_cam_name != NULL)
		strncpy(hwinfo[FRONTAUX_CAM_MFR].hwinfo_buf, frontaux_cam_name,
		        ((strlen(frontaux_cam_name) >= sizeof(hwinfo[FRONTAUX_CAM_MFR].hwinfo_buf) ?
		          sizeof(hwinfo[FRONTAUX_CAM_MFR].hwinfo_buf) : strlen(frontaux_cam_name))));
}
static void get_back_camera_id(void)
{
	if (back_cam_name != NULL)
		strncpy(hwinfo[BACK_CAM_MFR].hwinfo_buf, back_cam_name,
		        ((strlen(back_cam_name) >= sizeof(hwinfo[BACK_CAM_MFR].hwinfo_buf) ?
		          sizeof(hwinfo[BACK_CAM_MFR].hwinfo_buf) : strlen(back_cam_name))));
}
static void get_backaux_camera_id(void)
{
	if (backaux_cam_name != NULL)
		strncpy(hwinfo[BACKAUX_CAM_MFR].hwinfo_buf, backaux_cam_name,
		        ((strlen(backaux_cam_name) >= sizeof(hwinfo[BACKAUX_CAM_MFR].hwinfo_buf) ?
		          sizeof(hwinfo[BACKAUX_CAM_MFR].hwinfo_buf) : strlen(backaux_cam_name))));
}
static void get_backaux2_camera_id(void)
{
	if (backaux2_cam_name != NULL)
		strncpy(hwinfo[BACKAUX2_CAM_MFR].hwinfo_buf, backaux2_cam_name,
		        ((strlen(backaux2_cam_name) >= sizeof(hwinfo[BACKAUX2_CAM_MFR].hwinfo_buf) ?
		          sizeof(hwinfo[BACKAUX2_CAM_MFR].hwinfo_buf) : strlen(backaux2_cam_name))));
}
static void get_backaux2_camera_otp_status(void)
{
	if (backaux2_cam_otp_status != NULL)
		strncpy(hwinfo[BACKAUX2_CAM_OTP_STATUS].hwinfo_buf, backaux2_cam_otp_status,
		        ((strlen(backaux2_cam_otp_status) >= sizeof(hwinfo[BACKAUX2_CAM_OTP_STATUS].hwinfo_buf) ?
		          sizeof(hwinfo[BACKAUX2_CAM_OTP_STATUS].hwinfo_buf) : strlen(backaux2_cam_otp_status))));
}
static void get_backaux_camera_otp_status(void)
{
         if (backaux_cam_otp_status != NULL)
                 strncpy(hwinfo[BACKAUX_CAM_OTP_STATUS].hwinfo_buf, backaux_cam_otp_status,
                        ((strlen(backaux_cam_otp_status) >= sizeof(hwinfo[BACKAUX_CAM_OTP_STATUS].hwinfo_buf) ?
                           sizeof(hwinfo[BACKAUX_CAM_OTP_STATUS].hwinfo_buf) : strlen(backaux_cam_otp_status))));
}

static void get_back_camera_otp_status(void)
{
         if (back_cam_otp_status != NULL)
                 strncpy(hwinfo[BACK_CAM_OTP_STATUS].hwinfo_buf, back_cam_otp_status,
                        ((strlen(back_cam_otp_status) >= sizeof(hwinfo[BACK_CAM_OTP_STATUS].hwinfo_buf) ?
                           sizeof(hwinfo[BACK_CAM_OTP_STATUS].hwinfo_buf) : strlen(back_cam_otp_status))));
}


static void get_front_camera_otp_status(void)
{
         if (front_cam_otp_status != NULL)
                 strncpy(hwinfo[FRONT_CAM_OTP_STATUS].hwinfo_buf, front_cam_otp_status,
                        ((strlen(front_cam_otp_status) >= sizeof(hwinfo[FRONT_CAM_OTP_STATUS].hwinfo_buf) ?
                           sizeof(hwinfo[FRONT_CAM_OTP_STATUS].hwinfo_buf) : strlen(front_cam_otp_status))));
}

static void get_front_camera_efuse_id(void)
{
	if (front_cam_efuse_id != NULL)
		strncpy(hwinfo[FRONT_CAM_EFUSE].hwinfo_buf, front_cam_efuse_id,
		        ((strlen(front_cam_efuse_id) >= sizeof(hwinfo[FRONT_CAM_EFUSE].hwinfo_buf) ?
		          sizeof(hwinfo[FRONT_CAM_EFUSE].hwinfo_buf) : strlen(front_cam_efuse_id))));
}

static void get_frontaux_camera_efuse_id(void)
{
	if (frontaux_cam_efuse_id != NULL)
		strncpy(hwinfo[FRONTAUX_CAM_EFUSE].hwinfo_buf, frontaux_cam_efuse_id,
		        ((strlen(frontaux_cam_efuse_id) >= sizeof(hwinfo[FRONTAUX_CAM_EFUSE].hwinfo_buf) ?
		          sizeof(hwinfo[FRONTAUX_CAM_EFUSE].hwinfo_buf) : strlen(frontaux_cam_efuse_id))));
}

static void get_back_camera_efuse_id(void)
{
	if (back_cam_efuse_id != NULL)
		strncpy(hwinfo[BACK_CAM_EFUSE].hwinfo_buf, back_cam_efuse_id,
		        ((strlen(back_cam_efuse_id) >= sizeof(hwinfo[BACK_CAM_EFUSE].hwinfo_buf) ?
		          sizeof(hwinfo[BACK_CAM_EFUSE].hwinfo_buf) : strlen(back_cam_efuse_id))));
}

static void get_backaux_camera_efuse_id(void)
{
	if (backaux_cam_efuse_id != NULL)
		strncpy(hwinfo[BACKAUX_CAM_EFUSE].hwinfo_buf, backaux_cam_efuse_id,
		        ((strlen(backaux_cam_efuse_id) >= sizeof(hwinfo[BACKAUX_CAM_EFUSE].hwinfo_buf) ?
		          sizeof(hwinfo[BACKAUX_CAM_EFUSE].hwinfo_buf) : strlen(backaux_cam_efuse_id))));
}

static void get_backaux2_camera_efuse_id(void)
{
	if (backaux2_cam_efuse_id != NULL)
		strncpy(hwinfo[BACKAUX2_CAM_EFUSE].hwinfo_buf, backaux2_cam_efuse_id,
		        ((strlen(backaux2_cam_efuse_id) >= sizeof(hwinfo[BACKAUX2_CAM_EFUSE].hwinfo_buf) ?
		          sizeof(hwinfo[BACKAUX2_CAM_EFUSE].hwinfo_buf) : strlen(backaux2_cam_efuse_id))));
}

static void get_card_present(void)
{
	struct device_node *dn;
	unsigned int gpio_num = -1;
	char *buf = hwinfo[CARD_HOLDER_PRESENT].hwinfo_buf;

	dn = of_find_node_with_property(NULL, "cd-gpios");
	if (!dn) {
		pr_err("not found node via cd-gpios\n");
		strcpy(buf, "Unknown");
		return;
	}
	gpio_num = of_get_named_gpio_flags(dn, "cd-gpios", 0, NULL);
	pr_info("get_card_present: gpio(%d) value = %d\n", gpio_num, gpio_get_value(gpio_num));
	strcpy(buf, (gpio_get_value(gpio_num) == 1) ? "yes" : "no");
}

static int set_pon_reason(char *src)
{
	sprintf(hwinfo[pon_reason].hwinfo_buf, "%s", src);
	return 0;
}

__setup("androidboot.bootreason=", set_pon_reason);


static int set_band_id(char *src)
{
	sprintf(hwinfo[band_id].hwinfo_buf, "%s", src);
	return 0;
}

__setup("band_id=", set_band_id);

// resist calculation:  v / r = (adc_v - v) / r_up => r = v * r_up / (adc_v - v)
#define ADC_VOLTAGE 1800

#define BOARD_ID_ADC_R_UP 100
// this r table must be ordered!!!
int board_id_adc_r_table[] = { 100, 82, 43, 30 };
// this table must be aligned with above table!!!
char *board_id_version_table[] = { "EVT", "DVT1", "DVT2", "PVT" };

int map2index(const char *msg, int val, int array[], int count)
{
	int i;
	bool inc = (array[0] < array[count - 1]);
	int  small = inc ? 0 : count - 1;
	int  big = inc ? count - 1 : 0;
	int  midval;
	if (val > (array[big] * 3) / 2) {
		pr_err("%s: val=%d is too big.\n", msg, val);
		i = big;
	} else if (val < array[small] / 2) {
		pr_err("%s: val=%d is too small.\n", msg, val);
		i = small;
	} else {
		for (i = 0; i < count - 1; i++) {
			midval = (array[i] + array[i + 1]) / 2;
			if (inc ? val < midval : val > midval)
				break;
		}
	}
	return i;
}

#define BOARD_ID_ADC_FILE "/sys/devices/platform/11001000.auxadc/iio:device0/in_voltage2_input"
static void get_board_id(void)
{
	char data[16];
	char *id_buf = hwinfo[board_id].hwinfo_buf;
	char *ver_buf = hwinfo[hw_version].hwinfo_buf;
	int ret = hwinfo_read_file(BOARD_ID_ADC_FILE, data, sizeof(sizeof(data)));
	if (ret) {
		pr_err("read " BOARD_ID_ADC_FILE " fail, ret=%d\n", ret);
		strcpy(id_buf, "-1");
		strcpy(ver_buf, "Unknown");
		return;
	}
	int v;
	kstrtoint(data, 0, &v);
	int r = v * BOARD_ID_ADC_R_UP / (ADC_VOLTAGE - v);
	int i = map2index("board_id", r, board_id_adc_r_table, ARRAY_SIZE(board_id_adc_r_table));
	pr_info("board_id: data=%s v=%d r=%d i=%d r-ri=%d delta=%d%%\n",
		data, v, r, i, r - board_id_adc_r_table[i], 100 * (r - board_id_adc_r_table[i]) / board_id_adc_r_table[i]);
	sprintf(id_buf, "%d", i);
	if (ARRAY_SIZE(board_id_adc_r_table) != ARRAY_SIZE(board_id_version_table)) {
		pr_err("board_id: Error: board_id_adc_r_table and board_id_version_table have different size\n");
		strcpy(ver_buf, "Mismatch");
	} else
		strcpy(ver_buf, board_id_version_table[i]);
}

#define NFC_VENDOR_FILE "/sys/ontim_dev_debug/nfc/vendor"
char NFC_BUF[MAX_HWINFO_SIZE] = {"Unknow"};
EXPORT_SYMBOL(NFC_BUF);
static ssize_t get_nfc_deviceinfo(void)
{
    int ret = 0;

    ret = hwinfo_read_file(NFC_VENDOR_FILE, NFC_BUF, sizeof(NFC_BUF));
    if (ret != 0) {
        printk(KERN_CRIT "get_nfc_vendor failed.");
        return -1;
    }
    printk(KERN_INFO "NFC vendor: %s\n", NFC_BUF);
    if (NFC_BUF[strlen(NFC_BUF) - 1] == '\n')
        NFC_BUF[strlen(NFC_BUF) - 1] = '\0';
    strcpy(hwinfo[NFC_MFR].hwinfo_buf, NFC_BUF);

    return 0;
}

static int set_serialno(char *src)
{
	strlcpy(hwinfo[serialno].hwinfo_buf, src, MAX_HWINFO_SIZE);
	return 0;
}
__setup("androidboot.serialno=", set_serialno);


#define EMMC_SN_FILE     "/sys/class/mmc_host/mmc0/mmc0:0001/serial"
static void get_emmc_sn(void)
{
	char buf[MAX_HWINFO_SIZE] = {'\0'};
	int ret = 0;

	//read emmc sn
	memset(buf, 0x00, sizeof(buf));
	ret = hwinfo_read_file(EMMC_SN_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "EMMC_SN_FILE failed.");
		return;
	}

	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	memset(hwinfo[emmc_sn].hwinfo_buf, 0x00, sizeof(hwinfo[emmc_sn].hwinfo_buf));
	strncpy(hwinfo[emmc_sn].hwinfo_buf, buf, strlen(buf));
	printk("%s: emmc_sn_buf:%s\n", __func__, buf);
}

/*
pos: 0  1 2  3 4 5 6 7 8  9  0 1 2 3  4  5
cid: D6 0103 413341353531 12 A4F51990 19 B3
*/
static void parse_cid(char *cid)
{
	char bin[16];
	char *p = hwinfo[emmc_info].hwinfo_buf;
	if (strlen(cid) < 32) {
		strcpy(p, "Unknown");
		pr_err("%s: invalid cid: %s\n", __func__, cid);
		return;
	}
	hex2bin(bin, cid, sizeof(bin));
	p += sprintf(p, "%s", foreach_emmc_table(bin[0])); // mfr
	p += sprintf(p, " %02X%02X", bin[1], bin[2]); // oemid
	p += sprintf(p, " %6.6s", &bin[3]); // prod_name
	p += sprintf(p, " %02X", bin[9]); // prv
	p += sprintf(p, " %02X%02X%02X%02X", bin[10], bin[11], bin[12], bin[13]); // serial
	p += sprintf(p, " %d", bin[14] >> 4); // month
	p += sprintf(p, "/%d", (bin[14] & 0xF) + 1997 + 16); // year
}

#define EMMC_CID_FILE     "/sys/class/mmc_host/mmc0/mmc0:0001/cid"
static void get_emmc_cid(void)
{
	char buf[MAX_HWINFO_SIZE] = {'\0'};
	int ret = 0;

	//read emmc cid
	memset(buf, 0x00, sizeof(buf));
	ret = hwinfo_read_file(EMMC_CID_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "EMMC_CID_FILE failed.");
		return;
	}
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	memset(hwinfo[emmc_cid].hwinfo_buf, 0x00, sizeof(hwinfo[emmc_cid].hwinfo_buf));
	strncpy(hwinfo[emmc_cid].hwinfo_buf, buf, strlen(buf));
	printk("%s: emmc_cid_buf:%s\n", __func__, buf);
	parse_cid(buf);
}

#define LPDDR_SIZE_FILE   "/proc/meminfo"
static void get_ddr_cap(void)
{
	int lpddr_cap = 0;
	char buf[MAX_HWINFO_SIZE] = {'\0'};
	char* p = buf;
	char* q = buf;
	int ret = 0;

	memset(buf, 0x00, sizeof(buf));
	ret = hwinfo_read_file(LPDDR_SIZE_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "LPDDR_SIZE_FILE failed.");
		return;
	}

	while (*p++ != ':' && p < buf + MAX_HWINFO_SIZE);
	while (*p++ == ' ' && p < buf + MAX_HWINFO_SIZE);
	p = p - 1;
	q = p;
	while (*q++ != ' ' && q < buf + MAX_HWINFO_SIZE);
	memcpy(buf, p, q - p - 1);
	buf[q - p - 1] = '\0';
	buf[q - p]   = '\0';
	kstrtoint(buf, 0, &lpddr_cap);
	lpddr_cap = (lpddr_cap - 1) / (1024 * 1024) + 1; // convert from KB to GB
	pr_info("%s: raw=%sKB real=%dGB\n", __func__, buf, lpddr_cap);
	if (lpddr_cap > 4)
		if (lpddr_cap <= 6)
			lpddr_cap = 6;
		else if (lpddr_cap <= 8)
			lpddr_cap = 8;
	sprintf(hwinfo[lpddr_capacity].hwinfo_buf, "%dGB", lpddr_cap);
}

#define EMMC_LIFE_TIME_FILE "/sys/class/mmc_host/mmc0/mmc0:0001/life_time"
static void get_emmc_lifetime(void)
{
	char buf[MAX_HWINFO_SIZE] = {'\0'};
	int ret = 0;

	//read emmc life_time
	memset(buf, 0x00, sizeof(buf));
	ret = hwinfo_read_file(EMMC_LIFE_TIME_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "EMMC_LIFE_TIME_FILE failed.");
		return;
	}
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	strncpy(hwinfo[emmc_life].hwinfo_buf, buf, strlen(buf));
	printk("%s:emmc life %s \n", __func__, buf);
}

#define EMMC_SIZE_FILE   "/sys/class/mmc_host/mmc0/mmc0:0001/block/mmcblk0/size"
static void get_emmc_size(void)
{
	int emmc_cap = 0;
	char buf[MAX_HWINFO_SIZE] = {'\0'};
	int ret = 0;
	int i;

	memset(buf, 0x00, sizeof(buf));
	ret = hwinfo_read_file(EMMC_SIZE_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "EMMC_SIZE_FILE failed.");
		return;
	}
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	kstrtoint(buf, 0, &emmc_cap);
	emmc_cap = (emmc_cap - 1) / (1024 * 1024 * 2) + 1; // convert from sectors to GB
	pr_info("%s: raw=%s sectors  real=%dGB\n", __func__, buf, emmc_cap);
	int capacity[] = { 4, 8, 16, 32, 64, 128, 256, 512 };
	for (i = 0; i < ARRAY_SIZE(capacity); i++)
		if (emmc_cap < capacity[i])
			break;
	sprintf(hwinfo[emmc_capacity].hwinfo_buf, "%dGB", (i < ARRAY_SIZE(capacity)) ? capacity[i] : emmc_cap);
}

#define EMMC_MANFID_FILE "/sys/class/mmc_host/mmc0/mmc0:0001/manfid"
static void get_emmc_mfr(void)
{
	int emmc_mid = 0;
	const char *emmc_mid_name;
	char buf[MAX_HWINFO_SIZE] = {'\0'};
	int ret = 0;

	ret = hwinfo_read_file(EMMC_MANFID_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "EMMC_MANFID_FILE failed.");
		return;
	}
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	kstrtoint(buf, 0, &emmc_mid);

	emmc_mid_name = foreach_emmc_table(emmc_mid);
	if (emmc_mid_name == NULL) {
		printk(KERN_CRIT "cannot recognize emmc mid=0x%x", emmc_mid);
		emmc_mid_name = "Unknown";
	}
	strncpy(hwinfo[emmc_mfr].hwinfo_buf, emmc_mid_name, strlen(emmc_mid_name));
}

bool is_discrete_memory(void)
{
	return (discrete_memory_gpio >= 0) && (gpio_get_value(discrete_memory_gpio) == 0);
}

static void get_discrete_memory(void)
{
	char *buf = hwinfo[discrete_memory].hwinfo_buf;
	if (discrete_memory_gpio < 0)
		strcpy(buf, "Unknown");
	else
		strcpy(buf, is_discrete_memory() ? "yes" : "no");
}

static void get_lpddr_mfr(void)
{
	unsigned int vendor_id = get_dram_mr(5);
	char * buf = hwinfo[lpddr_mfr].hwinfo_buf;
	if (is_discrete_memory()) {
		switch (vendor_id) {
			case 0x06: strcpy(buf, "Kingston"); break;
			case 0x13: strcpy(buf, "CXMT"); break;
			case 0x1:
			case 0xFF: strcpy(buf, "Longsys"); break;
			default: sprintf(buf, "0x%02X", vendor_id);
		}
	} else {
		get_emmc_mfr();
		strcpy(buf, hwinfo[emmc_mfr].hwinfo_buf);
	}
}

// get_current_cpuid for imie
extern u32 get_devinfo_with_index(u32 index);
#define CPUID_REG_INDEX 12
#define CPUID_REG_NUM 4
static void get_current_cpuid(void)
{
	u32 ontim_cpuid = 0 ;
	int i =0 ;
	char temp_buffer[MAX_HWINFO_SIZE]={0};
	for(i = CPUID_REG_INDEX;i<(CPUID_REG_INDEX+CPUID_REG_NUM);i++){
		ontim_cpuid = get_devinfo_with_index(i);
		sprintf(temp_buffer+strlen(temp_buffer),"%02x%02x%02x%02x", ontim_cpuid&0xFF, (ontim_cpuid>>8)&0xFF, 
		    (ontim_cpuid>>16)&0xFF, (ontim_cpuid>>24)&0xFF);
	}
	sprintf(hwinfo[current_cpuid].hwinfo_buf,"%s",temp_buffer);
}

#ifdef CONFIG_HBM_SUPPORT
#include <primary_display.h>

bool g_hbm_enable = false;
extern unsigned int g_last_level;
extern int mtkfb_set_backlight_level(unsigned int level);
extern enum DISP_POWER_STATE primary_get_state(void);

static int set_hbm_status(const char * buf, int n)
{
	printk("hbm user buf:%s\n", buf);

#ifdef SMT_VERSION
	printk("SMT version,No hbm");
#else
	if (primary_get_state() == DISP_SLEPT) {
		printk("Sleep State, Do not set hbm\n");
		g_hbm_enable = false;
		return 0;
	}

	switch (buf[0]){
		case '0':
			if (!g_hbm_enable) {
				printk("Have been disabled hbm, exit!\n");
				break;
			}
			g_hbm_enable = false;
			mtkfb_set_backlight_level(g_last_level);
			break;
		case '3':
			if (g_hbm_enable) {
				printk("Have been enabled hbm, exit!\n");
				break;
			}
			g_hbm_enable = true;
			mtkfb_set_backlight_level(256);
			break;
		default:
			g_hbm_enable = false;
			break;
	}
#endif
	return 0;
}

static void get_hbm_status(void)
{
	char hbm_str_st[8] = {0};
	if (g_hbm_enable){
	    strcpy(hbm_str_st, "hbm:on");
	}else{
	    strcpy(hbm_str_st, "hbm:off");
	}
	sprintf(hwinfo[hbm].hwinfo_buf,"%s",hbm_str_st);
}
#endif

//extern unsigned int get_boot_mode(void);
// TODO: temp
unsigned int get_boot_mode(void)
{
return 0;
}


static ssize_t hwinfo_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	int i = 0;

	printk(KERN_INFO "hwinfo sys node %s \n", attr->attr.name);

	for (; i < HWINFO_MAX && strcmp(hwinfo[i].hwinfo_name, attr->attr.name) && ++i;);

	switch (i)
	{
	case CPU_TYPE:
		get_cpu_type();
		break;
	case NFC_MFR:
		get_nfc_deviceinfo();
		break;
	case SPEAKER_MFR:
		get_speaker_mfr();
		break;
	case POWER_USB_TYPE:
		get_power_usb_type();
		break;
	case BATTARY_MFR:
		get_battary_mfr();
		break;
	case BATTARY_VOL:
		get_battary_vol();
		break;
	case BATTARY_CAP:
		get_battary_cap();
		break;
	case battery_charging_enabled:
		get_battery_charging_enabled();
		break;
	case board_id:
	case hw_version:
		get_board_id();
		break;
	case LCD_MFR:
		get_lcd_type();
		break;
	case TP_MFR:
		get_tp_info();
		break;
	case TYPEC_MFR:
		get_typec_vendor();
		break;
	case TYPEC_CC_STATUS:
		get_typec_cc_status();
		break;
	case ADB_SN:
		get_adb_sn();
		break;
	case FRONT_CAM_MFR:
		get_front_camera_id();
		break;
	case FRONTAUX_CAM_MFR:
		get_frontaux_camera_id();
		break;
	case BACK_CAM_EFUSE:
		get_back_camera_efuse_id();
		break;
	case BACKAUX_CAM_EFUSE:
		get_backaux_camera_efuse_id();
		break;
	case BACKAUX2_CAM_EFUSE:
		get_backaux2_camera_efuse_id();
		break;
	case FRONT_CAM_EFUSE:
		get_front_camera_efuse_id();
		break;
	case FRONTAUX_CAM_EFUSE:
		get_frontaux_camera_efuse_id();
		break;
	case BACK_CAM_MFR:
		get_back_camera_id();
		break;
	case BACKAUX_CAM_MFR:
		get_backaux_camera_id();
		break;
	case BACKAUX2_CAM_MFR:
		get_backaux2_camera_id();
		break;
	case BACKAUX2_CAM_OTP_STATUS:
		get_backaux2_camera_otp_status();
		break; 
	case BACKAUX_CAM_OTP_STATUS:
		get_backaux_camera_otp_status();
		break;
	case BACK_CAM_OTP_STATUS:
		get_back_camera_otp_status();
		break;
	case FRONT_CAM_OTP_STATUS:
		get_front_camera_otp_status();
		break;
	case FP_MFR:
		get_fingerprint_id();
		break;
	case GSENSOR_MFR:
		get_gsensor_id();
		break;
	case ALSPS_MFR:
		get_alsps_id();
		break;
	case CARD_HOLDER_PRESENT:
		get_card_present();
		break;
	case CHARGER_IC_MFR: //houzn add
		get_charger_ic();
		break;
#ifdef CONFIG_USB_CABLE
	case RF_GPIO:
		get_rfgpio_state();
		break;
#endif
	case emmc_sn:
		get_emmc_sn();
		break;
	case emmc_cid:
	case emmc_info:
		get_emmc_cid();
		break;
	case emmc_mfr:
		get_emmc_mfr();
		break;
	case lpddr_mfr:
		get_lpddr_mfr();
		break;
	case discrete_memory:
		get_discrete_memory();
		break;
	case emmc_capacity:
		get_emmc_size();
		break;
	case lpddr_capacity:
		get_ddr_cap();
		break;
	case emmc_life:
		get_emmc_lifetime();
		break;
	case current_cpuid:
		get_current_cpuid();
		break;
#ifdef CONFIG_HBM_SUPPORT
	case hbm:
		get_hbm_status();
		break;
#endif
	default:
		break;
	}
	return sprintf(buf, "%s=%s \n",  attr->attr.name, ((i >= HWINFO_MAX || hwinfo[i].hwinfo_buf[0] == '\0') ? "unknow" : hwinfo[i].hwinfo_buf));
}

static ssize_t hwinfo_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	int i = 0;
	printk(KERN_INFO "hwinfo sys node %s \n", attr->attr.name);

	for (; i < HWINFO_MAX && strcmp(hwinfo[i].hwinfo_name, attr->attr.name) && ++i;);

	switch (i)
	{
	case battery_charging_enabled:
		put_battery_charging_enabled(buf, n);
		break;
	case BACKAUX2_CAM_OTP_STATUS:
		set_backaux2_camera_otp_status(buf, n);
		break; 
	case BACKAUX_CAM_OTP_STATUS:
		set_backaux_camera_otp_status(buf, n);
		break;
	case BACK_CAM_OTP_STATUS:
		set_back_camera_otp_status(buf, n);
		break;
	case FRONT_CAM_OTP_STATUS:
		set_front_camera_otp_status(buf, n);
		break;
#ifdef CONFIG_HBM_SUPPORT 
	case hbm:
		set_hbm_status(buf, n);
		break;
#endif
	default:
		break;
	};
	return n;
}
#define KEYWORD(_name) \
    static struct kobj_attribute hwinfo##_name##_attr = {   \
                .attr   = {                             \
                        .name = __stringify(_name),     \
                        .mode = 0644,                   \
                },                                      \
            .show   = hwinfo_show,                 \
            .store  = hwinfo_store,                \
        };

#include "hwinfo.h"
#undef KEYWORD

#define KEYWORD(_name)\
    [_name] = &hwinfo##_name##_attr.attr,

static struct attribute * g[] = {
#include "hwinfo.h"
	NULL
};
#undef KEYWORD

static struct attribute_group attr_group = {
	.attrs = g,
};

int ontim_hwinfo_register(enum HWINFO_E e_hwinfo, char *hwinfo_name)
{
	if ((e_hwinfo >= HWINFO_MAX) || (hwinfo_name == NULL))
		return -1;
	strncpy(hwinfo[e_hwinfo].hwinfo_buf, hwinfo_name, \
	        (strlen(hwinfo_name) >= 20 ? 19 : strlen(hwinfo_name)));
	return 0;
}
EXPORT_SYMBOL(ontim_hwinfo_register);

static int __init hwinfo_init(void)
{
	struct kobject *k_hwinfo = NULL;

	node = of_find_compatible_node(NULL, NULL, "ontim,hwinfo");
	if (!node) {
		pr_err("not find node: ontim,hwinfo\n");
		return -ENODEV;
	}
	discrete_memory_gpio = of_get_named_gpio(node, "discrete-memory-gpios", 0);
	pr_info("discrete_memory_gpio=%d\n", discrete_memory_gpio);

	if ( (k_hwinfo = kobject_create_and_add("hwinfo", NULL)) == NULL ) {
		printk(KERN_ERR "%s:hwinfo sys node create error \n", __func__);
	}

	if ( sysfs_create_group(k_hwinfo, &attr_group) ) {
		printk(KERN_ERR "%s: sysfs_create_group failed\n", __func__);
	}
#ifdef CONFIG_HBM_SUPPORT
	ontim_hwinfo_register(hbm, "hbm");
#endif
	//arch_read_hardware_id = msm_read_hardware_id;
	return 0;
}

static void __exit hwinfo_exit(void)
{
	return ;
}

late_initcall_sync(hwinfo_init);
module_exit(hwinfo_exit);
MODULE_AUTHOR("eko@ontim.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Product Hardward Info Exposure");
