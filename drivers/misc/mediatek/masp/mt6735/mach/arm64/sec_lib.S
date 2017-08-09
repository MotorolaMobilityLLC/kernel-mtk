	.cpu generic+fp+simd
	.file	"sec_lib.c"
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.align	2
	.global	masp_hal_sbc_enabled
	.type	masp_hal_sbc_enabled, %function
masp_hal_sbc_enabled:
.LFB118:
	.file 1 "sec_lib.c"
	.loc 1 63 0
	.cfi_startproc
	.loc 1 64 0
	adrp	x0, es_base
	ldr	x0, [x0,#:lo12:es_base]
	ldr	w0, [x0,96]
	.loc 1 65 0
	ubfx	x0, x0, 1, 1
	ret
	.cfi_endproc
.LFE118:
	.size	masp_hal_sbc_enabled, .-masp_hal_sbc_enabled
	.align	2
	.global	masp_hal_get_sbc_checksum
	.type	masp_hal_get_sbc_checksum, %function
masp_hal_get_sbc_checksum:
.LFB119:
	.loc 1 68 0
	.cfi_startproc
.LVL0:
	ldr	w4, [x0]
	.loc 1 71 0
	mov	w1, 0
.LVL1:
	adrp	x5, es_base
.L3:
	.loc 1 73 0 discriminator 3
	lsl	w2, w1, 2
	ldr	x3, [x5,#:lo12:es_base]
	.loc 1 71 0 discriminator 3
	add	w1, w1, 1
	add	x3, x3, 144
	.loc 1 73 0 discriminator 3
	sxtw	x2, w2
	.loc 1 71 0 discriminator 3
	cmp	w1, 8
	.loc 1 73 0 discriminator 3
	ldr	w2, [x3,x2]
	add	w4, w4, w2
	str	w4, [x0]
	.loc 1 71 0 discriminator 3
	bne	.L3
	.loc 1 77 0
	mov	w0, 0
.LVL2:
	ret
	.cfi_endproc
.LFE119:
	.size	masp_hal_get_sbc_checksum, .-masp_hal_get_sbc_checksum
.Letext0:
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x133
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.4byte	.LASF16
	.byte	0x1
	.4byte	.LASF17
	.4byte	.LASF18
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
	.uleb128 0x3
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
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF10
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF11
	.uleb128 0x4
	.byte	0x8
	.4byte	0x42
	.uleb128 0x5
	.4byte	.LASF19
	.byte	0x4
	.byte	0x36
	.4byte	0x42
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF12
	.uleb128 0x6
	.4byte	.LASF20
	.byte	0x1
	.byte	0x3e
	.4byte	0x49
	.8byte	.LFB118
	.8byte	.LFE118-.LFB118
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x7
	.4byte	.LASF21
	.byte	0x1
	.byte	0x43
	.4byte	0x49
	.8byte	.LFB119
	.8byte	.LFE119-.LFB119
	.uleb128 0x1
	.byte	0x9c
	.4byte	0xf9
	.uleb128 0x8
	.4byte	.LASF22
	.byte	0x1
	.byte	0x43
	.4byte	0x88
	.4byte	.LLST0
	.uleb128 0x9
	.string	"i"
	.byte	0x1
	.byte	0x45
	.4byte	0x49
	.uleb128 0x1
	.byte	0x51
	.byte	0
	.uleb128 0xa
	.4byte	0x49
	.4byte	0x104
	.uleb128 0xb
	.byte	0
	.uleb128 0xc
	.4byte	.LASF13
	.byte	0x2
	.byte	0x24
	.4byte	0xf9
	.uleb128 0xa
	.4byte	0x3b
	.4byte	0x11a
	.uleb128 0xb
	.byte	0
	.uleb128 0xd
	.4byte	.LASF14
	.byte	0x3
	.2byte	0x1b2
	.4byte	0x126
	.uleb128 0xe
	.4byte	0x10f
	.uleb128 0xc
	.4byte	.LASF15
	.byte	0x1
	.byte	0x2d
	.4byte	0x73
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
	.uleb128 0x7
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
	.uleb128 0x7
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
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0xc
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
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL2-.Ltext0
	.2byte	0x1
	.byte	0x50
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
.LASF8:
	.string	"long long int"
.LASF22:
	.string	"pChecksum"
.LASF3:
	.string	"unsigned int"
.LASF19:
	.string	"UINT32"
.LASF15:
	.string	"es_base"
.LASF0:
	.string	"long unsigned int"
.LASF9:
	.string	"long long unsigned int"
.LASF12:
	.string	"float"
.LASF14:
	.string	"hex_asc"
.LASF5:
	.string	"unsigned char"
.LASF2:
	.string	"char"
.LASF21:
	.string	"masp_hal_get_sbc_checksum"
.LASF10:
	.string	"long int"
.LASF13:
	.string	"console_printk"
.LASF11:
	.string	"_Bool"
.LASF7:
	.string	"short unsigned int"
.LASF4:
	.string	"signed char"
.LASF1:
	.string	"sizetype"
.LASF16:
	.ascii	"GNU C 4.9 20140514 (prerelease) -fpreprocessed -mbionic -mli"
	.ascii	"ttle-end"
.LASF20:
	.string	"masp_hal_sbc_enabled"
.LASF6:
	.string	"short int"
.LASF18:
	.string	"/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/out/target/product/evb6735_64/obj/KERNEL_OBJ"
.LASF17:
	.string	"sec_lib.c"
	.ident	"GCC: (GNU) 4.9 20140514 (prerelease)"
