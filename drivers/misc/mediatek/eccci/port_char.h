
extern int process_rpc_kernel_msg(struct ccci_port *port, struct ccci_request *req);

extern int rawbulk_push_upstream_buffer(int transfer_id, const void *buffer,
		unsigned int length);

int scan_image_list(int md_id, char fmt[], int out_img_list[], int img_list_size);

#ifndef CONFIG_MTK_ECCCI_C2K
#ifdef CONFIG_MTK_SVLTE_SUPPORT
extern void c2k_reset_modem(void);
#endif
#endif
