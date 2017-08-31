/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _DEV_H
#define _DEV_H

/*----------------------------------------------------------------------------*/
/* MD Direct Tethering only supports some specified network devices,          */
/* which are defined below.                                                   */
/*----------------------------------------------------------------------------*/
extern const char *fastpath_support_dev_names[];
extern const int fastpath_support_dev_num;
bool fastpath_is_support_dev(const char *dev_name);
int fastpath_dev_name_to_id(char *dev_name);
const char *fastpath_id_to_dev_name(int id);
const char *fastpath_data_usage_id_to_dev_name(int id);
bool fastpath_is_lan_dev(char *dev_name);

#endif
