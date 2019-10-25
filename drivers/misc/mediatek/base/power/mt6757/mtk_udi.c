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
/*
 * @file    mtk_udi.c
 * @brief   Driver for UDI interface
 *
 */
#define __MT_UDI_C__

/* system includes */
#ifdef __KERNEL__
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif
#else
#include <common.h> /* for printf */
#endif
/* local includes */
#include <mt-plat/sync_write.h> /* mt_reg_sync_writel */
#include <mtk_udi.h>

static unsigned int func_lv_mask_udi;

/* UDI pin mux ADDR */
/* 1. Write 0x1000_5330 bit[27:24]=0x3 to switch UDI pin, TCK */
/* 2. Write 0x1000_5340 bit[15:0]=0x3333 to switch UDI pin,TDI,TDO,TMS and */
/* TRST */
#ifdef CONFIG_OF
static void __iomem *udipin_base; /* 0x10005000 0x1000, UDI pinmux reg */
#endif

#ifdef __KERNEL__
#define UDIPIN_BASE (udipin_base)
#else
#define UDIPIN_BASE 0x10005000
#endif

/* 0x10005000 0x1000, DFD,UDI pinmux reg */
#define UDIPIN_UDI_JTAG1 (UDIPIN_BASE + 0x330)
#define UDIPIN_UDI_JTAG2 (UDIPIN_BASE + 0x340)
#define UDIPIN_UDI_EN (UDIPIN_BASE + 0x6E0)

#ifdef __KERNEL__
#define _udi_read(addr) readl(addr)
#define _udi_write(addr, val) mt_reg_sync_writel((val), ((void *)addr))
#endif

/*
 * LOG
 */
#define UDI_TAG "[mt_udi] "
#ifdef __KERNEL__
#ifdef USING_XLOG
#include <linux/xlog.h>
#define udi_emerg(fmt, args...)                                                \
	pr_debug(ANDROID_LOG_ERROR, UDI_TAG, fmt, ##args)
#define udi_alert(fmt, args...)                                                \
	pr_debug(ANDROID_LOG_ERROR, UDI_TAG, fmt, ##args)
#define udi_crit(fmt, args...) pr_debug(ANDROID_LOG_ERROR, UDI_TAG, fmt, ##args)
#define udi_error(fmt, args...)                                                \
	pr_debug(ANDROID_LOG_ERROR, UDI_TAG, fmt, ##args)
#define udi_warning(fmt, args...)                                              \
	pr_debug(ANDROID_LOG_WARN, UDI_TAG, fmt, ##args)
#define udi_notice(fmt, args...) pr_info(ANDROID_LOG_INFO, UDI_TAG, fmt, ##args)
#define udi_info(fmt, args...) pr_info(ANDROID_LOG_INFO, UDI_TAG, fmt, ##args)
#define udi_debug(fmt, args...)                                                \
	pr_debug(ANDROID_LOG_DEBUG, UDI_TAG, fmt, ##args)
#define udi_ver(fmt, args...)                                                  \
	do {                                                                   \
		if (func_lv_mask_udi)                                          \
			pr_info(ANDROID_LOG_INFO, UDI_TAG, fmt, ##args);       \
	} while (0)
#else
#define udi_emerg(fmt, args...) pr_debug(UDI_TAG fmt, ##args)
#define udi_alert(fmt, args...) pr_debug(UDI_TAG fmt, ##args)
#define udi_crit(fmt, args...) pr_debug(UDI_TAG fmt, ##args)
#define udi_error(fmt, args...) pr_debug(UDI_TAG fmt, ##args)
#define udi_warning(fmt, args...) pr_debug(UDI_TAG fmt, ##args)
#define udi_notice(fmt, args...) pr_notice(UDI_TAG fmt, ##args)
#define udi_info(fmt, args...) pr_info(UDI_TAG fmt, ##args)
#define udi_debug(fmt, args...) pr_debug(UDI_TAG fmt, ##args)
#define udi_ver(fmt, args...)                                                  \
	do {                                                                   \
		if (func_lv_mask_udi)                                          \
			pr_info(UDI_TAG fmt, ##args);                          \
	} while (0)
#endif
#else
#define udi_emerg(fmt, args...) printf(UDI_TAG fmt, ##args)
#define udi_alert(fmt, args...) printf(UDI_TAG fmt, ##args)
#define udi_crit(fmt, args...) printf(UDI_TAG fmt, ##args)
#define udi_error(fmt, args...) printf(UDI_TAG fmt, ##args)
#define udi_warning(fmt, args...) printf(UDI_TAG fmt, ##args)
#define udi_notice(fmt, args...) printf(UDI_TAG fmt, ##args)
#define udi_info(fmt, args...) printf(UDI_TAG fmt, ##args)
#define udi_debug(fmt, args...) printf(UDI_TAG fmt, ##args)
#define udi_ver(fmt, args...) printf(UDI_TAG fmt, ##args)
#endif

static unsigned int func_lv_mask_udi;
unsigned char IR_byte[UDI_FIFOSIZE], DR_byte[UDI_FIFOSIZE];
unsigned int IR_bit_count, IR_pause_count;
unsigned int DR_bit_count, DR_pause_count;
unsigned int jtag_sw_tck; /* default debug channel = 1 */
unsigned int udi_addr_phy = 0x10200750;
unsigned int tck_bit, tdi_bit, tms_bit, ntrst_bit, tdo_bit;

#define CTOI(char_ascii)                                                       \
	((char_ascii <= 0x39)                                                  \
	     ? (char_ascii - 0x30)                                             \
	     : ((char_ascii <= 0x46) ? (char_ascii - 55) : (char_ascii - 87)))

int udi_jtag_clock_read(void)
{
	int i, j;

	/* support ID or DR zero */
	if ((IR_bit_count == 0) && (DR_bit_count == 0))
		return 0;

	if (IR_bit_count) {
		/* into idel mode */
		udi_jtag_clock(jtag_sw_tck, 1, 0, 0, 1);
		/* Jog the state machine arround to shift IR */
		udi_jtag_clock(jtag_sw_tck, 1, 1, 0, 2);
		udi_jtag_clock(jtag_sw_tck, 1, 0, 0, 2);

		/* Shift the IR bits, assert TMS=1 for last bit */
		for (i = 0; i < IR_bit_count; i++) {
			j = udi_jtag_clock(
			    jtag_sw_tck, 1, ((i == (IR_bit_count - 1)) ? 1 : 0),
			    (((IR_byte[i >> 3]) >> (i & 7)) & 1), 1);
			IR_byte[i >> 3] &= ~(1 << (i & 7));
			IR_byte[i >> 3] |= (j << (i & 7));
		}

		/* Should be in UPDATE IR */
		if (IR_pause_count) {
			udi_jtag_clock(jtag_sw_tck, 1, 0, 0, IR_pause_count);
			udi_jtag_clock(jtag_sw_tck, 1, 1, 0, 2);
		} else
			udi_jtag_clock(jtag_sw_tck, 1, 1, 0, 1);

		if (DR_bit_count) {
			/* Jog the state machine arround to shift DR */
			udi_jtag_clock(jtag_sw_tck, 1, 1, 0, 1);
			udi_jtag_clock(jtag_sw_tck, 1, 0, 0, 2);
			/* Shift the DR bits, assert TMS=1 for last bit */
			for (i = 0; i < DR_bit_count; i++) {
				j = udi_jtag_clock(
				    jtag_sw_tck, 1,
				    ((i == (DR_bit_count - 1)) ? 1 : 0),
				    (((DR_byte[i >> 3]) >> (i & 7)) & 1), 1);
				DR_byte[i >> 3] &= ~(1 << (i & 7));
				DR_byte[i >> 3] |= (j << (i & 7));
			}

			/* Should be in UPDATE DR */
			if (DR_pause_count) {
				udi_jtag_clock(jtag_sw_tck, 1, 0, 0,
					       DR_pause_count);
				udi_jtag_clock(jtag_sw_tck, 1, 1, 0, 2);
			} else
				udi_jtag_clock(jtag_sw_tck, 1, 1, 0, 1);
		} else
			udi_ver("WARNING: IR-Only JTAG Command\n");

		/* Return the state machine to run-test-idle */
		udi_jtag_clock(jtag_sw_tck, 1, 0, 0, 1);

	} else if (DR_bit_count) {
		udi_ver("WARNING: DR-Only JTAG Command\n");

		/* into idel mode */
		udi_jtag_clock(jtag_sw_tck, 1, 0, 0, 1);
		/* Jog the state machine arround to shift DR */
		udi_jtag_clock(jtag_sw_tck, 1, 1, 0, 1);
		udi_jtag_clock(jtag_sw_tck, 1, 0, 0, 2);

		/* Shift the DR bits, assert TMS=1 for last bit */
		for (i = 0; i < DR_bit_count; i++) {
			j = udi_jtag_clock(
			    jtag_sw_tck, 1, ((i == (DR_bit_count - 1)) ? 1 : 0),
			    (((DR_byte[i >> 3]) >> (i & 7)) & 1), 1);
			DR_byte[i >> 3] &= ~(1 << (i & 7));
			DR_byte[i >> 3] |= (j << (i & 7));
		}

		/* Should be in UPDATE DR */
		if (DR_pause_count) {
			udi_jtag_clock(jtag_sw_tck, 1, 0, 0, DR_pause_count);
			udi_jtag_clock(jtag_sw_tck, 1, 1, 0, 2);
		} else
			udi_jtag_clock(jtag_sw_tck, 1, 1, 0, 1);

		/* Return the state machine to run-test-idle */
		udi_jtag_clock(jtag_sw_tck, 1, 0, 0, 1);
	} else
		udi_error(
		    "SCAN command with #IR=0 and #DR=0. Doing nothing!\n");

#ifndef __KERNEL__
	/* Print the IR/DR readback values to STDOUT */
	printf("Channel = %d, ", jtag_sw_tck);
	if (IR_bit_count) {
		printf("IR %u = ", IR_bit_count);
		for (i = ((IR_bit_count - 1) >> 3); i >= 0; i--)
			printf("%x ", IR_byte[i]);
		printf(" ");
	}

	if (DR_bit_count) {
		printf("DR %u = ", DR_bit_count);
		for (i = ((DR_bit_count - 1) >> 3); i >= 0; i--)
			printf("%x ", DR_byte[i]);
		printf("\n");
	}
#endif

	return 0;
}

#ifdef __KERNEL__ /* __KERNEL__ */
/* Device infrastructure */
static int udi_remove(struct platform_device *pdev) { return 0; }

static int udi_probe(struct platform_device *pdev)
{
	udi_ver("UDI Initial.\n");

	return 0;
}

static int udi_suspend(struct platform_device *pdev, pm_message_t state)
{
	udi_ver("UDI suspend\n");
	return 0;
}

static int udi_resume(struct platform_device *pdev)
{
	udi_ver("UDI resume\n");
	return 0;
}

struct platform_device udi_pdev = {
	.name = "mt_udi", .id = -1,
};

static struct platform_driver udi_pdrv = {
	.remove = udi_remove,
	.shutdown = NULL,
	.probe = udi_probe,
	.suspend = udi_suspend,
	.resume = udi_resume,
	.driver = {
	    .name = "mt_udi",
	},
};

/* -----------------------------------------------------------------------------
 */

#ifdef CONFIG_PROC_FS

static char *_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;
	if (count >= PAGE_SIZE)
		goto out0;
	if (copy_from_user(buf, buffer, count))
		goto out0;

	buf[count] = '\0';
	return buf;

out0:
	free_page((unsigned long)buf);
	return NULL;
}

/* udi_debug_reg */
static int udi_reg_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "Reg[%x] = 0x%zx.\n", udi_addr_phy,
		   udi_reg_read(udi_addr_phy));
	return 0;
}

static ssize_t udi_reg_proc_write(struct file *file, const char __user *buffer,
				  size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);
	unsigned int udi_value = 0, udi_reg_msb, udi_reg_lsb;
	unsigned char udi_rw[5] = {0, 0, 0, 0, 0};

	if (!buf)
		return -EINVAL;

	/* protect udi reg read/write */
	if (func_lv_mask_udi == 0) {
		free_page((unsigned long)buf);
		return count;
	}

	if (sscanf(buf, "%1s %x %d %d %x", udi_rw, &udi_addr_phy, &udi_reg_msb,
		   &udi_reg_lsb, &udi_value) == 5) {
		/* f format or 'f', addr, MSB, LSB, value */
		udi_reg_field(udi_addr_phy, udi_reg_msb
			      : udi_reg_lsb, udi_value);
		udi_info("Read back, Reg[%x] = 0x%zx.\n", udi_addr_phy,
			 udi_reg_read(udi_addr_phy));
	} else if (sscanf(buf, "%1s %x %x", udi_rw, &udi_addr_phy,
			  &udi_value) == 3) {
		/* w format or 'w', addr, value */
		udi_reg_write(udi_addr_phy, udi_value);
		udi_info("Read back, Reg[%x] = 0x%zx.\n", udi_addr_phy,
			 udi_reg_read(udi_addr_phy));
	} else if (sscanf(buf, "%1s %x", udi_rw, &udi_addr_phy) == 2) {
		/* r format or 'r', addr */
		udi_info("Read back, aReg[%x] = 0x%zx.\n", udi_addr_phy,
			 udi_reg_read(udi_addr_phy));
	} else {
		udi_info("echo dbg_lv (dec) > /proc/udi/udi_debug\n");
		memset(udi_rw, 0, sizeof(udi_rw));
	}

	free_page((unsigned long)buf);
	return count;
}

/* udi_pinmux_switch */
static int udi_pinmux_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "UDI pinmux reg[0x1000_5330] = 0x%x.\n",
		   _udi_read(UDIPIN_UDI_JTAG1));
	seq_printf(m, "UDI pinmux reg[0x1000_5340] = 0x%x.\n",
		   _udi_read(UDIPIN_UDI_JTAG2));
	seq_printf(m, "UDI pinmux reg[0x1000_56E0] = 0x%x.\n",
		   _udi_read(UDIPIN_UDI_EN));
	return 0;
}

static ssize_t udi_pinmux_proc_write(struct file *file,
				     const char __user *buffer, size_t count,
				     loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);
	unsigned int pin_switch = 0;

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &pin_switch)) {
		if (pin_switch == 1) {
			/* 1. Write 0x1000_5330 bit[27:24]=0x3 to switch UDI
			 * pin, TCK
			 * 2. Write 0x1000_5340 bit[15:0]=0x3333 to switch UDI
			 * pin, TDI, TDO, TMS and TRST
			 * 3. 0x07600000 = UDI + DFD enable, 0x05600000 = UDI
			 * enable
			 */
			_udi_write(UDIPIN_UDI_JTAG1, 0x03000000);
			_udi_write(UDIPIN_UDI_JTAG2, 0x00003333);
			_udi_write(UDIPIN_UDI_EN,
				   _udi_read(UDIPIN_UDI_EN) | 0x07600000);
		} else {
			/* default */
			_udi_write(UDIPIN_UDI_JTAG1, 0x11100000);
			_udi_write(UDIPIN_UDI_JTAG2, 0x00001111);
			_udi_write(UDIPIN_UDI_EN, 0xf801);
		}
	} else
		udi_error("echo dbg_lv (dec) > /proc/udi/udi_debug\n");

	free_page((unsigned long)buf);
	return count;
}

/* udi_debug_info_print_flag */
static int udi_debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "UDI debug (log level) = %d.\n", func_lv_mask_udi);
	return 0;
}

static ssize_t udi_debug_proc_write(struct file *file,
				    const char __user *buffer, size_t count,
				    loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);
	unsigned int dbg_lv = 0;

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &dbg_lv))
		func_lv_mask_udi = dbg_lv;
	else
		udi_error("echo dbg_lv (dec) > /proc/udi/udi_debug\n");

	free_page((unsigned long)buf);
	return count;
}

static int udi_jtag_clock_proc_show(struct seq_file *m, void *v)
{
	int i;

	udi_jtag_clock_read();

	/* Print the IR/DR readback values to STDOUT */
	seq_printf(m, "IR %u ", IR_bit_count);
	if (IR_bit_count) {
		for (i = ((IR_bit_count - 1) >> 3); i >= 0; i--)
			seq_printf(m, "%02x", IR_byte[i]);
	} else
		seq_puts(m, "00");

	seq_printf(m, " DR %u ", DR_bit_count);
	if (DR_bit_count) {
		for (i = ((DR_bit_count - 1) >> 3); i >= 0; i--)
			seq_printf(m, "%02x", DR_byte[i]);
	} else
		seq_puts(m, "00 ");

	seq_puts(m, "\n");

	return 0;
}

/* Please find attached the example C-code for the JTAG driver.
 *   This is the function we need build into kernel space.
 *
 *In this form there are 4 calling parameters:
 *%u ¡X number of IR bits
 *%x ¡X hex string for IR bit sequence
 *%u ¡X number of DR bits
 *%x ¡X hex string for DR bit sequence
 *
 *     echo SCAN Sub-Ch IR-bit IR-hex [IR_pause_count] DR-bit DR-hex
 * [DR_pause_count] > /proc/udi/udi_jtag_clock
 *     echo SCAN %u %u %x [%u] %u %x [%u] > /proc/udi/udi_jtag_clock
 * e.q 4 parameter:
 *     echo SCAN 0 58 003ffbffbfff807 35 7c000003f > /proc/udi/udi_jtag_clock
 *    6 parameter:
 *     echo SCAN 0 58 003ffbffbfff807 12 35 7c000003f 5 >
 * /proc/udi/udi_jtag_clock
 *
 * for MT6757/Elbrus :
 *     echo SCAN 1 11 006 11 000 > udi_jtag_clock
 *     (ans: IR 11 0000 DR 11 07a8)
 *
 */

static ssize_t udi_jtag_clock_proc_write(struct file *file,
					 const char __user *buffer,
					 size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);
	unsigned int i, numdigits, length;
	unsigned int recv_buf[5];
	unsigned char recv_char[2][UDI_FIFOSIZE * 2]; /* two char is one byte */
	unsigned char recv_key_word[10];

	if (!buf)
		return -EINVAL;

	/* data initial */
	jtag_sw_tck = 0;
	IR_bit_count = 0;
	DR_bit_count = 0;
	IR_pause_count = 0;
	DR_pause_count = 0;
	memset(IR_byte, 0, sizeof(IR_byte));
	memset(DR_byte, 0, sizeof(DR_byte));

	/* input check format */
	if (sscanf(buf, "%4s %u %u %512s %u %u %512s %u", &recv_key_word[0],
		   &recv_buf[0], &recv_buf[1], recv_char[0], &recv_buf[3],
		   &recv_buf[2], recv_char[1], &recv_buf[4]) == 8) {
		/* 6 parameter */
		IR_pause_count = recv_buf[3];
		DR_pause_count = recv_buf[4];
	} else if (sscanf(buf, "%4s %u %u %512s %u %512s", &recv_key_word[0],
			  &recv_buf[0], &recv_buf[1], recv_char[0],
			  &recv_buf[2], recv_char[1]) == 6) {
		/* 4 parameter */
		IR_pause_count = 0;
		DR_pause_count = 0;
	} else if (sscanf(buf, "%5s", &recv_key_word[0]) == 1) {
		/* RESET */
		if (!strcmp(recv_key_word, "RESET")) {
			/* rest mode by TRST = 1 */
			/* jtag_clock(sw_tck, i_trst, i_tms, i_tdi, count) */
			udi_jtag_clock(jtag_sw_tck, 0, 0, 0, 4);
			goto out1;
		}
	} else {
		udi_error("echo wrong format > /proc/udi/udi_jtag_clock\n");
		goto out1;
	}

	udi_ver("Input data: SCAN = %u\n", recv_key_word[0]);
	udi_ver("Input data: Channel = %u\n", recv_buf[0]);
	udi_ver("Input data: 1 = %u\n", recv_buf[1]);
	udi_ver("Input data: 2 = %s\n", recv_char[0]);
	udi_ver("Input data: 3 = %u\n", recv_buf[3]);
	udi_ver("Input data: 4 = %u\n", recv_buf[2]);
	udi_ver("Input data: 5 = %s\n", recv_char[1]);
	udi_ver("Input data: 6 = %u\n", recv_buf[4]);

	/* chekc first key word equ "SCAN" */
	if (strcmp(recv_key_word, "SCAN")) {
		udi_error("echo wrong format > /proc/udi/udi_jtag_clock\n");
		goto out1;
	}

	/* check channel 0~3 */
	if ((recv_buf[0] < 0) || (recv_buf[0] > 3)) {
		udi_error("ERROR: Sub-Chains  ont 1~3\n");
		goto out1;
	} else {
		jtag_sw_tck = recv_buf[0];
	}

	/* chek IR/DR bit counter */
	if ((recv_buf[1] == 0) && (recv_buf[2] == 0)) {
		udi_error("ERROR: IR and DR bit all zero\n");
		goto out1;
	}

	/* Parse the IR command into a bit string, for a05 must be 9~12bits
	 * range
	 */
	if (recv_buf[1] == 0)
		udi_ver("WARNING: DR-Only JTAG Command\n");
	else if ((recv_buf[1] > (strlen(recv_char[0]) * 4)) ||
		 (recv_buf[1] < ((strlen(recv_char[0]) * 4) - 3))) {
		udi_error("ERROR: IR string too short (%zu characters not match with %u bits)\n",
			  strlen(recv_char[0]), recv_buf[1]);
		goto out1;
	} else {
		IR_bit_count = recv_buf[1];
		udi_ver("Input data: IR_bit_count = %u\n", IR_bit_count);
		/* Parse the IR command into a bit string */
		length = strlen(recv_char[0]) - 1;
		numdigits = length / 2;

		for (i = 0; i <= numdigits; i++) {
			if (length == (i << 1)) {
				IR_byte[i] =
				    CTOI(recv_char[0][length - (2 * i)]);
			} else {
				IR_byte[i] =
				    (CTOI(recv_char[0][length - (2 * i) - 1])
				     << 4) +
				    CTOI(recv_char[0][length - (2 * i)]);
			}
			udi_ver("IR[%d] = 0x%02X\n", i, IR_byte[i]);
		}
	  /* example jtag_adb 1 12 a05 30 9b6a4109, IR[0]=0x05.IR[1]=0x0a */
	}

	/* Parse the DR command into a bit string, for 9b6a4109 must be
	 * 29~32bits range
	 */
	if (recv_buf[2] == 0)
		udi_ver("WARNING: IR-Only JTAG Command\n");
	else if ((recv_buf[2] > (strlen(recv_char[1]) * 4)) ||
		 (recv_buf[2] < ((strlen(recv_char[1]) * 4) - 3))) {
		udi_error("ERROR: DR string too short (%zu hex characters not match with %u bits)\n",
			  strlen(recv_char[1]), recv_buf[2]);
		goto out1;
	} else {
		DR_bit_count = recv_buf[2];
		udi_ver("Input data: DR_bit_count = %u\n", DR_bit_count);
		/* Parse the DR command into a bit string */
		length = strlen(recv_char[1]) - 1;
		numdigits = length / 2;

		for (i = 0; i <= numdigits; i++) {
			if (length == (i << 1))
				DR_byte[i] =
				    CTOI(recv_char[1][length - (2 * i)]);
			else
				DR_byte[i] =
				    (CTOI(recv_char[1][length - (2 * i) - 1])
				     << 4) +
				    CTOI(recv_char[1][length - (2 * i)]);
			udi_ver("DR[%d] = 0x%02X\n", i, DR_byte[i]);
		}
	  /* example jtag_adb 1 12 a05 30 9b6a4109, IR[0]=0x05.IR[1]=0x0a */
	}

out1:
	free_page((unsigned long)buf);
	return count;
}

/* udi_jtag_manual */
static int udi_jtag_manual_proc_show(struct seq_file *m, void *v)
{
	int i;

	if ((IR_bit_count == 0) && (DR_bit_count == 0)) {
		seq_puts(m, "IR_bit count or DR_bit_count is zero.\n");
		seq_puts(m, "Input error! 4 parameter: echo SCAN sub_c ir_bit ir dr_bit dr > udi_jtag_clock\n");
		seq_puts(m, "             6 parameter: echo SCAN sub_c ir_bit ir ir_p dr_bit dr dr_p> udi_jtag_clock\n");
		seq_puts(m, "Example: echo SCAN 0 12 a05 30 9b6a4109 > udi_jtag_clock\n");
		seq_puts(m, "         echo SCAN 0 12 a05 5 30 9b6a4109 3 > udi_jtag_clock\n");
		return 0;
	}

	seq_puts(m,
		 "================= 2015/11/10 Ver 5.1 ==================\n");
	seq_printf(m, "UDI Jtag sub-chains = %d\n", jtag_sw_tck);

	seq_printf(m, "IR_bits_count = %u , IR_Pause_count = %u, ",
		   IR_bit_count, IR_pause_count);
	if (IR_bit_count) {
		for (i = 0; i <= (IR_bit_count >> 3); i++)
			seq_printf(m, ",IR[%d] =0x%02X ", i, IR_byte[i]);
	}
	seq_puts(m, "\n");

	seq_printf(m, "DR_bits_count = %u , DR_Pause_count = %u, ",
		   DR_bit_count, DR_pause_count);
	if (DR_bit_count) {
		for (i = 0; i <= (DR_bit_count >> 3); i++)
			seq_printf(m, ",DR[%d] =0x%02X ", i, DR_byte[i]);
	}
	seq_puts(m, "\n");
	seq_puts(m,
		 "=======================================================\n");

	udi_jtag_clock_proc_show(m, v);

	return 0;
}

static ssize_t udi_jtag_manual_proc_write(struct file *file,
					  const char __user *buffer,
					  size_t count, loff_t *pos)
{
#if 1
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	free_page((unsigned long)buf);
	return count;

#else
	char *buf = _copy_from_user_for_proc(buffer, count);

	udi_ver("Pls input by udi_jag_clock command.\n");

	free_page((unsigned long)buf);
	return count;
#endif
}

/* udi bit control */
static int udi_bit_ctrl_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m,
		   "MT6757 SW UDI: TCK=%x, TDI=%x, TMS=%x, nTRST=%x, TDO=%x\n",
		   tck_bit, tdi_bit, tms_bit, ntrst_bit, tdo_bit);

	return 0;
}

static ssize_t udi_bit_ctrl_proc_write(struct file *file,
				       const char __user *buffer, size_t count,
				       loff_t *pos)
{
	unsigned int recv[4];

	char *buf = _copy_from_user_for_proc(buffer, count);

	tck_bit = 0;
	tdi_bit = 0;
	tms_bit = 0;
	ntrst_bit = 0;
	tdo_bit = 0;

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %d %d %d", &recv[0], &recv[1], &recv[2],
		   &recv[3]) == 4) {
		tck_bit = recv[0];
		tdi_bit = recv[1];
		tms_bit = recv[2];
		ntrst_bit = recv[3];
		tdo_bit = udi_bit_ctrl(recv[0], recv[1], recv[2], recv[3]);
		udi_info(
		    "MT6757 SW UDI: TCK=%x, TDI=%x, TMS=%x, nTRST=%x, TDO=%x\n",
		    tck_bit, tdi_bit, tms_bit, ntrst_bit, tdo_bit);
	}

	free_page((unsigned long)buf);
	return count;
}

#define PROC_FOPS_RW(name)                                                     \
	static int name##_proc_open(struct inode *inode, struct file *file)    \
	{                                                                      \
		return single_open(file, name##_proc_show, PDE_DATA(inode));   \
	}                                                                      \
	static const struct file_operations name##_proc_fops = {               \
	    .owner = THIS_MODULE,                                              \
	    .open = name##_proc_open,                                          \
	    .read = seq_read,                                                  \
	    .llseek = seq_lseek,                                               \
	    .release = single_release,                                         \
	    .write = name##_proc_write,                                        \
	}

#define PROC_FOPS_RO(name)                                                     \
	static int name##_proc_open(struct inode *inode, struct file *file)    \
	{                                                                      \
		return single_open(file, name##_proc_show, PDE_DATA(inode));   \
	}                                                                      \
	static const struct file_operations name##_proc_fops = {               \
	    .owner = THIS_MODULE,                                              \
	    .open = name##_proc_open,                                          \
	    .read = seq_read,                                                  \
	    .llseek = seq_lseek,                                               \
	    .release = single_release,                                         \
	}

#define PROC_ENTRY(name)                                                       \
	{                                                                      \
		__stringify(name), &name##_proc_fops                           \
	}

PROC_FOPS_RW(udi_reg);	 /* for any register read/write */
PROC_FOPS_RW(udi_pinmux);      /* for udi pinmux switch */
PROC_FOPS_RW(udi_debug);       /* for debug information */
PROC_FOPS_RW(udi_jtag_clock);  /* for udi jtag interface */
PROC_FOPS_RW(udi_jtag_manual); /* for udi jtag mnaual ctrl */
PROC_FOPS_RW(udi_bit_ctrl);    /* for udi bit ctrl */

static int _create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
	    PROC_ENTRY(udi_reg),	 PROC_ENTRY(udi_pinmux),
	    PROC_ENTRY(udi_debug),       PROC_ENTRY(udi_jtag_clock),
	    PROC_ENTRY(udi_jtag_manual), PROC_ENTRY(udi_bit_ctrl),
	};

	dir = proc_mkdir("udi", NULL);

	if (!dir) {
		udi_error("fail to create /proc/udi @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, 0664,
				 dir, entries[i].fops))
			udi_error("%s(), create /proc/udi/%s failed\n",
				  __func__, entries[i].name);
	}

	return 0;
}

#endif /* CONFIG_PROC_FS */

/*
 * Module driver
 */
static int __init udi_init(void)
{
	int err = 0;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,gpio");
	if (!node) {
		udi_error("error: cannot find node UDI_NODE!\n");
		WARN_ON(1);
	}

	/* Setup IO addresses and printf */
	udipin_base =
	    of_iomap(node, 0); /* 0x10005000 0x1000, DFD,UDI pinmux reg */
	udi_ver("udipin_base = 0x%lx.\n", (unsigned long)udipin_base);
	if (!udipin_base) {
		udi_error("udi pinmux get some base NULL.\n");
		WARN_ON(1);
	}

	/* register platform device/driver */
	err = platform_device_register(&udi_pdev);

	/* initial value */
	func_lv_mask_udi = 0;
	IR_bit_count = 0;
	DR_bit_count = 0;
	jtag_sw_tck = 1; /* default debug channel = 1 */

	if (err) {
		udi_error("fail to register UDI device @ %s()\n", __func__);
		goto out2;
	}

	err = platform_driver_register(&udi_pdrv);
	if (err) {
		udi_error("%s(), UDI driver callback register failed..\n",
			  __func__);
		return err;
	}

/* ver = mt_get_chip_sw_ver(); */
/* ptp2_lo_enable = 1; */
/* turn_on_LO(); */

#ifdef CONFIG_PROC_FS
	/* init proc */
	if (_create_procfs()) {
		err = -ENOMEM;
		goto out2;
	}
#endif /* CONFIG_PROC_FS */

out2:
	return err;
}

static void __exit udi_exit(void)
{
	udi_info("UDI de-initialization\n");
	platform_driver_unregister(&udi_pdrv);
	platform_device_unregister(&udi_pdev);
}

module_init(udi_init);
module_exit(udi_exit);

MODULE_DESCRIPTION("MediaTek UDI Driver v0.1");
MODULE_LICENSE("GPL");
#endif /* __KERNEL__ */
#undef __MT_UDI_C__
