#ifndef __AAL_CONTROL_H__
#define __AAL_CONTROL_H__

#define AAL_TAG                  "[ALS/AAL]"
#define AAL_LOG(fmt, args...)	 pr_debug(AAL_TAG fmt, ##args)
#define AAL_ERR(fmt, args...)    pr_err(AAL_TAG fmt, ##args)
extern int aal_use;
#endif

