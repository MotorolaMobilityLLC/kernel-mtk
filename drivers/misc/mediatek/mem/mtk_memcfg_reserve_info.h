#ifndef __MTK_MEMCFG_RESERVE_INFO__
#define __MTK_MEMCFG_RESERVE_INFO__
#include <linux/init.h>
#include <linux/proc_fs.h>

int __init mtk_memcfg_reserve_info_init(struct proc_dir_entry *mtk_memcfg_dir);

#endif /* end of __MTK_MEMCFG_RESERVE_INFO__ */
