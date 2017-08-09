	.cpu generic+fp+simd
	.file	"hacc_hk.c"
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.align	2
	.global	HACC_V3_Init
	.type	HACC_V3_Init, %function
HACC_V3_Init:
.LFB466:
	.file 1 "hacc_hk.c"
	.loc 1 63 0
	.cfi_startproc
.LVL0:
	stp	x29, x30, [sp, -48]!
	.cfi_def_cfa_offset 48
	.cfi_offset 29, -48
	.cfi_offset 30, -40
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	str	x19, [sp,16]
	.cfi_offset 19, -32
	.loc 1 63 0
	uxtb	w19, w0
.LVL1:
	str	x1, [x29,40]
	.loc 1 73 0
	bl	masp_hal_secure_algo_init
.LVL2:
	.loc 1 95 0
	mov	w0, 2
	cmp	w19, wzr
	mov	w7, 3
	ldr	x1, [x29,40]
.LVL3:
	csel	w7, w7, w0, ne
.LVL4:
	adrp	x2, hacc_base
	mov	x0, 0
.L3:
	ldr	x3, [x2,#:lo12:hacc_base]
	.loc 1 105 0 discriminator 3
	add	x4, x2, :lo12:hacc_base
	add	x3, x3, 32
	str	wzr, [x3,x0]
	add	x0, x0, 4
	.loc 1 103 0 discriminator 3
	cmp	x0, 32
	bne	.L3
	.loc 1 111 0
	ldr	x3, [x4]
	mov	w5, 2
	.loc 1 115 0
	mov	w6, 16
	.loc 1 120 0
	mov	x0, 0
	.loc 1 111 0
	str	w5, [x3,4]
	.loc 1 115 0
	ldr	x3, [x4]
	str	w6, [x3,12]
	.loc 1 117 0
	ldr	x6, [x4]
	ldr	w3, [x6,12]
	orr	w3, w3, 256
	str	w3, [x6,12]
	.loc 1 120 0
	ldr	x3, [x4]
	str	w5, [x3,8]
.L4:
	ldr	x3, [x2,#:lo12:hacc_base]
	.loc 1 126 0 discriminator 3
	add	x5, x2, :lo12:hacc_base
	ldr	w4, [x1,x0]
	add	x3, x3, 64
	str	w4, [x3,x0]
	add	x0, x0, 4
	.loc 1 124 0 discriminator 3
	cmp	x0, 16
	bne	.L4
	ldr	x4, [x5]
	adrp	x5, .LANCHOR0
	add	x5, x5, :lo12:.LANCHOR0
	.loc 1 139 0
	mov	w9, 1
	add	x8, x5, 48
.L8:
	.loc 1 133 0
	mov	x0, 0
	add	x3, x0, 16
	.loc 1 136 0
	ldr	w6, [x5,x0]
	add	x0, x0, 4
	.loc 1 134 0
	cmp	x0, 16
	.loc 1 136 0
	str	w6, [x3,x4]
	.loc 1 134 0
	beq	.L5
.L18:
	add	x3, x0, 16
	ldr	x4, [x2,#:lo12:hacc_base]
	.loc 1 136 0
	ldr	w6, [x5,x0]
	add	x0, x0, 4
	.loc 1 134 0
	cmp	x0, 16
	.loc 1 136 0
	str	w6, [x3,x4]
	.loc 1 134 0
	bne	.L18
.L5:
	.loc 1 139 0
	ldr	x0, [x2,#:lo12:hacc_base]
	str	w9, [x0,8]
	ldr	x4, [x2,#:lo12:hacc_base]
	add	x3, x4, 8
.L7:
	.loc 1 141 0 discriminator 1
	ldr	w0, [x3]
	tbz	x0, 15, .L7
	add	x5, x5, 16
	.loc 1 130 0 discriminator 2
	cmp	x5, x8
	bne	.L8
	.loc 1 150 0
	mov	w4, 2
	mov	x0, 0
	str	w4, [x3]
.L9:
	ldr	x3, [x2,#:lo12:hacc_base]
	.loc 1 156 0 discriminator 3
	add	x5, x2, :lo12:hacc_base
	ldr	w4, [x1,x0]
	add	x3, x3, 64
	str	w4, [x3,x0]
	add	x0, x0, 4
	.loc 1 154 0 discriminator 3
	cmp	x0, 16
	bne	.L9
	.loc 1 160 0
	ldr	x0, [x5]
	str	w7, [x0,4]
	.loc 1 161 0
	ldr	x0, [x5]
	str	wzr, [x0,12]
	.loc 1 162 0
	ldr	x19, [sp,16]
	.cfi_restore 19
.LVL5:
	ldp	x29, x30, [sp], 48
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa 31, 0
	ret
	.cfi_endproc
.LFE466:
	.size	HACC_V3_Init, .-HACC_V3_Init
	.align	2
	.global	HACC_V3_Run
	.type	HACC_V3_Run, %function
HACC_V3_Run:
.LFB467:
	.loc 1 165 0
	.cfi_startproc
.LVL6:
	.loc 1 172 0
	mov	x7, x2
	adrp	x5, hacc_base
	.loc 1 180 0
	mov	w9, 1
	.loc 1 172 0
	cbz	w1, .L19
.LVL7:
.L28:
	.loc 1 165 0
	mov	x3, 0
.LVL8:
.L21:
	ldr	x4, [x5,#:lo12:hacc_base]
	.loc 1 177 0 discriminator 3
	add	x8, x5, :lo12:hacc_base
	ldr	w6, [x0,x3]
	add	x4, x4, 16
	str	w6, [x3,x4]
	add	x3, x3, 4
	.loc 1 175 0 discriminator 3
	cmp	x3, 16
	bne	.L21
	.loc 1 180 0
	ldr	x3, [x8]
	str	w9, [x3,8]
	ldr	x6, [x8]
	add	x4, x6, 8
.L22:
	.loc 1 182 0 discriminator 1
	ldr	w3, [x4]
	tbz	x3, 15, .L22
	.loc 1 182 0 is_stmt 0 discriminator 3
	mov	x3, 0
	add	x4, x3, 80
	.loc 1 186 0 is_stmt 1 discriminator 3
	ldr	w4, [x4,x6]
	str	w4, [x7,x3]
	add	x3, x3, 4
	.loc 1 184 0 discriminator 3
	cmp	x3, 16
	beq	.L23
.L32:
	ldr	x6, [x5,#:lo12:hacc_base]
	add	x4, x3, 80
	.loc 1 186 0
	ldr	w4, [x4,x6]
	str	w4, [x7,x3]
	add	x3, x3, 4
	.loc 1 184 0
	cmp	x3, 16
	bne	.L32
.L23:
	.loc 1 172 0 discriminator 2
	add	x7, x7, 16
	add	x0, x0, 16
.LVL9:
	sub	w3, w7, w2
.LVL10:
	cmp	w1, w3
	bhi	.L28
.L19:
	.loc 1 189 0
	ret
	.cfi_endproc
.LFE467:
	.size	HACC_V3_Run, .-HACC_V3_Run
	.align	2
	.global	HACC_V3_Terminate
	.type	HACC_V3_Terminate, %function
HACC_V3_Terminate:
.LFB468:
	.loc 1 192 0
	.cfi_startproc
	.loc 1 196 0
	adrp	x2, hacc_base
	mov	w3, 2
	.loc 1 192 0
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	.loc 1 196 0
	mov	x0, 0
	.loc 1 192 0
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	.loc 1 196 0
	ldr	x1, [x2,#:lo12:hacc_base]
	str	w3, [x1,8]
.L34:
	ldr	x1, [x2,#:lo12:hacc_base]
	add	x1, x1, 32
	.loc 1 204 0 discriminator 3
	str	wzr, [x1,x0]
	add	x0, x0, 4
	.loc 1 202 0 discriminator 3
	cmp	x0, 32
	bne	.L34
	.loc 1 207 0
	bl	masp_hal_secure_algo_deinit
.LVL11:
	.loc 1 208 0
	ldp	x29, x30, [sp], 16
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa 31, 0
	ret
	.cfi_endproc
.LFE468:
	.size	HACC_V3_Terminate, .-HACC_V3_Terminate
	.section	.rodata
	.align	3
.LANCHOR0 = . + 0
	.type	g_CFG_RANDOM_PATTERN, %object
	.size	g_CFG_RANDOM_PATTERN, 48
g_CFG_RANDOM_PATTERN:
	.word	759479152
	.word	-1488661977
	.word	-794196405
	.word	-2080390588
	.word	2127046251
	.word	1139281842
	.word	27669642
	.word	543685024
	.word	1414751237
	.word	-716790095
	.word	482602042
	.word	-566789501
	.text
.Letext0:
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x2c6
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.4byte	.LASF39
	.byte	0x1
	.4byte	.LASF40
	.4byte	.LASF41
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
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF2
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF3
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
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF8
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF9
	.uleb128 0x4
	.string	"u64"
	.byte	0xc
	.byte	0x19
	.4byte	0x73
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF10
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF11
	.uleb128 0x5
	.4byte	.LASF12
	.byte	0x2
	.byte	0xa2
	.4byte	0x7a
	.uleb128 0x2
	.byte	0x10
	.byte	0x7
	.4byte	.LASF13
	.uleb128 0x6
	.byte	0x4
	.byte	0xd
	.2byte	0x136
	.4byte	0xc7
	.uleb128 0x7
	.4byte	.LASF14
	.sleb128 0
	.uleb128 0x7
	.4byte	.LASF15
	.sleb128 1
	.uleb128 0x7
	.4byte	.LASF16
	.sleb128 2
	.uleb128 0x7
	.4byte	.LASF17
	.sleb128 3
	.byte	0
	.uleb128 0x5
	.4byte	.LASF18
	.byte	0x3
	.byte	0xe
	.4byte	0x42
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF19
	.uleb128 0x8
	.4byte	.LASF22
	.byte	0x1
	.byte	0x3e
	.8byte	.LFB466
	.8byte	.LFE466-.LFB466
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x14b
	.uleb128 0x9
	.4byte	.LASF20
	.byte	0x1
	.byte	0x3e
	.4byte	0x8c
	.4byte	.LLST0
	.uleb128 0x9
	.4byte	.LASF21
	.byte	0x1
	.byte	0x3e
	.4byte	0x14b
	.4byte	.LLST1
	.uleb128 0xa
	.string	"i"
	.byte	0x1
	.byte	0x41
	.4byte	0xc7
	.uleb128 0xa
	.string	"j"
	.byte	0x1
	.byte	0x41
	.4byte	0xc7
	.uleb128 0xa
	.string	"p1"
	.byte	0x1
	.byte	0x42
	.4byte	0x14b
	.uleb128 0xb
	.4byte	.LASF27
	.byte	0x1
	.byte	0x44
	.4byte	0xc7
	.uleb128 0x1
	.byte	0x57
	.uleb128 0xc
	.8byte	.LVL2
	.4byte	0x2b3
	.byte	0
	.uleb128 0xd
	.byte	0x8
	.4byte	0x151
	.uleb128 0xe
	.4byte	0xc7
	.uleb128 0x8
	.4byte	.LASF23
	.byte	0x1
	.byte	0xa4
	.8byte	.LFB467
	.8byte	.LFE467-.LFB467
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x1be
	.uleb128 0xf
	.4byte	.LASF24
	.byte	0x1
	.byte	0xa4
	.4byte	0x1be
	.uleb128 0x1
	.byte	0x50
	.uleb128 0xf
	.4byte	.LASF25
	.byte	0x1
	.byte	0xa4
	.4byte	0xc7
	.uleb128 0x1
	.byte	0x51
	.uleb128 0x9
	.4byte	.LASF26
	.byte	0x1
	.byte	0xa4
	.4byte	0x1be
	.4byte	.LLST2
	.uleb128 0x10
	.string	"i"
	.byte	0x1
	.byte	0xa6
	.4byte	0xc7
	.4byte	.LLST3
	.uleb128 0xa
	.string	"j"
	.byte	0x1
	.byte	0xa6
	.4byte	0xc7
	.uleb128 0xa
	.string	"len"
	.byte	0x1
	.byte	0xa9
	.4byte	0xc7
	.byte	0
	.uleb128 0xd
	.byte	0x8
	.4byte	0x1c4
	.uleb128 0x11
	.4byte	0xc7
	.uleb128 0x8
	.4byte	.LASF28
	.byte	0x1
	.byte	0xbf
	.8byte	.LFB468
	.8byte	.LFE468-.LFB468
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x1fd
	.uleb128 0xa
	.string	"i"
	.byte	0x1
	.byte	0xc1
	.4byte	0xc7
	.uleb128 0xc
	.8byte	.LVL11
	.4byte	0x2be
	.byte	0
	.uleb128 0x12
	.4byte	0xc7
	.4byte	0x213
	.uleb128 0x13
	.4byte	0x34
	.byte	0x2
	.uleb128 0x13
	.4byte	0x34
	.byte	0x3
	.byte	0
	.uleb128 0xb
	.4byte	.LASF29
	.byte	0x1
	.byte	0x34
	.4byte	0x228
	.uleb128 0x9
	.byte	0x3
	.8byte	g_CFG_RANDOM_PATTERN
	.uleb128 0xe
	.4byte	0x1fd
	.uleb128 0x12
	.4byte	0x65
	.4byte	0x238
	.uleb128 0x14
	.byte	0
	.uleb128 0x15
	.4byte	.LASF30
	.byte	0x4
	.byte	0x24
	.4byte	0x22d
	.uleb128 0x12
	.4byte	0x3b
	.4byte	0x24e
	.uleb128 0x14
	.byte	0
	.uleb128 0x16
	.4byte	.LASF31
	.byte	0x5
	.2byte	0x1b2
	.4byte	0x25a
	.uleb128 0xe
	.4byte	0x243
	.uleb128 0x15
	.4byte	.LASF32
	.byte	0x6
	.byte	0x61
	.4byte	0x93
	.uleb128 0x15
	.4byte	.LASF33
	.byte	0x7
	.byte	0xa
	.4byte	0x65
	.uleb128 0x12
	.4byte	0x2d
	.4byte	0x28b
	.uleb128 0x13
	.4byte	0x34
	.byte	0x40
	.uleb128 0x13
	.4byte	0x34
	.byte	0
	.byte	0
	.uleb128 0x16
	.4byte	.LASF34
	.byte	0x8
	.2byte	0x2f9
	.4byte	0x297
	.uleb128 0xe
	.4byte	0x275
	.uleb128 0x16
	.4byte	.LASF35
	.byte	0x9
	.2byte	0x1db
	.4byte	0x2d
	.uleb128 0x15
	.4byte	.LASF36
	.byte	0xa
	.byte	0x34
	.4byte	0x73
	.uleb128 0x17
	.4byte	.LASF37
	.byte	0xb
	.byte	0x1c
	.4byte	0x50
	.uleb128 0x17
	.4byte	.LASF38
	.byte	0xb
	.byte	0x1d
	.4byte	0x50
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
	.uleb128 0x4
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
	.uleb128 0x5
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
	.uleb128 0x6
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
	.uleb128 0x7
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xd
	.byte	0
	.byte	0
	.uleb128 0x8
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
	.uleb128 0x9
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
	.uleb128 0xa
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
	.uleb128 0xb
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
	.uleb128 0xc
	.uleb128 0x4109
	.byte	0
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xf
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
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x10
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
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x35
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x15
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
	.uleb128 0x16
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
	.uleb128 0x17
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
	.8byte	.LVL1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL1-.Ltext0
	.8byte	.LVL5-.Ltext0
	.2byte	0x1
	.byte	0x63
	.8byte	0
	.8byte	0
.LLST1:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL2-1-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL3-.Ltext0
	.8byte	.LFE466-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST2:
	.8byte	.LVL6-.Ltext0
	.8byte	.LVL9-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	0
	.8byte	0
.LLST3:
	.8byte	.LVL7-.Ltext0
	.8byte	.LVL8-.Ltext0
	.2byte	0x1
	.byte	0x53
	.8byte	.LVL10-.Ltext0
	.8byte	.LFE467-.Ltext0
	.2byte	0x1
	.byte	0x53
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
.LASF13:
	.string	"__int128 unsigned"
.LASF30:
	.string	"console_printk"
.LASF32:
	.string	"memstart_addr"
.LASF27:
	.string	"acon_setting"
.LASF29:
	.string	"g_CFG_RANDOM_PATTERN"
.LASF24:
	.string	"p_src"
.LASF35:
	.string	"zero_pfn"
.LASF34:
	.string	"cpu_bit_bitmap"
.LASF25:
	.string	"src_len"
.LASF19:
	.string	"float"
.LASF9:
	.string	"long long unsigned int"
.LASF5:
	.string	"unsigned char"
.LASF14:
	.string	"MM_FILEPAGES"
.LASF0:
	.string	"long unsigned int"
.LASF7:
	.string	"short unsigned int"
.LASF16:
	.string	"MM_SWAPENTS"
.LASF28:
	.string	"HACC_V3_Terminate"
.LASF21:
	.string	"g_AC_CFG"
.LASF40:
	.string	"hacc_hk.c"
.LASF23:
	.string	"HACC_V3_Run"
.LASF18:
	.string	"uint32"
.LASF37:
	.string	"masp_hal_secure_algo_init"
.LASF3:
	.string	"unsigned int"
.LASF2:
	.string	"char"
.LASF22:
	.string	"HACC_V3_Init"
.LASF26:
	.string	"p_dst"
.LASF17:
	.string	"NR_MM_COUNTERS"
.LASF12:
	.string	"phys_addr_t"
.LASF1:
	.string	"sizetype"
.LASF8:
	.string	"long long int"
.LASF41:
	.string	"/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/out/target/product/evb6735_64/obj/KERNEL_OBJ"
.LASF38:
	.string	"masp_hal_secure_algo_deinit"
.LASF6:
	.string	"short int"
.LASF31:
	.string	"hex_asc"
.LASF36:
	.string	"hacc_base"
.LASF10:
	.string	"long int"
.LASF20:
	.string	"encode"
.LASF4:
	.string	"signed char"
.LASF15:
	.string	"MM_ANONPAGES"
.LASF11:
	.string	"_Bool"
.LASF33:
	.string	"debug_locks"
.LASF39:
	.ascii	"GNU C 4.9 20140514 (prerelease) -fpreprocessed -mbionic -mli"
	.ascii	"ttle-end"
	.ident	"GCC: (GNU) 4.9 20140514 (prerelease)"
