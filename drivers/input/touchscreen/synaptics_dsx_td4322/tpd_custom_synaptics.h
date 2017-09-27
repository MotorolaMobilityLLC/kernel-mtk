#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_POWER_SOURCE	MT6331_POWER_LDO_VGP1         
#define TPD_I2C_BUS			4
#define TPD_I2C_ADDR		0x20
#define TPD_WAKEUP_TRIAL	60
#define TPD_WAKEUP_DELAY	100


//#define TPD_HAVE_TREMBLE_ELIMINATION

/* Define the virtual button mapping */
//#define TPD_HAVE_BUTTON
#define TPD_BUTTON_HEIGH        (100)
#define TPD_KEY_COUNT           3
#define TPD_KEYS                {KEY_APPSELECT , KEY_HOMEPAGE, KEY_BACK}
#define TPD_KEYS_DIM            {{80,850,160,TPD_BUTTON_HEIGH},{240,850,160,TPD_BUTTON_HEIGH},{400,850,160,TPD_BUTTON_HEIGH}}

/* Define the touch dimension */
#ifdef TPD_HAVE_BUTTON
#define TPD_TOUCH_HEIGH_RATIO	80
#define TPD_DISPLAY_HEIGH_RATIO	73
#endif

/* Define the 0D button mapping */
#ifdef TPD_HAVE_BUTTON
#define TPD_0D_BUTTON		{}
#else
#define TPD_0D_BUTTON		{KEY_MENU, KEY_BACK, KEY_HOMEPAGE}
#endif

#endif /* TOUCHPANEL_H__ */

