	.cpu generic+fp+simd
	.file	"hacc_export.c"
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.align	2
	.type	sp_hacc_internal.isra.0, %function
sp_hacc_internal.isra.0:
.LFB471:
	.file 1 "hacc_export.c"
	.loc 1 89 0
	.cfi_startproc
.LVL0:
	stp	x29, x30, [sp, -64]!
	.cfi_def_cfa_offset 64
	.cfi_offset 29, -64
	.cfi_offset 30, -56
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	stp	x19, x20, [sp,16]
	.cfi_offset 19, -48
	.cfi_offset 20, -40
	uxtb	w20, w3
.LVL1:
	mov	x19, x0
	stp	x21, x22, [sp,32]
	.cfi_offset 21, -32
	.cfi_offset 22, -24
	.loc 1 96 0
	cmp	w20, 1
	.loc 1 89 0
	mov	w21, w1
	uxtb	w22, w5
.LVL2:
	.loc 1 96 0
	beq	.L24
	.loc 1 111 0
	cmp	w2, 3
	bls	.L25
	.loc 1 169 0
	mov	w0, 24588
.LVL3:
	b	.L12
.LVL4:
.L25:
	.loc 1 111 0
	adrp	x1, .L14
	add	x1, x1, :lo12:.L14
	ldrb	w0, [x1,w2,uxtw]
	adr	x1, .Lrtx14
	add	x0, x1, w0, sxtb #2
	br	x0
.Lrtx14:
	.section	.rodata
	.align	0
	.align	2
.L14:
	.byte	(.L5 - .Lrtx14) / 4
	.byte	(.L7 - .Lrtx14) / 4
	.byte	(.L8 - .Lrtx14) / 4
	.byte	(.L10 - .Lrtx14) / 4
	.text
.L8:
	.loc 1 139 0
	adrp	x0, .LANCHOR1
	str	x4, [x29,56]
	ldrb	w0, [x0,#:lo12:.LANCHOR1]
	cbz	w0, .L16
	.loc 1 146 0
	mov	w0, 2
	mov	w1, 32
	bl	hacc_set_key
.LVL5:
	.loc 1 148 0
	cbnz	w0, .L9
	.loc 1 151 0
	ldr	x4, [x29,56]
.LVL6:
	mov	x1, x19
	mov	x2, x19
	and	w3, w21, -128
	mov	w0, w4
.LVL7:
	bl	hacc_do_aes
.LVL8:
	.loc 1 153 0
	cbz	w0, .L6
.LVL9:
.L9:
	.loc 1 184 0
	cmp	w20, 1
	beq	.L3
.LVL10:
.L12:
	.loc 1 189 0
	mov	w2, w0
.LVL11:
	adrp	x1, .LC1
	adrp	x0, .LC0
.LVL12:
	add	x1, x1, :lo12:.LC1
	add	x0, x0, :lo12:.LC0
	bl	printk
.LVL13:
	.loc 1 191 0
	adrp	x3, .LANCHOR0
	adrp	x0, .LC2
	adrp	x1, .LC3
	add	x3, x3, :lo12:.LANCHOR0
	add	x0, x0, :lo12:.LC2
	add	x1, x1, :lo12:.LC3
	mov	w2, 191
	add	x3, x3, 96
	bl	printk
.LVL14:
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	panic
.LVL15:
.L7:
	.loc 1 128 0
	adrp	x1, .LANCHOR0
	cmp	w22, wzr
	add	x1, x1, :lo12:.LANCHOR0
	cset	w0, ne
	add	x1, x1, 32
.L21:
	.loc 1 161 0
	bl	HACC_V3_Init
.LVL16:
	.loc 1 163 0
	mov	x0, x19
	mov	w1, w21
	mov	x2, x19
	bl	HACC_V3_Run
.LVL17:
	.loc 1 165 0
	bl	HACC_V3_Terminate
.LVL18:
.L6:
	.loc 1 176 0
	cmp	w20, 1
	beq	.L26
	.loc 1 194 0
	mov	x0, x19
.LVL19:
	ldp	x19, x20, [sp,16]
	.cfi_remember_state
	.cfi_restore 20
	.cfi_restore 19
.LVL20:
	ldp	x21, x22, [sp,32]
	.cfi_restore 22
	.cfi_restore 21
.LVL21:
	ldp	x29, x30, [sp], 64
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa 31, 0
	ret
.LVL22:
.L5:
	.cfi_restore_state
	.loc 1 117 0
	cmp	w22, wzr
	adrp	x1, .LANCHOR0
	cset	w0, ne
	add	x1, x1, :lo12:.LANCHOR0
	b	.L21
.L10:
	.loc 1 161 0
	adrp	x1, .LANCHOR0
	cmp	w22, wzr
	add	x1, x1, :lo12:.LANCHOR0
	cset	w0, ne
	add	x1, x1, 64
	b	.L21
.LVL23:
.L26:
	.loc 1 178 0
	bl	osal_hacc_unlock
.LVL24:
	.loc 1 194 0
	mov	x0, x19
	ldp	x19, x20, [sp,16]
	.cfi_remember_state
	.cfi_restore 19
	.cfi_restore 20
.LVL25:
	ldp	x21, x22, [sp,32]
	.cfi_restore 21
	.cfi_restore 22
.LVL26:
	ldp	x29, x30, [sp], 64
	.cfi_restore 29
	.cfi_restore 30
	.cfi_def_cfa 31, 0
	ret
.LVL27:
.L24:
	.cfi_restore_state
	str	x4, [x29,48]
	str	x2, [x29,56]
	.loc 1 99 0
	bl	osal_hacc_lock
.LVL28:
	.loc 1 101 0
	ldr	x2, [x29,56]
.LVL29:
	ldr	x4, [x29,48]
.LVL30:
	cbnz	w0, .L27
	.loc 1 111 0
	cmp	w2, 3
	bls	.L28
	.loc 1 169 0
	mov	w0, 24588
.LVL31:
.L3:
	str	x0, [x29,56]
	.loc 1 186 0
	bl	osal_hacc_unlock
.LVL32:
	ldr	x0, [x29,56]
.LVL33:
	b	.L12
.LVL34:
.L28:
	.loc 1 111 0
	adrp	x0, .L13
	add	x0, x0, :lo12:.L13
	ldrb	w0, [x0,w2,uxtw]
	adr	x1, .Lrtx13
	add	x0, x1, w0, sxtb #2
	br	x0
.Lrtx13:
	.section	.rodata
	.align	0
	.align	2
.L13:
	.byte	(.L5 - .Lrtx13) / 4
	.byte	(.L7 - .Lrtx13) / 4
	.byte	(.L8 - .Lrtx13) / 4
	.byte	(.L10 - .Lrtx13) / 4
	.text
.L16:
	.loc 1 141 0
	mov	w0, 24583
.LVL35:
	b	.L9
.LVL36:
.L27:
	.loc 1 103 0
	mov	w0, 24586
.LVL37:
	b	.L3
	.cfi_endproc
.LFE471:
	.size	sp_hacc_internal.isra.0, .-sp_hacc_internal.isra.0
	.align	2
	.global	masp_hal_sp_hacc_enc
	.type	masp_hal_sp_hacc_enc, %function
masp_hal_sp_hacc_enc:
.LFB467:
	.loc 1 200 0
	.cfi_startproc
.LVL38:
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	.loc 1 201 0
	mov	w2, w3
.LVL39:
	mov	w3, w4
.LVL40:
	mov	w4, 1
.LVL41:
	.loc 1 200 0
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	.loc 1 201 0
	mov	w5, w4
	.loc 1 200 0
	.loc 1 201 0
	bl	sp_hacc_internal.isra.0
.LVL42:
	.loc 1 202 0
	ldp	x29, x30, [sp], 16
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa 31, 0
	ret
	.cfi_endproc
.LFE467:
	.size	masp_hal_sp_hacc_enc, .-masp_hal_sp_hacc_enc
	.align	2
	.global	masp_hal_sp_hacc_dec
	.type	masp_hal_sp_hacc_dec, %function
masp_hal_sp_hacc_dec:
.LFB468:
	.loc 1 209 0
	.cfi_startproc
.LVL43:
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	.loc 1 210 0
	mov	w2, w3
.LVL44:
	mov	w3, w4
.LVL45:
	mov	w4, 0
.LVL46:
	.loc 1 209 0
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	.loc 1 210 0
	mov	w5, w4
	.loc 1 209 0
	.loc 1 210 0
	bl	sp_hacc_internal.isra.0
.LVL47:
	.loc 1 211 0
	ldp	x29, x30, [sp], 16
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa 31, 0
	ret
	.cfi_endproc
.LFE468:
	.size	masp_hal_sp_hacc_dec, .-masp_hal_sp_hacc_dec
	.align	2
	.global	masp_hal_sp_hacc_blk_sz
	.type	masp_hal_sp_hacc_blk_sz, %function
masp_hal_sp_hacc_blk_sz:
.LFB469:
	.loc 1 217 0
	.cfi_startproc
	.loc 1 219 0
	mov	w0, 16
	ret
	.cfi_endproc
.LFE469:
	.size	masp_hal_sp_hacc_blk_sz, .-masp_hal_sp_hacc_blk_sz
	.align	2
	.global	masp_hal_sp_hacc_init
	.type	masp_hal_sp_hacc_init, %function
masp_hal_sp_hacc_init:
.LFB470:
	.loc 1 225 0
	.cfi_startproc
.LVL48:
	stp	x29, x30, [sp, -64]!
	.cfi_def_cfa_offset 64
	.cfi_offset 29, -64
	.cfi_offset 30, -56
	.loc 1 229 0
	cmp	w1, 16
	.loc 1 225 0
	mov	x1, x0
.LVL49:
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	.loc 1 231 0
	mov	w0, 24580
.LVL50:
	.loc 1 229 0
	beq	.L37
.L33:
	.loc 1 244 0
	ldp	x29, x30, [sp], 64
	.cfi_remember_state
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa 31, 0
	ret
.L37:
	.cfi_restore_state
	.loc 1 234 0
	mov	w0, 32
	add	x3, x29, 59
	str	w0, [x29,24]
	mov	x0, 0
.L34:
	add	x2, x29, 24
	add	x5, x2, x0
	.loc 1 237 0 discriminator 3
	ldrb	w2, [x1,x0]
	add	x0, x0, 1
	.loc 1 238 0 discriminator 3
	add	w4, w2, 1
	.loc 1 235 0 discriminator 3
	cmp	x0, 16
	.loc 1 237 0 discriminator 3
	strb	w2, [x5,4]
	.loc 1 238 0 discriminator 3
	strb	w4, [x3],-1
	.loc 1 235 0 discriminator 3
	bne	.L34
	.loc 1 241 0 discriminator 1
	ldrb	w1, [x29,28]
.LVL51:
	adrp	x0, .LC5
	ldrb	w2, [x29,29]
	add	x0, x0, :lo12:.LC5
	ldrb	w3, [x29,30]
	ldrb	w4, [x29,31]
	bl	printk
.LVL52:
	.loc 1 243 0 discriminator 1
	add	x0, x29, 24
	bl	hacc_init
.LVL53:
	b	.L33
	.cfi_endproc
.LFE470:
	.size	masp_hal_sp_hacc_init, .-masp_hal_sp_hacc_init
	.global	bHACC_SWKeyInit
	.global	bHACC_HWWrapKeyInit
	.section	.rodata
	.align	3
.LANCHOR0 = . + 0
	.type	g_HACC_CFG_1, %object
	.size	g_HACC_CFG_1, 32
g_HACC_CFG_1:
	.word	-1630272512
	.word	15238305
	.word	-470776899
	.word	793669002
	.word	-8155556
	.word	-381640477
	.word	-1924272954
	.word	1174145417
	.type	g_HACC_CFG_2, %object
	.size	g_HACC_CFG_2, 32
g_HACC_CFG_2:
	.word	-1437324070
	.word	1431445780
	.word	-470776899
	.word	1431445780
	.word	-1437324070
	.word	-1437324070
	.word	1431445780
	.word	-1437324070
	.type	g_HACC_CFG_3, %object
	.size	g_HACC_CFG_3, 32
g_HACC_CFG_3:
	.word	646231696
	.word	-345528130
	.word	-1592585140
	.word	393941525
	.word	378269286
	.word	310687364
	.word	-549728710
	.word	561999778
	.type	__func__.6732, %object
	.size	__func__.6732, 17
__func__.6732:
	.string	"sp_hacc_internal"
	.section	.rodata.str1.8,"aMS",%progbits,1
	.align	3
.LC0:
	.string	"[%s] HACC Fail (0x%x)\n"
	.zero	1
.LC1:
	.string	"ASF"
	.zero	4
.LC2:
	.string	"BUG: failure at %s:%d/%s()!\n"
	.zero	3
.LC3:
	.zero	2
.LC4:
	.string	"BUG!"
	.zero	3
.LC5:
	.string	"0x%x,0x%x,0x%x,0x%x\n"
	.bss
.LANCHOR1 = . + 0
	.type	bHACC_HWWrapKeyInit, %object
	.size	bHACC_HWWrapKeyInit, 1
bHACC_HWWrapKeyInit:
	.zero	1
	.type	bHACC_SWKeyInit, %object
	.size	bHACC_SWKeyInit, 1
bHACC_SWKeyInit:
	.zero	1
	.text
.Letext0:
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x6ef
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.4byte	.LASF70
	.byte	0x1
	.4byte	.LASF71
	.4byte	.LASF72
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF0
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF1
	.uleb128 0x3
	.byte	0x8
	.4byte	0x41
	.uleb128 0x4
	.4byte	0x46
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF2
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF3
	.uleb128 0x5
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF4
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF5
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF6
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF7
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF8
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF9
	.uleb128 0x6
	.string	"u64"
	.byte	0x2
	.byte	0x19
	.4byte	0x7e
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF10
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF11
	.uleb128 0x7
	.4byte	.LASF12
	.byte	0x4
	.byte	0xa2
	.4byte	0x85
	.uleb128 0x3
	.byte	0x8
	.4byte	0x62
	.uleb128 0x6
	.string	"U8"
	.byte	0x3
	.byte	0x2a
	.4byte	0x62
	.uleb128 0x6
	.string	"U32"
	.byte	0x3
	.byte	0x2e
	.4byte	0x4d
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF13
	.uleb128 0x7
	.4byte	.LASF14
	.byte	0x3
	.byte	0x68
	.4byte	0x62
	.uleb128 0x8
	.byte	0x4
	.byte	0x5
	.byte	0x12
	.4byte	0xf7
	.uleb128 0x9
	.4byte	.LASF15
	.sleb128 0
	.uleb128 0x9
	.4byte	.LASF16
	.sleb128 1
	.uleb128 0x9
	.4byte	.LASF17
	.sleb128 2
	.uleb128 0x9
	.4byte	.LASF18
	.sleb128 3
	.byte	0
	.uleb128 0x7
	.4byte	.LASF19
	.byte	0x5
	.byte	0x17
	.4byte	0xd6
	.uleb128 0x2
	.byte	0x10
	.byte	0x7
	.4byte	.LASF20
	.uleb128 0xa
	.byte	0x4
	.byte	0x6
	.2byte	0x136
	.4byte	0x12b
	.uleb128 0x9
	.4byte	.LASF21
	.sleb128 0
	.uleb128 0x9
	.4byte	.LASF22
	.sleb128 1
	.uleb128 0x9
	.4byte	.LASF23
	.sleb128 2
	.uleb128 0x9
	.4byte	.LASF24
	.sleb128 3
	.byte	0
	.uleb128 0x7
	.4byte	.LASF25
	.byte	0x7
	.byte	0xe
	.4byte	0x4d
	.uleb128 0x7
	.4byte	.LASF26
	.byte	0x7
	.byte	0xf
	.4byte	0x62
	.uleb128 0x8
	.byte	0x4
	.byte	0x8
	.byte	0xa0
	.4byte	0x156
	.uleb128 0x9
	.4byte	.LASF27
	.sleb128 0
	.uleb128 0x9
	.4byte	.LASF28
	.sleb128 1
	.byte	0
	.uleb128 0x7
	.4byte	.LASF29
	.byte	0x8
	.byte	0xa3
	.4byte	0x141
	.uleb128 0x8
	.byte	0x4
	.byte	0x8
	.byte	0xa6
	.4byte	0x17c
	.uleb128 0x9
	.4byte	.LASF30
	.sleb128 16
	.uleb128 0x9
	.4byte	.LASF31
	.sleb128 24
	.uleb128 0x9
	.4byte	.LASF32
	.sleb128 32
	.byte	0
	.uleb128 0x7
	.4byte	.LASF33
	.byte	0x8
	.byte	0xaa
	.4byte	0x161
	.uleb128 0x8
	.byte	0x4
	.byte	0x8
	.byte	0xad
	.4byte	0x1a2
	.uleb128 0x9
	.4byte	.LASF34
	.sleb128 0
	.uleb128 0x9
	.4byte	.LASF35
	.sleb128 1
	.uleb128 0x9
	.4byte	.LASF36
	.sleb128 2
	.byte	0
	.uleb128 0x7
	.4byte	.LASF37
	.byte	0x8
	.byte	0xb1
	.4byte	0x187
	.uleb128 0xb
	.byte	0x24
	.byte	0x8
	.byte	0xb7
	.4byte	0x1ce
	.uleb128 0xc
	.4byte	.LASF38
	.byte	0x8
	.byte	0xb8
	.4byte	0xb9
	.byte	0
	.uleb128 0xc
	.4byte	.LASF39
	.byte	0x8
	.byte	0xb9
	.4byte	0x1ce
	.byte	0x4
	.byte	0
	.uleb128 0xd
	.4byte	0xaf
	.4byte	0x1de
	.uleb128 0xe
	.4byte	0x34
	.byte	0x1f
	.byte	0
	.uleb128 0x7
	.4byte	.LASF40
	.byte	0x8
	.byte	0xba
	.4byte	0x1ad
	.uleb128 0xf
	.4byte	.LASF73
	.byte	0x1
	.byte	0x59
	.4byte	0x266
	.byte	0x1
	.4byte	0x266
	.uleb128 0x10
	.string	"buf"
	.byte	0x1
	.byte	0x59
	.4byte	0x266
	.uleb128 0x11
	.4byte	.LASF38
	.byte	0x1
	.byte	0x59
	.4byte	0x12b
	.uleb128 0x10
	.string	"bAC"
	.byte	0x1
	.byte	0x59
	.4byte	0xcb
	.uleb128 0x11
	.4byte	.LASF41
	.byte	0x1
	.byte	0x59
	.4byte	0xf7
	.uleb128 0x11
	.4byte	.LASF42
	.byte	0x1
	.byte	0x59
	.4byte	0xcb
	.uleb128 0x11
	.4byte	.LASF43
	.byte	0x1
	.byte	0x59
	.4byte	0x156
	.uleb128 0x10
	.string	"bEn"
	.byte	0x1
	.byte	0x59
	.4byte	0xcb
	.uleb128 0x12
	.string	"err"
	.byte	0x1
	.byte	0x5b
	.4byte	0x12b
	.uleb128 0x13
	.4byte	.LASF74
	.byte	0x1
	.byte	0xb7
	.uleb128 0x14
	.4byte	.LASF44
	.4byte	0x27c
	.4byte	.LASF73
	.byte	0
	.uleb128 0x3
	.byte	0x8
	.4byte	0x136
	.uleb128 0xd
	.4byte	0x46
	.4byte	0x27c
	.uleb128 0xe
	.4byte	0x34
	.byte	0x10
	.byte	0
	.uleb128 0x4
	.4byte	0x26c
	.uleb128 0x15
	.4byte	0x1e9
	.8byte	.LFB471
	.8byte	.LFE471-.LFB471
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x38c
	.uleb128 0x16
	.4byte	0x1f9
	.4byte	.LLST0
	.uleb128 0x16
	.4byte	0x204
	.4byte	.LLST1
	.uleb128 0x16
	.4byte	0x21a
	.4byte	.LLST2
	.uleb128 0x16
	.4byte	0x225
	.4byte	.LLST3
	.uleb128 0x16
	.4byte	0x230
	.4byte	.LLST4
	.uleb128 0x16
	.4byte	0x23b
	.4byte	.LLST5
	.uleb128 0x17
	.4byte	0x20f
	.uleb128 0x18
	.4byte	0x246
	.4byte	.LLST6
	.uleb128 0x19
	.4byte	0x251
	.8byte	.L9
	.uleb128 0x1a
	.4byte	0x258
	.uleb128 0x9
	.byte	0x3
	.8byte	__func__.6732
	.uleb128 0x1b
	.8byte	.LVL5
	.4byte	0x611
	.uleb128 0x1b
	.8byte	.LVL8
	.4byte	0x62b
	.uleb128 0x1b
	.8byte	.LVL13
	.4byte	0x64f
	.uleb128 0x1b
	.8byte	.LVL14
	.4byte	0x64f
	.uleb128 0x1b
	.8byte	.LVL15
	.4byte	0x665
	.uleb128 0x1b
	.8byte	.LVL16
	.4byte	0x677
	.uleb128 0x1b
	.8byte	.LVL17
	.4byte	0x698
	.uleb128 0x1b
	.8byte	.LVL18
	.4byte	0x6be
	.uleb128 0x1b
	.8byte	.LVL24
	.4byte	0x6c5
	.uleb128 0x1b
	.8byte	.LVL28
	.4byte	0x6cc
	.uleb128 0x1b
	.8byte	.LVL32
	.4byte	0x6c5
	.byte	0
	.uleb128 0x1c
	.4byte	.LASF45
	.byte	0x1
	.byte	0xc7
	.4byte	0xa9
	.8byte	.LFB467
	.8byte	.LFE467-.LFB467
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x406
	.uleb128 0x1d
	.string	"buf"
	.byte	0x1
	.byte	0xc7
	.4byte	0xa9
	.4byte	.LLST7
	.uleb128 0x1e
	.4byte	.LASF38
	.byte	0x1
	.byte	0xc7
	.4byte	0x4d
	.4byte	.LLST8
	.uleb128 0x1d
	.string	"bAC"
	.byte	0x1
	.byte	0xc7
	.4byte	0x62
	.4byte	.LLST9
	.uleb128 0x1e
	.4byte	.LASF41
	.byte	0x1
	.byte	0xc7
	.4byte	0xf7
	.4byte	.LLST10
	.uleb128 0x1e
	.4byte	.LASF42
	.byte	0x1
	.byte	0xc7
	.4byte	0x62
	.4byte	.LLST11
	.uleb128 0x1b
	.8byte	.LVL42
	.4byte	0x281
	.byte	0
	.uleb128 0x1c
	.4byte	.LASF46
	.byte	0x1
	.byte	0xd0
	.4byte	0xa9
	.8byte	.LFB468
	.8byte	.LFE468-.LFB468
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x480
	.uleb128 0x1d
	.string	"buf"
	.byte	0x1
	.byte	0xd0
	.4byte	0xa9
	.4byte	.LLST12
	.uleb128 0x1e
	.4byte	.LASF38
	.byte	0x1
	.byte	0xd0
	.4byte	0x4d
	.4byte	.LLST13
	.uleb128 0x1d
	.string	"bAC"
	.byte	0x1
	.byte	0xd0
	.4byte	0x62
	.4byte	.LLST14
	.uleb128 0x1e
	.4byte	.LASF41
	.byte	0x1
	.byte	0xd0
	.4byte	0xf7
	.4byte	.LLST15
	.uleb128 0x1e
	.4byte	.LASF42
	.byte	0x1
	.byte	0xd0
	.4byte	0x62
	.4byte	.LLST16
	.uleb128 0x1b
	.8byte	.LVL47
	.4byte	0x281
	.byte	0
	.uleb128 0x1f
	.4byte	.LASF75
	.byte	0x1
	.byte	0xd8
	.4byte	0x4d
	.8byte	.LFB469
	.8byte	.LFE469-.LFB469
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x1c
	.4byte	.LASF47
	.byte	0x1
	.byte	0xe0
	.4byte	0x4d
	.8byte	.LFB470
	.8byte	.LFE470-.LFB470
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x50e
	.uleb128 0x1e
	.4byte	.LASF48
	.byte	0x1
	.byte	0xe0
	.4byte	0xa9
	.4byte	.LLST17
	.uleb128 0x1e
	.4byte	.LASF38
	.byte	0x1
	.byte	0xe0
	.4byte	0x4d
	.4byte	.LLST18
	.uleb128 0x20
	.4byte	.LASF49
	.byte	0x1
	.byte	0xe2
	.4byte	0x1de
	.uleb128 0x2
	.byte	0x91
	.sleb128 -40
	.uleb128 0x12
	.string	"i"
	.byte	0x1
	.byte	0xe3
	.4byte	0x12b
	.uleb128 0x1b
	.8byte	.LVL52
	.4byte	0x64f
	.uleb128 0x1b
	.8byte	.LVL53
	.4byte	0x6d7
	.byte	0
	.uleb128 0xd
	.4byte	0x12b
	.4byte	0x51e
	.uleb128 0xe
	.4byte	0x34
	.byte	0x7
	.byte	0
	.uleb128 0x20
	.4byte	.LASF50
	.byte	0x1
	.byte	0x46
	.4byte	0x533
	.uleb128 0x9
	.byte	0x3
	.8byte	g_HACC_CFG_1
	.uleb128 0x4
	.4byte	0x50e
	.uleb128 0x20
	.4byte	.LASF51
	.byte	0x1
	.byte	0x4b
	.4byte	0x54d
	.uleb128 0x9
	.byte	0x3
	.8byte	g_HACC_CFG_2
	.uleb128 0x4
	.4byte	0x50e
	.uleb128 0x20
	.4byte	.LASF52
	.byte	0x1
	.byte	0x50
	.4byte	0x567
	.uleb128 0x9
	.byte	0x3
	.8byte	g_HACC_CFG_3
	.uleb128 0x4
	.4byte	0x50e
	.uleb128 0xd
	.4byte	0x54
	.4byte	0x577
	.uleb128 0x21
	.byte	0
	.uleb128 0x22
	.4byte	.LASF53
	.byte	0x9
	.byte	0x24
	.4byte	0x56c
	.uleb128 0xd
	.4byte	0x46
	.4byte	0x58d
	.uleb128 0x21
	.byte	0
	.uleb128 0x23
	.4byte	.LASF54
	.byte	0xa
	.2byte	0x1b2
	.4byte	0x599
	.uleb128 0x4
	.4byte	0x582
	.uleb128 0x22
	.4byte	.LASF55
	.byte	0xb
	.byte	0x61
	.4byte	0x9e
	.uleb128 0x22
	.4byte	.LASF56
	.byte	0xc
	.byte	0xa
	.4byte	0x54
	.uleb128 0xd
	.4byte	0x2d
	.4byte	0x5ca
	.uleb128 0xe
	.4byte	0x34
	.byte	0x40
	.uleb128 0xe
	.4byte	0x34
	.byte	0
	.byte	0
	.uleb128 0x23
	.4byte	.LASF57
	.byte	0xd
	.2byte	0x2f9
	.4byte	0x5d6
	.uleb128 0x4
	.4byte	0x5b4
	.uleb128 0x23
	.4byte	.LASF58
	.byte	0xe
	.2byte	0x1db
	.4byte	0x2d
	.uleb128 0x24
	.4byte	.LASF59
	.byte	0x1
	.byte	0x40
	.4byte	0xcb
	.uleb128 0x9
	.byte	0x3
	.8byte	bHACC_HWWrapKeyInit
	.uleb128 0x24
	.4byte	.LASF60
	.byte	0x1
	.byte	0x41
	.4byte	0xcb
	.uleb128 0x9
	.byte	0x3
	.8byte	bHACC_SWKeyInit
	.uleb128 0x25
	.4byte	.LASF61
	.byte	0x1
	.byte	0x2f
	.4byte	0x4d
	.4byte	0x62b
	.uleb128 0x26
	.4byte	0x1a2
	.uleb128 0x26
	.4byte	0x17c
	.byte	0
	.uleb128 0x25
	.4byte	.LASF62
	.byte	0x1
	.byte	0x30
	.4byte	0x4d
	.4byte	0x64f
	.uleb128 0x26
	.4byte	0x156
	.uleb128 0x26
	.4byte	0xa9
	.uleb128 0x26
	.4byte	0xa9
	.uleb128 0x26
	.4byte	0x4d
	.byte	0
	.uleb128 0x25
	.4byte	.LASF63
	.byte	0x9
	.byte	0x7e
	.4byte	0x54
	.4byte	0x665
	.uleb128 0x26
	.4byte	0x3b
	.uleb128 0x27
	.byte	0
	.uleb128 0x28
	.4byte	.LASF64
	.byte	0xa
	.byte	0xd0
	.4byte	0x677
	.uleb128 0x26
	.4byte	0x3b
	.uleb128 0x27
	.byte	0
	.uleb128 0x28
	.4byte	.LASF65
	.byte	0x1
	.byte	0x33
	.4byte	0x68d
	.uleb128 0x26
	.4byte	0x97
	.uleb128 0x26
	.4byte	0x68d
	.byte	0
	.uleb128 0x3
	.byte	0x8
	.4byte	0x693
	.uleb128 0x4
	.4byte	0x4d
	.uleb128 0x28
	.4byte	.LASF66
	.byte	0x1
	.byte	0x34
	.4byte	0x6b3
	.uleb128 0x26
	.4byte	0x6b3
	.uleb128 0x26
	.4byte	0xb9
	.uleb128 0x26
	.4byte	0x6b3
	.byte	0
	.uleb128 0x3
	.byte	0x8
	.4byte	0x6b9
	.uleb128 0x29
	.4byte	0xb9
	.uleb128 0x2a
	.4byte	.LASF67
	.byte	0x1
	.byte	0x35
	.uleb128 0x2a
	.4byte	.LASF68
	.byte	0xf
	.byte	0x17
	.uleb128 0x2b
	.4byte	.LASF76
	.byte	0xf
	.byte	0x16
	.4byte	0x54
	.uleb128 0x25
	.4byte	.LASF69
	.byte	0x1
	.byte	0x31
	.4byte	0x4d
	.4byte	0x6ec
	.uleb128 0x26
	.4byte	0x6ec
	.byte	0
	.uleb128 0x3
	.byte	0x8
	.4byte	0x1de
	.byte	0
	.section	.debug_abbrev,"",%progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x4
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xd
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x4
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0xa
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x34
	.uleb128 0x19
	.uleb128 0x1c
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x16
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x17
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x18
	.uleb128 0x34
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x19
	.uleb128 0xa
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.byte	0
	.byte	0
	.uleb128 0x1a
	.uleb128 0x34
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x1b
	.uleb128 0x4109
	.byte	0
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1c
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1d
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x1e
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x1f
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x20
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x21
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x22
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x23
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x24
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x25
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x26
	.uleb128 0x5
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x27
	.uleb128 0x18
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x28
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x29
	.uleb128 0x35
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x2a
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x2b
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",%progbits
.Ldebug_loc0:
.LLST0:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL4-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL4-.Ltext0
	.8byte	.LVL20-.Ltext0
	.2byte	0x1
	.byte	0x63
	.8byte	.LVL22-.Ltext0
	.8byte	.LVL25-.Ltext0
	.2byte	0x1
	.byte	0x63
	.8byte	.LVL27-.Ltext0
	.8byte	.LFE471-.Ltext0
	.2byte	0x1
	.byte	0x63
	.8byte	0
	.8byte	0
.LLST1:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL10-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL10-.Ltext0
	.8byte	.LVL21-.Ltext0
	.2byte	0x1
	.byte	0x65
	.8byte	.LVL22-.Ltext0
	.8byte	.LVL26-.Ltext0
	.2byte	0x1
	.byte	0x65
	.8byte	.LVL27-.Ltext0
	.8byte	.LFE471-.Ltext0
	.2byte	0x1
	.byte	0x65
	.8byte	0
	.8byte	0
.LLST2:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL5-1-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL9-.Ltext0
	.8byte	.LVL11-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL15-.Ltext0
	.8byte	.LVL16-1-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL22-.Ltext0
	.8byte	.LVL23-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL27-.Ltext0
	.8byte	.LVL28-1-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL29-.Ltext0
	.8byte	.LVL32-1-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL34-.Ltext0
	.8byte	.LFE471-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	0
	.8byte	0
.LLST3:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL1-.Ltext0
	.2byte	0x1
	.byte	0x53
	.8byte	.LVL1-.Ltext0
	.8byte	.LVL20-.Ltext0
	.2byte	0x1
	.byte	0x64
	.8byte	.LVL22-.Ltext0
	.8byte	.LVL25-.Ltext0
	.2byte	0x1
	.byte	0x64
	.8byte	.LVL27-.Ltext0
	.8byte	.LFE471-.Ltext0
	.2byte	0x1
	.byte	0x64
	.8byte	0
	.8byte	0
.LLST4:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL5-1-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL6-.Ltext0
	.8byte	.LVL8-1-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL9-.Ltext0
	.8byte	.LVL13-1-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL15-.Ltext0
	.8byte	.LVL16-1-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL22-.Ltext0
	.8byte	.LVL23-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL27-.Ltext0
	.8byte	.LVL28-1-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL30-.Ltext0
	.8byte	.LVL32-1-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL34-.Ltext0
	.8byte	.LFE471-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	0
	.8byte	0
.LLST5:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL2-.Ltext0
	.2byte	0x1
	.byte	0x55
	.8byte	.LVL2-.Ltext0
	.8byte	.LVL21-.Ltext0
	.2byte	0x1
	.byte	0x66
	.8byte	.LVL22-.Ltext0
	.8byte	.LVL26-.Ltext0
	.2byte	0x1
	.byte	0x66
	.8byte	.LVL27-.Ltext0
	.8byte	.LFE471-.Ltext0
	.2byte	0x1
	.byte	0x66
	.8byte	0
	.8byte	0
.LLST6:
	.8byte	.LVL3-.Ltext0
	.8byte	.LVL4-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL5-.Ltext0
	.8byte	.LVL7-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL8-.Ltext0
	.8byte	.LVL12-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL18-.Ltext0
	.8byte	.LVL19-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL23-.Ltext0
	.8byte	.LVL24-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL31-.Ltext0
	.8byte	.LVL32-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL33-.Ltext0
	.8byte	.LVL34-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL35-.Ltext0
	.8byte	.LVL36-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL37-.Ltext0
	.8byte	.LFE471-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	0
	.8byte	0
.LLST7:
	.8byte	.LVL38-.Ltext0
	.8byte	.LVL42-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	0
	.8byte	0
.LLST8:
	.8byte	.LVL38-.Ltext0
	.8byte	.LVL42-1-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST9:
	.8byte	.LVL38-.Ltext0
	.8byte	.LVL39-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	0
	.8byte	0
.LLST10:
	.8byte	.LVL38-.Ltext0
	.8byte	.LVL40-.Ltext0
	.2byte	0x1
	.byte	0x53
	.8byte	0
	.8byte	0
.LLST11:
	.8byte	.LVL38-.Ltext0
	.8byte	.LVL41-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	0
	.8byte	0
.LLST12:
	.8byte	.LVL43-.Ltext0
	.8byte	.LVL47-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	0
	.8byte	0
.LLST13:
	.8byte	.LVL43-.Ltext0
	.8byte	.LVL47-1-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST14:
	.8byte	.LVL43-.Ltext0
	.8byte	.LVL44-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	0
	.8byte	0
.LLST15:
	.8byte	.LVL43-.Ltext0
	.8byte	.LVL45-.Ltext0
	.2byte	0x1
	.byte	0x53
	.8byte	0
	.8byte	0
.LLST16:
	.8byte	.LVL43-.Ltext0
	.8byte	.LVL46-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	0
	.8byte	0
.LLST17:
	.8byte	.LVL48-.Ltext0
	.8byte	.LVL50-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL50-.Ltext0
	.8byte	.LVL51-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST18:
	.8byte	.LVL48-.Ltext0
	.8byte	.LVL49-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
	.section	.debug_aranges,"",%progbits
	.4byte	0x2c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.8byte	0
	.8byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF61:
	.string	"hacc_set_key"
.LASF14:
	.string	"BOOL"
.LASF6:
	.string	"short int"
.LASF1:
	.string	"sizetype"
.LASF60:
	.string	"bHACC_SWKeyInit"
.LASF71:
.LASF33:
	.string	"AES_KEY"
.LASF28:
	.string	"AES_ENC"
.LASF64:
	.string	"panic"
.LASF8:
	.string	"long long int"
.LASF74:
	.string	"_err"
.LASF68:
	.string	"osal_hacc_unlock"
.LASF56:
	.string	"debug_locks"
.LASF53:
	.string	"console_printk"
.LASF65:
	.string	"HACC_V3_Init"
.LASF67:
	.string	"HACC_V3_Terminate"
.LASF16:
	.string	"HACC_USER2"
.LASF13:
	.string	"float"
.LASF72:
	.string	"/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/out/target/product/evb6735_64/obj/KERNEL_OBJ"
.LASF20:
	.string	"__int128 unsigned"
.LASF32:
	.string	"AES_KEY_256"
.LASF47:
	.string	"masp_hal_sp_hacc_init"
.LASF59:
	.string	"bHACC_HWWrapKeyInit"
.LASF10:
	.string	"long int"
.LASF66:
	.string	"HACC_V3_Run"
.LASF45:
	.string	"masp_hal_sp_hacc_enc"
.LASF70:
	.ascii	"GNU C 4.9 20140514 (prerelease) -fpreprocessed -mbionic -mli"
	.ascii	"ttle-end"
.LASF63:
	.string	"printk"
.LASF37:
	.string	"AES_KEY_ID"
.LASF26:
	.string	"uchar"
.LASF19:
	.string	"HACC_USER"
.LASF5:
	.string	"unsigned char"
.LASF44:
	.string	"__func__"
.LASF31:
	.string	"AES_KEY_192"
.LASF69:
	.string	"hacc_init"
.LASF4:
	.string	"signed char"
.LASF9:
	.string	"long long unsigned int"
.LASF34:
	.string	"AES_SW_KEY"
.LASF3:
	.string	"unsigned int"
.LASF22:
	.string	"MM_ANONPAGES"
.LASF76:
	.string	"osal_hacc_lock"
.LASF29:
	.string	"AES_OPS"
.LASF50:
	.string	"g_HACC_CFG_1"
.LASF51:
	.string	"g_HACC_CFG_2"
.LASF52:
	.string	"g_HACC_CFG_3"
.LASF41:
	.string	"user"
.LASF46:
	.string	"masp_hal_sp_hacc_dec"
.LASF35:
	.string	"AES_HW_KEY"
.LASF57:
	.string	"cpu_bit_bitmap"
.LASF15:
	.string	"HACC_USER1"
.LASF2:
	.string	"char"
.LASF17:
	.string	"HACC_USER3"
.LASF18:
	.string	"HACC_USER4"
.LASF39:
	.string	"seed"
.LASF30:
	.string	"AES_KEY_128"
.LASF7:
	.string	"short unsigned int"
.LASF11:
	.string	"_Bool"
.LASF43:
	.string	"aes_type"
.LASF75:
	.string	"masp_hal_sp_hacc_blk_sz"
.LASF40:
	.string	"AES_KEY_SEED"
.LASF62:
	.string	"hacc_do_aes"
.LASF25:
	.string	"uint32"
.LASF0:
	.string	"long unsigned int"
.LASF49:
	.string	"keyseed"
.LASF38:
	.string	"size"
.LASF36:
	.string	"AES_HW_WRAP_KEY"
.LASF54:
	.string	"hex_asc"
.LASF42:
	.string	"bDoLock"
.LASF27:
	.string	"AES_DEC"
.LASF58:
	.string	"zero_pfn"
.LASF73:
	.string	"sp_hacc_internal"
.LASF55:
	.string	"memstart_addr"
.LASF24:
	.string	"NR_MM_COUNTERS"
.LASF21:
	.string	"MM_FILEPAGES"
.LASF12:
	.string	"phys_addr_t"
.LASF48:
	.string	"sec_seed"
.LASF23:
	.string	"MM_SWAPENTS"
	.ident	"GCC: (GNU) 4.9 20140514 (prerelease)"
