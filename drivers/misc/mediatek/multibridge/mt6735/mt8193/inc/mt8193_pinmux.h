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

#ifndef MT8193_PINMUX_H
#define MT8193_PINMUX_H


#include <generated/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <linux/slab.h>

#include <linux/module.h>

#include "mt8193.h"


/* ret code */

#define PINMUX_RET_OK 0
#define PINMUX_RET_INVALID_ARG (-1)
#define PINMUX_RET_FAIL (-2)



#define PINMUX_FUNCTION0        0
#define PINMUX_FUNCTION1        1
#define PINMUX_FUNCTION2        2
#define PINMUX_FUNCTION3        3
#define PINMUX_FUNCTION4        4
#define PINMUX_FUNCTION5        5
#define PINMUX_FUNCTION6        6
#define PINMUX_FUNCTION7        7
#define PINMUX_FUNCTION_MAX     8

enum MT8193_APD_PIN {
	PIN_UNSUPPORTED = -1,

	PIN_NFD6,
	PIN_NFD5,
	PIN_NFD4,
	PIN_NFD3,
	PIN_NFD2,
	PIN_NFD1,
	PIN_NFD0,
	PIN_TP_VPLL,
	PIN_AO0N,
	PIN_AO0P,
	PIN_AO1N,
	PIN_AO1P,
	PIN_AO2N,
	PIN_AO2P,
	PIN_AOCK0N,
	PIN_AOCK0P,
	PIN_AO3N,
	PIN_AO3P,
	PIN_G0,
	PIN_B5,
	PIN_B4,
	PIN_B3,
	PIN_B2,
	PIN_B1,
	PIN_B0,
	PIN_DE,
	PIN_VCLK,
	PIN_HSYNC,
	PIN_VSYNC,
	PIN_CEC,
	PIN_HDMISCK,
	PIN_HDMISD,
	PIN_HTPLG,
	PIN_I2S_DATA,
	PIN_I2S_LRCK,
	PIN_I2S_BCK,
	PIN_DPI1CK,
	PIN_DPI1D7,
	PIN_DPI1D6,
	PIN_DPI1D5,
	PIN_DPI1D4,
	PIN_DPI1D3,
	PIN_DPI1D2,
	PIN_DPI1D1,
	PIN_DPI1D0,
	PIN_DPI0CK,
	PIN_DPI0HSYNC,
	PIN_DPI0VSYNC,
	PIN_DPI0D11,
	PIN_DPI0D10,
	PIN_DPI0D9,
	PIN_DPI0D8,
	PIN_DPI0D7,
	PIN_DPI0D6,
	PIN_DPI0D5,
	PIN_DPI0D4,
	PIN_DPI0D3,
	PIN_DPI0D2,
	PIN_DPI0D1,
	PIN_DPI0D0,
	PIN_SDA,
	PIN_SCL,
	PIN_NRNB,
	PIN_NCLE,
	PIN_NALE,
	PIN_NWEB,
	PIN_NREB,
	PIN_NLD7,
	PIN_NLD6,
	PIN_NLD5,
	PIN_NLD4,
	PIN_NLD3,
	PIN_NLD2,
	PIN_NLD1,
	PIN_NLD0,
	PIN_RTC_32K_CK,
	PIN_INT,
	PIN_RESET,
	PIN_EN_BB,
	PIN_CK_SEL,
	PIN_NFRBN,
	PIN_NFCLE,
	PIN_NFALE,
	PIN_NFWEN,
	PIN_NFREN,
	PIN_NFCEN,
	PIN_NFD7,
	PIN_PROT0,
	PIN_PROT1,
	PIN_PROT2,
	PIN_PROT3,
	PIN_PROT4,
	PIN_PROT5,

	PIN_MAX
};


/*
function table

[pin][2 * function]

pin0:  func_pmux, func_sel (<<),  func_mask
pin1:  func_pmux, func_sel, func_mask
pin2:  func_pmux, func_sel, func_mask
*/
static const u32 _au4PinmuxFuncTbl[PIN_MAX][3] = {
	/* PIN_NFD6, */     {REG_RW_PMUX1, 0, 0x7},
	/* PIN_NFD5, */     {REG_RW_PMUX1, 3, 0x38},
	/* PIN_NFD4, */     {REG_RW_PMUX1, 6, 0x1C0},
	/* PIN_NFD3, */     {REG_RW_PMUX1, 9, 0xE00},
	/* PIN_NFD2, */     {REG_RW_PMUX1, 12, 0x7000},
	/* PIN_NFD1, */     {REG_RW_PMUX1, 15, 0x38000},
	/* PIN_NFD0, */     {REG_RW_PMUX1, 18, 0x1C0000},
	/* PIN_TP_VPLL, */   {REG_RW_PMUX8, 18, 0x1C0000},
	/* PIN_AO0N, */     {REG_RW_PMUX8, 21, 0xE00000},
	/* PIN_AO0P, */     {REG_RW_PMUX8, 24, 0x7000000},
	/* PIN_AO1N, */     {REG_RW_PMUX8, 27, 0x38000000},
	/* PIN_AO1P, */     {REG_RW_PMUX9, 0,  0x7},
	/* PIN_AO2N, */     {REG_RW_PMUX9, 3,  0x38},
	/* PIN_AO2P, */     {REG_RW_PMUX9, 6,  0x1C0},
	/* PIN_AOCK0N, */   {REG_RW_PMUX9, 9,  0xE00},
	/* PIN_AOCK0P, */   {REG_RW_PMUX9, 12,  0x7000},
	/* PIN_AO3N, */     {REG_RW_PMUX9, 15,  0x38000},
	/* PIN_AO3P, */     {REG_RW_PMUX9, 18,  0x1C0000},
	/* PIN_G0, */       {REG_RW_PMUX1, 21,  0xE00000},
	/* PIN_B5, */       {REG_RW_PMUX1, 24,  0x7000000},
	/* PIN_B4, */       {REG_RW_PMUX1, 27,  0x38000000},
	/* PIN_B3, */       {REG_RW_PMUX2, 0,  0x7},
	/* PIN_B2, */       {REG_RW_PMUX2, 3,  0x38},
	/* PIN_B1, */       {REG_RW_PMUX2, 6,  0x1C0},
	/* PIN_B0, */       {REG_RW_PMUX2, 9,  0xE00},
	/* PIN_DE, */       {REG_RW_PMUX2, 12,  0x7000},
	/* PIN_VCLK, */     {REG_RW_PMUX2, 15,  0x38000},
	/* PIN_HSYNC, */    {REG_RW_PMUX2, 18,  0x1C0000},
	/* PIN_VSYNC, */    {REG_RW_PMUX2, 21,  0xE00000},
	/* PIN_CEC, */      {REG_RW_PMUX2, 24,  0x7000000},
	/* PIN_HDMISCK, */  {REG_RW_PMUX2, 27,  0x38000000},
	/* PIN_HDMISD, */   {REG_RW_PMUX3, 0,  0x7},
	/* PIN_HTPLG, */     {REG_RW_PMUX3, 3,  0x38},
	/* PIN_I2S_DATA, */  {REG_RW_PMUX3, 6,  0x1C0},
	/* PIN_I2S_LRCK, */  {REG_RW_PMUX3, 9,  0xE00},
	/* PIN_I2S_BCK, */   {REG_RW_PMUX3, 12,  0x7000},
	/* PIN_DPI1CK, */    {REG_RW_PMUX3, 15,  0x38000},
	/* PIN_DPI1D7, */    {REG_RW_PMUX3, 18,  0x1C0000},
	/* PIN_DPI1D6, */    {REG_RW_PMUX3, 21,  0xE00000},
	/* PIN_DPI1D5, */    {REG_RW_PMUX3, 24,  0x7000000},
	/* PIN_DPI1D4, */    {REG_RW_PMUX3, 27,  0x38000000},
	/* PIN_DPI1D3, */    {REG_RW_PMUX4, 0,  0x7},
	/* PIN_DPI1D2, */    {REG_RW_PMUX4, 3,  0x38},
	/* PIN_DPI1D1, */    {REG_RW_PMUX4, 6,  0x1C0},
	/* PIN_DPI1D0, */    {REG_RW_PMUX4, 9,  0xE00},
	/* PIN_DPI0CK, */    {REG_RW_PMUX4, 12,  0x7000},
	/* PIN_DPI0HSYNC, */ {REG_RW_PMUX4, 15,  0x38000},
	/* PIN_DPI0VSYNC, */ {REG_RW_PMUX4, 18,  0x1C0000},
	/* PIN_DPI0D11, */   {REG_RW_PMUX4, 21,  0xE00000},
	/* PIN_DPI0D10, */   {REG_RW_PMUX4, 24,  0x7000000},
	/* PIN_DPI0D9, */    {REG_RW_PMUX4, 27,  0x38000000},
	/* PIN_DPI0D8, */    {REG_RW_PMUX5, 0,  0x7},
	/* PIN_DPI0D7, */    {REG_RW_PMUX5, 3,  0x38},
	/* PIN_DPI0D6, */    {REG_RW_PMUX5, 6,  0x1C0},
	/* PIN_DPI0D5, */    {REG_RW_PMUX5, 9,  0xE00},
	/* PIN_DPI0D4, */    {REG_RW_PMUX5, 12,  0x7000},
	/* PIN_DPI0D3, */    {REG_RW_PMUX5, 15,  0x38000},
	/* PIN_DPI0D2, */    {REG_RW_PMUX5, 18,  0x1C0000},
	/* PIN_DPI0D1, */    {REG_RW_PMUX5, 21,  0xE00000},
	/* PIN_DPI0D0, */    {REG_RW_PMUX5, 24,  0x7000000},
	/* PIN_SDA, */       {REG_RW_PMUX5, 27,  0x38000000},
	/* PIN_SCL, */       {REG_RW_PMUX6, 0,  0x7},
	/* PIN_NRNB, */      {REG_RW_PMUX6, 3,  0x38},
	/* PIN_NCLE, */      {REG_RW_PMUX6, 6,  0x1C0},
	/* PIN_NALE, */      {REG_RW_PMUX6, 9,  0xE00},
	/* PIN_NWEB, */     {REG_RW_PMUX6, 12,  0x7000},
	/* PIN_NREB, */      {REG_RW_PMUX6, 15,  0x38000},
	/* PIN_NLD7, */      {REG_RW_PMUX6, 18,  0x1C0000},
	/* PIN_NLD6, */      {REG_RW_PMUX6, 21,  0xE00000},
	/* PIN_NLD5, */      {REG_RW_PMUX6, 24,  0x7000000},
	/* PIN_NLD4, */      {REG_RW_PMUX6, 27,  0x38000000},
	/* PIN_NLD3, */      {REG_RW_PMUX7, 0,  0x7},
	/* PIN_NLD2, */      {REG_RW_PMUX7, 3,  0x38},
	/* PIN_NLD1, */      {REG_RW_PMUX7, 6,  0x1C0},
	/* PIN_NLD0, */      {REG_RW_PMUX7, 9,  0xE00},
	/* PIN_RTC_32K_CK, */  {REG_RW_PMUX7, 12,  0x7000},
	/* PIN_INT, */        {REG_RW_PMUX7, 15,  0x38000},
	/* PIN_RESET, */      {REG_RW_PMUX7, 18,  0x1C0000},
	/* PIN_EN_BB, */      {REG_RW_PMUX7, 21,  0xE00000},
	/* PIN_CK_SEL, */     {REG_RW_PMUX7, 24,  0x7000000},
	/* PIN_NFRBN, */      {REG_RW_PMUX7, 27,  0x38000000},
	/* PIN_NFCLE, */      {REG_RW_PMUX8, 0,  0x7},
	/* PIN_NFALE, */      {REG_RW_PMUX8, 3,  0x38},
	/* PIN_NFWEN, */     {REG_RW_PMUX8, 6,  0x1C0},
	/* PIN_NFREN, */      {REG_RW_PMUX8, 9,  0xE00},
	/* PIN_NFCEN, */      {REG_RW_PMUX8, 12,  0x7000},
	/* PIN_NFD7, */       {REG_RW_PMUX8, 15,  0x38000},
	/* PIN_PROT0, */      {REG_RW_PMUX9, 21,  0x100000},
	/* PIN_PROT1, */      {REG_RW_PMUX9, 22,  0x200000},
	/* PIN_PROT2, */      {REG_RW_PMUX9, 23,  0x400000},
	/* PIN_PROT3, */      {REG_RW_PMUX9, 24,  0x800000},
	/* PIN_PROT4, */      {REG_RW_PMUX9, 25,  0x1000000},
	/* PIN_PROT5, */      {REG_RW_PMUX9, 26,  0x2000000},
};

/* pu shift, pd shift*/
static const u32 _au4PinmuxPadPuPdTbl[PIN_MAX][2] = {
	/* PIN_NFD6, */     {0, 0},
	/* PIN_NFD5, */     {1, 1},
	/* PIN_NFD4, */     {2, 2},
	/* PIN_NFD3, */     {3, 3},
	/* PIN_NFD2, */     {4, 4},
	/* PIN_NFD1, */     {5, 5},
	/* PIN_NFD0, */     {6, 6},
	/* PIN_TP_VPLL, */   {0, 0},
	/* PIN_AO0N, */     {0, 0},
	/* PIN_AO0P, */     {0, 0},
	/* PIN_AO1N, */     {0, 0},
	/* PIN_AO1P, */     {0, 0},
	/* PIN_AO2N, */     {0, 0},
	/* PIN_AO2P, */     {0, 0},
	/* PIN_AOCK0N, */   {0, 0},
	/* PIN_AOCK0P, */   {0, 0},
	/* PIN_AO3N, */     {0, 0},
	/* PIN_AO3P, */     {0, 0},
	/* PIN_G0, */       {7, 7},
	/* PIN_B5, */       {8, 8},
	/* PIN_B4, */       {9, 9},
	/* PIN_B3, */       {10, 10},
	/* PIN_B2, */       {11, 11},
	/* PIN_B1, */       {12, 12},
	/* PIN_B0, */       {13, 13},
	/* PIN_DE, */       {14, 14},
	/* PIN_VCLK, */     {17, 17},
	/* PIN_HSYNC, */    {16, 16},
	/* PIN_VSYNC, */    {15, 15},
	/* PIN_CEC, */      {0, 0},
	/* PIN_HDMISCK, */  {19, 19},
	/* PIN_HDMISD, */   {20, 20},
	/* PIN_HTPLG, */     {21, 21},
	/* PIN_I2S_DATA, */  {22, 22},
	/* PIN_I2S_LRCK, */  {23, 23},
	/* PIN_I2S_BCK, */   {24, 24},
	/* PIN_DPI1CK, */    {25, 25},
	/* PIN_DPI1D7, */    {26, 26},
	/* PIN_DPI1D6, */    {27, 27},
	/* PIN_DPI1D5, */    {28, 28},
	/* PIN_DPI1D4, */    {29, 29},
	/* PIN_DPI1D3, */    {30, 30},
	/* PIN_DPI1D2, */    {31, 31},
	/* PIN_DPI1D1, */    {32, 32},
	/* PIN_DPI1D0, */    {33, 33},
	/* PIN_DPI0CK, */    {34, 34},
	/* PIN_DPI0HSYNC, */ {35, 35},
	/* PIN_DPI0VSYNC, */ {36, 36},
	/* PIN_DPI0D11, */   {37, 37},
	/* PIN_DPI0D10, */   {38, 38},
	/* PIN_DPI0D9, */    {39, 39},
	/* PIN_DPI0D8, */    {40, 40},
	/* PIN_DPI0D7, */    {41, 41},
	/* PIN_DPI0D6, */    {42, 42},
	/* PIN_DPI0D5, */    {43, 43},
	/* PIN_DPI0D4, */    {44, 44},
	/* PIN_DPI0D3, */    {45, 45},
	/* PIN_DPI0D2, */    {46, 46},
	/* PIN_DPI0D1, */    {47, 47},
	/* PIN_DPI0D0, */    {48, 48},
	/* PIN_SDA, */       {49, 49},
	/* PIN_SCL, */       {50, 50},
	/* PIN_NRNB, */      {51, 51},
	/* PIN_NCLE, */      {52, 52},
	/* PIN_NALE, */      {53, 53},
	/* PIN_NWEB, */     {54, 54},
	/* PIN_NREB, */      {55, 55},
	/* PIN_NLD7, */      {56, 56},
	/* PIN_NLD6, */      {57, 57},
	/* PIN_NLD5, */      {58, 58},
	/* PIN_NLD4, */      {59, 59},
	/* PIN_NLD3, */      {60, 60},
	/* PIN_NLD2, */      {61, 16},
	/* PIN_NLD1, */      {62, 62},
	/* PIN_NLD0, */      {63, 63},
	/* PIN_RTC_32K_CK, */  {0, 0},
	/* PIN_INT, */        {64, 64},
	/* PIN_RESET, */      {0, 0},
	/* PIN_EN_BB, */      {0, 0},
	/* PIN_CK_SEL, */     {0, 0},
	/* PIN_NFRBN, */      {0, 0},
	/* PIN_NFCLE, */      {0, 0},
	/* PIN_NFALE, */      {0, 0},
	/* PIN_NFWEN, */     {0, 0},
	/* PIN_NFREN, */      {0, 0},
	/* PIN_NFCEN, */      {0, 0},
	/* PIN_NFD7, */       {0, 0},
	/* PIN_PROT0, */      {0, 0},
	/* PIN_PROT1, */      {0, 0},
	/* PIN_PROT2, */      {0, 0},
	/* PIN_PROT3, */      {0, 0},
	/* PIN_PROT4, */      {0, 0},
	/* PIN_PROT5, */      {0, 0},
};

extern int mt8193_pinset(u32 pin_num, u32 function);

#endif /* MT8193_H */
