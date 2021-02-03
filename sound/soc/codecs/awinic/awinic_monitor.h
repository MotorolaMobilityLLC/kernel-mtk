#ifndef __AWINIC_MONITOR_H__
#define __AWINIC_MONITOR_H__

/*#define AW_DEBUG*/

struct aw_table;

#define AW_TABLE_SIZE	sizeof(struct aw_table)
#define AW882XX_MONITOR_DEFAULT_FLAG (0)
#define AW882XX_MONITOR_VBAT_RANGE (6025)
#define AW882XX_MONITOR_INT_10BIT (1023)
#define AW882XX_MONITOR_TEMP_SIGN_MASK (1<<9)
#define AW882XX_MONITOR_TEMP_NEG_MASK (0XFC00)
#define AW882XX_BIT_SYSCTRL2_BST_IPEAK_MASK (15<<0)
#define AW882XX_BIT_HAGCCFG4_GAIN_SHIFT (8)
#define AW882XX_BIT_HAGCCFG4_GAIN_MASK (0x00ff)
#define IPEAK_NONE	(0xFF)
#define GAIN_NONE	(0xFF)
#define VMAX_NONE	(0xFFFFFFFF)
#define GET_DB_INT(x)	(x / 2)
#define GET_DB_DECIMAL(x)	((x % 2) * 5)
#define AW_LOAD_MON_FW_DELAY_TIME	(3000)


#define GET_32_DATA(w, x, y, z) \
	((uint32_t)((((uint8_t)w) << 24) | (((uint8_t)x) << 16) | (((uint8_t)y) << 8) | ((uint8_t)z)))
#define GET_16_DATA(x, y) \
	((uint16_t)((((uint8_t)x) << 8) | (uint8_t)y))


enum aw_monitor_hdr_ver {
	AW_MONITOR_HDR_VER_0_1_0 = 0x00010000,
};

enum {
	AW_FIRST_ENTRY = 0,
	AW_NOT_FIRST_ENTRY = 1,
};

struct aw_table {
	int16_t min_val;
	int16_t max_val;
	uint16_t ipeak;
	uint16_t gain;
	uint32_t vmax;
};

struct aw_table_info {
	uint8_t table_num;
	struct aw_table *aw_table;
};


enum aw_monitor_init {
	AW_MON_CFG_ST = 0,
	AW_MON_CFG_OK = 1,
};

struct aw_monitor_cfg {
	uint8_t monitor_status;
	uint32_t monitor_switch;
	uint32_t monitor_time;
	uint32_t monitor_count;
	uint32_t temp_switch;
	uint32_t temp_aplha;
	uint32_t vol_switch;
	uint32_t vol_aplha;
	uint32_t ipeak_switch;
	uint32_t gain_switch;
	uint32_t vmax_switch;
	struct aw_table_info temp_info;
	struct aw_table_info vol_info;
};

struct aw_monitor_hdr {
	uint32_t check_sum;
	uint32_t monitor_ver;
	char chip_type[8];
	uint32_t ui_ver;
	uint32_t monitor_switch;
	uint32_t monitor_time;
	uint32_t monitor_count;
	uint32_t ipeak_switch;
	uint32_t gain_switch;
	uint32_t vmax_switch;
	uint32_t temp_switch;
	uint32_t temp_aplha;
	uint32_t temp_num;
	uint32_t single_temp_size;
	uint32_t temp_offset;
	uint32_t vol_switch;
	uint32_t vol_aplha;
	uint32_t vol_num;
	uint32_t single_vol_size;
	uint32_t vol_offset;
};

struct aw_monitor_trace {
	int32_t pre_val;
	int32_t sum_val;
	struct aw_table aw_table;
};

/******************************************************************
* struct aw882xx monitor
*******************************************************************/
struct aw882xx_monitor {
	struct delayed_work delay_work;
	struct delayed_work load_fw_work;
	struct aw_monitor_cfg *monitor_cfg;
	struct list_head list;
	uint32_t is_enable;
	uint8_t first_entry;
	uint8_t samp_count;
	uint32_t pre_vmax;

	struct aw_monitor_trace temp_trace;
	struct aw_monitor_trace vol_trace;
#ifdef AW_DEBUG
	uint16_t test_vol;
	int16_t test_temp;
#endif
};

/******************************************************************
* aw882xx monitor functions
*******************************************************************/
void aw882xx_monitor_start(struct aw882xx_monitor *monitor);
void aw882xx_monitor_stop(struct aw882xx_monitor *monitor);
void aw882xx_parse_monitor_dt(struct aw882xx_monitor *monitor);
void aw882xx_monitor_init(struct aw882xx_monitor *monitor);
void aw882xx_monitor_deinit(struct aw882xx_monitor *monitor);



#endif

