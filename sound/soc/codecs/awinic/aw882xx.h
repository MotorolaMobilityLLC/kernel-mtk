#ifndef _AW882XX_H_
#define _AW882XX_H_

#include <linux/version.h>
#include <sound/control.h>
#include <sound/soc.h>
#include "awinic_cali.h"
#include "awinic_monitor.h"


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 1)
#define AW_KERNEL_VER_OVER_4_19_1
#endif

/*
 * i2c transaction on Linux limited to 64k
 * (See Linux kernel documentation: Documentation/i2c/writing-clients)
*/
#define MAX_I2C_BUFFER_SIZE			(65536)
#define AW882XX_I2C_READ_MSG_NUM		(2)


#define AW882XX_FLAG_START_ON_MUTE		(1 << 0)
#define AW882XX_FLAG_SKIP_INTERRUPTS		(1 << 1)
#define AW882XX_FLAG_SAAM_AVAILABLE		(1 << 2)
#define AW882XX_FLAG_STEREO_DEVICE		(1 << 3)
#define AW882XX_FLAG_MULTI_MIC_INPUTS		(1 << 4)

#define AW882XX_NUM_RATES			(9)
#define AW882XX_SYSST_CHECK_MAX			(10)
#define AW882XX_MODE_SHIFT_MAX			(2)

#define AW882XX_CFG_NAME_MAX			(64)
#define AW_FADE_OUT_TARGET_VOL			(90 * 2)
#define AW882XX_VOLUME_STEP_DB			(6 * 2)


enum aw882xx_pa_switch_st {
	AW882XX_ON_PA = 0,
	AW882XX_OFF_PA,
};

enum aw882xx_chip_st {
	AW882XX_PA_CLOSE_ST = 0,
	AW882XX_PA_CLOSEING_ST,
	AW882XX_PA_OPEN_ST,
	AW882XX_PA_OPENING_ST,
};

enum aw882xx_audio_stream_st {
	AW882XX_AUDIO_STOP = 0,
	AW882XX_AUDIO_START = 1,
};

enum aw882xx_channel_mode_dsp {
	AW882XX_CHANNLE_LEFT_MONO = 0,
	AW882XX_CHANNLE_RIGHT = 1,
};

enum aw882xx_init {
	AW882XX_INIT_ST = 0,
	AW882XX_INIT_OK = 1,
	AW882XX_INIT_NG = 2,
};

enum aw882xx_chipid {
	AW882XX_ID = 0x1852,
};

enum aw882xx_modeshift {
	AW882XX_MODE_SPK_SHIFT = 0,
	AW882XX_MODE_RCV_SHIFT = 1,
};

enum aw882xx_mode_spk_rcv {
	AW882XX_SPEAKER_MODE = 0,
	AW882XX_RECEIVER_MODE = 1,
};

struct aw882xx_container {
	int len;
	unsigned char data[];
};

struct aw882xx_chan_info {
	unsigned int channel;
	char *name_suffix;
	char (*bin_cfg_name)[AW882XX_CFG_NAME_MAX];
};

#ifdef AW_KERNEL_VER_OVER_4_19_1
typedef struct snd_soc_component aw_snd_soc_codec_t;
typedef struct snd_soc_component_driver aw_snd_soc_codec_driver_t;
#else
typedef struct snd_soc_codec aw_snd_soc_codec_t;
typedef struct snd_soc_codec_driver aw_snd_soc_codec_driver_t;
#endif

struct aw_componet_codec_ops {
	aw_snd_soc_codec_t *(*aw_snd_soc_kcontrol_codec)(struct snd_kcontrol *kcontrol);
	void *(*aw_snd_soc_codec_get_drvdata)(aw_snd_soc_codec_t *codec);
	int (*aw_snd_soc_add_codec_controls)(aw_snd_soc_codec_t *codec,
		const struct snd_kcontrol_new *controls,
		unsigned int num_controls);
	void (*aw_snd_soc_unregister_codec)(struct device *dev);
	int (*aw_snd_soc_register_codec)(struct device *dev,
			const aw_snd_soc_codec_driver_t *codec_drv,
			struct snd_soc_dai_driver *dai_drv,
			int num_dai);
};

/********************************************
 * struct aw882xx
 *******************************************/
struct aw882xx {
	int sysclk;
	int rate;
	int pstream;
	int cstream;
	int hagce_enable;
	unsigned int hagce_val;
	unsigned int rmse_val;

	int reset_gpio;
	int irq_gpio;
	int aw882xx_pa_switch;
	int is_power_on;
	int audio_stream_st;
	uint16_t intmask;
	uint32_t fade_step;
	uint32_t fade_utime;
	uint32_t db_offset;

	unsigned char reg_addr;

	unsigned int flags;
	unsigned int chipid;
	unsigned int init;
	unsigned int cfg_num;
	unsigned int scene_mode;

	aw_snd_soc_codec_t *codec;
	struct regmap *regmap;
	struct i2c_client *i2c;
	struct device *dev;
	struct mutex lock;

	struct aw882xx_chan_info chan_info;
	struct aw_cali cali;
	struct aw882xx_monitor monitor;
};

/********************************************
 * print information control
 *******************************************/
#define aw_dev_err(dev, format, ...) \
			pr_err("[%s]" format, dev_name(dev), ##__VA_ARGS__)

#define aw_dev_info(dev, format, ...) \
			pr_info("[%s]" format, dev_name(dev), ##__VA_ARGS__)

#define aw_dev_dbg(dev, format, ...) \
			pr_debug("[%s]" format, dev_name(dev), ##__VA_ARGS__)


/*********************************************
* aw882xx functions
**********************************************/
int aw882xx_i2c_write(struct aw882xx *aw882xx,
	unsigned char reg_addr, unsigned int reg_data);
int aw882xx_i2c_read(struct aw882xx *aw882xx,
	unsigned char reg_addr, unsigned int *reg_data);
void aw882xx_append_channel(char *format, const char **change_name,
	struct aw882xx *aw882xx);
void aw882xx_smartpa_cfg(struct aw882xx *aw882xx, bool flag);
int aw882xx_set_volume(struct aw882xx *aw882xx, uint32_t value);
int aw882xx_get_volume(struct aw882xx *aw882xx, uint32_t *value);
uint32_t aw882xx_reg_val_to_db(uint32_t value);
int aw882xx_i2c_probe(struct i2c_client *i2c,const struct i2c_device_id *id);
int aw882xx_i2c_remove(struct i2c_client *i2c);

int aw882xx_append_suffix(char *format, const char **change_name,
		struct aw882xx *aw882xx);
#endif

