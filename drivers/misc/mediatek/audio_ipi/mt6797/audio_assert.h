#ifndef AUDIO_ASSERT_H
#define AUDIO_ASSERT_H

#ifdef CONFIG_MTK_AEE_FEATURE
#include <mt-plat/aee.h>
#endif

#ifdef CONFIG_MTK_AEE_FEATURE
#define AUD_ASSERT(exp) \
	do { \
		if (!(exp)) { \
			aee_kernel_exception_api(__FILE__, __LINE__, DB_OPT_DEFAULT, \
						 "[Audio]", "ASSERT("#exp") fail!!"); \
		} \
	} while (0)
#else
#define AUD_ASSERT(exp) \
	do { \
		if (!(exp)) { \
			pr_err("ASSERT("#exp") fail: \""  __FILE__ "\", %uL\n", __LINE__); \
		} \
	} while (0)
#endif

#endif /* end of AUDIO_ASSERT_H */

