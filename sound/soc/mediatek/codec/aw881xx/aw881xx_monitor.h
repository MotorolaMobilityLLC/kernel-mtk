#ifndef __AW881XX_MONITOR_H__
#define __AW881XX_MONITOR_H__

/********************************************
 *
 * enum
 *
 *******************************************/
enum {
	AW881XX_VOL_LIMIT_35 = 3500,
	AW881XX_VOL_LIMIT_36 = 3600,
	AW881XX_VOL_LIMIT_37 = 3700,
	AW881XX_VOL_LIMIT_38 = 3800,
	AW881XX_VOL_LIMIT_39 = 3900,
	AW881XX_VOL_LIMIT_40 = 4000,
};

enum {
	AW881XX_TEMP_LIMIT_NEG_5 = -5,
	AW881XX_TEMP_LIMIT_NEG_2 = -2,
	AW881XX_TEMP_LIMIT_0 = 0,
	AW881XX_TEMP_LIMIT_2 = 2,
	AW881XX_TEMP_LIMIT_5 = 5,
	AW881XX_TEMP_LIMIT_7 = 7,
};

enum {
	AW881XX_IPEAK_350 = 0X04,
	AW881XX_IPEAK_300 = 0X02,
	AW881XX_IPEAK_275 = 0X01,
	AW881XX_IPEAK_250 = 0X00,
	AW881XX_IPEAK_NONE = 0XFF,
};

enum {
	AW881XX_GAIN_00DB = 0X00,
	AW881XX_GAIN_NEG_05DB = 0X01,
	AW881XX_GAIN_NEG_10DB = 0X02,
	AW881XX_GAIN_NEG_15DB = 0X03,
	AW881XX_GAIN_NEG_30DB = 0X06,
	AW881XX_GAIN_NEG_45DB = 0X09,
	AW881XX_GAIN_NEG_60DB = 0X10,
	AW881XX_GAIN_NONE = 0XFF,
};

enum {
	AW881XX_VMAX_80 = 0XFA41,
	AW881XX_VMAX_70 = 0XF918,
	AW881XX_VMAX_60 = 0XF7C2,
	AW881XX_VMAX_50 = 0XF6C2,
	AW881XX_VMAX_NONE = 0XFFFF,
};

/********************************************
 *
 * struct aw881xx_monitor
 *
 *******************************************/
struct aw881xx_monitor {
	struct hrtimer monitor_timer;
	struct work_struct monitor_work;
	uint32_t monitor_flag;
	uint32_t monitor_timer_val;
	int32_t pre_temp;
	int32_t pre_vol_bst_ipeak;
	int32_t pre_vol_gain_db;
	int32_t pre_temp_bst_ipeak;
	int32_t pre_temp_gain_db;
	int32_t pre_temp_vmax;
};

void aw881xx_monitor_stop(struct aw881xx_monitor *monitor);
void aw881xx_monitor_start(struct aw881xx_monitor *monitor);
int aw881xx_monitor_init(struct aw881xx_monitor *monitor);

#endif
