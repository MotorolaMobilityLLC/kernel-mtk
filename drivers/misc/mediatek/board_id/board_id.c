// liupeng@wind-mobi.com 20161121 begin

#include "board_id.h"
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/slab.h>
//#include <mach/mt_gpio.h>
#include <mt-plat/mt_gpio.h>
#include <mach/gpio_const.h>
#include <asm/uaccess.h>
//driver
static struct platform_driver bid_platform_driver;
//pinctrl
/*
struct pinctrl *board_id_pin = NULL;
struct pinctrl_state *board_id_pin1 = NULL;
struct pinctrl_state *board_id_pin2 = NULL;
struct pinctrl_state *board_id_pin3 = NULL;
struct pinctrl_state *board_id_pin4 = NULL;
struct pinctrl_state *board_id_pin5 = NULL;
*/

// host/clust dev_id
static struct device *bid_dev;
static dev_t devno;
static int board_id_mojar=789;
static int board_id_minor=0;
static int num=0;
char num0,num1,num2,num3,num4,num5=0;
static char *board_str=NULL;
extern char* saved_command_line;
int wind_board_id = 0;	//tuwenzan@wind-mobi.com modify at 20170112
//huyunge@wind-mobi.com 20161227 start
static int bid_atoi(char *nptr)
 {
	int value=0;
	printk("hyg --atoi--start %s\n",nptr);
	while(*nptr>='0' && *nptr<='1')
	{
		value *= 2;
		value += *nptr - '0';
		nptr++;
	}
	printk("hyg --atoi--end %d\n",value);
	 return value;
 }
 //huyunge@wind-mobi.com 20161227 end*/
//class node ,DEV
static struct class *bid_class=NULL;
static struct board_id_dev *biddev=NULL;
/* for test lk pid param is ok??*/
// parse bid param
int get_bid_gpio(void)
{
	int ret=0xff;
	char *p= NULL;
	unsigned char len=6;
	unsigned char tmp[]="xxxxxx\n";
	//memset(p,0,len);
	p = strstr(saved_command_line, "bid_num=");
	p +=strlen("bid_num=");
	
	printk("[get_bid_gpio]can not get bid size strlen(tmp) %d\n",strlen(tmp));
	
	memset(tmp,0,strlen(tmp));
	strncpy(tmp,p,len);
	//huyunge@wind-mobi.com 20161208 stat
	if (p == NULL) {
		printk("[get_bid_gpio]can not get bid size from lk\n");
	} else {
		printk("[get_bid_gpio] bid is tmp %s\n",tmp);
		ret = bid_atoi(tmp);
		printk("[get_bid_gpio] bid is 0x%x\n",ret);
	}
	//huyunge@wind-mobi.com 20161208 end
	return ret;
}
//file ops
static ssize_t  bid_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset){
	//int ret =0;
	//char *bid_num=NULL;
	struct board_id_dev *biddev=filp->private_data;
	if(count>BID_MAX_SIZE){
		printk("dev max size >128 \n");
	}
	mutex_lock(&biddev->board_mutex);
	/*if(board_str==NULL){
		printk("board id is NULL \n");
		return 0;
	}*/
	////
	//sprintf(bid_num,"%s",num);
	if(copy_to_user(buf,&num,sizeof(num))){
		return 0;
	}
	mutex_unlock(&biddev->board_mutex);
	return 0; 
}

static ssize_t bid_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset){
	printk("board id %s have nothing to operation\n",__func__);
	return 0;
}

static loff_t bid_dev_llseek(struct file *filp ,loff_t offset,int orig){
	printk("board id %s have nothing to operation\n",__func__);
	return 0;
}

static int bid_dev_open(struct inode *inode,struct file *filp){
	
	struct board_id_dev *biddev=container_of(filp->private_data,struct board_id_dev,dev);
	
	filp->private_data=biddev;
	
	return 0;
}

static long  bid_dev_unlocked_ioctl(struct file *filp ,unsigned int cmd ,unsigned long args){
	//struct board_id_dev *biddev=filp->private_data;
	
	switch(cmd){ //if you need do somesthing for board id ,please add func at here.
		//read board_id --sunsiyuan@wind-mobi.com add at 20161229 begin
		case BOARD_ID_GET_VALUE:
			if (copy_to_user((void __user *)args, (void *)&num, sizeof(num))) {
				return -EFAULT;
			}
			break;
		//read board_id --sunsiyuan@wind-mobi.com add at 20161229 end
		case BOARD_ID_SET_PWR:
			if(args==AP_DS_NA_EVT)
				printk("this board is AP_DS_NA_EVT");
			else if(args==EMEA_DS_NA_EVT)
				printk("this board is EMEA_DS_NA_EVT");
			else if(args==EMEA_SS_NFC_EVT)
				printk("this board is EMEA_SS_NFC_EVT");
			else if(args==LATAM_DS_NA_EVT)
				printk("this board is LATAM_DS_NA_EVT");
			else if(args==ROLA_SS_NA_EVT)
				printk("this board is ROLA_SS_NA_EVT");
			else if(args==AP_DS_NA_DVT)
				printk("this board is AP_DS_NA_DVT");
			else if(args==EMEA_DS_NA_DVT)
				printk("this board is EMEA_DS_NA_DVT");
			else if(args==EMEA_SS_NA_DVT)
				printk("this board is EMEA_SS_NA_DVT");
			else if(args==EMEA_SS_NFC_DVT)
				printk("this board is EMEA_SS_NFC_DVT");
			else if(args==LATAM_DS_NA_DVT)
				printk("this board is LATAM_DS_NA_DVT");
			else if(args==ROLA_SS_NA_DVT)
				printk("this board is ROLA_SS_NA_DVT");
			else if(args==AP_DS_NA_DVT2)
				printk("this board is AP_DS_NA_DVT2");
			else if(args==EMEA_DS_NA_DVT2)
				printk("this board is EMEA_DS_NA_DVT2");
			else if(args==EMEA_SS_NFC_DVT2)
				printk("this board is EMEA_SS_NFC_DVT2");
			else if(args==LATAM_DS_NA_SKY77643_DVT2)
				printk("this board is LATAM_DS_NA_SKY77643_DVT2");
			else if(args==ROLA_SS_NA_SKY77643_DVT2)
				printk("this board is ROLA_SS_NA_SKY77643_DVT2");
			else if(args==LATAM_DS_NA_SKY77638_DVT2)
				printk("this board is LATAM_DS_NA_SKY77638_DVT2");
			else if(args==ROLA_SS_NA_SKY77638_DVT2)
				printk("this board is ROLA_SS_NA_SKY77638_DVT2");
			else if(args==AP_B28_DS_NA_DVT2)
				printk("this board is AP_B28_DS_NA_DVT2");
//sunjingtao@wind-mobi.com at 20170119 begin
			else if(args==AP_DS_NA_DVT2_1)
				printk("this board is AP_DS_NA_DVT2_1");
			else if(args==EMEA_DS_NA_DVT2_1)
				printk("this board is EMEA_DS_NA_DVT2_1");
			else if(args==EMEA_SS_NFC_DVT2_1)
				printk("this board is EMEA_SS_NFC_DVT2_1");
			else if(args==LATAM_DS_NA_SKY77643_DVT2_1)
				printk("this board is LATAM_DS_NA_SKY77643_DVT2_1");
			else if(args==ROLA_SS_NA_SKY77643_DVT2_1)
				printk("this board is ROLA_SS_NA_SKY77643_DVT2_1");
			else if(args==LATAM_DS_NA_SKY77638_DVT2_1)
				printk("this board is LATAM_DS_NA_SKY77638_DVT2_1");
			else if(args==ROLA_SS_NA_SKY77638_DVT2_1)
				printk("this board is ROLA_SS_NA_SKY77638_DVT2_1");
			else if(args==AP_B28_DS_NA_DVT2_1)
				printk("this board is AP_B28_DS_NA_DVT2_1");
//sunjingtao@wind-mobi.com at 20170119 end
//sunjingtao@wind-mobi.com at 20170322 begin
			else if(args==AP_B28_DS_SKY77638_DVT2_1)
				printk("this board is AP_B28_DS_SKY77638_DVT2_1");
			else if(args==AP_DS_NA_MP)
				printk("this board is AP_DS_NA_MP");
			else if(args==EMEA_DS_NA_MP)
				printk("this board is EMEA_DS_NA_MP");
			else if(args==EMEA_SS_NFC_MP)
				printk("this board is EMEA_SS_NFC_MP");
			else if(args==LATAM_DS_NA_SKY77643_MP)
				printk("this board is LATAM_DS_NA_SKY77643_MP");
			else if(args==ROLA_SS_NA_SKY77643_MP)
				printk("this board is ROLA_SS_NA_SKY77643_MP");
			else if(args==LATAM_DS_NA_SKY77638_MP)
				printk("this board is LATAM_DS_NA_SKY77638_MP");
			else if(args==ROLA_SS_NA_SKY77638_MP)
				printk("this board is ROLA_SS_NA_SKY77638_MP");
			else if(args==AP_B28_DS_NA_MP)
				printk("this board is AP_B28_DS_NA_MP");
			else if(args==AP_B28_DS_SKY77638_MP)
				printk("this board is AP_B28_DS_SKY77638_MP");
//sunjingtao@wind-mobi.com at 20170322 end
			break;
		default:
			break;
	}
	return 0;
}

//attr 
static ssize_t bid_num_store(struct device *dev,struct device_attribute *attr, const char *buf,size_t count){
	return 0;
}

static ssize_t bid_num_show(struct device *dev,struct device_attribute *attr, char *buf){
	
	if(board_str==NULL)
	return sprintf(buf,"%s","null");
	return sprintf(buf,"%s",board_str);
}

static DEVICE_ATTR(biddevnum, S_IRUGO|S_IWUSR, bid_num_show, bid_num_store);

static struct attribute *biddev_attributes[] = {
	&dev_attr_biddevnum.attr,
	NULL
};

static const struct  file_operations bid_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = bid_dev_llseek,
	.read = bid_dev_read,
	.write = bid_dev_write,
	.open = bid_dev_open,
	.unlocked_ioctl = bid_dev_unlocked_ioctl,
};
	
static struct attribute_group biddev_attribute_group = {
	.attrs = biddev_attributes
};

//probe
static int bid_platform_probe(struct platform_device *pdev){
	unsigned char ret =0;
	//dts pin
	/*board_id_pin = devm_pinctrl_get(&biddev->dev);
	if (IS_ERR(board_id_pin)) {
		printk("read board_id (dts)gpio err\n");
		return PTR_ERR(board_id_pin);
	}

	board_id_pin1 = pinctrl_lookup_state(board_id_pin, "bid_pin1");
	if (IS_ERR(board_id_pin1)) {
		ret = PTR_ERR(board_id_pin1);
		printk("%s: pinctrl err, board_id_pin1\n", __func__);
	}
	
	board_id_pin2 = pinctrl_lookup_state(board_id_pin, "bid_pin2");
	if (IS_ERR(board_id_pin2)) {
		ret = PTR_ERR(board_id_pin2);
		printk("%s: pinctrl err, board_id_pin2\n", __func__);
	}
	
	board_id_pin3 = pinctrl_lookup_state(board_id_pin, "bid_pin3");
	if (IS_ERR(board_id_pin3)) {
		ret = PTR_ERR(board_id_pin3);
		printk("%s: pinctrl err, board_id_pin2\n", __func__);
	}
	
	board_id_pin4 = pinctrl_lookup_state(board_id_pin, "bid_pin4");
	if (IS_ERR(board_id_pin4)) {
		ret = PTR_ERR(board_id_pin4);
		printk("%s: pinctrl err, board_id_pin4\n", __func__);
	}
	
	board_id_pin5 = pinctrl_lookup_state(board_id_pin, "bid_pin5");
	if (IS_ERR(board_id_pin5)) {
		ret = PTR_ERR(board_id_pin5);
		printk("%s: pinctrl err, board_id_pin5\n", __func__);
	}
	
	num5= pinctrl_select_state(board_id_pin, board_id_pin5);
	num4= pinctrl_select_state(board_id_pin, board_id_pin4);
	num3= pinctrl_select_state(board_id_pin, board_id_pin3);
	num2= pinctrl_select_state(board_id_pin, board_id_pin2);
	num1= pinctrl_select_state(board_id_pin, board_id_pin1);
	*/
	#ifdef GPIO_TYPE_ID5	
	mt_set_gpio_mode(GPIO_TYPE_ID5, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_TYPE_ID5, GPIO_DIR_IN);
	num5=mt_get_gpio_in(GPIO_TYPE_ID5);
	#endif
	
	#ifdef GPIO_TYPE_ID4	
	mt_set_gpio_mode(GPIO_TYPE_ID4, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_TYPE_ID4, GPIO_DIR_IN);
	num4=mt_get_gpio_in(GPIO_TYPE_ID4);
	#endif
	
	#ifdef GPIO_TYPE_ID3	
	mt_set_gpio_mode(GPIO_TYPE_ID3, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_TYPE_ID3, GPIO_DIR_IN);
	num3=mt_get_gpio_in(GPIO_TYPE_ID3);
	#endif
	
	#ifdef GPIO_TYPE_ID2	
	mt_set_gpio_mode(GPIO_TYPE_ID2, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_TYPE_ID2, GPIO_DIR_IN);
	num2=mt_get_gpio_in(GPIO_TYPE_ID2);
	#endif
	
	#ifdef GPIO_TYPE_ID1	
	mt_set_gpio_mode(GPIO_TYPE_ID1, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_TYPE_ID1, GPIO_DIR_IN);
	num1=mt_get_gpio_in(GPIO_TYPE_ID1);
	#endif
	
	#ifdef GPIO_TYPE_ID0	
	mt_set_gpio_mode(GPIO_TYPE_ID0, GPIO_MODE_00);	
	mt_set_gpio_dir(GPIO_TYPE_ID0, GPIO_DIR_IN);
	num0=mt_get_gpio_in(GPIO_TYPE_ID0);
	#endif
	
	
	num=(num5<<5)|(num4<<4)|(num3<<3)|(num2<<2)|(num1<<1)|(num0<<0);
	printk("the board id is num=%x\n",num);
	wind_board_id = num;	//tuwenzan@wind-mobi.com add at 20170112
	switch(num){
	case AP_DS_NA_EVT: //00
		board_str="000000";
		break;
    case EMEA_DS_NA_EVT:	//01
		board_str="000001";
		break;
	case EMEA_SS_NA_EVT:		//02
		board_str="000010";
		break;
	case EMEA_SS_NFC_EVT:	//03
		board_str="000011";
		break;
	case LATAM_DS_NA_EVT:	//04
		board_str="000100";
		break;
	case ROLA_SS_NA_EVT:		//05
		board_str="000101";
		break;
	case AP_DS_NA_DVT:		//06
		board_str="000110";
		break;
	case EMEA_DS_NA_DVT:		//07
		board_str="000111";
		break;
	case EMEA_SS_NA_DVT:		//08
		board_str="001000";
		break;
	case EMEA_SS_NFC_DVT:	//09
		board_str="001001";
		break;
	case LATAM_DS_NA_DVT:	//0a
		board_str="001010";
		break;
	case ROLA_SS_NA_DVT:		//0B
		board_str="001011";
		break;
	case AP_DS_NA_DVT2:		//0c
		board_str="001100";
		break;
	case EMEA_DS_NA_DVT2:		//0d
		board_str="001101";
		break;
	case EMEA_SS_NFC_DVT2:	//0e
		board_str="001110";
		break;
	case LATAM_DS_NA_SKY77643_DVT2:	//0f
		board_str="001111";
		break;
	case ROLA_SS_NA_SKY77643_DVT2:		//10
		board_str="010000";
		break;
	case LATAM_DS_NA_SKY77638_DVT2:	//11
		board_str="010001";
		break;
	case ROLA_SS_NA_SKY77638_DVT2:		//12
		board_str="010010";
		break;
	case AP_B28_DS_NA_DVT2:		//13
		board_str="010011";
//sunjingtao@wind-mobi.com at 20170119 begin
	case AP_DS_NA_DVT2_1:		//14
		board_str="010100";
		break;
	case EMEA_DS_NA_DVT2_1:		//15
		board_str="010101";
		break;
	case EMEA_SS_NFC_DVT2_1:	//16
		board_str="010110";
		break;
	case LATAM_DS_NA_SKY77643_DVT2_1:	//17
		board_str="010111";
		break;
	case ROLA_SS_NA_SKY77643_DVT2_1:		//18
		board_str="011000";
		break;
	case LATAM_DS_NA_SKY77638_DVT2_1:	//19
		board_str="011001";
		break;
	case ROLA_SS_NA_SKY77638_DVT2_1:		//1a
		board_str="011010";
		break;
	case AP_B28_DS_NA_DVT2_1:		//1b
		board_str="011011";
//sunjingtao@wind-mobi.com at 20170119 end
//sunjingtao@wind-mobi.com at 20170322 begin
	case AP_B28_DS_SKY77638_DVT2_1:		//1c
		board_str="011100";
		break;
	case AP_DS_NA_MP:		//1d
		board_str="011101";
		break;
	case EMEA_DS_NA_MP:		//1e
		board_str="011110";
		break;
	case EMEA_SS_NFC_MP:	//1f
		board_str="011111";
		break;
	case LATAM_DS_NA_SKY77643_MP:	//20
		board_str="100000";
		break;
	case ROLA_SS_NA_SKY77643_MP:		//21
		board_str="100001";
		break;
	case LATAM_DS_NA_SKY77638_MP:	//22
		board_str="100010";
		break;
	case ROLA_SS_NA_SKY77638_MP:		//23
		board_str="100011";
		break;
	case AP_B28_DS_NA_MP:		//24
		board_str="100100";
		break;
	case AP_B28_DS_SKY77638_MP:		//25
		board_str="100101";
		break;
//sunjingtao@wind-mobi.com at 20170322 end
	default:
		break;
	}
	//class node w/r 
	/*biddev->kobj=kobject_create_and_add("biddev",NULL);
	if(biddev->kobj==NULL)
		return -ENOMEM;

	ret=sysfs_create_file(biddev->kobj,biddev_attribute_group);
	if(ret<0){
		kobject_del(biddev->kobj);
		return -ENOMEM;
	}*/
	
	ret = sysfs_create_group(&pdev->dev.kobj, &biddev_attribute_group);
	if (0 != ret) 
	{
		printk("%s() - ERROR: sysfs_create_group() failed.\n", __func__);
		sysfs_remove_group(&pdev->dev.kobj, &biddev_attribute_group);
		return -EIO;
	} 
	else 
	{
		printk("sysfs_create_group() succeeded.\n");
	}
	
	return 0;
}

static int bid_platform_remove(struct platform_device *pdev){
	//kobject_del(biddev->kobj);
	printk("biddev platform remove");
	return 0;
}


/*  platform driver */
static const struct of_device_id bid_platform_of_match[] = {
	{.compatible = "mediatek,bid",},
	{},
};

static struct platform_driver bid_platform_driver = {
	.probe		= bid_platform_probe,
	.remove		= bid_platform_remove,
	.driver		= {
		.name = BOARD_ID_DRVNAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = bid_platform_of_match,
#endif
	},

};
static int __init bid_dev_init(void){
	int ret=0;
	//cdev
	devno = MKDEV(board_id_mojar,board_id_minor);
	if(board_id_mojar)
	ret=register_chrdev_region(devno,0,"bid");
	else{
	ret=alloc_chrdev_region(&devno,0,1,"bid");
	board_id_mojar=MAJOR(devno);
	}
	
	if(ret<0)
		return ret;
	biddev=kmalloc(sizeof(struct board_id_dev),GFP_KERNEL);
	
	if(!biddev){
		unregister_chrdev_region(devno,1);
		return -ENOMEM;
	}
	memset(biddev,0,sizeof(struct board_id_dev));
	
	cdev_init(&biddev->dev,&bid_dev_fops);
	biddev->dev.owner=THIS_MODULE;
	ret=cdev_add(&biddev->dev,devno,1);
	if(ret)
	printk("create board_id_z168 cdev fail,ret=%d\n",ret);
	
	mutex_init(&biddev->board_mutex);
	//class  node
	bid_class=class_create(THIS_MODULE,"board_id_z168");
	if(IS_ERR(bid_class))
		printk("board_id_z168 create class node failed\n");
	bid_dev=device_create(bid_class,NULL,devno,NULL,"board_id_z168_dev");
	if(IS_ERR(bid_dev))
		printk("board_id_z168 create dev failed\n");

	//driver 
	ret=platform_driver_register(&bid_platform_driver);
	if(ret)
		printk("board_id_z168 create driver fail");
		
	return 0;
}
arch_initcall(bid_dev_init); //tuwenzan@wind-mobi.com add at 20170112


static void __exit bid_dev_exit(void){
	//cdev
	cdev_del(&biddev->dev);
	kfree(biddev);
	device_del(bid_dev);
	unregister_chrdev_region(devno,1);
	//class node
	class_destroy(bid_class);
	printk("board id exit\n");
}
module_exit(bid_dev_exit);

MODULE_AUTHOR("lp");
MODULE_DESCRIPTION("board id driver");
MODULE_LICENSE("GPL");


// liupeng@wind-mobi.com 20161121 end