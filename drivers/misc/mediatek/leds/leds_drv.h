#include <linux/leds.h>
#include <leds_hal.h>

/****************************************************************************
 * LED DRV functions
 ***************************************************************************/

#ifdef CONTROL_BL_TEMPERATURE
int setMaxbrightness(int max_level, int enable);
#endif

extern int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level);
extern int backlight_brightness_set(int level);
extern int disp_bls_set_max_backlight(unsigned int level);
