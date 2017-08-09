	.cpu generic+fp+simd
	.file	"sec_uid.c"
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.align	2
	.global	masp_hal_get_uuid
	.type	masp_hal_get_uuid, %function
masp_hal_get_uuid:
.LFB466:
	.file 1 "sec_uid.c"
	.loc 1 48 0
	.cfi_startproc
.LVL0:
	.loc 1 49 0
	adrp	x3, es_base
	.loc 1 48 0
	mov	x2, x0
	.loc 1 49 0
	ldr	x1, [x3,#:lo12:es_base]
	add	x1, x1, 320
.LBB10:
.LBB11:
	.file 2 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/arch/arm64/include/asm/io.h"
	.loc 2 70 0
#APP
// 70 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/arch/arm64/include/asm/io.h" 1
	ldr w1, [x1]
// 0 "" 2
.LVL1:
#NO_APP
.LBE11:
.LBE10:
	.loc 1 49 0
	str	w1, [x0]
	.loc 1 50 0
	ldr	x1, [x3,#:lo12:es_base]
.LVL2:
	add	x1, x1, 324
.LBB12:
.LBB13:
	.loc 2 70 0
#APP
// 70 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/arch/arm64/include/asm/io.h" 1
	ldr w1, [x1]
// 0 "" 2
.LVL3:
#NO_APP
.LBE13:
.LBE12:
	.loc 1 50 0
	str	w1, [x0,4]
	.loc 1 51 0
	ldr	x1, [x3,#:lo12:es_base]
.LVL4:
	add	x0, x1, 320
.LVL5:
.LBB14:
.LBB15:
	.loc 2 70 0
#APP
// 70 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/arch/arm64/include/asm/io.h" 1
	ldr w0, [x0]
// 0 "" 2
.LVL6:
#NO_APP
.LBE15:
.LBE14:
	.loc 1 51 0
	str	w0, [x2,8]
	.loc 1 52 0
	add	x1, x1, 324
.LBB16:
.LBB17:
	.loc 2 70 0
#APP
// 70 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/arch/arm64/include/asm/io.h" 1
	ldr w1, [x1]
// 0 "" 2
.LVL7:
#NO_APP
.LBE17:
.LBE16:
	.loc 1 55 0
	mov	w0, 0
.LVL8:
	.loc 1 52 0
	str	w1, [x2,12]
	.loc 1 55 0
	ret
	.cfi_endproc
.LFE466:
	.size	masp_hal_get_uuid, .-masp_hal_get_uuid
.Letext0:
	.file 3 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/include/asm-generic/int-ll64.h"
	.file 4 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/include/linux/printk.h"
	.file 5 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/include/linux/kernel.h"
	.file 6 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/arch/arm64/include/asm/memory.h"
	.file 7 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/include/linux/debug_locks.h"
	.file 8 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/include/linux/cpumask.h"
	.file 9 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/include/asm-generic/pgtable.h"
	.file 10 "/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/kernel-3.10/include/linux/types.h"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x287
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.4byte	.LASF21
	.byte	0x1
	.4byte	.LASF22
	.4byte	.LASF23
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
	.string	"u32"
	.byte	0x3
	.byte	0x16
	.4byte	0x42
	.uleb128 0x4
	.string	"u64"
	.byte	0x3
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
	.byte	0xa
	.byte	0xa2
	.4byte	0x85
	.uleb128 0x2
	.byte	0x10
	.byte	0x7
	.4byte	.LASF13
	.uleb128 0x6
	.4byte	.LASF24
	.byte	0x2
	.byte	0x43
	.4byte	0x7a
	.byte	0x3
	.4byte	0xd7
	.uleb128 0x7
	.4byte	.LASF25
	.byte	0x2
	.byte	0x43
	.4byte	0xd7
	.uleb128 0x8
	.string	"val"
	.byte	0x2
	.byte	0x45
	.4byte	0x7a
	.byte	0
	.uleb128 0x9
	.byte	0x8
	.4byte	0xdd
	.uleb128 0xa
	.4byte	0xe2
	.uleb128 0xb
	.uleb128 0xc
	.4byte	.LASF26
	.byte	0x1
	.byte	0x2f
	.4byte	0x65
	.8byte	.LFB466
	.8byte	.LFE466-.LFB466
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x1fe
	.uleb128 0xd
	.4byte	.LASF27
	.byte	0x1
	.byte	0x2f
	.4byte	0x1fe
	.4byte	.LLST0
	.uleb128 0xe
	.4byte	0xb0
	.8byte	.LBB10
	.8byte	.LBE10-.LBB10
	.byte	0x1
	.byte	0x31
	.4byte	0x14f
	.uleb128 0xf
	.4byte	0xc0
	.uleb128 0x10
	.8byte	.LBB11
	.8byte	.LBE11-.LBB11
	.uleb128 0x11
	.4byte	0xcb
	.4byte	.LLST1
	.byte	0
	.byte	0
	.uleb128 0xe
	.4byte	0xb0
	.8byte	.LBB12
	.8byte	.LBE12-.LBB12
	.byte	0x1
	.byte	0x32
	.4byte	0x18b
	.uleb128 0xf
	.4byte	0xc0
	.uleb128 0x10
	.8byte	.LBB13
	.8byte	.LBE13-.LBB13
	.uleb128 0x11
	.4byte	0xcb
	.4byte	.LLST2
	.byte	0
	.byte	0
	.uleb128 0xe
	.4byte	0xb0
	.8byte	.LBB14
	.8byte	.LBE14-.LBB14
	.byte	0x1
	.byte	0x33
	.4byte	0x1c7
	.uleb128 0xf
	.4byte	0xc0
	.uleb128 0x10
	.8byte	.LBB15
	.8byte	.LBE15-.LBB15
	.uleb128 0x11
	.4byte	0xcb
	.4byte	.LLST3
	.byte	0
	.byte	0
	.uleb128 0x12
	.4byte	0xb0
	.8byte	.LBB16
	.8byte	.LBE16-.LBB16
	.byte	0x1
	.byte	0x34
	.uleb128 0xf
	.4byte	0xc0
	.uleb128 0x10
	.8byte	.LBB17
	.8byte	.LBE17-.LBB17
	.uleb128 0x13
	.4byte	0xcb
	.uleb128 0x1
	.byte	0x51
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x9
	.byte	0x8
	.4byte	0x42
	.uleb128 0x14
	.4byte	0x65
	.4byte	0x20f
	.uleb128 0x15
	.byte	0
	.uleb128 0x16
	.4byte	.LASF14
	.byte	0x4
	.byte	0x24
	.4byte	0x204
	.uleb128 0x14
	.4byte	0x3b
	.4byte	0x225
	.uleb128 0x15
	.byte	0
	.uleb128 0x17
	.4byte	.LASF15
	.byte	0x5
	.2byte	0x1b2
	.4byte	0x231
	.uleb128 0xa
	.4byte	0x21a
	.uleb128 0x16
	.4byte	.LASF16
	.byte	0x6
	.byte	0x61
	.4byte	0x9e
	.uleb128 0x16
	.4byte	.LASF17
	.byte	0x7
	.byte	0xa
	.4byte	0x65
	.uleb128 0x14
	.4byte	0x2d
	.4byte	0x262
	.uleb128 0x18
	.4byte	0x34
	.byte	0x40
	.uleb128 0x18
	.4byte	0x34
	.byte	0
	.byte	0
	.uleb128 0x17
	.4byte	.LASF18
	.byte	0x8
	.2byte	0x2f9
	.4byte	0x26e
	.uleb128 0xa
	.4byte	0x24c
	.uleb128 0x17
	.4byte	.LASF19
	.byte	0x9
	.2byte	0x1db
	.4byte	0x2d
	.uleb128 0x16
	.4byte	.LASF20
	.byte	0x1
	.byte	0x27
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
	.uleb128 0x7
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
	.uleb128 0x8
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
	.uleb128 0x9
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x35
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0xc
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
	.uleb128 0xd
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
	.uleb128 0xe
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x34
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0x34
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0x21
	.byte	0
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
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x17
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
	.uleb128 0x18
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",%progbits
.Ldebug_loc0:
.LLST0:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL5-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL5-.Ltext0
	.8byte	.LFE466-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	0
	.8byte	0
.LLST1:
	.8byte	.LVL1-.Ltext0
	.8byte	.LVL2-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST2:
	.8byte	.LVL3-.Ltext0
	.8byte	.LVL4-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST3:
	.8byte	.LVL6-.Ltext0
	.8byte	.LVL8-.Ltext0
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
.LASF13:
	.string	"__int128 unsigned"
.LASF14:
	.string	"console_printk"
.LASF16:
	.string	"memstart_addr"
.LASF7:
	.string	"short unsigned int"
.LASF19:
	.string	"zero_pfn"
.LASF18:
	.string	"cpu_bit_bitmap"
.LASF4:
	.string	"signed char"
.LASF5:
	.string	"unsigned char"
.LASF0:
	.string	"long unsigned int"
.LASF25:
	.string	"addr"
.LASF20:
	.string	"es_base"
.LASF3:
	.string	"unsigned int"
.LASF9:
	.string	"long long unsigned int"
.LASF24:
	.string	"__raw_readl"
.LASF12:
	.string	"phys_addr_t"
.LASF1:
	.string	"sizetype"
.LASF8:
	.string	"long long int"
.LASF23:
	.string	"/proj/mtk02464/Public/P4Android3/ALPS_SW/TRUNK/L0/alps/out/target/product/evb6735_64/obj/KERNEL_OBJ"
.LASF2:
	.string	"char"
.LASF6:
	.string	"short int"
.LASF15:
	.string	"hex_asc"
.LASF26:
	.string	"masp_hal_get_uuid"
.LASF10:
	.string	"long int"
.LASF27:
	.string	"uuid"
.LASF22:
	.string	"sec_uid.c"
.LASF11:
	.string	"_Bool"
.LASF17:
	.string	"debug_locks"
.LASF21:
	.ascii	"GNU C 4.9 20140514 (prerelease) -fpreprocessed -mbionic -mli"
	.ascii	"ttle-end"
	.ident	"GCC: (GNU) 4.9 20140514 (prerelease)"
