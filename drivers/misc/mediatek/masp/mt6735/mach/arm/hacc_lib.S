	.arch armv7-a
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 2
	.eabi_attribute 34, 1
	.eabi_attribute 18, 4
	.file	"hacc_lib.c"
	.text
.Ltext0:
	.align	2
	.global	masp_hal_secure_algo_init
	.type	masp_hal_secure_algo_init, %function
masp_hal_secure_algo_init:
.LFB184:
	.file 1 "hacc_lib.c"
	.loc 1 60 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
.LCFI0:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI1:
	sub	fp, ip, #4
.LCFI2:
	.loc 1 64 0
	mov	r0, #1
	ldmfd	sp, {fp, sp, pc}
.LFE184:
	.size	masp_hal_secure_algo_init, .-masp_hal_secure_algo_init
	.align	2
	.global	masp_hal_secure_algo_deinit
	.type	masp_hal_secure_algo_deinit, %function
masp_hal_secure_algo_deinit:
.LFB185:
	.loc 1 70 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
.LCFI3:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI4:
	sub	fp, ip, #4
.LCFI5:
	.loc 1 74 0
	mov	r0, #1
	ldmfd	sp, {fp, sp, pc}
.LFE185:
	.size	masp_hal_secure_algo_deinit, .-masp_hal_secure_algo_deinit
	.align	2
	.global	masp_hal_secure_algo
	.type	masp_hal_secure_algo, %function
masp_hal_secure_algo:
.LFB186:
	.loc 1 87 0
	@ args = 4, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL0:
	mov	ip, sp
.LCFI6:
	stmfd	sp!, {r4, r5, r6, r7, r8, fp, ip, lr, pc}
.LCFI7:
	sub	fp, ip, #4
.LCFI8:
	sub	sp, sp, #12
	ldr	r5, [fp, #4]
	mov	r6, r0
	mov	r8, r1
	mov	r4, r2
	mov	r7, r3
.L5:
	.loc 1 96 0 discriminator 1
	bl	osal_hacc_lock
.LVL1:
	.loc 1 97 0 discriminator 1
	cmp	r0, #0
	bne	.L5
	.loc 1 101 0
	mov	r0, r7
	mov	r1, #16
	bl	masp_hal_sp_hacc_init
.LVL2:
	.loc 1 103 0
	subs	r7, r0, #0
.LVL3:
	bne	.L6
	.loc 1 113 0
	cmp	r6, #0
	beq	.L7
	cmp	r6, #1
	bne	.L15
	.loc 1 119 0
	str	r7, [sp]
	mov	r0, r8
	mov	r2, r6
	mov	r1, r4
	mov	r3, #2
	bl	masp_hal_sp_hacc_enc
.LVL4:
.L9:
	.loc 1 136 0
	cmp	r4, #0
	movne	r3, #0
.LVL5:
	beq	.L11
.LVL6:
.L12:
	.loc 1 138 0 discriminator 2
	ldrb	ip, [r0, r3]	@ zero_extendqisi2
	strb	ip, [r5, r3]
	.loc 1 136 0 discriminator 2
	add	r3, r3, #1
	cmp	r3, r4
	bne	.L12
.L11:
	.loc 1 142 0
	bl	osal_hacc_unlock
.LVL7:
	.loc 1 153 0
	sub	sp, fp, #32
	ldmfd	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
.LVL8:
.L15:
	.loc 1 130 0
	mov	r7, #24576
.L6:
	.loc 1 148 0
	bl	osal_hacc_unlock
.LVL9:
	.loc 1 150 0
	mov	r2, r7
	movw	r0, #:lower16:.LC0
	movw	r1, #:lower16:.LC1
	movt	r0, #:upper16:.LC0
	movt	r1, #:upper16:.LC1
	bl	printk
.LVL10:
	.loc 1 151 0
@ 151 "/hacc_lib.c" 1
	.long ((0xe7f001f2) & 0xFFFFFFFF)


@ 0 "" 2
.L7:
	.loc 1 126 0
	str	r6, [sp]
	mov	r0, r8
	mov	r1, r4
	mov	r2, #1
	mov	r3, #2
	bl	masp_hal_sp_hacc_dec
.LVL11:
	.loc 1 127 0
	b	.L9
.LFE186:
	.size	masp_hal_secure_algo, .-masp_hal_secure_algo
	.section	.rodata.str1.4,"aMS",%progbits,1
	.align	2
.LC0:
	.ascii	"[%s] masp_hal_secure_algo error (0x%x)\012\000"
.LC1:
	.ascii	"CE\000"
	.section	.debug_frame,"",%progbits
.Lframe0:
	.4byte	.LECIE0-.LSCIE0
.LSCIE0:
	.4byte	0xffffffff
	.byte	0x3
	.ascii	"\000"
	.uleb128 0x1
	.sleb128 -4
	.uleb128 0xe
	.byte	0xc
	.uleb128 0xd
	.uleb128 0
	.align	2
.LECIE0:
.LSFDE0:
	.4byte	.LEFDE0-.LASFDE0
.LASFDE0:
	.4byte	.Lframe0
	.4byte	.LFB184
	.4byte	.LFE184-.LFB184
	.byte	0x4
	.4byte	.LCFI0-.LFB184
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI1-.LCFI0
	.byte	0x8b
	.uleb128 0x4
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8e
	.uleb128 0x2
	.byte	0x4
	.4byte	.LCFI2-.LCFI1
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE0:
.LSFDE2:
	.4byte	.LEFDE2-.LASFDE2
.LASFDE2:
	.4byte	.Lframe0
	.4byte	.LFB185
	.4byte	.LFE185-.LFB185
	.byte	0x4
	.4byte	.LCFI3-.LFB185
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI4-.LCFI3
	.byte	0x8b
	.uleb128 0x4
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8e
	.uleb128 0x2
	.byte	0x4
	.4byte	.LCFI5-.LCFI4
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE2:
.LSFDE4:
	.4byte	.LEFDE4-.LASFDE4
.LASFDE4:
	.4byte	.Lframe0
	.4byte	.LFB186
	.4byte	.LFE186-.LFB186
	.byte	0x4
	.4byte	.LCFI6-.LFB186
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI7-.LCFI6
	.byte	0x84
	.uleb128 0x9
	.byte	0x85
	.uleb128 0x8
	.byte	0x86
	.uleb128 0x7
	.byte	0x87
	.uleb128 0x6
	.byte	0x88
	.uleb128 0x5
	.byte	0x8b
	.uleb128 0x4
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8e
	.uleb128 0x2
	.byte	0x4
	.4byte	.LCFI8-.LCFI7
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE4:
	.text
.Letext0:
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x2de
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.LASF34
	.byte	0x1
	.4byte	.LASF35
	.4byte	.LASF36
	.4byte	.Ltext0
	.4byte	.Letext0-.Ltext0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF0
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF1
	.uleb128 0x3
	.byte	0x4
	.4byte	0x39
	.uleb128 0x4
	.4byte	0x3e
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
	.uleb128 0x5
	.byte	0x4
	.byte	0x5
	.ascii	"int\000"
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF8
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF9
	.uleb128 0x2
	.byte	0x4
	.byte	0x5
	.4byte	.LASF10
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF11
	.uleb128 0x3
	.byte	0x4
	.4byte	0x53
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF12
	.uleb128 0x6
	.byte	0x4
	.byte	0x2
	.byte	0x12
	.4byte	0xb9
	.uleb128 0x7
	.4byte	.LASF13
	.sleb128 0
	.uleb128 0x7
	.4byte	.LASF14
	.sleb128 1
	.uleb128 0x7
	.4byte	.LASF15
	.sleb128 2
	.uleb128 0x7
	.4byte	.LASF16
	.sleb128 3
	.byte	0
	.uleb128 0x8
	.4byte	.LASF17
	.byte	0x2
	.byte	0x17
	.4byte	0x98
	.uleb128 0x8
	.4byte	.LASF18
	.byte	0x3
	.byte	0xe
	.4byte	0x45
	.uleb128 0x8
	.4byte	.LASF19
	.byte	0x3
	.byte	0xf
	.4byte	0x53
	.uleb128 0x9
	.4byte	.LASF20
	.byte	0x1
	.byte	0x3b
	.4byte	0x53
	.4byte	.LFB184
	.4byte	.LFE184-.LFB184
	.uleb128 0x1
	.byte	0x9c
	.4byte	0xff
	.uleb128 0xa
	.ascii	"ret\000"
	.byte	0x1
	.byte	0x3d
	.4byte	0x84
	.byte	0
	.uleb128 0x9
	.4byte	.LASF21
	.byte	0x1
	.byte	0x45
	.4byte	0x53
	.4byte	.LFB185
	.4byte	.LFE185-.LFB185
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x124
	.uleb128 0xa
	.ascii	"ret\000"
	.byte	0x1
	.byte	0x47
	.4byte	0x84
	.byte	0
	.uleb128 0xb
	.4byte	.LASF37
	.byte	0x1
	.byte	0x56
	.4byte	.LFB186
	.4byte	.LFE186-.LFB186
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x20e
	.uleb128 0xc
	.4byte	.LASF22
	.byte	0x1
	.byte	0x56
	.4byte	0x53
	.4byte	.LLST0
	.uleb128 0xc
	.4byte	.LASF23
	.byte	0x1
	.byte	0x56
	.4byte	0x8b
	.4byte	.LLST1
	.uleb128 0xc
	.4byte	.LASF24
	.byte	0x1
	.byte	0x56
	.4byte	0x45
	.4byte	.LLST2
	.uleb128 0xc
	.4byte	.LASF25
	.byte	0x1
	.byte	0x56
	.4byte	0x8b
	.4byte	.LLST3
	.uleb128 0xc
	.4byte	.LASF26
	.byte	0x1
	.byte	0x56
	.4byte	0x8b
	.4byte	.LLST4
	.uleb128 0xd
	.ascii	"err\000"
	.byte	0x1
	.byte	0x58
	.4byte	0xc4
	.uleb128 0x1
	.byte	0x57
	.uleb128 0xa
	.ascii	"src\000"
	.byte	0x1
	.byte	0x59
	.4byte	0x20e
	.uleb128 0xe
	.ascii	"dst\000"
	.byte	0x1
	.byte	0x59
	.4byte	0x20e
	.4byte	.LLST5
	.uleb128 0xf
	.4byte	.LASF27
	.byte	0x1
	.byte	0x59
	.4byte	0x20e
	.uleb128 0xe
	.ascii	"i\000"
	.byte	0x1
	.byte	0x5a
	.4byte	0xc4
	.4byte	.LLST6
	.uleb128 0x10
	.4byte	.LASF38
	.byte	0x1
	.byte	0x92
	.4byte	.L6
	.uleb128 0x11
	.4byte	.LVL1
	.4byte	0x251
	.uleb128 0x11
	.4byte	.LVL2
	.4byte	0x25c
	.uleb128 0x11
	.4byte	.LVL4
	.4byte	0x276
	.uleb128 0x11
	.4byte	.LVL7
	.4byte	0x29f
	.uleb128 0x11
	.4byte	.LVL9
	.4byte	0x29f
	.uleb128 0x11
	.4byte	.LVL10
	.4byte	0x2a6
	.uleb128 0x11
	.4byte	.LVL11
	.4byte	0x2bc
	.byte	0
	.uleb128 0x3
	.byte	0x4
	.4byte	0xcf
	.uleb128 0x12
	.4byte	.LASF28
	.byte	0x4
	.byte	0xc
	.4byte	0x45
	.uleb128 0x13
	.4byte	0x68
	.4byte	0x22a
	.uleb128 0x14
	.byte	0
	.uleb128 0x12
	.4byte	.LASF29
	.byte	0x5
	.byte	0x24
	.4byte	0x21f
	.uleb128 0x13
	.4byte	0x3e
	.4byte	0x240
	.uleb128 0x14
	.byte	0
	.uleb128 0x15
	.4byte	.LASF30
	.byte	0x6
	.2byte	0x1b2
	.4byte	0x24c
	.uleb128 0x4
	.4byte	0x235
	.uleb128 0x16
	.4byte	.LASF39
	.byte	0x7
	.byte	0x16
	.4byte	0x68
	.uleb128 0x17
	.4byte	.LASF31
	.byte	0x2
	.byte	0x21
	.4byte	0x45
	.4byte	0x276
	.uleb128 0x18
	.4byte	0x8b
	.uleb128 0x18
	.4byte	0x45
	.byte	0
	.uleb128 0x17
	.4byte	.LASF32
	.byte	0x2
	.byte	0x23
	.4byte	0x8b
	.4byte	0x29f
	.uleb128 0x18
	.4byte	0x8b
	.uleb128 0x18
	.4byte	0x45
	.uleb128 0x18
	.4byte	0x53
	.uleb128 0x18
	.4byte	0xb9
	.uleb128 0x18
	.4byte	0x53
	.byte	0
	.uleb128 0x19
	.4byte	.LASF40
	.byte	0x7
	.byte	0x17
	.uleb128 0x17
	.4byte	.LASF33
	.byte	0x5
	.byte	0x7e
	.4byte	0x68
	.4byte	0x2bc
	.uleb128 0x18
	.4byte	0x33
	.uleb128 0x1a
	.byte	0
	.uleb128 0x1b
	.4byte	.LASF41
	.byte	0x2
	.byte	0x25
	.4byte	0x8b
	.uleb128 0x18
	.4byte	0x8b
	.uleb128 0x18
	.4byte	0x45
	.uleb128 0x18
	.4byte	0x53
	.uleb128 0x18
	.4byte	0xb9
	.uleb128 0x18
	.4byte	0x53
	.byte	0
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
	.uleb128 0x6
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
	.uleb128 0x9
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
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
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
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xc
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
	.uleb128 0xd
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
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0xe
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
	.uleb128 0xf
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
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0xa
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x11
	.uleb128 0x1
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x4109
	.byte	0
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x12
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
	.uleb128 0x13
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
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
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x16
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
	.uleb128 0x17
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
	.uleb128 0x18
	.uleb128 0x5
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x19
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
	.uleb128 0x1a
	.uleb128 0x18
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x1b
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
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",%progbits
.Ldebug_loc0:
.LLST0:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL8-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL8-.Ltext0
	.4byte	.LFE186-.Ltext0
	.2byte	0x1
	.byte	0x56
	.4byte	0
	.4byte	0
.LLST1:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL8-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	.LVL8-.Ltext0
	.4byte	.LFE186-.Ltext0
	.2byte	0x1
	.byte	0x58
	.4byte	0
	.4byte	0
.LLST2:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL8-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	.LVL8-.Ltext0
	.4byte	.LFE186-.Ltext0
	.2byte	0x1
	.byte	0x54
	.4byte	0
	.4byte	0
.LLST3:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL3-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	0
	.4byte	0
.LLST4:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL8-.Ltext0
	.2byte	0x2
	.byte	0x91
	.sleb128 0
	.4byte	.LVL8-.Ltext0
	.4byte	.LFE186-.Ltext0
	.2byte	0x1
	.byte	0x55
	.4byte	0
	.4byte	0
.LLST5:
	.4byte	.LVL6-.Ltext0
	.4byte	.LVL7-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST6:
	.4byte	.LVL5-.Ltext0
	.4byte	.LVL7-1-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	0
	.4byte	0
	.section	.debug_aranges,"",%progbits
	.4byte	0x1c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	.Ltext0
	.4byte	.Letext0-.Ltext0
	.4byte	0
	.4byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF29:
	.ascii	"console_printk\000"
.LASF11:
	.ascii	"_Bool\000"
.LASF41:
	.ascii	"masp_hal_sp_hacc_dec\000"
.LASF34:
	.ascii	"GNU C 4.8 -fpreprocessed -mlittle-endian -mapcs -mn"
	.ascii	"o-sched-prolog -mabi=aapcs-linux -mno-thumb-interwo"
	.ascii	"rk -marm -march=armv7-a -mfloat-abi=soft -mfpu=vfp "
	.ascii	"-g -O2 -fno-strict-aliasing -fno-common -fno-delete"
	.ascii	"-null-pointer-checks -fno-pic -fno-dwarf2-cfi-asm -"
	.ascii	"fno-stack-protector -fno-omit-frame-pointer -fno-op"
	.ascii	"timize-sibling-calls -fno-var-tracking-assignments "
	.ascii	"-fno-strict-overflow -fconserve-stack\000"
.LASF12:
	.ascii	"float\000"
.LASF5:
	.ascii	"unsigned char\000"
.LASF25:
	.ascii	"CustomSeed\000"
.LASF32:
	.ascii	"masp_hal_sp_hacc_enc\000"
.LASF0:
	.ascii	"long unsigned int\000"
.LASF37:
	.ascii	"masp_hal_secure_algo\000"
.LASF7:
	.ascii	"short unsigned int\000"
.LASF36:
	.ascii	"/proj/mtk02464/Public/P4Android4/ALPS_SW/TRUNK/L0/a"
	.ascii	"lps/out/target/product/evb6735/obj/KERNEL_OBJ\000"
.LASF19:
	.ascii	"uchar\000"
.LASF26:
	.ascii	"ResText\000"
.LASF28:
	.ascii	"elf_hwcap\000"
.LASF30:
	.ascii	"hex_asc\000"
.LASF24:
	.ascii	"ContentLen\000"
.LASF18:
	.ascii	"uint32\000"
.LASF31:
	.ascii	"masp_hal_sp_hacc_init\000"
.LASF35:
	.ascii	"/proj/mtk02464/Public/P4Android4/ALPS_SW/TRUNK/L0/a"
	.ascii	"lps/kernel-3.10/drivers/misc/mediatek/masp/mt6735/m"
	.ascii	"ach/arm/hacc_lib.c\000"
.LASF20:
	.ascii	"masp_hal_secure_algo_init\000"
.LASF3:
	.ascii	"unsigned int\000"
.LASF40:
	.ascii	"osal_hacc_unlock\000"
.LASF39:
	.ascii	"osal_hacc_lock\000"
.LASF9:
	.ascii	"long long unsigned int\000"
.LASF17:
	.ascii	"HACC_USER\000"
.LASF1:
	.ascii	"sizetype\000"
.LASF8:
	.ascii	"long long int\000"
.LASF2:
	.ascii	"char\000"
.LASF21:
	.ascii	"masp_hal_secure_algo_deinit\000"
.LASF6:
	.ascii	"short int\000"
.LASF33:
	.ascii	"printk\000"
.LASF27:
	.ascii	"seed\000"
.LASF22:
	.ascii	"Direction\000"
.LASF10:
	.ascii	"long int\000"
.LASF23:
	.ascii	"ContentAddr\000"
.LASF4:
	.ascii	"signed char\000"
.LASF13:
	.ascii	"HACC_USER1\000"
.LASF14:
	.ascii	"HACC_USER2\000"
.LASF15:
	.ascii	"HACC_USER3\000"
.LASF16:
	.ascii	"HACC_USER4\000"
.LASF38:
	.ascii	"_error\000"
	.ident	"GCC: (GNU) 4.8"
