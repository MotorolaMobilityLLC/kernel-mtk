/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/*
  boot logger: drivers/misc/mtprof/bootprof
  interface: /proc/bootprof
*/
#ifdef CONFIG_SCHEDSTATS
extern void log_boot(char *str);
#else
static inline void log_boot(char *str)
{
}
#endif
