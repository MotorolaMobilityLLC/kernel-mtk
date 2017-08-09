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
	.file	"hacc_sk.c"
	.text
.Ltext0:
	.align	2
	.global	hacc_set_key
	.type	hacc_set_key, %function
hacc_set_key:
.LFB186:
	.file 1 "hacc_sk.c"
	.loc 1 141 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL0:
	mov	ip, sp
.LCFI0:
	stmfd	sp!, {r4, r5, r6, fp, ip, lr, pc}
.LCFI1:
	sub	fp, ip, #4
.LCFI2:
	.loc 1 146 0
	cmp	r1, #24
	beq	.L13
	cmp	r1, #32
	beq	.L4
	cmp	r1, #16
	.loc 1 149 0
	moveq	r6, #0
.LVL1:
	.loc 1 146 0
	beq	.L3
	.loc 1 158 0
	movw	r0, #24578
.LVL2:
	ldmfd	sp, {r4, r5, r6, fp, sp, pc}
.LVL3:
.L13:
	.loc 1 152 0
	mov	r6, #16
.LVL4:
.L3:
	.loc 1 164 0
	movw	ip, #:lower16:hacc_base
	movt	ip, #:upper16:hacc_base
	.loc 1 161 0
	movw	r2, #:lower16:.LANCHOR0
	movt	r2, #:upper16:.LANCHOR0
	.loc 1 164 0
	ldr	r5, [ip]
	.loc 1 168 0
	mov	r3, #0
	.loc 1 161 0
	str	r1, [r2, #16]
	.loc 1 169 0
	mov	r4, r3
	.loc 1 164 0
	ldr	r2, [r5, #4]
	bic	r2, r2, #48
	str	r2, [r5, #4]
	.loc 1 165 0
	ldr	r2, [ip]
	ldr	r1, [r2, #4]
.LVL5:
	orr	r1, r6, r1
	str	r1, [r2, #4]
.L7:
	.loc 1 169 0 discriminator 2
	ldr	r1, [ip]
	movw	r2, #:lower16:hacc_base
	movt	r2, #:upper16:hacc_base
	add	r1, r1, #32
	str	r4, [r1, r3]
	.loc 1 168 0 discriminator 2
	add	r3, r3, #4
	cmp	r3, #32
	bne	.L7
	.loc 1 172 0
	cmp	r0, #1
	beq	.L9
	.loc 1 182 0
	ldr	r3, .L20
	cmp	r0, #2
	.loc 1 187 0
	ldr	r1, [ip]
	.loc 1 190 0
	mov	r2, #0
	.loc 1 182 0
	sub	r4, r3, #32
	moveq	r0, r3
.LVL6:
	movne	r0, r4
	.loc 1 187 0
	ldr	r4, [r1, #12]
	mov	r3, r0
	bic	r4, r4, #16
	str	r4, [r1, #12]
.LVL7:
.L11:
	.loc 1 192 0 discriminator 2
	ldrb	r5, [r0, r2]	@ zero_extendqisi2
	add	r3, r3, #4
	.loc 1 193 0 discriminator 2
	ldrb	r4, [r3, #-1]	@ zero_extendqisi2
.LVL8:
	.loc 1 192 0 discriminator 2
	ldrb	r6, [r3, #-3]	@ zero_extendqisi2
.LVL9:
	.loc 1 194 0 discriminator 2
	ldr	r1, [ip]
	.loc 1 192 0 discriminator 2
	orr	r4, r4, r5, asl #24
	.loc 1 193 0 discriminator 2
	ldrb	r5, [r3, #-2]	@ zero_extendqisi2
	.loc 1 194 0 discriminator 2
	add	r1, r1, #32
	.loc 1 192 0 discriminator 2
	orr	r4, r4, r6, asl #16
	.loc 1 193 0 discriminator 2
	orr	r4, r4, r5, asl #8
.LVL10:
	.loc 1 194 0 discriminator 2
	str	r4, [r1, r2]
	.loc 1 190 0 discriminator 2
	add	r2, r2, #4
	cmp	r2, #32
	bne	.L11
	.loc 1 197 0
	mov	r0, #0
.LVL11:
	.loc 1 198 0
	ldmfd	sp, {r4, r5, r6, fp, sp, pc}
.LVL12:
.L4:
	.loc 1 155 0
	mov	r6, r1
.LVL13:
	.loc 1 156 0
	b	.L3
.LVL14:
.L9:
	.loc 1 175 0
	ldr	r3, [r2]
	.loc 1 176 0
	mov	r0, #0
.LVL15:
	.loc 1 175 0
	ldr	r2, [r3, #12]
	orr	r2, r2, #16
	str	r2, [r3, #12]
	.loc 1 176 0
	ldmfd	sp, {r4, r5, r6, fp, sp, pc}
.L21:
	.align	2
.L20:
	.word	.LANCHOR0+52
.LFE186:
	.size	hacc_set_key, .-hacc_set_key
	.align	2
	.global	hacc_do_aes
	.type	hacc_do_aes, %function
hacc_do_aes:
.LFB187:
	.loc 1 201 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL16:
	mov	ip, sp
.LCFI3:
	stmfd	sp!, {r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
.LVL17:
.LCFI4:
	sub	fp, ip, #4
.LCFI5:
	.loc 1 206 0
	ands	ip, r3, #15
.LVL18:
	bne	.L41
	.loc 1 215 0
	movw	r6, #:lower16:hacc_base
	movt	r6, #:upper16:hacc_base
	movw	r8, #:lower16:.LANCHOR0
	movt	r8, #:upper16:.LANCHOR0
	ldr	r4, [r6]
	ldr	r5, [r4, #8]
	orr	r5, r5, #2
	str	r5, [r4, #8]
.L26:
	.loc 1 220 0 discriminator 2
	ldr	r5, [r6]
	movw	r4, #:lower16:hacc_base
	ldr	r7, [r8, ip]
	movt	r4, #:upper16:hacc_base
	add	r5, r5, #64
	str	r7, [r5, ip]
	.loc 1 218 0 discriminator 2
	add	ip, ip, #4
	cmp	ip, #16
	bne	.L26
	.loc 1 223 0
	cmp	r0, #1
	.loc 1 225 0
	ldr	r0, [r4]
.LVL19:
	ldr	ip, [r0, #4]
	orreq	ip, ip, #1
	.loc 1 229 0
	bicne	ip, ip, #1
	str	ip, [r0, #4]
.L35:
	.loc 1 238 0
	mov	ip, #0
.L30:
	.loc 1 239 0 discriminator 2
	ldr	r0, [r6]
	movw	r4, #:lower16:hacc_base
	ldr	r5, [r1, ip]
	movt	r4, #:upper16:hacc_base
	add	r0, r0, #16
	str	r5, [r0, ip]
	.loc 1 238 0 discriminator 2
	add	ip, ip, #4
	cmp	ip, #16
	bne	.L30
	.loc 1 242 0
	ldr	r0, [r4]
	add	r1, r1, #16
	ldr	ip, [r0, #8]
	orr	ip, ip, #1
	str	ip, [r0, #8]
	ldr	r5, [r4]
	add	r4, r5, #8
.L32:
	.loc 1 245 0 discriminator 1
	ldr	ip, [r4]
	tst	ip, #32768
	beq	.L32
	.loc 1 245 0 is_stmt 0
	mov	ip, #0
.L34:
	.loc 1 249 0 is_stmt 1 discriminator 2
	add	r4, ip, #80
	ldr	r4, [r4, r5]
	str	r4, [r2, ip]
	.loc 1 248 0 discriminator 2
	add	ip, ip, #4
	cmp	ip, #16
	ldrne	r5, [r6]
	bne	.L34
.L33:
	.loc 1 251 0
	cmp	r3, #0
	add	r2, r2, #16
	beq	.L37
	.loc 1 256 0
	subs	r3, r3, #16
	bne	.L35
.L37:
.L24:
	.loc 1 260 0
	mov	r0, r3
	.loc 1 261 0
	ldmfd	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
.LVL20:
.L41:
	.loc 1 208 0
	mov	r2, r3
.LVL21:
	movw	r0, #:lower16:.LC0
.LVL22:
	mov	r3, #16
.LVL23:
	movw	r1, #:lower16:.LC1
.LVL24:
	movt	r0, #:upper16:.LC0
	movt	r1, #:upper16:.LC1
	bl	printk
.LVL25:
	.loc 1 209 0
	movw	r0, #24579
	ldmfd	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
.LFE187:
	.size	hacc_do_aes, .-hacc_do_aes
	.align	2
	.global	hacc_deinit
	.type	hacc_deinit, %function
hacc_deinit:
.LFB188:
	.loc 1 264 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
.LCFI6:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI7:
	sub	fp, ip, #4
.LCFI8:
	.loc 1 268 0
	movw	r3, #:lower16:hacc_base
	movt	r3, #:upper16:hacc_base
	.loc 1 271 0
	mov	r0, #0
	.loc 1 268 0
	ldr	r3, [r3]
	ldr	r2, [r3, #8]
	orr	r2, r2, #2
	str	r2, [r3, #8]
	.loc 1 271 0
	ldmfd	sp, {fp, sp, pc}
.LFE188:
	.size	hacc_deinit, .-hacc_deinit
	.align	2
	.global	hacc_init
	.type	hacc_init, %function
hacc_init:
.LFB189:
	.loc 1 274 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL26:
	mov	ip, sp
.LCFI9:
	stmfd	sp!, {r3, r4, r5, r6, fp, ip, lr, pc}
.LCFI10:
	sub	fp, ip, #4
.LCFI11:
.LBB16:
.LBB17:
	.loc 1 268 0
	movw	r5, #:lower16:hacc_base
	movt	r5, #:upper16:hacc_base
.LBE17:
.LBE16:
.LBB20:
	.loc 1 292 0
	movw	r6, #:lower16:.LANCHOR0
	movt	r6, #:upper16:.LANCHOR0
.LBE20:
.LBB21:
.LBB18:
	.loc 1 268 0
	ldr	r3, [r5]
.LBE18:
.LBE21:
	.loc 1 274 0
	mov	r4, r0
.LBB22:
	.loc 1 292 0
	mov	r1, #84
	mov	r0, r6
.LVL27:
.LBE22:
.LBB23:
.LBB19:
	.loc 1 268 0
	ldr	r2, [r3, #8]
	orr	r2, r2, #2
	str	r2, [r3, #8]
.LBE19:
.LBE23:
	.loc 1 285 0
	ldr	r3, [r5]
	ldr	r2, [r3, #8]
	orr	r2, r2, #2
	str	r2, [r3, #8]
	.loc 1 288 0
	ldr	r3, [r5]
	ldr	r2, [r3, #8]
	bic	r2, r2, #4096
	bic	r2, r2, #2
	str	r2, [r3, #8]
	.loc 1 289 0
	ldr	r3, [r5]
	ldr	r2, [r3, #8]
	orr	r2, r2, #2
	str	r2, [r3, #8]
.LBB24:
	.loc 1 292 0
	bl	__memzero
.LVL28:
.LBE24:
	.loc 1 294 0
	ldr	r3, [r4]
	cmp	r3, #0
	addne	r2, r6, #19
	movne	r3, #0
.LVL29:
	beq	.L47
.L48:
	add	r1, r4, r3
	.loc 1 294 0 is_stmt 0 discriminator 2
	add	r3, r3, #1
	.loc 1 296 0 is_stmt 1 discriminator 2
	ldrb	r1, [r1, #4]	@ zero_extendqisi2
	strb	r1, [r2, #1]!
	.loc 1 294 0 discriminator 2
	ldr	r1, [r4]
	cmp	r1, r3
	bhi	.L48
.L47:
.LBB25:
.LBB26:
	.loc 1 122 0
	ldr	ip, [r5]
.LBE26:
.LBE25:
	.loc 1 301 0
	movw	r2, #12887
	.loc 1 302 0
	movw	r3, #21522
.LVL30:
	.loc 1 301 0
	movt	r2, 23130
	.loc 1 302 0
	movt	r3, 26263
	.loc 1 301 0
	str	r2, [r6]
	.loc 1 304 0
	str	r2, [r6, #12]
	.loc 1 315 0
	mov	r0, #1
	.loc 1 302 0
	str	r3, [r6, #4]
	.loc 1 315 0
	mov	r1, #16
	.loc 1 303 0
	str	r3, [r6, #8]
.LBB28:
.LBB27:
	.loc 1 122 0
	ldr	r3, [ip, #4]
	bic	r3, r3, #2
	str	r3, [ip, #4]
	.loc 1 131 0
	ldr	r3, [r5]
	ldr	r2, [r3, #4]
	orr	r2, r2, #2
	str	r2, [r3, #4]
.LBE27:
.LBE28:
	.loc 1 315 0
	bl	hacc_set_key
.LVL31:
	.loc 1 316 0
	cmp	r0, #0
.LVL32:
.LVL33:
	ldmnefd	sp, {r3, r4, r5, r6, fp, sp, pc}
	.loc 1 319 0
	ldr	r1, .L50
	mov	r3, #32
	mov	r0, #1
	add	r2, r1, r3
	bl	hacc_do_aes
.LVL34:
	.loc 1 320 0
	cmp	r0, #0
.LVL35:
.LVL36:
	ldmnefd	sp, {r3, r4, r5, r6, fp, sp, pc}
	.loc 1 323 0
	mov	r0, #2
	mov	r1, #32
	bl	hacc_set_key
.LVL37:
	.loc 1 324 0
	cmp	r0, #0
.LVL38:
.LVL39:
	ldmnefd	sp, {r3, r4, r5, r6, fp, sp, pc}
.L45:
	.loc 1 330 0
	movw	r2, #:lower16:bHACC_HWWrapKeyInit
	.loc 1 333 0
	movw	r3, #:lower16:bHACC_SWKeyInit
	.loc 1 330 0
	movt	r2, #:upper16:bHACC_HWWrapKeyInit
	.loc 1 333 0
	movt	r3, #:upper16:bHACC_SWKeyInit
	.loc 1 330 0
	mov	r1, #1
	strb	r1, [r2]
	.loc 1 333 0
	strb	r1, [r3]
	.loc 1 338 0
	ldmfd	sp, {r3, r4, r5, r6, fp, sp, pc}
.L51:
	.align	2
.L50:
	.word	.LANCHOR0+20
.LFE189:
	.size	hacc_init, .-hacc_init
	.section	.rodata.str1.4,"aMS",%progbits,1
	.align	2
.LC0:
	.ascii	"[%s] size = %d is not %d bytes alignment\012\000"
	.space	2
.LC1:
	.ascii	"HACC\000"
	.bss
	.align	2
	.set	.LANCHOR0,. + 0
	.type	hacc_ctx, %object
	.size	hacc_ctx, 84
hacc_ctx:
	.space	84
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
	.4byte	.LFB186
	.4byte	.LFE186-.LFB186
	.byte	0x4
	.4byte	.LCFI0-.LFB186
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI1-.LCFI0
	.byte	0x84
	.uleb128 0x7
	.byte	0x85
	.uleb128 0x6
	.byte	0x86
	.uleb128 0x5
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
	.4byte	.LFB187
	.4byte	.LFE187-.LFB187
	.byte	0x4
	.4byte	.LCFI3-.LFB187
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI4-.LCFI3
	.byte	0x83
	.uleb128 0xa
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
	.4byte	.LFB188
	.4byte	.LFE188-.LFB188
	.byte	0x4
	.4byte	.LCFI6-.LFB188
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI7-.LCFI6
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
.LSFDE6:
	.4byte	.LEFDE6-.LASFDE6
.LASFDE6:
	.4byte	.Lframe0
	.4byte	.LFB189
	.4byte	.LFE189-.LFB189
	.byte	0x4
	.4byte	.LCFI9-.LFB189
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI10-.LCFI9
	.byte	0x83
	.uleb128 0x8
	.byte	0x84
	.uleb128 0x7
	.byte	0x85
	.uleb128 0x6
	.byte	0x86
	.uleb128 0x5
	.byte	0x8b
	.uleb128 0x4
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8e
	.uleb128 0x2
	.byte	0x4
	.4byte	.LCFI11-.LCFI10
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE6:
	.text
.Letext0:
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x533
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.LASF59
	.byte	0x1
	.4byte	.LASF60
	.4byte	.LASF61
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
	.uleb128 0x6
	.4byte	.LASF12
	.byte	0x2
	.byte	0x43
	.4byte	0x45
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF11
	.uleb128 0x6
	.4byte	.LASF13
	.byte	0x3
	.byte	0x36
	.4byte	0x84
	.uleb128 0x7
	.byte	0x4
	.uleb128 0x8
	.ascii	"U8\000"
	.byte	0x4
	.byte	0x2a
	.4byte	0x53
	.uleb128 0x8
	.ascii	"U32\000"
	.byte	0x4
	.byte	0x2e
	.4byte	0x45
	.uleb128 0x6
	.4byte	.LASF14
	.byte	0x4
	.byte	0x36
	.4byte	0x45
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF15
	.uleb128 0x6
	.4byte	.LASF16
	.byte	0x4
	.byte	0x68
	.4byte	0x53
	.uleb128 0x9
	.byte	0x4
	.byte	0x5
	.byte	0x9a
	.4byte	0xea
	.uleb128 0xa
	.4byte	.LASF17
	.sleb128 0
	.uleb128 0xa
	.4byte	.LASF18
	.sleb128 1
	.byte	0
	.uleb128 0x6
	.4byte	.LASF19
	.byte	0x5
	.byte	0x9d
	.4byte	0xd5
	.uleb128 0x9
	.byte	0x4
	.byte	0x5
	.byte	0xa0
	.4byte	0x10a
	.uleb128 0xa
	.4byte	.LASF20
	.sleb128 0
	.uleb128 0xa
	.4byte	.LASF21
	.sleb128 1
	.byte	0
	.uleb128 0x6
	.4byte	.LASF22
	.byte	0x5
	.byte	0xa3
	.4byte	0xf5
	.uleb128 0x9
	.byte	0x4
	.byte	0x5
	.byte	0xa6
	.4byte	0x130
	.uleb128 0xa
	.4byte	.LASF23
	.sleb128 16
	.uleb128 0xa
	.4byte	.LASF24
	.sleb128 24
	.uleb128 0xa
	.4byte	.LASF25
	.sleb128 32
	.byte	0
	.uleb128 0x6
	.4byte	.LASF26
	.byte	0x5
	.byte	0xaa
	.4byte	0x115
	.uleb128 0x9
	.byte	0x4
	.byte	0x5
	.byte	0xad
	.4byte	0x156
	.uleb128 0xa
	.4byte	.LASF27
	.sleb128 0
	.uleb128 0xa
	.4byte	.LASF28
	.sleb128 1
	.uleb128 0xa
	.4byte	.LASF29
	.sleb128 2
	.byte	0
	.uleb128 0x6
	.4byte	.LASF30
	.byte	0x5
	.byte	0xb1
	.4byte	0x13b
	.uleb128 0xb
	.byte	0x10
	.byte	0x5
	.byte	0xb3
	.4byte	0x176
	.uleb128 0xc
	.4byte	.LASF32
	.byte	0x5
	.byte	0xb4
	.4byte	0x176
	.byte	0
	.byte	0
	.uleb128 0xd
	.4byte	0xa3
	.4byte	0x186
	.uleb128 0xe
	.4byte	0x2c
	.byte	0xf
	.byte	0
	.uleb128 0x6
	.4byte	.LASF31
	.byte	0x5
	.byte	0xb5
	.4byte	0x161
	.uleb128 0xb
	.byte	0x24
	.byte	0x5
	.byte	0xb7
	.4byte	0x1b2
	.uleb128 0xc
	.4byte	.LASF33
	.byte	0x5
	.byte	0xb8
	.4byte	0xad
	.byte	0
	.uleb128 0xc
	.4byte	.LASF34
	.byte	0x5
	.byte	0xb9
	.4byte	0x1b2
	.byte	0x4
	.byte	0
	.uleb128 0xd
	.4byte	0xa3
	.4byte	0x1c2
	.uleb128 0xe
	.4byte	0x2c
	.byte	0x1f
	.byte	0
	.uleb128 0x6
	.4byte	.LASF35
	.byte	0x5
	.byte	0xba
	.4byte	0x191
	.uleb128 0xf
	.4byte	.LASF62
	.byte	0x54
	.byte	0x5
	.byte	0xbc
	.4byte	0x20a
	.uleb128 0x10
	.ascii	"cfg\000"
	.byte	0x5
	.byte	0xbd
	.4byte	0x186
	.byte	0
	.uleb128 0xc
	.4byte	.LASF36
	.byte	0x5
	.byte	0xbe
	.4byte	0xad
	.byte	0x10
	.uleb128 0xc
	.4byte	.LASF37
	.byte	0x5
	.byte	0xbf
	.4byte	0x1b2
	.byte	0x14
	.uleb128 0xc
	.4byte	.LASF38
	.byte	0x5
	.byte	0xc0
	.4byte	0x1b2
	.byte	0x34
	.byte	0
	.uleb128 0x11
	.4byte	.LASF57
	.byte	0x1
	.2byte	0x107
	.4byte	0xad
	.byte	0x1
	.4byte	0x228
	.uleb128 0x12
	.ascii	"ret\000"
	.byte	0x1
	.2byte	0x109
	.4byte	0xad
	.byte	0
	.uleb128 0x13
	.4byte	.LASF39
	.byte	0x1
	.byte	0x70
	.4byte	0xad
	.byte	0x1
	.4byte	0x244
	.uleb128 0x14
	.ascii	"cfg\000"
	.byte	0x1
	.byte	0x70
	.4byte	0x244
	.byte	0
	.uleb128 0x3
	.byte	0x4
	.4byte	0x186
	.uleb128 0x13
	.4byte	.LASF40
	.byte	0x1
	.byte	0x76
	.4byte	0xad
	.byte	0x1
	.4byte	0x289
	.uleb128 0x15
	.4byte	.LASF41
	.byte	0x1
	.byte	0x76
	.4byte	0xea
	.uleb128 0x16
	.ascii	"cfg\000"
	.byte	0x1
	.byte	0x78
	.4byte	0x186
	.uleb128 0x17
	.uleb128 0x16
	.ascii	"__p\000"
	.byte	0x1
	.byte	0x7f
	.4byte	0xa1
	.uleb128 0x16
	.ascii	"__n\000"
	.byte	0x1
	.byte	0x7f
	.4byte	0x96
	.byte	0
	.byte	0
	.uleb128 0x18
	.4byte	.LASF45
	.byte	0x1
	.byte	0x8c
	.4byte	0xad
	.4byte	.LFB186
	.4byte	.LFE186-.LFB186
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x2f6
	.uleb128 0x19
	.ascii	"id\000"
	.byte	0x1
	.byte	0x8c
	.4byte	0x156
	.4byte	.LLST0
	.uleb128 0x19
	.ascii	"key\000"
	.byte	0x1
	.byte	0x8c
	.4byte	0x130
	.4byte	.LLST1
	.uleb128 0x16
	.ascii	"i\000"
	.byte	0x1
	.byte	0x8e
	.4byte	0xad
	.uleb128 0x1a
	.4byte	.LASF42
	.byte	0x1
	.byte	0x8e
	.4byte	0xad
	.4byte	.LLST2
	.uleb128 0x1a
	.4byte	.LASF43
	.byte	0x1
	.byte	0x8f
	.4byte	0xad
	.4byte	.LLST3
	.uleb128 0x1a
	.4byte	.LASF44
	.byte	0x1
	.byte	0x90
	.4byte	0x2f6
	.4byte	.LLST4
	.byte	0
	.uleb128 0x3
	.byte	0x4
	.4byte	0xa3
	.uleb128 0x18
	.4byte	.LASF46
	.byte	0x1
	.byte	0xc8
	.4byte	0xad
	.4byte	.LFB187
	.4byte	.LFE187-.LFB187
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x38e
	.uleb128 0x19
	.ascii	"ops\000"
	.byte	0x1
	.byte	0xc8
	.4byte	0x10a
	.4byte	.LLST5
	.uleb128 0x19
	.ascii	"src\000"
	.byte	0x1
	.byte	0xc8
	.4byte	0x2f6
	.4byte	.LLST6
	.uleb128 0x19
	.ascii	"dst\000"
	.byte	0x1
	.byte	0xc8
	.4byte	0x2f6
	.4byte	.LLST7
	.uleb128 0x1b
	.4byte	.LASF33
	.byte	0x1
	.byte	0xc8
	.4byte	0xad
	.4byte	.LLST8
	.uleb128 0x16
	.ascii	"i\000"
	.byte	0x1
	.byte	0xca
	.4byte	0xad
	.uleb128 0x16
	.ascii	"ds\000"
	.byte	0x1
	.byte	0xcb
	.4byte	0x38e
	.uleb128 0x16
	.ascii	"dt\000"
	.byte	0x1
	.byte	0xcb
	.4byte	0x38e
	.uleb128 0x16
	.ascii	"vt\000"
	.byte	0x1
	.byte	0xcb
	.4byte	0x38e
	.uleb128 0x1c
	.4byte	.LASF49
	.byte	0x1
	.2byte	0x102
	.4byte	.L24
	.uleb128 0x1d
	.4byte	.LVL25
	.4byte	0x50e
	.byte	0
	.uleb128 0x3
	.byte	0x4
	.4byte	0xad
	.uleb128 0x1e
	.4byte	0x20a
	.4byte	.LFB188
	.4byte	.LFE188-.LFB188
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x3ad
	.uleb128 0x1f
	.4byte	0x21b
	.byte	0
	.uleb128 0x20
	.4byte	.LASF47
	.byte	0x1
	.2byte	0x111
	.4byte	0xad
	.4byte	.LFB189
	.4byte	.LFE189-.LFB189
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x499
	.uleb128 0x21
	.4byte	.LASF48
	.byte	0x1
	.2byte	0x111
	.4byte	0x499
	.4byte	.LLST9
	.uleb128 0x22
	.ascii	"i\000"
	.byte	0x1
	.2byte	0x113
	.4byte	0xad
	.4byte	.LLST10
	.uleb128 0x23
	.4byte	.LASF32
	.byte	0x1
	.2byte	0x114
	.4byte	0x38e
	.uleb128 0x22
	.ascii	"ret\000"
	.byte	0x1
	.2byte	0x115
	.4byte	0xad
	.4byte	.LLST11
	.uleb128 0x1c
	.4byte	.LASF49
	.byte	0x1
	.2byte	0x14f
	.4byte	.L45
	.uleb128 0x24
	.4byte	0x20a
	.4byte	.LBB16
	.4byte	.Ldebug_ranges0+0
	.byte	0x1
	.2byte	0x117
	.4byte	0x42d
	.uleb128 0x25
	.4byte	.Ldebug_ranges0+0
	.uleb128 0x1f
	.4byte	0x21b
	.byte	0
	.byte	0
	.uleb128 0x26
	.4byte	.Ldebug_ranges0+0x20
	.4byte	0x458
	.uleb128 0x12
	.ascii	"__p\000"
	.byte	0x1
	.2byte	0x124
	.4byte	0xa1
	.uleb128 0x12
	.ascii	"__n\000"
	.byte	0x1
	.2byte	0x124
	.4byte	0x96
	.uleb128 0x1d
	.4byte	.LVL28
	.4byte	0x524
	.byte	0
	.uleb128 0x24
	.4byte	0x24a
	.4byte	.LBB25
	.4byte	.Ldebug_ranges0+0x40
	.byte	0x1
	.2byte	0x136
	.4byte	0x47d
	.uleb128 0x27
	.4byte	0x25a
	.uleb128 0x25
	.4byte	.Ldebug_ranges0+0x40
	.uleb128 0x1f
	.4byte	0x265
	.byte	0
	.byte	0
	.uleb128 0x1d
	.4byte	.LVL31
	.4byte	0x289
	.uleb128 0x1d
	.4byte	.LVL34
	.4byte	0x2fc
	.uleb128 0x1d
	.4byte	.LVL37
	.4byte	0x289
	.byte	0
	.uleb128 0x3
	.byte	0x4
	.4byte	0x1c2
	.uleb128 0x28
	.4byte	.LASF50
	.byte	0x1
	.byte	0x48
	.4byte	0x1cd
	.uleb128 0x5
	.byte	0x3
	.4byte	hacc_ctx
	.uleb128 0x29
	.4byte	.LASF51
	.byte	0x6
	.byte	0xc
	.4byte	0x45
	.uleb128 0xd
	.4byte	0x68
	.4byte	0x4c6
	.uleb128 0x2a
	.byte	0
	.uleb128 0x29
	.4byte	.LASF52
	.byte	0x7
	.byte	0x24
	.4byte	0x4bb
	.uleb128 0xd
	.4byte	0x3e
	.4byte	0x4dc
	.uleb128 0x2a
	.byte	0
	.uleb128 0x2b
	.4byte	.LASF53
	.byte	0x8
	.2byte	0x1b2
	.4byte	0x4e8
	.uleb128 0x4
	.4byte	0x4d1
	.uleb128 0x29
	.4byte	.LASF54
	.byte	0x5
	.byte	0x36
	.4byte	0x45
	.uleb128 0x29
	.4byte	.LASF55
	.byte	0x1
	.byte	0x42
	.4byte	0xca
	.uleb128 0x29
	.4byte	.LASF56
	.byte	0x1
	.byte	0x43
	.4byte	0xca
	.uleb128 0x2c
	.4byte	.LASF58
	.byte	0x7
	.byte	0x7e
	.4byte	0x68
	.4byte	0x524
	.uleb128 0x2d
	.4byte	0x33
	.uleb128 0x2e
	.byte	0
	.uleb128 0x2f
	.4byte	.LASF63
	.byte	0x9
	.byte	0x1b
	.uleb128 0x2d
	.4byte	0xa1
	.uleb128 0x2d
	.4byte	0x84
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
	.uleb128 0x7
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x8
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
	.uleb128 0x9
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
	.uleb128 0xa
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xd
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
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
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
	.uleb128 0x10
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
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
	.uleb128 0x11
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
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
	.uleb128 0x12
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x13
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
	.uleb128 0x14
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
	.uleb128 0x15
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
	.uleb128 0x16
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
	.uleb128 0x17
	.uleb128 0xb
	.byte	0x1
	.byte	0
	.byte	0
	.uleb128 0x18
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
	.uleb128 0x19
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
	.uleb128 0x1a
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
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x1b
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
	.uleb128 0x1c
	.uleb128 0xa
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x11
	.uleb128 0x1
	.byte	0
	.byte	0
	.uleb128 0x1d
	.uleb128 0x4109
	.byte	0
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1e
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x31
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
	.uleb128 0x1f
	.uleb128 0x34
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x20
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
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
	.uleb128 0x21
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x22
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
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
	.byte	0
	.byte	0
	.uleb128 0x24
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x52
	.uleb128 0x1
	.uleb128 0x55
	.uleb128 0x17
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x25
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x55
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x26
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x55
	.uleb128 0x17
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x27
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x28
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
	.uleb128 0x29
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
	.uleb128 0x2a
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x2b
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
	.uleb128 0x2c
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
	.uleb128 0x2d
	.uleb128 0x5
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x2e
	.uleb128 0x18
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x2f
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
	.4byte	.LVL3-.Ltext0
	.4byte	.LVL6-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL12-.Ltext0
	.4byte	.LVL15-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST1:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL5-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	.LVL12-.Ltext0
	.4byte	.LVL14-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	0
	.4byte	0
.LLST2:
	.4byte	.LVL1-.Ltext0
	.4byte	.LVL3-.Ltext0
	.2byte	0x1
	.byte	0x56
	.4byte	.LVL4-.Ltext0
	.4byte	.LVL9-.Ltext0
	.2byte	0x1
	.byte	0x56
	.4byte	.LVL13-.Ltext0
	.4byte	.LFE186-.Ltext0
	.2byte	0x1
	.byte	0x56
	.4byte	0
	.4byte	0
.LLST3:
	.4byte	.LVL7-.Ltext0
	.4byte	.LVL8-.Ltext0
	.2byte	0x1
	.byte	0x54
	.4byte	.LVL10-.Ltext0
	.4byte	.LVL12-.Ltext0
	.2byte	0x1
	.byte	0x54
	.4byte	0
	.4byte	0
.LLST4:
	.4byte	.LVL6-.Ltext0
	.4byte	.LVL11-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST5:
	.4byte	.LVL16-.Ltext0
	.4byte	.LVL19-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL20-.Ltext0
	.4byte	.LVL22-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST6:
	.4byte	.LVL16-.Ltext0
	.4byte	.LVL24-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	0
	.4byte	0
.LLST7:
	.4byte	.LVL16-.Ltext0
	.4byte	.LVL21-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
.LLST8:
	.4byte	.LVL16-.Ltext0
	.4byte	.LVL17-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	.LVL18-.Ltext0
	.4byte	.LVL23-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	0
	.4byte	0
.LLST9:
	.4byte	.LVL26-.Ltext0
	.4byte	.LVL27-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL27-.Ltext0
	.4byte	.LFE189-.Ltext0
	.2byte	0x1
	.byte	0x54
	.4byte	0
	.4byte	0
.LLST10:
	.4byte	.LVL29-.Ltext0
	.4byte	.LVL30-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	0
	.4byte	0
.LLST11:
	.4byte	.LVL32-.Ltext0
	.4byte	.LVL33-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL35-.Ltext0
	.4byte	.LVL36-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL38-.Ltext0
	.4byte	.LVL39-.Ltext0
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
	.section	.debug_ranges,"",%progbits
.Ldebug_ranges0:
	.4byte	.LBB16-.Ltext0
	.4byte	.LBE16-.Ltext0
	.4byte	.LBB21-.Ltext0
	.4byte	.LBE21-.Ltext0
	.4byte	.LBB23-.Ltext0
	.4byte	.LBE23-.Ltext0
	.4byte	0
	.4byte	0
	.4byte	.LBB20-.Ltext0
	.4byte	.LBE20-.Ltext0
	.4byte	.LBB22-.Ltext0
	.4byte	.LBE22-.Ltext0
	.4byte	.LBB24-.Ltext0
	.4byte	.LBE24-.Ltext0
	.4byte	0
	.4byte	0
	.4byte	.LBB25-.Ltext0
	.4byte	.LBE25-.Ltext0
	.4byte	.LBB28-.Ltext0
	.4byte	.LBE28-.Ltext0
	.4byte	0
	.4byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF45:
	.ascii	"hacc_set_key\000"
.LASF16:
	.ascii	"BOOL\000"
.LASF6:
	.ascii	"short int\000"
.LASF13:
	.ascii	"size_t\000"
.LASF1:
	.ascii	"sizetype\000"
.LASF56:
	.ascii	"bHACC_SWKeyInit\000"
.LASF39:
	.ascii	"hacc_set_cfg\000"
.LASF27:
	.ascii	"AES_SW_KEY\000"
.LASF26:
	.ascii	"AES_KEY\000"
.LASF21:
	.ascii	"AES_ENC\000"
.LASF63:
	.ascii	"__memzero\000"
.LASF14:
	.ascii	"UINT32\000"
.LASF41:
	.ascii	"mode\000"
.LASF61:
	.ascii	"/proj/mtk02464/Public/P4Android4/ALPS_SW/TRUNK/L0/a"
	.ascii	"lps/out/target/product/evb6735/obj/KERNEL_OBJ\000"
.LASF38:
	.ascii	"hw_key\000"
.LASF32:
	.ascii	"config\000"
.LASF52:
	.ascii	"console_printk\000"
.LASF51:
	.ascii	"elf_hwcap\000"
.LASF31:
	.ascii	"AES_CFG\000"
.LASF17:
	.ascii	"AES_ECB_MODE\000"
.LASF60:
	.ascii	"/proj/mtk02464/Public/P4Android4/ALPS_SW/TRUNK/L0/a"
	.ascii	"lps/kernel-3.10/drivers/misc/mediatek/masp/mt6735/m"
	.ascii	"ach/arm/hacc_sk.c\000"
.LASF44:
	.ascii	"tkey\000"
.LASF15:
	.ascii	"float\000"
.LASF8:
	.ascii	"long long int\000"
.LASF25:
	.ascii	"AES_KEY_256\000"
.LASF55:
	.ascii	"bHACC_HWWrapKeyInit\000"
.LASF49:
	.ascii	"_end\000"
.LASF10:
	.ascii	"long int\000"
.LASF57:
	.ascii	"hacc_deinit\000"
.LASF58:
	.ascii	"printk\000"
.LASF30:
	.ascii	"AES_KEY_ID\000"
.LASF42:
	.ascii	"acon\000"
.LASF36:
	.ascii	"blk_sz\000"
.LASF5:
	.ascii	"unsigned char\000"
.LASF47:
	.ascii	"hacc_init\000"
.LASF4:
	.ascii	"signed char\000"
.LASF9:
	.ascii	"long long unsigned int\000"
.LASF18:
	.ascii	"AES_CBC_MODE\000"
.LASF3:
	.ascii	"unsigned int\000"
.LASF0:
	.ascii	"long unsigned int\000"
.LASF54:
	.ascii	"hacc_base\000"
.LASF22:
	.ascii	"AES_OPS\000"
.LASF7:
	.ascii	"short unsigned int\000"
.LASF28:
	.ascii	"AES_HW_KEY\000"
.LASF2:
	.ascii	"char\000"
.LASF34:
	.ascii	"seed\000"
.LASF23:
	.ascii	"AES_KEY_128\000"
.LASF40:
	.ascii	"hacc_set_mode\000"
.LASF24:
	.ascii	"AES_KEY_192\000"
.LASF11:
	.ascii	"_Bool\000"
.LASF50:
	.ascii	"hacc_ctx\000"
.LASF35:
	.ascii	"AES_KEY_SEED\000"
.LASF46:
	.ascii	"hacc_do_aes\000"
.LASF48:
	.ascii	"keyseed\000"
.LASF33:
	.ascii	"size\000"
.LASF29:
	.ascii	"AES_HW_WRAP_KEY\000"
.LASF53:
	.ascii	"hex_asc\000"
.LASF20:
	.ascii	"AES_DEC\000"
.LASF43:
	.ascii	"akey\000"
.LASF12:
	.ascii	"__kernel_size_t\000"
.LASF59:
	.ascii	"GNU C 4.8 -fpreprocessed -mlittle-endian -mapcs -mn"
	.ascii	"o-sched-prolog -mabi=aapcs-linux -mno-thumb-interwo"
	.ascii	"rk -marm -march=armv7-a -mfloat-abi=soft -mfpu=vfp "
	.ascii	"-g -O2 -fno-strict-aliasing -fno-common -fno-delete"
	.ascii	"-null-pointer-checks -fno-pic -fno-dwarf2-cfi-asm -"
	.ascii	"fno-stack-protector -fno-omit-frame-pointer -fno-op"
	.ascii	"timize-sibling-calls -fno-var-tracking-assignments "
	.ascii	"-fno-strict-overflow -fconserve-stack\000"
.LASF19:
	.ascii	"AES_MODE\000"
.LASF37:
	.ascii	"sw_key\000"
.LASF62:
	.ascii	"hacc_context\000"
	.ident	"GCC: (GNU) 4.8"
