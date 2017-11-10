#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/proc_fs.h>

#include <linux/hardware_info.h>
//#include <lcm_drv.h>


#ifdef CONFIG_MTK_EMMC_SUPPORT
char emmc_device_name[30];//wingtech wangfuqiang add
static int __init set_emmc_name(char *str)
{
    int i = 0;
	printk("%s : %s\n",__func__,str);
	for(i = 0;  (*(str + i) != ' ' && *(str + i) != '\0'); i++);
	strncpy((char*)emmc_device_name, (char*)str, ((i<30) ? i:30));	
	return 1;
}
__setup("emmc_name=", set_emmc_name);
#endif

#ifdef WT_LCM_DEBUG   
static struct proc_dir_entry *lcmdebug_proc_entry;

void (*lcmdebug_write_function)(unsigned char* buffer, int len);
#endif	


static char hardwareinfo_name[HARDWARE_MAX_ITEM][HARDWARE_MAX_ITEM_LONGTH];

int hardwareinfo_set_prop(int cmd, char *name)
{
	if(cmd < 0 || cmd >= HARDWARE_MAX_ITEM)
		return -1;

	strcpy(hardwareinfo_name[cmd], name);

	return 0;
}
EXPORT_SYMBOL_GPL(hardwareinfo_set_prop);
#ifdef WT_LCM_DEBUG
EXPORT_SYMBOL_GPL(lcmdebug_write_function);
#endif

extern char ret_hw_secure_info[64];
extern char ret_sw_secure_info[64];

static long hardwareinfo_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
	int ret = 0, hardwareinfo_num;
	void __user *data = (void __user *)arg;

	//char info[HARDWARE_MAX_ITEM_LONGTH];
	
	switch (cmd) { 
	case HARDWARE_LCD_GET:
		hardwareinfo_num = HARDWARE_LCD;
		break;
	case HARDWARE_TP_GET:
		hardwareinfo_num = HARDWARE_TP;
		break;
	case HARDWARE_FLASH_GET:
		hardwareinfo_num = HARDWARE_FLASH;
		#ifdef CONFIG_MTK_EMMC_SUPPORT
		hardwareinfo_set_prop(HARDWARE_FLASH,emmc_device_name);
		#endif
		break;
	case HARDWARE_FRONT_CAM_GET:
		hardwareinfo_num = HARDWARE_FRONT_CAM;
		break;
	case HARDWARE_BACK_CAM_GET:
		hardwareinfo_num = HARDWARE_BACK_CAM;
		break;
	case HARDWARE_BT_GET:
		hardwareinfo_set_prop(HARDWARE_BT, "MT6625L");
		hardwareinfo_num = HARDWARE_BT;
		break;
	case HARDWARE_WIFI_GET:
		hardwareinfo_set_prop(HARDWARE_WIFI, "MT6625L");
		hardwareinfo_num = HARDWARE_WIFI;
		break;	
	case HARDWARE_ACCELEROMETER_GET:
		hardwareinfo_num = HARDWARE_ACCELEROMETER;
		break;
	case HARDWARE_ALSPS_GET:
		hardwareinfo_num = HARDWARE_ALSPS;
		break;
	case HARDWARE_GYROSCOPE_GET:
		hardwareinfo_num = HARDWARE_GYROSCOPE;
		break;
	case HARDWARE_MAGNETOMETER_GET:
		hardwareinfo_num = HARDWARE_MAGNETOMETER;
		break;		
	case HARDWARE_GPS_GET:
		hardwareinfo_set_prop(HARDWARE_GPS, "MT6625L");
	    hardwareinfo_num = HARDWARE_GPS;
		break;
	case HARDWARE_FM_GET:
		hardwareinfo_set_prop(HARDWARE_FM, "MT6625L");
	    hardwareinfo_num = HARDWARE_FM;		
		break;
	case HARDWARE_SDCARD_STATUS:
	    hardwareinfo_num = HARDWARE_SDSTATUS;		
		break;
/*	case HARDWARE_CHR_GET:
	    hardwareinfo_num = HARDWARE_CHR;		
		break;*/
	case HARDWARE_FRONT_CAM_ID_GET:
	    hardwareinfo_num = HARDWARE_FRONT_CAM_ID;
		break;
	case HARDWARE_BACK_CAM_ID_GET:
	    hardwareinfo_num = HARDWARE_BACK_CAM_ID;
		break;
	case HARDWARE_SECURE_INFO_GET:
		hardwareinfo_num = HARDWARE_SEC_INFO;
		hardwareinfo_set_prop(HARDWARE_SEC_INFO, ret_hw_secure_info);
		break;
	case SOFTWARE_SECURE_INFO_GET:
		hardwareinfo_num = SOFTWARE_SEC_INFO;
		hardwareinfo_set_prop(SOFTWARE_SEC_INFO, ret_sw_secure_info);
		break;
	default:
		ret = -EINVAL;
		goto err_out;
	}
	memset(data, 0, HARDWARE_MAX_ITEM_LONGTH);//clear the buffer
	if (copy_to_user(data, hardwareinfo_name[hardwareinfo_num], strlen(hardwareinfo_name[hardwareinfo_num]))){
		printk("%s, copy to usr error\n", __func__);
		ret =  -EINVAL;
	}
err_out:
	return ret;
}

#ifdef WT_LCM_DEBUG  
unsigned char lcm_debug_cmd[128];
unsigned char data_buffer[128];
int str2int8(unsigned char *buffer)
{
   int i = 0;
   
   for(i = 0; i < (strlen(buffer)-1)/2; i++)
   {
      if(buffer[2*i] >='0' && buffer[2*i] <= '9')
      {
          data_buffer[i] = (buffer[2*i] - '0');
      }
      else if(buffer[2*i] >='a' && buffer[2*i] <= 'f')
      {
          data_buffer[i] = (buffer[2*i] - 'a' + 0xa);
      }
      else if(buffer[2*i] >='A' && buffer[2*i] <= 'F')
      {
          data_buffer[i] = (buffer[2*i] - 'A' + 0xA);
      }
      
      data_buffer[i] = data_buffer[i] << 4;

      if(buffer[2*i+1] >='0' && buffer[2*i+1] <= '9')
      {
          data_buffer[i] |= (buffer[2*i+1] - '0');
      }
      else if(buffer[2*i+1] >='a' && buffer[2*i+1] <= 'f')
      {
          data_buffer[i] |= (buffer[2*i+1] - 'a' + 0xa);
      }
      else if(buffer[2*i+1] >='A' && buffer[2*i+1] <= 'F')
      {
          data_buffer[i] |= (buffer[2*i+1] - 'A' + 0xA);
      }
   }
   
    return i;
}
static s32 lcmdebug_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    u64 ret = 0;
    int data_len = 0;

memset(lcm_debug_cmd, 0, sizeof(lcm_debug_cmd));
memset(data_buffer, 0, sizeof(data_buffer));

    ret = copy_from_user(&lcm_debug_cmd, buff, len);
    if (ret)
    {
        printk("lcm_debug:copy_from_user failed.");
    }

	if(lcmdebug_write_function != NULL && !strncmp(lcm_debug_cmd, "debug:", strlen("debug:")))
	{
                data_len = str2int8(lcm_debug_cmd + strlen("debug:"));
		lcmdebug_write_function(data_buffer, data_len);
	}

return len;

}


void init_lcmdebug_node()
{
    lcmdebug_proc_entry = create_proc_entry("lcm_debug", 0666, NULL);	 
    if (lcmdebug_proc_entry == NULL)    
	{		
	       printk("lcm_debug:Couldn't create proc entry!");		 
	       //return FALSE;	 
	}	  
    else	  
	{ 	   printk("lcm_debug:Create proc entry success!");	
           lcmdebug_proc_entry->write_proc = lcmdebug_write;		
           //return TRUE;
    }
}
#endif

static struct file_operations hardwareinfo_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = hardwareinfo_ioctl,
};

static struct miscdevice hardwareinfo_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hardwareinfo",
	.fops = &hardwareinfo_fops,
};


static int __init hardwareinfo_init_module(void)
{
	int ret, i;
//	int fd = -1;

	for(i = 0; i < HARDWARE_MAX_ITEM; i++)
		strcpy(hardwareinfo_name[i], "NO THIS DEVICE");
	
	ret = misc_register(&hardwareinfo_device);
	if(ret < 0){
		printk("%s, misc_register error\n", __func__);
		return -ENODEV;
	}
	printk("%s, misc_register sucessful\n", __func__);
#ifdef WT_LCM_DEBUG  
	init_lcmdebug_node();
#endif

	
	return 0;
}

static void __exit hardwareinfo_exit_module(void)
{
	misc_deregister(&hardwareinfo_device);
}

subsys_initcall(hardwareinfo_init_module);
module_exit(hardwareinfo_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ming He <heming@wingtech.com>");


