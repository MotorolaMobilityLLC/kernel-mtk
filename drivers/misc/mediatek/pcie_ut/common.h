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

#ifndef	___COMMON_H__
#define	___COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define	PACKING

#define	AS_INT32(x)	(*(INT32 *)(x))
#define	AS_INT16(x)	(*(INT16 *)(x))
#define	AS_INT8(x)	(*(INT8	 *)(x))

#define	AS_UINT32(x)	(*(u32 *)(x))
#define	AS_UINT16(x)	(*(u16 *)(x))
#define	AS_UINT8(x)	(*(u8  *)(x))

#define	READ_REG32(reg)	\
	(*(u32 *const)((void *)(reg)))

#define	WRITE_REG32(reg, val) \
	((*(u32 *const)((void *)(reg))) = (val))

#define	SET_REG32(reg, val) \
	WRITE_REG32(reg, (READ_REG32(reg) | (val)))

#define	RESET_REG32(reg, val) \
	WRITE_REG32(reg, (READ_REG32(reg) & (~val)))

#define	READ_REGISTER_UINT32(reg) \
	(*(u32 *const)(reg))

#define	WRITE_REGISTER_UINT32(reg, val)	\
	((*(u32 *const)(reg)) = (val))

#define	READ_REGISTER_UINT16(reg) \
	(*(u16 *const)(reg))

#define	WRITE_REGISTER_UINT16(reg, val)	\
	((*(u16 *const)(reg)) = (val))

#define	READ_REGISTER_UINT8(reg) \
	(*(u8 *const)(reg))

#define	WRITE_REGISTER_UINT8(reg, val) \
	((*(u8 *const)(reg)) = (val))

#define	INREG8(x)		READ_REGISTER_UINT8((u8 *)(x))
#define	OUTREG8(x, y)		WRITE_REGISTER_UINT8((u8 *)(x), (UINT8)(y))
#define	SETREG8(x, y)		OUTREG8(x, INREG8(x) | (y))
#define	CLRREG8(x, y)		OUTREG8(x, INREG8(x) & ~(y))
#define	MASKREG8(x, y, z)	OUTREG8(x, (INREG8(x) & ~(y)) | (z))

#define	INREG16(x)		READ_REGISTER_UINT16((u16 *)(x))
#define	OUTREG16(x, y)		WRITE_REGISTER_UINT16((u16 *)(x), (UINT16)(y))
#define	SETREG16(x, y)		OUTREG16(x, INREG16(x) | (y))
#define	CLRREG16(x, y)		OUTREG16(x, INREG16(x) & ~(y))
#define	MASKREG16(x, y,	z)	OUTREG16(x, (INREG16(x) & ~(y)) | (z))

#define	INREG32(x)		READ_REGISTER_UINT32((u32 *)(x))
#define	OUTREG32(x, y)		WRITE_REGISTER_UINT32((u32 *)(x), (UINT32)(y))
#define	SETREG32(x, y)		OUTREG32(x, INREG32(x) | (y))
#define	CLRREG32(x, y)		OUTREG32(x, INREG32(x) & ~(y))
#define	MASKREG32(x, y,	z)	OUTREG32(x, (INREG32(x) & ~(y))|(z))

#define	REG_FLD(width, shift) \
	((u32)((((width) & 0xFF) << 16) | ((shift) &	0xFF)))

#define	REG_FLD_WIDTH(field) \
	((u32)(((field) >> 16) & 0xFF))

#define	REG_FLD_SHIFT(field) \
	((u32)((field) & 0xFF))

#define	REG_FLD_MASK(field) \
	(((u32)(1ULL	<< REG_FLD_WIDTH(field)) - 1) << REG_FLD_SHIFT(field))

#define	REG_FLD_GET(field, reg32) \
	(((reg32) & REG_FLD_MASK(field)) >> REG_FLD_SHIFT(field))

#define	REG_FLD_VAL(field, val)	\
	((val) << REG_FLD_SHIFT(field) & REG_FLD_MASK(field))

#define	REG_FLD_SET(field, reg32, val) \
	((reg32) = (((reg32) & ~REG_FLD_MASK(field)) | \
		REG_FLD_VAL((field), (val))))

#define	REG_SET(reg32, val) ((reg32) = (val))


#ifdef __cplusplus
}
#endif


#endif
