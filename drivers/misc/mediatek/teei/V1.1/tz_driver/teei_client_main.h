#ifndef __TEEI_CLIENT_MAIN_H__
#define __TEEI_CLIENT_MAIN_H__

extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
extern int mt_eint_set_deint(int eint_num, int irq_num);
extern int mt_eint_clr_deint(int eint_num);
extern void neu_disable_touch_irq(void);
extern void neu_enable_touch_irq(void);
extern void add_bdrv_queue(int bdrv_id);
extern unsigned long t_nt_buffer;
extern unsigned long fp_buff_addr;
extern struct work_queue *secure_wq;
extern unsigned char *daulOS_VFS_write_share_mem;
extern unsigned char *daulOS_VFS_read_share_mem;
extern unsigned char *vfs_flush_address;
extern unsigned long bdrv_message_buff;
extern unsigned long tlog_message_buff;
extern unsigned long teei_vfs_flag;
#endif /* __TEEI_CLIENT_MAIN_H__ */
