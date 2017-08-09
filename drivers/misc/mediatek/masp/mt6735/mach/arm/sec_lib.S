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
	.file	"sec_lib.c"
	.text
.Ltext0:
	.align	2
	.global	masp_hal_sbc_enabled
	.type	masp_hal_sbc_enabled, %function
masp_hal_sbc_enabled:
.LFB124:
	.file 1 "sec_lib.c"
	.loc 1 63 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
.LCFI0:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI1:
	sub	fp, ip, #4
.LCFI2:
	.loc 1 64 0
	movw	r3, #:lower16:es_base
	movt	r3, #:upper16:es_base
	ldr	r3, [r3]
	ldr	r0, [r3, #96]
	.loc 1 65 0
	ubfx	r0, r0, #1, #1
	ldmfd	sp, {fp, sp, pc}
.LFE124:
	.size	masp_hal_sbc_enabled, .-masp_hal_sbc_enabled
	.align	2
	.global	masp_hal_get_sbc_checksum
	.type	masp_hal_get_sbc_checksum, %function
masp_hal_get_sbc_checksum:
.LFB125:
	.loc 1 68 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL0:
	mov	ip, sp
.LCFI3:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI4:
	sub	fp, ip, #4
.LCFI5:
	ldr	r2, [r0]
	movw	ip, #:lower16:es_base
	movt	ip, #:upper16:es_base
	.loc 1 71 0
	mov	r3, #0
.LVL1:
.L4:
	.loc 1 73 0 discriminator 2
	ldr	r1, [ip]
	add	r1, r1, #144
	ldr	r1, [r1, r3, asl #2]
	.loc 1 71 0 discriminator 2
	add	r3, r3, #1
	cmp	r3, #8
	.loc 1 73 0 discriminator 2
	add	r2, r2, r1
	str	r2, [r0]
	.loc 1 71 0 discriminator 2
	bne	.L4
	.loc 1 77 0
	mov	r0, #0
.LVL2:
	ldmfd	sp, {fp, sp, pc}
.LFE125:
	.size	masp_hal_get_sbc_checksum, .-masp_hal_get_sbc_checksum
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
	.4byte	.LFB124
	.4byte	.LFE124-.LFB124
	.byte	0x4
	.4byte	.LCFI0-.LFB124
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
	.4byte	.LFB125
	.4byte	.LFE125-.LFB125
	.byte	0x4
	.4byte	.LCFI3-.LFB125
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
	.text
.Letext0:
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x126
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.LASF17
	.byte	0x1
	.4byte	.LASF18
	.4byte	.LASF19
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
	.uleb128 0x4
	.byte	0x4
	.4byte	0x3a
	.uleb128 0x5
	.4byte	.LASF20
	.byte	0x5
	.byte	0x36
	.4byte	0x3a
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF12
	.uleb128 0x6
	.4byte	.LASF21
	.byte	0x1
	.byte	0x3e
	.4byte	0x5d
	.4byte	.LFB124
	.4byte	.LFE124-.LFB124
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x7
	.4byte	.LASF22
	.byte	0x1
	.byte	0x43
	.4byte	0x5d
	.4byte	.LFB125
	.4byte	.LFE125-.LFB125
	.uleb128 0x1
	.byte	0x9c
	.4byte	0xe1
	.uleb128 0x8
	.4byte	.LASF23
	.byte	0x1
	.byte	0x43
	.4byte	0x80
	.4byte	.LLST0
	.uleb128 0x9
	.ascii	"i\000"
	.byte	0x1
	.byte	0x45
	.4byte	0x5d
	.uleb128 0x1
	.byte	0x53
	.byte	0
	.uleb128 0xa
	.4byte	.LASF13
	.byte	0x2
	.byte	0xc
	.4byte	0x3a
	.uleb128 0xb
	.4byte	0x5d
	.4byte	0xf7
	.uleb128 0xc
	.byte	0
	.uleb128 0xa
	.4byte	.LASF14
	.byte	0x3
	.byte	0x24
	.4byte	0xec
	.uleb128 0xb
	.4byte	0x33
	.4byte	0x10d
	.uleb128 0xc
	.byte	0
	.uleb128 0xd
	.4byte	.LASF15
	.byte	0x4
	.2byte	0x1b2
	.4byte	0x119
	.uleb128 0xe
	.4byte	0x102
	.uleb128 0xa
	.4byte	.LASF16
	.byte	0x1
	.byte	0x2f
	.4byte	0x3a
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
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
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
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x7
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
	.uleb128 0x8
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
	.uleb128 0x9
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
	.uleb128 0xa
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
	.uleb128 0xb
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0xd
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
	.uleb128 0xe
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",%progbits
.Ldebug_loc0:
.LLST0:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL2-.Ltext0
	.2byte	0x1
	.byte	0x50
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
.LASF8:
	.ascii	"long long int\000"
.LASF23:
	.ascii	"pChecksum\000"
.LASF3:
	.ascii	"unsigned int\000"
.LASF20:
	.ascii	"UINT32\000"
.LASF13:
	.ascii	"elf_hwcap\000"
.LASF16:
	.ascii	"es_base\000"
.LASF22:
	.ascii	"masp_hal_get_sbc_checksum\000"
.LASF0:
	.ascii	"long unsigned int\000"
.LASF9:
	.ascii	"long long unsigned int\000"
.LASF19:
	.ascii	"/proj/mtk02464/Public/P4Android4/ALPS_SW/TRUNK/L0/a"
	.ascii	"lps/out/target/product/evb6735/obj/KERNEL_OBJ\000"
.LASF15:
	.ascii	"hex_asc\000"
.LASF5:
	.ascii	"unsigned char\000"
.LASF2:
	.ascii	"char\000"
.LASF17:
	.ascii	"GNU C 4.8 -fpreprocessed -mlittle-endian -mapcs -mn"
	.ascii	"o-sched-prolog -mabi=aapcs-linux -mno-thumb-interwo"
	.ascii	"rk -marm -march=armv7-a -mfloat-abi=soft -mfpu=vfp "
	.ascii	"-g -O2 -fno-strict-aliasing -fno-common -fno-delete"
	.ascii	"-null-pointer-checks -fno-pic -fno-dwarf2-cfi-asm -"
	.ascii	"fno-stack-protector -fno-omit-frame-pointer -fno-op"
	.ascii	"timize-sibling-calls -fno-var-tracking-assignments "
	.ascii	"-fno-strict-overflow -fconserve-stack\000"
.LASF10:
	.ascii	"long int\000"
.LASF14:
	.ascii	"console_printk\000"
.LASF18:
	.ascii	"/proj/mtk02464/Public/P4Android4/ALPS_SW/TRUNK/L0/a"
	.ascii	"lps/kernel-3.10/drivers/misc/mediatek/masp/mt6735/m"
	.ascii	"ach/arm/sec_lib.c\000"
.LASF11:
	.ascii	"_Bool\000"
.LASF7:
	.ascii	"short unsigned int\000"
.LASF4:
	.ascii	"signed char\000"
.LASF12:
	.ascii	"float\000"
.LASF21:
	.ascii	"masp_hal_sbc_enabled\000"
.LASF6:
	.ascii	"short int\000"
.LASF1:
	.ascii	"sizetype\000"
	.ident	"GCC: (GNU) 4.8"
