/*
 * Copyright (C) 2015 MediaTek Inc.
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

#ifndef _MT6799_M4U_REG_H__
#define _MT6799_M4U_REG_H__

/* #include "mach/mt_reg_base.h" */

/* ================================================= */
/* common macro definitions */
#define F_VAL(val, msb, lsb) (((val)&((1<<(msb-lsb+1))-1))<<lsb)
#define F_VAL_L(val, msb, lsb) (((val)&((1L<<(msb-lsb+1))-1))<<lsb)
#define F_MSK(msb, lsb)     F_VAL(0xffffffff, msb, lsb)
#define F_MSK_L(msb, lsb)     F_VAL_L(0xffffffffffffffffL, msb, lsb)
#define F_BIT_SET(bit)	  (1<<(bit))
#define F_BIT_VAL(val, bit)  ((!!(val))<<(bit))
#define F_MSK_SHIFT(regval, msb, lsb) (((regval)&F_MSK(msb, lsb))>>lsb)


/* ===================================================== */
/* M4U register definition */
/* ===================================================== */

#define REG_MMUg_PT_BASE	   (0x0)
#define REG_MMUg_PT_BASE_SEC	 (0x4)


#define REG_MMU_PROG_EN		 0x10
#define F_MMU0_PROG_EN	       1

#define REG_MMU_PROG_VA		 0x14
#define F_PROG_VA_LOCK_BIT	     (1<<11)
#define F_PROG_VA_LAYER_BIT	    F_BIT_SET(9)
#define F_PROG_VA_SIZE16X_BIT	  F_BIT_SET(8)
#define F_PROG_VA_SECURE_BIT	   (1<<7)
#define F_PROG_VA_MASK	    0xfffff000

#define REG_MMU_PROG_DSC		0x18

#define REG_MMU_INVLD	    (0x20)
#define F_MMU_INV_VICT		(1<<2)
#define F_MMU_INV_ALL		(1<<1)
#define F_MMU_INV_RANGE	    (1)

#define REG_MMU_INVLD_SA		 (0x24)
#define REG_MMU_INVLD_EA	  (0x28)


#define REG_MMU_INVLD_SEC	   (0x2c)
#define F_MMU_INV_SEC_RANGE	0x1

#define REG_MMU_INVLD_SA_SEC	(0x30)
#define REG_MMU_INVLD_EA_SEC	(0x34)

#define REG_INVLID_SEL		   (0x38)
#define F_MMU_INV_VA_32	 (1<<2)
#define F_MMU_INV_EN_L2	 (1<<1)
#define F_MMU_INV_EN_L1	 (1<<0)

#define REG_INVLID_SEL_SEC	  (0x3c)
#define F_MMU_INV_SEC_VA_32	   (1<<6)
#define F_MMU_INV_SEC_DBG	   (1<<5)
#define F_MMU_INV_SEC_INV_INT_EN    (1<<4)
#define F_MMU_INV_SEC_INV_INT_CLR   (1<<3)
#define F_MMU_INV_SEC_INV_DONE      (1<<2)
#define F_MMU_INV_SEC_EN_L2	     (1<<1)
#define F_MMU_INV_SEC_EN_L1	     (1<<0)

#define REG_MMU_SEC_ABORT_INFO      (0x40)
#define F_MMU_SEC_ABORT_WR(en)    F_BIT_VAL(en, 31)
#define F_MMU_SEC_ABORT_ADDR    F_MSK_SHIFT(regval, 30, 21)
#define F_MMU_SEC_ABORT_DOMAIN    F_MSK_SHIFT(regval, 20, 16)
#define F_MMU_SEC_ABORT_ID    F_MSK_SHIFT(regval, 15, 0)

#define REG_MMU_DUMMY	       (0X44)
#define REG_MMU_MISC_CTRL      (0x48)
#define F_MMU_COHERENCE(id) (1<<(0+16*id))
#define F_MMU_INORDERWR(id) (1<<(1+16*id))
#define F_MMU_WBFU(id)      (1<<(2+16*id))
#define F_MMU_STD_AXI_MODE(id)      (1<<(3+16*id))
#define F_MMU_BLOCKING_MODE(id)     (1<<(4+16*id))
#define F_MMU_HALF_ENTRY_MODE(id)   (1<<(5+16*id))
#define F_MMU_ID_GROUPING(id)       (1<<(6+16*id))

#define REG_MMU_PRIORITY	(0x4c)
#define F_MMU_RS_FULL_ULTRA_EN  F_BIT_SET(2)
#define F_MMU_AUTO_PF_ULTRA_BIT F_BIT_SET(1)
#define F_MMU_TABLE_WALK_ULTRA_BIT F_BIT_SET(0)

#define REG_MMU_DCM_DIS	 (0x50)

#define REG_MMU_WR_LEN	  (0x54)
#define F_MMU_MMU_WRITE_THROTTLING_DIS(id)    (1<<(5+16*id))
#define F_MMU_MMU0_WRITE_LEN    F_MSK(4, 0)
#define F_MMU_MMU1_WRITE_LEN    F_MSK(20, 16)

#define REG_MMU_HW_DEBUG	(0x58)
#define F_MMU_HW_DBG_L2_SCAN_ALL    F_BIT_SET(1)
#define F_MMU_HW_DBG_PFQ_BRDCST     F_BIT_SET(0)


#define REG_MMU_READ_ENTRY       0x100
#define F_READ_ENTRY_EN		 F_BIT_SET(31)
#define F_READ_ENTRY_MMx_MAIN(id)       F_BIT_SET(27+id)
#define F_READ_ENTRY_PFH		F_BIT_SET(26)
#define F_READ_ENTRY_MAIN_IDX(id, idx)       ((id == 0)?F_READ_ENTRY_MMU0_IDX(idx):F_READ_ENTRY_MMU1_IDX(idx))
#define F_READ_ENTRY_MMU1_IDX(idx)      F_VAL(idx, 25, 20)
#define F_READ_ENTRY_MMU0_IDX(idx)      F_VAL(idx, 19, 14)
#define F_READ_ENTRY_VICT_TLB_SEL	    F_BIT_SET(13)
#define F_READ_ENTRY_PFH_IDX(idx)       F_VAL(idx, 11, 5)
#define F_READ_ENTRY_PFH_PAGE_IDX(idx)    F_VAL(idx, 4, 2)
#define F_READ_ENTRY_PFH_WAY(way)       F_VAL(way, 1, 0)

#define REG_MMU_DES_RDATA	0x104

#define REG_MMU_PFH_TAG_RDATA    0x108
#define F_PFH_TAG_VA_GET(mmu, tag)    (F_MMU0_PFH_TAG_VA_GET(tag))
#define F_MMU0_PFH_TAG_VA_GET(tag)    (F_MSK_SHIFT(tag, 13, 4)<<(MMU_SET_MSB_OFFSET(0)+1))
#define F_MMU_PFH_TAG_VA_LAYER0_MSK(mmu)  ((mmu == 0)?F_MSK(31, 29):F_MSK(31, 28))
#define F_PFH_TAG_LAYER_BIT	  F_BIT_SET(3)
#define F_PFH_TAG_VA_32	      F_BIT_SET(2)
#define F_PFH_TAG_SEC_BIT	  F_BIT_SET(1)
#define F_PFH_TAG_AUTO_PFH	  F_BIT_SET(0)

#define REG_MMU_VICT_VLD 0x10c

#define F_MMU_VICT_VLD_BIT(way)      F_BIT_SET((way)&0xf)

/* tag related macro */
#define MMU0_SET_ORDER	 7
#define MMU1_SET_ORDER	 5
#define MMU_SET_ORDER(mmu)      ((mmu == 0) ? MMU0_SET_ORDER : MMU1_SET_ORDER)
#define MMU_SET_NR(mmu)    (1<<MMU_SET_ORDER(mmu))
#define MMU_SET_LSB_OFFSET	       15
#define MMU_SET_MSB_OFFSET(mmu)	 (MMU_SET_LSB_OFFSET+MMU_SET_ORDER(mmu)-1)
#define MMU_PFH_VA_TO_SET(mmu, va)     F_MSK_SHIFT(va, MMU_SET_MSB_OFFSET(mmu), MMU_SET_LSB_OFFSET)

#define MMU_PAGE_PER_LINE      8
#define MMU_WAY_NR  4
#define MMU_PFH_TOTAL_LINE(mmu) (MMU_SET_NR(mmu)*MMU_WAY_NR)


#define REG_MMU_CTRL_REG	 0x110

#define F_MMU_CTRL_LEGACY_4KB_MODE(en)  F_BIT_VAL(en, 17)
#define F_MMU_CTRL_RESEND_L2_DIS(dis)   F_BIT_VAL(dis, 16)
#define F_MMU_CTRL_RESEND_PFQ_DIS(dis)  F_BIT_VAL(dis, 15)
#define F_MMU_CTRL_L2TLB_DIS(dis)       F_BIT_VAL(dis, 14)
#define F_MMU_CTRL_MTLB_DIS(dis)        F_BIT_VAL(dis, 13)

#define F_MMU_CTRL_VICT_TLB_EN(en)      F_BIT_VAL(en, 12)
#define F_MMU_CTRL_HIT_AT_PFQ_L2_EN(en) F_BIT_VAL(en, 10)
#define F_MMU_CTRL_HANG_PREVENTION(en)  F_BIT_VAL(en,  9)

#define F_MMU_CTRL_LAYER2_PFH_DIS(dis)  F_BIT_VAL(dis, 7)
#define F_MMU_CTRL_INT_HANG_EN(en)      F_BIT_VAL(en,  6)
#define F_MMU_CTRL_TF_PROTECT_SEL(en)   F_VAL(en, 5, 4)
#define F_MMU_CTRL_VA_8GB_EN(en)        F_BIT_VAL(en, 3)
#define F_MMU_CTRL_MONITOR_CLR(clr)     F_BIT_VAL(clr, 2)
#define F_MMU_CTRL_MONITOR_EN(en)       F_BIT_VAL(en,  1)
#define F_MMU_CTRL_PFH_DIS(dis)	 F_BIT_VAL(dis, 0)

#define REG_MMU_IVRP_PADDR       0x114
#define F_MMU_IVRP_PA_SET(regval)    ((regval & F_MSK(31, 7)) | (regval>>32))

#define REG_MMU_VLD_PA_RNG       0x118


#define REG_MMU_INT_L2_CONTROL      0x120
#define F_INT_L2_CLR_BIT (1<<12)
#define F_INT_L2_MULTI_HIT_FAULT		 F_BIT_SET(0)
#define F_INT_L2_TABLE_WALK_FAULT		 F_BIT_SET(1)
#define F_INT_L2_PFH_DMA_FIFO_OVERFLOW	 F_BIT_SET(2)
#define F_INT_L2_MISS_DMA_FIFO_OVERFLOW	 F_BIT_SET(3)
#define F_INT_L2_INVALD_DONE		     F_BIT_SET(4)
#define F_INT_L2_PFH_FIFO_ERROR		     F_BIT_SET(5)
#define F_INT_L2_MISS_FIFO_ERR		     F_BIT_SET(6)
#define F_INT_L2_CDB_SLICE_ERR		     F_BIT_SET(7)


#define REG_MMU_INT_MAIN_CONTROL    0x124
#define F_INT_TRANSLATION_FAULT(MMU)                 F_BIT_SET(0+(MMU)*7)
#define F_INT_MAIN_MULTI_HIT_FAULT(MMU)              F_BIT_SET(1+(MMU)*7)
#define F_INT_INVALID_PHYSICAL_ADDRESS_FAULT(MMU)    F_BIT_SET(2+(MMU)*7)
#define F_INT_ENTRY_REPLACEMENT_FAULT(MMU)           F_BIT_SET(3+(MMU)*7)
#define F_INT_TLB_MISS_FAULT(MMU)                    F_BIT_SET(4+(MMU)*7)
#define F_INT_MISS_FIFO_ERR(MMU)                     F_BIT_SET(5+(MMU)*7)
#define F_INT_PFH_FIFO_ERR(MMU)                      F_BIT_SET(6+(MMU)*7)


#define F_INT_MAU(mmu, set)     F_BIT_SET(14+(set)+(mmu*4))
/* Dual AXI (14+(set)+(mmu*4));  Single AXI (7+(set)+(mmu*4)); */

#define F_INT_MMU0_MAIN_MSK	  F_MSK(6, 0)
#define F_INT_MMU1_MAIN_MSK	  F_MSK(13, 7)
#define F_INT_MMU0_MAU_MSK	   F_MSK(17, 14)
#define F_INT_MMU1_MAU_MSK	   F_MSK(21, 18)

#define REG_MMU_CPE_DONE_SEC    0x128
#define REG_MMU_CPE_DONE	0x12C

#define REG_MMU_L2_FAULT_ST	 0x130
#define F_INT_L2_CDB_SLICE_ERROR		   F_BIT_SET(9)
#define F_INT_L2_MISS_IN_FIFO_ERROR		   F_BIT_SET(8)
#define F_INT_L2_MISS_OUT_FIFO_ERROR	   F_BIT_SET(7)
#define F_INT_L2_PFH_IN_FIFO_ERROR		   F_BIT_SET(6)
#define F_INT_L2_PFH_OUT_FIFO_ERROR		   F_BIT_SET(5)

#define REG_MMU_MAIN_FAULT_ST       0x134

#define REG_MMU_TBWALK_FAULT_VA	 0x138
#define F_MMU_TBWALK_FAULT_VA_MSK   F_MSK(31, 12)
#define F_MMU_TBWALK_FAULT_TBL_ID   F_MSK(7, 2)
#define F_MMU_TBWALK_FAULT_VA32(regval) F_MSK_SHIFT(regval, 1, 1)
#define F_MMU_TBWALK_FAULT_LAYER(regval) F_MSK_SHIFT(regval, 0, 0)

#define REG_MMU_FAULT_VA(mmu)	 (0x13c+((mmu)<<3))
#define F_MMU_FAULT_VA_MSK	F_MSK(31, 12)
#define F_MMU_FAULT_VA_MSB(regval)    F_MSK_SHIFT(regval, 8, 8)
#define F_MMU_FAULT_PA_MSB(regval)    F_MSK_SHIFT(regval, 7, 6)
#define F_MMU_FAULT_INVPA           F_BIT_SET(5)
#define F_MMU_FAULT_TF              F_BIT_SET(4)
#define F_MMU_FAULT_SEC_BIT         F_BIT_SET(2)
#define F_MMU_FAULT_VA_WRITE_BIT    F_BIT_SET(1)
#define F_MMU_FAULT_VA_LAYER_BIT    F_BIT_SET(0)

#define REG_MMU_INVLD_PA(mmu)	 (0x140+((mmu)<<3))
#define REG_MMU_INT_ID(mmu)	     (0x150+((mmu)<<2))
#define F_MMU0_INT_ID_TF_MSK	F_MSK(10, 2)      /* only for MM iommu. */


#define REG_MMU_PF_L1_CNT(mmu)	       (0x1a8+(mmu<<5))
#define REG_MMU_PF_L2_CNT(mmu)	       (0x1ac+(mmu<<5))
#define REG_MMU_ACC_CNT(mmu)	       (0x1c0+(mmu<<5))
#define REG_MMU_MAIN_L1_MSCNT(mmu)     (0x1c4+(mmu<<5))
#define REG_MMU_MAIN_L2_MSCNT(mmu)     (0x1c8+(mmu<<5))
#define REG_MMU_RS_PERF_CNT(mmu)       (0x1cc+(mmu<<5))
#define REG_MMU_LOOKUP_CNT(mmu)        (0x1d0+(mmu<<5))


#define REG_MMU_PFH_VLD_0   (0x200)
#define REG_MMU_PFH_VLD(mmu, set, way)     \
	(REG_MMU_PFH_VLD_0+(((set)>>5)<<2)+((way)<<((mmu == 0)?(MMU0_SET_ORDER - 3):(MMU1_SET_ORDER - 3))))
#define F_MMU_PFH_VLD_BIT(set, way)      F_BIT_SET((set)&0x1f)  /* set%32 */

#define MMU01_SQ_OFFSET (0x600-0x300)
#define REG_MMU_SQ(mmu, x)	     (0x300+((x)<<2)+((mmu)*MMU01_SQ_OFFSET))
#define F_SQ_EN_BIT		 (1<<17)


#define MMU_TOTAL_RS_NR	 16
#define REG_MMU_RSx_VA(mmu, x)      (0x380+((x)<<4)+((mmu)*MMU01_SQ_OFFSET))
#define F_MMU_RSx_VA_GET(regval)    ((regval)&F_MSK(31, 12))
#define F_MMU_RSx_VA_32(regval)     F_MSK_SHIFT(regval, 1, 0)

#define REG_MMU_RSx_PA(mmu, x)      (0x384+((x)<<4)+((mmu)*MMU01_SQ_OFFSET))
#define F_MMU_RSx_PA_GET(regval)    ((regval)&F_MSK(31, 12))

#define REG_MMU_RSx_2ND_BASE(mmu, x) (0x388+((x)<<4)+((mmu)*MMU01_SQ_OFFSET))

#define REG_MMU_RSx_ST(mmu, x)      (0x38c+((x)<<4)+((mmu)*MMU01_SQ_OFFSET))
#define F_MMU_RSx_ST_ID(regval)     F_MSK_SHIFT(regval, 31, 16)
#define F_MMU_RSx_ST_OTHER(regval)  F_MSK_SHIFT(regval, 8, 0)

#define REG_MMU_MAIN_TAG(mmu, x)       (0x500+((x)<<2)+((mmu)*MMU01_SQ_OFFSET))
#define F_MAIN_TLB_VA_MSK	   F_MSK(31, 12)
#define F_MAIN_TLB_LOCK_BIT	 (1<<11)
#define F_MAIN_TLB_VALID_BIT	(1<<10)
#define F_MAIN_TLB_LAYER_BIT	F_BIT_SET(9)
#define F_MAIN_TLB_16X_BIT	  F_BIT_SET(8)
#define F_MAIN_TLB_SEC_BIT	  F_BIT_SET(7)
#define F_MAIN_TLB_INV_DES_BIT      (1<<6)
#define F_MAIN_TLB_SQ_EN_BIT	(1<<5)
#define F_MAIN_TLB_SQ_INDEX_MSK     F_MSK(4, 1)
#define F_MAIN_TLB_SQ_INDEX_GET(regval)     F_MSK_SHIFT(regval, 4, 1)
#define F_MAIN_TLB_BIT32	  F_BIT_SET(0)


#define REG_MMU_MAU_START(mmu, mau)	      (0x900+(mmu*0x100+mau*0x24))
#define REG_MMU_MAU_START_BIT32(mmu, mau)	(0x904+(mmu*0x100+mau*0x24))
#define REG_MMU_MAU_END(mmu, mau)		(0x908+(mmu*0x100+mau*0x24))
#define REG_MMU_MAU_END_BIT32(mmu, mau)	  (0x90c+(mmu*0x100+mau*0x24))
#define REG_MMU_MAU_LARB_EN(mmu, mau)		(0x910+(mmu*0x100+mau*0x24))
#define REG_MMU_MAU_PORT_EN(mmu, mau)	    (0x914+(mmu*0x100+mau*0x24))
#define REG_MMU_MAU_ASSERT_ID(mmu, mau)	  (0x918+(mmu*0x100+mau*0x24))
#define REG_MMU_MAU_ADDR(mmu, mau)	       (0x91c+(mmu*0x100+mau*0x24))
#define REG_MMU_MAU_ADDR_BIT32(mmu, mau)	 (0x920+(mmu*0x100+mau*0x24))

#define F_MMU_MAU_ASSERT_ID_LARB(regval)    F_MSK_SHIFT(regval, 7, 5)
#define F_MMU_MAU_ASSERT_ID_PORT(regval)    F_MSK_SHIFT(regval, 4, 0)



#define F_MAU_LARB_VAL(mau, larb)	 ((larb)<<(mau*8))
#define F_MAU_LARB_MSK(larb)	     F_MSK_SHIFT(larb, 7, 0)
#define REG_MMU_MAU_CLR(mmu)		(0x990+(mmu*0x100))
#define REG_MMU_MAU_IO(mmu)		(0x994+(mmu*0x100))
#define F_MAU_BIT_VAL(val, mau)     F_BIT_VAL(val, mau)
#define REG_MMU_MAU_RW(mmu)		(0x998+(mmu*0x100))
#define REG_MMU_MAU_VA(mmu)		(0x99c+(mmu*0x100))
#define REG_MMU_MAU_ASSERT_ST(mmu)		(0x9a0+(mmu*0x100))

#define MMU_TOTAL_PROG_DIST_NR	 16

#define REG_MMU0_PROG_DIST(dist)      (0xb00+((dist)<<2))
#define REG_MMU1_PROG_DIST(dist)      (0xb80+((dist)<<2))

#define F_PF_ID_SEL(sel)    F_BIT_VAL(sel, 21)
#define F_PF_DIR(dir)	    F_BIT_VAL(dir, 20)
#define F_PF_DIST(dist)		(dist<<16)
#define F_PF_EN(en)	      F_BIT_VAL(en, 22)


#define REG_MMU_TRF_MON_EN(mmu)	       (0xe00+(mmu<<6))
#define REG_MMU_TRF_MON_CLR(mmu)       (0xe04+(mmu<<6))
#define REG_MMU_TRF_MON_ID(mmu)        (0xe08+(mmu<<6))
#define F_TRF_MON_TBL(sel)	           F_VAL(sel, 19, 18)
#define F_TRF_MON_ID_SEL(sel)	       F_BIT_VAL(sel, 17)
#define F_TRF_MON_ID_ALL(en)	       F_BIT_VAL(en, 16)
#define F_TRF_MON_ID_MSK	           F_MSK(15, 0)
#define F_TRF_MON_ID(larb, port)      ((larb) << 7 | ((port) << 2))

#define REG_MMU_TRF_MON_CON(mmu)        (0xe0c+(mmu<<6))
#define F_TRF_MON_WRITE(val)	       F_BIT_VAL(val, 3)
#define F_TRF_MON_DP_MODE(val)	       F_BIT_VAL(val, 2)
#define F_TRF_MON_REQ_SEL(sel)	       F_VAL(sel, 1, 0)

#define REG_MMU_TRF_MON_ACT_COUNT(mmu)        (0xe10+(mmu<<6))
#define REG_MMU_TRF_MON_REQ_COUNT(mmu)        (0xe14+(mmu<<6))
#define REG_MMU_TRF_MON_BEAT_COUNT(mmu)       (0xe18+(mmu<<6))
#define REG_MMU_TRF_MON_BYTE_COUNT(mmu)       (0xe1c+(mmu<<6))
#define REG_MMU_TRF_MON_DATA_COUNT(mmu)       (0xe20+(mmu<<6))
#define REG_MMU_TRF_MON_CP_COUNT(mmu)         (0xe24+(mmu<<6))
#define REG_MMU_TRF_MON_DP_COUNT(mmu)         (0xe28+(mmu<<6))
#define REG_MMU_TRF_MON_CP_MAX(mmu)           (0xe2c+(mmu<<6))
#define REG_MMU_TRF_MON_OSTD_COUNT(mmu)       (0xe30+(mmu<<6))
#define REG_MMU_TRF_MON_OSTD_MAX(mmu)         (0xe34+(mmu<<6))


/* ================================================================ */
/* SMI larb */
/* ================================================================ */

#define SMI_LARB_NONSEC_CON(port)  (0x380+(port<<2))
#define F_SMI_MMU_EN          F_BIT_SET(0)
#define F_SMI_CMDGP_EN        F_BIT_SET(1)
#define F_SMI_PATH_SEL        F_BIT_SET(4)
#define F_SMI_BANK_SEL(master, bank)   F_MSK_SHIFT(bank, master*2+8, master*2+8)


#define SMI_LARB_SEC_CON(port)     (0xf80+(port<<2))
#define F_SMI_SEC_MMU_EN       F_BIT_SET(0)
#define F_SMI_SEC_EN           F_BIT_SET(1)
#define SMI_LARB_DOMN(domn)	   F_MSK_SHIFT(domn, 8, 4)


#define REG_SMI_LARB_DOMN_OF_PORT(port)     (SMI_LARB_DOMN_0+(((port)>>3)<<2))
#define F_SMI_DOMN(port, domain)	((domain&0xf)<<(((port)&0x7)<<2))

/* ========================================================================= */
/* peripheral system */
/* ========================================================================= */
#define REG_PERIAXI_BUS_CTL3   (0x208)
#define F_PERI_MMU_EN(port, en)       ((en)<<((port)))


/* ========================================================================= */
/* Display fake engine */
/* ========================================================================= */
#define MMSYS_CG_CLR0		     0x108
#define DISP_FAKE_ENG_TEST_LEN	128
#define DISP_FAKE_ENG_EN		 0x200
#define DISP_FAKE_ENG_RST	       0x204
#define DISP_FAKE_ENG_CON0	    0x208
#define DISP_FAKE_ENG_CON1	      0x20C
#define DISP_FAKE_ENG_RD_ADDR       0x210
#define DISP_FAKE_ENG_WR_ADDR       0x214
#define DISP_FAKE_ENG_STATE	     0x218



#include <sync_write.h>


static inline unsigned int COM_ReadReg32(unsigned long addr)
{
	return ioread32((void *)addr);
}

static inline void COM_WriteReg32(unsigned long addr, unsigned int Val)
{
	mt_reg_sync_writel(Val, (void *)addr);
}

static inline unsigned int M4U_ReadReg32(unsigned long M4uBase, unsigned int Offset)
{
	unsigned int val;

	val = COM_ReadReg32((M4uBase + Offset));
	/* printk("M4U_ReadReg32: M4uBase: 0x%lx, Offset:0x%x, val:0x%x\n", M4uBase, Offset, val); */
	return val;
}

static inline void M4U_WriteReg32(unsigned long M4uBase, unsigned int Offset, unsigned int Val)
{
	COM_WriteReg32((M4uBase + Offset), Val);
	/* printk("M4U_WriteReg32: M4uBase: 0x%lx, Offset:0x%x, val:0x%x\n", M4uBase, Offset, Val); */
}

static inline unsigned int m4uHw_set_field(unsigned long M4UBase,
		unsigned int Reg, unsigned int bit_width, unsigned int shift,
		unsigned int value) {
	unsigned int mask = ((1 << bit_width) - 1) << shift;
	unsigned int old;

	value = (value << shift) & mask;
	old = M4U_ReadReg32(M4UBase, Reg);
	M4U_WriteReg32(M4UBase, Reg, (old & (~mask)) | value);
	return (old & mask) >> shift;
}

static inline void m4uHw_set_field_by_mask(unsigned long M4UBase, unsigned int reg,
					   unsigned long mask, unsigned int val)
{
	unsigned int regval;

	regval = M4U_ReadReg32(M4UBase, reg);
	regval = (regval & (~mask)) | val;
	M4U_WriteReg32(M4UBase, reg, regval);
}

static inline unsigned int m4uHw_get_field_by_mask(unsigned long M4UBase, unsigned int reg,
						   unsigned int mask)
{
	return M4U_ReadReg32(M4UBase, reg) & mask;
}




#endif
