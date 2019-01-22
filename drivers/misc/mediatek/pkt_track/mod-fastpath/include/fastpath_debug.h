#define K_EMERG		(1<<7)
#define K_ALET		(1<<6)
#define K_CRIT		(1<<5)
#define K_ERR		(1<<4)
#define K_WARNIN	(1<<3)
#define K_NOTICE	(1<<2)
#define K_INFO		(1<<1)
#define K_DEBUG		(1<<0)

/*Set the log level at fastpath.c*/
extern u32 fp_log_level;

#define fp_printk(level, fmt, args...) do { \
		if (fp_log_level & level) { \
			pr_err("[MDT]" fmt, ## args); \
		} \
	} while (0)
