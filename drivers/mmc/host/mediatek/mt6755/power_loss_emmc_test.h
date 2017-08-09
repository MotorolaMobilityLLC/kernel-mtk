#ifndef POWER_LOSS_EMMC_TEST_H
#define POWER_LOSS_EMMC_TEST_H

/* Don't include this file directly.
   This file shall be include via including power_loss_test.h */
#define MVG_EMMC_TIME_DIVISOR_SET       _IOW('V', 150, int)
#define MVG_EMMC_TIME_DIVISOR_GET       _IOW('V', 151, int)
#define MVG_EMMC_RESET_MODE_SET         _IOW('V', 152, int)
#define MVG_EMMC_RESET_TIME_MODE_SET    _IOW('V', 153, int)
#define MVG_EMMC_SET_ERASE_GROUP_SIZE   _IOW('V', 154, int)
#define MVG_EMMC_GET_DELAY_RESULT       _IOW('V', 155, int)
#define MVG_EMMC_SET_DELAY_TABLE        _IOW('V', 156, int)

#define MVG_EMMC_PERFTABLE_SIZE         16

#define GPIO_TRIGGER_HW_POWER_LOSS
#define SW_RESET_CONTROLLED_BY_TIMER
#define TIME_FROM_GPIO_TRIGGER_TO_POWER_DROP    28000000
	/*
	With 220u capacitor after GPIO118
	28ms=28000000ns for VIO18
	*/
	/*
	Without capacitor after GPIO118
	14ms=14000000ns for VIO18 and 5ms for VEMC, choose the large one.
	*/
	/*
	change it according to actual measurement result
	*/
#define TIME_POWER_DROPING                      22000000
	/*
	22ms, with 220u capacitor after GPIO118,
	VIO18 cannot drop to lowest
	*/
	/*
	32ms=32000000ns for VIO18
	*/

#define TIME_TRIGGER_POWER_DROP_BEFORE_CMD \
	      (TIME_FROM_GPIO_TRIGGER_TO_POWER_DROP + TIME_POWER_DROPING)

#if defined(PWR_LOSS_SPOH) && defined(CONFIG_MTK_EMMC_SUPPORT)
struct mvg_spoh_emmc_priv_type {
	int reset_time_divisor; /* For eMMC to try a shorter reset time */
	int emmc_reset_mode;
	int emmc_reset_time_mode;
	int emmc_erase_group_sector;
	int emmc_erase_group_sector_perf_table_idx;
	u32 match_len;
	int reset_delay_result; /*For UT without real reset
				  to query if busy end before planned delay */
	u32 erase_set;
	u64 erase_start;
	u64 erase_end;
};

enum mvg_emmc_reset_mode {
	MVG_EMMC_NO_RESET = 0,
	MVG_EMMC_RESET_WO_POWEROFF = 1,
	MVG_EMMC_RESET_W_POWEROFF = 2,
	MVG_EMMC_RESET_SWITCH_BETWEEN_W_POWEROFF_AND_WO_POWEROFF = 3,
	MVG_EMMC_EXTERNAL_RANDOM_POWEROFF = 4
};

enum mvg_emmc_reset_time_mode {
	MVG_EMMC_RESET_LEN_DEPEND = 0,
	MVG_EMMC_RESET_USER_SPEC = 1
};

extern struct mvg_spoh_emmc_priv_type mvg_spoh_emmc_priv;

void mvg_emmc_reset(void);
void mvg_emmc_check_busy_and_reset(int delay_ms, int delay_us, u64 addr,
	u32 len);
int mvg_emmc_match(void *host, u64 addr, u32 opcode, u32 len);
void mvg_emmc_hrtimer_start(int delay_ns, u64 addr, u32 len);
void mvg_emmc_hrtimer_init(unsigned long data);
void mvg_emmc_timer_init(unsigned long data);
void mvg_emmc_timer_set(u32 inc);

#if defined(GPIO_TRIGGER_HW_POWER_LOSS)
#define MVG_EMMC_SETUP(host) do { \
		if (host->hw->host_function == MSDC_EMMC) { \
			mvg_emmc_hrtimer_init((unsigned long)host); \
		} \
	} while (0)

#define MVG_EMMC_RESET() mvg_emmc_reset()

#define MVG_EMMC_CHECK_BUSY_AND_RESET(delay_ms, delay_us, addr, len)
#define MVG_EMMC_ERASE_RESET(delay_ms, delay_us, opcode)

#else
#if defined(SW_RESET_CONTROLLED_BY_TIMER)
#define MVG_EMMC_SETUP(host) do { \
		if (host->hw->host_function == MSDC_EMMC) { \
			mvg_emmc_hrtimer_init((unsigned long)host); \
		} \
	} while (0)

#define MVG_EMMC_RESET() mvg_emmc_reset()

#define MVG_EMMC_CHECK_BUSY_AND_RESET(delay_ms, delay_us, addr, len)
#define MVG_EMMC_ERASE_RESET(delay_ms, delay_us, opcode)

#else
#define MVG_EMMC_SETUP(host)
#define MVG_EMMC_RESET()

#define MVG_EMMC_CHECK_BUSY_AND_RESET(delay_ms, delay_us, addr, len) do { \
		if (mvg_spoh_emmc_priv.emmc_reset_mode != \
			MVG_EMMC_RESET_W_POWEROFF) { \
			if (delay_ms || delay_us) { \
				mvg_emmc_check_busy_and_reset(delay_ms, \
					delay_us, addr, len); \
			} \
		} \
	} while (0)

#define MVG_EMMC_ERASE_RESET(delay_ms, delay_us, opcode) do { \
		if (opcode == 38) { \
			if (mvg_spoh_emmc_priv.emmc_reset_mode != \
				MVG_EMMC_RESET_W_POWEROFF) { \
				if (delay_ms || delay_us) { \
					mvg_emmc_check_busy_and_reset( \
						delay_ms, delay_us, 0, 0); \
				} \
			} \
		} \
	} while (0)
#endif

#endif

#define MVG_EMMC_WRITE_MATCH(host, addr, ms, us, ns, opcode, len) do { \
		ms = 0; \
		us = 0; \
		ns = 0; \
		if (opcode == 24 || opcode == 25) { \
			if (host->hw->host_function == MSDC_EMMC) { \
				ns = mvg_emmc_match( \
					(void *)host, addr, opcode, len); \
				ms = ns / 1000000; \
				us = (ns / 1000) % 1000; \
			} \
		} \
	} while (0)


#define MVG_EMMC_ERASE_MATCH(host, addr, ms, us, ns, opcode) do { \
		ms = 0; \
		us = 0; \
		ns = 0; \
		if ((opcode == 35) || (opcode == 36) || (opcode == 38)) { \
			if (host->hw->host_function == MSDC_EMMC) { \
				ns = mvg_emmc_match( \
					(void *)host, addr, opcode, 0); \
				ms = ns / 1000000; \
				us = (ns / 1000) % 1000; \
			} \
		} \
	} while (0)

#define MVG_EMMC_DECLARE_INT32(var) int var

int  mvg_emmc_get_reset_time_divisor(void *power_loss_info);
void mvg_emmc_set_reset_time_divisor(void *power_loss_info, int divisor);
void mvg_emmc_reset_mode_set(void *power_loss_info, int mode);
void mvg_emmc_reset_time_mode_set(void *power_loss_info, int mode);
void mvg_emmc_set_erase_group_sector(void *power_loss_info, int sector);
int mvg_emmc_get_delay_result(void *power_loss_info);
int mvg_emmc_get_set_delay_table(void *power_loss_info, unsigned char *tbl);

#else

#define MVG_EMMC_CHECK_BUSY_AND_RESET(...)
#define MVG_EMMC_SETUP(...)
#define MVG_EMMC_RESET(...)
#define MVG_EMMC_WRITE_MATCH(...)
#define MVG_EMMC_ERASE_MATCH(...)
#define MVG_EMMC_ERASE_RESET(...)
#define MVG_EMMC_DECLARE_INT32(...)

#endif

#endif /* end of POWER_LOSS_TEST_H */

