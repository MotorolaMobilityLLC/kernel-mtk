#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "osal_typedef.h"
#include "stp_exp.h"
#include "wmt_exp.h"

#include "fm_typedef.h"
#include "fm_dbg.h"
#include "fm_err.h"
#include "fm_interface.h"
#include "fm_stdlib.h"
#include "fm_patch.h"
#include "fm_utils.h"
#include "fm_link.h"
#include "fm_config.h"
#include "fm_private.h"

#include "mt6631_fm_reg.h"
#include "mt6631_fm.h"
#include "mt6631_fm_lib.h"
#include "mt6631_fm_cmd.h"
#include "mt6631_fm_cust_cfg.h"

#define HQA_RETURN_ZERO_MAP 0
#define HQA_ZERO_DESENSE_MAP 0

/* #include "mach/mt_gpio.h" */

/* #define MT6631_FM_PATCH_PATH "/etc/firmware/mt6631/mt6631_fm_patch.bin" */
/* #define MT6631_FM_COEFF_PATH "/etc/firmware/mt6631/mt6631_fm_coeff.bin" */
/* #define MT6631_FM_HWCOEFF_PATH "/etc/firmware/mt6631/mt6631_fm_hwcoeff.bin" */
/* #define MT6631_FM_ROM_PATH "/etc/firmware/mt6631/mt6631_fm_rom.bin" */

static struct fm_patch_tbl mt6631_patch_tbl[5] = {
	{FM_ROM_V1, "/etc/firmware/mt6631/mt6631_fm_v1_patch.bin",
	"/etc/firmware/mt6631/mt6631_fm_v1_coeff.bin", NULL, NULL},
	{FM_ROM_V2, "/etc/firmware/mt6631/mt6631_fm_v2_patch.bin",
	"/etc/firmware/mt6631/mt6631_fm_v2_coeff.bin", NULL, NULL},
	{FM_ROM_V3, "/etc/firmware/mt6631/mt6631_fm_v3_patch.bin",
	"/etc/firmware/mt6631/mt6631_fm_v3_coeff.bin", NULL, NULL},
	{FM_ROM_V4, "/etc/firmware/mt6631/mt6631_fm_v4_patch.bin",
	"/etc/firmware/mt6631/mt6631_fm_v4_coeff.bin", NULL, NULL},
	{FM_ROM_V5, "/etc/firmware/mt6631/mt6631_fm_v5_patch.bin",
	"/etc/firmware/mt6631/mt6631_fm_v5_coeff.bin", NULL, NULL},
};

static struct fm_hw_info mt6631_hw_info = {
	.chip_id = 0x00006631,
	.eco_ver = 0x00000000,
	.rom_ver = 0x00000000,
	.patch_ver = 0x00000000,
	.reserve = 0x00000000,
};

#define PATCH_SEG_LEN 512

static fm_u8 *cmd_buf;
static struct fm_lock *cmd_buf_lock;
static struct fm_callback *fm_cb_op;
static struct fm_res_ctx *mt6631_res;
/* static fm_s32 Chip_Version = mt6631_E1; */

/* static fm_bool rssi_th_set = fm_false; */

#if 0				/* def CONFIG_MTK_FM_50KHZ_SUPPORT */
static struct fm_fifo *cqi_fifo;
#endif
static fm_s32 mt6631_is_dese_chan(fm_u16 freq);

#if 0
static fm_s32 mt6631_mcu_dese(fm_u16 freq, void *arg);
static fm_s32 mt6631_gps_dese(fm_u16 freq, void *arg);
static fm_s32 mt6631_I2s_Setting(fm_s32 onoff, fm_s32 mode, fm_s32 sample);
#endif
static fm_u16 mt6631_chan_para_get(fm_u16 freq);
static fm_s32 mt6631_desense_check(fm_u16 freq, fm_s32 rssi);
static fm_bool mt6631_TDD_chan_check(fm_u16 freq);
static fm_bool mt6631_SPI_hopping_check(fm_u16 freq);
static fm_s32 mt6631_soft_mute_tune(fm_u16 freq, fm_s32 *rssi, fm_bool *valid);
static fm_s32 mt6631_pwron(fm_s32 data)
{
	if (MTK_WCN_BOOL_FALSE == mtk_wcn_wmt_func_on(WMTDRV_TYPE_FM)) {
		WCN_DBG(FM_ALT | CHIP, "WMT turn on FM Fail!\n");
		return -FM_ELINK;
	}

	WCN_DBG(FM_DBG | CHIP, "WMT turn on FM OK!\n");
	return 0;

}

static fm_s32 mt6631_pwroff(fm_s32 data)
{
	if (MTK_WCN_BOOL_FALSE == mtk_wcn_wmt_func_off(WMTDRV_TYPE_FM)) {
		WCN_DBG(FM_ALT | CHIP, "WMT turn off FM Fail!\n");
		return -FM_ELINK;
	}
	WCN_DBG(FM_NTC | CHIP, "WMT turn off FM OK!\n");
	return 0;

}

static fm_s32 Delayms(fm_u32 data)
{
	WCN_DBG(FM_DBG | CHIP, "delay %dms\n", data);
	msleep(data);
	return 0;
}

static fm_s32 Delayus(fm_u32 data)
{
	WCN_DBG(FM_DBG | CHIP, "delay %dus\n", data);
	udelay(data);
	return 0;
}

fm_s32 mt6631_get_read_result(struct fm_res_ctx *result)
{
	if (result == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	mt6631_res = result;

	return 0;
}

static fm_s32 mt6631_read(fm_u8 addr, fm_u16 *val)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_get_reg(cmd_buf, TX_BUF_SIZE, addr);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_FSPI_RD, SW_RETRY_CNT, FSPI_RD_TIMEOUT, mt6631_get_read_result);

	if (!ret && mt6631_res)
		*val = mt6631_res->fspi_rd;

	FM_UNLOCK(cmd_buf_lock);

	return ret;
}

static fm_s32 mt6631_write(fm_u8 addr, fm_u16 val)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_set_reg(cmd_buf, TX_BUF_SIZE, addr, val);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_FSPI_WR, SW_RETRY_CNT, FSPI_WR_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	return ret;
}

static fm_s32 mt6631_set_bits(fm_u8 addr, fm_u16 bits, fm_u16 mask)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_set_bits_reg(cmd_buf, TX_BUF_SIZE, addr, bits, mask);
	ret = fm_cmd_tx(cmd_buf, pkt_size, (1 << 0x11), SW_RETRY_CNT, FSPI_WR_TIMEOUT, NULL);
	/* 0x11 this opcode won't be parsed as an opcode, so set here as spcial case. */
	FM_UNLOCK(cmd_buf_lock);

	return ret;
}

static fm_s32 mt6631_top_read(fm_u16 addr, fm_u32 *val)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_top_get_reg(cmd_buf, TX_BUF_SIZE, addr);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_CSPI_READ, SW_RETRY_CNT, FSPI_RD_TIMEOUT, mt6631_get_read_result);

	if (!ret && mt6631_res)
		*val = mt6631_res->cspi_rd;

	FM_UNLOCK(cmd_buf_lock);

	return ret;
}

static fm_s32 mt6631_top_write(fm_u16 addr, fm_u32 val)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_top_set_reg(cmd_buf, TX_BUF_SIZE, addr, val);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_CSPI_WRITE, SW_RETRY_CNT, FSPI_WR_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	return ret;
}


static fm_s32 mt6631_host_read(fm_u32 addr, fm_u32 *val)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_host_get_reg(cmd_buf, TX_BUF_SIZE, addr);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_HOST_READ, SW_RETRY_CNT, FSPI_RD_TIMEOUT, mt6631_get_read_result);

	if (!ret && mt6631_res)
		*val = mt6631_res->cspi_rd;

	FM_UNLOCK(cmd_buf_lock);

	return ret;
}

static fm_s32 mt6631_host_write(fm_u32 addr, fm_u32 val)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_host_set_reg(cmd_buf, TX_BUF_SIZE, addr, val);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_HOST_WRITE, SW_RETRY_CNT, FSPI_WR_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	return ret;
}


static fm_u16 mt6631_get_chipid(void)
{
	return 0x6631;
}

/*  MT6631_SetAntennaType - set Antenna type
 *  @type - 1, Short Antenna;  0, Long Antenna
 */
static fm_s32 mt6631_SetAntennaType(fm_s32 type)
{
	fm_u16 dataRead;

	WCN_DBG(FM_DBG | CHIP, "set ana to %s\n", type ? "short" : "long");
	mt6631_read(FM_MAIN_CG2_CTRL, &dataRead);

	if (type)
		dataRead |= ANTENNA_TYPE;
	else
		dataRead &= (~ANTENNA_TYPE);

	mt6631_write(FM_MAIN_CG2_CTRL, dataRead);

	return 0;
}

static fm_s32 mt6631_GetAntennaType(void)
{
	fm_u16 dataRead;

	mt6631_read(FM_MAIN_CG2_CTRL, &dataRead);
	WCN_DBG(FM_DBG | CHIP, "get ana type: %s\n", (dataRead & ANTENNA_TYPE) ? "short" : "long");

	if (dataRead & ANTENNA_TYPE)
		return FM_ANA_SHORT;	/* short antenna */
	else
		return FM_ANA_LONG;	/* long antenna */
}

static fm_s32 mt6631_Mute(fm_bool mute)
{
	fm_s32 ret = 0;
	fm_u16 dataRead;

	WCN_DBG(FM_DBG | CHIP, "set %s\n", mute ? "mute" : "unmute");
	/* mt6631_read(FM_MAIN_CTRL, &dataRead); */
	mt6631_read(0x9C, &dataRead);

	/* mt6631_top_write(0x0050, 0x00000007); */
	if (mute == 1)
		ret = mt6631_write(0x9C, (dataRead & 0xFFFC) | 0x0003);
	else
		ret = mt6631_write(0x9C, (dataRead & 0xFFFC));

	/* mt6631_top_write(0x0050, 0x0000000F); */

	return ret;
}


/* FMSYS Ramp Down Sequence*/
static fm_s32 mt6631_RampDown(void)
{
	fm_s32 ret = 0;
	fm_u32 tem;
	fm_u16 pkt_size;
	/* fm_u16 tmp; */

	WCN_DBG(FM_DBG | CHIP, "ramp down\n");

	/* switch SPI clock to 26MHz */
	ret = mt6631_host_read(0x81026004, &tem);   /* Set 0x81026004[0] = 0x0 */
	tem = tem & 0xFFFFFFFE;
	ret = mt6631_host_write(0x81026004, tem);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "RampDown Switch SPI clock to 26MHz failed\n");
		return ret;
	}
	WCN_DBG(FM_DBG | CHIP, "RampDown Switch SPI clock to 26MHz\n");

	/* unlock 64M */
	ret = mt6631_host_read(0x80026000, &tem);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: unlock 64M reg 0x80026000 failed\n", __func__);
	ret = mt6631_host_write(0x80026000, tem & (~(0x1 << 28)));
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: unlock 64M failed\n", __func__);

	/* Rlease TOP2/64M sleep */
	ret = mt6631_host_read(0x81021138, &tem);   /* Set 0x81021138[7] = 0x0 */
	tem = tem & 0xFFFFFF7F;
	ret = mt6631_host_write(0x81021138, tem);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "RampDown Rlease TOP2/64M sleep failed\n");
		return ret;
	}
	WCN_DBG(FM_DBG | CHIP, "RampDown Rlease TOP2/64M sleep\n");

	/* A0.0 Host control RF register */
	ret = mt6631_set_bits(0x60, 0x0007, 0xFFF0);  /*Set 0x60 [D3:D0] = 0x7*/
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down HOST control rf: Set 0x60 [D3:D0] = 0x7 failed\n");
		return ret;
	}

	/* A0.1 Update FM ADPLL fast tracking mode gain */
	ret = mt6631_set_bits(0x0F, 0x0000, 0xF800);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down ADPLL gainA/B: Set 0xFH [D10:D0] = 0x000 failed\n");
		return ret;
	}

	/* A0.2 Host control RF register */
	ret = mt6631_set_bits(0x60, 0x000F, 0xFFF0);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down Host control RF registerwr top 0x60 failed\n");
		return ret;
	}
	/*Clear dsp state*/
	ret = mt6631_set_bits(0x63, 0x0000, 0xFFF0);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down Host control RF registerwr top 0x63 failed\n");
		return ret;
	}
	/* Set DSP ramp down state*/
	ret = mt6631_set_bits(0x63, 0x0010, 0xFFEF);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down Host control RF registerwr top 0x63 failed\n");
		return ret;
	}

	ret = mt6631_write(FM_MAIN_INTRMASK, 0x0000);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "ramp down clean FM_MAIN_INTRMASK failed\n");

	ret = mt6631_write(FM_MAIN_EXTINTRMASK, 0x0000);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down clean FM_MAIN_EXTINTRMASK failed\n");
		return ret;
	}

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_rampdown(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_RAMPDOWN, SW_RETRY_CNT, RAMPDOWN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down failed\n");
		return ret;
	}

	ret = mt6631_write(FM_MAIN_EXTINTRMASK, 0x0021);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down wr FM_MAIN_EXTINTRMASK failed\n");
		return ret;
	}

	ret = mt6631_write(FM_MAIN_INTRMASK, 0x0021);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "ramp down wr FM_MAIN_INTRMASK failed\n");

#if 0
	Delayms(1);
	WCN_DBG(FM_DBG | CHIP, "ramp down delay 1ms\n");

	/* A1.1. Disable aon_osc_clk_cg */
	ret = mt6631_host_write(0x81024064, 0x00000004);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " Disable aon_osc_clk_cg failed\n");
		return ret;
	}
	/* A1.1. Disable FMAUD trigger */
	ret = mt6631_host_write(0x81024058, 0x88800000);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "Disable FMAUD trigger failed\n");
		return ret;
	}

	/* A1.1. issue fmsys memory powr down */
	ret = mt6631_host_write(0x81024054, 0x00000180);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " Issue fmsys memory powr down failed\n");
		return ret;
	}
#endif

	return ret;
}

static fm_s32 mt6631_get_rom_version(void)
{
	fm_u16 flag_Romcode;
	fm_u16 nRomVersion;
#define ROM_CODE_READY 0x0001
	fm_s32 ret;

	/* A1.1 DSP rom code version request enable --- set 0x61 b15=1 */
	mt6631_set_bits(0x61, 0x8000, 0x7FFF);

	/* A1.2 Release ASIP reset --- set 0x61 b1=1 */
	mt6631_set_bits(0x61, 0x0002, 0xFFFD);

	/* A1.3 Enable ASIP power --- set 0x61 b0=0 */
	mt6631_set_bits(0x61, 0x0000, 0xFFFE);

	/* A1.4 Wait until DSP code version ready --- wait loop 1ms */
	do {
		Delayus(1000);
		ret = mt6631_read(0x84, &flag_Romcode);
		/* ret=-4 means signal got when control FM. usually get sig 9 to kill FM process. */
		/* now cancel FM power up sequence is recommended. */
		if (ret)
			return ret;

		WCN_DBG(FM_DBG | CHIP, "ROM_CODE_READY flag 0x84=%x\n", flag_Romcode);
	} while (flag_Romcode != ROM_CODE_READY);


	/* A1.5 Read FM DSP code version --- rd 0x83[15:8] */
	mt6631_read(0x83, &nRomVersion);
	nRomVersion = (nRomVersion >> 8);

	/* A1.6 DSP rom code version request disable --- set 0x61 b15=0 */
	mt6631_set_bits(0x61, 0x0000, 0x7FFF);

	/* A1.7 Reset ASIP --- set 0x61[1:0] = 1 */
	mt6631_set_bits(0x61, 0x0001, 0xFFFC);

	return (fm_s32) nRomVersion;
}

static fm_s32 mt6631_get_patch_path(fm_s32 ver, const fm_s8 **ppath)
{
	fm_s32 i;
	fm_s32 max = sizeof(mt6631_patch_tbl) / sizeof(mt6631_patch_tbl[0]);

	/* check if the ROM version is defined or not */
	for (i = 0; i < max; i++) {
		if ((mt6631_patch_tbl[i].idx == ver)
			&& (fm_file_exist(mt6631_patch_tbl[i].patch) == 0)) {
			*ppath = mt6631_patch_tbl[i].patch;
			return 0;
		}
	}

	/* the ROM version isn't defined, find a latest patch instead */
	for (i = max; i > 0; i--) {
		if (fm_file_exist(mt6631_patch_tbl[i - 1].patch) == 0) {
			*ppath = mt6631_patch_tbl[i - 1].patch;
			WCN_DBG(FM_WAR | CHIP, "undefined ROM version\n");
			return 1;
		}
	}

	/* get path failed */
	WCN_DBG(FM_ERR | CHIP, "No valid patch file\n");
	return -FM_EPATCH;
}

static fm_s32 mt6631_get_coeff_path(fm_s32 ver, const fm_s8 **ppath)
{
	fm_s32 i;
	fm_s32 max = sizeof(mt6631_patch_tbl) / sizeof(mt6631_patch_tbl[0]);

	/* check if the ROM version is defined or not */
	for (i = 0; i < max; i++) {
		if ((mt6631_patch_tbl[i].idx == ver)
			&& (fm_file_exist(mt6631_patch_tbl[i].coeff) == 0)) {
			*ppath = mt6631_patch_tbl[i].coeff;
			return 0;
		}
	}

	/* the ROM version isn't defined, find a latest patch instead */
	for (i = max; i > 0; i--) {
		if (fm_file_exist(mt6631_patch_tbl[i - 1].coeff) == 0) {
			*ppath = mt6631_patch_tbl[i - 1].coeff;
			WCN_DBG(FM_WAR | CHIP, "undefined ROM version\n");
			return 1;
		}
	}

	/* get path failed */
	WCN_DBG(FM_ERR | CHIP, "No valid coeff file\n");
	return -FM_EPATCH;
}

/*
*  mt6631_DspPatch - DSP download procedure
*  @img - source dsp bin code
*  @len - patch length in byte
*  @type - rom/patch/coefficient/hw_coefficient
*/
static fm_s32 mt6631_DspPatch(const fm_u8 *img, fm_s32 len, enum IMG_TYPE type)
{
	fm_u8 seg_num;
	fm_u8 seg_id = 0;
	fm_s32 seg_len;
	fm_s32 ret = 0;
	fm_u16 pkt_size;

	if (img == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	if (len <= 0)
		return -1;

	seg_num = len / PATCH_SEG_LEN + 1;
	WCN_DBG(FM_DBG | CHIP, "binary len:%d, seg num:%d\n", len, seg_num);

	switch (type) {
	case IMG_PATCH:

		for (seg_id = 0; seg_id < seg_num; seg_id++) {
			seg_len = ((seg_id + 1) < seg_num) ? PATCH_SEG_LEN : (len % PATCH_SEG_LEN);
			WCN_DBG(FM_DBG | CHIP, "patch, [seg_id:%d], [seg_len:%d]\n", seg_id, seg_len);
			if (FM_LOCK(cmd_buf_lock))
				return -FM_ELOCK;
			pkt_size =
				mt6631_patch_download(cmd_buf, TX_BUF_SIZE, seg_num, seg_id,
						&img[seg_id * PATCH_SEG_LEN], seg_len);
			WCN_DBG(FM_DBG | CHIP, "pkt_size:%d\n", (fm_s32) pkt_size);
			ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_PATCH, SW_RETRY_CNT, PATCH_TIMEOUT, NULL);
			FM_UNLOCK(cmd_buf_lock);

			if (ret) {
				WCN_DBG(FM_ALT | CHIP, "mt6631_patch_download failed\n");
				return ret;
			}
		}

		break;

	case IMG_COEFFICIENT:

		for (seg_id = 0; seg_id < seg_num; seg_id++) {
			seg_len = ((seg_id + 1) < seg_num) ? PATCH_SEG_LEN : (len % PATCH_SEG_LEN);
			WCN_DBG(FM_DBG | CHIP, "coeff, [seg_id:%d], [seg_len:%d]\n", seg_id, seg_len);
			if (FM_LOCK(cmd_buf_lock))
				return -FM_ELOCK;
			pkt_size =
				mt6631_coeff_download(cmd_buf, TX_BUF_SIZE, seg_num, seg_id,
						&img[seg_id * PATCH_SEG_LEN], seg_len);
			WCN_DBG(FM_DBG | CHIP, "pkt_size:%d\n", (fm_s32) pkt_size);
			ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_COEFF, SW_RETRY_CNT, COEFF_TIMEOUT, NULL);
			FM_UNLOCK(cmd_buf_lock);

			if (ret) {
				WCN_DBG(FM_ALT | CHIP, "mt6631_coeff_download failed\n");
				return ret;
			}
		}

		break;
	default:
		break;
	}

	return 0;
}

static fm_s32 mt6631_PowerUp(fm_u16 *chip_id, fm_u16 *device_id)
{
#define PATCH_BUF_SIZE (4096*6)
	fm_s32 ret = 0;
	fm_u16 pkt_size;
	fm_u16 tmp_reg = 0;
	fm_u32 tem;
	fm_u32 host_reg = 0;

	const fm_s8 *path_patch = NULL;
	const fm_s8 *path_coeff = NULL;
	fm_s32 patch_len = 0;
	fm_u8 *dsp_buf = NULL;

	if (chip_id == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (device_id == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	WCN_DBG(FM_DBG | CHIP, "pwr on seq......\n");

	/* Wholechip FM Power Up: step 1, set common SPI parameter */
	ret = mt6631_host_write(0x8102600C, 0x0000800F);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " pwrup set CSPI failed\n");
		return ret;
	}
	/* Set top_clk_en_adie to trigger sleep controller before FM power on */
	mt6631_host_read(0x81024030, &tem);   /* Set 0x81024030[1] = 0x1 */
	tem = tem | 0x00000002;
	mt6631_host_write(0x81024030, tem);

	/* Disable 26M crystal sleep */
	mt6631_host_read(0x81021234, &tem);   /* Set 0x81021234[7] = 0x1 */
	tem = tem | 0x00000080;
	mt6631_host_write(0x81021234, tem);

	/* turn on RG_TOP_BGLDO */
	ret = mt6631_top_read(0x00c0, &host_reg);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "power up read top 0xc0 failed\n");
		return ret;
	}
	ret = mt6631_top_write(0x00c0, host_reg | (0x3 << 27));
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "power up write top 0xc0 failed\n");
		return ret;
	}

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;

	pkt_size = mt6631_pwrup_clock_on(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
			WCN_DBG(FM_ALT | CHIP, "mt6631_pwrup_clock_on failed\n");
			return ret;
	}

	/* Wholechip FM Power Up: step 2, read HW version */
	mt6631_read(0x62, &tmp_reg);
	/* chip_id = tmp_reg; */
	if (tmp_reg == 0x6631)
		*chip_id = 0x6631;
	*device_id = tmp_reg;
	mt6631_hw_info.chip_id = (fm_s32) tmp_reg;
	WCN_DBG(FM_DBG | CHIP, "chip_id:0x%04x\n", tmp_reg);

	if ((mt6631_hw_info.chip_id != 0x6631)) {
		WCN_DBG(FM_NTC | CHIP, "fm sys error, reset hw\n");
		return -FM_EFW;
	}

	mt6631_hw_info.eco_ver = (fm_s32) mtk_wcn_wmt_hwver_get();
	WCN_DBG(FM_DBG | CHIP, "ECO version:0x%08x\n", mt6631_hw_info.eco_ver);
	mt6631_hw_info.eco_ver += 1;


	/*  Wholechip FM Power Up: step 3, get mt6631 DSP ROM version */
	ret = mt6631_get_rom_version();
	if (ret >= 0) {
		mt6631_hw_info.rom_ver = ret;
		WCN_DBG(FM_DBG | CHIP, "ROM version: v%d\n", mt6631_hw_info.rom_ver);
	} else {
		WCN_DBG(FM_ERR | CHIP, "get ROM version failed\n");
		if (ret == -4)
			WCN_DBG(FM_ERR | CHIP, "signal got when control FM, usually get sig 9 to kill FM process.\n");
			/* now cancel FM power up sequence is recommended. */
		return ret;
	}

	/* Wholechip FM Power Up: step 4 download patch */
	dsp_buf = fm_vmalloc(PATCH_BUF_SIZE);
	if (!dsp_buf) {
		WCN_DBG(FM_ALT | CHIP, "-ENOMEM\n");
		return -ENOMEM;
	}

	ret = mt6631_get_patch_path(mt6631_hw_info.rom_ver, &path_patch);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " mt6631_get_patch_path failed\n");
		return ret;
	}
	patch_len = fm_file_read(path_patch, dsp_buf, PATCH_BUF_SIZE, 0);
	ret = mt6631_DspPatch((const fm_u8 *)dsp_buf, patch_len, IMG_PATCH);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " DL DSPpatch failed\n");
		return ret;
	}

	ret = mt6631_get_coeff_path(mt6631_hw_info.rom_ver, &path_coeff);
	patch_len = fm_file_read(path_coeff, dsp_buf, PATCH_BUF_SIZE, 0);

	mt6631_hw_info.rom_ver += 1;

	tmp_reg = dsp_buf[38] | (dsp_buf[39] << 8);	/* to be confirmed */
	mt6631_hw_info.patch_ver = (fm_s32) tmp_reg;
	WCN_DBG(FM_NTC | CHIP, "Patch version: 0x%08x\n", mt6631_hw_info.patch_ver);

	if (ret == 1) {
		dsp_buf[4] = 0x00;	/* if we found rom version undefined, we should disable patch */
		dsp_buf[5] = 0x00;
	}

	ret = mt6631_DspPatch((const fm_u8 *)dsp_buf, patch_len, IMG_COEFFICIENT);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " Download DSP coefficient failed\n");
		return ret;
	}

	/* Download HWACC coefficient */
	mt6631_write(0x92, 0x0000);
	mt6631_write(0x90, 0x0040); /* Reset download control  */
	mt6631_write(0x90, 0x0000); /* Disable memory control from host*/

	if (dsp_buf) {
		fm_vfree(dsp_buf);
		dsp_buf = NULL;
	}

	/* Wholechip FM Power Up: step 5, FM Digital Init: fm_rgf_maincon */
	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_pwrup_digital_init(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "mt6631_pwrup_digital_init failed\n");
		return ret;
	}

	/* Wholechip FM Power Up: step 6, FM RF fine tune setting */
	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_pwrup_fine_tune(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "mt6631_pwrup_fine_tune failed\n");
		return ret;
	}

	/* Enable connsys FM 2 wire RX */
	mt6631_write(0x9B, 0xF9AB);				/* G2: Set audio output i2s TX mode */
	mt6631_host_write(0x81024064, 0x00000014); /* G3: Enable aon_osc_clk_cg */
	mt6631_host_write(0x81024058, 0x888100C3); /* G4: Enable FMAUD trigger */
	mt6631_host_write(0x81024054, 0x00000100); /* G5: Release fmsys memory power down*/

	WCN_DBG(FM_NTC | CHIP, "pwr on seq ok\n");

	return ret;
}

static fm_s32 mt6631_PowerDown(void)
{
	fm_s32 ret = 0;
	fm_u32 tem;
	fm_u16 pkt_size;
	fm_u32 host_reg = 0;

	WCN_DBG(FM_DBG | CHIP, "pwr down seq\n");

	/* A0.1. Disable aon_osc_clk_cg */
	ret = mt6631_host_write(0x81024064, 0x00000004);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " Disable aon_osc_clk_cg failed\n");
		return ret;
	}
	/* A0.1. Disable FMAUD trigger */
	ret = mt6631_host_write(0x81024058, 0x88800000);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " Disable FMAUD trigger failed\n");
		return ret;
	}

	/* A0.1. issue fmsys memory powr down */
	ret = mt6631_host_write(0x81024054, 0x00000180);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " Issue fmsys memory powr down failed\n");
		return ret;
	}

	/*switch SPI clock to 26M*/
	WCN_DBG(FM_DBG | CHIP, "PowerDown: switch SPI clock to 26M\n");
	ret = mt6631_host_read(0x81026004, &tem);
	tem = tem & 0xFFFFFFFE;
	ret = mt6631_host_write(0x81026004, tem);
	if (ret)
		WCN_DBG(FM_ALT | CHIP, "PowerDown: switch SPI clock to 26M failed\n");

	/* unlock 64M */
	ret = mt6631_host_read(0x80026000, &tem);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: unlock 64M reg 0x80026000 failed\n", __func__);
	ret = mt6631_host_write(0x80026000, tem & (~(0x1 << 28)));
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: unlock 64M failed\n", __func__);

	/*Release TOP2/64M sleep*/
	WCN_DBG(FM_DBG | CHIP, "PowerDown: Release TOP2/64M sleep\n");
	ret = mt6631_host_read(0x81021138, &tem);
	tem = tem & 0xFFFFFF7F;
	ret = mt6631_host_write(0x81021138, tem);
	if (ret)
		WCN_DBG(FM_ALT | CHIP, "PowerDown: Release TOP2/64M sleep failed\n");

	/* Enable 26M crystal sleep */
	WCN_DBG(FM_DBG | CHIP, "PowerDown: Enable 26M crystal sleep,Set 0x81021234[7] = 0x0\n");
	ret = mt6631_host_read(0x81021234, &tem);   /* Set 0x81021234[7] = 0x0 */
	tem = tem & 0xFFFFFF7F;
	ret = mt6631_host_write(0x81021234, tem);
	if (ret)
		WCN_DBG(FM_ALT | CHIP, "PowerDown: Enable 26M crystal sleep,Set 0x81021234[7] = 0x0 failed\n");

	/* A0:set audio output I2X Rx mode: */
	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_pwrdown(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "mt6631_pwrdown failed\n");
		return ret;
	}
	/* FIX_ME, disable ext interrupt */
	/* mt6631_write(FM_MAIN_EXTINTRMASK, 0x00); */


   /* D0.  Clear top_clk_en_adie to indicate sleep controller after FM power off */
	ret = mt6631_host_read(0x80101030, &host_reg);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " pwroff read 0x80100030 failed\n");
		return ret;
	}
	ret = mt6631_host_write(0x80101030, host_reg & (~(0x1 << 1)));
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " pwroff disable top_ck_en_adie failed\n");
		return ret;
	}
	return ret;
}

/* just for dgb */
#if 0
static void mt6631_bt_write(fm_u32 addr, fm_u32 val)
{
	fm_u32 tem, i = 0;

	mt6631_host_write(0x80103020, addr);
	mt6631_host_write(0x80103024, val);
	mt6631_host_read(0x80103000, &tem);
	while ((tem == 4) && (i < 1000)) {
		i++;
		mt6631_host_read(0x80103000, &tem);
	}
}
#endif
static fm_bool mt6631_SetFreq(fm_u16 freq)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;
	fm_u16 chan_para = 0;
	fm_u16 freq_reg = 0;
	fm_u32 reg_val = 0;
	fm_u32 i = 0;
	fm_bool flag_spi_hopping = fm_false;
	fm_u16 tmp_reg[6] = {0};

	fm_cb_op->cur_freq_set(freq);

#if 0
	/* MCU clock adjust if need */
	ret = mt6631_mcu_dese(freq, NULL);
	if (ret < 0)
		WCN_DBG(FM_ERR | MAIN, "mt6631_mcu_dese FAIL:%d\n", ret);

	WCN_DBG(FM_INF | MAIN, "MCU %d\n", ret);

	/* GPS clock adjust if need */
	ret = mt6631_gps_dese(freq, NULL);
	if (ret < 0)
		WCN_DBG(FM_ERR | MAIN, "mt6631_gps_dese FAIL:%d\n", ret);

	WCN_DBG(FM_INF | MAIN, "GPS %d\n", ret);
#endif

	/* A0. Host contrl RF register */
	ret = mt6631_set_bits(0x60, 0x0007, 0xFFF0);  /* Set 0x60 [D3:D0] = 0x07*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Host Control RF register 0x60 = 0x7 failed\n", __func__);

	/* A0.1 Update FM ADPLL fast tracking mode gain */
	ret = mt6631_set_bits(0x0F, 0x0455, 0xF800);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set FM ADPLL gainA/B=0x455 failed\n", __func__);

	/* A0.2 Set FMSYS cell mode */
	if (mt6631_TDD_chan_check(freq)) {
		ret = mt6631_set_bits(0x30, 0x0008, 0xFFF3);	/* use TDD solution */
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: freq[%d]: use TDD solution failed\n", __func__, freq);
	} else {
		ret = mt6631_set_bits(0x30, 0x0000, 0xFFF3);	/* default use FDD solution */
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: freq[%d]: default use FDD solution failed\n", __func__, freq);
	}

	/* A0.3 Host control RF register */
	ret = mt6631_set_bits(0x60, 0x000F, 0xFFF0);	/* Set 0x60 [D3:D0] = 0x0F*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set 0x60 [D3:D0] = 0x0F failed\n", __func__);

	/* A1 Get Channel parameter from map list*/

	chan_para = mt6631_chan_para_get(freq);
	WCN_DBG(FM_DBG | CHIP, "%s: %d chan para = %d\n", __func__, (fm_s32) freq, (fm_s32) chan_para);

	freq_reg = freq;
	if (0 == fm_get_channel_space(freq_reg))
		freq_reg *= 10;

	freq_reg = (freq_reg - 6400) * 2 / 10;

	/*A1 Set rgfrf_chan = XXX*/
	ret = mt6631_set_bits(0x65, freq_reg, 0xFC00);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "%s: set rgfrf_chan = xxx = %d failed\n", __func__, freq_reg);
		return fm_false;
	}

	ret = mt6631_set_bits(0x65, (chan_para << 12), 0x0FFF);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "set freq wr 0x65 failed\n");
		return fm_false;
	}
	/* SPI hoppint setting*/
	if (mt6631_SPI_hopping_check(freq)) {

		WCN_DBG(FM_NTC | CHIP, "%s: freq:%d is SPI hopping channel,turn on 64M PLL\n", __func__, (fm_s32) freq);
		/*Disable TOP2/64M sleep*/
		ret = mt6631_host_read(0x81021138, &reg_val);
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: read 64M reg 0x81021138 failed\n", __func__);
		reg_val |= 0x00000080;
		ret = mt6631_host_write(0x81021138, reg_val);
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: disable 64M sleep failed\n", __func__);

		/* lock 64M */
		ret = mt6631_host_read(0x80026000, &reg_val);
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: lock 64M reg 0x80026000 failed\n", __func__);
		ret = mt6631_host_write(0x80026000, reg_val | (0x1 << 28));
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: lock 64M failed\n", __func__);

		for (i = 0; i < 100; i++) { /*rd 0x8002110C until D27 ==1*/

			ret = mt6631_host_read(0x8002110C, &reg_val);

			if (reg_val & 0x08000000) {
				flag_spi_hopping = fm_true;
				WCN_DBG(FM_NTC | CHIP, "%s: POLLING PLL_RDY success !\n", __func__);
				/* switch SPI clock to 64MHz */
				ret = mt6631_host_read(0x81026004, &reg_val); /* wr 0x81026004[0] 0x1	D0 */
				reg_val |= 0x00000001;
				ret = mt6631_host_write(0x81026004, reg_val);
				break;
			}
			Delayus(10);
		}
		if (fm_false == flag_spi_hopping)
			WCN_DBG(FM_ERR | CHIP, "%s: Polling to read rd 0x8002110C[27] ==0x1 failed !\n", __func__);
	}

	/* A0. Host contrl RF register */
	ret = mt6631_set_bits(0x60, 0x0007, 0xFFF0);  /* Set 0x60 [D3:D0] = 0x07*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Host Control RF register 0x60 = 0x7 failed\n", __func__);


	mt6631_read(0x62, &tmp_reg[0]);
	mt6631_read(0x64, &tmp_reg[1]);
	mt6631_read(0x69, &tmp_reg[2]);
	mt6631_read(0x6a, &tmp_reg[3]);
	mt6631_read(0x6b, &tmp_reg[4]);
	mt6631_read(0x9b, &tmp_reg[5]);

	WCN_DBG(FM_ALT | CHIP, "%s: Before tune--0x62 0x64 0x69 0x6a 0x6b 0x9b = %04x %04x %04x %04x %04x %04x\n",
			__func__,
			tmp_reg[0],
			tmp_reg[1],
			tmp_reg[2],
			tmp_reg[3],
			tmp_reg[4],
			tmp_reg[5]);

	/* A0.3 Host control RF register */
	ret = mt6631_set_bits(0x60, 0x000F, 0xFF00);	/* Set 0x60 [D3:D0] = 0x0F*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set 0x60 [D3:D0] = 0x0F failed\n", __func__);

	if (FM_LOCK(cmd_buf_lock))
		return fm_false;
	pkt_size = mt6631_tune(cmd_buf, TX_BUF_SIZE, freq, chan_para);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_TUNE | FLAG_TUNE_DONE, SW_RETRY_CNT, TUNE_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);


	/* A0. Host contrl RF register */
	ret = mt6631_set_bits(0x60, 0x0007, 0xFFF0);  /* Set 0x60 [D3:D0] = 0x07*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Host Control RF register 0x60 = 0x7 failed\n", __func__);

	memset(tmp_reg, 0, sizeof(tmp_reg[0])*6);

	mt6631_read(0x62, &tmp_reg[0]);
	mt6631_read(0x64, &tmp_reg[1]);
	mt6631_read(0x69, &tmp_reg[2]);
	mt6631_read(0x6a, &tmp_reg[3]);
	mt6631_read(0x6b, &tmp_reg[4]);
	mt6631_read(0x9b, &tmp_reg[5]);

	WCN_DBG(FM_ALT | CHIP, "%s: After tune--0x62 0x64 0x69 0x6a 0x6b 0x9b = %04x %04x %04x %04x %04x %04x\n",
			__func__,
			tmp_reg[0],
			tmp_reg[1],
			tmp_reg[2],
			tmp_reg[3],
			tmp_reg[4],
			tmp_reg[5]);

	/* A0.3 Host control RF register */
	ret = mt6631_set_bits(0x60, 0x000F, 0xFF00);	/* Set 0x60 [D3:D0] = 0x0F*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set 0x60 [D3:D0] = 0x0F failed\n", __func__);

	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "%s: mt6631_tune failed\n", __func__);
		return fm_false;
	}

	WCN_DBG(FM_DBG | CHIP, "%s: set freq to %d ok\n", __func__, freq);
#if 0
	/* ADPLL setting for dbg */
	mt6631_top_write(0x0050, 0x00000007);
	mt6631_top_write(0x0A08, 0xFFFFFFFF);
	mt6631_bt_write(0x82, 0x11);
	mt6631_bt_write(0x83, 0x11);
	mt6631_bt_write(0x84, 0x11);
	mt6631_top_write(0x0040, 0x1C1C1C1C);
	mt6631_top_write(0x0044, 0x1C1C1C1C);
	mt6631_write(0x70, 0x0010);
	/*0x0806 DCO clk
	0x0802 ref clk
	0x0804 feedback clk
	*/
	mt6631_write(0xE0, 0x0806);
#endif
	return fm_true;
}

#if 0
/*
* mt6631_Seek
* @pFreq - IN/OUT parm, IN start freq/OUT seek valid freq
* @seekdir - 0:up, 1:down
* @space - 1:50KHz, 2:100KHz, 4:200KHz
* return fm_true:seek success; fm_false:seek failed
*/
static fm_bool mt6631_Seek(fm_u16 min_freq, fm_u16 max_freq, fm_u16 *pFreq, fm_u16 seekdir, fm_u16 space)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size, temp;

	mt6631_RampDown();
	mt6631_read(FM_MAIN_CTRL, &temp);
	mt6631_Mute(fm_true);

	if (FM_LOCK(cmd_buf_lock))
		return fm_false;
	pkt_size = mt6631_seek(cmd_buf, TX_BUF_SIZE, seekdir, space, max_freq, min_freq);
	ret =
		fm_cmd_tx(cmd_buf, pkt_size, FLAG_SEEK | FLAG_SEEK_DONE, SW_RETRY_CNT, SEEK_TIMEOUT,
			mt6631_get_read_result);
	FM_UNLOCK(cmd_buf_lock);

	if (!ret && mt6631_res) {
		*pFreq = mt6631_res->seek_result;
		/* fm_cb_op->cur_freq_set(*pFreq); */
	} else {
		WCN_DBG(FM_ALT | CHIP, "mt6631_seek failed\n");
		return ret;
	}

	/* get the result freq */
	WCN_DBG(FM_NTC | CHIP, "seek, result freq:%d\n", *pFreq);
	mt6631_RampDown();
	if ((temp & 0x0020) == 0)
		mt6631_Mute(fm_false);

	return fm_true;
}
#endif
#define FM_CQI_LOG_PATH "/mnt/sdcard/fmcqilog"

static fm_s32 mt6631_full_cqi_get(fm_s32 min_freq, fm_s32 max_freq, fm_s32 space, fm_s32 cnt)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;
	fm_u16 freq, orig_freq;
	fm_s32 i, j, k;
	fm_s32 space_val, max, min, num;
	struct mt6631_full_cqi *p_cqi;
	fm_u8 *cqi_log_title = "Freq, RSSI, PAMD, PR, FPAMD, MR, ATDC, PRX, ATDEV, SMGain, DltaRSSI\n";
	fm_u8 cqi_log_buf[100] = { 0 };
	fm_s32 pos;
	fm_u8 cqi_log_path[100] = { 0 };

	/* for soft-mute tune, and get cqi */
	freq = fm_cb_op->cur_freq_get();
	if (0 == fm_get_channel_space(freq))
		freq *= 10;

	/* get cqi */
	orig_freq = freq;
	if (0 == fm_get_channel_space(min_freq))
		min = min_freq * 10;
	else
		min = min_freq;

	if (0 == fm_get_channel_space(max_freq))
		max = max_freq * 10;
	else
		max = max_freq;

	if (space == 0x0001)
		space_val = 5;	/* 50Khz */
	else if (space == 0x0002)
		space_val = 10;	/* 100Khz */
	else if (space == 0x0004)
		space_val = 20;	/* 200Khz */
	else
		space_val = 10;

	num = (max - min) / space_val + 1;	/* Eg, (8760 - 8750) / 10 + 1 = 2 */
	for (k = 0; (10000 == orig_freq) && (0xffffffff == g_dbg_level) && (k < cnt); k++) {
		WCN_DBG(FM_NTC | CHIP, "cqi file:%d\n", k + 1);
		freq = min;
		pos = 0;
		fm_memcpy(cqi_log_path, FM_CQI_LOG_PATH, strlen(FM_CQI_LOG_PATH));
		sprintf(&cqi_log_path[strlen(FM_CQI_LOG_PATH)], "%d.txt", k + 1);
		fm_file_write(cqi_log_path, cqi_log_title, strlen(cqi_log_title), &pos);
		for (j = 0; j < num; j++) {
			if (FM_LOCK(cmd_buf_lock))
				return -FM_ELOCK;
			pkt_size = mt6631_full_cqi_req(cmd_buf, TX_BUF_SIZE, &freq, 1, 1);
			ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_SM_TUNE, SW_RETRY_CNT,
					SM_TUNE_TIMEOUT, mt6631_get_read_result);
			FM_UNLOCK(cmd_buf_lock);

			if (!ret && mt6631_res) {
				WCN_DBG(FM_NTC | CHIP, "smt cqi size %d\n", mt6631_res->cqi[0]);
				p_cqi = (struct mt6631_full_cqi *)&mt6631_res->cqi[2];
				for (i = 0; i < mt6631_res->cqi[1]; i++) {
					/* just for debug */
					WCN_DBG(FM_NTC | CHIP,
						"freq %d, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n",
						p_cqi[i].ch, p_cqi[i].rssi, p_cqi[i].pamd,
						p_cqi[i].pr, p_cqi[i].fpamd, p_cqi[i].mr,
						p_cqi[i].atdc, p_cqi[i].prx, p_cqi[i].atdev,
						p_cqi[i].smg, p_cqi[i].drssi);
					/* format to buffer */
					sprintf(cqi_log_buf,
						"%04d, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x,\n",
						p_cqi[i].ch, p_cqi[i].rssi, p_cqi[i].pamd,
						p_cqi[i].pr, p_cqi[i].fpamd, p_cqi[i].mr,
						p_cqi[i].atdc, p_cqi[i].prx, p_cqi[i].atdev,
						p_cqi[i].smg, p_cqi[i].drssi);
					/* write back to log file */
					fm_file_write(cqi_log_path, cqi_log_buf, strlen(cqi_log_buf), &pos);
				}
			} else {
				WCN_DBG(FM_ALT | CHIP, "smt get CQI failed\n");
				ret = -1;
			}
			freq += space_val;
		}
		fm_cb_op->cur_freq_set(0);	/* avoid run too much times */
	}

	return ret;
}

/*
 * mt6631_GetCurRSSI - get current freq's RSSI value
 * RS=RSSI
 * If RS>511, then RSSI(dBm)= (RS-1024)/16*6
 *				else RSSI(dBm)= RS/16*6
 */
static fm_s32 mt6631_GetCurRSSI(fm_s32 *pRSSI)
{
	fm_u16 tmp_reg;

	mt6631_read(FM_RSSI_IND, &tmp_reg);
	tmp_reg = tmp_reg & 0x03ff;

	if (pRSSI) {
		*pRSSI = (tmp_reg > 511) ? (((tmp_reg - 1024) * 6) >> 4) : ((tmp_reg * 6) >> 4);
		WCN_DBG(FM_DBG | CHIP, "rssi:%d, dBm:%d\n", tmp_reg, *pRSSI);
	} else {
		WCN_DBG(FM_ERR | CHIP, "get rssi para error\n");
		return -FM_EPARA;
	}

	return 0;
}

static fm_u16 mt6631_vol_tbl[16] = { 0x0000, 0x0519, 0x066A, 0x0814,
	0x0A2B, 0x0CCD, 0x101D, 0x1449,
	0x198A, 0x2027, 0x287A, 0x32F5,
	0x4027, 0x50C3, 0x65AD, 0x7FFF
};

static fm_s32 mt6631_SetVol(fm_u8 vol)
{
	fm_s32 ret = 0;

	vol = (vol > 15) ? 15 : vol;
	ret = mt6631_write(0x7D, mt6631_vol_tbl[vol]);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "Set vol=%d Failed\n", vol);
		return ret;
	}
	WCN_DBG(FM_DBG | CHIP, "Set vol=%d OK\n", vol);


	if (vol == 10) {
		fm_print_cmd_fifo();	/* just for debug */
		fm_print_evt_fifo();
	}
	return 0;
}

static fm_s32 mt6631_GetVol(fm_u8 *pVol)
{
	int ret = 0;
	fm_u16 tmp;
	fm_s32 i;

	if (pVol == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	ret = mt6631_read(0x7D, &tmp);
	if (ret) {
		*pVol = 0;
		WCN_DBG(FM_ERR | CHIP, "Get vol Failed\n");
		return ret;
	}

	for (i = 0; i < 16; i++) {
		if (mt6631_vol_tbl[i] == tmp) {
			*pVol = i;
			break;
		}
	}

	WCN_DBG(FM_DBG | CHIP, "Get vol=%d OK\n", *pVol);
	return 0;
}

static fm_s32 mt6631_dump_reg(void)
{
	fm_s32 i;
	fm_u16 TmpReg;

	for (i = 0; i < 0xff; i++) {
		mt6631_read(i, &TmpReg);
		WCN_DBG(FM_NTC | CHIP, "0x%02x=0x%04x\n", i, TmpReg);
	}
	return 0;
}

/*0:mono, 1:stereo*/
static fm_bool mt6631_GetMonoStereo(fm_u16 *pMonoStereo)
{
#define FM_BF_STEREO 0x1000
	fm_u16 TmpReg;

	if (pMonoStereo) {
		mt6631_read(FM_RSSI_IND, &TmpReg);
		*pMonoStereo = (TmpReg & FM_BF_STEREO) >> 12;
	} else {
		WCN_DBG(FM_ERR | CHIP, "MonoStero: para err\n");
		return fm_false;
	}

	FM_LOG_NTC(CHIP, "Get MonoStero:0x%04x\n", *pMonoStereo);
	return fm_true;
}

static fm_s32 mt6631_SetMonoStereo(fm_s32 MonoStereo)
{
	fm_s32 ret = 0;

	FM_LOG_NTC(CHIP, "set to %s\n", MonoStereo ? "mono" : "auto");
	mt6631_top_write(0x50, 0x0007);

	if (MonoStereo)	/*mono */
		ret = mt6631_set_bits(0x75, 0x0008, ~0x0008);
	else
		ret = mt6631_set_bits(0x75, 0x0000, ~0x0008);

	mt6631_top_write(0x50, 0x000F);
	return ret;
}

static fm_s32 mt6631_GetCapArray(fm_s32 *ca)
{
	fm_u16 dataRead;
	fm_u16 tmp = 0;

	if (ca == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	mt6631_read(0x60, &tmp);
	mt6631_write(0x60, tmp & 0xFFF7);	/* 0x60 D3=0 */

	mt6631_read(0x26, &dataRead);
	*ca = dataRead;

	mt6631_write(0x60, tmp);	/* 0x60 D3=1 */
	return 0;
}

/*
 * mt6631_GetCurPamd - get current freq's PAMD value
 * PA=PAMD
 * If PA>511 then PAMD(dB)=  (PA-1024)/16*6,
 *				else PAMD(dB)=PA/16*6
 */
static fm_bool mt6631_GetCurPamd(fm_u16 *pPamdLevl)
{
	fm_u16 tmp_reg;
	fm_u16 dBvalue, valid_cnt = 0;
	int i, total = 0;

	for (i = 0; i < 8; i++) {
		if (mt6631_read(FM_ADDR_PAMD, &tmp_reg)) {
			*pPamdLevl = 0;
			return fm_false;
		}

		tmp_reg &= 0x03FF;
		dBvalue = (tmp_reg > 256) ? ((512 - tmp_reg) * 6 / 16) : 0;
		if (dBvalue != 0) {
			total += dBvalue;
			valid_cnt++;
			WCN_DBG(FM_DBG | CHIP, "[%d]PAMD=%d\n", i, dBvalue);
		}
		Delayms(3);
	}
	if (valid_cnt != 0)
		*pPamdLevl = total / valid_cnt;
	else
		*pPamdLevl = 0;

	WCN_DBG(FM_NTC | CHIP, "PAMD=%d\n", *pPamdLevl);
	return fm_true;
}

static fm_s32 mt6631_i2s_info_get(fm_s32 *ponoff, fm_s32 *pmode, fm_s32 *psample)
{
	if (ponoff == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (pmode == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (psample == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	*ponoff = mt6631_fm_config.aud_cfg.i2s_info.status;
	*pmode = mt6631_fm_config.aud_cfg.i2s_info.mode;
	*psample = mt6631_fm_config.aud_cfg.i2s_info.rate;

	return 0;
}

static fm_s32 mt6631fm_get_audio_info(fm_audio_info_t *data)
{
	memcpy(data, &mt6631_fm_config.aud_cfg, sizeof(fm_audio_info_t));
	return 0;
}

static fm_s32 mt6631_hw_info_get(struct fm_hw_info *req)
{
	if (req == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	req->chip_id = mt6631_hw_info.chip_id;
	req->eco_ver = mt6631_hw_info.eco_ver;
	req->patch_ver = mt6631_hw_info.patch_ver;
	req->rom_ver = mt6631_hw_info.rom_ver;

	return 0;
}

static fm_s32 mt6631_pre_search(void)
{
	mt6631_RampDown();
	/* disable audio output I2S Rx mode */
	mt6631_host_write(0x80101054, 0x00000000);
	/* disable audio output I2S Tx mode */
	mt6631_write(0x9B, 0x0000);
	/*FM_LOG_NTC(FM_NTC | CHIP, "search threshold: RSSI=%d, de-RSSI=%d, smg=%d %d\n",
	mt6631_fm_config.rx_cfg.long_ana_rssi_th, mt6631_fm_config.rx_cfg.desene_rssi_th,
	mt6631_fm_config.rx_cfg.smg_th); */
	return 0;
}

static fm_s32 mt6631_restore_search(void)
{
	mt6631_RampDown();
	/* set audio output I2S Tx mode */
	mt6631_write(0x9B, 0xF9AB);
	/* set audio output I2S Rx mode */
	mt6631_host_write(0x80101054, 0x00003f35);
	return 0;
}

static fm_s32 mt6631_soft_mute_tune(fm_u16 freq, fm_s32 *rssi, fm_bool *valid)
{
	fm_s32 ret = 0;
	fm_s32 i = 0;
	fm_u16 pkt_size;
	fm_u32 reg_val = 0;
	fm_bool flag_spi_hopping = fm_false;
	struct mt6631_full_cqi *p_cqi;
	fm_s32 RSSI = 0, PAMD = 0, MR = 0, ATDC = 0;
	fm_u32 PRX = 0, ATDEV = 0;
	fm_u16 softmuteGainLvl = 0;

	ret = mt6631_chan_para_get(freq);
	if (ret == 2)
		ret = mt6631_set_bits(FM_CHANNEL_SET, 0x2000, 0x0FFF);	/* mdf HiLo */
	else
		ret = mt6631_set_bits(FM_CHANNEL_SET, 0x0000, 0x0FFF);	/* clear FA/HL/ATJ */

	/* SPI hoppint setting*/
	if (mt6631_SPI_hopping_check(freq)) {
		WCN_DBG(FM_NTC | CHIP, "%s: freq:%d is SPI hopping channel,turn on 64M PLL\n", __func__, (fm_s32) freq);
		/*Disable TOP2/64M sleep*/
		ret = mt6631_host_read(0x81021138, &reg_val);
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: read 64M reg 0x81021138 failed\n", __func__);
		reg_val |= 0x00000080;
		ret = mt6631_host_write(0x81021138, reg_val);
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: disable 64M sleep failed\n", __func__);

		/* lock 64M */
		ret = mt6631_host_read(0x80026000, &reg_val);
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: lock 64M reg 0x80026000 failed\n", __func__);
		ret = mt6631_host_write(0x80026000, reg_val | (0x1 << 28));
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: lock 64M failed\n", __func__);

		for (i = 0; i < 100; i++) { /*rd 0x8002110C until D27 ==1*/

			ret = mt6631_host_read(0x8002110C, &reg_val);

			if (reg_val & 0x08000000) {
				flag_spi_hopping = fm_true;
				WCN_DBG(FM_NTC | CHIP, "%s: POLLING PLL_RDY success !\n", __func__);
				/* switch SPI clock to 64MHz */
				ret = mt6631_host_read(0x81026004, &reg_val); /* wr 0x81026004[0] 0x1	D0 */
				reg_val |= 0x00000001;
				ret = mt6631_host_write(0x81026004, reg_val);
				break;
			}
			Delayus(10);
		}
		if (fm_false == flag_spi_hopping)
			WCN_DBG(FM_ERR | CHIP, "%s: Polling to read rd 0x8002110C[27] ==0x1 failed !\n", __func__);
	}

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6631_full_cqi_req(cmd_buf, TX_BUF_SIZE, &freq, 1, 1);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_SM_TUNE, SW_RETRY_CNT, SM_TUNE_TIMEOUT, mt6631_get_read_result);
	FM_UNLOCK(cmd_buf_lock);

	if (!ret && mt6631_res) {
		p_cqi = (struct mt6631_full_cqi *)&mt6631_res->cqi[2];
		/* just for debug */
		WCN_DBG(FM_NTC | CHIP,
			"freq %d, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n",
			p_cqi->ch, p_cqi->rssi, p_cqi->pamd, p_cqi->pr, p_cqi->fpamd, p_cqi->mr,
			p_cqi->atdc, p_cqi->prx, p_cqi->atdev, p_cqi->smg, p_cqi->drssi);
		RSSI = ((p_cqi->rssi & 0x03FF) >= 512) ? ((p_cqi->rssi & 0x03FF) - 1024) : (p_cqi->rssi & 0x03FF);
		PAMD = ((p_cqi->pamd & 0x1FF) >= 256) ? ((p_cqi->pamd & 0x01FF) - 512) : (p_cqi->pamd & 0x01FF);
		MR = ((p_cqi->mr & 0x01FF) >= 256) ? ((p_cqi->mr & 0x01FF) - 512) : (p_cqi->mr & 0x01FF);
		ATDC = (p_cqi->atdc >= 32768) ? (65536 - p_cqi->atdc) : (p_cqi->atdc);
		if (ATDC < 0)
			ATDC = (~(ATDC)) - 1;	/* Get abs value of ATDC */

		PRX = (p_cqi->prx & 0x00FF);
		ATDEV = p_cqi->atdev;
		softmuteGainLvl = p_cqi->smg;
		/* check if the channel is valid according to each CQIs */
		if ((RSSI >= mt6631_fm_config.rx_cfg.long_ana_rssi_th)
			&& (PAMD <= mt6631_fm_config.rx_cfg.pamd_th)
			&& (ATDC <= mt6631_fm_config.rx_cfg.atdc_th)
			&& (MR >= mt6631_fm_config.rx_cfg.mr_th)
			&& (PRX >= mt6631_fm_config.rx_cfg.prx_th)
			&& (ATDEV >= ATDC)	/* sync scan algorithm */
			&& (softmuteGainLvl >= mt6631_fm_config.rx_cfg.smg_th)) {
			*valid = fm_true;
		} else {
			*valid = fm_false;
		}
		*rssi = RSSI;
/*		if (RSSI < -296)
			WCN_DBG(FM_NTC | CHIP, "rssi\n");
		else if (PAMD > -12)
			WCN_DBG(FM_NTC | CHIP, "PAMD\n");
		else if (ATDC > 3496)
			WCN_DBG(FM_NTC | CHIP, "ATDC\n");
		else if (MR < -67)
			WCN_DBG(FM_NTC | CHIP, "MR\n");
		else if (PRX < 80)
			WCN_DBG(FM_NTC | CHIP, "PRX\n");
		else if (ATDEV < ATDC)
			WCN_DBG(FM_NTC | CHIP, "ATDEV\n");
		else if (softmuteGainLvl < 16421)
			WCN_DBG(FM_NTC | CHIP, "softmuteGainLvl\n");
			*/
	} else {
		WCN_DBG(FM_ALT | CHIP, "smt get CQI failed\n");
		return fm_false;
	}
	WCN_DBG(FM_NTC | CHIP, "valid=%d\n", *valid);
	return fm_true;
}

static fm_bool mt6631_em_test(fm_u16 group_idx, fm_u16 item_idx, fm_u32 item_value)
{
	return fm_true;
}

/*
parm:
	parm.th_type: 0, RSSI. 1, desense RSSI. 2, SMG.
	parm.th_val: threshold value
*/
static fm_s32 mt6631_set_search_th(fm_s32 idx, fm_s32 val, fm_s32 reserve)
{
	switch (idx) {
	case 0:	{
		mt6631_fm_config.rx_cfg.long_ana_rssi_th = val;
		WCN_DBG(FM_NTC | CHIP, "set rssi th =%d\n", val);
		break;
	}
	case 1:	{
		mt6631_fm_config.rx_cfg.desene_rssi_th = val;
		WCN_DBG(FM_NTC | CHIP, "set desense rssi th =%d\n", val);
		break;
	}
	case 2:	{
		mt6631_fm_config.rx_cfg.smg_th = val;
		WCN_DBG(FM_NTC | CHIP, "set smg th =%d\n", val);
		break;
	}
	default:
		break;
	}
	return 0;
}

static fm_s32 MT6631fm_low_power_wa_default(fm_s32 fmon)
{
	return 0;
}

fm_s32 MT6631fm_low_ops_register(struct fm_lowlevel_ops *ops)
{
	fm_s32 ret = 0;
	/* Basic functions. */

	if (ops == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (ops->cb.cur_freq_get == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (ops->cb.cur_freq_set == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	fm_cb_op = &ops->cb;

	ops->bi.pwron = mt6631_pwron;
	ops->bi.pwroff = mt6631_pwroff;
	ops->bi.msdelay = Delayms;
	ops->bi.usdelay = Delayus;
	ops->bi.read = mt6631_read;
	ops->bi.write = mt6631_write;
	ops->bi.top_read = mt6631_top_read;
	ops->bi.top_write = mt6631_top_write;
	ops->bi.host_read = mt6631_host_read;
	ops->bi.host_write = mt6631_host_write;
	ops->bi.setbits = mt6631_set_bits;
	ops->bi.chipid_get = mt6631_get_chipid;
	ops->bi.mute = mt6631_Mute;
	ops->bi.rampdown = mt6631_RampDown;
	ops->bi.pwrupseq = mt6631_PowerUp;
	ops->bi.pwrdownseq = mt6631_PowerDown;
	ops->bi.setfreq = mt6631_SetFreq;
	ops->bi.low_pwr_wa = MT6631fm_low_power_wa_default;
	ops->bi.get_aud_info = mt6631fm_get_audio_info;
#if 0
	ops->bi.seek = mt6631_Seek;
	ops->bi.seekstop = mt6631_SeekStop;
	ops->bi.scan = mt6631_Scan;
	ops->bi.cqi_get = mt6631_CQI_Get;
#ifdef CONFIG_MTK_FM_50KHZ_SUPPORT
	ops->bi.scan = mt6631_Scan_50KHz;
	ops->bi.cqi_get = mt6631_CQI_Get_50KHz;
#endif
	ops->bi.scanstop = mt6631_ScanStop;
	ops->bi.i2s_set = mt6631_I2s_Setting;
#endif
	ops->bi.rssiget = mt6631_GetCurRSSI;
	ops->bi.volset = mt6631_SetVol;
	ops->bi.volget = mt6631_GetVol;
	ops->bi.dumpreg = mt6631_dump_reg;
	ops->bi.msget = mt6631_GetMonoStereo;
	ops->bi.msset = mt6631_SetMonoStereo;
	ops->bi.pamdget = mt6631_GetCurPamd;
	ops->bi.em = mt6631_em_test;
	ops->bi.anaswitch = mt6631_SetAntennaType;
	ops->bi.anaget = mt6631_GetAntennaType;
	ops->bi.caparray_get = mt6631_GetCapArray;
	ops->bi.hwinfo_get = mt6631_hw_info_get;
	ops->bi.i2s_get = mt6631_i2s_info_get;
	ops->bi.is_dese_chan = mt6631_is_dese_chan;
	ops->bi.softmute_tune = mt6631_soft_mute_tune;
	ops->bi.desense_check = mt6631_desense_check;
	ops->bi.cqi_log = mt6631_full_cqi_get;
	ops->bi.pre_search = mt6631_pre_search;
	ops->bi.restore_search = mt6631_restore_search;
	ops->bi.set_search_th = mt6631_set_search_th;

	cmd_buf_lock = fm_lock_create("27_cmd");
	ret = fm_lock_get(cmd_buf_lock);

	cmd_buf = fm_zalloc(TX_BUF_SIZE + 1);

	if (!cmd_buf) {
		WCN_DBG(FM_ALT | CHIP, "6631 fm lib alloc tx buf failed\n");
		ret = -1;
	}
#if 0				/* def CONFIG_MTK_FM_50KHZ_SUPPORT */
	cqi_fifo = fm_fifo_create("6628_cqi_fifo", sizeof(struct adapt_fm_cqi), 640);
	if (!cqi_fifo) {
		WCN_DBG(FM_ALT | CHIP, "6631 fm lib create cqi fifo failed\n");
		ret = -1;
	}
#endif

	return ret;
}

fm_s32 MT6631fm_low_ops_unregister(struct fm_lowlevel_ops *ops)
{
	fm_s32 ret = 0;
	/* Basic functions. */
	if (ops == NULL) {
		pr_err("%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

#if 0				/* def CONFIG_MTK_FM_50KHZ_SUPPORT */
	fm_fifo_release(cqi_fifo);
#endif

	if (cmd_buf) {
		fm_free(cmd_buf);
		cmd_buf = NULL;
	}

	ret = fm_lock_put(cmd_buf_lock);
	fm_memset(&ops->bi, 0, sizeof(struct fm_basic_interface));
	return ret;
}


static const fm_s8 mt6631_chan_para_map[] = {
 /* 0, X, 1, X, 2, X, 3, X, 4, X, 5, X, 6, X, 7, X, 8, X, 9, X*/
	0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6500~6595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6600~6695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,	/* 6700~6795 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6800~6895 */
	0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6900~6995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7000~7095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7100~7195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,	/* 7200~7295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0,	/* 7300~7395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,	/* 7400~7495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7500~7595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,	/* 7600~7695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7700~7795 */
	8, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7800~7895 */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7900~7995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8000~8095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8100~8195 */
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8200~8295 */
	0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,	/* 8300~8395 */
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8400~8495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8500~8595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8600~8695 */
	0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8700~8795 */
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8800~8895 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8900~8995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9000~9095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9100~9195 */
	0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9200~9295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9300~9395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9400~9495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9500~9595 */
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9600~9695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9700~9795 */
	0, 0, 0, 0, 0, 0, 8, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9800~9895 */
	0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,	/* 9900~9995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10000~10095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10100~10195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,	/* 10200~10295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,	/* 10300~10395 */
	8, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10400~10495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,	/* 10500~10595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10600~10695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,	/* 10700~10795 */
	0			/* 10800 */
};
static const fm_u16 mt6631_scan_dese_list[] = {
	6910, 6920, 7680, 7800, 8450, 9210, 9220, 9230, 9590, 9600, 9830, 9900, 9980, 9990, 10400, 10750, 10760
};

static const fm_u16 mt6631_SPI_hopping_list[] = {
	6510, 6520, 6530, 7780, 7790, 7800, 7810, 7820, 9090, 9100, 9110, 9120, 10380, 10390, 10400, 10410, 10420
};

static const fm_u16 mt6631_I2S_hopping_list[] = {
	6550, 6760, 6960, 6970, 7170, 7370, 7580, 7780, 7990, 8810, 9210, 9220, 10240
};

static const fm_u16 mt6631_TDD_list[] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6500~6595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6600~6695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6700~6795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6800~6895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6900~6995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7000~7095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7100~7195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7200~7295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7300~7395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7400~7495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7500~7595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7600~7695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7700~7795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7800~7895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7900~7995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8000~8095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8100~8195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8200~8295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8300~8395 */
	0x0101, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8400~8495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8500~8595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8600~8695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8700~8795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8800~8895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8900~8995 */
	0x0000, 0x0000, 0x0101, 0x0101, 0x0101,	/* 9000~9095 */
	0x0101, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9100~9195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9200~9295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9300~9395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9400~9495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9500~9595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9600~9695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0100,	/* 9700~9795 */
	0x0101, 0x0101, 0x0101, 0x0101, 0x0101,	/* 9800~9895 */
	0x0101, 0x0101, 0x0001, 0x0000, 0x0000,	/* 9900~9995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10000~10095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10100~10195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10200~10295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10300~10395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10400~10495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10500~10595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0100,	/* 10600~10695 */
	0x0101, 0x0101, 0x0101, 0x0101, 0x0101,	/* 10700~10795 */
	0x0001			/* 10800 */
};

static const fm_u16 mt6631_TDD_Mask[] = {
	0x0001, 0x0010, 0x0100, 0x1000
};

/* return value: 0, not a de-sense channel; 1, this is a de-sense channel; else error no */
static fm_s32 mt6631_is_dese_chan(fm_u16 freq)
{
	fm_s32 size;

	if (1 == HQA_ZERO_DESENSE_MAP) /*HQA only :skip desense channel check. */
		return 0;

	size = sizeof(mt6631_scan_dese_list) / sizeof(mt6631_scan_dese_list[0]);

	if (0 == fm_get_channel_space(freq))
		freq *= 10;

	while (size) {
		if (mt6631_scan_dese_list[size - 1] == freq)
			return 1;

		size--;
	}

	return 0;
}

/*  return value:
1, is desense channel and rssi is less than threshold;
0, not desense channel or it is but rssi is more than threshold.*/
static fm_s32 mt6631_desense_check(fm_u16 freq, fm_s32 rssi)
{
	if (mt6631_is_dese_chan(freq)) {
		if (rssi < mt6631_fm_config.rx_cfg.desene_rssi_th)
			return 1;

		WCN_DBG(FM_DBG | CHIP, "desen_rssi %d th:%d\n", rssi, mt6631_fm_config.rx_cfg.desene_rssi_th);
	}
	return 0;
}

static fm_bool mt6631_TDD_chan_check(fm_u16 freq)
{
	fm_u32 i = 0;
	fm_u16 freq_tmp = freq;
	fm_s32 ret = 0;

	ret = fm_get_channel_space(freq_tmp);
	if (0 == ret)
		freq_tmp *= 10;
	else if (-1 == ret)
		return fm_false;

	i = (freq_tmp - 6500) / 5;

	if (mt6631_TDD_list[i / 4] & mt6631_TDD_Mask[i % 4]) {
		WCN_DBG(FM_DBG | CHIP, "Freq %d use TDD solution\n", freq);
		return fm_true;
	} else
		return fm_false;
}

/* get channel parameter, HL side/ FA / ATJ */
static fm_u16 mt6631_chan_para_get(fm_u16 freq)
{
	fm_s32 pos, size;

	if (1 == HQA_RETURN_ZERO_MAP) {
		WCN_DBG(FM_NTC | CHIP, "HQA_RETURN_ZERO_CHAN mt6631_chan_para_map enabled!\n");
		return 0;
	}

	if (0 == fm_get_channel_space(freq))
		freq *= 10;

	if (freq < 6500)
		return 0;

	pos = (freq - 6500) / 5;

	size = sizeof(mt6631_chan_para_map) / sizeof(mt6631_chan_para_map[0]);

	pos = (pos < 0) ? 0 : pos;
	pos = (pos > (size - 1)) ? (size - 1) : pos;

	return mt6631_chan_para_map[pos];
}


static fm_bool mt6631_SPI_hopping_check(fm_u16 freq)
{
	fm_s32 size;

	size = sizeof(mt6631_SPI_hopping_list) / sizeof(mt6631_SPI_hopping_list[0]);

	if (0 == fm_get_channel_space(freq))
		freq *= 10;

	while (size) {
		if (mt6631_SPI_hopping_list[size - 1] == freq)
			return 1;
		size--;
	}

	return 0;
}

