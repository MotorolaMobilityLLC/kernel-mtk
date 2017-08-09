#include <charging.h>
/* Extern two API functions from battery driver to limit max charging current. */
/**
 *  return value means charging current in mA
 *  -1 means error
 *  Implementation in mt_battery.c and mt_battery_fan5405.c
 */
extern int get_bat_charging_current_level(void);

/**
 *  current_limit means limit of charging current in mA
 *  -1 means no limit
 *  Implementation in mt_battery.c and mt_battery_fan5405.c
 */
extern int set_bat_charging_current_limit(int current_limit);
extern CHARGER_TYPE mt_get_charger_type(void);

extern int read_tbat_value(void);

/*	TODO: temp extern functions from switching charger, use header files and correct kconfig
*/
#ifdef CONFIG_MTK_BQ25896_SUPPORT
#include "mtk_thermal_typedefs.h"
extern kal_uint32 set_chr_input_current_limit(int current_limit);
#endif

