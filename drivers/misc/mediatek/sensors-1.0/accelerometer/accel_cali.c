
#include <inc/accel.h>


#define PROINFO_CALI_DATA_OFFSET (2690)
#define PROINFO_CALI_DATA_SIZE 	 (6)

static mm_segment_t oldfs;
static char accel_cali_path[] = "/dev/block/platform/bootdevice/by-name/proinfo";

static struct file *openFile(char *path, int flag, int mode)
{
	struct file *fp = NULL;

	fp = filp_open(path, flag, mode);

	if (IS_ERR(fp) || !fp->f_op) {
		return NULL;
	} else {
		return fp;
	}
}

//whence--- SEEK_END/SEEK_CUR/SEEK_SET
static int seekFile(struct file *fp, int offset, int whence)
{
	if (fp->f_op && fp->f_op->llseek)
		return fp->f_op->llseek(fp, (loff_t) offset, whence);
	else
		return -1;
}

static int readFile(struct file *fp, char *buf, int readlen)
{
	if (fp && fp->f_op)
		return __vfs_read(fp, buf, readlen, &fp->f_pos);
	else
		return -1;
}

#if 0
static int writeFile(struct file *fp, char *buf, int writelen)
{
	if (fp->f_op && fp->f_op->write)
		return fp->f_op->write(fp, buf, writelen, &fp->f_pos);
	else
		return -1;
}
#endif

static int closeFile(struct file *fp)
{
	filp_close(fp, NULL);
	return 0;
}

static void initKernelEnv(void)
{
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	printk("initKernelEnv\n");
}

int accel_read_cali_file( int32_t data[3])
{
	int err  = 0;

	struct file *fd = NULL;
	char buffer[PROINFO_CALI_DATA_SIZE] = { 0 };  ///PS_CALI_DATA_LEN = 6

	initKernelEnv();

	memset(buffer, 0, sizeof(buffer));

	fd = openFile(accel_cali_path, O_RDONLY, 0);
	if ( NULL == fd) {
		ACC_PR_ERR("open %s failed\n", accel_cali_path);
		set_fs(oldfs);
		return (-1);
	}

	if (seekFile(fd, PROINFO_CALI_DATA_OFFSET, SEEK_SET) < 0) {
		ACC_PR_ERR("seek %s failed\n", accel_cali_path);
		goto read_error;
	}

	err = readFile(fd, buffer, sizeof(buffer));
	if (err > 0) {
		ACC_PR_ERR("read %s success.\n", accel_cali_path);
	} else {
		ACC_PR_ERR("read %s failed, errno=%d\n", accel_cali_path, err);
		goto read_error;
	}

	closeFile(fd);
	set_fs(oldfs);

	if( (0x80 & buffer[0]) == 0x80){
		data[0] = (0xFFFF&(((buffer[0]&0xFF) << 8) | buffer[1])) - 65536;
	} else {
		data[0] = (0xFFFF&(((buffer[0]&0xFF) << 8) | buffer[1]));
	}

	if( (0x80 & buffer[2]) == 0x80){
		data[1] = (0xFFFF&(((buffer[2]&0xFF) << 8) | buffer[3])) - 65536;
	} else {
		data[1] = (0xFFFF&(((buffer[2]&0xFF) << 8) | buffer[3]));
	}

	if( (0x80 & buffer[4]) == 0x80){
		data[2] = (0xFFFF&(((buffer[4]&0xFF) << 8) | buffer[5])) - 65536;
	} else {
		data[2] = (0xFFFF&(((buffer[4]&0xFF) << 8) | buffer[5]));
	}


#if 0
	{
		int i = 0;
		for( i = 0; i < 6; i++) {
			printk("%s +%d, buffer[%d]=%x\n", __func__, __LINE__, i, buffer[i]);
		}
	}
#endif
	return 0;

read_error:
	closeFile(fd);
	set_fs(oldfs);
	return (-1);
}
