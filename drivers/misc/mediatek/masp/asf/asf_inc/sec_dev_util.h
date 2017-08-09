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

#ifndef DEV_UTILS_H
#define DEV_UTILS_H

/**************************************************************************
 *  EXTERNAL FUNCTIONS
 *************************************************************************/
extern char *part2pl(char *part_name);
extern char *pl2part(char *part_name);
extern void sec_dev_part_name(unsigned int part_num, char *part_name);
extern char *get_android_name(void);
extern char *get_secro_name(void);

#endif				/* DEV_UTILS_INTER_H */
