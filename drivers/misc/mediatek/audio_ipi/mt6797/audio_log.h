#ifndef AUDIO_LOG_H
#define AUDIO_LOG_H

#include <linux/printk.h>


/*#define FORCE_ENABLE_AUDIO_LOG*/

#ifndef FORCE_ENABLE_AUDIO_LOG /* use pr_* to print audio log */

#ifndef AUD_LOG_V
#if 0
#define AUD_LOG_V(x...) pr_debug(x)
#else
#define AUD_LOG_V(x...)
#endif
#endif

#ifndef AUD_LOG_D
#define AUD_LOG_D pr_debug
#endif

#ifndef AUD_LOG_I
#define AUD_LOG_I pr_info
#endif

#ifndef AUD_LOG_W
#define AUD_LOG_W pr_warning
#endif

#ifndef AUD_LOG_E
#define AUD_LOG_E pr_err
#endif

#else /* force dump audio log (local debug only) */

#ifndef AUD_LOG_V
#if 0
#define AUD_LOG_V(x...) pr_emerg(x)
#else
#define AUD_LOG_V(x...)
#endif
#endif


#ifndef AUD_LOG_D
#define AUD_LOG_D pr_emerg
#endif

#ifndef AUD_LOG_I
#define AUD_LOG_I pr_emerg
#endif

#ifndef AUD_LOG_W
#define AUD_LOG_W pr_emerg
#endif

#ifndef AUD_LOG_E
#define AUD_LOG_E pr_emerg
#endif

#endif

#endif /* end of AUDIO_LOG_H */

