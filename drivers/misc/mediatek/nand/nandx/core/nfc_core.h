/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */
#ifndef __NFC_CORE_H__
#define __NFC_CORE_H__

#include "nandx_util.h"
#include "nandx_info.h"
#include "nandx_device_info.h"

enum WAIT_TYPE {
	IRQ_WAIT_RB,
	POLL_WAIT_RB,
	POLL_WAIT_TWHR2,
};

/**
 * struct nfc_format - nand info from spec
 * @pagesize: nand page size
 * @oobsize: nand oob size
 * @ecc_strength: spec required ecc strength per 1KB
 */
struct nfc_format {
	u32 page_size;
	u32 oob_size;
	u32 ecc_strength;
};

struct nfc_handler {
	u32 sector_size;
	u32 spare_size;
	u32 fdm_size;
	u32 fdm_ecc_size;
	/* ecc strength per sector_size */
	u32 ecc_strength;
	void (*send_command)(struct nfc_handler *, u8);
	void (*send_address)(struct nfc_handler *, u32, u32, u32, u32);
	int (*write_page)(struct nfc_handler *, u8 *, u8 *);
	void (*write_byte)(struct nfc_handler *, u8);
	int (*read_sectors)(struct nfc_handler *, int, u8 *, u8 *);
	u8 (*read_byte)(struct nfc_handler *);
	int (*change_interface)(struct nfc_handler *, enum INTERFACE_TYPE,
				 struct nand_timing *, void *);
	int (*change_mode)(struct nfc_handler *, enum OPS_MODE_TYPE, bool,
			    void *);
	bool (*get_mode)(struct nfc_handler *, enum OPS_MODE_TYPE);
	void (*select_chip)(struct nfc_handler *, int);
	void (*set_format)(struct nfc_handler *, struct nfc_format *);
	void (*enable_randomizer)(struct nfc_handler *, u32, bool);
	void (*disable_randomizer)(struct nfc_handler *);
	int (*wait_busy)(struct nfc_handler *, int, enum WAIT_TYPE);
	int (*calculate_ecc)(struct nfc_handler *, u8 *, u8 *, u32, u8);
	int (*correct_ecc)(struct nfc_handler *, u8 *, u32, u8);
	int (*calibration)(struct nfc_handler *);
	int (*suspend)(struct nfc_handler *);
	int (*resume)(struct nfc_handler *);
};

extern struct nfc_handler *nfc_init(struct nfc_resource *res);
extern void nfc_exit(struct nfc_handler *handler);

#endif
