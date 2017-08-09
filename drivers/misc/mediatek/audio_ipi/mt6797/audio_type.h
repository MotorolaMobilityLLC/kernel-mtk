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

#ifndef AUDIO_TYPE_H
#define AUDIO_TYPE_H

#include "audio_log.h"
#include "audio_assert.h"

typedef enum {
	NO_ERROR,
	UNKNOWN_ERROR,

	NO_MEMORY,
	NO_INIT,
	NOT_ENOUGH_DATA,

	ALREADY_EXISTS,

	BAD_INDEX,
	BAD_VALUE,
	BAD_TYPE,

	TIMED_OUT,
} audio_status_t;



#endif /* end of AUDIO_TYPE_H */
