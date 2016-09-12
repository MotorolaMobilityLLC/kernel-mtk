#ifndef __RRC_DEF_H__
#define __RRC_DEF_H__


#include <linux/kernel.h>
#include <primary_display.h>



typedef enum {
	RRC_STATE_NORMAL = 0,
	RRC_STATE_VIDEO ,
	RRC_STATE_VIDEO_120Hz,
	RRC_STATE_HIGH_120Hz ,
	RRC_STATE_HIGH_60Hz


} RRC_DRV_STATE;


extern int primary_display_set_lcm_refresh_rate(int fps);
extern int primary_display_get_lcm_refresh_rate(void);
extern int primary_display_get_hwc_refresh_rate(void);




#define RRC_LOG_ERROR   /* error */
#ifdef RRC_LOG_ERROR
#define RRC_ERR(...) pr_err(__VA_ARGS__)
#else
#define RRC_ERR(...)
#endif

#define RRC_LOG_WARNING /* warning */
#ifdef RRC_LOG_WARNING
#define RRC_WRN(...) pr_warn(__VA_ARGS__)
#else
#define RRC_WRN(...)
#endif


#define RRC_LOG_DEBUG   /* debug information */
#ifdef RRC_LOG_DEBUG
#define RRC_DBG(...) pr_debug(__VA_ARGS__)
#else
#define RRC_DBG(...)
#endif

#define RRC_LOG_INFO   /* info information */
#ifdef RRC_LOG_INFO
#define RRC_INFO(...) pr_debug(__VA_ARGS__)
#else
#define RRC_INFO(...)
#endif



#endif
