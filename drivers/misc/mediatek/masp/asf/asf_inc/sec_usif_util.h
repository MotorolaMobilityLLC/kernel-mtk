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

#ifndef USIF_UTILS_H
#define USIF_UTILS_H

/**************************************************************************
 *  EXTERNAL FUNCTIONS
 *************************************************************************/
extern char *usif2pl(char *part_name);
extern char *pl2usif(char *part_name);
extern bool sec_usif_enabled(void);
extern void sec_usif_part_path(unsigned int part_num, char *part_path, unsigned int part_path_len);

#endif				/* USIF_UTILS_H */
