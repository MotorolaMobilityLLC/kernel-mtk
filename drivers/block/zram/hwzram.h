/*
 *  Hardware Compressed RAM offload driver
 *
 *  Copyright (C) 2015 The Chromium OS Authors
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef _HWZRAM_H_
#define _HWZRAM_H_

#include <linux/types.h>

/* Hardware related definitions */

/* fixed 4096 byte block size */
#define UNCOMPRESSED_BLOCK_SIZE_LOG2 12
#define UNCOMPRESSED_BLOCK_SIZE (1 << UNCOMPRESSED_BLOCK_SIZE_LOG2)

/* possible sizes are 64, 128, 256, 512, 1024, 2048, 4096 */
#define ZRAM_NUM_OF_POSSIBLE_SIZES 7

/* useful constant for converting sizes to indices */
#define ZRAM_MIN_SIZE_SHIFT 6
#define ZRAM_MIN_BLOCK_SIZE	(1 << ZRAM_MIN_SIZE_SHIFT)

/* 6 blocks are supplied in the command */
#define ZRAM_NUM_OF_FREE_BLOCKS 6

/* Max compression fifo size */
#define HWZRAM_MAX_FIFO_SIZE 32768

/* Max number of buffers used */
#define HWZRAM_MAX_BUFFERS_USED 4

/* These are the possible values for the status field from the specification */
enum desc_status {
	/* descriptor not in use */
	HWZRAM_IDLE           = 0x0,

	/* descriptor completed with compressed bytes written to target */
	HWZRAM_COMPRESSED     = 0x1,

	/*
	* descriptor completed, incompressible page, uncompressed bytes written to
	* target
	*/
	HWZRAM_COPIED         = 0x2,

	/* descriptor completed, incompressible page, nothing written to target */
	HWZRAM_ABORT          = 0x3,

	/* descriptor completed, page was all zero, nothing written to target */
	HWZRAM_ZERO           = 0x4,

	/*
	* descriptor count not be completed dut to an error.
	* queue operation continued to next descriptor
	*/
	HWZRAM_ERROR_CONTINUE = 0x5,

	/*
	* descriptor count not be completed dut to an error.
	* queue operation halted
	*/
	HWZRAM_ERROR_HALTED   = 0x6,

	/* descriptor in queue or being processed by hardware */
	HWZRAM_PEND           = 0x7,
};

/* a type 0 compression descriptor */
struct compr_desc_0 {
	/* word 0 */
	/* 6 bits indicating which of the 6 buffers were used */
	unsigned int buf_sel:6;

	/* 10 bits reserved for future use*/
	unsigned int reserved1:10;

	/* 16 bits for compressed length*/
	uint16_t compr_len;

	/* 32 bits for the hash */
	uint32_t hash;

	/* word 1, status and interrupt request are in the lower 12 bits */
	union {
		struct {
			/* 1 bit for interrupt request */
			unsigned int intr_request:1;

			/* 3 bits for status */
			enum desc_status status:3;

			/* pad out the rest of the word */
			uint64_t padding:60;
		} s1;
#define ZRAM_COMPR_DESC_0_SRC_MASK ~(0xFULL)
		uint64_t src_addr;
	} u1;

	/* words 2 - 5, destination buffers */
	uint64_t dst_addr[ZRAM_NUM_OF_FREE_BLOCKS];
};

struct compr_desc_1 {
	/* 32-bit word 0 */
	union {
		uint32_t hash;
		uint32_t error_bits;
	} u1;

	/* 32-bit word 1 */
	union {
		unsigned int src_addr:23;
		struct {
			unsigned int buf_sel:6;           /* [5:0] */
			unsigned int reserved1:6;         /* [11:6] */
			unsigned int compressed_size:13;  /* [24:12] */
			unsigned int reserved2:1;         /* [25] */
			unsigned int zero:1;              /* [26] */
			unsigned int interrupt_request:1; /* [27] */
			unsigned int status:3;            /* [30:28] */
		} status;
		/* avoid incomplete assignment issue (ex. strb) */
		uint32_t value;
	} u2;

	/* 32-bit words 2 - 7 */
	uint32_t dst_addr[ZRAM_NUM_OF_FREE_BLOCKS];
};

/* Type 1 descriptors pack the destination addresses in more tightly */
#define ZRAM_TYPE1_DEST_SHIFT 5

#define COMPR_DESC_0_SIZE sizeof(struct compr_desc_0)
#define COMPR_DESC_1_SIZE sizeof(struct compr_desc_1)

/* Type1 descriptor bit fields for 0x4 */
#define COMPR_DESC_1_COPY_ENABLE	0x80000000
#define COMPR_DESC_1_STATUS_SHIFT	28ULL
#define COMPR_DESC_1_INTR_SHIFT		27ULL

/*
 * We're using a scheme for encoding the size of an aligned buffer
 * by setting the bit equal to size/2 which is smaller than the least
 * significant bit of the address.  So, let's have a few macros to
 * convert to and from the encoded addresses.
 */

/* Convert from an encoded address to a size */
#define ENCODED_ADDR_TO_SIZE(addr) (((addr) & ~((addr) - 1)) << 1)

/* Convert from an encoded address to a physical address */
#define ENCODED_ADDR_TO_PHYS(addr) ((addr) & ((addr) - 1))

/* Convert from a physical address and size to an encoded address */
#define PHYS_ADDR_TO_ENCODED(addr, size) ((addr) | (1ull << (ffs(size) - 2)))

/* register definitions */

#define HWZRAM_REGS_SIZE 0x2000

/* compression register bank offset */
/* decompression register bank offset */
#define ZRAM_DECOMP_OFFSET	0x1000

#define ZRAM_HWID                0x000
#define ZRAM_HWFEATURES          0x008
#define ZRAM_HWFEATURES2         0x010
#define ZRAM_GCTRL               0x030
#define ZRAM_INTRP_STS_ERROR     0x040
#define ZRAM_INTRP_MASK_ERROR    0x048
#define ZRAM_INTRP_STS_CMP       0x050
#define ZRAM_INTRP_MASK_CMP      0x058
#define ZRAM_INTRP_STS_DCMP      (ZRAM_DECOMP_OFFSET + 0x060)
#define ZRAM_INTRP_MASK_DCMP     (ZRAM_DECOMP_OFFSET + 0x068)
#define ZRAM_ERR_COND            0x080
#define ZRAM_ERR_ADDR            0x088
#define ZRAM_ERR_CMD             0x090
#define ZRAM_ERR_CNT             0x098
#define ZRAM_ERR_MSK             0x0A0
#define ZRAM_PMON_0              0x0C0
#define ZRAM_PMON_1              0x0C8
#define ZRAM_PMON_2              0x0D0
#define ZRAM_PMON_0_CFG          0x0E0
#define ZRAM_PMON_1_CFG          0x0E8
#define ZRAM_PMON_2_CFG          0x0F0
#define ZRAM_BUSCFG              0x100
#define ZRAM_BUSCFG2             0x108
#define ZRAM_PROT                0x110
#define ZRAM_INTCFG              0x118
#define ZRAM_BUSCFG3             0x120
#define ZRAM_DBGCTRL             0x200
#define ZRAM_DBGSEL              0x208
#define ZRAM_DBGBUF              0x210
#define ZRAM_DBGSTMBUF           0x218
#define ZRAM_DBGMEMADDR          0x220
#define ZRAM_DBGMASK             0x228
#define ZRAM_DBGMATCH            0x230
#define ZRAM_DBGTRIGMASK         0x238
#define ZRAM_DBGTRIGMATCH        0x240

#define SHIFT_AND_MASK(val, shift, mask) \
	(((val) >> (uint64_t)shift) & (uint64_t)(mask))

/* meanings of some of the register bits for global */
#define ZRAM_FEATURES2_DESC_FIXED_SHIFT        21ULL
#define ZRAM_FEATURES2_BUF_MAX_SHIFT          16ULL
#define ZRAM_FEATURES2_BUF_MAX_MASK          0x7ULL
#define ZRAM_FEATURES2_DECOMPR_CMDS_SHIFT      8ULL
#define ZRAM_FEATURES2_DECOMPR_CMDS_MASK    0xFFULL
#define ZRAM_FEATURES2_DESC_TYPE_SHIFT         5ULL
#define ZRAM_FEATURES2_DESC_TYPE_MASK        0x7ULL
#define ZRAM_GCTRL_RESET_SHIFT                 0ULL

#define ZRAM_ERR_COND_ERROR_OVERFLOW_SHIFT   36ULL
#define ZRAM_ERR_COND_ERROR_VEC_SHIFT         8ULL
#define ZRAM_ERR_COND_ERR_CNT_OV_SHIFT        7ULL
#define ZRAM_ERR_COND_Q_HALT_SHIFT            6ULL
#define ZRAM_ERR_COND_D_CMD_ERR_SHIFT         5ULL
#define ZRAM_ERR_COND_C_CMD_ERR_SHIFT         4ULL
#define ZRAM_ERR_COND_COR_SHIFT               3ULL
#define ZRAM_ERR_COND_UNC_SHIFT               2ULL
#define ZRAM_ERR_COND_DEV_FATAL_SHIFT         1ULL
#define ZRAM_ERR_COND_SYS_FATAL_SHIFT         0ULL
#define ZRAM_ERR_ADDR_LOG_ADDR_MASK       0xFFFFFFFFC0LL
#define ZRAM_ERR_ADDR_LOG_ADDR_VLD_SHIFT      1ULL
#define ZRAM_ERR_ADDR_LOG_BUSY_SHIFT          0ULL
#define ZRAM_ERR_CMD_LOG_BUS_ID_MASK       0xFFULL
#define ZRAM_ERR_CMD_LOG_BUS_ID_SHIFT        40ULL
#define ZRAM_ERR_CMD_LOG_CMD_MASK          0xFFULL
#define ZRAM_ERR_CMD_LOG_CMD_SHIFT           32ULL
#define ZRAM_ERR_CMD_LOG_INDEX_MASK      0xFFFFULL
#define ZRAM_ERR_CMD_LOG_INDEX_SHIFT         16ULL
#define ZRAM_ERR_CMD_LOG_BARRIER_SHIFT        9ULL
#define ZRAM_ERR_CMD_LOG_BUS_ID_VLD_SHIFT     8ULL
#define ZRAM_ERR_CMD_LOG_INDEX_VLD_SHIFT      7ULL
#define ZRAM_ERR_CMD_LOG_CMD_VLD_SHIFT        6ULL
#define ZRAM_ERR_CMD_LOG_PAYLOAD_SHIFT        5ULL
#define ZRAM_ERR_CMD_LOG_DESCR_SHIFT          4ULL
#define ZRAM_ERR_CMD_LOG_WRITE_SHIFT          3ULL
#define ZRAM_ERR_CMD_LOG_READ_SHIFT           2ULL
#define ZRAM_ERR_CMD_LOG_COMPRESS_SHIFT       1ULL
#define ZRAM_ERR_CMD_LOG_DECOMPRESS_SHIFT     0ULL

/* bits of error vec and error overflow in google implementation */
#define ZRAM_ERR_DBG_BIT              9ULL
#define ZRAM_ERR_DSIZE_BIT            8ULL
#define ZRAM_ERR_DENGINE_BIT          7ULL
#define ZRAM_ERR_BAD_DESC_BIT         6ULL
#define ZRAM_ERR_DEC_BIT              5ULL
#define ZRAM_ERR_SLV_BIT              4ULL
#define ZRAM_ERR_D_PIPE_TO_BIT        3ULL
#define ZRAM_ERR_C_PIPE_TO_BIT        2ULL
#define ZRAM_ERR_UR_BIT               1ULL
#define ZRAM_ERR_BUS_TO_BIT           0ULL

#define ZRAM_HWID_VENDOR(val)			SHIFT_AND_MASK(val, 16, 0xFFFF)
#define ZRAM_HWID_DEVICE(val)			SHIFT_AND_MASK(val,  0, 0xFFFF)
#define ZRAM_FEATURES2_RESULT_WRITE(val)	SHIFT_AND_MASK(val, 23, 0x1)
#define ZRAM_FEATURES2_REG_ACCESS_WIDE(val)	SHIFT_AND_MASK(val, 22, 0x1)
#define ZRAM_FEATURES2_DESC_FIXED(val)		SHIFT_AND_MASK(val, 21, 0x1)
#define ZRAM_FEATURES2_WR_COHERENT(val)		SHIFT_AND_MASK(val, 20, 0x1)
#define ZRAM_FEATURES2_RD_COHERENT(val)		SHIFT_AND_MASK(val, 19, 0x1)
#define ZRAM_FEATURES2_BUF_MAX(val)		SHIFT_AND_MASK(val, 16, 0x7)
#define ZRAM_FEATURES2_DECOMPR_CMDS(val)	SHIFT_AND_MASK(val,  8, 0xFF)
#define ZRAM_FEATURES2_DESC_TYPE(val)		SHIFT_AND_MASK(val,  5, 0x7)
#define ZRAM_FEATURES2_FIFOS(val)		SHIFT_AND_MASK(val,  0, 0x1F)

#define ZRAM_VENDOR_ID_GOOGLE   1
#define ZRAM_VENDOR_ID_MEDIATEK 0

/* compression related */
#define ZRAM_CDESC_LOC     0x400
#define ZRAM_CDESC_WRIDX   0x408
#define ZRAM_CDESC_RDIDX   0x410
#define ZRAM_CDESC_CTRL    0x418
#define ZRAM_CINTERP_CTRL  0x420
#define ZRAM_CINTERP_TIMER 0x428

#define ZRAM_CINTERP_CTRL_INTR_ON	(0x1 << 16)
#define ZRAM_CINTERP_CTRL_INTR_OFF	(0x0)
#define ZRAM_CINTERP_CTRL_INDEX_MASK	(ZRAM_CINTERP_CTRL_INTR_ON - 0x1)

/* meanings of some of the bits for compression */
#define ZRAM_CDESC_LOC_BASE_MASK                0xFFFFFFFFC0ULL
#define ZRAM_CDESC_LOC_NUM_DESC_MASK            0xFULL
#define ZRAM_CDESC_WRIDX_WRITE_IDX_MASK         0xFFFFULL
#define ZRAM_CDESC_CTRL_COMPRESS_BUSY_SHIFT     17ULL
#define ZRAM_CDESC_CTRL_COMPRESS_ENABLE_SHIFT   18ULL
#define ZRAM_CDESC_CTRL_COMPRESS_ERROR_SHIFT    19ULL
#define ZRAM_CDESC_CTRL_COMPRESS_HALTED_SHIFT   20ULL
#define ZRAM_CDESC_CTRL_COMPLETE_IDX_MASK       0xFFFFULL

enum decompr_status {
	DCMD_STATUS_IDLE         = 0x0,
	DCMD_STATUS_DECOMPRESSED = 0x1,
	DCMD_STATUS_ERROR        = 0x4,
	DCMD_STATUS_HASH         = 0x6,
	DCMD_STATUS_PEND         = 0x7,
};

/*
 * decompression related
 * we can have a number of register sets for decompression
 * each set fits within 0x40 bytes
 */

/* amount of space for each register set */
#define ZRAM_DCMD_REGSET_SIZE 0x40

/* helpers to calculate offsets for decompression register set "n" */
#define ZRAM_DCMD_CSIZE(n) (ZRAM_DECOMP_OFFSET + 0x800 + ((n) * ZRAM_DCMD_REGSET_SIZE))
#define ZRAM_DCMD_DEST(n)  (ZRAM_DECOMP_OFFSET + 0x808 + ((n) * ZRAM_DCMD_REGSET_SIZE))
#define ZRAM_DCMD_RES(n)   (ZRAM_DECOMP_OFFSET + 0x810 + ((n) * ZRAM_DCMD_REGSET_SIZE))
#define ZRAM_DCMD_BUF0(n)  (ZRAM_DECOMP_OFFSET + 0x818 + ((n) * ZRAM_DCMD_REGSET_SIZE))
#define ZRAM_DCMD_BUF1(n)  (ZRAM_DECOMP_OFFSET + 0x820 + ((n) * ZRAM_DCMD_REGSET_SIZE))
#define ZRAM_DCMD_BUF2(n)  (ZRAM_DECOMP_OFFSET + 0x828 + ((n) * ZRAM_DCMD_REGSET_SIZE))
#define ZRAM_DCMD_BUF3(n)  (ZRAM_DECOMP_OFFSET + 0x830 + ((n) * ZRAM_DCMD_REGSET_SIZE))

/*
 * Decompression command status from the specification
 */
enum decompr_status_flags {
	/* reset value */
	HWZRAM_DCMD_IDLE = 0x0,

	/* Decompressed bytes written to destination */
	HWZRAM_DCMD_DECOMPRESSED = 0x1,

	/* command completed with ERROR */
	HWZRAM_DCMD_ERROR = 0x4,

	/* buffer written buf hash didn't match decompressed bytes */
	HWZRAM_DCMD_HASH  = 0x6,

	/* command waiting or being processed by hardware */
	HWZRAM_DCMD_PEND = 0x7,
};

/* meanings of some of the bits for decompression */
#define ZRAM_DCMD_DEST_BUF_MASK          ~((1ULL << 12ULL) - 1ULL)
#define ZRAM_DCMD_DEST_STATUS_MASK       0xEULL
#define ZRAM_DCMD_DEST_STATUS_SHIFT      1ULL
#define ZRAM_DCMD_DEST_STATUS(val)       SHIFT_AND_MASK(val, 1, 0x7ULL)
#define ZRAM_DCMD_DEST_INTR_MASK         0x1ULL
#define ZRAM_DCMD_DEST_INTR_SHIFT        0x0ULL
#define ZRAM_DCMD_CSIZE_HASH_SHIFT       32ULL
#define ZRAM_DCMD_CSIZE_SIZE_SHIFT       16ULL
#define ZRAM_DCMD_CSIZE_HASH_ENABLE_MASK 0x1ULL
#define ZRAM_DCMD_BUF_SIZE_SHIFT         60ULL
#define ZRAM_DCMD_RES_ADDR_MASK          0x000000FFFFFFFFF8ULL
#define ZRAM_DCMD_RES_ENABLE_SHIFT       63ULL
#define ZRAM_DCMD_RES_OVERWRITE_SHIFT    62ULL

#define ZRAM_DCMD_DEST_TO_STATUS(d) (((d) & ZRAM_DCMD_DEST_STATUS_MASK) \
					>> ZRAM_DCMD_DEST_STATUS_SHIFT)

/* a mask for the register set */
#define ZRAM_DCMD_MASK 0x7C0

/* convert from a register offset to a register set */
#define ZRAM_DCMD_REGSET(offset) (((offset) & ZRAM_DCMD_MASK) >> 6)

/* (so far) everything above this offset is related to decompression */
#define ZRAM_DECOMPRESSION_REG_BASE ZRAM_DCMD_CSIZE(0)

#endif /* _HWZRAM_H_ */
