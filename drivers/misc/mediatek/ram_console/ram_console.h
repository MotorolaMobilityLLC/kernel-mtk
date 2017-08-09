#ifndef __RAM_CONSOLE_H__
#define __RAM_CONSOLE_H__
extern int card_dump_func_write(unsigned char *buf, unsigned int len, unsigned long long offset,
				int dev);
#ifdef CONFIG_MTPROF
extern int boot_finish;
#endif
extern struct file *expdb_open(void);
#ifdef CONFIG_PSTORE
extern void pstore_bconsole_write(struct console *con, const char *s, unsigned c);
#endif
#endif
