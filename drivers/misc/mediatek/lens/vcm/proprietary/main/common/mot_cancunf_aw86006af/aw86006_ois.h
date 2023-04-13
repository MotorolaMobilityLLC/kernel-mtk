#ifndef AW86006_OIS_H
#define AW86006_OIS_H

#define OIS_DRVNAME		"AW86006OIS_DRV"

/* Log Format */
#define AW_LOGI(format, ...) \
	pr_info("[%s][%04d]%s: " format "\n", OIS_DRVNAME, __LINE__, __func__, \
								##__VA_ARGS__)
#define AW_LOGD(format, ...) \
	pr_debug("[%s][%04d]%s: " format "\n", OIS_DRVNAME, __LINE__, \
							__func__, ##__VA_ARGS__)
#define AW_LOGE(format, ...) \
	pr_err("[%s][%04d]%s: " format "\n", OIS_DRVNAME, __LINE__, __func__, \
								##__VA_ARGS__)

#define CHECK_DIFF(x, y) ((x > 0) ? (x - y) : (-x - y))
#define AW_GYROACCEL_DIFT_LIMIT		(5000)

/* I2c address */
#define AW_SHUTDOWN_I2C_ADDR		(0xC2 >> 1) /* 0x61 */
#define AW_WAKEUP_I2C_ADDR		(0xD2 >> 1) /* 0x69 */

/* Register detail */
#define REG_CHIPID			(0x0000)
#define REG_OIS_ENABLE			(0x0001)
#define REG_OIS_STATUS			(0x0002)
#define REG_PACKING_RST			(0x0003)
#define REG_DATA_READY			(0x0004)
#define REG_SAMPLE_TIME			(0x0005)
#define REG_PACK_DATA			(0x0006)
#define REG_SPI_GYRO			(0x0007)
#define REG_SPI_CFG			(0x0008)
#define REG_AF_CODE_H			(0x0009)
#define REG_AF_CODE_L			(0x000A)
#define REG_AF_ARCT			(0x000B)
#define REG_AF_CFG			(0x000C)
#define REG_AF_BUSY			(0x000D)
#define REG_VERSION			(0x0010)
#define REG_STANDBY			(0xFF11)

/* Flag */
#define OIS_ENABLE			(0x01)
#define OIS_DISABLE			(0x00)
#define OIS_ERROR			(-1)
#define OIS_SUCCESS			(0)
#define AW_IC_STANDBY			(1)
#define AW_FLASH_WRITE_CONNECT
#define AW_SOC_FLASH_READ_FLAG

/* Loop */
#define AW_STAY_ON_MOVE_LOOP		(20)
#define AW_STAY_ON_BOOT_LOOP		(20)
#define AW_ERROR_LOOP			(5)
#define AW_FLASH_WRITE_ERROR_LOOP	(2)
#define AW_FLASH_READ_ERROR_LOOP	(2)
#define AW_FLASH_ERASE_ERROR_LOOP	(5)
#define AW_JUMP_BOOT_LOOP		(6)
#define AW_JUMP_MOVE_LOOP		(6)

/* Addr or data size */
#define AW_SIZE_BYTE_0			(0)
#define AW_SIZE_BYTE_1			(1)
#define AW_SIZE_BYTE_2			(2)
#define AW_SIZE_BYTE_3			(3)
#define AW_SIZE_BYTE_4			(4)
#define AW_SIZE_BYTE_12			(12)
#define AW_SIZE_BYTE_20			(20)
#define AW_PROTOCOL_SIZE		(14)
#define AW_DATA_SHIFT_0_BIT		(0)
#define AW_DATA_SHIFT_8_BIT		(8)
#define AW_DATA_SHIFT_16_BIT		(16)
#define AW_DATA_SHIFT_24_BIT		(24)
#define AW_I2C_WRITE_BYTE_ZERO		(0x00)

/* Delay */
#define AW_RESET_DELAY			(50) /* ms */
#define AW_JUMP_MOVE_DELAY		(9) /* ms */
#define AW_JUMP_MOVE_DELAY_MAX		(16) /* ms */
#define AW_JUMP_BOOT_DELAY		(2) /* ms */
#define AW_JUMP_BOOT_DELAY_MAX		(6) /* ms */
#define AW_SHUTDOWN_DELAY		(2000) /* us */

/* Flash and firmware */
#define AW_FLASH_BASE_ADDR		(0x01000000)
#define AW_FLASH_APP_ADDR		(0x01002000)
#define AW_FLASH_MOVE_LENGTH		(AW_FLASH_APP_ADDR - AW_FLASH_BASE_ADDR)
#define AW_FLASH_FULL_SIZE		(0xB000)
#define AW_FLASH_TOP_ADDR		(AW_FLASH_BASE_ADDR + AW_FLASH_FULL_SIZE)
#define AW_FLASH_ERASE_LEN		(512)
#define AW_FLASH_WRITE_LEN		(64)
#define AW_FLASH_READ_LEN		(64)
#define AW_FW_INFO_LENGTH		(64)
#define AW_FW_SHIFT_IDENTIFY		(AW_FLASH_MOVE_LENGTH)
#define AW_FW_SHIFT_CHECKSUM		(AW_FW_SHIFT_IDENTIFY + 8)
#define AW_FW_SHIFT_APP_CHECKSUM	(AW_FW_SHIFT_CHECKSUM + 4)
#define AW_FW_SHIFT_CHECKSUM_ADDR	(AW_FW_SHIFT_APP_CHECKSUM)
#define AW_FW_SHIFT_APP_LENGTH		(AW_FW_SHIFT_APP_CHECKSUM + 4)
#define AW_FW_SHIFT_APP_VERSION		(AW_FW_SHIFT_APP_LENGTH + 4)
#define AW_FW_SHIFT_APP_ID		(AW_FW_SHIFT_APP_VERSION + 4)
#define AW_FW_SHIFT_MOVE_CHECKSUM	(AW_FW_SHIFT_APP_ID + 4)
#define AW_FW_SHIFT_MOVE_VERSION	(AW_FW_SHIFT_MOVE_CHECKSUM + 4)
#define AW_FW_SHIFT_MOVE_LENGTH		(AW_FW_SHIFT_MOVE_VERSION + 4)
#define AW_FW_SHIFT_UPDATE_FLAG		(AW_FW_SHIFT_MOVE_LENGTH + 8)
#define AW_ARRAY_SHIFT_UPDATE_FLAG	(AW_FW_SHIFT_UPDATE_FLAG - AW_FLASH_MOVE_LENGTH)

/* SOC */
enum soc_status {
	SOC_OK = 0,
	SOC_ADDR_ERROR,
	SOC_PBUF_ERROR,
	SOC_HANK_ERROR,
	SOC_JUMP_ERROR,
	SOC_FLASH_ERROR,
	SOC_SPACE_ERROR,
};

enum soc_module {
	SOC_HANK	= 0x01,
	SOC_SRAM	= 0x02,
	SOC_FLASH	= 0x03,
	SOC_END		= 0x04,
};

enum soc_enum {
	SOC_VERSION		= 0x01,
	SOC_CTL			= 0x00,
	SOC_ACK			= 0x01,
	SOC_ADDR		= 0x84,
	SOC_READ_ADDR		= 0x48,
	SOC_ERASE_STRUCT_LEN	= 6,
	SOC_READ_STRUCT_LEN	= 6,
	SOC_PROTOCAL_HEAD	= 9,
	SOC_CONNECT_WRITE_LEN	= 9,
	SOC_ERASE_WRITE_LEN	= 15,
	SOC_READ_WRITE_LEN	= 15,
	SOC_ERASE_BLOCK_DELAY	= 8,
	SOC_WRITE_BLOCK_HEAD	= 13,
	SOC_WRITE_BLOCK_DELAY	= 1000, /* us */
	SOC_READ_BLOCK_DELAY	= 200, /* us */
	SOC_CONNECT_DELAY	= 200, /* us */
};

enum soc_flash_event {
	SOC_FLASH_WRITE			= 0x01,
	SOC_FLASH_WRITE_ACK		= 0x02,
	SOC_FLASH_READ			= 0x11,
	SOC_FLASH_READ_ACK		= 0x12,
	SOC_FLASH_ERASE_BLOCK		= 0x21,
	SOC_FLASH_ERASE_BLOCK_ACK	= 0x22,
	SOC_FLASH_ERASE_CHIP		= 0x23,
	SOC_FLASH_ERASE_CHIP_ACK	= 0x24,
};

enum soc_hank_event {
	SOC_HANK_CONNECT	= 0x01,
	SOC_HANK_CONNECT_ACK	= 0x02,
	SOC_HANK_PROTICOL	= 0x11,
	SOC_HANK_PROTICOL_ACK	= 0x12,
	SOC_HANK_VERSION	= 0x21,
	SOC_HANK_VERSION_ACK	= 0x22,
	SOC_HANK_ID		= 0x31,
	SOC_HANK_ID_ACK		= 0x32,
	SOC_HANK_DATE		= 0x33,
	SOC_HANK_DATA_ACK	= 0x34,
};

/* ISP */
enum isp_enum {
	ISP_READ_VERSION_DELAY	= 500, /* us */
	ISP_VERS_CONNECT_ACK	= 0x01,
	ISP_VERSION_ACK_LEN	= 5,
	ISP_EVENT_OK		= 0x01,
};

enum awrw_flag {
	AW_SEQ_WRITE	= 0,
	AW_SEQ_READ	= 1,
};

enum axis_info {
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
};

struct accelgyro_dift {
	int16_t gyro_dift[3];
	int16_t accel_dift[3];
};

/* Struct */
struct aw86006_chipinfo {
	uint32_t version;
	uint8_t chipid;
};

struct aw86006_i2c_rw {
	uint8_t rw;               //r:0, w:1
	u16 addr;
	u16 byte_addr;
	uint8_t pdata[8];
	u16 byte_data;
};

struct soc_protocol {
	uint8_t checksum;
	uint8_t protocol_ver;
	uint8_t addr;
	uint8_t module;
	uint8_t event;
	uint8_t len[2];
	uint8_t ack;
	uint8_t sum;
	uint8_t reserved[3];
	uint8_t ack_juge;
	uint8_t *p_data;
};

struct aw86006_fw {
	uint32_t checksum;
	uint32_t app_checksum;
	uint32_t app_length;
	uint32_t app_version;
	uint32_t app_id;
	uint32_t move_checksum;
	uint32_t move_version;
	uint32_t move_length;
	uint32_t update_flag;
	uint32_t size;
	uint8_t *data_p;
};

struct aw86006_info {
	uint8_t already_update;
	uint8_t checkinfo_fw[64];
	uint8_t checkinfo_rd[64];
	struct aw86006_fw fw;
};

struct awrw_ctrl {
	uint32_t addr[4];
	uint16_t reg_num;
	uint8_t flag;
	uint8_t *reg_data;
};

struct cam_ois_ctrl_t {
	struct work_struct aw_fw_update_work;
	struct i2c_client *client;
	struct mutex aw_ois_mutex;
	struct device *dev;
	struct class *debug_class;
	struct awrw_ctrl *awrw_ctrl;
	struct proc_dir_entry *proc;
};

/* ioctl info */
#define OIS_MAGIC 'O'

#define OISIOC_G_CHIPINFO _IOWR(OIS_MAGIC, 0, struct aw86006_chipinfo)
#define OISIOC_T_OISMODE _IOW(OIS_MAGIC, 1, u32)
#define OISIOC_T_UPDATE _IOW(OIS_MAGIC, 2, u32)
#define OISIOC_G_HEA  _IOWR(OIS_MAGIC, 3, struct aw86006_i2c_rw)

#endif

