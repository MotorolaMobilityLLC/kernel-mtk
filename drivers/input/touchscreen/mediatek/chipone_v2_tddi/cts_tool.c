#define LOG_TAG         "Tool"

#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_firmware.h"
#include "cts_test.h"

#ifdef CONFIG_CTS_LEGACY_TOOL

#pragma pack(1)
/** Tool command structure */
struct cts_tool_cmd {
	u8 cmd;
	u8 flag;
	u8 circle;
	u8 times;
	u8 retry;
	u32 data_len;
	u8 addr_len;
	u8 addr[2];
	u8 data[PAGE_SIZE];

};
#pragma pack()

#define CTS_TOOL_CMD_HEADER_LENGTH            (12)

enum cts_tool_cmd_code {
	CTS_TOOL_CMD_GET_PANEL_PARAM = 0,
	CTS_TOOL_CMD_GET_DOWNLOAD_STATUS = 2,
	CTS_TOOL_CMD_GET_RAW_DATA = 4,
	CTS_TOOL_CMD_GET_DIFF_DATA = 6,
	CTS_TOOL_CMD_READ_HOSTCOMM = 12,
	CTS_TOOL_CMD_READ_ADC_STATUS = 14,
	CTS_TOOL_CMD_READ_GESTURE_INFO = 16,
	CTS_TOOL_CMD_READ_HOSTCOMM_MULTIBYTE = 18,
	CTS_TOOL_CMD_READ_PROGRAM_MODE_MULTIBYTE = 20,
	CTS_TOOL_CMD_READ_ICTYPE = 22,
	CTS_TOOL_CMD_I2C_DIRECT_READ = 24,
	CTS_TOOL_CMD_GET_DRIVER_INFO = 26,

	CTS_TOOL_CMD_UPDATE_PANEL_PARAM_IN_SRAM = 1,
	CTS_TOOL_CMD_DOWNLOAD_FIRMWARE_WITH_FILENAME = 3,
	CTS_TOOL_CMD_DOWNLOAD_FIRMWARE = 5,
	CTS_TOOL_CMD_WRITE_HOSTCOMM = 11,
	CTS_TOOL_CMD_WRITE_HOSTCOMM_MULTIBYTE = 15,
	CTS_TOOL_CMD_WRITE_PROGRAM_MODE_MULTIBYTE = 17,
	CTS_TOOL_CMD_I2C_DIRECT_WRITE = 19,

};

struct cts_test_rawdata_ioctl_data {
	int frames;
	int min;
	int max;
};

struct cts_test_noise_ioctl_data {
	int frames;
	int max;
};

struct cts_test_open_ioctl_data {
	int min;
	int max;
};

struct cts_test_short_ioctl_data {
	int min;
	int max;
	bool display_on;
	bool disable_gas;
};

struct cts_test_comp_cap_ioctl_data {
	int min;
	int max;
};

#define CTS_TOOL_IOCTL_GET_DRIVER_VERSION	_IOR('C', 0x00, unsigned int)
#define CTS_TOOL_IOCTL_GET_DEVICE_TYPE		_IOR('C', 0x01, unsigned int)
#define CTS_TOOL_IOCTL_GET_FW_VERSION		_IOR('C', 0x02, unsigned short)

#define CTS_TOOL_IOCTL_TEST_RESET_PIN		_IO('C',  0x11)
#define CTS_TOOL_IOCTL_TEST_INT_PIN		_IO('C',  0x12)
#define CTS_TOOL_IOCTL_TEST_DEVICE_TYPE		_IOW('C', 0x13, unsigned int)
#define CTS_TOOL_IOCTL_TEST_FW_VERSION		_IOW('C', 0x14, unsigned short)
#define CTS_TOOL_IOCTL_TEST_RAWDATA		_IOW('C', 0x15, struct cts_test_rawdata_ioctl_data *)
#define CTS_TOOL_IOCTL_TEST_NOISE		_IOW('C', 0x16, struct cts_test_noise_ioctl_data *)
#define CTS_TOOL_IOCTL_TEST_OPEN		_IOW('C', 0x17, struct cts_test_open_ioctl_data *)
#define CTS_TOOL_IOCTL_TEST_SHORT		_IOW('C', 0x18, struct cts_test_short_ioctl_data *)
#define CTS_TOOL_IOCTL_TEST_COMP_CAP		_IOW('C', 0x19, struct cts_test_comp_cap_ioctl_data *)

#define CTS_DRIVER_VERSION_CODE     ((CFG_CTS_DRIVER_MAJOR_VERSION << 16) | \
					(CFG_CTS_DRIVER_MINOR_VERSION << 8) | \
					(CFG_CTS_DRIVER_PATCH_VERSION << 0))

static struct cts_tool_cmd cts_tool_cmd;
static char cts_tool_firmware_filepath[PATH_MAX];
/* If CFG_CTS_MAX_I2C_XFER_SIZE < 58(PC tool length), this is neccessary */
#ifdef CONFIG_CTS_I2C_HOST
//static u32 cts_tool_direct_access_addr = 0;
#endif /* CONFIG_CTS_I2C_HOST */

static int cts_tool_open(struct inode *inode, struct file *file)
{
	file->private_data = PDE_DATA(inode);
	return 0;
}

static ssize_t cts_tool_read(struct file *file,
			     char __user *buffer, size_t count, loff_t *ppos)
{
	struct chipone_ts_data *cts_data;
	struct cts_tool_cmd *cmd;
	struct cts_device *cts_dev;
	int ret = 0;

	cts_data = (struct chipone_ts_data *)file->private_data;
	if (cts_data == NULL) {
		cts_err("Read with private_data = NULL");
		return -EIO;
	}

	cmd = &cts_tool_cmd;
	cts_dev = &cts_data->cts_dev;
	cts_lock_device(cts_dev);

	switch (cmd->cmd) {
	case CTS_TOOL_CMD_GET_PANEL_PARAM:
		cts_info("Get panel param len: %u", cmd->data_len);
		ret = cts_get_panel_param(cts_dev, cmd->data, cmd->data_len);
		if (ret) {
			cts_err("Get panel param len: %u failed %d",
				cmd->data_len, ret);
		}
		break;

	case CTS_TOOL_CMD_GET_DOWNLOAD_STATUS:
		cmd->data[0] = 100;
		cts_info("Get update status = %hhu", cmd->data[0]);
		break;

	case CTS_TOOL_CMD_GET_RAW_DATA:
	case CTS_TOOL_CMD_GET_DIFF_DATA:
		cts_dbg("Get %s data row: %u col: %u len: %u",
			cmd->cmd == CTS_TOOL_CMD_GET_RAW_DATA ? "raw" : "diff",
			cmd->addr[1], cmd->addr[0], cmd->data_len);

		ret = cts_enable_get_rawdata(cts_dev);
		if (ret) {
			cts_err("Enable read raw/diff data failed %d", ret);
			break;
		}
		mdelay(1);

		if (cmd->cmd == CTS_TOOL_CMD_GET_RAW_DATA) {
			ret = cts_get_rawdata(cts_dev, cmd->data);
		} else if (cmd->cmd == CTS_TOOL_CMD_GET_DIFF_DATA) {
			ret = cts_get_diffdata(cts_dev, cmd->data);
		}
		if (ret) {
			cts_err("Get %s data failed %d",
				cmd->cmd ==
				CTS_TOOL_CMD_GET_RAW_DATA ? "raw" : "diff",
				ret);
			break;
		}

		ret = cts_disable_get_rawdata(cts_dev);
		if (ret) {
			cts_err("Disable read raw/diff data failed %d", ret);
			break;
		}

		break;

	case CTS_TOOL_CMD_READ_HOSTCOMM:
		ret = cts_fw_reg_readb(cts_dev,
				       get_unaligned_le16(cmd->addr),
				       cmd->data);
		if (ret) {
			cts_err("Read firmware reg addr 0x%04x failed %d",
				get_unaligned_le16(cmd->addr), ret);
		} else {
			cts_dbg("Read firmware reg addr 0x%04x, val=0x%02x",
				get_unaligned_le16(cmd->addr), cmd->data[0]);
		}
		break;

#ifdef CFG_CTS_GESTURE
	case CTS_TOOL_CMD_READ_GESTURE_INFO:
		ret = cts_get_gesture_info(cts_dev, cmd->data, true);
		if (ret) {
			cts_err("Get gesture info failed %d", ret);
		}
		break;
#endif /* CFG_CTS_GESTURE */

	case CTS_TOOL_CMD_READ_HOSTCOMM_MULTIBYTE:
		cmd->data_len = min((size_t) cmd->data_len, sizeof(cmd->data));
		ret = cts_fw_reg_readsb(cts_dev, get_unaligned_le16(cmd->addr),
					cmd->data, cmd->data_len);
		if (ret) {
			cts_err
			    ("Read firmware reg addr 0x%04x len %u failed %d",
			     get_unaligned_le16(cmd->addr), cmd->data_len, ret);
		} else {
			cts_dbg("Read firmware reg addr 0x%04x len %u",
				get_unaligned_le16(cmd->addr), cmd->data_len);
		}
		break;

	case CTS_TOOL_CMD_READ_PROGRAM_MODE_MULTIBYTE:
		cts_dbg("Read under program mode addr 0x%06x len %u",
			(cmd->flag << 16) | get_unaligned_le16(cmd->addr),
			cmd->data_len);
		ret = cts_enter_program_mode(cts_dev);
		if (ret) {
			cts_err("Enter program mode failed %d", ret);
			break;
		}

		ret = cts_sram_readsb(&cts_data->cts_dev,
				      get_unaligned_le16(cmd->addr), cmd->data,
				      cmd->data_len);
		if (ret) {
			cts_err("Read under program mode I2C xfer failed %d",
				ret);
			//break;
		}

		ret = cts_enter_normal_mode(cts_dev);
		if (ret) {
			cts_err("Enter normal mode failed %d", ret);
			break;
		}
		break;

	case CTS_TOOL_CMD_READ_ICTYPE:
		cts_info("Get IC type");
		if (cts_dev->hwdata) {
			switch (cts_dev->hwdata->hwid) {
			case CTS_DEV_HWID_ICNL9911:
				cmd->data[0] = 0x91;
				break;
			case CTS_DEV_HWID_ICNL9911S:
				cmd->data[0] = 0x91;
				break;
			case CTS_DEV_HWID_ICNL9911C:
				cmd->data[0] = 0x91;
				break;
			default:
				cmd->data[0] = 0x00;
				break;
			}
		} else {
			cmd->data[0] = 0x10;
		}
		break;

#ifdef CONFIG_CTS_I2C_HOST
	case CTS_TOOL_CMD_I2C_DIRECT_READ:
		{
			u32 addr_width;
			char *wr_buff = NULL;
			u8 addr_buff[4];
			size_t left_size, max_xfer_size;
			u8 *data;

			if (cmd->addr[0] != CTS_DEV_PROGRAM_MODE_I2CADDR) {
				cmd->addr[0] = CTS_DEV_NORMAL_MODE_I2CADDR;
				addr_width = 2;
			} else {
				addr_width =
				    cts_dev->hwdata->program_addr_width;
			}

			cts_dbg
			    ("Direct read from i2c_addr 0x%02x addr 0x%06x size %u",
			     cmd->addr[0], cts_tool_direct_access_addr,
			     cmd->data_len);

			left_size = cmd->data_len;
			max_xfer_size =
			    cts_plat_get_max_i2c_xfer_size(cts_dev->pdata);
			data = cmd->data;
			while (left_size) {
				size_t xfer_size =
				    min(left_size, max_xfer_size);
				ret =
				    cts_plat_i2c_read(cts_data->pdata,
						      cmd->addr[0], wr_buff,
						      addr_width, data,
						      xfer_size, 1, 0);
				if (ret) {
					cts_err
					    ("Direct read i2c_addr 0x%02x addr 0x%06x len %zu failed %d",
					     cmd->addr[0],
					     cts_tool_direct_access_addr,
					     xfer_size, ret);
					break;
				}

				left_size -= xfer_size;
				if (left_size) {
					data += xfer_size;
					cts_tool_direct_access_addr +=
					    xfer_size;
					if (addr_width == 2) {
						put_unaligned_be16
						    (cts_tool_direct_access_addr,
						     addr_buff);
					} else if (addr_width == 3) {
						put_unaligned_be24
						    (cts_tool_direct_access_addr,
						     addr_buff);
					}
					wr_buff = addr_buff;
				}
			}
		}
		break;
#endif
	case CTS_TOOL_CMD_GET_DRIVER_INFO:
		break;

	default:
		cts_warn("Read unknown command %u", cmd->cmd);
		ret = -EINVAL;
		break;
	}

	cts_unlock_device(cts_dev);

	if (ret == 0) {
		if (cmd->cmd == CTS_TOOL_CMD_I2C_DIRECT_READ) {
			ret = copy_to_user(buffer + CTS_TOOL_CMD_HEADER_LENGTH,
					   cmd->data, cmd->data_len);
		} else {
			ret = copy_to_user(buffer, cmd->data, cmd->data_len);
		}
		if (ret) {
			cts_err("Copy data to user buffer failed %d", ret);
			return 0;
		}

		return cmd->data_len;
	}

	return 0;
}

static ssize_t cts_tool_write(struct file *file,
			      const char __user *buffer, size_t count,
			      loff_t *ppos)
{
	struct chipone_ts_data *cts_data;
	struct cts_device *cts_dev;
	struct cts_tool_cmd *cmd;
	int ret = 0;

	if (count < CTS_TOOL_CMD_HEADER_LENGTH || count > PAGE_SIZE) {
		cts_err("Write len %zu invalid", count);
		return -EFAULT;
	}

	cts_data = (struct chipone_ts_data *)file->private_data;
	if (cts_data == NULL) {
		cts_err("Write with private_data = NULL");
		return -EIO;
	}

	cmd = &cts_tool_cmd;
	ret = copy_from_user(cmd, buffer, CTS_TOOL_CMD_HEADER_LENGTH);
	if (ret) {
		cts_err("Copy command header from user buffer failed %d", ret);
		return -EIO;
	} else {
		ret = CTS_TOOL_CMD_HEADER_LENGTH;
	}

	if (cmd->data_len > PAGE_SIZE) {
		cts_err("Write with invalid count %d", cmd->data_len);
		return -EIO;
	}

	if (cmd->cmd & BIT(0)) {
		if (cmd->data_len) {
			ret = copy_from_user(cmd->data,
					     buffer +
					     CTS_TOOL_CMD_HEADER_LENGTH,
					     cmd->data_len);
			if (ret) {
				cts_err
				    ("Copy command payload from user buffer len %u failed %d",
				     cmd->data_len, ret);
				return -EIO;
			}
		}
	} else {
		/* Prepare for read command */
		cts_dbg("Write read command(%d) header, prepare read size: %d",
			cmd->cmd, cmd->data_len);
		return CTS_TOOL_CMD_HEADER_LENGTH + cmd->data_len;
	}

	cts_dev = &cts_data->cts_dev;
	cts_lock_device(cts_dev);

	switch (cmd->cmd) {
	case CTS_TOOL_CMD_UPDATE_PANEL_PARAM_IN_SRAM:
		cts_info("Write panel param len %u data\n", cmd->data_len);
		ret = cts_fw_reg_writesb(cts_dev, CTS_DEVICE_FW_REG_PANEL_PARAM,
					 cmd->data, cmd->data_len);
		if (ret) {
			cts_err("Write panel param failed %d", ret);
			break;
		}

		ret = cts_send_command(cts_dev, CTS_CMD_RESET);
		if (ret) {

		}

		ret = cts_set_work_mode(cts_dev, 1);
		if (ret) {

		}

		mdelay(100);

		ret = cts_set_work_mode(cts_dev, 0);
		if (ret) {

		}
		mdelay(100);

		break;

	case CTS_TOOL_CMD_DOWNLOAD_FIRMWARE_WITH_FILENAME:
		cts_info("Write firmware path: '%.*s'",
			 cmd->data_len, cmd->data);

		memcpy(cts_tool_firmware_filepath, cmd->data, cmd->data_len);
		cts_tool_firmware_filepath[cmd->data_len] = '\0';
		break;

	case CTS_TOOL_CMD_DOWNLOAD_FIRMWARE:
		cts_info("Start download firmware path: '%s'",
			 cts_tool_firmware_filepath);

		ret = cts_stop_device(cts_dev);
		if (ret) {
			cts_err("Stop device failed %d", ret);
			break;
		}
		// TODO:  Use async mode such as thread, otherwise, host can not get update status.
		ret =
		    cts_update_firmware_from_file(cts_dev,
						  cts_tool_firmware_filepath,
						  true);
		if (ret) {
			cts_err("Updata firmware failed %d", ret);
			//break;
		}

		ret = cts_start_device(cts_dev);
		if (ret) {
			cts_err("Start device failed %d", ret);
			break;
		}
		break;

	case CTS_TOOL_CMD_WRITE_HOSTCOMM:
		cts_dbg("Write firmware reg addr: 0x%04x val=0x%02x",
			get_unaligned_le16(cmd->addr), cmd->data[0]);

		ret = cts_fw_reg_writeb(cts_dev,
					get_unaligned_le16(cmd->addr),
					cmd->data[0]);
		if (ret) {
			cts_err
			    ("Write firmware reg addr: 0x%04x val=0x%02x failed %d",
			     get_unaligned_le16(cmd->addr), cmd->data[0], ret);
		}
		break;

	case CTS_TOOL_CMD_WRITE_HOSTCOMM_MULTIBYTE:
		cts_dbg("Write firmare reg addr: 0x%04x len %u",
			get_unaligned_le16(cmd->addr), cmd->data_len);
		ret = cts_fw_reg_writesb(cts_dev, get_unaligned_le16(cmd->addr),
					 cmd->data, cmd->data_len);
		if (ret) {
			cts_err
			    ("Write firmare reg addr: 0x%04x len %u failed %d",
			     get_unaligned_le16(cmd->addr), cmd->data_len, ret);
		}
		break;

	case CTS_TOOL_CMD_WRITE_PROGRAM_MODE_MULTIBYTE:
		cts_dbg("Write to addr 0x%06x size %u under program mode",
			(cmd->flag << 16) | (cmd->addr[1] << 8) | cmd->addr[0],
			cmd->data_len);
		ret = cts_enter_program_mode(cts_dev);
		if (ret) {
			cts_err("Enter program mode failed %d", ret);
			break;
		}

		ret = cts_sram_writesb(cts_dev,
				       (cmd->flag << 16) | (cmd->addr[1] << 8) |
				       cmd->addr[0], cmd->data, cmd->data_len);
		if (ret) {
			cts_err("Write program mode multibyte failed %d", ret);
			//break;
		}

		ret = cts_enter_normal_mode(cts_dev);
		if (ret) {
			cts_err("Enter normal mode failed %d", ret);
			break;
		}

		break;

#ifdef CONFIG_CTS_I2C_HOST
	case CTS_TOOL_CMD_I2C_DIRECT_WRITE:
		{
			u32 addr_width;
			size_t left_payload_size;	/* Payload exclude address field */
			size_t max_xfer_size;
			char *payload;

			if (cmd->addr[0] != CTS_DEV_PROGRAM_MODE_I2CADDR) {
				cmd->addr[0] = CTS_DEV_NORMAL_MODE_I2CADDR;
				addr_width = 2;
				cts_tool_direct_access_addr =
				    get_unaligned_be16(cmd->data);
			} else {
				addr_width =
				    cts_dev->hwdata->program_addr_width;
				cts_tool_direct_access_addr =
				    get_unaligned_be24(cmd->data);
			}

			if (cmd->data_len < addr_width) {
				cts_err
				    ("Direct write too short %d < address width %d",
				     cmd->data_len, addr_width);
				ret = -EINVAL;
				break;
			}

			cts_dbg
			    ("Direct write to i2c_addr 0x%02x addr 0x%06x size %u",
			     cmd->addr[0], cts_tool_direct_access_addr,
			     cmd->data_len);

			left_payload_size = cmd->data_len - addr_width;
			max_xfer_size =
			    cts_plat_get_max_i2c_xfer_size(cts_dev->pdata);
			payload = cmd->data + addr_width;
			do {
				size_t xfer_payload_size =
				    min(left_payload_size,
					max_xfer_size - addr_width);
				size_t xfer_len =
				    xfer_payload_size + addr_width;

				ret =
				    cts_plat_i2c_write(cts_data->pdata,
						       cmd->addr[0],
						       payload - addr_width,
						       xfer_len, 1, 0);
				if (ret) {
					cts_err
					    ("Direct write i2c_addr 0x%02x addr 0x%06x len %zu failed %d",
					     cmd->addr[0],
					     cts_tool_direct_access_addr,
					     xfer_len, ret);
					break;
				}

				left_payload_size -= xfer_payload_size;
				if (left_payload_size) {
					payload += xfer_payload_size;
					cts_tool_direct_access_addr +=
					    xfer_payload_size;
					if (addr_width == 2) {
						put_unaligned_be16
						    (cts_tool_direct_access_addr,
						     payload - addr_width);
					} else if (addr_width == 3) {
						put_unaligned_be24
						    (cts_tool_direct_access_addr,
						     payload - addr_width);
					}
				}
			} while (left_payload_size);
		}
		break;
#endif
	default:
		cts_warn("Write unknown command %u", cmd->cmd);
		ret = -EINVAL;
		break;
	}

	cts_unlock_device(cts_dev);

	return ret ? 0 : cmd->data_len + CTS_TOOL_CMD_HEADER_LENGTH;
}

static long cts_tool_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	struct chipone_ts_data *cts_data;
	struct cts_device *cts_dev;

	cts_info("ioctl, cmd=0x%04x, arg=0x%02lx", cmd, arg);

	cts_data = file->private_data;
	if (cts_data == NULL) {
		cts_err("IOCTL with private data = NULL");
		return -EFAULT;
	}

	cts_dev = &cts_data->cts_dev;

	switch (cmd) {
	case CTS_TOOL_IOCTL_GET_DRIVER_VERSION:
		return put_user(CTS_DRIVER_VERSION_CODE,
				(unsigned int __user *)arg);
	case CTS_TOOL_IOCTL_GET_DEVICE_TYPE:
		return put_user(cts_dev->hwdata->hwid,
				(unsigned int __user *)arg);
	case CTS_TOOL_IOCTL_GET_FW_VERSION:
		return put_user(cts_dev->fwdata.version,
				(unsigned short __user *)arg);

	case CTS_TOOL_IOCTL_TEST_RESET_PIN:{
			return cts_reset_test(cts_dev);
		}
	case CTS_TOOL_IOCTL_TEST_INT_PIN:{
			return cts_test_int_pin(cts_dev);
		}
	case CTS_TOOL_IOCTL_TEST_DEVICE_TYPE:{
			if (cts_dev->hwdata->hwid == (u32) arg) {
				return 0;
			}
			return -EINVAL;
		}
	case CTS_TOOL_IOCTL_TEST_FW_VERSION:{
			if (cts_dev->fwdata.version == (u16) arg) {
				return 0;
			}
			return -EINVAL;
		}
	case CTS_TOOL_IOCTL_TEST_RAWDATA:{
			struct cts_test_rawdata_ioctl_data param;

			if (copy_from_user(&param,
					   (struct cts_test_rawdata_ioctl_data
					    __user *)arg, sizeof(param)))
				return -EFAULT;

			return cts_rawdata_test(cts_dev, param.min, param.max);
		}
	case CTS_TOOL_IOCTL_TEST_NOISE:{
			struct cts_test_noise_ioctl_data param;

			if (copy_from_user(&param,
					   (struct cts_test_noise_ioctl_data
					    __user *)arg, sizeof(param)))
				return -EFAULT;

			return cts_noise_test(cts_dev, param.frames, param.max);
		}
	case CTS_TOOL_IOCTL_TEST_OPEN:{
			struct cts_test_open_ioctl_data param;

			if (copy_from_user(&param,
					   (struct cts_test_open_ioctl_data
					    __user *)arg, sizeof(param)))
				return -EFAULT;

			return cts_open_test(cts_dev, param.max);
		}
	case CTS_TOOL_IOCTL_TEST_SHORT:{
			struct cts_test_short_ioctl_data param;

			if (copy_from_user(&param,
					   (struct cts_test_short_ioctl_data
					    __user *)arg, sizeof(param)))
				return -EFAULT;

			return cts_short_test(cts_dev, param.max);
		}
	case CTS_TOOL_IOCTL_TEST_COMP_CAP:{
			struct cts_test_comp_cap_ioctl_data param;

			if (copy_from_user(&param,
					   (struct cts_test_comp_cap_ioctl_data
					    __user *)arg, sizeof(param)))
				return -EFAULT;

			return cts_compensate_cap_test(cts_dev, param.min,
						       param.max);
		}

	default:
		break;
	}

	return -ENOTSUPP;
}

static struct file_operations cts_tool_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.open = cts_tool_open,
	.read = cts_tool_read,
	.write = cts_tool_write,
	.unlocked_ioctl = cts_tool_ioctl,
};

int cts_tool_init(struct chipone_ts_data *cts_data)
{
	cts_info("Init");

	cts_data->procfs_entry = proc_create_data(CFG_CTS_TOOL_PROC_FILENAME,
						  0666, NULL, &cts_tool_fops,
						  cts_data);
	if (cts_data->procfs_entry == NULL) {
		cts_err("Create proc entry failed");
		return -EFAULT;
	}

	return 0;
}

void cts_tool_deinit(struct chipone_ts_data *data)
{
	cts_info("Deinit");

	if (data->procfs_entry) {
		remove_proc_entry(CFG_CTS_TOOL_PROC_FILENAME, NULL);
	}
}
#endif /* CONFIG_CTS_LEGACY_TOOL */
