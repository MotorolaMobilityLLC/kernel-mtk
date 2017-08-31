/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/kernel.h>
#include <linux/types.h>

#include "fm_typedef.h"
#include "fm_dbg.h"
#include "fm_err.h"
#include "fm_rds.h"
#include "fm_config.h"
#include "fm_link.h"
#include "fm_cmd.h"

fm_s32 fm_bop_write(fm_u8 addr, fm_u16 value, fm_u8 *buf, fm_s32 size)
{
	if (size < (FM_WRITE_BASIC_OP_SIZE + 2)) {
		WCN_DBG(FM_ERR | CHIP, "%s : left size(%d)/need size(%d)\n",
			__func__, size, FM_WRITE_BASIC_OP_SIZE + 2);
		return -1;
	}

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s :buf invalid pointer\n", __func__);
		return -2;
	}

	buf[0] = FM_WRITE_BASIC_OP;
	buf[1] = FM_WRITE_BASIC_OP_SIZE;
	buf[2] = addr;
	buf[3] = (fm_u8) ((value) & 0x00FF);
	buf[4] = (fm_u8) ((value >> 8) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

	return FM_WRITE_BASIC_OP_SIZE + 2;
}

fm_s32 fm_bop_udelay(fm_u32 value, fm_u8 *buf, fm_s32 size)
{
	if (size < (FM_UDELAY_BASIC_OP_SIZE + 2)) {
		WCN_DBG(FM_ERR | CHIP, "%s : left size(%d)/need size(%d)\n",
			__func__, size, FM_UDELAY_BASIC_OP_SIZE + 2);
		return -1;
	}

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s :buf invalid pointer\n", __func__);
		return -2;
	}

	buf[0] = FM_UDELAY_BASIC_OP;
	buf[1] = FM_UDELAY_BASIC_OP_SIZE;
	buf[2] = (fm_u8) ((value) & 0x000000FF);
	buf[3] = (fm_u8) ((value >> 8) & 0x000000FF);
	buf[4] = (fm_u8) ((value >> 16) & 0x000000FF);
	buf[5] = (fm_u8) ((value >> 24) & 0x000000FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	return FM_UDELAY_BASIC_OP_SIZE + 2;
}

fm_s32 fm_bop_rd_until(fm_u8 addr, fm_u16 mask, fm_u16 value, fm_u8 *buf, fm_s32 size)
{
	if (size < (FM_RD_UNTIL_BASIC_OP_SIZE + 2)) {
		WCN_DBG(FM_ERR | CHIP, "%s : left size(%d)/need size(%d)\n",
			__func__, size, FM_RD_UNTIL_BASIC_OP_SIZE + 2);
		return -1;
	}

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s :buf invalid pointer\n", __func__);
		return -2;
	}

	buf[0] = FM_RD_UNTIL_BASIC_OP;
	buf[1] = FM_RD_UNTIL_BASIC_OP_SIZE;
	buf[2] = addr;
	buf[3] = (fm_u8) ((mask) & 0x00FF);
	buf[4] = (fm_u8) ((mask >> 8) & 0x00FF);
	buf[5] = (fm_u8) ((value) & 0x00FF);
	buf[6] = (fm_u8) ((value >> 8) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2],
		buf[3], buf[4], buf[5], buf[6]);

	return FM_RD_UNTIL_BASIC_OP_SIZE + 2;
}

fm_s32 fm_bop_modify(fm_u8 addr, fm_u16 mask_and, fm_u16 mask_or, fm_u8 *buf, fm_s32 size)
{
	if (size < (FM_MODIFY_BASIC_OP_SIZE + 2)) {
		WCN_DBG(FM_ERR | CHIP, "%s : left size(%d)/need size(%d)\n",
			__func__, size, FM_MODIFY_BASIC_OP_SIZE + 2);
		return -1;
	}

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s :buf invalid pointer\n", __func__);
		return -2;
	}

	buf[0] = FM_MODIFY_BASIC_OP;
	buf[1] = FM_MODIFY_BASIC_OP_SIZE;
	buf[2] = addr;
	buf[3] = (fm_u8) ((mask_and) & 0x00FF);
	buf[4] = (fm_u8) ((mask_and >> 8) & 0x00FF);
	buf[5] = (fm_u8) ((mask_or) & 0x00FF);
	buf[6] = (fm_u8) ((mask_or >> 8) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2],
		buf[3], buf[4], buf[5], buf[6]);

	return FM_MODIFY_BASIC_OP_SIZE + 2;
}

fm_s32 fm_bop_top_write(fm_u16 addr, fm_u32 value, fm_u8 *buf, fm_s32 size)
{
	if (size < (FM_TOP_WRITE_BOP_SIZE + 2)) {
		WCN_DBG(FM_ERR | CHIP, "%s : left size(%d)/need size(%d)\n",
			__func__, size, FM_TOP_WRITE_BOP_SIZE + 2);
		return -1;
	}

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s :buf invalid pointer\n", __func__);
		return -2;
	}

	buf[0] = FM_TOP_WRITE_BASIC_OP;
	buf[1] = FM_TOP_WRITE_BOP_SIZE;
	buf[2] = 04;
	buf[3] = (fm_u8) ((addr) & 0x00FF);
	buf[4] = (fm_u8) ((addr >> 8) & 0x00FF);
	buf[5] = (fm_u8) ((value) & 0x00FF);
	buf[6] = (fm_u8) ((value >> 8) & 0x00FF);
	buf[7] = (fm_u8) ((value >> 16) & 0x00FF);
	buf[8] = (fm_u8) ((value >> 24) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1],
		buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);

	return FM_TOP_WRITE_BOP_SIZE + 2;
}

fm_s32 fm_bop_top_rd_until(fm_u16 addr, fm_u32 mask, fm_u32 value, fm_u8 *buf, fm_s32 size)
{
	if (size < (FM_TOP_RD_UNTIL_BOP_SIZE + 2)) {
		WCN_DBG(FM_ERR | CHIP, "%s : left size(%d)/need size(%d)\n",
			__func__, size, FM_TOP_RD_UNTIL_BOP_SIZE + 2);
		return -1;
	}

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s :buf invalid pointer\n", __func__);
		return -2;
	}

	buf[0] = FM_TOP_RD_UNTIL_BASIC_OP;
	buf[1] = FM_TOP_RD_UNTIL_BOP_SIZE;
	buf[2] = 04;
	buf[3] = (fm_u8) ((addr) & 0x00FF);
	buf[4] = (fm_u8) ((addr >> 8) & 0x00FF);
	buf[5] = (fm_u8) ((mask) & 0x00FF);
	buf[6] = (fm_u8) ((mask >> 8) & 0x00FF);
	buf[7] = (fm_u8) ((mask >> 16) & 0x00FF);
	buf[8] = (fm_u8) ((mask >> 24) & 0x00FF);
	buf[9] = (fm_u8) ((value) & 0x00FF);
	buf[10] = (fm_u8) ((value >> 8) & 0x00FF);
	buf[11] = (fm_u8) ((value >> 16) & 0x00FF);
	buf[12] = (fm_u8) ((value >> 24) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9],
		buf[10], buf[11], buf[12]);

	return FM_TOP_RD_UNTIL_BOP_SIZE + 2;
}

fm_s32 fm_op_seq_combine_cmd(fm_u8 *buf, fm_u8 opcode, fm_s32 pkt_size)
{
	fm_s32 total_size = 0;

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s :buf invalid pointer\n", __func__);
		return -1;
	}

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = opcode;
	buf[2] = (fm_u8) (pkt_size & 0x00FF);
	buf[3] = (fm_u8) ((pkt_size >> 8) & 0x00FF);
	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

	total_size = pkt_size + 4;

	return total_size;
}

/*
 * fm_patch_download - Wholechip FM Power Up: step 3, download patch to f/w,
 * @buf - target buf
 * @buf_size - buffer size
 * @seg_num - total segments that this patch divided into
 * @seg_id - No. of Segments: segment that will now be sent
 * @src - patch source buffer
 * @seg_len - segment size: segment that will now be sent
 * return package size
 */
fm_s32 fm_patch_download(fm_u8 *buf, fm_s32 buf_size, fm_u8 seg_num, fm_u8 seg_id,
			     const fm_u8 *src, fm_s32 seg_len)
{
	fm_s32 pkt_size = 0;
	fm_u8 *dst = NULL;

	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FM_PATCH_DOWNLOAD_OPCODE;
	pkt_size = 4;

	buf[pkt_size++] = seg_num;
	buf[pkt_size++] = seg_id;

	if (seg_len > (buf_size - pkt_size))
		return -1;

	dst = &buf[pkt_size];
	pkt_size += seg_len;

	/* copy patch to tx buffer */
	while (seg_len--) {
		*dst = *src;
		src++;
		dst++;
	}

	buf[2] = (fm_u8) ((pkt_size - 4) & 0x00FF);
	buf[3] = (fm_u8) (((pkt_size - 4) >> 8) & 0x00FF);
	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2],
		buf[3], buf[4], buf[5], buf[6]);

	return pkt_size;
}

/*
 * fm_coeff_download - Wholechip FM Power Up: step 3,download coeff to f/w,
 * @buf - target buf
 * @buf_size - buffer size
 * @seg_num - total segments that this patch divided into
 * @seg_id - No. of Segments: segment that will now be sent
 * @src - patch source buffer
 * @seg_len - segment size: segment that will now be sent
 * return package size
 */
fm_s32 fm_coeff_download(fm_u8 *buf, fm_s32 buf_size, fm_u8 seg_num, fm_u8 seg_id,
			     const fm_u8 *src, fm_s32 seg_len)
{
	fm_s32 pkt_size = 0;
	fm_u8 *dst = NULL;

	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FM_COEFF_DOWNLOAD_OPCODE;
	pkt_size = 4;

	buf[pkt_size++] = seg_num;
	buf[pkt_size++] = seg_id;

	if (seg_len > (buf_size - pkt_size))
		return -1;

	dst = &buf[pkt_size];
	pkt_size += seg_len;

	/* copy patch to tx buffer */
	while (seg_len--) {
		*dst = *src;
		src++;
		dst++;
	}

	buf[2] = (fm_u8) ((pkt_size - 4) & 0x00FF);
	buf[3] = (fm_u8) (((pkt_size - 4) >> 8) & 0x00FF);
	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2],
		buf[3], buf[4], buf[5], buf[6]);

	return pkt_size;
}

/*
 * fm_full_cqi_req - execute request cqi info action,
 * @buf - target buf
 * @buf_size - buffer size
 * @freq - 7600 ~ 10800, freq array
 * @cnt - channel count
 * @type - request type, 1: a single channel; 2: multi channel;
 * 3:multi channel with 100Khz step; 4: multi channel with 50Khz step
 *
 * return package size
 */
fm_s32 fm_full_cqi_req(fm_u8 *buf, fm_s32 buf_size, fm_u16 *freq, fm_s32 cnt, fm_s32 type)
{
	fm_s32 pkt_size = 0;

	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FM_SOFT_MUTE_TUNE_OPCODE;
	pkt_size = 4;

	switch (type) {
	case 1:
		buf[pkt_size] = 0x0001;
		pkt_size++;
		buf[pkt_size] = (fm_u8) ((*freq) & 0x00FF);
		pkt_size++;
		buf[pkt_size] = (fm_u8) ((*freq >> 8) & 0x00FF);
		pkt_size++;
		break;
	case 2:
		buf[pkt_size] = 0x0002;
		pkt_size++;
		break;
	case 3:
		buf[pkt_size] = 0x0003;
		pkt_size++;
		break;
	case 4:
		buf[pkt_size] = 0x0004;
		pkt_size++;
		break;
	default:
		buf[pkt_size] = (fm_u16) type;
		pkt_size++;
		break;
	}

	buf[2] = (fm_u8) ((pkt_size - 4) & 0x00FF);
	buf[3] = (fm_u8) (((pkt_size - 4) >> 8) & 0x00FF);

	return pkt_size;
}


fm_s32 fm_get_reg(fm_u8 *buf, fm_s32 buf_size, fm_u8 addr)
{
	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FSPI_READ_OPCODE;
	buf[2] = 0x01;
	buf[3] = 0x00;
	buf[4] = addr;

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
	return 5;
}

fm_s32 fm_set_reg(fm_u8 *buf, fm_s32 buf_size, fm_u8 addr, fm_u16 value)
{
	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FSPI_WRITE_OPCODE;
	buf[2] = 0x03;
	buf[3] = 0x00;
	buf[4] = addr;
	buf[5] = (fm_u8) ((value) & 0x00FF);
	buf[6] = (fm_u8) ((value >> 8) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2],
		buf[3], buf[4], buf[5], buf[6]);
	return 7;
}

fm_s32 fm_set_bits_reg(fm_u8 *buf, fm_s32 buf_size, fm_u8 addr, fm_u16 bits, fm_u16 mask)
{
	fm_s32 pkt_size = 0;

	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = 0x11;		/* 0x11 this opcode won't be parsed as an opcode, so set here as spcial case. */
	pkt_size = 4;
	pkt_size += fm_bop_modify(addr, mask, bits, &buf[pkt_size], buf_size - pkt_size);

	buf[2] = (fm_u8) ((pkt_size - 4) & 0x00FF);
	buf[3] = (fm_u8) (((pkt_size - 4) >> 8) & 0x00FF);

	return pkt_size;
}

/*top register read*/
fm_s32 fm_top_get_reg(fm_u8 *buf, fm_s32 buf_size, fm_u16 addr)
{
	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = CSPI_READ_OPCODE;
	buf[2] = 0x03;
	buf[3] = 0x00;
	buf[4] = 0x04;		/* top 04,fm 02 */
	buf[5] = (fm_u8) ((addr) & 0x00FF);
	buf[6] = (fm_u8) ((addr >> 8) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2],
		buf[3], buf[4], buf[5], buf[6]);
	return 7;
}

fm_s32 fm_top_set_reg(fm_u8 *buf, fm_s32 buf_size, fm_u16 addr, fm_u32 value)
{
	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = CSPI_WRITE_OPCODE;
	buf[2] = 0x07;
	buf[3] = 0x00;
	buf[4] = 0x04;		/* top 04,fm 02 */
	buf[5] = (fm_u8) ((addr) & 0x00FF);
	buf[6] = (fm_u8) ((addr >> 8) & 0x00FF);
	buf[7] = (fm_u8) ((value) & 0x00FF);
	buf[8] = (fm_u8) ((value >> 8) & 0x00FF);
	buf[9] = (fm_u8) ((value >> 16) & 0x00FF);
	buf[10] = (fm_u8) ((value >> 24) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0],
		buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10]);
	return 11;
}

/*host register read*/
fm_s32 fm_host_get_reg(fm_u8 *buf, fm_s32 buf_size, fm_u32 addr)
{
	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FM_HOST_READ_OPCODE;
	buf[2] = 0x04;
	buf[3] = 0x00;
	buf[4] = (fm_u8) ((addr) & 0x00FF);
	buf[5] = (fm_u8) ((addr >> 8) & 0x00FF);
	buf[6] = (fm_u8) ((addr >> 16) & 0x00FF);
	buf[7] = (fm_u8) ((addr >> 24) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2],
		buf[3], buf[4], buf[5], buf[6], buf[7]);
	return 8;
}

fm_s32 fm_host_set_reg(fm_u8 *buf, fm_s32 buf_size, fm_u32 addr, fm_u32 value)
{
	if (buf_size < TX_BUF_SIZE)
		return -1;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FM_HOST_WRITE_OPCODE;
	buf[2] = 0x08;
	buf[3] = 0x00;
	buf[4] = (fm_u8) ((addr) & 0x00FF);
	buf[5] = (fm_u8) ((addr >> 8) & 0x00FF);
	buf[6] = (fm_u8) ((addr >> 16) & 0x00FF);
	buf[7] = (fm_u8) ((addr >> 24) & 0x00FF);
	buf[8] = (fm_u8) ((value) & 0x00FF);
	buf[9] = (fm_u8) ((value >> 8) & 0x00FF);
	buf[10] = (fm_u8) ((value >> 16) & 0x00FF);
	buf[11] = (fm_u8) ((value >> 24) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
	return 12;
}

fm_s32 fm_pmic_get_reg(fm_u8 *buf, fm_s32 buf_size, fm_u8 addr)
{
	if (buf_size < TX_BUF_SIZE)
		return -1;

	if (buf == NULL)
		return -2;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FM_READ_PMIC_CR_OPCODE;
	buf[2] = 0x01;
	buf[3] = 0x00;
	buf[4] = addr;

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2],
		buf[3], buf[4]);
	return 5;
}

fm_s32 fm_pmic_set_reg(fm_u8 *buf, fm_s32 buf_size, fm_u8 addr, fm_u32 val)
{
	if (buf_size < TX_BUF_SIZE)
		return -1;

	if (buf == NULL)
		return -2;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FM_WRITE_PMIC_CR_OPCODE;
	buf[2] = 0x05;
	buf[3] = 0x00;
	buf[4] = addr;
	buf[5] = (fm_u8) ((val) & 0x00FF);
	buf[6] = (fm_u8) ((val >> 8) & 0x00FF);
	buf[7] = (fm_u8) ((val >> 16) & 0x00FF);
	buf[8] = (fm_u8) ((val >> 24) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);
	return 9;
}

fm_s32 fm_pmic_mod_reg(fm_u8 *buf, fm_s32 buf_size, fm_u8 addr, fm_u32 mask_and, fm_u32 mask_or)
{
	if (buf_size < TX_BUF_SIZE)
		return -1;

	if (buf == NULL)
		return -2;

	buf[0] = FM_TASK_COMMAND_PKT_TYPE;
	buf[1] = FM_MODIFY_PMIC_CR_OPCODE;
	buf[2] = 0x09;
	buf[3] = 0x00;
	buf[4] = addr;
	buf[5] = (fm_u8) ((mask_and) & 0x00FF);
	buf[6] = (fm_u8) ((mask_and >> 8) & 0x00FF);
	buf[7] = (fm_u8) ((mask_and >> 16) & 0x00FF);
	buf[8] = (fm_u8) ((mask_and >> 24) & 0x00FF);
	buf[9] = (fm_u8) ((mask_or) & 0x00FF);
	buf[10] = (fm_u8) ((mask_or >> 8) & 0x00FF);
	buf[11] = (fm_u8) ((mask_or >> 16) & 0x00FF);
	buf[12] = (fm_u8) ((mask_or >> 24) & 0x00FF);

	WCN_DBG(FM_DBG | CHIP, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0],
		buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12]);

	return 13;
}

