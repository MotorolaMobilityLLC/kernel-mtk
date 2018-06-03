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
#ifndef UFOZIP_PRIVATE_H
#define UFOZIP_PRIVATE_H

#define UFO_REG_PADDR	0x11B00000

#define UFO_REG_BASE	0
#define	DEC_DICTSIZE	(1<<23)
#define	USBUF_SIZE	(1*1024)   /* jack modify for new bitfile */
#define kNumPosSlotBits	5		/* 5:(1<<16-1)64K;	6:(1<<27-1):128M */

#define   ENC_OFFSET	(0x0)
#define   DEC_OFFSET	(0x1000)
#define   CDESC_OFFSET	(0x2000)

#define	  INTRP_STS_ERR		      (UFO_REG_BASE+0x40)
#define	  INTRP_MASK_ERR	      (UFO_REG_BASE+0x48)
#define	  INTRP_STS_CMP		      (UFO_REG_BASE+0x50)
#define	  INTRP_MASK_CMP	      (UFO_REG_BASE+0x58)
#define	  INTRP_STS_DCMP	      (UFO_REG_BASE+0x60)
#define	  INTRP_MASK_DCMP	      (UFO_REG_BASE+0x68)


#define	  ENC_STATUS                  (UFO_REG_BASE+ENC_OFFSET+0x104)
#define	  ENC_DCM                     (UFO_REG_BASE+ENC_OFFSET+0x108)
#define   ENC_CUT_BLKSIZE             (UFO_REG_BASE+ENC_OFFSET+0x110)
#define	  ENC_LIMIT_LENPOS		(UFO_REG_BASE+ENC_OFFSET+0x114)
#define   ENC_ZIP_LEN                 (UFO_REG_BASE+ENC_OFFSET+0x118)
#define   ENC_DBG_REG                 (UFO_REG_BASE+ENC_OFFSET+0x11C)
#define   ENC_BLOCK_CRC_IN            (UFO_REG_BASE+ENC_OFFSET+0x120)
#define   ENC_BLOCK_CRC_OUT           (UFO_REG_BASE+ENC_OFFSET+0x124)
#define   ENC_PROB_STEP               (UFO_REG_BASE+ENC_OFFSET+0x128)
#define   ENC_INTF_RD_THR             (UFO_REG_BASE+ENC_OFFSET+0x12C)
#define   ENC_INTF_WR_THR             (UFO_REG_BASE+ENC_OFFSET+0x130)
#define   ENC_INTF_SYNC_VAL           (UFO_REG_BASE+ENC_OFFSET+0x134)
#define   ENC_INTF_RST_REQ            (UFO_REG_BASE+ENC_OFFSET+0x138)
#define   ENC_INTF_STAT               (UFO_REG_BASE+ENC_OFFSET+0x13C)
#define   ENC_INTF_BUS_BL             (UFO_REG_BASE+ENC_OFFSET+0x140)
#define   ENC_INTF_CTRL               (UFO_REG_BASE+ENC_OFFSET+0x144)
#define   ENC_INTF_USMARGIN           (UFO_REG_BASE+ENC_OFFSET+0x148)
#define   ENC_INTF_DBG0               (UFO_REG_BASE+ENC_OFFSET+0x14C)
#define   ENC_TMO_THR                 (UFO_REG_BASE+ENC_OFFSET+0x150)
#define   ENC_AXI_PARAM0              (UFO_REG_BASE+ENC_OFFSET+0x154)
#define   ENC_AXI_PARAM1              (UFO_REG_BASE+ENC_OFFSET+0x158)
#define   ENC_AXI_PARAM2              (UFO_REG_BASE+ENC_OFFSET+0x15C)
#define   ENC_INT_MASK                (UFO_REG_BASE+ENC_OFFSET+0x160)
#define	  ENC_DSUS                    (UFO_REG_BASE+ENC_OFFSET+0x164)
#define	  ENC_INT_CLR	      (UFO_REG_BASE+ENC_OFFSET+0x168)
#define	  ENC_UTIL_MODE0              (UFO_REG_BASE+ENC_OFFSET+0x16C)
#define	  ENC_RST_MARGIN              (UFO_REG_BASE+ENC_OFFSET+0x170)
#define   ENC_ZRAM_DSC                (UFO_REG_BASE+ENC_OFFSET+0x230)

#define		CDESC_CRC		(UFO_REG_BASE+0x0)
#define		CDESC_ZLEN		(UFO_REG_BASE+0x4)
#define		CDESC_ADRI		(UFO_REG_BASE+0x4)
#define		CDESC_ADROA		(UFO_REG_BASE+0x8)
#define		CDESC_ADROB		(UFO_REG_BASE+0xc)
#define		CDESC_ADROC		(UFO_REG_BASE+0x10)
#define		CDESC_ADROD		(UFO_REG_BASE+0x14)
#define		CDESC_ADROE		(UFO_REG_BASE+0x18)
#define		CDESC_ADROF		(UFO_REG_BASE+0x1c)


#define		DCMD_0_CSIZE	(UFO_REG_BASE+0x800)
#define		DCMD_0_DEST		(UFO_REG_BASE+0x808)
#define		DCMD_0_RES		(UFO_REG_BASE+0x810)
#define		DCMD_0_BUF0		(UFO_REG_BASE+0x818)
#define		DCMD_0_BUF1		(UFO_REG_BASE+0x820)
#define		DCMD_0_BUF2		(UFO_REG_BASE+0x828)
#define		DCMD_0_BUF3		(UFO_REG_BASE+0x830)




#define   DEC_CMD                     (UFO_REG_BASE+DEC_OFFSET+0x900)
#define   DEC_STATUS                  (UFO_REG_BASE+DEC_OFFSET+0x904)
#define   DEC_CONFIG                  (UFO_REG_BASE+DEC_OFFSET+0x908)
#define   DEC_ZIP_LEN                 (UFO_REG_BASE+DEC_OFFSET+0x90C)
#define   DEC_BLOCK_CRC_IN            (UFO_REG_BASE+DEC_OFFSET+0x910)
#define   DEC_BLOCK_CRC_OUT           (UFO_REG_BASE+DEC_OFFSET+0x914)
#define   DEC_RD_ADDRL                (UFO_REG_BASE+DEC_OFFSET+0x918)
#define   DEC_RD_ADDRH                (UFO_REG_BASE+DEC_OFFSET+0x91C)
#define   DEC_WR_ADDRL                (UFO_REG_BASE+DEC_OFFSET+0x920)
#define   DEC_WR_ADDRH                (UFO_REG_BASE+DEC_OFFSET+0x924)
#define   DEC_PROB_STEP               (UFO_REG_BASE+DEC_OFFSET+0x928)
#define   DEC_CONFIG2                 (UFO_REG_BASE+DEC_OFFSET+0x92C)
#define   DEC_INTF_RD_THR             (UFO_REG_BASE+DEC_OFFSET+0x940)
#define   DEC_INTF_WR_THR             (UFO_REG_BASE+DEC_OFFSET+0x944)
#define   DEC_INTF_SYNC_VAL           (UFO_REG_BASE+DEC_OFFSET+0x948)
#define   DEC_INTF_RST_REQ            (UFO_REG_BASE+DEC_OFFSET+0x94C)
#define   DEC_INTF_STAT               (UFO_REG_BASE+DEC_OFFSET+0x950)
#define   DEC_INTF_BUS_BL             (UFO_REG_BASE+DEC_OFFSET+0x954)
#define   DEC_INTF_CTRL               (UFO_REG_BASE+DEC_OFFSET+0x958)
#define   DEC_INTF_DBG0               (UFO_REG_BASE+DEC_OFFSET+0x960)
#define   DEC_TMO_THR                 (UFO_REG_BASE+DEC_OFFSET+0x968)
#define   DEC_AXI_PARAM0              (UFO_REG_BASE+DEC_OFFSET+0x96C)
#define   DEC_AXI_PARAM1              (UFO_REG_BASE+DEC_OFFSET+0x970)
#define   DEC_AXI_PARAM2              (UFO_REG_BASE+DEC_OFFSET+0x974)
#define   DEC_INT_MASK                (UFO_REG_BASE+DEC_OFFSET+0x978)
#define   DEC_AXI_PARAM3              (UFO_REG_BASE+DEC_OFFSET+0x97C)
#define	  DEC_DCM                     (UFO_REG_BASE+DEC_OFFSET+0x980)
#define   DEC_HYBRID_SYMBOL_NUM       (UFO_REG_BASE+DEC_OFFSET+0x990)
#define   DEC_DBGREG1                 (UFO_REG_BASE+DEC_OFFSET+0xa00)
#define   DEC_DBGREG2                 (UFO_REG_BASE+DEC_OFFSET+0xa04)
#define   DEC_DBGREG3                 (UFO_REG_BASE+DEC_OFFSET+0xa08)
#define   DEC_DBG_INBUF_SIZE          (UFO_REG_BASE+DEC_OFFSET+0xa0C)
#define   DEC_DBG_OUTBUF_SIZE         (UFO_REG_BASE+DEC_OFFSET+0xa10)
#define   DEC_DBG_INNERBUF_SIZE       (UFO_REG_BASE+DEC_OFFSET+0xa14)
#define   DEC_DBG_ERROR_STATUS        (UFO_REG_BASE+DEC_OFFSET+0xa18)
#define   DEC_TRACE_RAM_ERRORP        (UFO_REG_BASE+DEC_OFFSET+0xa1C)
#define   DEC_DBGMEM                  (UFO_REG_BASE+DEC_OFFSET+0xa40)
#define   DEC_INT_CLR                 (UFO_REG_BASE+DEC_OFFSET+0xa50)
#define   DEC_RST_CTRL                (UFO_REG_BASE+DEC_OFFSET+0xa54)
#define   DEC_UTIL_MODE0              (UFO_REG_BASE+DEC_OFFSET+0xa58)
#define   DEC_RST_MARGIN              (UFO_REG_BASE+DEC_OFFSET+0xa5C)
#define   DEC_BAT_CFG1	      (UFO_REG_BASE+DEC_OFFSET+0xb00)
#define   DEC_BAT_WBUFLEN	      (UFO_REG_BASE+DEC_OFFSET+0xb04)
#define   DEC_BAT_QUEUE	      (UFO_REG_BASE+DEC_OFFSET+0xb08)
#define   DEC_BAT_CRCIN	      (UFO_REG_BASE+DEC_OFFSET+0xb0C)
#define   DEC_BAT_CRCOUT	      (UFO_REG_BASE+DEC_OFFSET+0xb10)
#define   DEC_BAT_DBNUM	      (UFO_REG_BASE+DEC_OFFSET+0xb14)
#define   DEC_AXI_RCRC                (UFO_REG_BASE+DEC_OFFSET+0xC00)
#define   DEC_AXI_WCRC                (UFO_REG_BASE+DEC_OFFSET+0xC04)
#define   DEC_INTF_RAM1_RCRC          (UFO_REG_BASE+DEC_OFFSET+0xC10)
#define   DEC_INTF_RAM1_WCRC          (UFO_REG_BASE+DEC_OFFSET+0xC14)
#define   DEC_INTF_RAM4_RCRC          (UFO_REG_BASE+DEC_OFFSET+0xC28)
#define   DEC_INTF_RAM4_WCRC          (UFO_REG_BASE+DEC_OFFSET+0xC2C)
#define   DEC_RAM1_RCRC               (UFO_REG_BASE+DEC_OFFSET+0xC40)
#define   DEC_RAM1_WCRC               (UFO_REG_BASE+DEC_OFFSET+0xC44)
#define   DEC_RAM2_RCRC               (UFO_REG_BASE+DEC_OFFSET+0xC48)
#define   DEC_RAM2_WCRC               (UFO_REG_BASE+DEC_OFFSET+0xC4C)
#define   DEC_RAM5_RCRC               (UFO_REG_BASE+DEC_OFFSET+0xC60)
#define   DEC_BQRAM_RCRC              (UFO_REG_BASE+DEC_OFFSET+0xC70)
#define   DEC_BQRAM_WCRC              (UFO_REG_BASE+DEC_OFFSET+0xC74)
#define   DEC_MBIST_DONE              (UFO_REG_BASE+DEC_OFFSET+0xCA0)
#define   DEC_MBIST_FAIL              (UFO_REG_BASE+DEC_OFFSET+0xCA4)


/* UFOZIP_HOST_CMD */
#define   CMD_START_BLOCK             1
#define   CMD_START_BATCH             2
#define   CMD_START_QUEUE             4
#define   CMD_START_ZRAM              8

#ifndef UInt32
#define UInt32 uint32_t


#endif

#ifndef u32

#define u32 uint32_t
#endif


typedef	enum {
	HWCFG_NONE,
	HWCFG_INTF_BUS_BL,
	HWCFG_TMO_THR,
	HWCFG_ENC_DSUS,
	HWCFG_ENC_LIMIT_LENPOS,
	HWCFG_ENC_INTF_USMARGIN,
	HWCFG_AXI_MAX_OUT,
	HWCFG_INTF_SYNC_VAL,
	HWCFG_DEC_DBG_INBUF_SIZE,
	HWCFG_DEC_DBG_OUTBUF_SIZE,
	HWCFG_DEC_DBG_INNERBUF_SIZE,
	HWCFG_CORNER_CASE,
} UFOZIP_HWCONFIG;

typedef struct {
	UInt32 lenpos_max;
	UInt32 dsmax;
	UInt32 usmin;
	UInt32 intf_bus_bl;
	UInt32 tmo_thr;
	UInt32 enc_dsus;
	UInt32 enc_intf_usmargin;
	UInt32 axi_max_out;
	UInt32 intf_sync_val;
	UInt32 dec_dbg_inbuf_size;
	UInt32 dec_dbg_outbuf_size;
	UInt32 dec_dbg_innerbuf_size;
} UFOZIP_CORNER_CONFIG;

typedef	struct {
	UInt32 blksize;
	UInt32 cutValue;
	UInt32 compress_level;
	UInt32 refbyte_flag;
	UInt32 componly_mode;
	UInt32 limitSize;
	UInt32 dictsize;
	UInt32 probstep;
	UInt32 hashmask;
	UInt32 batch_mode;
	UInt32 batch_blocknum;
	UInt32 batch_srcbuflen;
	UInt32 batch_dstbuflen;
	UInt32 hwcfg_sel;
	UFOZIP_CORNER_CONFIG hwcfg;
	UInt32 hybrid_decode;
} UFOZIP_CONFIG;

#endif /* UFOZIP_PRIVATE_H */
