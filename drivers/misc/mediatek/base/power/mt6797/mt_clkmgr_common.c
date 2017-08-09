#include <linux/module.h>

#if 0
/*use ccf instead of these two functions, will delete this file after all owner already use CCF*/
int enable_clock(int id, char *name)
{
	return 0;
}
EXPORT_SYMBOL(enable_clock);


int disable_clock(int id, char *name)
{
	return 0;
}
EXPORT_SYMBOL(disable_clock);
#endif
