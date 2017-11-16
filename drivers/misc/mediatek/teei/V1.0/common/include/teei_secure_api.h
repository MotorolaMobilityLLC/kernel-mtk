#ifndef _TEEI_KERN_API_
#define _TEEI_KERN_API_

#if defined(__GNUC__) && \
	defined(__GNUC_MINOR__) && \
	defined(__GNUC_PATCHLEVEL__) && \
	((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)) \
	>= 40502
#define ARCH_EXTENSION_SEC
#endif

#define TEEI_FC_CPU_ON			(0xb4000080)
#define TEEI_FC_CPU_OFF			(0xb4000081)
#define TEEI_FC_CPU_DORMANT		(0xb4000082)
#define TEEI_FC_CPU_DORMANT_CANCEL	(0xb4000083)
#define TEEI_FC_CPU_ERRATA_802022	(0xb4000084)

static inline uint32_t teei_secure_call(uint32_t cmd, uint32_t param0, uint32_t param1, uint32_t param2)
{
	/* SMC expect values in r0-r3 */
	register u32 reg0 __asm__("r0") = cmd;
	register u32 reg1 __asm__("r1") = param0;
	register u32 reg2 __asm__("r2") = param1;
	register u32 reg3 __asm__("r3") = param2;
	int ret = 0;

	__asm__ volatile (
#ifdef ARCH_EXTENSION_SEC
		/* This pseudo op is supported and required from
		 * binutils 2.21 on */
		".arch_extension sec\n"
#endif
		"smc 0\n"
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
	);

	/* set response */
	ret = reg0;
	return ret;
}

#endif /* _TEEI_KERN_API_ */
