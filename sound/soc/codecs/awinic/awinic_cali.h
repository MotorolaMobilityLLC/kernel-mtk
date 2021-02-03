#ifndef __AWINIC_CALI_FS_H__
#define __AWINIC_CALI_FS_H__
#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include "awinic_dsp.h"

#define MOHM_TO_FIXED_RE(x)		((x << 12) / 1000)
#define FIXED_RE_TO_MOHM(x)		((x * 1000) >> 12)

#define AW_IOCTL_MSG_VERSION		(0)

/*********************************************
* dsp
**********************************************/
#define AW_CALI_STORE_EXAMPLE
/* ERRO CALI RE */
#define AW_ERRO_CALI_VALUE (0)

#define AWINIC_DSP_MSG
/*#define AWINIC_DSP_HMUTE*/
#define AW_CALI_RE_DEFAULT_TIMER (3000)

#define AW_CALI_RE_MIN_TIMER (1000)
#define AW_DEV_MAX (2)


/*********misc device ioctl fo cali**********/
#define AW882XX_CALI_CFG_NUM (3)
#define AW882XX_CALI_DATA_NUM (6)
#define AW882XX_CALI_STATUS_DATA_NUM (8)
#define AW882XX_PARAMS_NUM (400)
#define AW882XX_KILO_PARAMS_NUM (1000)


struct cali_cfg {
	int32_t data[AW882XX_CALI_CFG_NUM];
};
struct cali_data {
	int32_t data[AW882XX_CALI_DATA_NUM];
};
struct aw_cali_st {
	int32_t data[AW882XX_CALI_STATUS_DATA_NUM];
};
struct params_data {
	int32_t data[AW882XX_PARAMS_NUM];
};
struct ptr_params_data {
	int len;
	int32_t *data;
};

typedef struct {
	int32_t type;
	int32_t opcode_id;
	int32_t version;
	int32_t data_len;
	char *data_buf;
	int32_t reseriver[2];
}aw_ioctl_msg_t;

#define AW882XX_IOCTL_MAGIC			'a'
#define AW882XX_IOCTL_SET_CALI_CFG		_IOWR(AW882XX_IOCTL_MAGIC, 1, struct cali_cfg)
#define AW882XX_IOCTL_GET_CALI_CFG		_IOWR(AW882XX_IOCTL_MAGIC, 2, struct cali_cfg)
#define AW882XX_IOCTL_GET_CALI_DATA		_IOWR(AW882XX_IOCTL_MAGIC, 3, struct cali_data)
#define AW882XX_IOCTL_SET_NOISE			_IOWR(AW882XX_IOCTL_MAGIC, 4, int32_t)
#define AW882XX_IOCTL_GET_F0			_IOWR(AW882XX_IOCTL_MAGIC, 5, int32_t)
#define AW882XX_IOCTL_SET_CALI_RE		_IOWR(AW882XX_IOCTL_MAGIC, 6, int32_t)
#define AW882XX_IOCTL_GET_CALI_RE		_IOWR(AW882XX_IOCTL_MAGIC, 7, int32_t)
#define AW882XX_IOCTL_SET_VMAX			_IOWR(AW882XX_IOCTL_MAGIC, 8, int32_t)
#define AW882XX_IOCTL_GET_VMAX			_IOWR(AW882XX_IOCTL_MAGIC, 9, int32_t)
#define AW882XX_IOCTL_SET_PARAM			_IOWR(AW882XX_IOCTL_MAGIC, 10, struct params_data)
#define AW882XX_IOCTL_ENABLE_CALI		_IOWR(AW882XX_IOCTL_MAGIC, 11, int8_t)
#define AW882XX_IOCTL_SET_PTR_PARAM_NUM		_IOWR(AW882XX_IOCTL_MAGIC, 12, struct ptr_params_data)
#define AW882XX_IOCTL_GET_F0_Q			_IOWR(AW882XX_IOCTL_MAGIC, 13, struct f0_q_data)
#define AW882XX_IOCTL_SET_DSP_HMUTE		_IOWR(AW882XX_IOCTL_MAGIC, 14, int32_t)
#define AW882XX_IOCTL_SET_CALI_CFG_FLAG		_IOWR(AW882XX_IOCTL_MAGIC, 15, int32_t)
#define AW882XX_IOCTL_MSG			_IOWR(AW882XX_IOCTL_MAGIC, 16, aw_ioctl_msg_t)

typedef struct {
	int32_t type;
	int32_t opcode_id;
	int32_t version;
	int32_t reseriver[3];
}aw_dsp_msg_t;

struct aw_misc_cali {
	struct miscdevice misc_device;
};

struct aw_dbg_cali {
	struct dentry *dbg_dir;
	struct dentry *dbg_range;
	struct dentry *dbg_cali;
	struct dentry *dbg_status;
	struct dentry *dbg_f0;
};

enum {
	AW_IOCTL_MSG_IOCTL = 0,
	AW_IOCTL_MSG_RD_DSP,
	AW_IOCTL_MSG_WR_DSP
};

enum {
	AW_CALI_MODE_NONE = 0,
	AW_CALI_MODE_DBGFS,
	AW_CALI_MODE_MISC,
	AW_CALI_MODE_MAX
};

/*********************************************
* struct aw882xx cali
**********************************************/
struct aw_cali {
	unsigned char cali_mode;
	uint8_t first_cali_num;
	uint8_t end_cali_num;
	int32_t cali_re;
	int32_t cali_f0;
	int32_t re[AW_DEV_MAX];
	int32_t f0[AW_DEV_MAX];
	struct cali_cfg store_cfg[AW_DEV_MAX];
	uint32_t cali_re_time;

	struct aw_dbg_cali dbg_fs;
	struct aw_misc_cali misc;
};

/*********************************************
* aw882xx cali functions
**********************************************/
void aw_cali_init(struct aw_cali *cali);
void aw_cali_deinit(struct aw_cali *cali);

void aw882xx_load_cali_re(struct aw_cali *cali);
void aw882xx_parse_cali_mode_dt(struct aw_cali *cali);
void aw882xx_parse_cali_way_dt(struct aw_cali *cali);




#endif

