#ifndef _MLOG_DUMP_H
#define _MLOG_DUMP_H

extern void mlog_init_procfs(void);
extern int mlog_print_fmt(struct seq_file *m);
extern void mlog_doopen(void);
extern int mlog_doread(char __user *buf, size_t len);
extern int mlog_unread(void);

extern wait_queue_head_t mlog_wait;

#endif
