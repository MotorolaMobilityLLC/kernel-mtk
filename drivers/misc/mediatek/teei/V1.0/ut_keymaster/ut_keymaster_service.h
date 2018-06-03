/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __UT_KEYMASTER_SERVICE_H__
#define __UT_KEYMASTER_SERVICE_H__

extern unsigned long ut_keymaster_message_buf;
extern unsigned long ut_keymaster_fmessage_buf;
extern unsigned long ut_keymaster_bmessage_buf;

long create_keymaster_cmd_buf(void);
#endif
