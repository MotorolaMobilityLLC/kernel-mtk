

#ifdef CONFIG_TINNO_PRODUCT_INFO

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>             
#include <linux/mm.h>
#include "dev_info.h"
#include "external.h"
/*******************************************************************************************/
// camera info.
G_CAM_INFO_S g_cam_info_struct[MAX_DRIVERS] ;

//For MTK secureboot.add by yinglong.tang
extern int sec_schip_enabled(void);
extern int sec_usbdl_enabled(void);
extern int sec_boot_enabled(void);
/*******************************************************************************************/

static int get_lockmode_from_uboot(char *str)
{
    tinnoklog(" %s: %s\n",__func__,str);
    FULL_PRODUCT_DEVICE_INFO(ID_BL_LOCK_STATUS, (str != NULL ? str : "unknow"));
    return 1;
}

static int get_secboot_from_uboot(char *str)
{
    tinnoklog(" %s: %s\n",__func__,str);
    FULL_PRODUCT_DEVICE_INFO(ID_SECBOOT, (str != NULL ? str : "unknow"));
    return 1;
}

//For MTK secureboot.add by yinglong.tang
int get_mtk_secboot_cb(char *buf, void *args) 
{
    int sec_boot = sec_boot_enabled();
    int sec_usbdl = sec_usbdl_enabled();
    int sec_schip = sec_schip_enabled();

    if (!sec_boot && !sec_usbdl && !sec_schip) {
        return sprintf(buf, "NOT SUPPORT!");
    }

    return sprintf(buf, "[secboot:(%d) sec_usbdl:(%d) efuse:(%d)]", 
           sec_boot, sec_usbdl, sec_schip);
}

// uboot64/common/loader/sprd_cpcmdline.c : void cp_cmdline_fixup(void)
__setup("bootloader.lock=", get_lockmode_from_uboot);
__setup("secboot.enable=", get_secboot_from_uboot);

/*******************************************************************************************/

// for camera info.
int get_camera_info(char *buf, void *arg0) {
    char *tmp = (char *)arg0;
    int i = 0;
    long resolv = 0;
    int pi = 0;

    if (strcmp(tmp, "main") == 0) {
        i = 0;
    }
    else if (strcmp(tmp, "sub") == 0) {
        i = 1;
    }
    else if (strcmp(tmp, "main2") == 0) {
        i = 2;
    }
    else if (strcmp(tmp, "sub2") == 0) {
        i = 3;
    }
    else {
        i = 4;
    }

    resolv = g_cam_info_struct[i].w*g_cam_info_struct[i].h;
    pi = resolv/1000/1000 + (resolv/1000/100%10 > 5 ? 1 : 0);

    tinnoklog("%s:%d - (%s) %s:(%d*%d)\n",__func__, __LINE__, tmp,
        g_cam_info_struct[i].name, g_cam_info_struct[i].w, g_cam_info_struct[i].h);

    return sprintf(buf, "%s [%d*%d] %dM",
        g_cam_info_struct[i].name, g_cam_info_struct[i].w, g_cam_info_struct[i].h, pi);
}

/*******************************************************************************************/
// flash info.(mtk,sprd platform)
int get_mmc_chip_info(char *buf, void *arg0)
{										
    struct mmc_card *card = (struct mmc_card *)arg0;
    char tempID[64] = "";
    char vendorName[16] = "";
    char romsize[8] = "";
    char ramsize[8] = "";
    struct sysinfo si;
    si_meminfo(&si);
    if(si.totalram > 1572864 )				   // 6G = 1572864 	(256 *1024)*6
   		strcpy(ramsize , "8G");
    else if(si.totalram > 1048576)			  // 4G = 786432 	(256 *1024)*4
    		strcpy(ramsize , "6G");
    else if(si.totalram > 786432)			 // 3G = 786432 	(256 *1024)*3
    		strcpy(ramsize , "4G");
    else if(si.totalram > 524288)			// 2G = 524288 	(256 *1024)*2
    		strcpy(ramsize , "3G");
    else if(si.totalram > 262144)               // 1G = 262144		(256 *1024)     4K page size
    		strcpy(ramsize , "2G");
    else if(si.totalram > 131072)               // 512M = 131072		(256 *1024/2)   4K page size
    		strcpy(ramsize , "1G");
    else
    		strcpy(ramsize , "512M");
    
    if(card->ext_csd.sectors > 134217728)  		
		strcpy(romsize , "128G");
    else if(card->ext_csd.sectors > 67108864)  	// 67108864 = 32G *1024*1024*1024 /512            512 page	
		strcpy(romsize , "64G");
    else if(card->ext_csd.sectors > 33554432)  // 33554432 = 16G *1024*1024*1024 /512            512 page
		strcpy(romsize , "32G");
    else if(card->ext_csd.sectors > 16777216)  // 16777216 = 8G *1024*1024*1024 /512            512 page
		strcpy(romsize , "16G");
    else if(card->ext_csd.sectors > 8388608)  // 8388608 = 4G *1024*1024*1024 /512            512 page
		strcpy(romsize , "8G");
    else
		strcpy(romsize , "4G");	
	
    memset(tempID, 0, sizeof(tempID));
    sprintf(tempID, "%08x ", card->raw_cid[0]);
	tinnoklog("FlashID is %s, totalram= %ld, emmc_capacity =%d\n",tempID, si.totalram, card->ext_csd.sectors);

    if(strncasecmp((const char *)tempID, "90", 2) == 0)           // 90 is OEMid for Hynix 
   		strcpy(vendorName , "Hynix");
    else if(strncasecmp((const char *)tempID, "15", 2) == 0)		// 15 is OEMid for Samsung 
   		strcpy(vendorName , "Samsung");
    else if(strncasecmp((const char *)tempID, "45", 2) == 0)		// 45 is OEMid for Sandisk 
   		strcpy(vendorName , "Sandisk");
    else if(strncasecmp((const char *)tempID, "70", 2) == 0)		// 70 is OEMid for Kingston 
   		strcpy(vendorName , "Kingston");
    else if(strncasecmp((const char *)tempID, "88", 2) == 0)		// 88 is OEMid for Foresee 
   		strcpy(vendorName , "Foresee");
    else if(strncasecmp((const char *)tempID, "f4", 2) == 0)		// f4 is OEMid for Biwin 
   		strcpy(vendorName , "Biwin");
    else if(strncasecmp((const char *)tempID, "8F", 2) == 0)		// 8F is OEMid for UNIC 
   		strcpy(vendorName , "UNIC");
    else if(strncasecmp((const char *)tempID, "13", 2) == 0)		// 15 is OEMid for Samsung 
   		strcpy(vendorName , "micron");
    else
		strcpy(vendorName , "Unknown");

    memset(tempID, 0, sizeof(tempID));
    sprintf(tempID,"%s_%s+%s,prv:%02x,life:%02x,%02x",vendorName,romsize,ramsize,card->cid.prv,
           card->ext_csd.device_life_time_est_typ_a,card->ext_csd.device_life_time_est_typ_b);
   
    return sprintf(buf,"%s", tempID);					
}				
#endif
