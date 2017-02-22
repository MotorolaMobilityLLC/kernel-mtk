#include <linux/kernel.h>	/* printk() */
#include <linux/module.h>
#include <linux/types.h>	/* size_t */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/proc_fs.h>  /*proc*/
#include <linux/genhd.h>    //special
#include <linux/cdev.h>
#include <asm/uaccess.h> 

extern char* saved_command_line;
//#define SLT_DEVINFO_EMCP
#ifdef SLT_DEVINFO_EMCP
#include "emi.h"
#include <linux/dev_info.h>

extern int num_of_emi_records;
extern EMI_SETTINGS emi_settings[];


int devinfo_ram_size;
#define DUMP_MMC_VERSION '1.0'
extern int g_rom_size1;

extern u64 msdc_get_capacity(int get_emmc_total);
void dump_mmc(void)
{
    char *p_mdl = strstr(saved_command_line, "mdl_num=");
	int tmp_devinfo_ram_size = 0;
    int  ram_num = 0;
    int i = 0;
    unsigned long  ram_size;
    int rom_size;
    char *info;
	printk("saved_command_line:%s \n", saved_command_line);
	printk("%s, p_mdl:%s \n", __func__, p_mdl);

	//user_size = msdc_get_user_capacity(mtk_msdc_host[index]);
    if( p_mdl == NULL) {
	      ram_num = 0;
    }else{
             p_mdl += 8;
		ram_num  =  simple_strtol(p_mdl, NULL, 10);
    }
	printk("ram_num=%d \n", ram_num);

                                   //  rom_size = user_size/1024/1024;

                        for(i=0; i<num_of_emi_records; i++) {
				     struct devinfo_struct *dev = (struct devinfo_struct*)kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);;
                                switch (emi_settings[i].type) {
			            case 0x0001:
                                        dev->device_type =	"Discrete DDR1";
                                        break;
			            case 0x0002:
                                        dev->device_type =	"Discrete LPDDR2";
                                        break;
			            case 0x0003:
                                        dev->device_type =	"Discrete LPDDR3";
                                        break;
				     case 0x0004:
                                        dev->device_type =	"Discrete PCDDR3";
                                        break;
			            case 0x0101:
                                        dev->device_type =	"MCP(NAND+DDR1)";
                                        break;
			            case 0x0102:
                                        dev->device_type =	"MCP(NAND+DDR2)";
                                        break;
			            case 0x0103:
                                        dev->device_type =	"MCP(NAND+DDR3)";
                                        break;
			            case 0x0201:
                                        dev->device_type =	"MCP(eMMC+DDR1)";
                                        break;
			            case 0x0202:
                                        dev->device_type =	"MCP(eMMC+DDR2)";
                                        break;
			            case 0x0203:
                                        dev->device_type =	"MCP(eMMC+DDR3)";
                                        break;
			            default:
                                        dev->device_type =	"unknow";
                                        break;
		                   }

                                 if( (emi_settings[i].DEVINFO_MCP[0] == 'H') && (emi_settings[i].DEVINFO_MCP[1] == '9' )){
						   dev->device_vendor	 = 	"Hynix";
                                 	}else if ( (emi_settings[i].DEVINFO_MCP[0] == 'K' )&& (emi_settings[i].DEVINFO_MCP[1] == '3')) {
                                     	    dev->device_vendor	 = 	"Samsung";
                                 	}else if ( (emi_settings[i].DEVINFO_MCP[0] == 'E' )&& (emi_settings[i].DEVINFO_MCP[1] == 'D')) {
                                       	   dev->device_vendor	 = 	"Elpida"; 
                                    }else if ( (emi_settings[i].DEVINFO_MCP[0] == 'M' )&& (emi_settings[i].DEVINFO_MCP[1] == 'T')) {
                                       	   dev->device_vendor	 = 	"Micron"; 
                                 }else {
                                       	   dev->device_vendor	 = 	"unknow"; 
                                 	}
    			
		                   ram_size = (unsigned long  )(emi_settings[i].DRAM_RANK_SIZE[0] /(1024*1024) + emi_settings[i].DRAM_RANK_SIZE[1]/(1024*1024)
		                                          + emi_settings[i].DRAM_RANK_SIZE[2] /(1024*1024)+ emi_settings[i].DRAM_RANK_SIZE[3]/(1024*1024));

						   printk("%s, ram_size=%ld,  DRAM_RANK_SIZE=%0x \n", __func__, ram_size,  emi_settings[i].DRAM_RANK_SIZE[0]);
#if 0
		                   //Check if used on this board
		                   if((emi_settings[i].ID[0]==(mtk_msdc_host[index]->mmc->card->raw_cid[0]&0xFF000000)>>24) &&
		                       (emi_settings[i].ID[1]==(mtk_msdc_host[index]->mmc->card->raw_cid[0]&0x00FF0000)>>16) && 
		                       (emi_settings[i].ID[2]==(mtk_msdc_host[index]->mmc->card->raw_cid[0]&0x0000FF00)>>8) &&
		                       (emi_settings[i].ID[3]==(mtk_msdc_host[index]->mmc->card->raw_cid[0]&0x000000FF)>>0)) {
                                          info = kmalloc(64,GFP_KERNEL);
						  switch(ram_size) {
		                             case 2048:
                                                  sprintf(info,"ram:2048MB+rom:%dMB",rom_size);
                                                  break;
		                             case 1536:
                                                  sprintf(info,"ram:1536MB+rom:%dMB",rom_size);
                                                  break;
		                             case 1024:
                                                  sprintf(info,"ram:1024MB+rom:%dMB",rom_size);
                                                  break;
		                             case 768:
                                                  sprintf(info,"ram:768 MB+rom:%dMB",rom_size);
                                                  break;
		                             case 512:
                                                  sprintf(info,"ram:512 MB+rom:%dMB",rom_size);
                                                  break;
		                             default:
                                                  sprintf(info,"ram:512 MB+rom:%dMB",rom_size);
                                                  break;
                                           }
                                           dev->device_info = info;
                                  }else{
		                             switch(ram_size) {
		                             case 2048:
                                                 dev->device_info ="ram:2048MB";
                                                 break;
                                          case 1536:
                                                 dev->device_info ="ram:1536MB";
                                                 break;
                                          case 1024:
                                                 dev->device_info ="ram:1024MB";
                                                 break;
                                          case 768:
                                                 dev->device_info ="ram:768 MB";
                                                 break;
                                          case 512:
                                                 dev->device_info ="ram:512 MB";
                                                 break;
                                          default:
                                                 dev->device_info ="ram:1024 MB";
                                                 break;
                                        }
		                   }
#endif

                                   info = kmalloc(32,GFP_KERNEL);
								   //rom_size=16;
#if 0
									rom_size = msdc_get_capacity(1)/(1024*1024);
									printk("%s, %d, rom_size=%d \n", __func__, __LINE__, rom_size);
#endif


#if 1
									msdc_get_capacity(0);
									printk("%s, %d, rom_size=%d \n", __func__, __LINE__, g_rom_size1);
									if (g_rom_size1 <= (16*1024) && g_rom_size1 > (12*1024)){
											rom_size = 16;
									}else{ 
											rom_size = 32;
									}
#endif
					  switch(ram_size) {
                                     case 4096:
	                             case -4096:
                                              //sprintf(info,"ram:4096MB+rom:%dGB",rom_size);
                                              sprintf(info,"ram:4096MB");
											  tmp_devinfo_ram_size = 4096;
                                              break;
					  			 case 3072:
	                             case -3072:
                                              //sprintf(info,"ram:3072MB+rom:%dGB",rom_size);
                                              sprintf(info,"ram:3072MB");
											  tmp_devinfo_ram_size = 3072;
                                              break;
	                             case 2048:
	                             case -2048:
                                              //sprintf(info,"ram:2048MB+rom:%dGB",rom_size);
                                              sprintf(info,"ram:2048MB");
											  tmp_devinfo_ram_size = 2048;
                                              break;
	                             case 1536:
                                              //sprintf(info,"ram:1536MB+rom:%dGB",rom_size);
                                              sprintf(info,"ram:1536MB");
                                              break;
	                             case 1024:
							     case -1024:
                                              tmp_devinfo_ram_size = 1024;
                                              //sprintf(info,"ram:1024MB+rom:%dGB",rom_size);
                                              sprintf(info,"ram:1024MB");
                                              break;

	                             case 768:
                                              tmp_devinfo_ram_size = 768 ; 
                                              //sprintf(info,"ram:768 MB+rom:%dGB",rom_size);
                                              sprintf(info,"ram:768 MB");
                                              break;
	                             case 512:
                                              tmp_devinfo_ram_size = 512 ; 
                                              //sprintf(info,"ram:512 MB+rom:%dGB",rom_size);
                                              sprintf(info,"ram:512 MB");
                                              break;
	                             default:
                                              tmp_devinfo_ram_size = 1024 ; 
                                              //sprintf(info,"default ram:1024 MB+rom:%dGB",rom_size);
                                              sprintf(info,"default ram:1024 MB");
                                              break;
                                  }
                                  dev->device_info = info;
					if(ram_num == i ){
						 devinfo_ram_size = tmp_devinfo_ram_size;
				         dev->device_used = DEVINFO_USED;
						 printk("used mdl_num=%d \n", ram_num);
				    }else{
					      dev->device_used = DEVINFO_UNUSED;	
						 printk("unused mdl_num=%d \n", ram_num);
					}

                    dev->device_version = DEVINFO_NULL;
  				     dev->device_module = DEVINFO_NULL;     
                                dev->device_ic= emi_settings[i].DEVINFO_MCP;
                                devinfo_check_add_device(dev);
						}
}
#endif

/**** add by wangjiaxing 20170219 start ****/
int lct_read_boardid(void)
{
    char *p_boardid = strstr(saved_command_line, "BoardID=");

    int boardid[2];

    if (p_boardid == NULL)
    {
      return -1;

    }
    boardid[0] = *(p_boardid + 8)-'0';

    boardid[1] = *(p_boardid + 9)-'0';

    return (boardid[0]*10+boardid[1]);

}
/**** add by wangjiaxing 20170219 end ****/

//LCSH ADD by dingyin for boardid start
static int cf_show(struct seq_file *m, void *v)
{
    char *p_boardid = strstr(saved_command_line, "BoardID=");
    char boardid[2];

    if (p_boardid == NULL)
    {
      seq_printf(m, "0\n");
      return 0;
    }
    boardid[0] = *(p_boardid + 8);
    boardid[1] = *(p_boardid + 9);
    seq_printf(m, "%c%c\n", boardid[0], boardid[1]);

    return 0;
}
static void *cf_start(struct seq_file *m, loff_t *pos)
{
    return *pos < 1 ? (void *)1 : NULL;
}

static void *cf_next(struct seq_file *m, void *v, loff_t *pos)
{
    ++*pos;
    return NULL;
}

static void cf_stop(struct seq_file *m, void *v)
{
}
const struct seq_operations boardid_op = {
    .start  = cf_start,
    .next = cf_next,
    .stop = cf_stop,
    .show = cf_show
};

static int board_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &boardid_op);
}

static const struct file_operations proc_boardid_operations = {
    .open   = board_open,
    .read   = seq_read,
    .llseek   = seq_lseek,
    .release  = seq_release,
};
//LCSH ADD by dingyin for boardid end
//
static int __init lct_dump_mmc_init(void)
{
	printk("\n%s, %d\n",__func__, __LINE__);
	//dump_mmc();
  proc_create("BoardId", 0, NULL, &proc_boardid_operations); //LCSH ADD by dingyin for boardid
	return 0;
}

static void __exit lct_dump_mmc_exit(void)
{
	printk("\n%s, %d\n",__func__, __LINE__);
}

module_init(lct_dump_mmc_init);
module_exit(lct_dump_mmc_exit);

MODULE_LICENSE('GPLv2');
MODULE_DESCRIPTION('LCT dump mmc module');
MODULE_AUTHOR('libin2@longcheer.net');
MODULE_VERSION("version 1.0");
