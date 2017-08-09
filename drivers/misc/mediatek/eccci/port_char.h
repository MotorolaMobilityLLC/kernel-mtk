
extern int rawbulk_push_upstream_buffer(int transfer_id, const void *buffer,
		unsigned int length);
#ifndef CONFIG_MTK_ECCCI_C2K
#ifdef CONFIG_MTK_SVLTE_SUPPORT
extern void c2k_reset_modem(void);
#endif
#endif
