#ifndef __SCP_EXCEP_H__
#define __SCP_EXCEP_H__

typedef enum scp_excep_id {
	EXCEP_LOAD_FIRMWARE = 0,
	EXCEP_RESET,
	EXCEP_BOOTUP,
	EXCEP_RUNTIME,
	SCP_NR_EXCEP,
} scp_excep_id;

extern void scp_aed(scp_excep_id type);
extern void scp_aed_reset(scp_excep_id type);
extern void scp_aed_reset_inplace(scp_excep_id type);
extern void scp_get_log(int save);
extern void scp_dump_regs(void);
extern void aed_md32_exception_api(const int *log, int log_size, const int *phy,
int phy_size, const char *detail, const int db_opt);

#endif
