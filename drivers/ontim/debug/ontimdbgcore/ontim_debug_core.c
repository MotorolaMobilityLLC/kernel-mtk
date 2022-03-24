#define ONTIM_DEBUG_CORE_C
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/stat.h>
#include <linux/list.h>
#include <linux/module.h>
#include <ontim/ontim_dev_dgb.h>
#include <linux/i2c.h>
#include <linux/gpio.h>

  
struct core_dev_attr
{
    struct dev_attr *dev_attr;
};

#define DEBUG_TRACE_LEVEL_NAME "dbg_level"


struct ontim_core_debug
{
     char name[NAME_LEN];
     struct ontim_debug *dbg;
     struct list_head node;
     struct kobj_attribute  *kobj_attr;
};

static LIST_HEAD(ontim_core_debug_list);
static DEFINE_MUTEX(sysfs_lock);
static struct kobject *ontim_core_dbg_kobj;
static struct kobject *ontim_iic_dbg_kobj;
static struct kobject *ontim_gpio_dbg_kobj;

#define ACCESS_LEVEL 0644

char* get_dgb_name(const char *name,struct ontim_debug **ontim_debug)
{
    struct ontim_core_debug *entry;
    *ontim_debug=NULL;
    if (list_empty(&ontim_core_debug_list))
	return NULL;
    if(name==NULL)
        return NULL;
    list_for_each_entry(entry, &ontim_core_debug_list, node) {
        if (!strncmp(entry->name,name,NAME_LEN)) {
             //printk(KERN_ERR "entry->name=%s\n",entry->name);
             *ontim_debug=entry->dbg;
             //printk(KERN_ERR "*ontim_debug->dev_name=%s\n",(*ontim_debug)->dev_name);
	     return entry->name;
        }
    }
    return NULL;
}

char* get_dgb_dev_attr_name(char *name,struct dev_attr *dev_attr_array)
{
    return NULL;
}


static ssize_t ontim_core_debug_dev_attr_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
	//const struct gpio_desc	*desc = dev_get_drvdata(dev);
	ssize_t	status=0;
        char *s_name ;
        int i;
        struct dev_arrt *dev_attr_array;
        struct ontim_debug *ontim_debug;
	mutex_lock(&sysfs_lock);

//        printk(KERN_ERR "show name =%s kobj->parent->name=%s\n",attr->attr.name,kobj->parent->name);
//        printk(KERN_ERR "kobj->name=%s\n",kobj->name);
        s_name=get_dgb_name(kobj->name,&ontim_debug);
//        printk(KERN_ERR "s_name=%s\n",s_name);
        dev_attr_array=ontim_debug->dev_attr_array;
        if(ontim_debug){

             for(i=0;i<ontim_debug->dev_attr_array_len;i++)
             {
                 if(dev_attr_array[i].dev_attr_name){
                     if(!strcmp(dev_attr_array[i].dev_attr_name,attr->attr.name)){
//+modify by hzb
                          if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_VAL_8BIT)
                              status=sprintf(buf,"0x%02x\n",*((u8 *)dev_attr_array[i].dev_attr_context));
                          else if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_VAL_16BIT)
                              status=sprintf(buf,"0x%04x\n",*((u16 *)dev_attr_array[i].dev_attr_context));
                          else if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_VAL_32BIT)
                              status=sprintf(buf,"0x%08x\n",*((u32 *)dev_attr_array[i].dev_attr_context));
                          else if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_EXEC)
                              status=sprintf(buf,"Operation failed!!! \n");
                          else if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_VAL_RO)
                              status=sprintf(buf,"0x%x\n",*((u32 *)dev_attr_array[i].dev_attr_context));
                          else
                              status=sprintf(buf,"%s\n",(char *)dev_attr_array[i].dev_attr_context);
//-modify by hzb
                          break;
                     }
                 }
             }
        }
	mutex_unlock(&sysfs_lock);
	return status;
}

//+add by hzb
static ssize_t ontim_core_debug_dev_attr_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	//const struct gpio_desc	*desc = dev_get_drvdata(dev);
    //ssize_t	status=0;
    char *s_name ;
    int i,res=0;
    struct dev_arrt *dev_attr_array;
    struct ontim_debug *ontim_debug;
    mutex_lock(&sysfs_lock);

    //        printk(KERN_ERR "show name =%s kobj->parent->name=%s\n",attr->attr.name,kobj->parent->name);
    //        printk(KERN_ERR "kobj->name=%s\n",kobj->name);
    s_name=get_dgb_name(kobj->name,&ontim_debug);
    //        printk(KERN_ERR "s_name=%s\n",s_name);
    dev_attr_array=ontim_debug->dev_attr_array;
    if(ontim_debug){

        for(i=0;i<ontim_debug->dev_attr_array_len;i++)
        {
            if(dev_attr_array[i].dev_attr_name){
                if(!strcmp(dev_attr_array[i].dev_attr_name,attr->attr.name)){
                    unsigned int val;

			if(strnstr(buf,"0x",2))
			    res = kstrtouint(buf, 16, &val);
			else
			    res = kstrtouint(buf, 10, &val);

                    if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_VAL_8BIT)
                        *((u8 *)dev_attr_array[i].dev_attr_context)=val;
                    else if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_VAL_16BIT)
                        *((u16 *)dev_attr_array[i].dev_attr_context)=val;
                    else if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_VAL_32BIT)
                        *((u32 *)dev_attr_array[i].dev_attr_context)=val;
                    else if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_EXEC)
                        ((void (*)(int ))(dev_attr_array[i].dev_attr_context))(val);
                    else if (dev_attr_array[i].dev_attr_type == ONTIM_DEV_ARTTR_TYPE_EXEC_PARAM)
                        //((void (*)(int ))(dev_attr_array[i].dev_attr_context))(buf);
                    break;
                }
            }
        }
    }
    mutex_unlock(&sysfs_lock);
    return count;
}
//-add by hzb

static ssize_t ontim_core_debug_dev_attr_debug_trace_level_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
    //const struct gpio_desc	*desc = dev_get_drvdata(dev);
    ssize_t	status=0;
    char *s_name ;
    struct ontim_debug *ontim_debug;
    mutex_lock(&sysfs_lock);

    //        printk(KERN_ERR "show name =%s kobj->parent->name=%s\n",attr->attr.name,kobj->parent->name);
    //        printk(KERN_ERR "kobj->name=%s\n",kobj->name);
    s_name=get_dgb_name(kobj->name,&ontim_debug);
    //        printk(KERN_ERR "s_name=%s\n",s_name);
    if(ontim_debug){
        status=sprintf(buf,"%s=%u\n",DEBUG_TRACE_LEVEL_NAME,ontim_debug->dbg_trace_level);
    }
    mutex_unlock(&sysfs_lock);
    return status;
}
static ssize_t ontim_core_debug_dev_attr_debug_trace_level_store(struct kobject *kobj, struct kobj_attribute *attr,
        const char *buf, size_t count)
{
    unsigned int  dbg_trace_level;
    int res;
    char *s_name ;
    struct ontim_debug *ontim_debug;
    mutex_lock(&sysfs_lock);
    res = kstrtouint(buf, 10, &dbg_trace_level);

    //        printk(KERN_ERR "show name =%s kobj->parent->name=%s\n",attr->attr.name,kobj->parent->name);
    //        printk(KERN_ERR "kobj->name=%s\n",kobj->name);
    s_name=get_dgb_name(kobj->name,&ontim_debug);
    //        printk(KERN_ERR "s_name=%s\n",s_name);
    if(ontim_debug){
        ontim_debug->dbg_trace_level=dbg_trace_level;
    }
    mutex_unlock(&sysfs_lock);
    return count;
}



static const char debug_dev_trace_leve_name[]=DEBUG_TRACE_LEVEL_NAME;
int ontim_dev_debug_file_exist(struct ontim_debug *ontim_debug)
{
    int retval=0;
    struct kernfs_node	*ontim_debug_sd;
    ontim_debug_sd = sysfs_get_dirent(ontim_core_dbg_kobj->sd, ontim_debug->dev_name);
    if (!ontim_debug_sd) {
        retval = -ENODEV;
        printk(KERN_ERR "this direction is not exist \n");
    }
    return retval;
}
#define ONTIM_DEBUG_DEV_NUM 20
#define PER_DEV_ELEMENT_NUM  10
static struct kobj_attribute kobj_attribute_arrary[ONTIM_DEBUG_DEV_NUM*PER_DEV_ELEMENT_NUM];
static char kobj_attribute_arrary_state[ONTIM_DEBUG_DEV_NUM*PER_DEV_ELEMENT_NUM];
struct kobj_attribute * alloc_attribute_from_kobj_attribute_arrary(int num)
{
    int i=0;
    for(i=0;i<ONTIM_DEBUG_DEV_NUM*PER_DEV_ELEMENT_NUM;i++)
     {
            if( kobj_attribute_arrary_state[i]==0)
            {
                   if(num<ONTIM_DEBUG_DEV_NUM*PER_DEV_ELEMENT_NUM-i)
                   {
                         int j=0;
			    for(j=0;j<num;j++){
                              kobj_attribute_arrary_state[i+j] =1;
			    }
                   	    return &kobj_attribute_arrary[i];
                   }
		     else
		        return NULL;
            }
     }
     return NULL;
}

int regester_and_init_ontim_debug(struct ontim_debug *ontim_debug)
{
    int retval=0;
    int i=0;
    struct kobj_attribute *kobj_attr;
    struct attribute **properties_attrs;
    struct attribute_group *properties_attr_group;
    struct kobject *ontim_dev_debu_kobj;
    struct ontim_core_debug *ontim_core_dbg_item;
    struct kernfs_node	*ontim_debug_sd;



    ontim_core_dbg_item=kzalloc(sizeof(struct ontim_core_debug),GFP_KERNEL);

    if(ontim_core_dbg_item==NULL)
       goto malloc_fail;
    INIT_LIST_HEAD(&ontim_core_dbg_item->node);
    ontim_core_dbg_item->dbg=ontim_debug;
    strncpy(ontim_core_dbg_item->name,ontim_debug->dev_name,NAME_LEN);
//    ontim_debug->dbg_trace_level=0;
    ontim_debug_sd = sysfs_get_dirent(ontim_core_dbg_kobj->sd, ontim_core_dbg_item->name);
    if (ontim_debug_sd) {
	retval = -ENODEV;
        printk(KERN_ERR "this direction is exist \n");
	goto malloc_fail1;
    }

    ontim_dev_debu_kobj = kobject_create_and_add(ontim_core_dbg_item->name, ontim_core_dbg_kobj);
    if(ontim_dev_debu_kobj==NULL){
       goto malloc_fail1;
    }

//    kobj_attr=kzalloc(sizeof(struct kobj_attribute)*(ontim_debug->dev_attr_array_len+1),GFP_KERNEL);
    kobj_attr=alloc_attribute_from_kobj_attribute_arrary(ontim_debug->dev_attr_array_len+1);
    if(kobj_attr==NULL)
       goto malloc_fail1;

    properties_attrs=kzalloc(sizeof(struct attribute *)*(ontim_debug->dev_attr_array_len+2),GFP_KERNEL);
    if(properties_attrs==NULL)
       goto malloc_fail2;

    for(i=0;i<ontim_debug->dev_attr_array_len;i++)
    {
       kobj_attr[i].attr.name=(const char*)(ontim_debug->dev_attr_array[i].dev_attr_name);
       kobj_attr[i].attr.mode =ACCESS_LEVEL;
       kobj_attr[i].show=ontim_core_debug_dev_attr_show;
	kobj_attr[i].store=ontim_core_debug_dev_attr_store;
       properties_attrs[i]=&kobj_attr[i].attr;
    }

    kobj_attr[i].attr.name=debug_dev_trace_leve_name;
    kobj_attr[i].attr.mode =ACCESS_LEVEL;
    kobj_attr[i].show=ontim_core_debug_dev_attr_debug_trace_level_show;
    kobj_attr[i].store=ontim_core_debug_dev_attr_debug_trace_level_store;
    properties_attrs[i]=&kobj_attr[i].attr;
    properties_attrs[i+1]=NULL;





    properties_attr_group=kzalloc(sizeof(struct attribute_group),GFP_KERNEL);
    if(properties_attr_group==NULL)
       goto malloc_fail3;
    properties_attr_group->attrs=properties_attrs;

    retval = sysfs_create_group(ontim_dev_debu_kobj, properties_attr_group);
    if (retval){
	printk(KERN_ERR "failed to ontim debug,dev_name=%s\n",ontim_debug->dev_name);
        goto  exit;
    }
    list_add_tail(&ontim_core_dbg_item->node, &ontim_core_debug_list);

    return 0;
malloc_fail3:
    kfree(properties_attrs);
malloc_fail2:
    kfree(kobj_attr);
malloc_fail1:
    kfree(ontim_core_dbg_item);
malloc_fail:
exit:
    return -ENOMEM;

}



#define ONTIM_DEV_IIC_DEBUG_NAME_LEN  32
#define ONTIM_DEV_IIC_DEBUG_DATA_LEN  32
struct ontim_iic_dev_dbg_data
{
      unsigned int i_chip_num;
      unsigned int i_addr;
      unsigned int i_register_width;
      unsigned int i_data_width;
      unsigned int i_error;
};
static struct ontim_iic_dev_dbg_data  ontim_iic_dev_dbg_data=
{
    .i_chip_num=1,
    .i_addr=-1,
    .i_register_width=1,
    .i_data_width=1,
};

int check_data_valid(unsigned int  value,unsigned int width);
int covert_data_sort_byte_by_data_width(unsigned int  value,unsigned int width,unsigned char *buf);

/*+++++++++++++++++++++++++++++++++++++++++++*/
static ssize_t ontim_iic_chipnum_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
      int status;
      unsigned int i_chip_num=ontim_iic_dev_dbg_data.i_chip_num;
      mutex_lock(&sysfs_lock);
      if(i_chip_num>=0&&i_chip_num<8)
           status=sprintf(buf,"%s=0x%x\n","chip_num",i_chip_num);
      else
           status=sprintf(buf,"chip_num=error num!!!\n");
      mutex_unlock(&sysfs_lock);
      return status;
}
static ssize_t ontim_iic_chipnum_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
       unsigned int i_chip_num;
       int res=0;
	mutex_lock(&sysfs_lock);
	if(strnstr(buf,"0x",2))
	    res = kstrtouint(buf, 16, &i_chip_num);
	else
	    res = kstrtouint(buf, 10, &i_chip_num);
        if(!res&&i_chip_num>=0&&i_chip_num<8)
	      ontim_iic_dev_dbg_data.i_chip_num=i_chip_num;
	else
        {
           printk(KERN_ERR "input chip number have a error\n");
	   ontim_iic_dev_dbg_data.i_chip_num=0xffff;
        }
 	mutex_unlock(&sysfs_lock);
	return count;
}
static struct kobj_attribute ontim_iic_chipnum_attr = {
	.attr = {
		.name = "chip_num",
		.mode = ACCESS_LEVEL,
	},
	.show =&ontim_iic_chipnum_show,
	.store= &ontim_iic_chipnum_store,
};
/*--------------------------------------------------------------*/
/*+++++++++++++++++++++++++++++++++++++++++++*/
static ssize_t ontim_iic_addr_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
      int status;
      unsigned int i2c_addr=ontim_iic_dev_dbg_data.i_addr;
      mutex_lock(&sysfs_lock);
      if(i2c_addr>=0&&i2c_addr<128)
           status=sprintf(buf,"%s=0x%x\n","i2c_addr",i2c_addr);
      else
           status=sprintf(buf,"i2c_addr=error num!!!\n");
      mutex_unlock(&sysfs_lock);
      return status;
}
static ssize_t ontim_iic_addr_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
       unsigned int i2c_addr;
       int res=0;
	mutex_lock(&sysfs_lock);
	if(strnstr(buf,"0x",2))
	    res = kstrtouint(buf, 16, &i2c_addr);
	else
	    res = kstrtouint(buf, 10, &i2c_addr);
       if(!res&&i2c_addr>=0&&i2c_addr<128)
	      ontim_iic_dev_dbg_data.i_addr=i2c_addr;
	else
        {
	    ontim_iic_dev_dbg_data.i_addr=0xffff;
            printk(KERN_ERR "input iic address have a error\n");
        }
 	mutex_unlock(&sysfs_lock);
	return count;
}
static struct kobj_attribute ontim_iic_addr_attr = {
	.attr = {
		.name = "i2c_addr",
		.mode = ACCESS_LEVEL,
	},
	.show =&ontim_iic_addr_show,
	.store= &ontim_iic_addr_store,
};
/*--------------------------------------------------------------*/
/*+++++++++++++++++++++++++++++++++++++++++++*/
static ssize_t ontim_iic_register_width_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
      int status;
       unsigned int register_width=ontim_iic_dev_dbg_data.i_register_width;
      mutex_lock(&sysfs_lock);
      if(register_width>0&&register_width<4)
           status=sprintf(buf,"%s=%d byte per register\n","register_width",register_width);
      else
           status=sprintf(buf,"register_width=error num!!!\n");
      mutex_unlock(&sysfs_lock);
      return status;
}
static ssize_t ontim_iic_register_width_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
       unsigned int register_width;
       int res=0;
	mutex_lock(&sysfs_lock);
	if(strnstr(buf,"0x",2))
	    res = kstrtouint(buf, 16, &register_width);
	else
	    res = kstrtouint(buf, 10, &register_width);
        if(!res&&register_width>=0&&register_width<4)
	      ontim_iic_dev_dbg_data.i_register_width=register_width;
	else
        {
           printk(KERN_ERR "input register width have a error\n");
           ontim_iic_dev_dbg_data.i_register_width=0xffff;
        }
 	mutex_unlock(&sysfs_lock);
	return count;
}
static struct kobj_attribute ontim_iic_register_width_attr = {
	.attr = {
		.name = "reg_width",
		.mode = ACCESS_LEVEL,
	},
	.show =&ontim_iic_register_width_show,
	.store= &ontim_iic_register_width_store,
};
/*--------------------------------------------------------------*/
/*+++++++++++++++++++++++++++++++++++++++++++*/
static ssize_t ontim_iic_data_width_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
      int status;
       unsigned int data_width=ontim_iic_dev_dbg_data.i_data_width;
      mutex_lock(&sysfs_lock);
      if(data_width>0&&data_width<4)
           status=sprintf(buf,"%s=%d byte per register\n","register_width",data_width);
      else
           status=sprintf(buf,"register_width=error num!!!\n");
      mutex_unlock(&sysfs_lock);
      return status;
}
static ssize_t ontim_iic_data_width_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
       unsigned int data_width;
       int res=0;
	mutex_lock(&sysfs_lock);
	if(strnstr(buf,"0x",2))
	    res = kstrtouint(buf, 16, &data_width);
	else
	    res = kstrtouint(buf, 10, &data_width);
       if(!res&&data_width>=0&&data_width<4)
	      ontim_iic_dev_dbg_data.i_data_width=data_width;
	else
        {
           printk(KERN_ERR "input data width have a error\n");
           ontim_iic_dev_dbg_data.i_data_width=0xffff;
        }
 	mutex_unlock(&sysfs_lock);
	return count;
}

static struct kobj_attribute ontim_iic_data_width_attr = {
	.attr = {
		.name = "data_width",
		.mode = ACCESS_LEVEL,
	},
	.show =&ontim_iic_data_width_show,
	.store= &ontim_iic_data_width_store,
};
/*--------------------------------------------------------------*/



static int ontim_iic_dev_write_block(unsigned char  *buf,unsigned int len)
{
   int ret=0;
   struct i2c_adapter *adap;
   struct i2c_msg msgs[] = {
        {
            .addr=ontim_iic_dev_dbg_data.i_addr,
            .flags= 0,
            .len=len,
            .buf=buf,
        },
    };
    adap = i2c_get_adapter(ontim_iic_dev_dbg_data.i_chip_num);
    if(adap){
        ret = i2c_transfer(adap, msgs, 1);
        if (ret < 0){
            pr_err("msg %s i2c write error: %d\n", __func__, ret);
            printk(KERN_ERR "current i2c config, chip_num=%d,i2c_addr=0x%x\n",ontim_iic_dev_dbg_data.i_chip_num\
                                            ,ontim_iic_dev_dbg_data.i_addr);
            printk(KERN_ERR "please check i2c config for your settings\n");
        }
 	i2c_put_adapter(adap);
	ret=-ENOMEM;
    }
    return ret;
}
static int ontim_iic_dev_read_block(unsigned char  *buf,unsigned int len)
{
   int ret=0;
   struct i2c_adapter *adap;
   struct i2c_msg msgs[] = {
        {
            .addr=ontim_iic_dev_dbg_data.i_addr,
            .flags= 0,
            .len=ontim_iic_dev_dbg_data.i_register_width,
            .buf=buf,
        },
        {
          .addr=ontim_iic_dev_dbg_data.i_addr,
          .flags=I2C_M_RD,
          .len=len,
          .buf=buf,
        },
    };

    adap = i2c_get_adapter(ontim_iic_dev_dbg_data.i_chip_num);
    if(adap){
        ret = i2c_transfer(adap, msgs, 2);
        if (ret < 0){
            pr_err("msg %s i2c write error: %d\n", __func__, ret);
            printk(KERN_ERR "current i2c config, chip_num=%d,i2c_addr=0x%x\n",ontim_iic_dev_dbg_data.i_chip_num\
                                            ,ontim_iic_dev_dbg_data.i_addr);
            printk(KERN_ERR "please check i2c config for your settings\n");
        }
 	i2c_put_adapter(adap);
    }
    else
        ret=-ENOMEM;

    return ret;
}

/*+++++++++++++++++++++++++++++++++++++++++++*/
static ssize_t ontim_iic_write_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
      int status=0;
      mutex_lock(&sysfs_lock);
      status=sprintf(buf,"ontim_iic_write_show\n");
      mutex_unlock(&sysfs_lock);
      return status;
}
static ssize_t ontim_iic_write_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	int status;
	int n,m;
	int i=0;
       unsigned int *in_data;
	unsigned char *write_iic_data;
	unsigned int register_width=ontim_iic_dev_dbg_data.i_register_width;
       unsigned int data_width=ontim_iic_dev_dbg_data.i_data_width;
 	unsigned int *input_data;



	char *temp_data=kzalloc(32,GFP_KERNEL);
	if(!temp_data)
		goto malloc_fail;
       	input_data  = kzalloc(256*sizeof(unsigned int),GFP_KERNEL);
	in_data	=input_data;
	if(!input_data)
		goto malloc_fail1;
        write_iic_data  = kzalloc(256*sizeof(unsigned int),GFP_KERNEL);
	if(!write_iic_data)
		goto malloc_fail2;
	mutex_lock(&sysfs_lock);
        n=0;
        m=0;
        while(i<count){
	    memset(temp_data,0,32);
	    sscanf(&buf[i],"%s",temp_data);
            n=strlen(temp_data);
            if(n==0){
               printk(KERN_ERR "data over \n");
               break;
            }
            i+=n;
            while(i<count){
                if(buf[i]==' '||buf[i]==',')
                    i++;
                else
                    break;
            }
            printk(KERN_ERR "n = %d %s\n",n,temp_data);
	    if(strnstr(temp_data,"0x",2)){
	        status = kstrtouint(temp_data, 16, in_data++);
                if(status){
		     printk(KERN_ERR "input data%d %s have a error 1\n",m,temp_data);
                     goto malloc_fail2;
                }
            }
	    else
            {
	        status = kstrtouint(temp_data, 10, in_data++);
                if(status){
		    printk(KERN_ERR "input data%d %s have a error 2\n",m,temp_data);
		    goto malloc_fail2;
                }
	    }
	    m++;
       }
       status=check_data_valid(input_data[0],register_width);
       if(status){
            printk(KERN_ERR "input data%d have a error, register addr overflow  target register width, \n",m);
            goto malloc_fail2;
        }
	for(i=1;i<m;i++){
	       status=check_data_valid(input_data[1],data_width);
              if(status){
                   printk(KERN_ERR "input data%d have a error, register addr overflow  target data width \n",i);
                   goto malloc_fail2;
              }
	}
	i=0;
	n=0;
	if(covert_data_sort_byte_by_data_width(input_data[i],register_width,&write_iic_data[n])<0){
           printk(KERN_ERR "input data%d have a error, register addr overflow  target register addr  width\n",i);
           goto malloc_fail2;
	}
	n+=register_width;
        printk(KERN_ERR "input[%d]=0x%x  \n",i,input_data[i]);
	for(i=1;i<m;i++){
	    if(covert_data_sort_byte_by_data_width(input_data[i],data_width,&write_iic_data[i])<0){
               printk(KERN_ERR "input data%d have a error, data  overflow  target data width \n",i);
               goto malloc_fail2;
	    }
            printk(KERN_ERR "input[%d]=0x%x  \n",i,input_data[i]);

	    n+=data_width;
	}
        for(i=0;i<m;i++)
        {
           printk(KERN_ERR "write_iic_data[%d]=0x%x\n",i,write_iic_data[i]);
        }
        ontim_iic_dev_write_block(write_iic_data,m);



       kfree(write_iic_data);
malloc_fail2:
       kfree(input_data);
malloc_fail1:
       kfree(temp_data);
malloc_fail:
 	mutex_unlock(&sysfs_lock);
	return count;
}
static struct kobj_attribute ontim_iic_write_addr_attr = {
	.attr = {
		.name = "write",
		.mode = ACCESS_LEVEL,
	},
	.show =&ontim_iic_write_show,
	.store= &ontim_iic_write_store,
};
/*--------------------------------------------------------------*/
/*+++++++++++++++++++++++++++++++++++++++++++*/
static ssize_t ontim_iic_read_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
      int status=0;
      mutex_lock(&sysfs_lock);
      status=sprintf(buf,"ontim_iic_read_show\n");
      mutex_unlock(&sysfs_lock);
      return status;
}
static ssize_t ontim_iic_read_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	int status;
	int n,m;
	int i=0;
	int read_len;
       unsigned int *in_data;
	unsigned char *read_iic_data;
	unsigned int register_width=ontim_iic_dev_dbg_data.i_register_width;
//       unsigned int data_width=ontim_iic_dev_dbg_data.i_data_width;
 	unsigned int *input_data;
	char *temp_data=kzalloc(32,GFP_KERNEL);
	if(!temp_data)
		goto malloc_fail;
       	input_data  = kzalloc(256*sizeof(unsigned int),GFP_KERNEL);
	in_data	=input_data;
	if(!input_data)
		goto malloc_fail1;
        read_iic_data  = kzalloc(256*sizeof(unsigned int),GFP_KERNEL);
	if(!read_iic_data)
		goto malloc_fail2;
	mutex_lock(&sysfs_lock);

        n=0;
        m=0;
        while(i<count){
	    memset(temp_data,0,32);
	    sscanf(&buf[i],"%s",temp_data);
            n=strlen(temp_data);
            if(n==0){
               printk(KERN_ERR "data over \n");
               break;
            }
            i+=n;
            while(i<count){
                if(buf[i]==' '||buf[i]==',')
                    i++;
                else
                    break;
            }
            printk(KERN_ERR "n = %d %s\n",n,temp_data);
	    if(strnstr(temp_data,"0x",2)){
	        status = kstrtouint(temp_data, 16, in_data++);
                if(status){
		     printk(KERN_ERR "input data%d %s have a error 1\n",m,temp_data);
                     goto malloc_fail2;
                }
            }
	    else
            {
	        status = kstrtouint(temp_data, 10, in_data++);
                if(status){
		    printk(KERN_ERR "input data%d %s have a error 2\n",m,temp_data);
		    goto malloc_fail2;
                }
	    }
	    m++;
       }
       status=check_data_valid(input_data[0],register_width);
	if(status){
            printk(KERN_ERR "input data%d have a error, register addr overflow  target register width, \n",m);
            goto malloc_fail3;
	}
	read_len=input_data[1];
	if(read_len>255){
            printk(KERN_ERR "input data%d have a error, read line overflow  target 255, \n",m);
            goto malloc_fail3;
	}
        printk(KERN_ERR "iic read iic_addr=0x%x len=%d \n",input_data[0],input_data[1]);
	if(covert_data_sort_byte_by_data_width(input_data[0],register_width,&read_iic_data[n])<0){
           printk(KERN_ERR "input data[%d] have a error, register addr overflow  target register addr  width\n",0);
           goto malloc_fail2;
	}

        for(i=0;i<register_width;i++)
        {
           printk(KERN_ERR "write_iic_data[%d]=0x%x\n",i,read_iic_data[i]);
        }
        if(ontim_iic_dev_read_block(read_iic_data,read_len)>=0)
        {
            for(i=0;i<read_len;i++){
                printk(KERN_ERR "read data from device,data[%d]=0x%x\n",i,read_iic_data[i]);
            }
        }

 	mutex_unlock(&sysfs_lock);
malloc_fail3:
       kfree(read_iic_data);
malloc_fail2:
       kfree(input_data);
malloc_fail1:
       kfree(temp_data);
malloc_fail:
	return count;
}
static struct kobj_attribute ontim_iic_read_attr = {
	.attr = {
		.name = "read",
		.mode = ACCESS_LEVEL,
	},
	.show =&ontim_iic_read_show,
	.store= &ontim_iic_read_store,
};
/*--------------------------------------------------------------*/

int covert_data_sort_byte_by_data_width(unsigned int  value,unsigned int width,unsigned char *buf)
{
    int status=width;
    switch(width)
    {
       case 1:
           buf[0]=value&0xff;
           break;
       case 2:
           buf[0]=value&0xff;
           buf[1]=(value&0xff00)>>8;
            break;
       case 4:
           buf[0]=value&0xff;
           buf[1]=(value&0xff00)>>8;
           buf[2]=(value&0xff0000)>>16;
           buf[3]=(value&0xff000000)>>24;
           break;
       default:
           status=-1;
           break;
    }
    return status;
}
int check_data_valid(unsigned int  value,unsigned int width)
{
    int status=0;
    switch(width)
    {
       case 1:
           if(value>0xff)
               status=-1;
       break;
       case 2:
           if(value>0xffff)
               status=-1;
           break;
       case 4:
           if(value>0xffffffff)
               status=-1;
           break;
       default:
           status=-1;
           break;
    }
    return status;
}
/*--------------------------------------------------------------*/
/*+++++++++++++++++++++++++++++++++++++++++++*/
static ssize_t ontim_iic_help_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
      int status=0;
      mutex_lock(&sysfs_lock);
      status=sprintf(buf,"echo num > chip_num\n echo i2c_addr > i2c_addr\n echo reg_width > reg_width\n echo data_width > data_width\n echo reg lendth > read\n echo reg dat[0] ... > write \n " );
      mutex_unlock(&sysfs_lock);
      return status;
}

static struct kobj_attribute ontim_iic_help_attr = {
	.attr = {
		.name = "help",
		.mode = ACCESS_LEVEL,
	},
	.show =&ontim_iic_help_show,
	.store=NULL,
};
/*--------------------------------------------------------------*/


static struct attribute *ontim_iic_dev_properties_attrs[] = {
       &ontim_iic_chipnum_attr.attr,
       &ontim_iic_addr_attr.attr,
       &ontim_iic_register_width_attr.attr,
       &ontim_iic_data_width_attr.attr,
       &ontim_iic_write_addr_attr.attr,
       &ontim_iic_read_attr.attr,
       &ontim_iic_help_attr.attr,
       NULL
};

static struct attribute_group ontim_iic_dev_properties_attr_group = {
	.attrs = ontim_iic_dev_properties_attrs,
};


/*+++++++++++++++++++++++++++++++++++++++++++*/
static int gpio_force_num=0;
static ssize_t ontim_gpio_num_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
      int status=0;
      mutex_lock(&sysfs_lock);
#ifdef CONFIG_MTK_GPIO
 	mt_set_gpio_mode(gpio_force_num, GPIO_MODE_00);
	mt_set_gpio_dir(gpio_force_num, GPIO_DIR_IN);
	status=!!mt_get_gpio_in(gpio_force_num);
	status=sprintf(buf,"%s%d value is %d\n","GPIO",(unsigned int)(gpio_force_num&0x0000ffff),status);
#else
      if(gpio_is_valid(gpio_force_num)&&(gpio_force_num!=0))
      {
          status=!!gpio_get_value(gpio_force_num);
          status=sprintf(buf,"%s%d value is %d\n","GPIO",(unsigned int)gpio_force_num,status);
      }
      else
      {
           status=sprintf(buf, "%s%d is not valid \n","GPIO",(unsigned int)gpio_force_num);
      }
#endif
      mutex_unlock(&sysfs_lock);
      return status;
}
static ssize_t ontim_gpio_num_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t count)
{
    int res=0;
    mutex_lock(&sysfs_lock);
    if(strnstr(buf,"0x",2))
        res = kstrtouint(buf, 16, &gpio_force_num);
    else
	res = kstrtouint(buf, 10, &gpio_force_num);
#ifdef CONFIG_MTK_GPIO
	gpio_force_num|=0x80000000;
#else
    if(!gpio_is_valid(gpio_force_num)||(gpio_force_num==0))
    {
        printk(KERN_ERR "GPIO%d is not valid \n",(unsigned int)gpio_force_num);
    }
#endif
    mutex_unlock(&sysfs_lock);
    return count;
}
static struct kobj_attribute ontim_gpio_num_attr = {
	.attr = {
		.name = "gpio_num",
		.mode = ACCESS_LEVEL,
	},
	.show =&ontim_gpio_num_show,
	.store= &ontim_gpio_num_store,
};
/*--------------------------------------------------------------*/
/*+++++++++++++++++++++++++++++++++++++++++++*/
static ssize_t ontim_gpio_value_show(struct kobject *kobj,
                 struct kobj_attribute *attr, char *buf)
{
      int status;

      mutex_lock(&sysfs_lock);
#ifdef CONFIG_MTK_GPIO
 	mt_set_gpio_mode(gpio_force_num, GPIO_MODE_00);
	mt_set_gpio_dir(gpio_force_num, GPIO_DIR_IN);
	status=!!mt_get_gpio_out(gpio_force_num);
	status=sprintf(buf,"%s%d value is %d\n","GPIO",(unsigned int)(gpio_force_num&0x0000ffff),status);
#else
      if(gpio_is_valid(gpio_force_num)&&(gpio_force_num!=0))
      {
          status=!!gpio_get_value(gpio_force_num);
          status=sprintf(buf,"GPIO%d value is %d\n",(unsigned int)gpio_force_num,status);
      }
      else
      {
           status=sprintf(buf,"GPIO%d is not valid \n",(unsigned int)gpio_force_num);
      }
#endif
      mutex_unlock(&sysfs_lock);
      return status;
}
static ssize_t ontim_gpio_value_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t count)
{
    int res=0 ;
    int gpio_value;
    mutex_lock(&sysfs_lock);
    if(strnstr(buf,"0x",2))
        res = kstrtouint(buf, 16, &gpio_value);
    else
	res = kstrtouint(buf, 10, &gpio_value);
#ifdef CONFIG_MTK_GPIO
 	mt_set_gpio_mode(gpio_force_num, GPIO_MODE_00);
	mt_set_gpio_dir(gpio_force_num, GPIO_DIR_OUT);
	mt_set_gpio_out(gpio_force_num,!!gpio_value);
       printk(KERN_ERR "GPIO%d value is %d\n",(unsigned int)(gpio_force_num&0x0000ffff),!!gpio_value);
#else
    if(gpio_is_valid(gpio_force_num)&&(gpio_force_num!=0))
    {
        res=gpio_direction_output(gpio_force_num,!!gpio_value);
        printk(KERN_ERR "GPIO%d value is %d\n",(unsigned int)gpio_force_num,!!gpio_value);
    }
    else
    {
        printk(KERN_ERR "GPIO%d is not valid \n",(unsigned int)gpio_force_num);
    }
#endif
    mutex_unlock(&sysfs_lock);
    return count;
}
static struct kobj_attribute ontim_gpio_value_attr = {
	.attr = {
		.name = "value",
		.mode = ACCESS_LEVEL,
	},
	.show =&ontim_gpio_value_show,
	.store= &ontim_gpio_value_store,
};
/*--------------------------------------------------------------*/

static struct attribute *ontim_gpio_force_set_properties_attrs[] = {
       &ontim_gpio_num_attr.attr,
       &ontim_gpio_value_attr.attr,
       NULL
};

static struct attribute_group ontim_gpio_force_set_properties_attr_group = {
	.attrs = ontim_gpio_force_set_properties_attrs,
};





static int __init ontim_core_dbg_init(void)
{
    int ret=0;

    ontim_core_dbg_kobj = kobject_create_and_add("ontim_dev_debug", NULL);
    if(ontim_core_dbg_kobj == NULL) {
        printk(KERN_INFO   "__init ontim_core_dbg_init: ontim_dev_debug failed\n");
	ret = -ENOMEM;
        goto err;
    }
/* iic control*/
    ontim_iic_dbg_kobj = kobject_create_and_add("iic", ontim_core_dbg_kobj);
    if(ontim_iic_dbg_kobj==NULL){
       printk(KERN_INFO   "__init ontim_core_dbg_init: ontim_dev_debug failed 1\n");
       goto err;
    }
    ret = sysfs_create_group(ontim_iic_dbg_kobj, &ontim_iic_dev_properties_attr_group);
    if (ret){
        printk(KERN_INFO   "__init ontim_core_dbg_init: ontim_dev_debug failed 2\n");
        goto  err;
    }

/*gpio force set */
    ontim_gpio_dbg_kobj = kobject_create_and_add("gpio", ontim_core_dbg_kobj);
    if(ontim_gpio_dbg_kobj==NULL){
       printk(KERN_INFO   "__init ontim_core_dbg_init: ontim_dev_debug failed 1\n");
       goto err;
    }
    ret = sysfs_create_group(ontim_gpio_dbg_kobj, &ontim_gpio_force_set_properties_attr_group);
    if (ret){
        printk(KERN_INFO   "__init ontim_core_dbg_init: ontim_dev_debug failed 2\n");
        goto  err;
    }
    memset(kobj_attribute_arrary,0,ONTIM_DEBUG_DEV_NUM*PER_DEV_ELEMENT_NUM * sizeof(struct kobj_attribute));
    memset(kobj_attribute_arrary_state,0,ONTIM_DEBUG_DEV_NUM*PER_DEV_ELEMENT_NUM);

err:
    return ret;
}
static void __exit ontim_core_dbg_exit(void)
{
}

arch_initcall(ontim_core_dbg_init);
module_exit(ontim_core_dbg_exit);
EXPORT_SYMBOL_GPL(ontim_dev_debug_file_exist);
EXPORT_SYMBOL_GPL(regester_and_init_ontim_debug);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("luhongjiang <hongjiang.lu@ontim.cn>");
MODULE_DESCRIPTION("ontim devce debug ");
#undef  ONTIM_DEBUG_CORE_C




