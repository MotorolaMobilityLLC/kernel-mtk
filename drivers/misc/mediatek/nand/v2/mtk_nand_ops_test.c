/*
 * Copyright (c) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/******************************************************************************
* mtk_nand_ops_test.c
 *
* Copyright 2016 MediaTek Co.,Ltd.
 *
* DESCRIPTION:
*	This file provide test function for mtk_nand_ops.c to do unit test
*
* modification history
* ----------------------------------------
* v1.0, 27 Dec 2016, mtk
* ----------------------------------------
******************************************************************************/
#include "bmt.h"
#include "mtk_nand_ops.h"
#include "mtk_nand_ops_test.h"
#if MTK_NAND_SIM_ERR
#include <linux/random.h>
#endif
/************** UNIT TEST ***************/
#ifdef MTK_NAND_CHIP_TEST
int mtk_nand_debug_callback(struct mtk_nand_chip_info *info,
		unsigned char *data_buffer, unsigned char *oob_buffer,
		unsigned int block, unsigned int page, int status, void *userdata)
{
	nand_debug("mtk_nand_debug_callback block:0x%x", block);
	nand_debug("mtk_nand_debug_callback page:0x%x", page);
	nand_debug("mtk_nand_debug_callback status:0x%x", status);
	return 0;
}

#ifdef MTK_NAND_CHIP_DUMP_DATA_TEST
static void dump_buf_data(unsigned char *buf, unsigned size)
{
	unsigned int i;

	for (i = 0; i < size; i += 16) {
		pr_info("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       buf[i], buf[i+1], buf[i+2], buf[i+3],
		       buf[i+4], buf[i+5], buf[i+6], buf[i+7],
		       buf[i+8], buf[i+9], buf[i+10], buf[i+11],
		       buf[i+12], buf[i+13], buf[i+14], buf[i+15]);
	}
}
#endif

#define MAGIC_PATTERN_S (0x4E414542)
static int ticks = 1;
static int randseed = 12345;
static int total_count = 10;

static int rand(void)
{
	return (randseed = randseed * 12345 + 17);
}

static void seed(int seed)
{
	randseed = seed;
}


static unsigned short SS_RANDOM_SEED[128] = {
	/* for page 0~127 */
	0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
	0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
	0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
	0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
	0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
	0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
	0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
	0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
	0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
	0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
	0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
	0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
	0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
	0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
	0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
	0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7
};

static void mtk_nand_prandom_bytes(char *buf, unsigned int size)
{
	int i = 0;
	int temp;

	for (i = 0; i < size; i++) {
		temp = rand() % 256;
		temp = (temp < 128) ? temp : temp-256;
		buf[i] = (char)temp;
	}
	seed((int)SS_RANDOM_SEED[total_count++]+ticks);
	if (total_count > 127)
		total_count = 0;
	ticks++;
}

static void psequence_bytes(char *buf, unsigned int size)
{
	int i = 0;
	unsigned short *buf_tmp;

	buf_tmp = (unsigned short *)buf;
	for (i = 0; i < size/2; i++)
		buf_tmp[i] = i;
}

static int check_data_empty(void *data, unsigned size)
{
	unsigned i;
	u32 *tp = (u32 *) data;

	for (i = 0; i < size / 4; i++) {
		if (*(tp + i) != 0xffffffff)
			return 0;
	}

	return 1;
}

#ifdef MTK_NAND_READ_COMPARE
static int compare_data(unsigned char *sbuf, unsigned char *dbuf, unsigned size)
{
	unsigned i, ret = 0;

	for (i = 0; i < size; i++) {
		if (sbuf[i] != dbuf[i]) {
			nand_pr_err("compare data failed sbuf[%d]:%x dbuf[%d]:%x",
				i, sbuf[i], i, dbuf[i]);
			ret = 1;
		}
	}

	if (ret)
		dump_nfi();

	return 0;
}
#endif

#ifdef MTK_NAND_PERFORMANCE_TEST
static suseconds_t time_diff(struct timeval *end_time, struct timeval *start_time)
{
	struct timeval difference;

	difference.tv_sec = end_time->tv_sec - start_time->tv_sec;
	difference.tv_usec = end_time->tv_usec - start_time->tv_usec;

	/* Using while instead of if below makes the code slightly more robust. */

	while (difference.tv_usec < 0) {
		difference.tv_usec += 1000000;
		difference.tv_sec -= 1;
	}

	return 1000000LL * difference.tv_sec + difference.tv_usec;
}
#endif
static void mtk_chip_test_mark_bad(void)
{
#if !MTK_NAND_MARK_BAD_TEST
		return;
#endif
	/* markbad */
	mtk_chip_mark_bad_block(&data_info->chip_info, 0x8);
	mtk_chip_mark_bad_block(&data_info->chip_info, 0x50);
	mtk_chip_mark_bad_block(&data_info->chip_info, 0x78);
	mtk_chip_mark_bad_block(&data_info->chip_info, 0x9);
}

static int mtk_chip_test_erase(struct mtk_nand_chip_info *chip_info,
	unsigned int block, void *userdata)
{
	struct timeval stimer, etimer;
	suseconds_t total_time;
	int ret = 0;

	do_gettimeofday(&stimer);

	ret = mtk_nand_chip_erase_block(chip_info, block, 1,
		&mtk_nand_debug_callback, userdata);
	if (ret) {
		nand_pr_err("bad block here block:0x%x", block);
		return ret;
	}
#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
	ret = mtk_nand_chip_erase_block(chip_info, block+1, 1,
		&mtk_nand_debug_callback, userdata);
	if (ret) {
		nand_pr_err("bad block here block:0x%x", block);
		return ret;
	}
#endif
	mtk_nand_chip_sync(chip_info);

	do_gettimeofday(&etimer);
	total_time = time_diff(&etimer, &stimer);
	nand_info("Erase total_time:%ld mS", total_time/1000);
	return ret;
}

static int mtk_chip_test_write(struct mtk_nand_chip_info *chip_info,
	unsigned int block, unsigned char *data_buffer,
	unsigned char *oob_buffer, void *userdata)
{
	struct timeval stimer, etimer;
	suseconds_t total_time;
	unsigned int j;
	unsigned int pages_per_blk;
	unsigned long block_size;
	int ret = 0;

	if (block < chip_info->data_block_num)
		pages_per_blk = chip_info->data_page_num;
	else
		pages_per_blk = chip_info->log_page_num;

	do_gettimeofday(&stimer);

	for (j = 0; j < pages_per_blk; j++) {
#ifdef MTK_NAND_CHIP_DUMP_DATA_TEST
		nand_info("write blk:0x%x page:0x%x data dump", block, j);
		dump_buf_data(data_buffer, chip_info->data_oob_size);
		nand_info("oob dump here");
		dump_buf_data(oob_buffer, chip_info->data_oob_size);
#endif

		ret = mtk_nand_chip_write_page(chip_info, data_buffer, oob_buffer,
			block, j, 1, &mtk_nand_debug_callback, userdata);
		if (ret) {
			nand_pr_err("bad block here block:0x%x", block);
			return ret;
		}

#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
		ret = mtk_nand_chip_write_page(chip_info, data_buffer, oob_buffer,
			block+1, j, 1, &mtk_nand_debug_callback, userdata);
		if (ret) {
			nand_pr_err("bad block here block:0x%x", block);
			return ret;
		}
#endif
	}

	mtk_nand_chip_sync(chip_info);

	do_gettimeofday(&etimer);
	total_time = time_diff(&etimer, &stimer);
	block_size = (pages_per_blk*chip_info->data_page_size);
	block_size *= 1000;
#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
	block_size *= 2;
#endif
	nand_info("Write Speed: %ldKB/S, total_time:%ld mS, block_size:%ld",
		block_size/total_time, total_time/1000, block_size/1000);

	return ret;
}

static int mtk_chip_test_read_one_page(
	struct mtk_nand_chip_info *chip_info,
	unsigned int block, unsigned int page,
	unsigned char *r_data, unsigned char *r_oob,
	unsigned char *golden_data, unsigned char *golden_oob,
	suseconds_t *total_time)
{
	struct timeval stimer, etimer;
	int ret = 0;

	memset(r_data, 0, chip_info->data_page_size);
	memset(r_oob, 0, chip_info->data_oob_size);
	do_gettimeofday(&stimer);
	ret = mtk_nand_chip_read_page(chip_info, r_data, r_oob,
		block, page, 0, chip_info->data_page_size);
	if (ret && (ret != -ENANDFLIPS)) {
		nand_pr_err("read block failed block:0x%x page:0x%x", block, page);
		return ret;
	}

	do_gettimeofday(&etimer);
	*total_time += time_diff(&etimer, &stimer);

	if (check_data_empty(r_data, chip_info->data_page_size)) {
		nand_pr_err("*****Check empty page here blk:0x%x page:0x%x ", block, page);
		return -ENANDREAD;
	}

#ifdef MTK_NAND_CHIP_DUMP_DATA_TEST
	nand_info("read blk:0x%x page:0x%x data first 64 bytes dump", block, page);
	dump_buf_data(r_data, chip_info->data_oob_size);
	nand_info("data last 64 bytes dump");
	dump_buf_data((r_data + chip_info->data_page_size
			-chip_info->data_oob_size),
			chip_info->data_oob_size);

	nand_info("oob dump here");
	dump_buf_data(r_oob, chip_info->data_oob_size);
#endif

#ifdef MTK_NAND_READ_COMPARE
if (compare_data(r_data, golden_data,
		chip_info->data_page_size))
	nand_pr_err("$$$$$$check data failed i:0x%x j:0x%x", block, page);

if (compare_data(r_oob, golden_oob, chip_info->data_oob_size))
	nand_pr_err("######check fdm failed i:0x%x j:0x%x", block, page);
#endif

	return ret;
}

static int mtk_chip_test_read(
	struct mtk_nand_chip_info *chip_info,
	unsigned int block,
	unsigned char *golden_data, unsigned char *golden_oob)
{
	suseconds_t total_time = 0;
	unsigned int j;
	unsigned int pages_per_blk;
	unsigned long block_size;
	unsigned char *r_data, *r_oob;
	int ret = 0;

	r_data =  kmalloc(chip_info->data_page_size, GFP_KERNEL);
	r_oob =  kmalloc(1024, GFP_KERNEL);

	if (block < chip_info->data_block_num)
		pages_per_blk = chip_info->data_page_num;
	else
		pages_per_blk = chip_info->log_page_num;

	for (j = 0; j < pages_per_blk; j++) {
		ret = mtk_chip_test_read_one_page(chip_info, block, j,
				r_data, r_oob, golden_data, golden_oob, &total_time);
		if (ret)
			break;

#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
		ret = mtk_chip_test_read_one_page(chip_info, block+1, j,
				r_data, r_oob, golden_data, golden_oob, &total_time);
		if (ret)
			break;
#endif
	}

	block_size = (pages_per_blk*chip_info->data_page_size);
	block_size *= 1000;
#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
	block_size *= 2;
#endif
	nand_info("Read Speed: %ldKB/S, total_time:%ld mS, block_size:%ld",
		block_size/total_time, total_time/1000, block_size/1000);
	kfree(r_data);
	kfree(r_oob);

	return ret;
}

void mtk_chip_test_gen_page_data(
	struct mtk_nand_chip_info *chip_info,
	unsigned char *data, unsigned char *oob)
{
	unsigned int data_flag;

	data_flag = 0;
	if (data_flag++ & 1) {
		mtk_nand_prandom_bytes(data, chip_info->data_page_size);
		psequence_bytes(oob, chip_info->data_oob_size);
	} else {
		psequence_bytes(data, chip_info->data_page_size);
		mtk_nand_prandom_bytes(oob, chip_info->data_oob_size);
	}
}

void mtk_chip_unit_test(void)
{
#ifdef MTK_NAND_CHIP_TEST
	struct mtk_nand_chip_info *chip_info = &data_info->chip_info;
	unsigned char *golden_data, *golden_oob;
	unsigned int i, userdata;
	int start_blk, end_blk, step;
	int ret = 0;

	golden_data =  kmalloc(chip_info->data_page_size, GFP_KERNEL);
	golden_oob =  kmalloc(1024, GFP_KERNEL);

	/*** BMT Related ***/
	mtk_chip_test_mark_bad();

	nand_info("chip_info->data_block_num:0x%x",
		chip_info->data_block_num);

#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
	start_blk = chip_info->data_block_num - 8;
	end_blk = chip_info->data_block_num + 8;
	step = 2;
#else
	start_blk = 0;
	end_blk = chip_info->data_block_num + chip_info->log_block_num;
	step = 1;
#endif

	for (i = start_blk; i < end_blk; i += step) {
		nand_info("Begin to test block:0x%x", i);
		/* block test*/
		if (mtk_isbad_block(i)) {
			nand_pr_err("Skip bad blk:0x%x", i);
			continue;
		}

#ifdef MTK_NAND_CHIP_MULTI_PLANE_TEST
		if (mtk_isbad_block(i+1)) {
			nand_pr_err("Skip bad blk:0x%x", i);
			continue;
		}
#endif
		userdata = (i*chip_info->data_page_num) | (1<<31);
		mtk_chip_test_gen_page_data(chip_info, golden_data, golden_oob);

		/*** block Erase ***/
		ret = mtk_chip_test_erase(chip_info, i, &userdata);
		if (ret)
			continue;

		/*** block Write***/
		ret = mtk_chip_test_write(chip_info, i, golden_data, golden_oob, &userdata);
		if (ret)
			continue;

		/*** block Read ***/
		mtk_chip_test_read(chip_info, i, golden_data, golden_oob);
	}

	kfree(golden_data);
	kfree(golden_oob);
	nand_info("^^^^^^^TEST END^^^^^^^^");
#endif
}
#endif

#if MTK_NAND_SIM_ERR
static bool sim_err_hit(struct err_para *para,
	int block, int page)
{
	int rand = 0;
	bool hit = false;

	if (!para->count)
		return false;

	if (block == para->block &&
		((page == para->page) || page == -1)) {
		hit = true;
	} else if (para->rate > 0) {
		get_random_bytes(&rand, sizeof(int));
		rand = rand % 100;
		hit = (rand < para->rate);
	}

	if (hit && para->count != -1)
		para->count--;

	return hit;
}

int sim_nand_err(enum operation_types op, int block, int page)
{
	struct sim_err *err = &data_info->err;
	int ret = 0;

	switch (op) {
	case MTK_NAND_OP_READ:
		if (sim_err_hit(&err->read_fail, block, page))
			ret = -ENANDREAD;
		else if (sim_err_hit(&err->bitflip_fail, block, page))
			ret = -ENANDFLIPS;
		break;
	case MTK_NAND_OP_WRITE:
		if (sim_err_hit(&err->write_fail, block, page))
			ret = -ENANDWRITE;
		break;
	case MTK_NAND_OP_ERASE:
		if (sim_err_hit(&err->erase_fail, block, -1))
			ret = -ENANDERASE;
		break;
	default:
		break;
	}

	if (!ret && sim_err_hit(&err->bad_block, block, -1))
		ret = -ENANDBAD;

	if (ret)
		nand_pr_err("op mode:%d, err:%d, block:%d, page:%d",
			op, ret, block, page);

	return ret;
}
#endif
