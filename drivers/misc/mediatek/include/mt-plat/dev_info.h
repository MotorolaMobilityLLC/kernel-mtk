/* dev_info.h
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
/* dev_info.h
 * This function is from Longcheer (LCT) software.
 * This is designed to record information of all special devices for phonecells,
 * such as LCM, MCP, Touchpannel, cameras, sensors and so on.
 * By   : shaohui@longcheer.net
 * Date : 2012/11/26
 *
 * Add slab include 
 * */

/* dev_info.h
 * This function is from Longcheer (LCT) software.
 * This is designed to record information of all special devices for phonecells,
 * such as LCM, MCP, Touchpannel, cameras, sensors and so on.
 * By   : shaohui@longcheer.net
 * Date : 2016/01/03
 *
 * Add sysfs note support,enbale SYS_INFO_SUPPORT to enbale 
 * */

#ifndef __DEV_INFO_H
#define __DEV_INFO_H

#include <linux/ioctl.h>	  

#include <linux/slab.h>


/*shaohui add this to create sysfs note*/
//#define SYSFS_INFO_SUPPORT




#define DEVINFO_NULL "(null)"
#define DEVINFO_UNUSED "false"
#define DEVINFO_USED	"true"

struct devinfo_struct{
	char *device_type;			// Module type, as LCM, CAMERA/Sub CAMERA, MCP, TP ...
	char *device_module;		// device module, PID
	char *device_vendor;		// device Vendor information, VID
	char *device_ic;			// device module solutions 
	char *device_version;		// device module firmware version, as TP version no
	char *device_info;			// more device infos,as capcity,resolution ...
	char *device_used;			// indicate whether this device is used in this set
#ifdef SYSFS_INFO_SUPPORT
  	char *device_comments;		// this is for special use	
#endif
	struct list_head device_link;
};


//extern int devinfo_declare_new_device(char* dev_type, char* dev_module, char* dev_vendor, char* dev_ic, char* dev_ver, char* dev_info, int dev_used );
extern int devinfo_declare_new_device(struct devinfo_struct *dev);//char* dev_type, char* dev_module, char* dev_vendor, char* dev_ic, char* dev_ver, char* dev_info, char* dev_used )

extern int devinfo_check_add_device(struct devinfo_struct *dev);
 //void devinfo_declare(char* type,char* module,char* vendor,char* ic,char* version,char* info,char* used)
#define DEVINFO_DECLARE(TYPE,MODULE,VENDOR,IC,VERSION,INFO,USED)	\
do{		\
	void declare_new_devinfo(char *Type,char *Module,char* Vendor,char* Ic,char* Version,char* Info,char* Used)	\
	{																	\
		struct devinfo_struct *dev;										\
   		dev=(struct devinfo_struct*)kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);			\
		dev->device_type	=	(char*)Type;							\
		dev->device_module	=	(char*)Module;							\
		dev->device_vendor	=	(char*)Vendor;							\
		dev->device_ic		=	(char*)Ic;								\
		dev->device_version	=	(char*)Version;							\
		dev->device_info	=	(char*)Info;							\
		dev->device_used	=	(char*)Used;							\
		devinfo_declare_new_device(dev);								\
	}		\
	declare_new_devinfo((char*)TYPE,(char*)MODULE,(char*)VENDOR,(char*)IC,(char*)VERSION,(char*)INFO,(char*)USED);\
}while(0)


 //void devinfo_check_declare(char* type,char* module,char* vendor,char* ic,char* version,char* info,char* used)
#define DEVINFO_CHECK_DECLARE(TYPE,MODULE,VENDOR,IC,VERSION,INFO,USED)	\
do{		\
	void declare_new_devinfo(char *Type,char *Module,char* Vendor,char* Ic,char* Version,char* Info,char* Used)	\
	{																	\
		struct devinfo_struct *dev;										\
   		dev=(struct devinfo_struct*)kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);			\
		dev->device_type	=	(char*)Type;							\
		dev->device_module	=	(char*)Module;							\
		dev->device_vendor	=	(char*)Vendor;							\
		dev->device_ic		=	(char*)Ic;								\
		dev->device_version	=	(char*)Version;							\
		dev->device_info	=	(char*)Info;							\
		dev->device_used	=	(char*)Used;				\
		devinfo_check_add_device(dev);						\
	}		\
	declare_new_devinfo((char*)TYPE,(char*)MODULE,(char*)VENDOR,(char*)IC,(char*)VERSION,(char*)INFO,(char*)USED);\
}while(0)


#ifdef SYSFS_INFO_SUPPORT
#define DEVINFO_DECLARE_COMMEN(TYPE,MODULE,VENDOR,IC,VERSION,INFO,USED,COMMENT)	\
do{		\
	void declare_new_devinfo(char *Type,char *Module,char* Vendor,char* Ic,char* Version,char* Info,char* Used,char* comments)	\
	{																	\
		struct devinfo_struct *dev;										\
   		dev=(struct devinfo_struct*)kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);			\
		dev->device_type	=	(char*)Type;							\
		dev->device_module	=	(char*)Module;							\
		dev->device_vendor	=	(char*)Vendor;							\
		dev->device_ic		=	(char*)Ic;								\
		dev->device_version	=	(char*)Version;							\
		dev->device_info	=	(char*)Info;							\
		dev->device_used	=	(char*)Used;							\
		dev->device_comments=	(char*)comments;				\
		devinfo_declare_new_device(dev);								\
	}		\
	declare_new_devinfo((char*)TYPE,(char*)MODULE,(char*)VENDOR,(char*)IC,(char*)VERSION,(char*)INFO,(char*)USED,(char*)COMMENT);\
}while(0)


#define DEVINFO_CHECK_DECLARE_COMMEN(TYPE,MODULE,VENDOR,IC,VERSION,INFO,USED,COMMENT)	\
do{		\
	void declare_new_devinfo(char *Type,char *Module,char* Vendor,char* Ic,char* Version,char* Info,char* Used,char* comments)	\
	{																	\
		struct devinfo_struct *dev;										\
   		dev=(struct devinfo_struct*)kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);			\
		dev->device_type	=	(char*)Type;							\
		dev->device_module	=	(char*)Module;							\
		dev->device_vendor	=	(char*)Vendor;							\
		dev->device_ic		=	(char*)Ic;								\
		dev->device_version	=	(char*)Version;							\
		dev->device_info	=	(char*)Info;							\
		dev->device_used	=	(char*)Used;				\
		dev->device_comments=	(char*)comments;				\
		devinfo_check_add_device(dev);						\
	}		\
	declare_new_devinfo((char*)TYPE,(char*)MODULE,(char*)VENDOR,(char*)IC,(char*)VERSION,(char*)INFO,(char*)USED,(char*)COMMENT);\
}while(0)

#define DEVINFO_DECLARE_VERSION(INFO)	\
	DEVINFO_DECLARE_COMMEN(DEVINFO_NULL,DEVINFO_NULL,DEVINFO_NULL,DEVINFO_NULL,DEVINFO_NULL,DEVINFO_NULL,DEVINFO_USED,INFO)

#define DEVINFO_CHECK_DECLARE_VERSION(INFO)	\
	DEVINFO_CHECK_DECLARE_COMMEN(DEVINFO_NULL,DEVINFO_NULL,DEVINFO_NULL,DEVINFO_NULL,DEVINFO_NULL,DEVINFO_NULL,DEVINFO_USED,INFO)

#endif
// void devinfo_itoa(int src,char* str)
#define DEVINFO_ITOA(SRC,STR)	\
	do{	\
		char* dev_itoa(int src)	\
		{		\
			char* s=0;		\
		   sprintf(s,"%d",src);		\
		return s;		\
		}	\
		STR=dev_itoa((int)SRC);	\
	}while(0)


//followd is unused yet

#define DEVINFO_IO						   		'v'
#define DEVINFO_IOCTL_INIT                  _IO(DEVINFO_IO,  0x01)
#define DEVINFO_IOCTL_READ_INFO         _IOR(DEVINFO_IO, 0x02, int)

#endif
