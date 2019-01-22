/*
 * Author: yucong xiong <yucong.xion@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include "cust_biometric.h"
#include "mt6381.h"
#include "vsm_signal_reg.h"
#include "biometric.h"
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <mt-plat/aee.h>
#include "ppg_control.h"
/*----------------------------------------------------------------------------*/
#define DBG                 0
#define DBG_READ            0
#define DBG_WRITE           0
#define DBG_SRAM            0

#define MT6381_DEV_NAME        "MT6381_BIOMETRIC"
static const struct i2c_device_id mt6381_i2c_id[] = { {MT6381_DEV_NAME, 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id biometric_of_match[] = {
	{.compatible = "mediatek,biosensor"},
	{},
};
#endif

/* two slave addr */
#define MT2511_SLAVE_I	0x23
#define MT2511_SLAVE_II 0x33

/* sram type addr */
#define SRAM_EKG_ADDR	0xC8
#define SRAM_PPG1_ADDR	0xD8
#define SRAM_PPG2_ADDR	0xE8
#define SRAM_BISI_ADDR	0xF8

/* read counter addr */
#define SRAM_EKG_READ_COUNT_ADDR	0xC0
#define SRAM_PPG1_READ_COUNT_ADDR	0xD0
#define SRAM_PPG2_READ_COUNT_ADDR	0xE0
#define SRAM_BISI_READ_COUNT_ADDR	0xF0

/* write counter addr */
#define SRAM_EKG_WRITE_COUNT_ADDR	0xCC
#define SRAM_PPG1_WRITE_COUNT_ADDR	0xDC
#define SRAM_PPG2_WRITE_COUNT_ADDR	0xEC
#define SRAM_BISI_WRITE_COUNT_ADDR	0xFC

#define SRAM_COUNTER_RESET_MASK     0x20000000
#define SRAM_COUNTER_OFFSET         29

#ifdef MTK_SENSOR_BIO_CUSTOMER_GOMTEL
#define GPIO_MT2511_PPG_VDRV_EN     HAL_GPIO_11
#define GPIO_MT2511_32K             HAL_GPIO_13
#define GPIO_MT2511_RST_PORT_PIN    HAL_GPIO_14
#define GPIO_MT2511_AFE_PWD_PIN     HAL_GPIO_15
#else
#define GPIO_MT2511_32K             HAL_GPIO_14
#define GPIO_MT2511_AFE_PWD_PIN     HAL_GPIO_24
#define GPIO_MT2511_RST_PORT_PIN    HAL_GPIO_29
#endif


#define UPDATE_COMMAND_ADDR     0x2328
#define SEC_UPDATE_COMMAND_ADDR 0x2728

#define PPG1_GAIN_ADDR		0x3318
#define PPG1_GAIN_MASK      0x00000007
#define PPG1_GAIN_OFFSET    0

#define PPG2_GAIN_ADDR      PPG1_GAIN_ADDR
#define PPG2_GAIN_MASK      0x00000038
#define PPG2_GAIN_OFFSET    3

#define PPG_AMDAC_ADDR      PPG1_GAIN_ADDR
#define PPG_AMDAC_MASK      0x3C00000
#define PPG_AMDAC_OFFSET    22

#define PPG_AMDAC1_MASK      0x1C00000
#define PPG_AMDAC1_OFFSET    22
#define PPG_AMDAC2_MASK      0xE000000
#define PPG_AMDAC2_OFFSET    25


#define PPG_PGA_GAIN_ADDR      PPG1_GAIN_ADDR
#define PPG_PGA_GAIN_MASK      0x1C0000
#define PPG_PGA_GAIN_OFFSET    18

#define PPG1_CURR_ADDR      0x332C
#define PPG1_CURR_MASK      0x000000FF
#define PPG1_CURR_OFFSET    0
#define PPG2_CURR_ADDR      PPG1_CURR_ADDR
#define PPG2_CURR_MASK      0x0000FF00
#define PPG2_CURR_OFFSET    8

#define CHIP_VERSION_ADDR       0x23AC
#define CHIP_VERSION_E1         0X1
#define CHIP_VERSION_E2         0X2
#define CHIP_VERSION_UNKNOWN    0XFF

#define DIGITAL_START_ADDR 0x3360

#define EKG_SAMPLE_RATE_ADDR1 0x3364
#define EKG_SAMPLE_RATE_ADDR2 0x3310
#define EKG_DEFAULT_SAMPLE_RATE 256

#define PPG_SAMPLE_RATE_ADDR 0x232C
#define PPG_FSYS             1048576
#define PPG_DEFAULT_SAMPLE_RATE 125

#define VSM_BISI_SRAM_LEN   256

#define MAX_WRITE_LENGTH 4

enum {
	EKG = 0,
	PPG1,
	PPG2,
	NUM_OF_TYPE,
};

struct sensor_info {
	unsigned short read_counter;
	unsigned short write_counter;
	unsigned short upsram_rd_data;
	char *raw_data_path;
	struct file *filp;
	unsigned int numOfData;
	unsigned int enBit;
};

static struct sensor_info info[NUM_OF_TYPE];
static struct task_struct *bio_tsk[3] = { 0 };
static DEFINE_MUTEX(bio_data_collection_mutex);

/* Assign default capabilities setting */
static int64_t getCurNS(void)
{
	int64_t ns;
	struct timespec time;

	time.tv_sec = time.tv_nsec = 0;
	get_monotonic_boottime(&time);
	ns = time.tv_sec * 1000000000LL + time.tv_nsec;

	return ns;
}

/* static bool combo_usage; */
u64 pre_ppg1_timestamp;
u64 pre_ppg2_timestamp;
u64 pre_ekg_timestamp;
static int8_t vsm_chip_version = -1;
static int biosensor_init_flag = -1;

struct i2c_msg ekg_msg[VSM_SRAM_LEN * 2];
struct i2c_msg ppg1_msg[VSM_SRAM_LEN * 2];
struct i2c_msg ppg2_msg[VSM_SRAM_LEN * 2];
struct i2c_msg bisi_msg[VSM_SRAM_LEN * 2];
char mt6381_sram_addr[4] = { SRAM_EKG_ADDR, SRAM_PPG1_ADDR, SRAM_PPG2_ADDR, SRAM_BISI_ADDR };

/* used to store the data from i2c */
u32 ekg_buf[VSM_SRAM_LEN], sram1_buf[VSM_SRAM_LEN], sram2_buf[VSM_SRAM_LEN];
/* used to store the dispatched ppg data */
u32 temp_buf[VSM_SRAM_LEN], ppg1_buf2[VSM_SRAM_LEN], ppg2_buf2[VSM_SRAM_LEN];
/* used to store ambient light data */
u32 ppg1_amb_buf[VSM_SRAM_LEN], ppg2_amb_buf[VSM_SRAM_LEN];
/* used to store agc (auto. gain control) data for algorithm debug */
u32 ppg1_agc_buf[VSM_SRAM_LEN], ppg2_agc_buf[VSM_SRAM_LEN];
u32 ppg1_buf2_len, ppg2_buf2_len;
/* used for AGC */
u32 amb_temp_buf[VSM_SRAM_LEN];
/* used to store downsample data (16Hz) for AGC */
u32 agc_ppg1_buf[VSM_SRAM_LEN/32], agc_ppg1_amb_buf[VSM_SRAM_LEN/32];
u32 agc_ppg2_buf[VSM_SRAM_LEN/32], agc_ppg2_amb_buf[VSM_SRAM_LEN/32];
u32 agc_ppg1_buf_len, agc_ppg2_buf_len;

u32 ppg1_amb_buf[VSM_SRAM_LEN], ppg2_amb_buf[VSM_SRAM_LEN];
enum vsm_signal_t current_signal;
struct mutex op_lock;
struct signal_data_t VSM_SIGNAL_MODIFY_array[50];
struct signal_data_t VSM_SIGNAL_NEW_INIT_array[50];
int modify_array_len;
int new_init_array_len;
u32 set_AFE_TCTRL_CON2, set_AFE_TCTRL_CON3;
u32 AFE_TCTRL_CON2, AFE_TCTRL_CON3;

struct pinctrl *pinctrl_gpios;
struct pinctrl_state *bio_pins_default;
struct pinctrl_state *bio_pins_reset_high, *bio_pins_reset_low;
struct pinctrl_state *bio_pins_pwd_high, *bio_pins_pwd_low;

static int64_t enable_time;
static int64_t previous_timestamp[VSM_SRAM_PPG2+1];
static int64_t numOfData[3] = {0};
static int numOfEKGDataNeedToDrop;
static bool data_dropped;
atomic_t bio_trace;
static u32 lastAddress;
static u8 lastRead[] = {0, 0, 0, 0};
static unsigned int polling_delay;
static struct {
	bool in_latency_test;
	uint32_t first_data;
	uint32_t second_data;
	uint32_t delay_num;
	uint32_t ekg_num;
	uint32_t ppg1_num;
	uint32_t ppg2_num;
} latency_test_data;

static enum vsm_status_t vsm_driver_write_signal(struct signal_data_t *reg_addr, int32_t len, uint32_t *enable_data);
static int bio_file_write(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size);
static int mt6381_local_init(void);
static int mt6381_local_remove(void);
static struct bio_init_info mt6381_init_info = {
	.name = "mt6381",
	.init = mt6381_local_init,
	.uninit = mt6381_local_remove,
};

void mt6381_init_i2c_msg(u8 addr)
{
	int i;

	for (i = 0; i < VSM_SRAM_LEN; i++) {
		ekg_msg[2 * i].addr = addr;
		ekg_msg[2 * i].flags = 0;
		ekg_msg[2 * i].len = 1;
		ekg_msg[2 * i].buf = mt6381_sram_addr;
		ekg_msg[2 * i + 1].addr = addr;
		ekg_msg[2 * i + 1].flags = I2C_M_RD;
		ekg_msg[2 * i + 1].len = 4;
		ekg_msg[2 * i + 1].buf = (u8 *) (ekg_buf + i);

		ppg1_msg[2 * i].addr = addr;
		ppg1_msg[2 * i].flags = 0;
		ppg1_msg[2 * i].len = 1;
		ppg1_msg[2 * i].buf = mt6381_sram_addr + 1;
		ppg1_msg[2 * i + 1].addr = addr;
		ppg1_msg[2 * i + 1].flags = I2C_M_RD;
		ppg1_msg[2 * i + 1].len = 4;
		ppg1_msg[2 * i + 1].buf = (u8 *) (sram1_buf + i);

		ppg2_msg[2 * i].addr = addr;
		ppg2_msg[2 * i].flags = 0;
		ppg2_msg[2 * i].len = 1;
		ppg2_msg[2 * i].buf = mt6381_sram_addr + 2;
		ppg2_msg[2 * i + 1].addr = addr;
		ppg2_msg[2 * i + 1].flags = I2C_M_RD;
		ppg2_msg[2 * i + 1].len = 4;
		ppg2_msg[2 * i + 1].buf = (u8 *) (sram2_buf + i);
	}
}

struct i2c_client *mt6381_i2c_client;

int mt6381_i2c_write_read(u8 addr, u8 reg, u8 *buf, u16 length)
{
	int res;
	struct i2c_msg msg[2];

	msg[0].addr = addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;
	msg[0].buf[0] = reg;

	msg[1].addr = addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = length;
	msg[1].buf = buf;
	res = i2c_transfer(mt6381_i2c_client->adapter, msg, ARRAY_SIZE(msg));
	if (res < 0) {
		BIO_PR_ERR("mt6381 i2c read failed. addr:%x, reg:%x, errno:%d\n", addr, reg, res);
		return res;
	}
	return VSM_STATUS_OK;
}

int mt6381_i2c_write(u8 addr, u8 *buf, u16 length)
{
	mt6381_i2c_client->addr = addr;
	return i2c_master_send(mt6381_i2c_client, buf, length);
}

static int bio_thread(void *arg)
{
	struct sched_param param = {.sched_priority = 99 };
	struct sensor_info *s_info = (struct sensor_info *)arg;
	u8 buf[4];
	u32 rc, wc;
	/* u32 count = 0; */
	/* int status = 0; */
	size_t size;
	u8 str_buf[100];
	int len;
	uint32_t read_counter1 = 0, read_counter2 = 0;
	uint32_t write_counter1 = 0, write_counter2 = 0;
	struct bus_data_t data;

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);

	for (;;) {
		if (kthread_should_stop())
			break;

		msleep_interruptible(polling_delay);
		mutex_lock(&bio_data_collection_mutex);
		if (s_info->filp != NULL) {
			/* read counter */
			data.addr = s_info->read_counter >> 8;
			data.reg = s_info->read_counter & 0xFF;
			data.data_buf = (uint8_t *) &read_counter2;
			data.length = sizeof(read_counter2);
			vsm_driver_read_register(&data);
			/* vsm_driver_get_read_counter(sram_type, &read_counter2); */
			do {
				read_counter1 = read_counter2;
				/* vsm_driver_get_read_counter(sram_type, &read_counter2); */
				vsm_driver_read_register(&data);
			} while ((read_counter1 & 0x1ff0000) != (read_counter2 & 0x1ff0000));

			/* write counter */
			data.addr = s_info->write_counter >> 8;
			data.reg = s_info->write_counter & 0xFF;
			data.data_buf = (uint8_t *) &write_counter2;
			data.length = sizeof(write_counter2);
			vsm_driver_read_register(&data);
			/* vsm_driver_write_counter(sram_type, &write_counter2); */
			do {
				write_counter1 = write_counter2;
				/* vsm_driver_write_counter(sram_type, &write_counter2); */
				vsm_driver_read_register(&data);
			} while ((write_counter1 & 0x1ff0000) != (write_counter2 & 0x1ff0000));
			/* mt2511_read(s_info->read_counter, buf); */
			/* rc = ((*(int *)buf) & 0x1ff0000) >> 16; */
			rc = (read_counter2 & 0x1ff0000) >> 16;
			/* mt2511_read(s_info->write_counter, buf); */
			/* wc = ((*(int *)buf) & 0x1ff0000) >> 16; */
			wc = (write_counter2 & 0x1ff0000) >> 16;

			data.addr = s_info->upsram_rd_data >> 8;
			data.reg = s_info->upsram_rd_data & 0xFF;
			data.data_buf = (uint8_t *)buf;
			data.length = sizeof(buf);
			if (atomic_read(&bio_trace) != 0)
				BIO_LOG("rc = %d, wc = %d\n", rc, wc);
			while (rc != wc && s_info->numOfData) {
				vsm_driver_read_register(&data);
				if (atomic_read(&bio_trace) != 0)
					BIO_LOG("%d, %d, %x, %x, %lld\n", rc, wc, *(int *)buf,
						s_info->upsram_rd_data, sched_clock());
				len = sprintf(str_buf, "%x\n", *(int *)buf);
				size = bio_file_write(s_info->filp, 0, str_buf, len);
				rc = (rc + 1) % 384;
				s_info->numOfData--;
			}

			if (s_info->numOfData == 0) {
				vfs_fsync(s_info->filp, 0);
				filp_close(s_info->filp, NULL);
				s_info->filp = NULL;
			} else {
				/* read counter */
				data.addr = s_info->read_counter >> 8;
				data.reg = s_info->read_counter & 0xFF;
				data.data_buf = (uint8_t *) &read_counter2;
				data.length = sizeof(read_counter2);
				vsm_driver_read_register(&data);
				/* vsm_driver_get_read_counter(sram_type, &read_counter2); */
				do {
					read_counter1 = read_counter2;
					/* vsm_driver_get_read_counter(sram_type, &read_counter2); */
					vsm_driver_read_register(&data);
				} while ((read_counter1 & 0x1ff0000) != (read_counter2 & 0x1ff0000));
				/* mt2511_read(s_info->read_counter, buf); */
				/* rc = ((*(int *)buf) & 0x1ff0000) >> 16; */
				rc = (read_counter2 & 0x1ff0000) >> 16;
				if (rc != wc) {
					len = sprintf(str_buf, "Unexpected rc = %d, wr = %d\n", rc, wc);
					size = bio_file_write(s_info->filp, 0, str_buf, len);
				}
			}
		}
		mutex_unlock(&bio_data_collection_mutex);
	}
	return 0;
}

static void bio_test_init(void)
{
	static bool init_done;

	if (init_done == false) {
		info[EKG].write_counter = 0x33CC;
		info[EKG].read_counter = 0x33C0;
		info[EKG].upsram_rd_data = 0x33C8;
		info[EKG].raw_data_path = "/data/bio/ekg";
		info[EKG].filp = NULL;
		info[EKG].numOfData = 0;
		info[EKG].enBit = 0x18;

		info[PPG1].write_counter = 0x33DC;
		info[PPG1].read_counter = 0x33D0;
		info[PPG1].upsram_rd_data = 0x33D8;
		info[PPG1].raw_data_path = "/data/bio/ppg1";
		info[PPG1].filp = NULL;
		info[PPG1].numOfData = 0;
		info[PPG1].enBit = 0x124;

		info[PPG2].write_counter = 0x33EC;
		info[PPG2].read_counter = 0x33E0;
		info[PPG2].upsram_rd_data = 0x33E8;
		info[PPG2].raw_data_path = "/data/bio/ppg2";
		info[PPG2].filp = NULL;
		info[PPG2].numOfData = 0;
		info[PPG2].enBit = 0x144;

		bio_tsk[0] = kthread_create(bio_thread, (void *)&info[EKG], "EKG");
		bio_tsk[1] = kthread_create(bio_thread, (void *)&info[PPG1], "PPG1");
		bio_tsk[2] = kthread_create(bio_thread, (void *)&info[PPG2], "PPG2");
		wake_up_process(bio_tsk[0]);
		wake_up_process(bio_tsk[1]);
		wake_up_process(bio_tsk[2]);
	}
}

static struct file *bio_file_open(const char *path, int flags, int rights)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);

	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}

	return filp;
}

static int bio_file_write(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	struct inode *inode = file->f_mapping->host;

	oldfs = get_fs();
	set_fs(get_ds());

	offset = i_size_read(inode);
	ret = vfs_write(file, data, size, &offset);

	set_fs(oldfs);

	return ret;
}

static void insert_modify_setting(int addr, int value)
{
	int i;

	for (i = 0; i < modify_array_len; i++) {
		if (VSM_SIGNAL_MODIFY_array[i].addr == addr) {
			VSM_SIGNAL_MODIFY_array[i].value = value;
			return;
		}
	}

	if (i == sizeof(VSM_SIGNAL_MODIFY_array)) {
		BIO_INFO("modify array full\n");
	} else {
		VSM_SIGNAL_MODIFY_array[i].addr = addr;
		VSM_SIGNAL_MODIFY_array[i].value = value;
		modify_array_len++;
	}
}

static void remove_modify_setting(int addr, int value)
{
	int i;

	for (i = 0; i < modify_array_len; i++) {
		if (VSM_SIGNAL_MODIFY_array[i].addr == addr) {
			VSM_SIGNAL_MODIFY_array[i].addr = VSM_SIGNAL_MODIFY_array[modify_array_len - 1].addr;
			VSM_SIGNAL_MODIFY_array[i].value = VSM_SIGNAL_MODIFY_array[modify_array_len - 1].value;
			modify_array_len--;
			return;
		}
	}
}

static void clear_modify_setting(void)
{
	modify_array_len = 0;
}

static void update_new_init_setting(void)
{
	int i, j;

	if (modify_array_len != 0) {
		memcpy(VSM_SIGNAL_NEW_INIT_array, VSM_SIGNAL_INIT_array, sizeof(VSM_SIGNAL_INIT_array));
		new_init_array_len = ARRAY_SIZE(VSM_SIGNAL_INIT_array);

		for (i = 0; i < modify_array_len; i++) {
			for (j = 0; j < new_init_array_len; j++) {
				if (VSM_SIGNAL_MODIFY_array[i].addr == VSM_SIGNAL_NEW_INIT_array[j].addr) {
					VSM_SIGNAL_NEW_INIT_array[j].value = VSM_SIGNAL_MODIFY_array[i].value;
					break;
				}
			}
			if (j == new_init_array_len) {
				VSM_SIGNAL_NEW_INIT_array[j].addr = VSM_SIGNAL_MODIFY_array[i].addr;
				VSM_SIGNAL_NEW_INIT_array[j].value = VSM_SIGNAL_MODIFY_array[i].value;
				new_init_array_len++;
			}
		}
	} else
		new_init_array_len = 0;
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char *chipinfo = "hello,mt6381";

	return snprintf(buf, strlen(chipinfo), "%s", chipinfo);
}

static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;

	if (sscanf(buf, "0x%x", &trace) == 1)
		atomic_set(&bio_trace, trace);
	else
		BIO_INFO("invalid content: '%s', length = %zu\n", buf, count);

	return count;
}

static ssize_t store_rstb_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int value = 0;
	int ret = 0;

	ret = kstrtoint(buf, 10, &value);

	if (ret != 0) {
		if (value == 1)
			pinctrl_select_state(pinctrl_gpios, bio_pins_reset_high);
		else if (value == 0)
			pinctrl_select_state(pinctrl_gpios, bio_pins_reset_low);
		else
			BIO_INFO("Wrong parameter, %d\n", value);
	} else
		BIO_INFO("Wrong parameter, %s\n", buf);

	return count;
}

static ssize_t store_io_value(struct device_driver *ddri, const char *buf, size_t count)
{
	u32 value[4] = {0};
	u32 address[4] = {0};
	struct bus_data_t data;
	int num = sscanf(buf, "%x %x %x %x %x %x %x %x ", &address[0], &value[0], &address[1],
		&value[1], &address[2], &value[2], &address[3], &value[3]);

	BIO_LOG("(%d, %x, %x, %x, %x, %x, %x, %x, %x)\n", num, address[0], value[0], address[1],
		value[1], address[2], value[2], address[3], value[3]);
	if (num == 1) {
		data.addr = address[0] >> 8;
		data.reg = address[0] & 0xFF;
		data.data_buf = (uint8_t *) &lastRead;
		data.length = sizeof(lastRead);
		vsm_driver_read_register(&data);

		/* mt2511_read((u16)address[0], lastRead); */
		lastAddress = address[0];
		BIO_LOG("address[0x04%x] = %x\n", address[0], *(u32 *)lastRead);
	} else if (num >= 2) {
		data.addr = address[0] >> 8;
		data.reg = address[0] & 0xFF;
		data.length = sizeof(value[0]);
		data.data_buf = (uint8_t *) &value[0];
		vsm_driver_write_register(&data);
		/* mt2511_write((u16)address[0], value[0]); */
		BIO_LOG("address[0x04%x] = %x\n", address[0], value[0]);

		if (num >= 4) {
			data.addr = address[1] >> 8;
			data.reg = address[1] & 0xFF;
			data.length = sizeof(value[1]);
			data.data_buf = (uint8_t *) &value[1];
			vsm_driver_write_register(&data);
			/* mt2511_write((u16)address[1], value[1]); */
			BIO_LOG("address[0x04%x] = %x\n", address[1], value[1]);
		}

		if (num >= 6) {
			data.addr = address[2] >> 8;
			data.reg = address[2] & 0xFF;
			data.length = sizeof(value[2]);
			data.data_buf = (uint8_t *) &value[2];
			vsm_driver_write_register(&data);
			/* mt2511_write((u16)address[2], value[2]); */
			BIO_LOG("address[0x04%x] = %x\n", address[2], value[2]);
		}

		if (num == 8) {
			data.addr = address[3] >> 8;
			data.reg = address[3] & 0xFF;
			data.length = sizeof(value[3]);
			data.data_buf = (uint8_t *) &value[3];
			vsm_driver_write_register(&data);
			/* mt2511_write((u16)address[3], value[3]); */
			BIO_LOG("address[0x04%x] = %x\n", address[3], value[3]);
		}
	} else
		BIO_INFO("invalid format = '%s'\n", buf);

	return count;
}

static ssize_t show_io_value(struct device_driver *ddri, char *buf)
{

	return sprintf(buf, "address[%x] = %x\n", lastAddress, *(u32 *)lastRead);

}

static ssize_t store_delay(struct device_driver *ddri, const char *buf, size_t count)
{
	int delayTime = 0;
	int ret = 0;

	ret = kstrtoint(buf, 10, &delayTime);

	BIO_LOG("(%d)\n", delayTime);
	if (0 != ret && 0 < delayTime)
		mdelay(delayTime);

	return count;
}

static ssize_t show_delay(struct device_driver *ddri, char *buf)
{

	return sprintf(buf, "usage : echo delayNum(ms in decimal) > delay\n\r");

}

static ssize_t store_data(struct device_driver *ddri, const char *buf, size_t count)
{
	int en = 0;
	int readData[3];
	int i;
	int num = sscanf(buf, "%d %d %d %d", &en, &readData[EKG], &readData[PPG1], &readData[PPG2]);
	unsigned int enBit = 0;
	int32_t len;
	struct signal_data_t *temp;
	uint32_t enable_data;
	struct bus_data_t data;
	/* u32 value; */
	struct signal_data_t reset_counter_array[] = {
		{0x3360, 0x0},
		{0x33D0, 0x60000000},
		{0x33E0, 0x60000000},
		{0x33C0, 0x60000000}
	};

	if (num != 4) {
		BIO_INFO("Wrong parameters %d, %s\n", num, buf);
		return count;
	}

	bio_test_init();

	BIO_LOG("%d, %d, %d, %d\n", en, readData[EKG], readData[PPG1], readData[PPG2]);

	mutex_lock(&bio_data_collection_mutex);

	len = ARRAY_SIZE(reset_counter_array);
	temp = reset_counter_array;
	vsm_driver_write_signal(temp, len, &enable_data);
	/* reset R/W counter */
	/* mt2511_write(0x3360, 0x0); */ /* disable PPG function and reset PPG1 and PPG2 write counters to 0 */
	/* mt2511_write(0x33D0, 0x60000000); */ /* reset PPG1 read counter to 0 */
	/* mt2511_write(0x33E0, 0x60000000); */ /* reset PPG2 read counter to 0 */
	/* mt2511_write(0x33C0, 0x60000000); */ /* reset EKG read counter to 0 */

	for (i = 0; i < NUM_OF_TYPE; i++) {
		if (info[i].filp != NULL) {
			vfs_fsync(info[i].filp, 0);
			filp_close(info[i].filp, NULL);
			info[i].filp = NULL;
		}
	}

	for (i = 0; i < NUM_OF_TYPE; i++) {
		if (en & (1 << i)) {
			info[i].filp = bio_file_open(info[i].raw_data_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
			if (info[i].filp == NULL)
				BIO_INFO("open %s fail\n", info[i].raw_data_path);
			info[i].numOfData = readData[i];
			enBit |= info[i].enBit;
		} else {
			info[i].numOfData = 0;
		}
	}
	data.addr = 0x33;
	data.reg = 0x60;
	data.length = sizeof(enBit);
	data.data_buf = (uint8_t *) &enBit;
	vsm_driver_write_register(&data);
	/* mt2511_write(0x3360, enBit); */
	mutex_unlock(&bio_data_collection_mutex);

	return count;
}

static ssize_t show_data(struct device_driver *ddri, char *buf)
{
	while ((info[EKG].filp != NULL) || (info[PPG1].filp != NULL) || (info[PPG2].filp != NULL))
		msleep(100);
	return 0;
}

static ssize_t store_init(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, value;
	int num = sscanf(buf, "%x %x", &addr, &value);

	BIO_LOG("addr = %x, value = %x\n", addr, value);
	if (num == 2) {
		if (addr == 0x2330) {
			set_AFE_TCTRL_CON2 = 1;
			AFE_TCTRL_CON2 = value;
		} else if (addr == 0x2334) {
			set_AFE_TCTRL_CON3 = 1;
			AFE_TCTRL_CON3 = value;
		} else
			insert_modify_setting(addr, value);
	} else if (num == 1) {
		if (addr == 0) {
			clear_modify_setting();
			set_AFE_TCTRL_CON2 = 0;
			set_AFE_TCTRL_CON3 = 0;
		} else if (addr == 0x2330)
			set_AFE_TCTRL_CON2 = 0;
		else if (addr == 0x2334)
			set_AFE_TCTRL_CON3 = 0;
		else
			remove_modify_setting(addr, value);
	}

	update_new_init_setting();

	return count;
}

static ssize_t show_init(struct device_driver *ddri, char *buf)
{
	int i;
	char tmp_buf[100];
	int len = 0;

	strcpy(buf, "Modify settings:\n");
	if (set_AFE_TCTRL_CON2 == 1) {
		sprintf(tmp_buf, "0x2330, 0x%08x\n", AFE_TCTRL_CON2);
		strcat(buf, tmp_buf);
	}
	if (set_AFE_TCTRL_CON3 == 1) {
		sprintf(tmp_buf, "0x2334, 0x%08x\n", AFE_TCTRL_CON3);
		strcat(buf, tmp_buf);
	}
	for (i = 0; i < modify_array_len; i++) {
		sprintf(tmp_buf, "0x%x, 0x%08x\n", VSM_SIGNAL_MODIFY_array[i].addr, VSM_SIGNAL_MODIFY_array[i].value);
		strcat(buf, tmp_buf);
	}

	strcat(buf, "New init settings:\n");
	if (modify_array_len != 0) {
		for (i = 0; i < new_init_array_len; i++) {
			sprintf(tmp_buf, "0x%x, 0x%08x\n", VSM_SIGNAL_NEW_INIT_array[i].addr,
				VSM_SIGNAL_NEW_INIT_array[i].value);
			strcat(buf, tmp_buf);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(VSM_SIGNAL_INIT_array); i++) {
			sprintf(tmp_buf, "0x%x, 0x%08x\n", VSM_SIGNAL_INIT_array[i].addr,
				VSM_SIGNAL_INIT_array[i].value);
			strcat(buf, tmp_buf);
		}
	}

	len = strlen(buf);

	return len;
}

static ssize_t store_polling_delay(struct device_driver *ddri, const char *buf, size_t count)
{
	int delayTime = 0;
	int ret = 0;

	ret = kstrtoint(buf, 10, &delayTime);

	BIO_LOG("(%d)\n", delayTime);
	if (0 != ret && 0 <= delayTime)
		polling_delay = delayTime;

	return count;
}

static ssize_t show_polling_delay(struct device_driver *ddri, char *buf)
{
	return sprintf(buf, "polling delay = %d\n", polling_delay);
}

static ssize_t store_latency_test(struct device_driver *ddri, const char *buf, size_t count)
{
	int num = sscanf(buf, "%d %d %d", &latency_test_data.first_data,
		&latency_test_data.second_data, &latency_test_data.ekg_num);
	latency_test_data.ppg1_num = latency_test_data.ekg_num;
	latency_test_data.ppg2_num = latency_test_data.ekg_num;

	BIO_LOG("first_data = %d, second_data = %d, delay_num = %d\n", latency_test_data.first_data,
		latency_test_data.second_data, latency_test_data.ekg_num);

	if (num != 3) {
		latency_test_data.ekg_num = 0;
		latency_test_data.ppg1_num = 0;
		latency_test_data.ppg2_num = 0;
	}

	return count;
}

static DRIVER_ATTR(chipinfo, S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, NULL, store_trace_value);
static DRIVER_ATTR(rstb, S_IWUSR | S_IRUGO, NULL, store_rstb_value);
static DRIVER_ATTR(io, S_IRUGO | S_IWUSR, show_io_value, store_io_value);
static DRIVER_ATTR(delay, S_IRUGO | S_IWUSR, show_delay, store_delay);
static DRIVER_ATTR(data, S_IRUGO | S_IWUSR, show_data, store_data);
static DRIVER_ATTR(init, S_IRUGO | S_IWUSR, show_init, store_init);
static DRIVER_ATTR(polling_delay, S_IRUGO | S_IWUSR, show_polling_delay, store_polling_delay);
static DRIVER_ATTR(latency_test, S_IRUGO | S_IWUSR, NULL, store_latency_test);

static struct driver_attribute *mt6381_attr_list[] = {
	&driver_attr_chipinfo,	/*chip information */
	&driver_attr_trace,	/*trace log */
	&driver_attr_rstb,	/* set rstb */
	&driver_attr_io,
	&driver_attr_delay,
	&driver_attr_data,
	&driver_attr_init,
	&driver_attr_polling_delay,
	&driver_attr_latency_test,
};

static int mt6381_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = ARRAY_SIZE(mt6381_attr_list);

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, mt6381_attr_list[idx]);
		if (err != 0) {
			BIO_PR_ERR("driver_create_file (%s) = %d\n", mt6381_attr_list[idx]->attr.name,
				err);
			break;
		}
	}
	return err;
}

static int mt6381_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = ARRAY_SIZE(mt6381_attr_list);

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, mt6381_attr_list[idx]);

	return err;
}


/* mt6381 spec relevant */
enum vsm_status_t vsm_driver_read_register_batch(enum vsm_sram_type_t sram_type, u8 *buf,
						 u16 length)
{
	int res;
	/* ///int i; */
	struct i2c_msg *msg = NULL;

	switch (sram_type) {
	case VSM_SRAM_EKG:
		msg = ekg_msg;
		break;
	case VSM_SRAM_PPG1:
		msg = ppg1_msg;
		break;
	case VSM_SRAM_PPG2:
		msg = ppg2_msg;
		break;
	default:
		return -1;
	}

	res = i2c_transfer(mt6381_i2c_client->adapter, msg, length * 2);
	if (res < 0)
		BIO_PR_ERR("mt6381 i2c read failed. errno:%d\n", res);
	else
		memcpy(buf, msg[1].buf, length * 4);
/**
 * for (i = 0; i < length; i++) {
 *BIO_LOG("data[%d]:%d", i, *(u32 *)(buf + 4*i));
 * }
 * BIO_LOG("\n");
 */
	return res;
}

enum vsm_status_t vsm_driver_read_register(struct bus_data_t *data)
{
	int32_t ret = 0, i = 0;

	if (data == NULL) {
		pr_debug("NULL data parameter\n");
		return VSM_STATUS_INVALID_PARAMETER;
	}

	for (i = 0; i < 5; i++) {
		ret = mt6381_i2c_write_read(data->addr, data->reg, data->data_buf, data->length);
		if (ret == VSM_STATUS_OK)
			break;
	}

	if (DBG_READ) {
		pr_debug("%s():addr 0x%x, reg 0x%x, len %d, value 0x%x",
			 __func__, data->addr, data->reg, data->length,
			 *(uint32_t *) data->data_buf);
	}

	if (ret < 0) {
		BIO_PR_ERR("vsm_driver_read_register error (%d)\r\n", ret);
		return VSM_STATUS_ERROR;
	} else {
		return VSM_STATUS_OK;
	}

	return ret;
}

enum vsm_status_t vsm_driver_write_register(struct bus_data_t *data)
{
	unsigned char txbuffer[MAX_WRITE_LENGTH * 2];
	unsigned char reg_addr = data->reg;
	unsigned char data_len = data->length;
	int32_t ret, i = 0;

	if (data == NULL) {
		BIO_PR_ERR("NULL data parameter\n");
		return VSM_STATUS_INVALID_PARAMETER;
	}

	if (DBG_WRITE) {
		pr_debug("%s():addr 0x%x, reg 0x%x, value 0x%x, len %d",
			 __func__, data->addr, reg_addr, *(uint32_t *) (data->data_buf), data_len);
	}

	txbuffer[0] = reg_addr;
	memcpy(txbuffer + 1, data->data_buf, data_len);
	for (i = 0; i < 5; i++) {
		ret = mt6381_i2c_write(data->addr, txbuffer, data_len + 1);
		if (ret == (data_len + 1))
			break;

		BIO_LOG("mt6381_i2c_write error(%d), reg_addr = %x, reg_data = %x\n",
			ret, reg_addr, *(uint32_t *) (data->data_buf));
	}

	if (ret < 0) {
		BIO_PR_ERR("I2C Trasmit error(%d)\n", ret);
		return VSM_STATUS_ERROR;
	} else {
		return VSM_STATUS_OK;
	}
}

enum vsm_status_t vsm_driver_write_signal(struct signal_data_t *reg_addr, int32_t len,
					  uint32_t *enable_data)
{
	struct bus_data_t data;
	uint32_t data_buf;
	int32_t i = 0;
	enum vsm_status_t err = VSM_STATUS_OK;

	if (!reg_addr || !enable_data) {
		err = VSM_STATUS_INVALID_PARAMETER;
		return err;
	}

	for (i = 0; i < len; i++) {
		data.addr = ((reg_addr + i)->addr & 0xFF00) >> 8;
		data.reg = ((reg_addr + i)->addr & 0xFF);
		data.length = sizeof(data_buf);
		data.data_buf = (uint8_t *) &data_buf;

		/* process with combo signal */
		data_buf = (reg_addr + i)->value;
		data.length = sizeof(data_buf);
		if (DBG) {
			pr_debug
			    ("%d:vsm_driver_write, addr:0x%x, reg:0x%x, data 0x%x, length %d \r\n",
			     i, data.addr, data.reg, data_buf, data.length);
		}
		err = vsm_driver_write_register(&data);
		/* ms_delay(5); */
		if (((reg_addr + i)->addr == 0x3300) && ((reg_addr + i + 1)->addr == 0x3300))
			mdelay(50);
	}
	return err;
}

enum vsm_status_t vsm_driver_set_signal(enum vsm_signal_t signal)
{
	struct bus_data_t data;
	enum vsm_status_t err = VSM_STATUS_OK;
	int32_t len;
	struct signal_data_t *temp;
	uint32_t enable_data;
#if 0
	uint32_t signal_enable_data = 0;

	if (signal & VSM_SIGNAL_EKG) {
		len = ARRAY_SIZE(VSM_SIGNAL_EKG_array);
		temp = VSM_SIGNAL_EKG_array;
		vsm_driver_write_signal(temp, len, &enable_data);
		signal_enable_data |= enable_data;
	}

	if (signal & VSM_SIGNAL_PPG1) {
		len = ARRAY_SIZE(VSM_SIGNAL_PPG1_512HZ_array);
		temp = VSM_SIGNAL_PPG1_512HZ_array;
		vsm_driver_write_signal(temp, len, &enable_data);
		signal_enable_data |= enable_data;
	}

	if (signal & VSM_SIGNAL_PPG2) {
		len = ARRAY_SIZE(VSM_SIGNAL_PPG2_512HZ_array);
		temp = VSM_SIGNAL_PPG2_512HZ_array;
		vsm_driver_write_signal(temp, len, &enable_data);
		signal_enable_data |= enable_data;
	}

	pr_warn("signal_enable_data 0x%x, signal %d\r\n", signal_enable_data, signal);
	/*write 0x3360 to enable signal */

	/*enable signal, read first and write */
	data.addr = (DIGITAL_START_ADDR & 0xFF00) >> 8;
	data.reg = (DIGITAL_START_ADDR & 0xFF);
	data.length = sizeof(enable_data);
	data.data_buf = (uint8_t *) &enable_data;
	if (combo_usage)
		err = vsm_driver_read_register(&data);

	enable_data |= signal_enable_data;
	pr_warn("enable_data 0x%x, combo_usage %d\r\n", enable_data, combo_usage);
	err = vsm_driver_write_register(&data);

	combo_usage = true;
#else
	uint32_t value;
#if 0
	static int64_t first_enable_time;
	int64_t enable_time;
#endif

	if (current_signal == 0) {
		/* reset digital*/
		if (!IS_ERR(pinctrl_gpios)) {
			pinctrl_select_state(pinctrl_gpios, bio_pins_reset_low);
			msleep(20);
			pinctrl_select_state(pinctrl_gpios, bio_pins_reset_high);
		}
		/* initial setting */
		if (new_init_array_len != 0) {
			len = new_init_array_len;
			temp = VSM_SIGNAL_NEW_INIT_array;
		} else {
			len = ARRAY_SIZE(VSM_SIGNAL_INIT_array);
			temp = VSM_SIGNAL_INIT_array;
		}
		vsm_driver_write_signal(temp, len, &enable_data);
		enable_time = sched_clock();
		previous_timestamp[VSM_SRAM_EKG] = sched_clock();
		previous_timestamp[VSM_SRAM_PPG1] = sched_clock();
		previous_timestamp[VSM_SRAM_PPG2] = sched_clock();
		ppg_control_init();
		data_dropped = false;
		numOfData[VSM_SRAM_EKG] = 0;
		numOfData[VSM_SRAM_PPG1] = 0;
		numOfData[VSM_SRAM_PPG2] = 0;
		numOfEKGDataNeedToDrop = 2;
		ppg1_buf2_len = 0;
		ppg2_buf2_len = 0;
		agc_ppg1_buf_len = 0;
		agc_ppg2_buf_len = 0;
		if (latency_test_data.ekg_num != 0) {
			latency_test_data.ppg1_num = latency_test_data.ekg_num;
			latency_test_data.ppg2_num = latency_test_data.ekg_num;
			latency_test_data.in_latency_test = true;
		} else
			latency_test_data.in_latency_test = false;
	}

	current_signal |= signal;

	if (signal == VSM_SIGNAL_PPG1) {
		data.addr = (0x2330 & 0xFF00) >> 8;
		data.reg = (0x2330 & 0xFF);
		if (set_AFE_TCTRL_CON2 == 1)
			value = AFE_TCTRL_CON2;
		else
			value = 0x00C10182;
		data.length = sizeof(value);
		data.data_buf = (uint8_t *) &value;
		err = vsm_driver_write_register(&data);
	} else if (signal == VSM_SIGNAL_PPG2) {
		data.addr = (0x2334 & 0xFF00) >> 8;
		data.reg = (0x2334 & 0xFF);
		if (set_AFE_TCTRL_CON3 == 1)
			value = AFE_TCTRL_CON3;
		else
			value = 0x02430304;
		data.length = sizeof(value);
		data.data_buf = (uint8_t *) &value;
		err = vsm_driver_write_register(&data);
	}

	data.addr = (0x2328 & 0xFF00) >> 8;
	data.reg = (0x2328 & 0xFF);
	value = 0xFFFF0002;
	data.length = sizeof(value);
	data.data_buf = (uint8_t *) &value;
	err = vsm_driver_write_register(&data);

	data.addr = (0x2328 & 0xFF00) >> 8;
	data.reg = (0x2328 & 0xFF);
	value = 0xFFFF0000;
	data.length = sizeof(value);
	data.data_buf = (uint8_t *) &value;
	err = vsm_driver_write_register(&data);

#endif
	return err;
}

static enum vsm_status_t vsm_driver_set_read_counter(enum vsm_sram_type_t sram_type, uint32_t *counter)
{
	int err = VSM_STATUS_OK;
	struct bus_data_t data;

	if (counter == NULL) {
		err = VSM_STATUS_INVALID_PARAMETER;
		return err;
	}

	switch (sram_type) {
	case VSM_SRAM_EKG:
		data.reg = SRAM_EKG_READ_COUNT_ADDR;
		break;

	case VSM_SRAM_PPG1:
		data.reg = SRAM_PPG1_READ_COUNT_ADDR;
		break;

	case VSM_SRAM_PPG2:
		data.reg = SRAM_PPG2_READ_COUNT_ADDR;
		break;

	case VSM_SRAM_BISI:
		data.reg = SRAM_BISI_READ_COUNT_ADDR;
		break;

	case VSM_SRAM_NUMBER:
	default:
		err = VSM_STATUS_INVALID_PARAMETER;
		return err;
	}

	*counter |= 0x60000000;
	data.addr = MT2511_SLAVE_II;
	data.data_buf = (uint8_t *) counter;
	data.length = sizeof(uint32_t);
	err = vsm_driver_write_register(&data);

	if (err != VSM_STATUS_OK)
		BIO_PR_ERR("vsm_driver_set_read_counter fail : %d\n", err);

	return err;
}

static enum vsm_status_t vsm_driver_get_read_counter(enum vsm_sram_type_t sram_type, uint32_t *counter)
{
	int err = VSM_STATUS_OK;
	struct bus_data_t data;

	if (counter == NULL) {
		err = VSM_STATUS_INVALID_PARAMETER;
		return err;
	}

	switch (sram_type) {
	case VSM_SRAM_EKG:
		data.reg = SRAM_EKG_READ_COUNT_ADDR;
		break;

	case VSM_SRAM_PPG1:
		data.reg = SRAM_PPG1_READ_COUNT_ADDR;
		break;

	case VSM_SRAM_PPG2:
		data.reg = SRAM_PPG2_READ_COUNT_ADDR;
		break;

	case VSM_SRAM_BISI:
		data.reg = SRAM_BISI_READ_COUNT_ADDR;
		break;

	case VSM_SRAM_NUMBER:
	default:
		err = VSM_STATUS_INVALID_PARAMETER;
		return err;
	}

	data.addr = MT2511_SLAVE_II;
	data.data_buf = (uint8_t *) counter;
	data.length = sizeof(uint32_t);
	err = vsm_driver_read_register(&data);

	if (err == VSM_STATUS_OK) {
		*counter =
		    ((*counter & 0x01ff0000) >> 16) >
		    VSM_SRAM_LEN ? 0 : ((*counter & 0x01ff0000) >> 16);
		if (DBG)
			pr_debug("vsm_driver_get_read_counter 0x%x \r\n", *counter);
	}
	return err;
}


static enum vsm_status_t vsm_driver_write_counter(enum vsm_sram_type_t sram_type,
						  uint32_t *write_counter)
{
	int err = VSM_STATUS_OK;
	struct bus_data_t data;
	uint32_t counter = 0;

	if (write_counter == NULL) {
		err = VSM_STATUS_INVALID_PARAMETER;
		return err;
	}

	switch (sram_type) {
	case VSM_SRAM_EKG:
		data.reg = SRAM_EKG_WRITE_COUNT_ADDR;
		break;

	case VSM_SRAM_PPG1:
		data.reg = SRAM_PPG1_WRITE_COUNT_ADDR;
		break;

	case VSM_SRAM_PPG2:
		data.reg = SRAM_PPG2_WRITE_COUNT_ADDR;
		break;

	case VSM_SRAM_BISI:
		data.reg = SRAM_BISI_WRITE_COUNT_ADDR;
		break;

	case VSM_SRAM_NUMBER:
	default:
		err = VSM_STATUS_INVALID_PARAMETER;
		return err;
	}

	data.addr = MT2511_SLAVE_II;
	data.data_buf = (uint8_t *) &counter;
	data.length = sizeof(uint32_t);
	err = vsm_driver_read_register(&data);

	if (err == VSM_STATUS_OK) {
		counter = (counter & 0x01ff0000) >> 16;
		if (DBG)
			pr_debug("vsm_driver_write_counter 0x%x \r\n", counter);
	}
	*write_counter = counter;

	return err;
}

int32_t vsm_driver_check_sample_rate(enum vsm_sram_type_t sram_type)
{
	int32_t sample_rate, err = 0;
	struct bus_data_t data;

	switch (sram_type) {
	case VSM_SRAM_EKG:
		{
			int32_t ekg_sample_data1, ekg_sample_data2;

			data.addr = (EKG_SAMPLE_RATE_ADDR1 & 0xFF00) >> 8;
			data.reg = (EKG_SAMPLE_RATE_ADDR1 & 0xFF);
			data.data_buf = (uint8_t *) &ekg_sample_data1;
			data.length = sizeof(ekg_sample_data1);
			err = vsm_driver_read_register(&data);
			if (err == VSM_STATUS_OK) {
				data.addr = (EKG_SAMPLE_RATE_ADDR2 & 0xFF00) >> 8;
				data.reg = (EKG_SAMPLE_RATE_ADDR2 & 0xFF);
				data.data_buf = (uint8_t *) &ekg_sample_data2;
				err = vsm_driver_read_register(&data);
#if 0
				pr_debug("ekg_sample_data1 0x%x, ekg_sample_data2 0x%x",
						ekg_sample_data1, ekg_sample_data2);
#endif
				if (err == VSM_STATUS_OK) {
					if ((ekg_sample_data1 & 0x7) == 0
					    && ((ekg_sample_data2 & 0x1C0000) >> 18) == 0) {
						sample_rate = 64;
					} else if ((ekg_sample_data1 & 0x7) == 1
						   && ((ekg_sample_data2 & 0x1C0000) >> 18) == 1) {
						sample_rate = 128;
					} else if ((ekg_sample_data1 & 0x7) == 2
						   && ((ekg_sample_data2 & 0x1C0000) >> 18) == 2) {
						sample_rate = 256;
					} else if ((ekg_sample_data1 & 0x7) == 3
						   && ((ekg_sample_data2 & 0x1C0000) >> 18) == 3) {
						sample_rate = 512;
					} else if ((ekg_sample_data1 & 0x7) == 4
						   && ((ekg_sample_data2 & 0x1C0000) >> 18) == 3) {
						sample_rate = 1024;
					} else if ((ekg_sample_data1 & 0x7) == 5
						   && ((ekg_sample_data2 & 0x1C0000) >> 18) == 3) {
						sample_rate = 2048;
					} else if ((ekg_sample_data1 & 0x7) == 6
						   && ((ekg_sample_data2 & 0x1C0000) >> 18) == 3) {
						sample_rate = 4096;
					} else {
						sample_rate = EKG_DEFAULT_SAMPLE_RATE;
					}
				} else {
					sample_rate = EKG_DEFAULT_SAMPLE_RATE;
				}
			} else {
				sample_rate = EKG_DEFAULT_SAMPLE_RATE;
			}
		}
		break;

	case VSM_SRAM_PPG1:
	case VSM_SRAM_PPG2:
		{
			uint32_t ppg_sample_data;

			data.addr = (PPG_SAMPLE_RATE_ADDR & 0xFF00) >> 8;
			data.reg = (PPG_SAMPLE_RATE_ADDR & 0xFF);
			data.data_buf = (uint8_t *) &ppg_sample_data;
			data.length = sizeof(ppg_sample_data);
			err = vsm_driver_read_register(&data);
			if (err == VSM_STATUS_OK) {
				pr_debug("ppg_sample_data 0x%x\r\n", ppg_sample_data);
				sample_rate = PPG_FSYS / ((ppg_sample_data & 0x3FFF) + 1);
			} else {
				BIO_INFO("ppg_sample_data i2c error, err %d\r\n", err);
				sample_rate = PPG_DEFAULT_SAMPLE_RATE;
			}
		}
		break;
	case VSM_SRAM_BISI:
	case VSM_SRAM_NUMBER:
	default:
		return VSM_STATUS_INVALID_PARAMETER;
	}
	return sample_rate;
}

enum vsm_status_t vsm_driver_check_write_counter(enum vsm_sram_type_t sram_type,
						 uint32_t read_counter, uint32_t *write_counter)
{
	int i = 0;
	u64 *pre_timestamp, cur_timestamp;
	uint32_t sample_rate, err = VSM_STATUS_OK;
	uint32_t expected_counter = 0, real_counter = 0;

	if (write_counter == NULL)
		return VSM_STATUS_INVALID_PARAMETER;

	switch (sram_type) {
	case VSM_SRAM_EKG:
		pre_timestamp = &pre_ekg_timestamp;
		sample_rate = vsm_driver_check_sample_rate(sram_type);
		break;

	case VSM_SRAM_PPG1:
		pre_timestamp = &pre_ppg1_timestamp;
		sample_rate = vsm_driver_check_sample_rate(sram_type);
		break;

	case VSM_SRAM_PPG2:
		pre_timestamp = &pre_ppg2_timestamp;
		sample_rate = vsm_driver_check_sample_rate(sram_type);
		break;

	case VSM_SRAM_BISI:
		vsm_driver_write_counter(sram_type, write_counter);
		return VSM_STATUS_OK;

	case VSM_SRAM_NUMBER:
	default:
		err = VSM_STATUS_INVALID_PARAMETER;
		return err;
	}

	/* cur_timestamp = xTaskGetTickCount(); */
	/* ///cur_timestamp = vsm_driver_get_ms_tick(); */
	cur_timestamp = getCurNS();
	if (*pre_timestamp == 0) {
		err = vsm_driver_write_counter(sram_type, write_counter);
	} else {
		expected_counter =
		    (uint32_t) (((cur_timestamp - *pre_timestamp) * sample_rate) / 1000000000);
		for (i = 0; i < 10; i++) {
			vsm_driver_write_counter(sram_type, write_counter);
			real_counter =
			    (*write_counter >=
			     read_counter) ? (*write_counter - read_counter) : (VSM_SRAM_LEN -
										read_counter +
										*write_counter);
			if (sram_type == VSM_SRAM_PPG1 || sram_type == VSM_SRAM_PPG2)
				real_counter /= 2;

			pr_debug("sample_rate %u, real_counter %u,expected_counter %u", sample_rate,
				 real_counter, expected_counter);
			if (real_counter < (20 + expected_counter)
			    && real_counter > (expected_counter >
					       20 ? (expected_counter - 20) : 0)) {
				break;
			}
			pr_debug("try %d times", i);
		}
	}
	pr_debug("%s():cur_timestamp %llu, pre_timestamp %llu", __func__, cur_timestamp,
		 *pre_timestamp);
	*pre_timestamp = cur_timestamp;
	return err;
}

#define READ_DEBUG
enum vsm_status_t vsm_driver_read_sram(enum vsm_sram_type_t sram_type, uint32_t *data_buf, uint32_t *amb_buf,
				       u32 *len)
{
	int err = VSM_STATUS_OK;
	uint32_t temp;
	uint32_t read_counter = 0;
	uint32_t write_counter = 0;
	uint32_t amb_read_counter = 0;
	uint32_t sram_len;
	int64_t current_timestamp;
	static int64_t rate[3] = {0};
	uint32_t i;
	static uint32_t pre_amb_data;

	if (data_buf == NULL || len == NULL) {
		err = VSM_STATUS_INVALID_PARAMETER;
		return err;
	}
	/* 1. compute how many sram data */
	/* read counter */
	vsm_driver_get_read_counter(sram_type, &read_counter);
	do {
		temp = read_counter;
		vsm_driver_get_read_counter(sram_type, &read_counter);
	} while (temp != read_counter);

	/* write counter */
	/* vsm_driver_check_write_counter(sram_type, read_counter, &write_counter); */
	vsm_driver_write_counter(sram_type, &write_counter);
	do {
		temp = write_counter;
		vsm_driver_write_counter(sram_type, &write_counter);
	} while (temp != write_counter);

	/* sram_write_counter = ((register_val & 0x01ff0000) >> 16); // bit 25 ~ bit 16 */
	/* vm_log_info("w_check: sram_write_counter %d, sram_type %d", sram_write_counter, sram_type); */
	/* irq_th = register_val & 0x1ff; */
	/* only get even number to prevent the reverse of ppg and ambient */
	/* compute sram length */
	sram_len =
	    (write_counter >=
	     read_counter) ? (write_counter - read_counter) : (VSM_SRAM_LEN - read_counter +
							       write_counter);
	sram_len = ((sram_len % 2) == 0) ? sram_len : sram_len - 1;

	*len = sram_len;

	current_timestamp = sched_clock();
	/* sram will wraparound after 375000000ns = VSM_SRAM_LEN * (1000000000 / 1024) */
	if (((current_timestamp - previous_timestamp[sram_type]) >= 350000000LL) ||
		(((current_timestamp - previous_timestamp[sram_type]) >= 300000000LL) && sram_len < 100))
		aee_kernel_warning("MT6381", "Data dropped!! %d, %lld, %d\n", sram_type,
			current_timestamp - previous_timestamp[sram_type], sram_len);
	if (atomic_read(&bio_trace) != 0)
		BIO_LOG("Data read, %d, %lld, %d\n", sram_type,
			current_timestamp - previous_timestamp[sram_type], sram_len);
	previous_timestamp[sram_type] = current_timestamp;

	if (sram_len > 0) {
		if (sram_type == VSM_SRAM_EKG) {
			/* drop fisrt two garbage bytes of EKG data after enable */
			while (numOfEKGDataNeedToDrop > 0 && sram_len > 0) {
				err = vsm_driver_read_register_batch(sram_type, (u8 *) data_buf, 1);
				if (err < 0) {
					err = VSM_STATUS_INVALID_PARAMETER;
					*len = sram_len = 0;
				}
				numOfEKGDataNeedToDrop--;
				sram_len--;
			}

			*len = sram_len;
			if (sram_len <= 0)
				return err;
		}
		numOfData[sram_type] += sram_len;
		rate[sram_type] = numOfData[sram_type] * 1000000000 / (current_timestamp - enable_time);

		/* 2. get sram data to data_buf */
		/* get sram data */
		err = vsm_driver_read_register_batch(sram_type, (u8 *) data_buf, sram_len);
		if (err < 0) {
			err = VSM_STATUS_INVALID_PARAMETER;
			*len = sram_len = 0;
		}

		/* Read out ambient data */
		if (sram_type == VSM_SRAM_PPG2) {
			amb_read_counter = read_counter;
			for (i = 0; i < sram_len; i++) {
				/* down sample from 512Hz to 16Hz */
				if (amb_read_counter % 64 == 0) {
					vsm_driver_set_read_counter(VSM_SRAM_PPG1, &amb_read_counter);
					vsm_driver_read_register_batch(VSM_SRAM_PPG1, (u8 *)&amb_buf[i], 1);
					pre_amb_data = amb_buf[i];

					vsm_driver_get_read_counter(VSM_SRAM_PPG1, &read_counter);
					do {
						temp = read_counter;
						vsm_driver_get_read_counter(VSM_SRAM_PPG1, &read_counter);
					} while (temp != read_counter);
				} else
					amb_buf[i] = pre_amb_data;
				amb_read_counter = (amb_read_counter + 1) % VSM_SRAM_LEN;
			}
		}

		if (atomic_read(&bio_trace) != 0)
			BIO_LOG("%s =====> sram_type: %d, sram_len,%d,%d,%d,%lld,delta : %lld, %lld, %lld, %lld\n",
					__func__, sram_type, sram_len, read_counter, write_counter,
					rate[sram_type], sched_clock() - current_timestamp,
					numOfData[0], numOfData[1], numOfData[2]);
	} else {
		err = VSM_STATUS_INVALID_PARAMETER;
	}

	return err;
}

enum vsm_status_t vsm_driver_update_register(void)
{
	uint32_t write_data;
	struct bus_data_t data;
	int err = VSM_STATUS_OK;

	data.addr = (UPDATE_COMMAND_ADDR & 0xFF00) >> 8;
	data.reg = (UPDATE_COMMAND_ADDR & 0xFF);
	write_data = (uint32_t) 0xFFFF0002;
	data.data_buf = (uint8_t *) &write_data;
	data.length = sizeof(write_data);

	err = vsm_driver_write_register(&data);
	if (err == VSM_STATUS_OK) {
		write_data = 0xFFFF0000;
		err = vsm_driver_write_register(&data);
	}
	return err;
}

enum vsm_status_t vsm_driver_set_tia_gain(enum vsm_signal_t ppg_type, enum vsm_tia_gain_t input)
{
	int err = VSM_STATUS_OK;
	struct bus_data_t data;
	uint32_t read_data;

	if (ppg_type == VSM_SIGNAL_PPG1 || ppg_type == VSM_SIGNAL_PPG2) {

		switch (ppg_type) {
		case VSM_SIGNAL_PPG1:
			data.reg = (PPG1_GAIN_ADDR & 0xFF);
			data.addr = (PPG1_GAIN_ADDR & 0xFF00) >> 8;
			break;
		case VSM_SIGNAL_PPG2:
			data.reg = (PPG2_GAIN_ADDR & 0xFF);
			data.addr = (PPG2_GAIN_ADDR & 0xFF00) >> 8;
			break;
		default:
			data.reg = (PPG1_GAIN_ADDR & 0xFF);
			data.addr = (PPG1_GAIN_ADDR & 0xFF00) >> 8;
			break;
		}

		data.data_buf = (uint8_t *) &read_data;
		data.length = sizeof(read_data);
		err = vsm_driver_read_register(&data);

		if (err == VSM_STATUS_OK) {
			if (ppg_type == VSM_SIGNAL_PPG1) {
				read_data &= ~PPG1_GAIN_MASK;
				read_data |= (input & 0x7) << PPG1_GAIN_OFFSET;
			} else if (ppg_type == VSM_SIGNAL_PPG2) {
				read_data &= ~PPG2_GAIN_MASK;
				read_data |= (input & 0x7) << PPG2_GAIN_OFFSET;
			}
			err = vsm_driver_write_register(&data);
			/* update register setting */
			vsm_driver_update_register();
		}
	} else {
		err = VSM_STATUS_INVALID_PARAMETER;
	}

	return err;
}

enum vsm_status_t vsm_driver_set_pga_gain(enum vsm_pga_gain_t input)
{
	int err = VSM_STATUS_OK;
	struct bus_data_t data;
	uint32_t read_data;

	data.reg = (PPG_PGA_GAIN_ADDR & 0xFF);
	data.addr = (PPG_PGA_GAIN_ADDR & 0xFF00) >> 8;
	data.data_buf = (uint8_t *) &read_data;
	data.length = sizeof(read_data);
	err = vsm_driver_read_register(&data);

	if (err == VSM_STATUS_OK) {
		if (input > VSM_PGA_GAIN_6)
			input = VSM_PGA_GAIN_6;
		read_data &= ~PPG_PGA_GAIN_MASK;
		read_data |= (input & 0x7) << PPG_PGA_GAIN_OFFSET;
		err = vsm_driver_write_register(&data);
		/* update register setting */
		vsm_driver_update_register();
	}

	return err;
}

enum vsm_status_t vsm_driver_set_led_current(enum vsm_led_type_t led_type,
					     enum vsm_signal_t ppg_type, uint32_t input)
{
	int err = VSM_STATUS_OK;
	struct bus_data_t data;
	uint32_t read_data;

	if (led_type == VSM_LED_1 || led_type == VSM_LED_2) {
		switch (ppg_type) {
		case VSM_SIGNAL_PPG1:
			data.reg = (PPG1_CURR_ADDR & 0xFF);
			data.addr = (PPG1_CURR_ADDR & 0xFF00) >> 8;
			break;
		case VSM_SIGNAL_PPG2:
			data.reg = (PPG2_CURR_ADDR & 0xFF);
			data.addr = (PPG2_CURR_ADDR & 0xFF00) >> 8;
			break;
		default:
			data.reg = (PPG1_CURR_ADDR & 0xFF);
			data.addr = (PPG1_CURR_ADDR & 0xFF00) >> 8;
			break;
		}

		data.data_buf = (uint8_t *) &read_data;
		data.length = sizeof(read_data);
		err = vsm_driver_read_register(&data);

		if (err == VSM_STATUS_OK) {
			if (led_type == VSM_LED_1) {
				read_data &= ~PPG1_CURR_MASK;
				read_data |= (input & 0xFF) << PPG1_CURR_OFFSET;
			} else if (led_type == VSM_LED_2) {
				read_data &= ~PPG2_CURR_MASK;
				read_data |= (input & 0xFF) << PPG2_CURR_OFFSET;
			}
			err = vsm_driver_write_register(&data);
			/* update register setting */
			vsm_driver_update_register();
		}
	} else {
		err = VSM_STATUS_INVALID_PARAMETER;
	}

	return err;
}

int32_t vsm_driver_chip_version_get(void)
{
	int err = VSM_STATUS_OK;
	struct bus_data_t data;
	uint32_t read_data;

	if (vsm_chip_version == -1) {
		data.reg = (CHIP_VERSION_ADDR & 0xFF);
		data.addr = (CHIP_VERSION_ADDR & 0xFF00) >> 8;
		data.data_buf = (uint8_t *) &read_data;
		data.length = sizeof(read_data);
		err = vsm_driver_read_register(&data);
		if (err == VSM_STATUS_OK) {
			pr_debug("read back chip version:0x%x", read_data);
			if (read_data == 0x25110000)
				vsm_chip_version = CHIP_VERSION_E2;
			else if (read_data == 0xFFFFFFFF)
				vsm_chip_version = CHIP_VERSION_E1;
			else
				vsm_chip_version = CHIP_VERSION_UNKNOWN;
		} else {
			vsm_chip_version = CHIP_VERSION_UNKNOWN;
		}
	}

	return vsm_chip_version;
}

enum vsm_status_t vsm_driver_set_ambdac_current(enum vsm_ambdac_type_t ambdac_type,
						enum vsm_ambdac_current_t currentt)
{
	int err = VSM_STATUS_OK;
	struct bus_data_t data;
	uint32_t read_data;

	data.reg = (PPG_AMDAC_ADDR & 0xFF);
	data.addr = (PPG_AMDAC_ADDR & 0xFF00) >> 8;
	data.data_buf = (uint8_t *) &read_data;
	data.length = sizeof(read_data);
	err = vsm_driver_read_register(&data);

	if (err == VSM_STATUS_OK) {
		if (currentt > VSM_AMBDAC_CURR_06_MA)
			currentt = VSM_AMBDAC_CURR_06_MA;
		if (vsm_driver_chip_version_get() == CHIP_VERSION_E1) {
			read_data &= ~PPG_AMDAC_MASK;
			read_data |= (currentt & 0xF) << PPG_AMDAC_OFFSET;
			err = vsm_driver_write_register(&data);
		} else if (vsm_driver_chip_version_get() == CHIP_VERSION_E2) {
			if (ambdac_type == VSM_AMBDAC_1) {
				read_data &= ~PPG_AMDAC1_MASK;
				read_data |= (currentt & 0x7) << PPG_AMDAC1_OFFSET;
				err = vsm_driver_write_register(&data);
			} else if (ambdac_type == VSM_AMBDAC_2) {
				read_data &= ~PPG_AMDAC2_MASK;
				read_data |= (currentt & 0x7) << PPG_AMDAC2_OFFSET;
				err = vsm_driver_write_register(&data);
			}
		}
		/* update register setting */
		vsm_driver_update_register();
	}
	return err;
}

enum vsm_status_t vsm_driver_enable_signal(enum vsm_signal_t signal)
{
	enum vsm_status_t err = VSM_STATUS_OK;
	struct bus_data_t data;
	uint32_t enable_data, reg_data;

	/* {0x3360,0x00000187}   //Enable Mode[0:5]=[018:124:187:164:33C:3FF] */
	switch (signal) {
	case VSM_SIGNAL_EKG:
	case VSM_SIGNAL_EEG:
	case VSM_SIGNAL_EMG:
	case VSM_SIGNAL_GSR:
		enable_data = 0x18;
		break;
	case VSM_SIGNAL_PPG1:
	case VSM_SIGNAL_PPG1_512HZ:
		/* enable_data = 0x124; */
		enable_data = 0x124;
		break;
	case VSM_SIGNAL_PPG2:
		/* enable_data = 0x144; */
		enable_data = 0x144;
		break;
	case VSM_SIGNAL_BISI:
		/* enable_data = 0x187; */
		enable_data = 0x187;
		break;
	default:
		enable_data = 0x00;
		break;
	}
	data.addr = (DIGITAL_START_ADDR & 0xFF00) >> 8;
	data.reg = (DIGITAL_START_ADDR & 0xFF);
	data.data_buf = (uint8_t *) &reg_data;
	data.length = sizeof(reg_data);
	err = vsm_driver_read_register(&data);
	if (err == VSM_STATUS_OK) {
		reg_data |= (enable_data);
		err = vsm_driver_write_register(&data);
	}

	return err;
}

enum vsm_status_t vsm_driver_disable_signal(enum vsm_signal_t signal)
{
	int32_t len;
	struct signal_data_t *temp;
	uint32_t enable_data;
	enum vsm_status_t err = VSM_STATUS_OK;
	struct bus_data_t data;
	uint32_t reg_data;
#if 0
	/* {0x3360,0x00000187}   //Enable Mode[0:5]=[018:124:187:164:33C:3FF] */
	switch (signal) {
	case VSM_SIGNAL_EKG:
	case VSM_SIGNAL_EEG:
	case VSM_SIGNAL_EMG:
	case VSM_SIGNAL_GSR:
		enable_data = 0x18;
		break;
	case VSM_SIGNAL_PPG1:
	case VSM_SIGNAL_PPG1_512HZ:
		enable_data = 0x124;
		/* enable_data = 0x24; */
		break;
	case VSM_SIGNAL_PPG2:
		enable_data = 0x144;
		/* enable_data = 0x44; */
		break;
	default:
		enable_data = 0x00;
		break;
	}
	data.addr = (DIGITAL_START_ADDR & 0xFF00) >> 8;
	data.reg = (DIGITAL_START_ADDR & 0xFF);
	data.data_buf = (uint8_t *) &reg_data;
	data.length = sizeof(reg_data);
	err = vsm_driver_read_register(&data);
	if (err == VSM_STATUS_OK) {
		reg_data &= (~enable_data);
		if (reg_data == 0) {
			BIO_LOG("all sensor disabled\n");
			len = ARRAY_SIZE(VSM_SIGNAL_IDLE_array);
			temp = VSM_SIGNAL_IDLE_array;
			vsm_driver_write_signal(temp, len, &enable_data);
		}
		err = vsm_driver_write_register(&data);
	}
#else
	current_signal &= ~signal;

	if (signal == VSM_SIGNAL_PPG1) {
		/* turn off led */
		data.addr = (0x2330 & 0xFF00) >> 8;
		data.reg = (0x2330 & 0xFF);
		reg_data = 0x0;
		data.length = sizeof(reg_data);
		data.data_buf = (uint8_t *) &reg_data;
		err = vsm_driver_write_register(&data);
	} else if (signal == VSM_SIGNAL_PPG2) {
		/* turn off led */
		data.addr = (0x2334 & 0xFF00) >> 8;
		data.reg = (0x2334 & 0xFF);
		reg_data = 0x0;
		data.length = sizeof(reg_data);
		data.data_buf = (uint8_t *) &reg_data;
		err = vsm_driver_write_register(&data);
	}

	data.addr = (0x2328 & 0xFF00) >> 8;
	data.reg = (0x2328 & 0xFF);
	reg_data = 0xFFFF0002;
	data.length = sizeof(reg_data);
	data.data_buf = (uint8_t *) &reg_data;
	err = vsm_driver_write_register(&data);

	data.addr = (0x2328 & 0xFF00) >> 8;
	data.reg = (0x2328 & 0xFF);
	reg_data = 0xFFFF0000;
	data.length = sizeof(reg_data);
	data.data_buf = (uint8_t *) &reg_data;
	err = vsm_driver_write_register(&data);

	if (current_signal == 0) {
		len = ARRAY_SIZE(VSM_SIGNAL_IDLE_array);
		temp = VSM_SIGNAL_IDLE_array;
		vsm_driver_write_signal(temp, len, &enable_data);
	}
#endif
	return err;
}

enum vsm_signal_t vsm_driver_reset_PPG_counter(void)
{
	enum vsm_status_t err = VSM_STATUS_OK;
	struct bus_data_t data;
	uint32_t /*enable_data, */ reg_data = 0;
	/* step 1: (disable PPG function and reset PPG1 and PPG2 write counters to 0) */
	data.addr = (DIGITAL_START_ADDR & 0xFF00) >> 8;
	data.reg = (DIGITAL_START_ADDR & 0xFF);
	data.data_buf = (uint8_t *) &reg_data;
	data.length = sizeof(reg_data);
	err = vsm_driver_write_register(&data);
	if (err == VSM_STATUS_OK) {
		/* step 2: 0x33D0 = 0x2000_0000 (reset PPG1 read counter to 0) */
		data.reg = SRAM_PPG1_READ_COUNT_ADDR;
		reg_data = 0x60000000;
		err = vsm_driver_write_register(&data);
		if (err == VSM_STATUS_OK) {
			/* step 3: 0x33E0 = 0x2000_0000 (reset PPG2 read counter to 0) */
			data.reg = SRAM_PPG2_READ_COUNT_ADDR;
			reg_data = 0x60000000;
			err = vsm_driver_write_register(&data);
		}
	}
	return err;
}

int mt6381_enable_ekg(int en)
{
	enum vsm_status_t err = VSM_STATUS_OK;

	BIO_VER("%s en:%d\n", __func__, en);

	mutex_lock(&op_lock);
	if (en)
		err = vsm_driver_set_signal(VSM_SIGNAL_EKG);
	else
		err = vsm_driver_disable_signal(VSM_SIGNAL_EKG);
	mutex_unlock(&op_lock);

	return err;
}

int mt6381_enable_ppg1(int en)
{
	enum vsm_status_t err = VSM_STATUS_OK;

	BIO_VER("%s en:%d\n", __func__, en);

	mutex_lock(&op_lock);
	if (en)
		err = vsm_driver_set_signal(VSM_SIGNAL_PPG1);
	else
		err = vsm_driver_disable_signal(VSM_SIGNAL_PPG1);
	mutex_unlock(&op_lock);

	return err;
}

int mt6381_enable_ppg2(int en)
{
	enum vsm_status_t err = VSM_STATUS_OK;

	BIO_VER("%s en:%d\n", __func__, en);

	mutex_lock(&op_lock);
	if (en)
		err = vsm_driver_set_signal(VSM_SIGNAL_PPG2);
	else
		err = vsm_driver_disable_signal(VSM_SIGNAL_PPG2);
	mutex_unlock(&op_lock);

	return err;
}

int mt6381_set_delay_ekg(u64 delay)
{
	return 0;
}

int mt6381_set_delay_ppg1(u64 delay)
{
	return 0;
}

int mt6381_set_delay_ppg2(u64 delay)
{
	return 0;
}


/* ///uint32_t sram_data[VSM_SRAM_LEN]; */

int mt6381_get_data_ekg(int *raw_data, u32 *length)
{
	int i;
	/* ///int32_t output_len; */

	/* ///memset(sram_data, 0, sizeof(sram_data)); */
	mutex_lock(&op_lock);
	/* avoid to accumulate numOfData[] */
	if (current_signal & VSM_SIGNAL_EKG)
		vsm_driver_read_sram(VSM_SRAM_EKG, raw_data, NULL, length);
	else
		*length = 0;

	for (i = 0; i < *length; i++) {
		if (latency_test_data.in_latency_test) {
			/* verify only */
			if (latency_test_data.ekg_num > 0) {
				raw_data[i] = latency_test_data.first_data;
				latency_test_data.ekg_num--;
			} else
				raw_data[i] = latency_test_data.second_data;
		} else {
			if (raw_data[i] >= 0x400000)
				raw_data[i] = raw_data[i] - 0x800000;
		}
	}

	mutex_unlock(&op_lock);
	return 0;
}

int mt6381_get_data_ppg1(int *raw_data, int *amb_data, int *agc_data, u32 *length)
{
	int i;
	int32_t ppg_control_fs = 16;            /* The sampling frequency of PPG input (Support 125Hz only). */
	int32_t ppg_control_cfg = 1;             /* The input configuration, default value is 1. */
	int32_t ppg_control_src = 1;             /* The input source, default value is 1 (PPG channel 1). */
	struct ppg_control_t ppg1_control_input;           /* Input structure for the #ppg_control_process(). */
	int64_t numOfSRAMPPG2Data;
	u32 leddrv_con1;

	mutex_lock(&op_lock);
	numOfSRAMPPG2Data = numOfData[VSM_SRAM_PPG2];
	/* avoid to accumulate numOfData[] */
	if (current_signal & VSM_SIGNAL_PPG1) {
		mt6381_i2c_write_read(0x33, 0x2C, (u8 *)&leddrv_con1, 4);
		/* PPG1 data is stored in sram ppg2 as well */
		vsm_driver_read_sram(VSM_SRAM_PPG2, temp_buf, amb_temp_buf, length);
		for (i = 0; i < *length; i += 2) {
			temp_buf[i] = temp_buf[i] >= 0x400000 ? temp_buf[i] - 0x800000 : temp_buf[i];
			temp_buf[i + 1] = temp_buf[i + 1] >= 0x400000 ? temp_buf[i + 1] - 0x800000 : temp_buf[i + 1];
			amb_temp_buf[i] = amb_temp_buf[i] >= 0x400000 ? amb_temp_buf[i] - 0x800000 : amb_temp_buf[i];

			if (ppg1_buf2_len < VSM_SRAM_LEN) {
				ppg1_agc_buf[ppg1_buf2_len] = leddrv_con1;
				ppg1_amb_buf[ppg1_buf2_len] = amb_temp_buf[i];
				ppg1_buf2[ppg1_buf2_len++] = temp_buf[i];
			}
			if (ppg2_buf2_len < VSM_SRAM_LEN) {
				ppg2_agc_buf[ppg2_buf2_len] = leddrv_con1;
				ppg2_amb_buf[ppg2_buf2_len] = amb_temp_buf[i];
				ppg2_buf2[ppg2_buf2_len++] = temp_buf[i + 1];
			}

			/* downsample to 16hz for AGC */
			if ((numOfSRAMPPG2Data + i) % 64 == 0) {
				agc_ppg1_buf[agc_ppg1_buf_len] = temp_buf[i];
				agc_ppg1_amb_buf[agc_ppg1_buf_len++] = amb_temp_buf[i];
				agc_ppg2_buf[agc_ppg2_buf_len] = temp_buf[i + 1];
				agc_ppg2_amb_buf[agc_ppg2_buf_len++] = amb_temp_buf[i]; /* use ppg1 amb data */
			}
		}

	} else
		ppg1_buf2_len = 0;

	if (ppg1_buf2_len == VSM_SRAM_LEN)
		aee_kernel_warning("MT6381", "PPG1 data dropped\n");
	else if (ppg1_buf2_len > 0) {
		memcpy(raw_data, ppg1_buf2, ppg1_buf2_len * sizeof(ppg1_buf2[0]));
		memcpy(amb_data, ppg1_amb_buf, ppg1_buf2_len * sizeof(ppg1_buf2[0]));
		memcpy(agc_data, ppg1_agc_buf, ppg1_buf2_len * sizeof(ppg1_buf2[0]));
	}

	*length = ppg1_buf2_len;
	ppg1_buf2_len = 0;

	/* used for latency verification */
	for (i = 0; i < *length; i++) {
		if (latency_test_data.in_latency_test) {
			/* verify only */
			if (latency_test_data.ppg1_num > 0) {
				raw_data[i] = latency_test_data.first_data;
				latency_test_data.ppg1_num--;
			} else
				raw_data[i] = latency_test_data.second_data;
		}
	}

	if (atomic_read(&bio_trace) != 0)
		for (i = 0; i < agc_ppg1_buf_len; i++)
			BIO_LOG("ppg1 = %d, amb = %d\n", agc_ppg1_buf[i], agc_ppg1_amb_buf[i]);
	ppg1_control_input.input = agc_ppg1_buf;
	ppg1_control_input.input_amb = agc_ppg1_amb_buf;
	ppg1_control_input.input_fs = ppg_control_fs;
	ppg1_control_input.input_length = agc_ppg1_buf_len;
	ppg1_control_input.input_config = ppg_control_cfg;
	ppg1_control_input.input_source = ppg_control_src;
	ppg_control_process(&ppg1_control_input);
	agc_ppg1_buf_len = 0;

	mutex_unlock(&op_lock);
	return 0;
}

int mt6381_get_data_ppg2(int *raw_data, int *amb_data, int *agc_data, u32 *length)
{
	int i;
	int32_t ppg_control_fs = 16;            /* The sampling frequency of PPG input (Support 125Hz only). */
	int32_t ppg_control_cfg = 1;             /* The input configuration, default value is 1. */
	int32_t ppg_control_src = 2;             /* The input source, default value is 1 (PPG channel 1). */
	struct ppg_control_t ppg1_control_input;           /* Input structure for the #ppg_control_process(). */
	int64_t numOfSRAMPPG2Data;
	u32 leddrv_con1;

	mutex_lock(&op_lock);
	numOfSRAMPPG2Data = numOfData[VSM_SRAM_PPG2];
	/* avoid to accumulate numOfData[] */
	if (current_signal & VSM_SIGNAL_PPG2) {
		mt6381_i2c_write_read(0x33, 0x2C, (u8 *)&leddrv_con1, 4);
		vsm_driver_read_sram(VSM_SRAM_PPG2, temp_buf, amb_temp_buf, length);
		for (i = 0; i < *length; i += 2) {
			temp_buf[i] = temp_buf[i] >= 0x400000 ? temp_buf[i] - 0x800000 : temp_buf[i];
			temp_buf[i + 1] = temp_buf[i + 1] >= 0x400000 ? temp_buf[i + 1] - 0x800000 : temp_buf[i + 1];
			amb_temp_buf[i] = amb_temp_buf[i] >= 0x400000 ? amb_temp_buf[i] - 0x800000 : amb_temp_buf[i];

			if (ppg1_buf2_len < VSM_SRAM_LEN) {
				ppg1_agc_buf[ppg1_buf2_len] = leddrv_con1;
				ppg1_amb_buf[ppg1_buf2_len] = amb_temp_buf[i];
				ppg1_buf2[ppg1_buf2_len++] = temp_buf[i];
			}
			if (ppg2_buf2_len < VSM_SRAM_LEN) {
				ppg2_agc_buf[ppg2_buf2_len] = leddrv_con1;
				ppg2_amb_buf[ppg2_buf2_len] = amb_temp_buf[i];
				ppg2_buf2[ppg2_buf2_len++] = temp_buf[i + 1];
			}

			/* downsample to 16hz for AGC */
			if ((numOfSRAMPPG2Data + i) % 64 == 0) {
				agc_ppg1_buf[agc_ppg1_buf_len] = temp_buf[i];
				agc_ppg1_amb_buf[agc_ppg1_buf_len++] = amb_temp_buf[i];
				agc_ppg2_buf[agc_ppg2_buf_len] = temp_buf[i + 1];
				agc_ppg2_amb_buf[agc_ppg2_buf_len++] = amb_temp_buf[i]; /* use ppg1 amb data */
			}
		}

	} else
		ppg2_buf2_len = 0;

	if (ppg2_buf2_len == VSM_SRAM_LEN)
		aee_kernel_warning("MT6381", "PPG1 data dropped\n");
	else if (ppg2_buf2_len > 0) {
		memcpy(raw_data, ppg2_buf2, ppg2_buf2_len * sizeof(ppg2_buf2[0]));
		memcpy(amb_data, ppg2_amb_buf, ppg2_buf2_len * sizeof(ppg2_buf2[0]));
		memcpy(agc_data, ppg2_agc_buf, ppg2_buf2_len * sizeof(ppg2_buf2[0]));
	}

	*length = ppg2_buf2_len;
	ppg2_buf2_len = 0;

	/* used for latency verification */
	for (i = 0; i < *length; i++) {
		if (latency_test_data.in_latency_test) {
			if (latency_test_data.ppg2_num > 0) {
				raw_data[i] = latency_test_data.first_data;
				latency_test_data.ppg2_num--;
			} else
				raw_data[i] = latency_test_data.second_data;
		}
	}

	if (atomic_read(&bio_trace) != 0)
		for (i = 0; i < agc_ppg2_buf_len; i++)
			BIO_LOG("ppg2 = %d, amb = %d\n", agc_ppg2_buf[i], agc_ppg2_amb_buf[i]);
	ppg1_control_input.input = agc_ppg2_buf;
	ppg1_control_input.input_amb = agc_ppg2_amb_buf;
	ppg1_control_input.input_fs = ppg_control_fs;
	ppg1_control_input.input_length = agc_ppg2_buf_len;
	ppg1_control_input.input_config = ppg_control_cfg;
	ppg1_control_input.input_source = ppg_control_src;
	ppg_control_process(&ppg1_control_input);
	agc_ppg2_buf_len = 0;

	mutex_unlock(&op_lock);

	return 0;
}

static int pin_init(void)
{
	struct device_node *node = NULL;
	struct platform_device *pdev = NULL;
	int ret;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6381");
	if (node) {
		pdev = of_find_device_by_node(node);
		if (pdev) {
			pinctrl_gpios = devm_pinctrl_get(&pdev->dev);
			if (IS_ERR(pinctrl_gpios)) {
				ret = PTR_ERR(pinctrl_gpios);
				BIO_PR_ERR("%s can't find mt6381 pinctrl\n", __func__);
				return -1;
			}
		} else {
			BIO_PR_ERR("%s platform device is null\n", __func__);
		}
		/* it's normal that get "default" will failed */
		bio_pins_default = pinctrl_lookup_state(pinctrl_gpios, "default");
		if (IS_ERR(bio_pins_default)) {
			ret = PTR_ERR(bio_pins_default);
			BIO_PR_ERR("%s can't find mt6381 pinctrl default\n", __func__);
			/* return ret; */
		}
		bio_pins_reset_high = pinctrl_lookup_state(pinctrl_gpios, "reset_high");
		if (IS_ERR(bio_pins_reset_high)) {
			ret = PTR_ERR(bio_pins_reset_high);
			BIO_PR_ERR("%s can't find mt6381 pinctrl reset_high\n", __func__);
			return -1;
		}
		bio_pins_reset_low = pinctrl_lookup_state(pinctrl_gpios, "reset_low");
		if (IS_ERR(bio_pins_reset_low)) {
			ret = PTR_ERR(bio_pins_reset_low);
			BIO_PR_ERR("%s can't find mt6381 pinctrl reset_low\n", __func__);
			return -1;
		}
		bio_pins_pwd_high = pinctrl_lookup_state(pinctrl_gpios, "pwd_high");
		if (IS_ERR(bio_pins_pwd_high)) {
			ret = PTR_ERR(bio_pins_pwd_high);
			BIO_PR_ERR("%s can't find mt6381 pinctrl pwd_high\n", __func__);
			return -1;
		}
		bio_pins_pwd_low = pinctrl_lookup_state(pinctrl_gpios, "pwd_low");
		if (IS_ERR(bio_pins_pwd_low)) {
			ret = PTR_ERR(bio_pins_pwd_low);
			BIO_PR_ERR("%s can't find mt6381 pinctrl pwd_low\n", __func__);
			return -1;
		}
	} else {
		BIO_PR_ERR("Device Tree: can not find bio node!. Go to use old cust info\n");
		return -1;
	}

	return 0;
}

static int mt6381_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;
	struct bio_control_path ctl = { 0 };
	struct bio_data_path data = { 0 };
	u32 chip_id;

	mt6381_i2c_client = client;
	mt6381_i2c_write_read(mt6381_i2c_client->addr, 0xac, (u8 *) &chip_id, 4);
	BIO_VER("bio sensor inited. chip_id:%x\n", chip_id);
	if (chip_id != 0x25110000)
		goto exit;

	mt6381_init_i2c_msg(MT2511_SLAVE_II);

	err = mt6381_create_attr(&(mt6381_init_info.platform_diver_addr->driver)/*client->dev.driver*/);
	if (err) {
		BIO_PR_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	err = pin_init();
	if (err) {
		BIO_PR_ERR("pin_init fail\n");
		goto exit_create_attr_failed;
	}

	current_signal = 0;
	atomic_set(&bio_trace, 0);
	mutex_init(&op_lock);
	ppg1_buf2_len = 0;
	ppg2_buf2_len = 0;
	modify_array_len = 0;
	new_init_array_len = 0;
	set_AFE_TCTRL_CON2 = 0;
	set_AFE_TCTRL_CON3 = 0;
	AFE_TCTRL_CON2 = 0;
	AFE_TCTRL_CON3 = 0;
	polling_delay = 50;
	latency_test_data.in_latency_test = false;
	BIO_LOG("AGC version: %x\n", ppg_control_get_version());

	ctl.enable_ekg = mt6381_enable_ekg;
	ctl.enable_ppg1 = mt6381_enable_ppg1;
	ctl.enable_ppg2 = mt6381_enable_ppg2;
	ctl.set_delay_ekg = mt6381_set_delay_ekg;
	ctl.set_delay_ppg1 = mt6381_set_delay_ppg1;
	ctl.set_delay_ppg2 = mt6381_set_delay_ppg2;
	err = bio_register_control_path(&ctl);
	if (err) {
		BIO_PR_ERR("register bio control path err\n");
		goto exit_create_attr_failed;
	}

	data.get_data_ekg = mt6381_get_data_ekg;
	data.get_data_ppg1 = mt6381_get_data_ppg1;
	data.get_data_ppg2 = mt6381_get_data_ppg2;
	err = bio_register_data_path(&data);
	if (err) {
		BIO_PR_ERR("register bio data path err\n");
		goto exit_create_attr_failed;
	}

	biosensor_init_flag = 0;
	return 0;
exit_create_attr_failed:
	mt6381_delete_attr(client->dev.driver);
exit:
	biosensor_init_flag = -1;
	return err;
}

static int mt6381_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_driver mt6381_i2c_driver = {
	.probe = mt6381_i2c_probe,
	.remove = mt6381_i2c_remove,

	.id_table = mt6381_i2c_id,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = MT6381_DEV_NAME,
/* #ifdef CONFIG_PM_SLEEP */
#if 0
		   .pm = &lsm6ds3h_pm_ops,
#endif
#ifdef CONFIG_OF
		   .of_match_table = biometric_of_match,	/* need add in dtsi first */
#endif
		   },
};

static int mt6381_local_init(void)
{
	if (i2c_add_driver(&mt6381_i2c_driver)) {
		BIO_PR_ERR("add driver error\n");
		return -1;
	}
	if (biosensor_init_flag == -1) {
		BIO_PR_ERR("%s init failed!\n", __func__);
		return -1;
	}
	return 0;
}

static int mt6381_local_remove(void)
{
	return 0;
}

static int __init MT6381_init(void)
{
	bio_driver_add(&mt6381_init_info);

	return 0;
}

/*----------------------------------------------------------------------------*/
static void __exit MT6381_exit(void)
{
}

/*----------------------------------------------------------------------------*/
module_init(MT6381_init);
module_exit(MT6381_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Liujiang Chen");
MODULE_DESCRIPTION("MT6381 driver");
MODULE_LICENSE("GPL");
