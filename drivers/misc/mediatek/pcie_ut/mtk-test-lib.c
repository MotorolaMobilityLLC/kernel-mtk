/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#include <linux/kernel.h>       /* printk() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <asm/unaligned.h>
#include <linux/kthread.h>
#include <linux/pci.h>
#include <linux/pci-aspm.h>
#include <linux/delay.h>




#include "mtk-pcie.h"
#include "mtk-pci.h"
#include "mtk-dma.h"
#include "mtk-test-lib.h"

/*****************************************************************************
 *
 * PCIe Unit Test
 *
 *****************************************************************************/


#define aspm_num 4
int max_payload_size[6] = {128, 256, 512, 1024, 2048, 4096};

int f_register;

int t_pcie_register_pci(int argc, char **argv)
{
	int ret;

	ret = mtk_register_pci();
	f_register = 1;
	return ret;
}

int t_pcie_unregister_pci(int argc, char **argv)
{
	if (f_register) {
		mtk_unregister_pci();
		f_register = 0;
	}
	return 0;
}

int t_pcie_mmio(int argc, char **argv)
{
	int iterations, ret = 0, i, type;

	iterations = 1;
	type = MTK_MEM_READ;

	if (argc > 1) {
		if (!strcmp(argv[1], "read")) {
			pr_err("Test Memeoy Read\n");
			type = MTK_MEM_READ;
		} else if (!strcmp(argv[1], "write")) {
			pr_err("Test Memeoy Write\n");
			type = MTK_MEM_WRITE;
		}
	}

	if (argc > 2) {
		ret = kstrtoint(argv[2], 10, &iterations);
		pr_err("Iterations : %d\n", iterations);
	}

	for (i = 0;  i < iterations; i++) {
		pr_err("Iterations : %d\n", iterations);
		ret = mtk_mmio(type);
		if (ret) {
			pr_err("can't map dma buffer\n");
			return ret;
		}
	}

	return ret;
}

int t_pcie_setup_int(int argc, char **argv)
{
	int ret, type;

	type = TYPE_INTx;

	if (argc > 1) {
		if (!strcmp(argv[1], "MSI")) {
			pr_debug("Test Interrupt MSI\n");
			type = TYPE_MSI;
		}
		if (!strcmp(argv[1], "MSIX")) {
			pr_debug("Test Interrupt MSIX\n");
			type = TYPE_MSIX;
		}
		if (!strcmp(argv[1], "INTX")) {
			pr_debug("Test Interrupt INTX\n");
			type = TYPE_INTx;
		}
	}

	ret = mtk_setup_int(type);
	return ret;
}

int t_pcie_aspm(int argc, char **argv)
{
	int ret;
	struct mkt_aspm_state aspm = {0};

	if (argc > 1) {
		if (!strcmp(argv[1], "L0s")) {
			pr_debug("Test DS L0s aspm\n");
			aspm.aspm_ds = LINK_STATE_L0S;
		}
		if (!strcmp(argv[1], "L1")) {
			pr_debug("Test DS L1 aspm\n");
			aspm.aspm_ds = LINK_STATE_L1;
		}
		if (!strcmp(argv[1], "both")) {
			pr_debug("Test DS L0s/L1 aspm\n");
			aspm.aspm_ds = (LINK_STATE_L0S | LINK_STATE_L1);
		}
		if (!strcmp(argv[1], "disable")) {
			pr_debug("Disable DS aspm\n");
			aspm.aspm_ds = 0;
		}
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "L0s")) {
			pr_debug("Test US L0s aspm\n");
			aspm.aspm_us = LINK_STATE_L0S;
		}
		if (!strcmp(argv[2], "L1")) {
			pr_debug("Test US L1 aspm\n");
			aspm.aspm_us = LINK_STATE_L1;
		}
		if (!strcmp(argv[2], "both")) {
			pr_debug("Test US L0s/L1 aspm\n");
			aspm.aspm_us = (LINK_STATE_L0S | LINK_STATE_L1);
		}
		if (!strcmp(argv[2], "disable")) {
			pr_debug("Disable US aspm\n");
			aspm.aspm_us = 0;
		}
	}

	ret = mtk_pcie_aspm(aspm);
	return ret;
}

int t_pcie_retrain(int argc, char **argv)
{
	int iterations, ret, i;

	iterations = 1;

	if (argc > 1) {
		ret = kstrtoint(argv[1], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	for (i = 0;  i < iterations; i++) {
		ret = mtk_pcie_retrain();
		if (ret)
			return ret;

	}

	return 0;
}

int t_pcie_aspm_retrain(int argc, char **argv)
{
	int ret, iterations, i;
	struct mkt_aspm_state aspm = {0}, aspm1;
	u8 count;

	iterations = 1;

	if (argc > 1) {
		ret = kstrtoint(argv[1], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "L0s")) {
			pr_debug("Test DS L0s aspm\n");
			aspm.aspm_ds = LINK_STATE_L0S;
		}
		if (!strcmp(argv[2], "L1")) {
			pr_debug("Test DS L1 aspm\n");
			aspm.aspm_ds = LINK_STATE_L1;
		}
		if (!strcmp(argv[2], "both")) {
			pr_debug("Test DS L0s/L1 aspm\n");
			aspm.aspm_ds = (LINK_STATE_L0S | LINK_STATE_L1);
		}
		if (!strcmp(argv[2], "disable")) {
			pr_debug("Disable DS aspm\n");
			aspm.aspm_ds = 0;
		}
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "L0s")) {
			pr_debug("Test US L0s aspm\n");
			aspm.aspm_us = LINK_STATE_L0S;
		}
		if (!strcmp(argv[3], "L1")) {
			pr_debug("Test US L1 aspm\n");
			aspm.aspm_us = LINK_STATE_L1;
		}
		if (!strcmp(argv[3], "both")) {
			pr_debug("Test US L0s/L1 aspm\n");
			aspm.aspm_us = (LINK_STATE_L0S | LINK_STATE_L1);
		}
		if (!strcmp(argv[3], "disable")) {
			pr_debug("Disable US aspm\n");
			aspm.aspm_us = 0;
		}
	}

	for (i = 0;  i < iterations; i++) {

		ret = mtk_pcie_aspm(aspm);
		if (ret) {
			pr_err("can't enable pcie aspm\n");
			return ret;
		}

		get_random_bytes(&count, 1);
		mdelay(count);
		pr_debug("delay count : %d\n", count);

		ret = mtk_pcie_retrain();
		if (ret)
			return ret;

		aspm1.aspm_ds = 0;
		aspm1.aspm_us = 0;
		ret = mtk_pcie_aspm(aspm1);
	}

	return 0;
}

int t_pcie_pcipm(int argc, char **argv)
{
	int ret, iterations, i, pcipm, exit;
	u8 count;

	iterations = 1;

	if (argc > 1) {
		ret = kstrtoint(argv[1], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	pcipm = LINK_STATE_L1;

	if (argc > 2) {
		/* L1 EP exit: PME#  RC exit:Recovery */
		if (!strcmp(argv[2], "L1")) {
			pr_debug("Test L1 PCI-PM\n");
			pcipm = LINK_STATE_L1;
		}
		/* L2 EP exit:Beacon, #WAKE,( PME#)  RC exit:PERST# */
		if (!strcmp(argv[2], "L2")) {
			pr_debug("Test L2 PCI-PM\n");
			pcipm = LINK_STATE_L2;
		}
	}

	exit = EXIT_RC;

	if (argc > 3) {
		if (!strcmp(argv[3], "EXIT_RC")) {
			pr_debug("Test PCI-PM Exit by RC\n");
			exit = EXIT_RC;
		}
		if (!strcmp(argv[3], "EXIT_EP")) {
			pr_debug("Test PCI-PM Exit by EP\n");
			exit = EXIT_EP;
		}
	}

	for (i = 0;  i < iterations; i++) {
		if (pcipm == LINK_STATE_L1) {
			if (exit == EXIT_EP)
				mtk_enable_pme();

			ret = mtk_pcie_go_l1();
			if (ret) {
				pr_err("can't enable pci-pm L1\n");
				return ret;
			}

			get_random_bytes(&count, 1);
			pr_debug("delay :%d (ms)\n", count);
			msleep(count);

			if (exit == EXIT_RC) {
				if (pcipm == LINK_STATE_L1) {
					ret = mtk_pcie_wakeup();
					if (ret) {
						pr_err("can't enable pci-pm L1 exit\n");
						return ret;
					}
				}
			}

			if (exit == EXIT_EP) {
				if (pcipm == LINK_STATE_L1) {
					ret = mtk_wait_wakeup();
					if (ret) {
						pr_err("can't wakeup by PME#\n");
						return ret;
					}
				}
			}
		}

		if (pcipm == LINK_STATE_L2) {
			ret = mtk_pcie_l2(exit);
			if (ret) {
				pr_err("can't enable pci-pm L2\n");
				return ret;
			}
		}
	}
	return 0;
}

int t_pcie_l1pm(int argc, char **argv)
{
	int ret, enable = 0;

	if (argc > 1) {
		if (!strcmp(argv[1], "enable")) {
			pr_debug("Enable L1 PM\n");
			enable = 1;
		} else {
			pr_debug("Disable L1 PM\n");
			enable = 0;
		}
	}

	ret = mtk_setup_l1pm(enable);

	return ret;
}

int t_pcie_l1ss(int argc, char **argv)
{
	int ret, ds_mode, us_mode, aspm_mode;

	aspm_mode = 1;
	ds_mode = 0;
	us_mode = 0;

	if (argc > 1) {
		if (!strcmp(argv[1], "PCIPM")) {
			pr_debug("Test L1 PM Substates PCI-PM\n");
			aspm_mode = 0;
		}
		if (!strcmp(argv[1], "ASPM")) {
			pr_debug("Test L1 PM Substates ASPM\n");
			aspm_mode = 1;
		}
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "L11")) {
			pr_debug("Enable L1.1 (DS)\n");
			ds_mode |= (aspm_mode) ? L1SS_ASPM_L11 : L1SS_PCIPM_L11;
		}
		if (!strcmp(argv[2], "L12")) {
			pr_debug("Enable L1.2 (DS)\n");
			ds_mode |= (aspm_mode) ? L1SS_ASPM_L12 : L1SS_PCIPM_L12;
		}
		if (!strcmp(argv[2], "both")) {
			pr_debug("Enable L1.1 & L1.2 (DS)\n");
			ds_mode |= (aspm_mode) ?
				(L1SS_ASPM_L11 | L1SS_ASPM_L12) :
				(L1SS_PCIPM_L11 | L1SS_PCIPM_L12);
		}
		if (!strcmp(argv[2], "disable")) {
			pr_debug("Disable L1 PM Substates (DS)\n");
			ds_mode = 0;
		}
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "L11")) {
			pr_debug("Enable L1.1 (US)\n");
			us_mode |= (aspm_mode) ? L1SS_ASPM_L11 : L1SS_PCIPM_L11;
		}
		if (!strcmp(argv[3], "L12")) {
			pr_debug("Enable L1.2 (US)\n");
			us_mode |= (aspm_mode) ? L1SS_ASPM_L12 : L1SS_PCIPM_L12;
		}
		if (!strcmp(argv[3], "both")) {
			pr_debug("Enable L1.1 & L1.2 (US)\n");
			us_mode |= (aspm_mode) ?
				(L1SS_ASPM_L11 | L1SS_ASPM_L12) :
				(L1SS_PCIPM_L11 | L1SS_PCIPM_L12);
		}
		if (!strcmp(argv[3], "disable")) {
			pr_debug("Disable L1 PM Substates (US)\n");
			us_mode = 0;
		}
	}

	ret = mtk_setup_l1ss(ds_mode, us_mode);

	return ret;
}

int t_pcie_reset(int argc, char **argv)
{
	u32 iterations, ret = 0, i, type;

	iterations = 1;
	type = MTK_HOT_RESET;

	if (argc > 1) {
		if (!strcmp(argv[1], "hot")) {
			pr_debug("Test Hot Reset\n");
			type = MTK_HOT_RESET;
		} else if (!strcmp(argv[1], "flr")) {
			pr_debug("Test Function Level Reset\n");
			type = MTK_FLR;
		} else if (!strcmp(argv[1], "disable")) {
			pr_debug("Test Disable Link\n");
			type = MTK_DISABLED;
		} else if (!strcmp(argv[1], "warm")) {
			pr_debug("Test Warm Reset\n");
			type = MTK_WARM_RESET;
		}
	}

	if (argc > 2) {
		ret = kstrtoint(argv[2], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	for (i = 0;  i < iterations; i++) {
		if (type == MTK_HOT_RESET)
			ret = mtk_pcie_hot_reset();

		if (type == MTK_DISABLED)
			ret = mtk_pcie_disabled();

		if (type == MTK_FLR)
			ret = mtk_function_level_reset();

		if (type == MTK_WARM_RESET)
			ret = mtk_pcie_ut_warm_reset();

		if (ret) {
			pr_err("iterations = %d\n", i);
			return ret;
		}
	}

	return 0;
}

int t_pcie_change_speed(int argc, char **argv)
{
	int ret, speed;

	speed = MTK_SPEED_2_5GT;

	if (argc > 1) {
		if (!strcmp(argv[1], "5G")) {
			pr_debug("Test Speed 5G\n");
			speed = MTK_SPEED_5_0GT;
		} else {
			pr_debug("Test Speed 2.5G\n");
			speed = MTK_SPEED_2_5GT;
		}
	}

	ret = mtk_pcie_change_speed(speed);
	if (ret) {
		pr_err("can't change to target speed\n");
		return ret;
	}

	return 0;
}


int t_pcie_speed_test(int argc, char **argv)
{
	int ret, speed, iterations = 1, i;

	speed = MTK_SPEED_2_5GT;

	if (argc > 1) {
		ret = kstrtoint(argv[1], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	for (i = 0;  i < iterations * 2; i++) {
		ret = mtk_pcie_change_speed(speed);
		if (ret) {
			pr_err("can't change to target speed\n");
			return ret;
		}
		speed = (speed == MTK_SPEED_2_5GT) ?
			MTK_SPEED_5_0GT : MTK_SPEED_2_5GT;
	}

	return 0;
}


int t_pcie_configuration(int argc, char **argv)
{
	int iterations = 1, ret, i;

	if (argc > 1) {
		ret = kstrtoint(argv[1], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}
	for (i = 0;  i < iterations; i++) {
		ret = mtk_pcie_configuration();
		if (ret)
			return ret;

	}
	return 0;
}

int t_pcie_link_width(int argc, char **argv)
{
	int iterations = 1, ret, i;

	if (argc > 1) {
		ret = kstrtoint(argv[1], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	for (i = 0;  i < iterations; i++) {
		ret = mtk_pcie_link_width();
		if (ret)
			return ret;

	}

	return 0;
}

int t_pcie_reset_config(int argc, char **argv)
{
	int iterations = 1, ret, i;

	if (argc > 1) {
		ret = kstrtoint(argv[1], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	for (i = 0;  i < iterations; i++) {
		ret = mtk_pcie_reset_config();
		if (ret)
			return ret;

	}

	return 0;
}

int t_pcie_memory(int argc, char **argv)
{
	int iterations, ret, i, type, length;
	unsigned char ran;

	iterations = 1;
	length = 1024;
	type = MTK_MEM_READ;

	if (argc > 1) {
		if (!strcmp(argv[1], "read")) {
			pr_debug("Test Memeoy Read\n");
			type = MTK_MEM_READ;
		} else if (!strcmp(argv[1], "write")) {
			pr_debug("Test Memeoy Write\n");
			type = MTK_MEM_WRITE;
		}
	}

	if (argc > 2) {
		ret = kstrtoint(argv[2], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	for (i = 0;  i < iterations; i++) {
		get_random_bytes(&ran, 1);
		ran = (!ran) ? 1 : ran;
		length = (ran << 6) + ran;
		length = (length > MAX_DMA_LENGTH * 5) ?
			MAX_DMA_LENGTH * 5 : length;
#if 0
		ret = mtk_pcie_prepare_dma(type, length);
		if (ret)	{
			pr_err("can't map dma buffer\n");
			return ret;
		}
		ret = mtk_pcie_memory(type);
		if (ret) {
			pr_err("can't configure dma\n");
			return ret;
		}
		ret = wait_dma_done();
		if (ret)	{
			pr_err("dma timeout\n");
			return ret;
		}
		ret = mtk_pcie_dma_end(type);
		if (ret)	{
			pr_err("can't unmap dma buffer\n");
			return ret;
		}
#endif
	}

	return 0;
}

int t_pcie_loopback(int argc, char **argv)
{
	int type, ret = 0, random = 0;
	u32 packet_length, iterations, i;

	if (argc > 1) {
		ret = kstrtoint(argv[1], 10, &type);
		pr_debug("type :%d\n", type);
	}
	if (argc > 2) {
		/* packet_length : user defined or 0 means random. */
		ret = kstrtoint(argv[2], 10, &packet_length);
		pr_debug("packet_length : %d\n", packet_length);
	}
	if (argc > 3) {
		ret = kstrtoint(argv[3], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	if (packet_length == 0)
		random = 1;

	for (i = 0; i < iterations; i++) {

		if (random) {
			get_random_bytes(&packet_length, 4);
			packet_length = (packet_length % (MAX_DMA_LENGTH - 8)) + 8;
		}

		mtk_gen_tx_packet(packet_length);

		ret = mtk_transmit_tx_packet(packet_length);
		if (ret)
			break;

		ret = mtk_receive_rx_packet(packet_length);
		if (ret)
			break;

		ret = mtk_compare_txrx_packet(packet_length);
		if (ret)
			break;
	}

	if (ret)
		pr_err("length = %x, iteration = %d\n", packet_length, i);


	return ret;
}

int t_pcie_set_own(int argc, char **argv)
{
	int ret;

	ret = mtk_set_own();

	return ret;
}

int t_pcie_clear_own(int argc, char **argv)
{
	int ret;

	ret = mtk_clear_own();

	return ret;
}

int t_pcie_loopback_cr4(int argc, char **argv)
{
	int ret;
	int aspm_mode = 0;
	int packet_length = 0;
	int data_mode = 0;
	u32  iterations = 0;

	if (argc > 1) {
		ret = kstrtoint(argv[1], 10, &aspm_mode);
		pr_debug("aspm_mode :%d\n", aspm_mode);
	}
	if (argc > 2) {
		/* [1~0x3FFF] */
		ret = kstrtoint(argv[2], 10, &packet_length);
		pr_debug("packet_length : %d\n", packet_length);
	}
	if (argc > 3) {
		/* 0:random length, 1:fixed-length */
		ret = kstrtoint(argv[3], 10, &data_mode);
		pr_debug("data_mode : %d\n", data_mode);
	}
	if (argc > 4) {
		ret = kstrtoint(argv[4], 10, &iterations);
		pr_debug("Iterations : %d\n", iterations);
	}

	if (!packet_length) {
		pr_err("fail! packet_length is 0\n");
		return -1;
	}
	if (data_mode != 0 && data_mode != 1) {
		pr_err("fail! dma_mode = 0:random length, 1:fixed-length\n");
		return -1;
	}
#if 0
	ret = gen_packet(data_mode, packet_length);
	if (ret) {
		pr_err("fail! Gen test packet\n");
		goto release;
	}

	ret = init_pdma();
	if (ret) {
		pr_err("fail! Init_PDMA\n");
		goto release;
	}

	ret = transmit_tx_packet(iterations, data_mode);
	if (ret) {
		pr_err("fail! Transmit_Tx_Packet\n");
		goto release;
	}
release:
#endif

	return ret;
}


int t_pcie_test(int argc, char **argv)
{
	int ret;

	ret = mtk_pcie_test();

	return ret;
}

