#ifndef __MTK_MALI_CONFIG_H__
#define __MTK_MALI_CONFIG_H__

extern unsigned int _mtk_mali_ged_log;
#ifdef MTK_MALI_USE_GED_LOG

/* MTK logs */
#include <ged_log.h>
#define _MTK_MALI_PRINT(...) \
	do { ged_log_buf_print2(_mtk_mali_ged_log, GED_LOG_ATTR_TIME, __VA_ARGS__); } while (0)
#define MTK_err(...) \
	_MTK_MALI_PRINT(__VA_ARGS__)
#define dev_MTK_err(dev, ...) \
	do { _MTK_MALI_PRINT(__VA_ARGS__); dev_err(dev, __VA_ARGS__); } while (0)
#define dev_MTK_info(dev, ...) \
	do { _MTK_MALI_PRINT(__VA_ARGS__); dev_info(dev, __VA_ARGS__); } while (0)
#define pr_MTK_err( ...) \
	do { _MTK_MALI_PRINT(__VA_ARGS__); pr_err(__VA_ARGS__); } while (0)
#define pr_MTK_info( ...) \
	do { _MTK_MALI_PRINT(__VA_ARGS__); pr_info(__VA_ARGS__); } while (0)

#else

#define MTK_err(...)
#define dev_MTK_err dev_err
#define pr_MTK_err pr_err

#endif

#endif
