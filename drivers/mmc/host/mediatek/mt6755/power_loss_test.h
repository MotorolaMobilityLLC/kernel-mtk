#ifndef POWER_LOSS_TEST_H
#define POWER_LOSS_TEST_H

#include <asm-generic/ioctl.h>
#include <linux/rwsem.h>
#include <linux/proc_fs.h>
#include <linux/rtc.h>
#include <linux/delay.h> /* for PL_RESET */

#if !defined(CONFIG_MTK_GPT_SCHEME_SUPPORT)
#include "pmt.h"
#endif

#ifdef CONFIG_PWR_LOSS_MTK_SPOH
#define PWR_LOSS_DEBUG
#define PWR_LOSS_SPOH
#endif

/*#include <mach/power_loss_emmc_test.h>*/
#include "power_loss_emmc_test.h" /*work around before power_loss_test.h put into correct path */

/* Phyiscal Power-Cut Timing Configrations (us) */
#define MVG_CFG_GPIO_TO_VCC_DELAY       5000   /* 5ms */
#define MVG_CFG_VCC_DROP_TIME           4000   /* 4ms */

/* IO control interface for user mode programs */
typedef struct {
	char a[32];
} CHAR_32_ARRAY;

#define PRINT_REBOOT_TIMES              _IOW('V', 1, int)
#define PRINT_DATA_COMPARE_ERR          _IOW('V', 2, CHAR_32_ARRAY)
#define PRINT_FILE_OPERATION_ERR        _IOW('V', 3, int)
#define PRINT_GENERAL_INFO              _IOW('V', 4, int)
#define PRINT_NVRAM_ERR                 _IOW('V', 5, int)
#define PRINT_FAT_ERR                   _IOW('V', 6, int)
#define PRINT_RAW_RW_INFO               _IOW('V', 7, int)

#define MVG_ADDR_RANGE_ADD              _IOW('V', 128, struct mvg_addr)
#define MVG_ADDR_RANGE_DELETE           _IOW('V', 129, u64)
#define MVG_ADDR_RANGE_CHECK            _IOW('V', 130, u64)
#define MVG_ADDR_RANGE_CLEAR            _IOW('V', 131, int)
#define MVG_WDT_CONTROL                 _IOW('V', 132, int)
#define MVG_COUNTER_SET                 _IOW('V', 133, int)
#define MVG_COUNTER_GET                 _IOW('V', 134, int)
#define MVG_SET_CURR_CASE_NAME          _IOW('V', 135, CHAR_32_ARRAY)
#define MVG_SET_CURR_GROUP_NAME         _IOW('V', 136, CHAR_32_ARRAY)
#define MVG_SET_TRIGGER                 _IOW('V', 137, int)
#define MVG_METHOD_SET                  _IOW('V', 138, int)
#define MVG_METHOD_GET                  _IOW('V', 139, int)

#define MVG_NAND_WDT_CONTROL            _IOW('V', 172, int)


/*_IOW('V', 150~170, int) reserved for emmc, refer to power_loss_emmc_test.h*/

#define WDT_REBOOT_ON  1
#define WDT_REBOOT_OFF 0

#define MVG_NAME_LIMIT 64

struct mvg_addr {
	u64 base;
	u64 end;
};

struct mvg_addr_list_entry {
	struct list_head list;
	u64 base;
	u64 end;
};

#if defined(CONFIG_MTK_EMMC_SUPPORT) && defined(CONFIG_MTK_GPT_SCHEME_SUPPORT)
#define MAX_PARTITION_NAME_LEN 64
/*64bit*/
typedef struct {
	unsigned char name[MAX_PARTITION_NAME_LEN];     /* partition name */
	unsigned long long size;
	unsigned long long offset;
	unsigned long long mask_flags;

} pt_resident;
/*32bit*/
typedef struct {
	unsigned char name[MAX_PARTITION_NAME_LEN];     /* partition name */
	unsigned long size;
	unsigned long offset;
	unsigned long mask_flags;
} pt_resident32;
#endif

/* WDT reset or Physical Power Cut (via GPIO + Relay) */
enum mvg_method_enum {
	MVG_METHOD_SW_WDT = 0,  /* Default */
	MVG_METHOD_HW_CUT = 1
};

typedef struct {
	struct rw_semaphore rwsem;
	int wdt_reboot_support;

	/* Added for SPOH */
	int wdt_enable;    /* 1: Enable WDT,  0: Disable WDT */
	u32 flags;         /* flags passed from mvg_app (user mode) */
	int pl_trigger;    /* Trigger mask used to
			      control the probability of WDT rest. */
	int pl_counter;    /* Count of powerloss */
	int pl_method;     /* WDT reset or Physical Power Cut
			      (via GPIO + Relay) */

	char current_case_name[MVG_NAME_LIMIT+1];
	char current_group_name[MVG_NAME_LIMIT+1];

	struct list_head addr_list;

	void *fs_priv;
	void *mvg_priv;
	void *drv_priv;

} wdt_reboot_info;

enum mvg_wdt_enum {
	MVG_WDT_DISABLE    =  0,
	MVG_WDT_ENABLE     =  1,
	MVG_WDT_RESET_NOW  =  2
};


enum mvg_nand_wdt_enum {
	MVG_WDT_PROGRAM_DISABLE    =  0,
	MVG_WDT_PROGRAM_ENABLE     =  1,
	MVG_WDT_ERASE_DISABLE      =  2,
	MVG_WDT_ERASE_ENABLE       =  3
};

enum mvg_trigger_enum {
	MVG_TRIGGER_NEVER  =  0,
	MVG_TRIGGER_ALWAYS = -1
};


/* API for kenrel storage drivers */
int mvg_trigger(void);
void mvg_set_trigger(int trigger);

#ifdef CONFIG_MTK_MTD_NAND
void mvg_nand_set_program_wdt(int trigger);
void mvg_nand_set_erase_wdt(int trigger);
#endif

void mvg_set_wdt(int enable);
int mvg_get_wdt(void);

void mvg_set_counter(int counter);
int  mvg_get_counter(void);

void mvg_set_flag(u32 flag);
u32  mvg_get_flag(void);

void mvg_set_method(int method);
int mvg_get_method(void);

/* Get/Set current test group/case name */
void mvg_get_curr_case_name(char *str, int size);
void mvg_get_curr_group_name(char *str, int size);
void mvg_set_curr_case_name(char *str);
void mvg_set_curr_group_name(char *str);
int mvg_on_group_case(const char *group_name, const char *case_name);

/* Add/Delete/Clear testing address range */
int mvg_addr_range_add(u64 base, u64 end);
int mvg_partition_info(const char *name, pt_resident *info);
void mvg_addr_range_del(u64 addr);
void mvg_addr_range_clear(void);

/* Check testing address range */
int mvg_addr_range_check(u64 addr);

/* Reset control macros for kenerl drivers */
extern void mvg_wdt_reset(void);
extern void wdt_arch_reset(char mode);
extern void usleep_range(unsigned long min, unsigned long max);
extern void get_random_bytes(void *buf, int nbytes);

#if defined(PWR_LOSS_SPOH)

/* Print debug message, only when page address fall on testing range */
#define PL_PRINT(level, addr, str, ...) do { \
		if (mvg_addr_range_check(addr)) { \
			pr_err(level str, ##__VA_ARGS__); \
		} \
	} while (0)

#define PL_BEGIN(time) do_gettimeofday(&time)

#define PL_END(time, duration) do { \
		static struct timeval endtime; \
		do_gettimeofday(&endtime); \
		duration = (endtime.tv_sec * 1000000 + endtime.tv_usec) \
			- (time.tv_sec * 1000000 + time.tv_usec); \
	} while (0)


#define PL_DELAY(time) do { \
		mdelay(time/1000); \
		udelay(time%1000); \
	} while (0)

#define PL_IS_HW_RESET(time)  (time&0x80000000)

/*set output GPIO118 to 1*/
/*set GPIO118 to output mode*/
#define PL_GPIO_SWITCH(time) do { \
		if (PL_IS_HW_RESET(time)) { \
			u32 _t = time & 0x7FFFFFFF; \
			DRV_WriteReg32(0xF0005134, 0x400000); \
			DRV_WriteReg32(0xF0005034, 0x400000); \
			PL_DELAY(_t); \
		} \
	} while (0)

/* do endless while, if HW reset is going to happen */
#ifdef CONFIG_MTK_MTD_NAND
#define PL_RESET(time) do { \
		while (PL_IS_HW_RESET(time)) \
			; \
		if (time) { \
			PL_DELAY(time); \
			(void)mtk_nand_set_command(NAND_CMD_RESET); \
			mvg_wdt_reset(); \
		} \
	} while (0)
#else
#define PL_RESET(time) do { \
		while (PL_IS_HW_RESET(time)) \
			; \
		if (time) { \
			PL_DELAY(time); \
			mvg_wdt_reset(); \
		} \
	} while (0)
#endif

/* set bit 32 to indicate HW cut, don't care PL_RESET(),
   will delay 2^31us ~= 35min */
#define PL_TIME_RAND(addr, time, time_max) do { \
		time = 0; \
		if (mvg_addr_range_check(addr)) { \
			if ((time_max) && (mvg_trigger())) { \
				get_random_bytes(&time, sizeof(u32)); \
				if (mvg_get_method() == MVG_METHOD_HW_CUT) { \
					time = time % \
						(time_max + \
						MVG_CFG_VCC_DROP_TIME) \
						+ 1; \
					pr_err(TAG " HW reset in %d" \
						" (GPIO+%d) us\n", \
						time, \
						MVG_CFG_GPIO_TO_VCC_DELAY); \
					time |= 0x80000000; \
					time += MVG_CFG_GPIO_TO_VCC_DELAY; \
				} else { \
					time = time % time_max + 1; \
					pr_err(TAG " SW reset in %d us\n", \
						time); \
				} \
				dump_stack(); \
				udelay(350); \
			} \
		} \
	} while (0)

#define PL_RESET_ON_CASE(group_name, case_name) do { \
		if (mvg_on_group_case(group_name, case_name) && \
		    mvg_get_wdt()) { \
			pr_err("[MVG_TEST]: reset on case %s: %s\n", \
				group_name, case_name); \
			mvg_set_trigger(-1); \
		} \
	} while (0)

#else

#define PL_PRINT(addr, level, str, ...)
#define PL_BEGIN(time)
#define PL_END(time, duration)
#define PL_RESET(time)
#define PL_TIME_RAND(addr, time, time_max)
#define PL_RESET_ON_CASE(group_name, case_name)
#define PL_GPIO_SWITCH(time)


#endif

extern void wdt_arch_reset(char mode);

extern void get_random_bytes(void *buf, int nbytes);

#ifdef CONFIG_MTK_MTD_NAND
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <mach/mtk_nand.h>
extern struct mtk_nand_host *host;

#include <mach/mt_wdt.h>
#include <mach/mt_typedefs.h>
extern void L1_rst_disable(unsigned int disable);
extern void L2_rst_disable(unsigned int disable);
extern void wdt_dump_reg(void);
#endif

#ifdef CONFIG_MTK_EMMC_SUPPORT
extern pt_resident *lastest_part; /* body @ pmt.c */
#else
extern pt_resident lastest_part[];  /* body @ partition_mt.c */
#endif

#endif /* end of POWER_LOSS_TEST_H */

