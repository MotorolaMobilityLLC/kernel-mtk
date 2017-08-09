
	.text

	.global __switch_to_smp
	.global __switch_to_amp

.equ ACTLR_SMP_BIT ,0x00000040

__switch_to_smp:
	MRRC	p15,1,r0,r1,c15
	ORR	r0,r0,#ACTLR_SMP_BIT
	MCRR	p15,1,r0,r1,c15
	BX	lr

__switch_to_amp:
	MRRC	p15,1,r0,r1,c15
	BIC	r0,r0,#ACTLR_SMP_BIT
	MCRR	p15,1,r0,r1,c15
	BX	lr
