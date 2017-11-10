#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>

#include <linux/i2c.h>//iic
#include <linux/delay.h>//msleep
#include "focaltech_core.h"

#ifdef WT_CTP_OPEN_SHORT_TEST
#include "test_lib.h"
#include "tpd.h"
#include "tpd_ft5x0x_common.h"
#define GTP_TEST "ctp_test"
#define OPEN_TEST "opentest"
#define SHORT_TEST "shorttest"
#define FT6436U_INI_FILEPATH "/system/etc/"
#define FT6436U_TEST_DATA   "/sdcard/"

static struct proc_dir_entry *gtp_parent_proc = NULL;
static struct proc_dir_entry *gtp_open_proc = NULL;
static struct proc_dir_entry *gtp_short_proc = NULL;

static ssize_t ctp_openshort_proc_read(struct file *file, char __user *user_buf,size_t count, loff_t *ppos);
static ssize_t ctp_openshort_proc_write(struct file *filp, const char __user *userbuf,size_t count, loff_t *ppos);
static ssize_t ctp_short_proc_read(struct file *file, char __user *user_buf,size_t count, loff_t *ppos);
static ssize_t ctp_short_proc_write(struct file *filp, const char __user *userbuf,size_t count, loff_t *ppos);
static const struct file_operations ctp_openshort_procs_fops =
{
    .write = ctp_openshort_proc_write,
    .read = ctp_openshort_proc_read,
    .open = simple_open,
    .owner = THIS_MODULE,
};

static const struct file_operations ctp_short_procs_fops =
{
    .write = ctp_short_proc_write,
    .read = ctp_short_proc_read,
    .open = simple_open,
    .owner = THIS_MODULE,
};


static ssize_t ctp_openshort_proc_write(struct file *filp, const char __user *userbuf,size_t count, loff_t *ppos)
{
		return -1;
}
static ssize_t ctp_short_proc_read(struct file *file, char __user *user_buf,size_t count, loff_t *ppos)
{	
	char *ptr=user_buf;
	char result[7];
	if(*ppos)
	{
		printk("tp test short return\n");
		return 0;
	}
	*ppos += count;
	 strcpy(result,"pass");
	return sprintf(ptr, "%s\n",result);
}
static ssize_t ctp_short_proc_write(struct file *filp, const char __user *userbuf,size_t count, loff_t *ppos)
{
		return -1;
}

static int ftxxxx_GetInISize(char *config_name)
{
	struct file *pfile = NULL;
	struct inode *inode = NULL;
	//unsigned long magic;
	off_t fsize = 0;
	char filepath[128];
	memset(filepath, 0, sizeof(filepath));

	sprintf(filepath, "%s%s", FT6436U_INI_FILEPATH, config_name);

	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);

	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	//magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	filp_close(pfile, NULL);

	return fsize;
}

static int ftxxxx_ReadInIData(char *config_name, char *config_buf)
{
	struct file *pfile = NULL;
	struct inode *inode = NULL;
	//unsigned long magic;
	off_t fsize = 0;
	char filepath[128];
	loff_t pos = 0;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s", FT6436U_INI_FILEPATH, config_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	//magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_read(pfile, config_buf, fsize, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}

static int ftxxxx_SaveTestData(char *file_name, char *data_buf, int iLen)
{
	struct file *pfile = NULL;
	
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s",  FT6436U_TEST_DATA, file_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_CREAT|O_RDWR, 0);
	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(pfile, data_buf, iLen, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}


static int ftxxxx_get_testparam_from_ini(char *config_name)
{
	char *filedata = NULL;

	int inisize = ftxxxx_GetInISize(config_name);

	pr_info("inisize = %d \n ", inisize);
	if (inisize <= 0) {
		pr_err("%s ERROR:Get firmware size failed\n", __func__);
		return -EIO;
	}

	filedata = kmalloc(inisize + 1, GFP_ATOMIC);
		
	if (ftxxxx_ReadInIData(config_name, filedata)) {
		pr_err("%s() - ERROR: request_firmware failed\n", __func__);
		kfree(filedata);
		return -EIO;
	} else {
		pr_info("ftxxxx_ReadInIData successful\n");
	}

	set_param_data(filedata);
	return 0;
}


int focal_base_i2c_Read(unsigned char *writebuf,
		    int writelen, unsigned char *readbuf, int readlen)
{

	return fts_i2c_read(fts_i2c_client, writebuf, writelen, readbuf, readlen);
}

int focal_base_i2c_Write(unsigned char *writebuf, int writelen)
{
	return fts_i2c_write(fts_i2c_client, writebuf, writelen);
	
}
static ssize_t ctp_openshort_proc_read(struct file *file, char __user *user_buf,size_t count, loff_t *ppos)
{
	char *ptr = user_buf;
    	char cfgname[128];
	char *testdata = NULL;
	int iTestDataLen=0;
	char result[7];

	testdata = kmalloc(1024*80, GFP_ATOMIC);
	if(NULL == testdata)
	{
		pr_err("kmalloc failed in function:%s\n", __func__);
		return -1;
	}

	if(*ppos)
	{
		pr_err("tp test again return\n");
        	return 0;
	}

	*ppos += count;

	memset(cfgname, 0, sizeof(cfgname));
	sprintf(cfgname, "%s", "tp_test_sensor.ini");
	cfgname[18] = '\0';

	//mutex_lock(&g_device_mutex);
   
	init_i2c_write_func(focal_base_i2c_Write);
	init_i2c_read_func(focal_base_i2c_Read);
	
	
	if(ftxxxx_get_testparam_from_ini(cfgname) <0)
	{
		pr_err("get testparam from ini failure\n");
		sprintf(ptr, "result=%d\n", 0);
	}
	else {		
		if(true == start_test_tp())
		{
			pr_err("tp test pass\n");
			 strcpy(result,"pass");
		}
		else
		{
			pr_err("tp test fail\n");
			 strcpy(result,"fail");
		}
		
		iTestDataLen = get_test_data(testdata);
		ftxxxx_SaveTestData("testdata.csv", testdata, iTestDataLen);
		free_test_param_data();
	}
		
	//mutex_unlock(&g_device_mutex);
	if(NULL != testdata) kfree(testdata);

	tpd_gpio_output(tpd_rst_gpio_number, 0);
	msleep(5);
	tpd_gpio_output(tpd_rst_gpio_number, 1);
	
	return  sprintf(ptr, "%s\n",result);
}
void create_ctp_proc(void)
{
    gtp_parent_proc= proc_mkdir(GTP_TEST, NULL);
	
    if(gtp_parent_proc)
    {
        gtp_open_proc= proc_create(OPEN_TEST, 0666, gtp_parent_proc, &ctp_openshort_procs_fops);
         if (gtp_open_proc == NULL)
        {
           printk("ft6436u: create OPEN_TEST fail\n");
        }
        gtp_short_proc= proc_create(SHORT_TEST, 0666, gtp_parent_proc, &ctp_short_procs_fops);
        if (gtp_short_proc == NULL)
        {
          printk("ft6436u: create SHORT_TEST fail\n");
        }
    }   
}

void remove_ctp_proc(void)
{
     if (gtp_short_proc != NULL)
    {
        remove_proc_entry(SHORT_TEST, gtp_parent_proc);    		
    }

    if (gtp_open_proc != NULL)
    {
        remove_proc_entry(OPEN_TEST, gtp_parent_proc);	
    }
   if (gtp_parent_proc != NULL)
    {
        remove_proc_entry(GTP_TEST, NULL);	
    }

}
#endif
